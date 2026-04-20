cmake_minimum_required(VERSION 3.10)

project(AutoClicker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(AutoClicker 0xegg/0xegg.cpp)

# Lier les bibliothèques Windows nécessaires
if (WIN32)
    target_link_libraries(AutoClicker user32)
endif()