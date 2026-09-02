/*
VIRTUAL_KEYBOARD.C

symbols in this file:
000E4B50 0100:
	_virtual_keyboard_initialize (0000)
000E4C50 0060:
	_virtual_keyboard_dispose (0000)
000E4CB0 0140:
	_virtual_keyboard_launch (0000)
000E4DF0 0010:
	_virtual_keyboard_active (0000)
000E4E00 0010:
	_virtual_keyboard_last_exit_saved_text (0000)
000E4E10 0050:
	_code_000e4e10 (0000)
000E4E60 0050:
	_code_000e4e60 (0000)
000E4EB0 0050:
	_code_000e4eb0 (0000)
000E4F00 0050:
	_code_000e4f00 (0000)
000E4F50 0060:
	_code_000e4f50 (0000)
000E4FB0 00d0:
	_code_000e4fb0 (0000)
000E5080 0030:
	_code_000e5080 (0000)
000E50B0 0650:
	_code_000e50b0 (0000)
000E5700 0020:
	_code_000e5700 (0000)
000E5720 0060:
	_code_000e5720 (0000)
000E5780 0010:
	_virtual_keyboard_close (0000)
000E5790 0010:
	_virtual_keyboard_render (0000)
000E57A0 0440:
	_code_000e57a0 (0000)
000E5BE0 0350:
	_code_000e5be0 (0000)
000E5F30 0010:
	_virtual_keyboard_process (0000)
00278CC0 0037:
	_rdata_00278cc0 (0000)
00278CF8 0032:
	??_C@_0DC@DOACNNNI@failed?5to?5load?5virtual?5keyboard?5@ (0000)
00278D2C 0032:
	??_C@_0DC@DKJLNFKM@failed?5to?5load?5virtual?5keyboard?5@ (0000)
00278D60 0022:
	??_C@_0CC@BINJJDCB@virtual_keyboard_globals?4keyboar@ (0000)
00278D84 002c:
	??_C@_0CM@FNHOMJL@c?3?2halo?2SOURCE?2interface?2virtual@ (0000)
00278DB0 0073:
	??_C@_0HD@GDAACNMN@?$CIcaption_index?$DO?$DNFIRST_VIRTUAL_KE@ (0000)
00278E28 0053:
	??_C@_0FD@HLHLLBE@text_buffer?5?$CG?$CG?5buffer_size?5?$CG?$CG?5?$CB?$CI@ (0000)
00278E7C 002e:
	??_C@_0CO@ODBBPCAJ@keycode?5?$DM?5NUMBER_OF_CONFIGURABLE@ (0000)
00278EAC 002a:
	??_C@_0CK@MJDENDBH@virtual_keyboard_globals?4keyboar@ (0000)
00278ED8 0015:
	??_C@_0BF@LICBHHNC@?$CB?$CCwhat?5key?5is?5this?$DP?$CC?$AA@ (0000)
00278EF0 0015:
	??_C@_0BF@LNHKMLGP@keyboard_font_header?$AA@ (0000)
00278F08 006b:
	??_C@_0GL@GAMGNLLK@?$CIvirtual_keyboard_globals?4keyboa@ (0000)
00278F74 0012:
	??_C@_1BC@IBHDIJNB@?$AA?4?$AAf?$AAo?$AAr?$AAt?$AAu?$AAn?$AAe?$AA?$AA@ (0000)
00278F88 0027:
	??_C@_0CH@KEKEOCJD@virtual_keyboard_globals?4buffer_@ (0000)
00306338 0160:
	_data_00306338 (0000)
00454D10 006c:
	_bss_00454d10 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/cseries_windows.h"
#include "cseries/errors.h"
#include "interface/event_manager.h"
#include "interface/virtual_keyboard.h"
#include "input/input.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	VIRTUAL_KEYBOARD_ROW_COUNT = 5,
	VIRTUAL_KEYBOARD_COLUMN_COUNT = 11,
	NUMBER_OF_CONFIGURABLE_VIRTUAL_KEYS = 0x24,
	FIRST_VIRTUAL_KEYBOARD_CAPTION_STRING_INDEX = 8,
	NUMBER_OF_VIRTUAL_KEYBOARD_STRINGS = 11,
	MAXIMUM_VIRTUAL_KEYBOARD_BUFFER_SIZE = 0x40,
	MAXIMUM_VIRTUAL_KEYBOARD_SAVED_TEXT_LENGTH = 0x20,
	VIRTUAL_KEYBOARD_TAB_REPEAT_MILLISECONDS = 250,
	FIRST_VIRTUAL_KEYBOARD_FORTUNE_STRING_INDEX = 11,
	NUMBER_OF_VIRTUAL_KEYBOARD_FORTUNES = 10,
};

enum
{
	_virtual_key_done= NUMBER_OF_CONFIGURABLE_VIRTUAL_KEYS,
	_virtual_key_shift,
	_virtual_key_caps_lock,
	_virtual_key_symbols,
	_virtual_key_backspace,
	_virtual_key_cursor_left,
	_virtual_key_cursor_right,
	_virtual_key_space,
	NUMBER_OF_VIRTUAL_KEYS
};

enum
{
	_event_tab_left,
	_event_tab_right,
	_event_tab_up,
	_event_tab_down,
	_event_key_select,
	_event_cancel,
	NUMBER_OF_VIRTUAL_KEYBOARD_EVENTS
};

/* event_manager.c keeps these private; see header request in the ledger */
enum
{
	_event_type_null,
	_event_type_left_stick,
	_event_type_right_stick,
	_event_type_button
};

