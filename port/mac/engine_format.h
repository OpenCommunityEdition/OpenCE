#ifndef HALO_MAC_ENGINE_FORMAT_H
#define HALO_MAC_ENGINE_FORMAT_H
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
int _vsnprintf(char *, size_t, const char *, va_list);
int _snprintf(char *, size_t, const char *, ...);
/* Shared UTF-16 formatting calls these narrow helpers by their port names. */
int halo_linux_vsnprintf(char *, size_t, const char *, va_list);
int halo_linux_snprintf(char *, size_t, const char *, ...);
int halo_linux_vfprintf(FILE *, const char *, va_list);
int halo_linux_fprintf(FILE *, const char *, ...);
#endif
