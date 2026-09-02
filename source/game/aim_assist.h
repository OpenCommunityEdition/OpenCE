/*
AIM_ASSIST.H

header included in hcex build.
*/

#ifndef __AIM_ASSIST_H
#define __AIM_ASSIST_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct aim_assist_parameters
{
	real autoaim_angle;
	real autoaim_distance;
	real magnetism_angle;
	real magnetism_distance;
	real deviation_angle;
	real unused;
};

/* ---------- prototypes/AIM_ASSIST.C */

boolean autoaim_compute_target(
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	long unit_index,
	real_point3d *target_position,
	real_vector3d *target_direction,
	real *target_distance,
	real *target_angle);

/* ---------- globals */

/* ---------- public code */

#endif // __AIM_ASSIST_H
