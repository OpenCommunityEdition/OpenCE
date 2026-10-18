#ifndef HALO_MAC_NATIVE_SKY_ASSETS_H
#define HALO_MAC_NATIVE_SKY_ASSETS_H
#include "native_actor_assets.h"

/* Load one real sky into an existing native asset registry. The sky model,
   optional animation graph, and lens-flare primary bitmap are loaded from the
   supplied retail cache and external bitmaps.map. A nonempty fog-screen
   reference is refused until its renderer resources have a native provider.
   Dependencies published before an error remain owned by assets; dispose the
   asset registry before retrying. This does not claim a complete sky renderer. */
int mac_native_sky_assets_load(const struct mac_retail_cache *,const void *,size_t,
    struct mac_actor_assets *,uint32_t,const char **);
#endif
