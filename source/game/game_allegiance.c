/*
GAME_ALLEGIANCE.C

symbols in this file:
00095FB0 0030:
	_game_allegiance_initialize (0000)
00095FE0 0010:
	_game_allegiance_dispose (0000)
00095FF0 00a0:
	_game_allegiance_initialize_for_new_map (0000)
00096090 0010:
	_game_allegiance_dispose_from_old_map (0000)
000960A0 0060:
	_game_team_is_enemy (0000)
00096100 0050:
	_game_team_is_ally (0000)
00096150 0060:
	_game_team_ally_status_changed (0000)
000961B0 0080:
	_game_allegiance_get_incidents (0000)
00096230 0070:
	_game_allegiance_provoke (0000)
000962A0 0060:
	_game_allegiance_notify_change (0000)
00096300 01a0:
	_code_00096300 (0000)
000964A0 0080:
	_game_allegiance_update (0000)
00096520 00e0:
	_game_allegiance_create (0000)
00096600 00b0:
	_game_allegiance_remove (0000)
000966B0 00d0:
	_game_allegiance_incident (0000)
0025B0D0 0018:
	??_C@_0BI@FALCEHFN@game?5allegiance?5globals?$AA@ (0000)
0025B0E8 0018:
	??_C@_0BI@MMPFGAEA@game_allegiance_globals?$AA@ (0000)
0025B100 0026:
	??_C@_0CG@MBPCBMLP@c?3?2halo?2SOURCE?2game?2game_allegia@ (0000)
0025B128 0022:
	??_C@_0CC@EJEFDMDC@allegiance?9?$DOcurrent_incidents?5?$DO?5@ (0000)
0025B14C 003d:
	??_C@_0DN@FFLAEGAA@game_allegiance_create?3?5too?5many@ (0000)
002DE3B8 0028:
	_global_game_team_names (0000)
0043E490 0004:
	_bss_0043e490 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game/game_allegiance.h"
#include "saved games/game_state.h"

/* ---------- constants */

enum
{
	NUMBER_OF_GAME_TEAMS = 10
};

/* ---------- macros */

/* ---------- structures */

struct game_allegiance_record
{
	short team1_index;
	short team2_index;
	short incident_threshold;
	short incident_decay_time;
	boolean team1_suspicious;
	boolean team2_suspicious;
	boolean currently_broken;
	boolean status_changed;
	boolean requires_communication;
	byte reserved0D;
	short current_incidents;
	short current_incident_decay_time;
};

struct game_allegiance_globals
{
	short allegiance_count;
	struct game_allegiance_record allegiances[8];
	byte reserved92[2];
	unsigned long ally_bitvector[4];
	unsigned long friendly_bitvector[4];
};

typedef char game_allegiance_record_size_assert[
	sizeof(struct game_allegiance_record) == 0x12 ? 1 : -1];
typedef char game_allegiance_globals_size_assert[
	sizeof(struct game_allegiance_globals) == 0xB4 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct game_allegiance_globals *bss_0043e490;

#define game_allegiance_globals bss_0043e490

/* ---------- public code */

void game_allegiance_initialize(
	void)
{
	game_allegiance_globals = game_state_malloc(
		"game allegiance globals",
		NULL,
		sizeof(*game_allegiance_globals));
	csmemset(
		game_allegiance_globals,
		0,
		sizeof(*game_allegiance_globals));

	return;
}

void game_allegiance_dispose(
	void)
{
	return;
}

void game_allegiance_dispose_from_old_map(
	void)
{
	return;
}

boolean game_team_is_enemy(
	short our_team_index,
	short other_team_index)
{
	boolean result = TRUE;

	if (VALID_INDEX(our_team_index, NUMBER_OF_GAME_TEAMS) &&
		VALID_INDEX(other_team_index, NUMBER_OF_GAME_TEAMS))
	{
		result = !BIT_VECTOR_TEST_FLAG(
			game_allegiance_globals->friendly_bitvector,
			NUMBER_OF_GAME_TEAMS * our_team_index + other_team_index);
	}

	return result;
}

boolean game_team_is_ally(
	short our_team_index,
	short other_team_index)
{
	boolean result = FALSE;

	if (VALID_INDEX(our_team_index, NUMBER_OF_GAME_TEAMS) &&
		VALID_INDEX(other_team_index, NUMBER_OF_GAME_TEAMS))
	{
		result = BIT_VECTOR_TEST_FLAG(
			game_allegiance_globals->ally_bitvector,
			NUMBER_OF_GAME_TEAMS * our_team_index + other_team_index);
	}

	return result;
}

short game_allegiance_get_incidents(
	short our_team_index,
	short other_team_index,
	short *incident_threshold)
{
	struct game_allegiance_record *allegiance;
	short allegiance_count;
	short allegiance_index;
	short current_incidents = 0;
	short threshold = NONE;

	allegiance = game_allegiance_globals->allegiances;
	allegiance_count = game_allegiance_globals->allegiance_count;
	for (allegiance_index = 0;
		allegiance_index < allegiance_count;
		allegiance_index++, allegiance++)
	{
		if ((allegiance->team1_index == our_team_index &&
				allegiance->team2_index == other_team_index) ||
			(allegiance->team2_index == our_team_index &&
				allegiance->team1_index == other_team_index))
		{
			current_incidents = allegiance->current_incidents;
			threshold = allegiance->incident_threshold;
			break;
		}
	}

	if (incident_threshold)
		*incident_threshold = threshold;

	return current_incidents;
}

void game_allegiance_provoke(
	short aggressor_team_index,
	short victim_team_index)
{
	struct game_allegiance_record *allegiance;
	short allegiance_count;
	short allegiance_index;

	allegiance = game_allegiance_globals->allegiances;
	allegiance_count = game_allegiance_globals->allegiance_count;
	for (allegiance_index = 0;
		allegiance_index < allegiance_count;
		allegiance_index++, allegiance++)
	{
		if ((allegiance->team1_index == aggressor_team_index &&
				allegiance->team2_index == victim_team_index &&
				allegiance->team2_suspicious) ||
			(allegiance->team2_index == aggressor_team_index &&
				allegiance->team1_index == victim_team_index &&
				allegiance->team1_suspicious))
		{
			if (allegiance->current_incidents > 0 &&
				allegiance->incident_decay_time != NONE)
			{
				allegiance->current_incident_decay_time =
					allegiance->incident_decay_time;
			}

			return;
		}
	}

	return;
}

void game_allegiance_notify_change(
	short team1_index,
	short team2_index)
{
	struct game_allegiance_record *allegiance;
	short allegiance_count;
	short allegiance_index;

	allegiance = game_allegiance_globals->allegiances;
	allegiance_count = game_allegiance_globals->allegiance_count;
	for (allegiance_index = 0;
		allegiance_index < allegiance_count;
		allegiance_index++, allegiance++)
	{
		if ((allegiance->team1_index == team1_index &&
				allegiance->team2_index == team2_index) ||
			(allegiance->team2_index == team1_index &&
				allegiance->team1_index == team2_index))
		{
			allegiance->status_changed = FALSE;
			return;
		}
	}

	return;
}

/* ---------- private code */
