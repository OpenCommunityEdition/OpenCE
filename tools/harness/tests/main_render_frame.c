/*
MAIN_RENDER_FRAME.C (test)

The real decision of main.c's main loop whether a frame is drawn: as the
original, after the frames whose ticks ran (or paused, or slowed); with
Smooth Motion (display.interpolation) every frame too, but only once the map
has ticked, since before its first tick the world has none of the camera and
fade its scripts give it then (a cinematic's start showed the sky); the
menus' map always; a multiplayer game from its third tick.
test_main_render_frame.py takes the lines from the sources.
*/

#include "harness.h"

static struct { boolean main_menu_scenario_loaded; long halt_time_scale; } main_globals;
static boolean paused, interpolation, engine;
static short elapsed;
static real speed = 1.0f;
static long ticks;

#define game_time_get_paused() paused
#define game_time_get_elapsed() elapsed
#define game_time_get_speed() speed
#define game_time_get() ticks
#define halo_interpolation_enabled() interpolation
#define game_engine_running() engine

static boolean decide(boolean smooth, long tick, short ticks_this_frame)
{
	boolean render_frame;

	interpolation = smooth;
	ticks = tick;
	elapsed = ticks_this_frame;
#include "under_test.inc"
	return render_frame;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	main_globals.halt_time_scale = 1;
	CASE("first tick")
	{
		CHECK(!decide(TRUE, 0, 0), "drawn before the map's first tick");
		CHECK(decide(TRUE, 1, 1), "not drawn at the first tick");
		CHECK(decide(TRUE, 1, 0), "not drawn between ticks");
		CHECK(decide(TRUE, 400, 0), "not drawn between later ticks");
		return 0;
	}
	CASE("original")
	{
		CHECK(!decide(FALSE, 0, 0), "drawn before the map's first tick");
		CHECK(decide(FALSE, 1, 1), "not drawn at a tick");
		CHECK(!decide(FALSE, 5, 0), "drawn without a tick");
		return 0;
	}
	CASE("menu")
	{
		main_globals.main_menu_scenario_loaded = TRUE;
		CHECK(decide(TRUE, 0, 0) && decide(FALSE, 0, 0), "the menus' map not drawn");
		return 0;
	}
	CASE("paused")
	{
		paused = TRUE;
		CHECK(decide(TRUE, 0, 0) && decide(FALSE, 0, 0), "a paused map not drawn");
		main_globals.halt_time_scale = 0;
		CHECK(!decide(TRUE, 7, 0), "drawn with time halted");
		return 0;
	}
	CASE("multiplayer")
	{
		engine = TRUE;
		CHECK(!decide(TRUE, 2, 1), "drawn before the third tick");
		CHECK(decide(TRUE, 3, 0), "not drawn from the third tick");
		return 0;
	}
	fprintf(stderr, "no case named %s\n", case_name);
	return 2;
}
