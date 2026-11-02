/*
NETWORK_AFK.C

A dedicated server's kick of a player away from the controls
(docs/dedicated_server.md, "Players away"). The host sees each tick's input
of every player (update_server_update_actions): a button pressed or let go,
a stick or the trigger held, or the view turned shows a player there. A
machine none of whose players has shown so for server.afk_kick_minutes (5;
0: never) is told a minute before (half the time, for a minute's) and then
kicked, free to join again at once; with server.afk_kick_when_full only
while the game is full, after server.afk_kick_full_minutes (3), to make room
for the players who would join.

Only time in a game counts: not the lobby, the postgame, a cinematic, a
machine whose players are all dead (waiting to respawn, or watching a
co-op game), nor the host's own players. A machine is counted afresh as it
joins (network_afk_machine_joined) and as each game starts.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "game/players.h"
#include "game/player_queues_new.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "network_distributed.h"
#include "network_afk.h"
#include "server_director.h"
#include "../src/server_events.h" /* port: port/linux/src/server_events.c */
#include "../src/server_json.h" /* port: port/linux/src/server_json.c */
#include "../src/server_log.h" /* port: port/linux/src/server_log.c */

#include <math.h>
#include <stdio.h>

/* port_config.c's, network_server_manager.c's, the platform's */
int config_boolean(const char *name);
long config_integer(const char *name);
void network_game_server_kick_machine(long machine_index, boolean kept_out);
int halo_server_mode(void);

/* ---------- constants */

