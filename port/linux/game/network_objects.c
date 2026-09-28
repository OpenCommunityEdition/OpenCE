/*
NETWORK_OBJECTS.C

The distributed netcode's objects (port/linux/NETCODE.md, stages 3 and 4).

The game's objects that matter to more than the eye (units, vehicles,
weapons and equipment) are the host's, and every machine has them at the
same datum index (identifier and all), so that any message can name one:

- The host tells its clients (reliably) of each object it makes, with what
  it is, where, and how it looks, and of each it deletes; a client makes
  and deletes its copies to match, at the host's index. A client that has
  loaded asks for the host's objects all over and is told when it has them
  all; from then on it deletes any such object it has that the host has not
  told it of (what its own simulation made: a dying unit's grenades, a
  vehicle's weapons), and nothing but the host's word deletes the host's.
- A client places the game's objects when its map loads as the host did,
  at the same indices: the host's word on those finds them already there.
  Past loading, its own objects (projectiles, effects' objects: what only
  it sees) take indices from the upper half of the array, which the host's
  do not reach in practice; a host's object that does comes first.
- Every tick the host sends where its moving objects are (vehicles, items,
  bodies), and a few of those at rest, round the lot; a client puts its
  copies there, drawn gliding from where they were rather than jumping
  (render_interpolation.c). A client's own player's vehicle is its own, as
  its own player's unit is: it sends the host where it drives it, which the
  host takes within a tolerance, and puts it where the host has it only
  when far off.
- Ten times a second, the host sends what every unit carries: which
  weapons, slot for slot, their ammunition, the weapon in hand and the
  grenades; a client moves its copies of those weapons in and out of its
  copies of the units to match (it decides no pickups, swaps or drops
  itself).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "units/units.h"
#include "items/items.h"
#include "items/weapons.h"
#include "network_distributed.h"

/* units.c's */
void unit_network_add_weapon(long unit_index, long weapon_index, short slot);
void unit_network_drop_weapon(long unit_index, short slot);
void unit_network_forget_weapon(long unit_index, short slot);
/* players.c's */
void network_player_detach_unit(long player_index);
/* render_interpolation.c's */
void render_interpolation_correct_object(long object_index, real_vector3d const *offset);

enum
{
	INVENTORY_INTERVAL_TICKS = 3,
	/* the host's objects at rest sent each tick, round them all */
	RESTING_STATES_PER_TICK = 4,
	/* a client asks for the host's objects again this often until it has
	them (a host still loading misses the asking) */
	CLIENT_READY_INTERVAL_TICKS = TICKS_PER_SECOND,
	/* a client's own objects, from here up */
	LOCAL_OBJECTS_FIRST_INDEX = MAXIMUM_OBJECTS_PER_MAP / 2,
	MAXIMUM_ENTRIES_PER_MESSAGE = 64,
	/* the objects the host has at the same indices everywhere */
	NETWORKED_OBJECT_TYPES =
		_object_mask_biped | _object_mask_vehicle | _object_mask_weapon | _object_mask_equipment,
};

/* world units */
#define REMOTE_OBJECT_TOLERANCE 0.05f
#define LOCAL_VEHICLE_TOLERANCE 4.0f
/* (the host takes further than a client puts right: between the two they
would disagree for good) */
#define HOST_VEHICLE_ACCEPT_TOLERANCE 5.0f

enum
{
	_object_change_create,
	_object_change_delete,
};

/* struct distributed_object_change and struct distributed_object_state
flags */
enum
{
	/* an item in a unit's inventory, not in the world */
	_distributed_object_carried_bit = 0,
	_distributed_object_at_rest_bit,
};

struct distributed_object_change
{
	byte change;
	byte flags;
	byte owner_player_index;
	byte pad;
	long object_index;
	long definition_index;
	short owner_team_index;
	short variant_number;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d translational_velocity;
	real_vector3d angular_velocity;
	real_rgb_color change_colors[NUMBER_OF_OBJECT_CHANGE_COLORS];
	byte region_permutations[MAXIMUM_REGIONS_PER_OBJECT];
};

struct distributed_object_state
{
	long object_index;
	byte flags;
	byte pad[3];
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d translational_velocity;
	real_vector3d angular_velocity;
};

