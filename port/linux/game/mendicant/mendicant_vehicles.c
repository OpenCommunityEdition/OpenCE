/*
MENDICANT_VEHICLES.C

MENDICANT AI's use of the level's vehicles (M4, with mendicant_combat.c),
run every tick right after the AI update.

Marines board the Chief's vehicle when he slows near them, and get out when
he does; a squad the Chief has driven away from takes an empty Warthog or
Ghost and follows him. Elites take Ghosts, Banshees and Wraiths, Grunts take
Shades, and two Elites take a Warthog (driver and gunner, put in their seats
directly: an Elite has no animation to climb into one). The game's AI cannot
drive a Warthog (its driver only sits), so an autopilot does: each tick it
writes the driver's throttle and facing, toward the Chief for Marines and
toward the enemy for Elites, reversing when stuck; a crew bails out of a
flipped vehicle. "The Chief" is absolute player 0's unit.

ai.mendicant.vehicle_test: every ten seconds each empty vehicle is tried
with the nearest Marine, Elite and Grunt; the drill log says who could get
in.

Ported from the ai_brain hook library (vehicles.inc and ai_brain.c's
vehicle_try) with the same behaviour.
*/

#include "cseries.h"
#include "memory/data.h"
#include "game/game.h"
#include "ai/ai.h"
#include "ai/actors.h"
#include "ai/actions.h"
#include "objects/objects.h"
#include "units/units.h"
#include "tag_files/tag_files.h"

#include "mendicant_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---------- constants */

/* the autopilot's drivers at once */
#define VEHICLE_DRIVERS 16
/* a Warthog's gunner stays on the gun while the Chief stays this near */
#define GUNNER_STAYS 20.0f

enum
{
	VEHICLE_OTHER,
	VEHICLE_WARTHOG,
	VEHICLE_GHOST,
	VEHICLE_BANSHEE,
	VEHICLE_WRAITH,
	VEHICLE_SCORPION,
	VEHICLE_SHADE,
};

/* actor_action_try_to_enter_vehicle's seat desires */
enum
{
	SEAT_NOT_DRIVER,
	SEAT_GUNNER,
	SEAT_PASSENGER,
	SEAT_DRIVER,
	SEAT_ANY,
};

/* ---------- globals */

static struct
{
	long unit, vehicle;
	boolean hunts;
	long stuck_since, reverse_until;
} autopilot[VEHICLE_DRIVERS];
static long chief_vehicle_last = NONE;
static long vehicle_decide_time;
static long vehicle_tried[MENDICANT_MAX_ACTORS];
/* the Warthog the Chief got out of, its gunner left on the gun */
static long gunner_left_vehicle = NONE;
static long vehicle_test_time;

/* ---------- private code */

static struct actor_datum *vehicles_actor_at(int index)
{
	return (struct actor_datum *)((char *)actor_data->data + index * actor_data->size);
}

static struct object_header_datum *vehicles_header_at(int index)
{
	return (struct object_header_datum *)((char *)object_header_data->data + index * object_header_data->size);
}

/* an object of any type, read as a unit as the hook library read it */
static struct unit_datum *vehicles_unit(long object_index)
{
	if (object_index == NONE || !object_header_data || !object_header_data->valid)
		return NULL;
	return (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_all);
}

static char const *vehicles_tag_name(long tag_index)
{
	char const *name;

	if (tag_index == NONE)
		return "?";
	name = tag_get_name(tag_index);
	return name ? name : "?";
}

static real vehicles_range_to(struct actor_datum *actor, real_point3d const *target)
{
	real x = actor->input.pathfinding_point.x - target->x, y = actor->input.pathfinding_point.y - target->y;

	return (real)sqrt(x * x + y * y);
}

static real vehicles_actor_distance_squared(struct actor_datum *a, struct actor_datum *b)
{
	real x = a->input.pathfinding_point.x - b->input.pathfinding_point.x;
	real y = a->input.pathfinding_point.y - b->input.pathfinding_point.y;
	real z = a->input.pathfinding_point.z - b->input.pathfinding_point.z;

	return x * x + y * y + z * z;
}