/* ---------- macros */

/* ---------- structures */

struct virtual_keyboard_key
{
	short keycode;
	wchar_t character;
	wchar_t shift_character;
	wchar_t caps_character;
	wchar_t symbols_character;
	wchar_t shift_caps_character;
	wchar_t shift_symbols_character;
	wchar_t caps_symbols_character;
	byte reserved10[0x40];
};

struct virtual_keyboard_definition
{
	byte reserved0[0x30];
	struct tag_block keys;
};

struct virtual_keyboard_globals
{
	boolean active;
	boolean shift_active;
	boolean caps_active;
	boolean symbols_active;
	struct virtual_keyboard_definition *keyboard;
	short row;
	short column;
	unsigned short buffer_size;
	short last_event;
	short last_key;
	short number_of_event_repeats;
	short caption_index;
	boolean last_exit_saved_text;
	boolean first_key_replaces_buffer;
	wchar_t *text_buffer;
	wchar_t *cursor;
	unsigned long time_of_last_event;
	long caret_bitmap_index;
	wchar_t saved_text[MAXIMUM_VIRTUAL_KEYBOARD_SAVED_TEXT_LENGTH];
};

typedef char verify_virtual_keyboard_globals_size[
	sizeof(struct virtual_keyboard_globals) == 0x68 ? 1 : -1];
typedef char verify_virtual_keyboard_key_size[
	sizeof(struct virtual_keyboard_key) == 0x50 ? 1 : -1];

/* ---------- prototypes */

boolean code_000e4f50(
	void);
static wchar_t code_000e4fb0(
	unsigned short keycode);
wchar_t code_000e5080(
	void);
void code_000e50b0(
	void);
static boolean virtual_keyboard_select(
	void);
static void virtual_keyboard_process_internal(
	void);
unsigned long ustrlen(
	wchar_t const *string);
wchar_t *ustrncpy(
	wchar_t *destination,
	wchar_t const *source,
	unsigned long count);
void ui_play_audio_feedback_sound(
	short audio_feedback);
void display_error(
	short error_code,
	short local_player_index,
	boolean modal,
	boolean pause_game_time);
long ustrcmp(
	wchar_t const *string1,
	wchar_t const *string2);
wchar_t *ustrcpy(
	wchar_t *dest,
	wchar_t const *src);
boolean saved_game_file_name_unique(
	wchar_t const *name);

/* ---------- globals */

