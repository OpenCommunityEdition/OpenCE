/*
DUALSENSE_GAME.H

What each local player wants on a DualSense, from the game (the platform side, port/linux/src/dualsense.c, sends it;
input.adaptive_triggers, input.trigger_strength and input.lightbar). Plain types only.
*/

#ifndef __DUALSENSE_GAME_H
#define __DUALSENSE_GAME_H

/* Called after input updates, and at map/checkpoint boundaries. */
void dualsense_game_update(void);
void dualsense_game_reset(void);

#endif
