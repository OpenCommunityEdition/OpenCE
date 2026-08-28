/*
GAME_ENGINE_SLAYER.C

symbols in this file:
000A3CF0 0050:
	_code_000a3cf0 (0000)
000A3D40 0010:
	_code_000a3d40 (0000)
000A3D50 0030:
	_code_000a3d50 (0000)
000A3D80 0010:
	_code_000a3d80 (0000)
000A3D90 0030:
	_code_000a3d90 (0000)
000A3DC0 0010:
	_code_000a3dc0 (0000)
000A3DD0 0020:
	_code_000a3dd0 (0000)
000A3DF0 0010:
	_code_000a3df0 (0000)
000A3E00 0010:
	_code_000a3e00 (0000)
000A3E10 0010:
	_code_000a3e10 (0000)
000A3E20 0010:
	_code_000a3e20 (0000)
000A3E30 0010:
	_code_000a3e30 (0000)
000A3E40 0010:
	_code_000a3e40 (0000)
000A3E50 0010:
	_code_000a3e50 (0000)
000A3E60 0010:
	_code_000a3e60 (0000)
000A3E70 0110:
	_update_speed_for_score (0000)
000A3F80 0040:
	_code_000a3f80 (0000)
000A3FC0 0010:
	_code_000a3fc0 (0000)
000A3FD0 0040:
	_code_000a3fd0 (0000)
000A4010 0010:
	_code_000a4010 (0000)
000A4020 0030:
	_code_000a4020 (0000)
000A4050 0060:
	_code_000a4050 (0000)
000A40B0 0030:
	_code_000a40b0 (0000)
000A40E0 0190:
	_code_000a40e0 (0000)
000A4270 0090:
	_code_000a4270 (0000)
000A4300 0280:
	_code_000a4300 (0000)
000A4580 0190:
	_code_000a4580 (0000)
0025C190 0014:
	??_C@_0BE@HDNMKCGO@next_target?5?$CB?$DN?5NONE?$AA@ (0000)
0025C1A4 0029:
	??_C@_0CJ@GAGEHHLD@c?3?2halo?2SOURCE?2game?2game_engine_@ (0000)
0025C1D0 0036:
	??_C@_0DG@HCMPGGBD@?$CIindex?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIindex?5?$DM?5MULTIPL@ (0000)
0025C208 0004:
	__real@373a69dc (0000)
0025C20C 0004:
	__real@38e90453 (0000)
002DE670 0088:
	_slayer_engine (0000)
0043ED80 0080:
	_bss_0043ed80 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#include "game/game_engine.h"
#include "game/players.h"

/* ---------- constants */

enum
{
	_multiplayer_sound_slayer = 0x15,
	_multiplayer_sound_team_slayer = 0x23,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void code_000a3d40(
	void)
{
	return;
}

void code_000a3d80(
	void)
{
	return;
}

void code_000a3dc0(
	void)
{
	return;
}

void code_000a3e30(
	void)
{
	return;
}

void code_000a3e60(
	long player_index,
	long damage_owner_player_index,
	boolean damage_was_melee)
{
	return;
}

void code_000a3fc0(
	long player_index)
{
	return;
}

void code_000a3df0(
	void)
{
	return;
}

void code_000a3e00(
	void)
{
	return;
}

void code_000a3e10(
	void)
{
	return;
}

void code_000a3e20(
	void)
{
	return;
}

void code_000a3e40(
	void)
{
	return;
}

void code_000a3d90(
	long player_index)
{
	struct player_datum *player = player_get(player_index);

	player->unknown88 = NONE;

	return;
}

void code_000a3dd0(
	void)
{
	game_engine_play_multiplayer_sound(
		game_engine_has_teams()
			? _multiplayer_sound_team_slayer
			: _multiplayer_sound_slayer);

	return;
}

boolean code_000a3e50(
	long unit_index,
	long weapon_index)
{
	return TRUE;
}

/* ---------- private code */
