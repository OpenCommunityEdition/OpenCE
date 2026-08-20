/*
PLAYER_UI.C

symbols in this file:
000CFC30 0020:
	_player_ui_dispose (0000)
000CFC50 0020:
	_player_ui_reset_single_player_local_player_controllers (0000)
000CFC70 0080:
	_player_ui_set_single_player_local_player_controller (0000)
000CFCF0 0050:
	_player_ui_get_single_player_local_player_controller (0000)
000CFD40 0030:
	_player_ui_get_single_player_local_player_from_controller (0000)
000CFD70 0050:
	_player_ui_local_player_joined_multiplayer_game (0000)
000CFDC0 0050:
	_player_ui_local_player_wants_to_play_multiplayer (0000)
000CFE10 0050:
	_player_ui_clear_multiplayer_autojoin_for_local_player (0000)
000CFE60 0030:
	_player_ui_autojoin_players_to_next_multiplayer_game (0000)
000CFE90 0020:
	_player_ui_clear_multiplayer_variant (0000)
000CFEB0 0060:
	_player_ui_get_active_player_profile (0000)
000CFF10 0030:
	_player_ui_get_active_player_profile_index (0000)
000CFF40 0050:
	_player_ui_get_last_single_player_level_played (0000)
000CFF90 0050:
	_player_ui_set_game_variant (0000)
000CFFE0 0050:
	_player_ui_game_variant_specified (0000)
000D0030 0050:
	_player_ui_rumble_disabled (0000)
000D0080 00a0:
	_player_ui_autolevel_enabled (0000)
000D0120 0040:
	_player_ui_get_path_to_local_player_profile_directory (0000)
000D0160 0060:
	_player_ui_remember_player1_profile (0000)
000D01C0 0040:
	_player_ui_get_player1_last_used_profile_index (0000)
000D0200 00b0:
	_player_ui_fast_setup_network_server (0000)
000D02B0 0050:
	_player_ui_edit_profile_is_default_profile (0000)
000D0300 0070:
	_player_ui_edit_profile_name_is_dirty (0000)
000D0370 0060:
	_player_ui_prompt_user_to_rename_edit_profile (0000)
000D03D0 0020:
	_player_ui_get_edit_player_profile (0000)
000D03F0 0020:
	_player_ui_get_edit_playlist_profile (0000)
000D0410 00f0:
	_player_ui_edit_profile_is_dirty (0000)
000D0500 0030:
	_player_ui_activate_all_solo_levels (0000)
000D0530 0050:
	_code_000d0530 (0000)
000D0580 0010:
	_player0_look_pitch_is_inverted (0000)
000D0590 0020:
	_player0_joystick_set_is_normal (0000)
000D05B0 0040:
	_code_000d05b0 (0000)
000D05F0 0210:
	_code_000d05f0 (0000)
000D0800 0010:
	_code_000d0800 (0000)
000D0810 0070:
	_code_000d0810 (0000)
000D0880 00a0:
	_player_ui_initialize (0000)
000D0920 00a0:
	_player_ui_clear_multiplayer_joins (0000)
000D09C0 0070:
	_player_ui_set_active_player_profile (0000)
000D0A30 00b0:
	_player_ui_begin_editing_profile (0000)
000D0AE0 01b0:
	_player_ui_save_profile (0000)
000D0C90 0010:
	_player_ui_end_editing_profile (0000)
000D0CA0 0070:
	_player0_look_invert_pitch (0000)
00270BDC 0019:
	??_C@_0BJ@DFIFBFHJ@invalid?5controller?5index?$AA@ (0000)
00270BF8 001b:
	??_C@_0BL@FPFMMDCA@invalid?5local?5player?5index?$AA@ (0000)
00270C14 0025:
	??_C@_0CF@DLLNKDKO@c?3?2halo?2SOURCE?2interface?2player_@ (0000)
00270C40 0050:
	??_C@_0FA@COKNKHJP@?$CIlocal_player_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIloca@ (0000)
00270C90 0065:
	??_C@_0GF@IIJJCBPM@?$CIlocal_player_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIloca@ (0000)
00270CF8 0008:
	??_C@_07IFGLIMJI@variant?$AA@ (0000)
00270D00 002f:
	??_C@_0CP@JOJHLPFO@player?51?5has?5no?5active?5player?5pr@ (0000)
00270D30 006a:
	??_C@_0GK@DKOJLEOE@failed?5to?5load?5network?5pregame?5s@ (0000)
00270D9C 002d:
	??_C@_0CN@JHOFCMHM@failed?5to?5initiate?5a?5multiplayer@ (0000)
00270DD0 0056:
	??_C@_0FG@FFFNJFPJ@ui?2shell?2main_menu?2multiplayer_t@ (0000)
00270E28 002a:
	??_C@_0CK@FMHFAECE@unknown?5saved?5game?5file?5type?5bei@ (0000)
00270E54 0028:
	??_C@_0CI@CHDBALJG@not?5currently?5editing?5a?5saved?5ga@ (0000)
00270E7C 0022:
	??_C@_0CC@KDPBKPKM@unknown?5profile?5type?5being?5edite@ (0000)
00270EA0 0008:
	??_C@_07LNMDFKEJ@profile?$AA@ (0000)
00270EA8 0035:
	??_C@_0DF@EJKBCKFH@failed?5to?5retrieve?5player?5profil@ (0000)
00270EE0 0037:
	??_C@_0DH@HABNMGLM@failed?5to?5retrieve?5playlist?5prof@ (0000)
00270F18 001f:
	??_C@_0BP@IGBDNJOH@invalid?5profile?5index?5?$CI?$CD?$CF08lX?$CJ?$AA@ (0000)
00270F38 0032:
	??_C@_0DC@OODOEIMJ@?$CD?$CD?$CD?5WARNING?3?5saving?5over?5a?5defau@ (0000)
00270F70 0049:
	??_C@_0EJ@JACFDNDO@cannot?5save?5over?5default?5profile@ (0000)
00270FBC 0027:
	??_C@_0CH@HGHAKCCE@failed?5to?5save?5renamed?5profile?5t@ (0000)
00270FE8 0046:
	??_C@_0EG@JJOABCL@?$CD?$CD?$CD?5WARNING?3?5saving?5player?5profi@ (0000)
00271030 0036:
	??_C@_0DG@JHHBBKEF@failed?5to?5save?5profile?5because?5w@ (0000)
00271068 001e:
	??_C@_0BO@KMODNFHM@ui?2shell?2strings?2temp_strings?$AA@ (0000)
002FD5A4 0004:
	_data_002fd5a4 (0000)
00453D00 0330:
	_player_ui_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "text/unicode.h"
#include "input/input.h"
#include "networking/network_game_globals.h"
#include "interface/virtual_keyboard.h"
#include "player_ui.h"

/* ---------- constants */

