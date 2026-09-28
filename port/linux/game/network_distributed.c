/*
NETWORK_DISTRIBUTED.C

The distributed netcode's own messages (port/linux/NETCODE.md): the game's
"data" message kind (message header type 2, which the Xbox game never sent),
beside its packets, and handled here (network_*_message_handler.c).

- Every tick, a client sends the host where its own players' units are (it
  predicts them from its own input); the host takes that as they are,
  within a tolerance, as later Halo engines do.
- Every tick, the host sends every client every player's unit: alive or
  not, its shields and health, and where it is. A client kills, spawns and
  sets its copies of them to match (it decides no damage, deaths or spawns
  itself), puts the others' units where the host has them, and its own
  only when far off (a respawn, a teleport).
- Twice a second, the host sends every player's statistics (kills, deaths,
  ...), which clients take as they are.
- Ten times a second, the host sends what every player's unit carries (its
  weapons by kind, their ammunition, its grenades): a client gives its
  copies the same, and decides no pickups itself (players.c).
- Five times a second, the host sends the game type's state (the scores,
  and for king of the hill where the hill is), which clients take as it is
  (game_engine_write_network_state).
- The items lying about (weapons, grenades, power-ups) are the host's: it
  says when one appears (it spawned, or was dropped) and when one goes (it
  was picked up, or cleared away), and where the ones moving are. A client
  makes and removes its copies to match (reliably), and removes any other
  item it finds on the ground (its own drops), after every tick. The host's
  and a client's copies of an item are different objects; a client maps
  one to the other.

Players are named by their index, which is the same on every machine (each
machine creates its own units, so their object indices need not match).
*/

#include "cseries.h"
#include "bungie_net/common/message_header.h"
#include "game/game.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "networking/network_connection.h"
#include "objects/objects.h"
#include "units/units.h"
#include "items/items.h"
#include "items/weapons.h"

/* network_game_globals.c's and network_server_message_handler.c's */
boolean network_distributed_client_send(void *message, word size);
boolean network_distributed_server_send_to_all(void *message, word size);
boolean network_distributed_server_send_to_all_reliably(void *message, word size);
/* players.c's */
void network_player_spawn(long player_index);
/* game_engine.c's */
long game_engine_write_network_state(byte *buffer, long size);
void game_engine_read_network_state(byte const *buffer, long size);

enum
{
	_distributed_message_player_prediction = 1,
	_distributed_message_unit_states,
	_distributed_message_player_statistics,
	_distributed_message_inventories,
	_distributed_message_item_changes,
	_distributed_message_item_positions,
	_distributed_message_game_state,

	MAXIMUM_UNIT_STATES_PER_MESSAGE = 64,
	MAXIMUM_STATISTICS_PER_MESSAGE = 64,
	STATISTICS_INTERVAL_TICKS = 15,
	INVENTORY_INTERVAL_TICKS = 3,
	GAME_STATE_INTERVAL_TICKS = 6,
	MAXIMUM_GAME_STATE_SIZE = 0xF00,
	MAXIMUM_INVENTORIES_PER_MESSAGE = 48,
	MAXIMUM_ITEMS_PER_MESSAGE = 64,
	MAXIMUM_TRACKED_OBJECTS = HALO_PORT_MAXIMUM_OBJECTS_PER_MAP,
};

/* struct distributed_unit_state flags */
enum
{
	/* the player has a unit that is alive */
	_distributed_unit_alive_bit = 0,
	/* ... and not riding (its position is its own) */
	_distributed_unit_placed_bit,
};

/* world units */
#define HOST_ACCEPT_TOLERANCE 2.5f
#define REMOTE_CORRECTION_TOLERANCE 0.05f
#define LOCAL_CORRECTION_TOLERANCE 3.0f

struct distributed_unit_state
{
	byte player_index;
	byte flags;
	short pad;
	real_point3d position;
	real_vector3d velocity;
	real_vector3d forward;
	real_vector3d up;
	real body_vitality;
	real shield_vitality;
};

