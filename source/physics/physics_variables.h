/*
PHYSICS_VARIABLES.H

header included in hcex build.
*/

#ifndef __PHYSICS_VARIABLES_H
#define __PHYSICS_VARIABLES_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/PHYSICS_VARIABLES.C */

void physics_variable_speed_update(
	real *range,
	real *velocity,
	real delta);

void physics_variable_position_update(
	real *position,
	real *limits,
	boolean wrap,
	real delta);

void physics_variable_update(
	real *position,
	real *range,
	real *velocity,
	boolean update_velocity,
	real delta);

/* ---------- globals */

/* ---------- public code */

#endif // __PHYSICS_VARIABLES_H
