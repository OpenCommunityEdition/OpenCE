/*
ARMOR_SYNC.H

Each machine's Armor choices in network games (armor_sync.c): what the
netcode, the screen's overlays, spartan_colors.c, shield_color.c and
visor_color.c ask of it.
*/

#ifndef __ARMOR_SYNC_H
#define __ARMOR_SYNC_H
#pragma once

enum
{
	_armor_sync_armor,
	_armor_sync_lights,
	_armor_sync_flashlight,
	_armor_sync_shield,
	_armor_sync_visor,
	NUMBER_OF_ARMOR_SYNC_CHOICES
};

/* once a frame (interface_draw_fullscreen_overlays): this machine's choices
sent to the host, and the host's table to every machine */
void armor_sync_network_update(void);
/* whether a message of the distributed kind is the armors', which it takes
in the lobby too; and its handling (machine_index: the sender's on the host,
NONE on a client; from_stream: it came over that machine's own stream) */
boolean armor_sync_handles_message(word const *message, word size);
void armor_sync_handle_message(long machine_index, word const *message, word size, boolean from_stream);
/* in a network game */
boolean armor_sync_network_active(void);
/* a machine's choices as the host's table has them (NONE each: the game's
own); FALSE if not heard */
boolean armor_sync_of_machine(long machine_index, short choices[NUMBER_OF_ARMOR_SYNC_CHOICES]);

#endif // __ARMOR_SYNC_H
