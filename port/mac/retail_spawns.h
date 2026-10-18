#ifndef HALO_MAC_RETAIL_SPAWNS_H
#define HALO_MAC_RETAIL_SPAWNS_H
#include "retail_cache.h"
#define MAC_RETAIL_MAX_SPAWNS 256
struct mac_retail_spawn {
    float position[3], facing;
    int16_t team, bsp, game_types[4];
};
/* Decode all records before publishing output. Facing is radians about +Z;
   the engine's forward vector is (cos(facing),sin(facing),0), up is +Z.
   Caller supplies a validated immutable cache from mac_retail_open. */
int mac_retail_spawns(const struct mac_retail_cache *,
    struct mac_retail_spawn out[MAC_RETAIL_MAX_SPAWNS],
    uint32_t *count, const char **error);
/* Matches game_engine.c match_game_type, including its non-team wildcard
   meaning "not CTF". This is not a complete spawn rating/selection function:
   team restrictions, nearby vehicles, enemies and friendly bonuses belong
   to the engine. No implicit team or BSP filtering is introduced here. */
int mac_retail_spawn_game_matches(const struct mac_retail_spawn *, int game_type,
    int engine_running);
#endif
