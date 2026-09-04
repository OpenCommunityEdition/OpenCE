/*
HUD_UNIT.C

symbols in this file:
000C6590 0010:
	_code_000c6590 (0000)
000C65A0 0010:
	_code_000c65a0 (0000)
000C65B0 0010:
	_unit_hud_shield_meter_mapper_init (0000)
000C65C0 0040:
	_code_000c65c0 (0000)
000C6600 0070:
	_get_hud_state (0000)
000C6670 0040:
	_hud_initialize_unit_interface (0000)
000C66B0 00f0:
	_hud_initialize_unit_interface_for_new_map (0000)
000C67A0 0010:
	_hud_dispose_unit_interface_from_old_map (0000)
000C67B0 0010:
	_hud_dispose_unit_interface (0000)
000C67C0 0030:
	_scripted_hud_show_health (0000)
000C67F0 0030:
	_scripted_hud_blink_health (0000)
000C6820 0030:
	_scripted_hud_show_shield (0000)
000C6850 0030:
	_scripted_hud_blink_shield (0000)
000C6880 0030:
	_scripted_hud_show_motion_sensor (0000)
000C68B0 0030:
	_scripted_hud_blink_motion_sensor (0000)
000C68E0 0220:
	_hud_play_unit_sounds (0000)
000C6B00 0080:
	_hud_fix_unit_data (0000)
000C6B80 0220:
	_hud_update_unit_local_player (0000)
000C6DA0 02b0:
	_hud_render_damage_indicators (0000)
000C7050 0040:
	_hud_tick_shield (0000)
000C7090 0030:
	_hud_update_unit (0000)
000C70C0 0db0:
	_hud_render_unit_interface (0000)
00270268 0011:
	??_C@_0BB@MIFNILGL@unit_hud_globals?$AA@ (0000)
0027027C 0024:
	??_C@_0CE@ICNCJJA@c?3?2halo?2SOURCE?2interface?2hud_uni@ (0000)
002702A0 0013:
	??_C@_0BD@OOGPMJCA@hud?5unit?5interface?$AA@ (0000)
002702B4 0004:
	__real@3e400000 (0000)
002702B8 001d:
	??_C@_0BN@FNBCLDEB@new_local_player_index?$CB?$DNNONE?$AA@ (0000)
002702D8 001d:
	??_C@_0BN@NPOPPJAG@old_local_player_index?$CB?$DNNONE?$AA@ (0000)
002E4C68 0004:
	_data_002e4c68 (0000)
00453AC0 0004:
	_unit_hud_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cutscene/cinematics.h"
#include "effects/player_effects.h"
#include "game/game.h"
#include "game/players.h"
#include "cache/texture_cache.h"
#include "interface/hud_definitions.h"
#include "interface/hud_draw.h"
#include "interface/hud_unit.h"
#include "render/render.h"
#include "saved games/game_state.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_hud_panel_health_dont_show_bit = 0,
	_hud_panel_health_blink_bit,
	_hud_panel_shield_dont_show_bit,
	_hud_panel_shield_blink_bit,
	_hud_panel_motion_sensor_dont_show_bit,
	_hud_panel_motion_sensor_blink_bit
};

enum
{
	MAXIMUM_NUMBER_OF_LOCAL_PLAYERS = 4,
};

enum hud_damage_indicator_direction
{
	_hud_damage_indicator_top = 0,
	_hud_damage_indicator_left,
	_hud_damage_indicator_bottom,
	_hud_damage_indicator_right,

	NUMBER_OF_HUD_DAMAGE_INDICATOR_DIRECTIONS
};

enum
{
	STACK_BUFFER_LENGTH = 0x80,
	STACK_BUFFER_FILL = 0x62626262,
};

/* ---------- macros */

/* ---------- structures */

struct unit_hud_state
{
	real last_shield_vitality;
	real last_body_vitality;
	real fade_time;
	long last_shield_hit_time;
	long last_shield_flash_time;
	long last_health_flash_time;
	long last_motion_sensor_flash_time;
	long last_unit_index;
	word auxilary_active_type_flags;
	short auxilary_flash_time[1];
	word sound_flags;
	byte pad26[2];
	long last_sound_handles[12];
};