struct distributed_inventory
{
	long unit_index;
	char grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES];
	char current_weapon_index;
	byte pad;
	long weapon_indices[MAXIMUM_WEAPONS_PER_UNIT];
	/* each weapon's magazines */
	short rounds_total[MAXIMUM_WEAPONS_PER_UNIT][2];
	short rounds_loaded[MAXIMUM_WEAPONS_PER_UNIT][2];
	real age[MAXIMUM_WEAPONS_PER_UNIT];
};

struct distributed_object_change_message
{
	struct distributed_message_header header;
	struct distributed_object_change changes[MAXIMUM_ENTRIES_PER_MESSAGE];
};

struct distributed_object_state_message
{
	struct distributed_message_header header;
	struct distributed_object_state states[MAXIMUM_ENTRIES_PER_MESSAGE];
};

struct distributed_inventory_message
{
	struct distributed_message_header header;
	struct distributed_inventory inventories[MAXIMUM_ENTRIES_PER_MESSAGE];
};

/* ---------- globals */

/* the host: the objects it has told its clients of, by absolute index
(the object's datum index), NONE for none */
static long objects_host_told[MAXIMUM_TRACKED_OBJECTS];
static short objects_host_resting_cursor;
/* a client: the host's objects it has, by absolute index */
static long objects_client_has[MAXIMUM_TRACKED_OBJECTS];
/* ... all of them (the host said so), and when it last asked for them */
static boolean objects_client_synchronized;
static long objects_client_ready_time;
/* ... past loading: its own objects from the upper half */
static boolean objects_client_local_allocation;
static short objects_client_local_identifier;
/* ... making the host's object at the host's index, NONE for none */
static long objects_client_creating_index = NONE;
static boolean objects_client_creating;
/* ... deleting one on the host's word */
static boolean objects_client_deleting;

/* for the automated tests' reports (network_test.c) */
static struct
{
	long creates;
	long deletes;
	long create_failures;
	long own_objects_removed;
} objects_statistics;

void network_distributed_item_statistics(
	long *creates,
	long *deletes,
	long *failures,
	long *removed)
{
	*creates = objects_statistics.creates;
	*deletes = objects_statistics.deletes;
	*failures = objects_statistics.create_failures;
	*removed = objects_statistics.own_objects_removed;
}

/* ---------- objects (objects.c) */

/* the datum index a new object takes: the host's, for its object a client
makes, or a client's own from the upper half; NONE for the first free */
long network_objects_new_object_index(
	void)
{
	long object_index = objects_client_creating_index;
	short absolute_index;

	if (object_index != NONE)
	{
		objects_client_creating_index = NONE;
		return object_index;
	}
	if (!objects_client_local_allocation || !network_game_distributed_client())
		return NONE;
	for (absolute_index = LOCAL_OBJECTS_FIRST_INDEX; absolute_index < object_header_data->maximum_count; absolute_index++)
	{
		struct datum_header const *header = (struct datum_header const *)
			((byte const *)object_header_data->data + absolute_index * object_header_data->size);

		if (!header->identifier)
		{
			/* (any identifier but 0, which marks a free datum) */
			if (++objects_client_local_identifier == 0)
				objects_client_local_identifier = 1;
			return ((long)objects_client_local_identifier << 16) | absolute_index;
		}
	}
	return NONE;
}

/* whether a client is making the host's object: which the host has made as
the game type has it (object_new does not remap it again), with its own
parts (a unit's initial weapons are the host's objects too) */
boolean network_objects_creating_host_object(
	void)
{
	return objects_client_creating;
}

/* whether the object may be deleted: not the host's on a client, but on
the host's word */
boolean network_objects_may_delete(
	long object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	return objects_client_deleting || !network_game_distributed_client() ||
		absolute_index < 0 || absolute_index >= MAXIMUM_TRACKED_OBJECTS ||
		objects_client_has[absolute_index] != object_index;
}

/* the map's objects placed (game.c): a client's own come from the upper
half from now on */
void network_objects_placed(
	void)
{
	objects_client_local_allocation = network_game_distributed_client();
}

