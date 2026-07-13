#include <exception>
#include <iostream>

#include "database.h"

int main() {
    std::cout << "Hello, ne-giysem project started!" << std::endl;

    try {
        negiysem::Database db("data/ne-giysem.db");
        db.initSchema();
        std::cout << "Database ready (" << db.tableCount() << " tables)." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
