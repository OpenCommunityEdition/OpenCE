/*
DUALSENSE_GAME.H

What each local player wants on a DualSense, from the game (the platform side, port/linux/src/dualsense.c, sends it;
input.adaptive_triggers, input.trigger_strength and input.lightbar). Plain types only.
*/

#ifndef __DUALSENSE_GAME_H
#define __DUALSENSE_GAME_H

/* (player_control.c handle_one_player_input, after the zoom level is set, for a player with a unit) the weapon object
in hand (NONE: none) and the zoom level (NONE: not zoomed) */
void dualsense_player(short local_player_index, long weapon_index, short zoom_level);

#endif
