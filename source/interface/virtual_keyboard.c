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
#include "interface/virtual_keyboard.h"

/* ---------- constants */

enum
{
	VIRTUAL_KEYBOARD_ROW_COUNT = 5,
	VIRTUAL_KEYBOARD_COLUMN_COUNT = 11,
};

/* ---------- macros */

/* ---------- structures */

struct virtual_keyboard_globals
{
	boolean active;
	byte reserved1[7];
	short row;
	short column;
	unsigned short buffer_size;
	byte reservedE[0x8];
	boolean last_exit_saved_text;
	byte reserved17;
	wchar_t *text_buffer;
	wchar_t *cursor;
	byte reserved20[8];
	wchar_t saved_text[34];
};

typedef char verify_virtual_keyboard_globals_size[
	sizeof(struct virtual_keyboard_globals) == 0x6C ? 1 : -1];

/* ---------- prototypes */

boolean code_000e4f50(
	void);
void code_000e50b0(
	void);
void code_000e5be0(
	void);
unsigned long ustrlen(
	wchar_t const *string);
wchar_t *ustrncpy(
	wchar_t *destination,
	wchar_t const *source,
	unsigned long count);
void ui_play_audio_feedback_sound(
	short audio_feedback);

/* ---------- globals */

byte const virtual_keyboard_key_layout[VIRTUAL_KEYBOARD_ROW_COUNT][VIRTUAL_KEYBOARD_COLUMN_COUNT] =
{
	{ 0x24, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
	{ 0x25, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 },
	{ 0x26, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D },
	{ 0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x28, 0x28, 0x28, 0x28 },
	{ 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x29, 0x29, 0x2A, 0x2A },
};

struct virtual_keyboard_globals virtual_keyboard_globals;

/* ---------- public code */

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
	byte keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

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
	byte keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

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
	byte keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

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
	byte keycode = virtual_keyboard_key_layout[virtual_keyboard_globals.row][virtual_keyboard_globals.column];

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
		code_000e5be0();

	return;
}

/* ---------- private code */
