/*
ROUND_CLOCK.H

The clock of the game running, told to the players (round_clock.c).
*/

#ifndef __ROUND_CLOCK_H
#define __ROUND_CLOCK_H

/* every frame (main.c): the minutes a game has left and its last seconds, as
notices to this machine's players (game.time_notices) */
void round_clock_update(
	boolean main_menu_loaded,
	real seconds);

#endif // __ROUND_CLOCK_H
