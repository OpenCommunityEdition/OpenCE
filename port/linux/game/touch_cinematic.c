/*
TOUCH_CINEMATIC.C

The skip button on the Android build's touchscreen during a cinematic. A
cinematic that can be skipped is skipped with A (player_control.c), and in
network co-op A votes to skip it (network_coop.c). A phone without a
controller has no A: a tap anywhere during the cinematic pressed it
(touch_input.c), which nothing on the screen said, and a tap meant for
nothing skipped the cinematic. So while the touch controls would show
(input.touch_controls, touch_input.c), the screen's overlays draw a button
at the top right, in the letterbox's bar, SKIP, and in network co-op VOTED
once this machine has voted, and only a tap on it presses A. Where it does
not show (a controller connected, "off", no touchscreen), any tap still
does.
*/

#include "cseries.h"
#include "cutscene/cinematics.h"
#include "interface/hud_messaging.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "network_coop.h"
#include "touch_cinematic.h"

/* ---------- constants */

enum
{
	/* the button, in thousandths of the window's height: at the top right,
	in the letterbox's bar (an eighth of the height: cinematic_render), clear
	of Android's edge gestures and a camera's cutout at the side; a tap a
	little around it takes it too, for a finger */
	TOUCH_CINEMATIC_WIDTH = 300,
	TOUCH_CINEMATIC_HEIGHT = 100,
	TOUCH_CINEMATIC_RIGHT = 120,
	TOUCH_CINEMATIC_TOP = 12,
	TOUCH_CINEMATIC_SLACK = 20,
	/* the button drawn takes taps this long: it never outlives the cinematic */
	TOUCH_CINEMATIC_MILLISECONDS = 500,
	/* draw_string.c's centred text (its enum is its own) */
	_touch_cinematic_text_centred = 2,
};

/* ---------- prototypes */

/* cseries_windows.c's, touch_input.c's */
unsigned long system_milliseconds(void);
#ifdef HALO_ANDROID
int touch_input_screen_buttons(void);
#endif

/* ---------- globals */

static struct
{
	/* where a tap takes the button last drawn, in thousandths of the screen
	across and down (a tap's place: touch_cinematic_tap), and when */
	boolean drawn;
	short left;
	short top;
	short right;
	short bottom;
	unsigned long drawn_at;
} touch_cinematic;

/* ---------- private code */

/* the button's label: SKIP, or in network co-op VOTED once this machine has
voted, only while its vote is open; NULL where the cinematic cannot be
skipped */
static wchar_t const *touch_cinematic_label(
	boolean *voted)
{
	short votes, voters;

	*voted = FALSE;
	if (network_coop_active())
		return network_coop_skip_vote_status(&votes, &voters, voted) ? (*voted ? L"VOTED" : L"SKIP") : NULL;
	return cinematic_can_be_skipped() ? L"SKIP" : NULL;
}

/* ---------- public code */

boolean touch_cinematic_button_enabled(
	void)
{
#ifdef HALO_ANDROID
	return touch_input_screen_buttons() != 0;
#else
	return FALSE;
#endif
}

void touch_cinematic_draw(
	void)
{
	long font_index = hud_get_font_index();
	real_argb_color color = *global_real_argb_white;
	rectangle2d const *viewport = &render.camera.viewport_bounds;
	rectangle2d const *window = &render.camera.window_bounds;
	short viewport_width = (short)(viewport->x1 - viewport->x0);
	short viewport_height = (short)(viewport->y1 - viewport->y0);
	short window_height = (short)(window->y1 - window->y0);
	short slack = (short)(window_height * TOUCH_CINEMATIC_SLACK / 1000);
	wchar_t const *label;
	boolean voted;
	struct font_header *font;
	short line_height;
	rectangle2d button;
	rectangle2d text;

	/* not over a menu (the pause menu), as the letterbox is not */
	if (!cinematic_in_progress() || ui_widgets_active() || !touch_cinematic_button_enabled())
		return;
	label = touch_cinematic_label(&voted);
	if (!label || font_index == NONE || window_height <= 0 || viewport_width <= 0 || viewport_height <= 0)
		return;
	button.x1 = (short)(window->x1 - window_height * TOUCH_CINEMATIC_RIGHT / 1000);
	button.x0 = (short)(button.x1 - window_height * TOUCH_CINEMATIC_WIDTH / 1000);
	button.y0 = (short)(window->y0 + window_height * TOUCH_CINEMATIC_TOP / 1000);
	button.y1 = (short)(button.y0 + window_height * TOUCH_CINEMATIC_HEIGHT / 1000);
	draw_quad(&button, voted ? 0x60283848 : 0xA0283848);
	font = font_definition_get(font_index);
	line_height = (short)(font->ascending_height + font->descending_height);
	text = button;
	text.y0 = (short)(button.y0 + (button.y1 - button.y0 - line_height) / 2);
	text.y1 = (short)(text.y0 + line_height);
	draw_string_set_draw_mode(font_index, NONE, _touch_cinematic_text_centred, 0, &color);
	rasterizer_draw_unicode_string(&text, NULL, NULL, 0, label);
	/* (in thousandths of the viewport, the screen's overlays' and so all of
	the screen, as a tap's place is of it) */
	touch_cinematic.drawn = TRUE;
	touch_cinematic.left = (short)((button.x0 - slack - viewport->x0) * 1000 / viewport_width);
	touch_cinematic.right = (short)((button.x1 + slack - viewport->x0) * 1000 / viewport_width);
	touch_cinematic.top = (short)((button.y0 - slack - viewport->y0) * 1000 / viewport_height);
	touch_cinematic.bottom = (short)((button.y1 + slack - viewport->y0) * 1000 / viewport_height);
	touch_cinematic.drawn_at = system_milliseconds();
}

int touch_cinematic_tap(
	float x,
	float y)
{
	short across = (short)(x * 1000.0f);
	short down = (short)(y * 1000.0f);

	/* (no button on the screen: any tap) */
	if (!touch_cinematic.drawn || !touch_cinematic_button_enabled() ||
		system_milliseconds() - touch_cinematic.drawn_at > TOUCH_CINEMATIC_MILLISECONDS)
	{
		return TRUE;
	}
	return across >= touch_cinematic.left && across < touch_cinematic.right &&
		down >= touch_cinematic.top && down < touch_cinematic.bottom;
}
