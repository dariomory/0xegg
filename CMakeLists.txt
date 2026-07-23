cmake_minimum_required(VERSION 3.10)

project(Egg
    VERSION 0.2.0
    DESCRIPTION "A small Windows mouse and keyboard automation tool"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(Egg 0xegg/0xegg.cpp)

target_compile_definitions(Egg PRIVATE EGG_VERSION="${PROJECT_VERSION}")

set_target_properties(Egg PROPERTIES OUTPUT_NAME egg-cli)

target_link_libraries(Egg PRIVATE nlohmann_json::nlohmann_json)

# Lier les bibliothèques Windows nécessaires
if (WIN32)
    enable_language(RC)
    target_sources(Egg PRIVATE 0xegg/egg.rc)
    target_link_libraries(Egg PRIVATE user32)
endif()

# GUI target (Windows only) - Dear ImGui has no upstream CMakeLists, so its
# sources are listed directly rather than consumed as a package.
if (WIN32)
    FetchContent_Declare(imgui URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.tar.gz)
    FetchContent_MakeAvailable(imgui)

    # stb_image is a single header with no CMakeLists of its own, so it's
    # fetched directly rather than via FetchContent_MakeAvailable.
    set(STB_IMAGE_DIR "${CMAKE_BINARY_DIR}/stb")
    if (NOT EXISTS "${STB_IMAGE_DIR}/stb_image.h")
        file(DOWNLOAD
            https://raw.githubusercontent.com/nothings/stb/31c1ad37456438565541f4919958214b6e762fb4/stb_image.h
            "${STB_IMAGE_DIR}/stb_image.h"
        )
    endif()

    add_executable(EggGui WIN32
        0xegg/gui_main.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        0xegg/egg.rc
    )

    target_include_directories(EggGui PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${STB_IMAGE_DIR})
    target_compile_definitions(EggGui PRIVATE EGG_VERSION="${PROJECT_VERSION}")
    set_target_properties(EggGui PROPERTIES OUTPUT_NAME egg)
    target_link_libraries(EggGui PRIVATE nlohmann_json::nlohmann_json user32 d3d11 dxgi comdlg32)
endif()