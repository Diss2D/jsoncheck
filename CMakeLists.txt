cmake_minimum_required(VERSION 3.20)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
     APP_VERSION LIMIT_COUNT 1)

project(jsoncheck VERSION "${APP_VERSION}" LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)
# Поиск библиотеки cJSON
find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
# Статическая библиотека с основной логикой
add_library(jsoncore STATIC src/core.c)
target_include_directories(jsoncore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(jsoncore PUBLIC
    PkgConfig::CJSON
)

# Консольная программа
add_executable(jsoncheck src/main.c)

target_compile_definitions(jsoncheck PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(jsoncheck PRIVATE jsoncore)