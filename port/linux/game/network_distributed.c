/*
NETWORK_DISTRIBUTED.C

The distributed netcode's own messages (port/linux/NETCODE.md): the game's
"data" message kind (message header type 2, which the Xbox game never sent),
beside its packets, and handled here (network_*_message_handler.c).

The host decides; its clients predict their own players and show the
rest as the host has it:

- The game's objects (units, vehicles, weapons, equipment) are the host's,
  at the same datum index on every machine (network_objects.c): the host
  says which it has, where they are and what units carry.
- Every tick, a client sends the host its own players' input, each tick's
  buttons again with the next three ticks' (a press is lost only with four
  datagrams in a row), and the host takes each tick's buttons once. The
  host sends every client every player's input as its tick ran it, the
  same way, and the clients drive the other players with it
  (player_queues_new.c).
- Every tick, a client sends the host where its own players' units are (it
  predicts them from its own input); the host takes that as they are,
  within a tolerance, as later Halo engines do, at its next tick.
- Every tick, the host sends every client every player's unit: which unit
  the player has, alive or not, the seat it rides, its shields and health
  (down, recharging, the damage they show), and where it is (dead: who
  killed it, which a client announces when its copy dies). A client binds,
  kills, seats and places its copies to match (it decides no deaths or
  spawns itself), and its own only when far off (a respawn, a teleport).
  Players far from a client's own are sent to it less often.
- Twice a second, and with every kill, the host sends every player's
  statistics (kills, deaths, ...), which clients take as they are.
- Five times a second, the host sends the game type's state (the scores,
  the flags, the balls and the hill), which clients take as it is
  (game_engine_write_network_state).
- Damage is the host's: a client reports its own players' hits, which the
  host checks and deals, and replays the damage the host deals for its
  effects (network_damage.c).

The unreliable messages of a tick to a machine go in one datagram (a batch),
and each carries its tick (its header's game time): one that arrives after
a newer of its kind is dropped. Vectors travel in 16 bits a part.

Players are named by their absolute index, which is the same on every
machine (their datum identifiers need not be).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "game/player_queues_new.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "units/units.h"
#include "network_distributed.h"

#include <math.h>

/* network_game_globals.c's and network_server_message_handler.c's */
boolean network_distributed_client_send(void *message, word size);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_all_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine(long machine_index, void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
/* players.c's */
void network_player_attach_unit(long player_index, long unit_index);
void network_player_detach_unit(long player_index);
void network_player_show_pickup(long player_index, short kind, long definition_index, short count);
/* game_engine.c's */
long game_engine_write_network_state(byte *buffer, long size);
void game_engine_read_network_state(byte const *buffer, long size);

enum
{
	STATISTICS_INTERVAL_TICKS = 15,
	GAME_STATE_INTERVAL_TICKS = 6,
	MAXIMUM_GAME_STATE_SIZE = 0xF00,
	MAXIMUM_UNIT_STATES_PER_MESSAGE = 64,
	MAXIMUM_STATISTICS_PER_MESSAGE = 64,
	MAXIMUM_PICKUPS_PER_TICK = 64,
	/* ticks a client's own player may ride where the host says it does not
	(or the other way round) before it is put where the host has it: its
	own prediction reaches the host and comes back in about a round trip */
	SEAT_DISAGREEMENT_TICKS = 15,
	/* the machines, and the host: a client's messages' sender */
	MAXIMUM_SENDERS = HALO_PORT_MAXIMUM_NETWORK_MACHINES + 1,
	HOST_SENDER = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* a round trip before one is measured, and the longest taken (ticks) */
	DEFAULT_ROUND_TRIP_TICKS = 6,
	MAXIMUM_ROUND_TRIP_TICKS = 60,
};

/* struct distributed_unit_state flags */
enum
{
	/* the player has a unit that is alive */
	_distributed_unit_alive_bit = 0,
	/* ... and not riding (its position is its own) */
	_distributed_unit_placed_bit,
	/* (dead) how it died: killed by a teammate, or by an empty vehicle */
	_distributed_unit_friendly_fire_bit,
	_distributed_unit_killed_by_vehicle_bit,
	/* (alive) its shields: down, charging, overcharging */
	_distributed_unit_shield_depleted_bit,
	_distributed_unit_shield_charging_bit,
	_distributed_unit_shield_over_charging_bit,
};

/* struct distributed_unit_state unit flags */
enum
{
	/* (alive) camouflaged, and doubly so */
	_distributed_unit_camouflaged_bit = 0,
	_distributed_unit_super_camouflaged_bit,
};

/* world units: how far a client's own player's unit may be from the host's
before the host takes it no longer, and before the client is put where the
host has it (no further than that: between the two they would disagree for
good, the host's player somewhere its own is not) */
#define HOST_ACCEPT_TOLERANCE 3.5f
#define REMOTE_CORRECTION_TOLERANCE 0.05f
#define LOCAL_CORRECTION_TOLERANCE 3.0f

/* how far players are from a client's own (world units) before the host
sends them to it every second tick, and every third */
#define NEAR_PLAYER_DISTANCE 25.0f
#define FAR_PLAYER_DISTANCE 60.0f

/* shields, health and the damage they show in 16 bits: 0 to 4 */
#define VITALITY_SCALE 16384.0f

struct distributed_unit_state
{
	byte player_index;
	byte flags;
	/* (dead) the player who killed it, NO_PLAYER for none */
	byte killing_player_index;
	byte unit_flags;
	/* the player's unit (the host's), NONE for none */
	long unit_index;
	/* the vehicle it rides and its seat, NONE for none */
	long vehicle_index;
	short seat_index;
	/* how camouflaged the unit is, of 255 */
	byte active_camouflage;
	byte pad;
	real_point3d position;
	struct distributed_vector velocity;
	struct distributed_vector forward;
	struct distributed_vector up;
	short pad1;
	/* (VITALITY_SCALE) */
	word body_vitality;
	word shield_vitality;
	/* what the shields' and the HUD's effects show */
	word current_body_damage;
	word recent_body_damage;
	word current_shield_damage;
	word recent_shield_damage;
	/* the player's powerups: how long each has left */
	short powerup_durations[NUMBER_OF_PLAYER_POWERUPS];
};

/* a client's player's input at one of its ticks, with the buttons of the
ticks before it */
struct distributed_player_input
{
	byte player_index;
	byte pad[3];
	/* the client's tick */
	long tick;
	/* the latest host tick the client has had a message of (the host
	measures the round trip by it) */
	long host_time;
	struct player_action action;
	unsigned short control_flags[DISTRIBUTED_INPUT_HISTORY];
};

/* a player's input as the host ran it, in fewer bytes */
struct distributed_relayed_action
{
	byte player_index;
	signed char desired_weapon_index;
	signed char desired_grenade_index;
	signed char desired_zoom_level;
	/* the host's update */
	long update_number;
	unsigned short control_flags[DISTRIBUTED_INPUT_HISTORY];
	/* of a turn */
	short yaw;
	short pitch;
	/* of 127, and 255 */
	signed char throttle_i;
	signed char throttle_j;
	byte primary_trigger;
	byte pad;
};

struct distributed_pickup
{
	byte player_index;
	byte kind;
	short count;
	long definition_index;
};

struct distributed_player_statistics
{
	short player_index;
	short pad;
	struct game_statistics statistics;
};

struct distributed_unit_state_message
{
	struct distributed_message_header header;
	struct distributed_unit_state states[MAXIMUM_UNIT_STATES_PER_MESSAGE];
};

struct distributed_statistics_message
{
	struct distributed_message_header header;
	struct distributed_player_statistics players[MAXIMUM_STATISTICS_PER_MESSAGE];
};

/* a batch's datagram: its header, then each message's size and its bytes
past its message header */
struct distributed_batch
{
	word size;
	byte data[DATAGRAM_MAXIMUM_SIZE];
};

/* ---------- globals */

static long distributed_last_sent_time = NONE;

/* how each player last died, by absolute index: the host's own, which it
sends its clients, and a client's copy of the host's */
static struct distributed_death
{
	boolean valid;
	/* the killer's absolute index, or NONE */
	short killing_player_index;
	boolean friendly_fire;
	boolean killed_by_vehicle;
} distributed_deaths[MAXIMUM_TRACKED_PLAYERS];
/* the host: a kill this tick, whose statistics the clients should have
with it */
static boolean distributed_statistics_due;
/* the host: what players on other machines picked up this tick */
static struct distributed_pickup distributed_pickups[MAXIMUM_PICKUPS_PER_TICK];
static short distributed_pickup_count;
/* a client: the ticks each of its own players has ridden other than as
the host has it */
static short distributed_seat_disagreements[MAXIMUM_TRACKED_PLAYERS];

/* the host: each client's player's latest prediction, taken at the next
tick */
static struct
{
	boolean valid;
	struct distributed_unit_state state;
} distributed_predictions[MAXIMUM_TRACKED_PLAYERS];
/* the host: what each player's unit was last sent as (a change goes to
every client at once) */
static struct
{
	byte flags;
	long unit_index;
	long vehicle_index;
	short seat_index;
} distributed_sent_units[MAXIMUM_TRACKED_PLAYERS];

/* the latest tick of each kind of unreliable message had from each sender
(a machine, or the host), NONE for none */
static long distributed_received_times[MAXIMUM_SENDERS][NUMBER_OF_DISTRIBUTED_MESSAGES];
/* a client: the host's latest tick it has had a message of */
static long distributed_host_time = NONE;
/* the host: each client's round trip, in ticks, and its jitter */
static struct
{
	boolean valid;
	real average;
	real deviation;
} distributed_round_trips[HALO_PORT_MAXIMUM_NETWORK_MACHINES];

/* the unreliable messages of this tick, a batch for each machine and one
for the host */
static struct distributed_batch distributed_batches[MAXIMUM_SENDERS];

/* for the automated tests' reports (network_test.c) */
static struct
{
	long sent;
	long received;
	long corrections;
} distributed_statistics;

/* ---------- shared (network_distributed.h) */

void network_distributed_statistics(
	long *sent,
	long *received,
	long *corrections)
{
	*sent = distributed_statistics.sent;
	*received = distributed_statistics.received;
	*corrections = distributed_statistics.corrections;
}

void distributed_count_sent(
	void)
{
	distributed_statistics.sent++;
}

void distributed_count_correction(
	void)
{
	distributed_statistics.corrections++;
}

void distributed_vector_pack(
	real_vector3d const *vector,
	real scale,
	struct distributed_vector *result)
{
	real parts[3];
	short index;

	parts[0] = vector->i;
	parts[1] = vector->j;
	parts[2] = vector->k;
	for (index = 0; index < 3; index++)
	{
		real value = parts[index] * scale;

		value = value > 32767.0f ? 32767.0f : value < -32767.0f ? -32767.0f : value;
		parts[index] = (real)floor(value + 0.5f);
	}
	result->i = (short)parts[0];
	result->j = (short)parts[1];
	result->k = (short)parts[2];
}

void distributed_vector_unpack(
	struct distributed_vector const *vector,
	real scale,
	real_vector3d *result)
{
	result->i = (real)vector->i / scale;
	result->j = (real)vector->j / scale;
	result->k = (real)vector->k / scale;
}

void distributed_unit_vector_unpack(
	struct distributed_vector const *vector,
	real_vector3d *result)
{
	real length;

	distributed_vector_unpack(vector, DISTRIBUTED_UNIT_SCALE, result);
	length = (real)sqrt(result->i * result->i + result->j * result->j + result->k * result->k);
	if (length > 0.0f)
	{
		result->i /= length;
		result->j /= length;
		result->k /= length;
	}
}

static word distributed_vitality_pack(
	real value)
{
	value *= VITALITY_SCALE;
	value = value > 65535.0f ? 65535.0f : value < 0.0f ? 0.0f : value;
	return (word)(long)floor(value + 0.5f);
}

static real distributed_vitality_unpack(
	word value)
{
	return (real)value / VITALITY_SCALE;
}

/* an angle as a 16-bit fraction of a turn, and back (yaw from 0 to 2 pi,
pitch from -pi to pi) */
static short distributed_angle_pack(
	real angle)
{
	real turns = angle / (2.0f * _pi);

	turns -= (real)floor(turns);
	return (short)(word)((long)floor(turns * 65536.0f + 0.5f) & 0xFFFF);
}

static real distributed_angle_unpack(
	short value,
	boolean signed_angle)
{
	real angle = (real)(word)value * (2.0f * _pi) / 65536.0f;

	if (signed_angle && angle >= _pi)
		angle -= 2.0f * _pi;
	return angle;
}

static void distributed_batch_flush(short sender);

/* where a message's bytes past its message header go: a machine's batch,
sent at the tick's end (a message too large for one goes alone) */
static void distributed_batch_add(
	short sender,
	void const *message,
	word size)
{
	struct distributed_batch *batch = &distributed_batches[sender];
	word length = (word)(size - sizeof(message_header));

	if (sizeof(struct distributed_message_header) + sizeof(word) + length > DATAGRAM_MAXIMUM_SIZE)
	{
		byte buffer[0x1000];

		if (size > sizeof(buffer))
			return;
		csmemcpy(buffer, message, size);
		if (sender == HOST_SENDER)
			network_distributed_client_send(buffer, size);
		else
			network_distributed_server_send_to_machine(sender, buffer, size);
		return;
	}
	if (batch->size && batch->size + sizeof(word) + length > DATAGRAM_MAXIMUM_SIZE)
		distributed_batch_flush(sender);
	if (!batch->size)
		batch->size = sizeof(struct distributed_message_header);
	csmemcpy(batch->data + batch->size, &length, sizeof(word));
	csmemcpy(batch->data + batch->size + sizeof(word), (byte const *)message + sizeof(message_header), length);
	batch->size += (word)(sizeof(word) + length);
}

static void distributed_batch_flush(
	short sender)
{
	struct distributed_batch *batch = &distributed_batches[sender];
	struct distributed_message_header *header = (struct distributed_message_header *)batch->data;

	if (!batch->size)
		return;
	header->type = _distributed_message_batch;
	header->count = 0;
	header->game_time = game_time_get();
	header->header = 0;
	build_message_header(&header->header, batch->size, 2, 0);
	if (sender == HOST_SENDER)
		network_distributed_client_send(batch->data, batch->size);
	else
		network_distributed_server_send_to_machine(sender, batch->data, batch->size);
	batch->size = 0;
}

static void distributed_batches_flush(
	void)
{
	short sender;

	for (sender = 0; sender < MAXIMUM_SENDERS; sender++)
		distributed_batch_flush(sender);
}

static void distributed_fill_header(
	void *message,
	byte type,
	short count,
	word size)
{
	struct distributed_message_header *header = (struct distributed_message_header *)message;

	header->type = type;
	header->count = (byte)count;
	header->game_time = game_time_get();
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	distributed_statistics.sent++;
}

void distributed_send(
	void *message,
	byte type,
	short count,
	word size,
	short destination)
{
	distributed_fill_header(message, type, count, size);
	switch (destination)
	{
	case _distributed_to_clients:
	{
		long machine_indices[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
		short machine_count = distributed_client_machines(machine_indices, HALO_PORT_MAXIMUM_NETWORK_MACHINES);
		short index;

		for (index = 0; index < machine_count; index++)
			distributed_batch_add((short)machine_indices[index], message, size);
		break;
	}
	case _distributed_to_clients_reliably: network_distributed_server_send_to_all_reliably(message, size); break;
	case _distributed_to_host: distributed_batch_add(HOST_SENDER, message, size); break;
	case _distributed_to_host_reliably: network_distributed_client_send_reliably(message, size); break;
	}
}

void distributed_send_to_machine(
	long machine_index,
	void *message,
	byte type,
	short count,
	word size)
{
	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return;
	distributed_fill_header(message, type, count, size);
	distributed_batch_add((short)machine_index, message, size);
}

void distributed_send_to_machine_reliably(
	long machine_index,
	void *message,
	byte type,
	short count,
	word size)
{
	distributed_fill_header(message, type, count, size);
	network_distributed_server_send_to_machine_reliably(machine_index, message, size);
}

struct player_datum *distributed_player(
	short player_index)
{
	struct player_datum *player;

	if (player_index < 0 || player_index >= player_data->maximum_count)
		return NULL;
	player = (struct player_datum *)((byte *)player_data->data + player_index * player_data->size);
	return player->identifier ? player : NULL;
}

byte distributed_player_to_byte(
	long player_index)
{
	return player_index != NONE ? (byte)DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index) : NO_PLAYER;
}

long distributed_player_from_byte(
	byte player_index)
{
	struct player_datum *player = player_index != NO_PLAYER ? distributed_player(player_index) : NULL;

	return player ? DATUM_INDEX_NEW(player_index, player->identifier) : NONE;
}

boolean distributed_player_is_local(
	long player_index)
{
	struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;

	return player && player->local_player_index != NONE;
}

long distributed_living_unit(
	struct player_datum const *player)
{
	if (!player || player->unit_index == NONE || !object_try_and_get(player->unit_index) ||
		TEST_FLAG(object_get(player->unit_index)->object.damage_flags, _object_dead_bit))
	{
		return NONE;
	}
	return player->unit_index;
}

boolean distributed_machine_has_player(
	long machine_index,
	short player_index)
{
	long *player_list = machine_get_player_list(machine_index);
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		if (player_list[local_player_index] != NONE &&
			DATUM_INDEX_TO_ABSOLUTE_INDEX(player_list[local_player_index]) == player_index)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* whether the machine's players are this machine's (the host's own machine
is in the game as its clients are) */
static boolean distributed_machine_is_local(
	long machine_index)
{
	long *player_list = machine_get_player_list(machine_index);
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		struct player_datum *player = player_list[local_player_index] != NONE ?
			player_try_and_get(player_list[local_player_index]) : NULL;

		if (player && player->local_player_index != NONE)
			return TRUE;
	}
	return FALSE;
}

short distributed_client_machines(
	long *machine_indices,
	short maximum)
{
	short count = network_distributed_server_machines(machine_indices, maximum);
	short index;
	short kept = 0;

	for (index = 0; index < count; index++)
	{
		if (machine_indices[index] >= 0 && machine_indices[index] < HALO_PORT_MAXIMUM_NETWORK_MACHINES &&
			!distributed_machine_is_local(machine_indices[index]))
		{
			machine_indices[kept++] = machine_indices[index];
		}
	}
	return kept;
}

real distributed_machine_round_trip_ticks(
	long machine_index)
{
	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES ||
		!distributed_round_trips[machine_index].valid)
	{
		return (real)DEFAULT_ROUND_TRIP_TICKS;
	}
	return distributed_round_trips[machine_index].average + 2.0f * distributed_round_trips[machine_index].deviation;
}

