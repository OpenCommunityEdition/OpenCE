/*
WEATHER_PARTICLE_SYSTEMS.H

header included in hcex build.
*/

#ifndef __WEATHER_PARTICLE_SYSTEMS_H
#define __WEATHER_PARTICLE_SYSTEMS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

void weather_particle_systems_initialize(
	void);
void weather_particle_systems_initialize_for_new_map(
	void);
void weather_particle_systems_dispose_from_old_map(
	void);
void weather_particle_systems_dispose(
	void);

extern struct data_array *weather_particle_data;

#endif // __WEATHER_PARTICLE_SYSTEMS_H
