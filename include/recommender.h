#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace negiysem {

class Database;

struct RecommendationRequest {
    double temperature_c = 15.0;
    bool is_raining = false;
    std::string mood_slug;   // must match a row in the moods table
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
};

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

private:
    Database& db_;
};

}  // namespace negiysem
