/*
INPUT_XBOX.C

symbols in this file:
000BE780 0050:
	_fix_dead_zone (0000)
000BE7D0 0030:
	_update_ticks (0000)
000BE800 0030:
	_code_000be800 (0000)
000BE830 0050:
	_input_dispose (0000)
000BE880 0010:
	_input_activate (0000)
000BE890 0010:
	_input_deactivate (0000)
000BE8A0 0060:
	_input_flush (0000)
000BE900 00c0:
	_input_key_is_down (0000)
000BE9C0 0070:
	_input_get_key (0000)
000BEA30 0010:
	_input_get_mouse_state (0000)
000BEA40 0010:
	_input_suppress (0000)
000BEA50 0010:
	_input_mouse_button_is_down (0000)
000BEA60 0050:
	_input_has_gamepad (0000)
000BEAB0 0070:
	_input_get_gamepad_state (0000)
000BEB20 0060:
	_input_set_gamepad_rumbler_state (0000)
000BEB80 0030:
	_input_vertical_blank_interrupt (0000)
000BEBB0 0010:
	_code_000bebb0 (0000)
000BEBC0 0010:
	_code_000bebc0 (0000)
000BEBD0 04d0:
	_code_000bebd0 (0000)
000BF0A0 00b0:
	_code_000bf0a0 (0000)
000BF150 0280:
	_code_000bf150 (0000)
000BF3D0 00f0:
	_input_get_raw_data_string (0000)
000BF4C0 0060:
	_input_update (0000)
000BF520 0010:
	_input_frame_begin (0000)
000BF530 0010:
	_input_frame_end (0000)
000BF540 0020:
	_code_000bf540 (0000)
000BF560 0100:
	_input_initialize (0000)
0026F570 0310:
	_rdata_0026f570 (0000)
0026F880 0027:
	??_C@_0CH@PJPACDN@key_code?$DO?$DN0?5?$CG?$CG?5key_code?$DMNUMBER_O@ (0000)
0026F8A8 0022:
	??_C@_0CC@GELNFCFL@c?3?2halo?2SOURCE?2input?2input_xbox?4@ (0000)
0026F8D0 006e:
	??_C@_0GO@PMFMJCLJ@input_globals?4buffered_key_read_@ (0000)
0026F940 0033:
	??_C@_0DD@KEPDEKGB@gamepad_index?$DO?$DN0?5?$CG?$CG?5gamepad_inde@ (0000)
0026F974 0037:
	??_C@_0DH@CKEAMKJJ@XGetState?5?$CIgamepad?$CJ?5failed?5?$CI?$CD?$CFd?$CJ@ (0000)
0026F9AC 0038:
	??_C@_0DI@FEEPFG@XInputOpen?5?$CIgamepad?$CJ?5failed?5?$CI?$CD?$CFd@ (0000)
0026F9E4 0033:
	??_C@_0DD@LHONKLFJ@input_globals?4gamepad_handles?$FLga@ (0000)
0026FA18 002d:
	??_C@_0CN@LLLCEEMO@input_globals?4gamepad_handles?$FLga@ (0000)
0026FA48 002f:
	??_C@_0CP@EHHPCBHL@key?4key_code?$DO?$DN0?5?$CG?$CG?5key?4key_code?$DM@ (0000)
0026FA78 0048:
	??_C@_0EI@INGGDNKP@keystroke?4VirtualKey?$DO?$DN0?5?$CG?$CG?5keyst@ (0000)
0026FAC0 003c:
	??_C@_0DM@NFGLODNP@keystroke?4Ascii?$DO?$DN0?5?$CG?$CG?5keystroke?4@ (0000)
0026FAFC 0039:
	??_C@_0DJ@CLMKEEPN@XInputOpen?5?$CIkeyboard?$CJ?5failed?5?$CI?$CD?$CF@ (0000)
0026FB38 0024:
	??_C@_0CE@HLFAFMJM@input_globals?4keyboard_handle?$DN?$DNN@ (0000)
0026FB5C 001e:
	??_C@_0BO@BOABODDP@input_globals?4keyboard_handle?$AA@ (0000)
0026FB7C 0021:
	??_C@_0CB@EFGOEOI@gamepad?5?$CFd?$HMt?$CI?$CFd?0?5?$CFd?$CJ?$HMt?$CI?$CFd?0?5?$CFd?$CJ?$HMn@ (0000)
0026FBA0 002d:
	??_C@_0CN@OAEKILIO@?$HMn?$HMn?$HMn?$HMngamepad?$HMtleft?5stick?$HMtrig@ (0000)
0026FBD0 0007:
	??_C@_06NOMBLFJD@size?$DO0?$AA@ (0000)
0026FBD8 0044:
	??_C@_0EE@KIBLAJLM@XInputDebugInitKeyboardQueue?5fai@ (0000)
004536A0 040c:
	_bss_004536a0 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "input.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct input_globals
{
	unsigned char reserved0[0x138];
	boolean suppressed;
	unsigned char reserved1[3];
	void *gamepad_handles[MAXIMUM_GAMEPADS];
	struct gamepad_state gamepad_states[MAXIMUM_GAMEPADS];
	struct gamepad_state suppressed_gamepad_state;
	unsigned char reserved2[0x19];
	boolean frame_active;
	unsigned char reserved3[0x1DE];
};

typedef char verify_input_suppressed_offset[
	offsetof(struct input_globals, suppressed) == 0x138 ? 1 : -1];
typedef char verify_input_gamepad_handles_offset[
	offsetof(struct input_globals, gamepad_handles) == 0x13C ? 1 : -1];
typedef char verify_input_gamepad_states_offset[
	offsetof(struct input_globals, gamepad_states) == 0x14C ? 1 : -1];
typedef char verify_input_suppressed_gamepad_state_offset[
	offsetof(struct input_globals, suppressed_gamepad_state) == 0x1EC ? 1 : -1];
typedef char verify_input_frame_active_offset[
	offsetof(struct input_globals, frame_active) == 0x22D ? 1 : -1];
typedef char verify_input_globals_size[
	sizeof(struct input_globals) == 0x40C ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct input_globals bss_004536a0;

/* ---------- public code */

