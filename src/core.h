#ifndef JSONCHECK_CORE_H
#define JSONCHECK_CORE_H
#include <stdbool.h>
#include <stddef.h>

bool jsoncheck_validate(const char *text, size_t *error_offset);

#endif