#include "harness.h"

typedef unsigned short word;
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#include "declarations.inc"

/* A co-op client's objects: a unit, a piece of scenery, a vehicle and a rifle the host attaches to one of them.
An update that called the rifle carried left it off the map (not connected) and hidden. */
#define OBJECT_COUNT 8
#define OBJECT(n) (0x10000L | (n))
struct object_datum
{
	struct { unsigned long flags; long parent_object_index; short type; boolean visible; } object;
	struct { unsigned long flags; } item;
	struct
	{
		long weapon_object_indices[MAXIMUM_WEAPONS_PER_UNIT];
		long weapon_last_used_at_game_time[MAXIMUM_WEAPONS_PER_UNIT];
	} unit;
};
#define unit_datum object_datum
#define weapon_datum object_datum
static struct object_datum objects[OBJECT_COUNT];
static int errors;

static struct object_datum *object_get(long index) { return &objects[index & 0xFFFF]; }
#define item_get(index) object_get(index)
#define unit_get(index) object_get(index)
#define weapon_get(index) object_get(index)
static long object_find(short name_index, long object_index, long definition_index, unsigned long mask)
{
	return object_index;
}
/* like objects.c's asserts: off the map before a reconnect (objects.c#880), on it before a disconnect (objects.c#958) */
static void object_reconnect_to_map(long index, void const *location)
{
	CHECK(!TEST_FLAG(object_get(index)->object.flags, _object_connected_to_map_bit),
		"objects.c#880: object %lx reconnected on the map", index);
	object_get(index)->object.flags |= FLAG(_object_connected_to_map_bit);
}
static void object_disconnect_from_map(long index)
{
	CHECK(TEST_FLAG(object_get(index)->object.flags, _object_connected_to_map_bit),
		"objects.c#958: object %lx disconnected off the map", index);
	object_get(index)->object.flags &= ~FLAG(_object_connected_to_map_bit);
}
static void object_set_visibility(long index, boolean visible) { object_get(index)->object.visible = visible; }
/* objects.c's order: off the map, moved to the marker, back on as the parent's child */
static void object_attach_to_marker(long parent_index, char const *parent_marker, long child_index, char const *child_marker)
{
	object_disconnect_from_map(child_index);
	object_get(child_index)->object.parent_object_index = parent_index;
	object_reconnect_to_map(child_index, NULL);
}
static void object_detach(long child_index)
{
	object_disconnect_from_map(child_index);
	object_get(child_index)->object.parent_object_index = NONE;
	object_reconnect_to_map(child_index, NULL);
}
/* the bit items.c sets */
static void item_in_unit_inventory(long item_index, long unit_index)
{
	item_get(item_index)->item.flags |= FLAG(_item_attached_to_unit_bit);
}
#define object_marker_name(index, marker) ""
#define network_coop_note_attach(parent_index, parent_marker, child_index, child_marker) ((void)0)
#define network_coop_note_detach(parent_index, child_index) ((void)0)
#define error(priority, ...) (errors++)

#include "under_test.inc"

#define UNIT OBJECT(1)
#define RIFLE OBJECT(2)
#define SCENERY OBJECT(3)
#define VEHICLE OBJECT(4)

static void reset(boolean on_map)
{
	short slot;

	memset(objects, 0, sizeof(objects));
	errors = 0;
	objects[1].object.flags = objects[3].object.flags = objects[4].object.flags = FLAG(_object_connected_to_map_bit);
	objects[1].object.parent_object_index = objects[3].object.parent_object_index = NONE;
	objects[4].object.parent_object_index = NONE;
	objects[1].object.type = _object_type_biped;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		objects[1].unit.weapon_object_indices[slot] = NONE;
	objects[3].object.type = _object_type_scenery;
	objects[4].object.type = _object_type_vehicle;
	objects[2].object.flags = on_map ? FLAG(_object_connected_to_map_bit) : 0;
	objects[2].object.parent_object_index = NONE;
	objects[2].object.type = _object_type_weapon;
	objects[2].object.visible = on_map;
}

/* the host's attach or detach of the rifle (or a resend of the attach) */
static void event(short type, long parent_index)
{
	struct distributed_coop_event event = {0};

	event.type = type;
	event.object_index = parent_index;
	event.target = RIFLE;
	event.reals[0] = -1.0f;
	client_apply_attach(&event);
}

static void check_attached(long parent_index)
{
	CHECK(objects[2].object.parent_object_index == parent_index, "the rifle is attached to %lx",
		objects[2].object.parent_object_index);
	CHECK(TEST_FLAG(objects[2].object.flags, _object_connected_to_map_bit), "the attached rifle is off the map");
	CHECK(objects[2].object.visible, "the attached rifle is hidden");
}

