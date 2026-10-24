/*
LOBBY_COUNTDOWN.C (test)

The real pregame countdown of source/networking/network_server_manager.c:
whether the lobby's START NOW (the "faster" request,
_network_game_server_countdown_event_player_joined) starts the countdown,
for a host alone, a host with split screen players, and machines together;
and that the host's own START NOW starts the game at once, a client
machine's taking five seconds off.
test_lobby_countdown.py takes the constants, enums and timer structures
(config.inc) and the code (under_test.inc).
*/

#include "harness.h"

#define NETWORK_SERVER_MANAGER_FILE "server"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "config.inc"

struct network_player
{
	boolean valid;
	signed char machine_index;
	signed char team_index;
};

struct network_game_server_client_machine
{
	void *connection;
	boolean joined;
};

struct network_game_server
{
	short state;
	struct
	{
		short player_count;
		short minimum_players;
		struct
		{
			struct
			{
				boolean teams;
			} universal_variant;
		} variant;
		struct network_player players[MAXIMUM_NETWORK_PLAYER_COUNT];
	} game;
	struct network_game_server_client_machine client_machines[MAXIMUM_NETWORK_MACHINE_COUNT];
	struct network_game_server_countdown_state countdown_state;
};

static struct network_game_server server;
static int connection;

static unsigned long system_milliseconds(void) { return 1000; }
static boolean network_game_should_accept_remote_connections(void) { return TRUE; }
static boolean splitscreen;
static boolean network_game_is_splitscreen_local(void) { return splitscreen; }
static boolean network_player_is_valid(struct network_player *player) { return player->valid; }
static boolean network_game_server_client_machine_is_joined_to_game(
	struct network_game_server *s,
	struct network_game_server_client_machine *machine)
{
	return machine->joined;
}
static void network_game_server_countdown_started(struct network_game_server *s) { }
static boolean network_game_server_client_machine_is_local(
	struct network_game_server *s,
	struct network_game_server_client_machine *machine)
{
	return machine == &s->client_machines[0];
}

#include "under_test.inc"

static void join_machine(
	short machine_index)
{
	server.client_machines[machine_index].connection = &connection;
	server.client_machines[machine_index].joined = TRUE;
}

/* machine 0 is the host's; no players yet */
static void lobby(
	short machines)
{
	short index;

	memset(&server, 0, sizeof(server));
	splitscreen = FALSE;
	server.state = _network_game_server_state_pregame;
	server.game.minimum_players = 2;
	for (index = 0; index < machines; index++)
		join_machine(index);
}

static void add_player(
	short machine_index,
	short team_index)
{
	struct network_player *player = &server.game.players[server.game.player_count++];

	player->valid = TRUE;
	player->machine_index = machine_index;
	player->team_index = team_index;
}

/* the faster event (a client machine's START NOW) */
static void start_now(void)
{
	network_game_server_update_countdown(&server, _network_game_server_countdown_event_player_joined);
}

/* a machine's START NOW, as the host takes it (the message handler); the
next tick's countdown update lets the countdown change again */
static void machine_start_now(
	short machine_index)
{
	network_game_server_update_countdown(&server,
		network_game_server_client_machine_faster_event(&server, &server.client_machines[machine_index]));
	server.countdown_state.adjusted_time_this_tick = FALSE;
}

