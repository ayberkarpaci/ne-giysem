#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "database.h"
#include "recommender.h"
#include "seed.h"

using negiysem::Database;
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

const RecommendedItem* findCategory(const std::vector<RecommendedItem>& outfit,
                                    const std::string& category_slug) {
    auto it = std::find_if(outfit.begin(), outfit.end(), [&](const RecommendedItem& item) {
        return item.category_slug == category_slug;
    });
    return it == outfit.end() ? nullptr : &*it;
}

}  // namespace

TEST_CASE("temperatureFit") {
    SECTION("inside the range scores 1.0") {
        CHECK_THAT(negiysem::temperatureFit(20.0, 15.0, 25.0), WithinAbs(1.0, 1e-9));
    }
    SECTION("no range is neutral") {
        CHECK_THAT(negiysem::temperatureFit(20.0, std::nullopt, std::nullopt),
                   WithinAbs(0.5, 1e-9));
    }
    SECTION("decays by 0.15 per degree outside") {
        CHECK_THAT(negiysem::temperatureFit(11.0, 15.0, 25.0), WithinAbs(0.4, 1e-9));
        CHECK_THAT(negiysem::temperatureFit(27.0, 15.0, 25.0), WithinAbs(0.7, 1e-9));
    }
    SECTION("never goes below zero") {
        CHECK_THAT(negiysem::temperatureFit(-30.0, 15.0, 25.0), WithinAbs(0.0, 1e-9));
    }
}

TEST_CASE("rainAdjustment") {
    CHECK_THAT(negiysem::rainAdjustment(true, true), WithinAbs(0.5, 1e-9));
    CHECK_THAT(negiysem::rainAdjustment(true, false), WithinAbs(0.0, 1e-9));
    CHECK_THAT(negiysem::rainAdjustment(false, true), WithinAbs(-0.2, 1e-9));
    CHECK_THAT(negiysem::rainAdjustment(false, false), WithinAbs(0.0, 1e-9));
}

TEST_CASE("cold rainy weather picks waterproof outerwear") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 10.0;
    request.is_raining = true;
    request.mood_slug = "cozy";

    const auto outfit = Recommender(db).recommend(request);

    const auto* outerwear = findCategory(outfit, "outerwear");
    REQUIRE(outerwear != nullptr);
    CHECK(outerwear->item_slug == "raincoat");

    // Core categories are always present.
    CHECK(findCategory(outfit, "top") != nullptr);
    CHECK(findCategory(outfit, "bottom") != nullptr);
    CHECK(findCategory(outfit, "footwear") != nullptr);
}

TEST_CASE("hot dry weather picks summer clothes and no outerwear") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 32.0;
    request.is_raining = false;
    request.mood_slug = "energetic";

    const auto outfit = Recommender(db).recommend(request);

    CHECK(findCategory(outfit, "outerwear") == nullptr);

    const auto* top = findCategory(outfit, "top");
    REQUIRE(top != nullptr);
    CHECK(top->item_slug == "t-shirt");

    const auto* bottom = findCategory(outfit, "bottom");
    REQUIRE(bottom != nullptr);
    CHECK(bottom->item_slug == "shorts");
}

TEST_CASE("mood affinity breaks ties within a category") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 12.0;  // hoodie and sweater both fit
    request.is_raining = false;
    request.mood_slug = "cozy";

    const auto outfit = Recommender(db).recommend(request);

    const auto* top = findCategory(outfit, "top");
    REQUIRE(top != nullptr);
    CHECK(top->item_slug == "hoodie");  // cozy affinity 0.9 beats sweater's 0.8
}

TEST_CASE("mood blend") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 16.0;
    request.is_raining = false;

    SECTION("a single full-weight mood matches the plain mood_slug path") {
        request.mood_slug = "cozy";
        const auto plain = Recommender(db).recommend(request);

        request.mood_slug.clear();
        request.mood_weights = {{"cozy", 1.0}};
        const auto blended = Recommender(db).recommend(request);

        REQUIRE(plain.size() == blended.size());
        for (size_t i = 0; i < plain.size(); ++i) {
            CHECK(plain[i].item_slug == blended[i].item_slug);
        }
    }
    SECTION("the dominant mood steers the pick") {
        request.mood_weights = {{"cozy", 0.7}, {"confident", 0.3}};
        const auto cozy_outfit = Recommender(db).recommend(request);
        const auto* top = findCategory(cozy_outfit, "top");
        REQUIRE(top != nullptr);
        CHECK(top->item_slug == "hoodie");

        request.mood_weights = {{"confident", 0.7}, {"cozy", 0.3}};
        const auto confident_outfit = Recommender(db).recommend(request);
        const auto* top2 = findCategory(confident_outfit, "top");
        REQUIRE(top2 != nullptr);
        CHECK(top2->item_slug == "shirt");
    }
    SECTION("unknown mood in the blend throws") {
        request.mood_weights = {{"hangry", 1.0}};
        CHECK_THROWS(Recommender(db).recommend(request));
    }
}

TEST_CASE("display names use the requested language with slug fallback") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 32.0;
    request.mood_slug = "energetic";

    SECTION("Turkish") {
        request.lang = "tr";
        const auto outfit = Recommender(db).recommend(request);
        const auto* top = findCategory(outfit, "top");
        REQUIRE(top != nullptr);
        CHECK(top->item_name == "Tişört");
    }
    SECTION("unknown language falls back to slugs") {
        request.lang = "xx";
        const auto outfit = Recommender(db).recommend(request);
        const auto* top = findCategory(outfit, "top");
        REQUIRE(top != nullptr);
        CHECK(top->item_name == "t-shirt");
    }
}

TEST_CASE("no mood at all recommends purely by weather") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 32.0;
    request.is_raining = false;
    // mood_slug stays empty: the user skipped the question.

    const auto outfit = Recommender(db).recommend(request);
    const auto* top = findCategory(outfit, "top");
    REQUIRE(top != nullptr);
    CHECK_THAT(top->score, WithinAbs(1.0, 1e-9));  // temperature fit only
}

TEST_CASE("unknown mood throws") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.mood_slug = "hangry";
    CHECK_THROWS(Recommender(db).recommend(request));
}
