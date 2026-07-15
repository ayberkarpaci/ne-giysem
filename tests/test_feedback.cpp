#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>

#include "database.h"
#include "feedback.h"
#include "recommender.h"
#include "seed.h"
#include "wardrobe.h"

using negiysem::Database;
using negiysem::FeedbackRepository;
using negiysem::RecommendationRequest;
using negiysem::RecommendedItem;
using negiysem::Recommender;
using Catch::Matchers::WithinAbs;

namespace {

Database makeSeededDb() {
    Database db(":memory:");
    db.initSchema();
    negiysem::seedDatabase(db);
    return db;
}

RecommendationRequest coldCozyRequest() {
    RecommendationRequest request;
    request.temperature_c = 12.0;
    request.is_raining = false;
    request.mood_slug = "cozy";
    return request;
}

}  // namespace

TEST_CASE("feedbackAdjustment maps ratings to score shifts") {
    CHECK_THAT(negiysem::feedbackAdjustment(3.0), WithinAbs(0.0, 1e-9));
    CHECK_THAT(negiysem::feedbackAdjustment(5.0), WithinAbs(0.3, 1e-9));
    CHECK_THAT(negiysem::feedbackAdjustment(1.0), WithinAbs(-0.3, 1e-9));
    CHECK_THAT(negiysem::feedbackAdjustment(4.0), WithinAbs(0.15, 1e-9));
    CHECK_THAT(negiysem::feedbackAdjustment(99.0), WithinAbs(0.3, 1e-9));  // clamped
}

TEST_CASE("feedback roundtrip: record, rate, aggregate") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);

    const auto request = coldCozyRequest();
    const auto outfit = Recommender(db).recommend(request);
    REQUIRE_FALSE(outfit.empty());

    const int rec_id = repo.recordRecommendation(request, outfit, "catalog");
    CHECK(rec_id > 0);

    const auto stored = repo.recommendation(rec_id);
    REQUIRE(stored.has_value());
    CHECK(stored->source == "catalog");
    CHECK(stored->mood_slug == "cozy");
    CHECK_THAT(stored->temperature_c, WithinAbs(12.0, 1e-9));
    CHECK(stored->item_slugs.size() == outfit.size());

    CHECK(repo.addFeedback(rec_id, 2, "too warm"));
    const auto averages = repo.averageRatings();
    for (const auto& item : outfit) {
        REQUIRE(averages.count(item.item_slug) == 1);
        CHECK_THAT(averages.at(item.item_slug), WithinAbs(2.0, 1e-9));
    }
}

TEST_CASE("addFeedback validates its input") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);
    CHECK_FALSE(repo.addFeedback(12345, 4, ""));            // unknown id
    CHECK_THROWS(repo.addFeedback(12345, 0, ""));           // rating out of range
    CHECK_FALSE(repo.recommendation(12345).has_value());
}

TEST_CASE("excluded items never come back") {
    Database db = makeSeededDb();
    const auto base = Recommender(db).recommend(coldCozyRequest());
    REQUIRE_FALSE(base.empty());

    auto request = coldCozyRequest();
    for (const auto& item : base) {
        request.exclude_items.push_back(item.item_slug);
    }
    const auto retry = Recommender(db).recommend(request);
    for (const auto& item : retry) {
        CHECK(std::find(request.exclude_items.begin(), request.exclude_items.end(),
                        item.item_slug) == request.exclude_items.end());
    }
}

TEST_CASE("bad ratings push an item out of the pick") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);
    const auto request = coldCozyRequest();

    // At 12° cozy the catalog top is the hoodie (cozy 0.9 -> score 1.9);
    // the sweater (cozy 0.8 -> 1.8) is the runner-up. A one-star rating on
    // a hoodie outfit shifts hoodies by -0.3, so the sweater takes over.
    const auto before = Recommender(db).recommend(request);
    const auto topBefore = std::find_if(before.begin(), before.end(), [](const auto& i) {
        return i.category_slug == "top";
    });
    REQUIRE(topBefore != before.end());
    REQUIRE(topBefore->item_slug == "hoodie");

    const int rec_id = repo.recordRecommendation(request, {*topBefore}, "catalog");
    repo.addFeedback(rec_id, 1, "hate hoodies");

    const auto after = Recommender(db).recommend(request);
    const auto topAfter = std::find_if(after.begin(), after.end(), [](const auto& i) {
        return i.category_slug == "top";
    });
    REQUIRE(topAfter != after.end());
    CHECK(topAfter->item_slug == "sweater");
}

