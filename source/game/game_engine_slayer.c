/*
GAME_ENGINE_SLAYER.C

symbols in this file:
000A3CF0 0050:
	_target_is_eligible (0000)
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
	_slayer_engine_adjust_score (0000)
000A3FC0 0010:
	_slayer_engine_prespawn_player_update (0000)
000A3FD0 0040:
	_slayer_get_score (0000)
000A4010 0010:
	_slayer_test_flag (0000)
000A4020 0030:
	_slayer_get_score_string (0000)
000A4050 0060:
	_slayer_get_score_header_string (0000)
000A40B0 0030:
	_slayer_get_team_score_string (0000)
000A40E0 0190:
	_find_next_target (0000)
000A4270 0090:
	_slayer_engine_player_killed_player (0000)
000A4300 0280:
	_code_000a4300 (0000)
000A4580 0190:
	_slayer_player_update (0000)
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

#include "game/game_engine_place.h"
#include "game/game_engine_slayer.h"
#include "game/players.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"
#include "text/text_group.h"
#include "text/unicode.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_multiplayer_sound_slayer = 0x15,
	_multiplayer_sound_team_slayer = 0x23,
	_slayer_message_new_target = 0x1E,
	_game_engine_message_show_score = 0x16,
	_string_score = 0x9A,
	_string_n_team_n = 0xB3,
	_string_new_target_name = 0xB4,
	_string_name_kills_score_n_team_score_of_max = 0xB5,
	_string_name_kills_score_of_max = 0xB6,
	_game_engine_test_flag_rasterize_score = 1,
	_slayer_maximum_players = 16,
};

/* ---------- macros */

#define slayer_variant_assault unknown4C.byte0
#define slayer_variant_reset_on_capture unknown4C.byte1
#define slayer_variant_flag_must_reset unknown4C.byte2
#define slayer_variant_score_to_win unknown40

/* ---------- structures */

struct slayer_globals
{
	long team_score[16];
	long individual_score[16];
};

typedef char verify_slayer_globals_size[
	sizeof(struct slayer_globals) == 0x80 ? 1 : -1];

/* ---------- prototypes */

static boolean target_is_eligible(
	long player_index,
	long current_target_player_index,
	long candidate_player_index);

static void slayer_engine_adjust_score(
	long player_index,
	long score_delta);

static void find_next_target(
	long player_index);

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

	player->multiplayer_special = NONE;

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

