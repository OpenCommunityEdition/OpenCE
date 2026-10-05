/*
COOP_ENEMIES.C

Network co-op's extra enemies: Server Setup's EXTRA ENEMIES
(network.coop_enemies, a percentage). For each player past the first, each
squad of the players' enemies that a level places (encounters.c's
encounter_create) gets that much of its count more: at 100%, two players
meet twice the squad, four players four times it. The extra enemies stand
in rings about the squad's starting locations, on open ground, so they do
not start inside each other.

A squad a script loads into a dropship (vehicle_load_magic) can be larger
than the dropship's seats. The riders left without a seat are erased, and
kept: once the dropship's own riders get out (vehicle_unload, or any other
way), they are placed beside those riders, a few ticks apart, and drop and
fight as they do. A dropship gone first takes them with it.

Only the host runs the AI, so all of this is the host's.
*/

#include "cseries.h"
#include "ai/actor_placement.h"
#include "ai/actors.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/encounters.h"
#include "game/game.h"
#include "game/game_allegiance.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/units.h"
#include "units/unit_definitions.h"

#include "coop_enemies.h"
#include "network_coop.h"

#include <math.h>

/* port_config.c's */
long config_integer(char const *name);

/* ---------- constants */

enum
{
	/* the actors a level keeps for its own placements, which the extra
	enemies never take: as many as the Xbox's whole pool */
	COOP_ENEMIES_LEVEL_ACTORS = 256,
	/* EXTRA ENEMIES' greatest (Server Setup's choices: 0 to 200%) */
	COOP_ENEMIES_MAXIMUM_PERCENT = 200,
	/* the places tried for an extra enemy, a ring further out each */
	SPREAD_ATTEMPTS = 8,
	SPREAD_PLACES_PER_RING = 6,
	MAXIMUM_RIDING_VEHICLES = 32,
	MAXIMUM_SEATED_RIDERS = 16,
	MAXIMUM_KEPT_RIDERS = 512,
	/* a kept rider placed every this many ticks, as the seated ones
	jumped out */
	RIDER_RELEASE_TICKS = 4,
};

/* the rings' spacing, and the height over the ground the way to a place is
looked along, and the ground looked for below it */
#define SPREAD_SPACING 0.8f
#define SPREAD_STEP_HEIGHT 0.6f
#define SPREAD_GROUND_DEPTH 2.0f
/* a kept rider's place beside a rider who got out, each a little apart */
#define RIDER_SPACING 0.5f

/* ---------- structures */

/* a vehicle a script loaded riders into: its seated ones, and whether they
have begun to get out */
struct coop_riding_vehicle
{
	long vehicle_index;
	long seated_unit_indices[MAXIMUM_SEATED_RIDERS];
	short seat_indices[MAXIMUM_SEATED_RIDERS];
	short seated_count;
	boolean releasing;
	short released_count;
	long next_release_time;
};

/* a rider left without a seat, to place once its vehicle's get out */
struct coop_kept_rider
{
	long vehicle_index;
	long variant_definition_index;
	long encounter_index;
	short squad_index;
};

/* ---------- globals */

static struct
{
	/* EXTRA ENEMIES, as the game began with it */
	short percent;
	struct coop_riding_vehicle vehicles[MAXIMUM_RIDING_VEHICLES];
	short vehicle_count;
	struct coop_kept_rider riders[MAXIMUM_KEPT_RIDERS];
	short rider_count;
} coop_enemies;

/* ---------- private code */

static boolean coop_enemies_host(
	void)
{
	return game_connection() == _game_connection_network_server && network_coop_active();
}

/* the players in the game, every machine's: the network game's (a level
places its first enemies before the players' units are made, as it loads:
game_initialize_for_new_map) */
static short coop_enemies_player_count(
	void)
{
	struct network_game *game = network_game_get_game();
	short count = 0;
	short index;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		if (network_player_is_valid(&game->players[index]))
			count++;
	}
	return count;
}

/* whether the way from a to b is open (no structure between them) */
static boolean coop_enemies_open(
	real_point3d const *a,
	real_point3d const *b)
{
	struct collision_result collision;
	real_vector3d vector;

	vector.i = b->x - a->x;
	vector.j = b->y - a->y;
	vector.k = b->z - a->z;
	return !collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit), a, &vector, NONE, &collision);
}

