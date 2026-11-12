/*
TOUCH_CINEMATIC.H

The skip button on the Android build's touchscreen during a cinematic
(touch_cinematic.c).
*/

#ifndef __TOUCH_CINEMATIC_H
#define __TOUCH_CINEMATIC_H

/* (interface.c, the screen's overlays, after the letterbox) during a
cinematic, the button, where it shows and the cinematic can be skipped, or
voted to skip */
void touch_cinematic_draw(void);
/* (coop_spectate.c) whether the button shows where a cinematic can be
skipped, for the line of the network co-op vote */
boolean touch_cinematic_button_enabled(void);
/* (port/linux/src/touch_input.c, which declares it itself) a tap during a
skippable cinematic at a place of the window, 0 to 1 across and down:
whether it presses A, on the button while it shows, anywhere while not */
int touch_cinematic_tap(float x, float y);

#endif
