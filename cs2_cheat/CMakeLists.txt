cmake_minimum_required(VERSION 3.20)
project(cs2_cheat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only.")
endif()

# 64-bit only — CS2 is x64
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Build as x64. CS2 is a 64-bit process.")
endif()

include(FetchContent)

# --- ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

# --- MinHook ---
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# --- cheat DLL ---
add_library(cs2_cheat SHARED
    src/dllmain.cpp
    src/hooks.cpp
    src/render.cpp
    src/esp.cpp
    src/menu.cpp
    src/math.cpp
    ${IMGUI_SRC}
)

target_include_directories(cs2_cheat PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(cs2_cheat PRIVATE
    minhook
    d3d11
    dxgi
    d3dcompiler
)

target_compile_definitions(cs2_cheat PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set_target_properties(cs2_cheat PROPERTIES
    OUTPUT_NAME "cs2_cheat"
    PREFIX ""
    SUFFIX ".dll"
)

# --- injector executable ---
add_subdirectory(injector)
