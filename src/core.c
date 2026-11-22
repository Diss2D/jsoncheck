#include "core.h"
#include <cjson/cJSON.h>

bool jsoncheck_validate(const char *text, size_t *error_offset) {
    if (error_offset != NULL) {
        *error_offset = 0;
    }
    if (text == NULL) {
        return false;
    }
    const char *parse_end = NULL;
    cJSON *document = cJSON_ParseWithOpts(text, &parse_end, 1);
    if (document == NULL) {
        if (error_offset != NULL && parse_end != NULL) {
            *error_offset = (size_t)(parse_end - text);
        }

        return false;
    }

    cJSON_Delete(document);
    return true;
}

void jsoncheck_error_location(const char *text, size_t error_offset, size_t *line, size_t *column) {
    size_t current_line = 1;
    size_t current_column = 1;

    if (text != NULL) {
        for (size_t i = 0; i < error_offset && text[i] != '\0'; i++) {
            if (text[i] == '\n') {
                current_line++;
                current_column = 1;
            } else if (((unsigned char)text[i] & 0xC0) != 0x80) {
                current_column++;
            }
        }
    }

    if (line != NULL) {
        *line = current_line;
    }

    if (column != NULL) {
        *column = current_column;
    }
}

char *jsoncheck_format(const char *text) {
    if (text == NULL) {
        return NULL;
    }
    cJSON *document = cJSON_ParseWithOpts(text, NULL, 1);
    if (document == NULL) {
        return NULL;
    }
    char *formatted = cJSON_Print(document);
    cJSON_Delete(document);

    return formatted;
}

void jsoncheck_free(char *text) {
    cJSON_free(text);
}
