#include "cseries/cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_engine_list.h"
#include "game/game_engine_playlist.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "interface/player_ui.h"
#include "port/mac/engine_game_variant.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int mac_slayer_variant_selected;
static int mac_slayer_map_initialized;
/* A campaign level runs with no game engine at all (game_engine NULL, the
   zeroed game variant game_initialize leaves), as the solo game does. */
static int mac_campaign_selected;
static int mac_campaign_map_initialized;

int mac_engine_game_variant_select_campaign(const char **error)
{
    if (error)
        *error = NULL;
    if (game_in_progress() || game_map_loaded()) {
        if (error)
            *error = "cannot select the campaign after map loading";
        return 0;
    }
    if (game_engine) {
        if (error)
            *error = "game engine is already selected";
        return 0;
    }
    /* port: no game variant either (as main_load_ui_scenario does): a game
       played before this leaves its variant, which game_initialize_for_new_map
       would start the engine of */
    game_set_game_variant(NULL);
    mac_campaign_selected = 1;
    mac_campaign_map_initialized = 0;
    mac_slayer_variant_selected = 0;
    mac_slayer_map_initialized = 0;
    return 1;
}

static int select_variant(int use_menu_choice, const char **error)
{
    struct game_variant variant;
    struct game_variant_options expected_options;
    struct game_variant_options const *chosen_options = NULL;

    if (error)
        *error = NULL;
    if (game_in_progress() || game_map_loaded()) {
        if (error)
            *error = "cannot select a game variant after map loading";
        return 0;
    }
    if (game_engine) {
        if (error)
            *error = "game engine is already selected";
        return 0;
    }

    /* Use the game's actual stock preset. A zeroed structure with only the
       engine index set has score_to_win == 0, which makes Slayer end the
       game as soon as the zero-score player is updated. */
    build_game_variant_slayer(&variant);
    /* port: the gametype the menus chose (Create Game's Select Gametype), with
       its PC options; Slayer's stock preset when none was */
    if (use_menu_choice) {
        struct game_variant chosen;
        if (player_ui_game_variant_specified(&chosen) && chosen.game_engine_index &&
            chosen.game_engine_index <= last_usable_game_engine_index) {
            variant = chosen;
            chosen_options = player_ui_get_game_variant_options();
        }
    }
    /* HALO_MAC_WEAPON_SET (diagnostic): the variant's weapon set, so a hidden
       run starts with other weapons (3: plasma, 6: rocket launchers) */
    if (getenv("HALO_MAC_WEAPON_SET"))
        variant.universal_variant.weapon_set = atol(getenv("HALO_MAC_WEAPON_SET"));
    game_set_game_variant(&variant);
    if (chosen_options)
        game_set_game_variant_options(chosen_options);
    game_engine_initialize(&variant);

    if (!game_engine || game_engine->type != game_engine_get_variant()->game_engine_index) {
        game_engine_dispose();
        if (error)
            *error = "the game engine selection failed";
        return 0;
    }
    if (!chosen_options) {
        if (game_engine != game_engines[game_engine_slayer] ||
            game_engine->type != game_engine_slayer) {
            game_engine_dispose();
            if (error)
                *error = "stock Slayer game engine selection failed";
            return 0;
        }
        if (game_engine_get_variant()->universal_variant.score_to_win !=
            variant.universal_variant.score_to_win ||
            game_engine_get_variant()->universal_variant.score_to_win <= 0) {
            game_engine_dispose();
            if (error)
                *error = "stock Slayer score target is not initialized";
            return 0;
        }

        game_variant_options_default(&variant, &expected_options);
        if (csmemcmp(game_variant_options_get(), &expected_options,
            sizeof(expected_options)) != 0) {
            game_engine_dispose();
            if (error)
                *error = "stock Slayer options differ from source defaults";
            return 0;
        }
    }

    /* HALO_MAC_VEHICLES_PC (diagnostic): every vehicle the variant's sets could
       leave out may be made (the PC vehicle set), so a hidden run can try
       the banshee and the rocket warthog */
    if (getenv("HALO_MAC_VEHICLES_PC")) {
        struct game_variant_options *options = (struct game_variant_options *)game_variant_options_get();
        options->vehicle_set[0] = options->vehicle_set[1] = VARIANT_VEHICLE_SET_PC;
    }
    /* port: after a campaign level (the menus start games one after another) */
    mac_campaign_selected = 0;
    mac_campaign_map_initialized = 0;
    mac_slayer_variant_selected = 1;
    mac_slayer_map_initialized = 0;
    return 1;
}

