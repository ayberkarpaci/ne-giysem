#pragma once

#include <string>

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

// Pure parsers, exposed for unit testing. Both throw std::runtime_error on
// malformed or unsuccessful responses.
Location parseIpApiResponse(const std::string& json_text);
Weather parseOpenMeteoResponse(const std::string& json_text);

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
};

}  // namespace negiysem
