cmake_minimum_required(VERSION 3.10)

project(Egg
    VERSION 0.1.0
    DESCRIPTION "A small Windows mouse and keyboard automation tool"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Egg 0xegg/0xegg.cpp)

target_compile_definitions(Egg PRIVATE EGG_VERSION="${PROJECT_VERSION}")

set_target_properties(Egg PROPERTIES OUTPUT_NAME egg)

# Lier les bibliothèques Windows nécessaires
if (WIN32)
    enable_language(RC)
    target_sources(Egg PRIVATE 0xegg/egg.rc)
    target_link_libraries(Egg user32)
endif()