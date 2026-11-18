#include "core.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef APP_VERSION
#error "APP_VERSION is not defined"
#endif

static void print_help(FILE *stream) {
    fprintf(stream,
            "jsoncheck - JSON validator\n"
            "\n"
            "Usage:\n"
            "  jsoncheck validate <file>\n"
            "  jsoncheck --help\n"
            "  jsoncheck --version\n"
            "\n"
            "Commands:\n"
            "  validate    Check JSON syntax\n");
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Cannot open file: %s\n", path);
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Cannot seek file: %s\n", path);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);

    if (file_size < 0 || (uintmax_t)file_size >= (uintmax_t)SIZE_MAX) {
        fprintf(stderr, "Invalid file size: %s\n", path);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Cannot rewind file: %s\n", path);
        fclose(file);
        return NULL;
    }

    char *buffer = malloc((size_t)file_size + 1);

    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);

    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Cannot read file: %s\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';

    if (memchr(buffer, '\0', bytes_read) != NULL) {
        fprintf(stderr, "File contains null bytes: %s\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Cannot close file: %s\n", path);
        free(buffer);
        return NULL;
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help(stdout);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("jsoncheck %s\n", APP_VERSION);
        return 0;
    }
    if (argc != 3 || strcmp(argv[1], "validate") != 0) {
        print_help(stderr);
        return 2;
    }

    char *text = read_file(argv[2]);

    if (text == NULL) {
        return 1;
    }

    size_t error_offset = 0;
    bool valid = jsoncheck_validate(text, &error_offset);

    if (!valid) {
        size_t line = 1;
        size_t column = 1;

        jsoncheck_error_location(text, error_offset, &line, &column);

        fprintf(stderr,
                "Invalid JSON at line %zu, column %zu "
                "(byte offset %zu)\n",
                line,
                column,
                error_offset);

        free(text);
        return 1;
    }

    free(text);
    printf("JSON is valid\n");
    return 0;
}
