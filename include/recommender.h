#pragma once

#include <map>
#include <optional>
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
    // Attribute value slugs (colors, pattern, fit, ...) of a wardrobe item,
    // so a stylist pass can judge how well pieces go together.
    std::vector<std::string> value_slugs;
    int formality = 2;  // 0 (sporty) .. 5 (formal), from the catalog type
};

// Scored items per category slug, each list sorted best first.
using OutfitCandidates = std::map<std::string, std::vector<RecommendedItem>>;

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