struct unit_hud_globals
{
	struct unit_hud_state hud_states[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	long script_flags;
};

struct hud_scripted_globals
{
	boolean show_hud;
	boolean show_hud_help_text;
	byte reserved2[2];
};

struct hud_absolute_placement_definition
{
	short corner;
	short pad;
	long unused[8];
};

struct hud_placement_definition
{
	point2d offset;
	real_vector2d scale;
	short multiplayer_scaling_flags;
	short pad;
	long unused0[5];
};

struct hud_color_definition
{
	unsigned long color;
	unsigned long flash_color;
	real flash_period;
	real flash_delay;
	short number_of_flashes;
	unsigned short flash_flags;
	real flash_length;
	unsigned long disabled_color;
	union
	{
		long unused;
		struct
		{
			short up_ticks;
			short fade_ticks;
		} objective;
	} custom;
};

struct hud_messaging_parameters_definition
{
	struct hud_absolute_placement_definition absolute_placement;
	struct hud_placement_definition placement;
	struct tag_reference single_player_font;
	struct tag_reference multi_player_font;
	real up_time;
	real fade_time;
	real_argb_color state_color;
	real_argb_color text_color;
	real spacing;
	struct tag_reference hud_item_messages;
	struct tag_reference messaging_icons;
	struct tag_reference alternate_icon_text;
	struct tag_block button_icons;
	struct hud_color_definition color;
	struct tag_reference hud_messages;
	struct hud_color_definition objective_color;
};

struct hud_waypoint_definition
{
	real top_offset;
	real bottom_offset;
	real left_offset;
	real right_offset;
	long unused0[8];
	struct tag_reference arrow_bitmap;
	struct tag_block arrows;
	long unused1[20];
};

struct hud_multiplayer_parameters_definition
{
	real hud_scale;
	long unused[0x40];
};

struct hud_defaults_definition
{
	struct tag_reference default_weapon_hud;
	real motion_sensor_range;
	real motion_sensor_velocity_sensitivity;
	real motion_sensor_scale;
	rectangle2d default_title_bounds;
	long unused[0xB];
};

struct hud_damage_indicators_definition
{
	short top_offset;
	short bottom_offset;
	short left_offset;
	short right_offset;
	long unused[8];
	struct tag_reference indicator_bitmap;
	short sequence_index;
	short multiplayer_sequence_index;
	unsigned long color;
	long unused2[4];
};

struct hud_globals_definition
{
	struct hud_messaging_parameters_definition messaging;
	struct hud_waypoint_definition waypoint;
	struct hud_multiplayer_parameters_definition multiplayer;
	struct hud_defaults_definition defaults;
	struct hud_damage_indicators_definition damage_indicators;
};

typedef char unit_hud_state_auxilary_flash_time_offset_assert[
	offsetof(struct unit_hud_state, auxilary_flash_time) == 0x22 ? 1 : -1];
typedef char unit_hud_state_sound_flags_offset_assert[
	offsetof(struct unit_hud_state, sound_flags) == 0x24 ? 1 : -1];
typedef char unit_hud_state_last_sound_handles_offset_assert[
	offsetof(struct unit_hud_state, last_sound_handles) == 0x28 ? 1 : -1];
typedef char unit_hud_state_size_assert[
	sizeof(struct unit_hud_state) == 0x58 ? 1 : -1];
typedef char unit_hud_globals_script_flags_offset_assert[
	offsetof(struct unit_hud_globals, script_flags) == 0x160 ? 1 : -1];
typedef char unit_hud_globals_size_assert[
	sizeof(struct unit_hud_globals) == 0x164 ? 1 : -1];
typedef char hud_scripted_globals_size_assert[
	sizeof(struct hud_scripted_globals) == 0x4 ? 1 : -1];
typedef char hud_absolute_placement_definition_size_assert[
	sizeof(struct hud_absolute_placement_definition) == 0x24 ? 1 : -1];
typedef char hud_placement_definition_size_assert[
	sizeof(struct hud_placement_definition) == 0x24 ? 1 : -1];
typedef char hud_color_definition_size_assert[
	sizeof(struct hud_color_definition) == 0x20 ? 1 : -1];
typedef char hud_messaging_parameters_definition_size_assert[
	sizeof(struct hud_messaging_parameters_definition) == 0x120 ? 1 : -1];
typedef char hud_waypoint_definition_size_assert[
	sizeof(struct hud_waypoint_definition) == 0x9C ? 1 : -1];
typedef char hud_multiplayer_parameters_definition_size_assert[
	sizeof(struct hud_multiplayer_parameters_definition) == 0x104 ? 1 : -1];
typedef char hud_defaults_definition_size_assert[
	sizeof(struct hud_defaults_definition) == 0x50 ? 1 : -1];
typedef char hud_damage_indicators_definition_indicator_bitmap_offset_assert[
	offsetof(struct hud_damage_indicators_definition, indicator_bitmap) == 0x28 ? 1 : -1];
typedef char hud_damage_indicators_definition_size_assert[
	sizeof(struct hud_damage_indicators_definition) == 0x50 ? 1 : -1];
typedef char hud_globals_definition_damage_indicators_offset_assert[
	offsetof(struct hud_globals_definition, damage_indicators) == 0x310 ? 1 : -1];

/* ---------- prototypes */

static struct unit_hud_state *get_hud_state(
	short local_player_index);
static void hud_update_unit_local_player(
	short local_player_index);

/* ---------- globals */

static struct unit_hud_globals *unit_hud_globals = NULL;
extern struct hud_scripted_globals *hud_scripted_globals;
extern struct hud_globals_definition *hud_globals;

/* ---------- public code */

void code_000c6590(
	void)
{
	return;
}

void code_000c65a0(
	void)
{
	return;
}

void unit_hud_shield_meter_mapper_init(
	void)
{
	return;
}

/* ---------- private code */

static struct unit_hud_state *get_hud_state(
	short local_player_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x106,
		local_player_index>=0 &&
			local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x107,
		unit_hud_globals);

