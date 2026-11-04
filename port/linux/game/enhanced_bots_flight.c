/* Enhanced aircraft navigation. Uses authored vehicle physics through ordinary
player controls. Geometry is queried on the host; no physics, map conversion
or replicated rules are changed. A bounded 3D search owns each flight route. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/vehicle_datum.h"
#include "units/unit_definitions.h"
#include "physics/physics_definitions.h"
#include "physics/collisions.h"
#include "bots.h"
#include "enhanced_bots_flight.h"
#include <math.h>

long config_integer(char const *name);
void platform_log(char const *format, ...);

enum
{
	FLIGHT_NODES = 1024,
	FLIGHT_STEPS = 32
};
enum
{
	_flight_takeoff,
	_flight_cruise,
	_flight_approach,
	_flight_land
};
struct flight_node
{
	real_point3d point;
	real cost, estimate;
	short parent;
	boolean closed;
};
static struct flight_state
{
	long vehicle, retry, progress_tick;
	short phase, count, expanded, step_count, step;
	boolean searching, failed, hovering, landing_ready, landing_checked;
	real_point3d destination, target, progress_target, landing;
	real radius, height, spacing, remaining, cruise_height, waypoint_remaining;
	struct flight_node nodes[FLIGHT_NODES];
	real_point3d steps[FLIGHT_STEPS];
} flights[BOTS_MAXIMUM];

static real flight_distance(real_point3d const *a, real_point3d const *b)
{
	real x = a->x - b->x, y = a->y - b->y, z = a->z - b->z;
	return (real)sqrt(x * x + y * y + z * z);
}

static unsigned long flight_collision_flags(void)
{
	return FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		   FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_scenery_bit) |
		   FLAG(_collision_test_objects_machines_bit) | FLAG(_collision_test_objects_vehicles_bit);
}

static struct physics_definition *flight_physics(long vehicle_index)
{
	struct unit_definition *unit = unit_definition_get(object_get(vehicle_index)->definition_index);
	struct physics_definition *physics;
	if (unit->object.physics.index == NONE) return NULL;
	physics = physics_definition_get(unit->object.physics.index);
	return physics->mass_points.count > 0 && physics->mass_points.count <= MAXIMUM_MASS_POINTS_PER_PHYSICS ? physics : NULL;
}

/* Fallback rays include the craft's sides and top/bottom, rather than only
its center. Horizontal clearance uses the object radius; vertical clearance
uses its physical thickness. Neither depends on a tag name. */
static boolean flight_sweep(long vehicle, real_point3d const *a, real_point3d const *b, real radius, real height, boolean touchdown)
{
	struct collision_result hit;
	real_vector3d delta;
	long axis, sign;
	struct object_datum *object = object_get(vehicle);
	struct physics_definition *physics = flight_physics(vehicle);
	vector_from_points3d(a, b, &delta);
	/* Match physics_update_new: test segments from the authored mass points.
	The legacy BSP capsule query does not honor object/front-face flags
	and can reject a craft's resting contacts. Ordinary collision vectors
	check both map and object geometry using the same contract as motion. */
	if (physics)
	{
		long index;
		real_vector3d left;
		left.i = object->object.up.j * object->object.forward.k - object->object.up.k * object->object.forward.j;
		left.j = object->object.up.k * object->object.forward.i - object->object.up.i * object->object.forward.k;
		left.k = object->object.up.i * object->object.forward.j - object->object.up.j * object->object.forward.i;
		for (index = 0; index < physics->mass_points.count; index++)
		{
			struct mass_point_definition *point = TAG_BLOCK_GET_ELEMENT(&physics->mass_points, index, struct mass_point_definition);
			real_point3d start, local;
			/* Physics uses unscaled mass points relative to center of mass,
			not model space or the render scale (physics_instance_new). */
			local.x = point->position.x - physics->center_of_mass.x;
			local.y = point->position.y - physics->center_of_mass.y;
			local.z = point->position.z - physics->center_of_mass.z;
			start.x = a->x + object->object.position.x - object->object.bounding_sphere_center.x +
				(object->object.forward.i * local.x + left.i * local.y + object->object.up.i * local.z);
			start.y = a->y + object->object.position.y - object->object.bounding_sphere_center.y +
				(object->object.forward.j * local.x + left.j * local.y + object->object.up.j * local.z);
			start.z = a->z + object->object.position.z - object->object.bounding_sphere_center.z +
				(object->object.forward.k * local.x + left.k * local.y + object->object.up.k * local.z);
			if (collision_test_vector(flight_collision_flags(), &start, &delta, vehicle, &hit) &&
				!(touchdown && delta.k < 0.0f && hit.plane.n.k > 0.7f)) return FALSE;
		}
		return TRUE;
	}
	if (collision_test_vector(flight_collision_flags(), a, &delta, vehicle, &hit) &&
		!(touchdown && delta.k < 0.0f && hit.plane.n.k > 0.7f))
		return FALSE;
	for (axis = 0; axis < 3; axis++)
		for (sign = -1; sign <= 1; sign += 2)
		{
			real_point3d start = *a;
			start.n[axis] += (axis == 2 ? height : radius) * sign;
			if (collision_test_vector(flight_collision_flags(), &start, &delta, vehicle, &hit) &&
				!(touchdown && delta.k < 0.0f && hit.plane.n.k > 0.7f))
				return FALSE;
		}
	return TRUE;
}

