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
./build/ne-giysem
```

## Status

Early scaffolding — no business logic yet.
