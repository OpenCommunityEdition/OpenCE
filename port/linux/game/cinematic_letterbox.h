/*
CINEMATIC_LETTERBOX.H

display.cinematic_framing: a cutscene's shots with the letterbox framed as
the Xbox framed them, and the bars as thick as keep that picture's shape
(cinematic_letterbox.c).
*/

#ifndef __CINEMATIC_LETTERBOX_H
#define __CINEMATIC_LETTERBOX_H

/* ---------- prototypes */

/* a window's vertical angle (main.c, set_window_camera_values): with
display.cinematic_framing, while the scripted camera shows a shot with the
letterbox, the Xbox's picture between the bars across the window; else as
given */
real cinematic_letterbox_vertical_field_of_view(
	rectangle2d const *viewport_bounds,
	real vertical_field_of_view);

/* the letterbox's bars' height at their full amount, of the viewport's
(cinematics.c, cinematic_render): an eighth, as the Xbox drew them over its
4:3 picture; with display.cinematic_framing, as much as keeps the picture
between them the Xbox's 16:9, none on a window 16:9 or wider */
real cinematic_letterbox_bar_height(
	rectangle2d const *viewport_bounds);

#endif