struct distributed_player_statistics
{
	short player_index;
	short pad;
	struct game_statistics statistics;
};

struct distributed_inventory
{
	byte player_index;
	char grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES];
	char current_weapon_index;
	long weapon_definitions[MAXIMUM_WEAPONS_PER_UNIT];
	/* each weapon's magazines */
	short rounds_total[MAXIMUM_WEAPONS_PER_UNIT][2];
	short rounds_loaded[MAXIMUM_WEAPONS_PER_UNIT][2];
	real age[MAXIMUM_WEAPONS_PER_UNIT];
};

enum
{
	_item_change_create,
	_item_change_delete,
};

struct distributed_item
{
	byte change;
	byte pad[3];
	/* the host's object */
	long object_index;
	long definition_index;
	real_point3d position;
	real_vector3d velocity;
	real_vector3d forward;
	real_vector3d up;
};

struct distributed_message_header
{
	message_header header;
	byte type;
	byte count;
	long game_time;
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

struct distributed_inventory_message
{
	struct distributed_message_header header;
	struct distributed_inventory inventories[MAXIMUM_INVENTORIES_PER_MESSAGE];
};

struct distributed_item_message
{
	struct distributed_message_header header;
	struct distributed_item items[MAXIMUM_ITEMS_PER_MESSAGE];
};

/* the entries of a type that fit one unreliable message */
#define DATAGRAM_ENTRIES(type) \
	((short)((DATAGRAM_MAXIMUM_SIZE - sizeof(struct distributed_message_header)) / sizeof(type)))

static long distributed_last_sent_time = NONE;

/* the host: the items on the ground it has told the clients of, by
absolute index (the object's full index), NONE for none */
static long distributed_host_items[MAXIMUM_TRACKED_OBJECTS];
/* a client: its copy of each of the host's items, by the host's absolute
index (the host's full index in host_items, the copy's in copies) */
static long distributed_client_host_items[MAXIMUM_TRACKED_OBJECTS];
static long distributed_client_copies[MAXIMUM_TRACKED_OBJECTS];
/* a client: its objects that are copies of the host's items, by absolute
index (the full index) */
static long distributed_client_copy_of[MAXIMUM_TRACKED_OBJECTS];
static boolean distributed_items_ready;

/* for the automated tests' reports (network_test.c) */
static struct
{
	long sent;
	long received;
	long corrections;
	long item_creates;
	long item_deletes;
	long item_create_failures;
	long own_items_removed;
} distributed_statistics;

void network_distributed_item_statistics(
	long *creates,
	long *deletes,
	long *failures,
	long *removed)
{
	*creates = distributed_statistics.item_creates;
	*deletes = distributed_statistics.item_deletes;
	*failures = distributed_statistics.item_create_failures;
	*removed = distributed_statistics.own_items_removed;
}

void network_distributed_statistics(
	long *sent,
	long *received,
	long *corrections)
{
	*sent = distributed_statistics.sent;
	*received = distributed_statistics.received;
	*corrections = distributed_statistics.corrections;
}

/* the player at an absolute index, or NULL */
static struct player_datum *distributed_player(
	short player_index)
{
	struct player_datum *player;

	if (player_index < 0 || player_index >= player_data->maximum_count)
		return NULL;
	player = (struct player_datum *)((byte *)player_data->data + player_index * player_data->size);
	return player->identifier ? player : NULL;
}

/* the player's unit if it is alive, or NONE */
static long distributed_living_unit(
	struct player_datum const *player)
{
	if (!player || player->unit_index == NONE ||
		TEST_FLAG(object_get(player->unit_index)->object.damage_flags, _object_dead_bit))
	{
		return NONE;
	}
	return player->unit_index;
}

/* the player's living unit if it is not riding (a seat places a unit), or
NONE */
static long distributed_player_unit(
	short player_index)
{
	long unit_index = distributed_living_unit(distributed_player(player_index));

	if (unit_index == NONE || object_get(unit_index)->object.parent_object_index != NONE)
		return NONE;
	return unit_index;
}

static void distributed_state_from_player(
	short player_index,
	struct distributed_unit_state *state)
{
	long unit_index = distributed_living_unit(distributed_player(player_index));

