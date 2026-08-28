/*
GAME_ENGINE_CTF.C

symbols in this file:
0009EB60 0060:
	_code_0009eb60 (0000)
0009EBC0 0010:
	_code_0009ebc0 (0000)
0009EBD0 0060:
	_code_0009ebd0 (0000)
0009EC30 0040:
	_code_0009ec30 (0000)
0009EC70 0010:
	_code_0009ec70 (0000)
0009EC80 0020:
	_code_0009ec80 (0000)
0009ECA0 0010:
	_code_0009eca0 (0000)
0009ECB0 0010:
	_code_0009ecb0 (0000)
0009ECC0 0010:
	_code_0009ecc0 (0000)
0009ECD0 0010:
	_code_0009ecd0 (0000)
0009ECE0 0010:
	_code_0009ece0 (0000)
0009ECF0 0010:
	_code_0009ecf0 (0000)
0009ED00 0010:
	_code_0009ed00 (0000)
0009ED10 00c0:
	_code_0009ed10 (0000)
0009EDD0 0040:
	_code_0009edd0 (0000)
0009EE10 0070:
	_code_0009ee10 (0000)
0009EE80 0080:
	_code_0009ee80 (0000)
0009EF00 0010:
	_code_0009ef00 (0000)
0009EF10 0010:
	_code_0009ef10 (0000)
0009EF20 03a0:
	_code_0009ef20 (0000)
0009F2C0 0010:
	_code_0009f2c0 (0000)
0009F2D0 0010:
	_code_0009f2d0 (0000)
0009F2E0 0040:
	_code_0009f2e0 (0000)
0009F320 0020:
	_code_0009f320 (0000)
0009F340 0040:
	_code_0009f340 (0000)
0009F380 0040:
	_code_0009f380 (0000)
0009F3C0 0010:
	_code_0009f3c0 (0000)
0009F3D0 0040:
	_code_0009f3d0 (0000)
0009F410 0060:
	_code_0009f410 (0000)
0009F470 0030:
	_code_0009f470 (0000)
0009F4A0 03d0:
	_code_0009f4a0 (0000)
0009F870 0050:
	_code_0009f870 (0000)
0009F8C0 0090:
	_code_0009f8c0 (0000)
0009F950 0050:
	_code_0009f950 (0000)
0009F9A0 0150:
	_code_0009f9a0 (0000)
0009FAF0 0240:
	_code_0009faf0 (0000)
0009FD30 0080:
	_code_0009fd30 (0000)
0009FDB0 0160:
	_code_0009fdb0 (0000)
0009FF10 0120:
	_code_0009ff10 (0000)
0025BBC4 000f:
	??_C@_0P@HPMPGFLN@created?5a?5flag?$AA@ (0000)
0025BBD4 001a:
	??_C@_0BK@JCPFMGLB@failed?5to?5create?5the?5flag?$AA@ (0000)
0025BBF0 0015:
	??_C@_0BF@KKINPGPB@NONE?5?$CB?$DN?5weapon_index?$AA@ (0000)
0025BC08 0026:
	??_C@_0CG@KLNGAFMF@c?3?2halo?2SOURCE?2game?2game_engine_@ (0000)
0025BC30 0013:
	??_C@_0BD@MAOGAFHI@NONE?5?$CB?$DN?5team_index?$AA@ (0000)
0025BC44 0018:
	??_C@_0BI@HLCGDKNJ@game_engine_can_score?$CI?$CJ?$AA@ (0000)
0025BC60 004b:
	??_C@_0EL@BLLOIBND@NETGAME_FLAG_WARNING?5starting?5lo@ (0000)
0025BCAC 003c:
	??_C@_0DM@LOHOHMDO@NETGAME_FLAG_WARNING?5starting?5lo@ (0000)
0025BCE8 002f:
	??_C@_0CP@KMKIMAED@?$CIflag_to_create?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIflag_t@ (0000)
0025BD18 0024:
	??_C@_0CE@FKOBANOL@failed?5to?5find?5one?5of?5the?5ctf?5fl@ (0000)
0025BD3C 001d:
	??_C@_0BN@PNGPDONM@ctf?5started?5up?5without?5teams?$AA@ (0000)
0025BD5C 0013:
	??_C@_0BD@CBKEKJFH@NONE?5?$CB?$DN?5unit_index?$AA@ (0000)
0025BD70 0036:
	??_C@_0DG@PMEHEDDI@weapon?9?$DOobject?4owner_team_index?5@ (0000)
0025BDA8 000a:
	??_C@_09NOBKAKN@flag_blue?$AA@ (0000)
0025BDB8 0008:
	__real@3fd51eb860000000 (0000)
002DE400 0088:
	_ctf_engine (0000)
0043E914 0030:
	_bss_0043e914 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game_engine.h"
#include "players.h"
#include "items/weapons.h"
#include "text/unicode.h"

/* ---------- constants */