enum
{
	MAXIMUM_AFK_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* the warning before the kick (a minute's limit: half of it) */
	AFK_WARNING_SECONDS = 60,
};

/* the view turned by more than this (radians) is a player there; less is
the noise of a stick at rest */
#define AFK_FACING_EPSILON 0.002f

/* ---------- globals */

/* each player's input as last seen (by absolute index), to see it change */
static struct
{
	long player_index;
	unsigned long control_flags;
	real yaw;
	real pitch;
} afk_players[MAXIMUM_TRACKED_PLAYERS];

/* each machine's ticks away, and whether it was warned */
static struct
{
	long idle_ticks;
	boolean warned;
} afk_machines[MAXIMUM_AFK_MACHINES];

/* ---------- private code */

static boolean afk_host(
	void)
{
	return game_connection() == _game_connection_network_server && halo_server_mode();
}

/* the ticks away that kick, now (0: none): server.afk_kick_minutes; or, with
server.afk_kick_when_full, server.afk_kick_full_minutes while the game is
full, and none while it is not */
static long afk_limit_ticks(
	void)
{
	struct network_game *game = network_game_get_game();
	long minutes;

	if (config_boolean("server.afk_kick_when_full"))
	{
		if (!game || game->maximum_players <= 0 || game->player_count < game->maximum_players)
			return 0;
		minutes = PIN(config_integer("server.afk_kick_full_minutes"), 0, 60);
	}
	else
	{
		minutes = PIN(config_integer("server.afk_kick_minutes"), 0, 60);
	}
	return minutes * 60 * TICKS_PER_SECOND;
}

/* the ticks away that warn, of those that kick */
static long afk_warning_ticks(
	long limit_ticks)
{
	long before = AFK_WARNING_SECONDS * TICKS_PER_SECOND;

	return limit_ticks > 2 * before ? limit_ticks - before : limit_ticks / 2;
}

static real afk_angle_change(
	real a,
	real b)
{
	real difference = (real)fabs(a - b);

	/* (the yaw wraps about a turn) */
	if (difference > (real)M_PI)
		difference = 2.0f * (real)M_PI - difference;
	return difference;
}

/* the kick: told to everyone, logged and an event (server_events.c) */
static void afk_kick(
	long machine_index,
	long idle_ticks)
{
	char names[64];
	char notice[160];
	struct server_json json;

	distributed_machine_player_names(machine_index, names, sizeof(names));
	snprintf(notice, sizeof(notice), "%s kicked for being away (%ld minute%s)", names,
		idle_ticks / (60 * TICKS_PER_SECOND), idle_ticks / (60 * TICKS_PER_SECOND) == 1 ? "" : "s");
	distributed_send_notice(notice);
	server_log(_server_log_info, "afk", "%s (machine %ld) kicked: away %ld seconds", names, machine_index,
		idle_ticks / TICKS_PER_SECOND);
	server_json_begin(&json);
	server_json_object_begin(&json, NULL);
	server_json_string(&json, "event", "afk_kick");
	server_json_string(&json, "players", names);
	server_json_integer(&json, "seconds", idle_ticks / TICKS_PER_SECOND);
	server_json_object_end(&json);
	server_event_add("game", server_json_text(&json));
	server_json_free(&json);
	server_director_player_removed(machine_index, "afk");
	network_game_server_kick_machine(machine_index, FALSE);
	afk_machines[machine_index].idle_ticks = 0;
	afk_machines[machine_index].warned = FALSE;
}

/* ---------- public code */

void network_afk_machine_joined(
	long machine_index)
{
	if (machine_index >= 0 && machine_index < MAXIMUM_AFK_MACHINES)
	{
		afk_machines[machine_index].idle_ticks = 0;
		afk_machines[machine_index].warned = FALSE;
	}
}

void network_afk_new_game(
	void)
{
	csmemset(afk_machines, 0, sizeof(afk_machines));
	csmemset(afk_players, 0, sizeof(afk_players));
}

void network_afk_host_tick(
	void)
{
	/* (each machine: one of its players there, one alive to count) */
	boolean present[MAXIMUM_AFK_MACHINES];
	boolean alive[MAXIMUM_AFK_MACHINES];
	boolean seen[MAXIMUM_AFK_MACHINES];
	long update_number;
	short action_count = 0;
	struct player_action const *actions;
	struct data_iterator iterator;
	struct player_datum *player;
	long limit_ticks;
	long machine_index;

	if (!afk_host() || !player_data)
		return;
	limit_ticks = afk_limit_ticks();
	update_number = update_server_ticked_update_number();
	actions = update_number != NONE ? update_server_update_actions(update_number, &action_count) : NULL;
	csmemset(present, 0, sizeof(present));
	csmemset(alive, 0, sizeof(alive));
	csmemset(seen, 0, sizeof(seen));
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short absolute_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		struct player_action const *action;
		boolean changed;

		machine_index = distributed_player_machine(absolute_index);
		if (player->quit_out_of_game || machine_index < 0 || machine_index >= MAXIMUM_AFK_MACHINES ||
			absolute_index >= MAXIMUM_TRACKED_PLAYERS)
		{
			continue;
		}
		seen[machine_index] = TRUE;
		if (player->unit_index != NONE)
			alive[machine_index] = TRUE;
		if (!actions || absolute_index >= action_count)
			continue;
		action = &actions[absolute_index];
		if (afk_players[absolute_index].player_index != iterator.datum_index)
		{
			afk_players[absolute_index].player_index = iterator.datum_index;
			afk_players[absolute_index].control_flags = action->control_flags;
			afk_players[absolute_index].yaw = action->desired_facing.yaw;
			afk_players[absolute_index].pitch = action->desired_facing.pitch;
			present[machine_index] = TRUE;
			continue;
		}
		changed = action->control_flags != afk_players[absolute_index].control_flags ||
			action->throttle.i != 0.0f || action->throttle.j != 0.0f || action->primary_trigger != 0.0f ||
			afk_angle_change(action->desired_facing.yaw, afk_players[absolute_index].yaw) > AFK_FACING_EPSILON ||
			afk_angle_change(action->desired_facing.pitch, afk_players[absolute_index].pitch) > AFK_FACING_EPSILON;
		afk_players[absolute_index].control_flags = action->control_flags;
		afk_players[absolute_index].yaw = action->desired_facing.yaw;
		afk_players[absolute_index].pitch = action->desired_facing.pitch;
		if (changed)
			present[machine_index] = TRUE;
	}
	for (machine_index = 0; machine_index < MAXIMUM_AFK_MACHINES; machine_index++)
	{
		if (!seen[machine_index] || present[machine_index])
		{
			afk_machines[machine_index].idle_ticks = 0;
			afk_machines[machine_index].warned = FALSE;
			continue;
		}
		/* (nothing counts but play: not a cinematic, nor a game over, nor
		every player of the machine dead) */
		if (!server_director_playing() || cinematic_in_progress() || !alive[machine_index])
			continue;
		afk_machines[machine_index].idle_ticks++;
		if (!limit_ticks)
			continue;
		if (afk_machines[machine_index].idle_ticks >= limit_ticks)
		{
			afk_kick(machine_index, afk_machines[machine_index].idle_ticks);
		}
		else if (!afk_machines[machine_index].warned &&
			afk_machines[machine_index].idle_ticks >= afk_warning_ticks(limit_ticks))
		{
			char notice[128];
			long seconds = (limit_ticks - afk_machines[machine_index].idle_ticks + TICKS_PER_SECOND - 1) /
				TICKS_PER_SECOND;

			afk_machines[machine_index].warned = TRUE;
			snprintf(notice, sizeof(notice), "You will be kicked for being away in %ld seconds: move to stay", seconds);
			distributed_send_notice_to_machine(machine_index, notice);
		}
	}
}
