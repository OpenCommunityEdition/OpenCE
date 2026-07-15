/*
ORBITING_CAMERA.C

symbols in this file:
0007B580 0020:
	_orbiting_camera_new (0000)
0007B5A0 0470:
	_orbiting_camera_update (0000)
0025724C 0014:
	_rdata_0025724c (0000)
00257260 0028:
	??_C@_0CI@JLFPHGKF@c?3?2halo?2SOURCE?2camera?2orbiting_c@ (0000)
*/

/* ---------- headers */

#include "orbiting_camera.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void orbiting_camera_new(
	struct orbiting_camera *camera,
	real distance,
	real_vector3d const *forward)
{
	camera->distance = distance;
	euler_angles2d_from_vector3d(&camera->facing, forward);
	return;
}

/* ---------- private code */
