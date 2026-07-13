#pragma once

namespace negiysem {

class Database;

// Inserts the built-in catalog (categories, items, moods, mood affinities,
// English and Turkish translations). Idempotent: uses INSERT OR IGNORE.
void seedDatabase(Database& db);

}  // namespace negiysem
