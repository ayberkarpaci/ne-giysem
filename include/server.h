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
                         const std::string& mood_slug);
std::string moodsToJson(Database& db, const std::string& lang);

// HTTP server: serves the static web UI plus a small JSON API.
//   GET /api/moods?lang=..
//   GET /api/recommendation?mood=..&lang=..[&temp=..&rain=0|1]
// Weather is fetched automatically unless temp is given.
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
