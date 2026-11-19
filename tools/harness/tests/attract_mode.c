/*
ATTRACT_MODE.C (test)

The real attract_mode_should_start and attract_mode_movies_available
(source/interface/attract_mode.c): at the main menu, the menu music stops
1.5 seconds before attract mode's 75 and a demo movie starts, but only with
display.attract_mode and one of the three movies in the bink folder (in any
language); without them the music plays on. test_attract_mode.py takes the
code and the times from the sources.
*/

#include "harness.h"

#include "config.inc"

#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define _snprintf snprintf

struct file_reference { char path[64]; };

static unsigned long now, last_event;
static boolean setting, music;
static int music_stops, music_starts;
static const char *present;

static boolean cache_files_precache_in_progress(void) { return FALSE; }
static short cache_files_precache_map_status(real *progress) { (void)progress; return 0; }
static void cache_files_precache_map_end(void) { }
static boolean main_menu_screen_is_active(void) { return TRUE; }
static boolean network_game_is_active(void) { return FALSE; }
static boolean bink_playback_active(void) { return FALSE; }
static unsigned long system_milliseconds(void) { return now; }
static unsigned long event_manager_time_of_last_event(void) { return last_event; }
static boolean ui_main_menu_music_active(void) { return music; }
static void ui_stop_main_menu_music(void) { music = FALSE; music_stops++; }
static void ui_start_main_menu_music(void) { music = TRUE; music_starts++; }
static struct file_reference *file_reference_create_from_path(struct file_reference *reference, const char *path,
	boolean directory)
{
	(void)directory;
	snprintf(reference->path, sizeof(reference->path), "%s", path);
	return reference;
}
static boolean file_exists(struct file_reference *reference)
{
	return present && !strcmp(reference->path, present);
}
int config_boolean(const char *name)
{
	CHECK(!strcmp(name, "display.attract_mode"), "the setting read: %s", name);
	return setting;
}

static unsigned long attract_mode_countdown_timer;

#include "under_test.inc"

/* idles at the main menu from 0 to the time given, a frame each 100 ms: the
time attract mode starts (0 if it does not) */
static unsigned long idle_until(unsigned long end)
{
	music = TRUE;
	music_stops = music_starts = 0;
	last_event = 1;
	attract_mode_countdown_timer = 0;
	for (now = 1; now <= end; now += 100)
		if (attract_mode_should_start())
			return now;
	return 0;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	unsigned long started;

	CASE("movies")
	{
		setting = TRUE;
		present = "d:\\bink\\attract2_fr.bik";
		started = idle_until(80000);
		CHECK(started >= ATTRACT_MODE_COUNTDOWN && started < ATTRACT_MODE_COUNTDOWN + 200,
			"attract mode at %lu ms", started);
		CHECK(music_stops == 1 && !music, "the music stopped %d times", music_stops);
		return 0;
	}
	CASE("no movies")
	{
		setting = TRUE;
		present = "d:\\bink\\intro.bik";
		started = idle_until(200000);
		CHECK(!started, "attract mode at %lu ms", started);
		CHECK(!music_stops && music, "the music stopped %d times", music_stops);
		return 0;
	}
	CASE("off")
	{
		setting = FALSE;
		present = "d:\\bink\\attract1.bik";
		started = idle_until(200000);
		CHECK(!started, "attract mode at %lu ms", started);
		CHECK(!music_stops && music, "the music stopped %d times", music_stops);
		return 0;
	}
	fprintf(stderr, "no case named %s\n", case_name);
	return 2;
}
