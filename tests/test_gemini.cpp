#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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
