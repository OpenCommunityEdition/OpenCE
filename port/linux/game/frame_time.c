/*
FRAME_TIME.C

The frame-time overlay (display.frame_time): the milliseconds a frame takes
and the frames a second, averaged over half a second, in the terminal font
at the top right corner of the screen. It is drawn after every window, with
the game's frame statistics (rasterizer_xbox.c), so it is the same for one
window or four, and over the menus too. The Xbox had no such thing; it is
off by default.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "interface/interface.h"
#include "rasterizer/rasterizer.h"
#include "text/draw_string.h"
#include "text/text_group.h"
#include "frame_time.h"

#include <stdio.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

/* draw_string.c's */
enum
{
	_text_justification_left = 0,
	_text_justification_right
};

/* the frames counted together, at least this long */
#define FRAME_TIME_WINDOW_MILLISECONDS 500

static unsigned long frame_time_changes = (unsigned long)-1;
static boolean frame_time_enabled = FALSE;

static struct
{
	unsigned long window_start;
	long window_frames;
	/* the last window's, shown until the next is done */
	real milliseconds;
	real frames_per_second;
	boolean measured;
} frame_time;

void frame_time_draw(
	void)
{
	unsigned long now;
	unsigned long elapsed;

	if (frame_time_changes != config_changes())
	{
		frame_time_changes = config_changes();
		frame_time_enabled = config_boolean("display.frame_time") ? TRUE : FALSE;
		frame_time.window_frames = 0;
		frame_time.measured = FALSE;
	}
	if (!frame_time_enabled)
		return;

	now = system_milliseconds();
	if (frame_time.window_frames == 0)
	{
		frame_time.window_start = now;
	}
	else
	{
		elapsed = now - frame_time.window_start;
		if (elapsed >= FRAME_TIME_WINDOW_MILLISECONDS)
		{
			frame_time.milliseconds = (real)elapsed / (real)frame_time.window_frames;
			frame_time.frames_per_second = 1000.0f * (real)frame_time.window_frames / (real)elapsed;
			frame_time.measured = TRUE;
			frame_time.window_start = now;
			frame_time.window_frames = 0;
		}
	}
	frame_time.window_frames++;

	if (frame_time.measured)
	{
		static real_argb_color const color = { 1.0f, 1.0f, 1.0f, 1.0f };
		char text[48];
		rectangle2d bounds = rasterizer_globals.reserved04.frame_bounds;
		point2d cursor = { 0, 0 };

		sprintf(text, "%.2f ms  %.0f fps", frame_time.milliseconds, frame_time.frames_per_second);
		bounds.x1 -= 8;
		bounds.y0 += 4;
		interface_set_bitmap_text_draw_mode(_interface_font_terminal, _text_style_plain, _text_justification_right, 0,
			_interface_color_table_dialog, 0);
		draw_string_set_color(&color);
		rasterizer_draw_string(&bounds, NULL, &cursor, 0, text);
	}
}
