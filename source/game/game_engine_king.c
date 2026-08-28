/*
GAME_ENGINE_KING.C

symbols in this file:
000A0030 0010:
	_code_000a0030 (0000)
000A0040 0020:
	_code_000a0040 (0000)
000A0060 0350:
	_code_000a0060 (0000)
000A03B0 0010:
	_code_000a03b0 (0000)
000A03C0 0020:
	_code_000a03c0 (0000)
000A03E0 0010:
	_code_000a03e0 (0000)
000A03F0 0020:
	_code_000a03f0 (0000)
000A0410 0010:
	_code_000a0410 (0000)
000A0420 0010:
	_code_000a0420 (0000)
000A0430 0010:
	_code_000a0430 (0000)
000A0440 0010:
	_code_000a0440 (0000)
000A0450 0090:
	_code_000a0450 (0000)
000A04E0 0160:
	_code_000a04e0 (0000)
000A0640 01c0:
	_code_000a0640 (0000)
000A0800 0010:
	_code_000a0800 (0000)
000A0810 0010:
	_code_000a0810 (0000)
000A0820 0190:
	_code_000a0820 (0000)
000A09B0 0010:
	_code_000a09b0 (0000)
000A09C0 0040:
	_code_000a09c0 (0000)
000A0A00 0090:
	_render_dynamic_quad_initialize (0000)
000A0A90 02b0:
	_render_dynamic_quad (0000)
000A0D40 0040:
	_code_000a0d40 (0000)
000A0D80 0060:
	_code_000a0d80 (0000)
000A0DE0 0030:
	_code_000a0de0 (0000)
000A0E10 0020:
	_code_000a0e10 (0000)
000A0E30 0070:
	_code_000a0e30 (0000)
000A0EA0 0110:
	_code_000a0ea0 (0000)
000A0FB0 03a0:
	_code_000a0fb0 (0000)
000A1350 0110:
	_code_000a1350 (0000)
0025BDC0 000d:
	??_C@_0N@DGPCNCJC@NULL?5?$CB?$DN?5flag?$AA@ (0000)
0025BDD0 0027:
	??_C@_0CH@IHPMMFJJ@c?3?2halo?2SOURCE?2game?2game_engine_@ (0000)
0025BDF8 0023:
	??_C@_0CD@DHKICDEA@king_globals?4hill_point_count?5?$CB?$DN@ (0000)
0025BE1C 0014:
	??_C@_0BE@EEOONBNI@FAILED?5TO?5FIND?5HILL?$AA@ (0000)
0025BE30 000b:
	??_C@_0L@HEFOOJCG@crown_blue?$AA@ (0000)
0025BE3C 0038:
	??_C@_0DI@GPEOKAOO@failed?5to?5find?5hill?5?$CD?$CFd?5most?5lik@ (0000)
002DE488 0088:
	_king_engine (0000)
0043E948 0230:
	_bss_0043e948 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game_engine.h"
#include "players.h"

/* ---------- constants */

enum
{
	MAXIMUM_KING_SCORE_SLOTS = 16,
	_multiplayer_sound_team_king_of_the_hill = 0x20,
	_multiplayer_sound_king_of_the_hill = 0x24,
};

/* ---------- macros */

/* ---------- structures */

struct king_globals_prefix
{
	long scores[MAXIMUM_KING_SCORE_SLOTS];
};

/* ---------- prototypes */

/* ---------- globals */

extern struct king_globals_prefix bss_0043e948;

/* ---------- public code */

void code_000a0030(
	void)
{
	return;
}

void code_000a03b0(
	void)
{
	return;
}

void code_000a03e0(
	void)
{
	return;
}

void code_000a0410(
	void)
{
	return;
}

void code_000a0420(
	void)
{
	return;
}

void code_000a0430(
	void)
{
	return;
}

void code_000a0440(
	void)
{
	return;
}

void code_000a0800(
	void)
{
	return;
}

void code_000a0810(
	void)
{
	return;
}

void code_000a09b0(
	void)
{
	return;
}

void code_000a03c0(
	long player_index)
{
	player_get(player_index);

	return;
}

void code_000a03f0(
	void)
{
	game_engine_play_multiplayer_sound(
		game_engine_has_teams() ?
			_multiplayer_sound_team_king_of_the_hill :
			_multiplayer_sound_king_of_the_hill);

	return;
}

wchar_t *code_000a0d40(
	long player_index,
	wchar_t *buffer)
{
	struct player_datum *player = player_get(player_index);

	ticks_to_unicode_time_string(
		player->statistics.multiplayer_statistics.king_statistics.time_on_hill,
		256,
		buffer);

	return buffer;
}

wchar_t *code_000a0de0(
	long team_index,
	wchar_t *buffer)
{
	ticks_to_unicode_time_string(
		bss_0043e948.scores[team_index],
		256,
		buffer);

	return buffer;
}

/* ---------- private code */
