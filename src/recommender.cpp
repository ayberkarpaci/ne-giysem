#include "recommender.h"

#include <sqlite3.h>

#include <algorithm>
#include <cstdlib>
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

double formalityAdjustment(int item_formality, int target) {
    if (target < 0) return 0.0;
    return -0.15 * std::abs(item_formality - std::clamp(target, 0, 5));
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

// Color/pattern vocabulary the harmony terms reason over. Neutrals pair
// with everything; statement colors are counted and checked for clashes.
const std::vector<std::string> kNeutralColors = {
    "black", "white", "gray", "charcoal", "navy", "beige", "brown", "cream", "khaki"};
const std::vector<std::string> kStatementColors = {
    "blue", "red", "green", "yellow", "pink", "purple", "orange", "turquoise",
    "olive", "burgundy", "mustard", "teal", "lilac", "mint", "coral", "multicolor"};
const std::vector<std::pair<std::string, std::string>> kClashingColors = {
    {"red", "pink"}, {"red", "orange"}, {"red", "green"},
    {"purple", "green"}, {"orange", "pink"}};
const std::vector<std::string> kBoldPatterns = {
    "striped", "plaid", "floral", "polka-dot", "graphic", "camouflage",
    "gingham", "houndstooth", "herringbone", "paisley", "animal-print",
    "tie-dye", "color-block", "argyle", "embroidered", "sequin",
    "geometric", "abstract"};

bool listed(const std::vector<std::string>& list, const std::string& value) {
    return std::find(list.begin(), list.end(), value) != list.end();
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
// single mood otherwise, and no blend at all when neither is set (the user
// skipped the mood question). Every slug is validated against the moods
// table.
std::vector<std::pair<std::string, double>> resolveMoodWeights(
    sqlite3* db, const RecommendationRequest& request) {
    std::vector<std::pair<std::string, double>> weights = request.mood_weights;
    if (weights.empty() && !request.mood_slug.empty()) {
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
       COALESCE(tc.name, c.slug),
       i.formality
)sql";

const std::string kItemQuery = std::string(kSharedColumns) + R"sql(
FROM clothing_items i
JOIN clothing_categories c ON c.id = i.category_id
LEFT JOIN translations ti
       ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?1
LEFT JOIN translations tc
       ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?1
WHERE (?2 = '' OR i.gender = 'unisex' OR i.gender = ?2);
)sql";

// Appends w.id, label, photo_path, cutout_path as columns 9-12 after the
// shared ones.
const std::string kWardrobeQuery = std::string(kSharedColumns) + R"sql(
     , w.id, COALESCE(w.label, ''), COALESCE(w.photo_path, ''), COALESCE(w.cutout_path, '')
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
    item.formality = sqlite3_column_int(stmt, 8);
    item.score = scoreItem(temperatureFit(request.temperature_c, min_c, max_c),
                           mood_weight,
                           rainAdjustment(request.is_raining, waterproof)) +
                 formalityAdjustment(item.formality, request.formality_target);
    return item;
}

}  // namespace

double colorHarmony(const std::vector<RecommendedItem>& outfit) {
    std::vector<std::string> statement;
    for (const auto& item : outfit) {
        for (const auto& slug : item.value_slugs) {
            if (listed(kStatementColors, slug) && !listed(statement, slug)) {
                statement.push_back(slug);
            }
        }
    }
    // Neutrals are free; more than two statement colors starts to cost.
    double penalty = -0.2 * std::max(0, static_cast<int>(statement.size()) - 2);
    for (const auto& [a, b] : kClashingColors) {
        if (listed(statement, a) && listed(statement, b)) penalty -= 0.3;
    }
    return penalty;
}

double formalityConsistency(const std::vector<RecommendedItem>& outfit) {
    if (outfit.empty()) return 0.0;
    int min_f = 5;
    int max_f = 0;
    for (const auto& item : outfit) {
        min_f = std::min(min_f, item.formality);
        max_f = std::max(max_f, item.formality);
    }
    // A spread of up to two levels reads as intentional; beyond that the
    // outfit mixes dress codes (sweatpants with a blazer).
    return -0.2 * std::max(0, max_f - min_f - 2);
}

