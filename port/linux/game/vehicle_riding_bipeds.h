/*
VEHICLE_RIDING_BIPEDS.H

Shared movement data for the native biped physics and its riding hooks.
*/

#ifndef VEHICLE_RIDING_BIPEDS_H
#define VEHICLE_RIDING_BIPEDS_H

/* ---------- headers */

#include "vehicle_riding.h"

struct biped_datum;
struct biped_definition;
struct collision_plane;

/* ---------- structures */

enum
{
	_biped_physics_in_airborne_bit = 0,
	_biped_physics_in_slipping_bit,
	_biped_physics_in_crouched_bit,
	_biped_physics_in_trying_to_stand_bit,
	_biped_physics_in_flying_bit,
	_biped_physics_in_absolute_movement_bit,
	_biped_physics_in_no_collision_bit,
	_biped_physics_in_dead_bit,
	_biped_physics_in_pass_through_bipeds_bit,
	_biped_physics_in_climb_anything_bit,
	/* a player's, in a co-op game without player collisions: the other
	players' bipeds are not in its way (network_coop_player_collisions) */
	_biped_physics_in_pass_through_players_bit,
};

enum
{
	_biped_physics_out_airborne_bit = 0,
	_biped_physics_out_slipping_bit,
	_biped_physics_out_cannot_stand_bit,
	_biped_physics_out_splatter_bit,
	_biped_physics_out_volatile_collision_bit,
};

struct biped_physics
{
	long biped_index;
	word in_flags;
	word pad6;
	real_point3d position;
	real_vector3d forward;
	real_vector3d aiming;
	real_vector3d velocity;
	real crouch_velocity;
	real_vector3d movement_desired;
	real movement_penalty;
	real acceleration_maximum;
	real airborne_acceleration_maximum;
	real height;
	real width;
	real ground_tangential_velocity_max;
	real ground_tangential_angle;
	real minimum_normal_k;
	real downhill_k0;
	real downhill_k1;
	real downhill_velocity_scale;
	real uphill_k0;
	real uphill_k1;
	real uphill_velocity_scale;
	real_plane3d ground_plane;
	long existing_support_surface_index;
	real gravity;
	long bumped_object_index;
	long elevator_object_index;
	word out_flags;
	word padA2;
	long support_surface_index;
	long stick_surface_index;
	real_point3d new_position;
	real_vector3d new_velocity;
	real landing_velocity;
	real collision_velocity;
	/* (vehicle_riding.c) the deck it rides and the frame velocity the
	seed took off; the deck it is tracked to in the window and that deck's
	velocity at its spot; what it ended supported by, and that support's
	velocity */
	boolean riding_applies;
	long riding_object_index;
	real_vector3d riding_frame_velocity;
	long tracked_object_index;
	real_vector3d tracked_deck_velocity;
	long support_object_index;
	short support_node_index;
	word pad_support;
	real_vector3d support_velocity;
	/* (vehicle_riding.c) the frame the airborne branch's air control is
	relative to: zero but over the deck of the vehicle the biped just left */
	real_vector3d air_frame_velocity;
	/* (vehicle_riding.c) the vehicle whose steep contacts carry an
	enclosed rider this tick (NONE: none), and the contact's vehicle and
	node: the carry's support, never its physics' ground */
	long cabin_object_index;
	long carrier_object_index;
	short carrier_node_index;
	word pad_carrier;
};

/* Riding locals that live across the native movement, one biped tick only. */
struct biped_riding_tick
{
	struct vehicle_riding_state riding;
	struct vehicle_riding_vehicle riding_vehicle;
	struct vehicle_riding_step riding_step;
	boolean riding_applies;
	boolean riding_step_valid;
	boolean riding_carried;
	short local_player_index;
	short riding_player;
	struct vehicle_riding_air *riding_air;
	struct vehicle_riding_cabin *riding_cabin;
	boolean riding_exception;
	boolean riding_cabin_settled;
	boolean riding_idle;
};

void biped_riding_get(
	struct biped_datum const *biped,
	struct vehicle_riding_state *state);

short biped_riding_player(
	struct biped_datum const *biped);

void biped_riding_clear(
	struct biped_datum *biped);

real biped_riding_falling_velocity(
	struct biped_datum const *biped);

