/*
GAME_ENGINE_SLAYER.C

symbols in this file:
000A3CF0 0050:
	_code_000a3cf0 (0000)
000A3D40 0010:
	_slayer_engine_dispose (0000)
000A3D50 0030:
	_slayer_engine_initialize_for_new_map (0000)
000A3D80 0010:
	_slayer_engine_dispose_from_old_map (0000)
000A3D90 0030:
	_slayer_engine_player_added (0000)
000A3DC0 0010:
	_slayer_engine_game_ending (0000)
000A3DD0 0020:
	_slayer_engine_game_starting (0000)
000A3DF0 0010:
	_slayer_engine_statistics_append (0000)
000A3E00 0010:
	_slayer_engine_handle_client_message (0000)
000A3E10 0010:
	_slayer_engine_handle_server_message (0000)
000A3E20 0010:
	_slayer_engine_pregame_post_rasterize (0000)
000A3E30 0010:
	_slayer_engine_post_rasterize (0000)
000A3E40 0010:
	_slayer_engine_update (0000)
000A3E50 0010:
	_slayer_engine_allow_pick_up (0000)
000A3E60 0010:
	_slayer_engine_player_damaged_player (0000)
000A3E70 0110:
	_update_speed_for_score (0000)
000A3F80 0040:
	_code_000a3f80 (0000)
000A3FC0 0010:
	_slayer_engine_prespawn_player_update (0000)
000A3FD0 0040:
	_slayer_get_score (0000)
000A4010 0010:
	_code_000a4010 (0000)
000A4020 0030:
	_slayer_get_score_string (0000)
000A4050 0060:
	_code_000a4050 (0000)
000A40B0 0030:
	_slayer_get_team_score_string (0000)
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
	_slayer_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#include "game/game_engine.h"
#include "game/players.h"
#include "text/unicode.h"

/* ---------- constants */

enum
{
	_multiplayer_sound_slayer = 0x15,
	_multiplayer_sound_team_slayer = 0x23,
};

/* ---------- macros */

/* ---------- structures */

struct slayer_globals
{
	long team_score[16];
	long individual_score[16];
};

typedef char verify_slayer_globals_size[
	sizeof(struct slayer_globals) == 0x80 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct slayer_globals slayer_globals = { 0 };

/* ---------- public code */

void slayer_engine_dispose(
	void)
{
	return;
}

boolean slayer_engine_initialize_for_new_map(
	void)
{
	csmemset(slayer_globals.team_score, 0, sizeof(slayer_globals.team_score));
	csmemset(slayer_globals.individual_score, 0, sizeof(slayer_globals.individual_score));

	return TRUE;
}

void slayer_engine_dispose_from_old_map(
	void)
{
	return;
}

void slayer_engine_game_ending(
	void)
{
	return;
}

void slayer_engine_post_rasterize(
	void)
{
	return;
}

void slayer_engine_player_damaged_player(
	long damaging_player_index,
	long dead_player_index,
	boolean damage_type)
{
	return;
}

void slayer_engine_prespawn_player_update(
	long player_index)
{
	return;
}

void slayer_engine_statistics_append(
	long statistic)
{
	return;
}

void slayer_engine_handle_client_message(
	void *message)
{
	return;
}

void slayer_engine_handle_server_message(
	void *message)
{
	return;
}

void slayer_engine_pregame_post_rasterize(
	void)
{
	return;
}

void slayer_engine_update(
	void)
{
	return;
}

void slayer_engine_player_added(
	long player_index)
{
	struct player_datum *player = player_get(player_index);

	player->unknown88 = NONE;

	return;
}

void slayer_engine_game_starting(
	void)
{
	game_engine_play_multiplayer_sound(
		game_engine_has_teams()
			? _multiplayer_sound_team_slayer
			: _multiplayer_sound_slayer);

	return;
}

boolean slayer_engine_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	return TRUE;
}

long slayer_get_score(
	long player_index,
	enum get_score_type score_type)
{
	struct player_datum *player = player_get(player_index);

	if (score_type == _get_score_team)
		return slayer_globals.team_score[player->team_index];

	return slayer_globals.individual_score[
		DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)];
}

wchar_t *slayer_get_score_string(
	long player_index,
	wchar_t *buffer)
{
	usprintf(
		buffer,
		L"%d",
		slayer_globals.individual_score[
			DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)]);

	return buffer;
}

wchar_t *slayer_get_team_score_string(
	long team_index,
	wchar_t *buffer)
{
	usprintf(
		buffer,
		L"%d",
		slayer_globals.team_score[team_index]);

	return buffer;
}

/* ---------- private code */
