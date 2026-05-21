cmake_minimum_required(VERSION 3.20)
project(cs2-trainer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "cs2-trainer is Windows-only (CS2 is Windows-only).")
endif()

add_executable(cs2-trainer
    src/main.cpp
    src/process.cpp
    src/offsets.cpp
    src/sdk/player.cpp
)

target_include_directories(cs2-trainer PRIVATE src)

target_compile_definitions(cs2-trainer PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(cs2-trainer PRIVATE
    psapi
    user32
)

# nlohmann/json через vcpkg
find_package(nlohmann_json CONFIG QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(cs2-trainer PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(cs2-trainer PRIVATE HAS_NLOHMANN_JSON)
endif()
