/*
VISOR_COLOR.H

The visor's color (visor_color.c): what the netcode, the host's server and
the screen's overlays ask of it in network games.
*/

#ifndef __VISOR_COLOR_H
#define __VISOR_COLOR_H
#pragma once

/* once a frame (interface_draw_fullscreen_overlays): this machine's color
sent to the host, and the host's table to every machine */
void visor_color_network_update(void);
/* whether a message of the distributed kind is the visors' colors', which
it takes in the lobby too; and its handling (machine_index: the sender's on
the host, NONE on a client; from_stream: it came over that machine's own
stream) */
boolean visor_color_handles_message(word const *message, word size);
void visor_color_handle_message(long machine_index, word const *message, word size, boolean from_stream);

#endif // __VISOR_COLOR_H
