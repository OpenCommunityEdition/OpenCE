#ifndef HALO_MAC_GLSL120_H
#define HALO_MAC_GLSL120_H

#include <stddef.h>

/* Convert the shared NV2A shader generators' GLSL 4.50 output to the
   deliberately narrow GLSL 1.20 subset used by the legacy Mac context.
   Packed attributes, textures, and advanced fragment operations fail closed.
   The returned source is allocated with malloc and belongs to the caller. */
int mac_glsl120_vertex(const char *source, char **translated,
    unsigned long *attribute_mask, char *error, size_t error_size);
int mac_glsl120_pixel(const char *source, char **translated,
    char *error, size_t error_size);

#endif
