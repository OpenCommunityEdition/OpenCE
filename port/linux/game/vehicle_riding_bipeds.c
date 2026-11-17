/*
VEHICLE_RIDING_BIPEDS.C

The engine-facing riding hooks, kept separate from the decompiled biped physics.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "models/model_definitions.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "vehicle_riding.h" /* port/linux/game/vehicle_riding.c */
#include "physics/collision_bsp.h"
#include "physics/collision_features.h"
#include "physics/collision_models.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "physics/physics.h"
#include "scenario/scenario.h"
#include "network_coop.h" /* port/linux/game/network_coop.c */
#include "vehicle_riding_bipeds.h"
#include "saved games/game_state.h"

/* port/linux/game/network_objects.c's (a vehicle a client puts where
the host has it, with no physics of its own) */
boolean network_objects_vehicle_followed(long vehicle_index);

/* ---------- vehicle riding (vehicle_riding.c) */

/* the biped's riding fields as the riding code's state */
/* the transient mark uses the same player key as the air frame */
short biped_riding_player(struct biped_datum const *biped);

void biped_riding_get(
	struct biped_datum const *biped,
	struct vehicle_riding_state *state)
{
	struct vehicle_riding_mark *mark = vehicle_riding_player_mark(biped_riding_player(biped));

	/* (non-player bipeds have no transient snapshot) */
	state->mark.role = _vehicle_riding_role_simulated;
	state->mark.time = 0;
	vehicle_riding_correction_identity(&state->mark.correction, NONE);
	if (mark)
		state->mark = *mark;
	state->ticks = biped->biped.riding_ticks;
	state->falling = biped->biped.riding_falling;
	state->object_index = biped->biped.riding_object_index;
	state->vehicle_updates = biped->biped.riding_vehicle_updates;
	state->vehicle_position = biped->biped.riding_vehicle_position;
	state->vehicle_forward = biped->biped.riding_vehicle_forward;
	state->vehicle_up = biped->biped.riding_vehicle_up;
	state->velocity = biped->biped.riding_velocity;
	state->local_point = biped->biped.riding_local_point;
	state->node_index = biped->biped.riding_node_index;
	state->body_position = biped->biped.riding_body_position;
	state->body_forward = biped->biped.riding_body_forward;
	state->body_up = biped->biped.riding_body_up;
}

static void biped_riding_set(
	struct biped_datum *biped,
	struct vehicle_riding_state const *state)
{
	struct vehicle_riding_mark *mark = vehicle_riding_player_mark(biped_riding_player(biped));

	/* (the mark is transient, never a biped field or saved state) */
	if (mark)
		*mark = state->mark;
	biped->biped.riding_ticks = state->ticks;
	biped->biped.riding_falling = state->falling;
	biped->biped.riding_object_index = state->object_index;
	biped->biped.riding_vehicle_updates = state->vehicle_updates;
	biped->biped.riding_vehicle_position = state->vehicle_position;
	biped->biped.riding_vehicle_forward = state->vehicle_forward;
	biped->biped.riding_vehicle_up = state->vehicle_up;
	biped->biped.riding_velocity = state->velocity;
	biped->biped.riding_local_point = state->local_point;
	biped->biped.riding_node_index = state->node_index;
	biped->biped.riding_body_position = state->body_position;
	biped->biped.riding_body_forward = state->body_forward;
	biped->biped.riding_body_up = state->body_up;
	biped->biped.riding_pad = 0;
}

void biped_riding_clear(
	struct biped_datum *biped)
{
	struct vehicle_riding_state state;

	vehicle_riding_clear(&state);
	biped_riding_set(biped, &state);
}

void bipeds_riding_latch(
	void)
{
	struct object_iterator iterator;
	struct biped_datum *biped;

	if (vehicle_riding_latch())
		return;
	object_iterator_new(&iterator, _object_mask_biped, 0);
	while ((biped = (struct biped_datum *)object_iterator_next(&iterator)) != NULL)
	{
		biped_riding_clear(biped);
	}
}

/* (vehicle_riding.c) */
static boolean biped_riding_applies(
	struct biped_datum const *biped)
{
	return vehicle_riding_applies(
		biped->unit.player_index != NONE,
		TEST_FLAG(biped->object.damage_flags, _object_dead_bit));
}

/* a biped's slot in the riding code's per-player tables
(vehicle_riding_player_air): its player's absolute index, the same on every
machine, NONE for a biped with no player */
short biped_riding_player(
	struct biped_datum const *biped)
{
	return biped->unit.player_index != NONE ? (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(biped->unit.player_index) : NONE;
}

/* whether three parts of a point or vector are the +0 a cleared state stores, to the bit */
static boolean biped_riding_zero3(
	real a,
	real b,
	real c)
{
	return vehicle_riding_real_same(a, 0.f) && vehicle_riding_real_same(b, 0.f) && vehicle_riding_real_same(c, 0.f);
}

/* the riding fields as biped_riding_set stores a cleared state's */
static boolean biped_riding_fields_clear(
	struct biped_datum const *biped)
{
	return biped->biped.riding_ticks == 0 && biped->biped.riding_falling == 0 &&
		biped->biped.riding_object_index == NONE && biped->biped.riding_vehicle_updates == 0 &&
		biped_riding_zero3(biped->biped.riding_vehicle_position.x, biped->biped.riding_vehicle_position.y,
			biped->biped.riding_vehicle_position.z) &&
		biped_riding_zero3(biped->biped.riding_vehicle_forward.i, biped->biped.riding_vehicle_forward.j,
			biped->biped.riding_vehicle_forward.k) &&
		biped_riding_zero3(biped->biped.riding_vehicle_up.i, biped->biped.riding_vehicle_up.j,
			biped->biped.riding_vehicle_up.k) &&
		biped_riding_zero3(biped->biped.riding_velocity.i, biped->biped.riding_velocity.j,
			biped->biped.riding_velocity.k) &&
		biped_riding_zero3(biped->biped.riding_local_point.x, biped->biped.riding_local_point.y,
			biped->biped.riding_local_point.z) &&
		biped->biped.riding_node_index == NONE &&
		biped_riding_zero3(biped->biped.riding_body_position.x, biped->biped.riding_body_position.y,
			biped->biped.riding_body_position.z) &&
		biped_riding_zero3(biped->biped.riding_body_forward.i, biped->biped.riding_body_forward.j,
			biped->biped.riding_body_forward.k) &&
		biped_riding_zero3(biped->biped.riding_body_up.i, biped->biped.riding_body_up.j,
			biped->biped.riding_body_up.k) &&
		biped->biped.riding_pad == 0;
}

/* whether the riding hooks of this tick would write nothing new for the biped: riding cannot apply to it, and what
they clear is clear (its fields, its player's tables, its kept turn). The host's claim is not one: they only read it */
static boolean biped_riding_idle(
	struct biped_datum const *biped,
	short local_player_index)
{
	return !biped_riding_applies(biped) && biped_riding_fields_clear(biped) &&
		vehicle_riding_player_idle(biped_riding_player(biped)) &&
		vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f);
}

/* a vehicle as the riding code sees it, posed as its node (NONE: as
itself); FALSE: none, or not a vehicle */
boolean biped_riding_vehicle(
	long vehicle_index,
	short node_index,
	struct vehicle_riding_vehicle *vehicle)
{
	struct unit_datum *unit;

	if (vehicle_index == NONE)
		return FALSE;
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle);
	if (!unit)
		return FALSE;
	vehicle->object_index = vehicle_index;
	vehicle->node_index = node_index;
	object_get_origin(vehicle_index, &vehicle->origin);
	object_get_orientation(vehicle_index, &vehicle->body_forward, &vehicle->body_up);
	if (node_index != NONE)
	{
		/* (the node's matrix as of the vehicle's last update: a turret its
		gunner turned is posed where it turned) */
		real_matrix4x3 const *matrix = object_get_node_matrix(vehicle_index, node_index);

		vehicle->position = matrix->position;
		vehicle->forward = matrix->forward;
		vehicle->up = matrix->up;
	}
	else
	{
		vehicle->position = vehicle->origin;
		vehicle->forward = vehicle->body_forward;
		vehicle->up = vehicle->body_up;
	}
	vehicle->updates = object_update_count(vehicle_index);
	vehicle->translational_velocity = unit->object.translational_velocity;
	vehicle->angular_velocity = unit->object.angular_velocity;
	vehicle->bounding_sphere_center = unit->object.bounding_sphere_center;
	vehicle->bounding_sphere_radius = unit->object.bounding_sphere_radius;
	vehicle->has_player = unit->unit.player_index != NONE;
	/* (only the rider's pass can keep a host-chosen node) */
	vehicle->node_kept = FALSE;
	vehicle->role = network_objects_vehicle_followed(vehicle_index) ?
		_vehicle_riding_role_followed : _vehicle_riding_role_simulated;
	vehicle->time = game_time_get();
	vehicle->correction = *vehicle_riding_vehicle_correction(vehicle_index);
	return TRUE;
}

/* the deck part of a player's unit for the wire, in the selected snapshot
frame (vehicle_riding_deck_frame, vehicle_riding_deck_make): FALSE in no window, in a seat, or its vehicle
gone */
boolean biped_riding_deck(
	long biped_index,
	struct vehicle_riding_deck *deck)
{
	struct biped_datum *biped = biped_try_and_get(biped_index);
	struct vehicle_riding_state riding;
	struct vehicle_riding_vehicle vehicle;
	struct vehicle_riding_vehicle frame;
	real_point3d base;
	real height;
	real width;

