#ifndef JSONCHECK_CORE_H
#define JSONCHECK_CORE_H

#include <stdbool.h>
#include <stddef.h>

bool jsoncheck_validate(const char *text, size_t *error_offset);

void jsoncheck_error_location(const char *text, size_t error_offset, size_t *line, size_t *column);

char *jsoncheck_format(const char *text, bool pretty);

void jsoncheck_free(char *text);

#endif
