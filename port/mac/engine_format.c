/* Narrow MS length modifiers with the shared port's C99 snprintf semantics:
   return the full required length and NUL terminate when capacity is nonzero.
   Native va_list stays intact so PPC's register/alignment rules are honored. */
#include "engine_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void translate_format(const char *source, char *destination)
{
    while (*source) {
        *destination++ = *source;
        if (*source++ != '%') continue;
        if (*source == '%') {
            *destination++ = *source++;
            continue;
        }
        while (*source && strchr("-+ #0123456789.*", *source))
            *destination++ = *source++;
        if (strncmp(source, "I64", 3) == 0) {
            *destination++ = 'l';
            *destination++ = 'l';
            source += 3;
        } else if (strncmp(source, "I32", 3) == 0) {
            source += 3;
        }
    }
    *destination = 0;
}

int _vsnprintf(char *buffer, size_t count, const char *format, va_list arguments)
{
    char local[1024];
    char *translated = local;
    size_t length;
    int result;
    if (!strstr(format, "I64") && !strstr(format, "I32"))
        return vsnprintf(buffer, count, format, arguments);
    length = strlen(format);
    /* Translation only shortens the input. Never truncate a format string:
       losing a conversion can otherwise consume arguments with wrong types. */
    if (length >= sizeof(local)) {
        if (length == (size_t)-1 || !(translated = malloc(length + 1))) {
            if (count) buffer[0] = 0;
            errno = ENOMEM;
            return -1;
        }
    }
    translate_format(format, translated);
    result = vsnprintf(buffer, count, translated, arguments);
    if (translated != local) free(translated);
    return result;
}
int _snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list arguments;
    int result;
    va_start(arguments, format);
    result = _vsnprintf(buffer, count, format, arguments);
    va_end(arguments);
    return result;
}

int halo_linux_vsnprintf(char *buffer, size_t count, const char *format, va_list arguments)
{
    return _vsnprintf(buffer, count, format, arguments);
}
int halo_linux_snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list arguments;
    int result;
    va_start(arguments, format);
    result = halo_linux_vsnprintf(buffer, count, format, arguments);
    va_end(arguments);
    return result;
}

int halo_linux_vfprintf(FILE *stream, const char *format, va_list arguments)
{
    char local[1024];
    char *translated = local;
    size_t length;
    int result;
    if (!strstr(format, "I64") && !strstr(format, "I32"))
        return vfprintf(stream, format, arguments);
    length = strlen(format);
    if (length >= sizeof(local)) {
        if (length == (size_t)-1 || !(translated = malloc(length + 1))) {
            errno = ENOMEM;
            return -1;
        }
    }
    translate_format(format, translated);
    result = vfprintf(stream, translated, arguments);
    if (translated != local) free(translated);
    return result;
}

int halo_linux_fprintf(FILE *stream, const char *format, ...)
{
    va_list arguments;
    int result;
    va_start(arguments, format);
    result = halo_linux_vfprintf(stream, format, arguments);
    va_end(arguments);
    return result;
}
