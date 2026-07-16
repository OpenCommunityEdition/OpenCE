/*
POINT_PHYSICS.H

header included in hcex build.
*/

#ifndef __POINT_PHYSICS_H
#define __POINT_PHYSICS_H
#pragma once

/* ---------- headers */

#include "objects/objects.h"
#include "physics/physics_definitions.h"

/* ---------- constants */

enum
{
	POINT_PHYSICS_TAG = 'pphy',
};

/* ---------- macros */

#define point_physics_definition_get(index) ((struct point_physics_definition *)tag_get(POINT_PHYSICS_TAG, (index)))

/* ---------- structures */

struct point_physics_definition
{
	unsigned long flags;
	real density;
	real water_gravity_scale;
	real air_gravity_scale;
	byte unused[0x10];
	real unknown20;
	real air_friction;
	real water_friction;
	real surface_friction;
	real elasticity;
};

typedef char point_physics_definition_size_assert[
	sizeof(struct point_physics_definition) == 0x34 ? 1 : -1];

struct point_physics_globals
{
	real water_density;
	real air_density;
};

/* ---------- prototypes/POINT_PHYSICS.C */

void point_physics_dispose_from_old_map(
	void);
void point_physics_initialize_for_new_map(
	void);
real point_physics_definition_get_mass(
	struct point_physics_definition const *definition,
	real scale);
struct point_physics_definition *point_physics_definition_interpolate(
	struct point_physics_definition const *physics1,
	struct point_physics_definition const *physics2,
	real t,
	struct point_physics_definition *result);
unsigned long point_physics_update(
	unsigned long flags,
	struct point_physics_definition const *definition,
	struct location *location,
	long collision_flags,
	real_point3d *position,
	real_vector3d *translational_velocity,
	real_vector3d const *translational_force,
	real_vector3d *collision_normal,
	short *material_type,
	real scale,
	real delta_time);

/* ---------- globals */

/* ---------- public code */

#endif // __POINT_PHYSICS_H
