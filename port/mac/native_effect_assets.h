#ifndef HALO_MAC_NATIVE_EFFECT_ASSETS_H
#define HALO_MAC_NATIVE_EFFECT_ASSETS_H
#include "native_actor_assets.h"
/* Loads a bounded transitive effect/particle/shader resource graph into an unbound
   asset batch. Cycles are visited once. Unsupported families fail explicitly;
   partial published dependencies remain owned, so dispose the batch on failure.
   This establishes resource ownership, not effect execution/render readiness. */
int mac_native_effect_assets_load(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* Resolves supported effects on vehicle/weapon/equipment attachments and trigger
   projectiles, including impact/detonation damage and material responses. Other object dependencies remain the actor loader's work. */
int mac_native_object_effect_assets_load(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
#endif
