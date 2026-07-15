/*
FLYING_CAMERA.H

header included in hcex build.
*/

#ifndef __FLYING_CAMERA_H
#define __FLYING_CAMERA_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct flying_camera
{
	real_point3d position;
	real_euler_angles3d facing;
	real field_of_view;
};

/* ---------- prototypes/FLYING_CAMERA.C */

void flying_camera_new(
	struct flying_camera *camera);

/* ---------- globals */

/* ---------- public code */

#endif // __FLYING_CAMERA_H
