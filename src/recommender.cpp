#include "recommender.h"

#include <sqlite3.h>

#include <algorithm>
#include <map>
#include <stdexcept>

#include "database.h"
#include "feedback.h"

namespace negiysem {

double temperatureFit(double temp_c,
                      std::optional<double> min_c,
                      std::optional<double> max_c) {
    if (!min_c && !max_c) {
        return 0.5;  // no range: temperature says nothing about this item
    }
    double distance = 0.0;
    if (min_c && temp_c < *min_c) {
        distance = *min_c - temp_c;
    } else if (max_c && temp_c > *max_c) {
        distance = temp_c - *max_c;
    }
    return std::max(0.0, 1.0 - 0.15 * distance);
}

double rainAdjustment(bool is_raining, bool is_waterproof) {
    if (is_raining) {
        return is_waterproof ? 0.5 : 0.0;
    }
    // A raincoat or umbrella is an odd pick in dry weather.
    return is_waterproof ? -0.2 : 0.0;
}

double feedbackAdjustment(double average_rating) {
    return 0.3 * (std::clamp(average_rating, 1.0, 5.0) - 3.0) / 2.0;
}

double preferenceAdjustment(const std::vector<std::string>& item_values,
                            const std::vector<std::string>& preferred,
                            const std::vector<std::string>& avoided) {
    double adjustment = 0.0;
    for (const auto& value : item_values) {
        if (std::find(avoided.begin(), avoided.end(), value) != avoided.end()) {
            return -0.5;  // avoided wins outright
        }
        if (std::find(preferred.begin(), preferred.end(), value) != preferred.end()) {
            adjustment = 0.3;
        }
    }
    return adjustment;
}

namespace {

// Score formula: mood affinity scales the temperature fit (an item that is
// wrong for the weather should not win on mood alone), rain adjusts on top.
double scoreItem(double temp_fit, double mood_weight, double rain_adj) {
    return temp_fit * (1.0 + mood_weight) + rain_adj;
}

bool isCoreCategory(const std::string& slug) {
    return slug == "top" || slug == "bottom" || slug == "footwear";
}

void ensureMoodExists(sqlite3* db, const std::string& mood_slug) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM moods WHERE slug = ?1;", -1, &stmt, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, mood_slug.c_str(), -1, SQLITE_TRANSIENT);
    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    if (!found) {
        throw std::runtime_error("unknown mood: '" + mood_slug + "'");
    }
}

// The mood blend used for scoring: the explicit weights when given, the
// single mood otherwise. Every slug is validated against the moods table.
std::vector<std::pair<std::string, double>> resolveMoodWeights(
    sqlite3* db, const RecommendationRequest& request) {
    std::vector<std::pair<std::string, double>> weights = request.mood_weights;
    if (weights.empty()) {
        weights.emplace_back(request.mood_slug, 1.0);
    }
    for (const auto& [slug, _] : weights) {
        ensureMoodExists(db, slug);
    }
    return weights;
}

// mood slug -> affinity weight, per item id.
std::map<int, std::map<std::string, double>> loadAffinities(sqlite3* db) {
    constexpr const char* kAffinityQuery = R"sql(
        SELECT a.item_id, m.slug, a.weight
        FROM item_mood_affinity a
        JOIN moods m ON m.id = a.mood_id;
    )sql";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kAffinityQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    std::map<int, std::map<std::string, double>> affinities;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        affinities[sqlite3_column_int(stmt, 0)]
                  [reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))] =
            sqlite3_column_double(stmt, 2);
    }
    sqlite3_finalize(stmt);
    return affinities;
}

// Weighted sum of the item's affinities over the requested mood blend.
double blendedMoodWeight(const std::map<int, std::map<std::string, double>>& affinities,
                         int item_id,
                         const std::vector<std::pair<std::string, double>>& mood_weights) {
    const auto item_it = affinities.find(item_id);
    if (item_it == affinities.end()) {
        return 0.0;
    }
    double total = 0.0;
    for (const auto& [slug, user_weight] : mood_weights) {
        const auto aff_it = item_it->second.find(slug);
        if (aff_it != item_it->second.end()) {
            total += user_weight * aff_it->second;
        }
    }
    return total;
}

// Both queries share this column layout; the wardrobe query appends
// w.id, w.label and w.photo_path after it.
constexpr const char* kSharedColumns = R"sql(
SELECT i.slug,
       c.slug,
       i.min_temp_c,
       i.max_temp_c,
       i.is_waterproof,
       i.id,
       COALESCE(ti.name, i.slug),
       COALESCE(tc.name, c.slug)
)sql";

const std::string kItemQuery = std::string(kSharedColumns) + R"sql(
FROM clothing_items i
JOIN clothing_categories c ON c.id = i.category_id
LEFT JOIN translations ti
       ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?1
LEFT JOIN translations tc
       ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?1;
)sql";

const std::string kWardrobeQuery = std::string(kSharedColumns) + R"sql(
     , w.id, COALESCE(w.label, ''), COALESCE(w.photo_path, '')
FROM wardrobe_items w
JOIN clothing_items i ON i.id = w.type_id
JOIN clothing_categories c ON c.id = i.category_id
LEFT JOIN translations ti
       ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?1
LEFT JOIN translations tc
       ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?1;
)sql";