/* ---------- common */

static boolean distributed_object_networked(
	long object_index)
{
	struct object_header_datum *header = object_header_try_and_get(object_index);

	return header && header->datum && TEST_FLAG(NETWORKED_OBJECT_TYPES, header->type) &&
		!TEST_FLAG(header->flags, _object_header_being_deleted_bit) &&
		DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index) < MAXIMUM_TRACKED_OBJECTS;
}

boolean network_objects_client_has(
	long object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	return object_index != NONE && absolute_index >= 0 && absolute_index < MAXIMUM_TRACKED_OBJECTS &&
		objects_client_has[absolute_index] == object_index && object_try_and_get(object_index);
}

word network_objects_entry_size(
	byte type)
{
	switch (type)
	{
	case _distributed_message_object_changes: return sizeof(struct distributed_object_change);
	case _distributed_message_object_states:
	case _distributed_message_vehicle_prediction: return sizeof(struct distributed_object_state);
	case _distributed_message_inventories: return sizeof(struct distributed_inventory);
	}
	return 0;
}

void network_objects_correct(
	long object_index,
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *velocity,
	real_vector3d const *angular_velocity)
{
	struct object_datum *object = object_get(object_index);
	real_vector3d offset;

	/* (drawn from where it was, the difference fading over a few ticks) */
	offset.i = object->object.position.x - position->x;
	offset.j = object->object.position.y - position->y;
	offset.k = object->object.position.z - position->z;
	object_set_position(object_index, position, forward, up);
	if (velocity)
		object->object.translational_velocity = *velocity;
	if (angular_velocity)
		object->object.angular_velocity = *angular_velocity;
	render_interpolation_correct_object(object_index, &offset);
}

static void distributed_state_from_object(
	long object_index,
	struct distributed_object_state *state)
{
	struct object_datum *object = object_get(object_index);

	csmemset(state, 0, sizeof(*state));
	state->object_index = object_index;
	SET_FLAG(state->flags, _distributed_object_at_rest_bit, TEST_FLAG(object->object.flags, _object_at_rest_bit));
	state->position = object->object.position;
	state->forward = object->object.forward;
	state->up = object->object.up;
	state->translational_velocity = object->object.translational_velocity;
	state->angular_velocity = object->object.angular_velocity;
}

/* the vehicle the player's unit drives, or NONE */
static long distributed_driven_vehicle(
	struct player_datum const *player)
{
	long unit_index = distributed_living_unit(player);
	struct unit_datum *vehicle;

	if (unit_index == NONE || object_get(unit_index)->object.parent_object_index == NONE)
		return NONE;
	vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(
		object_get(unit_index)->object.parent_object_index, _object_mask_vehicle);
	return vehicle && vehicle->unit.driver_object_index == unit_index ?
		object_get(unit_index)->object.parent_object_index : NONE;
}

/* ---------- the host */

static void distributed_change_from_object(
	long object_index,
	struct distributed_object_change *change)
{
	struct object_datum *object = object_get(object_index);

	csmemset(change, 0, sizeof(*change));
	change->change = _object_change_create;
	change->object_index = object_index;
	change->definition_index = object->definition_index;
	change->owner_player_index = distributed_player_to_byte(object->object.owner_player_index);
	change->owner_team_index = object->object.owner_team_index;
	change->variant_number = object->object.variant_number;
	change->position = object->object.position;
	change->forward = object->object.forward;
	change->up = object->object.up;
	change->translational_velocity = object->object.translational_velocity;
	change->angular_velocity = object->object.angular_velocity;
	csmemcpy(change->change_colors, object->object.base_change_colors, sizeof(change->change_colors));
	csmemcpy(change->region_permutations, object->object.region_permutations, sizeof(change->region_permutations));
	SET_FLAG(change->flags, _distributed_object_at_rest_bit, TEST_FLAG(object->object.flags, _object_at_rest_bit));
	if (TEST_FLAG(_object_mask_item, object->object.type))
	{
		struct item_datum *item = item_get(object_index);

		SET_FLAG(change->flags, _distributed_object_carried_bit,
			TEST_FLAG(item->item.flags, _item_attached_to_unit_bit) ||
			object->object.parent_object_index != NONE ||
			!TEST_FLAG(object->object.flags, _object_connected_to_map_bit));
	}
}

