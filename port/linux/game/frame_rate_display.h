/*
FRAME_RATE_DISPLAY.H

A game's frame rate counter at the screen's bottom right
(display.show_fps: frame_rate_display.c).
*/

#ifndef __FRAME_RATE_DISPLAY_H
#define __FRAME_RATE_DISPLAY_H

/* (before interface.c's overlays draw the counter) shows or hides the
game's counter as display.show_fps says */
void frame_rate_display_update(void);

#endif
