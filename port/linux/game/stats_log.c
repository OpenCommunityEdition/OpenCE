/*
STATS_LOG.C

A line a second in the log with the frame rate, the mode, the map, the
renderer and the size of the 3D view (display.stats_log, off by default):
a graphics bug's report, or two builds compared, says which of them was
running and what the frame rate was, without reading numbers off a screen.

Nothing is sent anywhere and nothing is drawn: it is the port's log (logcat
on Android). The renderer's text is the driver's own (glGetString, in
d3d8_gl.c's gl_initialize) and the mode is the game engine's, as the
scoreboard names it.
*/

#include "cseries.h"
#include "cache/cache_files.h"
#include "game/game_engine.h"
#include "render/render.h"
#include "stats_log.h"

#include <stdio.h>
#include <string.h>

/* port_config.c's */
int config_boolean(const char *name);
unsigned long config_changes(void);
/* the port's log, logcat on Android (platform.h) */
void platform_log(const char *format, ...);
/* the engine's clock (source/cseries/cseries_windows.c) */
unsigned long system_milliseconds(void);

/* the window a line covers: a second of frames */
#define STATS_LOG_MILLISECONDS 1000

static char stats_log_renderer[160] = "?";

static int stats_log_enabled(void)
{
	static unsigned long changes = (unsigned long)-1;
	static int enabled;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.stats_log") != 0;
	}
	return enabled;
}

static char const *stats_log_mode(void)
{
	struct game_variant *variant = game_engine_get_variant();

	if (!variant)
		return "menus";
	switch (variant->game_engine_index)
	{
	case game_engine_ctf: return "CTF";
	case game_engine_slayer: return "Slayer";
	case game_engine_oddball: return "Oddball";
	case game_engine_king: return "King";
	case game_engine_race: return "Race";
	case game_engine_terminator: return "Juggernaut";
	default: return "Game";
	}
}

static char const *stats_log_map(void)
{
	char const *map = cache_file_loaded_map_name();

	return (map && map[0]) ? map : "-";
}

/* the frames in a window of milliseconds, a second's by the log's */
static float stats_log_fps(
	unsigned long elapsed_milliseconds,
	unsigned long frames)
{
	return elapsed_milliseconds ? (float)frames * 1000.0f / (float)elapsed_milliseconds : 0.0f;
}

void stats_log_set_renderer(
	const char *vendor,
	const char *renderer)
{
	if (renderer && renderer[0])
		snprintf(stats_log_renderer, sizeof(stats_log_renderer), "%s", renderer);
	else if (vendor && vendor[0])
		snprintf(stats_log_renderer, sizeof(stats_log_renderer), "%s", vendor);
	else
		snprintf(stats_log_renderer, sizeof(stats_log_renderer), "?");
}

void stats_log_tick(void)
{
	static unsigned long window_start;
	static unsigned long frames;
	unsigned long now;
	unsigned long elapsed;
	rectangle2d bounds;

	if (!stats_log_enabled())
	{
		window_start = 0;
		frames = 0;
		return;
	}
	now = system_milliseconds();
	frames++;
	if (!window_start)
	{
		window_start = now;
		return;
	}
	elapsed = now - window_start;
	if (elapsed < STATS_LOG_MILLISECONDS)
		return;
	bounds = render.camera.window_bounds;
	platform_log("STATS fps=%.1f mode=%s map=%s gpu=\"%s\" res=%dx%d",
		stats_log_fps(elapsed, frames), stats_log_mode(), stats_log_map(),
		stats_log_renderer, (int)(bounds.x1 - bounds.x0), (int)(bounds.y1 - bounds.y0));
	window_start = now;
	frames = 0;
}
