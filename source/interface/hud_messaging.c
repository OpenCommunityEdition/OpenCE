/*
HUD_MESSAGING.C

symbols in this file:
000C3810 0080:
	_render_state_text (0000)
000C3890 0190:
	_render_state_bitmap (0000)
000C3A20 0020:
	_hud_messaging_initialize (0000)
000C3A40 0030:
	_hud_messaging_initialize_for_new_map (0000)
000C3A70 0010:
	_hud_messaging_dispose_from_old_map (0000)
000C3A80 0010:
	_hud_messaging_dispose (0000)
000C3A90 0050:
	_scripted_hud_set_state_message (0000)
000C3AE0 0040:
	_scripted_hud_set_flashing_state (0000)
000C3B20 0040:
	_scripted_hud_restart_flashing (0000)
000C3B60 00a0:
	_scripted_hud_set_objective (0000)
000C3C00 0080:
	_scripted_hud_set_timer_time (0000)
000C3C80 0020:
	_scripted_hud_set_timer_warning_cutoff (0000)
000C3CA0 0060:
	_scripted_hud_set_timer_position (0000)
000C3D00 0020:
	_scripted_hud_show_timer (0000)
000C3D20 0050:
	_scripted_hud_pause_timer (0000)
000C3D70 0050:
	_scripted_hud_get_timer_ticks (0000)
000C3DC0 0030:
	_scripted_hud_time_code_show (0000)
000C3DF0 0040:
	_scripted_hud_time_code_start (0000)
000C3E30 0020:
	_scripted_hud_time_code_reset (0000)
000C3E50 02c0:
	_hud_render_timer (0000)
000C4110 00a0:
	_hud_set_state_message (0000)
000C41B0 0060:
	_hud_set_state_message_icon (0000)
000C4210 0070:
	_hud_set_state_message_text (0000)
000C4280 0070:
	_hud_enable_custom_state_message (0000)
000C42F0 0040:
	_hud_set_state_text (0000)
000C4330 00c0:
	_hud_messaging_get_objective (0000)
000C43F0 0080:
	_find_free_hud_message (0000)
000C4470 0030:
	_compare_messages (0000)
000C44A0 0040:
	_scripted_hud_messages_clear (0000)
000C44E0 0020:
	_hud_get_font_index (0000)
000C4500 0030:
	_hud_get_text_color (0000)
000C4530 0010:
	_hud_messaging_globals_update (0000)
000C4540 0080:
	_hud_print_message (0000)
000C45C0 00a0:
	_hud_add_item_message (0000)
000C4660 0070:
	_hud_broadcast_team_message (0000)
000C46D0 0b70:
	_hud_messaging_update (0000)
0026FEE4 000e:
	??_C@_0O@JAAAMDJP@hud?5messaging?$AA@ (0000)
0026FEF4 003f:
	??_C@_0DP@DEHGFPPE@trying?5to?5restart?5help?5text?5flas@ (0000)
0026FF34 002b:
	??_C@_0CL@KDPBOCMB@objective?5text?5MUST?5only?5be?5text@ (0000)
0026FF60 0008:
	__real@4004000000000000 (0000)
0026FF68 0029:
	??_C@_0CJ@FOFBAHGJ@c?3?2halo?2SOURCE?2interface?2hud_mes@ (0000)
0026FF94 0026:
	??_C@_0CG@FLCOGOLF@element?9?$DOtype?$DN?$DN_hud_message_type@ (0000)
0026FFBC 001a:
	??_C@_0BK@PKALLFKO@message?9?$DOelement_count?$DN?$DN1?$AA@ (0000)
0026FFD8 0022:
	??_C@_1CC@FMBGLMAK@?$AA?$DM?$AAn?$AAo?$AA?5?$AAb?$AAu?$AAt?$AAt?$AAo?$AAn?$AA?5?$AAi?$AAc?$AAo?$AAn?$AA?$DO?$AA?$AA@ (0000)
0026FFFC 0022:
	??_C@_0CC@OCKONGBA@help?5text?5cannot?5use?5custom?5icon@ (0000)
00270020 0037:
	??_C@_0DH@PPCCMLMB@help?5message?5using?5old?5code?4?5get@ (0000)
00270058 0014:
	??_C@_1BE@ELICABCK@?$AA?$DM?$AAu?$AAn?$AAk?$AAn?$AAo?$AAw?$AAn?$AA?$DO?$AA?$AA@ (0000)
0027006C 0037:
	??_C@_0DH@GJFOKBEC@global_scenario_get?$CI?$CJ?9?$DOcustom_ob@ (0000)
002700A4 0028:
	??_C@_0CI@PMMKILCE@custom_index?$DMNUMBER_OF_HUD_CUSTO@ (0000)
002700CC 0013:
	??_C@_0BD@LPFKFODL@show_state_message?$AA@ (0000)
002700E0 0054:
	??_C@_0FE@EGLPDJAB@hud_messaging_globals?9?$DOobjective@ (0000)
002E4C4C 001c:
	_button_mappings (0000)
	_time_code_time (0010)
	_time_code_stop_time (0014)
00453AB8 0004:
	_hud_messaging_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#include "cseries/errors.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "interface/hud_draw.h"
#include "interface/hud_messaging.h"
#include "interface/interface.h"
#include "render/render.h"
#include "render/render_debug.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "text/unicode.h"

#include <stddef.h>

/* ---------- constants */

