/*
CINEMATIC_TITLES.C (test)

The real cinematic_render (cinematics.c) over a fake scenario with one title, as
some Custom Edition maps show their name for the whole game: drawn in the game,
not over a menu (the screen's overlays are drawn after the players' menus), and
its time going on under one. Widescreen placement follows the title alignment;
MCC clipping expansion preserves that anchor. test_cinematic_titles.py takes the
structures, the slots' count (config.inc) and the code (under_test.inc).
*/

#include <math.h>
#include "harness.h"

typedef unsigned short word;
typedef unsigned long pixel32;
typedef struct { short y0, x0, y1, x1; } rectangle2d;
typedef struct { real alpha, red, green, blue; } real_argb_color;
#define TAG_STRING_LENGTH 31
#define TICKS_PER_SECOND 30
#define _real_epsilon 0.0001f
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(n, low, high) ((n) < (low) ? (low) : ((n) > (high) ? (high) : (n)))
#define fast_ftol(value) ((long)(value))

#include "config.inc"

static struct cinematic_global_data globals_data;
static struct cinematic_global_data *cinematic_globals = &globals_data;

/* a menu (any local player's widget) is up or not */
static boolean menu_up;
static boolean ui_widgets_active(void) { return menu_up; }

/* the game's clock: one tick a frame */
static long ticks;
#define game_time_get() (ticks)
#define game_time_get_paused() FALSE
#define game_time_get_elapsed() 1
#define game_in_editor() FALSE

/* the letterbox's camera and bars */
static struct { struct { rectangle2d viewport_bounds; } camera; } render;
static long quads;
static void draw_quad(rectangle2d const *bounds, pixel32 color) { quads++; }

/* the HUD's font and the scenario's one title, its text in the ingame help text */
static struct
{
	struct { struct { long index; } single_player_font; } messaging;
	struct { rectangle2d default_title_bounds; } defaults;
} hud_globals_data = { { { 3 } } }, *hud_globals = &hud_globals_data;
struct tag_block { long count; void *address; };
static struct scenario_cutscene_title titles[1];
static struct
{
	struct tag_block cutscene_chapter_titles;
	struct { long index; } ingame_help_text;
} scenario = { { 1, titles }, { 7 } };
#define global_scenario_get() (&scenario)
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(element)])
struct string_list { struct { long count; } strings; };
static struct string_list help_text = { { 1 } };
#define unicode_string_list_definition_get(tag_index) (&help_text)
#define unicode_string_list_get_string(tag_index, string_index) NULL

/* the text drawn: recorded */
static long draws;
static real drawn_alpha;
static pixel32 shadow, drawn_shadow;
static void pixel32_to_real_argb_color(pixel32 pixel, real_argb_color *color)
{
	color->alpha = (real)((pixel >> 24) & 0xFF) / 255.0f;
	color->red = (real)((pixel >> 16) & 0xFF) / 255.0f;
	color->green = (real)((pixel >> 8) & 0xFF) / 255.0f;
	color->blue = (real)(pixel & 0xFF) / 255.0f;
}
static void draw_string_set_draw_mode(long font_index, short style, short justification, unsigned long flags,
	real_argb_color const *color)
{
	drawn_alpha = color->alpha;
}
static void rasterizer_text_set_shadow_color(pixel32 color) { shadow = color; }
static short screen_width = 640;
static boolean mcc_loaded;
#define SHORT_MAX 32767
static short halo_screen_width(void) { return screen_width; }
static boolean mcc_cache_tags_loaded(void) { return mcc_loaded; }
static rectangle2d drawn_bounds;
#define rasterizer_draw_unicode_string(bounds, style, color, flags, string) (draws++, drawn_shadow = shadow, drawn_bounds = *(bounds))

#include "under_test.inc"

