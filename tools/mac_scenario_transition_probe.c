/* Negative checks for the native game.c scenario-load dispatch. */
#include "cseries.h"
#include "scenario/scenario.h"
#include "game/game_globals.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "engine_scenario_transition.h"

extern struct structure_bsp *global_structure_bsp;
extern struct collision_bsp *global_collision_bsp;
extern struct bsp3d *global_bsp3d;

int mac_scenario_transition_rejects_unscoped_load(void)
{
    struct scenario *scenario=global_scenario;
    struct structure_bsp *structure=global_structure_bsp;
    struct collision_bsp *collision=global_collision_bsp;
    struct bsp3d *bsp=global_bsp3d;
    struct game_globals *globals=global_game_globals;
    long scenario_index=global_scenario_index;
    short bsp_index=global_structure_bsp_index;
    short scenario_bsp_index=scenario_globals?scenario_globals->structure_bsp_index:NONE;

    if(mac_engine_scenario_load("bloodgulch"))return 0;
    return scenario==global_scenario && structure==global_structure_bsp &&
        collision==global_collision_bsp && bsp==global_bsp3d &&
        globals==global_game_globals && scenario_index==global_scenario_index &&
        bsp_index==global_structure_bsp_index &&
        (!scenario_globals || scenario_bsp_index==scenario_globals->structure_bsp_index);
}

int mac_scenario_transition_rejects_wrong_map(void)
{
    struct scenario *scenario=global_scenario;
    struct structure_bsp *structure=global_structure_bsp;
    struct collision_bsp *collision=global_collision_bsp;
    struct bsp3d *bsp=global_bsp3d;
    struct game_globals *globals=global_game_globals;
    long scenario_index=global_scenario_index;
    short bsp_index=global_structure_bsp_index;
    short scenario_bsp_index=scenario_globals?scenario_globals->structure_bsp_index:NONE;

    if(!scenario || !structure || !collision || !bsp ||
       mac_engine_scenario_load("not-the-loaded-map") ||
       mac_engine_scenario_load(NULL))return 0;
    return scenario==global_scenario && structure==global_structure_bsp &&
        collision==global_collision_bsp && bsp==global_bsp3d &&
        globals==global_game_globals && scenario_index==global_scenario_index &&
        bsp_index==global_structure_bsp_index && scenario_globals &&
        scenario_bsp_index==scenario_globals->structure_bsp_index;
}
