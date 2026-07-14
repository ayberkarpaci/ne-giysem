#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "weather.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("parseIpApiResponse") {
    SECTION("successful response") {
        const auto loc = negiysem::parseIpApiResponse(
            R"({"status":"success","lat":41.01,"lon":28.95,"city":"Istanbul"})");
        CHECK_THAT(loc.latitude, WithinAbs(41.01, 1e-9));
        CHECK_THAT(loc.longitude, WithinAbs(28.95, 1e-9));
        CHECK(loc.city == "Istanbul");
    }
    SECTION("failure status throws") {
        CHECK_THROWS(negiysem::parseIpApiResponse(R"({"status":"fail","message":"private range"})"));
    }
    SECTION("malformed JSON throws") {
        CHECK_THROWS(negiysem::parseIpApiResponse("<html>not json</html>"));
    }
}

TEST_CASE("parseOpenMeteoResponse") {
    SECTION("dry weather") {
        const auto w = negiysem::parseOpenMeteoResponse(
            R"({"current":{"temperature_2m":27.4,"precipitation":0.0,"weather_code":1}})");
        CHECK_THAT(w.temperature_c, WithinAbs(27.4, 1e-9));
        CHECK_FALSE(w.is_raining);
    }
    SECTION("precipitation means rain") {
        const auto w = negiysem::parseOpenMeteoResponse(
            R"({"current":{"temperature_2m":12.0,"precipitation":1.8,"weather_code":61}})");
        CHECK(w.is_raining);
    }
    SECTION("wet weather code without measured precipitation still counts") {
        const auto w = negiysem::parseOpenMeteoResponse(
            R"({"current":{"temperature_2m":5.0,"precipitation":0.0,"weather_code":71}})");
        CHECK(w.is_raining);
    }
    SECTION("missing current block throws") {
        CHECK_THROWS(negiysem::parseOpenMeteoResponse(R"({"error":true})"));
    }
}

namespace {

// Builds an hourly response of `days` * 24 hours where every hour has the
// given temperature, with optional overrides at one index.
std::string hourlyJson(int days, double temp, int wet_index = -1) {
    std::string temps, precip, codes;
    for (int i = 0; i < days * 24; ++i) {
        if (i > 0) {
            temps += ",";
            precip += ",";
            codes += ",";
        }
        temps += std::to_string(temp + (i % 24));  // rises through the day
        precip += (i == wet_index) ? "2.5" : "0.0";
        codes += (i == wet_index) ? "61" : "1";
    }
    return R"({"hourly":{"temperature_2m":[)" + temps + R"(],"precipitation":[)" +
           precip + R"(],"weather_code":[)" + codes + "]}}";
}

}  // namespace

TEST_CASE("parseOpenMeteoHourlyResponse") {
    SECTION("averages the temperatures in the window") {
        // Hours h have temperature 10 + h, so 9..17 averages to 10 + 13.
        const auto w = negiysem::parseOpenMeteoHourlyResponse(hourlyJson(1, 10.0), 0, 9, 17);
        CHECK_THAT(w.temperature_c, WithinAbs(23.0, 1e-9));
        CHECK_FALSE(w.is_raining);
    }
    SECTION("rain inside the window counts, outside does not") {
        const auto wet = negiysem::parseOpenMeteoHourlyResponse(
            hourlyJson(1, 10.0, /*wet_index=*/12), 0, 9, 17);
        CHECK(wet.is_raining);
        const auto dry = negiysem::parseOpenMeteoHourlyResponse(
            hourlyJson(1, 10.0, /*wet_index=*/20), 0, 9, 17);
        CHECK_FALSE(dry.is_raining);
    }
    SECTION("day offset skips ahead in whole days") {
        const auto w = negiysem::parseOpenMeteoHourlyResponse(hourlyJson(2, 10.0), 1, 9, 17);
        CHECK_THAT(w.temperature_c, WithinAbs(23.0, 1e-9));
    }
    SECTION("swapped or out-of-range hours are normalized") {
        const auto w = negiysem::parseOpenMeteoHourlyResponse(hourlyJson(1, 10.0), 0, 17, 9);
        CHECK_THAT(w.temperature_c, WithinAbs(23.0, 1e-9));
    }
    SECTION("window beyond the forecast throws") {
        CHECK_THROWS(negiysem::parseOpenMeteoHourlyResponse(hourlyJson(1, 10.0), 3, 9, 17));
        CHECK_THROWS(negiysem::parseOpenMeteoHourlyResponse(R"({"daily":{}})", 0, 9, 17));
    }
}

TEST_CASE("parseGeocodingResponse") {
    SECTION("labels candidates with region and country") {
        const auto results = negiysem::parseGeocodingResponse(
            R"({"results":[
                {"name":"Aydın","latitude":37.84,"longitude":27.84,
                 "admin1":"Aydın","country":"Türkiye"},
                {"name":"Efeler","latitude":37.85,"longitude":27.85,
                 "admin1":"Aydın","country":"Türkiye"}]})");
        REQUIRE(results.size() == 2);
        CHECK(results[0].city == "Aydın, Türkiye");  // repeated region is dropped
        CHECK(results[1].city == "Efeler, Aydın, Türkiye");
        CHECK_THAT(results[0].latitude, WithinAbs(37.84, 1e-9));
    }
    SECTION("no matches is an empty list, not an error") {
        CHECK(negiysem::parseGeocodingResponse(R"({"generationtime_ms":0.5})").empty());
    }
    SECTION("malformed JSON throws") {
        CHECK_THROWS(negiysem::parseGeocodingResponse("<html>oops</html>"));
    }
}

TEST_CASE("isWetWeatherCode") {
    CHECK(negiysem::isWetWeatherCode(61));   // rain
    CHECK(negiysem::isWetWeatherCode(75));   // snow
    CHECK(negiysem::isWetWeatherCode(82));   // violent showers
    CHECK(negiysem::isWetWeatherCode(95));   // thunderstorm
    CHECK_FALSE(negiysem::isWetWeatherCode(0));   // clear
    CHECK_FALSE(negiysem::isWetWeatherCode(3));   // overcast
    CHECK_FALSE(negiysem::isWetWeatherCode(45));  // fog
}
