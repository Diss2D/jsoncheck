#include "core.h"
#include "server.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef APP_VERSION
#error "APP_VERSION is not defined"
#endif

static void print_help(FILE *stream) {
    fprintf(stream,
            "jsoncheck - JSON validator and formatter\n"
            "\n"
            "Usage:\n"
            "  jsoncheck validate <file>\n"
            "  jsoncheck format [--pretty|--compact] <file>\n"
            "  jsoncheck serve\n"
            "  jsoncheck --help\n"
            "  jsoncheck --version\n"
            "\n"
            "Commands:\n"
            "  validate    Check JSON syntax\n"
            "  format      Print formatted JSON\n"
            "  serve       Start HTTP server\n"
            "\n"
            "Format options:\n"
            "  --pretty    Print indented JSON (default)\n"
            "  --compact   Print JSON without extra whitespace\n");
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

static bool load_format_config(bool *pretty) {
    const char *path = "/etc/jsoncheck/jsoncheck.conf";
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        if (errno == ENOENT) {
            return true;
        }

        fprintf(stderr, "Cannot open configuration file: %s\n", path);
        return false;
    }

    char line[256];
    size_t line_number = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (strcmp(line, "format=pretty") == 0) {
            *pretty = true;
        } else if (strcmp(line, "format=compact") == 0) {
            *pretty = false;
        } else {
            fprintf(stderr, "Invalid configuration at line %zu\n", line_number);
            fclose(file);
            return false;
        }
    }

    if (ferror(file)) {
        fprintf(stderr, "Cannot read configuration file: %s\n", path);
        fclose(file);
        return false;
    }
    if (fclose(file) != 0) {
        fprintf(stderr, "Cannot close configuration file: %s\n", path);
        return false;
    }

    return true;
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
    if (argc == 2 && strcmp(argv[1], "serve") == 0) {
        return jsoncheck_server_run(APP_VERSION);
    }
    bool pretty = true;
    const char *path = NULL;

    if (argc == 3 && (strcmp(argv[1], "validate") == 0 || strcmp(argv[1], "format") == 0)) {
        path = argv[2];
    } else if (argc == 4 && strcmp(argv[1], "format") == 0) {
        if (strcmp(argv[2], "--compact") == 0) {
            pretty = false;
        } else if (strcmp(argv[2], "--pretty") == 0) {
            pretty = true;
        } else {
            print_help(stderr);
            return 2;
        }

        path = argv[3];
    } else {
        print_help(stderr);
        return 2;
    }

    if (strcmp(argv[1], "format") == 0 && argc == 3) {
        if (!load_format_config(&pretty)) {
            return 1;
        }
    }

    char *text = read_file(path);

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

    if (strcmp(argv[1], "validate") == 0) {
        free(text);
        printf("JSON is valid\n");
        return 0;
    }

    char *formatted = jsoncheck_format(text, pretty);
    free(text);

    if (formatted == NULL) {
        fprintf(stderr, "Cannot format JSON\n");
        return 1;
    }

    int printed = printf("%s\n", formatted);
    jsoncheck_free(formatted);

    if (printed < 0) {
        fprintf(stderr, "Cannot write formatted JSON\n");
        return 1;
    }

    return 0;
}
