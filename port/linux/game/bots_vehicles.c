/* Focused adaptation of BirchWoodGod/halo-ce-vita's ground-vehicle bots,
via thelinkin3000/halo-ce-universal 18a94774a73ca275e9c0ab4f281e492ad8bb656e.
The inherited vehicle definitions and seat checks are GPL-3.0-only.
See docs/reading/bots_and_android_features.md for provenance.

The standalone enhanced controller owns paths and combat. This module chooses an accessible seat,
uses the action button to enter/leave, and translates a path direction into
driving controls. No new attachment, vehicle or network rules are needed. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "game/game_engine.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "units/vehicle_definitions.h"
#include "tag_files/tag_groups.h"
#include "bots.h"
#include "bots_tactics.h"
#include "bots_vehicles.h"
#include "infection.h"
#include "enhanced_bots_flight.h"
#include "mcc_seats.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include <math.h>
#include <stddef.h>

int config_boolean(char const *name);

/* The public vehicle tag layout used by the fork; no SDK declarations. */
struct bots_vehicle_definition
{
	byte unit[0x2F4];
	short type;
	short pad;
	byte unknown[0x94];
	real radius;
};
typedef char bots_vehicle_radius_offset[offsetof(struct bots_vehicle_definition, radius) == 0x38C ? 1 : -1];

static struct bots_vehicle_state
{
	long sought, ridden, avoided;
	short seat;
	long next_scan, next_companion_scan, deadline, next_press, stuck_tick, reverse_until, stuck_count;
	long unit_index, spawn_tick, sought_tick, ride_tick, avoided_until;
	long boarding_until, driverless_tick, off_arc_tick, last_enemy_tick, charge_tick;
	long target_unit;
	long last_progress;
	real best_distance;
	boolean flipping;
	real_point3d entrance, last_position;
} vehicles[BOTS_MAXIMUM];

static real distance(real_point3d const *a, real_point3d const *b)
{
	real x = a->x - b->x, y = a->y - b->y, z = a->z - b->z;
	return (real)sqrt(x*x + y*y + z*z);
}

static real angle(real value)
{
	value = (real)fmod(value, 6.28318531f);
	if (value > 3.14159265f) value -= 6.28318531f;
	if (value < -3.14159265f) value += 6.28318531f;
	return value;
}

static struct bots_vehicle_definition *definition(long index)
{
	static struct bots_vehicle_definition seated_biped;
	if (mcc_seat_parent(index))
	{
		seated_biped.type = -1;
		return &seated_biped;
	}
	return (struct bots_vehicle_definition *)tag_get(VEHICLE_DEFINITION_TAG, object_get(index)->definition_index);
}

static long vehicle_driver(long index)
{
	long controller = mcc_seat_controller(index);
	return controller != NONE ? controller : unit_get(index)->unit.driver_object_index;
}

static boolean drivable(long index)
{
	short type = definition(index)->type;
	/* Authored ground and flying physics supported by the engine. */
	return type == 0 || type == 1 || type == 3 || type == 4 || type == 5;
}

void bots_vehicles_reset_player(long index)
{
	csmemset(&vehicles[index], 0, sizeof(vehicles[index]));
	vehicles[index].sought = vehicles[index].ridden = vehicles[index].avoided = NONE;
	vehicles[index].seat = NONE;
	vehicles[index].unit_index = NONE;
	vehicles[index].target_unit = NONE;
	enhanced_flight_reset(index);
}

void bots_vehicles_reset(void)
{
	long index;
	for (index = 0; index < BOTS_MAXIMUM; index++)
		bots_vehicles_reset_player(index);
}

long bots_vehicle_parent(long unit_index)
{
	struct object_datum *object = object_try_and_get(unit_index);
	long parent = object ? object->object.parent_object_index : NONE;
	return parent != NONE && (object_try_and_get_and_verify_type(parent, _object_mask_vehicle) || mcc_seat_parent(parent)) ? parent : NONE;
}

