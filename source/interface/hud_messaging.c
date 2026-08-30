/*
HUD_MESSAGING.C

symbols in this file:
000C3810 0080:
	_code_000c3810 (0000)
000C3890 0190:
	_code_000c3890 (0000)
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
	_code_000c43f0 (0000)
000C4470 0030:
	_code_000c4470 (0000)
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
	_data_002e4c4c (0000)
	_time_code_time (0010)
	_time_code_stop_time (0014)
00453AB8 0004:
	_bss_00453ab8 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#include "cseries/errors.h"
#include "game/game.h"
#include "game/players.h"
#include "interface/hud_messaging.h"
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
	NUMBER_OF_HUD_MESSAGING_DATUMS = 4,
	NUMBER_OF_HUD_MESSAGES_PER_DATUM = 4,
	MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH = 256
};

/* ---------- macros */

#define HUD_MESSAGE_TEXT_DEFINITION_GET(index) \
	((struct hud_message_text_definition *)tag_get(hud_message_text_group_tag, (index)))

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
	byte reserved[0x40];
};

struct hud_state_message_runtime_definition
{
	wchar_t message_buffer[MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH];
	byte reserved200[0x2C];
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
	byte reserved000[0x20];
	struct tag_block messages;
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
	short position[2];
	short corner;
	boolean paused;
	boolean enabled;
};

struct hud_messaging_globals_definition
{
	struct hud_messaging_datum_definition message_data[NUMBER_OF_HUD_MESSAGING_DATUMS];
	long flash_start_time;
	boolean use_flash;
	byte magic_number;
	byte reserved1186[6];
	struct hud_state_message_definition *help_message;
	byte reserved1190[8];
	struct hud_timer_data_definition timer;
};

struct hud_messaging_parameters_definition
{
	byte reserved000[0x48];
	struct tag_reference single_player_font;
	struct tag_reference multi_player_font;
	real up_time;
	real fade_time;
	real_argb_color state_color;
	byte reserved080[0xA0];
};

struct hud_globals_definition
{
	struct hud_messaging_parameters_definition messaging;
};

typedef char hud_timer_data_size_assert[
	sizeof(struct hud_timer_data_definition) == 0x10 ? 1 : -1];
typedef char hud_message_valid_offset_assert[
	offsetof(struct hud_message_definition, valid) == 0x82 ? 1 : -1];
typedef char hud_message_size_assert[
	sizeof(struct hud_message_definition) == 0x8C ? 1 : -1];
typedef char hud_state_message_runtime_size_assert[
	sizeof(struct hud_state_message_runtime_definition) == 0x22C ? 1 : -1];
typedef char hud_messaging_datum_state_message_offset_assert[
	offsetof(struct hud_messaging_datum_definition, state_message) == 0x230 ? 1 : -1];
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

/* ---------- prototypes */

/* ---------- globals */

extern struct hud_messaging_globals_definition *bss_00453ab8;
extern struct hud_globals_definition *hud_globals;
extern struct hud_messaging_parameters_definition *hud_msg_def;
extern struct hud_scripted_globals_definition *hud_scripted_globals;
extern long time_code_time;
extern long time_code_stop_time;

/* ---------- public code */

void hud_messaging_initialize(
	void)
{
	bss_00453ab8 = game_state_malloc(
		"hud messaging",
		NULL,
		sizeof(*bss_00453ab8));
	return;
}

void hud_messaging_initialize_for_new_map(
	void)
{
	hud_msg_def = &hud_globals->messaging;
	csmemset(bss_00453ab8, 0, sizeof(*bss_00453ab8));
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

		bss_00453ab8->help_message = TAG_BLOCK_GET_ELEMENT(
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
	struct hud_messaging_globals_definition *globals;

	if (flash && !bss_00453ab8->use_flash)
	{
		time = game_time_get();
		globals = bss_00453ab8;
		globals->flash_start_time = time;
		globals->use_flash = flash;
		return;
	}
	bss_00453ab8->use_flash = flash;

	return;
}

void scripted_hud_restart_flashing(
	void)
{
	if (bss_00453ab8->use_flash)
		bss_00453ab8->flash_start_time = game_time_get();
	else
		error(
			_error_silent,
			"trying to restart help text flashing when flashing is disabled");

	return;
}

void scripted_hud_set_timer_warning_cutoff(
	short minutes,
	word seconds)
{
	bss_00453ab8->timer.flash_cutoff = 30 * (60 * minutes + seconds);
	return;
}

void scripted_hud_set_timer_position(
	short x,
	short y,
	short corner)
{
	struct hud_messaging_globals_definition *globals = bss_00453ab8;

	globals->timer.position[0] = x;
	globals->timer.position[1] = y;
	if (corner >= 0)
		globals->timer.corner = corner > 4 ? 4 : corner;
	else
		globals->timer.corner = 0;

	return;
}

void scripted_hud_show_timer(
	boolean show)
{
	bss_00453ab8->timer.enabled = show;
	return;
}

void scripted_hud_pause_timer(
	boolean paused)
{
	struct hud_timer_data_definition *timer = &bss_00453ab8->timer;
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

void hud_set_state_text(
	short local_player_index,
	wchar_t const *message)
{
	struct hud_messaging_datum_definition *datum =
		&bss_00453ab8->message_data[local_player_index];

	ustrncpy(
		datum->state_message.message_buffer,
		message,
		MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH - 1);
	datum->state_message.message_buffer[MAXIMUM_HUD_STATE_MESSAGE_TEXT_LENGTH - 1] = L'\0';

	return;
}

void scripted_hud_messages_clear(
	void)
{
	struct hud_messaging_datum_definition *datum = bss_00453ab8->message_data;
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
	bss_00453ab8->magic_number = 0;
	return;
}

/* ---------- private code */