enum
{
	MAXIMUM_NUMBER_OF_LOCAL_PLAYERS = 4,
	PLAYER_UI_DISPOSE_SIZE = 0x230,
	SAVED_GAME_FILE_TYPE_PLAYER_PROFILE = 0,
	SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE = 1,
	NUMBER_OF_SAVED_GAME_FILE_TYPES = 2
};

/* ---------- macros */

/* ---------- structures */

struct player_ui_local_player
{
	byte unknown0[0x1c];
	byte solo_levels[10];
	short last_single_player_level;
	byte unknown28;
	byte joystick_set;
	byte unknown2A;
	boolean look_pitch_inverted;
	boolean rumble_disabled;
	byte unknown2D;
	boolean autolevel;
	byte unknown2F;
	long active_profile_index;
	boolean autojoin_next_multiplayer_game;
	byte unknown35[3];
};

struct player_profile
{
	wchar_t name[12];
	byte unknown18[0x18];
};

struct playlist_profile
{
	wchar_t name[12];
	byte unknown18[0x50];
};

union player_ui_edit_profile_data
{
	struct player_profile player;
	struct playlist_profile playlist;
};

struct player_ui_edit_profile
{
	union player_ui_edit_profile_data current;
	union player_ui_edit_profile_data original;
	byte unknownD0[4];
};

