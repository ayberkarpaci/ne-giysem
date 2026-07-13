#pragma once

#include <optional>
#include <string>
#include <vector>

namespace negiysem {

class Database;

struct RecommendationRequest {
    double temperature_c = 15.0;
    bool is_raining = false;
    std::string mood_slug;   // must match a row in the moods table
    std::string lang = "en"; // BCP 47 code used to look up display names
};

struct RecommendedItem {
    std::string item_slug;
    std::string item_name;      // localized, falls back to slug
    std::string category_slug;
    std::string category_name;  // localized, falls back to slug
    double score = 0.0;
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

// Picks the best-scoring item per category. Core categories (top, bottom,
// footwear) are always present; optional ones (outerwear, accessory) only
// when their best item scores at least kOptionalCategoryThreshold.
class Recommender {
public:
    static constexpr double kOptionalCategoryThreshold = 0.8;

    explicit Recommender(Database& db) : db_(db) {}

    // Throws std::runtime_error if the mood slug is unknown.
    std::vector<RecommendedItem> recommend(const RecommendationRequest& request) const;

private:
    Database& db_;
};

}  // namespace negiysem