double patternClashPenalty(const std::vector<RecommendedItem>& outfit) {
    int bold = 0;
    for (const auto& item : outfit) {
        for (const auto& slug : item.value_slugs) {
            if (listed(kBoldPatterns, slug)) {
                ++bold;
                break;
            }
        }
    }
    return -0.25 * std::max(0, bold - 1);
}

double pairAffinityBonus(const std::vector<RecommendedItem>& outfit,
                         const PairAffinities& affinities) {
    if (affinities.empty()) return 0.0;
    double bonus = 0.0;
    for (size_t i = 0; i < outfit.size(); ++i) {
        for (size_t j = i + 1; j < outfit.size(); ++j) {
            auto key = std::minmax(outfit[i].item_slug, outfit[j].item_slug);
            const auto it = affinities.find({key.first, key.second});
            if (it != affinities.end()) {
                bonus += std::min(0.15, 0.05 * it->second);
            }
        }
    }
    return bonus;
}

namespace {

// Harmony penalties plus the learned pair bonus — everything about how the
// pieces relate, as opposed to how good each piece is on its own.
double combinationTerms(const std::vector<RecommendedItem>& outfit,
                        const PairAffinities& affinities) {
    return colorHarmony(outfit) + formalityConsistency(outfit) +
           patternClashPenalty(outfit) + pairAffinityBonus(outfit, affinities);
}

// Mean item score + combination terms: means keep one-piece combos (one
// garment) comparable with top+bottom combos (two garments).
double comboScore(const std::vector<RecommendedItem>& combo,
                  const PairAffinities& affinities) {
    if (combo.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& item : combo) sum += item.score;
    return sum / static_cast<double>(combo.size()) + combinationTerms(combo, affinities);
}

const std::vector<RecommendedItem> kNone;  // empty candidate list fallback

const std::vector<RecommendedItem>& candidatesFor(const OutfitCandidates& candidates,
                                                  const char* category) {
    const auto it = candidates.find(category);
    return it == candidates.end() ? kNone : it->second;
}

}  // namespace

std::vector<RecommendedItem> assembleOutfit(const OutfitCandidates& candidates,
                                            double optional_threshold,
                                            const PairAffinities& affinities) {
    const auto& tops = candidatesFor(candidates, "top");
    const auto& bottoms = candidatesFor(candidates, "bottom");
    const auto& one_pieces = candidatesFor(candidates, "one-piece");
    const auto& shoes = candidatesFor(candidates, "footwear");

    // Enumerate every core combination: top+bottom pairs (or whichever of
    // the two exists) and one-piece garments, each with every shoe option.
    std::vector<std::vector<RecommendedItem>> bases;
    if (!tops.empty() && !bottoms.empty()) {
        for (const auto& top : tops) {
            for (const auto& bottom : bottoms) bases.push_back({top, bottom});
        }
    } else {
        for (const auto& top : tops) bases.push_back({top});
        for (const auto& bottom : bottoms) bases.push_back({bottom});
    }
    for (const auto& piece : one_pieces) bases.push_back({piece});
    if (bases.empty()) bases.push_back({});

    std::vector<RecommendedItem> best;
    double best_score = -1e9;
    for (const auto& base : bases) {
        if (shoes.empty()) {
            if (comboScore(base, affinities) > best_score && !base.empty()) {
                best_score = comboScore(base, affinities);
                best = base;
            }
            continue;
        }
        for (const auto& shoe : shoes) {
            std::vector<RecommendedItem> combo = base;
            combo.push_back(shoe);
            const double score = comboScore(combo, affinities);
            if (score > best_score) {
                best_score = score;
                best = std::move(combo);
            }
        }
    }

    // Optional layers join only when they earn their place: their own score
    // plus the harmony/affinity change must clear the threshold.
    for (const char* category : {"outerwear", "accessory", "jewelry"}) {
        const auto& options = candidatesFor(candidates, category);
        if (options.empty()) continue;
        const RecommendedItem& option = options.front();
        std::vector<RecommendedItem> extended = best;
        extended.push_back(option);
        const double delta = combinationTerms(extended, affinities) -
                             combinationTerms(best, affinities);
        if (option.score + delta >= optional_threshold) {
            best = std::move(extended);
        }
    }

    std::sort(best.begin(), best.end(),
              [](const RecommendedItem& a, const RecommendedItem& b) {
                  return a.score > b.score;
              });
    return best;
}

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

