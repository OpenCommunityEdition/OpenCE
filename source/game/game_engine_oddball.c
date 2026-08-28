/*
GAME_ENGINE_ODDBALL.C

symbols in this file:
000A1630 0010:
	_code_000a1630 (0000)
000A1640 0010:
	_code_000a1640 (0000)
000A1650 0020:
	_code_000a1650 (0000)
000A1670 0010:
	_code_000a1670 (0000)
000A1680 0010:
	_code_000a1680 (0000)
000A1690 0010:
	_code_000a1690 (0000)
000A16A0 0010:
	_code_000a16a0 (0000)
000A16B0 0010:
	_code_000a16b0 (0000)
000A16C0 0010:
	_code_000a16c0 (0000)
000A16D0 0010:
	_code_000a16d0 (0000)
000A16E0 00d0:
	_code_000a16e0 (0000)
000A17B0 0040:
	_code_000a17b0 (0000)
000A17F0 0030:
	_code_000a17f0 (0000)
000A1820 0010:
	_code_000a1820 (0000)
000A1830 0030:
	_player_has_ball (0000)
000A1860 0040:
	_ball_available (0000)
000A18A0 02e0:
	_code_000a18a0 (0000)
000A1B80 0010:
	_code_000a1b80 (0000)
000A1B90 0040:
	_code_000a1b90 (0000)
000A1BD0 0040:
	_code_000a1bd0 (0000)
000A1C10 0020:
	_code_000a1c10 (0000)
000A1C30 0020:
	_code_000a1c30 (0000)
000A1C50 0020:
	_code_000a1c50 (0000)
000A1C70 0020:
	_code_000a1c70 (0000)
000A1C90 0050:
	_code_000a1c90 (0000)
000A1CE0 0050:
	_code_000a1ce0 (0000)
000A1D30 0070:
	_code_000a1d30 (0000)
000A1DA0 0050:
	_code_000a1da0 (0000)
000A1DF0 0140:
	_code_000a1df0 (0000)
000A1F30 0090:
	_code_000a1f30 (0000)
000A1FC0 0120:
	_code_000a1fc0 (0000)
000A20E0 0070:
	_code_000a20e0 (0000)
000A2150 0090:
	_code_000a2150 (0000)
000A21E0 01c0:
	_code_000a21e0 (0000)
000A23A0 00c0:
	_code_000a23a0 (0000)
000A2460 00d0:
	_code_000a2460 (0000)
000A2530 01c0:
	_code_000a2530 (0000)
000A26F0 00b0:
	_code_000a26f0 (0000)
0025BE74 002a:
	??_C@_0CK@LKLFFONL@c?3?2halo?2SOURCE?2game?2game_engine_@ (0000)
0025BEA0 0034:
	??_C@_0DE@EJKHILOB@0?5?$CG?$CG?5?$CCthis?5map?5was?5not?5correctly@ (0000)
0025BED8 0040:
	??_C@_0EA@MPBOOMHB@?$CD?$CD?$CD?5failed?5to?5find?5any?5suitable?5@ (0000)
0025BF18 0023:
	??_C@_0CD@CKBFGHJI@?$CD?$CD?$CD?5failed?5to?5find?5the?5flag?5obje@ (0000)
0025BF3C 0028:
	??_C@_0CI@KJBBGHKH@?$CD?$CD?$CD?5failed?5to?5find?5ball?5spawn?5te@ (0000)
0025BF64 0028:
	??_C@_0CI@LAAKFGOG@?$CD?$CD?$CD?5failed?5to?5find?5ball?5spawn?5te@ (0000)
0025BF8C 000a:
	??_C@_09GMEHKGKL@ball_blue?$AA@ (0000)
0025BF98 000c:
	??_C@_0M@FNMEMKCB@target_blue?$AA@ (0000)
0025BFA4 002d:
	??_C@_0CN@DJFDFML@?$CBball_available?$CI?$CJ?5?$HM?$HM?5?$CIcapture_in@ (0000)
002DE560 0088:
	_oddball_engine (0000)
0043EBA8 0104:
	_bss_0043eba8 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "items/weapons.h"
#include "players.h"

/* ---------- constants */

enum
{
	MAXIMUM_ODDBALLS = 16,
};

/* ---------- macros */

/* ---------- structures */

struct oddball_globals
{
	long score_to_win;
	long team_score[MAXIMUM_ODDBALLS];
	long individual_score[MAXIMUM_ODDBALLS];
	long ball_spawn_timer[MAXIMUM_ODDBALLS];
	long current_ball_owner[MAXIMUM_ODDBALLS];
};

typedef char verify_oddball_globals_size[
	sizeof(struct oddball_globals) == 0x104 ? 1 : -1];
typedef char verify_oddball_globals_ball_spawn_timer_offset[
	offsetof(struct oddball_globals, ball_spawn_timer) == 0x84 ? 1 : -1];
typedef char verify_oddball_globals_current_ball_owner_offset[
	offsetof(struct oddball_globals, current_ball_owner) == 0xC4 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct oddball_globals bss_0043eba8 = { 0 };

/* ---------- public code */

void code_000a1630(
	void)
{
	return;
}

void code_000a1640(
	void)
{
	return;
}

void code_000a1670(
	void)
{
	return;
}

void code_000a1680(
	void)
{
	return;
}

void code_000a1690(
	void)
{
	return;
}

void code_000a16a0(
	void)
{
	return;
}

void code_000a16b0(
	void)
{
	return;
}

void code_000a16c0(
	void)
{
	return;
}

void code_000a16d0(
	void)
{
	return;
}

void code_000a1820(
	void)
{
	return;
}

void code_000a1b80(
	void)
{
	return;
}

void code_000a1650(
	long player_index)
{
	player_get(player_index);

	return;
}

void code_000a1b90(
	long weapon_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\game_engine_oddball.c",
		0x3C8,
		weapon_is_flag(weapon_index));

	return;
}

/* ---------- private code */
