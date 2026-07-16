#include "feedback.h"

#include <sqlite3.h>

#include <algorithm>
#include <stdexcept>

#include "database.h"

namespace negiysem {

namespace {

// Small RAII helper so early returns/throws always finalize the statement.
class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
        }
    }
    ~Statement() { sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void bindText(int index, const std::string& value) {
        sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    void bindInt(int index, int value) { sqlite3_bind_int(stmt_, index, value); }
    void bindDouble(int index, double value) { sqlite3_bind_double(stmt_, index, value); }

    bool step() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error(std::string("step failed: ") + sqlite3_errmsg(db_));
    }

    int columnInt(int col) const { return sqlite3_column_int(stmt_, col); }
    double columnDouble(int col) const { return sqlite3_column_double(stmt_, col); }
    std::string columnText(int col) const {
        const unsigned char* text = sqlite3_column_text(stmt_, col);
        return text ? reinterpret_cast<const char*>(text) : "";
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace

int FeedbackRepository::recordRecommendation(const RecommendationRequest& request,
                                             const std::vector<RecommendedItem>& outfit,
                                             const std::string& source,
                                             const std::string& explanation) {
    sqlite3* db = db_.handle();
    db_.execute("BEGIN;");
    try {
        {
            Statement insert(db,
                             "INSERT INTO recommendations "
                             "(source, mood_slug, temperature_c, is_raining, lang, explanation) "
                             "VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
            insert.bindText(1, source);
            insert.bindText(2, request.mood_slug);
            insert.bindDouble(3, request.temperature_c);
            insert.bindInt(4, request.is_raining ? 1 : 0);
            insert.bindText(5, request.lang);
            insert.bindText(6, explanation);
            insert.step();
        }
        const int id = static_cast<int>(sqlite3_last_insert_rowid(db));
        for (const auto& item : outfit) {
            Statement link(db,
                           "INSERT INTO recommendation_items "
                           "(recommendation_id, item_slug, wardrobe_item_id) "
                           "VALUES (?1, ?2, ?3);");
            link.bindInt(1, id);
            link.bindText(2, item.item_slug);
            if (item.wardrobe_id > 0) {
                link.bindInt(3, item.wardrobe_id);
            }  // otherwise stays NULL: a catalog piece
            link.step();
        }
        db_.execute("COMMIT;");
        return id;
    } catch (...) {
        db_.execute("ROLLBACK;");
        throw;
    }
}

std::vector<SavedOutfit> FeedbackRepository::listOutfits(const std::string& lang,
                                                         int limit) const {
    sqlite3* db = db_.handle();
    std::vector<SavedOutfit> outfits;
    {
        Statement stmt(db,
                       "SELECT id, created_at, source, COALESCE(mood_slug, ''), "
                       "COALESCE(explanation, ''), COALESCE(title, '') "
                       "FROM recommendations ORDER BY id DESC LIMIT ?1;");
        stmt.bindInt(1, limit);
        while (stmt.step()) {
            SavedOutfit outfit;
            outfit.id = stmt.columnInt(0);
            outfit.created_at = stmt.columnText(1);
            outfit.source = stmt.columnText(2);
            outfit.mood_slug = stmt.columnText(3);
            outfit.explanation = stmt.columnText(4);
            outfit.title = stmt.columnText(5);
            outfits.push_back(std::move(outfit));
        }
    }
    for (auto& outfit : outfits) {
        Statement items(db, R"sql(
            SELECT ri.item_slug,
                   CASE WHEN w.label IS NOT NULL AND w.label != '' THEN w.label
                        ELSE COALESCE(ti.name, ri.item_slug) END,
                   COALESCE(c.slug, ''),
                   COALESCE(ri.wardrobe_item_id, 0),
                   COALESCE(w.photo_path, ''),
                   COALESCE(w.cutout_path, '')
            FROM recommendation_items ri
            LEFT JOIN wardrobe_items w ON w.id = ri.wardrobe_item_id
            LEFT JOIN clothing_items ci ON ci.slug = ri.item_slug
            LEFT JOIN clothing_categories c ON c.id = ci.category_id
            LEFT JOIN translations ti
                   ON ti.entity_type = 'item' AND ti.entity_id = ci.id AND ti.lang_code = ?2
            WHERE ri.recommendation_id = ?1;
        )sql");
        items.bindInt(1, outfit.id);
        items.bindText(2, lang);
        while (items.step()) {
            SavedOutfitItem item;
            item.item_slug = items.columnText(0);
            item.item_name = items.columnText(1);
            item.category_slug = items.columnText(2);
            item.wardrobe_id = items.columnInt(3);
            item.photo_path = items.columnText(4);
            item.cutout_path = items.columnText(5);
            outfit.items.push_back(std::move(item));
        }
    }
    return outfits;
}

const std::vector<std::string>& FeedbackRepository::allowedTags() {
    static const std::vector<std::string> tags = {
        "too-hot", "too-cold", "colors-clash", "too-formal", "too-sporty",
        "uncomfortable",
    };
    return tags;
}

bool FeedbackRepository::setOutfitTitle(int recommendation_id, const std::string& title) {
    Statement stmt(db_.handle(), "UPDATE recommendations SET title = ?1 WHERE id = ?2;");
    stmt.bindText(1, title);
    stmt.bindInt(2, recommendation_id);
    stmt.step();
    return sqlite3_changes(db_.handle()) > 0;
}

bool FeedbackRepository::removeRecommendation(int recommendation_id) {
    Statement stmt(db_.handle(), "DELETE FROM recommendations WHERE id = ?1;");
    stmt.bindInt(1, recommendation_id);
    stmt.step();
    return sqlite3_changes(db_.handle()) > 0;
}

bool FeedbackRepository::addFeedback(int recommendation_id, int rating,
                                     const std::string& comment,
                                     const std::vector<std::string>& tags) {
    if (rating < 1 || rating > 5) {
        throw std::runtime_error("rating must be between 1 and 5");
    }
    const auto& allowed = allowedTags();
    for (const auto& tag : tags) {
        if (std::find(allowed.begin(), allowed.end(), tag) == allowed.end()) {
            throw std::runtime_error("unknown feedback tag: " + tag);
        }
    }
    if (!recommendation(recommendation_id)) return false;
    sqlite3* db = db_.handle();
    {
        Statement insert(db,
                         "INSERT INTO recommendation_feedback "
                         "(recommendation_id, rating, comment) VALUES (?1, ?2, ?3);");
        insert.bindInt(1, recommendation_id);
        insert.bindInt(2, rating);
        insert.bindText(3, comment);
        insert.step();
    }
    const int feedback_id = static_cast<int>(sqlite3_last_insert_rowid(db));
    for (const auto& tag : tags) {
        Statement link(db,
                       "INSERT OR IGNORE INTO feedback_tags (feedback_id, tag) "
                       "VALUES (?1, ?2);");
        link.bindInt(1, feedback_id);
        link.bindText(2, tag);
        link.step();
    }
    return true;
}

std::vector<std::string> FeedbackRepository::tagsFor(int recommendation_id) const {
    Statement stmt(db_.handle(), R"sql(
        SELECT DISTINCT ft.tag
        FROM feedback_tags ft
        JOIN recommendation_feedback f ON f.id = ft.feedback_id
        WHERE f.recommendation_id = ?1
        ORDER BY ft.tag;
    )sql");
    stmt.bindInt(1, recommendation_id);
    std::vector<std::string> tags;
    while (stmt.step()) {
        tags.push_back(stmt.columnText(0));
    }
    return tags;
}

std::optional<StoredRecommendation> FeedbackRepository::recommendation(int id) const {
    sqlite3* db = db_.handle();
    StoredRecommendation stored;
    {
        Statement stmt(db,
                       "SELECT id, source, COALESCE(mood_slug, ''), "
                       "COALESCE(temperature_c, 0), is_raining, COALESCE(lang, 'en') "
                       "FROM recommendations WHERE id = ?1;");
        stmt.bindInt(1, id);
        if (!stmt.step()) return std::nullopt;
        stored.id = stmt.columnInt(0);
        stored.source = stmt.columnText(1);
        stored.mood_slug = stmt.columnText(2);
        stored.temperature_c = stmt.columnDouble(3);
        stored.is_raining = stmt.columnInt(4) != 0;
        stored.lang = stmt.columnText(5);
    }
    Statement items(db,
                    "SELECT item_slug, COALESCE(wardrobe_item_id, 0) "
                    "FROM recommendation_items WHERE recommendation_id = ?1;");
    items.bindInt(1, id);
    while (items.step()) {
        stored.item_slugs.push_back(items.columnText(0));
        if (const int wardrobe_id = items.columnInt(1); wardrobe_id > 0) {
            stored.wardrobe_item_ids.push_back(wardrobe_id);
        }
    }
    return stored;
}

PairAffinities FeedbackRepository::pairAffinities(int min_rating) const {
    Statement stmt(db_.handle(), R"sql(
        SELECT a.item_slug, b.item_slug, COUNT(*)
        FROM recommendation_feedback f
        JOIN recommendation_items a ON a.recommendation_id = f.recommendation_id
        JOIN recommendation_items b ON b.recommendation_id = f.recommendation_id
             AND a.item_slug < b.item_slug
        WHERE f.rating >= ?1
        GROUP BY a.item_slug, b.item_slug;
    )sql");
    stmt.bindInt(1, min_rating);
    PairAffinities affinities;
    while (stmt.step()) {
        affinities[{stmt.columnText(0), stmt.columnText(1)}] = stmt.columnInt(2);
    }
    return affinities;
}

void FeedbackRepository::nudgeTemperature(const std::vector<std::string>& item_slugs,
                                          double delta_c) {
    for (const auto& slug : item_slugs) {
        Statement stmt(db_.handle(), R"sql(
            INSERT INTO type_temp_offsets (item_slug, offset_c)
            VALUES (?1, MAX(-5.0, MIN(5.0, ?2)))
            ON CONFLICT(item_slug) DO UPDATE
            SET offset_c = MAX(-5.0, MIN(5.0, offset_c + ?2));
        )sql");
        stmt.bindText(1, slug);
        stmt.bindDouble(2, delta_c);
        stmt.step();
    }
}

std::map<std::string, double> FeedbackRepository::temperatureOffsets() const {
    Statement stmt(db_.handle(), "SELECT item_slug, offset_c FROM type_temp_offsets;");
    std::map<std::string, double> offsets;
    while (stmt.step()) {
        offsets[stmt.columnText(0)] = stmt.columnDouble(1);
    }
    return offsets;
}

std::map<std::string, double> FeedbackRepository::averageRatings() const {
    Statement stmt(db_.handle(), R"sql(
        SELECT ri.item_slug, AVG(f.rating)
        FROM recommendation_feedback f
        JOIN recommendation_items ri ON ri.recommendation_id = f.recommendation_id
        GROUP BY ri.item_slug;
    )sql");
    std::map<std::string, double> averages;
    while (stmt.step()) {
        averages[stmt.columnText(0)] = stmt.columnDouble(1);
    }
    return averages;
}

}  // namespace negiysem
