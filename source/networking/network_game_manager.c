/*
NETWORK_GAME_MANAGER.C

symbols in this file:
0011A170 0040:
	_network_game_invalidate_player (0000)
0011A1B0 0090:
	_network_game_add_machine (0000)
0011A240 0090:
	_network_game_update_machine (0000)
0011A2D0 0070:
	_xbox_set_machine_name (0000)
0011A340 0090:
	_network_game_generate_local_machine_name (0000)
0011A3D0 0040:
	_network_game_end_and_load_ui (0000)
0011A410 00b0:
	_network_game_reset_for_next_round (0000)
0011A4C0 0010:
	_code_0011a4c0 (0000)
0011A4D0 0030:
	_network_player_is_valid (0000)
0011A500 00b0:
	_network_game_invalidate_machine (0000)
0011A5B0 01a0:
	_network_game_add_player (0000)
0011A750 0140:
	_code_0011a750 (0000)
0011A890 0080:
	_network_game_spawn_player (0000)
0011A910 0090:
	_network_game_player_is_valid (0000)
0011A9A0 00a0:
	_network_game_invalidate (0000)
0011AA40 00a0:
	_network_game_update_player (0000)
0011AAE0 00c0:
	_network_game_remove_player (0000)
0011ABA0 01b0:
	_network_game_create_game_objects (0000)
0011AD50 00e0:
	_network_game_remove_machine (0000)
00283DA4 0031:
	??_C@_0DB@DPKLAPFE@c?3?2halo?2SOURCE?2networking?2networ@ (0000)
00283DD8 0035:
	??_C@_0DF@FMJAGNKK@game?5?$CG?$CG?5machine?5?$CG?$CG?5network_machi@ (0000)
00283E10 0043:
	??_C@_0ED@LDCLPLHI@?8?$CFs?8?5is?5not?5a?5valid?5machine?5name@ (0000)
00283E54 0016:
	??_C@_0BG@OIMNPPEI@XSetNickname?$CI?$CJ?5failed?$AA@ (0000)
00283E6C 002d:
	??_C@_0CN@JCIPKPMA@XSetNickname?$CI?$CJ?5failed?5to?5set?5sys@ (0000)
00283E9C 001c:
	??_C@_0BM@GOCBNICP@system?5nickname?5set?5to?5?8?$CFs?8?$AA@ (0000)
00283EB8 0036:
	??_C@_0DG@KPIAMNFF@game?5?$CG?$CG?5?$CImachine_index?$DMMAXIMUM_N@ (0000)
00283EF0 0039:
	??_C@_0DJ@PKEDFKIN@game?5is?5already?5at?5maximum?5playe@ (0000)
00283F2C 000f:
	??_C@_0P@KHIDKAOH@game?5?$CG?$CG?5player?$AA@ (0000)
00283F40 004b:
	??_C@_0EL@LDBNCIG@multiple?5players?5on?5the?5same?5mac@ (0000)
00283F8C 000f:
	??_C@_0P@NHMFPDJJ@player?5?$CG?$CG?5game?$AA@ (0000)
00283F9C 002c:
	??_C@_0CM@KIIMJJBI@tried?5to?5update?5a?5player?5with?5in@ (0000)
00283FC8 002c:
	??_C@_0CM@NFHLBLAN@tried?5to?5remove?5a?5player?5with?5in@ (0000)
00283FF4 0017:
	??_C@_0BH@GFBBGLI@?$CB?$CCbad?5game?5connection?$CC?$AA@ (0000)
0028400C 0024:
	??_C@_0CE@GKPEJODL@failed?5to?5remove?5a?5machine?8s?5pla@ (0000)
00284030 0010:
	??_C@_0BA@OPIINEDG@game?5?$CG?$CG?5machine?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "main/main.h"
#include "network_game_globals.h"
#include "network_game_manager.h"
#include "text/unicode.h"

/* ---------- constants */

/* ---------- macros */

#define network_machine_is_valid(machine) \
	((machine) && (machine)->machine_index >= 0 && (machine)->machine_index < 4)

/* ---------- structures */

struct network_machine
{
	byte __unknown0[0x40];
	char machine_index;
	byte __padding41[3];
};

struct network_game
{
	wchar_t name[16];
	byte map[0x84];
	byte __unknownA4[0x69];
	byte game_mode;
	byte maximum_player_count;
	byte __padding10F;
	short difficulty;
	short machine_count;
	struct network_machine machines[4];
	short player_count;
	struct network_player players[16];
	byte __unknown426[0xA];
	boolean load_ui;
	byte __padding431[3];
};

/* ---------- prototypes */

void main_load_ui_scenario(
	boolean load_ui);
long __stdcall XFindFirstNicknameW(
	long this_title_only,
	wchar_t *nickname,
	unsigned long size);
long __stdcall XFindClose(
	long handle);
long __stdcall XSetNicknameW(
	wchar_t const *nickname,
	long preserve_case);
wchar_t const *network_game_get_random_player_name(
	void);
boolean network_game_player_is_local(
	struct network_player const *player);
long player_new(
	long machine_index,
	long unit_index,
	short controller_index,
	struct network_player const *player);
void network_game_invalidate_machine(
	struct network_game *game,
	short machine_index);

/* ---------- globals */

/* ---------- public code */

