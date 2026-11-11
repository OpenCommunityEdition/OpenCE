/*
TOUCH_CINEMATIC.C (test)

The touchscreen's skip button during a cinematic, real code over a fake
screen: port/linux/game/touch_cinematic.c draws it (SKIP, or in network co-op
VOTED once this machine has voted) where the touch controls would show and
says whether a tap presses A, and port/linux/game/touch_game.c says whether
a cinematic can be skipped, or voted to skip by a network co-op client.
test_touch_cinematic.py takes the constants and the state (config.inc) and
the code (under_test.inc).
*/

#include "harness.h"

#include <wchar.h>

#define HALO_ANDROID

typedef unsigned long pixel32;
typedef struct { short y0, x0, y1, x1; } rectangle2d;
typedef struct { real alpha, red, green, blue; } real_argb_color;
struct font_header { short ascending_height, descending_height; };

/* the fake world: the screen's overlays' window, the HUD's font */
static struct
{
	struct
	{
		rectangle2d viewport_bounds;
		rectangle2d window_bounds;
	} camera;
} render;
static real_argb_color const white = { 1.0f, 1.0f, 1.0f, 1.0f };
static real_argb_color const *global_real_argb_white = &white;
static struct font_header hud_font = { 10, 4 };
static long hud_font_index = 3;
static unsigned long now_milliseconds = 100000;

/* where the touch controls would show (touch_input.c), a menu, the cinematic
and the network co-op vote */
static int screen_buttons = TRUE;
static boolean menu_up = FALSE;
static void *cinematic_globals = &hud_font;
static boolean game_running = TRUE;
static boolean cinematic_playing = TRUE;
static boolean can_be_skipped = TRUE;
static boolean coop = FALSE;
static boolean vote_open = FALSE;
static boolean voted_already = FALSE;
static boolean coop_client_offered = FALSE;

/* what was drawn */
static int quads;
static rectangle2d quad;
static pixel32 quad_color;
static wchar_t const *drawn_label;
static rectangle2d drawn_label_bounds;

#define system_milliseconds() (now_milliseconds)
#define touch_input_screen_buttons() (screen_buttons)
#define ui_widgets_active() (menu_up)
#define hud_get_font_index() (hud_font_index)
#define font_definition_get(index) (&hud_font)
#define cinematic_in_progress() (cinematic_playing)
#define cinematic_can_be_skipped() (can_be_skipped)
#define game_in_progress() (game_running)
#define network_coop_active() (coop)
#define network_coop_skip_offered() (coop_client_offered)
#define draw_string_set_draw_mode(index, style, justification, flags, color) ((void)0)

static boolean network_coop_skip_vote_status(
	short *votes,
	short *voters,
	boolean *voted)
{
	if (!vote_open)
		return FALSE;
	*votes = voted_already ? 1 : 0;
	*voters = 2;
	*voted = voted_already;
	return TRUE;
}

static void draw_quad(
	rectangle2d *rectangle,
	pixel32 color)
{
	quads++;
	quad = *rectangle;
	quad_color = color;
}

static void rasterizer_draw_unicode_string(
	rectangle2d const *bounds,
	void const *style,
	void const *color,
	unsigned long flags,
	wchar_t const *string)
{
	drawn_label_bounds = *bounds;
	drawn_label = string;
}

#include "config.inc"
#include "under_test.inc"

/* a screen of width by 480 with the window across all of it, as Android's
display is */
static void screen(
	short width)
{
	render.camera.viewport_bounds.x0 = 0;
	render.camera.viewport_bounds.y0 = 0;
	render.camera.viewport_bounds.x1 = width;
	render.camera.viewport_bounds.y1 = 480;
	render.camera.window_bounds = render.camera.viewport_bounds;
}

/* a point of the screen, as a tap has it */
static float tap_x(
	short x)
{
	return (float)x / (float)(render.camera.viewport_bounds.x1 - render.camera.viewport_bounds.x0);
}

static float tap_y(
	short y)
{
	return (float)y / (float)(render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0);
}

