/*
CINEMATIC_LETTERBOX.C

display.cinematic_framing (off by default): every cinematic shot with the
letterbox drawn as the Xbox showed it, as Bungie's later games set their
bars by the screen's shape. The Xbox drew a 4:3 picture and covered an
eighth of its height with each of the letterbox's bars (cinematic_render),
which left a 16:9 picture between them. The port keeps a camera's vertical
angle and widens the view to the window's shape (main.c,
set_window_camera_values), cinematics too, so on a 16:9 window a shot
showed a third more across than the Xbox's, under the same bars.

With the setting, the shot is drawn with the picture between the Xbox's
bars across the window, and the bars are only as thick as keep that
picture's shape (cinematic_letterbox_bar_height). On a 16:9 window there
are no bars and the shot is the Xbox's width (its vertical angle three
quarters the stock one's tangent); a 4:3 window is drawn as the Xbox drew
it; 16:10 has bars of a twentieth; a window wider than 16:9 keeps the
picture's height and shows more at its sides, as play does. The view
changes while the scripted camera shows the shot with the letterbox up or
coming up: cinematic_start and camera_control come in the same tick in the
maps' cutscenes, so the change is drawn with the cut to the shot (a cut is
drawn from the tick it comes on, render_interpolation.c), and the player's
own view under the bars (a chapter's title) keeps its own; when the bars go
while the camera stays on the shot, the view widens back as they go. Only
the drawing changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"

#include "cinematic_letterbox.h"

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

/* ---------- constants */

/* the Xbox's window, for whose width a camera's angle is made
(render_camera_get_adjusted_field_of_view_tangent's, 0.75 of it up and down) */
#define CINEMATIC_LETTERBOX_XBOX_WINDOW_ASPECT (4.f / 3.f)
/* the Xbox's picture between the letterbox's bars: that window less two
eighths of its height */
#define CINEMATIC_LETTERBOX_XBOX_PICTURE_ASPECT (16.f / 9.f)
/* the bars' height, each, at their full amount, of the window's */
#define CINEMATIC_LETTERBOX_STOCK_BAR_HEIGHT 0.125f

/* ---------- private code */

/* display.cinematic_framing */
static boolean cinematic_letterbox_enabled(
	void)
{
	static boolean initialized, enabled;
	static unsigned long read_at;
	unsigned long changes = config_changes();

	if (!initialized || read_at != changes)
	{
		enabled = config_boolean("display.cinematic_framing") != 0;
		initialized = TRUE;
		read_at = changes;
	}

	return enabled;
}

/* how far the view is taken to the Xbox's picture between the bars (1, all
of the way): all of it while the scripted camera shows a shot with the
letterbox up or coming up, as far as the bars still are while they go and
the camera stays on the shot, and none for any other view */
static real cinematic_letterbox_xbox_picture_amount(
	void)
{
	if (!cinematic_letterbox_enabled() || !director_camera_scripted || !*director_camera_scripted ||
		!cinematic_globals)
	{
		return 0.f;
	}
	if (cinematic_globals->show_letterbox)
	{
		return 1.f;
	}

	return PIN(cinematic_globals->letterbox_amount, 0.f, 1.f);
}

/* the vertical angle on a window of that width and height that shows the
Xbox's picture between the bars across it, so far of the way to it: its 4:3
window's width on a window 16:9 or narrower, its height on a wider one */
static real cinematic_letterbox_xbox_picture(
	real vertical_field_of_view,
	real width,
	real height,
	real amount)
{
	real scale;

	if (!(height > 0.f) || !(width > 0.f) || !(amount > 0.f) ||
		!(vertical_field_of_view > 0.f && vertical_field_of_view < _pi))
	{
		return vertical_field_of_view;
	}
	scale = CINEMATIC_LETTERBOX_XBOX_WINDOW_ASPECT / MIN(width / height, CINEMATIC_LETTERBOX_XBOX_PICTURE_ASPECT);
	scale = 1.f + (scale - 1.f) * MIN(amount, 1.f);

	return 2.f * arctangent(tangent(0.5f * vertical_field_of_view) * scale, 1.f);
}

/* ---------- public code */

real cinematic_letterbox_vertical_field_of_view(
	rectangle2d const *viewport_bounds,
	real vertical_field_of_view)
{
	if (!viewport_bounds)
	{
		return vertical_field_of_view;
	}

	return cinematic_letterbox_xbox_picture(
		vertical_field_of_view,
		(real)(viewport_bounds->x1 - viewport_bounds->x0),
		(real)(viewport_bounds->y1 - viewport_bounds->y0),
		cinematic_letterbox_xbox_picture_amount());
}

real cinematic_letterbox_bar_height(
	rectangle2d const *viewport_bounds)
{
	real width, height;

	if (!viewport_bounds || !cinematic_letterbox_enabled())
	{
		return CINEMATIC_LETTERBOX_STOCK_BAR_HEIGHT;
	}
	width = (real)(viewport_bounds->x1 - viewport_bounds->x0);
	height = (real)(viewport_bounds->y1 - viewport_bounds->y0);
	if (!(height > 0.f) || !(width > 0.f))
	{
		return CINEMATIC_LETTERBOX_STOCK_BAR_HEIGHT;
	}

	return MAX(0.f, 0.5f * (1.f - (width / height) / CINEMATIC_LETTERBOX_XBOX_PICTURE_ASPECT));
}