char const virtual_keyboard_key_layout[VIRTUAL_KEYBOARD_ROW_COUNT][VIRTUAL_KEYBOARD_COLUMN_COUNT] =
{
	{ 0x24, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
	{ 0x25, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 },
	{ 0x26, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D },
	{ 0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x28, 0x28, 0x28, 0x28 },
	{ 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x29, 0x29, 0x2A, 0x2A },
};

struct virtual_keyboard_globals virtual_keyboard_globals;

/* ---------- public code */

boolean virtual_keyboard_initialize(
	void)
{
	long keyboard_index;

	virtual_keyboard_globals.active= FALSE;
	virtual_keyboard_globals.shift_active= FALSE;
	virtual_keyboard_globals.caps_active= FALSE;
	virtual_keyboard_globals.symbols_active= FALSE;

	keyboard_index= tag_loaded('vcky', "ui\\english");
	if (keyboard_index != NONE)
	{
		virtual_keyboard_globals.keyboard= tag_get('vcky', keyboard_index);
		match_assert(
			"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
			364,
			virtual_keyboard_globals.keyboard);

		virtual_keyboard_globals.row= 0;
		virtual_keyboard_globals.column= 0;
		virtual_keyboard_globals.buffer_size= 0;
		virtual_keyboard_globals.last_event= NONE;
		virtual_keyboard_globals.last_key= NONE;
		virtual_keyboard_globals.number_of_event_repeats= 0;
		virtual_keyboard_globals.text_buffer= NULL;
		virtual_keyboard_globals.cursor= NULL;
		virtual_keyboard_globals.time_of_last_event= 0;
	}
	else
	{
		error(2, "failed to load virtual keyboard for '%s' language", "<unknown>");
	}

	virtual_keyboard_globals.caret_bitmap_index=
		tag_loaded('bitm', "ui\\shell\\bitmaps\\white");
	if (virtual_keyboard_globals.caret_bitmap_index == NONE)
	{
		error(
			2,
			"failed to load virtual keyboard caret bitmap '%s'",
			"ui\\shell\\bitmaps\\white");
	}

	return virtual_keyboard_globals.keyboard != NULL;
}

void virtual_keyboard_dispose(
	void)
{
	virtual_keyboard_globals.active= FALSE;
	virtual_keyboard_globals.shift_active= FALSE;
	virtual_keyboard_globals.caps_active= FALSE;
	virtual_keyboard_globals.symbols_active= FALSE;
	virtual_keyboard_globals.keyboard= NULL;
	virtual_keyboard_globals.row= 0;
	virtual_keyboard_globals.column= 0;
	virtual_keyboard_globals.buffer_size= 0;
	virtual_keyboard_globals.last_event= NONE;
	virtual_keyboard_globals.last_key= NONE;
	virtual_keyboard_globals.number_of_event_repeats= 0;
	virtual_keyboard_globals.text_buffer= NULL;
	virtual_keyboard_globals.cursor= NULL;
	virtual_keyboard_globals.time_of_last_event= 0;

	event_manager_flush();

	return;
}

boolean virtual_keyboard_launch(
	wchar_t *text_buffer,
	unsigned short buffer_size,
	short caption_index)
{
	boolean result= FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
		418,
		text_buffer && buffer_size && !(buffer_size&1) && !virtual_keyboard_globals.active);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
		419,
		(caption_index>=FIRST_VIRTUAL_KEYBOARD_CAPTION_STRING_INDEX) && (caption_index<NUMBER_OF_VIRTUAL_KEYBOARD_STRINGS));

	if (!virtual_keyboard_globals.active && virtual_keyboard_globals.keyboard)
	{
		event_manager_flush();
		virtual_keyboard_globals.row= 0;
		virtual_keyboard_globals.column= 0;
		virtual_keyboard_globals.active= TRUE;
		virtual_keyboard_globals.text_buffer= text_buffer;
		virtual_keyboard_globals.cursor= text_buffer + ustrlen(text_buffer);
		virtual_keyboard_globals.buffer_size= buffer_size;
		if (virtual_keyboard_globals.buffer_size >= MAXIMUM_VIRTUAL_KEYBOARD_BUFFER_SIZE)
			virtual_keyboard_globals.buffer_size= MAXIMUM_VIRTUAL_KEYBOARD_BUFFER_SIZE;
		virtual_keyboard_globals.last_event= NONE;
		virtual_keyboard_globals.time_of_last_event= system_milliseconds();
		virtual_keyboard_globals.caption_index= caption_index;
		virtual_keyboard_globals.shift_active= FALSE;
		virtual_keyboard_globals.caps_active= FALSE;
		virtual_keyboard_globals.symbols_active= FALSE;
		virtual_keyboard_globals.first_key_replaces_buffer= TRUE;
		ustrncpy(
			virtual_keyboard_globals.saved_text,
			text_buffer,
			MAXIMUM_VIRTUAL_KEYBOARD_SAVED_TEXT_LENGTH);
		virtual_keyboard_globals.saved_text[MAXIMUM_VIRTUAL_KEYBOARD_SAVED_TEXT_LENGTH - 1]= L'\0';
		virtual_keyboard_globals.last_exit_saved_text= FALSE;
		ui_play_audio_feedback_sound(2);
		result= TRUE;
	}

	return result;
}