long bots_vehicle_navigation(long unit_index, real *radius)
{
	long parent = bots_vehicle_parent(unit_index);
	if (parent != NONE && vehicle_driver(parent) == unit_index && drivable(parent))
	{
		real width = definition(parent)->radius;
		*radius = width > 0.1f && width < 64.0f ? width : MAX(1.0f, MIN(object_get(parent)->object.bounding_sphere_radius, 64.0f));
		return parent;
	}
	return unit_index;
}

boolean bots_vehicle_flying(long unit_index)
{
	long parent = bots_vehicle_parent(unit_index);
	return parent != NONE && (definition(parent)->type == 3 || definition(parent)->type == 5);
}

boolean bots_vehicle_seeking(long index)
{
	return config_boolean("bots.vehicles") && vehicles[index].sought != NONE;
}

long bots_vehicle_sought(long index)
{
	return vehicles[index].sought;
}

void bots_vehicle_cancel(long index)
{
	struct bots_vehicle_state *state = &vehicles[index];
	if (state->sought != NONE)
	{
		state->avoided = state->sought;
		state->avoided_until = game_time_get() + 10 * TICKS_PER_SECOND;
	}
	state->sought = NONE;
	state->flipping = FALSE;
	state->next_scan = game_time_get() + TICKS_PER_SECOND;
}

long bots_vehicle_weapon(long unit_index, long weapon_index)
{
	long parent = bots_vehicle_parent(unit_index);
	struct unit_datum *vehicle;
	short slot;
	if (parent == NONE) return weapon_index;
	vehicle = unit_get(parent);
	slot = vehicle->unit.current_weapon_index;
	if ((vehicle->unit.gunner_object_index == unit_index ||
		(vehicle->unit.driver_object_index == unit_index && definition(parent)->type == 6)) &&
		slot >= 0 && slot < MAXIMUM_WEAPONS_PER_UNIT)
		return vehicle->unit.weapon_object_indices[slot];
	return weapon_index;
}

static short turret_seat(long vehicle_index);

static boolean seat_takeover(long other, long unit_index, long vehicle_index)
{
	struct object_datum *claimed = object_try_and_get(vehicles[other].unit_index);
	struct player_datum *player, *owner;
	if (!claimed || !game_engine_has_teams()) return FALSE;
	player = player_try_and_get(unit_get(unit_index)->unit.player_index);
	owner = player_try_and_get(unit_get(vehicles[other].unit_index)->unit.player_index);
	return player && owner && player->team_index == owner->team_index &&
		distance(&claimed->object.position, &object_get(vehicle_index)->object.position) >
		distance(&object_get(unit_index)->object.position, &object_get(vehicle_index)->object.position) + 15.0f;
}

static boolean seat_available(long index, long unit_index, long vehicle_index, short seat_index)
{
	struct unit_datum *vehicle = unit_get(vehicle_index);
	struct unit_definition *tag = unit_definition_get(vehicle->definition_index);
	struct unit_seat *seat;
	long other;
	if (seat_index < 0 || seat_index >= tag->unit.seats.count ||
		vehicle->object.up.k < 0.5f || TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit) ||
		TEST_FLAG(vehicle->unit.flags, _unit_not_enterable_by_player_bit)) return FALSE;
	seat = TAG_BLOCK_GET_ELEMENT(&tag->unit.seats, seat_index, struct unit_seat);
	if (TEST_FLAG(seat->flags, _unit_seat_driver_bit) && !drivable(vehicle_index) &&
		turret_seat(vehicle_index) != seat_index) return FALSE;
	if (!TEST_FLAG(seat->flags, _unit_seat_driver_bit) && !TEST_FLAG(seat->flags, _unit_seat_gunner_bit) &&
		vehicle_driver(vehicle_index) == NONE) return FALSE;
	/* A seat marked as requiring a driver follows the engine's player check. */
	if (TEST_FLAG(seat->flags, 9) && vehicle_driver(vehicle_index) == NONE) return FALSE;
	for (other = 0; other < BOTS_MAXIMUM; other++)
		if (other != index && vehicles[other].sought == vehicle_index && vehicles[other].seat == seat_index &&
			!seat_takeover(other, unit_index, vehicle_index))
			return FALSE;
	return unit_has_animation_to_enter_seat(unit_index, vehicle_index, seat_index) &&
		unit_can_enter_seat(unit_index, vehicle_index, seat_index, NULL);
}

