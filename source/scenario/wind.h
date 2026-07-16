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

struct wind_globals
{
	boolean initialized;
};

/* ---------- prototypes/WIND.C */

boolean scenario_get_current(
	struct location const *location,
	real_point3d const *position,
	real_vector3d *current,
	unsigned long flags);
void scenario_get_current_from_weather_palette(
	real_point3d const *position,
	real_vector3d *current,
	byte flags,
	short weather_palette_index);
void wind_dispose_from_old_map(
	void);
void wind_update(
	void);

/* ---------- globals */

extern struct wind_globals wind_globals;

#endif // __WIND_H
