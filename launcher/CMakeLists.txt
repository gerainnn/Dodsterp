cmake_minimum_required(VERSION 3.20)
project(launcher CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "launcher is Windows-only")
endif()

add_executable(launcher
    src/main.cpp
)

target_compile_definitions(launcher PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

target_link_libraries(launcher PRIVATE
    advapi32
    shell32
    user32
)
