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
  bodies), and a few of those at rest, round the lot; a client moves its
  copies there, a little off half of the way each tick, further drawn
  gliding from where they were rather than jumping (render_interpolation.c).
  A client's own player's vehicle is its own, as its own player's unit is:
  it sends the host where it drives it, which the host takes within a
  tolerance at its next tick, and puts it where the host has it only when
  far off.
- Ten times a second, the host sends what a unit carries when that has
  changed (and once a second whatever it is): which weapons, slot for slot,
  their ammunition, the weapon in hand and the grenades; a client moves its
  copies of those weapons in and out of its copies of the units to match (it
  decides no pickups, swaps or drops itself).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "objects/object_definitions.h"
#include "models/model_definitions.h"
#include "units/units.h"
#include "units/biped_definitions.h"
#include "units/vehicle_definitions.h"
#include "items/items.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/equipment_definitions.h"
#include "network_distributed.h"

#include <math.h>

/* units.c's */
void unit_network_add_weapon(long unit_index, long weapon_index, short slot);
void unit_network_drop_weapon(long unit_index, short slot);
void unit_network_forget_weapon(long unit_index, short slot);
/* players.c's */
void network_player_detach_unit(long player_index);
/* render_interpolation.c's */
void render_interpolation_correct_object(long object_index, real_vector3d const *offset);
/* cache_files.c's */
boolean tag_index_is_group(long tag_index, long group_tag);

enum
{
	INVENTORY_INTERVAL_TICKS = 3,
	/* a unit's inventory is sent this often even unchanged (one lost is
	made good) */
	INVENTORY_REFRESH_TICKS = TICKS_PER_SECOND,
	/* ... and a unit's that has come to carry nothing, for this long after */
	EMPTY_INVENTORY_TICKS = 3 * INVENTORY_REFRESH_TICKS,
	/* the host's objects at rest sent each tick, round them all */
	RESTING_STATES_PER_TICK = 4,
	/* a client asks for the host's objects again until it has them (a host
	still loading misses the asking), after this, twice as long each time
	up to the most */
	CLIENT_READY_INTERVAL_TICKS = TICKS_PER_SECOND,
	CLIENT_READY_MAXIMUM_INTERVAL_TICKS = 4 * TICKS_PER_SECOND,
	/* the host tells a machine all its objects at most once in this long
	(asked again sooner, they are on their way) */
	HOST_OBJECTS_RESEND_TICKS = 10 * TICKS_PER_SECOND,
	/* a client that failed to make one of the host's objects asks for them
	all again this long after (past the host's), twice as long each time it
	fails again up to the most */
	CLIENT_RETRY_TICKS = HOST_OBJECTS_RESEND_TICKS + TICKS_PER_SECOND,
	CLIENT_RETRY_MAXIMUM_TICKS = 60 * TICKS_PER_SECOND,
	/* a client's own objects, from here up */
	LOCAL_OBJECTS_FIRST_INDEX = MAXIMUM_OBJECTS_PER_MAP / 2,
	MAXIMUM_ENTRIES_PER_MESSAGE = 64,
	/* the objects the host has at the same indices everywhere */
	NETWORKED_OBJECT_TYPES =
		_object_mask_biped | _object_mask_vehicle | _object_mask_weapon | _object_mask_equipment,
};

