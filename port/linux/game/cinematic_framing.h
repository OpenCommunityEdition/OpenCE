/*
CINEMATIC_FRAMING.H

A cinematic shot drawn no wider than the Xbox drew it (cinematic_framing.c).
*/

#ifndef __CINEMATIC_FRAMING_H
#define __CINEMATIC_FRAMING_H

/* ---------- prototypes */

/* a window's vertical angle (main.c, set_window_camera_values): while the
scripted camera looks from a shot's point that keeps the Xbox's width, on
a window wider than 4:3, narrowed to show a 4:3 window's width; else as
given */
real cinematic_framing_vertical_field_of_view(
	rectangle2d const *viewport_bounds,
	real vertical_field_of_view);

#endif
