#include "feedback.h"

#include <sqlite3.h>

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
                                             const std::string& source) {
    sqlite3* db = db_.handle();
    db_.execute("BEGIN;");
    try {
        {
            Statement insert(db,
                             "INSERT INTO recommendations "
                             "(source, mood_slug, temperature_c, is_raining, lang) "
                             "VALUES (?1, ?2, ?3, ?4, ?5);");
            insert.bindText(1, source);
            insert.bindText(2, request.mood_slug);
            insert.bindDouble(3, request.temperature_c);
            insert.bindInt(4, request.is_raining ? 1 : 0);
            insert.bindText(5, request.lang);
            insert.step();
        }
        const int id = static_cast<int>(sqlite3_last_insert_rowid(db));
        for (const auto& item : outfit) {
            Statement link(db,
                           "INSERT INTO recommendation_items "
                           "(recommendation_id, item_slug) VALUES (?1, ?2);");
            link.bindInt(1, id);
            link.bindText(2, item.item_slug);
            link.step();
        }
        db_.execute("COMMIT;");
        return id;
    } catch (...) {
        db_.execute("ROLLBACK;");
        throw;
    }
}

bool FeedbackRepository::addFeedback(int recommendation_id, int rating,
                                     const std::string& comment) {
    if (rating < 1 || rating > 5) {
        throw std::runtime_error("rating must be between 1 and 5");
    }
    if (!recommendation(recommendation_id)) return false;
    Statement insert(db_.handle(),
                     "INSERT INTO recommendation_feedback "
                     "(recommendation_id, rating, comment) VALUES (?1, ?2, ?3);");
    insert.bindInt(1, recommendation_id);
    insert.bindInt(2, rating);
    insert.bindText(3, comment);
    insert.step();
    return true;
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
                    "SELECT item_slug FROM recommendation_items "
                    "WHERE recommendation_id = ?1;");
    items.bindInt(1, id);
    while (items.step()) {
        stored.item_slugs.push_back(items.columnText(0));
    }
    return stored;
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
