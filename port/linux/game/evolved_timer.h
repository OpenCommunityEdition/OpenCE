#ifndef EVOLVED_TIMER_H
#define EVOLVED_TIMER_H

/* An evolved match's clock, and the globals a map announcer waits on.
   Classic games are left alone. */

/* After the map's scripts have run for this tick. The startup script turns
   the announcer off, and the clock script sleeps for good if that stands. */
void evolved_timer_after_scripts(void);

/* The match clock, bottom right, as one time. Nothing is drawn over a menu
   or outside an evolved match. */
void evolved_timer_render(void);

/* The top of the clock's digits, in the 480-tall screen. The random
   percent uses it so the two sit on one line. */
short evolved_timer_top(void);

#endif