boolean virtual_keyboard_active(
	void)
{
	return virtual_keyboard_globals.active;
}

boolean virtual_keyboard_last_exit_saved_text(
	void)
{
	return virtual_keyboard_globals.last_exit_saved_text;
}

boolean code_000e4e10(
	void)
{
	char keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

	do
	{
		virtual_keyboard_globals.column--;
		if (virtual_keyboard_globals.column < 0)
			virtual_keyboard_globals.column = VIRTUAL_KEYBOARD_COLUMN_COUNT - 1;
	}
	while (virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column] == keycode);

	ui_play_audio_feedback_sound(1);

	return TRUE;
}

boolean code_000e4e60(
	void)
{
	char keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

	do
	{
		virtual_keyboard_globals.column++;
		if (virtual_keyboard_globals.column == VIRTUAL_KEYBOARD_COLUMN_COUNT)
			virtual_keyboard_globals.column = 0;
	}
	while (virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column] == keycode);

	ui_play_audio_feedback_sound(1);

	return TRUE;
}

boolean code_000e4eb0(
	void)
{
	char keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

	do
	{
		virtual_keyboard_globals.row--;
		if (virtual_keyboard_globals.row < 0)
			virtual_keyboard_globals.row = VIRTUAL_KEYBOARD_ROW_COUNT - 1;
	}
	while (virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column] == keycode);

	ui_play_audio_feedback_sound(1);

	return TRUE;
}

boolean code_000e4f00(
	void)
{
	char keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

	do
	{
		virtual_keyboard_globals.row++;
		if (virtual_keyboard_globals.row == VIRTUAL_KEYBOARD_ROW_COUNT)
			virtual_keyboard_globals.row = 0;
	}
	while (virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column] == keycode);

	ui_play_audio_feedback_sound(1);

	return TRUE;
}

boolean code_000e4f50(
	void)
{
	virtual_keyboard_globals.active= FALSE;
	if (virtual_keyboard_globals.text_buffer)
	{
		ustrncpy(
			virtual_keyboard_globals.text_buffer,
			virtual_keyboard_globals.saved_text,
			virtual_keyboard_globals.buffer_size / sizeof(wchar_t));
		virtual_keyboard_globals.text_buffer[
			virtual_keyboard_globals.buffer_size / sizeof(wchar_t) - 1]= L'\0';
	}

	virtual_keyboard_globals.text_buffer= NULL;
	virtual_keyboard_globals.saved_text[0]= L'\0';
	virtual_keyboard_globals.last_exit_saved_text= FALSE;
	ui_play_audio_feedback_sound(3);

	return TRUE;
}

static wchar_t code_000e4fb0(
	unsigned short keycode)
{
	struct virtual_keyboard_key *key;
	wchar_t character;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
		987,
		virtual_keyboard_globals.keyboard != NULL);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
		988,
		keycode < NUMBER_OF_CONFIGURABLE_VIRTUAL_KEYS);

	key= (struct virtual_keyboard_key *)virtual_keyboard_globals.keyboard->keys.address + keycode;
	if (virtual_keyboard_globals.shift_active)
	{
		if (virtual_keyboard_globals.caps_active)
			character= key->shift_caps_character;
		else if (virtual_keyboard_globals.symbols_active)
			character= key->shift_symbols_character;
		else
			character= key->shift_character;
	}
	else
	{
		if (virtual_keyboard_globals.caps_active)
		{
			if (virtual_keyboard_globals.symbols_active)
				character= key->caps_symbols_character;
			else
				character= key->caps_character;
		}
		else if (virtual_keyboard_globals.symbols_active)
			character= key->symbols_character;
		else
			character= key->character;
	}

	if (!character)
		character= 0x7F;

	return character;
}

