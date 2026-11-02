/* MCC startup selections wait for local admission; pause-menu restarts use
 * the reliable round transition and an MCC-owned snapshot. Included by the
 * server implementation to keep its private representation private. */
#include "mcc_cache.h"
#include "mcc_validation.h"
#include "mcc_identity.h"

static struct network_game_server *mcc_restart_server;
static struct network_game mcc_restart_game;
static struct network_game_server *mcc_restart_ui_server;
static long mcc_restart_ui_round;
static struct network_game_server *mcc_pending_map_server;
static char mcc_pending_map[64];
static boolean mcc_pending_local_client;
static int mcc_pending_error_shown;
static struct network_game_server *mcc_validation_server;

boolean mcc_ui_network_defer_map(struct network_game_server *server, char const *name)
{
    /* Fast host setup can choose MCC while its loopback client is joining.
     * Leave the temporary lobby usable until that admission completes;
     * synchronous package checks must not occupy its five-second window. */
    if (server == mcc_pending_map_server) mcc_pending_map_server = NULL;
    mcc_pending_error_shown=0;
    mcc_validation_cancel(); /* Explicit reselection may retry a transient read failure. */
    if (!mcc_level_name(name)) {mcc_validation_server=NULL;return FALSE;}
    if (strlen(name)>=sizeof(mcc_pending_map) || server!=global_network_game_server_get()) return FALSE;
    if (mcc_validation_request(name,NULL,NULL)==1 &&
        (!global_network_game_client_get() || network_game_client_get_local_machine_index()!=NONE)) return FALSE;
    strcpy(mcc_pending_map, name);
    mcc_pending_map_server = server;
    mcc_pending_local_client = global_network_game_client_get()!=NULL;
    network_event("mcc: map selection waits for package validation and local admission");
    return TRUE;
}

boolean mcc_ui_network_map_pending(struct network_game_server *server)
{
    return server && (server==mcc_pending_map_server ||
        (mcc_level_name(server->game.map.name) &&
         (!server->game.map.version || !mcc_validation_lookup(server->game.map.name,NULL,NULL))));
}

void mcc_ui_network_update_map(struct network_game_server *server)
{
    char name[sizeof(mcc_pending_map)];
    char const *selected;
    uint32_t checksum;
    int status;
    wchar_t message[320];
    unsigned i;
    if (server!=global_network_game_server_get() ||
        server->state!=_network_game_server_state_pregame || server->sent_start_game_message ||
        (server==mcc_pending_map_server && mcc_pending_local_client && !global_network_game_client_get())) {
        if (server==mcc_pending_map_server || server==mcc_validation_server) {
            mcc_pending_map_server=NULL;
            mcc_validation_server=NULL;
            mcc_validation_cancel();
        }
        return;
    }
    selected=server==mcc_pending_map_server ? mcc_pending_map : server->game.map.name;
    if (!mcc_level_name(selected)) return;
    status=mcc_validation_request(selected,&checksum,NULL);
    if (!status) {mcc_validation_server=server;mcc_validation_update();return;}
    if (status<0) {
        if (!mcc_pending_error_shown) {
            char const *reason=mcc_validation_error();
            for (i=0;reason[i] && i<319;i++) message[i]=(unsigned char)reason[i];
            message[i]=0;
            display_error_text_when_main_menu_loaded(message);
            mcc_pending_error_shown=1;
        }
        return;
    }
    if (!mcc_identity_prepare_host(selected)) return;
    if (global_network_game_client_get() && network_game_client_get_local_machine_index()==NONE) return;
    if (server==mcc_pending_map_server || server==mcc_validation_server ||
        (uint32_t)server->game.map.version!=checksum) {
        strcpy(name,selected);
        mcc_pending_map_server=NULL;
        mcc_validation_server=NULL;
        network_game_server_change_map_name(server,name);
    }
}