short fix_dead_zone(
	short value,
	short dead_range)
{
	if (value > dead_range)
	{
		return (short)((value - dead_range) * SHORT_MAX / (SHORT_MAX - dead_range));
	}

	if (value < -dead_range)
	{
		return (short)((value + dead_range) * SHORT_MIN / (SHORT_MIN + dead_range));
	}

	return 0;
}

void update_ticks(
	byte *ticks,
	boolean down)
{
	long value;

	if (down)
	{
		value = *ticks + 1;
		if (value > UNSIGNED_CHAR_MAX)
		{
			value = UNSIGNED_CHAR_MAX;
		}
	}
	else
	{
		value = 0;
	}

	*ticks = (byte)value;

	return;
}

void code_000bebb0(
	void)
{
	return;
}

void code_000bebc0(
	void)
{
	return;
}

void input_suppress(
	void)
{
	bss_004536a0.suppressed = TRUE;

	return;
}

void input_activate(
	void)
{
	return;
}

void input_deactivate(
	void)
{
	return;
}

const struct mouse_state *input_get_mouse_state(
	void)
{
	return NULL;
}

boolean input_mouse_button_is_down(
	short button_index)
{
	return FALSE;
}

const struct gamepad_state *input_get_gamepad_state(
	short gamepad_index)
{
	const struct gamepad_state *result = NULL;

	match_assert(
		"c:\\halo\\SOURCE\\input\\input_xbox.c",
		0x17B,
		gamepad_index>=0 && gamepad_index<MAXIMUM_GAMEPADS);

	if (bss_004536a0.gamepad_handles[gamepad_index])
	{
		result = bss_004536a0.suppressed
			? &bss_004536a0.suppressed_gamepad_state
			: &bss_004536a0.gamepad_states[gamepad_index];
	}

	return result;
}

void input_frame_end(
	void)
{
	bss_004536a0.frame_active = FALSE;

	return;
}

/* ---------- private code */