enum
{
	hud_message_text_group_tag = 'hmt ',
	hud_number_group_tag = 'hud#',
	NUMBER_OF_HUD_MESSAGING_DATUMS = 4,
	NUMBER_OF_HUD_MESSAGES_PER_DATUM = 4,
	MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH = 256
};

enum hud_corner
{
	_hud_corner_top_left,
	_hud_corner_top_right,
	_hud_corner_bottom_left,
	_hud_corner_bottom_right,
	_hud_corner_center,
	NUMBER_OF_HUD_CORNERS
};

enum hud_number_show_flags
{
	_hud_number_show_all_leading_zeros_bit,
	_hud_number_show_only_when_zoomed_bit,
	_hud_number_show_trailing_m_bit,
	NUMBER_OF_HUD_NUMBER_SHOW_FLAGS
};

/* ---------- macros */

#define HUD_MESSAGE_TEXT_DEFINITION_GET(index) \
	((struct hud_message_text_definition *)tag_get(hud_message_text_group_tag, (index)))
#define HUD_NUMBER_DEFINITION_GET(index) \
	((struct hud_number_definition *)tag_get(hud_number_group_tag, (index)))

/* ---------- structures */

struct hud_message_definition
{
	long time;
	wchar_t text[63];
	boolean valid;
	boolean magic_number;
	long item_definition_index;
	short quantity;
	char message_offset;
	byte pad8B;
};

struct hud_state_message_definition
{
	char name[32];
	word text_start_index;
	word element_start_index;
	byte element_count;
	byte pad25[3];
	long unused28[6];
};

enum
{
	_hud_message_type_text,
	_hud_message_type_icon,
	NUMBER_OF_HUD_MESSAGE_TYPES
};

struct hud_state_message_element
{
	byte type;
	byte data;
};

struct icon_hud_element_definition;

struct hud_state_message_text_info_definition
{
	short string_index;
	boolean uses_scenario_names;
	byte reserved3;
};

union hud_state_message_info_definition
{
	struct icon_hud_element_definition const *icon;
	struct hud_state_message_text_info_definition text;
};

struct hud_state_message_runtime_definition
{
	wchar_t message_buffer[MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH];
	byte reserved200[4];
	union hud_state_message_info_definition info[8];
	struct hud_state_message_definition *state_message;
	boolean valid;
	byte is_text_flags;
	byte reserved22A[2];
};

struct hud_messaging_datum_definition
{
	struct hud_message_definition messages[NUMBER_OF_HUD_MESSAGES_PER_DATUM];
	struct hud_state_message_runtime_definition state_message;
	short first_message;
	boolean leave_first_line_blank;
	boolean custom_message;
};

struct hud_message_text_definition
{
	struct tag_data text_data;
	struct tag_block elements;
	struct tag_block messages;
	long unused2C[21];
};

struct hud_scripted_globals_definition
{
	boolean show_hud;
	boolean show_hud_help_text;
	byte reserved2[2];
};

struct hud_timer_data_definition
{
	long reference_time;
	short ticks;
	short flash_cutoff;
	point2d position;
	short corner;
	boolean paused;
	boolean enabled;
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
	long unused[5];
};

