#ifndef HALO_MAC_ENGINE_SYNCHRONOUS_PRECACHE_H
#define HALO_MAC_ENGINE_SYNCHRONOUS_PRECACHE_H

#include "retail_cache.h"

/* Bind the one already-open, validated native map. These are bookkeeping
   callbacks only: native sessions never copy a map or report async work. */
int mac_engine_precache_bind(const struct mac_retail_cache *cache);
int mac_engine_precache_unbind(const struct mac_retail_cache *cache);

#endif