/* ---------- units */

static void distributed_state_from_player(
	short player_index,
	struct distributed_unit_state *state)
{
	struct player_datum *player = distributed_player(player_index);
	long unit_index = distributed_living_unit(player);

	csmemset(state, 0, sizeof(*state));
	state->player_index = (byte)player_index;
	state->unit_index = NONE;
	state->vehicle_index = NONE;
	state->seat_index = NONE;
	if (unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(unit_index);
		struct damage_network_state damage;
		real camouflage = unit->unit.active_camouflage;

		state->unit_index = unit_index;
		SET_FLAG(state->flags, _distributed_unit_alive_bit, TRUE);
		SET_FLAG(state->flags, _distributed_unit_placed_bit, unit->object.parent_object_index == NONE);
		if (unit->object.parent_object_index != NONE && unit->unit.parent_seat_index != NONE)
		{
			state->vehicle_index = unit->object.parent_object_index;
			state->seat_index = unit->unit.parent_seat_index;
		}
		state->position = unit->object.position;
		distributed_vector_pack(&unit->object.translational_velocity, DISTRIBUTED_VELOCITY_SCALE, &state->velocity);
		distributed_vector_pack(&unit->object.forward, DISTRIBUTED_UNIT_SCALE, &state->forward);
		distributed_vector_pack(&unit->object.up, DISTRIBUTED_UNIT_SCALE, &state->up);
		damage_get_network_state(unit_index, &damage);
		SET_FLAG(state->flags, _distributed_unit_shield_depleted_bit, damage.shield_depleted);
		SET_FLAG(state->flags, _distributed_unit_shield_charging_bit, damage.shield_charging);
		SET_FLAG(state->flags, _distributed_unit_shield_over_charging_bit, damage.shield_over_charging);
		state->body_vitality = distributed_vitality_pack(damage.body_vitality);
		state->shield_vitality = distributed_vitality_pack(damage.shield_vitality);
		state->current_body_damage = distributed_vitality_pack(damage.current_body_damage);
		state->recent_body_damage = distributed_vitality_pack(damage.recent_body_damage);
		state->current_shield_damage = distributed_vitality_pack(damage.current_shield_damage);
		state->recent_shield_damage = distributed_vitality_pack(damage.recent_shield_damage);
		csmemcpy(state->powerup_durations, player->powerup_durations, sizeof(state->powerup_durations));
		SET_FLAG(state->unit_flags, _distributed_unit_camouflaged_bit,
			TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit));
		SET_FLAG(state->unit_flags, _distributed_unit_super_camouflaged_bit,
			TEST_FLAG(unit->unit.flags, _unit_super_camouflaged_bit));
		camouflage = camouflage > 1.0f ? 1.0f : camouflage < 0.0f ? 0.0f : camouflage;
		state->active_camouflage = (byte)(long)floor(camouflage * 255.0f + 0.5f);
	}
	state->killing_player_index = NO_PLAYER;
	if (unit_index == NONE && player_index < MAXIMUM_TRACKED_PLAYERS && distributed_deaths[player_index].valid)
	{
		struct distributed_death const *death = &distributed_deaths[player_index];

		if (death->killing_player_index != NONE)
			state->killing_player_index = (byte)death->killing_player_index;
		SET_FLAG(state->flags, _distributed_unit_friendly_fire_bit, death->friendly_fire);
		SET_FLAG(state->flags, _distributed_unit_killed_by_vehicle_bit, death->killed_by_vehicle);
	}
}

