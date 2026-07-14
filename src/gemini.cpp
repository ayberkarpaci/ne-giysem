#include "gemini.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <thread>

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

// Standard base64, needed to embed photo bytes in the JSON request.
std::string base64Encode(const std::string& bytes) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        unsigned int chunk = static_cast<unsigned char>(bytes[i]) << 16;
        if (i + 1 < bytes.size()) chunk |= static_cast<unsigned char>(bytes[i + 1]) << 8;
        if (i + 2 < bytes.size()) chunk |= static_cast<unsigned char>(bytes[i + 2]);
        out.push_back(kAlphabet[(chunk >> 18) & 0x3F]);
        out.push_back(kAlphabet[(chunk >> 12) & 0x3F]);
        out.push_back(i + 1 < bytes.size() ? kAlphabet[(chunk >> 6) & 0x3F] : '=');
        out.push_back(i + 2 < bytes.size() ? kAlphabet[chunk & 0x3F] : '=');
    }
    return out;
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

std::string buildClassifyPrompt(const std::vector<std::string>& type_slugs,
                                const AttributeVocabulary& attribute_values) {
    std::ostringstream prompt;
    prompt << "You classify ONE clothing item shown in a photo for a wardrobe app.\n"
           << "Garment types: [" << joinQuoted(type_slugs) << "]\n"
           << "Attributes and their allowed values:\n";
    for (const auto& [attribute, values] : attribute_values) {
        prompt << "- " << attribute << ": [" << joinQuoted(values) << "]\n";
    }
    prompt << "Reply with ONLY a JSON object, no markdown, matching exactly:\n"
           << "{\"type\": <garment type>, \"values\": {\"<attribute>\": <value, array of "
              "values, or null>, ...}}\n"
           << "The type MUST be exactly one slug from the garment types list — never "
              "invent a new one; when nothing matches perfectly, choose the closest "
              "type anyway. For \"color\" list every clearly visible color (up to 3, "
              "dominant first). For each other attribute pick the closest allowed "
              "value, or null only when it is truly not visible or not applicable.";
    return prompt.str();
}

