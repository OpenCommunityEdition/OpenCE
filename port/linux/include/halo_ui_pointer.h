/*
HALO_UI_POINTER.H

The mouse in the menus of the desktop builds, and the touchscreen in those
of the Android build. While a menu is up, the mouse is released and its
pointer shows (port/linux/src/sdl_platform.c), or taps and drags go to the
menus (port/linux/src/touch_input.c); each frame the menus
(source/interface/ui_widget.c) ask where the pointer is, in their own 640x480
coordinates (port/linux/src/d3d8_gl.c undoes the letterbox, the scale and the
widescreen centering), and what it did since.
*/

#ifndef HALO_UI_POINTER_H
#define HALO_UI_POINTER_H

struct halo_ui_pointer
{
	short x, y;
	/* where the latest left click was */
	short click_x, click_y;
	unsigned char moved;
	unsigned char left_clicks;
	unsigned char right_clicks;
	/* whole wheel notches, away from the user positive */
	signed char wheel_steps;
	/* nonzero when the pointer is the touchscreen, not a mouse: a drag and a
	wheel notch are the same steps, but the menus treat them differently */
	unsigned char touch;
	/* fingers down since the last read and where the latest went down; only
	the touchscreen reports them (debug.touch_targets shows them) */
	unsigned char downs;
	short down_x, down_y;
};

/* gives the menus the pointer; frees the mouse for the menus while
menus_active and captures it for aiming when not (touchscreen: taps and
drags go to the menus while menus_active); returns nonzero while menus are
active and the pointer could be read */
int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer);

/* the open scoreboard's pointer (game_engine.c), offered (a network game's)
or not: 1 while a right click has freed it, with where it is and what it
did since the last call, in the screen's coordinates (the game's drawing,
not the menus' centered 640); 0 while it is not; -1 where there is none
(Android) */
int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer);

#endif
