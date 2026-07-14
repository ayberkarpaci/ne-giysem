#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>

#include "gemini.h"
#include "weather.h"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("buildParsePrompt embeds the date and controlled vocabulary") {
    const auto prompt = negiysem::buildParsePrompt("yarın işe gideceğim", "2026-07-13", "Monday");
    CHECK_THAT(prompt, ContainsSubstring("2026-07-13"));
    CHECK_THAT(prompt, ContainsSubstring("Monday"));
    CHECK_THAT(prompt, ContainsSubstring("\"cozy\""));
    CHECK_THAT(prompt, ContainsSubstring("\"navy\""));
    CHECK_THAT(prompt, ContainsSubstring("\"striped\""));
    CHECK_THAT(prompt, ContainsSubstring("yarın işe gideceğim"));
}

TEST_CASE("buildMoodPrompt embeds the mood vocabulary and the answer") {
    const auto prompt = negiysem::buildMoodPrompt("yorgunum ama mutluyum");
    CHECK_THAT(prompt, ContainsSubstring("\"energetic\""));
    CHECK_THAT(prompt, ContainsSubstring("\"adventurous\""));
    CHECK_THAT(prompt, ContainsSubstring("yorgunum ama mutluyum"));
}

TEST_CASE("parseMoodWeightsJson") {
    SECTION("reads weights sorted by weight descending") {
        const auto weights = negiysem::parseMoodWeightsJson(
            R"({"moods":{"relaxed":0.3,"cozy":0.7}})");
        REQUIRE(weights.size() == 2);
        CHECK(weights[0].first == "cozy");
        CHECK(weights[0].second == 0.7);
        CHECK(weights[1].first == "relaxed");
        CHECK(weights[1].second == 0.3);
    }
    SECTION("drops unknown moods and clamps out-of-range weights") {
        const auto weights = negiysem::parseMoodWeightsJson(
            R"({"moods":{"hangry":0.9,"cozy":1.4,"relaxed":0.0}})");
        REQUIRE(weights.size() == 1);
        CHECK(weights[0].first == "cozy");
        CHECK(weights[0].second == 1.0);
    }
    SECTION("an empty object means the text said nothing about mood") {
        CHECK(negiysem::parseMoodWeightsJson(R"({"moods":{}})").empty());
    }
    SECTION("markdown fences are tolerated") {
        const auto weights = negiysem::parseMoodWeightsJson(
            "```json\n{\"moods\":{\"energetic\":1.0}}\n```");
        REQUIRE(weights.size() == 1);
        CHECK(weights[0].first == "energetic");
    }
    SECTION("rejects malformed responses") {
        CHECK_THROWS(negiysem::parseMoodWeightsJson("not json"));
        CHECK_THROWS(negiysem::parseMoodWeightsJson(R"({"no_moods_key":1})"));
        CHECK_THROWS(negiysem::parseMoodWeightsJson(R"({"moods":[1,2]})"));
    }
}

TEST_CASE("garment classification") {
    const std::vector<std::string> types = {"t-shirt", "jeans", "jersey"};
    const negiysem::AttributeVocabulary vocabulary = {
        {"color", {"black", "red"}},
        {"pattern", {"solid", "striped"}},
    };

    SECTION("buildClassifyPrompt embeds both vocabularies") {
        const auto prompt = negiysem::buildClassifyPrompt(types, vocabulary);
        CHECK_THAT(prompt, ContainsSubstring("\"jersey\""));
        CHECK_THAT(prompt, ContainsSubstring("color: [\"black\", \"red\"]"));
        CHECK_THAT(prompt, ContainsSubstring("\"striped\""));
    }
    SECTION("parses type and values") {
        const auto garment = negiysem::parseClassifiedGarmentJson(
            R"({"type":"jersey","values":{"color":"red","pattern":"striped"}})",
            types, vocabulary);
        CHECK(garment.type_slug == "jersey");
        REQUIRE(garment.values.size() == 2);
        CHECK(garment.values.at("color") == std::vector<std::string>{"red"});
        CHECK(garment.values.at("pattern") == std::vector<std::string>{"striped"});
    }
    SECTION("accepts several colors and drops unknown or repeated ones") {
        const auto garment = negiysem::parseClassifiedGarmentJson(
            R"({"type":"jersey","values":{"color":["red","black","red","neon"]}})",
            types, vocabulary);
        CHECK(garment.values.at("color") == std::vector<std::string>({"red", "black"}));
    }
    SECTION("drops null, unknown and out-of-vocabulary values") {
        const auto garment = negiysem::parseClassifiedGarmentJson(
            R"({"type":"jeans","values":{"color":null,"pattern":"tie-dye","era":"90s"}})",
            types, vocabulary);
        CHECK(garment.type_slug == "jeans");
        CHECK(garment.values.empty());
    }
    SECTION("throws when the type is missing or unknown") {
        CHECK_THROWS(negiysem::parseClassifiedGarmentJson(
            R"({"type":"spaceship","values":{}})", types, vocabulary));
        CHECK_THROWS(negiysem::parseClassifiedGarmentJson(R"({"values":{}})", types, vocabulary));
        CHECK_THROWS(negiysem::parseClassifiedGarmentJson("not json", types, vocabulary));
    }
    SECTION("markdown fences are tolerated") {
        const auto garment = negiysem::parseClassifiedGarmentJson(
            "```json\n{\"type\":\"t-shirt\"}\n```", types, vocabulary);
        CHECK(garment.type_slug == "t-shirt");
    }
}