/* world units */
#define REMOTE_OBJECT_TOLERANCE 0.05f
/* ... and the cosine of the angle (an object at rest turned) */
#define REMOTE_OBJECT_ANGLE_TOLERANCE 0.98f
#define LOCAL_VEHICLE_TOLERANCE 4.0f
/* (the host takes further than a client puts right: between the two they
would disagree for good) */
#define HOST_VEHICLE_ACCEPT_TOLERANCE 5.0f
/* how far from the origin an object is (world units) */
#define OBJECT_WORLD_BOUND 32768.0f
/* how fast a client's own player's vehicle moves (world units, radians a
tick) */
#define MAXIMUM_PREDICTED_VEHICLE_SPEED 3.0f
#define MAXIMUM_PREDICTED_VEHICLE_ANGULAR_SPEED 1.0f
/* the teams a flag's or ball's owner team names (game_engine_ctf.c's
NUMBER_OF_CTF_TEAMS, game_engine_oddball.c's MAXIMUM_ODDBALLS) */
#define CTF_FLAG_TEAMS 2
#define ODDBALL_TEAMS 16

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
	/* a unit dead (struct distributed_object_change) */
	_distributed_object_dead_bit,
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
	struct distributed_vector forward;
	struct distributed_vector up;
	struct distributed_vector translational_velocity;
	struct distributed_vector angular_velocity;
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
/* ... the next object at rest to send, round them all */
static long objects_host_resting_cursor;
/* ... past the highest of them */
static long objects_host_told_count;
/* ... what each unit's inventory was last sent as, and when; when it last
carried anything (NONE: never) */
static struct
{
	unsigned long checksum;
	long time;
	long carried_time;
} objects_host_inventories[MAXIMUM_TRACKED_OBJECTS];
/* ... each machine it has told all its objects: when, and which players it
had then (another machine in its place has others) */
static struct
{
	long time;
	long player_indices[MAXIMUM_LOCAL_PLAYERS];
} objects_host_machines[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
/* ... each client's player's latest vehicle prediction, taken at the next
tick */
static struct
{
	boolean valid;
	long machine_index;
	struct distributed_object_state state;
} objects_host_vehicle_predictions[MAXIMUM_TRACKED_PLAYERS];
/* a client: the host's objects it has, by absolute index */
static long objects_client_has[MAXIMUM_TRACKED_OBJECTS];
/* ... all of them (the host said so), when it last asked for them and how
long until it asks again */
static boolean objects_client_synchronized;
static long objects_client_ready_time;
static long objects_client_ready_interval;
/* ... when it last failed to make one (NONE: not since the host last told
it all), and how long it waits after the next failure */
static long objects_client_failed_time;
static long objects_client_retry_ticks;
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

static boolean distributed_vector_valid(
	real_vector3d const *vector)
{
	return distributed_real_valid(vector->i) && distributed_real_valid(vector->j) && distributed_real_valid(vector->k);
}

/* whether a message's transform can be: the position finite and within the
world, the axes a rotation (made exactly one in valid_forward, valid_up),
the velocities finite */
static boolean distributed_transform_valid(
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *velocity,
	real_vector3d const *angular_velocity,
	real_vector3d *valid_forward,
	real_vector3d *valid_up)
{
	if (!position || !forward || !up || !distributed_point_valid(position, OBJECT_WORLD_BOUND) ||
		(velocity && !distributed_vector_valid(velocity)) ||
		(angular_velocity && !distributed_vector_valid(angular_velocity)))
	{
		return FALSE;
	}
	*valid_forward = *forward;
	*valid_up = *up;
	return distributed_axes_make_valid(valid_forward, valid_up);
}

/* the vector no longer than maximum */
static void distributed_vector_clamp(
	real_vector3d *vector,
	real maximum)
{
	real length = (real)sqrt(vector->i * vector->i + vector->j * vector->j + vector->k * vector->k);

	if (length > maximum)
	{
		vector->i *= maximum / length;
		vector->j *= maximum / length;
		vector->k *= maximum / length;
	}
}

/* (the transform checked) */
static void distributed_object_move(
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

void network_objects_correct(
	long object_index,
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *velocity,
	real_vector3d const *angular_velocity)
{
	real_vector3d valid_forward, valid_up;

	/* (what cannot be, from a message: not taken) */
	if (distributed_transform_valid(position, forward, up, velocity, angular_velocity, &valid_forward, &valid_up))
		distributed_object_move(object_index, position, &valid_forward, &valid_up, velocity, angular_velocity);
}

boolean network_objects_reconcile(
	long object_index,
	real_point3d const *position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *velocity,
	real_vector3d const *angular_velocity,
	real blend_distance)
{
	struct object_datum *object = object_get(object_index);
	real_vector3d valid_forward, valid_up;
	real_point3d blended;
	real dx, dy, dz;

	/* (what cannot be, from a message: not taken) */
	if (!distributed_transform_valid(position, forward, up, velocity, angular_velocity, &valid_forward, &valid_up))
		return FALSE;
	dx = position->x - object->object.position.x;
	dy = position->y - object->object.position.y;
	dz = position->z - object->object.position.z;
	/* (so written that its own position gone wrong, not a number, is put
	right) */
	if (!(dx * dx + dy * dy + dz * dz <= blend_distance * blend_distance))
	{
		distributed_object_move(object_index, position, &valid_forward, &valid_up, velocity, angular_velocity);
		return TRUE;
	}
	/* (half of the way: the tick's snapshots draw it moving, no jump) */
	blended.x = object->object.position.x + dx * 0.5f;
	blended.y = object->object.position.y + dy * 0.5f;
	blended.z = object->object.position.z + dz * 0.5f;
	object_set_position(object_index, &blended, &valid_forward, &valid_up);
	if (velocity)
		object->object.translational_velocity = *velocity;
	if (angular_velocity)
		object->object.angular_velocity = *angular_velocity;
	return FALSE;
}

/* an object's state as the host sent it, in full */
static void distributed_object_state_unpack(
	struct distributed_object_state const *state,
	real_vector3d *forward,
	real_vector3d *up,
	real_vector3d *velocity,
	real_vector3d *angular_velocity)
{
	/* (as they came: distributed_transform_valid makes them a rotation or
	refuses them) */
	distributed_vector_unpack(&state->forward, DISTRIBUTED_UNIT_SCALE, forward);
	distributed_vector_unpack(&state->up, DISTRIBUTED_UNIT_SCALE, up);
	distributed_vector_unpack(&state->translational_velocity, DISTRIBUTED_VELOCITY_SCALE, velocity);
	distributed_vector_unpack(&state->angular_velocity, DISTRIBUTED_ANGULAR_VELOCITY_SCALE, angular_velocity);
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
	distributed_vector_pack(&object->object.forward, DISTRIBUTED_UNIT_SCALE, &state->forward);
	distributed_vector_pack(&object->object.up, DISTRIBUTED_UNIT_SCALE, &state->up);
	distributed_vector_pack(&object->object.translational_velocity, DISTRIBUTED_VELOCITY_SCALE,
		&state->translational_velocity);
	distributed_vector_pack(&object->object.angular_velocity, DISTRIBUTED_ANGULAR_VELOCITY_SCALE,
		&state->angular_velocity);
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
	SET_FLAG(change->flags, _distributed_object_dead_bit, TEST_FLAG(_object_mask_unit, object->object.type) &&
		TEST_FLAG(object->object.damage_flags, _object_dead_bit));
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

	/* (none set past those told of) */
	csmemset(seen, 0, objects_host_told_count * sizeof(seen[0]));
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
		if (absolute_index >= objects_host_told_count)
			objects_host_told_count = absolute_index + 1;
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
	for (absolute_index = 0; absolute_index < objects_host_told_count; absolute_index++)
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
and word that that is all of them (asked again soon after, only the word:
they are on their way ahead of it) */
void network_objects_client_ready(
	long machine_index)
{
	struct distributed_object_change_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, RELIABLE_ENTRIES(struct distributed_object_change));
	long absolute_index;
	long *player_list;

	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return;
	player_list = machine_get_player_list(machine_index);
	if (objects_host_machines[machine_index].time == NONE ||
		game_time_get() - objects_host_machines[machine_index].time >= HOST_OBJECTS_RESEND_TICKS ||
		csmemcmp(objects_host_machines[machine_index].player_indices, player_list,
			sizeof(objects_host_machines[machine_index].player_indices)) != 0)
	{
		objects_host_machines[machine_index].time = game_time_get();
		csmemcpy(objects_host_machines[machine_index].player_indices, player_list,
			sizeof(objects_host_machines[machine_index].player_indices));
	}
	else
	{
		distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_objects_synchronized, 0,
			(word)sizeof(message.header));
		return;
	}
	distributed_host_update_objects();
	for (absolute_index = 0; absolute_index < objects_host_told_count; absolute_index++)
	{
		/* (what every unit carries, with the next of them) */
		objects_host_inventories[absolute_index].checksum = 0;
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

/* the host's object at an absolute index whose state goes to its clients
(not players' living units, which have their own messages, nor what is
attached or carried), NONE for none */
static long distributed_host_placed_object(
	long absolute_index)
{
	long object_index = objects_host_told[absolute_index];
	struct object_datum *object;

	if (object_index == NONE || !object_try_and_get(object_index))
		return NONE;
	object = object_get(object_index);
	if (object->object.parent_object_index != NONE || !TEST_FLAG(object->object.flags, _object_connected_to_map_bit))
		return NONE;
	/* (out of the map, where nobody sees it: a body fallen through the
	ground falls on for good, ever faster, never at rest) */
	if (TEST_FLAG(object->object.flags, _object_outside_of_map_bit))
		return NONE;
	if (TEST_FLAG(_object_mask_unit, object->object.type) && unit_get(object_index)->unit.player_index != NONE &&
		!TEST_FLAG(object->object.damage_flags, _object_dead_bit))
	{
		return NONE;
	}
	return object_index;
}

/* where the moving objects are, and a few at rest, round them all (one
whose last move was lost is put right when its turn comes) */
static void distributed_host_send_states(
	void)
{
	struct distributed_object_state_message message;
	short count = 0;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_object_state));
	short resting = 0;
	long told_count = objects_host_told_count;
	long absolute_index;
	long step;

	for (step = 0; step < 2 * told_count; step++)
	{
		long object_index;
		boolean at_rest;

		/* (the first pass the moving ones, the second those at rest from
		the cursor on, round to it) */
		if (step < told_count)
			absolute_index = step;
		else if (resting < RESTING_STATES_PER_TICK)
			absolute_index = (objects_host_resting_cursor + step - told_count) % told_count;
		else
			break;
		object_index = distributed_host_placed_object(absolute_index);
		if (object_index == NONE)
			continue;
		at_rest = TEST_FLAG(object_get(object_index)->object.flags, _object_at_rest_bit);
		if (step < told_count ? at_rest : !at_rest)
			continue;
		if (step >= told_count)
		{
			resting++;
			objects_host_resting_cursor = (absolute_index + 1) % told_count;
		}
		distributed_state_from_object(object_index, &message.states[count]);
		if (++count == limit)
		{
			distributed_send(&message, _distributed_message_object_states, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_object_state)),
				_distributed_to_clients);
			count = 0;
		}
	}
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

