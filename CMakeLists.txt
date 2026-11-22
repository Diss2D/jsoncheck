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

# Автоматизированное тестирование
include(CTest)

if(BUILD_TESTING)
    add_executable(test_core tests/test_core.c)

    target_link_libraries(test_core PRIVATE jsoncore)

    add_test(NAME core COMMAND test_core)

    # Проверка версии программы
    add_test(NAME cli_version COMMAND jsoncheck --version)

    set_tests_properties(cli_version PROPERTIES
        PASS_REGULAR_EXPRESSION "jsoncheck ${PROJECT_VERSION}"
    )

    # Проверка неизвестного аргумента
    add_test(NAME cli_bad_args COMMAND jsoncheck --no-such-option)

    set_tests_properties(cli_bad_args PROPERTIES
        WILL_FAIL TRUE
    )
endif()

# Установка исполняемого файла
install(TARGETS jsoncheck
    RUNTIME DESTINATION bin
)

# Установка конфигурационного файла
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/jsoncheck.conf"
    DESTINATION /etc/jsoncheck
)

#Подготовка сжатой истории изменений
find_program(GZIP_EXECUTABLE gzip REQUIRED)

execute_process(
    COMMAND "${GZIP_EXECUTABLE}" -9n -c
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/changelog"
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    RESULT_VARIABLE GZIP_RESULT
)

if(NOT GZIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to compress changelog")
endif()

# Установка документации
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/copyright"
    "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    DESTINATION share/doc/jsoncheck
)

# Cправочная страница man
execute_process(
    COMMAND "${GZIP_EXECUTABLE}" -9n -c
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/jsoncheck.1"
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/jsoncheck.1.gz"
    RESULT_VARIABLE MAN_GZIP_RESULT
)

if(NOT MAN_GZIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to compress manual page")
endif()

# Установка справочной страницы
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/jsoncheck.1.gz"
    DESTINATION share/man/man1
)
