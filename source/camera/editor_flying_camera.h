/*
EDITOR_FLYING_CAMERA.H

header included in hcex build.
*/

#ifndef __EDITOR_FLYING_CAMERA_H
#define __EDITOR_FLYING_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EDITOR_FLYING_CAMERA.C */

long editor_camera_get_speed(
	void);

long editor_camera_get_unit_focus(
	void);

short editor_camera_get_mode(
	void);

boolean editor_camera_get_scripted(
	void);

real editor_camera_get_field_of_view(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __EDITOR_FLYING_CAMERA_H