static boolean flight_clear(long vehicle, real_point3d const *a, real_point3d const *b, real radius, real height)
{
	return flight_sweep(vehicle, a, b, radius, height, FALSE);
}

/* A wide, flat aircraft does not need its wingspan in vertical clearance.
Use the authored collision mass points, projected through the current attitude.
Fallback to the object sphere when no usable physics shape is available. */
static real flight_height(long vehicle_index)
{
	struct object_datum *vehicle = object_get(vehicle_index);
	struct physics_definition *physics = flight_physics(vehicle_index);
	real height = 0.0f;
	real left_z = vehicle->object.up.i * vehicle->object.forward.j - vehicle->object.up.j * vehicle->object.forward.i;
	long index;
	if (!physics) return vehicle->object.bounding_sphere_radius;
	for (index = 0; index < physics->mass_points.count; index++)
	{
		struct mass_point_definition *point = TAG_BLOCK_GET_ELEMENT(&physics->mass_points, index, struct mass_point_definition);
		real z = vehicle->object.position.z - vehicle->object.bounding_sphere_center.z +
			vehicle->object.forward.k * (point->position.x - physics->center_of_mass.x) +
			left_z * (point->position.y - physics->center_of_mass.y) +
			vehicle->object.up.k * (point->position.z - physics->center_of_mass.z);
		height = MAX(height, (real)fabs(z) + MAX(0.0f, point->radius));
	}
	return MAX(height, 0.5f);
}

static boolean flight_ground(long vehicle, real_point3d const *point, real *height)
{
	struct collision_result hit;
	real_vector3d down = {0.0f, 0.0f, -128.0f};
	if (!collision_test_vector(flight_collision_flags(), point, &down, vehicle, &hit))
		return FALSE;
	*height = hit.point.z;
	return TRUE;
}

/* A long trip needs clearance over intervening terrain, not an immediate
straight descent toward the destination's floor. Nine bounded probes choose
cruise height; the local route still checks every movement segment. */
static real flight_cruise_height(long vehicle, real_point3d const *start, real_point3d const *goal, real height)
{
	struct collision_result hit;
	real_vector3d down = {0.0f, 0.0f, -8192.0f};
	real clearance = MAX(8.0f, height * 2.0f + 4.0f);
	real result = MAX(start->z, goal->z + clearance);
	long index;
	for (index = 0; index <= 8; index++)
	{
		real fraction = index / 8.0f;
		real_point3d probe;
		probe.x = start->x + (goal->x - start->x) * fraction;
		probe.y = start->y + (goal->y - start->y) * fraction;
		probe.z = MAX(start->z, goal->z) + 4096.0f;
		if (collision_test_vector(flight_collision_flags(), &probe, &down, vehicle, &hit))
			result = MAX(result, hit.point.z + clearance);
	}
	return result;
}

