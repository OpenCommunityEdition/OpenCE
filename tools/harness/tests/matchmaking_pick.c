/*
MATCHMAKING_PICK.C (test)

Step 1's pick of a public game (p2p_lobby.c).
*/

#include "harness.h"
#include "enums.inc"
#include "under_test.inc"

static struct p2p_listing games[16];
static int count;

static void add(const char *name, int engine, int players, int most, int ping, int dedicated)
{
	struct p2p_listing *game = &games[count++];

	memset(game, 0, sizeof(*game));
	snprintf(game->name, sizeof(game->name), "%s", name);
	snprintf(game->invite, sizeof(game->invite), "halo://join/%s", name);
	game->engine_type = (unsigned char)engine;
	game->player_count = (unsigned char)players;
	game->maximum_player_count = (unsigned char)most;
	game->has_teams = 1;
	game->open = 1;
	game->ping = (short)ping;
	game->dedicated = game->master = (unsigned char)dedicated;
	game->network_version = HALO_PORT_NETWORK_VERSION;
}

static const char *picked(void)
{
	struct p2p_lobby_wanted wanted = { 1u << 2, 4, 1 };
	int index = p2p_lobby_pick(games, count, &wanted);

	return index < 0 ? "none" : games[index].name;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("e2_most_players")
	{
		count = 0;
		add("two", 2, 2, 4, 120, 0);
		add("three", 2, 3, 4, 180, 0);
		CHECK(!strcmp(picked(), "three"), "E2: most players first: %s", picked());
		count = 0;
		add("slow", 2, 3, 4, 150, 0);
		add("quick", 2, 3, 4, 60, 0);
		CHECK(!strcmp(picked(), "quick"), "then the lowest ping: %s", picked());
	}
	CASE("e3_passed_over")
	{
		count = 0;
		add("full", 2, 4, 4, 40, 0);
		add("far", 2, 3, 4, 240, 0);
		add("at_limit", 2, 3, 4, 200, 0);
		add("unpinged", 2, 3, 4, -1, 0);
		add("password", 2, 3, 4, 40, 0);
		games[count - 1].locked = 1;
		add("failed", 2, 3, 4, 40, 0);
		games[count - 1].failed = 1;
		add("empty", 2, 0, 4, 40, 0);
		add("ctf", 1, 3, 4, 40, 0);
		add("eight", 2, 3, 8, 40, 0);
		add("ffa", 2, 3, 4, 40, 0);
		games[count - 1].has_teams = 0;
		add("old", 2, 3, 4, 40, 0);
		games[count - 1].network_version = HALO_PORT_NETWORK_VERSION - 1;
		CHECK(!strcmp(picked(), "none"), "E3: every one passed over: %s", picked());
	}
	CASE("e51_e56_dedicated")
	{
		count = 0;
		/* (pings given, so that only the dedicated rule passes them over) */
		add("dedicated16", 2, 3, 16, 30, 1);
		add("dedicated4", 2, 3, 4, 30, 1);
		CHECK(!strcmp(picked(), "none"), "E51: small sizes take no dedicated server");
		count = 0;
		add("brokers_only", 2, 3, 4, 50, 0);
		CHECK(!strcmp(picked(), "brokers_only"), "E56: a server the brokers alone list is a player's game");
	}
	return 0;
}
