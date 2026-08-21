/*
CAMERA_SCRIPTING.C

symbols in this file:
00073650 0020:
	_scripted_camera_enable (0000)
00073670 00d0:
	_scripted_camera_set_animation (0000)
00073740 0040:
	_scripted_camera_set_first_person (0000)
00073780 0040:
	_scripted_camera_set_dead (0000)
000737C0 0030:
	_scripted_camera_object_is_first_person_camera (0000)
000737F0 00e0:
	_scripted_camera_set (0000)
000738D0 0020:
	_scripted_camera_set_absolute (0000)
000738F0 00d0:
	_scripted_camera_set_camera_point_relative (0000)
000739C0 0030:
	_scripted_camera_set_camera_point_absolute (0000)
000739F0 0010:
	_scripted_camera_next_camera_point (0000)
00073A00 0010:
	_scripted_camera_object_relative_to (0000)
00073A10 0020:
	_scripted_camera_time (0000)
00073A30 0680:
	_scripted_camera_update (0000)
00256A7C 003d:
	??_C@_0DN@JNLEKBID@cannot?5set?5first?5person?5camera?5o@ (0000)
00256ABC 0029:
	??_C@_0CJ@GLCFCHKA@c?3?2halo?2SOURCE?2camera?2camera_scr@ (0000)
002DCB60 0040:
	_data_002dcb60 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct scripted_camera_globals_view
{
	boolean enabled;
	boolean first_update;
	short mode;
	short camera_point_index;
	byte reserved06[2];
	real timer;
	byte reserved0C[0x28];
	long relative_object_index;
};

/* ---------- prototypes */

void scripted_camera_set(
	short camera_point_index,
	word transition_time,
	long relative_object_index);
void scripted_camera_set_camera_point_relative(
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real field_of_view,
	word transition_time,
	long relative_object_index);

/* ---------- globals */

extern struct scripted_camera_globals_view data_002dcb60;

/* ---------- public code */

void scripted_camera_enable(
	boolean enabled)
{
	data_002dcb60.enabled = enabled;
	data_002dcb60.first_update = TRUE;
	return;
}

void scripted_camera_set_absolute(
	short camera_point_index,
	word transition_time)
{
	scripted_camera_set(camera_point_index, transition_time, NONE);
	return;
}

void scripted_camera_set_camera_point_absolute(
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real field_of_view,
	word transition_time)
{
	scripted_camera_set_camera_point_relative(
		position,
		forward,
		up,
		field_of_view,
		transition_time,
		NONE);
	return;
}

short scripted_camera_next_camera_point(
	void)
{
	return data_002dcb60.camera_point_index;
}

long scripted_camera_object_relative_to(
	void)
{
	return data_002dcb60.relative_object_index;
}

short scripted_camera_time(
	void)
{
	return (short)(data_002dcb60.timer * 30.f);
}

/* ---------- private code */
