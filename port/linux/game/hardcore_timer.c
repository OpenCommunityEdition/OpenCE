/*
HARDCORE_TIMER.C

A hardcore match draws one clock at the bottom right, from the game's own
ticks, on whichever map it started. A map that still speaks from talking_timer
and power_call is left running. Classic games do not draw the clock.
*/

#include "cseries.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "cutscene/cinematics.h"
#include "text/font_group.h"
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
#include "hardcore_rules.h"
#include "hardcore_timer.h"

/* the minutes field of those titles starts at column 534 of 640, on the row
   215 of a 240-tall cell. that row is the bottom of the screen, not of a
   480-tall one, which is where the titles themselves land. */
#define HARDCORE_TIMER_LEFT 534
#define HARDCORE_TIMER_ROW_NUMERATOR 215
#define HARDCORE_TIMER_ROW_DENOMINATOR 240
#define HARDCORE_TIMER_HEIGHT 480
#define HARDCORE_TIMER_WIDTH 640

/* the same blue the titles use for the digits */
#define HARDCORE_TIMER_ALPHA (177.0f / 255.0f)
#define HARDCORE_TIMER_RED (100.0f / 255.0f)
#define HARDCORE_TIMER_GREEN (177.0f / 255.0f)
#define HARDCORE_TIMER_BLUE (255.0f / 255.0f)

/* the counters' column, left of the clock, measured on a 640-wide screen.
   the digits are three quarters of the clock, then another quarter smaller.
   the square stays at that first size. half opacity, the clock's blue. */
#define HARDCORE_SCORE_LEFT 449
#define HARDCORE_SCORE_BLOCK 0.75f
#define HARDCORE_SCORE_TEXT 0.75f
#define HARDCORE_SCORE_ALPHA 0.5f
/* the digit ink sits this far below the middle of the ascender box */
#define HARDCORE_SCORE_INK_BIAS 1.5f

extern struct data_array *hs_global_data;
extern short const hs_external_global_count;

/* one runtime global, as hs_runtime.c keeps it: the boolean is the first
   byte of value */
struct hardcore_timer_global
{
	short identifier;
	short unused;
	long value;
};

static void hardcore_timer_force_true(
	short designator)
{
	long index;
	struct hardcore_timer_global *global;

	/* a negative designator is missing, or an engine global. neither is
	   this map's announcer switch */
	if (designator<0 || hs_global_get_type(designator)!=_hs_type_boolean)
		return;

	index = (designator & 0x7FFF) + hs_external_global_count;
	global = datum_get(hs_global_data, index);
	global->value = 1;

	return;
}

void hardcore_timer_after_scripts(
	void)
{
	static long resolved_for = -1;
	static short talking_timer = NONE;
	static short power_call = NONE;

	if (!custom_edition_cache_is_hardcore() || !hs_global_data)
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

	hardcore_timer_force_true(talking_timer);
	hardcore_timer_force_true(power_call);

	return;
}

short hardcore_timer_top(
	void)
{
	return (short)(HARDCORE_TIMER_HEIGHT * HARDCORE_TIMER_ROW_NUMERATOR /
		HARDCORE_TIMER_ROW_DENOMINATOR);
}

/* the clock's own sideways move, so the counters stay a fixed gap left of it */
static short hardcore_timer_wide_shift(
	void)
{
	return (short)(((HARDCORE_TIMER_LEFT + HARDCORE_TIMER_WIDTH) / 2) *
		(halo_screen_width() - HARDCORE_TIMER_WIDTH) / HARDCORE_TIMER_WIDTH);
}

/* a clock under a minute is " :45". left of a square, that blank would
   shove the digits right of the line above. */
static void hardcore_score_blank_minute(
	wchar_t *text)
{
	if (text[0]==L' ' && text[1]==L':')
		text[0] = L'0';
}

static pixel32 hardcore_score_swatch(
	real_rgb_color const *rgb)
{
	pixel32 alpha = (pixel32)(HARDCORE_SCORE_ALPHA * 255.0f + 0.5f);

	return (alpha << 24) |
		((pixel32)(rgb->red * 255.0f + 0.5f) << 16) |
		((pixel32)(rgb->green * 255.0f + 0.5f) << 8) |
		(pixel32)(rgb->blue * 255.0f + 0.5f);
}

/* your team's score above the other team's, in the clock's column.
   the sheet's switch is the host's. the order is this machine's team. */
