/*
NETWORK_CHAT.H

The text chat of network games (network_chat.c): what the main loop, the
HUD (hud.c, hud_unit.c), the keyboard (xinput_sdl.c) and the netcode ask of
it.
*/

#ifndef __NETWORK_CHAT_H
#define __NETWORK_CHAT_H
#pragma once

/* ---------- prototypes/NETWORK_CHAT.C */

/* every frame (main.c): the keys that start typing a line, and those typed */
void network_chat_update(void);
/* the first local player says a line, as if typed and sent (to the team:
team); the automated tests' (network_test.c) */
void network_chat_say(boolean team, char const *text);
/* whether a line is being typed: the keyboard types into it, and drives no
player (xinput_sdl.c), as with the console */
boolean network_chat_typing(void);
/* (hud_unit.c) where the local player's motion sensor was drawn: its left
and the top of its circle, in the window's units */
void network_chat_note_motion_sensor(short local_player_index, short left, short top);
/* (hud.c, in the HUD of the first local player's view) the lines, above the
motion sensor, and the one being typed */
void network_chat_draw(short local_player_index);

#endif // __NETWORK_CHAT_H
