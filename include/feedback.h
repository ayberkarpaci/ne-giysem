#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "recommender.h"

namespace negiysem {

class Database;

// The stored context of a served outfit, enough to rebuild the request for
// a retry after negative feedback.
struct StoredRecommendation {
    int id = 0;
    std::string source;      // 'catalog' | 'wardrobe'
    std::string mood_slug;
    double temperature_c = 0.0;
    bool is_raining = false;
    std::string lang;
    std::vector<std::string> item_slugs;  // the outfit's catalog type slugs
};

// One piece of a saved outfit, resolved against the wardrobe when the
// piece came from there (a deleted wardrobe item keeps its name only).
struct SavedOutfitItem {
    std::string item_slug;
    std::string item_name;      // localized type name, or the user's label
    std::string category_slug;
    int wardrobe_id = 0;
    std::string photo_path;
    std::string cutout_path;
};

// A saved outfit for the lookbook: the pieces plus the stylist's reason.
struct SavedOutfit {
    int id = 0;
    std::string created_at;
    std::string source;
    std::string mood_slug;
    std::string explanation;
    std::vector<SavedOutfitItem> items;
};

// Persists served outfits and the user's ratings on top of the shared
// Database, and aggregates them so the recommender can learn.
class FeedbackRepository {
public:
    explicit FeedbackRepository(Database& db) : db_(db) {}

    // Records a served outfit; returns its id for later feedback.
    int recordRecommendation(const RecommendationRequest& request,
                             const std::vector<RecommendedItem>& outfit,
                             const std::string& source,
                             const std::string& explanation = "");

    // The most recent saved outfits (newest first), for the lookbook's
    // OUTFITS tab. Item names prefer the user's label, localized type
    // names otherwise.
    std::vector<SavedOutfit> listOutfits(const std::string& lang, int limit) const;

    // Stores a 1-5 rating with an optional comment and optional one-tap
    // reason tags. Returns false when the recommendation id is unknown;
    // throws on out-of-range ratings or unknown tags.
    bool addFeedback(int recommendation_id, int rating, const std::string& comment,
                     const std::vector<std::string>& tags = {});

    // The reason tags collected across all feedback on a recommendation.
    std::vector<std::string> tagsFor(int recommendation_id) const;

    // The allowed one-tap reason slugs, e.g. "too-hot", "colors-clash".
    static const std::vector<std::string>& allowedTags();

    // The stored context of a recommendation, or nullopt when unknown.
    std::optional<StoredRecommendation> recommendation(int id) const;

    // Average rating per item slug over all feedback, for score adjustments.
    std::map<std::string, double> averageRatings() const;

private:
    Database& db_;
};

}  // namespace negiysem
