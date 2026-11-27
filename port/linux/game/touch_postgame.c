/*
TOUCH_POSTGAME.C

The carnage report's buttons on the Android build's touchscreen. After a
multiplayer game the carnage report waits for a controller: the host's A
goes on to the next game (PICK GAME) and anyone's B leaves it
(game_engine_update_non_deterministic), as the prompt at its bottom says. A
phone without a controller has neither: the touch controls hide while the
report is up, and a tap is the menus' (ui_widget.c), which the report does
not read. So in the prompt's place the report draws the buttons it names,
CONTINUE (the host's only) and LEAVE, and a tap on one presses it for the
report. They show when the touch controls would (input.touch_controls,
touch_input.c): on a touchscreen, with "auto", the default, while no
controller is connected, and with "on" also with one. Otherwise (a
controller, "off", no touchscreen) the prompt is drawn as on the Xbox.
*/

#include "cseries.h"
#include "input/input.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "touch_postgame.h"

/* ---------- constants */

enum
{
	_touch_postgame_continue = 0,
	_touch_postgame_leave,
	NUMBER_OF_TOUCH_POSTGAME_BUTTONS,

	/* the buttons on the report's 640x480 screen: on the prompt's line
	(410), right-aligned, CONTINUE left of LEAVE, tall enough for a finger */
	TOUCH_POSTGAME_TOP = 396,
	TOUCH_POSTGAME_BOTTOM = 452,
	TOUCH_POSTGAME_RIGHT = 610,
	TOUCH_POSTGAME_WIDTH = 150,
	TOUCH_POSTGAME_GAP = 12,
	/* the report's line of text (game_engine.c's drawline) */
	TOUCH_POSTGAME_LINE_HEIGHT = 18,
	/* a press waits this long for the report's update to take it, and the
	buttons drawn take taps this long: neither outlives the report */
	TOUCH_POSTGAME_MILLISECONDS = 500,
	/* draw_string.c's centred text (its enum is its own) */
	_touch_postgame_text_centred = 2,
};

/* ---------- prototypes */

/* cseries_windows.c's, cinematics.c's, touch_input.c's */
unsigned long system_milliseconds(void);
void draw_quad(rectangle2d *rectangle, pixel32 color);
#ifdef HALO_ANDROID
int touch_input_screen_buttons(void);
#endif

/* ---------- globals */

static struct
{
	/* the buttons drawn at the report's latest frame, on its 640x480
	screen, and when */
	boolean shown[NUMBER_OF_TOUCH_POSTGAME_BUTTONS];
	rectangle2d bounds[NUMBER_OF_TOUCH_POSTGAME_BUTTONS];
	unsigned long drawn_at;
	/* a press the report has not taken: the gamepad's button, and when */
	boolean press_waiting;
	short pressed_button;
	unsigned long pressed_at;
} touch_postgame;

/* ---------- private code */

/* the Android build, as its touch controls would show */
static boolean touch_postgame_enabled(
	void)
{
#ifdef HALO_ANDROID
	return touch_input_screen_buttons() != 0;
#else
	return FALSE;
#endif
}

/* ---------- public code */

boolean touch_postgame_draw(
	long font_index,
	boolean host,
	real_argb_color const *color)
{
	static wchar_t const *const labels[NUMBER_OF_TOUCH_POSTGAME_BUTTONS] = { L"CONTINUE", L"LEAVE" };
	pixel32 fill = (pixel32)(PIN(color->alpha, 0.0f, 1.0f) * 160.0f + 0.5f) << 24 | 0x283848;
	short index;

	if (!touch_postgame_enabled())
		return FALSE;
	for (index = 0; index < NUMBER_OF_TOUCH_POSTGAME_BUTTONS; index++)
	{
		rectangle2d button;
		rectangle2d text;

		touch_postgame.shown[index] = index == _touch_postgame_leave || host;
		if (!touch_postgame.shown[index])
			continue;
		button.x1 = (short)(TOUCH_POSTGAME_RIGHT -
			(NUMBER_OF_TOUCH_POSTGAME_BUTTONS - 1 - index) * (TOUCH_POSTGAME_WIDTH + TOUCH_POSTGAME_GAP));
		button.x0 = (short)(button.x1 - TOUCH_POSTGAME_WIDTH);
		button.y0 = TOUCH_POSTGAME_TOP;
		button.y1 = TOUCH_POSTGAME_BOTTOM;
		touch_postgame.bounds[index] = button;
		/* (drawn from the viewport's top left, as the prompt is) */
		offset_rectangle2d(
			&button,
			-render.camera.viewport_bounds.x0,
			-render.camera.viewport_bounds.y0);
		draw_quad(&button, fill);
		text = button;
		text.y0 = (short)(button.y0 + (button.y1 - button.y0 - TOUCH_POSTGAME_LINE_HEIGHT) / 2);
		text.y1 = (short)(text.y0 + TOUCH_POSTGAME_LINE_HEIGHT);
		draw_string_set_draw_mode(font_index, NONE, _touch_postgame_text_centred, 0, color);
		rasterizer_draw_unicode_string(&text, NULL, NULL, 0, labels[index]);
	}
	touch_postgame.drawn_at = system_milliseconds();
	return TRUE;
}

boolean touch_postgame_click(
	short x,
	short y)
{
	unsigned long now = system_milliseconds();
	short index;

	if (!touch_postgame_enabled() || now - touch_postgame.drawn_at > TOUCH_POSTGAME_MILLISECONDS)
		return FALSE;
	for (index = 0; index < NUMBER_OF_TOUCH_POSTGAME_BUTTONS; index++)
	{
		rectangle2d const *bounds = &touch_postgame.bounds[index];

		if (touch_postgame.shown[index] &&
			x >= bounds->x0 && x < bounds->x1 && y >= bounds->y0 && y < bounds->y1)
		{
			touch_postgame.press_waiting = TRUE;
			touch_postgame.pressed_button = index == _touch_postgame_continue ?
				_gamepad_analog_button_a : _gamepad_analog_button_b;
			touch_postgame.pressed_at = now;
			return TRUE;
		}
	}
	return FALSE;
}

boolean touch_postgame_pressed(
	short button_index)
{
	if (!touch_postgame.press_waiting || touch_postgame.pressed_button != button_index)
		return FALSE;
	touch_postgame.press_waiting = FALSE;
	return system_milliseconds() - touch_postgame.pressed_at <= TOUCH_POSTGAME_MILLISECONDS;
}
