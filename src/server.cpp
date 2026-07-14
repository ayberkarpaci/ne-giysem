#include "server.h"

#include <httplib.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

#include "database.h"
#include "env.h"
#include "feedback.h"
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

json weatherToJson(const WeatherReport& weather) {
    json body = {
        {"temperature_c", weather.temperature_c},
        {"is_raining", weather.is_raining},
        {"city", weather.city},
        {"basis", weather.basis},
    };
    if (weather.basis == "window") {
        body["start_hour"] = weather.start_hour;
        body["end_hour"] = weather.end_hour;
    }
    return body;
}

// Optional user overrides shared by every recommendation endpoint: a manual
// weather entry, a corrected location and/or the hour window they are out.
struct WeatherOverrides {
    std::optional<double> temperature_c;
    bool is_raining = false;
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::string city;
    std::optional<int> start_hour;
    std::optional<int> end_hour;
};

WeatherOverrides overridesFromParams(const httplib::Request& req) {
    WeatherOverrides o;
    if (req.has_param("temp")) {
        o.temperature_c = std::stod(req.get_param_value("temp"));
        o.is_raining = req.get_param_value("rain") == "1";
    }
    if (req.has_param("lat") && req.has_param("lon")) {
        o.latitude = std::stod(req.get_param_value("lat"));
        o.longitude = std::stod(req.get_param_value("lon"));
    }
    if (req.has_param("city")) o.city = req.get_param_value("city");
    if (req.has_param("start_hour") && req.has_param("end_hour")) {
        o.start_hour = std::stoi(req.get_param_value("start_hour"));
        o.end_hour = std::stoi(req.get_param_value("end_hour"));
    }
    return o;
}

WeatherOverrides overridesFromJson(const json& body) {
    WeatherOverrides o;
    if (body.contains("temp") && body["temp"].is_number()) {
        o.temperature_c = body["temp"].get<double>();
        o.is_raining = body.value("rain", false);
    }
    if (body.contains("lat") && body["lat"].is_number() &&
        body.contains("lon") && body["lon"].is_number()) {
        o.latitude = body["lat"].get<double>();
        o.longitude = body["lon"].get<double>();
    }
    o.city = body.value("city", "");
    if (body.contains("start_hour") && body["start_hour"].is_number_integer() &&
        body.contains("end_hour") && body["end_hour"].is_number_integer()) {
        o.start_hour = body["start_hour"].get<int>();
        o.end_hour = body["end_hour"].get<int>();
    }
    return o;
}

// Only the two known values pass through; anything else means no filter.
std::string sanitizeGender(const std::string& gender) {
    return (gender == "male" || gender == "female") ? gender : "";
}

// Decides the weather for a recommendation: manual entry wins, then the
// user's corrected location, then IP geolocation; with an hour window the
// hourly forecast is averaged, otherwise current/daily conditions are used.
WeatherReport resolveWeather(const WeatherOverrides& overrides, int day_offset) {
    WeatherReport report;
    report.city = overrides.city;
    if (overrides.temperature_c) {
        report.temperature_c = *overrides.temperature_c;
        report.is_raining = overrides.is_raining;
        report.basis = "manual";
        return report;
    }

    WeatherService service;
    Location location;
    if (overrides.latitude && overrides.longitude) {
        location.latitude = *overrides.latitude;
        location.longitude = *overrides.longitude;
        location.city = overrides.city;
    } else {
        location = service.detectLocation();
        report.city = location.city;
    }

    Weather weather;
    if (overrides.start_hour && overrides.end_hour) {
        weather = service.fetchWindow(location, day_offset, *overrides.start_hour,
                                      *overrides.end_hour);
        report.basis = "window";
        report.start_hour = std::clamp(*overrides.start_hour, 0, 23);
        report.end_hour = std::clamp(*overrides.end_hour, 0, 23);
        if (report.end_hour < report.start_hour) {
            std::swap(report.start_hour, report.end_hour);
        }
    } else {
        weather = service.fetchForecast(location, day_offset);
        report.basis = day_offset > 0 ? "daily" : "current";
    }
    report.temperature_c = weather.temperature_c;
    report.is_raining = weather.is_raining;
    return report;
}

}  // namespace

