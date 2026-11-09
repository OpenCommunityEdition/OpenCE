/* MCC diagnostics have no dependency on the CE loader or its error state. */
#ifndef MCC_ERRORS_H
#define MCC_ERRORS_H
#include "mcc_cache_format.h"

char const *mcc_status_explanation(enum mcc_cache_status status);
void mcc_error_message(char const *name, char const *reason, char *out, size_t size);
void mcc_error_text(char const *name, char const *reason, wchar_t *out, size_t size);
#endif