struct player_ui_globals
{
	struct player_ui_local_player local_players[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	boolean multiplayer_autojoin[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	short single_player_controller[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	byte multiplayer_variant[0x68];
	boolean multiplayer_variant_specified;
	byte unknown155[3];
	long edit_profile_index;
	struct player_ui_edit_profile edit_profile;
	char player1_last_used_profile_directory[0x100];
};

typedef char player_profile_size_assert[
	sizeof(struct player_profile) == 0x30 ? 1 : -1];
typedef char playlist_profile_size_assert[
	sizeof(struct playlist_profile) == 0x68 ? 1 : -1];
typedef char player_ui_edit_profile_size_assert[
	sizeof(struct player_ui_edit_profile) == 0xD4 ? 1 : -1];

/* ---------- prototypes */

void game_connection_set(
	short connection);
void game_engine_dispose(
	void);
void game_set_game_variant(
	struct game_variant *variant);
word saved_game_file_get_type(
	long profile_index);
boolean player_profile_get_enclosing_directory_path(
	long profile,
	char *full_path);
void player_profile_save(
	long profile,
	void *data);
boolean saved_game_file_retrieve_player1_last_used_profile_directory(
	char *directory);
long saved_game_file_find_profile_index_for_directory_path(
	char *directory,
	long file_type);
void saved_game_file_remember_player1_last_used_profile_directory(
	char *directory);
boolean player_profile_get(
	long profile_index,
	struct player_profile *profile);
boolean playlist_profile_get(
	long profile_index,
	struct playlist_profile *profile);

/* ---------- globals */

long data_002fd5a4 = NONE;
struct player_ui_globals player_ui_globals = { 0 };

/* ---------- public code */

void player_ui_dispose(
	void)
{
	csmemset(&player_ui_globals, 0, PLAYER_UI_DISPOSE_SIZE);
	return;
}

void player_ui_reset_single_player_local_player_controllers(
	void)
{
	csmemset(
		player_ui_globals.single_player_controller,
		NONE,
		sizeof(player_ui_globals.single_player_controller));
	return;
}

short player_ui_get_single_player_local_player_from_controller(
	short controller_index)
{
	short local_player_index;
	short result;

	result = NONE;
	for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
	{
		if (player_ui_globals.single_player_controller[local_player_index] == controller_index)
		{
			result = local_player_index;
			break;
		}
	}
	return result;
}

void player_ui_autojoin_players_to_next_multiplayer_game(
	void)
{
	player_ui_globals.local_players[0].autojoin_next_multiplayer_game = player_ui_globals.multiplayer_autojoin[0];
	player_ui_globals.local_players[1].autojoin_next_multiplayer_game = player_ui_globals.multiplayer_autojoin[1];
	player_ui_globals.local_players[2].autojoin_next_multiplayer_game = player_ui_globals.multiplayer_autojoin[2];
	player_ui_globals.local_players[3].autojoin_next_multiplayer_game = player_ui_globals.multiplayer_autojoin[3];
	return;
}

void player_ui_clear_multiplayer_variant(
	void)
{
	player_ui_globals.multiplayer_variant_specified = FALSE;
	game_connection_set(0);
	game_engine_dispose();
	game_set_game_variant(NULL);
	return;
}

long player_ui_get_active_player_profile_index(
	short local_player_index)
{
	long result;

	if (local_player_index >= 0 && local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)
		result = player_ui_globals.local_players[local_player_index].active_profile_index;
	else
		result = NONE;
	return result;
}

struct player_profile *player_ui_get_edit_player_profile(
	void)
{
	struct player_profile *result;

	if (saved_game_file_get_type(player_ui_globals.edit_profile_index) == SAVED_GAME_FILE_TYPE_PLAYER_PROFILE)
		result = &player_ui_globals.edit_profile.current.player;
	else
		result = NULL;
	return result;
}

struct playlist_profile *player_ui_get_edit_playlist_profile(
	void)
{
	struct playlist_profile *result;

	if (saved_game_file_get_type(player_ui_globals.edit_profile_index) == SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE)
		result = &player_ui_globals.edit_profile.current.playlist;
	else
		result = NULL;
	return result;
}

boolean player0_look_pitch_is_inverted(
	void)
{
	return player_ui_globals.local_players[0].look_pitch_inverted;
}

boolean player0_joystick_set_is_normal(
	void)
{
	return player_ui_globals.local_players[0].joystick_set == 0 ||
		player_ui_globals.local_players[0].joystick_set == 1;
}

void player_ui_end_editing_profile(
	void)
{
	player_ui_globals.edit_profile_index = NONE;
	return;
}

boolean player_ui_local_player_wants_to_play_multiplayer(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 167, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	return player_ui_globals.local_players[local_player_index].autojoin_next_multiplayer_game;
}

void player_ui_clear_multiplayer_autojoin_for_local_player(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 175, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	player_ui_globals.local_players[local_player_index].autojoin_next_multiplayer_game = FALSE;
	return;
}

short player_ui_get_last_single_player_level_played(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 265, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	return player_ui_globals.local_players[local_player_index].last_single_player_level;
}

short player_ui_get_single_player_local_player_controller(
	short local_player_index)
{
	match_vassert("c:\\halo\\SOURCE\\interface\\player_ui.c", 132,
		(local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS),
		"invalid local player index");

	return player_ui_globals.single_player_controller[local_player_index];
}

void player_ui_local_player_joined_multiplayer_game(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 157, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	player_ui_globals.local_players[local_player_index].autojoin_next_multiplayer_game = TRUE;
	player_ui_globals.multiplayer_autojoin[local_player_index] = TRUE;
	return;
}

boolean player_ui_rumble_disabled(
	short local_player_index)
{
	if (local_player_index == NONE)
		return FALSE;

	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 305, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	return player_ui_globals.local_players[local_player_index].rumble_disabled;
}

boolean player_ui_get_path_to_local_player_profile_directory(
	short local_player_index,
	char *path)
{
	if (local_player_index >= 0 && local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)
		return player_profile_get_enclosing_directory_path(
			player_ui_globals.local_players[local_player_index].active_profile_index,
			path);
	return FALSE;
}

void player_ui_get_active_player_profile(
	short local_player_index,
	void *profile)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 238, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS) && (profile != NULL));

