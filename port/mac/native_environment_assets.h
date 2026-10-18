#ifndef HALO_MAC_NATIVE_ENVIRONMENT_ASSETS_H
#define HALO_MAC_NATIVE_ENVIRONMENT_ASSETS_H
#include "native_actor_assets.h"
/* Loads complete environment and Chicago shader roots and the base/lightmap bitmap data
   consumed by object_lights.c light_particle. Chicago dependencies and other shader resources (detail,
   reflection, lens) remain separate rendering dependencies. Fails on any
   material shader family without a native decoder. Dispose partial batches. */
int mac_native_environment_lighting_load(const struct mac_retail_cache *,
    const struct mac_engine_world *,const void *,size_t,struct mac_actor_assets *,const char **);
#endif
