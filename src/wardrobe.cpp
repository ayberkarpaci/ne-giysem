#include "wardrobe.h"

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

    bool step() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error(std::string("step failed: ") + sqlite3_errmsg(db_));
    }

    int columnInt(int col) const { return sqlite3_column_int(stmt_, col); }
    std::string columnText(int col) const {
        const unsigned char* text = sqlite3_column_text(stmt_, col);
        return text ? reinterpret_cast<const char*>(text) : "";
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

int lookupId(sqlite3* db, const char* sql, const std::string& slug, const char* what) {
    Statement stmt(db, sql);
    stmt.bindText(1, slug);
    if (!stmt.step()) {
        throw std::runtime_error(std::string("unknown ") + what + ": '" + slug + "'");
    }
    return stmt.columnInt(0);
}

}  // namespace

int WardrobeRepository::addItem(const std::string& type_slug,
                                const std::string& label,
                                const std::vector<std::string>& value_slugs) {
    sqlite3* db = db_.handle();
    const int type_id =
        lookupId(db, "SELECT id FROM clothing_items WHERE slug = ?1;", type_slug, "garment type");

    db_.execute("BEGIN;");
    try {
        {
            Statement insert(db, "INSERT INTO wardrobe_items (type_id, label) VALUES (?1, ?2);");
            insert.bindInt(1, type_id);
            insert.bindText(2, label);
            insert.step();
        }
        const int item_id = static_cast<int>(sqlite3_last_insert_rowid(db));

        for (const auto& slug : value_slugs) {
            const int value_id = lookupId(
                db, "SELECT id FROM attribute_values WHERE slug = ?1;", slug, "attribute value");
            Statement link(db,
                           "INSERT INTO wardrobe_item_attributes "
                           "(wardrobe_item_id, attribute_value_id) VALUES (?1, ?2);");
            link.bindInt(1, item_id);
            link.bindInt(2, value_id);
            link.step();
        }
        db_.execute("COMMIT;");
        return item_id;
    } catch (...) {
        db_.execute("ROLLBACK;");
        throw;
    }
}

bool WardrobeRepository::removeItem(int id) {
    Statement stmt(db_.handle(), "DELETE FROM wardrobe_items WHERE id = ?1;");
    stmt.bindInt(1, id);
    stmt.step();
    return sqlite3_changes(db_.handle()) > 0;
}

void WardrobeRepository::setPhotoPath(int id, const std::string& file_name) {
    Statement stmt(db_.handle(), "UPDATE wardrobe_items SET photo_path = ?1 WHERE id = ?2;");
    stmt.bindText(1, file_name);
    stmt.bindInt(2, id);
    stmt.step();
}

std::optional<std::string> WardrobeRepository::photoPath(int id) const {
    Statement stmt(db_.handle(), "SELECT COALESCE(photo_path, '') FROM wardrobe_items WHERE id = ?1;");
    stmt.bindInt(1, id);
    if (!stmt.step()) return std::nullopt;
    return stmt.columnText(0);
}

int WardrobeRepository::count() const {
    Statement stmt(db_.handle(), "SELECT COUNT(*) FROM wardrobe_items;");
    stmt.step();
    return stmt.columnInt(0);
}