/* Objectives often sit inside bases. Pick an open descent column nearby,
then let the pedestrian controller cover the final distance. A roof between
cruise and the objective is not a reason to hover above it indefinitely. */
static boolean flight_landing_site(struct flight_state *state, real_point3d const *goal)
{
	long ring, direction;
	real spacing = MAX(8.0f, state->radius);
	for (ring = 0; ring <= 3; ring++)
		for (direction = 0; direction < (ring ? 8 : 1); direction++)
		{
			real angle = direction * 0.78539816f;
			real_point3d probe = *goal, above, landing;
			real ground;
			probe.x += (real)cos(angle) * spacing * ring;
			probe.y += (real)sin(angle) * spacing * ring;
			probe.z += 4.0f;
			if (!flight_ground(state->vehicle, &probe, &ground)) continue;
			landing = probe;
			landing.z = ground + state->height + 0.5f;
			above = landing;
			above.z = MAX(state->cruise_height, landing.z + 4.0f);
			if (!flight_clear(state->vehicle, &above, &landing, state->radius, state->height)) continue;
			state->landing = probe;
			state->landing.z = ground;
			return TRUE;
		}
	return FALSE;
}

void enhanced_flight_reset(long index)
{
	csmemset(&flights[index], 0, sizeof(flights[index]));
	flights[index].vehicle = NONE;
}

boolean enhanced_flight_failed(long index)
{
	return flights[index].failed;
}

boolean enhanced_flight_can_exit(long vehicle_index)
{
	struct vehicle_datum *vehicle = vehicle_datum_get(vehicle_index);
	real height;
	real_vector3d const *velocity = &vehicle->object.translational_velocity;
	if (vehicle->object.up.k <= 0.7f) return FALSE;
	if (vehicle->vehicle.on_ground_ticks >= 5 && magnitude3d(velocity) < 0.1f) return TRUE;
	/* Ordinary seat exit is safe just above the floor during a controlled
	approach. Requiring a fighter to stop completely in the air removes
	the thrust it needs to oppose gravity and forces an unnecessary impact. */
	return fabs(velocity->k) < 0.06f && velocity->i * velocity->i + velocity->j * velocity->j < 0.1225f &&
		flight_ground(vehicle_index, &vehicle->object.position, &height) &&
		vehicle->object.position.z - height < 2.0f;
}

boolean enhanced_flight_landing(long index)
{
	return flights[index].phase == _flight_land;
}