static long time_remaining(void)
{
	return countdown_timer_get_time_remaining(&server.countdown_state.timer);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("split-screen-start-now")
	{
		lobby(1);
		add_player(0, 0);
		add_player(0, 1);
		start_now();
		CHECK(server.countdown_state.active, "one machine's two players never start the countdown");
		CHECK(server.countdown_state.timer.time_remaining == NETWORK_GAME_COUNTDOWN_TIME,
			"%ld ms to go, not a system link game's countdown", server.countdown_state.timer.time_remaining);
		return 0;
	}
	CASE("alone-start-now")
	{
		lobby(1);
		add_player(0, 0);
		start_now();
		CHECK(server.countdown_state.active, "the host alone no longer starts the countdown");
		return 0;
	}
	CASE("empty-lobby-waits")
	{
		lobby(1);
		CHECK(!server_ok_to_countdown(&server), "an empty lobby may count down");
		start_now();
		CHECK(!server.countdown_state.active, "an empty lobby started the countdown");
		return 0;
	}
	CASE("two-machines-start-now")
	{
		lobby(2);
		add_player(0, 0);
		add_player(1, 1);
		start_now();
		CHECK(server.countdown_state.active, "two machines no longer start the countdown");
		return 0;
	}
	CASE("two-machines-team-game-needs-both-teams")
	{
		lobby(2);
		server.game.variant.universal_variant.teams = TRUE;
		add_player(0, 0);
		add_player(1, 0);
		CHECK(!server_ok_to_countdown(&server), "two machines' players of one team may count down");
		start_now();
		CHECK(!server.countdown_state.active, "two machines' players of one team started the countdown");
		return 0;
	}
	/* server_ok_to_countdown is checked each tick, and a failure stops the countdown */
	CASE("playerless-machine-keeps-countdown")
	{
		lobby(1);
		add_player(0, 0);
		start_now();
		CHECK(server.countdown_state.active, "the host alone no longer starts the countdown");
		join_machine(1);
		CHECK(server_ok_to_countdown(&server), "a machine joining without a player stops the countdown");
		return 0;
	}
	/* intended: one machine skips the team rule, as a lone host does */
	CASE("split-screen-team-game-one-team")
	{
		lobby(1);
		server.game.variant.universal_variant.teams = TRUE;
		add_player(0, 0);
		add_player(0, 0);
		start_now();
		CHECK(server.countdown_state.active, "one machine's players of one team never start the countdown");
		return 0;
	}
	CASE("host-start-now-starts")
	{
		lobby(2);
		add_player(0, 0);
		add_player(1, 1);
		machine_start_now(0);
		CHECK(server.countdown_state.active, "the host's START NOW did not start the countdown");
		CHECK(time_remaining() == 0, "%ld ms to go after the host's START NOW", time_remaining());
		return 0;
	}
	CASE("host-start-now-ends-countdown")
	{
		lobby(2);
		add_player(0, 0);
		add_player(1, 1);
		machine_start_now(1);
		machine_start_now(0);
		CHECK(time_remaining() == 0, "%ld ms to go after the host's START NOW", time_remaining());
		return 0;
	}
	CASE("split-screen-host-start-now-starts")
	{
		lobby(1);
		splitscreen = TRUE;
		add_player(0, 0);
		add_player(0, 1);
		machine_start_now(0);
		CHECK(server.countdown_state.active, "the split screen host's START NOW did not start the countdown");
		CHECK(time_remaining() == 0, "%ld ms to go after the split screen host's START NOW", time_remaining());
		return 0;
	}
	CASE("client-start-now-five-seconds")
	{
		lobby(2);
		add_player(0, 0);
		add_player(1, 1);
		machine_start_now(1);
		CHECK(time_remaining() == NETWORK_GAME_COUNTDOWN_TIME,
			"%ld ms to go after a client's first START NOW", time_remaining());
		machine_start_now(1);
		CHECK(time_remaining() == NETWORK_GAME_COUNTDOWN_TIME - NETWORK_GAME_COUNTDOWN_ADJUSTMENT,
			"%ld ms to go after a client's second START NOW", time_remaining());
		return 0;
	}
	CASE("host-start-now-team-game-needs-both-teams")
	{
		lobby(2);
		server.game.variant.universal_variant.teams = TRUE;
		add_player(0, 0);
		add_player(1, 0);
		machine_start_now(0);
		CHECK(!server.countdown_state.active, "the host's START NOW started a game one team short");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
