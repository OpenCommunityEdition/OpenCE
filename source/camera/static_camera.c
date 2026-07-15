/*
STATIC_CAMERA.C

symbols in this file:
0007BA10 0070:
	_static_camera_new (0000)
0007BA80 0420:
	_static_camera_update (0000)
00257288 0026:
	??_C@_0CG@CBDIIJJN@c?3?2halo?2SOURCE?2camera?2static_cam@ (0000)
*/

/* ---------- headers */

#include "static_camera.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void static_camera_new(
	struct static_camera *camera,
	real_point3d const *position,
	long location,
	real_vector3d const *forward,
	real_vector3d const *up,
	real field_of_view,
	long timer,
	long flags)
{
	camera->position = *position;
	camera->location = location;
	camera->forward = *forward;
	camera->up = *up;
	camera->field_of_view = field_of_view;
	camera->timer = timer;
	camera->initialized = FALSE;
	camera->flags = flags;
	return;
}

/* ---------- private code */
