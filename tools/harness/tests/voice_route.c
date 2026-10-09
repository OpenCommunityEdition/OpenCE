/*
VOICE_ROUTE.C (test)

The real rules of port/linux/game/network_voice.c that the host sends each
voice by: who hears whom in the lobby and in each of network.voice_mode's
modes, and the largest packet a quality allows. test_voice_route.py takes
the modes and routes (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "config.inc"
#include "under_test.inc"

/* a listener's route to a speaker in a game: teammates or not, near (both
alive, within the range) or far, or one of them dead */
static short in_game(short mode, boolean teammates, real distance, boolean both_alive)
{
	return voice_route(mode, FALSE, TRUE, teammates, both_alive, distance, 15.0f);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short mode;

	/* the lobby: everyone, or no one, whatever the game's mode */
	CASE("lobby")
	{
		for (mode = 0; mode < NUMBER_OF_VOICE_MODES; mode++)
		{
			CHECK(voice_route(mode, TRUE, TRUE, FALSE, FALSE, 1000.0f, 15.0f) == _voice_route_global,
				"mode %d: the lobby's voice not heard by everyone", mode);
			CHECK(voice_route(mode, TRUE, FALSE, TRUE, TRUE, 0.0f, 15.0f) == _voice_route_none,
				"mode %d: the lobby heard with its voice off", mode);
		}
		return 0;
	}
	CASE("off")
	{
		CHECK(in_game(_voice_mode_off, TRUE, 0.0f, TRUE) == _voice_route_none, "off: a teammate heard");
		CHECK(in_game(_voice_mode_off, FALSE, 0.0f, TRUE) == _voice_route_none, "off: an enemy heard");
		return 0;
	}
	CASE("team-proximity")
	{
		CHECK(in_game(_voice_mode_team_proximity, TRUE, 10.0f, TRUE) == _voice_route_proximity, "a near teammate");
		CHECK(in_game(_voice_mode_team_proximity, TRUE, 20.0f, TRUE) == _voice_route_none, "a far teammate heard");
		CHECK(in_game(_voice_mode_team_proximity, FALSE, 1.0f, TRUE) == _voice_route_none, "a near enemy heard");
		CHECK(in_game(_voice_mode_team_proximity, TRUE, 1.0f, FALSE) == _voice_route_none, "the dead heard near");
		return 0;
	}
	CASE("team-enemy-proximity")
	{
		CHECK(in_game(_voice_mode_team_enemy_proximity, TRUE, 10.0f, TRUE) == _voice_route_proximity,
			"a near teammate");
		CHECK(in_game(_voice_mode_team_enemy_proximity, FALSE, 10.0f, TRUE) == _voice_route_proximity,
			"a near enemy");
		CHECK(in_game(_voice_mode_team_enemy_proximity, TRUE, 20.0f, TRUE) == _voice_route_none,
			"a far teammate heard");
		CHECK(in_game(_voice_mode_team_enemy_proximity, FALSE, 20.0f, TRUE) == _voice_route_none,
			"a far enemy heard");
		return 0;
	}
	CASE("team-global")
	{
		CHECK(in_game(_voice_mode_team_global, TRUE, 1000.0f, FALSE) == _voice_route_global,
			"a teammate anywhere, alive or not");
		CHECK(in_game(_voice_mode_team_global, FALSE, 1.0f, TRUE) == _voice_route_none, "a near enemy heard");
		return 0;
	}
	CASE("team-global-enemy-proximity")
	{
		CHECK(in_game(_voice_mode_team_global_enemy_proximity, TRUE, 1000.0f, FALSE) == _voice_route_global,
			"a teammate anywhere");
		CHECK(in_game(_voice_mode_team_global_enemy_proximity, FALSE, 10.0f, TRUE) == _voice_route_proximity,
			"a near enemy");
		CHECK(in_game(_voice_mode_team_global_enemy_proximity, FALSE, 20.0f, TRUE) == _voice_route_none,
			"a far enemy heard");
		CHECK(in_game(_voice_mode_team_global_enemy_proximity, FALSE, 1.0f, FALSE) == _voice_route_none,
			"a dead enemy heard");
		return 0;
	}
	/* a quality's packets fit, and no more than twice their share */
	CASE("packet-limit")
	{
		short kbps;

		for (kbps = 8; kbps <= 64; kbps++)
		{
			long share = (long)kbps * 1000 / 8 / 50;

			CHECK(voice_packet_limit(kbps) >= share, "%d kbps: its own packets refused", kbps);
			CHECK(voice_packet_limit(kbps) <= 2 * share + 16, "%d kbps: %ld bytes taken", kbps, voice_packet_limit(kbps));
			CHECK(voice_packet_limit(kbps) <= VOICE_MAXIMUM_PACKET, "%d kbps: more than a packet", kbps);
		}
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
