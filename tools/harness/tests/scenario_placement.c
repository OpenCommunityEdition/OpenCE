/*
SCENARIO_PLACEMENT.C (test)

The real scenario_placement_withheld_by_name (scenario_placement.c): a10
places Sergeant Johnson as it loads, though only its opening cinematic uses
him (it creates him anew, then deletes him); a player who skipped the
cinematic found him standing in a hallway. He is not placed as a local game
of a10 loads; in a network game, where the objects placed as a map loads
must be alike on every machine, he is, as is every other object anywhere.
test_scenario_placement.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define csstrcmp strcmp

enum
{
	_game_connection_local = 0,
	_game_connection_network_client,
	_game_connection_network_server,
	_game_connection_film_playback,
};

#include "under_test.inc"

#define A10 "levels\\a10\\a10"
#define MARINE "characters\\marine_armored\\marine_armored"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a local game of a10: Johnson not placed */
	CASE("a10 local")
	{
		CHECK(scenario_placement_withheld_by_name(_game_connection_local, A10, "johnson", MARINE), "placed");
		return 0;
	}
	/* a network game of a10 (a co-op host or client), a film: placed */
	CASE("a10 network")
	{
		CHECK(!scenario_placement_withheld_by_name(_game_connection_network_server, A10, "johnson", MARINE), "host");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_network_client, A10, "johnson", MARINE), "client");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_film_playback, A10, "johnson", MARINE), "film");
		return 0;
	}
	/* any other object, map or definition: placed */
	CASE("anything else")
	{
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, A10, "keyes", "characters\\captain\\captain"),
			"keyes");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, A10, "hangar_marine_1", MARINE),
			"another marine");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, "levels\\b30\\b30", "johnson", MARINE),
			"another map's johnson");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, A10, "johnson", "characters\\captain\\captain"),
			"another definition");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, A10, NULL, MARINE), "no name");
		CHECK(!scenario_placement_withheld_by_name(_game_connection_local, NULL, "johnson", MARINE), "no map");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