// Reads the shared columns and computes the score. Extra columns (if any)
// are the caller's business.
RecommendedItem readScoredItem(sqlite3_stmt* stmt,
                               const RecommendationRequest& request,
                               const std::map<int, std::map<std::string, double>>& affinities,
                               const std::vector<std::pair<std::string, double>>& mood_weights) {
    std::optional<double> min_c;
    std::optional<double> max_c;
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
        min_c = sqlite3_column_double(stmt, 2);
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
        max_c = sqlite3_column_double(stmt, 3);
    }
    const bool waterproof = sqlite3_column_int(stmt, 4) != 0;
    const double mood_weight =
        blendedMoodWeight(affinities, sqlite3_column_int(stmt, 5), mood_weights);

    RecommendedItem item;
    item.item_slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    item.category_slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    item.item_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    item.category_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    item.score = scoreItem(temperatureFit(request.temperature_c, min_c, max_c),
                           mood_weight,
                           rainAdjustment(request.is_raining, waterproof));
    return item;
}

// Applies the per-category selection and ordering rules to scored items.
std::vector<RecommendedItem> pickOutfit(std::map<std::string, RecommendedItem>&& best,
                                        double optional_threshold) {
    std::vector<RecommendedItem> outfit;
    for (auto& [category, item] : best) {
        if (isCoreCategory(category) || item.score >= optional_threshold) {
            outfit.push_back(std::move(item));
        }
    }
    std::sort(outfit.begin(), outfit.end(),
              [](const RecommendedItem& a, const RecommendedItem& b) { return a.score > b.score; });
    return outfit;
}

}  // namespace

namespace {

bool isExcluded(const RecommendationRequest& request, const std::string& item_slug) {
    return std::find(request.exclude_items.begin(), request.exclude_items.end(),
                     item_slug) != request.exclude_items.end();
}

// Adds the learned adjustment from past user ratings, if any.
void applyFeedback(RecommendedItem& item, const std::map<std::string, double>& ratings) {
    const auto it = ratings.find(item.item_slug);
    if (it != ratings.end()) {
        item.score += feedbackAdjustment(it->second);
    }
}

}  // namespace

std::vector<RecommendedItem> Recommender::recommend(const RecommendationRequest& request) const {
    sqlite3* db = db_.handle();
    const auto mood_weights = resolveMoodWeights(db, request);
    const auto affinities = loadAffinities(db);
    const auto ratings = FeedbackRepository(db_).averageRatings();

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kItemQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, request.lang.c_str(), -1, SQLITE_TRANSIENT);

    // Best-scoring item per category.
    std::map<std::string, RecommendedItem> best;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RecommendedItem item = readScoredItem(stmt, request, affinities, mood_weights);
        if (isExcluded(request, item.item_slug)) continue;
        applyFeedback(item, ratings);
        auto it = best.find(item.category_slug);
        if (it == best.end() || item.score > it->second.score) {
            best[item.category_slug] = std::move(item);
        }
    }
    sqlite3_finalize(stmt);

    return pickOutfit(std::move(best), kOptionalCategoryThreshold);
}

std::vector<RecommendedItem> Recommender::recommendFromWardrobe(
    const RecommendationRequest& request) const {
    sqlite3* db = db_.handle();
    const auto mood_weights = resolveMoodWeights(db, request);
    const auto affinities = loadAffinities(db);
    const auto ratings = FeedbackRepository(db_).averageRatings();

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kWardrobeQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, request.lang.c_str(), -1, SQLITE_TRANSIENT);

    // Colors and patterns per wardrobe item, for the preference adjustments.
    std::map<int, std::vector<std::string>> colors;
    std::map<int, std::vector<std::string>> patterns;
    const bool wants_colors =
        !request.colors_preferred.empty() || !request.colors_avoided.empty();
    const bool wants_patterns =
        !request.patterns_preferred.empty() || !request.patterns_avoided.empty();
    if (wants_colors || wants_patterns) {
        sqlite3_stmt* attr_stmt = nullptr;
        constexpr const char* kAttrQuery = R"sql(
            SELECT wa.wardrobe_item_id, a.slug, v.slug
            FROM wardrobe_item_attributes wa
            JOIN attribute_values v ON v.id = wa.attribute_value_id
            JOIN attributes a ON a.id = v.attribute_id
            WHERE a.slug IN ('color', 'pattern');
        )sql";
        if (sqlite3_prepare_v2(db, kAttrQuery, -1, &attr_stmt, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
        }
        while (sqlite3_step(attr_stmt) == SQLITE_ROW) {
            const int wardrobe_id = sqlite3_column_int(attr_stmt, 0);
            const std::string attr = reinterpret_cast<const char*>(sqlite3_column_text(attr_stmt, 1));
            auto& target = attr == "color" ? colors : patterns;
            target[wardrobe_id].emplace_back(
                reinterpret_cast<const char*>(sqlite3_column_text(attr_stmt, 2)));
        }
        sqlite3_finalize(attr_stmt);
    }

    std::map<std::string, RecommendedItem> best;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RecommendedItem item = readScoredItem(stmt, request, affinities, mood_weights);
        if (isExcluded(request, item.item_slug)) continue;
        applyFeedback(item, ratings);
        item.wardrobe_id = sqlite3_column_int(stmt, 8);
        const std::string label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.photo_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        if (!label.empty()) {
            item.item_name = label;
        }
        const auto colors_it = colors.find(item.wardrobe_id);
        if (colors_it != colors.end()) {
            item.score += preferenceAdjustment(colors_it->second, request.colors_preferred,
                                               request.colors_avoided);
        }
        const auto patterns_it = patterns.find(item.wardrobe_id);
        if (patterns_it != patterns.end()) {
            item.score += preferenceAdjustment(patterns_it->second, request.patterns_preferred,
                                               request.patterns_avoided);
        }
        auto it = best.find(item.category_slug);
        if (it == best.end() || item.score > it->second.score) {
            best[item.category_slug] = std::move(item);
        }
    }
    sqlite3_finalize(stmt);

    return pickOutfit(std::move(best), kOptionalCategoryThreshold);
}

}  // namespace negiysem
