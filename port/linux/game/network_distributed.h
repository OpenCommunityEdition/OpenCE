/*
NETWORK_DISTRIBUTED.H

What the distributed netcode's modules share (port/linux/NETCODE.md):
network_distributed.c (the messages, the players, the game's state),
network_objects.c (the game's objects: who has which, where they are, what
units carry) and network_damage.c (damage: the host's, replayed on its
clients, and the clients' hits, reported to the host).
*/

#ifndef __NETWORK_DISTRIBUTED_H
#define __NETWORK_DISTRIBUTED_H
#pragma once

#include "bungie_net/common/message_header.h"
#include "networking/network_connection.h"

/* ---------- constants */

/* the messages, all of the game's "data" kind (network_distributed.c) */
enum
{
	/* a client's own players' units, every tick (unreliable) */
	_distributed_message_player_prediction = 1,
	/* every player's unit, every tick (unreliable) */
	_distributed_message_unit_states,
	/* the players' statistics (unreliable) */
	_distributed_message_player_statistics,
	/* what units carry (unreliable) */
	_distributed_message_inventories,
	/* objects created and deleted (reliable) */
	_distributed_message_object_changes,
	/* where objects are (unreliable) */
	_distributed_message_object_states,
	/* the game type's state (reliable) */
	_distributed_message_game_state,
	/* the host's objects all told (reliable, to one client) */
	_distributed_message_objects_synchronized,
	/* a client has loaded the game: it wants the host's objects (reliable) */
	_distributed_message_client_ready,
	/* damage the host dealt, for its clients' effects (unreliable) */
	_distributed_message_damage_events,
	/* a client's own players' hits (reliable) */
	_distributed_message_hit_reports,
	/* the vehicle a client's own player drives, every tick (unreliable) */
	_distributed_message_vehicle_prediction,
	/* what players picked up, for their clients to show (reliable) */
	_distributed_message_pickups,

	NUMBER_OF_DISTRIBUTED_MESSAGES
};

/* where a message goes */
enum
{
	_distributed_to_clients,
	_distributed_to_clients_reliably,
	_distributed_to_host,
	_distributed_to_host_reliably,
};

enum
{
	/* the objects tracked by index: all of them */
	MAXIMUM_TRACKED_OBJECTS = HALO_PORT_MAXIMUM_OBJECTS_PER_MAP,
	MAXIMUM_TRACKED_PLAYERS = HALO_PORT_MAXIMUM_NETWORK_PLAYERS,
	/* a player index in a byte: none */
	NO_PLAYER = 0xFF,
};

/* ---------- structures */

struct distributed_message_header
{
	message_header header;
	byte type;
	byte count;
	long game_time;
};

/* ---------- macros */

/* the entries of a type that fit one unreliable message */
#define DATAGRAM_ENTRIES(type) \
	((short)((DATAGRAM_MAXIMUM_SIZE - sizeof(struct distributed_message_header)) / sizeof(type)))
/* ... and one reliable message (its count a byte) */
#define RELIABLE_ENTRIES(type) \
	((short)MIN(255, (MAXIMUM_MESSAGE_SIZE - sizeof(struct distributed_message_header)) / sizeof(type)))

/* ---------- prototypes/NETWORK_DISTRIBUTED.C */

/* sends a message (its header filled in here) where destination says */
void distributed_send(void *message, byte type, short count, word size, short destination);
/* ... reliably to one client (the host) */
void distributed_send_to_machine_reliably(long machine_index, void *message, byte type, short count, word size);
/* the player at an absolute index, or NULL */
struct player_datum *distributed_player(short player_index);
/* a player's absolute index for a message, NO_PLAYER for none; and back */
byte distributed_player_to_byte(long player_index);
long distributed_player_from_byte(byte player_index);
/* whether the player is one of this machine's */
boolean distributed_player_is_local(long player_index);
/* the player's living unit, or NONE */
long distributed_living_unit(struct player_datum const *player);
/* whether the player is one of that client machine's (the host) */
boolean distributed_machine_has_player(long machine_index, short player_index);
void distributed_count_sent(void);
void distributed_count_correction(void);

/* ---------- prototypes/NETWORK_OBJECTS.C */

void network_objects_new_game(void);
/* after each tick */
void network_objects_host_tick(void);
void network_objects_client_tick(void);
void network_objects_client_ready(long machine_index);
void network_objects_handle_changes(void const *entries, short count);
void network_objects_handle_synchronized(void);
void network_objects_handle_states(void const *entries, short count);
void network_objects_handle_inventories(void const *entries, short count);
void network_objects_handle_vehicle_prediction(long machine_index, void const *entries, short count);
word network_objects_entry_size(byte type);
/* whether this client has the host's object at this index */
boolean network_objects_client_has(long object_index);
/* moves the object where the host has it, drawn gliding from where it was */
void network_objects_correct(long object_index, real_point3d const *position, real_vector3d const *forward,
	real_vector3d const *up, real_vector3d const *velocity, real_vector3d const *angular_velocity);
/* a unit in the vehicle's seat as the host has it (NONE: in none) */
void network_objects_set_seat(long unit_index, long vehicle_index, short seat_index);

/* ---------- prototypes/NETWORK_DAMAGE.C */

void network_damage_new_game(void);
void network_damage_host_tick(void);
void network_damage_client_tick(void);
void network_damage_handle_events(void const *entries, short count);
void network_damage_handle_reports(long machine_index, void const *entries, short count);
word network_damage_entry_size(byte type);

#endif // __NETWORK_DISTRIBUTED_H
