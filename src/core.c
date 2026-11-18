#include "core.h"
#include <cjson/cJSON.h>

bool jsoncheck_validate(const char *text, size_t *error_offset)
{
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