/* the bytes' checksum (FNV-1a) */
static unsigned long distributed_checksum(
	void const *data,
	long size)
{
	byte const *bytes = (byte const *)data;
	unsigned long checksum = 2166136261UL;
	long index;

	for (index = 0; index < size; index++)
		checksum = (checksum ^ bytes[index]) * 16777619UL;
	return checksum;
}

/* what every unit carries, when that has changed or not been sent for a
while */
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
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index);
		short weapon_slot;
		boolean carries = unit->unit.grenade_counts[0] || unit->unit.grenade_counts[1];

		for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
			carries |= unit->unit.weapon_object_indices[weapon_slot] != NONE;
		if (!distributed_object_networked(iterator.index) || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		/* (one carrying nothing, as it has come to for a while: its last
		weapon out of its hands everywhere, and then no more of it) */
		if (carries)
			objects_host_inventories[absolute_index].carried_time = game_time_get();
		else if (objects_host_inventories[absolute_index].carried_time == NONE ||
			game_time_get() - objects_host_inventories[absolute_index].carried_time >= EMPTY_INVENTORY_TICKS)
		{
			continue;
		}
		distributed_inventory_from_unit(iterator.index, &message.inventories[count]);
		{
			unsigned long checksum = distributed_checksum(&message.inventories[count], sizeof(struct distributed_inventory));

			if (objects_host_inventories[absolute_index].checksum == checksum &&
				game_time_get() - objects_host_inventories[absolute_index].time < INVENTORY_REFRESH_TICKS)
			{
				continue;
			}
			objects_host_inventories[absolute_index].checksum = checksum;
			objects_host_inventories[absolute_index].time = game_time_get();
		}
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

/* (the host) the vehicles a client's own players drive: the latest of
each, taken at the next tick */
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
		struct unit_datum *vehicle = distributed_object_index_valid(state->object_index) ?
			(struct unit_datum *)object_try_and_get_and_verify_type(state->object_index, _object_mask_vehicle) : NULL;
		short player_index;

		if (!vehicle || vehicle->unit.driver_object_index == NONE)
			continue;
		if (unit_get(vehicle->unit.driver_object_index)->unit.player_index == NONE)
			continue;
		player_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_get(vehicle->unit.driver_object_index)->unit.player_index);
		if (player_index < 0 || player_index >= MAXIMUM_TRACKED_PLAYERS ||
			!distributed_machine_has_player(machine_index, player_index))
		{
			continue;
		}
		objects_host_vehicle_predictions[player_index].valid = TRUE;
		objects_host_vehicle_predictions[player_index].machine_index = machine_index;
		objects_host_vehicle_predictions[player_index].state = *state;
	}
}

