cmake_minimum_required(VERSION 3.20)
project(dodsterp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "dodsterp is Windows-only (CS2 is Windows-only).")
endif()

include(FetchContent)

# ---------------------------------------------------------------------- ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# -------------------------------------------------------------------- MinHook
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minhook)

# minhook ships its own CMakeLists, target = "minhook"

# ----------------------------------------------------------------- dodsterp.dll
add_library(dodsterp SHARED
    src/dllmain.cpp
    src/overlay/hook.cpp
    src/overlay/renderer.cpp
    src/overlay/theme.cpp
    src/overlay/menu.cpp
)

set_target_properties(dodsterp PROPERTIES
    OUTPUT_NAME "dodsterp"
    PREFIX      ""
    SUFFIX      ".dll"
)

target_include_directories(dodsterp PRIVATE src)

target_compile_definitions(dodsterp PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

target_link_libraries(dodsterp PRIVATE
    imgui
    minhook
    d3d11
    dxgi
    d3dcompiler
    user32
    gdi32
    dwmapi
)

# Manual mapping не любит ICF/REF — функции стаба должны лежать рядом
if(MSVC)
    target_link_options(dodsterp PRIVATE /OPT:NOICF /OPT:NOREF)
    target_compile_options(dodsterp PRIVATE /utf-8)
endif()