/* The nearest living enemy of a side, by where it really is (the Chief
counts for the Covenant). */
static boolean vehicles_nearest_enemy_point(int side, real_point3d const *from, real_point3d *point)
{
	int count = actor_data->count, index;
	real best = 1e30f;
	boolean found = FALSE;

	for (index = 0; index < count; index++)
	{
		struct actor_datum *actor = vehicles_actor_at(index);
		int team;
		real dx, dy;

		if (!actor->identifier || actor->input.body_vitality <= 0.0f)
			continue;
		team = mendicant_side_of(actor->meta.team_index);
		if (mendicant_unit_side(actor) == side || (team != TEAM_HUMAN && team != TEAM_COVENANT))
			continue;
		dx = actor->input.pathfinding_point.x - from->x;
		dy = actor->input.pathfinding_point.y - from->y;
		if (dx * dx + dy * dy < best)
		{
			best = dx * dx + dy * dy;
			*point = actor->input.pathfinding_point;
			found = TRUE;
		}
	}
	if (side == SIDE_COVENANT)
	{
		struct unit_datum *unit = vehicles_unit(mendicant_player_unit());

		if (unit)
		{
			real dx = unit->object.position.x - from->x, dy = unit->object.position.y - from->y;

			if (dx * dx + dy * dy < best)
			{
				*point = unit->object.position;
				found = TRUE;
			}
		}
	}
	return found;
}

static int vehicle_kind(struct unit_datum *vehicle)
{
	char const *name = vehicles_tag_name(vehicle->definition_index);

	if (strstr(name, "warthog") || strstr(name, "jeep"))
		return VEHICLE_WARTHOG;
	if (strstr(name, "ghost"))
		return VEHICLE_GHOST;
	if (strstr(name, "banshee"))
		return VEHICLE_BANSHEE;
	if (strstr(name, "wraith"))
		return VEHICLE_WRAITH;
	if (strstr(name, "scorpion"))
		return VEHICLE_SCORPION;
	if (strstr(name, "gun turret") || strstr(name, "shade"))
		return VEHICLE_SHADE;
	return VEHICLE_OTHER;
}

/* upright and not destroyed (Warthogs have no health: indestructible) */
static boolean vehicle_usable(struct unit_datum *vehicle)
{
	return vehicle->object.up.k > 0.5f &&
		(vehicle->object.maximum_body_vitality <= 0.0f || vehicle->object.body_vitality > 0.0f);
}

static real vehicle_speed(struct unit_datum *vehicle)
{
	real x = vehicle->object.translational_velocity.i;
	real y = vehicle->object.translational_velocity.j;
	real z = vehicle->object.translational_velocity.k;

	return (real)sqrt(x * x + y * y + z * z) * 30.0f;
}

/* how many units ride in it (driver, gunner, passengers) */
static int vehicle_riders(long vehicle_index)
{
	int riders = 0, count = actor_data->count, index;
	struct unit_datum *chief;

	for (index = 0; index < count; index++)
	{
		struct actor_datum *actor = vehicles_actor_at(index);
		struct unit_datum *unit = actor->identifier ? vehicles_unit(actor->meta.unit_index) : NULL;

		riders += unit && unit->object.parent_object_index == vehicle_index;
	}
	chief = vehicles_unit(mendicant_player_unit());
	return riders + (chief && chief->object.parent_object_index == vehicle_index);
}

/* the nearest empty, usable vehicle of the kinds in mask, within reach of a point */
static long vehicle_free_near(real_point3d const *point, real reach, unsigned long mask, struct unit_datum **out)
{
	int count, index;
	long best = NONE;
	real best_distance = reach * reach;

	if (!object_header_data || !object_header_data->valid)
		return NONE;
	count = object_header_data->count;
	for (index = 0; index < count; index++)
	{
		struct object_header_datum *header = vehicles_header_at(index);
		struct unit_datum *vehicle = (struct unit_datum *)header->datum;
		real x, y, z, distance;
		long vehicle_index;

		if (!header->identifier || header->type != _object_type_vehicle || !vehicle || !vehicle_usable(vehicle) ||
			!(mask >> vehicle_kind(vehicle) & 1) || vehicle->unit.driver_object_index != NONE)
			continue;
		x = vehicle->object.position.x - point->x;
		y = vehicle->object.position.y - point->y;
		z = vehicle->object.position.z - point->z;
		distance = x * x + y * y + z * z;
		vehicle_index = ((long)header->identifier << 16) | index;
		if (distance < best_distance && vehicle_riders(vehicle_index) == 0)
		{
			best = vehicle_index;
			best_distance = distance;
			*out = vehicle;
		}
	}
	return best;
}