/* ... taken as they are, within a tolerance, if that machine's player
still drives it: a little off closed by half, more put there */
void network_objects_apply_vehicle_predictions(
	void)
{
	short player_index;

	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		struct distributed_object_state const *state = &objects_host_vehicle_predictions[player_index].state;
		struct unit_datum *vehicle;
		struct unit_datum *driver;
		real_vector3d forward, up, velocity, angular_velocity;
		real dx, dy, dz;

		if (!objects_host_vehicle_predictions[player_index].valid)
			continue;
		objects_host_vehicle_predictions[player_index].valid = FALSE;
		vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(state->object_index, _object_mask_vehicle);
		if (!vehicle || vehicle->unit.driver_object_index == NONE || vehicle->object.parent_object_index != NONE)
			continue;
		driver = unit_get(vehicle->unit.driver_object_index);
		if (driver->unit.player_index == NONE ||
			DATUM_INDEX_TO_ABSOLUTE_INDEX(driver->unit.player_index) != player_index ||
			!distributed_machine_has_player(objects_host_vehicle_predictions[player_index].machine_index, player_index))
		{
			continue;
		}
		dx = state->position.x - vehicle->object.position.x;
		dy = state->position.y - vehicle->object.position.y;
		dz = state->position.z - vehicle->object.position.z;
		/* (so written that a position not a number is not taken) */
		if (!(dx * dx + dy * dy + dz * dz <= HOST_VEHICLE_ACCEPT_TOLERANCE * HOST_VEHICLE_ACCEPT_TOLERANCE))
			continue;
		distributed_object_state_unpack(state, &forward, &up, &velocity, &angular_velocity);
		/* (no faster than a vehicle goes) */
		distributed_vector_clamp(&velocity, MAXIMUM_PREDICTED_VEHICLE_SPEED);
		distributed_vector_clamp(&angular_velocity, MAXIMUM_PREDICTED_VEHICLE_ANGULAR_SPEED);
		network_objects_reconcile(state->object_index, &state->position, &forward, &up, &velocity, &angular_velocity,
			HOST_VEHICLE_BLEND_DISTANCE);
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
	/* (a player's action on it, the prompt the hud draws: deleted between
	ticks, it would draw on with the object gone until the next tick looked
	again) */
	{
		struct data_iterator iterator;
		struct player_datum *player;

		data_iterator_new(&iterator, player_data);
		while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		{
			if (player->action_object_index == object_index)
			{
				player->action_result = _player_action_result_reload;
				player->action_object_index = NONE;
			}
		}
	}
	objects_client_deleting = TRUE;
	object_delete_immediately(object_index);
	objects_client_deleting = FALSE;
}