void biped_riding_support(
	struct biped_physics *physics,
	struct collision_plane const *collision);

short biped_riding_claimed_support(
	struct biped_physics const *physics,
	struct collision_plane const *collisions,
	short collision_count,
	short best);

void biped_riding_cabin_carrier(
	struct biped_physics *physics,
	struct collision_plane const *collisions,
	short collision_count);

void biped_riding_seated(
	long biped_index,
	struct biped_datum *biped);

void biped_riding_prepare(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition *definition,
	struct biped_physics *physics,
	struct biped_riding_tick *tick);

void biped_riding_finish(
	long biped_index,
	struct biped_datum *biped,
	struct biped_definition *definition,
	struct biped_physics *physics,
	struct biped_riding_tick *tick);

/* ---------- prototypes/VEHICLE_RIDING_BIPEDS.C */

/* riding on a moving vehicle (vehicle_riding.c) taken from the
settings; off, no biped keeps riding state */
void bipeds_riding_latch(void);

/* a biped that updates after the main loop, and a unit's velocity
relative to the deck it rides (vehicle_riding.c) */
boolean biped_riding_deferred(long biped_index);
void biped_riding_take_off(long biped_index, struct biped_datum const *biped, struct vehicle_riding_state const *riding);
void biped_riding_relative_velocity(long unit_index, real_vector3d *velocity);

/* the deck part of a player's unit for the wire, FALSE outside any
window; and how many nodes a vehicle a deck part names has, NONE for none
(vehicle_riding.c's deck functions, network_distributed.c) */
boolean biped_riding_deck(long biped_index, struct vehicle_riding_deck *deck);
/* the unit's own window for deck-frame network commits */
boolean biped_riding_get_state(long biped_index, struct vehicle_riding_state *state);
void biped_riding_drop(long biped_index);
boolean biped_riding_rebase(long biped_index);
boolean biped_riding_deck_position(long biped_index, struct vehicle_riding_deck const *deck, real_point3d *position);
/* take a remote window with this machine's deck frame */
boolean biped_riding_take_deck(long biped_index, struct vehicle_riding_deck const *deck);
boolean biped_riding_air_correct(long biped_index, real_vector3d const *difference, real peak, short verdict);
boolean biped_riding_air_take(long biped_index, struct vehicle_riding_deck const *deck);
/* the host's legacy verdict does not wait for a positional correction */
boolean biped_riding_air_verdict_host(long biped_index, struct vehicle_riding_deck const *deck, boolean current_episode);
/* a host's deck part's cabin verdict for the unit's player (deck NULL: none) */
void biped_riding_cabin_host(long biped_index, struct vehicle_riding_deck const *deck, boolean current_window);
boolean biped_riding_frame_velocity(long biped_index, struct vehicle_riding_deck const *deck,
	real_point3d const *base, real_vector3d *velocity);
short biped_riding_vehicle_node_count(long vehicle_index);
/* a vehicle as the riding code sees it, posed as its node (NONE: as
itself); FALSE: none, or not a vehicle */
boolean biped_riding_vehicle(long vehicle_index, short node_index, struct vehicle_riding_vehicle *vehicle);

/* how a vehicle's collision treats a biped standing on it, just off
it or in the air over it: an enum vehicle_riding_collision
(vehicle_riding.h); a touch it does not hit is recorded for the biped's
own tick (base: the pill's, of that height; contact_normal: the contact
plane's, NULL when the base is inside the model) */
short biped_riding_collision(long biped_index, long vehicle_index, real_vector3d const *contact_normal,
	real_point3d const *base, real height);

/* a vehicle's state correction carries the biped it newly touches
(biped_riding_vehicle_writing measures before the write,
biped_riding_vehicle_written carries after it) */
void biped_riding_vehicle_writing(long vehicle_index);
void biped_riding_vehicle_written(long vehicle_index, real_point3d const *before_position,
	real_vector3d const *before_forward, real_vector3d const *before_up);
/* Engine object update hooks (vehicle_riding_bipeds.c). */
void vehicle_riding_objects_initialize(void);
void vehicle_riding_objects_reset(void);
void objects_update_passes(
	boolean dont_update_object,
	boolean (*deferred)(long object_index),
	boolean (*update)(long object_index));
boolean object_update_deferred(
	long object_index);

#endif