/* the objects made and deleted since the last time, to every client */
static void distributed_host_update_objects(
	void)
{
	static boolean seen[MAXIMUM_TRACKED_OBJECTS];
	struct distributed_object_change_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, RELIABLE_ENTRIES(struct distributed_object_change));
	struct object_iterator iterator;
	long absolute_index;

	csmemset(seen, 0, sizeof(seen));
	object_iterator_new(&iterator, NETWORKED_OBJECT_TYPES, 0);
	while (object_iterator_next(&iterator))
	{
		if (!distributed_object_networked(iterator.index))
			continue;
		absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index);
		seen[absolute_index] = TRUE;
		if (objects_host_told[absolute_index] == iterator.index)
			continue;
		/* (another object in the same place: that one is gone) */
		if (objects_host_told[absolute_index] != NONE)
		{
			csmemset(&message.changes[count], 0, sizeof(message.changes[count]));
			message.changes[count].change = _object_change_delete;
			message.changes[count].object_index = objects_host_told[absolute_index];
			objects_statistics.deletes++;
			if (++count == limit)
			{
				distributed_send(&message, _distributed_message_object_changes, count,
					(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)),
					_distributed_to_clients_reliably);
				count = 0;
			}
		}
		objects_host_told[absolute_index] = iterator.index;
		objects_statistics.creates++;
		distributed_change_from_object(iterator.index, &message.changes[count]);
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_object_changes, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)),
				_distributed_to_clients_reliably);
			count = 0;
		}
	}
	for (absolute_index = 0; absolute_index < MAXIMUM_TRACKED_OBJECTS; absolute_index++)
	{
		if (objects_host_told[absolute_index] == NONE || seen[absolute_index])
			continue;
		csmemset(&message.changes[count], 0, sizeof(message.changes[count]));
		message.changes[count].change = _object_change_delete;
		message.changes[count].object_index = objects_host_told[absolute_index];
		objects_host_told[absolute_index] = NONE;
		objects_statistics.deletes++;
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_object_changes, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)),
				_distributed_to_clients_reliably);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_object_changes, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)),
			_distributed_to_clients_reliably);
	}
}

/* a client has loaded the game: every object the host has, to it alone,
and word that that is all of them */
void network_objects_client_ready(
	long machine_index)
{
	struct distributed_object_change_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, RELIABLE_ENTRIES(struct distributed_object_change));
	long absolute_index;

	distributed_host_update_objects();
	for (absolute_index = 0; absolute_index < MAXIMUM_TRACKED_OBJECTS; absolute_index++)
	{
		if (objects_host_told[absolute_index] == NONE)
			continue;
		distributed_change_from_object(objects_host_told[absolute_index], &message.changes[count]);
		if (++count == limit)
		{
			distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_object_changes, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)));
			count = 0;
		}
	}
	if (count)
	{
		distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_object_changes, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_object_change)));
	}
	distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_objects_synchronized, 0,
		(word)sizeof(message.header));
}

/* where the moving objects are (not players' living units, which have
their own messages, nor what is attached or carried), and a few at rest */
static void distributed_host_send_states(
	void)
{
	struct distributed_object_state_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_object_state));
	short resting = 0;
	long absolute_index;

	for (absolute_index = 0; absolute_index < MAXIMUM_TRACKED_OBJECTS; absolute_index++)
	{
		long object_index = objects_host_told[absolute_index];
		struct object_datum *object;
		boolean send;

		if (object_index == NONE || !object_try_and_get(object_index))
			continue;
		object = object_get(object_index);
		if (object->object.parent_object_index != NONE ||
			!TEST_FLAG(object->object.flags, _object_connected_to_map_bit))
		{
			continue;
		}
		if (TEST_FLAG(_object_mask_unit, object->object.type))
		{
			struct unit_datum *unit = unit_get(object_index);

			if (unit->unit.player_index != NONE && !TEST_FLAG(object->object.damage_flags, _object_dead_bit))
				continue;
		}
		send = !TEST_FLAG(object->object.flags, _object_at_rest_bit);
		/* (the resting ones round the lot, a few a tick) */
		if (!send && resting < RESTING_STATES_PER_TICK &&
			((absolute_index - objects_host_resting_cursor) & 63) == 0)
		{
			send = TRUE;
			resting++;
		}
		if (!send)
			continue;
		distributed_state_from_object(object_index, &message.states[count]);
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_object_states, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_object_state)),
				_distributed_to_clients);
			count = 0;
		}
	}
	objects_host_resting_cursor = (short)((objects_host_resting_cursor + 1) & 63);
	if (count)
	{
		distributed_send(&message, _distributed_message_object_states, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_object_state)),
			_distributed_to_clients);
	}
}

