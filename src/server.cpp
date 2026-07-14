#include "server.h"

#include <httplib.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

#include "database.h"
#include "env.h"
#include "gemini.h"
#include "recommender.h"
#include "wardrobe.h"
#include "weather.h"

namespace negiysem {

using nlohmann::json;

namespace {

json itemToJson(const RecommendedItem& item) {
    json entry = {
        {"category_slug", item.category_slug},
        {"category_name", item.category_name},
        {"item_slug", item.item_slug},
        {"item_name", item.item_name},
        {"score", item.score},
    };
    if (item.wardrobe_id > 0) {
        entry["wardrobe_id"] = item.wardrobe_id;
        entry["photo_url"] = item.photo_path.empty() ? "" : "/photos/" + item.photo_path;
    }
    return entry;
}

}  // namespace

std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         double temperature_c,
                         bool is_raining,
                         const std::string& city,
                         const std::string& mood_slug,
                         const std::string& source) {
    json items = json::array();
    for (const auto& item : outfit) {
        items.push_back(itemToJson(item));
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

// slug -> localized display name for every mood.
std::map<std::string, std::string> moodNames(Database& db, const std::string& lang) {
    constexpr const char* sql = R"sql(
        SELECT m.slug, COALESCE(t.name, m.slug)
        FROM moods m
        LEFT JOIN translations t
               ON t.entity_type = 'mood' AND t.entity_id = m.id AND t.lang_code = ?1;
    )sql";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db.handle()));
    }
    sqlite3_bind_text(stmt, 1, lang.c_str(), -1, SQLITE_TRANSIENT);
    std::map<std::string, std::string> names;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        names[reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))] =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    }
    sqlite3_finalize(stmt);
    return names;
}

// Maps an uploaded image content type to a file extension; empty if unsupported.
std::string photoExtension(const std::string& content_type) {
    if (content_type == "image/jpeg") return ".jpg";
    if (content_type == "image/png") return ".png";
    if (content_type == "image/webp") return ".webp";
    return "";
}

// Every garment type slug, for the photo classifier's vocabulary.
std::vector<std::string> typeSlugs(Database& db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(), "SELECT slug FROM clothing_items ORDER BY id;", -1,
                           &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db.handle()));
    }
    std::vector<std::string> slugs;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        slugs.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return slugs;
}

