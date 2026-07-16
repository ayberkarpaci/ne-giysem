#pragma once

#include <map>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace negiysem {

class Database;

struct RecommendationRequest {
    double temperature_c = 15.0;
    bool is_raining = false;
    std::string mood_slug;   // a row in the moods table, or empty for no mood bias
    // A blend of moods with weights (e.g. from Gemini's analysis of a
    // free-text answer). When non-empty this takes precedence over
    // mood_slug; {mood_slug, 1.0} and a plain mood_slug score identically.
    std::vector<std::pair<std::string, double>> mood_weights;
    std::string lang = "en"; // BCP 47 code used to look up display names
    // Color/pattern value slugs; only wardrobe items carry attributes, so
    // these only influence recommendFromWardrobe.
    std::vector<std::string> colors_preferred;
    std::vector<std::string> colors_avoided;
    std::vector<std::string> patterns_preferred;
    std::vector<std::string> patterns_avoided;
    // Item type slugs to leave out entirely, e.g. when the user disliked a
    // suggestion and asked for another one.
    std::vector<std::string> exclude_items;
    // 'male' or 'female' narrows catalog suggestions to unisex + that
    // gender; empty shows everything. The user's own wardrobe is never
    // filtered — they own those clothes.
    std::string gender;
    // Target formality 0 (sporty) .. 5 (formal), usually derived from the
    // parsed occasion; -1 means no occasion, so formality is not scored.
    int formality_target = -1;
};

struct RecommendedItem {
    std::string item_slug;
    std::string item_name;      // localized, falls back to slug
    std::string category_slug;
    std::string category_name;  // localized, falls back to slug
    double score = 0.0;
    // Set only when the item comes from the user's wardrobe.
    int wardrobe_id = 0;
    std::string photo_path;
    std::string cutout_path;  // transparent PNG under data/photos/cutouts/
    // Attribute value slugs (colors, pattern, fit, ...) of a wardrobe item,
    // so a stylist pass can judge how well pieces go together.
    std::vector<std::string> value_slugs;
    int formality = 2;  // 0 (sporty) .. 5 (formal), from the catalog type
};

// Scored items per category slug, each list sorted best first.
using OutfitCandidates = std::map<std::string, std::vector<RecommendedItem>>;

// How often two item types appeared together in well-rated outfits. Keys
// are slug pairs with the smaller slug first (see FeedbackRepository::
// pairAffinities); values are co-occurrence counts.
using PairAffinities = std::map<std::pair<std::string, std::string>, int>;

// Scoring building blocks, exposed for unit testing.
//
// 1.0 when the temperature is inside the item's comfort range, decaying by
// 0.15 per degree outside it; 0.5 (neutral) when the item has no range.
double temperatureFit(double temp_c,
                      std::optional<double> min_c,
                      std::optional<double> max_c);

// Bonus/penalty depending on rain and the item being waterproof. Waterproof
// gear is boosted in rain and slightly penalized in dry weather.
double rainAdjustment(bool is_raining, bool is_waterproof);

// +0.3 if any of the item's values (colors, patterns, ...) is preferred,
// -0.5 if any is avoided.
double preferenceAdjustment(const std::vector<std::string>& item_values,
                            const std::vector<std::string>& preferred,
                            const std::vector<std::string>& avoided);

// How past user ratings shift an item's score: a 1..5 average maps linearly
// to -0.3..+0.3 around the neutral rating of 3.
double feedbackAdjustment(double average_rating);

// Penalty for missing the occasion's formality: 0 without a target (< 0),
// otherwise -0.15 per level of distance from it.
double formalityAdjustment(int item_formality, int target);

// Combination-level harmony terms, each <= 0, exposed for unit testing.
// Colors/patterns come from wardrobe value_slugs; catalog items score 0.
double colorHarmony(const std::vector<RecommendedItem>& outfit);
double formalityConsistency(const std::vector<RecommendedItem>& outfit);
double patternClashPenalty(const std::vector<RecommendedItem>& outfit);

// Outfit memory (>= 0): pieces that shone together in past well-rated
// outfits earn a bonus when combined again — 0.05 per co-occurrence,
// capped at 0.15 per pair.
double pairAffinityBonus(const std::vector<RecommendedItem>& outfit,
                         const PairAffinities& affinities);

// Real-life variety (wardrobe only): a piece worn `days` ago is penalized
// -0.3/days — yesterday hurts, last week barely registers. A piece that
// was never worn (nullopt) gets a small +0.05 exploration bonus instead.
double varietyAdjustment(std::optional<double> days_since_worn);

// The learned style profile applied to one item: 0.1 per point of summed
// value weights (see FeedbackRepository::stylePreferences), clamped to
// [-0.3, 0.3] so taste seasons the ranking without ruling it.
double styleAdjustment(const std::vector<std::string>& value_slugs,
                       const std::map<std::string, double>& preferences);

// Assembles the best outfit from per-category candidates by scoring whole
// combinations (mean of core item scores + harmony terms + pair-affinity
// bonus) instead of picking each category independently. Core pieces are
// top+bottom or a one-piece, plus footwear; optional categories join when
// their score plus the harmony change clears `optional_threshold`.
// With an `rng`, the pick falls uniformly among combinations within
// `tie_tolerance` of the best score, so near-tied outfits take turns;
// without one the best combination always wins (deterministic).
std::vector<RecommendedItem> assembleOutfit(const OutfitCandidates& candidates,
                                            double optional_threshold,
                                            const PairAffinities& affinities = {},
                                            std::mt19937* rng = nullptr,
                                            double tie_tolerance = 0.05);

// Picks the best-scoring item per category. Core categories (top, bottom,
// footwear) are always present; optional ones (outerwear, accessory) only
// when their best item scores at least kOptionalCategoryThreshold.
class Recommender {
public:
    static constexpr double kOptionalCategoryThreshold = 0.8;

    explicit Recommender(Database& db) : db_(db) {}

    // Throws std::runtime_error if the mood slug is unknown.
    std::vector<RecommendedItem> recommend(const RecommendationRequest& request) const;

    // Same scoring, but over the user's own garments. Categories the user
    // owns nothing in are simply absent from the result. Item names prefer
    // the user's label, falling back to the localized type name.
    std::vector<RecommendedItem> recommendFromWardrobe(
        const RecommendationRequest& request) const;

    // The top `limit` scored items per category, best first, so a stylist
    // pass can choose the most coherent combination.
    OutfitCandidates candidates(const RecommendationRequest& request, int limit) const;
    OutfitCandidates candidatesFromWardrobe(const RecommendationRequest& request,
                                            int limit) const;

private:
    Database& db_;
};

}  // namespace negiysem