namespace {

negiysem::RecommendedItem candidate(const std::string& slug, const std::string& category,
                                    double score,
                                    std::vector<std::string> values = {}) {
    negiysem::RecommendedItem item;
    item.item_slug = slug;
    item.item_name = slug;
    item.category_slug = category;
    item.score = score;
    item.value_slugs = std::move(values);
    return item;
}

negiysem::OutfitCandidates stylistCandidates() {
    return {
        {"top", {candidate("t-shirt", "top", 1.6, {"gray", "solid"}),
                 candidate("shirt", "top", 1.5, {"white"})}},
        {"bottom", {candidate("jeans", "bottom", 1.2, {"navy"})}},
        {"footwear", {candidate("sneakers", "footwear", 1.0)}},
        {"accessory", {candidate("cap", "accessory", 0.9)}},
    };
}

}  // namespace

TEST_CASE("stylist prompt and picks") {
    const auto candidates = stylistCandidates();

    SECTION("buildStylistPrompt numbers candidates and includes attributes") {
        const auto prompt =
            negiysem::buildStylistPrompt("dinner date", candidates, {18.0, false}, "tr");
        CHECK_THAT(prompt, ContainsSubstring("dinner date"));
        CHECK_THAT(prompt, ContainsSubstring("1) t-shirt (gray, solid)"));
        CHECK_THAT(prompt, ContainsSubstring("2) shirt (white)"));
        CHECK_THAT(prompt, ContainsSubstring("1) sneakers"));
        CHECK_THAT(prompt, ContainsSubstring("formality"));
        CHECK_THAT(prompt, ContainsSubstring("'tr'"));
    }
    SECTION("valid picks map back to the items, with the reason") {
        const auto styled = negiysem::parseStylistPicksJson(
            R"({"picks":{"top":2,"bottom":1,"footwear":1},"reason":"crisp and clean"})",
            candidates);
        REQUIRE(styled.items.size() == 3);
        CHECK(styled.items[0].item_slug == "shirt");  // sorted by score
        CHECK(styled.items[1].item_slug == "jeans");
        CHECK(styled.items[2].item_slug == "sneakers");
        CHECK(styled.reason == "crisp and clean");
    }
    SECTION("skipped optional categories stay out; reason may be missing") {
        const auto styled = negiysem::parseStylistPicksJson(
            R"({"picks":{"top":1,"bottom":1,"footwear":1}})", candidates);
        for (const auto& item : styled.items) {
            CHECK(item.category_slug != "accessory");
        }
        CHECK(styled.reason.empty());
    }
    SECTION("core categories are backfilled when dropped or out of range") {
        const auto styled = negiysem::parseStylistPicksJson(
            R"({"picks":{"top":99,"accessory":1}})", candidates);
        std::vector<std::string> categories;
        for (const auto& item : styled.items) categories.push_back(item.category_slug);
        CHECK(std::count(categories.begin(), categories.end(), "top") == 1);
        CHECK(std::count(categories.begin(), categories.end(), "bottom") == 1);
        CHECK(std::count(categories.begin(), categories.end(), "footwear") == 1);
        // The out-of-range pick fell back to the best top.
        const auto top = std::find_if(styled.items.begin(), styled.items.end(),
                                      [](const auto& i) {
            return i.category_slug == "top";
        });
        CHECK(top->item_slug == "t-shirt");
    }
    SECTION("a one-piece pick suppresses the top and bottom backfill") {
        auto with_dress = candidates;
        with_dress["one-piece"] = {candidate("dress", "one-piece", 1.4, {"burgundy"})};
        const auto styled = negiysem::parseStylistPicksJson(
            R"({"picks":{"one-piece":1,"footwear":1}})", with_dress);
        std::vector<std::string> categories;
        for (const auto& item : styled.items) categories.push_back(item.category_slug);
        CHECK(std::count(categories.begin(), categories.end(), "one-piece") == 1);
        CHECK(std::count(categories.begin(), categories.end(), "top") == 0);
        CHECK(std::count(categories.begin(), categories.end(), "bottom") == 0);
        CHECK(std::count(categories.begin(), categories.end(), "footwear") == 1);
    }
    SECTION("unparseable responses throw") {
        CHECK_THROWS(negiysem::parseStylistPicksJson("not json", candidates));
        CHECK_THROWS(negiysem::parseStylistPicksJson(R"({"no_picks":1})", candidates));
    }
}