	if (!biped || !biped_riding_applies(biped) || biped->object.parent_object_index != NONE)
		return FALSE;
	biped_riding_get(biped, &riding);
	if (riding.ticks <= 0 || !biped_riding_vehicle(riding.object_index, riding.node_index, &vehicle))
		return FALSE;
	biped_get_physics_pill(biped_index, &base, &height, &width);
	vehicle_riding_deck_frame(&riding, &vehicle, &frame);
	return vehicle_riding_deck_make(&riding, &frame, &base, &biped->object.translational_velocity,
		vehicle_riding_player_air(biped_riding_player(biped)), vehicle_riding_player_cabin(biped_riding_player(biped)),
		deck);
}

/* the unit's own window for deck-frame network commits */
boolean biped_riding_get_state(
	long biped_index,
	struct vehicle_riding_state *state)
{
	struct biped_datum *biped = biped_try_and_get(biped_index);

	if (!biped || !biped_riding_applies(biped) || biped->object.parent_object_index != NONE)
		return FALSE;
	biped_riding_get(biped, state);
	return TRUE;
}

/* end the unit's riding state without a network-supplied frame */
void biped_riding_drop(
	long biped_index)
{
	struct biped_datum *biped = biped_try_and_get(biped_index);
	struct vehicle_riding_state state;

	if (!biped)
		return;
	biped_riding_get(biped, &state);
	vehicle_riding_drop(&state);
	biped_riding_set(biped, &state);
}

/* retake this machine's vehicle pose at the unit's current pill base */
boolean biped_riding_rebase(
	long biped_index)
{
	struct vehicle_riding_state state;
	struct vehicle_riding_vehicle vehicle;
	real_point3d base;
	real height;
	real width;

	if (!biped_riding_get_state(biped_index, &state) || state.ticks <= 0 ||
		!biped_riding_vehicle(state.object_index, state.node_index, &vehicle))
		return FALSE;
	biped_get_physics_pill(biped_index, &base, &height, &width);
	if (!vehicle_riding_rebase(&state, &vehicle, &base))
		return FALSE;
	biped_riding_set(biped_get(biped_index), &state);
	return TRUE;
}

/* the object position whose pill base is the deck part's local place */
boolean biped_riding_deck_position(
	long biped_index,
	struct vehicle_riding_deck const *deck,
	real_point3d *position)
{
	struct biped_datum *biped = biped_try_and_get(biped_index);
	struct vehicle_riding_vehicle vehicle;
	real_point3d base;
	real height;
	real width;
	short node_count = biped_riding_vehicle_node_count(deck->vehicle_index);

	if (!biped || node_count == NONE || (deck->node_index != NONE &&
		(deck->node_index < 0 || deck->node_index >= node_count)) ||
		!biped_riding_vehicle(deck->vehicle_index, deck->node_index, &vehicle))
		return FALSE;
	biped_get_physics_pill(biped_index, &base, &height, &width);
	vehicle_riding_point_to_world(&vehicle.position, &vehicle.forward, &vehicle.up, &deck->place, position);
	/* (the wire's place is the pill base, not the object's position) */
	position->x -= base.x - biped->object.position.x;
	position->y -= base.y - biped->object.position.y;
	position->z -= base.z - biped->object.position.z;
	return TRUE;
}

/* a remote window anchored where this machine's correction left the pill */
boolean biped_riding_take_deck(
	long biped_index,
	struct vehicle_riding_deck const *deck)
{
	struct vehicle_riding_state state;
	struct vehicle_riding_vehicle vehicle;
	real_point3d base;
	real height;
	real width;

	if (!biped_riding_get_state(biped_index, &state) ||
		!biped_riding_vehicle(deck->vehicle_index, deck->node_index, &vehicle))
		return FALSE;
	biped_get_physics_pill(biped_index, &base, &height, &width);
	if (!vehicle_riding_take_deck(&state, &vehicle, deck, &base))
		return FALSE;
	biped_riding_set(biped_get(biped_index), &state);
	vehicle_riding_air_take(vehicle_riding_player_air(biped_riding_player(biped_get(biped_index))), deck);
	return TRUE;
}

/* an own air correction moves the frame and the world velocity together */
boolean biped_riding_air_correct(
	long biped_index,
	real_vector3d const *difference,
	real peak,
	short verdict)
{
	struct vehicle_riding_state state;
	struct biped_datum *biped;

	if (!biped_riding_get_state(biped_index, &state))
		return FALSE;
	biped = biped_get(biped_index);
	vehicle_riding_air_correct(&state, vehicle_riding_player_air(biped_riding_player(biped)), difference,
		peak, verdict, &biped->object.translational_velocity);
	biped_riding_set(biped, &state);
	return TRUE;
}

/* without a matching air echo there is no historical difference to keep */
boolean biped_riding_air_take(
	long biped_index,
	struct vehicle_riding_deck const *deck)
{
	struct vehicle_riding_state state;
	real_vector3d difference;

	if (!biped_riding_get_state(biped_index, &state))
		return FALSE;
	subtract_vectors3d(&deck->velocity, &state.velocity, &difference);
	return biped_riding_air_correct(biped_index, &difference, deck->peak,
		(short)((deck->flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK));
}

/* a current legacy verdict holds without a positional correction or a local resample */
boolean biped_riding_air_verdict_host(
	long biped_index,
	struct vehicle_riding_deck const *deck,
	boolean current_episode)
{
	struct vehicle_riding_state state;

	if (!deck || !biped_riding_get_state(biped_index, &state) || state.ticks <= 0 ||
		state.object_index != deck->vehicle_index)
		return FALSE;
	return vehicle_riding_air_take_verdict(vehicle_riding_player_air(biped_riding_player(biped_get(biped_index))),
		deck, current_episode);
}

/* the cabin verdict of a host's deck part taken for a unit's player (deck NULL: its part
absent or not whole, so none); current_window: the word is of the window the unit is in now */
void biped_riding_cabin_host(
	long biped_index,
	struct vehicle_riding_deck const *deck,
	boolean current_window)
{
	struct vehicle_riding_state state;
	struct vehicle_riding_cabin *cabin;

	if (!biped_riding_get_state(biped_index, &state))
		return;
	cabin = vehicle_riding_player_cabin(biped_riding_player(biped_get(biped_index)));
	if (!cabin)
		return;
	if (!deck)
		vehicle_riding_cabin_drop_host(cabin);
	else
		vehicle_riding_cabin_take_host(cabin, biped_index, &state, deck->vehicle_index,
			(short)((deck->flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK),
			deck->ticks == RIDING_TICKS ? _vehicle_riding_cabin_sample_standing : _vehicle_riding_cabin_sample_jump,
			TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit), current_window);
}

/* the frame a take will keep, before the correction writes the unit */
boolean biped_riding_frame_velocity(
	long biped_index,
	struct vehicle_riding_deck const *deck,
	real_point3d const *base,
	real_vector3d *velocity)
{
	struct vehicle_riding_state state;
	struct vehicle_riding_vehicle vehicle;

	if (!biped_riding_get_state(biped_index, &state) ||
		!biped_riding_vehicle(deck->vehicle_index, deck->node_index, &vehicle))
		return FALSE;
	if (state.ticks > 0 && state.object_index == deck->vehicle_index && state.node_index == deck->node_index)
		*velocity = state.velocity;
	else
		vehicle_riding_deck_point_velocity(&vehicle, base, game_players_are_double_speed(), velocity);
	return TRUE;
}

/* how many nodes a vehicle a deck part names has
(vehicle_riding_deck_valid): no more than it has matrices for, as
object_has_node allows them (with no model, its first); NONE for no such
vehicle (a slot deleted or reused, or not a vehicle) */
short biped_riding_vehicle_node_count(
	long vehicle_index)
{
	struct object_datum *object;
	long model_index;
	short count;

	/* (an identifier of 0 is looked at first: datum_try_and_get takes any datum in the slot by it) */
	if (!vehicle_riding_deck_vehicle_named(vehicle_index))
		return NONE;
	object = (struct object_datum *)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle);
	if (!object)
		return NONE;
	count = (short)(object->object.node_matrices.size / (long)sizeof(real_matrix4x3));
	model_index = object_definition_get(object->definition_index)->object.model.index;
	if (model_index == NONE)
		return (short)MIN(count, 1);
	return (short)MIN(count, model_definition_get(model_index)->nodes.count);
}

/* the flags biped_update_physics sweeps a living biped with (the in flags
biped_update_moving sets), for the carry */
static unsigned long biped_riding_collision_flags(
	struct biped_datum const *biped,
	struct biped_definition const *definition)
{
	unsigned long flags = TEST_FLAG(definition->biped.flags, _biped_passes_through_bipeds_bit) ?
		_collision_test_for_bipeds_passthrough_living_flags :
		_collision_test_for_bipeds_living_flags;

	if (biped->unit.player_index != NONE && !network_coop_player_collisions())
		flags |= FLAG(_collision_test_skip_player_bipeds_bit);
	return flags;
}