wchar_t code_000e5080(
	void)
{
	return code_000e4fb0(virtual_keyboard_key_layout[
		virtual_keyboard_globals.row][virtual_keyboard_globals.column]);
}

long code_000e5700(
	void)
{
	return virtual_keyboard_globals.buffer_size -
		2 * (ustrlen(virtual_keyboard_globals.text_buffer) + 1);
}

void code_000e5720(
	void)
{
	if (virtual_keyboard_globals.cursor > virtual_keyboard_globals.text_buffer)
	{
		long remaining_size= virtual_keyboard_globals.buffer_size -
			((byte *)virtual_keyboard_globals.cursor - (byte *)virtual_keyboard_globals.text_buffer);

		if (remaining_size>=0)
		{
			csmemmove(
				virtual_keyboard_globals.cursor - 1,
				virtual_keyboard_globals.cursor,
				remaining_size);
			virtual_keyboard_globals.text_buffer[virtual_keyboard_globals.buffer_size / 2 - 1]= L'\0';
			virtual_keyboard_globals.cursor--;
		}
	}

	ui_play_audio_feedback_sound(1);

	return;
}

void virtual_keyboard_close(
	void)
{
	code_000e4f50();

	return;
}

void virtual_keyboard_render(
	void)
{
	if (virtual_keyboard_globals.active)
		code_000e50b0();

	return;
}

void virtual_keyboard_process(
	void)
{
	if (virtual_keyboard_globals.active)
		virtual_keyboard_process_internal();

	return;
}

/* ---------- private code */