/* a create failed (the object pool full, its place not to be had): all of
them again from the host, a while after (it tells a machine all of them at
most once in HOST_OBJECTS_RESEND_TICKS), longer each time it fails again */
static void distributed_client_create_failed(
	void)
{
	objects_statistics.create_failures++;
	/* (not asked since the last: that asking covers this one) */
	if (objects_client_failed_time != NONE && objects_client_ready_time <= objects_client_failed_time)
		return;
	objects_client_failed_time = game_time_get();
	objects_client_synchronized = FALSE;
	objects_client_ready_time = game_time_get();
	objects_client_ready_interval = objects_client_retry_ticks;
	objects_client_retry_ticks = MIN(2 * objects_client_retry_ticks, CLIENT_RETRY_MAXIMUM_TICKS);
}

/* whether the object is this machine's own player's unit or the vehicle it
drives (where they are is its own) */
static boolean distributed_client_own_object(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	long unit_index = object_index;

	if (!TEST_FLAG(_object_mask_unit, object->object.type))
		return FALSE;
	if (object->object.type == _object_type_vehicle)
		unit_index = unit_get(object_index)->unit.driver_object_index;
	return unit_index != NONE && distributed_player_is_local(unit_get(unit_index)->unit.player_index);
}

/* whether the host's object can be made: of the kinds the host has, its
definition a tag of the group its type says, at a whole index, of a team
the game has, where the world is (its axes made a rotation in forward, up) */
static boolean distributed_client_change_valid(
	struct distributed_object_change const *change,
	real_vector3d *forward,
	real_vector3d *up)
{
	long group_tag;
	short type;
	short team_count = MAXIMUM_TRACKED_PLAYERS;

	if (!distributed_object_index_valid(change->object_index) ||
		!tag_index_is_group(change->definition_index, OBJECT_DEFINITION_TAG))
	{
		return FALSE;
	}
	type = object_definition_get(change->definition_index)->object.type;
	switch (type)
	{
	case _object_type_biped: group_tag = BIPED_DEFINITION_TAG; break;
	case _object_type_vehicle: group_tag = VEHICLE_DEFINITION_TAG; break;
	case _object_type_weapon: group_tag = WEAPON_DEFINITION_TAG; break;
	case _object_type_equipment: group_tag = EQUIPMENT_DEFINITION_TAG; break;
	default: return FALSE;
	}
	if (!TEST_FLAG(NETWORKED_OBJECT_TYPES, type) || !tag_index_is_group(change->definition_index, group_tag))
		return FALSE;
	/* (a flag's or ball's team names the game type's flag or ball: an index
	of its arrays; weapon_is_flag's bit) */
	if (type == _object_type_weapon && ((weapon_definition_get(change->definition_index)->weapon.flags >> 3) & 1))
		team_count = game_engine_get_variant()->game_engine_index == game_engine_ctf ? CTF_FLAG_TEAMS : ODDBALL_TEAMS;
	if (change->owner_team_index != NONE && (change->owner_team_index < 0 || change->owner_team_index >= team_count))
		return FALSE;
	return distributed_transform_valid(&change->position, &change->forward, &change->up,
		&change->translational_velocity, &change->angular_velocity, forward, up);
}

