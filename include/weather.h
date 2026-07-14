#pragma once

#include <string>
#include <vector>

namespace negiysem {

struct Location {
    double latitude = 0.0;
    double longitude = 0.0;
    std::string city;
};

struct Weather {
    double temperature_c = 0.0;
    bool is_raining = false;  // true for any wet precipitation (rain, showers, snow)
};

// Pure parsers, exposed for unit testing. All throw std::runtime_error on
// malformed or unsuccessful responses.
Location parseIpApiResponse(const std::string& json_text);
Weather parseOpenMeteoResponse(const std::string& json_text);
Weather parseOpenMeteoDailyResponse(const std::string& json_text, int day_offset);
// Averages the hourly temperatures of `day_offset` between start_hour and
// end_hour (inclusive); wet if any hour in the window is wet.
Weather parseOpenMeteoHourlyResponse(const std::string& json_text, int day_offset,
                                     int start_hour, int end_hour);
// City candidates from the open-meteo geocoding API; empty when nothing
// matches. `city` holds a display label like "Aydın, Türkiye".
std::vector<Location> parseGeocodingResponse(const std::string& json_text);

// WMO weather interpretation codes used by Open-Meteo: drizzle, rain,
// showers, snow and thunderstorms all count as wet.
bool isWetWeatherCode(int code);

// Fetches location and current weather from free, keyless public APIs
// (ip-api.com for IP geolocation, open-meteo.com for weather).
class WeatherService {
public:
    // Geolocates the machine's public IP. Throws std::runtime_error on
    // network failure.
    Location detectLocation() const;

    // Current conditions at the given coordinates. Throws std::runtime_error
    // on network failure.
    Weather fetchCurrent(const Location& location) const;

    // Forecast for a day 0 (today) .. 6 from now; day 0 uses fetchCurrent.
    Weather fetchForecast(const Location& location, int day_offset) const;

    // Average conditions between start_hour and end_hour (0-23, inclusive)
    // of the given day. Throws std::runtime_error on network failure.
    Weather fetchWindow(const Location& location, int day_offset,
                        int start_hour, int end_hour) const;

    // Searches for a city by (partial) name so the user can correct the
    // IP-based guess. `lang` localizes the place names when supported.
    std::vector<Location> searchCity(const std::string& name,
                                     const std::string& lang) const;
};

}  // namespace negiysem
