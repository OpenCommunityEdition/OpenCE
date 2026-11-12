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
carries it to every machine (network_player_attach_unit). A dead player is
moved at once, and their respawn goes on as it was. A living player's unit
is killed with no statistics (unit_kill_no_statistics), a death no one is
credited with, so no suicide is scored, and they respawn as after any
death. Their move waits for that death, which the next objects update
deals, and is made where the auto team balance makes its own, once the
unit has dropped what it carried (a flag, the ball): moved before, a flag
carrier would hold their new team's own flag until then, and the game
engine's update, which runs ahead of the objects', would see it (a capture
by the new team, near its base). While a move waits, the balance counts the
player in their new team. A kill by another that comes first stays theirs,
and the player is moved all the same.

What a player scored stays with the team they scored it for, and their own
score with them. Every machine is told in the chat, and the server's log
has it; a refusal is told to the player alone. Each new map forgets every
cooldown and waiting move (players_initialize_for_new_map): its players
reuse the last map's player indices. Only the host makes moves; on a client
the hooks find none.
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
#include "units/units.h"
#include "infection.h"
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

/* a living player's move, waiting for their unit's death: the player, the
unit, the team they go to; whether the death being dealt is the
no-statistics one the switch asked for (server_team_switch_damage), and
whether the unit has died (server_team_switch_player_killed) */
struct team_switch_move
{
	boolean active;
	boolean no_statistics;
	boolean died;
	short team;
	long player_index;
	long unit_index;
};

/* ---------- globals */

/* each player's, by their absolute index */
static struct team_switch_record team_switch_records[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static struct team_switch_move team_switch_moves[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

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

/* a player's waiting move, if they have one (NULL: none) */
static struct team_switch_move *team_switch_move_get(
	long player_index)
{
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

	if (slot >= NUMBEROF(team_switch_moves) || !team_switch_moves[slot].active ||
		team_switch_moves[slot].player_index != player_index)
	{
		return NULL;
	}
	return &team_switch_moves[slot];
}

/* the team the balance counts a player in: the one they are moving to,
while their move waits */
static short team_switch_counted_team(
	struct player_datum const *player,
	long player_index)
{
	struct team_switch_move const *move = team_switch_move_get(player_index);

	return move ? move->team : player->team_index;
}

/* moves a player: their team, their network data's (which their next unit
carries to every machine) and the host's game record's */
static void team_switch_move_player(
	struct player_datum *player,
	short team)
{
	struct network_game_server *server = global_network_game_server_get();
	struct network_game *game = server ? network_game_server_get_game(server) : NULL;
	long list_index = player->network_player_data.player_list_index;

	player->team_index = team;
	player->network_player_data.team_index = (char)team;
	if (game && VALID_INDEX(list_index, NUMBEROF(game->players)))
		game->players[list_index].team_index = (char)team;
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
	short counts[2] = { 0, 0 };
	short team;
	long seconds_left;

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
	/* (with no cooldown, a second !switch before the first's death) */
	if (team_switch_move_get(player_index))
	{
		snprintf(notice, (size_t)notice_size, "You are switching teams already");
		return FALSE;
	}
	data_iterator_new(&iterator, player_data);
	while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short other_team = team_switch_counted_team(other, iterator.datum_index);

		if (!other->quit_out_of_game && VALID_INDEX(other_team, 2))
			counts[other_team]++;
	}
	if (game_variant_options_get()->auto_team_balance && !team_switch_balanced(counts, team))
	{
		snprintf(notice, (size_t)notice_size, "%s has %d players, %s %d: that would unbalance the teams",
			team_switch_team_names[1 - team], (int)counts[1 - team], team_switch_team_names[team], (int)counts[team]);
		return FALSE;
	}

	team = 1 - team;
	if (player->unit_index == NONE)
	{
		/* (the dead carry nothing: they are moved now) */
		team_switch_move_player(player, team);
	}
	else
	{
		/* (the living are moved after their unit's death, no one's kill) */
		team_switch_moves[slot].active = TRUE;
		team_switch_moves[slot].no_statistics = FALSE;
		team_switch_moves[slot].died = FALSE;
		team_switch_moves[slot].team = team;
		team_switch_moves[slot].player_index = player_index;
		team_switch_moves[slot].unit_index = player->unit_index;
		unit_kill_no_statistics(player->unit_index);
	}
	team_switch_records[slot].player_index = player_index;
	team_switch_records[slot].milliseconds = now;
	server_log(_server_log_info, "player", "%s switched to the %s team (!switch)", name, team_switch_team_names[team]);
	snprintf(notice, (size_t)notice_size, "%s switched to the %s team", name, team_switch_team_names[team]);
	return TRUE;
}

void server_team_switch_initialize_for_new_map(
	void)
{
	csmemset(team_switch_records, 0, sizeof(team_switch_records));
	csmemset(team_switch_moves, 0, sizeof(team_switch_moves));
}

void server_team_switch_damage(
	long player_index,
	long unit_index,
	boolean no_statistics)
{
	struct team_switch_move *move = team_switch_move_get(player_index);

	if (move && move->unit_index == unit_index)
		move->no_statistics = no_statistics;
}

void server_team_switch_player_killed(
	long *killing_player_index,
	long *killing_object_index,
	long dead_player_index,
	boolean *friendly_fire)
{
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(dead_player_index);
	struct team_switch_move *move;
	struct player_datum *player;

	if (slot >= NUMBEROF(team_switch_moves) || !team_switch_moves[slot].active)
		return;
	move = &team_switch_moves[slot];
	player = player_try_and_get(dead_player_index);
	/* (a move left by a player who has gone, or for a unit of theirs that
	went without dying, is dropped) */
	if (move->player_index != dead_player_index || !player || player->unit_index != move->unit_index)
	{
		move->active = FALSE;
		return;
	}
	move->died = TRUE;
	/* (the death the switch asked for is no one's; another's kill, come
	first, stays theirs) */
	if (move->no_statistics && *killing_player_index == dead_player_index &&
		*killing_object_index == move->unit_index)
	{
		*killing_player_index = NONE;
		*killing_object_index = NONE;
		*friendly_fire = FALSE;
	}
}

void server_team_switch_player_dead(
	long dead_player_index)
{
	struct team_switch_move *move = team_switch_move_get(dead_player_index);
	struct player_datum *player;

	if (!move || !move->died)
		return;
	move->active = FALSE;
	player = player_try_and_get(dead_player_index);
	if (player && !player->quit_out_of_game)
		team_switch_move_player(player, move->team);
}