/* The actor walks to the vehicle and gets in, if its animations have that seat. */
static boolean try_to_enter(long actor_index, long vehicle_index, short desire)
{
	return actor_action_try_to_enter_vehicle(actor_index, vehicle_index, "", desire, 0, NULL);
}

/* Puts a unit straight into the named seat; TRUE when it is in the vehicle
afterwards. */
static boolean magic_enter(long unit_index, long vehicle_index, char const *seat)
{
	struct unit_datum *unit;

	unit_scripting_enter_vehicle(unit_index, vehicle_index, seat);
	unit = vehicles_unit(unit_index);
	return unit && unit->object.parent_object_index == vehicle_index;
}

/* seats: a part of the seat names ("" every seat, "W-passenger" a Warthog's passengers) */
static void unload_seats(long vehicle_index, char const *seats)
{
	vehicle_scripting_unload(vehicle_index, seats);
}

static void unload_vehicle(long vehicle_index)
{
	unload_seats(vehicle_index, "");
}

static void autopilot_add(long unit, long vehicle, boolean hunts)
{
	int slot;

	for (slot = 0; slot < VEHICLE_DRIVERS; slot++)
	{
		if (autopilot[slot].unit == unit)
			return;
	}
	for (slot = 0; slot < VEHICLE_DRIVERS; slot++)
	{
		if (autopilot[slot].unit == NONE || autopilot[slot].unit == 0)
		{
			autopilot[slot].unit = unit;
			autopilot[slot].vehicle = vehicle;
			autopilot[slot].hunts = hunts;
			autopilot[slot].stuck_since = -1;
			autopilot[slot].reverse_until = 0;
			return;
		}
	}
}

/* the autopilot, every tick: Marines drive to the Chief and stop 10 world
units off; Elites drive at the nearest human and keep about 15 off,
circling; stuck: a second in reverse */
static void autopilot_update(long time)
{
	struct unit_datum *chief = vehicles_unit(mendicant_player_unit());
	int slot;

	for (slot = 0; slot < VEHICLE_DRIVERS; slot++)
	{
		struct unit_datum *unit, *vehicle;
		real_point3d target;
		real stop, dx, dy, distance, throttle, speed;
		real_vector3d facing;

		if (autopilot[slot].unit == NONE || autopilot[slot].unit == 0)
			continue;
		unit = vehicles_unit(autopilot[slot].unit);
		vehicle = vehicles_unit(autopilot[slot].vehicle);
		if (!unit || !vehicle || unit->object.parent_object_index != autopilot[slot].vehicle ||
			unit->object.body_vitality <= 0.0f)
		{
			autopilot[slot].unit = NONE; /* got out, or died */
			continue;
		}
		if (!vehicle_usable(vehicle))
		{
			/* flipped: everyone out (an Elite with no animation for it, at once) */
			if (autopilot[slot].hunts)
				unit_exit_seat_end(autopilot[slot].unit);
			else
				unload_vehicle(autopilot[slot].vehicle);
			autopilot[slot].unit = NONE;
			continue;
		}
		if (autopilot[slot].hunts)
		{
			if (!vehicles_nearest_enemy_point(SIDE_COVENANT, &vehicle->object.position, &target))
				continue;
			stop = 15.0f;
		}
		else
		{
			if (!chief)
				continue;
			target = chief->object.position;
			stop = 10.0f;
		}
		dx = target.x - vehicle->object.position.x;
		dy = target.y - vehicle->object.position.y;
		distance = (real)sqrt(dx * dx + dy * dy) + 0.001f;
		facing.i = dx / distance;
		facing.j = dy / distance;
		facing.k = 0.0f;
		throttle = distance > stop ? 1.0f : 0.0f;
		if (autopilot[slot].hunts && distance <= stop)
		{
			/* circle the enemy so the gunner keeps firing */
			facing.i = -dy / distance;
			facing.j = dx / distance;
			throttle = 0.6f;
		}
		speed = vehicle_speed(vehicle);
		if (throttle > 0.0f && speed < 0.5f)
		{
			if (autopilot[slot].stuck_since < 0)
				autopilot[slot].stuck_since = time;
			if (time - autopilot[slot].stuck_since > 60 && time > autopilot[slot].reverse_until + 60)
				autopilot[slot].reverse_until = time + 30;
		}
		else
		{
			autopilot[slot].stuck_since = -1;
		}
		if (time < autopilot[slot].reverse_until)
			throttle = -1.0f;
		unit->unit.throttle.i = throttle;
		unit->unit.throttle.j = 0.0f;
		unit->unit.throttle.k = 0.0f;
		unit->unit.desired_facing_vector = facing;
	}
}

