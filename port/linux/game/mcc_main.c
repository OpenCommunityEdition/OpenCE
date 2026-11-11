/* MCC transitions use the native menu lifecycle, without selecting Xbox
 * campaign progression or its credits movie for a standalone MCC scenario. */
#include "cseries.h"
#include "errors.h"
#include "main/main.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "mcc_cache.h"
#include "mcc_main.h"
#include "mcc_errors.h"
#include "mcc_maps.h"
#include "mcc_progress.h"
#include "mcc_ui.h"
#include "game/game.h"
#include "networking/network_server_manager.h"
#include "networking/network_game_manager.h"
#include "../src/port_config.h"

boolean mcc_main_campaign_won(void)
{
    struct network_game *game;
    char const *current;
    char next[64];
    wchar_t text[512];
    int state;
    boolean server;
    if (!mcc_cache_tags_loaded() || mcc_cache_scenario_type() != 0 ||
        !config_boolean("game.mcc_campaigns")) return FALSE;
    /* Clients follow the host's existing round/settings messages. */
    if (game_connection() == _game_connection_network_client) return TRUE;
    server = game_connection() == _game_connection_network_server;
    game = server ? network_game_get_game() : NULL;
    current = game ? game->map.name : main_get_map_name();
    mcc_progress_complete(current, game_difficulty_level_get());
    state = mcc_maps_next_mission(current, next);
    if (state < 0) {
        error(_error_silent, "mcc campaign: next mission for %s is unavailable: %s", current, next);
        mcc_error_text(current, "The next campaign mission is missing or invalid. Install the complete package; no mission was skipped.", text, 512);
        display_error_text_deferred(text, 0);
    }
    if (server) {
        /* Ending a campaign returns to the lobby on its last selection;
         * it never wraps to the first mission or chooses an Xbox level. */
        mcc_ui_network_campaign_won(state == 1 ? next : NULL);
    } else if (state == 1) {
        error(_error_silent, "mcc campaign: advancing from %s to %s", current, next);
        main_set_map_name(next);
        main_disallow_persistent_storage();
    } else {
        error(_error_silent, "mcc campaign: %s completed; returning to the menu", current);
        main_goto_main_menu();
    }
    return TRUE;
}

boolean mcc_main_map_completed(void)
{
    if (!mcc_cache_tags_loaded()) return FALSE;
    /* Unknown campaign levels reach main_roll_credits. MCC packages have
     * no native next-level mapping: return normally instead of starting the
     * Xbox outro immediately after menu precaching. That movie borrows and
     * protects texture-cache memory while menu reads may still be pending. */
    error(_error_silent, "mcc: scenario completed; returning to the menu (select the next campaign map manually)");
    main_goto_main_menu();
    return TRUE;
}

boolean mcc_main_load_failed(char const *name)
{
    wchar_t text[512];
    if (!mcc_level_name(name)) return FALSE;
    error(_error_silent, "mcc: returning to the menu after failed load of %s", name);
    mcc_error_text(name,"Its level could not be prepared. See debug.txt for the failing setup check.",text,512);
    display_error_text_when_main_menu_loaded(text);
    errors_clear();
    main_goto_main_menu();
    main_menu_load();
    return TRUE;
}

boolean mcc_main_network_load_failed(char const *name)
{
    wchar_t text[512];
    if (!mcc_level_name(name)) return FALSE;
    error(_error_silent, "mcc: ending the network session after failed load of %s", name);
    mcc_error_text(name,"Its level could not be prepared. See debug.txt for the failing setup check.",text,512);
    display_error_text_when_main_menu_loaded(text);
    errors_clear();
    /* This runs inside a network receive callback. Queue both operations:
     * the next frame needs the menu's globals before input_abstraction_update,
     * which runs before the normal network abort is processed. Do not free
     * the connection or load the menu on this receive stack. */
    network_game_abort();
    main_goto_main_menu();
    return TRUE;
}