static void draw(
	void)
{
	quads = 0;
	drawn_label = NULL;
	touch_cinematic_draw();
}

/* a tap on the drawn button's middle */
static boolean tap_middle(
	void)
{
	return touch_cinematic_tap(tap_x((short)((quad.x0 + quad.x1) / 2)), tap_y((short)((quad.y0 + quad.y1) / 2)));
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* (a negative control takes away the code's only read of these, which
	clang 23 would refuse as set but not used) */
	(void)menu_up;
	(void)coop;
	(void)coop_client_offered;
	screen(854);

	/* a solo cinematic that can be skipped: SKIP at the top right, in the
	letterbox's bar (the top eighth), finger sized, its label inside it; a
	tap on it, or just beside it, presses A, a tap elsewhere does not */
	CASE("solo-button")
	{
		draw();
		CHECK(quads == 1 && drawn_label && !wcscmp(drawn_label, L"SKIP"), "%d quads, label %ls", quads,
			drawn_label ? drawn_label : L"-");
		CHECK(quad.x1 < 854 - 480 / 20 && quad.x1 > 854 - 480 / 5 && quad.x0 > 854 / 2, "button at x %d to %d",
			quad.x0, quad.x1);
		CHECK(quad.y0 > 0 && quad.y1 <= 480 / 8, "button at y %d to %d, not in the letterbox's bar", quad.y0,
			quad.y1);
		CHECK(quad.y1 - quad.y0 >= 48 && quad.x1 - quad.x0 >= 2 * (quad.y1 - quad.y0), "button %d by %d",
			quad.x1 - quad.x0, quad.y1 - quad.y0);
		CHECK(drawn_label_bounds.y0 >= quad.y0 && drawn_label_bounds.y1 <= quad.y1 &&
			drawn_label_bounds.x0 == quad.x0 && drawn_label_bounds.x1 == quad.x1, "label outside the button");
		CHECK(tap_middle(), "a tap on the button did not press A");
		CHECK(touch_cinematic_tap(tap_x((short)(quad.x0 + 2)), tap_y((short)(quad.y1 - 2))),
			"a tap inside the button's corner did not press A");
		CHECK(touch_cinematic_tap(tap_x((short)(quad.x0 - 4)), tap_y((short)(quad.y1 + 4))),
			"a tap just beside the button did not press A");
		CHECK(!touch_cinematic_tap(0.5f, 0.5f), "a tap in the middle of the screen pressed A");
		CHECK(!touch_cinematic_tap(0.1f, 0.9f), "a tap at the bottom left pressed A");
		CHECK(!touch_cinematic_tap(tap_x((short)(quad.x0 - 30)), tap_y((short)((quad.y0 + quad.y1) / 2))),
			"a tap well left of the button pressed A");
		CHECK(!touch_cinematic_tap(tap_x((short)((quad.x0 + quad.x1) / 2)), tap_y((short)(quad.y1 + 30))),
			"a tap well under the button pressed A");
		return 0;
	}
	/* no button where the touch controls would not show (a controller, "off",
	no touchscreen), outside a cinematic, nor before one is drawn: any tap
	presses A, as before */
	CASE("no-button-any-tap")
	{
		CHECK(touch_cinematic_tap(0.5f, 0.5f), "before any button, a tap did not press A");
		screen_buttons = FALSE;
		draw();
		CHECK(quads == 0 && !drawn_label, "a button drawn without the touch controls");
		CHECK(touch_cinematic_tap(0.5f, 0.5f), "without a button, a tap did not press A");
		screen_buttons = TRUE;
		cinematic_playing = FALSE;
		draw();
		CHECK(quads == 0, "a button drawn outside a cinematic");
		cinematic_playing = TRUE;
		hud_font_index = NONE;
		draw();
		CHECK(quads == 0, "a button drawn without the HUD's font");
		return 0;
	}
	/* not over a menu (the pause menu during a cinematic), as the letterbox
	is not; the menu takes the taps then (touch_input.c) */
	CASE("not-over-a-menu")
	{
		menu_up = TRUE;
		draw();
		CHECK(quads == 0 && !drawn_label, "a button drawn over a menu");
		menu_up = FALSE;
		draw();
		CHECK(quads == 1, "no button once the menu went");
		return 0;
	}
	/* the button goes with the cinematic: once it is no longer drawn, any tap
	presses A again */
	CASE("stale-button")
	{
		draw();
		CHECK(!touch_cinematic_tap(0.5f, 0.5f), "a tap off the drawn button pressed A");
		now_milliseconds += TOUCH_CINEMATIC_MILLISECONDS / 2;
		CHECK(!touch_cinematic_tap(0.5f, 0.5f), "a tap off a button drawn a moment ago pressed A");
		now_milliseconds += TOUCH_CINEMATIC_MILLISECONDS;
		CHECK(touch_cinematic_tap(0.5f, 0.5f), "a button no longer drawn still took the taps");
		return 0;
	}
	/* in network co-op the button shows only while the vote is open (the host
	waits for the save first, though its cinematic can be skipped), SKIP, and
	VOTED, fainter, once this machine has voted */
	CASE("coop-vote")
	{
		pixel32 open_color;

		coop = TRUE;
		vote_open = FALSE;
		can_be_skipped = TRUE;
		draw();
		CHECK(quads == 0 && !drawn_label, "a button drawn before the vote opened");
		vote_open = TRUE;
		can_be_skipped = FALSE;
		draw();
		CHECK(quads == 1 && drawn_label && !wcscmp(drawn_label, L"SKIP"), "open vote: %d quads, label %ls", quads,
			drawn_label ? drawn_label : L"-");
		open_color = quad_color;
		CHECK(tap_middle(), "a tap on the vote's button did not press A");
		CHECK(!touch_cinematic_tap(0.5f, 0.5f), "a tap off the vote's button pressed A");
		voted_already = TRUE;
		draw();
		CHECK(quads == 1 && drawn_label && !wcscmp(drawn_label, L"VOTED"), "voted: %d quads, label %ls", quads,
			drawn_label ? drawn_label : L"-");
		CHECK((quad_color >> 24) < (open_color >> 24), "voted button not fainter: %08lx, %08lx", quad_color,
			open_color);
		return 0;
	}
	/* a window narrower than the screen (a 4:3 window centred on a wide
	screen): the button keeps to the window, and a tap is placed on the
	screen */
	CASE("window-inside-the-screen")
	{
		render.camera.window_bounds.x0 = 107;
		render.camera.window_bounds.x1 = 747;
		draw();
		CHECK(quads == 1 && quad.x1 < 747 && quad.x0 > 107, "button at x %d to %d, not in the window", quad.x0,
			quad.x1);
		CHECK(tap_middle(), "a tap on the button did not press A");
		CHECK(!touch_cinematic_tap(tap_x(850), tap_y((short)((quad.y0 + quad.y1) / 2))),
			"a tap at the screen's right edge pressed A");
		return 0;
	}
	/* touch_game.c: a cinematic that can be skipped, or that a network co-op
	client may vote to skip (its own cinematic never can be); none outside a
	cinematic or before the game exists */
	CASE("skippable")
	{
		CHECK(touch_game_cinematic_skippable(), "a skippable cinematic not skippable");
		can_be_skipped = FALSE;
		CHECK(!touch_game_cinematic_skippable(), "an unskippable cinematic skippable");
		coop_client_offered = TRUE;
		CHECK(touch_game_cinematic_skippable(), "a co-op client's open vote not skippable: its tap cannot vote");
		cinematic_playing = FALSE;
		CHECK(!touch_game_cinematic_skippable(), "skippable outside a cinematic");
		cinematic_playing = TRUE;
		game_running = FALSE;
		CHECK(!touch_game_cinematic_skippable(), "skippable outside a game");
		game_running = TRUE;
		cinematic_globals = NULL;
		CHECK(!touch_game_cinematic_skippable(), "skippable before the game exists");
		return 0;
	}
	fprintf(stderr, "unknown case %s\n", case_name);
	return 2;
}
