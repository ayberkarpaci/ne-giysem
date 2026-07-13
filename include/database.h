#pragma once

#include <string>

struct sqlite3;

namespace negiysem {

// RAII wrapper around a SQLite connection.
// Opens (creating if necessary) the database file and applies the schema.
class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    // Creates all tables if they do not exist yet.
    void initSchema();

    // Runs a statement that returns no rows; throws std::runtime_error on failure.
    void execute(const std::string& sql);

    // Returns the number of user tables in the database.
    int tableCount() const;

    sqlite3* handle() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

}  // namespace negiysem