	csmemset(state, 0, sizeof(*state));
	state->player_index = (byte)player_index;
	if (unit_index != NONE)
	{
		struct object_datum *object = object_get(unit_index);

		SET_FLAG(state->flags, _distributed_unit_alive_bit, TRUE);
		SET_FLAG(state->flags, _distributed_unit_placed_bit, object->object.parent_object_index == NONE);
		state->position = object->object.position;
		state->velocity = object->object.translational_velocity;
		state->forward = object->object.forward;
		state->up = object->object.up;
		state->body_vitality = object->object.body_vitality;
		state->shield_vitality = object->object.shield_vitality;
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
	object_set_position(unit_index, &state->position, &state->forward, &state->up);
	object->object.translational_velocity = state->velocity;
}

static void distributed_send_unit_state_message(
	struct distributed_unit_state_message *message,
	short count,
	boolean host)
{
	if (!count)
		return;
	message->header.type = host ? _distributed_message_unit_states : _distributed_message_player_prediction;
	message->header.count = (byte)count;
	message->header.game_time = game_time_get();
	message->header.header = 0;
	build_message_header(&message->header.header,
		(word)(sizeof(message->header) + count * sizeof(struct distributed_unit_state)), 2, 0);
	distributed_statistics.sent++;
	if (host)
		network_distributed_server_send_to_all(message, GET_MESSAGE_SIZE(message->header.header));
	else
		network_distributed_client_send(message, GET_MESSAGE_SIZE(message->header.header));
}

static void distributed_send_unit_states(
	boolean host)
{
	struct distributed_unit_state_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short player_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		long unit_index;

		/* a client speaks for its own players only, where they are */
		if (!host)
		{
			unit_index = distributed_player_unit(player_index);
			if (player->local_player_index == NONE || unit_index == NONE)
				continue;
		}
		distributed_state_from_player(player_index, &message.states[count++]);
		if (count == DATAGRAM_ENTRIES(struct distributed_unit_state))
		{
			distributed_send_unit_state_message(&message, count, host);
			count = 0;
		}
	}
	distributed_send_unit_state_message(&message, count, host);
}

static void distributed_send_statistics_message(
	struct distributed_statistics_message *message,
	short count)
{
	if (!count)
		return;
	message->header.type = _distributed_message_player_statistics;
	message->header.count = (byte)count;
	message->header.game_time = game_time_get();
	message->header.header = 0;
	build_message_header(&message->header.header,
		(word)(sizeof(message->header) + count * sizeof(struct distributed_player_statistics)), 2, 0);
	network_distributed_server_send_to_all(message, GET_MESSAGE_SIZE(message->header.header));
}

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
			distributed_send_statistics_message(&message, count);
			count = 0;
		}
	}
	distributed_send_statistics_message(&message, count);
}

/* ---------- items */

static void distributed_items_reset(
	void)
{
	long index;

	for (index = 0; index < MAXIMUM_TRACKED_OBJECTS; index++)
	{
		distributed_host_items[index] = NONE;
		distributed_client_host_items[index] = NONE;
		distributed_client_copies[index] = NONE;
		distributed_client_copy_of[index] = NONE;
	}
	distributed_items_ready = TRUE;
}

/* whether the object is an item lying on the ground (not one in a unit's
hand, parented to it, nor one carried, disconnected from the map) */
static boolean distributed_ground_item(
	long object_index)
{
	struct item_datum *item = item_try_and_get(object_index);
	struct object_header_datum *header;

	if (!item || item->object.parent_object_index != NONE ||
		!TEST_FLAG(item->object.flags, _object_connected_to_map_bit) ||
		TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
	{
		return FALSE;
	}
	header = (struct object_header_datum *)((byte *)object_header_data->data +
		DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index) * object_header_data->size);
	return !TEST_FLAG(header->flags, _object_header_being_deleted_bit);
}