enum
{
	_ctf_weapon_handled_bit = 6,
	_multiplayer_sound_capture_the_flag = 0x16,
};

/* ---------- macros */

/* ---------- structures */

struct scenario_netgame_flag;

struct ctf_globals
{
	struct scenario_netgame_flag *flags[2];
	long weapon_indices[2];
	long scores[2];
	long score_to_win;
	boolean flag_warnings[2];
	byte pad1E[2];
	long flag_warning_ticks[2];
	long flag_swap_timer;
	long next_flag_failure_time;
};

typedef char verify_ctf_globals_scores_offset[
	offsetof(struct ctf_globals, scores) == 0x10 ? 1 : -1];
typedef char verify_ctf_globals_size[
	sizeof(struct ctf_globals) == 0x30 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct ctf_globals bss_0043e914;

/* ---------- public code */

void code_0009ebc0(
	void)
{
	return;
}

void code_0009ec70(
	void)
{
	return;
}

void code_0009eca0(
	void)
{
	return;
}

void code_0009ecc0(
	void)
{
	return;
}

void code_0009ecd0(
	void)
{
	return;
}

void code_0009ece0(
	void)
{
	return;
}

void code_0009ecf0(
	void)
{
	return;
}

void code_0009ed00(
	void)
{
	return;
}

void code_0009ef00(
	void)
{
	return;
}

void code_0009ef10(
	void)
{
	return;
}

void code_0009f2c0(
	void)
{
	return;
}

void code_0009f2d0(
	void)
{
	return;
}

void code_0009ec80(
	long player_index)
{
	player_get(player_index);

	return;
}

void code_0009ecb0(
	void)
{
	game_engine_play_multiplayer_sound(
		_multiplayer_sound_capture_the_flag);

	return;
}

boolean code_0009ee80(
	long unit_index,
	long weapon_index)
{
	boolean allow_pick_up = TRUE;
	long player_index = player_index_from_unit_index(unit_index);

	if (player_index != NONE && weapon_index != NONE)
	{
		struct player_datum *player = player_get(player_index);
		struct weapon_datum *weapon = weapon_try_and_get(weapon_index);

		if (weapon &&
			weapon_is_flag(weapon_index) &&
			!TEST_FLAG(weapon->weapon.flags, _ctf_weapon_handled_bit) &&
			weapon->object.owner_team_index == player->team_index)
		{
			allow_pick_up = FALSE;
		}
	}

	return allow_pick_up;
}

wchar_t *code_0009f3d0(
	long player_index,
	wchar_t *buffer)
{
	struct player_datum *player = player_get(player_index);

	usprintf(
		buffer,
		L"%d",
		player->statistics.multiplayer_statistics.ctf_statistics.flag_scores);

	return buffer;
}

wchar_t *code_0009f470(
	long team_index,
	wchar_t *buffer)
{
	usprintf(
		buffer,
		L"%d",
		bss_0043e914.scores[team_index]);

	return buffer;
}

/* ---------- private code */
