/*
LATENCY_METER.C (test)

The latency meter's real arithmetic (display.latency_meter): the colours
and bars of a round trip (port/linux/game/latency_meter.c), and from
port/linux/game/network_distributed.c the round trip timed by when this
machine sent the tick it comes back naming, its smoothing, and when the host
(to a client) or a client (to the host) has a connection problem.
test_latency_meter.py takes the constants (config.inc) and the code
(under_test.inc).
*/

#include "harness.h"

typedef struct
{
	real alpha, red, green, blue;
} real_argb_color;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "config.inc"

enum
{
	_game_connection_local = 0,
	_game_connection_network_client,
	_game_connection_network_server,
};

/* the fake world: the clock, the connection, the game over or not */
static unsigned long now_milliseconds;
static short connection = _game_connection_network_client;
static boolean in_play = TRUE;

#define system_milliseconds() (now_milliseconds)
#define game_connection() (connection)
#define game_engine_in_play() (in_play)

/* network_distributed.c's state the code reads */
static struct
{
	long time;
	unsigned long milliseconds;
} distributed_tick_sent_times[TICK_SENT_TIMES];
static long distributed_host_time = NONE;
static unsigned long distributed_host_time_received;
static struct
{
	unsigned long heard;
} distributed_round_trips[HALO_PORT_MAXIMUM_NETWORK_MACHINES];

#include "under_test.inc"

/* this machine sends tick `time` at `milliseconds`, as network_distributed_tick notes it */
static void sent(
	long time,
	unsigned long milliseconds)
{
	distributed_tick_sent_times[time & (TICK_SENT_TIMES - 1)].time = time;
	distributed_tick_sent_times[time & (TICK_SENT_TIMES - 1)].milliseconds = milliseconds;
}

