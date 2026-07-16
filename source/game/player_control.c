/*
PLAYER_CONTROL.C

symbols in this file:
000A56F0 0050:
	_player_control_get (0000)
000A5740 0020:
	_player_control_initialize (0000)
000A5760 0010:
	_player_control_dispose (0000)
000A5770 0010:
	_player_control_dispose_from_old_map (0000)
000A5780 0020:
	_code_000a5780 (0000)
000A57A0 0030:
	_scripted_player_control_set_camera_control (0000)
000A57D0 0060:
	_player_control_inhibit_buttons (0000)
000A5830 00f0:
	_evaluate_piecewise_linear_function (0000)
000A5920 0010:
	_code_000a5920 (0000)
000A5930 0060:
	_player_control_get_aiming_unit_index (0000)
000A5990 0070:
	_player_control_get_target_object_index (0000)
000A5A00 00b0:
	_player_control_get_field_of_view (0000)
000A5AB0 0130:
	_player_control_get_unit_camera_info (0000)
000A5BE0 0050:
	_player_control_get_unit_index (0000)
000A5C30 0080:
	_player_control_get_desired_weapon (0000)
000A5CB0 0050:
	_player_control_get_autoaim_level (0000)
000A5D00 0040:
	_unit_get_local_player_index (0000)
000A5D40 0050:
	_players_unzoom_all (0000)
000A5D90 0050:
	_player_control_unzoom (0000)
000A5DE0 0020:
	_player_control_get_zoom_level (0000)
000A5E00 0020:
	_player_control_action_test_reset (0000)
000A5E20 0020:
	_player_control_action_test_accept (0000)
000A5E40 0020:
	_player_control_action_test_back (0000)
000A5E60 0020:
	_player_control_action_test_action (0000)
000A5E80 0010:
	_player_control_action_test_jump (0000)
000A5E90 0010:
	_player_control_action_test_primary_trigger (0000)
000A5EA0 0010:
	_player_control_action_test_grenade_trigger (0000)
000A5EB0 0010:
	_player_control_action_test_zoom (0000)
000A5EC0 0020:
	_player_control_action_test_move_relative_all_directions (0000)
000A5EE0 0020:
	_player_control_action_test_look_relative_all_directions (0000)
000A5F00 0010:
	_player_control_action_test_look_relative_left (0000)
000A5F10 0010:
	_player_control_action_test_look_relative_right (0000)
000A5F20 0010:
	_player_control_action_test_look_relative_up (0000)
000A5F30 0010:
	_player_control_action_test_look_relative_down (0000)
000A5F40 0200:
	_code_000a5f40 (0000)
000A6140 0040:
	_signed_angular_difference (0000)
000A6180 0050:
	_limit2d (0000)
000A61D0 0040:
	_interpolate_scalar (0000)
000A6210 0120:
	_player_control_set_facing (0000)
000A6330 00f0:
	_player_control_new_unit (0000)
000A6420 0d80:
	_code_000a6420 (0000)
000A71A0 00e0:
	_player_control_get_facing_angles (0000)
000A7280 0030:
	_player_control_get_facing_direction (0000)
000A72B0 0050:
	_player_control_set_desired_weapon (0000)
000A7300 0630:
	_code_000a7300 (0000)
000A7930 0090:
	_player_control_initialize_for_new_map (0000)
000A79C0 0620:
	_code_000a79c0 (0000)
000A7FE0 0080:
	_player_control_update (0000)
000A8060 0040:
	_player_control_permanent_impulse (0000)
0025C614 0016:
	??_C@_0BG@KLOMMKO@player_control_update?$AA@ (0000)
0025C62C 000c:
	_MOUSE_YAW_SCALE (0000)
	_MOUSE_PITCH_SCALE (0004)
0025C638 0025:
	??_C@_0CF@CDJDDDIC@c?3?2halo?2SOURCE?2game?2player_contr@ (0000)
0025C660 0017:
	??_C@_0BH@GDMLDNGJ@player?5control?5globals?$AA@ (0000)
0025C678 0046:
	??_C@_0EG@FFIEHMMD@function?5?$CG?$CG?5low_index?$DO?$DN0?5?$CG?$CG?5low_@ (0000)
0025C6C0 0004:
	__real@3f9c61aa (0000)
0025C6C4 000c:
	??_C@_0M@KHNJLLND@camera_info?$AA@ (0000)
0025C6D0 0004:
	__real@c0490fdb (0000)
0025C6D4 0023:
	??_C@_0CD@PHGHILFD@player_control?9?$DOdesired_angles?4y@ (0000)
0025C6F8 0025:
	??_C@_0CF@JIDICGJ@player_control?9?$DOdesired_angles?4p@ (0000)
0025C720 0017:
	??_C@_0BH@BJMHFMPG@input?9?$DOprimary_trigger?$AA@ (0000)
0025C738 0004:
	__real@3f75dcc7 (0000)
0025C73C 0004:
	__real@3d567750 (0000)
0025C740 0004:
	__real@bd567750 (0000)
0025C744 0004:
	__real@3dd67750 (0000)
0025C748 0004:
	__real@bdd67750 (0000)
0025C74C 0027:
	??_C@_0CH@OIFLGJJD@constants?9?$DOlook_acceleration_tim@ (0000)
0025C774 0021:
	??_C@_0CB@KGNPCEEG@constants?9?$DOlook_function?4count?$DO1@ (0000)
0025C798 002e:
	??_C@_0CO@HHADHJPE@valid_euler_angles2d?$CI?$CGplayer?9?$DOde@ (0000)
0025C7C8 0004:
	__real@bfbf0243 (0000)
0025C7CC 0004:
	__real@3fbf0243 (0000)
0025C7D0 0004:
	__real@3c490fdb (0000)
0025C7D4 0004:
	__real@bc490fdb (0000)
0025C7D8 0004:
	__real@3da3d70a (0000)
0025C7DC 001d:
	??_C@_0BN@JFNDAOJI@player?9?$DOdesired_angles?4pitch?$AA@ (0000)
0025C800 0008:
	__real@3fe45f3060000000 (0000)
0025C808 001a:
	??_C@_0BK@GKNPAGDI@action?4desired_facing?4yaw?$AA@ (0000)
0025C824 001c:
	??_C@_0BM@BMIDFCCA@action?4desired_facing?4pitch?$AA@ (0000)
0025C840 0018:
	??_C@_0BI@OPEALALJ@player?9?$DOprimary_trigger?$AA@ (0000)
0025C858 001b:
	??_C@_0BL@MOACFICP@player?9?$DOdesired_angles?4yaw?$AA@ (0000)
0025C874 0006:
	??_C@_05NCGEDJPM@delta?$AA@ (0000)
002DE808 0600:
	_player_autoaim_flag (0000)
	_player_magnetism_flag (0001)
	_controls_swapped (0002)
	_player_look_zoomed_scale (0004)
0043EE30 002b:
	_bss_0043ee30 (0000)
	_debug_input_target (0004)
	_player_look_yaw_rate (0008)
	_player_look_pitch_rate (0018)
	_controls_enable_crouch (0028)
	_controls_enable_doubled_spin (0029)
	_controls_swap_doubled_spin_state (002a)
*/