	return &unit_hud_globals->hud_states[local_player_index];
}

/* ---------- public code */

void hud_initialize_unit_interface(
	void)
{
	unit_hud_globals = game_state_malloc(
		"hud unit interface",
		NULL,
		sizeof(*unit_hud_globals));

	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x110,
		unit_hud_globals);

	return;
}

void hud_initialize_unit_interface_for_new_map(
	void)
{
	short local_player_index;
	struct unit_hud_state *hud_state;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x11B,
		unit_hud_globals);
	csmemset(
		unit_hud_globals,
		0,
		sizeof(*unit_hud_globals));

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		match_assert(
			"c:\\halo\\SOURCE\\interface\\hud_unit.c",
			0x106,
			local_player_index>=0 &&
				local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
		match_assert(
			"c:\\halo\\SOURCE\\interface\\hud_unit.c",
			0x107,
			unit_hud_globals);

		hud_state = &unit_hud_globals->hud_states[local_player_index];
		csmemset(
			hud_state->auxilary_flash_time,
			NONE,
			sizeof(hud_state->auxilary_flash_time));
		hud_state->last_body_vitality = -1.0f;
		hud_state->last_shield_vitality = -1.0f;
		hud_state->last_health_flash_time = NONE;
		hud_state->last_motion_sensor_flash_time = NONE;
		hud_state->fade_time = -1.0f;
		hud_state->last_unit_index = NONE;
		hud_state->sound_flags = 0;
		csmemset(
			hud_state->last_sound_handles,
			NONE,
			sizeof(hud_state->last_sound_handles));
	}

	return;
}

void hud_dispose_unit_interface_from_old_map(
	void)
{
	return;
}

void hud_dispose_unit_interface(
	void)
{
	return;
}

void hud_fix_unit_data(
	short old_local_player_index,
	short new_local_player_index)
{
	struct unit_hud_state *old_hud_state;
	struct unit_hud_state *new_hud_state;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x1AB,
		old_local_player_index!=NONE);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_unit.c",
		0x1AC,
		new_local_player_index!=NONE);
	old_hud_state = get_hud_state(old_local_player_index);
	new_hud_state = get_hud_state(new_local_player_index);
	*new_hud_state = *old_hud_state;

	return;
}

