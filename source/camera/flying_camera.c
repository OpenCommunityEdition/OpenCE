/*
FLYING_CAMERA.C

symbols in this file:
000779A0 0020:
	_flying_camera_new (0000)
000779C0 0050:
	_flying_camera_new_from_point_and_vector (0000)
00077A10 04b0:
	_flying_camera_update (0000)
00256E34 0026:
	??_C@_0CG@FECMEHCO@c?3?2halo?2SOURCE?2camera?2flying_cam@ (0000)
*/

/* ---------- headers */

#include "flying_camera.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void flying_camera_new(
	struct flying_camera *camera)
{
	camera->position.y = 0.f;
	camera->position.x = 0.f;
	camera->facing.yaw = 0.f;
	camera->facing.pitch = 0.f;
	camera->facing.roll = 0.f;
	camera->field_of_view = DEGREES_TO_RADIANS(70);
	return;
}

/* ---------- private code */