struct hud_objective_runtime_definition
{
	struct hud_state_message_definition *message;
	short uptime;
	short pad6;
};

struct hud_messaging_globals_definition
{
	struct hud_messaging_datum_definition message_data[NUMBER_OF_HUD_MESSAGING_DATUMS];
	long flash_start_time;
	boolean use_flash;
	byte magic_number;
	byte reserved1186[6];
	struct hud_state_message_definition *help_message;
	struct hud_objective_runtime_definition objective;
	struct hud_timer_data_definition timer;
};

struct hud_color_definition
{
	unsigned long color;
	unsigned long flash_color;
	real flash_period;
	real flash_delay;
	short number_of_flashes;
	word flash_flags;
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

struct number_hud_element_definition
{
	struct hud_placement_definition placement;
	struct hud_color_definition colors;
	char digits;
	byte number_flags;
	char fractional_digits;
	byte pad;
	long unused[3];
};

struct hud_number_definition
{
	struct tag_reference number_bitmap;
	char character_width;
	char screen_width;
	char x_offset;
	char y_offset;
	char decimal_point_width;
	char colon_width;
	short pad;
	long unused[19];
};

struct hud_timer_definition
{
	struct hud_color_definition color;
	struct hud_color_definition time_up_color;
	long unused[10];
};

struct hud_messaging_parameters_definition
{
	byte reserved000[0x48];
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

struct hud_globals_definition
{
	struct hud_messaging_parameters_definition messaging;
	byte reserved120[0x240];
	struct hud_timer_definition timer_definition;
};

typedef char hud_timer_data_size_assert[
	sizeof(struct hud_timer_data_definition) == 0x10 ? 1 : -1];
typedef char hud_message_valid_offset_assert[
	offsetof(struct hud_message_definition, valid) == 0x82 ? 1 : -1];
typedef char hud_message_size_assert[
	sizeof(struct hud_message_definition) == 0x8C ? 1 : -1];
typedef char hud_state_message_text_info_size_assert[
	sizeof(struct hud_state_message_text_info_definition) == 4 ? 1 : -1];
typedef char hud_state_message_info_size_assert[
	sizeof(union hud_state_message_info_definition) == 4 ? 1 : -1];
typedef char hud_state_message_runtime_size_assert[
	sizeof(struct hud_state_message_runtime_definition) == 0x22C ? 1 : -1];
typedef char hud_state_message_runtime_info_offset_assert[
	offsetof(struct hud_state_message_runtime_definition, info) == 0x204 ? 1 : -1];
typedef char hud_state_message_runtime_state_message_offset_assert[
	offsetof(struct hud_state_message_runtime_definition, state_message) == 0x224 ? 1 : -1];
typedef char hud_state_message_runtime_valid_offset_assert[
	offsetof(struct hud_state_message_runtime_definition, valid) == 0x228 ? 1 : -1];
typedef char hud_state_message_runtime_is_text_flags_offset_assert[
	offsetof(struct hud_state_message_runtime_definition, is_text_flags) == 0x229 ? 1 : -1];
typedef char hud_messaging_datum_state_message_offset_assert[
	offsetof(struct hud_messaging_datum_definition, state_message) == 0x230 ? 1 : -1];
typedef char hud_messaging_datum_leave_first_line_blank_offset_assert[
	offsetof(struct hud_messaging_datum_definition, leave_first_line_blank) == 0x45E ? 1 : -1];
typedef char hud_messaging_datum_custom_message_offset_assert[
	offsetof(struct hud_messaging_datum_definition, custom_message) == 0x45F ? 1 : -1];
typedef char hud_messaging_datum_size_assert[
	sizeof(struct hud_messaging_datum_definition) == 0x460 ? 1 : -1];
typedef char hud_messaging_globals_size_assert[
	sizeof(struct hud_messaging_globals_definition) == 0x11A8 ? 1 : -1];
typedef char hud_messaging_flash_start_time_offset_assert[
	offsetof(struct hud_messaging_globals_definition, flash_start_time) == 0x1180 ? 1 : -1];
typedef char hud_messaging_use_flash_offset_assert[
	offsetof(struct hud_messaging_globals_definition, use_flash) == 0x1184 ? 1 : -1];
typedef char hud_messaging_magic_number_offset_assert[
	offsetof(struct hud_messaging_globals_definition, magic_number) == 0x1185 ? 1 : -1];
typedef char hud_messaging_help_message_offset_assert[
	offsetof(struct hud_messaging_globals_definition, help_message) == 0x118C ? 1 : -1];
typedef char hud_messaging_timer_offset_assert[
	offsetof(struct hud_messaging_globals_definition, timer) == 0x1198 ? 1 : -1];
typedef char hud_messaging_timer_flash_cutoff_offset_assert[
	offsetof(struct hud_messaging_globals_definition, timer.flash_cutoff) == 0x119E ? 1 : -1];
typedef char hud_messaging_timer_enabled_offset_assert[
	offsetof(struct hud_messaging_globals_definition, timer.enabled) == 0x11A7 ? 1 : -1];
typedef char hud_messaging_parameters_size_assert[
	sizeof(struct hud_messaging_parameters_definition) == 0x120 ? 1 : -1];
typedef char hud_messaging_single_player_font_index_offset_assert[
	offsetof(struct hud_messaging_parameters_definition, single_player_font.index) == 0x54 ? 1 : -1];
typedef char hud_messaging_multi_player_font_index_offset_assert[
	offsetof(struct hud_messaging_parameters_definition, multi_player_font.index) == 0x64 ? 1 : -1];
typedef char hud_messaging_state_color_offset_assert[
	offsetof(struct hud_messaging_parameters_definition, state_color) == 0x70 ? 1 : -1];
typedef char hud_globals_messaging_offset_assert[
	offsetof(struct hud_globals_definition, messaging) == 0 ? 1 : -1];
typedef char hud_absolute_placement_size_assert[
	sizeof(struct hud_absolute_placement_definition) == 0x24 ? 1 : -1];
typedef char hud_placement_size_assert[
	sizeof(struct hud_placement_definition) == 0x24 ? 1 : -1];
typedef char number_hud_element_size_assert[
	sizeof(struct number_hud_element_definition) == 0x54 ? 1 : -1];
typedef char hud_number_size_assert[
	sizeof(struct hud_number_definition) == 0x64 ? 1 : -1];
typedef char hud_globals_timer_definition_offset_assert[
	offsetof(struct hud_globals_definition, timer_definition) == 0x360 ? 1 : -1];

/* ---------- prototypes */

static struct hud_message_definition *find_free_hud_message(
	struct hud_messaging_datum_definition *datum,
	long item_definition_index,
	char message_offset);

/* ---------- globals */

static struct hud_messaging_globals_definition *hud_messaging_globals;
extern struct hud_globals_definition *hud_globals;
extern struct hud_messaging_parameters_definition *hud_msg_def;
extern struct hud_scripted_globals_definition *hud_scripted_globals;
extern long time_code_time;
extern long time_code_stop_time;

/* ---------- public code */

void hud_messaging_initialize(
	void)
{
	hud_messaging_globals = game_state_malloc(
		"hud messaging",
		NULL,
		sizeof(*hud_messaging_globals));
	return;
}

void hud_messaging_initialize_for_new_map(
	void)
{
	hud_msg_def = &hud_globals->messaging;
	csmemset(hud_messaging_globals, 0, sizeof(*hud_messaging_globals));
	return;
}

void hud_messaging_dispose_from_old_map(
	void)
{
	return;
}

void hud_messaging_dispose(
	void)
{
	return;
}

void scripted_hud_set_state_message(
	short message_index)
{
	struct scenario *scenario = global_scenario_get();

	if (hud_scripted_globals->show_hud_help_text &&
		scenario->hud_messages.index != NONE)
	{
		struct hud_message_text_definition *hud_messages = HUD_MESSAGE_TEXT_DEFINITION_GET(
			scenario->hud_messages.index);

		hud_messaging_globals->help_message = TAG_BLOCK_GET_ELEMENT(
			&hud_messages->messages,
			message_index,
			struct hud_state_message_definition);
	}

	return;
}

void scripted_hud_set_flashing_state(
	boolean flash)
{
	long time;

	if (flash && !hud_messaging_globals->use_flash)
	{
		time = game_time_get();
		hud_messaging_globals->flash_start_time = time;
		hud_messaging_globals->use_flash = flash;
		return;
	}
	hud_messaging_globals->use_flash = flash;

	return;
}

void scripted_hud_restart_flashing(
	void)
{
	if (hud_messaging_globals->use_flash)
		hud_messaging_globals->flash_start_time = game_time_get();
	else
		error(
			_error_silent,
			"trying to restart help text flashing when flashing is disabled");

	return;
}

void scripted_hud_set_objective(
	short message_index)
{
	struct scenario *scenario = global_scenario_get();

	if (scenario->hud_messages.index != NONE)
	{
		struct hud_message_text_definition *hud_messages =
			HUD_MESSAGE_TEXT_DEFINITION_GET(scenario->hud_messages.index);
		struct hud_state_message_definition *message = TAG_BLOCK_GET_ELEMENT(
			&hud_messages->messages,
			message_index,
			struct hud_state_message_definition);
		struct hud_state_message_element *element = TAG_BLOCK_GET_ELEMENT(
			&hud_messages->elements,
			message->element_start_index,
			struct hud_state_message_element);

		if (message->element_count == 1 && element->type == _hud_message_type_text)
		{
			struct hud_globals_definition *hud = hud_globals;
			struct hud_messaging_globals_definition *globals = hud_messaging_globals;
			short up_ticks;
			short fade_ticks;

			globals->objective.message = message;
			up_ticks = hud->messaging.objective_color.custom.objective.up_ticks;
			fade_ticks = hud->messaging.objective_color.custom.objective.fade_ticks;
			globals->objective.uptime = fade_ticks + up_ticks;
		}
		else
		{
			error(
				_error_silent,
				"objective text MUST only be text, no icons");
		}
	}

	return;
}

void scripted_hud_set_timer_time(
	short minutes,
	word seconds)
{
	struct hud_timer_data_definition *timer;
	long time;

	hud_messaging_globals->timer.ticks = 30 * (60 * minutes + seconds);
	timer = &hud_messaging_globals->timer;
	timer->paused = FALSE;
	timer->enabled = TRUE;
	time = game_time_get();
	timer = &hud_messaging_globals->timer;
	timer->reference_time = time;
	timer->corner = PIN(timer->corner, 0, 4);

	return;
}

void scripted_hud_set_timer_warning_cutoff(
	short minutes,
	word seconds)
{
	hud_messaging_globals->timer.flash_cutoff = 30 * (60 * minutes + seconds);
	return;
}

void scripted_hud_set_timer_position(
	short x,
	short y,
	short corner)
{
	struct hud_messaging_globals_definition *globals = hud_messaging_globals;

	globals->timer.position.n[0] = x;
	globals->timer.position.n[1] = y;
	globals->timer.corner = PIN(corner, 0, 4);

	return;
}

void scripted_hud_show_timer(
	boolean show)
{
	hud_messaging_globals->timer.enabled = show;
	return;
}

void scripted_hud_pause_timer(
	boolean paused)
{
	struct hud_timer_data_definition *timer = &hud_messaging_globals->timer;
	short now;

	timer->paused = paused;
	if (timer->ticks > 0)
	{
		if (paused)
		{
			now = (short)game_time_get();
			timer->ticks += timer->reference_time - now;
			return;
		}

		now = (short)game_time_get();
		timer->ticks += now - timer->reference_time;
	}

	return;
}

short scripted_hud_get_timer_ticks(
	void)
{
	struct hud_timer_data_definition *timer = &hud_messaging_globals->timer;
	boolean enabled = timer->enabled;
	short result = 0;

	if (enabled)
	{
		if (timer->ticks == NONE)
			return NONE;
		result = timer->ticks;

		if (!timer->paused)
		{
			return (short)(
				(word)(timer->ticks + (word)timer->reference_time) -
				game_time_get());
		}
	}

	return result;
}

void scripted_hud_time_code_show(
	boolean show)
{
	if (show)
	{
		time_code_time = game_time_get();
		time_code_stop_time = time_code_time;
		return;
	}
	time_code_time = NONE;

	return;
}

void scripted_hud_time_code_start(
	boolean start)
{
	if (start)
	{
		time_code_time += game_time_get() - time_code_stop_time;
		time_code_stop_time = NONE;
		return;
	}
	time_code_stop_time = game_time_get();

	return;
}

void scripted_hud_time_code_reset(
	void)
{
	long time = game_time_get();

	time_code_time = time;
	if (time_code_stop_time != NONE)
		time_code_stop_time = time;

	return;
}

void hud_render_timer(
	void)
{
	struct hud_timer_data_definition *timer = &hud_messaging_globals->timer;

	if (timer->enabled)
	{
		struct hud_absolute_placement_definition placement = { timer->corner };
		struct number_hud_element_definition numbers;
		long current_time = game_time_get();
		short flash = FALSE;
		short digit_advance = 0;
		short timer_ticks = scripted_hud_get_timer_ticks();
		long hud_number_index;

		numbers.digits = 2;
		numbers.fractional_digits = 4;
		numbers.number_flags = FLAG(_hud_number_show_all_leading_zeros_bit);
		numbers.placement.offset = timer->position;
		numbers.placement.scale.i = 1.0f;
		numbers.placement.scale.j = 1.0f;

		hud_number_index = interface_get_tag_index(_interface_hud_digits);
		if (hud_number_index != NONE)
		{
			struct hud_number_definition *hud_number =
				HUD_NUMBER_DEFINITION_GET(hud_number_index);

			digit_advance = (short)(long)((real)hud_number->screen_width * 2.0f);
		}

		switch (timer->corner)
		{
		case _hud_corner_top_left:
		case _hud_corner_bottom_left:
			break;

		case _hud_corner_top_right:
		case _hud_corner_bottom_right:
			numbers.placement.offset.x += 5 * digit_advance;
			digit_advance = -digit_advance;
			break;

		case _hud_corner_center:
			numbers.placement.offset.x -= 3 * digit_advance;
			break;

		default:
			match_assert(
				"c:\\halo\\SOURCE\\interface\\hud_messaging.c",
				0x1F8,
				!"unreachable");
			break;
		}

		if (timer_ticks > 0)
		{
			numbers.colors = hud_globals->timer_definition.color;
			{
				short flash_cutoff = timer->flash_cutoff;

				if (timer_ticks <= flash_cutoff)
				{
					flash = TRUE;
					if (timer->ticks > flash_cutoff)
					{
						timer->ticks = flash_cutoff;
						timer->reference_time =
							flash_cutoff - timer_ticks + current_time;
					}
				}
			}
		}
		else
		{
			long reference_time = timer->reference_time;

			timer->ticks = NONE;
			numbers.colors = hud_globals->timer_definition.time_up_color;
			flash = TRUE;
			if (reference_time == NONE)
				timer->reference_time = game_time_get();
		}

		{
			short clamped_ticks = MAX(timer_ticks, 0);
			short total_seconds = clamped_ticks / TICKS_PER_SECOND;
			double offset_advance;

			hud_draw_numbers(
				render.local_player_index,
				&placement,
				&numbers,
				total_seconds / SECONDS_PER_MINUTE,
				NONE,
				flash,
				timer->reference_time,
				2.0f);

			offset_advance = (double)digit_advance * 2.5;
			numbers.placement.offset.x = (short)(long)(
				(double)numbers.placement.offset.x + offset_advance);
			hud_draw_numbers(
				render.local_player_index,
				&placement,
				&numbers,
				total_seconds % SECONDS_PER_MINUTE,
				NONE,
				flash,
				timer->reference_time,
				2.0f);

			numbers.placement.offset.x = (short)(long)(
				(double)numbers.placement.offset.x + offset_advance);
			hud_draw_numbers(
				render.local_player_index,
				&placement,
				&numbers,
				100 * (clamped_ticks % TICKS_PER_SECOND) / TICKS_PER_SECOND,
				NONE,
				flash,
				timer->reference_time,
				2.0f);
		}
	}

	if (time_code_time != NONE)
	{
		long stop_time = time_code_stop_time;

		if (stop_time == NONE)
			stop_time = game_time_get();
		sprintf(temporary, "%d", stop_time - time_code_time);
		render_debug_string(TRUE, temporary);
	}

	return;
}

void hud_set_state_message(
	short local_player_index,
	short message_index)
{
	if (!hud_scripted_globals->show_hud_help_text)
	{
		long hud_messages_index = hud_globals->messaging.hud_messages.index;

		if (hud_messages_index != NONE)
		{
			struct hud_messaging_globals_definition *globals = hud_messaging_globals;
			struct hud_messaging_datum_definition *datum =
				&globals->message_data[local_player_index];

			if (message_index != NONE)
			{
				struct hud_message_text_definition *hud_messages =
					HUD_MESSAGE_TEXT_DEFINITION_GET(hud_messages_index);

				if (message_index < hud_messages->messages.count)
				{
					datum->state_message.state_message = TAG_BLOCK_GET_ELEMENT(
						&hud_messages->messages,
						message_index,
						struct hud_state_message_definition);
					datum->state_message.is_text_flags = 0;
				}
				else
				{
					message_index = NONE;
				}
			}

			datum->state_message.valid = message_index != NONE;
		}
	}

	return;
}

void
hud_set_state_message_icon(
	short local_player_index,
	short custom_icon_index,
	struct icon_hud_element_definition const *icon)
{
	struct hud_messaging_datum_definition *datum =
		&hud_messaging_globals->message_data[local_player_index];

