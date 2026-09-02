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

enum observer_command_flags
{
	_observer_command_valid_bit,
	_observer_command_force_under_media_bit,
	_observer_command_force_above_media_bit,
	_observer_command_force_time_bit,
	_observer_command_ignore_obstructions_bit,
	_observer_command_freeze_camera_bit,
	NUMBER_OF_OBSERVER_COMMAND_FLAGS
};

enum
{
	NUMBER_OF_OBSERVER_COMMAND_PARAMETERS = 5
};

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
	byte parameter_flags[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS];
	byte pad51[3];
	real parameter_timers[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS];
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
void observer_reconnect_to_structure_bsp(
	void);
void observer_update(
	real time_delta_sec);

struct observer_result const *observer_get_camera(
	short local_player_index);
boolean observer_command_has_finished(
	short local_player_index);

void observer_up_from_forward(
	real_vector3d const *forward,
	real_vector3d *up);
void observer_set_camera(
	short local_player_index,
	struct observer_command *command);

/* ---------- globals */

/* ---------- public code */

#endif // __OBSERVER_H
