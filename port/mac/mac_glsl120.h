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


/* port: the GLSL version the driver takes, 120 (the default) or 110 (Mac OS X
   10.4's); the text made for 120 is unchanged. Set before the first shader. */
void mac_glsl120_set_version(int version);
int mac_glsl120_version(void);
/* HALO_MAC_GLSL_DUMP=<folder>: writes a shader's text once (kind "vertex", "pixel"
   or "arbfragment"); a diagnostic for validating the text offline. */
void mac_glsl_dump(const char *kind, const char *source);

#endif
