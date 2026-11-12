/* MCC's in-game team picker uses the existing authenticated settings message.
 * Only its team byte is accepted; the ordinary pregame handler is untouched. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "memory/data.h"
#include "networking/network_game_manager.h"
#include "networking/network_messages.h"
#include "networking/network_server_manager_internal.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/units.h"
#include "mcc_cache.h"
#include "mcc_ui_teams.h"
#include <string.h>

struct mcc_team_respawn {
    boolean active;
    boolean no_statistics;
    long player_index;
    long unit_index;
};
static struct mcc_team_respawn mcc_team_respawns[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

void mcc_ui_teams_reset(void)
{
    memset(mcc_team_respawns, 0, sizeof(mcc_team_respawns));
}

static void mcc_ui_team_respawn_track(long player_index, long unit_index)
{
    unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
    if (slot >= NUMBEROF(mcc_team_respawns) || unit_index == NONE) return;
    mcc_team_respawns[slot].active = TRUE;
    mcc_team_respawns[slot].no_statistics = FALSE;
    mcc_team_respawns[slot].player_index = player_index;
    mcc_team_respawns[slot].unit_index = unit_index;
}

void mcc_ui_team_respawn_damage(long player_index, long unit_index, boolean active)
{
    unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
    struct mcc_team_respawn *pending;
    if (!mcc_cache_tags_loaded() || slot >= NUMBEROF(mcc_team_respawns)) return;
    pending = &mcc_team_respawns[slot];
    if (pending->active && pending->player_index == player_index && pending->unit_index == unit_index)
        pending->no_statistics = active;
}

void mcc_ui_team_respawn_death(long *killer, long *object, long dead, boolean *friendly)
{
    unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(dead);
    struct mcc_team_respawn pending;
    struct player_datum *player;
    struct unit_datum *unit;
    if (!mcc_cache_tags_loaded() || slot >= NUMBEROF(mcc_team_respawns)) return;
    pending = mcc_team_respawns[slot];
    /* The next death consumes the record even if an unrelated death won the
     * race, or a recycled player/unit slot has a different salt. */
    mcc_team_respawns[slot].active = FALSE;
    if (!pending.active || !pending.no_statistics || pending.player_index != dead ||
        game_connection() != _game_connection_network_server ||
        !global_scenario || global_scenario->type != 1 ||
        !killer || !object || !friendly || *killer != dead ||
        *object != pending.unit_index || !*friendly) return;
    player = player_try_and_get(dead);
    unit = unit_try_and_get(pending.unit_index);
    if (!player || player->unit_index != pending.unit_index || !unit ||
        !TEST_FLAG(unit->object.damage_flags, _object_die_act_of_god_no_statistics_bit)) return;
    /* The native no-statistics death still invokes Slayer's score callback
     * as a suicide. Neutral attribution keeps the normal death, objectives
     * and respawn processing, and is replicated by the following engine hook. */
    *killer = NONE;
    *object = NONE;
    *friendly = FALSE;
}

boolean mcc_ui_team_balance_allows(short machine, short controller, short team)
{
    struct data_iterator iterator;
    struct player_datum *player;
    long counts[2] = {0, 0}, old_team = NONE;
    if (!VALID_INDEX(team, 2)) return FALSE;
    if (!game_variant_options_get()->auto_team_balance) return TRUE;
    data_iterator_new(&iterator, player_data);
    while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL) {
        if (player->quit_out_of_game || !VALID_INDEX(player->team_index, 2)) continue;
        ++counts[player->team_index];
        if (player->network_player_data.machine_index == machine &&
            player->network_player_data.controller_index == controller) old_team = player->team_index;
    }
    return old_team == team || (VALID_INDEX(old_team, 2) && counts[team] < counts[1 - team]);
}

boolean mcc_ui_team_request(struct network_game_server *server,
    struct network_game_server_client_machine *machine, word *message, short size)
{
    struct network_game *game;
    struct network_player request, unchanged;
    struct network_player *current;
    struct player_datum *player;
    struct data_iterator iterator;
    short packet_type = _message_client_player_settings_request;
    short packet_version = HALO_PORT_NETWORK_GAME_MESSAGE_VERSION;
    long machine_index, slot;
    /* Server state 1 is a running round. No pregame/postgame interception. */
    if (!mcc_cache_tags_loaded() || !server ||
        network_game_server_get_state(server, NULL) != 1) return FALSE;
    game = network_game_server_get_game(server);
    if (!game || !mcc_level_name(game->map.name)) return FALSE;
    if (!global_scenario || global_scenario->type != 1 ||
        !game->variant.universal_variant.teams || !machine || !message || size <= (short)sizeof(word))
        return TRUE;
    /* Wire version byte + all 32 player bytes + packet type + word header. */
    if (size != sizeof(struct network_player) + 4 ||
        ((unsigned char const *)message)[size - 1] != _message_client_player_settings_request) return TRUE;
    size -= sizeof(word);
    if (!decode_network_game_message(&request, message + 1, &size,
            &packet_type, &packet_version, 3) ||
        packet_type != _message_client_player_settings_request) return TRUE;
    slot = request.player_list_index;
    if (!network_game_server_get_client_machine(server, machine, &machine_index) ||
        !VALID_INDEX(machine_index, NUMBEROF(game->machines))) return TRUE;
    if (!VALID_INDEX(slot, NUMBEROF(game->players)) || request.machine_index != machine_index ||
        !VALID_INDEX(request.controller_index, 4) || !VALID_INDEX(request.team_index, 2)) return TRUE;
    current = &game->players[slot];
    if (!network_player_is_valid(current)) return TRUE;
    unchanged = request;
    unchanged.team_index = current->team_index;
    /* Includes name, icon, color, machine, controller and exact player slot. */
    if (memcmp(&unchanged, current, sizeof(unchanged))) return TRUE;
    data_iterator_new(&iterator, player_data);
    while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL) {
        struct network_player *identity = &player->network_player_data;
        if (player->quit_out_of_game || identity->player_list_index != slot ||
            identity->machine_index != machine_index ||
            identity->controller_index != request.controller_index) continue;
        if (player->team_index == request.team_index) return TRUE;
        /* Respect the host's existing automatic balance rule. */
        if (!mcc_ui_team_balance_allows((short)machine_index, request.controller_index, request.team_index)) return TRUE;
        current->team_index = request.team_index;
        identity->team_index = request.team_index;
        player->team_index = request.team_index;
        /* A normal new unit carries its team's ownership/color to every
         * client. Drop objectives and respawn, without a suicide score. */
        if (player->unit_index != NONE) {
            mcc_ui_team_respawn_track(iterator.datum_index, player->unit_index);
            unit_kill_no_statistics(player->unit_index);
        }
        network_event("mcc: player %ld requested team %d", slot, (int)request.team_index);
        return TRUE;
    }
    return TRUE;
}