// Allowed value slugs per attribute slug, for the photo classifier.
AttributeVocabulary attributeVocabulary(Database& db) {
    constexpr const char* sql = R"sql(
        SELECT a.slug, v.slug
        FROM attribute_values v
        JOIN attributes a ON a.id = v.attribute_id
        ORDER BY a.id, v.id;
    )sql";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db.handle()));
    }
    AttributeVocabulary vocabulary;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        vocabulary[reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))].emplace_back(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    }
    sqlite3_finalize(stmt);
    return vocabulary;
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

    server.Post("/api/ask", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const std::string api_key = getConfigValue("GEMINI_API_KEY");
            if (api_key.empty()) {
                res.status = 503;
                res.set_content(
                    json{{"error", "GEMINI_API_KEY is not set"}, {"code", "no_api_key"}}.dump(),
                    "application/json");
                return;
            }
            const json body = json::parse(req.body);
            const std::string text = body.at("text").get<std::string>();
            const std::string lang = body.value("lang", "en");

            std::string model = getConfigValue("GEMINI_MODEL");
            if (model.empty()) model = "gemini-flash-latest";
            const GeminiClient gemini(api_key, model);

            const ParsedRequest parsed = gemini.parseUserRequest(text);

            WeatherService weather_service;
            const auto location = weather_service.detectLocation();
            const auto weather = weather_service.fetchForecast(location, parsed.day_offset);

            RecommendationRequest request;
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;
            request.mood_slug = parsed.mood_slug.empty() ? "relaxed" : parsed.mood_slug;
            request.lang = lang;
            request.colors_preferred = parsed.colors_preferred;
            request.colors_avoided = parsed.colors_avoided;
            request.patterns_preferred = parsed.patterns_preferred;
            request.patterns_avoided = parsed.patterns_avoided;

            const bool use_wardrobe = WardrobeRepository(db_).count() > 0;
            const Recommender recommender(db_);
            const auto outfit = use_wardrobe ? recommender.recommendFromWardrobe(request)
                                             : recommender.recommend(request);

            // The explanation is presentation only: if it fails, the outfit
            // still goes out.
            std::string explanation;
            try {
                explanation = gemini.explainOutfit(text, outfit, weather, lang);
            } catch (const std::exception& e) {
                std::cerr << "explanation failed: " << e.what() << std::endl;
            }

            json items = json::array();
            for (const auto& item : outfit) {
                items.push_back(itemToJson(item));
            }
            res.set_content(
                json{
                    {"weather",
                     {{"temperature_c", weather.temperature_c},
                      {"is_raining", weather.is_raining},
                      {"city", location.city}}},
                    {"parsed",
                     {{"day_offset", parsed.day_offset},
                      {"mood", request.mood_slug},
                      {"occasion", parsed.occasion},
                      {"colors_preferred", parsed.colors_preferred},
                      {"colors_avoided", parsed.colors_avoided},
                      {"patterns_preferred", parsed.patterns_preferred},
                      {"patterns_avoided", parsed.patterns_avoided}}},
                    {"source", use_wardrobe ? "wardrobe" : "catalog"},
                    {"outfit", items},
                    {"explanation", explanation},
                }
                    .dump(),
                "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Post("/api/classify-photo",
                [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const std::string api_key = getConfigValue("GEMINI_API_KEY");
            if (api_key.empty()) {
                res.status = 503;
                res.set_content(
                    json{{"error", "GEMINI_API_KEY is not set"}, {"code", "no_api_key"}}.dump(),
                    "application/json");
                return;
            }
            const std::string content_type = req.get_header_value("Content-Type");
            if (photoExtension(content_type).empty()) {
                res.status = 415;
                res.set_content(json{{"error", "expected image/jpeg, image/png or image/webp"}}
                                    .dump(),
                                "application/json");
                return;
            }

            std::string model = getConfigValue("GEMINI_MODEL");
            if (model.empty()) model = "gemini-flash-latest";
            const ClassifiedGarment garment =
                GeminiClient(api_key, model)
                    .classifyGarment(req.body, content_type, typeSlugs(db_),
                                     attributeVocabulary(db_));

            json values = json::object();
            for (const auto& [attribute, slugs] : garment.values) {
                values[attribute] = slugs;  // always an array, even for one value
            }
            res.set_content(json{{"type", garment.type_slug}, {"values", values}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Post("/api/feel", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const std::string api_key = getConfigValue("GEMINI_API_KEY");
            if (api_key.empty()) {
                res.status = 503;
                res.set_content(
                    json{{"error", "GEMINI_API_KEY is not set"}, {"code", "no_api_key"}}.dump(),
                    "application/json");
                return;
            }
            const json body = json::parse(req.body);
            const std::string text = body.at("text").get<std::string>();
            const std::string lang = body.value("lang", "en");

            std::string model = getConfigValue("GEMINI_MODEL");
            if (model.empty()) model = "gemini-flash-latest";
            MoodWeights weights = GeminiClient(api_key, model).analyzeMood(text);
            if (weights.empty()) {
                weights = {{"relaxed", 1.0}};  // the text said nothing about mood
            }

            WeatherService weather_service;
            const auto location = weather_service.detectLocation();
            const auto weather = weather_service.fetchCurrent(location);

            RecommendationRequest request;
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;
            request.mood_weights = weights;
            request.mood_slug = weights.front().first;  // for the response only
            request.lang = lang;

            std::string source = body.value("source", "catalog");
            if (source == "wardrobe" && WardrobeRepository(db_).count() == 0) {
                source = "catalog";
            }
            const Recommender recommender(db_);
            const auto outfit = source == "wardrobe"
                                    ? recommender.recommendFromWardrobe(request)
                                    : recommender.recommend(request);
            if (source != "wardrobe") source = "catalog";

            const auto names = moodNames(db_, lang);
            json moods = json::array();
            for (const auto& [slug, weight] : weights) {
                const auto name_it = names.find(slug);
                moods.push_back({{"slug", slug},
                                 {"name", name_it != names.end() ? name_it->second : slug},
                                 {"weight", weight}});
            }
            json items = json::array();
            for (const auto& item : outfit) {
                items.push_back(itemToJson(item));
            }
            res.set_content(
                json{
                    {"weather",
                     {{"temperature_c", weather.temperature_c},
                      {"is_raining", weather.is_raining},
                      {"city", location.city}}},
                    {"moods", moods},
                    {"source", source},
                    {"outfit", items},
                }
                    .dump(),
                "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
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