std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         const WeatherReport& weather,
                         const std::string& mood_slug,
                         const std::string& source,
                         int recommendation_id,
                         const std::string& explanation) {
    json items = json::array();
    for (const auto& item : outfit) {
        items.push_back(itemToJson(item));
    }
    json body = {
        {"weather", weatherToJson(weather)},
        {"mood", mood_slug},
        {"source", source},
        {"outfit", items},
        {"explanation", explanation},
    };
    if (recommendation_id > 0) body["recommendation_id"] = recommendation_id;
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

    server.Get("/api/location", [](const httplib::Request&, httplib::Response& res) {
        try {
            const Location location = WeatherService().detectLocation();
            res.set_content(json{{"city", location.city},
                                 {"latitude", location.latitude},
                                 {"longitude", location.longitude}}
                                .dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    server.Get("/api/geocode", [](const httplib::Request& req, httplib::Response& res) {
        try {
            const std::string name = req.get_param_value("name");
            if (name.size() < 2) {
                res.set_content(json{{"results", json::array()}}.dump(), "application/json");
                return;
            }
            const std::string lang =
                req.has_param("lang") ? req.get_param_value("lang") : "en";
            json results = json::array();
            for (const auto& loc : WeatherService().searchCity(name, lang)) {
                results.push_back({{"city", loc.city},
                                   {"latitude", loc.latitude},
                                   {"longitude", loc.longitude}});
            }
            res.set_content(json{{"results", results}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

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
            // No mood parameter means no mood bias, not a default mood.
            request.mood_slug = req.get_param_value("mood");
            request.lang = req.has_param("lang") ? req.get_param_value("lang") : "en";
            request.gender = sanitizeGender(req.get_param_value("gender"));

            const WeatherReport weather = resolveWeather(overridesFromParams(req), 0);
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;

            // Fall back to the catalog when the wardrobe is empty, and say so
            // in the response so the UI can explain.
            std::string source = req.get_param_value("source");
            if (source == "wardrobe" && WardrobeRepository(db_).count() == 0) {
                source = "catalog";
            }
            const Recommender recommender(db_);

            // With a Gemini key the stylist assembles and explains the
            // outfit here too; without one the rule-based picks still work.
            std::vector<RecommendedItem> outfit;
            std::string explanation;
            const std::string api_key = getConfigValue("GEMINI_API_KEY");
            if (!api_key.empty()) {
                try {
                    std::string model = getConfigValue("GEMINI_MODEL");
                    if (model.empty()) model = "gemini-flash-latest";
                    Weather conditions;
                    conditions.temperature_c = request.temperature_c;
                    conditions.is_raining = request.is_raining;
                    const std::string context =
                        request.mood_slug.empty()
                            ? "An everyday outfit; no particular plan or mood was given."
                            : "An everyday outfit; the user's mood is '" +
                                  request.mood_slug + "'.";
                    const auto candidates =
                        source == "wardrobe"
                            ? recommender.candidatesFromWardrobe(request, 3)
                            : recommender.candidates(request, 3);
                    const StyledOutfit styled = GeminiClient(api_key, model)
                        .styleOutfit(context, candidates, conditions, request.lang);
                    outfit = styled.items;
                    explanation = styled.reason;
                } catch (const std::exception& e) {
                    std::cerr << "stylist failed: " << e.what() << std::endl;
                }
            }
            if (outfit.empty()) {
                outfit = source == "wardrobe" ? recommender.recommendFromWardrobe(request)
                                              : recommender.recommend(request);
            }
            if (source != "wardrobe") source = "catalog";

            const int rec_id =
                FeedbackRepository(db_).recordRecommendation(request, outfit, source);
            res.set_content(outfitToJson(outfit, weather, request.mood_slug, source, rec_id,
                                         explanation),
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

            const WeatherReport weather =
                resolveWeather(overridesFromJson(body), parsed.day_offset);

            RecommendationRequest request;
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;
            request.mood_slug = parsed.mood_slug.empty() ? "relaxed" : parsed.mood_slug;
            request.lang = lang;
            request.gender = sanitizeGender(body.value("gender", ""));
            request.colors_preferred = parsed.colors_preferred;
            request.colors_avoided = parsed.colors_avoided;
            request.patterns_preferred = parsed.patterns_preferred;
            request.patterns_avoided = parsed.patterns_avoided;

            const bool use_wardrobe = WardrobeRepository(db_).count() > 0;
            const Recommender recommender(db_);
            Weather conditions;
            conditions.temperature_c = weather.temperature_c;
            conditions.is_raining = weather.is_raining;

            // A stylist pass picks the most coherent combination from the
            // top candidates and says why; the plain best-per-category
            // outfit is the fallback when it fails.
            std::vector<RecommendedItem> outfit;
            std::string explanation;
            try {
                const auto candidates =
                    use_wardrobe ? recommender.candidatesFromWardrobe(request, 3)
                                 : recommender.candidates(request, 3);
                const StyledOutfit styled =
                    gemini.styleOutfit(text, candidates, conditions, lang);
                outfit = styled.items;
                explanation = styled.reason;
            } catch (const std::exception& e) {
                std::cerr << "stylist failed: " << e.what() << std::endl;
            }
            if (outfit.empty()) {
                outfit = use_wardrobe ? recommender.recommendFromWardrobe(request)
                                      : recommender.recommend(request);
            }
            // The explanation is presentation only: if it fails, the outfit
            // still goes out.
            if (explanation.empty()) {
                try {
                    explanation = gemini.explainOutfit(text, outfit, conditions, lang);
                } catch (const std::exception& e) {
                    std::cerr << "explanation failed: " << e.what() << std::endl;
                }
            }

            const int rec_id = FeedbackRepository(db_).recordRecommendation(
                request, outfit, use_wardrobe ? "wardrobe" : "catalog");
            json items = json::array();
            for (const auto& item : outfit) {
                items.push_back(itemToJson(item));
            }
            res.set_content(
                json{
                    {"recommendation_id", rec_id},
                    {"weather", weatherToJson(weather)},
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
            // Tell the client when this was the API quota rather than the
            // photo, so bulk uploads can offer a retry instead of blaming
            // the image.
            const std::string what = e.what();
            const bool rate_limited = what.find("rate limit") != std::string::npos ||
                                      what.find("RESOURCE_EXHAUSTED") != std::string::npos;
            res.status = rate_limited ? 429 : 500;
            json body = {{"error", what}};
            if (rate_limited) body["code"] = "rate_limited";
            res.set_content(body.dump(), "application/json");
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
            const GeminiClient gemini(api_key, model);
            MoodWeights weights = gemini.analyzeMood(text);
            if (weights.empty()) {
                weights = {{"relaxed", 1.0}};  // the text said nothing about mood
            }

            const WeatherReport weather = resolveWeather(overridesFromJson(body), 0);

            RecommendationRequest request;
            request.temperature_c = weather.temperature_c;
            request.is_raining = weather.is_raining;
            request.mood_weights = weights;
            request.mood_slug = weights.front().first;  // for the response only
            request.lang = lang;
            request.gender = sanitizeGender(body.value("gender", ""));

            std::string source = body.value("source", "catalog");
            if (source == "wardrobe" && WardrobeRepository(db_).count() == 0) {
                source = "catalog";
            }
            const Recommender recommender(db_);

            // Same stylist pass as /api/ask, with the mood answer as context.
            std::vector<RecommendedItem> outfit;
            std::string explanation;
            try {
                Weather conditions;
                conditions.temperature_c = weather.temperature_c;
                conditions.is_raining = weather.is_raining;
                const auto candidates =
                    source == "wardrobe" ? recommender.candidatesFromWardrobe(request, 3)
                                         : recommender.candidates(request, 3);
                const StyledOutfit styled =
                    gemini.styleOutfit(text, candidates, conditions, lang);
                outfit = styled.items;
                explanation = styled.reason;
            } catch (const std::exception& e) {
                std::cerr << "stylist failed: " << e.what() << std::endl;
            }
            if (outfit.empty()) {
                outfit = source == "wardrobe" ? recommender.recommendFromWardrobe(request)
                                              : recommender.recommend(request);
            }
            if (source != "wardrobe") source = "catalog";

            const auto names = moodNames(db_, lang);
            json moods = json::array();
            for (const auto& [slug, weight] : weights) {
                const auto name_it = names.find(slug);
                moods.push_back({{"slug", slug},
                                 {"name", name_it != names.end() ? name_it->second : slug},
                                 {"weight", weight}});
            }
            const int rec_id =
                FeedbackRepository(db_).recordRecommendation(request, outfit, source);
            json items = json::array();
            for (const auto& item : outfit) {
                items.push_back(itemToJson(item));
            }
            res.set_content(
                json{
                    {"recommendation_id", rec_id},
                    {"weather", weatherToJson(weather)},
                    {"moods", moods},
                    {"source", source},
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

    server.Post("/api/feedback", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const json body = json::parse(req.body);
            const int id = body.at("recommendation_id").get<int>();
            const int rating = body.at("rating").get<int>();
            const std::string comment = body.value("comment", "");

            FeedbackRepository repo(db_);
            if (!repo.addFeedback(id, rating, comment)) {
                res.status = 404;
                res.set_content(json{{"error", "no such recommendation"}}.dump(),
                                "application/json");
                return;
            }

            json response = {{"ok", true}};
            // A poor rating earns an immediate alternative: same context,
            // but with everything from the disliked outfit left out.
            if (rating <= 2) {
                const auto stored = repo.recommendation(id);
                RecommendationRequest request;
                request.mood_slug = stored->mood_slug;  // empty = no mood bias
                request.temperature_c = stored->temperature_c;
                request.is_raining = stored->is_raining;
                request.lang = body.value("lang", stored->lang);
                request.gender = sanitizeGender(body.value("gender", ""));
                request.exclude_items = stored->item_slugs;

                const Recommender recommender(db_);
                const auto outfit = stored->source == "wardrobe"
                                        ? recommender.recommendFromWardrobe(request)
                                        : recommender.recommend(request);
                if (!outfit.empty()) {
                    response["recommendation_id"] =
                        repo.recordRecommendation(request, outfit, stored->source);
                    json items = json::array();
                    for (const auto& item : outfit) {
                        items.push_back(itemToJson(item));
                    }
                    response["outfit"] = items;
                    response["source"] = stored->source;
                }
            }
            res.set_content(response.dump(), "application/json");
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
