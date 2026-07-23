cmake_minimum_required(VERSION 3.20)
project(ICU_NEWS LANGUAGES CXX)

# Exporta build/compile_commands.json (usado pelo IntelliSense do VS Code para
# resolver os includes de cada arquivo, incluindo GoogleTest/GoogleMock).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(icu_news
    src/main.cpp
    src/CLI.cpp
    src/Dashboard.cpp
    src/ICU.cpp
    src/LED.cpp
    src/News.cpp
)

target_include_directories(icu_news PRIVATE include)

if(MSVC)
    target_compile_options(icu_news PRIVATE /W4)
else()
    target_compile_options(icu_news PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_subdirectory(tests)