/* moves the unit toward the state if it is further than tolerance from it:
within blend_distance part of the way, further all of it */
static void distributed_apply_state(
	long unit_index,
	struct distributed_unit_state const *state,
	real tolerance,
	real blend_distance)
{
	struct object_datum *object = object_get(unit_index);
	real_vector3d error;
	real_vector3d velocity;
	real_vector3d forward;
	real_vector3d up;

	error.i = state->position.x - object->object.position.x;
	error.j = state->position.y - object->object.position.y;
	error.k = state->position.z - object->object.position.z;
	if (error.i * error.i + error.j * error.j + error.k * error.k <= tolerance * tolerance)
		return;
	distributed_vector_unpack(&state->velocity, DISTRIBUTED_VELOCITY_SCALE, &velocity);
	distributed_unit_vector_unpack(&state->forward, &forward);
	distributed_unit_vector_unpack(&state->up, &up);
	if (network_objects_reconcile(unit_index, &state->position, &forward, &up, &velocity, NULL, blend_distance))
		distributed_statistics.corrections++;
}

/* (a client) its own players' units, to the host */
static void distributed_client_send_predictions(
	void)
{
	struct distributed_unit_state_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL &&
		count < MAXIMUM_UNIT_STATES_PER_MESSAGE)
	{
		long unit_index = distributed_living_unit(player);

		/* a client speaks for its own players only, where they are */
		if (player->local_player_index == NONE || unit_index == NONE ||
			object_get(unit_index)->object.parent_object_index != NONE)
		{
			continue;
		}
		distributed_state_from_player((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index),
			&message.states[count++]);
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_player_prediction, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_unit_state)), _distributed_to_host);
	}
}