static void distributed_inventory_from_unit(
	long unit_index,
	struct distributed_inventory *inventory)
{
	struct unit_datum *unit = unit_get(unit_index);
	short weapon_slot;

	csmemset(inventory, 0, sizeof(*inventory));
	inventory->unit_index = unit_index;
	csmemcpy(inventory->grenade_counts, unit->unit.grenade_counts, sizeof(inventory->grenade_counts));
	inventory->current_weapon_index = (char)unit->unit.current_weapon_index;
	for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
		struct weapon_datum *weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;

		inventory->weapon_indices[weapon_slot] = NONE;
		if (weapon)
		{
			short magazine;

			inventory->weapon_indices[weapon_slot] = weapon_index;
			for (magazine = 0; magazine < 2; magazine++)
			{
				inventory->rounds_total[weapon_slot][magazine] = weapon->weapon.magazines[magazine].rounds_total;
				inventory->rounds_loaded[weapon_slot][magazine] = weapon->weapon.magazines[magazine].rounds_loaded;
			}
			inventory->age[weapon_slot] = weapon->weapon.age;
		}
	}
}

/* what every unit carries */
static void distributed_host_send_inventories(
	void)
{
	struct distributed_inventory_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_inventory));
	struct object_iterator iterator;

	object_iterator_new(&iterator, _object_mask_unit, 0);
	while (object_iterator_next(&iterator))
	{
		struct unit_datum *unit = unit_get(iterator.index);
		short weapon_slot;
		boolean carries = unit->unit.grenade_counts[0] || unit->unit.grenade_counts[1];

		for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
			carries |= unit->unit.weapon_object_indices[weapon_slot] != NONE;
		if (!carries || !distributed_object_networked(iterator.index) ||
			TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		{
			continue;
		}
		distributed_inventory_from_unit(iterator.index, &message.inventories[count]);
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_inventories, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_inventory)),
				_distributed_to_clients);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_inventories, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_inventory)),
			_distributed_to_clients);
	}
}

/* (a client) the vehicles its own players drive: taken as they are,
within a tolerance, if that machine's player drives it */
void network_objects_handle_vehicle_prediction(
	long machine_index,
	void const *entries,
	short count)
{
	struct distributed_object_state const *states = (struct distributed_object_state const *)entries;
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_object_state const *state = &states[index];
		struct unit_datum *vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(
			state->object_index, _object_mask_vehicle);
		struct unit_datum *driver;
		real dx, dy, dz;

		if (!vehicle || vehicle->unit.driver_object_index == NONE || vehicle->object.parent_object_index != NONE)
			continue;
		driver = unit_get(vehicle->unit.driver_object_index);
		if (driver->unit.player_index == NONE ||
			!distributed_machine_has_player(machine_index, (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(driver->unit.player_index)))
		{
			continue;
		}
		dx = state->position.x - vehicle->object.position.x;
		dy = state->position.y - vehicle->object.position.y;
		dz = state->position.z - vehicle->object.position.z;
		if (dx * dx + dy * dy + dz * dz <= HOST_VEHICLE_ACCEPT_TOLERANCE * HOST_VEHICLE_ACCEPT_TOLERANCE)
		{
			network_objects_correct(state->object_index, &state->position, &state->forward, &state->up,
				&state->translational_velocity, &state->angular_velocity);
		}
	}
}