/* diagnostic: the gametype that started and the goal objects it placed
   (the flags, balls) with the map's netgame flags */
void mac_engine_game_variant_report(void)
{
    struct object_iterator iterator;
    struct scenario *scenario = global_scenario_try_and_get();
    long flags = 0, weapons = 0, goals = 0;
    void *object;

    object_iterator_new(&iterator, _object_mask_item, 0);
    while ((object = object_iterator_next(&iterator)) != NULL) {
        const char *name = tag_get_name(((struct object_datum *)object)->definition_index);
        ++weapons;
        if (name && (strstr(name, "flag") || strstr(name, "ball")))
            ++goals;
    }
    if (scenario)
        flags = scenario->netgame_flags.count;
    fprintf(stderr, "game engine: type %d (variant engine %d), %ld netgame flags, %ld items, %ld flag/ball items\n",
        game_engine ? game_engine->type : 0, (int)game_engine_get_variant()->game_engine_index, flags, weapons, goals);
}

int mac_engine_game_variant_select_slayer(const char **error)
{
    return select_variant(0, error);
}

/* port: the menus' gametype (Slayer's when they chose none) */
int mac_engine_game_variant_select_multiplayer(const char **error)
{
    return select_variant(1, error);
}

int mac_engine_game_variant_initialize_map(const char **error)
{
    struct scenario *scenario;

    if (error)
        *error = NULL;
    if (mac_campaign_selected) {
        scenario = global_scenario_try_and_get();
        /* port: the title screen's map (ui.map, scenario type main menu) runs
           as a campaign level does, with no game engine and no start point */
        if (!scenario || (scenario->type != _scenario_type_solo &&
            scenario->type != _scenario_type_main_menu)) {
            if (error)
                *error = "the loaded scenario is not a campaign level";
            return 0;
        }
        if (!game_map_loaded()) {
            if (error)
                *error = "game map is not loaded";
            return 0;
        }
        game_initialize_for_new_map();
        if (game_engine) {
            if (error)
                *error = "campaign new-map initialization selected a game engine";
            return 0;
        }
        mac_campaign_map_initialized = 1;
        return 1;
    }
    if (!mac_slayer_variant_selected || !game_engine) {
        if (error)
            *error = "no multiplayer game variant is selected";
        return 0;
    }
    scenario = global_scenario_try_and_get();
    /* (any multiplayer map's rows: Blood Gulch has 37, others more or none) */
    if (!scenario || scenario->netgame_equipment.count < 0 ||
        (scenario->netgame_equipment.count && !scenario->netgame_equipment.address)) {
        if (error)
            *error = "loaded scenario does not expose its native netgame equipment rows";
        return 0;
    }
    if (!game_map_loaded()) {
        if (error)
            *error = "game map is not loaded";
        return 0;
    }

    /* Follow the public game.c lifecycle. This initializes map services and
       lets the real game initializer publish active state after objects_place. */
    game_initialize_for_new_map();
    if (!game_engine) {
        if (error)
            *error = "the gametype's new-map initialization failed";
        return 0;
    }
    mac_slayer_map_initialized = 1;
    return 1;
}

int mac_engine_game_variant_update(const char **error)
{
    struct scenario *scenario;

    if (error)
        *error = NULL;
    if (mac_campaign_selected) {
        if (!mac_campaign_map_initialized || game_engine) {
            if (error)
                *error = "campaign map initialization has not completed";
            return 0;
        }
        return 1;
    }
    if (!mac_slayer_map_initialized || !game_engine) {
        if (error)
            *error = "the multiplayer map initialization has not completed";
        return 0;
    }
    if (!game_in_progress() || !game_map_loaded()) {
        if (error)
            *error = "game is not in progress on a loaded map";
        return 0;
    }

    scenario = global_scenario_try_and_get();
    if (!scenario || scenario->netgame_equipment.count < 0 ||
        (scenario->netgame_equipment.count && !scenario->netgame_equipment.address)) {
        if (error)
            *error = "scenario equipment rows are unavailable to game_engine_update";
        return 0;
    }

    game_engine_update();
    return 1;
}