void scripted_hud_show_health(
	boolean show)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_health_dont_show_bit,
		!show);

	return;
}

void scripted_hud_blink_health(
	boolean blink)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_health_blink_bit,
		blink);

	return;
}

void scripted_hud_show_shield(
	boolean show)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_shield_dont_show_bit,
		!show);

	return;
}

void scripted_hud_blink_shield(
	boolean blink)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_shield_blink_bit,
		blink);

	return;
}

void scripted_hud_show_motion_sensor(
	boolean show)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_motion_sensor_dont_show_bit,
		!show);

	return;
}

void scripted_hud_blink_motion_sensor(
	boolean blink)
{
	SET_FLAG(
		unit_hud_globals->script_flags,
		_hud_panel_motion_sensor_blink_bit,
		blink);

	return;
}

/* ---------- private code */

static void hud_update_unit_local_player(
	short local_player_index)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	if (local_player_get_player_index(local_player_index) != NONE)
	{
		long unit_index = player_get(
			local_player_get_player_index(local_player_index))->unit_index;

		if (unit_index != NONE)
		{
			struct unit_datum *unit = unit_get(unit_index);
			struct unit_hud_state *hud_state = get_hud_state(local_player_index);

			if (hud_state->last_body_vitality == -1.0f)
				hud_state->last_body_vitality = unit->object.body_vitality;
			if (hud_state->last_shield_vitality == -1.0f)
				hud_state->last_shield_vitality = unit->object.shield_vitality;

			if (hud_state->last_shield_vitality > unit->object.shield_vitality)
			{
				if (hud_state->fade_time < 0.0f || hud_state->fade_time > 1.0f)
					hud_state->last_shield_hit_time = game_time_get();

				if (game_time_get() - hud_state->last_shield_hit_time < 15)
				{
					hud_state->fade_time = 0.0f;
					goto update_finished;
				}

				hud_state->last_shield_vitality = unit->object.shield_vitality;
				hud_state->fade_time +=
					(real)(game_time_get() - hud_state->last_shield_hit_time) *
					(1.0f / TICKS_PER_SECOND);
			}
			else
			{
				if (hud_state->last_shield_vitality < unit->object.shield_vitality)
				{
					hud_state->last_shield_vitality = unit->object.shield_vitality;
					hud_state->fade_time = -1.0f;
				}
				else
				{
					hud_state->last_shield_vitality = unit->object.shield_vitality;
					if (hud_state->fade_time > 0.0f)
					{
						hud_state->fade_time +=
							(real)(game_time_get() - hud_state->last_shield_hit_time) *
							(1.0f / TICKS_PER_SECOND);
					}
				}
			}

			hud_state->last_shield_hit_time = game_time_get();
		}
	}

update_finished:
	if (cinematic_in_progress())
	{
		long player_index = local_player_get_player_index(local_player_index);

		if (player_index != NONE)
		{
			hud_play_unit_sounds(
				player_get(player_index),
				hud_scripted_globals->show_hud);
		}
	}

	{
		short corrupt_index;
		short buffer_index;

		for (buffer_index = STACK_BUFFER_LENGTH - 1; buffer_index >= 0; buffer_index--)
		{
			if (stack_buffer[buffer_index] != STACK_BUFFER_FILL)
				goto corrupt_stack_found;
		}

		corrupt_index = NONE;
		goto stack_buffer_checked;

corrupt_stack_found:
		corrupt_index = buffer_index;

stack_buffer_checked:
		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_unit.c",
			0x201,
			return_eip == get_return_eip(),
			"corrupt return address!");
		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_unit.c",
			0x201,
			corrupt_index == NONE,
			csprintf(temporary, "corrupt stack at %d!", corrupt_index));
	}

	return;
}

/* ---------- public code */

