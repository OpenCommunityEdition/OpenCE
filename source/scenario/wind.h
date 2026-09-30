/*
WIND.H
*/

#ifndef __WIND_H
#define __WIND_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- structures */

/* ---------- prototypes/WIND.C */

void wind_dispose_from_old_map(
	void);
void wind_initialize_for_new_map(
	void);
void wind_update(
	void);
void scenario_get_wind(
	struct location const *location,
	real_point3d const *position,
	real_vector3d *wind_vector,
	long flags);
void scenario_get_water_current(
	struct location const *location,
	real_point3d const *position,
	real_vector3d *wind_vector,
	long flags);

/* ---------- globals */

#endif // __WIND_H