/* Forward-flight physics has no vertical crouch lift. Glide onto known
upward-facing ground, allowing touchdown contact but rejecting walls. Engine
physics still owns collision response; stop thrust as soon as contact settles. */
static void flight_descend(struct flight_state *state, real_point3d const *position, struct player_action *action)
{
	struct vehicle_datum *vehicle = vehicle_datum_get(state->vehicle);
	real heading = (real)atan2(vehicle->object.forward.j, vehicle->object.forward.i);
	real reach = MAX(2.0f, state->height * 2.0f);
	real ground, pitch, throttle;
	real clearance = REAL_MAX;
	long turn;
	if (state->hovering)
	{
		action->control_flags |= FLAG(_unit_control_crouch_modifier_bit);
		return;
	}
	if (vehicle->vehicle.on_ground_ticks)
	{
		action->throttle.i = magnitude3d(&vehicle->object.translational_velocity) > 0.03f ? -0.2f : 0.0f;
		return;
	}
	if (flight_ground(state->vehicle, &vehicle->object.position, &ground))
		clearance = vehicle->object.position.z - ground;
	{
		real desired_vertical = clearance < state->height + 10.0f ? -0.04f : -0.15f;
		real speed = MAX(0.1f, magnitude3d(&vehicle->object.translational_velocity));
		pitch = PIN((real)atan2(desired_vertical, speed) +
			(desired_vertical - vehicle->object.translational_velocity.k) * 2.0f, -0.2f, 0.65f);
		throttle = 0.6f;
		if (clearance < state->height + 25.0f)
		{
			struct physics_definition *physics = flight_physics(state->vehicle);
			/* The newer fighter model can approach at low forward speed.
			Keep a positive steering input: reverse can engage its authored
			brake flag, which zeros the velocity-correction force. */
			if (!physics || physics->radius <= 0.0f)
			{
				desired_vertical = -0.025f;
				/* Very low thrust also caps the force opposing gravity.
				Retain control authority while slowing the approach. */
				throttle = 0.3f;
				pitch = PIN((real)atan2(desired_vertical, speed) +
					(desired_vertical - vehicle->object.translational_velocity.k) * 4.0f, -0.1f, 0.65f);
			}
		}
		/* Forward-flight craft descend in a circuit around the landing
		point. Continuing straight would carry them far beyond the objective
		while losing altitude. The radial term keeps the circuit bounded. */
		{
			real_point3d const *site = state->landing_ready ? &state->landing : &state->destination;
			real dx = site->x - position->x;
			real dy = site->y - position->y;
			real range = (real)sqrt(dx * dx + dy * dy);
			real orbit = MAX(12.0f, state->radius * 2.0f);
			if (range > 0.1f)
				heading = (real)atan2(dy, dx) + 1.57079633f -
					PIN((range - orbit) / orbit, -1.0f, 1.0f) * 1.57079633f;
		}
	}
	for (turn = 0; turn < 4; turn++)
	{
		real yaw = heading + turn * 1.57079633f;
		real_point3d next = *position;
		next.x += (real)cos(yaw) * reach;
		next.y += (real)sin(yaw) * reach;
		next.z += reach * pitch;
		if (!flight_sweep(state->vehicle, position, &next, state->radius, state->height, TRUE)) continue;
		action->desired_facing.yaw = yaw;
		action->desired_facing.pitch = pitch;
		action->throttle.i = turn == 0 ? throttle : 0.0f;
		return;
	}
}

static void flight_begin(struct flight_state *state, real_point3d const *start, real_point3d const *target)
{
	state->target = *target;
	state->count = 1;
	state->expanded = 0;
	state->step = state->step_count = 0;
	state->searching = TRUE;
	state->nodes[0].point = *start;
	state->nodes[0].cost = 0.0f;
	state->nodes[0].estimate = flight_distance(start, target);
	state->nodes[0].parent = NONE;
	state->nodes[0].closed = FALSE;
}

/* Retain a validated prefix when a large map exceeds one search slice.
The next search starts where that prefix ends, rather than retrying forever
at the same place. Partial answers must reduce distance to the destination. */
static void flight_route(struct flight_state *state, long best, boolean complete)
{
	real_point3d reverse[FLIGHT_NODES];
	long count = 0, at = best, node;
	if (complete) reverse[count++] = state->target;
	while (at > 0 && count < FLIGHT_NODES)
	{
		reverse[count++] = state->nodes[at].point;
		at = state->nodes[at].parent;
	}
	state->step_count = (short)MIN(count, FLIGHT_STEPS);
	for (node = 0; node < state->step_count; node++)
		state->steps[node] = reverse[count - 1 - node];
	state->step = 0;
	state->searching = FALSE;
}

static boolean flight_segment(struct flight_state const *state, real_point3d const *a, real_point3d const *b)
{
	real dx = b->x - a->x, dy = b->y - a->y;
	/* A clear ray is not a flyable shortcut when it requires a forward-flight
	craft to climb vertically or exceed its commanded pitch. */
	if (!state->hovering && fabs(b->z - a->z) > 0.5f * sqrt(dx * dx + dy * dy))
		return FALSE;
	return flight_clear(state->vehicle, a, b, state->radius, state->height);
}

