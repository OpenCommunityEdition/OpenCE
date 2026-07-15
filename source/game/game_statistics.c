/*
GAME_STATISTICS.C

symbols in this file:
000A49B0 0010:
	_game_statistics_start (0000)
000A49C0 0090:
	_game_statistics_stop (0000)
000A4A50 0010:
	_game_statistics_record_damage (0000)
000A4A60 02a0:
	_game_statistics_record_kill (0000)
0043EE00 0001:
	_game_statistics_active (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "real_math.h"
#include "game/players.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

boolean game_statistics_active = FALSE;

/* ---------- public code */

void game_statistics_start(
	void)
{
	game_statistics_active = 1;

	return;
}

void
game_statistics_stop(
	short winning_team_index)
{
	struct data_iterator iterator;
	struct player_datum *player;

	data_iterator_new(&iterator, player_data);
	player = (struct player_datum *)data_iterator_next(&iterator);

	while (player)
	{
		player->statistics.seconds_online = game_time_get() / 30;
		player->statistics.games_played = 1;

		if (player->team_index == winning_team_index)
			player->statistics.games_won++;

		player = (struct player_datum *)data_iterator_next(&iterator);
	}

	game_statistics_active = FALSE;

	return;
}

void game_statistics_record_damage(
	long object_index,
	real damage,
	long owner_player_index,
	long owner_object_index,
	short owner_team_index)
{
	return;
}

/* ---------- private code */
