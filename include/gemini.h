#pragma once

#include <map>
#include <string>
#include <utility>
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

// A mood blend, e.g. {{"cozy", 0.7}, {"relaxed", 0.3}}, sorted by weight
// descending. Slugs come from the moods table vocabulary.
using MoodWeights = std::vector<std::pair<std::string, double>>;

// Allowed value slugs per attribute slug, e.g. {"color": {"black", ...}}.
using AttributeVocabulary = std::map<std::string, std::vector<std::string>>;

// What Gemini reads off a garment photo, in database slugs. An attribute may
// carry several values — most garments are not a single color.
struct ClassifiedGarment {
    std::string type_slug;                       // one of the clothing_items slugs
    std::map<std::string, std::vector<std::string>> values;  // attribute slug -> value slugs
};

// Pure helpers, exposed for unit testing.
std::string buildParsePrompt(const std::string& user_text, const std::string& today_iso,
                             const std::string& weekday);
std::string buildMoodPrompt(const std::string& user_text);
std::string buildClassifyPrompt(const std::vector<std::string>& type_slugs,
                                const AttributeVocabulary& attribute_values);
// What the stylist pass produces: the chosen items plus one warm sentence
// (in the requested language) saying why the combination works.
struct StyledOutfit {
    std::vector<RecommendedItem> items;
    std::string reason;  // may be empty when the model skips it
};

std::string buildStylistPrompt(const std::string& context,
                               const OutfitCandidates& candidates,
                               const Weather& weather,
                               const std::string& lang);
// The image-editing prompt that reconstructs the garment alone on a uniform
// chroma background of `chroma_hex` (e.g. "#FF00FF"). `item_hint` names the
// garment when known (e.g. "jeans") so the model extracts the right piece.
std::string buildExtractPrompt(const std::string& chroma_hex,
                               const std::string& item_hint);
std::string extractGeminiText(const std::string& api_response_json);
// The raw bytes of the first inline image in a generateContent response.
// Throws when the response carries no image.
std::string extractGeminiImage(const std::string& api_response_json);
ParsedRequest parseParsedRequestJson(const std::string& text);
MoodWeights parseMoodWeightsJson(const std::string& text);
// The stylist's numbered picks mapped back onto the candidates: at most one
// item per category, core categories (top, bottom, footwear) backfilled
// with the best candidate when the model skips them. Throws when the
// response is not parseable at all.
StyledOutfit parseStylistPicksJson(const std::string& text,
                                   const OutfitCandidates& candidates);
// Throws when no valid garment type is found; invalid values are dropped.
ClassifiedGarment parseClassifiedGarmentJson(const std::string& text,
                                             const std::vector<std::string>& type_slugs,
                                             const AttributeVocabulary& attribute_values);

// Thin REST client for the Gemini API (generativelanguage.googleapis.com).
class GeminiClient {
public:
    // "gemini-flash-latest" is Google's rolling alias for the current Flash
    // model, so the default keeps working as models are retired. The image
    // model handles the garment-extraction shots and has no rolling alias.
    explicit GeminiClient(std::string api_key, std::string model = "gemini-flash-latest",
                          std::string image_model = "gemini-2.5-flash-image");

    // Free text -> structured request. Throws std::runtime_error on API or
    // parse failure.
    ParsedRequest parseUserRequest(const std::string& user_text) const;

    // "How do you feel today?" answer -> mood blend. May be empty when the
    // text says nothing about mood; throws on API or parse failure.
    MoodWeights analyzeMood(const std::string& user_text) const;

    // Garment photo (raw bytes + content type) -> type and attribute slugs.
    // Throws on API failure or when no garment is recognized.
    ClassifiedGarment classifyGarment(const std::string& image_bytes,
                                      const std::string& mime_type,
                                      const std::vector<std::string>& type_slugs,
                                      const AttributeVocabulary& attribute_values) const;

    // Garment photo -> catalog-style shot of the empty garment on a uniform
    // background of `chroma_hex`, ready for chroma-key removal. Returns the
    // generated image bytes (PNG or JPEG). Uses the image model, not the
    // text model. Throws on API failure or when no image comes back.
    std::string extractGarmentImage(const std::string& image_bytes,
                                    const std::string& mime_type,
                                    const std::string& chroma_hex,
                                    const std::string& item_hint) const;

    // Picks the most coherent outfit from scored candidates, applying
    // fashion rules (color harmony, consistent formality) in the prompt,
    // and explains the choice in one sentence in `lang`.
    // Throws on API or parse failure — callers fall back to the top picks.
    StyledOutfit styleOutfit(const std::string& context,
                             const OutfitCandidates& candidates,
                             const Weather& weather,
                             const std::string& lang) const;

    // One friendly sentence (in `lang`) explaining the chosen outfit.
    std::string explainOutfit(const std::string& user_text,
                              const std::vector<RecommendedItem>& outfit,
                              const Weather& weather,
                              const std::string& lang) const;

private:
    std::string generate(const std::string& prompt, bool json_response) const;

    std::string api_key_;
    std::string model_;
    std::string image_model_;
};

}  // namespace negiysem
