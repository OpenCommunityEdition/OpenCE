#ifndef HALO_MAC_NATIVE_DECAL_TAGS_H
#define HALO_MAC_NATIVE_DECAL_TAGS_H
#include "native_object_tags.h"
/* Decodes a complete decal definition and validates its bounded acyclic
   next-decal chain. The returned owner owns the definition's native storage;
   referenced decal and bitmap tags remain separate registry dependencies. */
int mac_native_decal_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
#endif