void network_objects_host_tick(
	void)
{
	distributed_host_update_objects();
	distributed_host_send_states();
	if (game_time_get() % INVENTORY_INTERVAL_TICKS == 0)
		distributed_host_send_inventories();
}

/* ---------- a client */

/* the item out of whatever unit has it */
static void distributed_client_release_item(
	long item_index,
	boolean deleting)
{
	struct object_iterator iterator;

	object_iterator_new(&iterator, _object_mask_unit, 0);
	while (object_iterator_next(&iterator))
	{
		struct unit_datum *unit = unit_get(iterator.index);
		short weapon_slot;

		for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
		{
			if (unit->unit.weapon_object_indices[weapon_slot] != item_index)
				continue;
			if (deleting)
				unit_network_forget_weapon(iterator.index, weapon_slot);
			else
				unit_network_drop_weapon(iterator.index, weapon_slot);
		}
	}
}

/* deletes an object (the host's, on its word, or one the host has not
told of), undoing what refers to it first */
static void distributed_client_delete(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);

	if (!object)
		return;
	if (TEST_FLAG(_object_mask_item, object->object.type))
		distributed_client_release_item(object_index, TRUE);
	if (TEST_FLAG(_object_mask_unit, object->object.type))
	{
		struct unit_datum *unit = unit_get(object_index);
		long child_index = unit->object.first_child_object_index;

		/* (its riders out first: they are deleted with it otherwise) */
		while (child_index != NONE)
		{
			long next_index = object_get(child_index)->object.next_object_index;

			if (TEST_FLAG(_object_mask_unit, object_get(child_index)->object.type) &&
				unit_get(child_index)->unit.parent_seat_index != NONE)
			{
				unit_exit_seat_end(child_index);
			}
			child_index = next_index;
		}
		if (unit->unit.player_index != NONE && player_try_and_get(unit->unit.player_index) &&
			player_get(unit->unit.player_index)->unit_index == object_index)
		{
			network_player_detach_unit(unit->unit.player_index);
		}
	}
	objects_client_deleting = TRUE;
	object_delete_immediately(object_index);
	objects_client_deleting = FALSE;
}

static void distributed_client_create(
	struct distributed_object_change const *change)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(change->object_index);
	struct datum_header const *header;
	struct object_placement_data placement;
	long object_index;

	if (absolute_index < 0 || absolute_index >= MAXIMUM_TRACKED_OBJECTS ||
		absolute_index >= object_header_data->maximum_count)
	{
		return;
	}
	header = (struct datum_header const *)((byte const *)object_header_data->data +
		absolute_index * object_header_data->size);
	if (header->identifier)
	{
		long existing_index = ((long)header->identifier << 16) | absolute_index;
		struct object_datum *existing = object_try_and_get(existing_index);

		/* placed here when the map loaded, as on the host: the same object */
		if (existing_index == change->object_index && existing &&
			existing->definition_index == change->definition_index)
		{
			objects_client_has[absolute_index] = existing_index;
			object_set_garbage(existing_index, FALSE);
			return;
		}
		/* something else in its place: gone */
		distributed_client_delete(existing_index);
		objects_client_has[absolute_index] = NONE;
		if (header->identifier)
		{
			objects_statistics.create_failures++;
			return;
		}
	}
	object_placement_data_new(&placement, change->definition_index, NONE);
	placement.owner_player_index = distributed_player_from_byte(change->owner_player_index);
	placement.owner_team_index = change->owner_team_index;
	placement.variant_number = change->variant_number;
	placement.position = change->position;
	placement.forward = change->forward;
	placement.up = change->up;
	placement.translational_velocity = change->translational_velocity;
	placement.angular_velocity = change->angular_velocity;
	csmemcpy(placement.change_colors, change->change_colors, sizeof(placement.change_colors));
	objects_client_creating_index = change->object_index;
	objects_client_creating = TRUE;
	object_index = object_new(&placement);
	objects_client_creating = FALSE;
	objects_client_creating_index = NONE;
	objects_statistics.creates++;
	if (object_index != change->object_index)
	{
		objects_statistics.create_failures++;
		if (object_index != NONE)
		{
			objects_client_deleting = TRUE;
			object_delete_immediately(object_index);
			objects_client_deleting = FALSE;
		}
		return;
	}
	objects_client_has[absolute_index] = object_index;
	{
		struct object_datum *object = object_get(object_index);

		csmemcpy(object->object.base_change_colors, change->change_colors, sizeof(object->object.base_change_colors));
		csmemcpy(object->object.region_permutations, change->region_permutations,
			sizeof(object->object.region_permutations));
		object_set_garbage(object_index, FALSE);
		/* (in a unit's inventory: the next inventories put it there) */
		if (TEST_FLAG(change->flags, _distributed_object_carried_bit) &&
			TEST_FLAG(object->object.flags, _object_connected_to_map_bit))
		{
			object_disconnect_from_map(object_index);
			object_set_visibility(object_index, FALSE);
		}
	}
}

