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

static real code_00143f40(
	real *limits,
	real position,
	boolean wrap,
	real target);

/* ---------- globals */

/* ---------- public code */

void
physics_variable_speed_update(
	real *speed,
	struct physics_variable_speed_parameters *parameters,
	real delta)
{
	real magnitude = (real)fabs(delta);
	real acceleration = magnitude * parameters->acceleration;
	real deceleration = magnitude * parameters->deceleration;

	if (delta > 0.f)
	{
		if (*speed <= -deceleration)
			*speed += deceleration;
		else if (*speed >= 0.f)
			*speed += acceleration;
		else
			*speed = (*speed / deceleration + 1.f) * acceleration;

		*speed = MIN(*speed, magnitude * parameters->positive_scale);

		return;
	}

	if (delta < 0.f)
	{
		if (*speed >= deceleration)
			*speed -= deceleration;
		else if (*speed <= 0.f)
			*speed -= acceleration;
		else
			*speed = (*speed / deceleration - 1.f) * acceleration;

		*speed = MAX(-magnitude * parameters->negative_scale, *speed);
	}

	return;
}

boolean
physics_variable_speed_update_seek(
	real *speed,
	struct physics_variable_speed_parameters *parameters,
	real target,
	real delta)
{
	boolean result = FALSE;

	if (*speed > target)
	{
		physics_variable_speed_update(speed, parameters, -delta);
		if (*speed <= target)
		{
			*speed = target;
			result = TRUE;
		}
	}
	else if (*speed < target)
	{
		physics_variable_speed_update(speed, parameters, delta);
		if (*speed >= target)
		{
			*speed = target;
			result = TRUE;
		}
	}
	else
	{
		result = TRUE;
	}

	return result;
}

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

boolean
physics_variable_position_update_seek(
	real *position,
	real *limits,
	boolean wrap,
	real target,
	real delta)
{
	real direction = code_00143f40(limits, *position, wrap, target);

	if (direction != 0.f)
	{
		physics_variable_position_update(position, limits, wrap, direction * delta);
		if (code_00143f40(limits, *position, wrap, target) == direction)
			return FALSE;
	}

	*position = target;

	return TRUE;
}

void
physics_variable_update(
	real *position,
	real *range,
	real *velocity,
	boolean update_velocity,
	real delta)
{
	physics_variable_speed_update(
		range,
		(struct physics_variable_speed_parameters *)(velocity + 2),
		delta);
	physics_variable_position_update(position, velocity, update_velocity, *range);

	return;
}

/* ---------- private code */

static real
code_00143f40(
	real *limits,
	real position,
	boolean wrap,
	real target)
{
	real direction = target - position;

	if (direction != 0.f)
	{
		if (wrap && fabs(direction) > (limits[0] - limits[1]) * 0.5f)
			direction = -direction;

		direction = direction > 0.f ? 1.f : -1.f;
	}

	return direction;
}
