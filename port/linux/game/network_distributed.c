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
- Every tick, a client sends the host where its own players' units are (it
  predicts them from its own input); the host takes that as they are,
  within a tolerance, as later Halo engines do.
- Every tick, the host sends every client every player's unit: which unit
  the player has, alive or not, the seat it rides, its shields and health
  (down, recharging, the damage they show), and where it is (dead: who
  killed it, which a client announces when its copy dies). A client binds,
  kills, seats and places its copies to match (it decides no deaths or
  spawns itself), and its own only when far off (a respawn, a teleport).
- Twice a second, and with every kill, the host sends every player's
  statistics (kills, deaths, ...), which clients take as they are.
- Five times a second, the host sends the game type's state (the scores,
  the flags, the balls and the hill), which clients take as it is
  (game_engine_write_network_state).
- Damage is the host's: a client reports its own players' hits, which the
  host checks and deals, and replays the damage the host deals for its
  effects (network_damage.c).

Players are named by their absolute index, which is the same on every
machine (their datum identifiers need not be).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "units/units.h"
#include "network_distributed.h"

/* network_game_globals.c's and network_server_message_handler.c's */
boolean network_distributed_client_send(void *message, word size);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_all(void *message, word size);
boolean network_distributed_server_send_to_all_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
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
	short pad1;
	real_point3d position;
	real_vector3d velocity;
	real_vector3d forward;
	real_vector3d up;
	real body_vitality;
	real shield_vitality;
	/* what the shields' and the HUD's effects show */
	real current_body_damage;
	real recent_body_damage;
	real current_shield_damage;
	real recent_shield_damage;
	/* the player's powerups: how long each has left, and how camouflaged the
	unit is */
	short powerup_durations[NUMBER_OF_PLAYER_POWERUPS];
	real active_camouflage;
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

void distributed_send(
	void *message,
	byte type,
	short count,
	word size,
	short destination)
{
	struct distributed_message_header *header = (struct distributed_message_header *)message;

	header->type = type;
	header->count = (byte)count;
	header->game_time = game_time_get();
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	distributed_statistics.sent++;
	switch (destination)
	{
	case _distributed_to_clients: network_distributed_server_send_to_all(message, size); break;
	case _distributed_to_clients_reliably: network_distributed_server_send_to_all_reliably(message, size); break;
	case _distributed_to_host: network_distributed_client_send(message, size); break;
	case _distributed_to_host_reliably: network_distributed_client_send_reliably(message, size); break;
	}
}

void distributed_send_to_machine_reliably(
	long machine_index,
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

		state->unit_index = unit_index;
		SET_FLAG(state->flags, _distributed_unit_alive_bit, TRUE);
		SET_FLAG(state->flags, _distributed_unit_placed_bit, unit->object.parent_object_index == NONE);
		if (unit->object.parent_object_index != NONE && unit->unit.parent_seat_index != NONE)
		{
			state->vehicle_index = unit->object.parent_object_index;
			state->seat_index = unit->unit.parent_seat_index;
		}
		state->position = unit->object.position;
		state->velocity = unit->object.translational_velocity;
		state->forward = unit->object.forward;
		state->up = unit->object.up;
		damage_get_network_state(unit_index, &damage);
		SET_FLAG(state->flags, _distributed_unit_shield_depleted_bit, damage.shield_depleted);
		SET_FLAG(state->flags, _distributed_unit_shield_charging_bit, damage.shield_charging);
		SET_FLAG(state->flags, _distributed_unit_shield_over_charging_bit, damage.shield_over_charging);
		state->body_vitality = damage.body_vitality;
		state->shield_vitality = damage.shield_vitality;
		state->current_body_damage = damage.current_body_damage;
		state->recent_body_damage = damage.recent_body_damage;
		state->current_shield_damage = damage.current_shield_damage;
		state->recent_shield_damage = damage.recent_shield_damage;
		csmemcpy(state->powerup_durations, player->powerup_durations, sizeof(state->powerup_durations));
		SET_FLAG(state->unit_flags, _distributed_unit_camouflaged_bit,
			TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit));
		SET_FLAG(state->unit_flags, _distributed_unit_super_camouflaged_bit,
			TEST_FLAG(unit->unit.flags, _unit_super_camouflaged_bit));
		state->active_camouflage = unit->unit.active_camouflage;
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