static void hardcore_score_render(
	long font_index,
	short shift)
{
	struct player_datum *player;
	struct font_header *font;
	short slot;
	wchar_t text[2][256];
	real_rgb_color const *swatch[2];
	long mine;
	short ascending;
	short descending;
	short line_box;
	short cap;
	short side;
	short gap;
	short text_x;
	short top;
	real scale;
	real_argb_color color;
	long line;

	if (!hardcore_rules_live()->team_scores || !game_engine_has_teams() ||
		!game_engine || !game_engine->format_team_name || !players_globals)
		return;
	/* the first occupied slot, which is the team this screen is showing */
	slot = local_player_get_next(NONE);
	player = player_try_and_get(slot == NONE ? NONE : local_player_get_player_index(slot));
	if (!player || player->team_index<0)
		return;

	mine = player->team_index>0 ? 1 : 0;
	csmemset(text, 0, sizeof(text));
	game_engine->format_team_name(mine, text[0]);
	game_engine->format_team_name(mine ? 0 : 1, text[1]);
	hardcore_score_blank_minute(text[0]);
	hardcore_score_blank_minute(text[1]);
	swatch[0] = mine ? global_real_rgb_blue : global_real_rgb_red;
	swatch[1] = mine ? global_real_rgb_red : global_real_rgb_blue;

	font = font_definition_get(font_index);
	ascending = font && font->ascending_height>4 ? font->ascending_height : 14;
	descending = font ? font->descending_height : 4;
	line_box = (short)(ascending + descending + 4);
	cap = (short)((real)ascending - HARDCORE_SCORE_INK_BIAS * 2.0f);
	if (cap<4)
		cap = 4;
	side = (short)((real)cap * 0.5f * HARDCORE_SCORE_BLOCK);
	if (side<2)
		side = 2;
	scale = HARDCORE_SCORE_BLOCK * HARDCORE_SCORE_TEXT;
	gap = (short)((real)cap * 0.5f * scale);
	text_x = (short)(HARDCORE_SCORE_LEFT + shift + side + gap);
	/* the two lines' ink straddles the clock's ink, not the screen */
	top = (short)((real)hardcore_timer_top() + (real)ascending * 0.5f +
		HARDCORE_SCORE_INK_BIAS - scale * ((real)ascending * 0.5f +
		HARDCORE_SCORE_INK_BIAS + (real)line_box * 0.5f) + 0.5f);

	color.alpha = HARDCORE_SCORE_ALPHA;
	color.red = HARDCORE_TIMER_RED;
	color.green = HARDCORE_TIMER_GREEN;
	color.blue = HARDCORE_TIMER_BLUE;
	for (line = 0; line<2; line++)
	{
		rectangle2d bounds;
		rectangle2d square;
		short row;
		real ink;

		if (!text[line][0])
			continue;
		row = (short)((real)top + (real)line * (real)line_box * scale);
		ink = (real)row + ((real)ascending * 0.5f + HARDCORE_SCORE_INK_BIAS) * scale;
		square.x0 = (short)(HARDCORE_SCORE_LEFT + shift);
		square.x1 = (short)(square.x0 + side);
		square.y0 = (short)(ink - (real)side * 0.5f);
		square.y1 = (short)(square.y0 + side);
		draw_quad(&square, hardcore_score_swatch(swatch[line]));

		rasterizer_text_set_scale(scale, (real)text_x, (real)row);
		draw_string_set_draw_mode(font_index, NONE, 0, 0, &color);
		bounds.x0 = text_x;
		bounds.y0 = row;
		bounds.x1 = (short)halo_screen_width();
		bounds.y1 = (short)(row + line_box);
		rasterizer_text_set_shadow_color(0xC0000000);
		rasterizer_draw_unicode_string(&bounds, &bounds, NULL, 0, text[line]);
	}
	rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
	rasterizer_text_set_shadow_color(0);
}

void hardcore_timer_render(
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
	if (!hardcore_rules_active() || !game_in_progress() ||
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

	color.alpha = HARDCORE_TIMER_ALPHA;
	color.red = HARDCORE_TIMER_RED;
	color.green = HARDCORE_TIMER_GREEN;
	color.blue = HARDCORE_TIMER_BLUE;
	draw_string_set_draw_mode(font_index, NONE, 0, 0, &color);

	bounds.y0 = hardcore_timer_top();
	bounds.x0 = HARDCORE_TIMER_LEFT;
	bounds.y1 = HARDCORE_TIMER_HEIGHT;
	bounds.x1 = HARDCORE_TIMER_WIDTH;
	/* the same side shift a chapter title gets on a screen wider than 640 */
	shift = hardcore_timer_wide_shift();
	bounds.x0 = (short)(bounds.x0 + shift);
	bounds.x1 = (short)(bounds.x1 + shift);

	rasterizer_text_set_shadow_color(0xC0000000);
	rasterizer_draw_unicode_string(&bounds, &bounds, NULL, 0, text);
	rasterizer_text_set_shadow_color(0);
	hardcore_score_render(font_index, shift);

	return;
}