/* (the host) every player's unit, to each client: all of them every tick
but for those far from its own players (every second or third tick), and
any whose life, seat or shields' state changed at once */
static void distributed_host_send_unit_states(
	void)
{
	static struct distributed_unit_state states[MAXIMUM_TRACKED_PLAYERS];
	static boolean present[MAXIMUM_TRACKED_PLAYERS];
	static boolean changed[MAXIMUM_TRACKED_PLAYERS];
	static real_point3d origins[MAXIMUM_TRACKED_PLAYERS];
	static boolean placed[MAXIMUM_TRACKED_PLAYERS];
	struct distributed_unit_state_message message;
	long machine_indices[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
	short machine_count = distributed_client_machines(machine_indices, HALO_PORT_MAXIMUM_NETWORK_MACHINES);
	short limit = MIN(MAXIMUM_UNIT_STATES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_unit_state));
	short player_index;
	short machine_number;

	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		struct player_datum *player = distributed_player(player_index);
		struct distributed_unit_state *state = &states[player_index];
		long unit_index;

		present[player_index] = player != NULL;
		placed[player_index] = FALSE;
		if (!player)
			continue;
		distributed_state_from_player(player_index, state);
		unit_index = distributed_living_unit(player);
		if (unit_index != NONE)
		{
			object_get_origin(unit_index, &origins[player_index]);
			placed[player_index] = TRUE;
		}
		changed[player_index] =
			distributed_sent_units[player_index].flags != state->flags ||
			distributed_sent_units[player_index].unit_index != state->unit_index ||
			distributed_sent_units[player_index].vehicle_index != state->vehicle_index ||
			distributed_sent_units[player_index].seat_index != state->seat_index;
		distributed_sent_units[player_index].flags = state->flags;
		distributed_sent_units[player_index].unit_index = state->unit_index;
		distributed_sent_units[player_index].vehicle_index = state->vehicle_index;
		distributed_sent_units[player_index].seat_index = state->seat_index;
	}
	for (machine_number = 0; machine_number < machine_count; machine_number++)
	{
		long machine_index = machine_indices[machine_number];
		long *player_list = machine_get_player_list(machine_index);
		real_point3d viewers[MAXIMUM_LOCAL_PLAYERS];
		short viewer_count = 0;
		short count = 0;
		short index;

		for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
		{
			short viewer_index = player_list[index] != NONE ?
				(short)DATUM_INDEX_TO_ABSOLUTE_INDEX(player_list[index]) : NONE;

			if (viewer_index >= 0 && viewer_index < MAXIMUM_TRACKED_PLAYERS && placed[viewer_index])
				viewers[viewer_count++] = origins[viewer_index];
		}
		for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
		{
			boolean send;

			if (!present[player_index])
				continue;
			send = changed[player_index] || !placed[player_index] || !viewer_count ||
				distributed_machine_has_player(machine_index, player_index);
			if (!send)
			{
				real nearest = -1.0f;
				short period;

				for (index = 0; index < viewer_count; index++)
				{
					real dx = origins[player_index].x - viewers[index].x;
					real dy = origins[player_index].y - viewers[index].y;
					real dz = origins[player_index].z - viewers[index].z;
					real distance_squared = dx * dx + dy * dy + dz * dz;

					if (nearest < 0.0f || distance_squared < nearest)
						nearest = distance_squared;
				}
				period = nearest < NEAR_PLAYER_DISTANCE * NEAR_PLAYER_DISTANCE ? 1 :
					nearest < FAR_PLAYER_DISTANCE * FAR_PLAYER_DISTANCE ? 2 : 3;
				send = (game_time_get() + player_index) % period == 0;
			}
			if (!send)
				continue;
			message.states[count++] = states[player_index];
			if (count == limit)
			{
				distributed_send_to_machine(machine_index, &message, _distributed_message_unit_states, count,
					(word)(sizeof(message.header) + count * sizeof(struct distributed_unit_state)));
				count = 0;
			}
		}
		if (count)
		{
			distributed_send_to_machine(machine_index, &message, _distributed_message_unit_states, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_unit_state)));
		}
	}
}

