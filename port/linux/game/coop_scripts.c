/*
COOP_SCRIPTS.C

Campaign scripts made to work for every player in network co-op.

The campaign's scripts were written for one or two players. Only the host
runs them (game.c), against every player's unit, so most of what they do
already happens for everyone: a trigger volume tested against (players)
fires for whoever walks into it, and what it starts is sent to the clients
(network_coop.c). Three habits of the scripts don't carry over:

- Waiting for every player at once (volume_test_objects_all on the
  players): the Maw's run to the Pillar of Autumn's bridge, a30's Pelican
  pickups. Split-screen players stand together; network players don't, so
  the wait would never end. In co-op one player arriving is enough.
- Waiting until it's safe (game_safe_to_save) before a cutscene: the
  game's test fails while any enemy sees any player. In co-op one player
  out of danger is enough.
- Naming players one at a time: the scripts reach the players through
  player0 and player1 (the first two in the players list), to put them in
  a Pelican, take them out of it, stage them for a cutscene or give them a
  starting profile. Everyone past the second would be left out, so they
  follow player0: wherever the scripts put player0, they go too.
*/

/* ---------- headers */

#include "cseries.h"
#include "ai/ai.h"
#include "game/game.h"
#include "game/players.h"
#include "hs/object_lists.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "units/bipeds.h"
#include "units/units.h"

#include "coop_scripts.h"
#include "network_coop.h"

#include <string.h>

/* ---------- constants */

/* players the scripts name: player0 and player1 */
#define NAMED_PLAYERS 2

/* ---------- private code */

static boolean coop_scripts_host(
	void)
{
	return game_connection() == _game_connection_network_server && network_coop_active();
}

/* the seat label to look for when boarding a follower: player0's own up to
and including "rider" (Pelicans have "p-riderlf", "p-riderrf", "p-riderlb"
and so on), else up to the '-' ("b-driver" gives "b-") */
static void follower_seat_name(
	char const *seat_name,
	char *follower_seat,
	size_t size)
{
	char *rider;
	char *dash;

	/* (lower case: vehicle_scripting_find_available_seats compares it with
	lowered seat labels) */
	snprintf(follower_seat, size, "%s", seat_name);
	strlwr(follower_seat);
	rider = strstr(follower_seat, "rider");
	dash = strchr(follower_seat, '-');
	if (rider)
		rider[strlen("rider")] = 0;
	else if (dash)
		dash[1] = 0;
}

/* ---------- public code */

boolean coop_scripts_any_player_will_do(
	long object_list_index)
{
	long reference_index;
	long object_index;
	boolean any = FALSE;

	if (!coop_scripts_host())
		return FALSE;
	for (object_index = object_list_get_first(object_list_index, &reference_index);
		object_index != NONE;
		object_index = object_list_get_next(object_list_index, &reference_index))
	{
		if (player_index_from_unit_index(object_index) == NONE)
			return FALSE;
		any = TRUE;
	}
	return any;
}

/* the units of the players following player0, if `unit_index` is
player0's; how many */
static short players_following(
	long unit_index,
	long *unit_indices,
	short maximum_count)
{
	struct data_iterator iterator;
	struct player_datum *player;
	short position = 0;
	short count = 0;

	if (unit_index == NONE || !coop_scripts_host())
		return 0;
	/* (in the order (players) lists them: player0 is the first with a unit) */
	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index == NONE)
			continue;
		if (position == 0 && player->unit_index != unit_index)
			return 0;
		if (position >= NAMED_PLAYERS && count < maximum_count)
			unit_indices[count++] = player->unit_index;
		position++;
	}
	return count;
}

void coop_scripts_teleport_followers(
	long unit_index)
{
	long followers[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short count = players_following(unit_index, followers, NUMBEROF(followers));
	short index;

	for (index = 0; index < count; index++)
	{
		player_teleport(player_index_from_unit_index(followers[index]), unit_index,
			&object_get(unit_index)->object.position);
	}
}

void coop_scripts_suspend_followers(
	long unit_index,
	boolean suspended)
{
	long followers[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short count = players_following(unit_index, followers, NUMBEROF(followers));
	short index;

	for (index = 0; index < count; index++)
		unit_scripting_suspended(followers[index], suspended);
}

void coop_scripts_exit_followers(
	long unit_index,
	long vehicle_index)
{
	long followers[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short count = players_following(unit_index, followers, NUMBEROF(followers));
	short index;

	for (index = 0; index < count; index++)
	{
		if (object_get(followers[index])->object.parent_object_index == vehicle_index)
			unit_scripting_exit_vehicle(followers[index]);
	}
}

void coop_scripts_board_followers(
	long unit_index,
	long vehicle_index,
	char const *seat_name)
{
	long followers[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short count = players_following(unit_index, followers, NUMBEROF(followers));
	char follower_seat[64];
	short index;

	if (count == 0 || vehicle_index == NONE)
		return;
	follower_seat_name(seat_name, follower_seat, sizeof(follower_seat));
	/* (each list freed at once: there are few, and the scripts' own are only
	collected after the tick's threads have run) */
	for (index = 0; index < count; index++)
	{
		long list_index = object_list_new();

		if (list_index == NONE)
			break;
		object_list_add(list_index, followers[index]);
		vehicle_scripting_load_magic(vehicle_index, follower_seat, list_index);
		object_list_delete(list_index);
	}
}

boolean coop_scripts_safe_to_save(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	if (game_safe_to_save())
		return TRUE;
	if (!coop_scripts_host())
		return FALSE;
	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL)
	{
		struct biped_datum *biped = biped_try_and_get(player->unit_index);

		if (biped && !TEST_FLAG(biped->object.damage_flags, _object_dead_bit) &&
			!TEST_FLAG(biped->biped.flags, _biped_airborne_bit) && !ai_port_enemies_can_see_unit(player->unit_index))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void coop_scripts_player_add_equipment(
	long unit_index,
	short starting_profile_index,
	boolean reset_equipment)
{
	long followers[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short count = players_following(unit_index, followers, NUMBEROF(followers));
	short index;

	player_add_equipment(unit_index, starting_profile_index, reset_equipment);
	for (index = 0; index < count; index++)
		player_add_equipment(followers[index], starting_profile_index, reset_equipment);
}