std::string buildStylistPrompt(const std::string& context,
                               const OutfitCandidates& candidates,
                               const Weather& weather,
                               const std::string& lang) {
    std::ostringstream prompt;
    prompt << "You are a fashion stylist assembling ONE coherent outfit.\n"
           << "Situation: " << context << "\n"
           << "Weather: " << weather.temperature_c << " C, "
           << (weather.is_raining ? "wet" : "dry") << ".\n"
           << "Candidates by category, already sorted by weather/mood fit "
              "(known attributes in parentheses):\n";
    for (const auto& [category, items] : candidates) {
        prompt << "- " << category << ":";
        for (size_t i = 0; i < items.size(); ++i) {
            prompt << " " << (i + 1) << ") " << items[i].item_slug
                   << " [formality " << items[i].formality << "/5]";
            if (!items[i].value_slugs.empty()) {
                prompt << " (";
                for (size_t v = 0; v < items[i].value_slugs.size(); ++v) {
                    if (v > 0) prompt << ", ";
                    prompt << items[i].value_slugs[v];
                }
                prompt << ")";
            }
        }
        prompt << "\n";
    }
    prompt << "Rules:\n"
           << "- Pick at most one item per category, by its number.\n"
           << "- Always pick a top, a bottom and footwear when offered.\n"
           << "- EXCEPTION: a one-piece (dress, jumpsuit) replaces both top "
              "and bottom — when you pick from one-piece, pick no top and "
              "no bottom.\n"
           << "- Add outerwear, accessory or jewelry only when it genuinely "
              "completes the look for this weather and situation.\n"
           << "- Keep formality consistent across pieces (no sweatpants with "
              "a blazer, no heels with a hoodie).\n"
           << "- Coordinate colors: neutrals (black, white, gray, navy, "
              "beige, brown) go with anything; keep it to about three color "
              "families; avoid clashing saturated colors together.\n"
           << "- At most one boldly patterned piece per outfit.\n"
           << "Reply with ONLY a JSON object, no markdown, matching exactly:\n"
           << "{\"picks\": {\"<category>\": <number>, ...}, \"reason\": <string>}\n"
           << "\"reason\" is ONE warm, natural sentence (max 25 words) in the "
              "language with BCP 47 code '" << lang
           << "' telling the user why this combination works for the weather "
              "and their situation.";
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

StyledOutfit parseStylistPicksJson(const std::string& text,
                                   const OutfitCandidates& candidates) {
    const json j = json::parse(stripCodeFences(text), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object() || !j.contains("picks") ||
        !j["picks"].is_object()) {
        throw std::runtime_error("could not parse stylist picks: " + text);
    }
    std::vector<RecommendedItem> outfit;
    std::vector<std::string> picked_categories;
    for (const auto& [category, number] : j["picks"].items()) {
        const auto candidates_it = candidates.find(category);
        if (candidates_it == candidates.end() || !number.is_number_integer()) continue;
        const int index = number.get<int>();
        if (index < 1 || index > static_cast<int>(candidates_it->second.size())) continue;
        if (contains(picked_categories, category)) continue;
        outfit.push_back(candidates_it->second[index - 1]);
        picked_categories.push_back(category);
    }
    // The model must not silently drop a core category it was offered —
    // unless a one-piece was picked, which stands in for top and bottom.
    const bool has_one_piece = contains(picked_categories, "one-piece");
    for (const char* core : {"top", "bottom", "footwear"}) {
        if (has_one_piece && (std::string(core) == "top" || std::string(core) == "bottom")) {
            continue;
        }
        const auto candidates_it = candidates.find(core);
        if (candidates_it != candidates.end() && !contains(picked_categories, core)) {
            outfit.push_back(candidates_it->second.front());
        }
    }
    std::sort(outfit.begin(), outfit.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });
    StyledOutfit styled;
    styled.items = std::move(outfit);
    if (j.contains("reason") && j["reason"].is_string()) {
        styled.reason = j["reason"];
    }
    return styled;
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

ClassifiedGarment parseClassifiedGarmentJson(const std::string& text,
                                             const std::vector<std::string>& type_slugs,
                                             const AttributeVocabulary& attribute_values) {
    const json j = json::parse(stripCodeFences(text), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) {
        throw std::runtime_error("could not parse classification: " + text);
    }
    ClassifiedGarment garment;
    if (!j.contains("type") || !j["type"].is_string() ||
        !contains(type_slugs, j["type"].get<std::string>())) {
        throw std::runtime_error("no garment recognized in: " + text);
    }
    garment.type_slug = j["type"];
    if (j.contains("values") && j["values"].is_object()) {
        for (const auto& [attribute, value] : j["values"].items()) {
            const auto vocab_it = attribute_values.find(attribute);
            if (vocab_it == attribute_values.end()) continue;
            // The model may answer with one value or an array (multi-color
            // garments); normalize to a list and keep only known slugs.
            std::vector<std::string> candidates;
            if (value.is_string()) {
                candidates.push_back(value.get<std::string>());
            } else if (value.is_array()) {
                for (const auto& entry : value) {
                    if (entry.is_string()) candidates.push_back(entry.get<std::string>());
                }
            }
            for (const auto& candidate : candidates) {
                if (contains(vocab_it->second, candidate) &&
                    !contains(garment.values[attribute], candidate)) {
                    garment.values[attribute].push_back(candidate);
                }
            }
            if (garment.values[attribute].empty()) garment.values.erase(attribute);
        }
    }
    return garment;
}

namespace {

// Shared POST to generateContent; `parts` may mix text and inline image data.
// Free-tier requests hit per-minute rate limits during bulk work (HTTP 429),
// so short limits are waited out with a few increasingly patient retries
// instead of failing the whole call.
std::string postGenerate(const std::string& api_key, const std::string& model,
                         const json& parts, bool json_response) {
    json body = {
        {"contents", json::array({json{{"parts", parts}}})},
    };
    if (json_response) {
        body["generationConfig"] = {{"responseMimeType", "application/json"}};
    }

    cpr::Response r;
    for (int attempt = 0;; ++attempt) {
        r = cpr::Post(
            cpr::Url{"https://generativelanguage.googleapis.com/v1beta/models/" + model +
                     ":generateContent"},
            cpr::Header{{"Content-Type", "application/json"}, {"x-goog-api-key", api_key}},
            cpr::Body{body.dump()}, cpr::Timeout{60000});
        if (r.status_code == 0) {
            throw std::runtime_error("gemini request failed: " + r.error.message);
        }
        const bool retryable = r.status_code == 429 || r.status_code == 503;
        if (!retryable || attempt >= 3) break;
        std::this_thread::sleep_for(std::chrono::seconds(10LL << attempt));  // 10s, 20s, 40s
    }
    if (r.status_code == 429) {
        throw std::runtime_error("gemini rate limit exceeded (HTTP 429): " + r.text);
    }
    return extractGeminiText(r.text);
}

}  // namespace

GeminiClient::GeminiClient(std::string api_key, std::string model)
    : api_key_(std::move(api_key)), model_(std::move(model)) {}

std::string GeminiClient::generate(const std::string& prompt, bool json_response) const {
    return postGenerate(api_key_, model_, json::array({json{{"text", prompt}}}), json_response);
}

ClassifiedGarment GeminiClient::classifyGarment(const std::string& image_bytes,
                                                const std::string& mime_type,
                                                const std::vector<std::string>& type_slugs,
                                                const AttributeVocabulary& attribute_values) const {
    const json parts = json::array({
        json{{"text", buildClassifyPrompt(type_slugs, attribute_values)}},
        json{{"inlineData", {{"mimeType", mime_type}, {"data", base64Encode(image_bytes)}}}},
    });
    return parseClassifiedGarmentJson(postGenerate(api_key_, model_, parts, /*json_response=*/true),
                                      type_slugs, attribute_values);
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

StyledOutfit GeminiClient::styleOutfit(const std::string& context,
                                       const OutfitCandidates& candidates,
                                       const Weather& weather,
                                       const std::string& lang) const {
    return parseStylistPicksJson(
        generate(buildStylistPrompt(context, candidates, weather, lang),
                 /*json_response=*/true),
        candidates);
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
