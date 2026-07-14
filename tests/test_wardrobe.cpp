#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "database.h"
#include "recommender.h"
#include "seed.h"
#include "wardrobe.h"

using negiysem::Database;
using negiysem::RecommendationRequest;
using negiysem::Recommender;
using negiysem::WardrobeRepository;

namespace {

Database makeSeededDb() {
    Database db(":memory:");
    db.initSchema();
    negiysem::seedDatabase(db);
    return db;
}

}  // namespace

TEST_CASE("addItem stores type, label and attribute values") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    const int id = repo.addItem("sweater", "navy sweater", {"navy", "wool", "turtleneck"});
    CHECK(id > 0);
    CHECK(repo.count() == 1);

    const auto items = repo.listItems("tr");
    REQUIRE(items.size() == 1);
    CHECK(items[0].id == id);
    CHECK(items[0].type_name == "Kazak");
    CHECK(items[0].category_slug == "top");
    CHECK(items[0].label == "navy sweater");
    REQUIRE(items[0].values.size() == 3);
    CHECK(items[0].values[0].attribute_slug == "color");
    CHECK(items[0].values[0].value_name == "Lacivert");
}

TEST_CASE("addItem rejects unknown slugs and rolls back") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    CHECK_THROWS(repo.addItem("hoverboard", "", {}));
    CHECK_THROWS(repo.addItem("sweater", "", {"navy", "no-such-value"}));
    CHECK(repo.count() == 0);  // the failed insert left nothing behind
}

TEST_CASE("removeItem deletes the item and its values") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);
    const int id = repo.addItem("jeans", "", {"blue", "denim"});

    CHECK(repo.removeItem(id));
    CHECK(repo.count() == 0);
    CHECK_FALSE(repo.removeItem(id));  // already gone
}

TEST_CASE("attributesForType follows the category mapping") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    const auto forTop = repo.attributesForType("sweater", "en");
    std::vector<std::string> slugs;
    for (const auto& def : forTop) slugs.push_back(def.slug);
    CHECK(std::find(slugs.begin(), slugs.end(), "collar") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "fit") == slugs.end());

    const auto forBottom = repo.attributesForType("jeans", "en");
    slugs.clear();
    for (const auto& def : forBottom) slugs.push_back(def.slug);
    CHECK(std::find(slugs.begin(), slugs.end(), "fit") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "collar") == slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "pattern") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "sleeve") == slugs.end());
}

TEST_CASE("expanded catalog supports jerseys with pattern and sleeve") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    const int id = repo.addItem("jersey", "", {"striped", "short-sleeve", "red"});
    CHECK(id > 0);

    const auto items = repo.listItems("tr");
    REQUIRE(items.size() == 1);
    CHECK(items[0].type_name == "Forma");
    CHECK(items[0].category_slug == "top");
    CHECK(items[0].values.size() == 3);
}

TEST_CASE("dresses live in the one-piece category with rich attributes") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    const auto forDress = repo.attributesForType("dress", "en");
    std::vector<std::string> slugs;
    for (const auto& def : forDress) slugs.push_back(def.slug);
    CHECK(std::find(slugs.begin(), slugs.end(), "collar") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "sleeve") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "length") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "fit") == slugs.end());

    const int id = repo.addItem("dress", "",
                                {"burgundy", "satin", "asymmetric-neck", "midi"});
    CHECK(id > 0);
    const auto items = repo.listItems("tr");
    REQUIRE(items.size() == 1);
    CHECK(items[0].category_slug == "one-piece");
    CHECK(items[0].type_name == "Elbise");
    CHECK(items[0].values.size() == 4);
}

TEST_CASE("jewelry items carry metal and color but no garment attributes") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);

    const auto forJewelry = repo.attributesForType("necklace", "en");
    std::vector<std::string> slugs;
    for (const auto& def : forJewelry) slugs.push_back(def.slug);
    CHECK(std::find(slugs.begin(), slugs.end(), "metal") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "color") != slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "material") == slugs.end());
    CHECK(std::find(slugs.begin(), slugs.end(), "sleeve") == slugs.end());

    const int id = repo.addItem("watch", "everyday watch", {"steel", "black"});
    CHECK(id > 0);
    const auto items = repo.listItems("tr");
    REQUIRE(items.size() == 1);
    CHECK(items[0].category_slug == "jewelry");
    CHECK(items[0].type_name == "Kol saati");
}

TEST_CASE("pattern preference steers the wardrobe pick") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);
    repo.addItem("t-shirt", "plain tee", {"solid"});
    repo.addItem("t-shirt", "striped tee", {"striped"});

    RecommendationRequest request;
    request.temperature_c = 25.0;
    request.mood_slug = "relaxed";

    // Same type and score, so which tee wins is unspecified without a
    // preference; with one, the striped tee must win.
    request.patterns_preferred = {"striped"};
    auto outfit = Recommender(db).recommendFromWardrobe(request);
    REQUIRE(outfit.size() == 1);
    CHECK(outfit[0].item_name == "striped tee");

    request.patterns_preferred.clear();
    request.patterns_avoided = {"striped"};
    outfit = Recommender(db).recommendFromWardrobe(request);
    REQUIRE(outfit.size() == 1);
    CHECK(outfit[0].item_name == "plain tee");
}

TEST_CASE("recommendFromWardrobe only uses owned garments") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);
    repo.addItem("sweater", "navy sweater", {"navy", "wool"});
    repo.addItem("jeans", "", {"blue"});

    RecommendationRequest request;
    request.temperature_c = 8.0;
    request.mood_slug = "cozy";

    const auto outfit = Recommender(db).recommendFromWardrobe(request);
    REQUIRE(outfit.size() == 2);  // no footwear owned, so none suggested

    for (const auto& item : outfit) {
        CHECK(item.wardrobe_id > 0);
    }
    const auto top = std::find_if(outfit.begin(), outfit.end(), [](const auto& item) {
        return item.category_slug == "top";
    });
    REQUIRE(top != outfit.end());
    CHECK(top->item_name == "navy sweater");  // label wins over type name
}
