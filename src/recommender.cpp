#include "recommender.h"

#include <sqlite3.h>

#include <algorithm>
#include <map>
#include <stdexcept>

#include "database.h"

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

constexpr const char* kItemQuery = R"sql(
SELECT i.slug,
       c.slug,
       i.min_temp_c,
       i.max_temp_c,
       i.is_waterproof,
       COALESCE(a.weight, 0.0),
       COALESCE(ti.name, i.slug),
       COALESCE(tc.name, c.slug)
FROM clothing_items i
JOIN clothing_categories c ON c.id = i.category_id
LEFT JOIN moods m ON m.slug = ?1
LEFT JOIN item_mood_affinity a ON a.item_id = i.id AND a.mood_id = m.id
LEFT JOIN translations ti
       ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?2
LEFT JOIN translations tc
       ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?2;
)sql";

}  // namespace

std::vector<RecommendedItem> Recommender::recommend(const RecommendationRequest& request) const {
    sqlite3* db = db_.handle();
    ensureMoodExists(db, request.mood_slug);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kItemQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, request.mood_slug.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, request.lang.c_str(), -1, SQLITE_TRANSIENT);

    // Best-scoring item per category.
    std::map<std::string, RecommendedItem> best;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::optional<double> min_c;
        std::optional<double> max_c;
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            min_c = sqlite3_column_double(stmt, 2);
        }
        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            max_c = sqlite3_column_double(stmt, 3);
        }
        const bool waterproof = sqlite3_column_int(stmt, 4) != 0;
        const double mood_weight = sqlite3_column_double(stmt, 5);

        RecommendedItem item;
        item.item_slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        item.category_slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        item.item_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.category_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.score = scoreItem(temperatureFit(request.temperature_c, min_c, max_c),
                               mood_weight,
                               rainAdjustment(request.is_raining, waterproof));

        auto it = best.find(item.category_slug);
        if (it == best.end() || item.score > it->second.score) {
            best[item.category_slug] = std::move(item);
        }
    }
    sqlite3_finalize(stmt);

    std::vector<RecommendedItem> outfit;
    for (auto& [category, item] : best) {
        if (isCoreCategory(category) || item.score >= kOptionalCategoryThreshold) {
            outfit.push_back(std::move(item));
        }
    }
    // Highest score first, so the strongest picks lead the list.
    std::sort(outfit.begin(), outfit.end(),
              [](const RecommendedItem& a, const RecommendedItem& b) { return a.score > b.score; });
    return outfit;
}

}  // namespace negiysem
