#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "database.h"
#include "recommender.h"
#include "seed.h"
#include "wardrobe.h"

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

TEST_CASE("gender narrows the catalog to unisex plus that gender") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 20.0;
    request.mood_slug = "confident";

    // Confident footwear: heels (0.9, female) beat classic shoes (0.8).
    request.gender = "female";
    auto outfit = Recommender(db).recommend(request);
    const auto* footwear = findCategory(outfit, "footwear");
    REQUIRE(footwear != nullptr);
    CHECK(footwear->item_slug == "heels");

    // A man never gets heels or other female-only pieces suggested; the
    // best remaining confident shoe (loafers) wins instead.
    request.gender = "male";
    outfit = Recommender(db).recommend(request);
    const auto* male_footwear = findCategory(outfit, "footwear");
    REQUIRE(male_footwear != nullptr);
    CHECK(male_footwear->item_slug == "loafers");

    // No gender set shows the full catalog.
    request.gender.clear();
    outfit = Recommender(db).recommend(request);
    const auto* any_footwear = findCategory(outfit, "footwear");
    REQUIRE(any_footwear != nullptr);
    CHECK(any_footwear->item_slug == "heels");
}

TEST_CASE("formalityAdjustment") {
    CHECK_THAT(negiysem::formalityAdjustment(3, -1), WithinAbs(0.0, 1e-9));  // no target
    CHECK_THAT(negiysem::formalityAdjustment(3, 3), WithinAbs(0.0, 1e-9));
    CHECK_THAT(negiysem::formalityAdjustment(0, 4), WithinAbs(-0.6, 1e-9));
    CHECK_THAT(negiysem::formalityAdjustment(5, 3), WithinAbs(-0.3, 1e-9));
}

TEST_CASE("occasion formality steers the pick") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.temperature_c = 18.0;
    request.gender = "male";  // keeps footwear ties deterministic

    // A sporty occasion favors formality-0 shoes over sneakers (1).
    request.formality_target = 0;
    auto outfit = Recommender(db).recommend(request);
    const auto* sporty = findCategory(outfit, "footwear");
    REQUIRE(sporty != nullptr);
    CHECK(sporty->item_slug == "running-shoes");

    // A special occasion favors formality-4 classic shoes.
    request.formality_target = 4;
    outfit = Recommender(db).recommend(request);
    const auto* formal = findCategory(outfit, "footwear");
    REQUIRE(formal != nullptr);
    CHECK(formal->item_slug == "classic-shoes");
}

namespace {

RecommendedItem piece(const std::string& slug, const std::string& category, double score,
                      std::vector<std::string> values = {}, int formality = 2) {
    RecommendedItem item;
    item.item_slug = slug;
    item.item_name = slug;
    item.category_slug = category;
    item.score = score;
    item.value_slugs = std::move(values);
    item.formality = formality;
    return item;
}

}  // namespace

TEST_CASE("outfit harmony terms") {
    SECTION("clashing statement colors are penalized, neutrals are free") {
        const std::vector<RecommendedItem> clash = {
            piece("t-shirt", "top", 1.0, {"red"}),
            piece("chinos", "bottom", 1.0, {"green"})};
        CHECK_THAT(negiysem::colorHarmony(clash), WithinAbs(-0.3, 1e-9));

        const std::vector<RecommendedItem> neutral = {
            piece("t-shirt", "top", 1.0, {"black"}),
            piece("chinos", "bottom", 1.0, {"beige", "white"})};
        CHECK_THAT(negiysem::colorHarmony(neutral), WithinAbs(0.0, 1e-9));

        const std::vector<RecommendedItem> loud = {
            piece("t-shirt", "top", 1.0, {"blue", "yellow"}),
            piece("chinos", "bottom", 1.0, {"turquoise"})};
        CHECK_THAT(negiysem::colorHarmony(loud), WithinAbs(-0.2, 1e-9));
    }
    SECTION("formality spread beyond two levels costs") {
        const std::vector<RecommendedItem> mixed = {
            piece("sweatpants", "bottom", 1.0, {}, 0),
            piece("blazer", "outerwear", 1.0, {}, 4)};
        CHECK_THAT(negiysem::formalityConsistency(mixed), WithinAbs(-0.4, 1e-9));
        const std::vector<RecommendedItem> fine = {
            piece("jeans", "bottom", 1.0, {}, 2),
            piece("shirt", "top", 1.0, {}, 3)};
        CHECK_THAT(negiysem::formalityConsistency(fine), WithinAbs(0.0, 1e-9));
    }
    SECTION("more than one bold pattern costs") {
        const std::vector<RecommendedItem> busy = {
            piece("shirt", "top", 1.0, {"striped"}),
            piece("chinos", "bottom", 1.0, {"floral"})};
        CHECK_THAT(negiysem::patternClashPenalty(busy), WithinAbs(-0.25, 1e-9));
    }
}

