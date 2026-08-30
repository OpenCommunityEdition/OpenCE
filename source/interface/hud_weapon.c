/*
HUD_WEAPON.C

symbols in this file:
000C7E70 0040:
	_hud_initialize_weapon_interface (0000)
000C7EB0 0040:
	_hud_initialize_weapon_interface_for_new_map (0000)
000C7EF0 0010:
	_hud_dispose_weapon_interface_from_old_map (0000)
000C7F00 0010:
	_hud_dispose_weapon_interface (0000)
000C7F10 0030:
	_scripted_hud_show_crosshair (0000)
000C7F40 0070:
	_code_000c7f40 (0000)
000C7FB0 0070:
	_code_000c7fb0 (0000)
000C8020 0050:
	_code_000c8020 (0000)
000C8070 02e0:
	_code_000c8070 (0000)
000C8350 0020:
	_strip_path_name (0000)
000C8370 08d0:
	_code_000c8370 (0000)
000C8C40 00a0:
	_hud_fix_weapon_data (0000)
000C8CE0 05c0:
	_code_000c8ce0 (0000)
000C92A0 0a60:
	_code_000c92a0 (0000)
000C9D00 0270:
	_hud_update_weapon (0000)
000C9F70 01f0:
	_hud_render_weapon_interface (0000)
002702F8 0013:
	??_C@_0BD@NOPIHEPB@weapon_hud_globals?$AA@ (0000)
0027030C 0026:
	??_C@_0CG@DOLNIFA@c?3?2halo?2SOURCE?2interface?2hud_wea@ (0000)
00270334 0015:
	??_C@_0BF@HCPMNCNB@hud?5weapon?5interface?$AA@ (0000)
00270350 0008:
	__real@3ff4000000000000 (0000)
00270358 0052:
	??_C@_0FC@PHPDNACJ@frame?5index?5NONE?5when?5drawing?5cr@ (0000)
002703AC 0030:
	??_C@_0DA@GANDHJMM@too?5many?5levels?5in?5current?5weapo@ (0000)
00453AC4 0004:
	_bss_00453ac4 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "interface/hud_weapon.h"
#include "saved games/game_state.h"

#include <string.h>

/* ---------- constants */

/* ---------- macros */

#define weapon_hud_globals bss_00453ac4

/* ---------- structures */

struct weapon_hud_globals
{
	byte reserved[0x1E0];
	long script_flags;
};

typedef char weapon_hud_globals_script_flags_offset_assert[
	offsetof(struct weapon_hud_globals, script_flags) == 0x1E0 ? 1 : -1];
typedef char weapon_hud_globals_size_assert[
	sizeof(struct weapon_hud_globals) == 0x1E4 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct weapon_hud_globals *bss_00453ac4;

/* ---------- public code */

void hud_initialize_weapon_interface(
	void)
{
	weapon_hud_globals = game_state_malloc(
		"hud weapon interface",
		NULL,
		sizeof(*weapon_hud_globals));

	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_weapon.c",
		0x6B,
		weapon_hud_globals);

	return;
}

void hud_initialize_weapon_interface_for_new_map(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_weapon.c",
		0x73,
		weapon_hud_globals);

	csmemset(
		weapon_hud_globals,
		NONE,
		sizeof(*weapon_hud_globals));

	return;
}

void hud_dispose_weapon_interface_from_old_map(
	void)
{
	return;
}

void hud_dispose_weapon_interface(
	void)
{
	return;
}

/* ---------- private code */

char *strip_path_name(
	char *path)
{
	char *name = strrchr(path, '\\');

	if (name)
		return name + 1;

	return path;
}
