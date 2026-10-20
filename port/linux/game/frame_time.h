/*
FRAME_TIME.H

The frame-time overlay (frame_time.c).
*/

#ifndef FRAME_TIME_H
#define FRAME_TIME_H

/* display.frame_time: the time a frame takes and the frames a second, drawn
in the top right corner of the screen once every window is drawn; nothing
when the setting is off */
void frame_time_draw(void);

#endif
