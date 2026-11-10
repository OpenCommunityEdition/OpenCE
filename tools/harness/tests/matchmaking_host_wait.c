/*
MATCHMAKING_HOST_WAIT.C (test)

A matchmade host whose own player never counts (matchmaking.c).
*/

#include "harness.h"
#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("never_counted")
	{
		CHECK(!matchmaking_host_gave_up(MATCHMAKING_PLAYER_SECONDS + 1.0f, 0), "within the host wait: waits");
		CHECK(matchmaking_host_gave_up(MATCHMAKING_PLAYER_SECONDS + P2P_MM_HOST_WAIT_SECONDS, 0), "past it: given up");
	}
	CASE("counted")
	{
		CHECK(!matchmaking_host_gave_up(MATCHMAKING_PLAYER_SECONDS + P2P_MM_HOST_WAIT_SECONDS + 60.0f, 1),
			"its player counts: kept");
	}
	return 0;
}
