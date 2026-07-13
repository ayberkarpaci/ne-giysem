cmake_minimum_required(VERSION 3.21)

# vcpkg integration — must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/ayberk/vcpkg/scripts/buildsystems/vcpkg.cmake"
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

add_executable(ne-giysem
    src/main.cpp
    src/database.cpp
)

target_include_directories(ne-giysem PRIVATE include)
target_link_libraries(ne-giysem PRIVATE unofficial::sqlite3::sqlite3)