static void distributed_item_from_object(
	long object_index,
	byte change,
	struct distributed_item *item)
{
	struct object_datum *object = object_get(object_index);

	csmemset(item, 0, sizeof(*item));
	item->change = change;
	item->object_index = object_index;
	item->definition_index = object->definition_index;
	item->position = object->object.position;
	item->velocity = object->object.translational_velocity;
	item->forward = object->object.forward;
	item->up = object->object.up;
}

static void distributed_send_items(
	struct distributed_item_message *message,
	short count,
	boolean reliably)
{
	if (!count)
		return;
	message->header.type = reliably ? _distributed_message_item_changes : _distributed_message_item_positions;
	message->header.count = (byte)count;
	message->header.game_time = game_time_get();
	message->header.header = 0;
	build_message_header(&message->header.header,
		(word)(sizeof(message->header) + count * sizeof(struct distributed_item)), 2, 0);
	if (reliably)
		network_distributed_server_send_to_all_reliably(message, GET_MESSAGE_SIZE(message->header.header));
	else
		network_distributed_server_send_to_all(message, GET_MESSAGE_SIZE(message->header.header));
}

/* the host, every tick: the items that appeared and went since the last,
and where the moving ones are */
static void distributed_host_update_items(
	void)
{
	static boolean seen[MAXIMUM_TRACKED_OBJECTS];
	struct distributed_item_message changes, positions;
	short change_count = 0, position_count = 0;
	struct object_iterator iterator;
	long index;

	csmemset(seen, 0, sizeof(seen));
	object_iterator_new(&iterator, _object_mask_weapon | _object_mask_equipment, 0);
	while (object_iterator_next(&iterator))
	{
		long object_index = iterator.index;
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
		struct object_datum *object;

		if (absolute_index >= MAXIMUM_TRACKED_OBJECTS || !distributed_ground_item(object_index))
			continue;
		seen[absolute_index] = TRUE;
		object = object_get(object_index);
		if (distributed_host_items[absolute_index] != object_index)
		{
			if (change_count == MAXIMUM_ITEMS_PER_MESSAGE)
			{
				distributed_send_items(&changes, change_count, TRUE);
				change_count = 0;
			}
			distributed_host_items[absolute_index] = object_index;
			distributed_statistics.item_creates++;
			distributed_item_from_object(object_index, _item_change_create, &changes.items[change_count++]);
		}
		else if (!TEST_FLAG(object->object.flags, _object_at_rest_bit))
		{
			if (position_count == DATAGRAM_ENTRIES(struct distributed_item))
			{
				distributed_send_items(&positions, position_count, FALSE);
				position_count = 0;
			}
			distributed_item_from_object(object_index, _item_change_create, &positions.items[position_count++]);
		}
	}
	for (index = 0; index < MAXIMUM_TRACKED_OBJECTS; index++)
	{
		if (distributed_host_items[index] != NONE && !seen[index])
		{
			if (change_count == MAXIMUM_ITEMS_PER_MESSAGE)
			{
				distributed_send_items(&changes, change_count, TRUE);
				change_count = 0;
			}
			csmemset(&changes.items[change_count], 0, sizeof(changes.items[change_count]));
			changes.items[change_count].change = _item_change_delete;
			distributed_statistics.item_deletes++;
			changes.items[change_count].object_index = distributed_host_items[index];
			change_count++;
			distributed_host_items[index] = NONE;
		}
	}
	distributed_send_items(&changes, change_count, TRUE);
	distributed_send_items(&positions, position_count, FALSE);
}

/* a client: its copy of the host's item, if it still has it */
static long distributed_client_copy(
	long host_object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(host_object_index);
	long copy_index;

	if (absolute_index >= MAXIMUM_TRACKED_OBJECTS ||
		distributed_client_host_items[absolute_index] != host_object_index)
	{
		return NONE;
	}
	copy_index = distributed_client_copies[absolute_index];
	return copy_index != NONE && object_try_and_get(copy_index) ? copy_index : NONE;
}