/* ---------- public code */

void mendicant_vehicles_reset(
	void)
{
	int slot;

	for (slot = 0; slot < VEHICLE_DRIVERS; slot++)
	{
		autopilot[slot].unit = NONE;
		autopilot[slot].vehicle = NONE;
		autopilot[slot].hunts = FALSE;
		autopilot[slot].stuck_since = -1;
		autopilot[slot].reverse_until = 0;
	}
	memset(vehicle_tried, 0, sizeof(vehicle_tried));
	chief_vehicle_last = NONE;
	gunner_left_vehicle = NONE;
	vehicle_decide_time = 0;
	vehicle_test_time = 0;
}

/* After the AI update, every tick: the autopilot; once a second, who gets
in and out. */
void mendicant_vehicles_update(
	void)
{
	long time, chief_vehicle, chief_unit;
	int count, index;
	struct unit_datum *chief, *chief_ride;
	boolean chief_drives;

	if (!mendicant_active)
		return;
	time = mendicant_time();
	autopilot_update(time);
	if (time >= vehicle_decide_time && time - vehicle_decide_time < 30)
		return;
	vehicle_decide_time = time;
	if (!actor_data || !actor_data->valid)
		return;
	count = actor_data->count;
	chief_unit = mendicant_player_unit();
	chief = vehicles_unit(chief_unit);
	chief_vehicle = chief ? chief->object.parent_object_index : NONE;
	chief_ride = chief_vehicle != NONE ? vehicles_unit(chief_vehicle) : NULL;
	chief_drives = chief_ride && chief_ride->unit.driver_object_index == chief_unit;

	/* the Chief got out: his passengers get out with him; a Warthog's gunner
	stays on the gun while the Chief stays near, and comes after him once he
	goes further than 20 world units */
	if (chief_vehicle == NONE && chief_vehicle_last != NONE)
	{
		struct unit_datum *left = vehicles_unit(chief_vehicle_last);

		if (left && vehicle_kind(left) == VEHICLE_WARTHOG && left->unit.gunner_object_index != NONE)
		{
			unload_seats(chief_vehicle_last, "W-passenger");
			gunner_left_vehicle = chief_vehicle_last;
		}
		else if (left)
		{
			unload_vehicle(chief_vehicle_last);
		}
		if (mendicant_drill_file)
			fprintf(mendicant_drill_file, "%ld,vehicle,unload,%ld,gunner_stays=%d\n", time, chief_vehicle_last,
				gunner_left_vehicle == chief_vehicle_last);
	}
	if (gunner_left_vehicle != NONE)
	{
		struct unit_datum *left = vehicles_unit(gunner_left_vehicle);

		if (!left || chief_vehicle == gunner_left_vehicle || left->unit.gunner_object_index == NONE)
		{
			gunner_left_vehicle = NONE; /* the Chief is back in, or the gunner is gone */
		}
		else if (chief && chief_vehicle == NONE &&
			(chief->object.position.x - left->object.position.x) * (chief->object.position.x - left->object.position.x) +
			(chief->object.position.y - left->object.position.y) * (chief->object.position.y - left->object.position.y) >
			GUNNER_STAYS * GUNNER_STAYS)
		{
			unload_seats(gunner_left_vehicle, "W-gunner");
			if (mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,vehicle,gunner_follows,%ld\n", time, gunner_left_vehicle);
			gunner_left_vehicle = NONE;
		}
	}
	chief_vehicle_last = chief_drives ? chief_vehicle : NONE;

	for (index = 0; index < count && index < MENDICANT_MAX_ACTORS; index++)
	{
		struct actor_datum *actor = vehicles_actor_at(index);
		long actor_index;
		struct unit_datum *unit;
		int team, type;
		real_point3d const *at;

		if (!actor->identifier || actor->input.vehicle_index != NONE || actor->state.action == _actor_action_vehicle ||
			time - vehicle_tried[index] < 300 || mendicant_team_mode(actor->meta.team_index) == MODE_GAME)
			continue;
		actor_index = ((long)actor->identifier << 16) | index;
		unit = vehicles_unit(actor->meta.unit_index);
		if (!unit || actor->meta.unit_index == chief_unit || unit->object.body_vitality <= 0.0f)
			continue;
		team = mendicant_side_of(actor->meta.team_index);
		type = actor->meta.type;
		at = &unit->object.position;
		if (team == TEAM_HUMAN)
		{
			if (chief_drives && vehicle_speed(chief_ride) < 3.0f && vehicles_range_to(actor, &chief_ride->object.position) < 12.0f)
			{
				/* ride with the Chief: the gun first, then a seat */
				short desire = chief_ride->unit.gunner_object_index == NONE ? SEAT_GUNNER : SEAT_PASSENGER;

				mendicant_follow_cancel(actor_index); /* no walking order holds him off the seat */
				if (try_to_enter(actor_index, chief_vehicle, desire))
				{
					if (mendicant_drill_file)
						fprintf(mendicant_drill_file, "%ld,vehicle,board_chief,%d,seat=%d\n", time, index, desire);
				}
				vehicle_tried[index] = time;
			}
			else if (chief_drives && vehicles_range_to(actor, &chief->object.position) > 30.0f)
			{
				/* he drove off: an empty Warthog or Ghost near them to follow him in */
				struct unit_datum *vehicle = NULL;
				long vehicle_index = vehicle_free_near(at, 20.0f, 1UL << VEHICLE_WARTHOG | 1UL << VEHICLE_GHOST, &vehicle);

				if (vehicle_index != NONE)
					mendicant_follow_cancel(actor_index);
				if (vehicle_index != NONE && try_to_enter(actor_index, vehicle_index, SEAT_DRIVER))
				{
					if (mendicant_drill_file)
						fprintf(mendicant_drill_file, "%ld,vehicle,take,%d,kind=%d\n", time, index, vehicle_kind(vehicle));
				}
				vehicle_tried[index] = time;
			}
		}
		else if (team == TEAM_COVENANT)
		{
			real_point3d enemy;
			real ex, ey;
			struct unit_datum *vehicle = NULL;
			long vehicle_index;

			if (!vehicles_nearest_enemy_point(SIDE_COVENANT, at, &enemy))
				continue;
			ex = enemy.x - at->x;
			ey = enemy.y - at->y;
			if (ex * ex + ey * ey > 70.0f * 70.0f)
				continue; /* nobody to fight with it */
			if (ACTOR_IS_ELITE(type))
			{
				vehicle_index = vehicle_free_near(at, 15.0f,
					1UL << VEHICLE_GHOST | 1UL << VEHICLE_BANSHEE | 1UL << VEHICLE_WRAITH, &vehicle);
				if (vehicle_index != NONE)
				{
					if (try_to_enter(actor_index, vehicle_index, SEAT_DRIVER))
					{
						if (mendicant_drill_file)
							fprintf(mendicant_drill_file, "%ld,vehicle,elite_takes,%d,kind=%d\n", time, index,
								vehicle_kind(vehicle));
					}
				}
				else if ((vehicle_index = vehicle_free_near(at, 8.0f, 1UL << VEHICLE_WARTHOG, &vehicle)) != NONE)
				{
					/* a Warthog: this Elite drives, the nearest other Elite mans the gun */
					long gunner = NONE;
					int index_other;

					for (index_other = 0; index_other < count && gunner == NONE; index_other++)
					{
						struct actor_datum *other = vehicles_actor_at(index_other);

						if (index_other != index && other->identifier && ACTOR_IS_ELITE(other->meta.type) &&
							other->meta.team_index == actor->meta.team_index && other->input.vehicle_index == NONE &&
							vehicles_actor_distance_squared(actor, other) < 10.0f * 10.0f)
							gunner = other->meta.unit_index;
					}
					if (gunner != NONE && magic_enter(actor->meta.unit_index, vehicle_index, "W-driver") &&
						magic_enter(gunner, vehicle_index, "W-gunner"))
					{
						autopilot_add(actor->meta.unit_index, vehicle_index, TRUE);
						if (mendicant_drill_file)
							fprintf(mendicant_drill_file, "%ld,vehicle,elites_take_warthog,%d\n", time, index);
					}
				}
			}
			else if (ACTOR_IS_GRUNT(type))
			{
				vehicle_index = vehicle_free_near(at, 10.0f, 1UL << VEHICLE_SHADE, &vehicle);
				if (vehicle_index != NONE && try_to_enter(actor_index, vehicle_index, SEAT_GUNNER))
				{
					if (mendicant_drill_file)
						fprintf(mendicant_drill_file, "%ld,vehicle,grunt_mans_shade,%d\n", time, index);
				}
			}
			vehicle_tried[index] = time;
		}
	}

	/* Marines who drove a Warthog: the autopilot takes them to the Chief;
	near him, on foot, they get out; and the riders of a Marine vehicle near
	a Chief on foot get out too */
	for (index = 0; index < count && index < MENDICANT_MAX_ACTORS; index++)
	{
		struct actor_datum *actor = vehicles_actor_at(index);
		long vehicle_index;
		struct unit_datum *vehicle;

		if (!actor->identifier || mendicant_side_of(actor->meta.team_index) != TEAM_HUMAN ||
			actor->input.vehicle_index == NONE)
			continue;
		vehicle_index = actor->input.vehicle_index;
		vehicle = vehicles_unit(vehicle_index);
		if (!vehicle || vehicle_index == chief_vehicle || vehicle_index == gunner_left_vehicle)
			continue;
		if (vehicle->unit.driver_object_index == actor->meta.unit_index && vehicle_kind(vehicle) == VEHICLE_WARTHOG)
			autopilot_add(actor->meta.unit_index, vehicle_index, FALSE);
		if (chief && chief_vehicle == NONE && vehicles_range_to(actor, &chief->object.position) < 15.0f &&
			vehicle_speed(vehicle) < 2.0f)
		{
			unload_vehicle(vehicle_index);
			if (mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,vehicle,unload_at_chief,%ld\n", time, vehicle_index);
		}
	}
}

/* ai.mendicant.vehicle_test: every ten seconds each empty vehicle is tried
with the nearest Marine, Elite and Grunt. */
void mendicant_vehicle_try(
	void)
{
	long time = mendicant_time();
	int actor_count, object_count, object;

	if (time >= vehicle_test_time && time - vehicle_test_time < 300)
		return;
	vehicle_test_time = time;
	if (!actor_data || !actor_data->valid || !object_header_data)
		return;
	actor_count = actor_data->count;
	object_count = object_header_data->count;
	for (object = 0; object < object_count; object++)
	{
		struct object_header_datum *header = vehicles_header_at(object);
		struct unit_datum *vehicle = (struct unit_datum *)header->datum;
		long vehicle_index;
		int kind;

		if (!header->identifier || header->type != _object_type_vehicle || !vehicle ||
			vehicle->unit.driver_object_index != NONE || vehicle->object.body_vitality <= 0.0f)
			continue;
		vehicle_index = ((long)header->identifier << 16) | object;
		for (kind = 0; kind < 3; kind++)
		{
			struct actor_datum *best = NULL;
			long best_index = NONE;
			real best_distance = 100.0f * 100.0f;
			boolean entered;
			int index;

			for (index = 0; index < actor_count; index++)
			{
				struct actor_datum *actor = vehicles_actor_at(index);
				int type = actor->meta.type;
				boolean fits = kind == 0 ? ACTOR_IS_MARINE(type) : kind == 1 ? ACTOR_IS_ELITE(type) : ACTOR_IS_GRUNT(type);
				real x, y;

				if (!actor->identifier || !fits || actor->input.vehicle_index != NONE ||
					actor->meta.team_index == TEAM_PLAYER)
					continue;
				x = actor->input.pathfinding_point.x - vehicle->object.position.x;
				y = actor->input.pathfinding_point.y - vehicle->object.position.y;
				if (x * x + y * y < best_distance)
				{
					best = actor;
					best_index = ((long)actor->identifier << 16) | index;
					best_distance = x * x + y * y;
				}
			}
			if (!best)
				continue;
			entered = try_to_enter(best_index, vehicle_index, SEAT_DRIVER);
			if (mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,vehicle_test,%s,%s,distance=%.0f,entered=%d\n", time,
					vehicles_tag_name(vehicle->definition_index), kind == 0 ? "marine" : kind == 1 ? "elite" : "grunt",
					(real)sqrt(best_distance), entered);
			if (entered)
				break;
		}
	}
}