TEST_CASE("extractGeminiText") {
    SECTION("pulls the first candidate's text") {
        const auto text = negiysem::extractGeminiText(
            R"({"candidates":[{"content":{"parts":[{"text":"hello"}]}}]})");
        CHECK(text == "hello");
    }
    SECTION("concatenates split parts and skips thoughts") {
        const auto text = negiysem::extractGeminiText(
            R"({"candidates":[{"content":{"parts":[
                {"text":"internal reasoning","thought":true},
                {"text":"{\"a\":"},
                {"text":" 1}"}]}}]})");
        CHECK(text == "{\"a\": 1}");
    }
    SECTION("only thoughts means no text") {
        CHECK_THROWS(negiysem::extractGeminiText(
            R"({"candidates":[{"content":{"parts":[{"text":"hmm","thought":true}]}}]})"));
    }
    SECTION("surfaces API errors") {
        CHECK_THROWS(negiysem::extractGeminiText(
            R"({"error":{"message":"API key not valid"}})"));
    }
    SECTION("rejects malformed responses") {
        CHECK_THROWS(negiysem::extractGeminiText("not json"));
        CHECK_THROWS(negiysem::extractGeminiText(R"({"candidates":[]})"));
    }
}

TEST_CASE("parseParsedRequestJson") {
    SECTION("full response") {
        const auto parsed = negiysem::parseParsedRequestJson(
            R"({"day_offset":1,"mood":"confident","occasion":"work",
                "colors_preferred":["navy"],"colors_avoided":["pink"],
                "patterns_preferred":["striped"],"patterns_avoided":["floral"]})");
        CHECK(parsed.day_offset == 1);
        CHECK(parsed.mood_slug == "confident");
        CHECK(parsed.occasion == "work");
        REQUIRE(parsed.colors_preferred.size() == 1);
        CHECK(parsed.colors_preferred[0] == "navy");
        REQUIRE(parsed.colors_avoided.size() == 1);
        CHECK(parsed.colors_avoided[0] == "pink");
        REQUIRE(parsed.patterns_preferred.size() == 1);
        CHECK(parsed.patterns_preferred[0] == "striped");
        REQUIRE(parsed.patterns_avoided.size() == 1);
        CHECK(parsed.patterns_avoided[0] == "floral");
    }
    SECTION("nulls become defaults") {
        const auto parsed = negiysem::parseParsedRequestJson(
            R"({"day_offset":0,"mood":null,"occasion":null,
                "colors_preferred":[],"colors_avoided":[]})");
        CHECK(parsed.day_offset == 0);
        CHECK(parsed.mood_slug.empty());
        CHECK(parsed.occasion.empty());
    }
    SECTION("out-of-range day offset is clamped") {
        CHECK(negiysem::parseParsedRequestJson(R"({"day_offset":42})").day_offset == 6);
        CHECK(negiysem::parseParsedRequestJson(R"({"day_offset":-3})").day_offset == 0);
    }
    SECTION("values outside the vocabulary are dropped") {
        const auto parsed = negiysem::parseParsedRequestJson(
            R"({"mood":"hangry","occasion":"moon-landing",
                "colors_preferred":["navy","chartreuse"],
                "patterns_preferred":["striped","tie-dye"]})");
        CHECK(parsed.mood_slug.empty());
        CHECK(parsed.occasion.empty());
        REQUIRE(parsed.colors_preferred.size() == 1);
        CHECK(parsed.colors_preferred[0] == "navy");
        REQUIRE(parsed.patterns_preferred.size() == 1);
        CHECK(parsed.patterns_preferred[0] == "striped");
    }
    SECTION("markdown fences are tolerated") {
        const auto parsed = negiysem::parseParsedRequestJson(
            "```json\n{\"day_offset\":2}\n```");
        CHECK(parsed.day_offset == 2);
    }
}

TEST_CASE("parseOpenMeteoDailyResponse") {
    const std::string body = R"({"daily":{
        "temperature_2m_max":[30.0,24.0,18.0],
        "temperature_2m_min":[20.0,16.0,10.0],
        "precipitation_sum":[0.0,3.2,0.0],
        "weather_code":[1,61,3]}})";
    SECTION("averages min and max for the requested day") {
        const auto w = negiysem::parseOpenMeteoDailyResponse(body, 0);
        CHECK(w.temperature_c == 25.0);
        CHECK_FALSE(w.is_raining);
    }
    SECTION("rainy forecast day") {
        const auto w = negiysem::parseOpenMeteoDailyResponse(body, 1);
        CHECK(w.temperature_c == 20.0);
        CHECK(w.is_raining);
    }
    SECTION("day beyond the forecast throws") {
        CHECK_THROWS(negiysem::parseOpenMeteoDailyResponse(body, 5));
    }
}

TEST_CASE("preferenceAdjustment") {
    using negiysem::preferenceAdjustment;
    CHECK(preferenceAdjustment({"navy"}, {"navy"}, {}) == 0.3);
    CHECK(preferenceAdjustment({"navy"}, {}, {"navy"}) == -0.5);
    CHECK(preferenceAdjustment({"navy", "pink"}, {"navy"}, {"pink"}) == -0.5);  // avoided wins
    CHECK(preferenceAdjustment({"gray"}, {"navy"}, {"pink"}) == 0.0);
    CHECK(preferenceAdjustment({}, {"navy"}, {"pink"}) == 0.0);
    CHECK(preferenceAdjustment({"striped"}, {"striped"}, {}) == 0.3);  // patterns work the same
}