std::vector<WardrobeItem> WardrobeRepository::listItems(const std::string& lang) const {
    sqlite3* db = db_.handle();
    std::vector<WardrobeItem> items;

    Statement stmt(db, R"sql(
        SELECT w.id, i.slug, COALESCE(ti.name, i.slug),
               c.slug, COALESCE(tc.name, c.slug),
               COALESCE(w.label, ''), COALESCE(w.photo_path, '')
        FROM wardrobe_items w
        JOIN clothing_items i ON i.id = w.type_id
        JOIN clothing_categories c ON c.id = i.category_id
        LEFT JOIN translations ti
               ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?1
        LEFT JOIN translations tc
               ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?1
        ORDER BY w.id DESC;
    )sql");
    stmt.bindText(1, lang);
    while (stmt.step()) {
        WardrobeItem item;
        item.id = stmt.columnInt(0);
        item.type_slug = stmt.columnText(1);
        item.type_name = stmt.columnText(2);
        item.category_slug = stmt.columnText(3);
        item.category_name = stmt.columnText(4);
        item.label = stmt.columnText(5);
        item.photo_path = stmt.columnText(6);
        items.push_back(std::move(item));
    }

    for (auto& item : items) {
        Statement values(db, R"sql(
            SELECT a.slug, COALESCE(ta.name, a.slug), v.slug, COALESCE(tv.name, v.slug)
            FROM wardrobe_item_attributes wa
            JOIN attribute_values v ON v.id = wa.attribute_value_id
            JOIN attributes a ON a.id = v.attribute_id
            LEFT JOIN translations ta
                   ON ta.entity_type = 'attribute' AND ta.entity_id = a.id AND ta.lang_code = ?2
            LEFT JOIN translations tv
                   ON tv.entity_type = 'attribute_value' AND tv.entity_id = v.id AND tv.lang_code = ?2
            WHERE wa.wardrobe_item_id = ?1
            ORDER BY a.id, v.id;
        )sql");
        values.bindInt(1, item.id);
        values.bindText(2, lang);
        while (values.step()) {
            item.values.push_back({values.columnText(0), values.columnText(1),
                                   values.columnText(2), values.columnText(3)});
        }
    }
    return items;
}

std::vector<TypeOption> WardrobeRepository::listTypes(const std::string& lang) const {
    Statement stmt(db_.handle(), R"sql(
        SELECT i.slug, COALESCE(ti.name, i.slug), c.slug, COALESCE(tc.name, c.slug)
        FROM clothing_items i
        JOIN clothing_categories c ON c.id = i.category_id
        LEFT JOIN translations ti
               ON ti.entity_type = 'item' AND ti.entity_id = i.id AND ti.lang_code = ?1
        LEFT JOIN translations tc
               ON tc.entity_type = 'category' AND tc.entity_id = c.id AND tc.lang_code = ?1
        ORDER BY c.id, i.id;
    )sql");
    stmt.bindText(1, lang);
    std::vector<TypeOption> types;
    while (stmt.step()) {
        types.push_back({stmt.columnText(0), stmt.columnText(1), stmt.columnText(2),
                         stmt.columnText(3)});
    }
    return types;
}

std::vector<AttributeDef> WardrobeRepository::attributesForType(const std::string& type_slug,
                                                                const std::string& lang) const {
    sqlite3* db = db_.handle();
    std::vector<AttributeDef> defs;
    std::vector<int> attribute_ids;

    {
        Statement stmt(db, R"sql(
            SELECT a.id, a.slug, COALESCE(ta.name, a.slug)
            FROM clothing_items i
            JOIN category_attributes ca ON ca.category_id = i.category_id
            JOIN attributes a ON a.id = ca.attribute_id
            LEFT JOIN translations ta
                   ON ta.entity_type = 'attribute' AND ta.entity_id = a.id AND ta.lang_code = ?2
            WHERE i.slug = ?1
            ORDER BY a.id;
        )sql");
        stmt.bindText(1, type_slug);
        stmt.bindText(2, lang);
        while (stmt.step()) {
            attribute_ids.push_back(stmt.columnInt(0));
            AttributeDef def;
            def.slug = stmt.columnText(1);
            def.name = stmt.columnText(2);
            defs.push_back(std::move(def));
        }
    }

    for (size_t i = 0; i < defs.size(); ++i) {
        Statement stmt(db, R"sql(
            SELECT v.slug, COALESCE(tv.name, v.slug)
            FROM attribute_values v
            LEFT JOIN translations tv
                   ON tv.entity_type = 'attribute_value' AND tv.entity_id = v.id
                  AND tv.lang_code = ?2
            WHERE v.attribute_id = ?1
            ORDER BY v.id;
        )sql");
        stmt.bindInt(1, attribute_ids[i]);
        stmt.bindText(2, lang);
        while (stmt.step()) {
            defs[i].values.push_back({stmt.columnText(0), stmt.columnText(1)});
        }
    }
    return defs;
}

}  // namespace negiysem
