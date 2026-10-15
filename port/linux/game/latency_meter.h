/*
LATENCY_METER.H

A network game's round trip to the host, at the screen's top right while
the game is played (display.latency_meter: latency_meter.c).
*/

#ifndef __LATENCY_METER_H
#define __LATENCY_METER_H

/* display.latency_meter */
boolean latency_meter_enabled(void);
/* ... and a network game being played */
boolean latency_meter_shown(void);
/* a round trip's colour (green, yellow or red), and the bars it earns (4
to 1) */
short latency_meter_color(long milliseconds, real_argb_color *color);
/* (interface.c's overlays, over the whole screen) */
void latency_meter_draw(void);

#endif