static boolean friendly_driver(long unit_index, long vehicle_index)
{
	long driver = vehicle_driver(vehicle_index);
	struct player_datum *player, *other;
	if (driver == NONE || !game_engine_has_teams()) return FALSE;
	player = player_try_and_get(unit_get(unit_index)->unit.player_index);
	other = player_try_and_get(unit_get(driver)->unit.player_index);
	return player && other && player->team_index == other->team_index;
}

static boolean human_driver(long vehicle_index)
{
	long driver = vehicle_driver(vehicle_index);
	struct player_datum *player = driver != NONE ? player_try_and_get(unit_get(driver)->unit.player_index) : NULL;
	return player && !bots_player_owned(player->network_player_data.machine_index);
}

static boolean vehicle_free(long vehicle_index)
{
	long child = object_get(vehicle_index)->object.first_child_object_index;
	while (child != NONE)
	{
		struct object_datum *object = object_try_and_get(child);
		if (!object || TEST_FLAG(_object_mask_unit, object->object.type)) return FALSE;
		child = object->object.next_object_index;
	}
	return TRUE;
}

static short turret_seat(long vehicle_index)
{
	struct unit_definition *tag = unit_definition_get(object_get(vehicle_index)->definition_index);
	short seat, gunner = NONE, driver = NONE;
	for (seat = 0; seat < tag->unit.seats.count; seat++)
	{
		struct unit_seat *entry = TAG_BLOCK_GET_ELEMENT(&tag->unit.seats, seat, struct unit_seat);
		if (TEST_FLAG(entry->flags, _unit_seat_driver_bit) && driver == NONE) driver = seat;
		if (TEST_FLAG(entry->flags, _unit_seat_gunner_bit) && gunner == NONE) gunner = seat;
	}
	if (definition(vehicle_index)->type == 6) return gunner != NONE ? gunner : driver;
	return driver == NONE && definition(vehicle_index)->type != -1 ? gunner : NONE;
}

/* Tank, jeep, scout: reach, share of the journey, nearby exception,
allowable detour, and preference. The fork credits BirchWoodGod's drivers. */
static struct { real reach, share, close, slack, bonus; } const vehicle_worth[3] =
{
	{ 50.0f, 1.0f, 20.0f, 20.0f, 12.0f },
	{ 30.0f, 0.6f, 0.0f, 5.0f, 0.0f },
	{ 40.0f, 0.8f, 10.0f, 10.0f, 5.0f },
};