/* moves the unit to the state if it is further than tolerance from it */
static void distributed_apply_state(
	long unit_index,
	struct distributed_unit_state const *state,
	real tolerance)
{
	struct object_datum *object = object_get(unit_index);
	real_vector3d error;

	error.i = state->position.x - object->object.position.x;
	error.j = state->position.y - object->object.position.y;
	error.k = state->position.z - object->object.position.z;
	if (error.i * error.i + error.j * error.j + error.k * error.k <= tolerance * tolerance)
		return;
	distributed_statistics.corrections++;
	network_objects_correct(unit_index, &state->position, &state->forward, &state->up, &state->velocity, NULL);
}

static void distributed_send_unit_states(
	boolean host)
{
	struct distributed_unit_state_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;
	byte type = host ? _distributed_message_unit_states : _distributed_message_player_prediction;
	short destination = host ? _distributed_to_clients : _distributed_to_host;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short player_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		long unit_index;

		/* a client speaks for its own players only, where they are */
		if (!host)
		{
			unit_index = distributed_living_unit(player);
			if (player->local_player_index == NONE || unit_index == NONE ||
				object_get(unit_index)->object.parent_object_index != NONE)
			{
				continue;
			}
		}
		distributed_state_from_player(player_index, &message.states[count++]);
		if (count == DATAGRAM_ENTRIES(struct distributed_unit_state))
		{
			distributed_send(&message, type, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_unit_state)), destination);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, type, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_unit_state)), destination);
	}
}

/* (the host) a client's own players: taken as they are, within a
tolerance, from the players of that machine only */
static void distributed_handle_predictions(
	long machine_index,
	struct distributed_unit_state const *states,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_unit_state const *state = &states[index];
		long unit_index;

		if (!distributed_machine_has_player(machine_index, state->player_index))
			continue;
		unit_index = distributed_living_unit(distributed_player(state->player_index));
		if (unit_index != NONE && object_get(unit_index)->object.parent_object_index == NONE)
		{
			struct object_datum *object = object_get(unit_index);
			real dx = state->position.x - object->object.position.x;
			real dy = state->position.y - object->object.position.y;
			real dz = state->position.z - object->object.position.z;

			if (dx * dx + dy * dy + dz * dz <= HOST_ACCEPT_TOLERANCE * HOST_ACCEPT_TOLERANCE)
				distributed_apply_state(unit_index, state, 0.0f);
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
			damage.body_vitality = state->body_vitality;
			damage.shield_vitality = state->shield_vitality;
			damage.current_body_damage = state->current_body_damage;
			damage.recent_body_damage = state->recent_body_damage;
			damage.current_shield_damage = state->current_shield_damage;
			damage.recent_shield_damage = state->recent_shield_damage;
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
			unit->unit.active_camouflage = state->active_camouflage;
		}
		if (TEST_FLAG(state->flags, _distributed_unit_placed_bit) &&
			object_get(unit_index)->object.parent_object_index == NONE)
		{
			distributed_apply_state(unit_index, state, local ? LOCAL_CORRECTION_TOLERANCE : REMOTE_CORRECTION_TOLERANCE);
		}
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
	distributed_last_sent_time = NONE;
	csmemset(distributed_deaths, 0, sizeof(distributed_deaths));
	csmemset(distributed_seat_disagreements, 0, sizeof(distributed_seat_disagreements));
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
		/* the objects first (created before anything names them), the damage
		dealt this tick before the units it hurt and killed, and a kill's
		statistics before the kill, so that a client announcing it counts it
		(a double kill, a killing spree) */
		network_objects_host_tick();
		network_damage_host_tick();
		if (distributed_statistics_due || game_time_get() % STATISTICS_INTERVAL_TICKS == 0)
			distributed_send_statistics();
		distributed_statistics_due = FALSE;
		distributed_send_unit_states(TRUE);
		distributed_send_pickups();
		if (game_time_get() % GAME_STATE_INTERVAL_TICKS == 0)
			distributed_send_game_state();
	}
	else if (connection == _game_connection_network_client)
	{
		distributed_send_unit_states(FALSE);
		network_objects_client_tick();
		network_damage_client_tick();
	}
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
	switch (header.type)
	{
	case _distributed_message_player_prediction:
	case _distributed_message_unit_states: entry_size = sizeof(struct distributed_unit_state); break;
	case _distributed_message_player_statistics: entry_size = sizeof(struct distributed_player_statistics); break;
	case _distributed_message_pickups: entry_size = sizeof(struct distributed_pickup); break;
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
		if (machine_index == NONE || game_connection() != _game_connection_network_server)
			return;
		break;
	default:
		if (game_connection() != _game_connection_network_client)
			return;
		break;
	}

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
