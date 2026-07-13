#pragma once

#include <string>
#include <vector>

#include "recommender.h"
#include "weather.h"

namespace negiysem {

// What Gemini extracts from a free-text request. Every field uses our own
// controlled vocabulary (slugs), so it maps 1:1 onto the database.
struct ParsedRequest {
    int day_offset = 0;                          // 0 = today .. 6
    std::string mood_slug;                       // one of the moods table, or empty
    std::string occasion;                        // work|casual|sport|date|special, or empty
    std::vector<std::string> colors_preferred;   // color value slugs
    std::vector<std::string> colors_avoided;
    std::vector<std::string> patterns_preferred; // pattern value slugs
    std::vector<std::string> patterns_avoided;
};

// Pure helpers, exposed for unit testing.
std::string buildParsePrompt(const std::string& user_text, const std::string& today_iso,
                             const std::string& weekday);
std::string extractGeminiText(const std::string& api_response_json);
ParsedRequest parseParsedRequestJson(const std::string& text);

// Thin REST client for the Gemini API (generativelanguage.googleapis.com).
class GeminiClient {
public:
    // "gemini-flash-latest" is Google's rolling alias for the current Flash
    // model, so the default keeps working as models are retired.
    explicit GeminiClient(std::string api_key, std::string model = "gemini-flash-latest");

    // Free text -> structured request. Throws std::runtime_error on API or
    // parse failure.
    ParsedRequest parseUserRequest(const std::string& user_text) const;

    // One friendly sentence (in `lang`) explaining the chosen outfit.
    std::string explainOutfit(const std::string& user_text,
                              const std::vector<RecommendedItem>& outfit,
                              const Weather& weather,
                              const std::string& lang) const;

private:
    std::string generate(const std::string& prompt, bool json_response) const;

    std::string api_key_;
    std::string model_;
};

}  // namespace negiysem