	csmemcpy(profile, &player_ui_globals.local_players[local_player_index], 0x30);
	return;
}

void player_ui_activate_all_solo_levels(
	void)
{
	long level_index;

	level_index = 0;
	do
	{
		player_ui_globals.local_players[0].solo_levels[level_index] |= 0xf;
	}
	while (++level_index < 10);

	if (player_ui_globals.local_players[0].active_profile_index != NONE)
		player_profile_save(
			player_ui_globals.local_players[0].active_profile_index,
			&player_ui_globals.local_players[0]);
	return;
}

void player_ui_set_game_variant(
	struct game_variant *variant)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 273, variant);

	csmemcpy(player_ui_globals.multiplayer_variant, variant, 0x68);
	player_ui_globals.multiplayer_variant_specified = TRUE;
	return;
}

boolean player_ui_game_variant_specified(
	struct game_variant *variant)
{
	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 284, variant);

	if (player_ui_globals.multiplayer_variant_specified)
		csmemcpy(variant, player_ui_globals.multiplayer_variant, 0x68);
	return player_ui_globals.multiplayer_variant_specified;
}

void player_ui_set_single_player_local_player_controller(
	short local_player_index,
	short controller_index)
{
	match_vassert("c:\\halo\\SOURCE\\interface\\player_ui.c", 119,
		(local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS),
		"invalid local player index");
	match_vassert("c:\\halo\\SOURCE\\interface\\player_ui.c", 121,
		(controller_index>=0) && (controller_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS),
		"invalid controller index");

	player_ui_globals.single_player_controller[local_player_index] = controller_index;
	return;
}

long player_ui_get_player1_last_used_profile_index(
	void)
{
	if (!player_ui_globals.player1_last_used_profile_directory[0] &&
		saved_game_file_retrieve_player1_last_used_profile_directory(
			player_ui_globals.player1_last_used_profile_directory))
	{
		data_002fd5a4 = saved_game_file_find_profile_index_for_directory_path(
			player_ui_globals.player1_last_used_profile_directory, 0);
	}
	return data_002fd5a4;
}

boolean player_ui_edit_profile_is_default_profile(
	void)
{
	boolean result;

	result = FALSE;
	if (player_ui_globals.edit_profile_index != NONE)
	{
		long type = saved_game_file_get_type(player_ui_globals.edit_profile_index);

		if (type>=0 && type<=SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE)
			result = ((unsigned long)player_ui_globals.edit_profile_index>>30) & 1;
		else
			error(_error_silent, "unknown saved game file type being edited");
	}

	return result;
}

void player_ui_remember_player1_profile(
	boolean save)
{
	if (data_002fd5a4 != player_ui_globals.local_players[0].active_profile_index)
	{
		if (player_ui_globals.local_players[0].active_profile_index==NONE ||
			!player_profile_get_enclosing_directory_path(
				player_ui_globals.local_players[0].active_profile_index,
				player_ui_globals.player1_last_used_profile_directory))
		{
			error(_error_silent, "player 1 has no active player profile assigned");
		}

		data_002fd5a4 = player_ui_globals.local_players[0].active_profile_index;
	}

	if (save && player_ui_globals.player1_last_used_profile_directory[0])
		saved_game_file_remember_player1_last_used_profile_directory(
			player_ui_globals.player1_last_used_profile_directory);

	return;
}

