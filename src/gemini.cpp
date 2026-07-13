#include "gemini.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <ctime>
#include <sstream>
#include <stdexcept>

namespace negiysem {

using nlohmann::json;

namespace {

const std::vector<std::string> kMoods = {"energetic", "cozy", "confident", "relaxed",
                                         "adventurous"};
const std::vector<std::string> kOccasions = {"work", "casual", "sport", "date", "special"};
const std::vector<std::string> kColors = {"black", "white", "gray",   "navy", "blue", "red",
                                          "green", "beige", "brown",  "yellow", "pink", "purple"};
const std::vector<std::string> kPatterns = {"solid",     "striped", "plaid",     "floral",
                                            "polka-dot", "graphic", "camouflage"};

std::string joinQuoted(const std::vector<std::string>& values) {
    std::ostringstream out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << ", ";
        out << '"' << values[i] << '"';
    }
    return out.str();
}

bool contains(const std::vector<std::string>& values, const std::string& v) {
    return std::find(values.begin(), values.end(), v) != values.end();
}

// Gemini sometimes wraps JSON in markdown fences despite instructions.
std::string stripCodeFences(std::string text) {
    const auto first_brace = text.find('{');
    const auto last_brace = text.rfind('}');
    if (first_brace != std::string::npos && last_brace != std::string::npos &&
        last_brace > first_brace) {
        return text.substr(first_brace, last_brace - first_brace + 1);
    }
    return text;
}

}  // namespace

std::string buildParsePrompt(const std::string& user_text, const std::string& today_iso,
                             const std::string& weekday) {
    std::ostringstream prompt;
    prompt << "You convert a user's outfit request (any language) into filters for an outfit "
              "recommender. Today is "
           << today_iso << " (" << weekday << ").\n"
           << "Reply with ONLY a JSON object, no markdown, matching exactly:\n"
           << "{\n"
           << "  \"day_offset\": <int 0-6; 0=today, 1=tomorrow, ...>,\n"
           << "  \"mood\": <one of [" << joinQuoted(kMoods) << "] or null>,\n"
           << "  \"occasion\": <one of [" << joinQuoted(kOccasions) << "] or null>,\n"
           << "  \"colors_preferred\": <array, subset of [" << joinQuoted(kColors) << "]>,\n"
           << "  \"colors_avoided\": <same vocabulary>,\n"
           << "  \"patterns_preferred\": <array, subset of [" << joinQuoted(kPatterns) << "]>,\n"
           << "  \"patterns_avoided\": <same vocabulary>\n"
           << "}\n"
           << "Use null/empty when the request does not say. User request:\n"
           << user_text;
    return prompt.str();
}

std::string buildMoodPrompt(const std::string& user_text) {
    std::ostringstream prompt;
    prompt << "The user was asked how they feel today and answered (any language):\n\""
           << user_text << "\"\n"
           << "Map their state of mind to weights over exactly these moods: ["
           << joinQuoted(kMoods) << "].\n"
           << "Reply with ONLY a JSON object, no markdown, matching exactly:\n"
           << "{\"moods\": {\"<mood>\": <weight 0..1>, ...}}\n"
           << "Use 1-3 moods that genuinely apply, weights summing to about 1. "
              "Use an empty object if the answer says nothing about how they feel.";
    return prompt.str();
}

std::string extractGeminiText(const std::string& api_response_json) {
    const json j = json::parse(api_response_json, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        throw std::runtime_error("gemini returned malformed JSON: " + api_response_json);
    }
    if (j.contains("error")) {
        throw std::runtime_error("gemini error: " +
                                 j["error"].value("message", api_response_json));
    }
    try {
        // Thinking models may split the answer across several parts (and mark
        // internal reasoning with "thought": true) — concatenate the real text.
        std::string text;
        for (const auto& part : j.at("candidates").at(0).at("content").at("parts")) {
            if (part.value("thought", false)) continue;
            if (part.contains("text")) text += part["text"].get<std::string>();
        }
        if (text.empty()) {
            throw std::runtime_error("gemini returned no text: " + api_response_json);
        }
        return text;
    } catch (const json::exception&) {
        throw std::runtime_error("unexpected gemini response shape: " + api_response_json);
    }
}