	if (datum->state_message.valid &&
		!hud_scripted_globals->show_hud_help_text &&
		datum->state_message.state_message)
	{
		datum->state_message.info[custom_icon_index].icon = icon;
		datum->state_message.is_text_flags &= ~(1 << custom_icon_index);
	}

	return;
}

void
hud_set_state_message_text(
	short local_player_index,
	short custom_icon_index,
	short icon_string_index,
	boolean uses_scenario_names)
{
	struct hud_messaging_datum_definition *datum =
		&hud_messaging_globals->message_data[local_player_index];

	if (datum->state_message.valid &&
		!hud_scripted_globals->show_hud_help_text &&
		datum->state_message.state_message)
	{
		datum->state_message.info[custom_icon_index].text.string_index = icon_string_index;
		datum->state_message.info[custom_icon_index].text.uses_scenario_names = uses_scenario_names;
		datum->state_message.is_text_flags |= 1 << custom_icon_index;
	}

	return;
}

void
hud_enable_custom_state_message(
	short local_player_index,
	boolean enabled)
{
	struct hud_messaging_datum_definition *datum =
		&hud_messaging_globals->message_data[local_player_index];

	datum->leave_first_line_blank |= datum->state_message.valid != enabled;
	datum->state_message.valid = enabled;
	datum->state_message.state_message = NULL;
	if (enabled)
	{
		datum->state_message.state_message = NULL;
		ustrncpy(
			datum->state_message.message_buffer,
			L"",
			MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH - 1);
	}
	datum->custom_message = enabled;

	return;
}

void hud_set_state_text(
	short local_player_index,
	wchar_t const *message)
{
	struct hud_messaging_datum_definition *datum =
		&hud_messaging_globals->message_data[local_player_index];

	ustrncpy(
		datum->state_message.message_buffer,
		message,
		MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH - 1);
	datum->state_message.message_buffer[MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH - 1] = L'\0';

	return;
}

wchar_t *hud_messaging_get_objective(
	void)
{
	wchar_t *result = NULL;

	if (hud_messaging_globals->objective.message)
	{
		struct scenario *scenario = global_scenario_get();
		struct hud_message_text_definition *hud_messages =
			HUD_MESSAGE_TEXT_DEFINITION_GET(scenario->hud_messages.index);
		struct hud_state_message_definition *message =
			hud_messaging_globals->objective.message;
		struct hud_state_message_element *element = TAG_BLOCK_GET_ELEMENT(
			&hud_messages->elements,
			message->element_start_index,
			struct hud_state_message_element);

		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_messaging.c",
			0x2A2,
			message->element_count == 1,
			"message->element_count==1");
		match_vassert(
			"c:\\halo\\SOURCE\\interface\\hud_messaging.c",
			0x2A3,
			element->type == _hud_message_type_text,
			"element->type==_hud_message_type_text");

		result = tag_data_get_pointer(
			&hud_messages->text_data,
			message->text_start_index * sizeof(wchar_t),
			element->data * sizeof(wchar_t));
	}

