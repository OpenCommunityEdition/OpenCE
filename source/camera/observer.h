/*
OBSERVER.H

header included in hcex build.
*/

#ifndef __OBSERVER_H
#define __OBSERVER_H
#pragma once

/* ---------- headers */

#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct observer_result
{
	real_point3d position;
	struct location location;
	real_vector3d velocity;
	real_vector3d forward;
	real_vector3d up;
	real field_of_view;
};

struct observer_command
{
	long flags;
	real_point3d focus_position;
	real_vector3d focus_offset;
	real focus_distance;
	real field_of_view;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d focus_velocity;
	real timer;
	byte parameter_flags[5];
	byte pad51[3];
	real parameter_timers[5];
};

typedef char observer_command_size_assert[
	sizeof(struct observer_command) == 0x68 ? 1 : -1];

/* ---------- prototypes/OBSERVER.C */

void observer_obsolete_position(
	short local_player_index);

void observer_initialize(
	void);
void observer_dispose_from_old_map(
	void);
void observer_initialize_for_new_map(
	void);

struct observer_result const *observer_get_camera(
	short local_player_index);

/* ---------- globals */

/* ---------- public code */

#endif // __OBSERVER_H