void player_ui_begin_editing_profile(
	long profile_index)
{
	long type;

	player_ui_globals.edit_profile_index = NONE;
	type = saved_game_file_get_type(profile_index);

	switch (type)
	{
		case SAVED_GAME_FILE_TYPE_PLAYER_PROFILE:
			if (player_profile_get(
				profile_index,
				&player_ui_globals.edit_profile.original.player))
			{
				csmemcpy(
					&player_ui_globals.edit_profile.current.player,
					&player_ui_globals.edit_profile.original.player,
					sizeof(struct player_profile));
			}
			else
			{
				error(_error_silent, "failed to retrieve player profile #%08lX for editing", profile_index);
				return;
			}
			break;

		case SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE:
			if (playlist_profile_get(
				profile_index,
				&player_ui_globals.edit_profile.original.playlist))
			{
				csmemcpy(
					&player_ui_globals.edit_profile.current.playlist,
					&player_ui_globals.edit_profile.original.playlist,
					sizeof(struct playlist_profile));
			}
			else
			{
				error(_error_silent, "failed to retrieve playlist profile #%08lX for editing", profile_index);
				return;
			}
			break;

		default:
			error(_error_silent, "invalid profile index (#%08lX)", profile_index);
			return;
	}

	player_ui_globals.edit_profile_index = profile_index;
	return;
}

boolean player_ui_autolevel_enabled(
	short controller_index)
{
	short local_player_index;

	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 320, (controller_index>=0) && (controller_index<MAXIMUM_GAMEPADS));

	if (network_game_is_active())
		local_player_index = controller_index;
	else
		local_player_index = player_ui_get_single_player_local_player_from_controller(controller_index);

	if (local_player_index == NONE)
		return FALSE;

	match_assert("c:\\halo\\SOURCE\\interface\\player_ui.c", 340, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	return player_ui_globals.local_players[local_player_index].autolevel;
}

boolean player_ui_edit_profile_name_is_dirty(
	void)
{
	boolean result;

	result = FALSE;
	if (player_ui_globals.edit_profile_index != NONE)
	{
		long type = saved_game_file_get_type(player_ui_globals.edit_profile_index);

		switch (type)
		{
			case SAVED_GAME_FILE_TYPE_PLAYER_PROFILE:
				if (ustrncmp(
					player_ui_globals.edit_profile.current.player.name,
					player_ui_globals.edit_profile.original.player.name,
					12)!=0)
				{
					result = TRUE;
				}
				break;

			case SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE:
				if (ustrncmp(
					player_ui_globals.edit_profile.current.playlist.name,
					player_ui_globals.edit_profile.original.playlist.name,
					12)!=0)
				{
					result = TRUE;
				}
				break;

			default:
				error(_error_silent, "unknown saved game file type being edited");
				break;
		}
	}
	else
	{
		error(_error_silent, "not currently editing a saved game file");
	}

	return result;
}

boolean player_ui_prompt_user_to_rename_edit_profile(
	void)
{
	boolean result;

	result = FALSE;
	if (player_ui_globals.edit_profile_index != NONE)
	{
		long type = saved_game_file_get_type(player_ui_globals.edit_profile_index);

		switch (type)
		{
			case SAVED_GAME_FILE_TYPE_PLAYER_PROFILE:
				result = virtual_keyboard_launch(
					player_ui_globals.edit_profile.current.player.name,
					sizeof(player_ui_globals.edit_profile.current.player.name),
					10);
				break;

			case SAVED_GAME_FILE_TYPE_PLAYLIST_PROFILE:
				result = virtual_keyboard_launch(
					player_ui_globals.edit_profile.current.playlist.name,
					sizeof(player_ui_globals.edit_profile.current.playlist.name),
					10);
				break;

			default:
				error(_error_silent, "unknown saved game file type being edited");
				break;
		}
	}
	else
	{
		error(_error_silent, "not currently editing a saved game file");
	}

	return result;
}

/* ---------- private code */

void code_000d0800(
	void)
{
	player_ui_globals.edit_profile_index = NONE;
	return;
}
