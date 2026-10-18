#ifndef HALO_MAC_NATIVE_FONT_TAGS_H
#define HALO_MAC_NATIVE_FONT_TAGS_H
#include "retail_cache.h"
#include "native_tags.h"
/* Converts the retail font root, its byte-indexed character tables, glyph
   records, and alpha pixels into owned engine structures. Font bytes are
   local map data and are never bundled with the port. */
int mac_native_font_decode(const struct mac_retail_cache *,
    struct mac_native_tags *,uint32_t handle,void **root,size_t *root_bytes,
    size_t *owned_bytes,void **owner,const char **error);
void mac_native_font_owner_free(void *owner);
#endif