boolean slayer_test_flag(
	long flag)
{
	boolean result = FALSE;

	switch (flag)
	{
	case _game_engine_test_flag_rasterize_score:
		result = TRUE;
		break;
	}

	return result;
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

wchar_t *slayer_get_score_header_string(
	wchar_t *buffer)
{
	long string_list_index = tag_loaded(
		UNICODE_STRING_LIST_TAG,
		"ui\\multiplayer_game_text");

	if (string_list_index != NONE)
	{
		wchar_t *string = unicode_string_list_get_string(
			string_list_index,
			_string_score);
		wchar_t *result = buffer;

		ustrcpy(result, string);

		return result;
	}
	else
	{
		wchar_t *result = buffer;

		ustrcpy(result, L"");

		return result;
	}
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

static boolean target_is_eligible(
	long player_index,
	long current_target_player_index,
	long candidate_player_index)
{
	boolean eligible = FALSE;
	struct player_datum *player = player_get(player_index);
	struct player_datum *candidate_player = player_get(candidate_player_index);

	if (candidate_player_index != player_index &&
		candidate_player_index != current_target_player_index &&
		candidate_player->team_index != player->team_index &&
		candidate_player->unit_index != NONE)
	{
		eligible = TRUE;
	}

	return eligible;
}

void update_speed_for_score(
	long dead_player_index,
	long killing_player_index)
{
	struct player_datum *killing_player = player_get(killing_player_index);
	struct player_datum *dead_player = player_get(dead_player_index);

	if (!game_engine_get_variant()->slayer_variant_reset_on_capture)
	{
		killing_player->speed_multiplier -= 0.02f;
		if (killing_player->speed_multiplier >= 1.0f)
		{
			killing_player->speed_multiplier -= 0.15000001f;
			killing_player->speed_multiplier = MAX(
				killing_player->speed_multiplier,
				1.0f);
		}

		killing_player->speed_multiplier = MAX(
			killing_player->speed_multiplier,
			0.89999998f);
	}

	if (!game_engine_get_variant()->slayer_variant_assault)
	{
		dead_player->speed_multiplier += 0.1f;
		if (dead_player->speed_multiplier <= 1.0f)
		{
			dead_player->speed_multiplier += 0.1f;
			dead_player->speed_multiplier = MIN(
				dead_player->speed_multiplier,
				1.0f);
		}

		dead_player->speed_multiplier = MIN(
			dead_player->speed_multiplier,
			1.5f);
	}

	return;
}

static void slayer_engine_adjust_score(
	long player_index,
	long score_delta)
{
	struct player_datum *player = player_get(player_index);

	slayer_globals.team_score[player->team_index] += score_delta;
	slayer_globals.individual_score[
		DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)] += score_delta;

	return;
}

static void find_next_target(
	long player_index)
{
	struct player_datum *player = player_get(player_index);
	long current_target_player_index = player->multiplayer_special;
	long next_target = NONE;
	long eligible_player_count = 0;
	struct data_iterator iterator;

	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		if (target_is_eligible(
			player_index,
			current_target_player_index,
			iterator.datum_index))
		{
			eligible_player_count++;
		}
	}

	if (eligible_player_count > 0)
	{
		long chosen_player_offset = seed_random_range(
			get_global_random_seed_address(),
			0,
			(short)eligible_player_count);

		data_iterator_new(&iterator, player_data);
		while (data_iterator_next(&iterator))
		{
			if (target_is_eligible(
				player_index,
				current_target_player_index,
				iterator.datum_index))
			{
				if (chosen_player_offset == 0)
				{
					next_target = iterator.datum_index;
					break;
				}
				chosen_player_offset--;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\game\\game_engine_slayer.c",
			0xC3,
			next_target != NONE);
	}

	player->multiplayer_special = next_target;
	if (next_target != NONE)
	{
		game_show_score_extended(
			player_index,
			_slayer_message_new_target,
			next_target);
	}

	return;
}

void slayer_engine_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct player_datum *dead_player = player_get(dead_player_index);

	if (!dead_player->quit_out_of_game && killing_player_index != NONE)
	{
		struct player_datum *killing_player = player_get(killing_player_index);

		if (!friendly_fire)
		{
			update_speed_for_score(dead_player_index, killing_player_index);

			if (game_engine_get_variant()->slayer_variant_flag_must_reset)
			{
				if (killing_player->multiplayer_special != dead_player_index)
					return;
				find_next_target(killing_player_index);
			}

			slayer_engine_adjust_score(killing_player_index, 1);
		}
		else
		{
			slayer_engine_adjust_score(killing_player_index, -1);
		}
	}

	return;
}

