#include "server.h"

#include <httplib.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>

#include "database.h"
#include "recommender.h"
#include "weather.h"

namespace negiysem {

using nlohmann::json;

std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         double temperature_c,
                         bool is_raining,
                         const std::string& city,
                         const std::string& mood_slug) {
    json items = json::array();
    for (const auto& item : outfit) {
        items.push_back({
            {"category_slug", item.category_slug},
            {"category_name", item.category_name},
            {"item_slug", item.item_slug},
            {"item_name", item.item_name},
            {"score", item.score},
        });
    }
    const json body = {
        {"weather", {{"temperature_c", temperature_c}, {"is_raining", is_raining}, {"city", city}}},
        {"mood", mood_slug},
        {"outfit", items},
    };
    return body.dump();
}

std::string moodsToJson(Database& db, const std::string& lang) {
    constexpr const char* sql = R"sql(
        SELECT m.slug, COALESCE(t.name, m.slug)
        FROM moods m
        LEFT JOIN translations t
               ON t.entity_type = 'mood' AND t.entity_id = m.id AND t.lang_code = ?1
        ORDER BY m.id;
    )sql";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db.handle()));
    }
    sqlite3_bind_text(stmt, 1, lang.c_str(), -1, SQLITE_TRANSIENT);

    json moods = json::array();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        moods.push_back({
            {"slug", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))},
            {"name", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))},
        });
    }
    sqlite3_finalize(stmt);
    return json{{"moods", moods}}.dump();
}

bool Server::run(int port) {
    httplib::Server server;

    if (!server.set_mount_point("/", web_root_)) {
        std::cerr << "Warning: web root '" << web_root_ << "' not found; API only." << std::endl;
    }

    server.Get("/api/moods", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            res.set_content(moodsToJson(db_, req.get_param_value("lang")), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Get("/api/recommendation", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            RecommendationRequest request;
            request.mood_slug = req.has_param("mood") ? req.get_param_value("mood") : "cozy";
            request.lang = req.has_param("lang") ? req.get_param_value("lang") : "en";

            std::string city;
            if (req.has_param("temp")) {
                request.temperature_c = std::stod(req.get_param_value("temp"));
                request.is_raining = req.get_param_value("rain") == "1";
            } else {
                WeatherService weather_service;
                const auto location = weather_service.detectLocation();
                const auto weather = weather_service.fetchCurrent(location);
                request.temperature_c = weather.temperature_c;
                request.is_raining = weather.is_raining;
                city = location.city;
            }

            const auto outfit = Recommender(db_).recommend(request);
            res.set_content(outfitToJson(outfit, request.temperature_c, request.is_raining, city,
                                         request.mood_slug),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    std::cout << "ne-giysem server listening on http://localhost:" << port << std::endl;
    return server.listen("127.0.0.1", port);
}

}  // namespace negiysem
