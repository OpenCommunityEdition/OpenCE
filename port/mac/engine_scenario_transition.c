#include "cseries.h"
#include "game/game_globals.h"
#include "game/game.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "tag_files/tag_files.h"
#include "objects/objects.h"
#include "objects/object_lights.h"
#include "objects/object_types.h"
#include "ai/ai.h"
#include "effects/effects.h"
#include "effects/particles.h"
#include "effects/particle_systems.h"
#include "effects/contrails.h"
#include "effects/decals.h"
#include "structures/structures.h"
#include "camera/observer.h"
#include "game/players.h"
#include "sound/sound_manager.h"
#include "cache/sound_cache.h"
#include "main/main.h"
#include "physics/collision_usage.h"
#include "engine_scenario_transition.h"
#include "engine_cache_files.h"
#include "native_model_buffers.h"

extern struct structure_bsp *global_structure_bsp;
extern struct collision_bsp *global_collision_bsp;
extern struct bsp3d *global_bsp3d;

static int ascii_lower(int c)
{
    if(c>='A' && c<='Z')return c-'A'+'a';
    return c;
}

static const char *path_leaf(const char *path)
{
    const char *leaf=path;
    const char *p;
    if(!path)return NULL;
    for(p=path;*p;++p)if(*p=='/' || *p=='\\')leaf=p+1;
    return leaf;
}

/* port: what the world published for this session scope
   (mac_engine_world_scope in engine_world.c), kept when a game-map
   transition releases the source globals below. Starting a network game
   unloads the map and loads it again (network_game_manager.c), and the
   world's scenario, bsp and tags outlive that: only these pointers are let
   go, so the next load of the same map can put them back. */
static struct scenario *held_scenario;
static struct structure_bsp *held_structure_bsp;
static struct collision_bsp *held_collision_bsp;
static struct bsp3d *held_bsp3d;
static long held_scenario_index=NONE;
static short held_structure_bsp_index=NONE;

static int same_leaf(const char *requested,const char *tag_name)
{
    const char *a=path_leaf(requested),*b=path_leaf(tag_name);
    if(!a || !b || !*a || !*b)return 0;
    while(*a && *b && ascii_lower((unsigned char)*a)==ascii_lower((unsigned char)*b)) {
        ++a;++b;
    }
    return !*a && !*b;
}

boolean mac_engine_scenario_load(const char *map_name)
{
    struct scenario_structure_bsp_reference *reference;
    long globals_index;
    struct game_globals *globals;
    const char *scenario_name;

    /* port: an Xbox map is loaded by the engine's own scenario_load, which
       reads the map's tags and first bsp as the console does */
    if(mac_cache_files_xbox_active()) {
        if(game_map_loaded() || game_in_progress() || !map_name || !*map_name)return FALSE;
        if(!scenario_load(map_name))return FALSE;
        mac_native_model_buffers_certify_xbox();
        return TRUE;
    }
    /* This dispatch is valid only inside the immutable native session scope:
       the world owns and has already published the converted scenario/BSP. */
    if(game_map_loaded() || game_in_progress() || !map_name || !*map_name)
        return FALSE;
    /* port: the second load of a session, which starting a network game asks
       for after game_unload. The world still owns this map; take back what
       the unload let go, and only for the map it holds, so that a request
       for another one is refused here as it was before. */
    if(!global_scenario && held_scenario && held_scenario_index!=NONE &&
       same_leaf(map_name,tag_get_name(held_scenario_index)))
    {
        global_scenario=held_scenario;
        global_structure_bsp=held_structure_bsp;
        global_collision_bsp=held_collision_bsp;
        global_bsp3d=held_bsp3d;
        global_scenario_index=held_scenario_index;
        global_structure_bsp_index=held_structure_bsp_index;
    }
    if(!global_scenario || !global_structure_bsp ||
       !global_collision_bsp || !global_bsp3d || !scenario_globals ||
       global_scenario_index==NONE || global_structure_bsp_index<0 ||
       global_structure_bsp_index>=global_scenario->structure_bsp_references.count ||
       global_structure_bsp_index>=MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO)
        return FALSE;

    scenario_name=tag_get_name(global_scenario_index);
    if(!same_leaf(map_name,scenario_name))return FALSE;

    reference=TAG_BLOCK_GET_ELEMENT(&global_scenario->structure_bsp_references,
        global_structure_bsp_index,struct scenario_structure_bsp_reference);
    if(!reference || reference->structure_bsp.group_tag!=STRUCTURE_BSP_TAG ||
       reference->structure_bsp.index==NONE)
        return FALSE;
    /* port: any one bsp the session's world holds, not only the first: a
       campaign level starts in the bsp its scripts switch to first
       (native_scenario_scripts.c) */

    globals_index=tag_loaded(GAME_GLOBALS_TAG,"globals\\globals");
    if(globals_index==NONE || !tag_index_is_group(globals_index,GAME_GLOBALS_TAG))
        return FALSE;
    globals=game_globals_definition_get(globals_index);
    if(!globals)return FALSE;

    /* Initial scenario_load has no old BSP. The source switch routine calls
       reconnect consumers only when had_old_structure_bsp is true. On first
       load their data arrays become valid later in game_initialize_for_new_map;
       reconnecting here would traverse uninitialized object/particle arrays.
       This native adapter loads the map the world holds, the first time and
       again after an unload, but never replaces a live BSP with another. */
    /* scenario_tags_load opens the original sound cache before any sound
       request. The native tag owner replaces loading, not this lifecycle. */
    sound_cache_open();
    main_stop_time();
    collision_log_enable(FALSE);
    global_game_globals=globals;
    scenario_globals->structure_bsp_index=global_structure_bsp_index;
    collision_log_enable(TRUE);
    main_start_time();
    return TRUE;
}

void mac_engine_scenario_unload(void)
{
    if(mac_cache_files_xbox_active()) {
        mac_native_model_buffers_release_xbox();
        scenario_unload();
        return;
    }
    /* Match scenario_tags_unload after sound_dispose_from_old_map, while the
       native sound permutations and bound cache files are still alive. */
    sound_cache_close();
    /* The native world/tag owners outlive this game-map transition. Release
       only source globals exactly as scenario_unload does after cache unload,
       keeping them so a load of the same map can take them back. */
    held_scenario=global_scenario;
    held_structure_bsp=global_structure_bsp;
    held_collision_bsp=global_collision_bsp;
    held_bsp3d=global_bsp3d;
    held_scenario_index=global_scenario_index;
    held_structure_bsp_index=global_structure_bsp_index;
    if(scenario_globals)scenario_globals->structure_bsp_index=NONE;
    global_scenario_index=NONE;
    global_structure_bsp_index=NONE;
    global_scenario=NULL;
    global_structure_bsp=NULL;
    global_collision_bsp=NULL;
    global_bsp3d=NULL;
    global_game_globals=NULL;
}
