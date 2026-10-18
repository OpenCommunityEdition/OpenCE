#ifndef HALO_MAC_ENGINE_SCRIPT_CACHE_H
#define HALO_MAC_ENGINE_SCRIPT_CACHE_H
#include "native_scenario_scripts.h"
/* Engine-thread lease of live owned script allocations. Unbind before freeing
   the owner. Nested bindings and mismatched releases leave the lease intact.
   Only script storage is currently queried by the engine cache membership API;
   other tag families require their own accurate allocation metadata. */
int mac_engine_script_cache_bind(const struct mac_native_scenario_scripts *);
int mac_engine_script_cache_unbind(const struct mac_native_scenario_scripts *);
#endif
