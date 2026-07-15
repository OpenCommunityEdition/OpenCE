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

#define point_physics_definition_get(index) ((struct physics_definition *)tag_get(POINT_PHYSICS_TAG, (index)))

/* ---------- structures */

/* ---------- prototypes/POINT_PHYSICS.C */

void point_physics_update(
	unsigned long flags,
	struct physics_definition const *definition,
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