/* the host's word on an object made or found here: how it looks, whether
carried, whether dead */
static void distributed_client_apply_change(
	long object_index,
	struct distributed_object_change const *change)
{
	struct object_datum *object = object_get(object_index);

	csmemcpy(object->object.base_change_colors, change->change_colors, sizeof(object->object.base_change_colors));
	/* (a permutation the model has, or none: the model's renderer takes it
as it is) */
	{
		long model_index = object_definition_get(object->definition_index)->object.model.index;
		struct model *model = model_index != NONE ? model_definition_get(model_index) : NULL;
		short region_index;

		for (region_index = 0; model && region_index < model->regions.count &&
			region_index < MAXIMUM_REGIONS_PER_OBJECT; region_index++)
		{
			struct model_region *region = TAG_BLOCK_GET_ELEMENT(&model->regions, region_index, struct model_region);
			byte permutation_index = change->region_permutations[region_index];

			if (permutation_index == (byte)NONE || permutation_index < region->permutations.count)
				object->object.region_permutations[region_index] = permutation_index;
		}
	}
	object_set_garbage(object_index, FALSE);
	/* (in a unit's inventory: the next inventories put it there) */
	if (TEST_FLAG(change->flags, _distributed_object_carried_bit) &&
		TEST_FLAG(object->object.flags, _object_connected_to_map_bit) &&
		object->object.parent_object_index == NONE)
	{
		object_disconnect_from_map(object_index);
		object_set_visibility(object_index, FALSE);
	}
	/* (a body: dead on the host before this machine had it, killed here
	with nothing to show or count of it) */
	if (TEST_FLAG(change->flags, _distributed_object_dead_bit) && TEST_FLAG(_object_mask_unit, object->object.type) &&
		!TEST_FLAG(object->object.damage_flags, _object_dead_bit))
	{
		unit_kill_silent(object_index);
		unit_kill_no_statistics(object_index);
		/* (at once: a body where no player is may not be updated for long) */
		object_damage_update(object_index);
	}
}