static boolean near(
	real a,
	real b)
{
	return a - b < 0.001f && b - a < 0.001f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	long index;

	for (index = 0; index < TICK_SENT_TIMES; index++)
		distributed_tick_sent_times[index].time = NONE;

	/* green with four bars below 80 ms, yellow with three below 150, red
	with two below 250 and one from it */
	CASE("colors")
	{
		static const struct
		{
			long milliseconds;
			short bars;
			char color;
		} expected[] =
		{
			{ 0, 4, 'g' }, { 79, 4, 'g' }, { 80, 3, 'y' }, { 149, 3, 'y' }, { 150, 2, 'r' }, { 249, 2, 'r' },
			{ 250, 1, 'r' }, { 9999, 1, 'r' },
		};
		short case_index;

		for (case_index = 0; case_index < (short)(sizeof(expected) / sizeof(expected[0])); case_index++)
		{
			real_argb_color color;
			short bars = latency_meter_color(expected[case_index].milliseconds, &color);
			char kind = color.green == 1.0f && color.red < 1.0f ? 'g' :
				color.red == 1.0f && color.green > 0.5f ? 'y' :
				color.red == 1.0f && color.green < 0.5f ? 'r' : '?';

			CHECK(bars == expected[case_index].bars, "%ld ms: %d bars, not %d", expected[case_index].milliseconds,
				bars, expected[case_index].bars);
			CHECK(kind == expected[case_index].color, "%ld ms: colour %c, not %c", expected[case_index].milliseconds,
				kind, expected[case_index].color);
			CHECK(color.alpha == 1.0f, "%ld ms: alpha %g", expected[case_index].milliseconds, color.alpha);
		}
		return 0;
	}
	/* a tick's round trip is the time since it was sent; one no longer kept
	(overwritten by a tick TICK_SENT_TIMES later) or never sent is none */
	CASE("since-tick")
	{
		sent(1000, 50000);
		now_milliseconds = 50117;
		CHECK(distributed_milliseconds_since_tick(1000) == 117, "%ld ms, not 117",
			distributed_milliseconds_since_tick(1000));
		CHECK(distributed_milliseconds_since_tick(999) == NONE, "an unsent tick timed");
		CHECK(distributed_milliseconds_since_tick(NONE) == NONE, "NONE timed");
		sent(1000 + TICK_SENT_TIMES, 52000);
		now_milliseconds = 52010;
		CHECK(distributed_milliseconds_since_tick(1000) == NONE, "an overwritten tick timed");
		CHECK(distributed_milliseconds_since_tick(1000 + TICK_SENT_TIMES) == 10, "%ld ms, not 10",
			distributed_milliseconds_since_tick(1000 + TICK_SENT_TIMES));
		return 0;
	}
	/* at most MAXIMUM_PING_MILLISECONDS, and right across the clock's wrap */
	CASE("since-tick-bounds")
	{
		sent(7, 0xFFFFFFF0UL);
		now_milliseconds = 0x00000010UL;
		CHECK(distributed_milliseconds_since_tick(7) == 32, "%ld ms across the wrap, not 32",
			distributed_milliseconds_since_tick(7));
		now_milliseconds = 0xFFFFFFF0UL + MAXIMUM_PING_MILLISECONDS + 5000;
		CHECK(distributed_milliseconds_since_tick(7) == MAXIMUM_PING_MILLISECONDS, "%ld ms, not at most %d",
			distributed_milliseconds_since_tick(7), MAXIMUM_PING_MILLISECONDS);
		return 0;
	}
	/* the first sample as it is, then an eighth of each difference */
	CASE("smoothing")
	{
		boolean valid = FALSE;
		real milliseconds = 0.0f;

		distributed_smooth_ping(&valid, &milliseconds, 100);
		CHECK(valid && near(milliseconds, 100.0f), "first sample %g, not 100", milliseconds);
		distributed_smooth_ping(&valid, &milliseconds, 180);
		CHECK(near(milliseconds, 110.0f), "%g after 180, not 110", milliseconds);
		distributed_smooth_ping(&valid, &milliseconds, 30);
		CHECK(near(milliseconds, 100.0f), "%g after 30, not 100", milliseconds);
		return 0;
	}
	/* a client: a problem once the host has been silent more than two
	seconds; none before the host's first message, after the game, or on
	the host */
	CASE("connection-problem")
	{
		connection = _game_connection_network_client;
		now_milliseconds = 10000;
		CHECK(!distributed_connection_problem(), "a problem before the host's first message");
		distributed_host_time = 300;
		distributed_host_time_received = 10000;
		now_milliseconds = 10000 + CONNECTION_PROBLEM_MILLISECONDS;
		CHECK(!distributed_connection_problem(), "a problem at %d ms", CONNECTION_PROBLEM_MILLISECONDS);
		now_milliseconds++;
		CHECK(distributed_connection_problem(), "no problem after %d ms", CONNECTION_PROBLEM_MILLISECONDS + 1);
		in_play = FALSE;
		CHECK(!distributed_connection_problem(), "a problem after the game (the host sends nothing of it)");
		in_play = TRUE;
		connection = _game_connection_network_server;
		CHECK(!distributed_connection_problem(), "a problem on the host");
		connection = _game_connection_network_client;
		distributed_host_time_received = now_milliseconds - 30;
		CHECK(!distributed_connection_problem(), "a problem once heard again");
		CHECK(CONNECTION_PROBLEM_MILLISECONDS == 2000, "the wait is %d ms, not two seconds",
			CONNECTION_PROBLEM_MILLISECONDS);
		return 0;
	}
	/* the host: a client silent more than two seconds, once heard from */
	CASE("machine-silent")
	{
		connection = _game_connection_network_server;
		now_milliseconds = 5000;
		CHECK(!distributed_machine_silent(3), "a machine never heard from silent");
		distributed_round_trips[3].heard = 5000;
		now_milliseconds = 5000 + CONNECTION_PROBLEM_MILLISECONDS;
		CHECK(!distributed_machine_silent(3), "silent at %d ms", CONNECTION_PROBLEM_MILLISECONDS);
		now_milliseconds++;
		CHECK(distributed_machine_silent(3), "not silent after %d ms", CONNECTION_PROBLEM_MILLISECONDS + 1);
		CHECK(!distributed_machine_silent(NONE) && !distributed_machine_silent(HALO_PORT_MAXIMUM_NETWORK_MACHINES),
			"a machine out of range silent");
		in_play = FALSE;
		CHECK(!distributed_machine_silent(3), "silent after the game");
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
