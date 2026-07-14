#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>

#include "database.h"
#include "feedback.h"
#include "recommender.h"
#include "seed.h"

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
