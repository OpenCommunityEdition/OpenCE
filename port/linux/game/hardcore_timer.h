#ifndef HARDCORE_TIMER_H
#define HARDCORE_TIMER_H

/* A hardcore match's clock, and the globals a map announcer waits on.
   Classic games are left alone. */

/* After the map's scripts have run for this tick. The startup script turns
   the announcer off, and the clock script sleeps for good if that stands. */
void hardcore_timer_after_scripts(void);

/* The match clock, bottom right, as one time. Nothing is drawn over a menu
   or outside a hardcore match. */
void hardcore_timer_render(void);

/* The top of the clock's digits, in the 480-tall screen. The random
   percent uses it so the two sit on one line. */
short hardcore_timer_top(void);

#endif