/* Each aircraft gets one slice every eight ticks, independent of player
iteration order. Full searches and unreachable destinations have a cooldown. */
static void flight_search(long index, struct flight_state *state)
{
	long work;
	if ((game_time_get() + index) % 8)
		return;
	for (work = 0; work < 4 && state->searching; work++)
	{
		long node, best = NONE, x, y, z;
		real score = REAL_MAX;
		struct flight_node *from;
		for (node = 0; node < state->count; node++)
			if (!state->nodes[node].closed && state->nodes[node].cost + state->nodes[node].estimate < score)
			{
				best = node;
				score = state->nodes[node].cost + state->nodes[node].estimate;
			}
		if (best == NONE || state->count >= FLIGHT_NODES || state->expanded >= 256)
		{
			long closest = 0;
			for (node = 1; node < state->count; node++)
				if (state->nodes[node].estimate < state->nodes[closest].estimate) closest = node;
			if (closest && state->nodes[closest].estimate + state->spacing * 0.5f < state->nodes[0].estimate)
				flight_route(state, closest, FALSE);
			else
			{
				state->searching = FALSE;
				state->retry = game_time_get() + TICKS_PER_SECOND * 3;
			}
			return;
		}
		from = &state->nodes[best];
		from->closed = TRUE;
		state->expanded++;
		if (flight_segment(state, &from->point, &state->target))
		{
			flight_route(state, best, TRUE);
			return;
		}
		/* A transport leaving a roofed hangar can climb as soon as its
		whole hull clears the roof, without filling the grid with tiny
		altitude steps across a very large map. */
		if (state->hovering && state->target.z > from->point.z + state->spacing)
		{
			real_point3d above = from->point;
			above.z = state->target.z;
			if (flight_segment(state, &from->point, &above) && flight_segment(state, &above, &state->target))
			{
				long climb = state->count++;
				state->nodes[climb].point = above;
				state->nodes[climb].parent = (short)best;
				flight_route(state, climb, TRUE);
				return;
			}
		}
		/* Sloped segments are flyable without requiring a fixed-wing craft
		to stop and climb straight upward. VTOL physics may also use vertical
		segments. Four expansions bound each slice of this larger graph. */
		for (x = -1; x <= 1 && state->count < FLIGHT_NODES; x++)
			for (y = -1; y <= 1 && state->count < FLIGHT_NODES; y++)
				for (z = -1; z <= 1 && state->count < FLIGHT_NODES; z++)
			{
				real_point3d next = from->point;
				real edge_cost;
				long existing;
				if ((!x && !y && !z) || (!x && !y && !state->hovering)) continue;
				next.x += x * state->spacing;
				next.y += y * state->spacing;
				next.z += z * state->spacing * 0.5f;
				edge_cost = flight_distance(&from->point, &next);
				for (existing = 0; existing < state->count; existing++)
					if (fabs(state->nodes[existing].point.x - next.x) < state->spacing * 0.25f &&
						fabs(state->nodes[existing].point.y - next.y) < state->spacing * 0.25f &&
						fabs(state->nodes[existing].point.z - next.z) < state->spacing * 0.25f)
						break;
				if (existing < state->count)
				{
					if (from->cost + edge_cost < state->nodes[existing].cost &&
						flight_segment(state, &from->point, &next))
					{
						state->nodes[existing].cost = from->cost + edge_cost;
						state->nodes[existing].parent = (short)best;
						state->nodes[existing].closed = FALSE;
					}
					continue;
				}
				if (!flight_segment(state, &from->point, &next)) continue;
				state->nodes[state->count].point = next;
				state->nodes[state->count].cost = from->cost + edge_cost;
				state->nodes[state->count].estimate = flight_distance(&next, &state->target);
				state->nodes[state->count].parent = (short)best;
				state->nodes[state->count].closed = FALSE;
				state->count++;
			}
	}
}

