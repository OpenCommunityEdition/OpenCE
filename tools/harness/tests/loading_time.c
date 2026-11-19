/*
LOADING_TIME.C (test)

The real loading_time.c (port/linux/game): how long a level's loading screen
is held (display.loading_seconds) and the progress it shows meanwhile, no
further than the time held has gone. test_loading_time.py takes the code
and the game's connections from the sources.
*/

#include "harness.h"

#include "config.inc"

static double setting;
static short connection;
static unsigned long now;

double config_real(const char *name)
{
	CHECK(!strcmp(name, "display.loading_seconds"), "the setting read: %s", name);
	return setting;
}

short game_connection(void)
{
	return connection;
}

unsigned long GetTickCount(void)
{
	return now;
}

#include "under_test.inc"

static void begin(double seconds, short which, unsigned long at)
{
	setting = seconds;
	connection = which;
	now = at;
	loading_time_begin();
}

/* at a time after the load began: the progress shown for the load's, and
whether the loading screen is held */
static void expect(unsigned long start, unsigned long gone, real progress, real shown, boolean held)
{
	real difference;

	now = start + gone;
	difference = loading_time_progress(progress) - shown;
	CHECK(difference < 1e-6f && difference > -1e-6f, "after %lu ms, the load at %g: %g shown, not %g", gone,
		progress, loading_time_progress(progress), shown);
	CHECK(!loading_time_holding() == !held, "after %lu ms: %s", gone, held ? "not held" : "held");
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("off")
	{
		begin(0.0, _game_connection_local, 1000);
		expect(1000, 0, 0.25f, 0.25f, FALSE);
		expect(1000, 0, 1.0f, 1.0f, FALSE);
		begin(-3.0, _game_connection_local, 1000);
		expect(1000, 0, 1.0f, 1.0f, FALSE);
		return 0;
	}
	CASE("held")
	{
		begin(5.0, _game_connection_local, 1000);
		expect(1000, 0, 1.0f, 0.0f, TRUE);
		expect(1000, 2500, 1.0f, 0.5f, TRUE);
		/* (a load slower than the time: its own progress) */
		expect(1000, 2500, 0.25f, 0.25f, TRUE);
		expect(1000, 4999, 1.0f, 4999.0f / 5000.0f, TRUE);
		expect(1000, 5000, 1.0f, 1.0f, FALSE);
		expect(1000, 9000, 0.75f, 0.75f, FALSE);
		loading_time_end();
		expect(1000, 0, 0.5f, 0.5f, FALSE);
		return 0;
	}
	CASE("network")
	{
		begin(5.0, _game_connection_network_client, 1000);
		expect(1000, 0, 1.0f, 1.0f, FALSE);
		begin(5.0, _game_connection_network_server, 1000);
		expect(1000, 0, 1.0f, 1.0f, FALSE);
		return 0;
	}
	CASE("longest")
	{
		begin(100.0, _game_connection_local, 1000);
		expect(1000, 15000, 1.0f, 0.5f, TRUE);
		expect(1000, 29999, 1.0f, 29999.0f / 30000.0f, TRUE);
		expect(1000, 30000, 1.0f, 1.0f, FALSE);
		return 0;
	}
	CASE("tick wrap")
	{
		/* (GetTickCount goes back to 0 after 49.7 days) */
		begin(5.0, _game_connection_local, 0xFFFFF000UL);
		expect(0xFFFFF000UL, 100, 1.0f, 0.02f, TRUE);
		expect(0xFFFFF000UL, 2500, 1.0f, 0.5f, TRUE);
		expect(0xFFFFF000UL, 4999, 1.0f, 4999.0f / 5000.0f, TRUE);
		expect(0xFFFFF000UL, 5000, 1.0f, 1.0f, FALSE);
		return 0;
	}
	fprintf(stderr, "no case named %s\n", case_name);
	return 2;
}
