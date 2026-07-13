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

API endpoints: `GET /api/moods?lang=..` and
`GET /api/recommendation?mood=..&lang=..[&temp=..&rain=0|1]`.

Moods: `energetic`, `cozy`, `confident`, `relaxed`, `adventurous`.
Languages: `en`, `tr` (display names come from the `translations` table).

Both weather APIs are free and require no API key.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## Status

Core engine, SQLite catalog, automatic weather lookup and a bilingual (EN/TR)
web UI are in place. Next up: a mobile app with full i18n.