boolean slayer_engine_display_score(
	long player_index,
	long message,
	long message_data,
	wchar_t *buffer,
	long buffer_size)
{
	wchar_t score[128];
	struct player_datum *player;

	player_get(player_index);
	player = NULL;

	if (message == _slayer_message_new_target)
	{
		if (game_engine_get_variant()->has_teams)
		{
			long string_list_index = tag_loaded(
				UNICODE_STRING_LIST_TAG,
				"ui\\multiplayer_game_text");
			wchar_t *format = string_list_index != NONE
				? unicode_string_list_get_string(
					string_list_index,
					_string_n_team_n)
				: L"";

			usnprintf(
				score,
				NUMBEROF(score),
				format,
				slayer_get_score(player_index, _get_score_individual),
				slayer_get_score(player_index, _get_score_team));
		}
		else
		{
			usnprintf(
				score,
				NUMBEROF(score),
				L"%d",
				slayer_get_score(player_index, _get_score_individual));
		}

		player = player_get(message_data);
	}

	switch (message)
	{
	case _game_engine_message_show_score:
		if (game_engine_get_variant()->has_teams)
		{
			struct game_engine_place place = game_engine_get_place(
				player_index,
				_get_score_team);
			wchar_t *place_name = get_place_name(place);
			long string_list_index = tag_loaded(
				UNICODE_STRING_LIST_TAG,
				"ui\\multiplayer_game_text");

			wchar_t *format = string_list_index != NONE
				? unicode_string_list_get_string(
					string_list_index,
					_string_name_kills_score_n_team_score_of_max)
				: L"";

			usnprintf(
				buffer,
				buffer_size,
				format,
				place_name,
				slayer_get_score(player_index, _get_score_individual),
				slayer_get_score(player_index, _get_score_team),
				game_engine_get_variant()->slayer_variant_score_to_win);
		}
		else
		{
			struct game_engine_place place = game_engine_get_place(
				player_index,
				_get_score_team);
			wchar_t *place_name = get_place_name(place);
			long string_list_index = tag_loaded(
				UNICODE_STRING_LIST_TAG,
				"ui\\multiplayer_game_text");

			wchar_t *format = string_list_index != NONE
				? unicode_string_list_get_string(
					string_list_index,
					_string_name_kills_score_of_max)
				: L"";

			usnprintf(
				buffer,
				buffer_size,
				format,
				place_name,
				slayer_get_score(player_index, _get_score_team),
				game_engine_get_variant()->slayer_variant_score_to_win);
		}

		return TRUE;

	case _slayer_message_new_target:
	{
		long string_list_index = tag_loaded(
			UNICODE_STRING_LIST_TAG,
			"ui\\multiplayer_game_text");
		wchar_t *format = string_list_index != NONE
			? unicode_string_list_get_string(
				string_list_index,
				_string_new_target_name)
			: L"";

		usnprintf(
			buffer,
			buffer_size,
			format,
			player->name);

		return TRUE;
	}

	default:
		return FALSE;
	}
}

void slayer_player_update(
	long index)
{
	struct player_datum *player = player_get(index);

	if (game_engine_get_variant()->slayer_variant_reset_on_capture &&
		player->speed_multiplier > 1.0f)
	{
		player->speed_multiplier -= 0.00011111111f;
		player->speed_multiplier = MAX(
			player->speed_multiplier,
			1.0f);
	}

	if (game_engine_get_variant()->slayer_variant_assault &&
		player->speed_multiplier < 1.0f)
	{
		player->speed_multiplier += 0.000011111111f;
		player->speed_multiplier = MIN(
			player->speed_multiplier,
			1.0f);
	}

	if (game_engine_get_variant()->slayer_variant_flag_must_reset)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\game\\game_engine_slayer.c",
			0x201,
			VALID_INDEX((short)index, _slayer_maximum_players),
			"(index >= 0) && (index < MULTIPLAYER_MAXIMUM_PLAYERS)");

		game_engine_clear_goal_position(index);

		if (player->multiplayer_special != NONE)
		{
			struct player_datum *target_player = player_get(
				player->multiplayer_special);

			if (target_player->unit_index != NONE)
			{
				struct unit_datum *target_unit = unit_get(
					target_player->unit_index);

				game_engine_set_goal_position(
					index,
					&target_unit->object.bounding_sphere_center,
					0.0f,
					"target_blue",
					index,
					NONE,
					NONE);
			}
		}

		if (player->unit_index != NONE &&
			player->multiplayer_special == NONE)
		{
			find_next_target(index);
		}

		if (player->multiplayer_special != NONE &&
			game_engine_man_out(player->multiplayer_special))
		{
			find_next_target(index);
		}
	}

	if (slayer_get_score(index, _get_score_team) >=
		game_engine_get_variant()->slayer_variant_score_to_win)
	{
		game_engine_end_game();
	}

	return;
}