static void distributed_client_delete_copy(
	long host_absolute_index)
{
	long copy_index = distributed_client_copies[host_absolute_index];

	if (copy_index != NONE && object_try_and_get(copy_index))
	{
		distributed_client_copy_of[DATUM_INDEX_TO_ABSOLUTE_INDEX(copy_index)] = NONE;
		object_delete(copy_index);
	}
	distributed_client_host_items[host_absolute_index] = NONE;
	distributed_client_copies[host_absolute_index] = NONE;
}

static void distributed_client_apply_items(
	struct distributed_item const *items,
	short count,
	boolean changes)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_item const *item = &items[index];
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->object_index);

		if (absolute_index >= MAXIMUM_TRACKED_OBJECTS)
			continue;
		if (changes && item->change == _item_change_delete)
		{
			distributed_statistics.item_deletes++;
			if (distributed_client_host_items[absolute_index] == item->object_index)
				distributed_client_delete_copy(absolute_index);
		}
		else if (changes)
		{
			struct object_placement_data placement;
			long copy_index;

			if (distributed_client_host_items[absolute_index] != NONE)
				distributed_client_delete_copy(absolute_index);
			object_placement_data_new(&placement, item->definition_index, NONE);
			placement.position = item->position;
			placement.translational_velocity = item->velocity;
			placement.forward = item->forward;
			placement.up = item->up;
			copy_index = object_new(&placement);
			distributed_statistics.item_creates++;
			if (copy_index == NONE)
				distributed_statistics.item_create_failures++;
			if (copy_index != NONE && DATUM_INDEX_TO_ABSOLUTE_INDEX(copy_index) < MAXIMUM_TRACKED_OBJECTS)
			{
				/* (only the host decides when it goes) */
				object_set_garbage(copy_index, FALSE);
				distributed_client_host_items[absolute_index] = item->object_index;
				distributed_client_copies[absolute_index] = copy_index;
				distributed_client_copy_of[DATUM_INDEX_TO_ABSOLUTE_INDEX(copy_index)] = copy_index;
			}
		}
		else
		{
			long copy_index = distributed_client_copy(item->object_index);

			if (copy_index != NONE && object_get(copy_index)->object.parent_object_index == NONE)
			{
				object_set_position(copy_index, &item->position, &item->forward, &item->up);
				object_get(copy_index)->object.translational_velocity = item->velocity;
			}
		}
	}
}

/* a client, after every tick: the items on the ground that are not the
host's (its own drops, and its own spawns) go */
static void distributed_client_remove_own_items(
	void)
{
	struct object_iterator iterator;

	object_iterator_new(&iterator, _object_mask_weapon | _object_mask_equipment, 0);
	while (object_iterator_next(&iterator))
	{
		long object_index = iterator.index;
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

		if (absolute_index < MAXIMUM_TRACKED_OBJECTS && distributed_ground_item(object_index) &&
			distributed_client_copy_of[absolute_index] != object_index)
		{
			distributed_statistics.own_items_removed++;
			object_delete(object_index);
		}
	}
}

/* ---------- inventories */

static void distributed_inventory_from_player(
	short player_index,
	struct distributed_inventory *inventory)
{
	long unit_index = distributed_living_unit(distributed_player(player_index));
	short weapon_slot;

	csmemset(inventory, 0, sizeof(*inventory));
	inventory->player_index = (byte)player_index;
	for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
		inventory->weapon_definitions[weapon_slot] = NONE;
	if (unit_index == NONE)
		return;
	{
		struct unit_datum *unit = unit_get(unit_index);

		csmemcpy(inventory->grenade_counts, unit->unit.grenade_counts, sizeof(inventory->grenade_counts));
		inventory->current_weapon_index = (char)unit->unit.current_weapon_index;
		for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
		{
			long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
			struct weapon_datum *weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;

			if (weapon)
			{
				short magazine;

				inventory->weapon_definitions[weapon_slot] = weapon->definition_index;
				for (magazine = 0; magazine < 2; magazine++)
				{
					inventory->rounds_total[weapon_slot][magazine] = weapon->weapon.magazines[magazine].rounds_total;
					inventory->rounds_loaded[weapon_slot][magazine] = weapon->weapon.magazines[magazine].rounds_loaded;
				}
				inventory->age[weapon_slot] = weapon->weapon.age;
			}
		}
	}
}

