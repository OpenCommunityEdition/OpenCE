/* Runtime actions for MCC's own in-map widgets. Callback numbers are wire
 * values from the public UIEventHandlerReferenceFunction definition:
 * https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/ui_widget_definition.json
 * No Xbox/CE tag, callback table or settings implementation is rewritten. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "interface/event_manager.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "saved games/game_state.h"
#include "saved games/player_profile.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "mcc_cache.h"
#include "mcc_maps.h"
#include "mcc_pause.h"
#include "mcc_ui.h"
#include "mcc_ui_network.h"
#include "mcc_ui_teams.h"
#include "network_coop.h"
#include <string.h>

/* Only the public prefix shared by every widget is needed here. */
struct mcc_ui_widget_prefix {
    long definition;
    char const *name;
    short controller;
};
typedef char mcc_ui_controller_offset[offsetof(struct mcc_ui_widget_prefix, controller) == 8 ? 1 : -1];

char const *config_string(char const *name);
boolean pc_menu_tag(long tag_index);
boolean ui_widget_port_dispatch_event(struct widget_instance *widget, short type, short controller, boolean *deleted);

static short mcc_settings_controller = NONE;

void mcc_ui_settings_profile_released(void)
{
    mcc_settings_controller = NONE;
}

void mcc_ui_settings_close(short controller)
{
    if (mcc_settings_controller != NONE &&
        (controller == NONE || controller == mcc_settings_controller)) {
        mcc_settings_controller = NONE;
        player_ui_end_editing_profile();
    }
}

static boolean mcc_ui_settings_begin(short controller)
{
    long index;
    boolean fallback = FALSE;
    struct player_profile *profile;
    if (controller < 0 || controller >= 4 || local_player_count() != 1 ||
        player_ui_get_edit_player_profile() || player_ui_get_edit_playlist_profile()) return FALSE;
    mcc_settings_controller = NONE;
    index = player_ui_get_active_player_profile_index(controller);
    if (index != NONE) player_ui_begin_editing_profile(index);
    /* Retain the native player-1 convenience for console/quickstart games
     * without assigning that profile to a different local controller. */
    if (!player_ui_get_edit_player_profile() && controller == 0) {
        word count = 1;
        fallback = TRUE;
        index = player_ui_get_player1_last_used_profile_index();
        if (index != NONE) player_ui_begin_editing_profile(index);
        if (!player_ui_get_edit_player_profile()) {
            index = NONE;
            player_profiles_enumerate_available_to_local_player_index(controller, &count, &index, FALSE);
            if (count && index != NONE) player_ui_begin_editing_profile(index);
        }
    }
    profile = player_ui_get_edit_player_profile();
    if (!profile) {
        player_ui_end_editing_profile();
        return FALSE;
    }
    if (fallback) player_ui_set_active_player_profile(controller, index, profile);
    mcc_settings_controller = controller;
    return TRUE;
}

static boolean mcc_ui_owns_widget(struct widget_instance *widget)
{
    struct mcc_ui_widget_prefix const *prefix = (void const *)widget;
    return widget && mcc_cache_tags_loaded() && prefix->definition != NONE &&
        (mcc_pause_owns(prefix->definition) ||
         mcc_cache_contains(tag_get('DeLa', prefix->definition), 0x60));
}

boolean mcc_ui_trusted_action(struct widget_instance *widget, word function)
{
    struct mcc_ui_widget_prefix const *prefix = (void const *)widget;
    return widget && mcc_cache_tags_loaded() &&
        function >= MCC_PAUSE_ACTION_RESUME && function <= MCC_PAUSE_ACTION_SETTINGS &&
        mcc_pause_owns(prefix->definition);
}

boolean mcc_ui_settings_needed(char const *map_name)
{
    return map_name && mcc_level_name(map_name) && mcc_cache_tags_loaded() &&
        !mcc_maps_level_campaign(map_name) &&
        !csstrcasecmp(config_string("display.menus"), "pc");
}

static boolean mcc_ui_scenario_type(short type)
{
    /* main_get_map_name is the last solo selection; network rounds load
     * their own map without replacing it. Classify live actions using the
     * currently loaded MCC scenario, independently of the menu catalog. */
    return mcc_cache_tags_loaded() && global_scenario && global_scenario->type == type;
}

static short mcc_ui_controller(struct widget_instance *widget, struct event_record *event)
{
    short controller = event ? event->controller_index :
        ((struct mcc_ui_widget_prefix const *)widget)->controller;
    return controller >= 0 && controller < 4 ? controller : 0;
}

static boolean mcc_ui_failure(short controller, wchar_t const *text)
{
    display_error_text_deferred(text, controller);
    return FALSE;
}