	return result;
}

void scripted_hud_messages_clear(
	void)
{
	struct hud_messaging_datum_definition *datum = hud_messaging_globals->message_data;
	long datum_count = NUMBER_OF_HUD_MESSAGING_DATUMS;

	do
	{
		struct hud_message_definition *message = datum->messages;
		long message_count = NUMBER_OF_HUD_MESSAGES_PER_DATUM;

		do
		{
			message->valid = FALSE;
			message++;
		}
		while (--message_count);

		datum++;
	}
	while (--datum_count);

	return;
}

long hud_get_font_index(
	void)
{
	if (local_player_count() > 1 &&
		hud_msg_def->multi_player_font.index != NONE)
	{
		return hud_msg_def->multi_player_font.index;
	}

	return hud_msg_def->single_player_font.index;
}

real_argb_color *hud_get_text_color(
	real_argb_color *result)
{
	*result = hud_msg_def->state_color;

	return result;
}

void hud_messaging_globals_update(
	void)
{
	hud_messaging_globals->magic_number = 0;
	return;
}

void hud_print_message(
	short local_player_index,
	wchar_t const *message_text)
{
	if (local_player_index != NONE)
	{
		struct hud_messaging_datum_definition *datum =
			&hud_messaging_globals->message_data[local_player_index];
		struct hud_message_definition *message = find_free_hud_message(
			datum,
			NONE,
			0);

		ustrncpy(message->text, message_text, NUMBEROF(message->text));
		message->item_definition_index = NONE;
		message->time = game_time_get();
		message->valid = TRUE;
		message->magic_number = hud_messaging_globals->magic_number++;
		datum->leave_first_line_blank = FALSE;
	}

	return;
}

void hud_add_item_message(
	short local_player_index,
	long item_definition_index,
	short quantity,
	char message_offset)
{
	if (local_player_index != NONE)
	{
		struct hud_messaging_datum_definition *datum =
			&hud_messaging_globals->message_data[local_player_index];
		struct hud_message_definition *message = find_free_hud_message(
			datum,
			item_definition_index,
			message_offset);

		if (!message->valid)
			message->quantity = 0;
		message->quantity += quantity;
		message->item_definition_index = item_definition_index;
		message->message_offset = message_offset;
		message->time = game_time_get();
		message->valid = TRUE;
		message->magic_number = hud_messaging_globals->magic_number++;
		datum->leave_first_line_blank = FALSE;
	}

	return;
}

void hud_broadcast_team_message(
	long victim_player_index,
	wchar_t const *message)
{
	if (game_engine_running())
	{
		short local_player_index;

		for (local_player_index = 0;
			local_player_index < MAXIMUM_LOCAL_PLAYERS;
			local_player_index++)
		{
			long player_index = local_player_get_player_index(local_player_index);

			if (player_index != NONE &&
				player_get(player_index)->team_index == player_get(victim_player_index)->team_index)
			{
				hud_print_message(local_player_index, message);
			}
		}
	}

	return;
}

/* ---------- private code */

static struct hud_message_definition *find_free_hud_message(
	struct hud_messaging_datum_definition *datum,
	long item_definition_index,
	char message_offset)
{
	struct hud_message_definition *result = NULL;
	long oldest_time = LONG_MAX;
	short oldest_message_index = 0;
	short message_index = 0;

	do
	{
		struct hud_message_definition *message = &datum->messages[message_index];

		if ((item_definition_index != NONE &&
			item_definition_index == message->item_definition_index &&
			message_offset == message->message_offset) || !message->valid)
		{
			result = message;
			if (item_definition_index == NONE ||
				item_definition_index == message->item_definition_index)
			{
				break;
			}
		}
		else if (oldest_time > message->time)
		{
			oldest_time = message->time;
			oldest_message_index = message_index;
		}

		message_index++;
	}
	while ((word)message_index < NUMBER_OF_HUD_MESSAGES_PER_DATUM);

	if (!result)
		result = &datum->messages[oldest_message_index];

	return result;
}

static long compare_messages(
	struct hud_message_definition const *message_a,
	struct hud_message_definition const *message_b)
{
	long difference = message_b->time - message_a->time;

	if (!difference)
	{
		difference = message_b->item_definition_index - message_a->item_definition_index;
		if (!difference)
			difference = message_b->magic_number - message_a->magic_number;
	}

	return difference;
}
