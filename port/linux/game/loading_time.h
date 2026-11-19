/*
LOADING_TIME.H

The least time a level's loading screen stays up, with its sweep paced to
that time (display.loading_seconds: loading_time.c).
*/

#ifndef __LOADING_TIME_H
#define __LOADING_TIME_H

/* (game.c, as a level's loading screen starts) reads the setting for this
load: none in a network game */
void loading_time_begin(void);

/* (progress_bar.c) the progress the loading screen shows: the load's, but
no further than the time this load is held for has gone */
real loading_time_progress(real progress);

/* (game.c, once the level is ready) whether the loading screen stays up */
boolean loading_time_holding(void);

/* (game.c, as the loading screen ends) */
void loading_time_end(void);

#endif
