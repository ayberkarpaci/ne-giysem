#include "weather.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

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

Weather parseOpenMeteoDailyResponse(const std::string& json_text, int day_offset) {
    const json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.contains("daily")) {
        throw std::runtime_error("unexpected open-meteo daily response: " + json_text);
    }
    const json& daily = j.at("daily");
    const auto& max = daily.at("temperature_2m_max");
    const auto& min = daily.at("temperature_2m_min");
    if (day_offset < 0 || day_offset >= static_cast<int>(max.size())) {
        throw std::runtime_error("forecast day out of range: " + std::to_string(day_offset));
    }
    Weather weather;
    weather.temperature_c =
        (max.at(day_offset).get<double>() + min.at(day_offset).get<double>()) / 2.0;
    const double precipitation_mm =
        daily.value("precipitation_sum", json::array()).size() > static_cast<size_t>(day_offset)
            ? daily["precipitation_sum"].at(day_offset).get<double>()
            : 0.0;
    const int code =
        daily.value("weather_code", json::array()).size() > static_cast<size_t>(day_offset)
            ? daily["weather_code"].at(day_offset).get<int>()
            : 0;
    weather.is_raining = precipitation_mm > 0.5 || isWetWeatherCode(code);
    return weather;
}

Weather parseOpenMeteoHourlyResponse(const std::string& json_text, int day_offset,
                                     int start_hour, int end_hour) {
    const json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.contains("hourly")) {
        throw std::runtime_error("unexpected open-meteo hourly response: " + json_text);
    }
    const json& hourly = j.at("hourly");
    const auto& temps = hourly.at("temperature_2m");

    start_hour = std::clamp(start_hour, 0, 23);
    end_hour = std::clamp(end_hour, 0, 23);
    if (end_hour < start_hour) std::swap(start_hour, end_hour);

    double sum = 0.0;
    int count = 0;
    bool wet = false;
    for (int hour = start_hour; hour <= end_hour; ++hour) {
        const size_t index = static_cast<size_t>(day_offset) * 24 + hour;
        if (index >= temps.size() || temps.at(index).is_null()) continue;
        sum += temps.at(index).get<double>();
        ++count;
        const auto& precipitation = hourly.value("precipitation", json::array());
        if (precipitation.size() > index && !precipitation.at(index).is_null() &&
            precipitation.at(index).get<double>() > 0.05) {
            wet = true;
        }
        const auto& codes = hourly.value("weather_code", json::array());
        if (codes.size() > index && !codes.at(index).is_null() &&
            isWetWeatherCode(codes.at(index).get<int>())) {
            wet = true;
        }
    }
    if (count == 0) {
        throw std::runtime_error("no forecast hours in the requested window");
    }
    Weather weather;
    weather.temperature_c = sum / count;
    weather.is_raining = wet;
    return weather;
}

std::vector<Location> parseGeocodingResponse(const std::string& json_text) {
    const json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) {
        throw std::runtime_error("unexpected geocoding response: " + json_text);
    }
    std::vector<Location> candidates;
    if (!j.contains("results") || !j["results"].is_array()) {
        return candidates;  // no match is a valid answer
    }
    for (const auto& result : j["results"]) {
        if (!result.contains("latitude") || !result.contains("longitude")) continue;
        Location loc;
        loc.latitude = result["latitude"].get<double>();
        loc.longitude = result["longitude"].get<double>();
        std::string label = result.value("name", "");
        const std::string admin1 = result.value("admin1", "");
        const std::string country = result.value("country", "");
        if (!admin1.empty() && admin1 != label) label += ", " + admin1;
        if (!country.empty()) label += ", " + country;
        loc.city = label;
        candidates.push_back(std::move(loc));
    }
    return candidates;
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

Weather WeatherService::fetchForecast(const Location& location, int day_offset) const {
    if (day_offset <= 0) {
        return fetchCurrent(location);
    }
    std::ostringstream url;
    url << "https://api.open-meteo.com/v1/forecast?latitude=" << location.latitude
        << "&longitude=" << location.longitude
        << "&daily=temperature_2m_max,temperature_2m_min,precipitation_sum,weather_code"
        << "&forecast_days=7&timezone=auto";
    cpr::Response r = cpr::Get(cpr::Url{url.str()}, cpr::Timeout{5000});
    if (r.status_code != 200) {
        throw std::runtime_error("forecast request failed (HTTP " +
                                 std::to_string(r.status_code) + "): " + r.error.message);
    }
    return parseOpenMeteoDailyResponse(r.text, day_offset);
}

Weather WeatherService::fetchWindow(const Location& location, int day_offset,
                                    int start_hour, int end_hour) const {
    day_offset = std::clamp(day_offset, 0, 6);
    std::ostringstream url;
    url << "https://api.open-meteo.com/v1/forecast?latitude=" << location.latitude
        << "&longitude=" << location.longitude
        << "&hourly=temperature_2m,precipitation,weather_code"
        << "&forecast_days=" << (day_offset + 1) << "&timezone=auto";
    cpr::Response r = cpr::Get(cpr::Url{url.str()}, cpr::Timeout{5000});
    if (r.status_code != 200) {
        throw std::runtime_error("hourly forecast request failed (HTTP " +
                                 std::to_string(r.status_code) + "): " + r.error.message);
    }
    return parseOpenMeteoHourlyResponse(r.text, day_offset, start_hour, end_hour);
}

std::vector<Location> WeatherService::searchCity(const std::string& name,
                                                 const std::string& lang) const {
    // cpr::Parameters URL-encodes the query, so Turkish characters are safe.
    cpr::Response r = cpr::Get(
        cpr::Url{"https://geocoding-api.open-meteo.com/v1/search"},
        cpr::Parameters{{"name", name}, {"count", "5"}, {"language", lang},
                        {"format", "json"}},
        cpr::Timeout{5000});
    if (r.status_code != 200) {
        throw std::runtime_error("geocoding request failed (HTTP " +
                                 std::to_string(r.status_code) + "): " + r.error.message);
    }
    return parseGeocodingResponse(r.text);
}

}  // namespace negiysem