void network_objects_handle_changes(
	void const *entries,
	short count)
{
	struct distributed_object_change const *changes = (struct distributed_object_change const *)entries;
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_object_change const *change = &changes[index];
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(change->object_index);

		if (absolute_index < 0 || absolute_index >= MAXIMUM_TRACKED_OBJECTS)
			continue;
		if (change->change == _object_change_create)
		{
			distributed_client_create(change);
		}
		else if (objects_client_has[absolute_index] == change->object_index)
		{
			objects_statistics.deletes++;
			distributed_client_delete(change->object_index);
			objects_client_has[absolute_index] = NONE;
		}
	}
}

void network_objects_handle_synchronized(
	void)
{
	objects_client_synchronized = TRUE;
}

void network_objects_handle_states(
	void const *entries,
	short count)
{
	struct distributed_object_state const *states = (struct distributed_object_state const *)entries;
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_object_state const *state = &states[index];
		struct object_datum *object;
		real tolerance = REMOTE_OBJECT_TOLERANCE;
		real dx, dy, dz;

		if (!network_objects_client_has(state->object_index))
			continue;
		object = object_get(state->object_index);
		if (object->object.parent_object_index != NONE ||
			!TEST_FLAG(object->object.flags, _object_connected_to_map_bit))
		{
			continue;
		}
		/* (a vehicle this machine's own player drives is its own) */
		if (object->object.type == _object_type_vehicle)
		{
			struct unit_datum *vehicle = unit_get(state->object_index);

			if (vehicle->unit.driver_object_index != NONE &&
				distributed_player_is_local(unit_get(vehicle->unit.driver_object_index)->unit.player_index))
			{
				tolerance = LOCAL_VEHICLE_TOLERANCE;
			}
		}
		dx = state->position.x - object->object.position.x;
		dy = state->position.y - object->object.position.y;
		dz = state->position.z - object->object.position.z;
		if (dx * dx + dy * dy + dz * dz <= tolerance * tolerance)
			continue;
		distributed_count_correction();
		network_objects_correct(state->object_index, &state->position, &state->forward, &state->up,
			&state->translational_velocity, &state->angular_velocity);
		SET_FLAG(object->object.flags, _object_at_rest_bit, TEST_FLAG(state->flags, _distributed_object_at_rest_bit));
	}
}

/* the unit carries what the host's does */
static void distributed_client_apply_inventory(
	struct distributed_inventory const *inventory)
{
	struct unit_datum *unit;
	boolean local;
	short weapon_slot;

