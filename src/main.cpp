#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

#include "database.h"
#include "recommender.h"
#include "seed.h"

namespace {

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [temp_c] [rain 0|1] [mood] [lang]\n"
              << "  temp_c  temperature in Celsius (default 15)\n"
              << "  rain    1 if it is raining (default 0)\n"
              << "  mood    energetic | cozy | confident | relaxed | adventurous (default cozy)\n"
              << "  lang    display language, e.g. en or tr (default en)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    negiysem::RecommendationRequest request;
    request.mood_slug = "cozy";

    if (argc > 1 && std::string(argv[1]) == "--help") {
        printUsage(argv[0]);
        return 0;
    }
    if (argc > 1) request.temperature_c = std::atof(argv[1]);
    if (argc > 2) request.is_raining = std::string(argv[2]) == "1";
    if (argc > 3) request.mood_slug = argv[3];
    if (argc > 4) request.lang = argv[4];

    try {
        negiysem::Database db("data/ne-giysem.db");
        db.initSchema();
        negiysem::seedDatabase(db);

        negiysem::Recommender recommender(db);
        const auto outfit = recommender.recommend(request);

        std::cout << "Outfit for " << request.temperature_c << " C, "
                  << (request.is_raining ? "rainy" : "dry") << " weather, mood '"
                  << request.mood_slug << "':\n";
        for (const auto& item : outfit) {
            std::cout << "  " << std::left << std::setw(12) << (item.category_name + ":")
                      << item.item_name << "  (score " << std::fixed << std::setprecision(2)
                      << item.score << ")\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