/* (the host) a client's own players: the latest of each, taken at the next
tick */
static void distributed_handle_predictions(
	long machine_index,
	struct distributed_unit_state const *states,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_unit_state const *state = &states[index];

		if (state->player_index >= MAXIMUM_TRACKED_PLAYERS ||
			!distributed_machine_has_player(machine_index, state->player_index))
		{
			continue;
		}
		distributed_predictions[state->player_index].valid = TRUE;
		distributed_predictions[state->player_index].state = *state;
	}
}

/* (the host) the clients' players where they say, within a tolerance: a
little off closed by half, more put there */
static void distributed_apply_predictions(
	void)
{
	short player_index;

	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		struct distributed_unit_state const *state = &distributed_predictions[player_index].state;
		long unit_index;

		if (!distributed_predictions[player_index].valid)
			continue;
		distributed_predictions[player_index].valid = FALSE;
		unit_index = distributed_living_unit(distributed_player(player_index));
		if (unit_index != NONE && object_get(unit_index)->object.parent_object_index == NONE)
		{
			struct object_datum *object = object_get(unit_index);
			real dx = state->position.x - object->object.position.x;
			real dy = state->position.y - object->object.position.y;
			real dz = state->position.z - object->object.position.z;

			if (dx * dx + dy * dy + dz * dz <= HOST_ACCEPT_TOLERANCE * HOST_ACCEPT_TOLERANCE)
				distributed_apply_state(unit_index, state, 0.0f, HOST_BLEND_DISTANCE);
		}
	}
}

/* (a client) the host's word on every player's unit */
static void distributed_handle_unit_states(
	struct distributed_unit_state const *states,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_unit_state const *state = &states[index];
		struct player_datum *player = distributed_player(state->player_index);
		long player_index;
		long unit_index;
		boolean alive = TEST_FLAG(state->flags, _distributed_unit_alive_bit);
		boolean local;

		if (!player)
			continue;
		player_index = DATUM_INDEX_NEW(state->player_index, player->identifier);
		local = player->local_player_index != NONE;
		/* how it died, for when this machine's copy dies
		(network_distributed_player_killed) */
		if (state->player_index < MAXIMUM_TRACKED_PLAYERS)
		{
			struct distributed_death *death = &distributed_deaths[state->player_index];

			death->valid = !alive;
			death->killing_player_index = state->killing_player_index != NO_PLAYER ?
				state->killing_player_index : NONE;
			death->friendly_fire = TEST_FLAG(state->flags, _distributed_unit_friendly_fire_bit);
			death->killed_by_vehicle = TEST_FLAG(state->flags, _distributed_unit_killed_by_vehicle_bit);
		}
		unit_index = distributed_living_unit(player);
		if (!alive)
		{
			/* died on the host (who counts it; the damage that killed it,
			network_damage.c, usually kills it here first) */
			if (unit_index != NONE)
				unit_kill_no_statistics(unit_index);
			continue;
		}
		/* spawned on the host: the host's unit is the player's here too, once
		this machine has it (network_objects.c) */
		if (state->unit_index == NONE || !network_objects_client_has(state->unit_index) ||
			TEST_FLAG(object_get(state->unit_index)->object.damage_flags, _object_dead_bit))
		{
			continue;
		}
		if (player->unit_index != state->unit_index)
		{
			if (player->unit_index != NONE)
				network_player_detach_unit(player_index);
			network_player_attach_unit(player_index, state->unit_index);
			/* (a unit of a life this machine missed the end of, no player's
			now: unit_kill_no_statistics is for players' units only) */
			if (unit_index != NONE && unit_index != state->unit_index)
				unit_kill(unit_index);
		}
		unit_index = state->unit_index;
		/* the seat it rides: a client's own player's, once it has ridden
		otherwise for longer than its prediction takes to reach the host and
		come back */
		{
			struct unit_datum *unit = unit_get(unit_index);
			long vehicle_index = unit->object.parent_object_index != NONE && unit->unit.parent_seat_index != NONE ?
				unit->object.parent_object_index : NONE;
			boolean same = vehicle_index == state->vehicle_index &&
				(vehicle_index == NONE || unit->unit.parent_seat_index == state->seat_index);
			short *disagreement = &distributed_seat_disagreements[state->player_index];

			if (same)
				*disagreement = 0;
			else if (!local || ++*disagreement > SEAT_DISAGREEMENT_TICKS)
			{
				network_objects_set_seat(unit_index, state->vehicle_index, state->seat_index);
				*disagreement = 0;
			}
		}
		{
			struct damage_network_state damage;

			damage.shield_depleted = TEST_FLAG(state->flags, _distributed_unit_shield_depleted_bit);
			damage.shield_charging = TEST_FLAG(state->flags, _distributed_unit_shield_charging_bit);
			damage.shield_over_charging = TEST_FLAG(state->flags, _distributed_unit_shield_over_charging_bit);
			damage.body_vitality = distributed_vitality_unpack(state->body_vitality);
			damage.shield_vitality = distributed_vitality_unpack(state->shield_vitality);
			damage.current_body_damage = distributed_vitality_unpack(state->current_body_damage);
			damage.recent_body_damage = distributed_vitality_unpack(state->recent_body_damage);
			damage.current_shield_damage = distributed_vitality_unpack(state->current_shield_damage);
			damage.recent_shield_damage = distributed_vitality_unpack(state->recent_shield_damage);
			damage_set_network_state(unit_index, &damage);
		}
		/* the host's powerups (the host decides pickups) */
		{
			struct unit_datum *unit = unit_get(unit_index);

			csmemcpy(player->powerup_durations, state->powerup_durations, sizeof(player->powerup_durations));
			SET_FLAG(unit->unit.flags, _unit_active_camouflaged_bit,
				TEST_FLAG(state->unit_flags, _distributed_unit_camouflaged_bit));
			SET_FLAG(unit->unit.flags, _unit_super_camouflaged_bit,
				TEST_FLAG(state->unit_flags, _distributed_unit_super_camouflaged_bit));
			unit->unit.active_camouflage = (real)state->active_camouflage / 255.0f;
		}
		if (TEST_FLAG(state->flags, _distributed_unit_placed_bit) &&
			object_get(unit_index)->object.parent_object_index == NONE)
		{
			if (local)
				distributed_apply_state(unit_index, state, LOCAL_CORRECTION_TOLERANCE, 0.0f);
			else
				distributed_apply_state(unit_index, state, REMOTE_CORRECTION_TOLERANCE, REMOTE_BLEND_DISTANCE);
		}
	}
}

/* ---------- input */