	if (!network_objects_client_has(inventory->unit_index) ||
		!object_try_and_get_and_verify_type(inventory->unit_index, _object_mask_unit))
	{
		return;
	}
	unit = unit_get(inventory->unit_index);
	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	local = distributed_player_is_local(unit->unit.player_index);
	/* picked up, swapped or dropped on the host: the same weapons here,
	slot for slot (the host's own objects, moved in and out as
	unit_add_weapon_to_inventory and unit_drop_current_weapon do, without
	their rules: the host has applied them) */
	for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
	{
		long wanted_index = inventory->weapon_indices[weapon_slot];
		long weapon_index = unit->unit.weapon_object_indices[weapon_slot];

		if (weapon_index == wanted_index)
			continue;
		/* (not here yet: as it is, until it is) */
		if (wanted_index != NONE &&
			(!network_objects_client_has(wanted_index) || !object_try_and_get_and_verify_type(wanted_index, _object_mask_weapon)))
		{
			continue;
		}
		if (weapon_index != NONE)
			unit_network_drop_weapon(inventory->unit_index, weapon_slot);
		if (wanted_index != NONE)
		{
			distributed_client_release_item(wanted_index, FALSE);
			unit_network_add_weapon(inventory->unit_index, wanted_index, weapon_slot);
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

		if (!weapon || weapon_index != inventory->weapon_indices[weapon_slot])
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

void network_objects_handle_inventories(
	void const *entries,
	short count)
{
	struct distributed_inventory const *inventories = (struct distributed_inventory const *)entries;
	short index;

	for (index = 0; index < count; index++)
		distributed_client_apply_inventory(&inventories[index]);
}

void network_objects_set_seat(
	long unit_index,
	long vehicle_index,
	short seat_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	long occupant_index = NONE;

	if (vehicle_index != NONE &&
		(!network_objects_client_has(vehicle_index) ||
			!object_try_and_get_and_verify_type(vehicle_index, _object_mask_unit)))
	{
		return;
	}
	if (unit->object.parent_object_index != NONE && unit->unit.parent_seat_index != NONE)
		unit_exit_seat_end(unit_index);
	if (vehicle_index == NONE || unit->object.parent_object_index != NONE)
		return;
	/* (whoever this machine still has in the seat: out) */
	unit_can_enter_seat(unit_index, vehicle_index, seat_index, &occupant_index);
	if (occupant_index != NONE && occupant_index != unit_index)
		unit_exit_seat_end(occupant_index);
	unit_enter_seat(unit_index, vehicle_index, seat_index);
}

/* the vehicles its own players drive, to the host */
static void distributed_client_send_vehicles(
	void)
{
	struct distributed_object_state_message message;
	struct data_iterator iterator;
	struct player_datum *player;
	short count = 0;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL && count < MAXIMUM_LOCAL_PLAYERS)
	{
		long vehicle_index = player->local_player_index != NONE ? distributed_driven_vehicle(player) : NONE;

		if (vehicle_index != NONE && network_objects_client_has(vehicle_index) &&
			object_get(vehicle_index)->object.parent_object_index == NONE)
		{
			distributed_state_from_object(vehicle_index, &message.states[count++]);
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_vehicle_prediction, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_object_state)), _distributed_to_host);
	}
}

/* the objects of the kinds the host has that it has not told of go (what
this machine's own simulation made) */
static void distributed_client_remove_own_objects(
	void)
{
	struct object_iterator iterator;

	object_iterator_new(&iterator, NETWORKED_OBJECT_TYPES, 0);
	while (object_iterator_next(&iterator))
	{
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index);

		if (!distributed_object_networked(iterator.index) || objects_client_has[absolute_index] == iterator.index)
			continue;
		objects_statistics.own_objects_removed++;
		distributed_client_delete(iterator.index);
	}
}

void network_objects_client_tick(
	void)
{
	/* (loaded: the host's objects, please) */
	if (!objects_client_synchronized &&
		(objects_client_ready_time == NONE || game_time_get() - objects_client_ready_time >= CLIENT_READY_INTERVAL_TICKS))
	{
		struct distributed_message_header message;

		objects_client_ready_time = game_time_get();
		distributed_send(&message, _distributed_message_client_ready, 0, (word)sizeof(message),
			_distributed_to_host_reliably);
	}
	if (objects_client_synchronized)
		distributed_client_remove_own_objects();
	distributed_client_send_vehicles();
}

/* ---------- the game */

void network_objects_new_game(
	void)
{
	long absolute_index;

	for (absolute_index = 0; absolute_index < MAXIMUM_TRACKED_OBJECTS; absolute_index++)
	{
		objects_host_told[absolute_index] = NONE;
		objects_client_has[absolute_index] = NONE;
	}
	objects_host_resting_cursor = 0;
	objects_client_synchronized = FALSE;
	objects_client_ready_time = NONE;
	objects_client_local_allocation = FALSE;
	objects_client_creating_index = NONE;
	objects_client_creating = FALSE;
	objects_client_deleting = FALSE;
}
