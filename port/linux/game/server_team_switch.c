/*
SERVER_TEAM_SWITCH.C

A dedicated server's players change their own team: in a team game a
player says "!switch" (or "!team") in the chat, which the host takes and
sends on to no one (network_chat_handle_up). The host alone decides, and
moves them only

- with server.team_switch on;
- in a game being played (not the postgame) that has teams: not co-op, nor
  Infection, whose teams are its roles (infection.c);
- once in server.team_switch_cooldown_seconds, each player;
- with the gametype's auto team balance on, to a team with fewer players
  than theirs (them counted): the new team is then one player bigger than
  the old at most. 5 against 4 may become 4 against 5; 4 against 4 stays.

The player is moved as the auto team balance moves the dead
(game_engine_player_killed): the host sets their team (and its game's
record, which late joins count and the lobby shows), and their next unit
carries it to every machine (network_player_attach_unit). A living player's
unit dies first, as MCC's team picker's does (mcc_ui_teams.c): a death no one
is credited with, so no suicide is scored; what it carries (a flag, the
ball) drops, and they respawn as after any death. A dead player's respawn
goes on as it was. What a player scored stays with the team they scored it
for, and their own score with them. Every machine is told in the chat, and
the server's log has it; a refusal is told to the player alone.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "memory/data.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager.h"
#include "networking/network_server_manager_internal.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "infection.h"
#include "mcc_ui_teams.h"
#include "server_team_switch.h"
#include "../src/port_config.h" /* port: port/linux/src/port_config.c */
#include "../src/server_log.h" /* port: port/linux/src/server_log.c */

#include <stdio.h>

/* the platform layer's */
unsigned long system_milliseconds(void);

/* ---------- constants */

enum
{
	/* the longest server.team_switch_cooldown_seconds is held to */
	TEAM_SWITCH_MAXIMUM_COOLDOWN_SECONDS = 3600,
};

/* ---------- structures */

/* a player's last switch: the player (a later one in their slot is not
held) and when */
struct team_switch_record
{
	long player_index;
	unsigned long milliseconds;
};

/* ---------- globals */

/* each player's, by their absolute index */
static struct team_switch_record team_switch_records[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

static char const *const team_switch_team_names[2] = { "Red", "Blue" };

/* ---------- code */

/* whether the auto team balance lets a player leave their team for the
other: the other has fewer players than theirs (them counted), so it is one
bigger than theirs after at most */
static boolean team_switch_balanced(
	short const counts[2],
	short team)
{
	return counts[1 - team] < counts[team];
}

/* the seconds before a player may switch again (0: now) */
static long team_switch_seconds_left(
	struct team_switch_record const *record,
	long player_index,
	unsigned long now,
	long cooldown_seconds)
{
	unsigned long elapsed = now - record->milliseconds;
	unsigned long cooldown;

	if (cooldown_seconds <= 0 || record->player_index != player_index)
		return 0;
	cooldown = (unsigned long)MIN(cooldown_seconds, (long)TEAM_SWITCH_MAXIMUM_COOLDOWN_SECONDS) * 1000;
	if (elapsed >= cooldown)
		return 0;
	return (long)((cooldown - elapsed + 999) / 1000);
}

boolean server_team_switch(
	long player_index,
	char const *name,
	char *notice,
	long notice_size)
{
	struct player_datum *player = player_try_and_get(player_index);
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	unsigned long now = system_milliseconds();
	struct data_iterator iterator;
	struct player_datum *other;
	struct network_game_server *server;
	struct network_game *game;
	short counts[2] = { 0, 0 };
	short team;
	long seconds_left;
	long list_index;

	if (!player || player->quit_out_of_game || slot >= NUMBEROF(team_switch_records))
	{
		snprintf(notice, (size_t)notice_size, "You can't switch teams now");
		return FALSE;
	}
	if (!config_boolean("server.team_switch"))
	{
		snprintf(notice, (size_t)notice_size, "Team switching is off on this server");
		return FALSE;
	}
	if (!global_scenario || global_scenario->type != _scenario_type_multiplayer)
	{
		snprintf(notice, (size_t)notice_size, "Co-op has no teams to switch");
		return FALSE;
	}
	if (!game_engine_running() || !game_engine_can_score())
	{
		snprintf(notice, (size_t)notice_size, "You can switch teams only while a game is played");
		return FALSE;
	}
	if (!game_engine_has_teams())
	{
		snprintf(notice, (size_t)notice_size, "!switch works only in team games");
		return FALSE;
	}
	if (infection_enabled())
	{
		snprintf(notice, (size_t)notice_size, "Infection decides the teams");
		return FALSE;
	}
	team = player->team_index;
	if (!VALID_INDEX(team, 2))
	{
		snprintf(notice, (size_t)notice_size, "You can't switch teams now");
		return FALSE;
	}
	seconds_left = team_switch_seconds_left(&team_switch_records[slot], player_index, now,
		config_integer("server.team_switch_cooldown_seconds"));
	if (seconds_left > 0)
	{
		snprintf(notice, (size_t)notice_size, "You can switch teams again in %ld second%s", seconds_left,
			seconds_left == 1 ? "" : "s");
		return FALSE;
	}
	data_iterator_new(&iterator, player_data);
	while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (!other->quit_out_of_game && VALID_INDEX(other->team_index, 2))
			counts[other->team_index]++;
	}
	if (game_variant_options_get()->auto_team_balance && !team_switch_balanced(counts, team))
	{
		snprintf(notice, (size_t)notice_size, "%s has %d players, %s %d: that would unbalance the teams",
			team_switch_team_names[1 - team], (int)counts[1 - team], team_switch_team_names[team], (int)counts[team]);
		return FALSE;
	}

	team = 1 - team;
	player->team_index = team;
	player->network_player_data.team_index = (char)team;
	server = global_network_game_server_get();
	game = server ? network_game_server_get_game(server) : NULL;
	list_index = player->network_player_data.player_list_index;
	if (game && VALID_INDEX(list_index, NUMBEROF(game->players)))
		game->players[list_index].team_index = (char)team;
	/* (a living player's unit dies, no one's kill; their next is the new
	team's) */
	mcc_ui_team_change_kill(player_index, player->unit_index);
	team_switch_records[slot].player_index = player_index;
	team_switch_records[slot].milliseconds = now;
	server_log(_server_log_info, "player", "%s switched to the %s team (!switch)", name, team_switch_team_names[team]);
	snprintf(notice, (size_t)notice_size, "%s switched to the %s team", name, team_switch_team_names[team]);
	return TRUE;
}