/* the ground below a point, within SPREAD_GROUND_DEPTH of it */
static boolean coop_enemies_ground(
	real_point3d const *point,
	real_point3d *ground)
{
	struct collision_result collision;
	real_vector3d vector = { 0.0f, 0.0f, -(SPREAD_STEP_HEIGHT + SPREAD_GROUND_DEPTH) };

	if (!collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
		point, &vector, NONE, &collision))
	{
		return FALSE;
	}
	*ground = collision.point;
	return TRUE;
}

static struct coop_riding_vehicle *coop_enemies_riding_vehicle(
	long vehicle_index,
	boolean create)
{
	short index;

	for (index = 0; index < coop_enemies.vehicle_count; index++)
	{
		if (coop_enemies.vehicles[index].vehicle_index == vehicle_index)
			return &coop_enemies.vehicles[index];
	}
	if (!create || coop_enemies.vehicle_count >= MAXIMUM_RIDING_VEHICLES)
		return NULL;
	csmemset(&coop_enemies.vehicles[coop_enemies.vehicle_count], 0, sizeof(coop_enemies.vehicles[0]));
	coop_enemies.vehicles[coop_enemies.vehicle_count].vehicle_index = vehicle_index;
	return &coop_enemies.vehicles[coop_enemies.vehicle_count++];
}

/* the riders kept for a vehicle forgotten, and the vehicle with them */
static void coop_enemies_forget_vehicle(
	short vehicle_number)
{
	long vehicle_index = coop_enemies.vehicles[vehicle_number].vehicle_index;
	short index, kept = 0;

	for (index = 0; index < coop_enemies.rider_count; index++)
	{
		if (coop_enemies.riders[index].vehicle_index != vehicle_index)
			coop_enemies.riders[kept++] = coop_enemies.riders[index];
	}
	coop_enemies.rider_count = kept;
	coop_enemies.vehicles[vehicle_number] = coop_enemies.vehicles[--coop_enemies.vehicle_count];
}

/* whether a vehicle's seated riders have begun to get out: one alive and
out of it (one killed in its seat is not getting out) */
static boolean coop_enemies_riders_getting_out(
	struct coop_riding_vehicle const *vehicle)
{
	short index;

