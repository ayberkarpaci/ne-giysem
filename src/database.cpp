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
    warmth_level  INTEGER NOT NULL DEFAULT 0,  -- 0 (none) .. 5 (very warm)
    gender        TEXT NOT NULL DEFAULT 'unisex',  -- 'unisex' | 'male' | 'female'
    formality     INTEGER NOT NULL DEFAULT 2  -- 0 (sporty) .. 5 (formal)
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
    entity_type TEXT NOT NULL,           -- 'category' | 'item' | 'mood' | 'attribute' | 'attribute_value'
    entity_id   INTEGER NOT NULL,
    lang_code   TEXT NOT NULL,           -- BCP 47, e.g. 'en', 'tr'
    name        TEXT NOT NULL,
    PRIMARY KEY (entity_type, entity_id, lang_code)
);

-- Detail dimensions a garment can have (color, material, collar, fit).
CREATE TABLE IF NOT EXISTS attributes (
    id   INTEGER PRIMARY KEY,
    slug TEXT NOT NULL UNIQUE
);

-- Which attributes make sense for which category (collar only for tops, ...).
CREATE TABLE IF NOT EXISTS category_attributes (
    category_id  INTEGER NOT NULL REFERENCES clothing_categories(id),
    attribute_id INTEGER NOT NULL REFERENCES attributes(id),
    PRIMARY KEY (category_id, attribute_id)
);

-- Controlled vocabulary per attribute; slugs are globally unique so a bare
-- value slug (e.g. 'navy') is unambiguous.
CREATE TABLE IF NOT EXISTS attribute_values (
    id           INTEGER PRIMARY KEY,
    attribute_id INTEGER NOT NULL REFERENCES attributes(id),
    slug         TEXT NOT NULL UNIQUE
);

-- The user's own garments. The type points into the clothing_items catalog,
-- which supplies temperature range, waterproofness and mood affinities.
CREATE TABLE IF NOT EXISTS wardrobe_items (
    id         INTEGER PRIMARY KEY,
    type_id    INTEGER NOT NULL REFERENCES clothing_items(id),
    label      TEXT,                     -- optional user-given name
    photo_path TEXT,                     -- file name under data/photos/
    cutout_path TEXT,                    -- transparent PNG under data/photos/cutouts/
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    is_dirty   INTEGER NOT NULL DEFAULT 0,   -- in the laundry basket
    wears_since_wash INTEGER NOT NULL DEFAULT 0
);

-- Every time the user actually wore a piece ("I wore this").
CREATE TABLE IF NOT EXISTS wear_history (
    id               INTEGER PRIMARY KEY,
    wardrobe_item_id INTEGER NOT NULL REFERENCES wardrobe_items(id) ON DELETE CASCADE,
    worn_at          TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS wardrobe_item_attributes (
    wardrobe_item_id   INTEGER NOT NULL REFERENCES wardrobe_items(id) ON DELETE CASCADE,
    attribute_value_id INTEGER NOT NULL REFERENCES attribute_values(id),
    PRIMARY KEY (wardrobe_item_id, attribute_value_id)
);

-- Every outfit we served, with the context it was computed from, so the
-- user's feedback can point back at it and a retry can rebuild the request.
CREATE TABLE IF NOT EXISTS recommendations (
    id            INTEGER PRIMARY KEY,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    source        TEXT NOT NULL,       -- 'catalog' | 'wardrobe'
    mood_slug     TEXT,
    temperature_c REAL,
    is_raining    INTEGER NOT NULL DEFAULT 0,
    lang          TEXT,
    explanation   TEXT,                -- the stylist's one-sentence reason
    title         TEXT                 -- user-given name for manual outfits
);

CREATE TABLE IF NOT EXISTS recommendation_items (
    recommendation_id INTEGER NOT NULL REFERENCES recommendations(id) ON DELETE CASCADE,
    item_slug         TEXT NOT NULL,   -- catalog type slug of the suggested piece
    wardrobe_item_id  INTEGER          -- the user's own piece, when one was used
);

-- The user's verdict on a served outfit; the recommender learns from it.
CREATE TABLE IF NOT EXISTS recommendation_feedback (
    id                INTEGER PRIMARY KEY,
    recommendation_id INTEGER NOT NULL REFERENCES recommendations(id) ON DELETE CASCADE,
    rating            INTEGER NOT NULL CHECK (rating BETWEEN 1 AND 5),
    comment           TEXT,
    created_at        TEXT NOT NULL DEFAULT (datetime('now'))
);

-- One-tap reasons attached to a rating ('too-hot', 'colors-clash', ...);
-- the controlled vocabulary lives in kFeedbackTags (feedback.cpp).
CREATE TABLE IF NOT EXISTS feedback_tags (
    feedback_id INTEGER NOT NULL REFERENCES recommendation_feedback(id) ON DELETE CASCADE,
    tag         TEXT NOT NULL,
    PRIMARY KEY (feedback_id, tag)
);

-- Personal comfort calibration: 'too hot/cold' feedback shifts a type's
-- comfort temperature range by this many degrees for this user.
CREATE TABLE IF NOT EXISTS type_temp_offsets (
    item_slug TEXT PRIMARY KEY,          -- clothing_items catalog slug
    offset_c  REAL NOT NULL DEFAULT 0    -- bounded to [-5, 5]
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
    // Databases created before the gender column existed need it added; the
    // ALTER fails harmlessly ("duplicate column") on up-to-date databases.
    try {
        execute("ALTER TABLE clothing_items ADD COLUMN gender TEXT NOT NULL "
                "DEFAULT 'unisex';");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE clothing_items ADD COLUMN formality INTEGER NOT NULL "
                "DEFAULT 2;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE wardrobe_items ADD COLUMN cutout_path TEXT;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE recommendations ADD COLUMN explanation TEXT;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE recommendation_items ADD COLUMN wardrobe_item_id INTEGER;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE recommendations ADD COLUMN title TEXT;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE wardrobe_items ADD COLUMN is_dirty INTEGER NOT NULL DEFAULT 0;");
    } catch (const std::runtime_error&) {
    }
    try {
        execute("ALTER TABLE wardrobe_items ADD COLUMN wears_since_wash INTEGER NOT NULL "
                "DEFAULT 0;");
    } catch (const std::runtime_error&) {
    }
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
