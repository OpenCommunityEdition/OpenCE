#include "harness.h"
#include "teams.inc"

/* teams a script has made allies of the players (none at a map's start) */
static boolean allied_with_players[NUMBER_OF_SOLO_CAMPAIGN_TEAMS];

static boolean game_team_is_enemy(short team_index, short other_team_index)
{
	if (team_index == other_team_index)
		return FALSE;
	return !(team_index == _game_team_player && allied_with_players[other_team_index]);
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc == 2 ? argv[1] : "";

	CASE("start")
	{
		CHECK(!coop_enemies_team_against_players(_game_team_human), "humans counted as enemies");
		CHECK(!coop_enemies_team_against_players(_game_team_player), "players counted as enemies");
		CHECK(coop_enemies_team_against_players(_game_team_covenant), "covenant not counted");
		CHECK(coop_enemies_team_against_players(_game_team_flood), "flood not counted");
	}
	else CASE("allied")
	{
		allied_with_players[_game_team_sentinel] = TRUE;
		CHECK(!coop_enemies_team_against_players(_game_team_sentinel), "allied sentinels counted as enemies");
		CHECK(coop_enemies_team_against_players(_game_team_covenant), "covenant not counted");
	}
	else return 2;
	return 0;
}
