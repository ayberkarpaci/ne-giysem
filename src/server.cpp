#include "server.h"

#include <httplib.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "database.h"
#include "recommender.h"
#include "wardrobe.h"
#include "weather.h"

namespace negiysem {

using nlohmann::json;

std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         double temperature_c,
                         bool is_raining,
                         const std::string& city,
                         const std::string& mood_slug,
                         const std::string& source) {
    json items = json::array();
    for (const auto& item : outfit) {
        json entry = {
            {"category_slug", item.category_slug},
            {"category_name", item.category_name},
            {"item_slug", item.item_slug},
            {"item_name", item.item_name},
            {"score", item.score},
        };
        if (item.wardrobe_id > 0) {
            entry["wardrobe_id"] = item.wardrobe_id;
            entry["photo_url"] =
                item.photo_path.empty() ? "" : "/photos/" + item.photo_path;
        }
        items.push_back(std::move(entry));
    }
    const json body = {
        {"weather", {{"temperature_c", temperature_c}, {"is_raining", is_raining}, {"city", city}}},
        {"mood", mood_slug},
        {"source", source},
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

namespace {

// Maps an uploaded image content type to a file extension; empty if unsupported.
std::string photoExtension(const std::string& content_type) {
    if (content_type == "image/jpeg") return ".jpg";
    if (content_type == "image/png") return ".png";
    if (content_type == "image/webp") return ".webp";
    return "";
}

const char* kPhotoDir = "data/photos";

}  // namespace

bool Server::run(int port) {
    httplib::Server server;

    std::filesystem::create_directories(kPhotoDir);
    server.set_mount_point("/photos", kPhotoDir);
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

            // Fall back to the catalog when the wardrobe is empty, and say so
            // in the response so the UI can explain.
            std::string source = req.get_param_value("source");
            if (source == "wardrobe" && WardrobeRepository(db_).count() == 0) {
                source = "catalog";
            }
            const Recommender recommender(db_);
            const auto outfit = source == "wardrobe"
                                    ? recommender.recommendFromWardrobe(request)
                                    : recommender.recommend(request);
            if (source != "wardrobe") source = "catalog";

            res.set_content(outfitToJson(outfit, request.temperature_c, request.is_raining, city,
                                         request.mood_slug, source),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Get("/api/types", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json types = json::array();
            for (const auto& t : WardrobeRepository(db_).listTypes(req.get_param_value("lang"))) {
                types.push_back({{"slug", t.slug},
                                 {"name", t.name},
                                 {"category_slug", t.category_slug},
                                 {"category_name", t.category_name}});
            }
            res.set_content(json{{"types", types}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Get("/api/attributes", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json attrs = json::array();
            const auto defs = WardrobeRepository(db_).attributesForType(
                req.get_param_value("type"), req.get_param_value("lang"));
            for (const auto& def : defs) {
                json values = json::array();
                for (const auto& v : def.values) {
                    values.push_back({{"slug", v.slug}, {"name", v.name}});
                }
                attrs.push_back({{"slug", def.slug}, {"name", def.name}, {"values", values}});
            }
            res.set_content(json{{"attributes", attrs}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Get("/api/wardrobe", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json items = json::array();
            for (const auto& item : WardrobeRepository(db_).listItems(req.get_param_value("lang"))) {
                json values = json::array();
                for (const auto& v : item.values) {
                    values.push_back({{"attribute_slug", v.attribute_slug},
                                      {"attribute_name", v.attribute_name},
                                      {"slug", v.value_slug},
                                      {"name", v.value_name}});
                }
                items.push_back({
                    {"id", item.id},
                    {"type_slug", item.type_slug},
                    {"type_name", item.type_name},
                    {"category_slug", item.category_slug},
                    {"category_name", item.category_name},
                    {"label", item.label},
                    {"photo_url", item.photo_path.empty() ? "" : "/photos/" + item.photo_path},
                    {"values", values},
                });
            }
            res.set_content(json{{"items", items}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Post("/api/wardrobe", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const json body = json::parse(req.body);
            const int id = WardrobeRepository(db_).addItem(
                body.at("type").get<std::string>(),
                body.value("label", ""),
                body.value("values", std::vector<std::string>{}));
            res.status = 201;
            res.set_content(json{{"id", id}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Put(R"(/api/wardrobe/(\d+)/photo)",
               [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const int id = std::stoi(req.matches[1]);
            WardrobeRepository repo(db_);
            if (!repo.photoPath(id)) {
                res.status = 404;
                res.set_content(json{{"error", "no such wardrobe item"}}.dump(),
                                "application/json");
                return;
            }
            const std::string ext = photoExtension(req.get_header_value("Content-Type"));
            if (ext.empty()) {
                res.status = 415;
                res.set_content(json{{"error", "expected image/jpeg, image/png or image/webp"}}
                                    .dump(),
                                "application/json");
                return;
            }
            const std::string file_name = std::to_string(id) + ext;
            std::ofstream out(std::filesystem::path(kPhotoDir) / file_name, std::ios::binary);
            out.write(req.body.data(), static_cast<std::streamsize>(req.body.size()));
            out.close();
            repo.setPhotoPath(id, file_name);
            res.set_content(json{{"photo_url", "/photos/" + file_name}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Delete(R"(/api/wardrobe/(\d+))",
                  [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const int id = std::stoi(req.matches[1]);
            WardrobeRepository repo(db_);
            const auto photo = repo.photoPath(id);
            if (!repo.removeItem(id)) {
                res.status = 404;
                res.set_content(json{{"error", "no such wardrobe item"}}.dump(),
                                "application/json");
                return;
            }
            if (photo && !photo->empty()) {
                std::error_code ignored;
                std::filesystem::remove(std::filesystem::path(kPhotoDir) / *photo, ignored);
            }
            res.set_content(json{{"deleted", id}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    std::cout << "ne-giysem server listening on http://localhost:" << port << std::endl;
    return server.listen("127.0.0.1", port);
}

}  // namespace negiysem
