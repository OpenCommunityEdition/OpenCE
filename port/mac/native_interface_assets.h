#ifndef HALO_MAC_NATIVE_INTERFACE_ASSETS_H
#define HALO_MAC_NATIVE_INTERFACE_ASSETS_H
#include "native_tags.h"
#include "native_actor_assets.h"
/* Owns hudg, hud#, unhi, wphi, grhi, hmt, font and string roots plus direct bitmap and
   transitive sound dependencies consumed by those HUD records. This provides
   data ownership only; texture upload, font drawing and sound playback remain
   separate engine services. Partial publication remains owned for teardown. */
int mac_native_interface_assets_load(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,const char **);
/* Storage-only variant used by the format probe; runtime sessions use the
   resource-owning API above. */
int mac_native_interface_assets_load_roots(const struct mac_retail_cache *,
    struct mac_native_tags *,const char **);
#endif
