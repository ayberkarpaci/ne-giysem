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

TEST_CASE("isWetWeatherCode") {
    CHECK(negiysem::isWetWeatherCode(61));   // rain
    CHECK(negiysem::isWetWeatherCode(75));   // snow
    CHECK(negiysem::isWetWeatherCode(82));   // violent showers
    CHECK(negiysem::isWetWeatherCode(95));   // thunderstorm
    CHECK_FALSE(negiysem::isWetWeatherCode(0));   // clear
    CHECK_FALSE(negiysem::isWetWeatherCode(3));   // overcast
    CHECK_FALSE(negiysem::isWetWeatherCode(45));  // fog
}