/* the pill base carried by the deck's motion */
static void biped_riding_carry(
	long biped_index,
	struct biped_datum const *biped,
	struct biped_definition const *definition,
	long vehicle_index,
	real_vector3d const *displacement,
	real height,
	real width,
	real_point3d *position)
{
	if (TEST_FLAG(biped->biped.flags, _biped_no_collision_bit))
	{
		position->x += displacement->i;
		position->y += displacement->j;
		position->z += displacement->k;
		return;
	}
	collision_move_pill_carry(
		biped_riding_collision_flags(biped, definition),
		position,
		displacement,
		height,
		width,
		biped_index,
		vehicle_index,
		position);
}

/* the biped's facing, aim and look turned by its vehicle's yaw, so this
tick's movement follows the turned facing */
static void biped_riding_turn(
	struct biped_datum *biped,
	struct biped_physics *physics,
	real dyaw)
{
	real_vector3d *const vectors[8] = {
		&biped->object.forward, &biped->unit.desired_facing_vector, &biped->unit.desired_aiming_vector,
		&biped->unit.aiming_vector, &biped->unit.desired_looking_vector, &biped->unit.looking_vector,
		&physics->forward, &physics->aiming };

	vehicle_riding_turn_vectors(vectors, 8, dyaw);
}

/* the node of a vehicle's collision model under a contact: a short test
from the pill's base, where the sweep reports it, down through the surface
it stands on (as the melee test reads a hit's node); NONE when it finds
nothing */
static short biped_riding_support_node(
	long vehicle_index,
	struct collision_plane const *collision,
	real width)
{
	struct collision_model_instance instance;
	struct collision_model_test_vector_result result;
	real_point3d start;
	real_vector3d down;

	start.x = collision->point.x + collision->plane.n.i*0.01f;
	start.y = collision->point.y + collision->plane.n.j*0.01f;
	start.z = collision->point.z + collision->plane.n.k*0.01f;
	down.i = -collision->plane.n.i*(width*2.f + 0.05f);
	down.j = -collision->plane.n.j*(width*2.f + 0.05f);
	down.k = -collision->plane.n.k*(width*2.f + 0.05f);
	if (collision_model_instance_new(&instance, vehicle_index) &&
		collision_model_test_vector(
			&instance,
			FLAG(_collision_test_front_facing_surfaces_bit),
			&start,
			&down,
			&result))
	{
		return result.node_index;
	}
	return NONE;
}

/* whether the biped is over the deck of the vehicle it left: a ray
straight down from its pill's base against that vehicle alone, as far as its
own jump takes it plus a pill height (vehicle_riding_over_deck) */
static boolean biped_riding_over_deck(
	struct biped_definition const *definition,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real height)
{
	struct collision_model_instance instance;
	struct collision_model_test_vector_result result;
	real_vector3d down;
	real reach = vehicle_riding_reach(definition->biped.jump_velocity, global_gravity, height);
	boolean hit;

	down.i = 0.f;
	down.j = 0.f;
	down.k = -reach;
	hit = collision_model_instance_new(&instance, vehicle->object_index) &&
		collision_model_test_vector(
			&instance,
			FLAG(_collision_test_front_facing_surfaces_bit),
			base,
			&down,
			&result);
	/* (the hit's t is the part of the ray it took) */
	return vehicle_riding_over_deck(hit, hit ? result.bsp_result.t*reach : 0.f, reach, &vehicle->body_up);
}

/* whether the vehicle the biped left encloses it: a ray straight up
from its pill's top against that vehicle alone, both faces, as far as its
own jump rises, or the top already inside it (vehicle_riding_air_enclosure) */
static void biped_riding_enclosure(
	struct biped_definition const *definition,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real height,
	struct vehicle_riding_air *air)
{
	struct collision_model_instance instance;
	struct collision_model_test_vector_result result;
	real_point3d top;
	real_vector3d upward;
	real jump_height = vehicle_riding_reach(definition->biped.jump_velocity, global_gravity, 0.f);
	boolean inside = FALSE;
	boolean hit = FALSE;

	top = *base;
	top.z += height;
	upward.i = 0.f;
	upward.j = 0.f;
	upward.k = jump_height;
	if (collision_model_instance_new(&instance, vehicle->object_index))
	{
		inside = collision_model_test_point(&instance, &top);
		/* (no gravity, no jump to rise by: the inside test alone) */
		hit = jump_height > 0.f &&
			collision_model_test_vector(
				&instance,
				FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit),
				&top,
				&upward,
				&result);
	}
	/* (the hit's t is the part of the ray it took) */
	vehicle_riding_air_enclosure(air, hit, hit ? result.bsp_result.t*jump_height : 0.f, jump_height, inside);
}

/* whether a floor the cabin's ray found faces along the axis (the
vehicle's own floor under the pill, never a wall or a ceiling's top; the
node's normal as ride_deck_surface reads it) */
static boolean biped_riding_cabin_floor(
	struct collision_model_instance const *instance,
	struct collision_model_test_vector_result const *result,
	real_vector3d const *axis)
{
	real_vector3d normal;

	if (result->node_index < 0 || result->node_index >= instance->node_count || !result->bsp_result.plane)
		return FALSE;
	matrix4x3_transform_normal(&instance->matrices[result->node_index], &result->bsp_result.plane->n, &normal);
	return dot_product3d(&normal, axis) > 0.f;
}

/* the two-sided cabin test at a pill against the vehicle alone
(vehicle_riding_cabin_hit): along the axis from the end nearer the ceiling,
the inside test and a ray a jump long, both faces; against it from the
other end, a ray a jump and a pill long for the vehicle's own front-facing
floor. TRUE: enclosed */
static boolean biped_riding_cabin_rays(
	long vehicle_index,
	real_vector3d const *axis,
	real_point3d const *base,
	real height,
	real jump_height,
	struct vehicle_riding_cabin_rays *rays)
{
	struct collision_model_instance instance;
	struct collision_model_test_vector_result result;
	real_point3d ceiling_start;
	real_point3d floor_start;
	real_vector3d ray;
	real reach = jump_height + height;

	csmemset(rays, 0, sizeof(*rays));
	if (!collision_model_instance_new(&instance, vehicle_index))
		return FALSE;
	rays->valid = TRUE;
	vehicle_riding_cabin_ends(base, height, axis, &ceiling_start, &floor_start);
	rays->inside = collision_model_test_point(&instance, &ceiling_start);
	if (jump_height > 0.f)
	{
		scale_vector3d(axis, jump_height, &ray);
		if (collision_model_test_vector(&instance,
			FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit),
			&ceiling_start, &ray, &result))
		{
			rays->ceiling = TRUE;
			rays->ceiling_distance = result.bsp_result.t*jump_height;
		}
	}
	scale_vector3d(axis, -reach, &ray);
	if (collision_model_test_vector(&instance, FLAG(_collision_test_front_facing_surfaces_bit), &floor_start,
		&ray, &result) && biped_riding_cabin_floor(&instance, &result, axis))
	{
		rays->floor = TRUE;
		rays->floor_distance = result.bsp_result.t*reach;
	}
	rays->hit = vehicle_riding_cabin_hit(rays, jump_height, height);
	return rays->hit;
}

/* whether the way between two pill bases crosses a surface of the
vehicle (either face, never at an end): a hull between them */
static boolean biped_riding_cabin_crosses(
	long vehicle_index,
	real_point3d const *from,
	real_point3d const *to)
{
	struct collision_model_instance instance;
	struct collision_model_test_vector_result result;
	real_vector3d segment;
	boolean hit;

	vector_from_points3d(from, to, &segment);
	if (magnitude3d(&segment) < 0.0001f || !collision_model_instance_new(&instance, vehicle_index))
		return FALSE;
	hit = collision_model_test_vector(&instance,
		FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit), from,
		&segment, &result);
	return vehicle_riding_cabin_crossing(hit, hit ? result.bsp_result.t : 0.f);
}

/* a cabin sample at a pill (vehicle_riding_cabin_axis moves the
rays' axis), against the vehicle's body up, never a node's */
static boolean biped_riding_cabin_sample(
	struct biped_definition const *definition,
	struct vehicle_riding_cabin *cabin,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real height)
{
	struct vehicle_riding_cabin_rays rays;
	real_vector3d axis;

	vehicle_riding_cabin_axis(cabin, &vehicle->body_up, definition->biped.runtime_minimum_normal_k, &axis);
	return biped_riding_cabin_rays(vehicle->object_index, &axis, base, height,
		vehicle_riding_reach(definition->biped.jump_velocity, global_gravity, 0.f), &rays);
}

