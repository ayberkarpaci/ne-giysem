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

TEST_CASE("cutout path round-trips and dies with the photo it came from") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);
    const int id = repo.addItem("jeans", "", {"blue"});

    repo.setPhotoPath(id, "1.png");
    repo.setCutoutPath(id, "cutouts/1.png");
    CHECK(repo.cutoutPath(id).value() == "cutouts/1.png");
    CHECK(repo.listItems("en")[0].cutout_path == "cutouts/1.png");

    // A new photo invalidates the cutout made from the old one.
    repo.setPhotoPath(id, "1-new.png");
    CHECK(repo.cutoutPath(id).value() == "");

    CHECK_FALSE(repo.cutoutPath(9999).has_value());
}

TEST_CASE("wearing a piece uses up its category's wear budget") {
    Database db = makeSeededDb();
    WardrobeRepository repo(db);
    const int tee = repo.addItem("t-shirt", "", {"white"});
    const int sneakers = repo.addItem("sneakers", "", {"white"});

    // Tops go dirty after two wears...
    CHECK(repo.recordWear(tee));
    CHECK_FALSE(repo.listItems("en")[1].is_dirty);
    CHECK(repo.recordWear(tee));
    auto items = repo.listItems("en");
    const auto& worn_tee = items[1];  // newest first: sneakers, tee
    CHECK(worn_tee.is_dirty);
    CHECK(worn_tee.wears_since_wash == 2);
    CHECK_FALSE(worn_tee.last_worn_at.empty());

    // ...footwear never does.
    for (int i = 0; i < 6; ++i) repo.recordWear(sneakers);
    CHECK_FALSE(repo.listItems("en")[0].is_dirty);

    // Washing resets the budget.
    CHECK(repo.setDirty(tee, false));
    const auto washed_items = repo.listItems("en");
    CHECK_FALSE(washed_items[1].is_dirty);
    CHECK(washed_items[1].wears_since_wash == 0);

    CHECK_FALSE(repo.recordWear(9999));
}
