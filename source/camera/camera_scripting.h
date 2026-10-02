/*
CAMERA_SCRIPTING.H

header included in hcex build.
*/

#ifndef __CAMERA_SCRIPTING_H
#define __CAMERA_SCRIPTING_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/CAMERA_SCRIPTING.C */

struct camera_control;
struct dead_camera;
struct scripted_camera_command;
union real_point3d;
union real_vector3d;

void scripted_camera_enable(
	boolean enabled);
void scripted_camera_set_animation(
	long animation_graph_index,
	char const *animation_name);
void scripted_camera_set_first_person(
	long object_index);
void scripted_camera_set_dead(
	long object_index);
boolean scripted_camera_object_is_first_person_camera(
	long object_index);
void scripted_camera_set(
	short camera_point_index,
	word transition_time,
	long relative_object_index);
void scripted_camera_set_absolute(
	short camera_point_index,
	word transition_time);
void scripted_camera_set_camera_point_relative(
	union real_point3d const *position,
	union real_vector3d const *forward,
	union real_vector3d const *up,
	real field_of_view,
	word transition_time,
	long relative_object_index);
short scripted_camera_time(
	void);
void scripted_camera_update(
	struct dead_camera *camera,
	struct camera_control const *controls,
	struct scripted_camera_command *result);

/* ---------- globals */

/* ---------- public code */

#endif // __CAMERA_SCRIPTING_H