static void distributed_send_inventory_message(
	struct distributed_inventory_message *message,
	short count)
{
	if (!count)
		return;
	message->header.type = _distributed_message_inventories;
	message->header.count = (byte)count;
	message->header.game_time = game_time_get();
	message->header.header = 0;
	build_message_header(&message->header.header,
		(word)(sizeof(message->header) + count * sizeof(struct distributed_inventory)), 2, 0);
	network_distributed_server_send_to_all(message, GET_MESSAGE_SIZE(message->header.header));
}

static void distributed_send_inventories(
	void)
{
	struct distributed_inventory_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		distributed_inventory_from_player((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index),
			&message.inventories[count++]);
		if (count == DATAGRAM_ENTRIES(struct distributed_inventory))
		{
			distributed_send_inventory_message(&message, count);
			count = 0;
		}
	}
	distributed_send_inventory_message(&message, count);
}

/* a client: its copy of a player's unit carries what the host's does */
static void distributed_client_apply_inventory(
	struct distributed_inventory const *inventory)
{
	struct player_datum *player = distributed_player(inventory->player_index);
	long unit_index = distributed_living_unit(player);
	struct unit_datum *unit;
	boolean local = player && player->local_player_index != NONE;
	boolean same = TRUE;
	short weapon_slot;

	if (unit_index == NONE)
		return;
	unit = unit_get(unit_index);
	for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
		long definition_index = weapon_index != NONE ? weapon_get(weapon_index)->definition_index : NONE;

		if (definition_index != inventory->weapon_definitions[weapon_slot])
			same = FALSE;
	}
	if (!same)
	{
		/* picked up, swapped or dropped on the host: the same weapons here,
		slot for slot (as unit_add_weapon_to_inventory puts one in, without
		its rules: the host has applied them) */
		for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
		{
			long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
			long definition_index = weapon_index != NONE ? weapon_get(weapon_index)->definition_index : NONE;
			struct object_placement_data placement;

			if (definition_index == inventory->weapon_definitions[weapon_slot])
				continue;
			if (weapon_index != NONE)
			{
				object_delete(weapon_index);
				unit->unit.weapon_object_indices[weapon_slot] = NONE;
				if (unit->unit.desired_weapon_index == weapon_slot)
					unit->unit.desired_weapon_index = NONE;
				if (unit->unit.current_weapon_index == weapon_slot)
					unit->unit.current_weapon_index = NONE;
			}
			if (inventory->weapon_definitions[weapon_slot] == NONE)
				continue;
			object_placement_data_new(&placement, inventory->weapon_definitions[weapon_slot], unit_index);
			placement.position = unit->object.position;
			weapon_index = object_new(&placement);
			if (weapon_index == NONE)
				continue;
			object_disconnect_from_map(weapon_index);
			object_set_visibility(weapon_index, FALSE);
			item_in_unit_inventory(weapon_index, unit_index);
			unit->unit.weapon_object_indices[weapon_slot] = weapon_index;
			unit->unit.weapon_last_used_at_game_time[weapon_slot] = 0;
		}
	}
	/* the weapon in hand: a client's own player chooses its own, unless its
	choice is gone */
	if (inventory->current_weapon_index >= 0 && inventory->current_weapon_index < MAXIMUM_WEAPONS_PER_UNIT &&
		unit->unit.current_weapon_index != inventory->current_weapon_index &&
		(!local || unit->unit.current_weapon_index == NONE ||
			unit->unit.weapon_object_indices[unit->unit.current_weapon_index] == NONE))
	{
		unit->unit.desired_weapon_index = inventory->current_weapon_index;
	}
	/* the ammunition: a client's own player spends its own as it fires
	(the host's count trails it), so it is only brought into line when it
	differs by more than that, or the host's is higher (a pickup) */
	for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
		struct weapon_datum *weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;
		short magazine;

		if (!weapon || weapon->definition_index != inventory->weapon_definitions[weapon_slot])
			continue;
		for (magazine = 0; magazine < 2; magazine++)
		{
			struct weapon_magazine *state = &weapon->weapon.magazines[magazine];
			short total = inventory->rounds_total[weapon_slot][magazine];
			short loaded = inventory->rounds_loaded[weapon_slot][magazine];

			if (!local || total > state->rounds_total || state->rounds_total - total > 8)
			{
				state->rounds_total = total;
				state->rounds_loaded = loaded;
			}
		}
		if (!local || inventory->age[weapon_slot] < weapon->weapon.age - 0.1f)
			weapon->weapon.age = inventory->age[weapon_slot];
	}
	{
		short grenade_type;

		for (grenade_type = 0; grenade_type < NUMBER_OF_UNIT_GRENADE_TYPES; grenade_type++)
		{
			if (!local || inventory->grenade_counts[grenade_type] > unit->unit.grenade_counts[grenade_type] ||
				unit->unit.grenade_counts[grenade_type] - inventory->grenade_counts[grenade_type] > 1)
			{
				unit->unit.grenade_counts[grenade_type] = inventory->grenade_counts[grenade_type];
			}
		}
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

	if (size <= 0)
		return;
	message.header.type = _distributed_message_game_state;
	message.header.count = 0;
	message.header.game_time = game_time_get();
	message.header.header = 0;
	build_message_header(&message.header.header, (word)(sizeof(message.header) + size), 2, 0);
	/* (larger than a datagram) */
	network_distributed_server_send_to_all_reliably(&message, GET_MESSAGE_SIZE(message.header.header));
}

