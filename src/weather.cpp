#include "weather.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>

namespace negiysem {

using nlohmann::json;

Location parseIpApiResponse(const std::string& json_text) {
    const json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || j.value("status", "") != "success") {
        throw std::runtime_error("ip-api geolocation failed: " + json_text);
    }
    Location loc;
    loc.latitude = j.at("lat").get<double>();
    loc.longitude = j.at("lon").get<double>();
    loc.city = j.value("city", "");
    return loc;
}

bool isWetWeatherCode(int code) {
    // WMO codes: 51-67 drizzle/rain, 71-77 snow, 80-86 showers, 95-99 thunderstorm.
    return (code >= 51 && code <= 67) || (code >= 71 && code <= 77) ||
           (code >= 80 && code <= 86) || (code >= 95 && code <= 99);
}

Weather parseOpenMeteoResponse(const std::string& json_text) {
    const json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.contains("current")) {
        throw std::runtime_error("unexpected open-meteo response: " + json_text);
    }
    const json& current = j.at("current");
    Weather weather;
    weather.temperature_c = current.at("temperature_2m").get<double>();
    const double precipitation_mm = current.value("precipitation", 0.0);
    const int code = current.value("weather_code", 0);
    weather.is_raining = precipitation_mm > 0.05 || isWetWeatherCode(code);
    return weather;
}

Location WeatherService::detectLocation() const {
    // The free ip-api.com tier is HTTP-only.
    cpr::Response r = cpr::Get(cpr::Url{"http://ip-api.com/json"}, cpr::Timeout{5000});
    if (r.status_code != 200) {
        throw std::runtime_error("geolocation request failed (HTTP " +
                                 std::to_string(r.status_code) + "): " + r.error.message);
    }
    return parseIpApiResponse(r.text);
}

Weather WeatherService::fetchCurrent(const Location& location) const {
    std::ostringstream url;
    url << "https://api.open-meteo.com/v1/forecast?latitude=" << location.latitude
        << "&longitude=" << location.longitude
        << "&current=temperature_2m,precipitation,weather_code";
    cpr::Response r = cpr::Get(cpr::Url{url.str()}, cpr::Timeout{5000});
    if (r.status_code != 200) {
        throw std::runtime_error("weather request failed (HTTP " +
                                 std::to_string(r.status_code) + "): " + r.error.message);
    }
    return parseOpenMeteoResponse(r.text);
}

}  // namespace negiysem