static boolean virtual_keyboard_select(
	void)
{
	short keycode= virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

	switch (keycode)
	{
	case _virtual_key_done:
		if (ustrcmp(virtual_keyboard_globals.saved_text, virtual_keyboard_globals.text_buffer)!=0)
		{
			if (virtual_keyboard_globals.text_buffer[0])
			{
				if (saved_game_file_name_unique(virtual_keyboard_globals.text_buffer))
				{
					virtual_keyboard_globals.last_exit_saved_text= TRUE;
				}
				else
				{
					display_error(27, NONE, TRUE, FALSE); /* already a saved game file with that name */
					code_000e4f50();
				}
			}
			else
			{
				display_error(29, NONE, TRUE, FALSE); /* cannot create a saved game file with an empty name */
				code_000e4f50();
			}
		}
		else
		{
			virtual_keyboard_globals.last_exit_saved_text= TRUE;
		}
		ui_play_audio_feedback_sound(3);
		virtual_keyboard_globals.active= FALSE;
		event_manager_flush();
		break;

	case _virtual_key_shift:
		ui_play_audio_feedback_sound(1);
		virtual_keyboard_globals.shift_active= !virtual_keyboard_globals.shift_active;
		break;

	case _virtual_key_caps_lock:
		ui_play_audio_feedback_sound(1);
		virtual_keyboard_globals.caps_active= !virtual_keyboard_globals.caps_active;
		break;

	case _virtual_key_symbols:
		ui_play_audio_feedback_sound(1);
		virtual_keyboard_globals.symbols_active= !virtual_keyboard_globals.symbols_active;
		break;

	case _virtual_key_backspace:
		if (virtual_keyboard_globals.first_key_replaces_buffer==TRUE)
		{
			match_assert(
				"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
				854,
				virtual_keyboard_globals.buffer_size>0);
			csmemset(
				virtual_keyboard_globals.text_buffer,
				0,
				virtual_keyboard_globals.buffer_size);
			virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer;
			virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
		}
		else
		{
			code_000e5720();
		}
		break;

	case _virtual_key_cursor_left:
		if (virtual_keyboard_globals.cursor>virtual_keyboard_globals.text_buffer)
			virtual_keyboard_globals.cursor--;
		virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
		ui_play_audio_feedback_sound(1);
		break;

	case _virtual_key_cursor_right:
		if (*virtual_keyboard_globals.cursor)
			virtual_keyboard_globals.cursor++;
		virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
		ui_play_audio_feedback_sound(1);
		break;

	case _virtual_key_space:
		if (virtual_keyboard_globals.first_key_replaces_buffer==TRUE)
		{
			match_assert(
				"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
				889,
				virtual_keyboard_globals.buffer_size>0);
			csmemset(
				virtual_keyboard_globals.text_buffer,
				0,
				virtual_keyboard_globals.buffer_size);
			virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer;
			virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
		}
		if (code_000e5700()>=2)
		{
			csmemmove(
				virtual_keyboard_globals.cursor+1,
				virtual_keyboard_globals.cursor,
				virtual_keyboard_globals.buffer_size-((byte *)virtual_keyboard_globals.cursor-(byte *)virtual_keyboard_globals.text_buffer)-sizeof(wchar_t));
			*virtual_keyboard_globals.cursor++= L' ';
			ui_play_audio_feedback_sound(2);
		}
		else
		{
			ui_play_audio_feedback_sound(4);
		}
		break;

	default:
		if (virtual_keyboard_globals.first_key_replaces_buffer==TRUE)
		{
			match_assert(
				"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
				914,
				virtual_keyboard_globals.buffer_size>0);
			csmemset(
				virtual_keyboard_globals.text_buffer,
				0,
				virtual_keyboard_globals.buffer_size);
			virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer;
			virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
		}
		{
			long buffer_size= virtual_keyboard_globals.buffer_size;
			long free_space= buffer_size-2*(ustrlen(virtual_keyboard_globals.text_buffer)+1);

			if (free_space>=2)
			{
				csmemmove(
					virtual_keyboard_globals.cursor+1,
					virtual_keyboard_globals.cursor,
					buffer_size-((byte *)virtual_keyboard_globals.cursor-(byte *)virtual_keyboard_globals.text_buffer)-sizeof(wchar_t));
				*virtual_keyboard_globals.cursor++= code_000e4fb0(
					virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column]);
				if (ustrcmp(virtual_keyboard_globals.text_buffer, L".fortune")==0)
				{
					unsigned long fortune_index= system_milliseconds()%NUMBER_OF_VIRTUAL_KEYBOARD_FORTUNES;
					fortune_index= MIN(fortune_index, NUMBER_OF_VIRTUAL_KEYBOARD_FORTUNES-1);
					virtual_keyboard_globals.caption_index= FIRST_VIRTUAL_KEYBOARD_FORTUNE_STRING_INDEX+
						fortune_index;
					if (virtual_keyboard_globals.saved_text[0])
					{
						ustrcpy(virtual_keyboard_globals.text_buffer, virtual_keyboard_globals.saved_text);
						virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer+
							ustrlen(virtual_keyboard_globals.text_buffer);
					}
					else
					{
						csmemset(
							virtual_keyboard_globals.text_buffer,
							0,
							virtual_keyboard_globals.buffer_size);
						virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer;
					}
				}
				ui_play_audio_feedback_sound(2);
			}
			else
			{
				ui_play_audio_feedback_sound(4);
			}
		}
		break;
	}

	if (virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column]!=_virtual_key_shift)
		virtual_keyboard_globals.shift_active= FALSE;

	return TRUE;
}

