#ifndef HALO_MAC_NATIVE_HUD_TAGS_H
#define HALO_MAC_NATIVE_HUD_TAGS_H
#include "native_object_tags.h"
/* Full owned hudg root, icons, arrows and copied reference names. Dependencies
   remain handles: their conversion/loading is separate, not implied here.
   Free and enumerate references with the native object ownership API. */
int mac_native_hud_globals_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
#endif
