/*
FOLLOWING_CAMERA.H

header included in hcex build.
*/

#ifndef __FOLLOWING_CAMERA_H
#define __FOLLOWING_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct following_camera
{
	boolean initialized;
	boolean field_1;
	boolean unit_is_biped;
	boolean field_3;
	short zoom_level;
	short pad6;
	long unit_index;
	short seat_index;
	short padE;
	real_euler_angles2d facing_angles;
	real position_scale;
};

/* ---------- prototypes/FOLLOWING_CAMERA.C */

void following_camera_new(
	struct following_camera *camera);

/* ---------- globals */

/* ---------- public code */

#endif // __FOLLOWING_CAMERA_H