static boolean bots_vehicle_choose_goal(long index, long unit_index, real_point3d const *destination,
	real_point3d *goal, long *surface, boolean companion)
{
	struct bots_vehicle_state *state = &vehicles[index];
	struct object_datum *unit = object_get(unit_index);
	long now = game_time_get();
	if (!config_boolean("bots.vehicles") || bots_vehicle_parent(unit_index) != NONE)
	{
		state->sought = NONE;
		return FALSE;
	}
	if (infection_enabled() && infection_unit_is_zombie(unit_index)) return FALSE;
	if (state->unit_index != unit_index) state->spawn_tick = now;
	state->unit_index = unit_index;
	if (state->sought != NONE && state->flipping &&
		(object_try_and_get_and_verify_type(state->sought, _object_mask_vehicle) || mcc_seat_parent(state->sought)) && object_get(state->sought)->object.up.k >= 0.5f)
	{
		state->sought = NONE;
		state->flipping = FALSE;
		state->next_scan = now;
	}
	if (state->sought != NONE && (!(object_try_and_get_and_verify_type(state->sought, _object_mask_vehicle) || mcc_seat_parent(state->sought)) ||
		now >= state->deadline || (state->flipping ? !vehicle_free(state->sought) :
		!seat_available(index, unit_index, state->sought, state->seat))))
	{
		state->avoided = state->sought;
		state->avoided_until = now + TICKS_PER_SECOND * 30;
		state->sought = NONE;
		state->next_scan = now + TICKS_PER_SECOND * 10;
	}
	if (state->sought == NONE && now >= (companion ? state->next_companion_scan : state->next_scan))
	{
		struct object_iterator iterator;
		struct object_datum *vehicle;
		real best = REAL_MAX;
		real journey = distance(&unit->object.position, destination);
		if (companion) state->next_companion_scan = now + TICKS_PER_SECOND;
		else state->next_scan = now + TICKS_PER_SECOND * 2;
		object_iterator_new(&iterator, _object_mask_unit, 0);
		while ((vehicle = object_iterator_next(&iterator)) != NULL)
		{
			struct unit_definition *tag;
			short seat;
			real reach = distance(&unit->object.position, &vehicle->object.position);
			short turret, type, kind;
			real close;
			boolean friendly;
			if (iterator.index == unit_index || (!object_try_and_get_and_verify_type(iterator.index, _object_mask_vehicle) &&
				!mcc_seat_parent(iterator.index))) continue;
			turret = turret_seat(iterator.index);
			type = definition(iterator.index)->type;
			friendly = friendly_driver(unit_index, iterator.index);
			if (companion && (!friendly || !human_driver(iterator.index))) continue;
			kind = type == 0 ? 0 : type == 4 ? 2 : 1;
			close = vehicle_worth[kind].close;
			if ((iterator.index == state->avoided && now < state->avoided_until) ||
				TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit) ||
				TEST_FLAG(unit_get(iterator.index)->unit.flags, _unit_not_enterable_by_player_bit))
				continue;
			if (now - state->spawn_tick < 10 * TICKS_PER_SECOND) close = MAX(close + 10.0f, 15.0f);
			if (turret != NONE)
			{
				struct object_datum *target = object_try_and_get(state->target_unit);
				if (!target || reach > 12.0f || now - state->last_enemy_tick > 5 * TICKS_PER_SECOND ||
					distance(&target->object.position, &vehicle->object.position) > 60.0f) continue;
			}
			else if (!friendly && (!drivable(iterator.index) || journey < 35.0f || reach > MIN(vehicle_worth[kind].reach,
				MAX(close, journey * vehicle_worth[kind].share)) || (reach > close &&
				distance(&vehicle->object.position, destination) > journey + vehicle_worth[kind].slack))) continue;
			/* A large custom hull can have an entrance far from its origin. */
			if (friendly && reach > 25.0f + MAX(0.0f, vehicle->object.bounding_sphere_radius)) continue;
			if (vehicle->object.up.k < 0.5f)
			{
				long other;
				if (turret != NONE || reach > 15.0f || reach + 10.0f >= best || !vehicle_free(iterator.index)) continue;
				for (other = 0; other < BOTS_MAXIMUM; other++)
					if (other != index && vehicles[other].sought == iterator.index && vehicles[other].flipping) break;
				if (other < BOTS_MAXIMUM) continue;
				best = reach + 10.0f;
				state->sought = iterator.index;
				state->seat = NONE;
				state->flipping = TRUE;
				state->entrance = vehicle->object.position;
				continue;
			}
			tag = unit_definition_get(vehicle->definition_index);
			for (seat = 0; seat < tag->unit.seats.count; seat++)
			{
				real_point3d entrance, seat_point;
				struct unit_seat *entry = TAG_BLOCK_GET_ELEMENT(&tag->unit.seats, seat, struct unit_seat);
				boolean driver = TEST_FLAG(entry->flags, _unit_seat_driver_bit);
				real score = reach - (driver ? vehicle_worth[kind].bonus :
					TEST_FLAG(entry->flags, _unit_seat_gunner_bit) ? 3.5f : -3.0f);
				if (score >= best || (turret != NONE && seat != turret)) continue;
				if (turret == NONE && !driver && !friendly) continue;
				/* A human may creep forward while teammates approach. Do not
				chase a vehicle travelling faster than a running biped. */
				if (turret == NONE && magnitude3d(&vehicle->object.translational_velocity) > 0.08f) continue;
				if (seat_available(index, unit_index, iterator.index, seat) &&
					unit_get_seat_entrance_point(unit_index, iterator.index, seat, &entrance, &seat_point, NULL))
				{
					if (friendly && distance(&unit->object.position, &entrance) > 25.0f) continue;
					best = score;
					state->sought = iterator.index;
					state->seat = seat;
					state->entrance = entrance;
					state->flipping = FALSE;
				}
			}
		}
		if (state->sought != NONE)
		{
			long other;
			for (other = 0; other < BOTS_MAXIMUM; other++)
				if (other != index && vehicles[other].sought == state->sought && vehicles[other].seat == state->seat &&
					seat_takeover(other, unit_index, state->sought))
				{
					vehicles[other].sought = NONE;
					vehicles[other].next_scan = now + 3 * TICKS_PER_SECOND;
				}
			state->sought_tick = now;
			state->last_progress = now;
			state->best_distance = distance(&unit->object.position, &state->entrance);
			state->deadline = now + (long)(MAX(25.0f, distance(&unit->object.position, &state->entrance) / 1.6f + 8.0f) * TICKS_PER_SECOND);
		}
	}
	if (state->sought == NONE) return FALSE;
	if (!state->flipping)
	{
		real_point3d seat_point;
		if (!unit_get_seat_entrance_point(unit_index, state->sought, state->seat, &state->entrance, &seat_point, NULL)) return FALSE;
	}
	{
		real remaining = distance(&unit->object.position, &state->entrance);
		if (remaining < state->best_distance - 0.5f)
		{
			state->best_distance = remaining;
			state->last_progress = now;
		}
		if (now - state->last_progress > 8 * TICKS_PER_SECOND)
		{
			bots_vehicle_cancel(index);
			return FALSE;
		}
	}
	*surface = bots_routes_ground(&state->entrance, goal);
	return *surface != NONE;
}

