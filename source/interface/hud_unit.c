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
#include "game/game.h"
#include "game/players.h"
#include "interface/hud_draw.h"
#include "interface/hud_unit.h"
#include "saved games/game_state.h"
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

/* ---------- prototypes */

static struct unit_hud_state *get_hud_state(
	short local_player_index);
static void hud_update_unit_local_player(
	short local_player_index);

/* ---------- globals */

static struct unit_hud_globals *unit_hud_globals = NULL;
extern struct hud_scripted_globals *hud_scripted_globals;

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