/* a rider's cabin this tick (vehicle_riding.c): its standing or its
jump's raw sample when one is due, then whether its tilted vehicle
protects it (vehicle_riding_cabin_exception) */
static boolean biped_riding_cabin_tick(
	struct biped_datum *biped,
	struct biped_definition const *definition,
	struct vehicle_riding_cabin *cabin,
	struct vehicle_riding_state const *riding,
	struct vehicle_riding_step const *step,
	struct vehicle_riding_air const *air,
	real_point3d const *base,
	real height)
{
	real_vector3d const *up = vehicle_riding_cabin_up(step);
	real minimum_normal_k = definition->biped.runtime_minimum_normal_k;
	boolean tilted = vehicle_riding_cabin_tilted(up, minimum_normal_k);
	short sample = vehicle_riding_cabin_phase(cabin, riding, step, TEST_FLAG(biped->biped.flags, _biped_airborne_bit),
		tilted);

	if (sample != _vehicle_riding_cabin_sample_none)
	{
		real_point3d origin = *base;
		boolean hit;

		/* (a rider in the window is a tick of the deck's travel behind its place on it until its physics moves it: its jump's
		one sample is of the vehicle it left, where the deck will have carried it) */
		if (sample == _vehicle_riding_cabin_sample_jump && !vehicle_riding_is_riding(riding))
			vehicle_riding_window_base(riding, &step->vehicle, base, &origin);
		hit = biped_riding_cabin_sample(definition, cabin, &step->vehicle, &origin, height);

		if (sample == _vehicle_riding_cabin_sample_standing)
			vehicle_riding_cabin_standing(cabin, hit, vehicle_riding_cabin_grace());
		else
			vehicle_riding_cabin_jump(cabin, hit);
	}
	cabin->exception = vehicle_riding_cabin_exception(biped_riding_applies(biped), TRUE,
		vehicle_riding_cabin_encloses(cabin, riding, air), up, minimum_normal_k);
	return cabin->exception;
}

/* whether a rider in the window of a vehicle is protected by it
during the vehicle's update: its stored cabin verdict, the vehicle's tilt
now (never a sample here: that is the rider's own tick's) */
static boolean biped_riding_cabin_protected(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition const *definition,
	struct vehicle_riding_state const *riding,
	struct vehicle_riding_vehicle const *vehicle)
{
	short player = biped_riding_player(biped);
	struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(player);

	return cabin && cabin->unit_index == biped_index && cabin->vehicle_index == vehicle->object_index &&
		vehicle_riding_cabin_exception(biped_riding_applies(biped),
			riding->ticks > 0 && riding->object_index == vehicle->object_index,
			vehicle_riding_cabin_encloses(cabin, riding, vehicle_riding_player_air(player)),
			&vehicle->body_up, definition->biped.runtime_minimum_normal_k);
}

/* the window's ray at the biped's own tick, from where the deck
carried its base since its last tick (vehicle_riding_window_base), an
enclosed rider's tilted vehicle a deck too (its exception this tick) */
static boolean biped_riding_window_over_deck(
	struct biped_definition const *definition,
	struct vehicle_riding_state const *riding,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real height,
	boolean exception)
{
	real_point3d origin;

	vehicle_riding_window_base(riding, vehicle, base, &origin);
	return biped_riding_over_deck(definition, vehicle, &origin, height) || exception;
}

/* the window's ray again during the vehicle's update, which comes
before the biped's own tick: from the same carried base against the vehicle
as the update sees it, and an enclosed rider protected by its stored
verdict as at its tick, so the two gates never differ */
static boolean biped_riding_recast_over_deck(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition const *definition,
	struct vehicle_riding_state const *riding,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real height)
{
	real_point3d origin;

	vehicle_riding_window_base(riding, vehicle, base, &origin);
	return biped_riding_over_deck(definition, vehicle, &origin, height) ||
		biped_riding_cabin_protected(biped_index, biped, definition, riding, vehicle);
}

/* what the biped landed on, the node of it, and that support's velocity */
void biped_riding_support(
	struct biped_physics *physics,
	struct collision_plane const *collision)
{
	struct vehicle_riding_vehicle vehicle;
	real_point3d contact;
	real_vector3d other;

	physics->support_object_index = NONE;
	physics->support_node_index = NONE;
	physics->support_velocity = *global_zero_vector3d;
	if (!physics->riding_applies || !biped_riding_vehicle(collision->object_index, NONE, &vehicle))
		return;
	physics->support_object_index = collision->object_index;
	physics->support_node_index = biped_riding_support_node(collision->object_index, collision, physics->width);
	/* (at the surface it stands on: the sweep reports the pill's base, a width above it) */
	contact.x = collision->point.x - collision->plane.n.i*physics->width;
	contact.y = collision->point.y - collision->plane.n.j*physics->width;
	contact.z = collision->point.z - collision->plane.n.k*physics->width;
	vehicle_riding_deck_point_velocity(&vehicle, &contact, game_players_are_double_speed(), &other);
	vehicle_riding_support_velocity(
		collision->object_index,
		physics->riding_object_index,
		&physics->riding_frame_velocity,
		physics->tracked_object_index,
		&physics->tracked_deck_velocity,
		&other,
		&physics->support_velocity);
}

/* the hit that carries the biped (vehicle_riding_claim): the ridden
vehicle's while it still holds the biped up */
short biped_riding_claimed_support(
	struct biped_physics const *physics,
	struct collision_plane const *collisions,
	short collision_count,
	short best)
{
	long objects[16];
	real normal_ks[16];
	short index;

	/* (vehicle_riding_claim keeps the best for a biped that rides nothing, so the arrays are not filled) */
	if (physics->riding_object_index == NONE)
		return best;
	for (index = 0; index < collision_count && index < 16; index++)
	{
		objects[index] = collisions[index].object_index;
		normal_ks[index] = collisions[index].plane.n.k;
	}
	return vehicle_riding_claim(physics->riding_object_index, index, objects, normal_ks,
		physics->minimum_normal_k, best);
}

/* the contact that carries an enclosed rider on its steep vehicle
(vehicle_riding_cabin_carrier), and its node: the ground, the slipping and
the landing its physics chose stay as they were */
void biped_riding_cabin_carrier(
	struct biped_physics *physics,
	struct collision_plane const *collisions,
	short collision_count)
{
	long objects[16];
	short index;
	short carrier;

	for (index = 0; index < collision_count && index < 16; index++)
		objects[index] = collisions[index].object_index;
	carrier = vehicle_riding_cabin_carrier(physics->cabin_object_index, index, objects);
	if (carrier == NONE)
		return;
	physics->carrier_object_index = collisions[carrier].object_index;
	physics->carrier_node_index = biped_riding_support_node(collisions[carrier].object_index, &collisions[carrier],
		physics->width);
}

/* (vehicle_riding.c) */
real biped_riding_falling_velocity(
	struct biped_datum const *biped)
{
	struct vehicle_riding_state riding;

	/* (vehicle_riding_falling_velocity's own test, read off the stored fields: the world velocity
	otherwise, with no copy of the state) */
	if (biped->biped.riding_ticks != RIDING_TICKS && !biped->biped.riding_falling)
		return biped->object.translational_velocity.k;
	biped_riding_get(biped, &riding);
	return vehicle_riding_falling_velocity(&riding, biped->object.translational_velocity.k);
}

/* (vehicle_riding.c) */
boolean biped_riding_deferred(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);

	/* (vehicle_riding_deferred's terms, read off the fields: no copy of the state) */
	return biped->biped.riding_ticks > 0 && biped_riding_applies(biped);
}

/* (vehicle_riding.c) */
void biped_riding_take_off(
	long biped_index,
	struct biped_datum const *biped,
	struct vehicle_riding_state const *riding)
{
	vehicle_riding_cabin_take_off(vehicle_riding_player_cabin(biped_riding_player(biped)), riding, biped_index);
}

/* (vehicle_riding.c) */
void biped_riding_relative_velocity(
	long unit_index,
	real_vector3d *velocity)
{
	struct biped_datum *biped = biped_try_and_get(unit_index);
	struct vehicle_riding_state riding;

	*velocity = object_get(unit_index)->object.translational_velocity;
	if (!biped || !biped_riding_applies(biped))
		return;
	biped_riding_get(biped, &riding);
	if (vehicle_riding_is_riding(&riding))
	{
		velocity->i -= riding.velocity.i;
		velocity->j -= riding.velocity.j;
		velocity->k -= riding.velocity.k;
	}
}

/* twice the biped's own run and jump speed a tick, from its tags (only on
the window's path: the tags are not read for anything else) */
static real biped_riding_damage_speed(
	struct biped_datum const *biped)
{
	struct game_globals *globals = scenario_get_game_globals();
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct game_globals_player_information *information = NULL;
	real player_speed_multiplier = 1.f;

	if (globals && globals->player_information.count > 0)
	{
		information = TAG_BLOCK_GET_ELEMENT(&globals->player_information, 0,
			struct game_globals_player_information);
	}
	if (biped->unit.player_index != NONE)
		player_speed_multiplier = player_get(biped->unit.player_index)->speed_multiplier;
	return vehicle_riding_damage_speed(
		information != NULL,
		information ? information->run_forward_speed : 0.f,
		information ? information->run_backward_speed : 0.f,
		information ? information->run_sideways_speed : 0.f,
		game_players_are_double_speed(),
		information ? information->double_speed_multiplier : 1.f,
		player_speed_multiplier,
		definition->biped.jump_velocity);
}