boolean bots_vehicle_goal(long index, long unit_index, real_point3d const *destination,
	real_point3d *goal, long *surface)
{
	return bots_vehicle_choose_goal(index, unit_index, destination, goal, surface, FALSE);
}

/* Nearby human transports take precedence over an optional weapon detour.
Carriers and bots already holding a hill keep their objective responsibilities. */
boolean bots_vehicle_player_goal(long index, long unit_index, real_point3d const *destination,
	real_point3d *goal, long *surface)
{
	return bots_vehicle_choose_goal(index, unit_index, destination, goal, surface, TRUE);
}

static boolean boarding(long index, long vehicle_index)
{
	struct bots_vehicle_state *driver = &vehicles[index];
	long other, now = game_time_get();
	boolean waiting = FALSE;
	for (other = 0; other < BOTS_MAXIMUM; other++)
	{
		struct bots_vehicle_state *rider = &vehicles[other];
		struct object_datum *unit = object_try_and_get(rider->unit_index);
		if (other != index && unit && rider->sought == vehicle_index && !rider->flipping &&
			now - rider->sought_tick < 12 * TICKS_PER_SECOND && friendly_driver(rider->unit_index, vehicle_index) &&
			distance(&unit->object.position, &object_get(vehicle_index)->object.position) < 25.0f)
			waiting = TRUE;
	}
	if (!waiting) { driver->boarding_until = 0; return FALSE; }
	if (!driver->boarding_until) driver->boarding_until = now + 12 * TICKS_PER_SECOND;
	return now < driver->boarding_until;
}

