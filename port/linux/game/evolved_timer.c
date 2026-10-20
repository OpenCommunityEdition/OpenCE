/*
EVOLVED_TIMER.C

An evolved match draws one clock at the bottom right, from the game's own
ticks, on whichever map it started. A map that still speaks from talking_timer
and power_call is left running. Classic games do not draw the clock.
*/

#include "cseries.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/players.h"
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "interface/hud_definitions.h"
#include "interface/ui_widget.h"
#include "memory/data.h"
#include "rasterizer/rasterizer.h"
#include "scenario/scenario.h"
#include "text/draw_string.h"
#include "hs/hs.h"
#include "custom_edition_cache.h"
#include "evolved_rules.h"
#include "evolved_timer.h"

/* the minutes field of those titles starts at column 534 of 640, on the row
   215 of a 240-tall cell. that row is the bottom of the screen, not of a
   480-tall one, which is where the titles themselves land. */
#define EVOLVED_TIMER_LEFT 534
#define EVOLVED_TIMER_ROW_NUMERATOR 215
#define EVOLVED_TIMER_ROW_DENOMINATOR 240
#define EVOLVED_TIMER_HEIGHT 480
#define EVOLVED_TIMER_WIDTH 640

/* the same blue the titles use for the digits */
#define EVOLVED_TIMER_ALPHA (177.0f / 255.0f)
#define EVOLVED_TIMER_RED (100.0f / 255.0f)
#define EVOLVED_TIMER_GREEN (177.0f / 255.0f)
#define EVOLVED_TIMER_BLUE (255.0f / 255.0f)

extern struct data_array *hs_global_data;
extern short const hs_external_global_count;

/* one runtime global, as hs_runtime.c keeps it: the boolean is the first
   byte of value */
struct evolved_timer_global
{
	short identifier;
	short unused;
	long value;
};

static void evolved_timer_force_true(
	short designator)
{
	long index;
	struct evolved_timer_global *global;

	/* a negative designator is missing, or an engine global. neither is
	   this map's announcer switch */
	if (designator<0 || hs_global_get_type(designator)!=_hs_type_boolean)
		return;

	index = (designator & 0x7FFF) + hs_external_global_count;
	global = datum_get(hs_global_data, index);
	global->value = 1;

	return;
}

void evolved_timer_after_scripts(
	void)
{
	static long resolved_for = -1;
	static short talking_timer = NONE;
	static short power_call = NONE;

	if (!custom_edition_cache_is_evolved() || !hs_global_data)
		return;

	/* resolved once a map, and again when the next one loads. the startup
	   script sets both false on the first tick, so this runs after the
	   scripts and every tick after that */
	if (resolved_for!=global_scenario_index)
	{
		resolved_for = global_scenario_index;
		talking_timer = hs_find_global_by_name("talking_timer");
		power_call = hs_find_global_by_name("power_call");
	}

	evolved_timer_force_true(talking_timer);
	evolved_timer_force_true(power_call);

	return;
}

short evolved_timer_top(
	void)
{
	return (short)(EVOLVED_TIMER_HEIGHT * EVOLVED_TIMER_ROW_NUMERATOR /
		EVOLVED_TIMER_ROW_DENOMINATOR);
}

void evolved_timer_render(
	void)
{
	long font_index;
	long ticks;
	long seconds;
	long minutes;
	wchar_t text[6];
	real_argb_color color;
	rectangle2d bounds;
	short shift;

	/* port: the match, not the map folder. a classic game does not set it. */
	if (!evolved_rules_active() || !game_in_progress() ||
		ui_widgets_active() || local_player_count()<=0 || !hud_globals)
		return;

	font_index = hud_globals->messaging.single_player_font.index;
	if (font_index==NONE)
		font_index = hud_get_font_index();
	if (font_index==NONE)
		return;

	ticks = game_time_get();
	if (ticks<0)
		ticks = 0;
	seconds = ticks / TICKS_PER_SECOND;
	minutes = seconds / 60;
	seconds %= 60;
	if (minutes>99)
		minutes = 99;

	text[0] = (wchar_t)(L'0' + minutes / 10);
	text[1] = (wchar_t)(L'0' + minutes % 10);
	text[2] = L':';
	text[3] = (wchar_t)(L'0' + seconds / 10);
	text[4] = (wchar_t)(L'0' + seconds % 10);
	text[5] = 0;

	color.alpha = EVOLVED_TIMER_ALPHA;
	color.red = EVOLVED_TIMER_RED;
	color.green = EVOLVED_TIMER_GREEN;
	color.blue = EVOLVED_TIMER_BLUE;
	draw_string_set_draw_mode(font_index, NONE, 0, 0, &color);

	bounds.y0 = evolved_timer_top();
	bounds.x0 = EVOLVED_TIMER_LEFT;
	bounds.y1 = EVOLVED_TIMER_HEIGHT;
	bounds.x1 = EVOLVED_TIMER_WIDTH;
	/* the same side shift a chapter title gets on a screen wider than 640 */
	shift = (short)(((bounds.x0 + bounds.x1) / 2) *
		(halo_screen_width() - EVOLVED_TIMER_WIDTH) / EVOLVED_TIMER_WIDTH);
	bounds.x0 = (short)(bounds.x0 + shift);
	bounds.x1 = (short)(bounds.x1 + shift);

	rasterizer_text_set_shadow_color(0xC0000000);
	rasterizer_draw_unicode_string(&bounds, &bounds, NULL, 0, text);
	rasterizer_text_set_shadow_color(0);

	return;
}