/* how a vehicle's collision treats this biped (vehicle_riding.c); a
touch it does not hit is recorded for the biped's own tick
(biped_riding_contacts), which resolves it against the vehicle as it is
then. Here the pill is where the biped's last tick left it (moving it or
its velocity during the vehicle's update would count the vehicle's motion
twice, or the biped's own friction would undo it) */
short biped_riding_collision(
	long biped_index,
	long vehicle_index,
	real_vector3d const *contact_normal,
	real_point3d const *base,
	real height)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct vehicle_riding_state riding;
	struct vehicle_riding_vehicle vehicle;
	real_vector3d deck_velocity;
	real_vector3d normal;
	boolean over_deck = FALSE;
	short decision;
	short deferred;

	/* (vehicle_riding_collision_window's first test, read off the fields: no copy of the state) */
	if (biped->biped.riding_ticks <= 0 || !biped_riding_applies(biped))
		return _vehicle_riding_collision_original;
	biped_riding_get(biped, &riding);
	decision = vehicle_riding_collision_window(&riding, biped_riding_applies(biped), vehicle_index);
	if (decision == _vehicle_riding_collision_original)
		return decision;
	if (!biped_riding_vehicle(vehicle_index, NONE, &vehicle))
		return decision == _vehicle_riding_collision_judge ? _vehicle_riding_collision_original : decision;
	/* (an enclosed rider's vehicle, protecting it or with its interior
	history: its touch waits for the rider's own tick, read from the stored
	verdict, never a ray here) */
	deferred = vehicle_riding_cabin_collision(decision, vehicle_riding_cabin_defers(
		vehicle_riding_player_cabin(biped_riding_player(biped)), &riding, biped_index, vehicle_index,
		biped_riding_cabin_protected(biped_index, biped, definition, &riding, &vehicle)));
	if (decision != _vehicle_riding_collision_judge && deferred != _vehicle_riding_collision_resolved)
		return decision;
	/* (the vehicle updates before the biped casts its own ray of the tick:
	cast again, against this vehicle, only for the vehicle it left) */
	if (!vehicle_riding_is_riding(&riding) && riding.object_index == vehicle_index)
		over_deck = biped_riding_recast_over_deck(biped_index, biped, definition, &riding, &vehicle, base, height);
	vehicle_riding_deck_point_velocity(&vehicle, base, game_players_are_double_speed(), &deck_velocity);
	decision = vehicle_riding_collision_contact(&riding, vehicle_index, over_deck, &vehicle.bounding_sphere_center,
		&biped->object.bounding_sphere_center, contact_normal, &deck_velocity, biped_riding_damage_speed(biped),
		&biped->object.translational_velocity, &normal);
	if (deferred == _vehicle_riding_collision_resolved)
		decision = _vehicle_riding_collision_resolved;
	/* (a touch not kept, a further vehicle than a tick keeps, waits for its
	next touch: resolved all the same, no push, fix or damage now) */
	if (decision == _vehicle_riding_collision_resolved)
	{
		short player = biped_riding_player(biped);

		vehicle_riding_contact_record(biped_riding_player(biped), vehicle_index, &normal);
		/* (the deferral travels with the touch: the rider's tick may find its own gate closed) */
		if (deferred == _vehicle_riding_collision_resolved)
			vehicle_riding_contact_defer(player, vehicle_index);
	}
	return decision;
}

/* whether a pill at base, its own motion ahead made, still touches
a vehicle, judged as physics_compute_biped_collision finds a touch: the
point inside the model, or that vehicle's features at the pill's width
less 1/64, so a pill resting within that band is not moved again, and the
ground, a slope or another object never count */
static boolean biped_riding_touches(
	long vehicle_index,
	real_point3d const *base,
	real_vector3d const *ahead,
	real height,
	real width)
{
	struct collision_model_instance instance;
	struct collision_feature_list features;
	struct collision_plane plane;
	real_point3d point;
	real_point3d center;

	if (!collision_model_instance_new(&instance, vehicle_index))
		return FALSE;
	set_real_point3d(&point, base->x + ahead->i, base->y + ahead->j, base->z + ahead->k);
	if (collision_model_test_point(&instance, &point))
		return TRUE;
	collision_features_new(&features);
	set_real_point3d(&center, point.x, point.y, point.z + height*0.5f);
	collision_model_get_features_in_sphere(&instance, &center, height*0.5f + width, height,
		MAX(width - 0.015625f, 0.015625f), &features);
	return collision_features_test_point(&features, &point, &plane);
}

/* whether a pill at candidate is a cabin rider's place
(vehicle_riding_cabin_candidate): raw interior, reached from from with only
the vehicle ignored (its own pill's way, so another vehicle, a rider or the
structure blocks it), free of the vehicle, and no surface of the vehicle
between the anchor and it */
static boolean biped_riding_cabin_valid(
	long biped_index,
	struct biped_datum const *biped,
	struct biped_definition const *definition,
	struct vehicle_riding_vehicle const *vehicle,
	real_vector3d const *axis,
	real_point3d const *anchor,
	real_point3d const *from,
	real_point3d const *candidate,
	real height,
	real width,
	real jump_height)
{
	struct vehicle_riding_cabin_rays rays;
	real_vector3d way;
	real_point3d reached = *from;
	boolean interior = biped_riding_cabin_rays(vehicle->object_index, axis, candidate, height, jump_height, &rays);
	boolean free;

	vector_from_points3d(from, candidate, &way);
	biped_riding_carry(biped_index, biped, definition, vehicle->object_index, &way, height, width, &reached);
	free = distance3d(&reached, candidate) < 0.001f &&
		!biped_riding_touches(vehicle->object_index, candidate, global_zero_vector3d, height, width);
	return vehicle_riding_cabin_candidate(interior, free, biped_riding_cabin_crosses(vehicle->object_index, anchor,
		candidate));
}

/* an enclosed rider's place settled against its vehicle as it is
now (vehicle_riding_cabin_recover): an overlap to the near anchor, the
touch's way out (normal) or the fix's offset; a through-hull escape back to
its anchor, or a place about it; a clear door exit let go. Failing, keep the
anchor and the place it came with (entry), retry next tick and log at most once
a second, with no push or damage. final: after its own movement,
where a raw interior place becomes the anchor */
static short biped_riding_cabin_settle(
	long biped_index,
	struct biped_datum const *biped,
	struct biped_definition const *definition,
	struct vehicle_riding_cabin *cabin,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *entry,
	real_vector3d const *normal,
	real_vector3d const *ahead,
	boolean final,
	real height,
	real width,
	real_point3d *base)
{
	struct vehicle_riding_cabin_check check;
	struct vehicle_riding_cabin_rays rays;
	real_vector3d axis;
	real_point3d anchor = *base;
	real_point3d candidate = *base;
	real_point3d fixed_base = *base;
	real_point3d around = *base;
	real_point3d local;
	real jump_height = vehicle_riding_reach(definition->biped.jump_velocity, global_gravity, 0.f);
	boolean acting;
	short settle;
	short rejected;
	short report;

	csmemset(&check, 0, sizeof(check));
	check.have_anchor = cabin->have_anchor;
	check.width = width;
	vehicle_riding_cabin_current_axis(cabin, &vehicle->body_up, &axis);
	check.interior = biped_riding_cabin_rays(vehicle->object_index, &axis, base, height, jump_height, &rays);
	check.touching = biped_riding_touches(vehicle->object_index, base, ahead, height, width);
	if (cabin->have_anchor)
	{
		vehicle_riding_point_to_world(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &cabin->anchor, &anchor);
		check.crosses = biped_riding_cabin_crosses(vehicle->object_index, &anchor, base);
		check.anchor_distance = distance3d(&anchor, base);
	}
	/* (the candidates only when there is something to do: an overlap, or out
	through the hull) */
	acting = cabin->have_anchor && (check.touching || (!check.interior && check.crosses));
	if (acting)
	{
		check.anchor_valid = biped_riding_cabin_valid(biped_index, biped, definition, vehicle, &axis, &anchor, base,
			&anchor, height, width, jump_height);
		if (normal)
		{
			candidate.x = base->x + normal->i*width*2.f;
			candidate.y = base->y + normal->j*width*2.f;
			candidate.z = base->z + normal->k*width*2.f;
			check.normal_valid = biped_riding_cabin_valid(biped_index, biped, definition, vehicle, &axis, &anchor, base,
				&candidate, height, width, jump_height);
		}
		check.fixed_valid = collision_fix_pill(_collision_test_for_bipeds_living_flags, base, width*2.f, height, width,
			biped_index, &fixed_base) && biped_riding_cabin_valid(biped_index, biped, definition, vehicle, &axis, &anchor,
			base, &fixed_base, height, width, jump_height);
		check.around_valid = collision_fix_pill(_collision_test_for_bipeds_living_flags, &anchor, width*2.f, height, width,
			biped_index, &around) && biped_riding_cabin_valid(biped_index, biped, definition, vehicle, &axis, &anchor, base,
			&around, height, width, jump_height);
	}
	settle = vehicle_riding_cabin_recover(&check);
	rejected = (short)(!acting ? 0 : (check.anchor_valid ? 0 : 1) | (normal && !check.normal_valid ? 2 : 0) |
		(check.fixed_valid ? 0 : 4) | (check.around_valid ? 0 : 8));
	report = vehicle_riding_cabin_log(cabin, settle, game_time_get(), final);
	switch (settle)
	{
	case _vehicle_riding_cabin_settle_anchor:
		*base = anchor;
		break;
	case _vehicle_riding_cabin_settle_candidate:
		*base = candidate;
		break;
	case _vehicle_riding_cabin_settle_fixed:
		*base = fixed_base;
		break;
	case _vehicle_riding_cabin_settle_around:
		*base = around;
		break;
	case _vehicle_riding_cabin_settle_failed:
		*base = *entry;
		if (report > 0)
			error(_error_log, "riding cabin: no place tick=%ld unit=%ld vehicle=%ld reason=%s rejected=%d", game_time_get(),
			biped_index, vehicle->object_index, !check.have_anchor ? "no-anchor" : !check.interior && check.crosses ?
			"hull" : "overlap", (int)rejected);
		break;
	default:
		break;
	}
	vehicle_riding_point_to_local(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, base, &local);
	if (report < 0)
		error(_error_log, "riding cabin: recovery ended tick=%ld unit=%ld vehicle=%ld settle=%d", game_time_get(),
			biped_index, vehicle->object_index, (int)settle);
	vehicle_riding_cabin_settled(cabin, settle, final && (settle == _vehicle_riding_cabin_settle_none ?
		check.interior && !check.touching : settle != _vehicle_riding_cabin_settle_door), &local);
	return settle;
}