/* (a client) its own players' input of the tick just run, with the buttons
of the ticks before it, to the host */
static void distributed_client_send_inputs(
	void)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_player_input inputs[MAXIMUM_LOCAL_PLAYERS];
	} message;
	short count = 0;
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		long player_index = local_player_get_player_index(local_player_index);
		struct distributed_player_input *input = &message.inputs[count];

		if (player_index == NONE)
			continue;
		csmemset(input, 0, sizeof(*input));
		if (!update_client_distributed_input(local_player_index, &input->tick, &input->action, input->control_flags))
			continue;
		input->player_index = distributed_player_to_byte(player_index);
		input->host_time = distributed_host_time;
		count++;
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_player_inputs, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_player_input)), _distributed_to_host);
	}
}

/* (the host) a client's players' input, and from it how long a message
takes that client and back */
static void distributed_handle_inputs(
	long machine_index,
	struct distributed_player_input const *inputs,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_player_input const *input = &inputs[index];
		struct player_datum *player;

		if (input->player_index >= MAXIMUM_TRACKED_PLAYERS ||
			!distributed_machine_has_player(machine_index, input->player_index))
		{
			continue;
		}
		player = distributed_player(input->player_index);
		if (!player)
			continue;
		update_server_handle_distributed_input(DATUM_INDEX_NEW(input->player_index, player->identifier), input->tick,
			&input->action, input->control_flags, DISTRIBUTED_INPUT_HISTORY);
		if (input->host_time != NONE && machine_index >= 0 && machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		{
			long sample = game_time_get() - input->host_time;

			if (sample >= 0 && sample <= MAXIMUM_ROUND_TRIP_TICKS)
			{
				/* (as TCP smooths its own: an eighth, a quarter) */
				if (!distributed_round_trips[machine_index].valid)
				{
					distributed_round_trips[machine_index].valid = TRUE;
					distributed_round_trips[machine_index].average = (real)sample;
					distributed_round_trips[machine_index].deviation = (real)sample / 2.0f;
				}
				else
				{
					real difference = (real)sample - distributed_round_trips[machine_index].average;

					distributed_round_trips[machine_index].average += difference / 8.0f;
					distributed_round_trips[machine_index].deviation +=
						((real)fabs(difference) - distributed_round_trips[machine_index].deviation) / 4.0f;
				}
			}
		}
	}
}

/* (the host) every player's input as its last tick ran it, with the
buttons of the ticks before it, to every client */
static void distributed_host_send_actions(
	void)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_relayed_action actions[MAXIMUM_UNIT_STATES_PER_MESSAGE];
	} message;
	short limit = MIN(MAXIMUM_UNIT_STATES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_relayed_action));
	long update_number = update_server_ticked_update_number();
	struct player_action const *recent[DISTRIBUTED_INPUT_HISTORY];
	short recent_counts[DISTRIBUTED_INPUT_HISTORY];
	short history;
	short action_index;
	short count = 0;

	if (update_number == NONE)
		return;
	for (history = 0; history < DISTRIBUTED_INPUT_HISTORY; history++)
		recent[history] = update_server_update_actions(update_number - history, &recent_counts[history]);
	if (!recent[0])
		return;
	for (action_index = 0; action_index < recent_counts[0] && action_index < MAXIMUM_TRACKED_PLAYERS; action_index++)
	{
		struct player_action const *action = &recent[0][action_index];
		struct distributed_relayed_action *relayed = &message.actions[count];
		real throttle_i = action->throttle.i > 1.0f ? 1.0f : action->throttle.i < -1.0f ? -1.0f : action->throttle.i;
		real throttle_j = action->throttle.j > 1.0f ? 1.0f : action->throttle.j < -1.0f ? -1.0f : action->throttle.j;
		real trigger = action->primary_trigger > 1.0f ? 1.0f : action->primary_trigger < 0.0f ? 0.0f :
			action->primary_trigger;

		if (!distributed_player(action_index))
			continue;
		csmemset(relayed, 0, sizeof(*relayed));
		relayed->player_index = (byte)action_index;
		relayed->update_number = update_number;
		relayed->desired_weapon_index = (signed char)action->desired_weapon_index;
		relayed->desired_grenade_index = (signed char)action->desired_grenade_index;
		relayed->desired_zoom_level = (signed char)MIN(action->desired_zoom_level, 127);
		for (history = 0; history < DISTRIBUTED_INPUT_HISTORY; history++)
		{
			relayed->control_flags[history] = recent[history] && action_index < recent_counts[history] ?
				(unsigned short)recent[history][action_index].control_flags : 0;
		}
		relayed->yaw = distributed_angle_pack(action->desired_facing.yaw);
		relayed->pitch = distributed_angle_pack(action->desired_facing.pitch);
		relayed->throttle_i = (signed char)(long)floor(throttle_i * 127.0f + 0.5f);
		relayed->throttle_j = (signed char)(long)floor(throttle_j * 127.0f + 0.5f);
		relayed->primary_trigger = (byte)(long)floor(trigger * 255.0f + 0.5f);
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_relayed_actions, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_relayed_action)),
				_distributed_to_clients);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_relayed_actions, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_relayed_action)),
			_distributed_to_clients);
	}
}

/* (a client) the host's input for its players, for the others it drives */
static void distributed_handle_actions(
	struct distributed_relayed_action const *actions,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_relayed_action const *relayed = &actions[index];
		struct player_action action;

		csmemset(&action, 0, sizeof(action));
		action.control_flags = relayed->control_flags[0];
		action.desired_facing.yaw = distributed_angle_unpack(relayed->yaw, FALSE);
		action.desired_facing.pitch = distributed_angle_unpack(relayed->pitch, TRUE);
		action.throttle.i = (real)relayed->throttle_i / 127.0f;
		action.throttle.j = (real)relayed->throttle_j / 127.0f;
		action.primary_trigger = (real)relayed->primary_trigger / 255.0f;
		action.desired_weapon_index = relayed->desired_weapon_index;
		action.desired_grenade_index = relayed->desired_grenade_index;
		action.desired_zoom_level = relayed->desired_zoom_level;
		update_client_handle_relayed_action(relayed->player_index, relayed->update_number, &action,
			relayed->control_flags, DISTRIBUTED_INPUT_HISTORY);
	}
}

/* ---------- deaths */

