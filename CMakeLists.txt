cmake_minimum_required(VERSION 3.20)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
     APP_VERSION LIMIT_COUNT 1)

project(jsoncheck VERSION "${APP_VERSION}" LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)
add_executable(jsoncheck src/main.c)

target_compile_definitions(jsoncheck PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)