/* ---------- headers */

#include "cseries.h"
#include "game/game.h"
#include "game/player_control.h"
#include "players.h"

#include "items/weapons.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"

#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

#define valid_euler_angles2d(angles) ( \
	valid_real((angles)->pitch) && \
	(angles)->pitch <= DEGREES_TO_RADIANS(85.5f) && \
	(angles)->pitch >= -DEGREES_TO_RADIANS(85.5f) && \
	valid_real((angles)->yaw) && \
	(angles)->yaw <= _pi * 2.f && \
	(angles)->yaw >= 0.f)

/* ---------- structures */

/* ---------- prototypes */

void player_aiming_vector_from_facing(
	long player_index,
	real_vector3d *facing_direction,
	real_euler_angles2d const *facing_angles);
real weapon_get_field_of_view(
	long weapon_index,
	real field_of_view,
	short zoom_level);

/* ---------- globals */

struct player_control_globals_data *bss_0043ee30;

/* ---------- public code */

void player_control_initialize(
	void)
{
	player_control_globals = game_state_malloc(
		"player control globals",
		NULL,
		sizeof(*player_control_globals));
	return;
}

struct player_control *player_control_get(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\game\\player_control.c", 0xB1,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	return &player_control_globals->players[local_player_index];
}

void player_control_dispose(
	void)
{
	return;
}

void player_control_dispose_from_old_map(
	void)
{
	return;
}

boolean code_000a5780(
	void)
{
	return (boolean)(!TEST_FLAG(
		player_control_globals->flags,
		_player_control_camera_control_disabled_bit) &&
		!game_time_get_paused());
}

boolean scripted_player_control_set_camera_control(
	boolean camera_control)
{
	SET_FLAG(
		player_control_globals->flags,
		_player_control_camera_control_disabled_bit,
		!camera_control);
	return camera_control;
}

void player_control_inhibit_buttons(
	short local_player_index,
	word action_flags,
	boolean persistent)
{
	struct player_control *control = player_control_get(local_player_index);

	control->inhibited_action_flags |= action_flags;
	if (persistent)
	{
		control->persistent_inhibited_action_flags |= action_flags;
	}
	return;
}

long player_control_get_target_object_index(
	short local_player_index)
{
	struct player_control *control = player_control_get(local_player_index);

	if (object_try_and_get(control->target_object_index))
	{
		return control->target_object_index;
	}
	return NONE;
}

