#pragma once

#include <string>
#include <vector>

namespace negiysem {

class Database;
struct RecommendedItem;

// The weather a recommendation was based on, including how it was decided,
// so the UI can tell the user (e.g. "average between 09-18h in Aydın").
struct WeatherReport {
    double temperature_c = 0.0;
    bool is_raining = false;
    std::string city;
    std::string basis = "current";  // current | daily | window | manual
    int start_hour = -1;            // set only when basis == "window"
    int end_hour = -1;
};

// JSON builders, exposed for unit testing. Both return serialized JSON.
// recommendation_id > 0 is included so the client can send feedback on it.
std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         const WeatherReport& weather,
                         const std::string& mood_slug,
                         const std::string& source = "catalog",
                         int recommendation_id = 0);
std::string moodsToJson(Database& db, const std::string& lang);

// HTTP server: serves the static web UI plus a small JSON API.
//   POST   /api/ask                          JSON {text, lang, <weather overrides>}
//                                            -> Gemini-parsed request, outfit, explanation
//   GET    /api/moods?lang=..
//   GET    /api/recommendation?mood=..&lang=..[&source=wardrobe][<weather overrides>]
//   GET    /api/location                     IP-based city + coordinates
//   GET    /api/geocode?name=..&lang=..      city search for manual correction
//   GET    /api/types?lang=..
//   GET    /api/attributes?type=..&lang=..
//   GET    /api/wardrobe?lang=..
//   POST   /api/wardrobe                     JSON {type, label, values: [..]}
//   PUT    /api/wardrobe/<id>/photo          raw image body (jpeg/png/webp)
//   DELETE /api/wardrobe/<id>
// Weather overrides (query params on GET, JSON fields on POST): manual
// weather (temp + rain), a corrected location (lat + lon + city) and/or an
// hour window (start_hour + end_hour) that averages the hourly forecast.
// Photos are stored under data/photos/ and served at /photos/.
class Server {
public:
    Server(Database& db, std::string web_root) : db_(db), web_root_(std::move(web_root)) {}

    // Blocks serving requests; returns false if the port cannot be bound.
    bool run(int port);

private:
    Database& db_;
    std::string web_root_;
};

}  // namespace negiysem
