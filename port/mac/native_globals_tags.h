#ifndef HALO_MAC_NATIVE_GLOBALS_TAGS_H
#define HALO_MAC_NATIVE_GLOBALS_TAGS_H
#include "native_object_tags.h"
/* Complete owned matg storage; dependency references retain handles and must
   be separately loaded. Playlist storage is converted too; its runtime use remains unsupported. No sound,
   HUD, renderer or full game readiness is implied by storage conversion. */
int mac_native_globals_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
#endif