TEST_CASE("assembleOutfit scores combinations, not items") {
    using negiysem::OutfitCandidates;

    SECTION("a slightly weaker top wins when it avoids a color clash") {
        OutfitCandidates candidates = {
            {"top", {piece("red-tee", "top", 1.6, {"red"}),
                     piece("white-tee", "top", 1.5, {"white"})}},
            {"bottom", {piece("green-pants", "bottom", 1.5, {"green"})}},
            {"footwear", {piece("sneakers", "footwear", 1.0)}},
        };
        const auto outfit = negiysem::assembleOutfit(candidates, 0.8);
        const auto* top = findCategory(outfit, "top");
        REQUIRE(top != nullptr);
        CHECK(top->item_slug == "white-tee");  // 1.5 beats 1.6 - 0.3 clash
    }
    SECTION("a strong one-piece replaces top and bottom") {
        OutfitCandidates candidates = {
            {"top", {piece("tee", "top", 1.3)}},
            {"bottom", {piece("jeans", "bottom", 1.3)}},
            {"one-piece", {piece("dress", "one-piece", 2.0)}},
            {"footwear", {piece("sneakers", "footwear", 1.0)}},
        };
        const auto outfit = negiysem::assembleOutfit(candidates, 0.8);
        CHECK(findCategory(outfit, "one-piece") != nullptr);
        CHECK(findCategory(outfit, "top") == nullptr);
        CHECK(findCategory(outfit, "bottom") == nullptr);
    }
    SECTION("an optional layer must fit the outfit, not just score well") {
        OutfitCandidates candidates = {
            {"top", {piece("tee", "top", 1.5, {}, 1)}},
            {"bottom", {piece("jeans", "bottom", 1.5, {}, 1)}},
            {"footwear", {piece("sneakers", "footwear", 1.0, {}, 1)}},
            {"accessory", {piece("tie", "accessory", 0.9, {}, 5)}},
        };
        auto outfit = negiysem::assembleOutfit(candidates, 0.8);
        CHECK(findCategory(outfit, "accessory") == nullptr);  // formality clash

        candidates["accessory"] = {piece("scarf", "accessory", 0.9, {}, 2)};
        outfit = negiysem::assembleOutfit(candidates, 0.8);
        CHECK(findCategory(outfit, "accessory") != nullptr);
    }
}

TEST_CASE("unknown mood throws") {
    Database db = makeSeededDb();
    RecommendationRequest request;
    request.mood_slug = "hangry";
    CHECK_THROWS(Recommender(db).recommend(request));
}

TEST_CASE("pairAffinityBonus rewards pieces that shone together") {
    using negiysem::PairAffinities;
    const std::vector<RecommendedItem> outfit = {
        piece("t-shirt", "top", 1.0),
        piece("jeans", "bottom", 1.0),
        piece("sneakers", "footwear", 1.0)};

    SECTION("no memory, no bonus") {
        CHECK_THAT(negiysem::pairAffinityBonus(outfit, {}), WithinAbs(0.0, 1e-9));
    }
    SECTION("0.05 per co-occurrence, order-independent") {
        const PairAffinities memory = {{{"jeans", "t-shirt"}, 2}};
        CHECK_THAT(negiysem::pairAffinityBonus(outfit, memory), WithinAbs(0.1, 1e-9));
    }
    SECTION("caps at 0.15 per pair, sums across pairs") {
        const PairAffinities memory = {{{"jeans", "t-shirt"}, 10},
                                       {{"jeans", "sneakers"}, 1}};
        CHECK_THAT(negiysem::pairAffinityBonus(outfit, memory),
                   WithinAbs(0.15 + 0.05, 1e-9));
    }
}