/* a new map loading (game.c), before any of the new game's messages can
apply: nothing sent or copied yet */
void network_distributed_new_game(
	void)
{
	distributed_items_reset();
	distributed_last_sent_time = NONE;
}

/* after each tick (game_time.c) */
void network_distributed_tick(
	void)
{
	short connection = game_connection();

	if (!network_game_distributed() || game_time_get() == distributed_last_sent_time)
		return;
	if (!distributed_items_ready)
		distributed_items_reset();
	distributed_last_sent_time = game_time_get();
	if (connection == _game_connection_network_server)
	{
		distributed_send_unit_states(TRUE);
		distributed_host_update_items();
		if (game_time_get() % INVENTORY_INTERVAL_TICKS == 0)
			distributed_send_inventories();
		if (game_time_get() % STATISTICS_INTERVAL_TICKS == 0)
			distributed_send_statistics();
		if (game_time_get() % GAME_STATE_INTERVAL_TICKS == 0)
			distributed_send_game_state();
	}
	else if (connection == _game_connection_network_client)
	{
		distributed_send_unit_states(FALSE);
		distributed_client_remove_own_items();
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
	struct distributed_unit_state const *states;
	short index;

	/* (none between games: loading, or in the menus) */
	if (size < sizeof(header) || !network_game_distributed() || !game_in_progress())
		return;
	csmemcpy(&header, message, sizeof(header));
	{
		word entry_size;

		switch (header.type)
		{
		case _distributed_message_player_statistics: entry_size = sizeof(struct distributed_player_statistics); break;
		case _distributed_message_inventories: entry_size = sizeof(struct distributed_inventory); break;
		case _distributed_message_item_changes:
		case _distributed_message_item_positions: entry_size = sizeof(struct distributed_item); break;
		default: entry_size = sizeof(struct distributed_unit_state); break;
		}
		if (size < sizeof(header) + header.count * entry_size)
			return;
	}
	if (!distributed_items_ready)
		distributed_items_reset();
	states = (struct distributed_unit_state const *)((byte const *)message + sizeof(header));
	distributed_statistics.received++;

	switch (header.type)
	{
	case _distributed_message_player_prediction:
	{
		/* (the host) a client's own players: taken as they are, within a
		tolerance, from the players of that machine only */
		long *player_list;

		if (machine_index == NONE || game_connection() != _game_connection_network_server)
			return;
		player_list = machine_get_player_list(machine_index);
		for (index = 0; index < header.count; index++)
		{
			short local_player_index;
			long unit_index;

			for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
			{
				if (player_list[local_player_index] != NONE &&
					DATUM_INDEX_TO_ABSOLUTE_INDEX(player_list[local_player_index]) == states[index].player_index)
				{
					break;
				}
			}
			if (local_player_index == MAXIMUM_LOCAL_PLAYERS)
				continue;
			unit_index = distributed_player_unit(states[index].player_index);
			if (unit_index != NONE)
			{
				struct object_datum *object = object_get(unit_index);
				real dx = states[index].position.x - object->object.position.x;
				real dy = states[index].position.y - object->object.position.y;
				real dz = states[index].position.z - object->object.position.z;

				if (dx * dx + dy * dy + dz * dz <= HOST_ACCEPT_TOLERANCE * HOST_ACCEPT_TOLERANCE)
					distributed_apply_state(unit_index, &states[index], 0.0f);
			}
		}
		break;
	}
	case _distributed_message_unit_states:
		/* (a client) the host's word on every player's unit */
		if (game_connection() != _game_connection_network_client)
			return;
		for (index = 0; index < header.count; index++)
		{
			struct distributed_unit_state const *state = &states[index];
			struct player_datum *player = distributed_player(state->player_index);
			long unit_index = distributed_living_unit(player);
			boolean alive = TEST_FLAG(state->flags, _distributed_unit_alive_bit);

			if (!player)
				continue;
			if (alive && unit_index == NONE && player->unit_index == NONE)
			{
				/* spawned on the host: here too, then put where the host has it */
				network_player_spawn(DATUM_INDEX_NEW(state->player_index, player->identifier));
				unit_index = distributed_living_unit(player);
				if (unit_index != NONE && TEST_FLAG(state->flags, _distributed_unit_placed_bit))
					distributed_apply_state(unit_index, state, 0.0f);
				continue;
			}
			if (!alive && unit_index != NONE)
			{
				/* died on the host (who counts it) */
				unit_kill_no_statistics(unit_index);
				continue;
			}
			if (unit_index == NONE)
				continue;
			object_get(unit_index)->object.body_vitality = state->body_vitality;
			object_get(unit_index)->object.shield_vitality = state->shield_vitality;
			if (TEST_FLAG(state->flags, _distributed_unit_placed_bit) &&
				object_get(unit_index)->object.parent_object_index == NONE)
			{
				distributed_apply_state(unit_index, state,
					player->local_player_index != NONE ? LOCAL_CORRECTION_TOLERANCE : REMOTE_CORRECTION_TOLERANCE);
			}
		}
		break;
	case _distributed_message_player_statistics:
	{
		/* (a client) the host's count of kills, deaths, ... */
		struct distributed_player_statistics const *players =
			(struct distributed_player_statistics const *)((byte const *)message + sizeof(header));

		if (game_connection() != _game_connection_network_client)
			return;
		for (index = 0; index < header.count; index++)
		{
			struct player_datum *player = distributed_player(players[index].player_index);

			if (player)
				player->statistics = players[index].statistics;
		}
		break;
	}
	case _distributed_message_inventories:
	{
		struct distributed_inventory const *inventories =
			(struct distributed_inventory const *)((byte const *)message + sizeof(header));

		if (game_connection() != _game_connection_network_client)
			return;
		for (index = 0; index < header.count; index++)
			distributed_client_apply_inventory(&inventories[index]);
		break;
	}
	case _distributed_message_item_changes:
	case _distributed_message_item_positions:
		if (game_connection() != _game_connection_network_client)
			return;
		distributed_client_apply_items((struct distributed_item const *)((byte const *)message + sizeof(header)),
			header.count, header.type == _distributed_message_item_changes);
		break;
	case _distributed_message_game_state:
		if (game_connection() != _game_connection_network_client)
			return;
		game_engine_read_network_state((byte const *)message + sizeof(header), size - sizeof(header));
		break;
	}
}
