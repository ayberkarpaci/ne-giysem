cmake_minimum_required(VERSION 3.21)

# vcpkg integration — must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_ROOT_DIR "$ENV{VCPKG_ROOT}")
    else()
        set(VCPKG_ROOT_DIR "$ENV{USERPROFILE}/vcpkg")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(ne-giysem
    VERSION 0.1.0
    DESCRIPTION "Weather and mood based outfit recommendation system"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)

add_library(negiysem_core STATIC
    src/database.cpp
    src/recommender.cpp
    src/seed.cpp
    src/weather.cpp
    src/server.cpp
    src/wardrobe.cpp
    src/gemini.cpp
    src/env.cpp
    src/feedback.cpp
    src/cutout.cpp
    src/rembg.cpp
)
target_include_directories(negiysem_core PUBLIC include)
target_include_directories(negiysem_core PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries(negiysem_core
    PUBLIC unofficial::sqlite3::sqlite3
    PRIVATE cpr::cpr nlohmann_json::nlohmann_json httplib::httplib
)

add_executable(ne-giysem src/main.cpp)
target_link_libraries(ne-giysem PRIVATE negiysem_core)

enable_testing()
find_package(Catch2 3 CONFIG REQUIRED)
add_executable(negiysem_tests
    tests/test_recommender.cpp
    tests/test_weather.cpp
    tests/test_server.cpp
    tests/test_wardrobe.cpp
    tests/test_gemini.cpp
    tests/test_feedback.cpp
    tests/test_cutout.cpp
)
target_include_directories(negiysem_tests PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries(negiysem_tests PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(negiysem_tests PRIVATE negiysem_core Catch2::Catch2WithMain)
include(Catch)
catch_discover_tests(negiysem_tests)