static boolean mcc_ui_host_required(short controller)
{
    if (game_connection() == _game_connection_network_client)
        return mcc_ui_failure(controller, L"Only the host can restart or revert this game.");
    return TRUE;
}

static boolean mcc_ui_restart(short controller)
{
    if (!mcc_ui_host_required(controller)) return FALSE;
    if (global_network_game_server_get()) {
        if (!mcc_ui_restart_network_game())
            return mcc_ui_failure(controller, L"The game could not be restarted.");
    } else main_reset_map();
    return TRUE;
}

static boolean mcc_ui_end_round(short controller, boolean *deleted)
{
    /* End the round normally: connected players reach the carnage report
     * and the host can pick the next map in the native lobby. */
    if (!global_network_game_server_get())
        return mcc_ui_failure(controller, L"Only the host can choose a new game.");
    if (!game_engine_running() || !game_engine_can_score())
        return mcc_ui_failure(controller, L"This round is already ending.");
    /* game_engine_end_game closes the complete widget tree immediately.
     * The dispatcher must never inspect this widget again afterward. */
    *deleted = TRUE;
    game_engine_end_game();
    return TRUE;
}

boolean mcc_ui_new_game(struct widget_instance *widget, boolean *deleted)
{
    struct mcc_ui_widget_prefix const *prefix = (void const *)widget;
    char const *name;
    if (!mcc_ui_owns_widget(widget)) return FALSE;
    name = tag_get_name(prefix->definition);
    if (!name || strcmp(name, "ui\\shell\\multiplayer_game\\pause_game\\new_game_button")) return FALSE;
    mcc_ui_end_round(mcc_ui_controller(widget, NULL), deleted);
    return TRUE;
}

static boolean mcc_ui_request_team(short team, short controller)
{
    struct network_game *game = network_game_get_game();
    short machine = network_game_client_get_local_machine_index();
    long i;
    if (!game || !game->variant.universal_variant.teams)
        return mcc_ui_failure(controller, L"This game does not use teams.");
    if (!mcc_ui_team_balance_allows(machine, controller, team))
        return mcc_ui_failure(controller, L"The host's automatic team balance prevents this team change.");
    for (i = 0; i < (long)NUMBEROF(game->players); i++) {
        struct network_player player = game->players[i];
        if (network_player_is_valid(&player) && player.machine_index == machine &&
            player.controller_index == controller) {
            player.team_index = (char)team;
            if (network_game_client_update_local_player_data(global_network_game_client_get(), &player))
                return TRUE;
            break;
        }
    }
    return mcc_ui_failure(controller, L"The team change could not be sent to the host.");
}

static boolean mcc_ui_choose_team(struct widget_instance *widget, short controller)
{
    struct mcc_ui_widget_prefix const *prefix = (void const *)widget;
    char const *name = tag_get_name(prefix->definition), *leaf;
    leaf = name ? strrchr(name, '\\') : NULL;
    leaf = leaf ? leaf + 1 : name;
    if (leaf && !strcmp(leaf, "red_team_button")) return mcc_ui_request_team(0, controller);
    if (leaf && !strcmp(leaf, "blue_team_button")) return mcc_ui_request_team(1, controller);
    return mcc_ui_failure(controller, L"This team choice is not supported.");
}

static void mcc_ui_close_for_controller(short controller, boolean *deleted)
{
    /* A generated confirmation has its pause screen in this controller's
     * history. Close both, preserving every other split-screen player's UI.
     * Publish deletion before the native helper can free the caller. */
    *deleted = TRUE;
    ui_widgets_close_all_for_local_player(controller);
}