void mcc_ui_network_server_dispose(struct network_game_server *server)
{
    if (server==global_network_game_server_get()) mcc_validation_cancel();
    if (server==mcc_validation_server) mcc_validation_server=NULL;
    mcc_pending_error_shown=0;
    if (server == mcc_pending_map_server) mcc_pending_map_server = NULL;
    if (server == mcc_restart_server) mcc_restart_server = NULL;
    if (server == mcc_restart_ui_server) mcc_restart_ui_server = NULL;
}

boolean mcc_ui_network_restart_pregame(void)
{
    struct network_game_server *server = global_network_game_server_get();
    if (!mcc_restart_ui_server) return FALSE;
    if (!server || server != mcc_restart_ui_server) {
        mcc_restart_ui_server = NULL;
        return FALSE;
    }
    mcc_restart_ui_server = NULL;
    if (server->state != _network_game_server_state_pregame ||
        server->game.number_of_games_played != mcc_restart_ui_round ||
        server->game.map.version != mcc_restart_game.map.version ||
        strcmp(server->game.map.name, mcc_restart_game.map.name)) return FALSE;
    /* The host's own switch-to-pregame message arrives after reset returns.
     * Ordinary postgame UI opens map selection and pauses the countdown.
     * Consume this exact restart at that boundary, after the UI map loads;
     * MCC tags may already be unloaded, so ownership is the pending token. */
    network_game_server_pause_countdown(server, FALSE);
    network_game_server_begin_game_start_countdown(server, 3000);
    return TRUE;
}

boolean mcc_ui_network_restart_settings(struct network_game_server *server)
{
    long i;
    if (!server || server != mcc_restart_server) return FALSE;
    mcc_restart_server = NULL;
    server->game.map = mcc_restart_game.map;
    server->game.variant = mcc_restart_game.variant;
    server->game.variant_options = mcc_restart_game.variant_options;
    server->game.cooperative_flags = mcc_restart_game.cooperative_flags;
    server->game.minimum_players = mcc_restart_game.minimum_players;
    server->game.maximum_players = mcc_restart_game.maximum_players;
    server->game.maximum_teams = mcc_restart_game.maximum_teams;
    server->game.difficulty = mcc_restart_game.difficulty;
    server->game.number_of_games_played = (long)((unsigned long)mcc_restart_game.number_of_games_played + 1);
    csmemcpy(server->game.name, mcc_restart_game.name, sizeof(server->game.name));
    /* A normal next round swaps red and blue. Restart preserves the current
     * choices without restoring players who may have left the session. */
    for (i = 0; i < MAXIMUM_NETWORK_PLAYER_COUNT; ++i) {
        struct network_player *player = &server->game.players[i];
        struct network_player const *saved = &mcc_restart_game.players[i];
        if (network_player_is_valid(player) &&
            player->machine_index == saved->machine_index &&
            player->controller_index == saved->controller_index &&
            player->player_list_index == saved->player_list_index)
            player->team_index = saved->team_index;
    }
    main_set_multiplayer_map_name(server->game.map.name);
    network_game_server_open_game(server);
    return TRUE;
}

boolean mcc_ui_restart_network_game(void)
{
    struct network_game_server *server = global_network_game_server_get();
    boolean success;
    if (!server) return FALSE;
    if (mcc_restart_server) {
        /* A failed reliable transition can leave the session in postgame.
         * Retry that transition with its original settings, not the playlist. */
        if (server != mcc_restart_server || server->state != _network_game_server_state_postgame)
            return FALSE;
    } else {
        if (!mcc_cache_tags_loaded() || !mcc_level_name(server->game.map.name) ||
            server->state != _network_game_server_state_ingame) return FALSE;
        mcc_restart_game = server->game;
        mcc_restart_server = server;
        network_game_server_switch_to_postgame(server);
    }
    success = network_game_server_reset_to_pregame(server);
    if (success) {
        /* Start the ordinary precache/countdown path once the host's local
         * client receives the reliable reset and opens the pregame UI. */
        mcc_restart_ui_server = server;
        mcc_restart_ui_round = server->game.number_of_games_played;
        network_event("mcc: restarting the current network map");
    }
    return success;
}
