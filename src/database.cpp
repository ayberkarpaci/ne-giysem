#include "database.h"

#include <sqlite3.h>

#include <stdexcept>

namespace negiysem {

namespace {

// All display names live in the translations table so the app can be
// localized later; the core tables only store stable slugs.
constexpr const char* kSchema = R"sql(
CREATE TABLE IF NOT EXISTS clothing_categories (
    id   INTEGER PRIMARY KEY,
    slug TEXT NOT NULL UNIQUE            -- e.g. 'outerwear', 'top', 'bottom', 'footwear', 'accessory'
);

CREATE TABLE IF NOT EXISTS clothing_items (
    id            INTEGER PRIMARY KEY,
    category_id   INTEGER NOT NULL REFERENCES clothing_categories(id),
    slug          TEXT NOT NULL UNIQUE,  -- e.g. 'raincoat', 't-shirt'
    min_temp_c    REAL,                  -- comfortable temperature range
    max_temp_c    REAL,
    is_waterproof INTEGER NOT NULL DEFAULT 0,
    warmth_level  INTEGER NOT NULL DEFAULT 0  -- 0 (none) .. 5 (very warm)
);

CREATE TABLE IF NOT EXISTS moods (
    id   INTEGER PRIMARY KEY,
    slug TEXT NOT NULL UNIQUE            -- e.g. 'energetic', 'cozy', 'confident'
);

-- How well an item fits a mood; the recommender scores outfits with this.
CREATE TABLE IF NOT EXISTS item_mood_affinity (
    item_id INTEGER NOT NULL REFERENCES clothing_items(id),
    mood_id INTEGER NOT NULL REFERENCES moods(id),
    weight  REAL NOT NULL DEFAULT 1.0,
    PRIMARY KEY (item_id, mood_id)
);

-- Localized display names for any slug-bearing entity.
CREATE TABLE IF NOT EXISTS translations (
    entity_type TEXT NOT NULL,           -- 'category' | 'item' | 'mood'
    entity_id   INTEGER NOT NULL,
    lang_code   TEXT NOT NULL,           -- BCP 47, e.g. 'en', 'tr'
    name        TEXT NOT NULL,
    PRIMARY KEY (entity_type, entity_id, lang_code)
);
)sql";

}  // namespace

Database::Database(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("failed to open database '" + db_path + "': " + msg);
    }
    execute("PRAGMA foreign_keys = ON;");
}

Database::Database(Database&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        sqlite3_close(db_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

Database::~Database() {
    sqlite3_close(db_);
}

void Database::initSchema() {
    execute(kSchema);
}

void Database::execute(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("sql error: " + msg);
    }
}

int Database::tableCount() const {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%';";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db_));
    }
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

}  // namespace negiysem
