/*
PHYSICS_VARIABLES.C

symbols in this file:
00143D20 0070:
	_physics_variable_position_update (0000)
00143D90 00f0:
	_physics_variable_speed_update (0000)
00143E80 0080:
	_physics_variable_speed_update_seek (0000)
00143F00 0040:
	_physics_variable_update (0000)
00143F40 0060:
	_code_00143f40 (0000)
00143FA0 0080:
	_physics_variable_position_update_seek (0000)
00144020 0090:
	_physics_variable_update_seek (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "physics/physics_variables.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void
physics_variable_position_update(
	real *position,
	real *limits,
	boolean wrap,
	real delta)
{
	*position += delta;

	if (*position < limits[1])
	{
		if (wrap)
			*position += limits[0] - limits[1];
		else
			*position = limits[1];

		return;
	}

	if (*position > limits[0])
	{
		if (wrap)
			*position -= limits[0] - limits[1];
		else
			*position = limits[0];
	}

	return;
}

void
physics_variable_update(
	real *position,
	real *range,
	real *velocity,
	boolean update_velocity,
	real delta)
{
	physics_variable_speed_update(range, velocity + 2, delta);
	physics_variable_position_update(position, velocity, update_velocity, *range);

	return;
}

/* ---------- private code */