/* the touches the vehicles recorded on this biped during their
update this tick (biped_riding_collision), each resolved against its
vehicle as it is now: the pill moved out along the touch by the vehicle's
motion toward it beyond the biped's own (vehicle_riding_contact_move),
through everything but that vehicle, its velocity left alone; an overlap
of that vehicle still left once its own motion is made is a real one:
one spot further out along the touch, then the fix, then the original
crush (vehicle_riding_contact_settle). TRUE: it settled a touch of the
cabin's vehicle */
static boolean biped_riding_contacts(
	long biped_index,
	struct biped_datum const *biped,
	struct biped_definition const *definition,
	boolean applies,
	struct vehicle_riding_step const *step,
	boolean carried,
	real_vector3d const *velocity,
	short player_index,
	struct vehicle_riding_cabin *cabin,
	long cabin_vehicle,
	real height,
	real width,
	real_point3d *base)
{
	struct vehicle_riding_contact contacts[RIDING_CONTACT_RECORDS];
	struct vehicle_riding_vehicle vehicle;
	real_vector3d move;
	real_vector3d ahead;
	real_vector3d further;
	real_vector3d push;
	real_point3d candidate;
	real_point3d fixed_base;
	short count = vehicle_riding_contact_take(player_index, contacts);
	short index;
	boolean settled = FALSE;

	/* (taken all the same: a biped that died or stopped riding since keeps none) */
	if (!applies)
		return FALSE;
	for (index = 0; index < count; index++)
	{
		real into;
		real_point3d entry;
		boolean touching;
		boolean candidate_touching = TRUE;
		boolean fixed = FALSE;
		short settle;

		/* (a vehicle gone since its update moves nothing) */
		if (!biped_riding_vehicle(contacts[index].vehicle_index, NONE, &vehicle))
			continue;
		into = vehicle_riding_contact_move(step, carried, &vehicle, base, velocity,
			&contacts[index].normal, game_players_are_double_speed(), &move, &ahead);
		/* (an enclosed rider's own vehicle, or a touch the vehicle's update
		deferred whatever this tick's own gate then finds: its move staged, then
		settled in the cabin, never pushed out of it or hurt. In the window the
		base is a tick of the deck's travel behind until its physics moves it,
		and a return here would be added to that motion: it settles at finish) */
		if (vehicle.object_index == cabin_vehicle || (cabin && contacts[index].deferred))
		{
			entry = *base;
			if (into > 0.f)
				biped_riding_carry(biped_index, biped, definition, vehicle.object_index, &move, height, width, base);
			if (carried)
			{
				biped_riding_cabin_settle(biped_index, biped, definition, cabin, &vehicle, &entry, &contacts[index].normal,
					&ahead, FALSE, height, width, base);
			}
			settled = TRUE;
			continue;
		}
		if (into > 0.f)
			biped_riding_carry(biped_index, biped, definition, vehicle.object_index, &move, height, width, base);
		touching = biped_riding_touches(vehicle.object_index, base, &ahead, height, width);
		if (touching)
		{
			scale_vector3d(&contacts[index].normal, width*2.f, &further);
			candidate = *base;
			biped_riding_carry(biped_index, biped, definition, vehicle.object_index, &further, height, width,
				&candidate);
			candidate_touching = biped_riding_touches(vehicle.object_index, &candidate, &ahead, height, width);
			if (candidate_touching)
			{
				fixed = collision_fix_pill(_collision_test_for_bipeds_living_flags, base, width*2.0f, height, width,
					biped_index, &fixed_base);
			}
		}
		settle = vehicle_riding_contact_settle(touching, candidate_touching, fixed);
		switch (settle)
		{
		case _vehicle_riding_settle_candidate:
			*base = candidate;
			break;
		case _vehicle_riding_settle_fixed:
			*base = fixed_base;
			break;
		case _vehicle_riding_settle_crushed:
			physics_biped_vehicle_collision_push(vehicle.object_index, biped_index, &push);
			physics_biped_vehicle_collision_damage(vehicle.object_index, biped_index, &push);
			break;
		default:
			break;
		}
	}
	return settled;
}

/* what biped_riding_vehicle_writing measured of a player's biped against the unwritten hull: the vehicle
and whether it touched the pill */
struct biped_riding_write_record
{
	long vehicle_index;
	boolean touched_before;
	boolean recorded;
};

