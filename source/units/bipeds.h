/*
BIPEDS.H

header included in hcex build.
*/

#ifndef __BIPEDS_H
#define __BIPEDS_H
#pragma once

/* ---------- headers */

#include "units.h"

/* ---------- constants */

enum
{
	_biped_limping_bit = 0,
};

/* ---------- macros */

#define biped_get(index)			((struct biped_datum *)object_get_and_verify_type((index), _object_mask_biped))
#define biped_try_and_get(index)	((struct biped_datum *)object_try_and_get_and_verify_type((index), _object_mask_biped))

/* ---------- structures */

struct animation;

struct _biped_datum
{
	unsigned long flags;
	char landing_recovery_counter;
	char landing_recovery_time;
	char state;
	char elevator_ticks;
	long elevator_object_index;
	long support_surface_index;
	long pathfinding_surface_index;
	real_point3d pathfinding_point;
	long last_pathfinding_attempt_time;
	long last_pathfinding_surface_index;
	long impact_target_object_index;
	long last_falling_communication_time;
	long bump_object_index;
	char bump_ticks;
	char airborne_ticks;
	char slipping_ticks;
	char stop_ticks;
	char jump_recovery_timer;
	char player_melee_ticks;
	char player_melee_attack_tick;
	short landing;
	real crouch;
	real bank;
	real_plane3d ground_plane;
	byte limp_body_current_relaxation_iterations;
	byte limp_body_max_relaxation_iterations;
};

struct biped_datum
{
	long definition_index;
	struct _object_datum object;
	struct _unit_datum unit;
	struct _biped_datum biped;
};

/* ---------- prototypes/BIPEDS.C */

char animation_left_foot_frame_index(
	struct animation const *animation);
char animation_right_foot_frame_index(
	struct animation const *animation);

void bipeds_initialize(
	void);
void bipeds_initialize_for_new_map(
	void);
void bipeds_dispose_from_old_map(
	void);
void bipeds_dispose(
	void);
void biped_delete(
	long biped_index);
boolean biped_flying_through_air(
	long biped_index);

void biped_get_sight_position(
	long biped_index,
	short estimate_mode,
	real_point3d const *estimated_body_position,
	real_vector3d *desired_facing,
	real_vector3d const *desired_gun_offset,
	real_point3d *sight_position);
void biped_get_physics_pill(long biped_index, real_point3d *base, real *height, real *width);

void biped_stop_limp_body_physics(long biped_index);
void biped_start_limp_body_physics(
	long biped_index);

void biped_build_flying_axes(real_vector3d const *forward_vector, real_vector3d *left_vector, real_vector3d *up_vector);

/* ---------- globals */

/* ---------- public code */

#endif // __BIPEDS_H