/* the title in its slot, at its start: up for up_time ticks, then fading out for fade_out_time */
static void queue_title(real up_time, real fade_out_time)
{
	short slot;

	for (slot = 0; slot < MAXIMUM_QUEUED_CINEMATIC_TITLES; slot++)
	{
		globals_data.queued_titles[slot].title_index = NONE;
		globals_data.queued_titles[slot].time = NONE;
	}
	globals_data.queued_titles[0].title_index = 0;
	globals_data.queued_titles[0].time = 0;
	titles[0].bounds.x0 = 400;
	titles[0].bounds.x1 = 640;
	titles[0].bounds.y0 = 440;
	titles[0].bounds.y1 = 480;
	titles[0].foreground_color = 0xFF80C0FF;
	titles[0].shadow_color = 0xFF000000;
	titles[0].fade_in_time = 0.0f;
	titles[0].up_time = up_time;
	titles[0].fade_out_time = fade_out_time;
}

/* one frame: the draws it made */
static long frame(void)
{
	draws = 0;
	ticks++;
	cinematic_render();
	return draws;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct cinematic_title const *slot = &globals_data.queued_titles[0];
	long index;

	CASE("title-screen-anchors")
	{
		static short const widths[] = { 640, 852, 1120, 480 };
		static short const shifts[3][4] = {
			{ 0, 13, 30, -10 }, /* left edge at 40 */
			{ 0, 197, 447, -149 }, /* right edge at 597 */
			{ 0, 105, 238, -79 } /* center at 318 */
		};
		short alignment, width;
		rectangle2d original;
		queue_title(1000000.0f, 1.0f);
		titles[0].bounds.x0 = 40;
		titles[0].bounds.x1 = 597;
		original = titles[0].bounds;
		for (alignment = 0; alignment < 3; alignment++)
		{
			titles[0].justification = alignment;
			for (width = 0; width < 4; width++)
			{
				screen_width = widths[width];
				frame();
				CHECK(drawn_bounds.x0 == 40 + shifts[alignment][width] &&
					drawn_bounds.x1 == 597 + shifts[alignment][width],
					"alignment %d at width %d: bounds %d..%d", alignment, screen_width, drawn_bounds.x0, drawn_bounds.x1);
				CHECK(drawn_bounds.y0 == original.y0 && drawn_bounds.y1 == original.y1, "vertical bounds moved");
				CHECK(!memcmp(&original, &titles[0].bounds, sizeof(original)), "authored bounds mutated");
			}
		}
		return 0;
	}
	CASE("default-title-anchor")
	{
		queue_title(1000000.0f, 1.0f);
		hud_globals->defaults.default_title_bounds = titles[0].bounds;
		hud_globals->defaults.default_title_bounds.x0 = 0;
		hud_globals->defaults.default_title_bounds.x1 = 597;
		titles[0].bounds.x1 = titles[0].bounds.x0;
		titles[0].justification = 1;
		screen_width = 852;
		frame();
		CHECK(drawn_bounds.x0 == 197 && drawn_bounds.x1 == 794, "default bounds did not use right anchor");
		CHECK(hud_globals->defaults.default_title_bounds.x1 == 597, "HUD default bounds mutated");
		return 0;
	}
	CASE("mcc-title-screen-anchor")
	{
		queue_title(1000000.0f, 1.0f);
		titles[0].bounds.x0 = 40;
		titles[0].bounds.x1 = 597;
		titles[0].justification = 1;
		screen_width = 852;
		mcc_loaded = TRUE;
		frame();
		CHECK(drawn_bounds.x0 == 0 && drawn_bounds.x1 == 794, "MCC expansion changed right anchor");
		titles[0].text_flags = 1;
		frame();
		CHECK(drawn_bounds.x0 == 237 && drawn_bounds.x1 == 794, "wrapping title moved its anchor");
		mcc_loaded = FALSE;
		titles[0].text_flags = 0;
		frame();
		CHECK(drawn_bounds.x0 == 237 && drawn_bounds.x1 == 794, "MCC expansion leaked into legacy title");
		screen_width = 640;
		frame();
		CHECK(drawn_bounds.x0 == 40 && drawn_bounds.x1 == 597, "resolution change retained wide bounds");
		return 0;
	}
	CASE("mcc-title-bounds")
	{
		rectangle2d original;
		queue_title(1000000.0f, 1.0f);
		titles[0].bounds.x0 = 240;
		titles[0].bounds.x1 = 400;
		titles[0].justification = 2;
		original = titles[0].bounds;
		/* Xbox and CE keep the exact authored clipping rectangle. */
		frame();
		CHECK(drawn_bounds.x0 == 240 && drawn_bounds.x1 == 400, "legacy bounds changed");
		mcc_loaded = TRUE;
		frame();
		CHECK(drawn_bounds.x0 == 0 && drawn_bounds.x1 == 640, "MCC title still clipped");
		CHECK(drawn_bounds.y0 == original.y0 && drawn_bounds.y1 == original.y1, "vertical bounds changed");
		CHECK(!memcmp(&original, &titles[0].bounds, sizeof(original)), "authored tag changed");
		screen_width = 854;
		frame();
		CHECK(drawn_bounds.x0 == 0 && drawn_bounds.x1 == 854, "widescreen center moved");
		titles[0].text_flags = 1;
		frame();
		CHECK(drawn_bounds.x0 == 347 && drawn_bounds.x1 == 507, "wrapping bounds changed");
		titles[0].text_flags = 0;
		mcc_loaded = FALSE;
		frame();
		CHECK(drawn_bounds.x0 == 347 && drawn_bounds.x1 == 507, "MCC bounds leaked after unload");
		return 0;
	}
	CASE("mcc-title-anchors")
	{
		rectangle2d bounds = { 160, 100, 500, 261 };
		mcc_loaded = TRUE;
		mcc_hud_title_bounds(&bounds, 2, 0);
		CHECK(bounds.x0 == 0 && bounds.x1 == 361, "odd-width center moved");
		bounds.x0 = 240; bounds.x1 = 400;
		mcc_hud_title_bounds(&bounds, 0, 0);
		CHECK(bounds.x0 == 240 && bounds.x1 == 640, "left anchor moved");
		bounds.x0 = 240; bounds.x1 = 400;
		mcc_hud_title_bounds(&bounds, 1, 0);
		CHECK(bounds.x0 == 0 && bounds.x1 == 400, "right anchor moved");
		bounds.x0 = -10; bounds.x1 = 400;
		mcc_hud_title_bounds(&bounds, 2, 0);
		CHECK(bounds.x0 == -10 && bounds.x1 == 400, "offscreen bounds changed");
		bounds.x0 = 400; bounds.x1 = 240;
		mcc_hud_title_bounds(&bounds, 2, 0);
		CHECK(bounds.x0 == 400 && bounds.x1 == 240, "inverted bounds changed");
		return 0;
	}
	/* a title for the whole game, no menu: drawn each frame, opaque */
	CASE("drawn-in-the-game")
	{
		queue_title(1000000.0f, 1.0f);
		for (index = 0; index < 3; index++)
			CHECK(frame() == 1, "the title was drawn %ld times in frame %ld", draws, index);
		CHECK(drawn_alpha > 0.99f, "the title was drawn with alpha %f", drawn_alpha);
		CHECK(drawn_shadow >> 24 == 0xFF, "the title's shadow was drawn with alpha %lu", drawn_shadow >> 24);
		CHECK(quads == 0, "%ld letterbox bars drawn without a letterbox", quads);
		return 0;
	}
	/* a menu up: the title is not drawn over it, and is again once it is gone */
	CASE("not-over-a-menu")
	{
		queue_title(1000000.0f, 1.0f);
		menu_up = TRUE;
		for (index = 0; index < 3; index++)
			CHECK(frame() == 0, "the title was drawn over a menu in frame %ld", index);
		menu_up = FALSE;
		CHECK(frame() == 1, "the title was not drawn after the menu was gone");
		return 0;
	}
	/* a menu up: the title's time goes on, so it ends when it would have */
	CASE("time-goes-on-under-a-menu")
	{
		queue_title(5.0f, 5.0f);
		menu_up = TRUE;
		for (index = 0; index < 3; index++)
			frame();
		CHECK(slot->time == 3, "the title's time is %d after 3 frames under a menu", slot->time);
		for (index = 0; index < 7; index++)
			frame();
		CHECK(slot->title_index == NONE, "the title's slot is still taken after its 10 ticks under a menu");
		menu_up = FALSE;
		CHECK(frame() == 0, "an ended title was drawn after the menu was gone");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
