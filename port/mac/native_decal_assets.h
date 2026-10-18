#ifndef HALO_MAC_NATIVE_DECAL_ASSETS_H
#define HALO_MAC_NATIVE_DECAL_ASSETS_H
#include "native_actor_assets.h"
/* Add complete decal chains and bitmap resources before cache binding.
   On failure caller must dispose assets, including any dependencies added. */
int mac_native_decal_assets_load(const struct mac_retail_cache *,const void *,size_t,
    struct mac_actor_assets *,const uint32_t *,unsigned,const char **);
#endif