real player_control_get_field_of_view(
	short local_player_index)
{
	struct player_control *control = player_control_get(local_player_index);
	real field_of_view = DEGREES_TO_RADIANS(70.f);

	if (control->unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(control->unit_index);
		struct unit_definition *definition = unit_definition_get(unit->definition_index);
		long weapon_index = unit_inventory_get_weapon(
			control->unit_index,
			unit->unit.current_weapon_index);

		if (weapon_index != NONE)
		{
			field_of_view = weapon_get_field_of_view(
				weapon_index,
				definition->unit.camera_field_of_view,
				control->zoom_level);
		}
		else
		{
			field_of_view = definition->unit.camera_field_of_view;
		}
	}
	return field_of_view;
}

void player_control_get_unit_camera_info(
	short local_player_index,
	struct player_control_unit_camera_info *camera_info)
{
	struct player_control *control;
	struct unit_datum *unit;

	match_assert("c:\\halo\\SOURCE\\game\\player_control.c", 0x402, camera_info);
	camera_info->camera = NULL;
	control = player_control_get(local_player_index);
	camera_info->unit_index = control->unit_index;
	camera_info->seat_index = NONE;

	if (camera_info->unit_index != NONE)
	{
		unit = unit_get(camera_info->unit_index);
		unit_get_camera_position(camera_info->unit_index, &camera_info->position);

		if (unit->object.parent_object_index != NONE)
		{
			struct unit_datum *vehicle = vehicle_try_and_get(
				unit->object.parent_object_index);

			if (vehicle)
			{
				struct unit_definition *definition = vehicle_definition_get(
					vehicle->definition_index);
				struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(
					&definition->unit.seats,
					unit->unit.parent_seat_index,
					struct unit_seat);

				camera_info->unit_index = unit->object.parent_object_index;
				camera_info->camera = &seat->camera;
				camera_info->seat_index = unit->unit.parent_seat_index;
				unit = unit_get(camera_info->unit_index);
			}
		}

		if (camera_info->seat_index == NONE)
		{
			camera_info->camera = &unit_definition_get(unit->definition_index)->unit.camera;
		}
	}
	return;
}

real evaluate_piecewise_linear_function(
	short count,
	real const *function,
	real input)
{
	boolean negate = TRUE;
	short low_index;
	short high_index;
	real result;

	if (!(input < 0.f))
	{
		negate = FALSE;
	}
	input = PIN(fabs(input) * (count - 1), 0.0, (real)count - 1.f);
	low_index = PIN((short)input, 0, count - 1);
	high_index = MIN(low_index + 1, count - 1);
	match_assert("c:\\halo\\SOURCE\\game\\player_control.c", 0x14B,
		function && low_index>=0 && low_index<=high_index && high_index<count);
	result = (function[high_index] - function[low_index]) *
		(input - low_index) + function[low_index];
	if (negate)
	{
		result = -result;
	}
	return result;
}

long player_control_get_unit_index(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\game\\player_control.c", 0xB1,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	return player_control_globals->players[local_player_index].unit_index;
}

short player_control_get_zoom_level(
	short local_player_index)
{
	short zoom_level = NONE;

	if (local_player_index != NONE)
	{
		zoom_level = player_control_get(local_player_index)->zoom_level;
	}
	return zoom_level;
}

float player_control_get_autoaim_level(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\game\\player_control.c", 0xB1,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	return player_control_globals->players[local_player_index].autoaim_level;
}

long player_control_get_desired_weapon(
	short local_player_index,
	long unit_index)
{
	struct player_control *control = player_control_get(local_player_index);
	long weapon_index = NONE;

	if (control->unit_index != unit_index ||
		(weapon_index = unit_inventory_get_weapon(
			unit_index,
			control->desired_weapon_index)) == NONE)
	{
		weapon_index = unit_inventory_get_weapon(
			unit_index,
			unit_get(unit_index)->unit.current_weapon_index);
	}
	return weapon_index;
}

void players_unzoom_all(
	void)
{
	short local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		player_control_get(local_player_index)->zoom_level = NONE;
	}
	return;
}

void player_control_unzoom(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	long player_index = unit->unit.player_index;

	if (player_index != NONE)
	{
		struct player_datum *player = player_get(player_index);

		if (player->local_player_index != NONE)
		{
			player_control_get(player->local_player_index)->zoom_level = NONE;
		}
	}
	return;
}

real_euler_angles2d const *player_control_get_facing_angles(
	short local_player_index)
{
	struct player_control *player = player_control_get(local_player_index);

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_control.c",
		0x3C0,
		valid_euler_angles2d(&player->desired_angles));
	return &player->desired_angles;
}

