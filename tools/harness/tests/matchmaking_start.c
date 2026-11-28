/*
MATCHMAKING_START.C (test)

The countdown of a full small game (matchmaking.c).
*/

#include "harness.h"

struct network_game_server;

static struct network_game_server *server;
static boolean ok, paused;
static long begun;
static short stopped;

boolean network_game_server_port_ok_to_countdown(struct network_game_server *unused)
{
	return ok;
}

boolean network_game_server_port_countdown_paused(struct network_game_server *unused)
{
	return paused;
}

void network_game_server_port_countdown_begin(struct network_game_server *unused, long milliseconds)
{
	begun = milliseconds;
}

void network_game_server_port_countdown_stop(struct network_game_server *unused)
{
	stopped++;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	ok = TRUE;
	CASE("full_starts")
	{
		matchmaking_start_countdown(server, 4, 4);
		CHECK(begun == 4999, "the countdown began with %ld ms", begun);
		CHECK(!stopped, "a full lobby stopped the countdown");
		return 0;
	}
	CASE("leave_stops")
	{
		matchmaking_start_countdown(server, 3, 4);
		CHECK(stopped == 1, "a player leaving stopped %d countdowns", stopped);
		CHECK(!begun, "three of four counted down: %ld ms", begun);
		return 0;
	}
	CASE("not_ready")
	{
		ok = FALSE;
		matchmaking_start_countdown(server, 4, 4);
		CHECK(!begun, "a countdown the server would refuse began: %ld ms", begun);
		ok = TRUE;
		paused = TRUE;
		matchmaking_start_countdown(server, 4, 4);
		CHECK(!begun, "a paused lobby began counting: %ld ms", begun);
		CHECK(!stopped, "a paused lobby was stopped");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
