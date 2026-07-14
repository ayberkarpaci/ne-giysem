# ne-giysem

An outfit recommendation system that suggests what to wear based on the current
weather and your mood.

## Project structure

```
ne-giysem/
├── src/        # C++ source files
├── include/    # Header files
├── data/       # SQLite database (created at runtime, not committed)
├── web/        # Web UI (HTML/CSS/JS)
├── tests/      # Test files
└── CMakeLists.txt
```

## Building

Requires CMake (3.21+), a C++17 compiler, and [vcpkg](https://github.com/microsoft/vcpkg).
The vcpkg toolchain path is set in `CMakeLists.txt`; override it with
`-DCMAKE_TOOLCHAIN_FILE=<path>/scripts/buildsystems/vcpkg.cmake` if yours lives elsewhere.

```sh
cmake -S . -B build
cmake --build build
```

## Usage

```sh
# Automatic: geolocates by IP (ip-api.com) and fetches current weather (open-meteo.com)
ne-giysem [mood] [lang]

# Manual weather
ne-giysem <temp_c> <rain 0|1> [mood] [lang]

# Web UI + JSON API on http://localhost:8080 (run from the repo root)
ne-giysem serve [port]
```

Main API endpoints (see `include/server.h` for the full list):

- `GET /api/recommendation?mood=..&lang=..` — outfit for the current weather
- `POST /api/ask` — free-text plan, parsed by Gemini; a stylist pass picks
  the most coherent combination (color harmony, consistent formality) from
  the top candidates per category
- `POST /api/feel` — free-text mood, blended by Gemini (same stylist pass)
- `POST /api/feedback` — rate a suggestion 1-5 (+ comment); a poor rating
  returns an alternative outfit, and ratings feed future scoring
- `GET /api/location`, `GET /api/geocode?name=..` — detect / correct the city
- `POST /api/classify-photo` — prefill the add form from a garment photo

Weather overrides on every recommendation call: manual weather
(`temp` + `rain`), corrected location (`lat` + `lon` + `city`) and/or an hour
window (`start_hour` + `end_hour`) that averages the hourly forecast.

Moods: `energetic`, `cozy`, `confident`, `relaxed`, `adventurous`.
Languages: `en`, `tr` (display names come from the `translations` table).

The weather, geocoding and geolocation APIs are free and require no API key;
the Gemini-backed features need `GEMINI_API_KEY` in `.env`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## Status

Core engine, SQLite catalog (55 garment types incl. jewelry), Gemini photo
classification with multi-color support, weather with location correction,
hour windows and manual entry, and a feedback loop that learns from ratings
are in place — all behind a bilingual (EN/TR) web UI. Next up: a mobile app
with full i18n.
