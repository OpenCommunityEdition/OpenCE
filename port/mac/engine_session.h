#ifndef HALO_MAC_ENGINE_SESSION_H
#define HALO_MAC_ENGINE_SESSION_H

#include "engine_cache_files.h"
#include "engine_world.h"
#include "native_actor_assets.h"
#include "retail_spawns.h"

/* A real offline retail-v7 map session. Initialize the structure to all-zero
   before its first open; open rejects an already-owned session. This owner
   validates and maps the selected map/resources, converts the selected
   spawn's BSP and actor assets, and binds only native-owned tags/resources.
   It does not use Xbox's raw tag
   cache loader, initialize the platform, or claim network/CE support. */
struct mac_engine_session {
    struct mac_cache_files files;
    struct mac_actor_assets assets;
    struct mac_engine_world world;
    struct mac_retail_spawn spawns[MAC_RETAIL_MAX_SPAWNS];
    uint32_t spawn_count,spawn_index,bsp_index;
    int bound;
    /* port: the title screen's map (ui.map, scenario type main menu): no
       player start points; the scenes and scripts show the camera */
    int main_menu;
};

int mac_engine_session_open(const char *map_path,const char *bitmaps_path,
    const char *sounds_path,uint32_t spawn_index,struct mac_engine_session *,
    const char **error);
int mac_engine_session_bind(struct mac_engine_session *);
int mac_engine_session_unbind(struct mac_engine_session *);
int mac_engine_session_scope(const struct mac_engine_session *,
    int (*operation)(void *),void *context);
const struct mac_retail_cache *mac_engine_session_cache(const struct mac_engine_session *);
int mac_engine_session_spawn(const struct mac_engine_session *,
    struct mac_retail_spawn *spawn);
void mac_engine_session_close(struct mac_engine_session *);

#endif
