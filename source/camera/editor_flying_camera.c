/*
EDITOR_FLYING_CAMERA.C

symbols in this file:
00075E70 00d0:
	_editor_camera_new (0000)
00075F40 0080:
	_editor_camera_get_focus (0000)
00075FC0 0080:
	_editor_camera_set_focus (0000)
00076040 00a0:
	_editor_camera_set_position (0000)
000760E0 0040:
	_editor_camera_bump_speed (0000)
00076120 0010:
	_editor_camera_get_speed (0000)
00076130 0030:
	_editor_camera_use_roll (0000)
00076160 0010:
	_editor_camera_get_unit_focus (0000)
00076170 00e0:
	_editor_camera_set_mode (0000)
00076250 0010:
	_editor_camera_get_mode (0000)
00076260 0010:
	_editor_camera_get_scripted (0000)
00076270 0070:
	_code_00076270 (0000)
000762E0 0010:
	_editor_camera_get_field_of_view (0000)
000762F0 0080:
	_editor_camera_move_to_point (0000)
00076370 01b0:
	_editor_camera_set_position_and_roll (0000)
00076520 0070:
	_editor_camera_set_unit_focus (0000)
00076590 0130:
	_editor_camera_update (0000)
000766C0 01b0:
	_editor_camera_set_scripted (0000)
00076870 0060:
	_code_00076870 (0000)
000768D0 0580:
	_code_000768d0 (0000)
00076E50 0460:
	_code_00076e50 (0000)
00256C64 0030:
	_rdata_00256c64 (0000)
00256C94 0008:
	??_C@_07NACDKLFL@exiting?$AA@ (0000)
00256C9C 0010:
	??_C@_0BA@NDFANOEE@orbiting?5camera?$AA@ (0000)
00256CAC 000e:
	??_C@_0O@GNANFALA@flying?5camera?$AA@ (0000)
00256CBC 0007:
	??_C@_06GNEAMJOC@angles?$AA@ (0000)
00256CC4 0009:
	??_C@_08OAGMDKAF@position?$AA@ (0000)
00256CD0 002d:
	??_C@_0CN@ELKCHNLL@c?3?2halo?2SOURCE?2camera?2editor_fly@ (0000)
00256D00 0011:
	??_C@_0BB@JCBBBJE@speed?5is?5now?5x?$CFf?$AA@ (0000)
00256D14 0025:
	??_C@_0CF@ONCNFIFO@translate_funcs?$FLmode?$FN?$FL_translate@ (0000)
00256D3C 002e:
	??_C@_0CO@LELHEFFL@translate_funcs?$FLcamera_mode?$FN?$FL_tr@ (0000)
00256D6C 001a:
	??_C@_0BK@JFLLMCBA@update_funcs?$FLcamera_mode?$FN?$AA@ (0000)
00256D88 0018:
	??_C@_0BI@CHKFHLBI@?$CFs?5scripted?5camera?5mode?$AA@ (0000)
00256DA0 0004:
	__real@3fc8a8ea (0000)
00256DA4 0004:
	__real@bfc8a8ea (0000)
00256DA8 0009:
	??_C@_08JLGBNAMD@controls?$AA@ (0000)
00256DB4 0004:
	__real@3fa0d97c (0000)
00256DB8 0004:
	__real@bfa0d97c (0000)
002DCC28 0034:
	_data_002dcc28 (0000)
	_editor_custom_render (0008)
0031D438 007c:
	_bss_0031d438 (0000)
*/

/* ---------- headers */

#include "editor_flying_camera.h"
#include "flying_camera.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

enum
{
	NUMBER_OF_EDITOR_CAMERA_SPEED_STEPS = 5
};

/* ---------- macros */

/* ---------- structures */

struct editor_camera_data
{
	real speed;
	long unit_focus;
};

struct editor_camera_focus
{
	real_point3d position;
	real_euler_angles2d angles;
};

struct editor_camera_globals
{
	boolean scripted;
	boolean use_roll;
	boolean initialized;
	byte _unknown03;
	struct editor_camera_focus focus;
	struct flying_camera *camera;
	byte _unknown1c[0x10];
	short mode;
	byte _unknown2e[0x4A];
	unsigned long speed_step;
};

typedef char editor_camera_globals_speed_step_offset_assert[
	offsetof(struct editor_camera_globals, speed_step) == 0x78 ? 1 : -1];
typedef char editor_camera_globals_size_assert[
	sizeof(struct editor_camera_globals) == 0x7C ? 1 : -1];

struct editor_camera_constants
{
	long speed_steps[5];
	real field_of_view_by_mode[7];
};

