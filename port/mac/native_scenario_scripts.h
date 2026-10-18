#ifndef HALO_MAC_NATIVE_SCENARIO_SCRIPTS_H
#define HALO_MAC_NATIVE_SCENARIO_SCRIPTS_H
#include "retail_cache.h"
struct scenario;
struct mac_native_scenario_scripts { void *state; };
/* Owned native syntax storage, console strings, scripts and globals for
   retail scenarios. Calls and engine globals are renumbered by name to this
   build's tables (when the game's hs.c is linked); scenarios with source
   files are refused. The tags the scripts name are kept for the session. */
int mac_native_scenario_scripts_decode(const struct mac_retail_cache *,
    struct mac_native_scenario_scripts *,const char **);
void mac_native_scenario_scripts_free(struct mac_native_scenario_scripts *);
int mac_native_scenario_scripts_attach(const struct mac_native_scenario_scripts *,struct scenario *);
/* Allocation membership for the native cache provider. No raw-map fallback.
   This alone does not replace cache_file_tag_cache_contains in the engine. */
int mac_native_scenario_scripts_contains(const struct mac_native_scenario_scripts *,const void *,size_t);
/* The tags the scripts name (hs_references), as group and handle; fails when
   the capacity is too small. */
int mac_native_scenario_scripts_references(const struct mac_native_scenario_scripts *,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *count);
/* The structure bsp a campaign level's scripts switch to first (see the
   source); 0 for a multiplayer map or when none is found. */
int mac_native_scenario_scripts_start_bsp(const struct mac_retail_cache *,uint32_t *bsp);
#endif
