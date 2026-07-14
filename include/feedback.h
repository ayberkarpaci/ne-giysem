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

// Persists served outfits and the user's ratings on top of the shared
// Database, and aggregates them so the recommender can learn.
class FeedbackRepository {
public:
    explicit FeedbackRepository(Database& db) : db_(db) {}

    // Records a served outfit; returns its id for later feedback.
    int recordRecommendation(const RecommendationRequest& request,
                             const std::vector<RecommendedItem>& outfit,
                             const std::string& source);

    // Stores a 1-5 rating with an optional comment. Returns false when the
    // recommendation id is unknown; throws on out-of-range ratings.
    bool addFeedback(int recommendation_id, int rating, const std::string& comment);

    // The stored context of a recommendation, or nullopt when unknown.
    std::optional<StoredRecommendation> recommendation(int id) const;

    // Average rating per item slug over all feedback, for score adjustments.
    std::map<std::string, double> averageRatings() const;

private:
    Database& db_;
};

}  // namespace negiysem
