/*
NETWORK_CHAT.H

The text chat of network games (network_chat.c): what the main loop, the
HUD (hud.c, hud_unit.c), the keyboard (xinput_sdl.c) and the netcode ask of
it.
*/

#ifndef __NETWORK_CHAT_H
#define __NETWORK_CHAT_H
#pragma once

/* ---------- constants */

/* who a line is to */
enum
{
	_chat_channel_everyone,
	_chat_channel_team,
	/* those riding in the sender's vehicle */
	_chat_channel_vehicle,
	NUMBER_OF_CHAT_CHANNELS
};

/* ---------- prototypes/NETWORK_CHAT.C */

/* every frame (main.c): the keys that start typing a line, and those typed */
void network_chat_update(void);
/* the first local player says a line to a channel, as if typed and sent;
the automated tests' (network_test.c) */
void network_chat_say(short channel, char const *text);
/* ... sent past this machine's own checks of the host's settings, as a
changed build could send it, for the host to refuse (network_test.c) */
void network_chat_say_unchecked(short channel, char const *text);
/* whether a line is being typed: the keyboard types into it, and drives no
player (xinput_sdl.c), as with the console */
boolean network_chat_typing(void);
/* (hud_unit.c) where the local player's motion sensor was drawn: its left
and the top of its circle, in the window's units */
void network_chat_note_motion_sensor(short local_player_index, short left, short top);
/* (hud.c, in the HUD of each local player's view) the lines that view's
player hears, above the motion sensor (with the touch controls, under the
weapon's panel at the top left), and in the first local player's the one
being typed */
void network_chat_draw(short local_player_index);
/* (the host) the minutes a player (absolute index) is muted for everyone;
0 for none */
short network_chat_host_mute_minutes(short player_index);
/* (the host: the scoreboard) a player (absolute index) muted for everyone,
their lines and their voice, for /mute's minutes when it gives none (every
machine told), or unmuted */
void network_chat_host_mute(short player_index, boolean mute);
/* (the host: network_voice.c) whether a player's name (the network game's,
in the lobby too; at most 12 characters, ended or not) is muted for everyone:
their machine's voice goes no further */
boolean network_chat_host_name_muted(wchar_t const *name);

#endif // __NETWORK_CHAT_H