void hud_render_damage_indicators(
	short local_player_index)
{
	long unit_index;

	if (local_player_index == NONE)
		return;

	if (local_player_get_player_index(local_player_index) == NONE)
	{
		unit_index = NONE;
	}
	else
	{
		unit_index = player_get(
			local_player_get_player_index(local_player_index))->unit_index;
	}

	if (unit_try_and_get(unit_index))
	{
		struct hud_damage_indicators_definition *definition =
			&hud_globals->damage_indicators;
		real theta;
		real_point2d screen_position;
		real scale = hud_globals_get_scale(local_player_count() > 1);
		byte damage_indicators[NUMBER_OF_HUD_DAMAGE_INDICATOR_DIRECTIONS];
		short direction;

		player_effect_get_damage_indicators(
			local_player_index,
			damage_indicators);

		for (direction = 0;
			direction < NUMBER_OF_HUD_DAMAGE_INDICATOR_DIRECTIONS;
			direction++)
		{
			if (damage_indicators[direction] > 0 &&
				damage_indicators[direction] < TICKS_PER_SECOND)
			{
				long bitmap_group_index;
				short sequence_index;
				struct bitmap_data *bitmap;
				real_rectangle2d const *clip;

				switch (direction)
				{
				case _hud_damage_indicator_top:
					theta = _pi;
					screen_position.y = (real)(
						render.camera.window_bounds.y0 +
						definition->top_offset);
					screen_position.x = (real)(
						(render.camera.viewport_bounds.x0 +
							render.camera.viewport_bounds.x1) / 2);
					break;

				case _hud_damage_indicator_left:
					theta = _pi / 2.0f;
					screen_position.x = (real)(
						render.camera.window_bounds.x0 +
						definition->left_offset);
					screen_position.y = (real)(
						(render.camera.viewport_bounds.y0 +
							render.camera.viewport_bounds.y1) / 2);
					break;

				case _hud_damage_indicator_bottom:
					theta = 0.0f;
					screen_position.y = (real)(
						render.camera.window_bounds.y1 -
						definition->bottom_offset);
					screen_position.x = (real)(
						(render.camera.viewport_bounds.x0 +
							render.camera.viewport_bounds.x1) / 2);
					break;

				case _hud_damage_indicator_right:
					theta = 3.0f * _pi / 2.0f;
					screen_position.x = (real)(
						render.camera.window_bounds.x1 -
						definition->right_offset);
					screen_position.y = (real)(
						(render.camera.viewport_bounds.y0 +
							render.camera.viewport_bounds.y1) / 2);
					break;

				default:
					match_assert(
						"c:\\halo\\SOURCE\\interface\\hud_unit.c",
						0x400,
						!"unreachable");
					break;
				}

				screen_position.x -= (real)render.camera.viewport_bounds.x0;
				screen_position.y -= (real)render.camera.viewport_bounds.y0;
				bitmap_group_index = definition->indicator_bitmap.index;
				if (local_player_count() > 1)
					sequence_index = definition->multiplayer_sequence_index;
				else
					sequence_index = definition->sequence_index;
				bitmap = NULL;
				clip = NULL;
				hud_retrieve_bitmap_and_bounding_rect(
					bitmap_group_index,
					sequence_index,
					0,
					&bitmap,
					&clip);

				if (bitmap &&
					_texture_cache_bitmap_get_hardware_format(
						bitmap,
						FALSE,
						TRUE))
				{
					point2d corner;

					corner.x = (short)(long)screen_position.x;
					corner.y = (short)(long)screen_position.y;
					hud_draw_bitmap_direct(
						bitmap,
						_hud_anchor_center,
						&corner,
						clip,
						scale,
						theta,
						definition->color,
						FALSE);
				}
			}
		}
	}
	else
	{
		player_effect_clear_damage_indicators(local_player_index);
	}

	return;
}

/* ---------- public code */

void hud_tick_shield(
	long player_index,
	real amount)
{
	short local_player_index = player_get(player_index)->local_player_index;

	if (local_player_index != NONE)
		get_hud_state(local_player_index)->last_shield_vitality -= amount;

	return;
}

void hud_update_unit(
	void)
{
	short local_player_index = local_player_get_next(NONE);

	while (local_player_index != NONE)
	{
		hud_update_unit_local_player(local_player_index);
		local_player_index = local_player_get_next(local_player_index);
	}

	return;
}

/* ---------- private code */