static struct biped_riding_write_record biped_riding_writes[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* the slots the last write recorded: the next clears just those, not all of them, so that a record of a player who has
since left cannot carry */
static short biped_riding_write_slots[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static short biped_riding_write_slot_count;

/* the units of the players whose biped a state correction may carry, in ascending object index (the order the walk
of every object gave) */
static short biped_riding_write_candidates(
	long *units)
{
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		struct biped_datum *biped = player->unit_index != NONE ? biped_try_and_get(player->unit_index) : NULL;
		short at;

		/* (a biped that names a player whose unit is another is no candidate: no code path makes that state, and any
		walk over the players diverges from the walk over the objects there) */
		if (!biped || biped->unit.player_index != iterator.datum_index || count >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
			continue;
		for (at = count; at > 0 && DATUM_INDEX_TO_ABSOLUTE_INDEX(units[at - 1]) > DATUM_INDEX_TO_ABSOLUTE_INDEX(player->unit_index); at--)
			units[at] = units[at - 1];
		units[at] = player->unit_index;
		count++;
	}
	return count;
}

/* before a vehicle's state correction is written (its node matrices still the unwritten pose) */
void biped_riding_vehicle_writing(
	long vehicle_index)
{
	long units[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	struct object_datum const *vehicle = object_get(vehicle_index);
	short count = biped_riding_write_candidates(units);
	short at;

	for (at = 0; at < biped_riding_write_slot_count; at++)
	{
		biped_riding_writes[biped_riding_write_slots[at]].recorded = FALSE;
	}
	biped_riding_write_slot_count = 0;
	for (at = 0; at < count; at++)
	{
		struct biped_datum *biped = biped_get(units[at]);
		struct biped_riding_write_record *record;
		real_point3d base;
		real height;
		real width;
		short player = biped_riding_player(biped);

		if (player < 0 || player >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS || !biped_riding_applies(biped) ||
			biped->object.parent_object_index != NONE)
			continue;
		biped_get_physics_pill(units[at], &base, &height, &width);
		/* (the snapshot's own sphere test) */
		if (distance3d(&base, &vehicle->object.bounding_sphere_center) > vehicle->object.bounding_sphere_radius + height + width)
			continue;
		record = &biped_riding_writes[player];
		record->vehicle_index = vehicle_index;
		record->touched_before = biped_riding_touches(vehicle_index, &base, global_zero_vector3d, height, width);
		record->recorded = TRUE;
		biped_riding_write_slots[biped_riding_write_slot_count++] = player;
	}
}

/* after the write, the bipeds it newly touches carried with it */
void biped_riding_vehicle_written(
	long vehicle_index,
	real_point3d const *before_position,
	real_vector3d const *before_forward,
	real_vector3d const *before_up)
{
	long units[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	struct object_datum const *vehicle = object_get(vehicle_index);
	short count = biped_riding_write_candidates(units);
	short at;

	for (at = 0; at < count; at++)
	{
		struct biped_datum *biped = biped_get(units[at]);
		struct biped_riding_write_record *record;
		struct biped_definition *definition;
		struct vehicle_riding_state riding;
		real_point3d base;
		real_point3d local;
		real_point3d carried;
		real_point3d reached;
		real_vector3d way;
		real height;
		real width;
		short player = biped_riding_player(biped);
		boolean touching;

		if (player < 0 || player >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
			continue;
		record = &biped_riding_writes[player];
		if (!record->recorded || record->vehicle_index != vehicle_index)
			continue;
		record->recorded = FALSE;
		if (!biped_riding_applies(biped) || biped->object.parent_object_index != NONE)
			continue;
		definition = biped_definition_get(biped->definition_index);
		biped_get_physics_pill(units[at], &base, &height, &width);
		vehicle_riding_point_to_local(before_position, before_forward, before_up, &base, &local);
		vehicle_riding_point_to_world(&vehicle->object.position, &vehicle->object.forward, &vehicle->object.up, &local,
			&carried);
		touching = biped_riding_touches(vehicle_index, &base, global_zero_vector3d, height, width);
		biped_riding_get(biped, &riding);
		if (vehicle_riding_write_carries(&riding, vehicle_index, record->touched_before, touching,
			distance3d(&carried, &base)))
		{
			reached = base;
			vector_from_points3d(&base, &carried, &way);
			biped_riding_carry(units[at], biped, definition, vehicle_index, &way, height, width, &reached);
			/* (a sweep that ends still touching the written hull leaves the biped where it was, to the original) */
			if (!biped_riding_touches(vehicle_index, &reached, global_zero_vector3d, height, width))
			{
				real_point3d moved;

				moved.x = biped->object.position.x + (reached.x - base.x);
				moved.y = biped->object.position.y + (reached.y - base.y);
				moved.z = biped->object.position.z + (reached.z - base.z);
				object_set_position(units[at], &moved, NULL, NULL);
				if (riding.ticks > 0)
					biped_riding_rebase(units[at]);
			}
		}
	}
}

/* a biped in a seat rides nothing, and its view is not turned */
void biped_riding_seated(
	long biped_index,
	struct biped_datum *biped)
{
	short local_player_index = biped->unit.player_index != NONE ? unit_get_local_player_index(biped_index) : NONE;
	struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(biped_riding_player(biped));

	/* (nothing to clear: the full path would write these same values) */
	if (biped_riding_fields_clear(biped) && vehicle_riding_player_idle(biped_riding_player(biped)) &&
		vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f))
		return;
	biped_riding_clear(biped);
	/* (a seat ends the cabin: its exit claims afresh) */
	if (cabin)
		vehicle_riding_cabin_clear(cabin);
	if (local_player_index != NONE)
		vehicle_riding_look(local_player_index, FALSE, 0.f);
}


void biped_riding_prepare(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition *definition,
	struct biped_physics *physics,
	struct biped_riding_tick *tick)
{
	tick->riding_step_valid = FALSE;
	tick->riding_exception = FALSE;
	tick->riding_cabin_settled = FALSE;
	tick->riding_idle = FALSE;

	/* (nothing to clear: the full path would write these same values) */
	if (biped_riding_idle(biped, biped->unit.player_index != NONE ? unit_get_local_player_index(biped_index) : NONE))
	{
		tick->riding_idle = TRUE;
		physics->riding_applies = FALSE;
		physics->riding_object_index = NONE;
		physics->tracked_object_index = NONE;
		physics->tracked_deck_velocity = *global_zero_vector3d;
		physics->support_object_index = NONE;
		physics->support_node_index = NONE;
		physics->support_velocity = *global_zero_vector3d;
		physics->cabin_object_index = NONE;
		physics->carrier_object_index = NONE;
		physics->carrier_node_index = NONE;
		physics->riding_frame_velocity = *global_zero_vector3d;
		physics->air_frame_velocity = *global_zero_vector3d;
		physics->velocity = biped->object.translational_velocity;
		return;
	}

	/* carried by the vehicle the biped stands on (vehicle_riding.c):
	the deck's motion since the biped's last tick, its turn of the view, and
	the velocity taken in the deck's frame */
	biped_riding_get(biped, &tick->riding);
	tick->riding_applies = biped_riding_applies(biped);
	if (!tick->riding_applies)
	{
		vehicle_riding_clear(&tick->riding);
	}
	else if (tick->riding.ticks > 0)
	{
		/* (a taken host claim outlasts the local node judgement) */
		struct vehicle_riding_claim *claim = vehicle_riding_player_claim(biped_riding_player(biped));
		boolean have_vehicle = biped_riding_vehicle(tick->riding.object_index, tick->riding.node_index, &tick->riding_vehicle);

		if (have_vehicle && claim && claim->taken && claim->vehicle_index == tick->riding.object_index &&
			claim->node_index == tick->riding.node_index)
			tick->riding_vehicle.node_kept = TRUE;
		tick->riding_step_valid = vehicle_riding_snapshot_step(
			&tick->riding,
			have_vehicle ? &tick->riding_vehicle : NULL,
			&physics->position,
			physics->height,
			physics->width,
			game_players_are_double_speed(),
			biped_riding_damage_speed(biped),
			&tick->riding_step);
	}
	tick->riding_carried = tick->riding_step_valid && vehicle_riding_is_riding(&tick->riding);
	if (tick->riding_carried)
	{
		biped_riding_carry(biped_index, biped, definition, tick->riding.object_index, &tick->riding_step.displacement,
			physics->height, physics->width, &physics->position);
	}
	tick->riding_player = biped_riding_player(biped);
	tick->riding_air = vehicle_riding_player_air(tick->riding_player);
	/* (its cabin, once a tick after the carry and before the window's ray is
	judged: the ray's verdict needs it, never the other way round) */
	tick->riding_cabin = vehicle_riding_player_cabin(tick->riding_player);
	if (tick->riding_cabin)
	{
		vehicle_riding_cabin_begin(tick->riding_cabin, biped_index, &tick->riding, tick->riding_applies && tick->riding_step_valid);
		if (tick->riding_applies && tick->riding_step_valid)
		{
			tick->riding_exception = biped_riding_cabin_tick(biped, definition, tick->riding_cabin, &tick->riding, &tick->riding_step, tick->riding_air,
				&physics->position, physics->height);
		}
	}
	/* (in the window, once before its physics: whether it is over the deck it
	left, for the grip, the air control and the hold; an enclosed rider's
	tilted vehicle is one) */
	if (tick->riding_step_valid && !tick->riding_carried)
	{
		tick->riding_step.over_deck = biped_riding_window_over_deck(definition, &tick->riding, &tick->riding_step.vehicle,
			&physics->position, physics->height, tick->riding_exception);
	}
	tick->local_player_index = biped->unit.player_index != NONE ? unit_get_local_player_index(biped_index) : NONE;
	/* (every local player's tick sets its kept turn, 0 when it is not carried) */
	if (tick->local_player_index != NONE)
	{
		real dyaw = vehicle_riding_look(tick->local_player_index, tick->riding_carried, tick->riding_carried ? tick->riding_step.dyaw : 0.f);

		if (dyaw != 0.f)
			biped_riding_turn(biped, physics, dyaw);
	}
	/* (the window's air frame beyond the biped's fields, its player's:
	cleared while it rides, and whether the vehicle it left encloses it
	decided once a jump) */
	if (vehicle_riding_air_begin(tick->riding_air, &tick->riding, tick->riding_step_valid ? &tick->riding_step : NULL,
		TEST_FLAG(biped->biped.flags, _biped_airborne_bit)))
	{
		biped_riding_enclosure(definition, &tick->riding_step.vehicle, &physics->position, physics->height, tick->riding_air);
	}
	vehicle_riding_seed(&tick->riding, &biped->object.translational_velocity, &physics->velocity,
		&physics->riding_frame_velocity);
	/* (a rider's velocity relative to the deck turns with the deck; in the
	window the air frame follows the deck it is over) */
	vehicle_riding_turn_relative(&tick->riding, tick->riding_step_valid ? &tick->riding_step : NULL, &physics->velocity);
	vehicle_riding_air_frame(&tick->riding, tick->riding_step_valid ? &tick->riding_step : NULL, tick->riding_air,
		TEST_FLAG(biped->biped.flags, _biped_airborne_bit), &physics->velocity, &physics->air_frame_velocity);
	/* (the touches the vehicles recorded during their update this tick,
	resolved before its physics, against the vehicles as they are now and
	after the air frame: the biped's own motion is the velocity its physics
	moves it by, so a change of the frame along the touch counts once) */
	if (tick->riding_player != NONE)
	{
		tick->riding_cabin_settled = biped_riding_contacts(biped_index, biped, definition, tick->riding_applies,
			tick->riding_step_valid ? &tick->riding_step : NULL, tick->riding_carried, &physics->velocity, tick->riding_player, tick->riding_cabin,
			tick->riding_step_valid && vehicle_riding_cabin_defers(tick->riding_cabin, &tick->riding, biped_index, tick->riding.object_index,
				tick->riding_exception) ? tick->riding.object_index : NONE,
			physics->height, physics->width, &physics->position);
	}
	/* (an enclosed rider its vehicle's update recorded no touch of, a
	vehicle a client follows never does: its place checked after the carry
	all the same, with no closing move) */
	if (tick->riding_step_valid && tick->riding_carried && !tick->riding_cabin_settled && vehicle_riding_cabin_watched(tick->riding_cabin, &tick->riding, biped_index))
	{
		real_point3d entry = physics->position;

		biped_riding_cabin_settle(biped_index, biped, definition, tick->riding_cabin, &tick->riding_step.vehicle, &entry, NULL,
			global_zero_vector3d, FALSE, physics->height, physics->width, &physics->position);
	}
	physics->riding_applies = tick->riding_applies;
	physics->riding_object_index = tick->riding_carried ? tick->riding.object_index : NONE;
	physics->tracked_object_index = tick->riding_step_valid && !tick->riding_carried ? tick->riding.object_index : NONE;
	physics->tracked_deck_velocity = tick->riding_step_valid ? tick->riding_step.tracked_deck_velocity : *global_zero_vector3d;
	physics->support_object_index = NONE;
	physics->support_node_index = NONE;
	physics->support_velocity = *global_zero_vector3d;
	physics->cabin_object_index = vehicle_riding_cabin_carrier_vehicle(tick->riding_cabin, &tick->riding, tick->riding_exception);
	physics->carrier_object_index = NONE;
	physics->carrier_node_index = NONE;

}

void biped_riding_finish(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition *definition,
	struct biped_physics *physics,
	struct biped_riding_tick *tick)
{
	if (tick->riding_idle)
		return;
	/* (an enclosed rider's place after its own movement, against its
	vehicle as it is now (vehicle_riding.c): an overlap settled in the cabin,
	a through-hull escape returned, a door exit let go; before the recording,
	so the place it keeps on the deck is the settled one) */
	if (tick->riding_step_valid && vehicle_riding_cabin_watched(tick->riding_cabin, &tick->riding, biped_index))
	{
		real_point3d entry = physics->new_position;

		biped_riding_cabin_settle(biped_index, biped, definition, tick->riding_cabin, &tick->riding_step.vehicle, &entry, NULL,
			global_zero_vector3d, TRUE, physics->height, physics->width, &physics->new_position);
	}

	/* what the biped stands on now, for its next tick
	(vehicle_riding.c): the deck's motion added back, the vehicle it rides
	and its snapshot, or its window running down */
	if (tick->riding_applies)
	{
		struct vehicle_riding_vehicle support;
		boolean supported = !TEST_FLAG(physics->out_flags, _biped_physics_out_airborne_bit);
		boolean carried;

		/* (the rider the deck carried into this tick and its exception names stays on it with no touch: past the
		floor's limit a sweep of one tick's gravity finds the vehicle only by chance) */
		if (physics->carrier_object_index == NONE &&
			vehicle_riding_cabin_holds(physics->cabin_object_index, tick->riding_carried))
		{
			physics->carrier_object_index = physics->cabin_object_index;
			physics->carrier_node_index = tick->riding.node_index;
		}
		/* (an enclosed rider's steep vehicle carries it before whatever its
		physics stood it on; that support, its ground and its landing stay
		its physics') */
		carried = physics->carrier_object_index != NONE ?
			biped_riding_vehicle(physics->carrier_object_index, physics->carrier_node_index, &support) :
			supported && biped_riding_vehicle(physics->support_object_index, physics->support_node_index, &support);

		if (vehicle_riding_record(
			&tick->riding,
			tick->riding_step_valid ? &tick->riding_step : NULL,
			carried ? &support : NULL,
			supported,
			&physics->support_velocity,
			&physics->ground_plane.n,
			&physics->new_position,
			TEST_FLAG(biped->unit.flags, _unit_suspended_bit),
			&physics->new_velocity))
		{
			/* (the elevator moves a biped by these, device_machines.c, and would move a rider twice) */
			biped->biped.elevator_object_index = NONE;
			biped->biped.elevator_ticks = 0;
		}
	}
	biped_riding_set(biped, &tick->riding);
	/* (the take-off marked by the last tick's jump is spent; a vehicle
	taken this tick has its cabin keyed to it and sampled once where the
	rider ended, to seed its next tick) */
	if (tick->riding_cabin && tick->riding_applies)
	{
		struct vehicle_riding_vehicle acquired;

		tick->riding_cabin->take_off = FALSE;
		if (vehicle_riding_cabin_acquire(tick->riding_cabin, biped_index, &tick->riding) &&
			biped_riding_vehicle(tick->riding.object_index, NONE, &acquired))
		{
			vehicle_riding_cabin_standing(tick->riding_cabin,
				biped_riding_cabin_sample(definition, tick->riding_cabin, &acquired, &physics->new_position, physics->height),
				vehicle_riding_cabin_grace());
		}
	}
}

/* how many times each object slot was updated: a rider's carry spans
its vehicle's updates since its snapshot (vehicle_riding.c); game state, so
saved and reverted together with the bipeds' snapshots */
static long *object_update_counts;

void vehicle_riding_objects_initialize(void)
{
	vehicle_riding_map_initialize();
	object_update_counts = (long *)game_state_malloc("object update counts", NULL,
		MAXIMUM_OBJECTS_PER_MAP*sizeof(long));
}

void vehicle_riding_objects_reset(void)
{
	vehicle_riding_map_reset();
	vehicle_riding_map_decide();
	if (object_update_counts)
		memset(object_update_counts, 0, MAXIMUM_OBJECTS_PER_MAP*sizeof(long));
}

/* one more update of an object (object_update, children included) */
void object_count_update(
	long object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	if (object_update_counts && absolute_index < MAXIMUM_OBJECTS_PER_MAP)
		++object_update_counts[absolute_index];
}

/* an object's updates so far (vehicle_riding.c) */
long object_update_count(
	long object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	return object_update_counts && absolute_index < MAXIMUM_OBJECTS_PER_MAP ?
		object_update_counts[absolute_index] : 0;
}

/* the object update's main loop, then the bipeds it deferred: a biped
in a vehicle's window updates after every vehicle, so its carry always spans
its vehicle's whole update of the tick (vehicle_riding.c) */
void objects_update_passes(
	boolean dont_update_object,
	boolean (*deferred)(long object_index),
	boolean (*update)(long object_index))
{
	static long riders[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	struct object_header_datum *object_header;
	long rider_count = 0;
	long rider_index;
	short i;
	/* (nothing rides while it is off: whether it is on cannot change inside the loop) */
	boolean riding = vehicle_riding_enabled();

	object_header = (struct object_header_datum *)object_header_data->data;
	for (i = 0; i<object_header_data->count; ++object_header)
	{
		if (object_header->identifier)
		{
			if (TEST_FLAG(object_header->flags, _object_header_active_bit) &&
				!TEST_FLAG(object_header->flags, _object_header_being_created_bit))
			{
				long object_index = DATUM_INDEX_NEW(i, object_header->identifier);
				match_assert("c:\\halo\\SOURCE\\objects\\objects.c", 416, object_get(object_index)->object.parent_object_index==NONE);
				match_assert("c:\\halo\\SOURCE\\objects\\objects.c", 417, object_get(object_index)->object.next_object_index==NONE);

				/* (a player's biped is the only one that defers, so the list holds every player;
				one that did not fit would update in this loop, as a biped that is not deferred) */
				if (riding && rider_count < HALO_PORT_MAXIMUM_NETWORK_PLAYERS && deferred(object_index))
				{
					riders[rider_count++] = object_index;
				}
				else if (!dont_update_object ||
					(TEST_FLAG(_object_mask_unit, object_header->type) &&
					unit_get(object_index)->unit.player_index!=NONE))
				{
					update(object_index);
				}
			}
		}
		++i;
	}

	for (rider_index = 0; rider_index < rider_count; rider_index++)
	{
		long object_index = riders[rider_index];
		struct object_header_datum *rider_header = object_header_try_and_get(object_index);

		/* (an update in the main loop can delete, deactivate or attach another
		object; whether it still rides is not asked again: it takes its one
		update of the tick) */
		if (rider_header &&
			TEST_FLAG(rider_header->flags, _object_header_active_bit) &&
			!TEST_FLAG(rider_header->flags, _object_header_being_created_bit) &&
			object_get(object_index)->object.parent_object_index == NONE &&
			(!dont_update_object ||
				(TEST_FLAG(_object_mask_unit, rider_header->type) &&
				unit_get(object_index)->unit.player_index != NONE)))
		{
			update(object_index);
		}
	}
}

/* a biped in a vehicle's window updates after the main loop (vehicle_riding.c) */
boolean object_update_deferred(
	long object_index)
{
	return object_get_type(object_index) == _object_type_biped && biped_riding_deferred(object_index);
}

/* the features of one object taken out of a list */
static void collision_features_remove_object(
	struct collision_feature_list *features,
	long object_index)
{
	short kept;
	short index;

	for (kept = 0, index = 0; index < features->count[_collision_feature_sphere]; index++)
	{
		if (features->spheres[index].object_index != object_index)
			features->spheres[kept++] = features->spheres[index];
	}
	features->count[_collision_feature_sphere] = kept;
	for (kept = 0, index = 0; index < features->count[_collision_feature_cylinder]; index++)
	{
		if (features->cylinders[index].object_index != object_index)
			features->cylinders[kept++] = features->cylinders[index];
	}
	features->count[_collision_feature_cylinder] = kept;
	for (kept = 0, index = 0; index < features->count[_collision_feature_prism]; index++)
	{
		if (features->prisms[index].object_index != object_index)
			features->prisms[kept++] = features->prisms[index];
	}
	features->count[_collision_feature_prism] = kept;
}

/* a biped standing on a vehicle carried by the vehicle's motion
(vehicle_riding.c): its pill moved as collision_move_pill moves it, through
everything but that vehicle and itself, sliding along what it meets */
void collision_move_pill_carry(
	unsigned long flags,
	real_point3d const *position,
	real_vector3d const *displacement,
	real height,
	real radius,
	long biped_index,
	long vehicle_index,
	real_point3d *carried_position)
{
	struct collision_feature_list features;
	struct collision_plane collisions[16];
	real_point3d start = *position;
	real_point3d center;
	real_vector3d clipped_velocity;

	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 1214, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = 7;

	center.x = start.x + displacement->i * 0.5f;
	center.y = start.y + displacement->j * 0.5f;
	center.z = height * 0.5f + (start.z + displacement->k * 0.5f);

	if (collision_get_features_in_sphere(
		flags,
		&center,
		magnitude3d(displacement) * 0.5f + height * 0.5f + radius,
		height,
		radius,
		vehicle_index,
		&features))
	{
		collision_features_remove_object(&features, biped_index);
		collision_move_point(&start, displacement, &features, carried_position, &clipped_velocity, 16, collisions);
	}
	else
	{
		add_vectors3d((real_vector3d const *)&start, displacement, (real_vector3d *)carried_position);
	}

	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 1230, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;
}
