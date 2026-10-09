/*
NETWORK_VOICE.H

Voice chat (network_voice.c): what the main loop, the netcode, the host's
server, the lobby (menu_functions.c) and the scoreboard (game_engine.c) ask
of it.
*/

#ifndef __NETWORK_VOICE_H
#define __NETWORK_VOICE_H
#pragma once

/* ---------- constants */

/* network.voice_mode: who hears whom in a game */
enum
{
	_voice_mode_off,
	_voice_mode_team_proximity,
	_voice_mode_team_enemy_proximity,
	_voice_mode_team_global,
	_voice_mode_team_global_enemy_proximity,
	NUMBER_OF_VOICE_MODES
};

/* how a listener hears a speaker */
enum
{
	_voice_route_none,
	/* as spoken */
	_voice_route_global,
	/* quieter further, from the speaker's side */
	_voice_route_proximity,
};

/* ---------- prototypes/NETWORK_VOICE.C */

/* every frame (main.c): the microphone, the host's settings told */
void network_voice_update(void);
/* whether a message of the distributed kind is voice chat's, which it takes
in the lobby too; and its handling (machine_index: the sender's on the
host, NONE on a client) */
boolean network_voice_handles_message(word const *message, word size);
void network_voice_handle_message(long machine_index, word const *message, word size);
/* (the host's server) a machine joined at the slot: its new key */
void network_voice_machine_joined(long machine_index);
/* whether this machine has voice chat now (the host's settings heard) */
boolean network_voice_available(void);
/* whether a machine (its index in the network game) is talking now (this
machine's own, as it sends); muted ones never */
boolean network_voice_machine_speaking(long machine_index);
/* this machine's mutes of others (which it tells no one) */
boolean network_voice_machine_muted(long machine_index);
void network_voice_mute_machine(long machine_index, boolean mute);
/* a speaker icon, at the rectangle's left and as tall as it (muted: struck
through), fading with alpha */
void network_voice_draw_icon(rectangle2d const *bounds, boolean muted, real alpha);

#endif // __NETWORK_VOICE_H