static void distributed_client_create(
	struct distributed_object_change const *change)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(change->object_index);
	struct datum_header const *header;
	struct object_placement_data placement;
	real_vector3d forward, up;
	long object_index;

	if (absolute_index < 0 || absolute_index >= MAXIMUM_TRACKED_OBJECTS ||
		absolute_index >= object_header_data->maximum_count ||
		!distributed_client_change_valid(change, &forward, &up))
	{
		objects_statistics.create_failures++;
		return;
	}
	header = (struct datum_header const *)((byte const *)object_header_data->data +
		absolute_index * object_header_data->size);
	if (header->identifier)
	{
		long existing_index = ((long)header->identifier << 16) | absolute_index;
		struct object_datum *existing = object_try_and_get(existing_index);

		/* placed here when the map loaded, as on the host, or told of
		before: the same object, where the host has it (unless carried, or
		this machine's own) */
		if (existing_index == change->object_index && existing &&
			existing->definition_index == change->definition_index)
		{
			objects_client_has[absolute_index] = existing_index;
			if (!TEST_FLAG(change->flags, _distributed_object_carried_bit) &&
				existing->object.parent_object_index == NONE &&
				TEST_FLAG(existing->object.flags, _object_connected_to_map_bit) &&
				!distributed_client_own_object(existing_index))
			{
				distributed_object_move(existing_index, &change->position, &forward, &up,
					&change->translational_velocity, &change->angular_velocity);
			}
			distributed_client_apply_change(existing_index, change);
			return;
		}
		/* something else in its place: gone */
		distributed_client_delete(existing_index);
		objects_client_has[absolute_index] = NONE;
		if (header->identifier)
		{
			distributed_client_create_failed();
			return;
		}
	}
	object_placement_data_new(&placement, change->definition_index, NONE);
	placement.owner_player_index = distributed_player_from_byte(change->owner_player_index);
	placement.owner_team_index = change->owner_team_index;
	placement.variant_number = change->variant_number;
	placement.position = change->position;
	placement.forward = forward;
	placement.up = up;
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
		if (object_index != NONE)
		{
			objects_client_deleting = TRUE;
			object_delete_immediately(object_index);
			objects_client_deleting = FALSE;
		}
		distributed_client_create_failed();
		return;
	}
	objects_client_has[absolute_index] = object_index;
	distributed_client_apply_change(object_index, change);
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

		if (!distributed_object_index_valid(change->object_index) ||
			absolute_index < 0 || absolute_index >= MAXIMUM_TRACKED_OBJECTS)
		{
			continue;
		}
		if (change->change == _object_change_create)
		{
			distributed_client_create(change);
		}
		else if (change->change == _object_change_delete &&
			objects_client_has[absolute_index] == change->object_index)
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
	/* (a create failed since this machine last asked: not all of them, it
	asks again) */
	if (objects_client_failed_time != NONE)
	{
		if (objects_client_ready_time <= objects_client_failed_time)
			return;
		objects_client_failed_time = NONE;
		objects_client_retry_ticks = CLIENT_RETRY_TICKS;
	}
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
		real angle_tolerance = REMOTE_OBJECT_ANGLE_TOLERANCE;
		real blend_distance = REMOTE_BLEND_DISTANCE;
		real_vector3d forward, up, velocity, angular_velocity;
		real dx, dy, dz;

		if (!distributed_object_index_valid(state->object_index) || !network_objects_client_has(state->object_index))
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

			blend_distance = REMOTE_VEHICLE_BLEND_DISTANCE;
			if (vehicle->unit.driver_object_index != NONE &&
				distributed_player_is_local(unit_get(vehicle->unit.driver_object_index)->unit.player_index))
			{
				tolerance = LOCAL_VEHICLE_TOLERANCE;
				/* (turned as it drives it) */
				angle_tolerance = -1.0f;
				blend_distance = 0.0f;
			}
		}
		distributed_object_state_unpack(state, &forward, &up, &velocity, &angular_velocity);
		if (!distributed_transform_valid(&state->position, &forward, &up, NULL, NULL, &forward, &up))
			continue;
		dx = state->position.x - object->object.position.x;
		dy = state->position.y - object->object.position.y;
		dz = state->position.z - object->object.position.z;
		/* (close enough where it is, and turned as it is: one at rest too) */
		if (dx * dx + dy * dy + dz * dz <= tolerance * tolerance &&
			forward.i * object->object.forward.i + forward.j * object->object.forward.j +
				forward.k * object->object.forward.k >= angle_tolerance &&
			up.i * object->object.up.i + up.j * object->object.up.j + up.k * object->object.up.k >= angle_tolerance)
		{
			continue;
		}
		if (network_objects_reconcile(state->object_index, &state->position, &forward, &up, &velocity,
			&angular_velocity, blend_distance))
		{
			distributed_count_correction();
		}
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
	struct unit_datum *unit = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	long occupant_index = NONE;

	if (!unit)
		return;
	/* (a seat the vehicle has) */
	if (vehicle_index != NONE)
	{
		struct unit_datum *vehicle;

		if (vehicle_index == unit_index || !distributed_object_index_valid(vehicle_index) ||
			!network_objects_client_has(vehicle_index))
		{
			return;
		}
		vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle);
		if (!vehicle || seat_index < 0 ||
			seat_index >= unit_definition_get(vehicle->definition_index)->unit.seats.count)
		{
			return;
		}
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
		(objects_client_ready_time == NONE || game_time_get() - objects_client_ready_time >= objects_client_ready_interval))
	{
		struct distributed_message_header message;

		/* (asked, less often the longer the host takes: a slow link does
		not queue up the asking) */
		if (objects_client_ready_time != NONE)
			objects_client_ready_interval = MIN(2 * objects_client_ready_interval, CLIENT_READY_MAXIMUM_INTERVAL_TICKS);
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

	short machine_index;

	csmemset(objects_host_inventories, 0, sizeof(objects_host_inventories));
	for (absolute_index = 0; absolute_index < MAXIMUM_TRACKED_OBJECTS; absolute_index++)
	{
		objects_host_told[absolute_index] = NONE;
		objects_host_inventories[absolute_index].carried_time = NONE;
		objects_client_has[absolute_index] = NONE;
	}
	objects_host_told_count = 0;
	objects_host_resting_cursor = 0;
	for (machine_index = 0; machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES; machine_index++)
		objects_host_machines[machine_index].time = NONE;
	csmemset(objects_host_vehicle_predictions, 0, sizeof(objects_host_vehicle_predictions));
	objects_client_synchronized = FALSE;
	objects_client_ready_time = NONE;
	objects_client_ready_interval = CLIENT_READY_INTERVAL_TICKS;
	objects_client_failed_time = NONE;
	objects_client_retry_ticks = CLIENT_RETRY_TICKS;
	objects_client_local_allocation = FALSE;
	objects_client_creating_index = NONE;
	objects_client_creating = FALSE;
	objects_client_deleting = FALSE;
}
