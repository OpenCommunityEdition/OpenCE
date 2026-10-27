/*
TOUCH_POSTGAME.H

The carnage report's buttons on the Android build's touchscreen, in place
of its prompt while the touch controls would show (touch_postgame.c).
*/

#ifndef __TOUCH_POSTGAME_H
#define __TOUCH_POSTGAME_H

/* (game_engine.c's carnage report) draws the buttons in the font and colour
of the prompt they replace, CONTINUE for the host and LEAVE; returns FALSE,
having drawn nothing, where the prompt is drawn instead */
boolean touch_postgame_draw(long font_index, boolean host, real_argb_color const *color);
/* (ui_widget.c) a click at a point of the report's 640x480 screen; returns
whether it was on a button, which is then pressed */
boolean touch_postgame_click(short x, short y);
/* (game_engine.c) whether the button for the gamepad's button (A or B) was
pressed since it was last asked, forgetting the press */
boolean touch_postgame_pressed(short button_index);

#endif