// Keeps the per-category list sorted best first and capped at `limit`.
// Ties keep insertion (catalog) order, matching the old single-best pick.
void insertCandidate(OutfitCandidates& by_category, RecommendedItem&& item, int limit) {
    auto& list = by_category[item.category_slug];
    list.push_back(std::move(item));
    std::stable_sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });
    if (static_cast<int>(list.size()) > limit) list.resize(limit);
}

}  // namespace

std::vector<RecommendedItem> Recommender::recommend(const RecommendationRequest& request) const {
    return assembleOutfit(candidates(request, 3), kOptionalCategoryThreshold,
                          FeedbackRepository(db_).pairAffinities());
}

std::vector<RecommendedItem> Recommender::recommendFromWardrobe(
    const RecommendationRequest& request) const {
    return assembleOutfit(candidatesFromWardrobe(request, 3), kOptionalCategoryThreshold,
                          FeedbackRepository(db_).pairAffinities());
}

OutfitCandidates Recommender::candidates(const RecommendationRequest& request,
                                         int limit) const {
    sqlite3* db = db_.handle();
    const auto mood_weights = resolveMoodWeights(db, request);
    const auto affinities = loadAffinities(db);
    const auto ratings = FeedbackRepository(db_).averageRatings();

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kItemQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, request.lang.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, request.gender.c_str(), -1, SQLITE_TRANSIENT);

    OutfitCandidates by_category;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RecommendedItem item = readScoredItem(stmt, request, affinities, mood_weights);
        if (isExcluded(request, item.item_slug)) continue;
        applyFeedback(item, ratings);
        insertCandidate(by_category, std::move(item), limit);
    }
    sqlite3_finalize(stmt);
    return by_category;
}

OutfitCandidates Recommender::candidatesFromWardrobe(const RecommendationRequest& request,
                                                     int limit) const {
    sqlite3* db = db_.handle();
    const auto mood_weights = resolveMoodWeights(db, request);
    const auto affinities = loadAffinities(db);
    const auto ratings = FeedbackRepository(db_).averageRatings();

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kWardrobeQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, request.lang.c_str(), -1, SQLITE_TRANSIENT);

    // All attribute values per wardrobe item: colors and patterns feed the
    // preference adjustments, the full list describes the piece to a stylist.
    std::map<int, std::vector<std::string>> colors;
    std::map<int, std::vector<std::string>> patterns;
    std::map<int, std::vector<std::string>> all_values;
    {
        sqlite3_stmt* attr_stmt = nullptr;
        constexpr const char* kAttrQuery = R"sql(
            SELECT wa.wardrobe_item_id, a.slug, v.slug
            FROM wardrobe_item_attributes wa
            JOIN attribute_values v ON v.id = wa.attribute_value_id
            JOIN attributes a ON a.id = v.attribute_id;
        )sql";
        if (sqlite3_prepare_v2(db, kAttrQuery, -1, &attr_stmt, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
        }
        while (sqlite3_step(attr_stmt) == SQLITE_ROW) {
            const int wardrobe_id = sqlite3_column_int(attr_stmt, 0);
            const std::string attr =
                reinterpret_cast<const char*>(sqlite3_column_text(attr_stmt, 1));
            const std::string value =
                reinterpret_cast<const char*>(sqlite3_column_text(attr_stmt, 2));
            if (attr == "color") colors[wardrobe_id].push_back(value);
            if (attr == "pattern") patterns[wardrobe_id].push_back(value);
            all_values[wardrobe_id].push_back(value);
        }
        sqlite3_finalize(attr_stmt);
    }

    OutfitCandidates by_category;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RecommendedItem item = readScoredItem(stmt, request, affinities, mood_weights);
        if (isExcluded(request, item.item_slug)) continue;
        applyFeedback(item, ratings);
        item.wardrobe_id = sqlite3_column_int(stmt, 9);
        const std::string label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.photo_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        item.cutout_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        if (!label.empty()) {
            item.item_name = label;
        }
        const auto values_it = all_values.find(item.wardrobe_id);
        if (values_it != all_values.end()) {
            item.value_slugs = values_it->second;
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
        insertCandidate(by_category, std::move(item), limit);
    }
    sqlite3_finalize(stmt);
    return by_category;
}

}  // namespace negiysem