boolean network_game_update_machine(
	struct network_game *game,
	struct network_machine *machine)
{
	long machine_index;
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x81,
		game && machine && network_machine_is_valid(machine));

	for (machine_index = 0; machine_index < 4; machine_index++)
	{
		if (game->machines[machine_index].machine_index == machine->machine_index)
		{
			csmemcpy(&game->machines[machine_index], machine, sizeof(*machine));
			result = TRUE;
			break;
		}
	}

	return result;
}

void xbox_set_machine_name(
	char const *machine_name)
{
	wchar_t wide_machine_name[32];

	if (machine_name && machine_name[0])
	{
		if (ascii_to_wide(machine_name, wide_machine_name, sizeof(wide_machine_name)))
		{
			wide_machine_name[31] = 0;
			if (!XSetNicknameW(wide_machine_name, TRUE))
				error(2, "XSetNickname() failed");
		}
		else
		{
			error(2, "'%s' is not a valid machine name (max. name length= %d characters)", wide_machine_name, 31);
		}
	}

	return;
}

void network_game_generate_local_machine_name(
	wchar_t *machine_name)
{
	char ascii_machine_name[32];
	long find_handle = XFindFirstNicknameW(FALSE, machine_name, 32);

	if (find_handle == NONE)
	{
		ustrncpy(machine_name, network_game_get_random_player_name(), 32);
		machine_name[31] = 0;
		if (XSetNicknameW(machine_name, TRUE))
			error(2, "system nickname set to '%s'", wide_to_ascii(machine_name, ascii_machine_name, 32));
		else
			error(2, "XSetNickname() failed to set system nickname");
	}
	else
	{
		XFindClose(find_handle);
	}

	machine_name[31] = 0;
	return;
}

void network_game_invalidate_player(
	struct network_player *player)
{
	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x58,
		player);

	player->machine_index = NONE;
	player->controller_index = NONE;
	player->team_index = NONE;
	player->player_list_index = NONE;
	player->name[0] = 0;

	return;
}

void network_game_end_and_load_ui(
	struct network_game *game)
{
	if (game->load_ui)
		main_load_ui_scenario(TRUE);

	csmemset(&game->load_ui, 0, sizeof(long));

	return;
}

void code_0011a4c0(
	void)
{
	return;
}

boolean network_player_is_valid(
	struct network_player *player)
{
	if (player &&
		player->controller_index >= 0 &&
		player->controller_index < MAXIMUM_LOCAL_PLAYERS &&
		player->machine_index >= 0 &&
		player->machine_index < MAXIMUM_LOCAL_PLAYERS)
	{
		return TRUE;
	}

	return FALSE;
}

boolean network_game_spawn_player(
	struct network_player *player)
{
	long player_index;
	short controller_index;

	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x1BC,
		network_player_is_valid(player));

	controller_index = network_game_player_is_local(player) ? player->controller_index : NONE;
	player_index = player_new(player->machine_index, NONE, controller_index, player);
	if (player_index != NONE)
	{
		player->player_list_index = (char)player_index;
		return TRUE;
	}

	return FALSE;
}

boolean network_game_player_is_valid(
	struct network_player *player,
	struct network_game *game)
{
	long player_index;
	struct network_player *current_player;
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x247,
		player && game);

	if (network_player_is_valid(player))
	{
		current_player = &game->players[0];
		for (player_index = 0; player_index < 16; player_index++, current_player++)
		{
			if (current_player->machine_index == player->machine_index &&
				current_player->controller_index == player->controller_index)
			{
				result = TRUE;
				break;
			}
		}
	}

	return result;
}

void network_game_reset_for_next_round(
	struct network_game *game,
	boolean load_ui)
{
	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x22D,
		game);

	if (load_ui && game->load_ui)
	{
		main_load_ui_scenario(TRUE);
		csmemset(&game->load_ui, 0, sizeof(long));
		if (global_network_game_server_get())
			game_connection_set(2);
		else if (global_network_game_client_get())
			game_connection_set(1);
	}
	else
	{
		csmemset(&game->load_ui, 0, sizeof(long));
	}

	game_time_end();
	return;
}

void network_game_invalidate(
	struct network_game *game)
{
	long machine_index;

	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x23,
		game);

	csmemset(game, 0, sizeof(*game));
	csmemset(game->map, 0, sizeof(game->map));
	game->machine_count = 0;
	game->player_count = 0;

	for (machine_index = 0; machine_index < 4; machine_index++)
		network_game_invalidate_machine(game, (short)machine_index);

	csmemset(game->players, NONE, sizeof(game->players));
	game->game_mode = 2;
	game->maximum_player_count = 16;
	game->load_ui = FALSE;

	return;
}

boolean network_game_update_player(
	struct network_game *game,
	struct network_player *player)
{
	struct network_player *current_player;
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\networking\\network_game_manager.c",
		0x101,
		game && player);

	if (network_game_player_is_valid(player, game))
	{
		current_player = &game->players[player->player_list_index];
		if (current_player->controller_index == player->controller_index &&
			current_player->machine_index == player->machine_index)
		{
			csmemcpy(current_player, player, sizeof(*current_player));
			result = TRUE;
		}
	}

	if (!result)
		error(2, "tried to update a player with indvalid data");

	return result;
}

/* ---------- private code */
