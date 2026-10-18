/* Complete game implementation with a checked shared core initializer.
   Compile instead of game.c. Full game_initialize retains its service order. */
#include "cseries.h"
#include "engine_scenario_transition.h"
/* Route only the authentic game.c map entry points through the native
   session owner. The native provider validates and adopts its already-owned
   scenario/BSP; it never enters the retail raw-cache loader. */
#define scenario_load mac_engine_scenario_load
#define scenario_unload mac_engine_scenario_unload
#include "mac_game_state_body.h"
#undef scenario_unload
#undef scenario_load
#include "engine_game_runtime.h"
#include <string.h>
/* Native storage for the engine-declared variant, initialized by the core. */
struct game_variant game_variant_global;
static int mac_core_owned;
static struct game_variant mac_previous_variant;
int mac_game_core_start(void)
{
    if(mac_core_owned || game_globals)return 0;
    mac_previous_variant=game_variant_global;
    mac_game_initialize_core();
    mac_core_owned=1;
    return 1;
}
int mac_game_core_stop(void)
{
    if(!mac_core_owned || !game_globals || game_globals->active || game_globals->map_loaded)return 0;
    /* The arena owns the allocation; release the reference before its teardown. */
    game_globals=NULL;game_variant_global=mac_previous_variant;mac_core_owned=0;
    return 1;
}
int mac_game_load_native_map(const char *map_name)
{
    struct game_options options;
    size_t bytes;
    if(!map_name || !*map_name)return 0;
    bytes=strlen(map_name);
    if(bytes>=sizeof(options.map_name))return 0;
    game_options_new(&options);
    memcpy(options.map_name,map_name,bytes+1);
    if(!game_options_verify(&options))return 0;
    return game_load(&options)?1:0;
}
