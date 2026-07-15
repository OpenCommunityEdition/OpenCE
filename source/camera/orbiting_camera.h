/*
ORBITING_CAMERA.H

header included in hcex build.
*/

#ifndef __ORBITING_CAMERA_H
#define __ORBITING_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct orbiting_camera
{
	real_euler_angles2d facing;
	real distance;
};

/* ---------- prototypes/ORBITING_CAMERA.C */

void orbiting_camera_new(
	struct orbiting_camera *camera,
	real distance,
	real_vector3d const *forward);

/* ---------- globals */

/* ---------- public code */

#endif // __ORBITING_CAMERA_H