TEST_CASE("listOutfits returns saved outfits with wardrobe pieces resolved") {
    Database db = makeSeededDb();
    negiysem::WardrobeRepository wardrobe(db);
    const int jeans_id = wardrobe.addItem("jeans", "favorite jeans", {"blue"});
    wardrobe.setPhotoPath(jeans_id, "7.png");
    wardrobe.setCutoutPath(jeans_id, "cutouts/7.png");

    FeedbackRepository repo(db);
    RecommendedItem jeans;
    jeans.item_slug = "jeans";
    jeans.category_slug = "bottom";
    jeans.wardrobe_id = jeans_id;
    RecommendedItem catalog_piece;
    catalog_piece.item_slug = "sweater";
    catalog_piece.category_slug = "top";

    const auto request = coldCozyRequest();
    repo.recordRecommendation(request, {jeans, catalog_piece}, "wardrobe",
                              "Cozy and casual for a cold day.");

    const auto outfits = repo.listOutfits("en", 10);
    REQUIRE(outfits.size() == 1);
    CHECK(outfits[0].mood_slug == "cozy");
    CHECK(outfits[0].explanation == "Cozy and casual for a cold day.");
    REQUIRE(outfits[0].items.size() == 2);

    const auto& saved_jeans = *std::find_if(
        outfits[0].items.begin(), outfits[0].items.end(),
        [](const auto& item) { return item.item_slug == "jeans"; });
    CHECK(saved_jeans.item_name == "favorite jeans");
    CHECK(saved_jeans.category_slug == "bottom");
    CHECK(saved_jeans.wardrobe_id == jeans_id);
    CHECK(saved_jeans.cutout_path == "cutouts/7.png");

    const auto& saved_sweater = *std::find_if(
        outfits[0].items.begin(), outfits[0].items.end(),
        [](const auto& item) { return item.item_slug == "sweater"; });
    CHECK(saved_sweater.item_name == "Sweater");  // localized type, no label
    CHECK(saved_sweater.wardrobe_id == 0);
    CHECK(saved_sweater.cutout_path == "");
}

TEST_CASE("setLabel renames a wardrobe item") {
    Database db = makeSeededDb();
    negiysem::WardrobeRepository repo(db);
    const int id = repo.addItem("jeans", "", {"blue"});
    CHECK(repo.setLabel(id, "weekend jeans"));
    CHECK(repo.listItems("en")[0].label == "weekend jeans");
    CHECK_FALSE(repo.setLabel(9999, "nope"));
}

TEST_CASE("feedback tags are stored, validated and read back") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);
    const auto request = coldCozyRequest();
    const auto outfit = Recommender(db).recommend(request);
    const int rec_id = repo.recordRecommendation(request, outfit, "catalog");

    SECTION("round-trip across multiple ratings") {
        CHECK(repo.addFeedback(rec_id, 2, "", {"too-hot", "colors-clash"}));
        CHECK(repo.addFeedback(rec_id, 3, "", {"too-hot"}));  // duplicate tag ok
        const auto tags = repo.tagsFor(rec_id);
        REQUIRE(tags.size() == 2);  // distinct, sorted
        CHECK(tags[0] == "colors-clash");
        CHECK(tags[1] == "too-hot");
    }
    SECTION("unknown tags are rejected") {
        CHECK_THROWS(repo.addFeedback(rec_id, 2, "", {"too-itchy"}));
    }
    SECTION("no tags stays valid") {
        CHECK(repo.addFeedback(rec_id, 5, "great"));
        CHECK(repo.tagsFor(rec_id).empty());
    }
}

TEST_CASE("pairAffinities counts pairs from well-rated outfits only") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);
    const auto request = coldCozyRequest();

    RecommendedItem tee;
    tee.item_slug = "t-shirt";
    RecommendedItem jeans;
    jeans.item_slug = "jeans";
    RecommendedItem boots;
    boots.item_slug = "boots";

    const int loved = repo.recordRecommendation(request, {tee, jeans, boots}, "wardrobe");
    repo.addFeedback(loved, 5, "");
    const int hated = repo.recordRecommendation(request, {tee, boots}, "wardrobe");
    repo.addFeedback(hated, 1, "");

    const auto memory = repo.pairAffinities();
    CHECK(memory.size() == 3);  // pairs from the loved outfit only
    CHECK(memory.at({"jeans", "t-shirt"}) == 1);
    CHECK(memory.at({"boots", "jeans"}) == 1);
    CHECK(memory.at({"boots", "t-shirt"}) == 1);  // the 1-star pair doesn't add

    // A second high rating on the same outfit strengthens the memory.
    repo.addFeedback(loved, 4, "");
    CHECK(repo.pairAffinities().at({"jeans", "t-shirt"}) == 2);
}

TEST_CASE("temperature offsets accumulate in bounded half-degree steps") {
    Database db = makeSeededDb();
    FeedbackRepository repo(db);

    repo.nudgeTemperature({"t-shirt", "jeans"}, -0.5);
    repo.nudgeTemperature({"t-shirt"}, -0.5);
    auto offsets = repo.temperatureOffsets();
    CHECK_THAT(offsets.at("t-shirt"), WithinAbs(-1.0, 1e-9));
    CHECK_THAT(offsets.at("jeans"), WithinAbs(-0.5, 1e-9));

    // The shift never runs away: bounded to [-5, 5].
    for (int i = 0; i < 30; ++i) repo.nudgeTemperature({"t-shirt"}, -0.5);
    CHECK_THAT(repo.temperatureOffsets().at("t-shirt"), WithinAbs(-5.0, 1e-9));
    for (int i = 0; i < 30; ++i) repo.nudgeTemperature({"t-shirt"}, 0.5);
    CHECK_THAT(repo.temperatureOffsets().at("t-shirt"), WithinAbs(5.0, 1e-9));
}