/* as the update left it: no parent, off the map, hidden */
static void check_untouched(void)
{
	CHECK(objects[2].object.parent_object_index == NONE, "the rifle is attached to %lx", objects[2].object.parent_object_index);
	CHECK(!TEST_FLAG(objects[2].object.flags, _object_connected_to_map_bit), "the rifle is on the map");
	CHECK(!objects[2].object.visible, "the rifle is shown");
}

/* the unit's first slot, as distributed_client_apply_inventory leaves a weapon there */
static void check_in_inventory(void)
{
	CHECK(objects[1].unit.weapon_object_indices[0] == RIFLE, "the rifle is not in the unit's slot");
	CHECK(TEST_FLAG(objects[2].item.flags, _item_attached_to_unit_bit), "the rifle is not marked as in an inventory");
	CHECK(objects[2].object.parent_object_index == NONE, "the rifle in the inventory is attached to %lx",
		objects[2].object.parent_object_index);
	CHECK(!TEST_FLAG(objects[2].object.flags, _object_connected_to_map_bit) && !objects[2].object.visible,
		"the rifle in the inventory is on the map or shown");
}

int main(int argc, char **argv)
{
	char const *case_name = argc == 2 ? argv[1] : "";

	CASE("on-map")
	{
		reset(TRUE);
		event(_coop_attach, UNIT);
		check_attached(UNIT);
	}
	/* attached to scenery by a script: no inventory comes for it; then a resend */
	else CASE("carried-scenery")
	{
		reset(FALSE);
		event(_coop_attach, SCENERY);
		check_attached(SCENERY);
		event(_coop_attach, SCENERY);
		check_attached(SCENERY);
		CHECK(errors == 0, "%d errors", errors);
	}
	/* the same at a vehicle's marker */
	else CASE("carried-vehicle")
	{
		reset(FALSE);
		event(_coop_attach, VEHICLE);
		check_attached(VEHICLE);
		event(_coop_attach, VEHICLE);
		check_attached(VEHICLE);
		CHECK(errors == 0, "%d errors", errors);
	}
	/* a detach of a rifle that is attached to nothing here */
	else CASE("carried-detach")
	{
		reset(FALSE);
		event(_coop_detach, SCENERY);
		check_untouched();
		CHECK(errors == 0, "%d errors", errors);
	}
	/* an attach to one of the rifle's own children (hierarchies that differ here) is dropped */
	else CASE("carried-own-child")
	{
		reset(FALSE);
		objects[3].object.parent_object_index = RIFLE;
		event(_coop_attach, SCENERY);
		check_untouched();
		CHECK(errors == 0, "%d errors", errors);
	}
	/* a child with a parent stays as it is, off the map too */
	else CASE("parented-off-map")
	{
		reset(FALSE);
		objects[2].object.parent_object_index = UNIT;
		event(_coop_attach, SCENERY);
		CHECK(objects[2].object.parent_object_index == UNIT, "the rifle is attached to %lx",
			objects[2].object.parent_object_index);
		CHECK(!TEST_FLAG(objects[2].object.flags, _object_connected_to_map_bit) && !objects[2].object.visible,
			"the rifle is on the map or shown");
		CHECK(errors == 0, "%d errors", errors);
	}
	/* Firefight's loadout: the attach and detach at the player, then the player's inventory with the rifle */
	else CASE("attach-detach-inventory")
	{
		reset(FALSE);
		event(_coop_attach, UNIT);
		check_attached(UNIT);
		event(_coop_detach, UNIT);
		CHECK(objects[2].object.parent_object_index == NONE &&
			TEST_FLAG(objects[2].object.flags, _object_connected_to_map_bit), "the detached rifle is not on the map");
		unit_network_add_weapon(UNIT, RIFLE, 0);
		check_in_inventory();
		CHECK(errors == 0, "%d errors", errors);
	}
	/* attached to the unit, then the unit's inventory takes it */
	else CASE("attach-inventory")
	{
		reset(FALSE);
		event(_coop_attach, UNIT);
		check_attached(UNIT);
		unit_network_add_weapon(UNIT, RIFLE, 0);
		check_in_inventory();
		CHECK(errors == 0, "%d errors", errors);
	}
	/* the inventory came first: the attach, its resend and the detach leave it there */
	else CASE("inventory-attach-detach")
	{
		reset(FALSE);
		unit_network_add_weapon(UNIT, RIFLE, 0);
		event(_coop_attach, UNIT);
		event(_coop_attach, UNIT);
		event(_coop_detach, UNIT);
		check_in_inventory();
		CHECK(errors == 0, "%d errors", errors);
	}
	else return 2;
	return 0;
}