boolean mcc_ui_event_function(struct widget_instance *widget,
    struct event_record *event, word function, boolean *deleted, boolean *result)
{
    short controller;
    if (!mcc_ui_owns_widget(widget)) return FALSE;
    controller = mcc_ui_controller(widget, event);
    *result = TRUE;
    if (function >= MCC_PAUSE_ACTION_RESUME && function <= MCC_PAUSE_ACTION_SETTINGS) {
        if (!mcc_ui_trusted_action(widget, function)) {
            *result = FALSE;
            return TRUE;
        }
        switch (function) {
        case MCC_PAUSE_ACTION_RESUME:
            mcc_ui_close_for_controller(controller, deleted);
            return TRUE;
        case MCC_PAUSE_ACTION_REVERT: function = 11; break;
        case MCC_PAUSE_ACTION_RESTART: function = 12; break;
        case MCC_PAUSE_ACTION_SAVE: function = 179; break;
        case MCC_PAUSE_ACTION_QUIT:
            if (network_coop_active() || mcc_ui_scenario_type(0)) function = 13;
            else {
                /* Cache all caller state and mark deletion before native
                 * leave helpers, which may synchronously close widgets. */
                boolean split_screen = local_player_count() > 1;
                *deleted = TRUE;
                if (global_network_game_client_get()) {
                    network_game_client_local_player_quit(controller);
                    if (split_screen) player_ui_local_player_left_multiplayer_game(controller);
                } else main_goto_main_menu();
                mcc_ui_close_for_controller(controller, deleted);
                return TRUE;
            }
            break;
        case MCC_PAUSE_ACTION_END_GAME:
            *result = mcc_ui_end_round(controller, deleted);
            return TRUE;
        case MCC_PAUSE_ACTION_RED_TEAM:
        case MCC_PAUSE_ACTION_BLUE_TEAM:
            *result = mcc_ui_request_team(function == MCC_PAUSE_ACTION_RED_TEAM ? 0 : 1, controller);
            if (*result) mcc_ui_close_for_controller(controller, deleted);
            return TRUE;
        case MCC_PAUSE_ACTION_SETTINGS: function = 137; break;
        }
    }
    switch (function) {
    case 11: /* Confirm revert to checkpoint. */
        if (!mcc_ui_host_required(controller)) { *result = FALSE; break; }
        if (!game_state_port_saved_game_valid()) {
            *result = mcc_ui_failure(controller, L"There is no saved checkpoint to revert to.");
            break;
        }
        main_revert_map(); /* Existing engine host-revert path synchronizes co-op. */
        break;
    case 12: /* Confirm restart level. */
    case 163: /* PC multiplayer options: restart game. */
        *result = mcc_ui_restart(controller);
        break;
    case 13: /* Campaign quit: retain this MCC campaign's checkpoint. */
        if (network_coop_active()) {
            short i;
            for (i = 0; i < 4; i++) network_game_client_local_player_quit(i);
        } else {
            if (mcc_ui_scenario_type(0) && game_state_port_saved_game_valid())
                game_state_save_to_persistent_storage();
            main_goto_main_menu();
        }
        break;
    case 108: /* Mouse accept: use the same A/confirmation route as a controller. */
    case 109:
    case 110:
    case 111:
    case 128:
        event_manager_post_button(controller,
            function == 108 ? 0 : function == 109 ? 1 : function == 110 ? 10 : function == 111 ? 11 : 2);
        break;
    case 137: /* Open trusted native settings, never the map's PC configuration widgets. */
        if (!mcc_ui_scenario_type(1) || csstrcasecmp(config_string("display.menus"), "pc") ||
            local_player_count() != 1 ||
            !pc_menu_tag(tag_loaded('DeLa', "pc\\main_menu\\settings_select\\player_setup\\player_profile_edit\\player_profile_edit_screen")) ||
            !mcc_ui_settings_begin(controller)) {
            *result = mcc_ui_failure(controller, L"Settings could not be opened for this player.");
            break;
        }
        {
            struct mcc_ui_widget_prefix const *root = (void const *)widget_instance_get_topmost_parent(widget);
            long root_tag = root->definition;
            /* The native port-open helper always uses stack zero. Capture
             * the caller's history first and open on its actual controller;
             * a successful load synchronously frees the old widget tree. */
            *result = ui_widget_load_by_name_or_tag(
                "pc\\main_menu\\settings_select\\player_setup\\player_profile_edit\\player_profile_edit_screen",
                NONE, NULL, controller, root_tag, NONE, NONE) != NULL;
            if (*result) *deleted = TRUE;
            else mcc_ui_settings_close(controller);
        }
        break;
    case 155: /* PC pause/options creation has no engine state to initialize. */
    case 156:
        break;
    case 157:
        *result = mcc_ui_choose_team(widget, controller);
        break;
    case 164:
        *result = ui_widget_port_dispatch_event(widget_instance_get_topmost_parent(widget), 32, controller, deleted);
        break;
    case 179: /* Explicit save checkpoint from the campaign pause screen. */
        if (game_connection() == _game_connection_network_client) {
            *result = mcc_ui_failure(controller, L"Only the host can save a checkpoint.");
            break;
        }
        game_state_save();
        if (!game_state_port_saved_game_valid()) {
            *result = mcc_ui_failure(controller, L"The checkpoint could not be saved.");
            break;
        }
        if (!network_coop_active() && mcc_ui_scenario_type(0))
            game_state_save_to_persistent_storage();
        break;
    default:
        /* Native-compatible IDs still pass through the original dispatcher
         * and its map-widget permissions. Unknown PC callbacks must fail so
         * their open/close flags cannot claim an operation succeeded. */
        if (function < 102) return FALSE;
        *result = mcc_ui_failure(controller, L"This map's menu action is not supported by the native game.");
        break;
    }
    return TRUE;
}