ParsedRequest parseParsedRequestJson(const std::string& text) {
    const json j = json::parse(stripCodeFences(text), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) {
        throw std::runtime_error("could not parse extracted request: " + text);
    }
    ParsedRequest parsed;
    if (j.contains("day_offset") && j["day_offset"].is_number_integer()) {
        parsed.day_offset = std::clamp(j["day_offset"].get<int>(), 0, 6);
    }
    if (j.contains("mood") && j["mood"].is_string() && contains(kMoods, j["mood"])) {
        parsed.mood_slug = j["mood"];
    }
    if (j.contains("occasion") && j["occasion"].is_string() &&
        contains(kOccasions, j["occasion"])) {
        parsed.occasion = j["occasion"];
    }
    const auto readSlugs = [&](const char* key, const std::vector<std::string>& vocabulary,
                               std::vector<std::string>& out) {
        if (!j.contains(key) || !j[key].is_array()) return;
        for (const auto& c : j[key]) {
            if (c.is_string() && contains(vocabulary, c)) out.push_back(c);
        }
    };
    readSlugs("colors_preferred", kColors, parsed.colors_preferred);
    readSlugs("colors_avoided", kColors, parsed.colors_avoided);
    readSlugs("patterns_preferred", kPatterns, parsed.patterns_preferred);
    readSlugs("patterns_avoided", kPatterns, parsed.patterns_avoided);
    return parsed;
}

MoodWeights parseMoodWeightsJson(const std::string& text) {
    const json j = json::parse(stripCodeFences(text), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object() || !j.contains("moods") || !j["moods"].is_object()) {
        throw std::runtime_error("could not parse mood analysis: " + text);
    }
    MoodWeights weights;
    for (const auto& [slug, value] : j["moods"].items()) {
        if (!contains(kMoods, slug) || !value.is_number()) continue;
        const double weight = std::clamp(value.get<double>(), 0.0, 1.0);
        if (weight > 0.0) weights.emplace_back(slug, weight);
    }
    std::sort(weights.begin(), weights.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    return weights;
}

GeminiClient::GeminiClient(std::string api_key, std::string model)
    : api_key_(std::move(api_key)), model_(std::move(model)) {}

std::string GeminiClient::generate(const std::string& prompt, bool json_response) const {
    json body = {
        {"contents", json::array({json{{"parts", json::array({json{{"text", prompt}}})}}})},
    };
    if (json_response) {
        body["generationConfig"] = {{"responseMimeType", "application/json"}};
    }
    cpr::Response r = cpr::Post(
        cpr::Url{"https://generativelanguage.googleapis.com/v1beta/models/" + model_ +
                 ":generateContent"},
        cpr::Header{{"Content-Type", "application/json"}, {"x-goog-api-key", api_key_}},
        cpr::Body{body.dump()}, cpr::Timeout{20000});
    if (r.status_code == 0) {
        throw std::runtime_error("gemini request failed: " + r.error.message);
    }
    return extractGeminiText(r.text);
}

ParsedRequest GeminiClient::parseUserRequest(const std::string& user_text) const {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char date[16];
    std::strftime(date, sizeof(date), "%Y-%m-%d", &local);
    char weekday[16];
    std::strftime(weekday, sizeof(weekday), "%A", &local);

    return parseParsedRequestJson(
        generate(buildParsePrompt(user_text, date, weekday), /*json_response=*/true));
}

MoodWeights GeminiClient::analyzeMood(const std::string& user_text) const {
    return parseMoodWeightsJson(generate(buildMoodPrompt(user_text), /*json_response=*/true));
}

std::string GeminiClient::explainOutfit(const std::string& user_text,
                                        const std::vector<RecommendedItem>& outfit,
                                        const Weather& weather,
                                        const std::string& lang) const {
    std::ostringstream prompt;
    prompt << "The user asked: \"" << user_text << "\"\n"
           << "Weather that day: " << weather.temperature_c << " C, "
           << (weather.is_raining ? "wet" : "dry") << ".\n"
           << "We picked this outfit:";
    for (const auto& item : outfit) {
        prompt << " " << item.item_name << " (" << item.category_slug << ");";
    }
    prompt << "\nIn the language with BCP 47 code '" << lang
           << "', write ONE warm, natural sentence (max 25 words) telling the user why this "
              "outfit fits their plan and the weather. Reply with only that sentence.";
    return generate(prompt.str(), /*json_response=*/false);
}

}  // namespace negiysem
