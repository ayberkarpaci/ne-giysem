#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

#include "database.h"
#include "recommender.h"
#include "seed.h"
#include "server.h"
#include "weather.h"

namespace {

void printUsage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " [mood] [lang]                weather fetched automatically\n"
              << "  " << program << " <temp_c> <rain 0|1> [mood] [lang]   manual weather\n"
              << "  " << program << " serve [port]                 web UI + JSON API (default port 8080)\n"
              << "  mood: energetic | cozy | confident | relaxed | adventurous (default cozy)\n"
              << "  lang: display language, e.g. en or tr (default en)\n";
}

bool isNumber(const std::string& s) {
    char* end = nullptr;
    std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
    negiysem::RecommendationRequest request;
    request.mood_slug = "cozy";
    std::string place;

    if (argc > 1 && std::string(argv[1]) == "--help") {
        printUsage(argv[0]);
        return 0;
    }

    try {
        if (argc > 1 && std::string(argv[1]) == "serve") {
            const int port = argc > 2 ? std::atoi(argv[2]) : 8080;
            negiysem::Database db("data/ne-giysem.db");
            db.initSchema();
            negiysem::seedDatabase(db);
            return negiysem::Server(db, "web").run(port) ? 0 : 1;
        }

        if (argc > 1 && isNumber(argv[1])) {
            // Manual mode: temp and rain from the command line.
            request.temperature_c = std::atof(argv[1]);
            if (argc > 2) request.is_raining = std::string(argv[2]) == "1";
            if (argc > 3) request.mood_slug = argv[3];
            if (argc > 4) request.lang = argv[4];
        } else {
            // Auto mode: geolocate by IP and fetch current weather.
            if (argc > 1) request.mood_slug = argv[1];
            if (argc > 2) request.lang = argv[2];

            negiysem::WeatherService weather_service;
            const auto location = weather_service.detectLocation();
            const auto weather = weather_service.fetchCurrent(location);
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;
            place = location.city;
        }

        negiysem::Database db("data/ne-giysem.db");
        db.initSchema();
        negiysem::seedDatabase(db);

        negiysem::Recommender recommender(db);
        const auto outfit = recommender.recommend(request);

        std::cout << "Outfit for " << request.temperature_c << " C, "
                  << (request.is_raining ? "rainy" : "dry") << " weather";
        if (!place.empty()) {
            std::cout << " in " << place;
        }
        std::cout << ", mood '" << request.mood_slug << "':\n";
        for (const auto& item : outfit) {
            std::cout << "  " << std::left << std::setw(12) << (item.category_name + ":")
                      << item.item_name << "  (score " << std::fixed << std::setprecision(2)
                      << item.score << ")\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                  << "Tip: pass the weather manually, e.g. `ne-giysem 15 0 cozy en`."
                  << std::endl;
        return 1;
    }

    return 0;
}
