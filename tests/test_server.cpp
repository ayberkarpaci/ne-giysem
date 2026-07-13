#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include "database.h"
#include "recommender.h"
#include "seed.h"
#include "server.h"

using nlohmann::json;

TEST_CASE("outfitToJson serializes weather and items") {
    std::vector<negiysem::RecommendedItem> outfit(1);
    outfit[0].item_slug = "raincoat";
    outfit[0].item_name = "Yağmurluk";
    outfit[0].category_slug = "outerwear";
    outfit[0].category_name = "Dış giyim";
    outfit[0].score = 1.5;

    const json j = json::parse(
        negiysem::outfitToJson(outfit, 9.5, true, "Istanbul", "cozy"));

    CHECK(j["weather"]["temperature_c"] == 9.5);
    CHECK(j["weather"]["is_raining"] == true);
    CHECK(j["weather"]["city"] == "Istanbul");
    CHECK(j["mood"] == "cozy");
    REQUIRE(j["outfit"].size() == 1);
    CHECK(j["outfit"][0]["item_name"] == "Yağmurluk");
    CHECK(j["outfit"][0]["category_slug"] == "outerwear");
}

TEST_CASE("moodsToJson lists all moods localized") {
    negiysem::Database db(":memory:");
    db.initSchema();
    negiysem::seedDatabase(db);

    const json j = json::parse(negiysem::moodsToJson(db, "tr"));
    REQUIRE(j["moods"].size() == 5);
    CHECK(j["moods"][0]["slug"] == "energetic");
    CHECK(j["moods"][0]["name"] == "Enerjik");
}
