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
#include "mcc_identity.h"
#include "mcc_package.h"
#include "../src/port_config.h"
#include <string.h>

/* MCC's map_name: the map it names, and the map whose script named it. */
static char mcc_main_requested[64], mcc_main_requesting[64];

static char const *mcc_main_current_map(void)
{
    struct network_game *game = game_connection() == _game_connection_network_server ?
        network_game_get_game() : NULL;
    return game ? game->map.name : main_get_map_name();
}

/* A map in the same package as the current one, by its file name (MCC
 * scripts name maps as Halo's levels do, without a folder or extension). */
static boolean mcc_main_sibling_map(char const *current, char const *name, char next[64])
{
    char canonical[64];
    char const *base, *separator;
    size_t prefix;
    if (!name || !mcc_package_name(mcc_identity_local_name(current), canonical)) return FALSE;
    base = name;
    for (separator = name; *separator; separator++)
        if (*separator == '\\' || *separator == '/') base = separator + 1;
    separator = strrchr(canonical, '\\');
    prefix = (size_t)(separator - canonical) + 1;
    if (!*base || prefix + strlen(base) >= 64) return FALSE;
    memcpy(next, canonical, prefix);
    strcpy(next + prefix, base);
    if (!mcc_package_name(next, canonical)) return FALSE;
    strcpy(next, canonical);
    return TRUE;
}

void mcc_main_request_map(char const *name)
{
    char const *current;
    if (!mcc_cache_tags_loaded() || game_connection() == _game_connection_network_client || !name) return;
    current = mcc_main_current_map();
    if (!mcc_main_sibling_map(current, name, mcc_main_requested)) {
        error(_error_silent, "mcc: map_name names no map of this package: %.64s", name);
        csstrncpy(mcc_main_requested, name, sizeof(mcc_main_requested) - 1);
        mcc_main_requested[sizeof(mcc_main_requested) - 1] = 0;
    }
    csstrncpy(mcc_main_requesting, current, sizeof(mcc_main_requesting) - 1);
    mcc_main_requesting[sizeof(mcc_main_requesting) - 1] = 0;
    /* Change maps where a won mission does: after the scripts' update, in
     * the main loop, with the host moving every machine of a network game. */
    main_won_map();
}

/* A map named by map_name, or an error when it is not installed here. The
 * request belongs to the map that made it; a stale one is dropped. */
static boolean mcc_main_requested_map(char const *current, boolean server)
{
    wchar_t text[512];
    boolean requested = mcc_main_requested[0] && !csstrcasecmp(current, mcc_main_requesting);
    char next[64];
    strcpy(next, mcc_main_requested);
    mcc_main_requested[0] = mcc_main_requesting[0] = 0;
    if (!requested) return FALSE;
    if (mcc_maps_find(next) == NONE) {
        error(_error_silent, "mcc: map_name's map %s is unavailable", next);
        mcc_error_text(current, "The map its script changes to is not installed.", text, 512);
        display_error_text_deferred(text, 0);
        if (server) mcc_ui_network_campaign_won(NULL);
        else main_goto_main_menu();
    } else if (server) {
        mcc_ui_network_campaign_won(next);
    } else {
        error(_error_silent, "mcc: map_name changes from %s to %s", current, next);
        main_set_map_name(next);
        main_disallow_persistent_storage();
    }
    return TRUE;
}

boolean mcc_main_campaign_won(void)
{
    struct network_game *game;
    char const *current;
    char next[64];
    wchar_t text[512];
    int state;
    boolean server;
    if (mcc_cache_tags_loaded() && game_connection() != _game_connection_network_client &&
        mcc_main_requested_map(mcc_main_current_map(), game_connection() == _game_connection_network_server))
        return TRUE;
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