real_vector3d *player_control_get_facing_direction(
	short local_player_index,
	real_vector3d *facing_direction)
{
	player_aiming_vector_from_facing(
		local_player_get_player_index(local_player_index),
		facing_direction,
		player_control_get_facing_angles(local_player_index));
	return facing_direction;
}

void player_control_set_desired_weapon(
	long unit_index,
	short desired_weapon_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	if (unit->unit.player_index != NONE)
	{
		struct player_datum *player = player_get(unit->unit.player_index);

		if (player->local_player_index != NONE)
		{
			player_control_get(player->local_player_index)->desired_weapon_index =
				desired_weapon_index;
		}
	}
	return;
}

void player_control_set_facing(
	short local_player_index,
	real_vector3d const *facing_direction)
{
	struct player_control *player_control = player_control_get(local_player_index);

	euler_angles2d_from_vector3d(
		&player_control->desired_angles,
		facing_direction);
	match_assert_valid_real(
		"c:\\halo\\SOURCE\\game\\player_control.c",
		0xBB,
		player_control->desired_angles.pitch);
	match_assert_valid_real(
		"c:\\halo\\SOURCE\\game\\player_control.c",
		0xBC,
		player_control->desired_angles.yaw);
	if (player_control->desired_angles.yaw < 0.f)
	{
		player_control->desired_angles.yaw += _pi * 2.f;
	}
	return;
}

void player_control_new_unit(
	short local_player_index,
	long unit_index)
{
	struct player_control *control = player_control_get(local_player_index);

	csmemset(control, 0, sizeof(*control));
	control->unit_index = unit_index;
	control->desired_weapon_index = NONE;
	control->desired_grenade_index = NONE;
	control->zoom_level = NONE;
	control->unknown26 = FALSE;
	control->target_object_index = NONE;
	control->pitch_maximum = DEGREES_TO_RADIANS(85.5f);
	control->pitch_minimum = -DEGREES_TO_RADIANS(85.5f);
	control->inhibited_action_flags = 0;
	control->persistent_inhibited_action_flags = 0;

	if (unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(unit_index);

		euler_angles2d_from_vector3d(
			&control->desired_angles,
			&unit->unit.desired_facing_vector);
		if (control->desired_angles.yaw < 0.f)
		{
			control->desired_angles.yaw += _pi * 2.f;
		}
		control->desired_weapon_index = unit->unit.desired_weapon_index;
		control->desired_grenade_index = unit->unit.desired_grenade_index;
		control->zoom_level = unit->unit.desired_zoom_level;
	}
	return;
}

void player_control_action_test_reset(
	void)
{
	struct player_control_globals_data *globals = player_control_globals;

	globals->action_flags = 0;
	globals->action_test_flags = 0;
	return;
}

boolean player_control_action_test_accept(
	void)
{
	struct player_control_globals_data *globals = player_control_globals;

	globals->action_test_flags |= FLAG(_player_control_accept_bit);
	globals->suppressed_action_flags |= FLAG(_player_control_accept_bit);
	return TEST_FLAG(
		globals->action_flags,
		_player_control_accept_bit);
}

boolean player_control_action_test_back(
	void)
{
	struct player_control_globals_data *globals = player_control_globals;

	globals->action_test_flags |= FLAG(_player_control_back_bit);
	globals->suppressed_action_flags |= FLAG(_player_control_back_bit);
	return TEST_FLAG(
		globals->action_flags,
		_player_control_back_bit);
}

boolean player_control_action_test_action(
	void)
{
	struct player_control_globals_data *globals = player_control_globals;

	globals->action_test_flags |= FLAG(_player_control_action_bit);
	globals->suppressed_action_flags |= FLAG(_player_control_action_bit);
	return TEST_FLAG(
		globals->action_flags,
		_player_control_action_bit);
}

boolean player_control_action_test_jump(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_jump_bit);
}

boolean player_control_action_test_primary_trigger(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_primary_trigger_bit);
}

boolean player_control_action_test_grenade_trigger(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_grenade_trigger_bit);
}

boolean player_control_action_test_zoom(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_zoom_bit);
}

boolean player_control_action_test_look_relative_left(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_look_relative_left_bit);
}

boolean player_control_action_test_look_relative_right(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_look_relative_right_bit);
}

boolean player_control_action_test_look_relative_up(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_look_relative_up_bit);
}

boolean player_control_action_test_look_relative_down(
	void)
{
	return TEST_FLAG(
		player_control_globals->action_flags,
		_player_control_look_relative_down_bit);
}

boolean player_control_action_test_move_relative_all_directions(
	void)
{
	return !(~player_control_globals->action_flags &
		_player_control_move_relative_all_directions_flags);
}

boolean player_control_action_test_look_relative_all_directions(
	void)
{
	return !(~player_control_globals->action_flags &
		_player_control_look_relative_all_directions_flags);
}

/* ---------- private code */
