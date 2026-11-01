/* Player-facing explanations for the existing Custom Edition loader. */
#ifndef CUSTOM_EDITION_ERRORS_H
#define CUSTOM_EDITION_ERRORS_H

#include "cache_file_formats.h"

char const *custom_edition_status_explanation(enum cache_file_status status);
void custom_edition_error_message(char const *map, char const *reason, char *out, size_t size);

#endif