struct editor_camera_player_starting_location
{
	real_point3d position;
	real facing;
	short team_index;
	short structure_bsp_reference_index;
	byte _unknown14[0x20];
};

typedef char editor_camera_player_starting_location_size_assert[
	sizeof(struct editor_camera_player_starting_location) == 0x34 ? 1 : -1];

typedef void (*editor_camera_update_function)(
	struct flying_camera *camera,
	struct flying_camera_action const *controls,
	struct camera_command *result);
typedef void (*editor_camera_translate_function)(
	struct flying_camera *camera);

struct editor_camera_dispatch_data
{
	struct render_globals *custom_render;
	editor_camera_update_function update_functions[2];
	editor_camera_translate_function translate_functions[2][2];
};

typedef char editor_camera_dispatch_translate_functions_offset_assert[
	offsetof(struct editor_camera_dispatch_data, translate_functions) == 0xC ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct editor_camera_data data_002dcc28;
extern struct editor_camera_globals bss_0031d438;
extern struct editor_camera_constants const rdata_00256c64;
extern struct editor_camera_dispatch_data editor_custom_render;

/* ---------- public code */

void editor_camera_new(
	struct flying_camera *camera,
	short local_player_index)
{
	real_vector3d forward;

	if (!bss_0031d438.initialized)
	{
		if (global_scenario_get()->players.count &&
			global_scenario_get()->players.address)
		{
			struct editor_camera_player_starting_location *starting_location =
				TAG_BLOCK_GET_ELEMENT(
					&global_scenario_get()->players,
					0,
					struct editor_camera_player_starting_location);

			bss_0031d438.focus.position = starting_location->position;
			bss_0031d438.focus.angles.yaw = starting_location->facing;
		}
		else
		{
			csmemset(
				&bss_0031d438.focus,
				0,
				sizeof(bss_0031d438.focus));
		}
	}

	bss_0031d438.initialized = TRUE;
	vector3d_from_euler_angles2d(
		&forward,
		&bss_0031d438.focus.angles);
	flying_camera_new_from_point_and_vector(
		camera,
		&bss_0031d438.focus.position,
		&forward);

	if (!local_player_index)
		bss_0031d438.camera = camera;
	if (bss_0031d438.mode)
	{
		editor_custom_render.translate_functions[bss_0031d438.mode][1](
			camera);
	}

	return;
}

void editor_camera_get_focus(
	real_point3d *position,
	real_euler_angles2d *angles)
{
	match_assert("c:\\halo\\SOURCE\\camera\\editor_flying_camera.c", 120, position);
	match_assert("c:\\halo\\SOURCE\\camera\\editor_flying_camera.c", 121, angles);

	*position = bss_0031d438.focus.position;
	*angles = bss_0031d438.focus.angles;

	return;
}

void editor_camera_set_focus(
	real_point3d const *position,
	real_euler_angles2d const *angles)
{
	match_assert("c:\\halo\\SOURCE\\camera\\editor_flying_camera.c", 129, position);
	match_assert("c:\\halo\\SOURCE\\camera\\editor_flying_camera.c", 130, angles);

	bss_0031d438.focus.position = *position;
	bss_0031d438.focus.angles = *angles;

	return;
}

void editor_camera_bump_speed(
	void)
{
	bss_0031d438.speed_step =
		(bss_0031d438.speed_step + 1) % NUMBER_OF_EDITOR_CAMERA_SPEED_STEPS;
	data_002dcc28.speed =
		(real)rdata_00256c64.speed_steps[bss_0031d438.speed_step];
	terminal_printf(
		global_real_argb_white,
		"speed is now x%f",
		data_002dcc28.speed);

	return;
}

long editor_camera_get_speed(
	void)
{
	return (long)data_002dcc28.speed;
}

boolean editor_camera_use_roll(
	boolean new_use_roll)
{
	boolean previous_use_roll = bss_0031d438.use_roll;

	bss_0031d438.use_roll = new_use_roll;
	if (!new_use_roll && bss_0031d438.camera)
		bss_0031d438.camera->facing.roll = 0.0f;

	return previous_use_roll;
}

long editor_camera_get_unit_focus(
	void)
{
	return data_002dcc28.unit_focus;
}

short editor_camera_get_mode(
	void)
{
	return bss_0031d438.mode;
}

boolean editor_camera_get_scripted(
	void)
{
	return bss_0031d438.scripted;
}

real editor_camera_get_field_of_view(
	void)
{
	return rdata_00256c64.field_of_view_by_mode[5 + bss_0031d438.mode];
}

/* ---------- private code */