static boolean turret_action(struct bots_vehicle_state *state, long vehicle_index, long target_index,
	struct player_action *action)
{
	long now = game_time_get();
	struct unit_datum *vehicle = unit_get(vehicle_index);
	struct object_datum *target = object_try_and_get(target_index);
	if (target)
	{
		real_vector3d toward;
		real length, aim;
		long weapon = bots_vehicle_weapon(state->unit_index, NONE);
		vector_from_points3d(&vehicle->object.bounding_sphere_center, &target->object.bounding_sphere_center, &toward);
		length = magnitude3d(&toward);
		aim = length > 0.01f ? dot_product3d(&toward, &vehicle->unit.aiming_vector) / length : 1.0f;
		if (aim < 0.90630779f)
		{
			if (!state->off_arc_tick) state->off_arc_tick = now;
			if (now - state->off_arc_tick > 2 * TICKS_PER_SECOND) return TRUE;
		}
		else state->off_arc_tick = 0;
		if (weapon != NONE)
		{
			struct weapon_definition *tag = weapon_definition_get(weapon_get(weapon)->definition_index);
			real charge = tag->weapon.triggers.count > 0 ?
				TAG_BLOCK_GET_ELEMENT(&tag->weapon.triggers, 0, struct weapon_trigger_definition)->charging_time : 0.0f;
			if (charge > 0.0f)
			{
				long ticks = (long)(MIN(charge, 5.0f) * TICKS_PER_SECOND + 0.5f);
				if (!state->charge_tick && action->primary_trigger > 0.0f) state->charge_tick = now;
				if (state->charge_tick)
				{
					action->primary_trigger = now - state->charge_tick <= ticks + 3 ? 1.0f : 0.0f;
					if (!action->primary_trigger) state->charge_tick = 0;
					SET_FLAG(action->control_flags, _unit_control_weapon_primary_trigger_bit, action->primary_trigger > 0.0f);
				}
			}
		}
	}
	else { state->off_arc_tick = 0; state->charge_tick = 0; }
	return now - MAX(state->ride_tick, state->last_enemy_tick) > 6 * TICKS_PER_SECOND;
}

