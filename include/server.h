#pragma once

#include <string>
#include <vector>

namespace negiysem {

class Database;
struct RecommendedItem;

// JSON builders, exposed for unit testing. Both return serialized JSON.
std::string outfitToJson(const std::vector<RecommendedItem>& outfit,
                         double temperature_c,
                         bool is_raining,
                         const std::string& city,
                         const std::string& mood_slug,
                         const std::string& source = "catalog");
std::string moodsToJson(Database& db, const std::string& lang);

// HTTP server: serves the static web UI plus a small JSON API.
//   GET    /api/moods?lang=..
//   GET    /api/recommendation?mood=..&lang=..[&temp=..&rain=0|1][&source=wardrobe]
//   GET    /api/types?lang=..
//   GET    /api/attributes?type=..&lang=..
//   GET    /api/wardrobe?lang=..
//   POST   /api/wardrobe                     JSON {type, label, values: [..]}
//   PUT    /api/wardrobe/<id>/photo          raw image body (jpeg/png/webp)
//   DELETE /api/wardrobe/<id>
// Weather is fetched automatically unless temp is given. Photos are stored
// under data/photos/ and served at /photos/.
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