/* a player died (game_engine_player_killed, before it announces who killed
whom): the host notes who killed them for its clients; on a client, whose
copy of the death knows nothing of the killer, the host's killer, as this
machine has them */
void network_distributed_player_killed(
	long *killing_player_index,
	long *killing_object_index,
	long dead_player_index,
	boolean *friendly_fire)
{
	short dead_absolute_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(dead_player_index);
	struct distributed_death *death;

	if (!network_game_distributed() || dead_absolute_index < 0 || dead_absolute_index >= MAXIMUM_TRACKED_PLAYERS)
		return;
	death = &distributed_deaths[dead_absolute_index];
	if (game_connection() == _game_connection_network_server)
	{
		struct object_datum *killing_object = *killing_object_index != NONE ?
			object_try_and_get(*killing_object_index) : NULL;

		death->valid = TRUE;
		death->killing_player_index = *killing_player_index != NONE ?
			(short)DATUM_INDEX_TO_ABSOLUTE_INDEX(*killing_player_index) : NONE;
		death->friendly_fire = *friendly_fire;
		death->killed_by_vehicle = *killing_player_index == NONE && killing_object &&
			killing_object->object.type == _object_type_vehicle;
		distributed_statistics_due = TRUE;
	}
	else if (game_connection() == _game_connection_network_client && death->valid)
	{
		struct player_datum *killing_player = death->killing_player_index != NONE ?
			distributed_player(death->killing_player_index) : NULL;

		*friendly_fire = death->friendly_fire;
		*killing_player_index = NONE;
		if (killing_player)
		{
			*killing_player_index = DATUM_INDEX_NEW(death->killing_player_index, killing_player->identifier);
			*killing_object_index = killing_player->unit_index;
		}
		/* (an empty vehicle's: the one that did it, the same object here) */
		else if (!death->killed_by_vehicle || *killing_object_index == NONE ||
			!object_try_and_get(*killing_object_index) ||
			object_get(*killing_object_index)->object.type != _object_type_vehicle)
		{
			*killing_object_index = NONE;
		}
	}
}

/* (a client) the host's word on how a player died, with the damage that
killed them (network_damage.c), before this machine's copy dies */
void distributed_set_death(
	short dead_player_index,
	byte killing_player_index,
	boolean friendly_fire,
	boolean killed_by_vehicle)
{
	struct distributed_death *death;

	if (dead_player_index < 0 || dead_player_index >= MAXIMUM_TRACKED_PLAYERS)
		return;
	death = &distributed_deaths[dead_player_index];
	death->valid = TRUE;
	death->killing_player_index = killing_player_index != NO_PLAYER ? killing_player_index : NONE;
	death->friendly_fire = friendly_fire;
	death->killed_by_vehicle = killed_by_vehicle;
}

/* (the host) how a player died, for the damage that killed them */
boolean distributed_get_death(
	short dead_player_index,
	byte *killing_player_index,
	boolean *friendly_fire,
	boolean *killed_by_vehicle)
{
	struct distributed_death const *death;

	if (dead_player_index < 0 || dead_player_index >= MAXIMUM_TRACKED_PLAYERS)
		return FALSE;
	death = &distributed_deaths[dead_player_index];
	*killing_player_index = death->killing_player_index != NONE ? (byte)death->killing_player_index : NO_PLAYER;
	*friendly_fire = death->friendly_fire;
	*killed_by_vehicle = death->killed_by_vehicle;
	return death->valid;
}

/* ---------- pickups */

/* (the host) a player on another machine picked something up (players.c),
for that machine to show */
void network_distributed_player_picked_up(
	long player_index,
	short kind,
	long definition_index,
	short count)
{
	struct distributed_pickup *pickup;

	if (!network_game_distributed() || game_connection() != _game_connection_network_server ||
		distributed_pickup_count >= MAXIMUM_PICKUPS_PER_TICK)
	{
		return;
	}
	pickup = &distributed_pickups[distributed_pickup_count++];
	pickup->player_index = distributed_player_to_byte(player_index);
	pickup->kind = (byte)kind;
	pickup->count = count;
	pickup->definition_index = definition_index;
}

static void distributed_send_pickups(
	void)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_pickup pickups[MAXIMUM_PICKUPS_PER_TICK];
	} message;

	if (!distributed_pickup_count)
		return;
	csmemcpy(message.pickups, distributed_pickups, distributed_pickup_count * sizeof(struct distributed_pickup));
	distributed_send(&message, _distributed_message_pickups, distributed_pickup_count,
		(word)(sizeof(message.header) + distributed_pickup_count * sizeof(struct distributed_pickup)),
		_distributed_to_clients_reliably);
	distributed_pickup_count = 0;
}

/* ---------- statistics */

static void distributed_send_statistics(
	void)
{
	struct distributed_statistics_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		message.players[count].player_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		message.players[count].pad = 0;
		message.players[count].statistics = player->statistics;
		count++;
		if (count == DATAGRAM_ENTRIES(struct distributed_player_statistics))
		{
			distributed_send(&message, _distributed_message_player_statistics, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_player_statistics)),
				_distributed_to_clients);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_player_statistics, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_player_statistics)),
			_distributed_to_clients);
	}
}

/* ---------- the game type's state */

static void distributed_send_game_state(
	void)
{
	struct
	{
		struct distributed_message_header header;
		byte data[MAXIMUM_GAME_STATE_SIZE];
	} message;
	long size = game_engine_write_network_state(message.data, sizeof(message.data));

	/* (larger than a datagram) */
	if (size > 0)
	{
		distributed_send(&message, _distributed_message_game_state, 0, (word)(sizeof(message.header) + size),
			_distributed_to_clients_reliably);
	}
}

/* ---------- the game */

/* a new map loading (game.c), before any of the new game's messages can
apply: nothing sent or had yet */
void network_distributed_new_game(
	void)
{
	short sender;
	short type;
	short player_index;

	distributed_last_sent_time = NONE;
	csmemset(distributed_deaths, 0, sizeof(distributed_deaths));
	csmemset(distributed_seat_disagreements, 0, sizeof(distributed_seat_disagreements));
	csmemset(distributed_predictions, 0, sizeof(distributed_predictions));
	csmemset(distributed_round_trips, 0, sizeof(distributed_round_trips));
	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		distributed_sent_units[player_index].flags = 0;
		distributed_sent_units[player_index].unit_index = NONE;
		distributed_sent_units[player_index].vehicle_index = NONE;
		distributed_sent_units[player_index].seat_index = NONE;
	}
	for (sender = 0; sender < MAXIMUM_SENDERS; sender++)
	{
		distributed_batches[sender].size = 0;
		for (type = 0; type < NUMBER_OF_DISTRIBUTED_MESSAGES; type++)
			distributed_received_times[sender][type] = NONE;
	}
	distributed_host_time = NONE;
	distributed_statistics_due = FALSE;
	distributed_pickup_count = 0;
	network_objects_new_game();
	network_damage_new_game();
}

/* after each tick (game_time.c) */
void network_distributed_tick(
	void)
{
	short connection = game_connection();

	if (!network_game_distributed() || game_time_get() == distributed_last_sent_time)
		return;
	distributed_last_sent_time = game_time_get();
	if (connection == _game_connection_network_server)
	{
		/* the clients' players where they say they are, then the objects
		first (created before anything names them), the damage dealt this
		tick before the units it hurt and killed, and a kill's statistics
		before the kill, so that a client announcing it counts it (a double
		kill, a killing spree) */
		distributed_apply_predictions();
		network_objects_apply_vehicle_predictions();
		network_objects_host_tick();
		network_damage_host_tick();
		if (distributed_statistics_due || game_time_get() % STATISTICS_INTERVAL_TICKS == 0)
			distributed_send_statistics();
		distributed_statistics_due = FALSE;
		distributed_host_send_actions();
		distributed_host_send_unit_states();
		distributed_send_pickups();
		if (game_time_get() % GAME_STATE_INTERVAL_TICKS == 0)
			distributed_send_game_state();
	}
	else if (connection == _game_connection_network_client)
	{
		distributed_client_send_inputs();
		distributed_client_send_predictions();
		network_objects_client_tick();
		network_damage_client_tick();
	}
	distributed_batches_flush();
}