TEST_CASE("assembleOutfit prefers a remembered pair over a slightly better item") {
    negiysem::OutfitCandidates candidates;
    candidates["top"] = {piece("blazer", "top", 1.0), piece("t-shirt", "top", 0.92)};
    candidates["bottom"] = {piece("jeans", "bottom", 1.0)};

    // Without memory the blazer wins on raw score...
    const auto plain = negiysem::assembleOutfit(candidates, 0.8);
    CHECK(findCategory(plain, "top")->item_slug == "blazer");

    // ...but a well-rated t-shirt+jeans history flips the choice.
    const negiysem::PairAffinities memory = {{{"jeans", "t-shirt"}, 3}};
    const auto remembered = negiysem::assembleOutfit(candidates, 0.8, memory);
    CHECK(findCategory(remembered, "top")->item_slug == "t-shirt");
}

TEST_CASE("varietyAdjustment: recent wears step back, new pieces step up") {
    CHECK_THAT(negiysem::varietyAdjustment(std::nullopt), WithinAbs(0.05, 1e-9));
    CHECK_THAT(negiysem::varietyAdjustment(0.2), WithinAbs(-0.3, 1e-9));  // today
    CHECK_THAT(negiysem::varietyAdjustment(3.0), WithinAbs(-0.1, 1e-9));
    CHECK_THAT(negiysem::varietyAdjustment(30.0), WithinAbs(-0.01, 1e-9));
}

TEST_CASE("dirty pieces sit out, and wearing one just now demotes it") {
    negiysem::Database db = []() {
        negiysem::Database d(":memory:");
        d.initSchema();
        negiysem::seedDatabase(d);
        return d;
    }();
    negiysem::WardrobeRepository wardrobe(db);
    const int tee_a = wardrobe.addItem("t-shirt", "tee A", {"white"});
    const int tee_b = wardrobe.addItem("t-shirt", "tee B", {"white"});
    wardrobe.addItem("jeans", "", {"blue"});

    negiysem::RecommendationRequest request;
    request.temperature_c = 22.0;
    const negiysem::Recommender recommender(db);

    // Wearing tee A today pushes it below the otherwise identical tee B.
    wardrobe.recordWear(tee_a);
    auto candidates = recommender.candidatesFromWardrobe(request, 3);
    REQUIRE(candidates["top"].size() == 2);
    CHECK(candidates["top"][0].wardrobe_id == tee_b);

    // Once its budget is gone the piece disappears from candidates entirely.
    wardrobe.recordWear(tee_a);  // tops go dirty on the second wear
    candidates = recommender.candidatesFromWardrobe(request, 3);
    REQUIRE(candidates["top"].size() == 1);
    CHECK(candidates["top"][0].wardrobe_id == tee_b);
}

TEST_CASE("assembleOutfit with an rng lets near-tied outfits take turns") {
    negiysem::OutfitCandidates candidates;
    candidates["top"] = {piece("blazer", "top", 1.0), piece("t-shirt", "top", 0.98)};
    candidates["bottom"] = {piece("jeans", "bottom", 1.0)};

    std::set<std::string> tops_seen;
    for (unsigned seed = 0; seed < 24; ++seed) {
        std::mt19937 rng(seed);
        const auto outfit = negiysem::assembleOutfit(candidates, 0.8, {}, &rng);
        tops_seen.insert(findCategory(outfit, "top")->item_slug);
    }
    CHECK(tops_seen.size() == 2);  // both near-tied tops got their day

    // A distant runner-up (outside the tolerance) never wins.
    candidates["top"][1].score = 0.5;
    tops_seen.clear();
    for (unsigned seed = 0; seed < 24; ++seed) {
        std::mt19937 rng(seed);
        const auto outfit = negiysem::assembleOutfit(candidates, 0.8, {}, &rng);
        tops_seen.insert(findCategory(outfit, "top")->item_slug);
    }
    CHECK(tops_seen == std::set<std::string>{"blazer"});
}

TEST_CASE("styleAdjustment seasons scores with the learned profile") {
    using negiysem::styleAdjustment;
    const std::map<std::string, double> prefs = {{"navy", 0.8}, {"yellow", -1.0}};
    CHECK_THAT(styleAdjustment({"navy"}, prefs), WithinAbs(0.08, 1e-9));
    CHECK_THAT(styleAdjustment({"yellow", "navy"}, prefs), WithinAbs(-0.02, 1e-9));
    CHECK_THAT(styleAdjustment({"gray"}, prefs), WithinAbs(0.0, 1e-9));
    CHECK_THAT(styleAdjustment({"navy"}, {}), WithinAbs(0.0, 1e-9));
    // Clamped: taste never overrules weather outright.
    const std::map<std::string, double> extreme = {{"navy", 9.0}};
    CHECK_THAT(styleAdjustment({"navy"}, extreme), WithinAbs(0.3, 1e-9));
}
