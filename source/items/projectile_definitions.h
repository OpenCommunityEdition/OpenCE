/*
PROJECTILE_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __PROJECTILE_DEFINITIONS_H
#define __PROJECTILE_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "objects/object_definitions.h"

/* ---------- constants */

enum
{
	PROJECTILE_DEFINITION_TAG = 'proj',
};

/* ---------- macros */

#define projectile_definition_get(index) ((struct projectile_definition *)tag_get(PROJECTILE_DEFINITION_TAG, index))

/* ---------- structures */

struct _projectile_definition
{
	unsigned long flags;
	short detonation_timer_starts;
	short impact_noise;
	short function_inputs[4];
	struct tag_reference super_detonation;
	real ai_perception_radius;
	real collision_radius;
	real arming_time;
	real danger_radius;
	struct tag_reference effect;
	real timer_lower_bound;
	real timer_upper_bound;
	real minimum_velocity;
	real maximum_range;
	real air_gravity_scale;
	real air_damage_range_lower_bound;
	real air_damage_range_upper_bound;
	real water_gravity_scale;
	real water_damage_range_lower_bound;
	real water_damage_range_upper_bound;
	real initial_velocity;
	real final_velocity;
	real guided_angular_velocity;
	short detonation_noise;
	word pad;
	struct tag_reference detonation_started;
	struct tag_reference flyby_sound;
	struct tag_reference attached_detonation_damage;
	struct tag_reference impact_damage;
	long unused[3];
	struct tag_block material_responses;
};

struct projectile_definition
{
	struct _object_definition object;
	struct _projectile_definition projectile;
};

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __PROJECTILE_DEFINITIONS_H