/* Human-plane neutral lift is slightly below gravity. Stabilize while a
search is pending instead of falling back onto the ground and restarting
takeoff before the search can finish. */
static void flight_hold(struct flight_state const *state, struct object_datum const *vehicle, struct player_action *action)
{
	if (!state->hovering) return;
	if (vehicle->object.translational_velocity.k < -0.005f)
		action->control_flags |= FLAG(_unit_control_jump_bit);
	else if (vehicle->object.translational_velocity.k > 0.02f)
		action->control_flags |= FLAG(_unit_control_crouch_modifier_bit);
}

void enhanced_flight_action(long index, long vehicle_index, boolean hovering, real_point3d const *goal, boolean land, boolean wait,
							struct player_action *action)
{
	struct flight_state *state = &flights[index];
	struct object_datum *vehicle = object_get(vehicle_index);
	real_point3d position = vehicle->object.bounding_sphere_center, target;
	real ground, dx, dy, horizontal, pitch, yaw, heading, offset, speed;
	long now = game_time_get();
	long trace = PIN(config_integer("debug.bot_trace"), 0, 3600) * TICKS_PER_SECOND;
	real_point3d const *waypoint = state->step < state->step_count ? &state->steps[state->step] : &state->target;
	if (trace && !(now % trace))
	{
		platform_log("bots: flight %ld phase %d search %d nodes %d expanded %d route %d/%d waypoint %.1f %.1f %.1f cruise %.1f remaining %.1f failed %d",
			index + 1, state->phase, state->searching, state->count, state->expanded,
			state->step, state->step_count, (double)waypoint->x, (double)waypoint->y, (double)waypoint->z, (double)state->cruise_height, (double)state->remaining, state->failed);
		platform_log("bots: flight motion position %.2f %.2f %.2f velocity %.3f %.3f %.3f up %.2f ground %d health %.2f",
			(double)vehicle->object.position.x, (double)vehicle->object.position.y, (double)vehicle->object.position.z,
			(double)vehicle->object.translational_velocity.i, (double)vehicle->object.translational_velocity.j,
			(double)vehicle->object.translational_velocity.k, (double)vehicle->object.up.k,
			vehicle_datum_get(vehicle_index)->vehicle.on_ground_ticks, (double)vehicle->object.body_vitality);
	}
	if (state->vehicle != vehicle_index)
	{
		enhanced_flight_reset(index);
		state->vehicle = vehicle_index;
		state->radius = MAX(vehicle->object.bounding_sphere_radius, 0.5f);
		/* Large hulls still need small steps to align with hangar exits. */
		state->spacing = MAX(4.0f, MIN(12.0f, state->radius * 0.5f + 2.0f));
		state->destination = *goal;
		state->landing_ready = state->landing_checked = FALSE;
		state->progress_tick = now;
		state->remaining = flight_distance(&position, goal);
		state->cruise_height = flight_cruise_height(vehicle_index, &position, goal, flight_height(vehicle_index));
	}
	state->hovering = hovering;
	state->height = flight_height(vehicle_index);
	/* The flight controller owns steering even while waiting for a route.
	Do not leave the combat controller's aim on an aircraft with no path. */
	heading = (real)atan2(vehicle->object.forward.j, vehicle->object.forward.i);
	action->desired_facing.yaw = heading;
	action->desired_facing.pitch = 0.0f;
	action->throttle.i = action->throttle.j = 0.0f;
	action->primary_trigger = 0.0f;
	action->control_flags &= ~(FLAG(_unit_control_weapon_primary_trigger_bit) | FLAG(_unit_control_jump_bit) |
							   FLAG(_unit_control_crouch_modifier_bit));
	if (wait && enhanced_flight_can_exit(vehicle_index))
		return;
	if (flight_distance(&state->destination, goal) > 6.0f)
	{
		state->destination = *goal;
		state->landing_ready = state->landing_checked = FALSE;
		state->progress_tick = now;
		state->remaining = flight_distance(&position, goal);
		state->searching = FALSE;
		state->step_count = 0;
		state->retry = 0;
		state->phase = _flight_cruise;
		state->cruise_height = flight_cruise_height(vehicle_index, &position, goal, state->height);
	}
	if (land && !state->landing_checked)
	{
		state->landing_checked = TRUE;
		state->landing_ready = flight_landing_site(state, goal);
		if (!state->landing_ready) state->failed = TRUE;
	}
	if (land && state->landing_ready) goal = &state->landing;
	target = *goal;
	target.z = state->cruise_height;
	/* A blocked aircraft must not hold its bot's objective indefinitely.
	Recover over known ground, then let ordinary exit input resume walking. */
	if (flight_distance(&position, goal) + 1.0f < state->remaining)
	{
		state->remaining = flight_distance(&position, goal);
		state->progress_tick = now;
	}
	if (now - state->progress_tick > 45 * TICKS_PER_SECOND && flight_ground(vehicle_index, &position, &ground))
		state->failed = TRUE;
	if (state->failed || (land && state->phase == _flight_land))
	{
		flight_descend(state, &position, action);
		return;
	}
	dx = goal->x - position.x;
	dy = goal->y - position.y;
	horizontal = (real)sqrt(dx * dx + dy * dy);
	speed = magnitude3d(&vehicle->object.translational_velocity);
	/* Brake before a fast custom craft reaches the destination, and keep
	the approach committed as it slows instead of oscillating into cruise. */
	if (land && (state->phase >= _flight_approach ||
		horizontal < MAX(MAX(MAX(10.0f, state->radius * 2.0f), hovering ? 0.0f : (position.z - goal->z) * 2.0f),
			speed * speed * 100.0f + speed * 20.0f)))
	{
		state->phase = _flight_approach;
		target = *goal;
		if (flight_ground(vehicle_index, goal, &ground)) target.z = ground;
		target.z += state->height + 0.5f;
		/* Hover aircraft align with the validated descent column first.
		A diagonal approach can intersect a nearby roof or hangar wall. */
		if (hovering) target.z = MAX(position.z, target.z);
		if (horizontal < MAX(2.0f, state->radius * 0.5f) ||
			(!hovering && horizontal < MAX(60.0f, state->radius * 2.0f) && position.z - target.z < 25.0f))
		{
			state->phase = _flight_land;
			flight_descend(state, &position, action);
			return;
		}
	}
	else if (flight_ground(vehicle_index, &position, &ground) && position.z - ground < state->height + 4.0f)
	{
		state->phase = _flight_takeoff;
		target = position;
		/* Turn toward the departure route, not into the wall a parked
		aircraft may face. Lift while turning before applying forward thrust. */
		target.x += (horizontal > 1.0f ? dx / horizontal : vehicle->object.forward.i) * state->spacing;
		target.y += (horizontal > 1.0f ? dy / horizontal : vehicle->object.forward.j) * state->spacing;
		target.z += state->spacing;
	}
	else
		state->phase = _flight_cruise;
	if (state->phase == _flight_cruise)
	{
		if (state->step < state->step_count &&
			flight_distance(&position, &state->steps[state->step]) < MAX(1.0f, MIN(3.0f, state->radius * 0.25f)))
			state->step++;
		if (state->step >= state->step_count && !state->searching && now >= state->retry)
			flight_begin(state, &position, &target);
		if (state->searching)
			flight_search(index, state);
		/* Look ahead through already validated route points, then recheck
		the shortcut against the current hull attitude. Heavy transports
		cannot stop and turn at every small grid cell. */
		if (state->step < state->step_count && !((now + index) % 8))
		{
			long look = MIN(state->step_count - 1, state->step + 8);
			for (; look > state->step; look--)
				if (flight_segment(state, &position, &state->steps[look]))
				{ state->step = (short)look; break; }
		}
		if (state->step < state->step_count)
			target = state->steps[state->step];
		else
		{
			flight_hold(state, vehicle, action);
			return;
		}
	}
	/* A climb or detour can increase distance to the final objective.
	Measure progress toward the current waypoint as well, so a heavy craft
	climbing out of a hangar is not declared stuck after 45 seconds. */
	if (flight_distance(&state->progress_target, &target) > 1.0f)
	{
		state->progress_target = target;
		state->waypoint_remaining = flight_distance(&position, &target);
	}
	else if (flight_distance(&position, &target) + 1.0f < state->waypoint_remaining)
	{
		state->waypoint_remaining = flight_distance(&position, &target);
		state->progress_tick = now;
	}
	if (!flight_clear(vehicle_index, &position, &target, state->radius, state->height))
	{
		state->step_count = 0;
		state->searching = FALSE;
		state->retry = now + TICKS_PER_SECOND;
		if (state->phase == _flight_cruise) flight_hold(state, vehicle, action);
		return;
	}
	dx = target.x - position.x;
	dy = target.y - position.y;
	horizontal = (real)sqrt(dx * dx + dy * dy);
	if (hovering && horizontal < MAX(1.0f, state->spacing * 0.25f) && fabs(target.z - position.z) > 1.0f)
	{
		/* Human-plane tags implement vertical lift through ordinary jump
		and crouch input; forward thrust cannot reach a vertical waypoint. */
		action->control_flags |= FLAG(target.z > position.z ? _unit_control_jump_bit : _unit_control_crouch_modifier_bit);
		return;
	}
	yaw = horizontal > 0.1f ? (real)atan2(dy, dx) : heading;
	pitch = (real)atan2(target.z - position.z, MAX(horizontal, 1.0f));
	offset = (real)atan2(sin(yaw - heading), cos(yaw - heading));
	/* Player actions carry an absolute desired heading. The authored
	vehicle physics limits the turn; a tiny heading increment would starve
	heavy aircraft of steering torque and make them circle or stall. */
	action->desired_facing.yaw = yaw;
	/* Human-plane lift follows the hull up vector. Pitching up at low
	thrust tips that lift backwards: a heavy transport slides away from its
	route. Keep hover craft level and control altitude with lift input. */
	action->desired_facing.pitch = hovering ? 0.0f : PIN(pitch, -0.5f, 0.65f);
	/* Both fighter models need forward input: the newer model also
	scales its velocity correction by throttle, so zero cannot arrest a
	fall even when the authored antigravity provides little support. */
	if (!state->hovering)
	{
		real forward_speed = MAX(0.1f, speed);
		real desired_vertical = PIN((target.z - position.z) * 0.03f, -0.15f, 0.25f);
		action->desired_facing.pitch = PIN((real)atan2(desired_vertical, forward_speed) +
			(desired_vertical - vehicle->object.translational_velocity.k) * 2.0f, -0.35f, 0.65f);
	}
	if (hovering && target.z > position.z + 1.0f)
		action->control_flags |= FLAG(_unit_control_jump_bit);
	else if (hovering && target.z < position.z - 1.0f)
		action->control_flags |= FLAG(_unit_control_crouch_modifier_bit);
	if (!hovering)
		/* Forward-flight craft need airspeed to take off as well as cruise. */
		action->throttle.i = state->phase == _flight_approach ? 0.6f :
			state->phase == _flight_takeoff && fabs(offset) > 0.8f ? 0.0f : 0.8f;
	else if (state->phase == _flight_takeoff)
		action->throttle.i = fabs(offset) > 0.8f ? 0.0f : 0.35f;
	else if (state->phase == _flight_approach)
		action->throttle.i = speed > 0.08f ? 0.0f : 0.2f;
	else if (fabs(offset) <= 0.8f)
		action->throttle.i = horizontal < state->spacing ? 0.35f : 0.8f;
}