	for (index = 0; index < vehicle->seated_count; index++)
	{
		struct object_datum *unit = object_try_and_get_and_verify_type(vehicle->seated_unit_indices[index],
			_object_mask_unit);

		if (unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
			unit->object.parent_object_index != vehicle->vehicle_index)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* where a vehicle's `number`th kept rider is placed: beside one of the
riders who got out of it (where it is now: out of the vehicle, in the
open), a ring further about them each round; else at a seat's marker, else
the vehicle's place */
static void coop_enemies_rider_place(
	struct coop_riding_vehicle const *vehicle,
	short number,
	real_point3d *position,
	real *facing)
{
	struct object_datum *object = object_get(vehicle->vehicle_index);
	struct unit_definition *definition = unit_definition_get(object->definition_index);
	long out_unit_indices[MAXIMUM_SEATED_RIDERS];
	short out_count = 0;
	short index, round;
	real angle;

	*position = object->object.position;
	*facing = (real)atan2(object->object.forward.j, object->object.forward.i);
	for (index = 0; index < vehicle->seated_count; index++)
	{
		struct object_datum *unit = object_try_and_get_and_verify_type(vehicle->seated_unit_indices[index],
			_object_mask_unit);

		if (unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
			unit->object.parent_object_index != vehicle->vehicle_index)
		{
			out_unit_indices[out_count++] = vehicle->seated_unit_indices[index];
		}
	}
	if (out_count > 0)
	{
		*position = object_get(out_unit_indices[number % out_count])->object.position;
		round = (short)(number / out_count);
	}
	else
	{
		struct object_marker marker;

		round = number;
		if (vehicle->seated_count > 0 && VALID_INDEX(vehicle->seat_indices[0], definition->unit.seats.count))
		{
			struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, vehicle->seat_indices[0],
				struct unit_seat);

			if (seat->marker_name[0] &&
				object_get_marker_by_name(vehicle->vehicle_index, seat->marker_name, &marker, 1) > 0)
			{
				*position = marker.matrix.position;
			}
		}
	}
	/* (never on the rider's own place: a ring about it, further each round) */
	angle = (real)number * (_pi * 2.0f / SPREAD_PLACES_PER_RING);
	position->x += (real)cos(angle) * RIDER_SPACING * (real)(round / SPREAD_PLACES_PER_RING + 1);
	position->y += (real)sin(angle) * RIDER_SPACING * (real)(round / SPREAD_PLACES_PER_RING + 1);
}

/* ---------- public code */

void coop_enemies_new_game(
	void)
{
	csmemset(&coop_enemies, 0, sizeof(coop_enemies));
	coop_enemies.percent = (short)PIN(config_integer("network.coop_enemies"), 0, COOP_ENEMIES_MAXIMUM_PERCENT);
}

void coop_enemies_reset(
	void)
{
	coop_enemies.vehicle_count = 0;
	coop_enemies.rider_count = 0;
}

short coop_enemies_extra_count(
	long encounter_index,
	short count)
{
	struct encounter_datum *encounter;
	long extra, room;
	short players;

	if (count <= 0 || coop_enemies.percent <= 0 || !coop_enemies_host())
		return 0;
	encounter = encounter_get(encounter_index);
	if (!game_team_is_enemy(_game_team_player, encounter->team_index))
		return 0;
	players = coop_enemies_player_count();
	if (players <= 1)
		return 0;
	extra = ((long)count * coop_enemies.percent * (players - 1) + 50) / 100;
	room = MAXIMUM_ACTORS - COOP_ENEMIES_LEVEL_ACTORS - actor_data->actual_count;
	return (short)PIN(extra, 0, MAX(room, 0));
}

boolean coop_enemies_spread_position(
	real_point3d const *origin,
	short number,
	real_point3d *position)
{
	real_point3d from = *origin;
	short place = (short)((number - 1) % SPREAD_PLACES_PER_RING);
	short ring = (short)((number - 1) / SPREAD_PLACES_PER_RING);
	short attempt;

	from.z += SPREAD_STEP_HEIGHT;
	for (attempt = 0; attempt < SPREAD_ATTEMPTS; attempt++, ring++)
	{
		/* (each ring turned half a place from the last, so its places fall
		between the last's) */
		real angle = ((real)place + 0.5f * (real)(ring % 2)) * (_pi * 2.0f / SPREAD_PLACES_PER_RING);
		real radius = SPREAD_SPACING * (real)(ring + 1);
		real_point3d to = from;

		to.x += (real)cos(angle) * radius;
		to.y += (real)sin(angle) * radius;
		if (coop_enemies_open(&from, &to) && coop_enemies_ground(&to, position))
			return TRUE;
	}
	return FALSE;
}

void coop_enemies_rider_seated(
	long vehicle_index,
	long unit_index)
{
	struct coop_riding_vehicle *vehicle;
	struct unit_datum *unit;

	if (coop_enemies.percent <= 0 || !coop_enemies_host())
		return;
	vehicle = coop_enemies_riding_vehicle(vehicle_index, TRUE);
	unit = unit_get(unit_index);
	if (!vehicle)
		return;
	/* (a vehicle loaded again: the riders that got out of it before are not
	this load's) */
	{
		short index, kept = 0;

		for (index = 0; index < vehicle->seated_count; index++)
		{
			struct object_datum *seated = object_try_and_get_and_verify_type(vehicle->seated_unit_indices[index],
				_object_mask_unit);

			if (seated && seated->object.parent_object_index == vehicle_index)
			{
				vehicle->seated_unit_indices[kept] = vehicle->seated_unit_indices[index];
				vehicle->seat_indices[kept++] = vehicle->seat_indices[index];
			}
		}
		vehicle->seated_count = kept;
	}
	if (vehicle->seated_count >= MAXIMUM_SEATED_RIDERS)
		return;
	vehicle->seated_unit_indices[vehicle->seated_count] = unit_index;
	vehicle->seat_indices[vehicle->seated_count] = unit->unit.parent_seat_index;
	vehicle->seated_count++;
	/* (a vehicle loaded again, after its riders got out: they get out again) */
	vehicle->releasing = FALSE;
}

boolean coop_enemies_rider_unseated(
	long vehicle_index,
	long unit_index)
{
	struct unit_datum *unit;
	struct actor_datum *actor;
	struct coop_kept_rider *rider;
	struct coop_riding_vehicle *vehicle;

	if (coop_enemies.percent <= 0 || !coop_enemies_host() || coop_enemies.rider_count >= MAXIMUM_KEPT_RIDERS)
		return FALSE;
	unit = unit_get(unit_index);
	vehicle = coop_enemies_riding_vehicle(vehicle_index, FALSE);
	/* (kept only for a vehicle with riders seated, whose getting out places
	it: one of a load that seated nobody is left where it is) */
	if (unit->object.type != _object_type_biped || unit->unit.actor_index == NONE || !vehicle ||
		vehicle->seated_count == 0)
	{
		return FALSE;
	}
	actor = actor_get(unit->unit.actor_index);
	if (actor->meta.swarm || actor->meta.encounter_index == NONE ||
		!game_team_is_enemy(_game_team_player, encounter_get(actor->meta.encounter_index)->team_index))
	{
		return FALSE;
	}
	rider = &coop_enemies.riders[coop_enemies.rider_count++];
	rider->vehicle_index = vehicle_index;
	rider->variant_definition_index = actor->meta.variant_definition_index;
	rider->encounter_index = actor->meta.encounter_index;
	rider->squad_index = actor->meta.squad_index;
	return TRUE;
}

void coop_enemies_update(
	void)
{
	long now = game_time_get();
	short vehicle_number;

	if (!coop_enemies_host())
		return;
	for (vehicle_number = coop_enemies.vehicle_count - 1; vehicle_number >= 0; vehicle_number--)
	{
		struct coop_riding_vehicle *vehicle = &coop_enemies.vehicles[vehicle_number];
		struct object_datum *object = object_try_and_get_and_verify_type(vehicle->vehicle_index, _object_mask_unit);
		short index;

		/* (the vehicle's next kept rider) */
		for (index = 0; index < coop_enemies.rider_count; index++)
		{
			if (coop_enemies.riders[index].vehicle_index == vehicle->vehicle_index)
				break;
		}
		/* (a vehicle gone, or destroyed, takes its kept riders with it; one
		with none kept, every one loaded seated or all placed, is done with) */
		if (!object || TEST_FLAG(object->object.damage_flags, _object_dead_bit) || index >= coop_enemies.rider_count)
		{
			coop_enemies_forget_vehicle(vehicle_number);
			continue;
		}
		if (!vehicle->releasing)
		{
			if (!coop_enemies_riders_getting_out(vehicle))
				continue;
			vehicle->releasing = TRUE;
			vehicle->released_count = 0;
			vehicle->next_release_time = now;
		}
		if (now < vehicle->next_release_time)
			continue;
		{
			struct coop_kept_rider rider = coop_enemies.riders[index];
			struct encounter_definition *encounter_definition;
			struct squad_definition *squad_definition;
			struct actor_starting_location location;

			coop_enemies.riders[index] = coop_enemies.riders[--coop_enemies.rider_count];
			encounter_definition = TAG_BLOCK_GET_ELEMENT(&global_scenario_get()->ai_encounters,
				DATUM_INDEX_TO_ABSOLUTE_INDEX(rider.encounter_index), struct encounter_definition);
			squad_definition = TAG_BLOCK_GET_ELEMENT(&encounter_definition->squads, rider.squad_index,
				struct squad_definition);
			/* (as the squad's: its states, no command list of a starting
			location's) */
			csmemset(&location, 0, sizeof(location));
			coop_enemies_rider_place(vehicle, vehicle->released_count++, &location.position, &location.facing);
			location.cluster_index = NONE;
			location.noncombat_sequence_id = NONE;
			location.default_state = squad_definition->default_state;
			location.initial_state = squad_definition->initial_state;
			location.actor_variant_index = NONE;
			location.command_list_index = NONE;
			actor_place(rider.variant_definition_index, rider.encounter_index, rider.squad_index, &location, FALSE, 0);
			encounter_update_status(rider.encounter_index);
		}
		vehicle->next_release_time = now + RIDER_RELEASE_TICKS;
	}
}