void bots_vehicle_action(long index, long unit_index, real_point3d const *goal,
	boolean objective, boolean moving, real direction, long target_unit, boolean hurt, struct player_action *action)
{
	struct bots_vehicle_state *state = &vehicles[index];
	long parent = bots_vehicle_parent(unit_index), now = game_time_get();
	struct unit_datum *vehicle;
	boolean leave = FALSE, human_transport;
	if (state->unit_index != unit_index) state->spawn_tick = now;
	state->unit_index = unit_index;
	if (target_unit != NONE) { state->target_unit = target_unit; state->last_enemy_tick = now; }
	if (parent == NONE)
	{
		if (state->ridden != NONE)
		{
			platform_log("bots: bot %ld left vehicle %ld", index + 1, state->ridden);
			state->avoided = state->ridden;
			state->avoided_until = now + (enhanced_flight_failed(index) ? 120 : 30) * TICKS_PER_SECOND;
			enhanced_flight_reset(index);
			state->ridden = state->sought = NONE;
			state->next_scan = now + TICKS_PER_SECOND * 15;
		}
		if (config_boolean("bots.vehicles") && state->sought != NONE && now >= state->next_press &&
			(object_try_and_get_and_verify_type(state->sought, _object_mask_vehicle) || mcc_seat_parent(state->sought)))
		{
			short nearest;
			if (state->flipping)
			{
				struct object_datum *target = object_get(state->sought);
				real_point3d const *position = &object_get(unit_index)->object.position;
				if (distance(position, &target->object.position) < MAX(1.0f, target->object.bounding_sphere_radius * 0.8f))
				{
					action->throttle.i = action->throttle.j = 0.0f;
					action->desired_facing.yaw = (real)atan2(target->object.position.y - position->y, target->object.position.x - position->x);
					if (vehicle_free(state->sought)) action->control_flags |= FLAG(_unit_control_action_bit);
					state->next_press = now + TICKS_PER_SECOND / 2;
				}
				return;
			}
			/* Press Use only for the chosen seat; the normal player code
			checks reach, motion, hostility and occupancy again. */
			if (unit_find_nearby_seat(unit_index, state->sought, &nearest) == _unit_nearby_seat_available && nearest == state->seat)
			{
				action->throttle.i = action->throttle.j = 0.0f;
				action->control_flags |= FLAG(_unit_control_action_bit);
				state->next_press = now + TICKS_PER_SECOND / 2;
			}
			else if (distance(&object_get(unit_index)->object.position, &state->entrance) < 3.0f)
			{
				real_point3d *position = &object_get(unit_index)->object.position;
				real_point3d aim = state->entrance;
				real_point3d const *center = &object_get(state->sought)->object.position;
				real dx = aim.x - position->x, dy = aim.y - position->y;
				real vx = center->x - position->x, vy = center->y - position->y;
				real length = MAX(0.01f, (real)sqrt(dx * dx + dy * dy));
				real along = (dx * vx + dy * vy) / (length * length);
				real width = distance(center, &state->entrance);
				real toward, relative;
				if (length > 1.2f && along > 0.0f && along < 1.2f && fabs(dx * vy - dy * vx) / length < width * 0.9f)
				{
					real side = -dy * vx + dx * vy > 0.0f ? -1.0f : 1.0f;
					aim.x = center->x - dy / length * width * 1.1f * side;
					aim.y = center->y + dx / length * width * 1.1f * side;
				}
				toward = (real)atan2(aim.y - position->y, aim.x - position->x);
				relative = angle(toward - action->desired_facing.yaw);
				action->throttle.i = (real)cos(relative) * 0.5f;
				action->throttle.j = (real)sin(relative) * 0.5f;
			}
		}
		return;
	}
	vehicle = unit_get(parent);
	human_transport = vehicle_driver(parent) != unit_index && friendly_driver(unit_index, parent) && human_driver(parent);
	if (state->ridden != parent)
	{
		state->ridden = parent;
		state->sought = NONE;
		state->stuck_tick = now;
		state->ride_tick = now;
		state->boarding_until = state->driverless_tick = state->off_arc_tick = state->charge_tick = 0;
		state->last_position = vehicle->object.position;
		state->stuck_count = state->reverse_until = 0;
		platform_log("bots: bot %ld entered vehicle %ld seat %d type %d (%s)", index + 1, parent,
			unit_get(unit_index)->unit.parent_seat_index, definition(parent)->type, tag_get_name(vehicle->definition_index));
	}
	action->control_flags &= ~(FLAG(_unit_control_jump_bit) | FLAG(_unit_control_throw_grenade_bit) |
		FLAG(_unit_control_use_equipment_bit));
	action->throttle.i = action->throttle.j = 0.0f;
	if (bots_vehicle_flying(unit_index))
	{
		boolean landing = (objective && !human_transport) || !config_boolean("bots.vehicles");
		real dx = goal->x - vehicle->object.position.x, dy = goal->y - vehicle->object.position.y;
		real landing_radius = MAX(10.0f, vehicle->object.bounding_sphere_radius);
		if (vehicle_driver(parent) == unit_index)
		{
			real_point3d destination = config_boolean("bots.vehicles") ? *goal : vehicle->object.position;
			real aimed_yaw = action->desired_facing.yaw, aimed_pitch = action->desired_facing.pitch;
			real trigger = action->primary_trigger;
			unsigned long firing = action->control_flags & FLAG(_unit_control_weapon_primary_trigger_bit);
			enhanced_flight_action(index, parent, definition(parent)->type == 3, &destination, landing, boarding(index, parent), action);
			/* Retain an aimed shot only when flight steering still faces it. */
			if (target_unit != NONE && fabs(angle(action->desired_facing.yaw - aimed_yaw)) < 0.15f &&
				fabs(action->desired_facing.pitch - aimed_pitch) < 0.15f)
			{
				action->primary_trigger = trigger;
				action->control_flags |= firing;
			}
		}
		if ((landing || vehicle_driver(parent) == NONE || enhanced_flight_failed(index)) && enhanced_flight_can_exit(parent) &&
			(!config_boolean("bots.vehicles") || !objective || vehicle_driver(parent) == NONE || enhanced_flight_failed(index) || enhanced_flight_landing(index) ||
			 dx * dx + dy * dy < landing_radius * landing_radius) && now >= state->next_press)
		{
			action->control_flags |= FLAG(_unit_control_action_bit);
			state->next_press = now + TICKS_PER_SECOND / 2;
		}
		return;
	}
	if (vehicle_driver(parent) == unit_index && drivable(parent))
	{
		real heading = (real)atan2(vehicle->object.forward.j, vehicle->object.forward.i);
		real aimed_yaw = action->desired_facing.yaw, aimed_pitch = action->desired_facing.pitch;
		boolean wait = now - state->ride_tick < TICKS_PER_SECOND || boarding(index, parent);
		if (target_unit != NONE && !objective)
		{
			struct object_datum *target = object_try_and_get(target_unit);
			if (target)
			{
				real range = distance(&target->object.position, &vehicle->object.position);
				short type = definition(parent)->type;
				if (type == 0 || type == 4)
				{
					direction = aimed_yaw;
					moving = range > (type == 0 ? 20.0f : 12.0f);
				}
				else
				{
					direction = (real)atan2(target->object.position.y - vehicle->object.position.y,
						target->object.position.x - vehicle->object.position.x);
					if (vehicle->unit.gunner_object_index != NONE && range < 18.0f)
						direction += (index & 1 ? 1.0f : -1.0f) * (range < 10.0f ? 1.885f : 1.257f);
					moving = TRUE;
				}
			}
		}
		if (wait) moving = FALSE;
		if (now - state->stuck_tick >= TICKS_PER_SECOND)
		{
			if (moving && distance(&state->last_position, &vehicle->object.position) < 0.6f)
			{
				state->stuck_count++;
				if (state->stuck_count >= 2) bots_vehicle_obstacle(parent, goal);
				if (state->stuck_count % 2 == 0) state->reverse_until = now + TICKS_PER_SECOND;
			}
			else state->stuck_count = 0;
			state->last_position = vehicle->object.position;
			state->stuck_tick = now;
		}
		if (moving)
		{
			if (definition(parent)->type == 4 && target_unit != NONE)
			{
				/* The Ghost can keep aiming while translating along its route. */
				action->throttle.i = (real)cos(angle(direction - aimed_yaw));
				action->throttle.j = (real)sin(angle(direction - aimed_yaw));
			}
			else
			{
				action->desired_facing.yaw = direction;
				action->desired_facing.pitch = 0.0f;
				action->throttle.i = fabs(angle(direction - heading)) > 1.57f ? 0.6f : 1.0f;
			}
			if (now < state->reverse_until)
			{
				action->throttle.i = -0.6f;
				action->desired_facing.yaw = heading - PIN(angle(direction - heading), -1.0f, 1.0f);
			}
		}
		/* Changing the driver's aim must not turn a shot at an enemy into
		a shot down the route. Gunners retain their separate combat controls. */
		if (fabs(angle(action->desired_facing.yaw - aimed_yaw)) > 0.15f ||
			fabs(action->desired_facing.pitch - aimed_pitch) > 0.15f)
		{
			action->control_flags &= ~FLAG(_unit_control_weapon_primary_trigger_bit);
			action->primary_trigger = 0.0f;
		}
	}
	else if (turret_seat(parent) == unit_get(unit_index)->unit.parent_seat_index)
	{
		leave = turret_action(state, parent, target_unit, action);
		/* A human may be driving toward the fight; keep the gunner aboard. */
		leave = hurt || (leave && !human_transport);
	}
	else if (vehicle_driver(parent) == NONE)
	{
		if (!state->driverless_tick) state->driverless_tick = now;
		leave = now - state->driverless_tick > 3 * TICKS_PER_SECOND && now - state->last_enemy_tick > 2 * TICKS_PER_SECOND;
	}
	else state->driverless_tick = 0;
	if ((leave || !config_boolean("bots.vehicles") || vehicle->object.up.k < 0.3f || state->stuck_count >= 6 ||
		TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit) ||
		(objective && !human_transport && distance(goal, &vehicle->object.position) < 10.0f && turret_seat(parent) == NONE)) && now >= state->next_press)
	{
		action->control_flags |= FLAG(_unit_control_action_bit);
		state->next_press = now + TICKS_PER_SECOND / 2;
	}
}