static void virtual_keyboard_process_internal(
	void)
{
	static unsigned long time_of_last_tab;
	unsigned long time= system_milliseconds();
	struct event_record event;
	long action= NONE;
	boolean handled= FALSE;

	while (get_next_event(&event, NONE))
	{
		switch (event.type)
		{
		case _event_type_left_stick:
			if (event.data.stick.y==SHORT_MAX)
				action= _event_tab_up;
			else if (event.data.stick.y==SHORT_MIN)
				action= _event_tab_down;
			else if (event.data.stick.x==SHORT_MIN)
				action= _event_tab_left;
			else if (event.data.stick.x==SHORT_MAX)
				action= _event_tab_right;
			break;

		case _event_type_button:
			switch (event.data.button.index)
			{
			case _gamepad_analog_button_a:
				if (event.data.button.value==1)
					action= _event_key_select;
				break;

			case _gamepad_analog_button_b:
			case _gamepad_binary_button_back:
				if (event.data.button.value==1)
					action= _event_cancel;
				break;

			case _gamepad_binary_button_dpad_up:
				if (virtual_keyboard_globals.last_event!=_event_tab_up ||
					time-time_of_last_tab>=VIRTUAL_KEYBOARD_TAB_REPEAT_MILLISECONDS ||
					event.data.button.value==1)
				{
					action= _event_tab_up;
					time_of_last_tab= time;
				}
				break;

			case _gamepad_binary_button_dpad_left:
				if (virtual_keyboard_globals.last_event!=_event_tab_left ||
					time-time_of_last_tab>=VIRTUAL_KEYBOARD_TAB_REPEAT_MILLISECONDS ||
					event.data.button.value==1)
				{
					action= _event_tab_left;
					time_of_last_tab= time;
				}
				break;

			case _gamepad_binary_button_dpad_down:
				if (virtual_keyboard_globals.last_event!=_event_tab_down ||
					time-time_of_last_tab>=VIRTUAL_KEYBOARD_TAB_REPEAT_MILLISECONDS ||
					event.data.button.value==1)
				{
					action= _event_tab_down;
					time_of_last_tab= time;
				}
				break;

			case _gamepad_binary_button_dpad_right:
				if (virtual_keyboard_globals.last_event!=_event_tab_right ||
					time-time_of_last_tab>=VIRTUAL_KEYBOARD_TAB_REPEAT_MILLISECONDS ||
					event.data.button.value==1)
				{
					action= _event_tab_right;
					time_of_last_tab= time;
				}
				break;

			case _gamepad_binary_button_start:
				if (event.data.button.value==1)
				{
					virtual_keyboard_globals.row= 0;
					virtual_keyboard_globals.column= 0;
					action= _event_key_select;
				}
				break;

			case _gamepad_analog_button_x:
				if (event.data.button.value==1)
				{
					if (virtual_keyboard_globals.first_key_replaces_buffer==TRUE)
					{
						match_assert(
							"c:\\halo\\SOURCE\\interface\\virtual_keyboard.c",
							635,
							virtual_keyboard_globals.buffer_size>0);
						csmemset(
							virtual_keyboard_globals.text_buffer,
							0,
							virtual_keyboard_globals.buffer_size);
						virtual_keyboard_globals.cursor= virtual_keyboard_globals.text_buffer;
						virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
						ui_play_audio_feedback_sound(1);
					}
					else
					{
						code_000e5720();
					}
					handled= TRUE;
				}
				break;

			case _gamepad_analog_button_left_trigger:
				if (event.data.button.value==1)
				{
					virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
					if (virtual_keyboard_globals.cursor>virtual_keyboard_globals.text_buffer)
						virtual_keyboard_globals.cursor--;
					ui_play_audio_feedback_sound(1);
					handled= TRUE;
				}
				break;

			case _gamepad_analog_button_right_trigger:
				if (event.data.button.value==1)
				{
					virtual_keyboard_globals.first_key_replaces_buffer= FALSE;
					if (*virtual_keyboard_globals.cursor)
						virtual_keyboard_globals.cursor++;
					ui_play_audio_feedback_sound(1);
					handled= TRUE;
				}
				break;
			}
			break;
		}
	}

	if (action!=NONE)
	{
		virtual_keyboard_globals.last_key= virtual_keyboard_key_layout[
			virtual_keyboard_globals.row][virtual_keyboard_globals.column];
		switch (action)
		{
		case _event_tab_left: handled= code_000e4e10(); break;
		case _event_tab_right: handled= code_000e4e60(); break;
		case _event_tab_up: handled= code_000e4eb0(); break;
		case _event_tab_down: handled= code_000e4f00(); break;
		case _event_key_select: handled= virtual_keyboard_select(); break;
		case _event_cancel: handled= code_000e4f50(); break;
		}

		if (handled==TRUE)
		{
			virtual_keyboard_globals.time_of_last_event= time;
			virtual_keyboard_globals.last_event= action;
		}
	}

	return;
}