/* whether an unreliable message of the kind is older than one had already
(then it is dropped: the newer has overtaken it) */
static boolean distributed_message_stale(
	long machine_index,
	struct distributed_message_header const *header)
{
	short sender = machine_index == NONE ? HOST_SENDER : (short)machine_index;
	long *latest;

	switch (header->type)
	{
	case _distributed_message_player_prediction:
	case _distributed_message_unit_states:
	case _distributed_message_player_statistics:
	case _distributed_message_inventories:
	case _distributed_message_object_states:
	case _distributed_message_vehicle_prediction:
	case _distributed_message_player_inputs:
	case _distributed_message_relayed_actions:
		break;
	default:
		return FALSE;
	}
	if (sender < 0 || sender >= MAXIMUM_SENDERS)
		return TRUE;
	latest = &distributed_received_times[sender][header->type];
	/* (a tick's several messages of a kind have its time alike) */
	if (*latest != NONE && header->game_time < *latest)
		return TRUE;
	*latest = header->game_time;
	return FALSE;
}

/* a message of the distributed kind; machine_index is the sender's on the
host, NONE on a client */
void network_distributed_handle_message(
	long machine_index,
	word const *message,
	word size)
{
	struct distributed_message_header header;
	void const *entries = (byte const *)message + sizeof(header);
	short index;
	word entry_size;

	/* (none between games: loading, or in the menus) */
	if (size < sizeof(header) || !network_game_distributed() || !game_in_progress())
		return;
	csmemcpy(&header, message, sizeof(header));
	/* a tick's messages in one: each as if it came alone */
	if (header.type == _distributed_message_batch)
	{
		word offset = sizeof(header);

		while (offset + sizeof(word) <= size)
		{
			word buffer[(sizeof(message_header) + DATAGRAM_MAXIMUM_SIZE + 1) / sizeof(word)];
			word length;

			csmemcpy(&length, (byte const *)message + offset, sizeof(word));
			offset += sizeof(word);
			if (length > size - offset || length < sizeof(header) - sizeof(message_header) ||
				sizeof(message_header) + length > sizeof(buffer))
			{
				break;
			}
			csmemcpy(buffer, message, sizeof(message_header));
			csmemcpy((byte *)buffer + sizeof(message_header), (byte const *)message + offset, length);
			offset += length;
			/* (no batch in a batch) */
			if (((struct distributed_message_header const *)buffer)->type != _distributed_message_batch)
				network_distributed_handle_message(machine_index, buffer, (word)(sizeof(message_header) + length));
		}
		return;
	}
	switch (header.type)
	{
	case _distributed_message_player_prediction:
	case _distributed_message_unit_states: entry_size = sizeof(struct distributed_unit_state); break;
	case _distributed_message_player_statistics: entry_size = sizeof(struct distributed_player_statistics); break;
	case _distributed_message_pickups: entry_size = sizeof(struct distributed_pickup); break;
	case _distributed_message_player_inputs: entry_size = sizeof(struct distributed_player_input); break;
	case _distributed_message_relayed_actions: entry_size = sizeof(struct distributed_relayed_action); break;
	case _distributed_message_game_state:
	case _distributed_message_objects_synchronized:
	case _distributed_message_client_ready: entry_size = 0; break;
	case _distributed_message_damage_events:
	case _distributed_message_hit_reports: entry_size = network_damage_entry_size(header.type); break;
	default: entry_size = network_objects_entry_size(header.type); break;
	}
	if (header.type == 0 || header.type >= NUMBER_OF_DISTRIBUTED_MESSAGES ||
		size < sizeof(header) + header.count * entry_size)
	{
		return;
	}
	distributed_statistics.received++;

	/* (each kind from the host, or from a client) */
	switch (header.type)
	{
	case _distributed_message_player_prediction:
	case _distributed_message_client_ready:
	case _distributed_message_hit_reports:
	case _distributed_message_vehicle_prediction:
	case _distributed_message_player_inputs:
		if (machine_index == NONE || game_connection() != _game_connection_network_server)
			return;
		break;
	default:
		if (game_connection() != _game_connection_network_client)
			return;
		/* (the host's latest tick, which this client's input messages tell
		it back) */
		if (distributed_host_time == NONE || header.game_time > distributed_host_time)
			distributed_host_time = header.game_time;
		break;
	}
	if (distributed_message_stale(machine_index, &header))
		return;

	switch (header.type)
	{
	case _distributed_message_player_prediction:
		distributed_handle_predictions(machine_index, (struct distributed_unit_state const *)entries, header.count);
		break;
	case _distributed_message_unit_states:
		distributed_handle_unit_states((struct distributed_unit_state const *)entries, header.count);
		break;
	case _distributed_message_player_statistics:
	{
		/* the host's count of kills, deaths, ... */
		struct distributed_player_statistics const *players = (struct distributed_player_statistics const *)entries;

		for (index = 0; index < header.count; index++)
		{
			struct player_datum *player = distributed_player(players[index].player_index);

			if (player)
				player->statistics = players[index].statistics;
		}
		break;
	}
	case _distributed_message_inventories:
		network_objects_handle_inventories(entries, header.count);
		break;
	case _distributed_message_object_changes:
		network_objects_handle_changes(entries, header.count);
		break;
	case _distributed_message_object_states:
		network_objects_handle_states(entries, header.count);
		break;
	case _distributed_message_game_state:
		game_engine_read_network_state((byte const *)entries, size - sizeof(header));
		break;
	case _distributed_message_objects_synchronized:
		network_objects_handle_synchronized();
		break;
	case _distributed_message_client_ready:
		network_objects_client_ready(machine_index);
		break;
	case _distributed_message_damage_events:
		network_damage_handle_events(entries, header.count);
		break;
	case _distributed_message_hit_reports:
		network_damage_handle_reports(machine_index, entries, header.count);
		break;
	case _distributed_message_vehicle_prediction:
		network_objects_handle_vehicle_prediction(machine_index, entries, header.count);
		break;
	case _distributed_message_player_inputs:
		distributed_handle_inputs(machine_index, (struct distributed_player_input const *)entries, header.count);
		break;
	case _distributed_message_relayed_actions:
		distributed_handle_actions((struct distributed_relayed_action const *)entries, header.count);
		break;
	case _distributed_message_pickups:
	{
		/* what the host says this machine's players picked up */
		struct distributed_pickup const *pickups = (struct distributed_pickup const *)entries;

		for (index = 0; index < header.count; index++)
		{
			long player_index = distributed_player_from_byte(pickups[index].player_index);

			if (distributed_player_is_local(player_index))
			{
				network_player_show_pickup(player_index, pickups[index].kind, pickups[index].definition_index,
					pickups[index].count);
			}
		}
		break;
	}
	}
}
