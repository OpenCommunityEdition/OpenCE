/*
NETWORK_CHAT.H

Text chat in network games (network_chat.c): what the netcode, the host's
server and the screen's overlays ask of it.
*/

#ifndef __NETWORK_CHAT_H
#define __NETWORK_CHAT_H
#pragma once

/* ---------- prototypes/NETWORK_CHAT.C */

/* once a frame, over everything (interface_draw_fullscreen_overlays): the
keys typed, and the chat drawn, in the lobby and in the game */
void network_chat_render(void);
/* whether a message of the distributed kind is text chat's, which it takes
in the lobby too; and its handling (machine_index: the sender's on the host,
NONE on a client; from_stream: it came over that machine's own stream) */
boolean network_chat_handles_message(word const *message, word size);
void network_chat_handle_message(long machine_index, word const *message, word size, boolean from_stream);

#endif // __NETWORK_CHAT_H
