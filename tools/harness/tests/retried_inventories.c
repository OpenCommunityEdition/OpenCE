#include "harness.h"
#include <math.h>

typedef unsigned short word;
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include "config.inc"

/* A client's world: the host's objects, each here or not yet (the stream
makes them, and can trail the host's inventories, which go as datagrams);
units that carry weapons of one magazine. */
#define OBJECT_COUNT 256
#define OBJECT(n) (0x10000L | (n))
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
struct tag_block { long count; void *address; };
struct weapon_magazine_definition { short rounds_total_initial, rounds_total_maximum, rounds_loaded_maximum; };
struct weapon_definition { struct { struct tag_block magazines; } weapon; };
struct weapon_magazine { short rounds_total, rounds_loaded; };
struct weapon_datum { long definition_index; struct { struct weapon_magazine magazines[2]; real age; } weapon; };
struct unit_datum
{
	struct { unsigned long damage_flags; } object;
	struct
	{
		long player_index;
		long weapon_object_indices[MAXIMUM_WEAPONS_PER_UNIT];
		short current_weapon_index, desired_weapon_index;
		char grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES];
		short grenade_throw_state, current_grenade_index, desired_grenade_index;
	} unit;
};
struct player_datum { short local_player_index; };
static struct { boolean here; short type; } objects[OBJECT_COUNT];
static struct unit_datum units[OBJECT_COUNT];
static struct weapon_datum weapons[OBJECT_COUNT];
static struct weapon_magazine_definition magazine_definition = {0, 500, 100};
static struct weapon_definition weapon_definition = {{{1, &magazine_definition}}};
static struct player_datum player;
static long now;

static boolean here(long index)
{
	return index != NONE && (index >> 16) == 1 && (index & 0xFFFF) < OBJECT_COUNT && objects[index & 0xFFFF].here;
}
static boolean network_objects_client_has(long index) { return here(index); }
static void *object_try_and_get_and_verify_type(long index, unsigned long mask)
{
	if (!here(index) || !TEST_FLAG(mask, objects[index & 0xFFFF].type))
		return NULL;
	return objects[index & 0xFFFF].type == _object_type_weapon ? (void *)&weapons[index & 0xFFFF] : (void *)&units[index & 0xFFFF];
}
static struct unit_datum *unit_get(long index)
{
	CHECK(object_try_and_get_and_verify_type(index, _object_mask_unit), "not a unit here: %lx", index);
	return &units[index & 0xFFFF];
}
static struct weapon_datum *weapon_try_and_get(long index) { return object_try_and_get_and_verify_type(index, _object_mask_weapon); }
static struct weapon_definition *weapon_definition_get(long index) { return &weapon_definition; }
static long game_time_get(void) { return now; }
static struct player_datum *player_get(long index) { return &player; }
static boolean distributed_player_is_local(long player_index) { return FALSE; }
static boolean distributed_client_own_settled(long time) { return TRUE; }
static void distributed_client_release_item(long index, boolean dropped) {}
static void unit_network_drop_weapon(long unit_index, short slot) { unit_get(unit_index)->unit.weapon_object_indices[slot] = NONE; }
static void unit_network_add_weapon(long unit_index, long weapon_index, short slot)
{
	unit_get(unit_index)->unit.weapon_object_indices[slot] = weapon_index;
}
struct distributed_own_inventory;
static void distributed_client_note_own_inventory(struct distributed_own_inventory *own, long unit_index, boolean compare) {}
#include "under_test.inc"

/* the stream makes one of the host's objects here: a unit carrying nothing,
or a weapon */
static void arrive(long index, short type)
{
	struct unit_datum *unit = &units[index & 0xFFFF];
	short slot;

	objects[index & 0xFFFF].here = TRUE;
	objects[index & 0xFFFF].type = type;
	memset(unit, 0, sizeof(*unit));
	unit->unit.player_index = NONE;
	unit->unit.current_weapon_index = NONE;
	unit->unit.current_grenade_index = NONE;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		unit->unit.weapon_object_indices[slot] = NONE;
}

/* the host's inventory of a unit carrying two weapons (NONE: none) */
static struct distributed_inventory carrying(long unit_index, long first, long second, char grenades)
{
	struct distributed_inventory inventory;
	short slot;

	memset(&inventory, 0, sizeof(inventory));
	inventory.unit_index = unit_index;
	inventory.grenade_counts[0] = grenades;
	inventory.current_weapon_index = 0;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		inventory.weapon_indices[slot] = slot == 0 ? first : slot == 1 ? second : NONE;
		inventory.rounds_total[slot][0] = (short)(100 * (slot + 1));
		inventory.rounds_loaded[slot][0] = (short)(10 * (slot + 1));
	}
	return inventory;
}

/* the host's inventory comes, as network_objects_handle_inventories takes it */
static void receive(struct distributed_inventory inventory)
{
	distributed_client_apply_inventory(&inventory);
}

static boolean kept(long unit_index)
{
	short index;

	for (index = 0; index < objects_client_retried_inventory_count; index++)
	{
		if (objects_client_retried_inventories[index].inventory.unit_index == unit_index)
			return TRUE;
	}
	return FALSE;
}

static long held(long unit_index, short slot) { return unit_get(unit_index)->unit.weapon_object_indices[slot]; }

static void reset(void)
{
	memset(objects, 0, sizeof(objects));
	memset(&player, 0, sizeof(player));
	memset(objects_client_retried_inventories, 0, sizeof(objects_client_retried_inventories));
	objects_client_retried_inventory_count = 0;
	now = 1000;
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	long unit = OBJECT(1), first = OBJECT(2), second = OBJECT(3);
	short index;

	reset();
	/* a player spawned: its inventory comes before its unit and its weapon,
	is kept, and is applied in full the tick they have all come */
	CASE("unit-late")
	{
		receive(carrying(unit, first, NONE, 3));
		CHECK(kept(unit) && objects_client_retried_inventory_count == 1, "unit-late: not kept");
		arrive(first, _object_type_weapon);
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(kept(unit), "unit-late: let go of before its unit came");
		arrive(unit, _object_type_biped);
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(held(unit, 0) == first, "unit-late: its weapon not given once it came");
		CHECK(units[1].unit.grenade_counts[0] == 3 && weapons[2].weapon.magazines[0].rounds_total == 100 &&
			weapons[2].weapon.magazines[0].rounds_loaded == 10, "unit-late: its grenades or rounds not given");
		CHECK(!kept(unit) && objects_client_retried_inventory_count == 0, "unit-late: kept once applied");
	}
	/* its unit here, one of its weapons not: what is here is given at once,
	the rest once it comes */
	CASE("weapon-late")
	{
		arrive(unit, _object_type_biped);
		arrive(first, _object_type_weapon);
		receive(carrying(unit, first, second, 2));
		CHECK(held(unit, 0) == first && held(unit, 1) == NONE, "weapon-late: what is here not given at once");
		CHECK(kept(unit), "weapon-late: not kept for the weapon to come");
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(kept(unit) && held(unit, 1) == NONE, "weapon-late: applied before its weapon came");
		arrive(second, _object_type_weapon);
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(held(unit, 1) == second && weapons[3].weapon.magazines[0].rounds_total == 200,
			"weapon-late: the weapon not given once it came");
		CHECK(!kept(unit), "weapon-late: kept once applied");
	}
	/* a unit's newer inventory takes its older one's place */
	CASE("newest")
	{
		receive(carrying(unit, first, NONE, 1));
		now++;
		receive(carrying(unit, first, NONE, 4));
		CHECK(objects_client_retried_inventory_count == 1, "newest: %d kept of one unit", objects_client_retried_inventory_count);
		arrive(unit, _object_type_biped);
		arrive(first, _object_type_weapon);
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(units[1].unit.grenade_counts[0] == 4, "newest: not the newest applied");
	}
	/* the list full: one more takes the oldest's place, and a kept unit's
	newer one its own */
	CASE("full")
	{
		for (index = 0; index < MAXIMUM_CLIENT_RETRIED_INVENTORIES; index++)
		{
			now++;
			receive(carrying(OBJECT(17 + index), NONE, NONE, 1));
		}
		CHECK(objects_client_retried_inventory_count == MAXIMUM_CLIENT_RETRIED_INVENTORIES,
			"full: %d kept of %d", objects_client_retried_inventory_count, MAXIMUM_CLIENT_RETRIED_INVENTORIES);
		now++;
		receive(carrying(OBJECT(200), NONE, NONE, 1));
		CHECK(!kept(OBJECT(17)) && kept(OBJECT(18)) && kept(OBJECT(200)), "full: not the oldest's place taken");
		now++;
		receive(carrying(OBJECT(201), NONE, NONE, 1));
		CHECK(!kept(OBJECT(18)) && kept(OBJECT(19)) && kept(OBJECT(201)), "full: not the next oldest's taken");
		now++;
		receive(carrying(OBJECT(19), NONE, NONE, 2));
		CHECK(kept(OBJECT(19)) && kept(OBJECT(20)), "full: a kept unit's newer one took another's place");
		CHECK(objects_client_retried_inventory_count == MAXIMUM_CLIENT_RETRIED_INVENTORIES, "full: %d kept",
			objects_client_retried_inventory_count);
	}
	/* a unit's whole inventory, come once all it names is here: its kept one
	is let go of, and never applied over it */
	CASE("forget")
	{
		long other = OBJECT(6);

		receive(carrying(unit, first, NONE, 1));
		receive(carrying(other, NONE, NONE, 1));
		arrive(unit, _object_type_biped);
		arrive(first, _object_type_weapon);
		receive(carrying(unit, first, NONE, 5));
		CHECK(!kept(unit) && kept(other) && objects_client_retried_inventory_count == 1, "forget: not let go of");
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(units[1].unit.grenade_counts[0] == 5, "forget: the older applied over the newer");
	}
	/* let go of EMPTY_INVENTORY_TICKS after the newest of its unit's came,
	and not applied when its unit comes after */
	CASE("expiry")
	{
		long newest = now + EMPTY_INVENTORY_TICKS / 2;

		receive(carrying(unit, first, NONE, 1));
		now = newest;
		receive(carrying(unit, first, NONE, 1));
		now = newest + EMPTY_INVENTORY_TICKS - 1;
		distributed_client_apply_retried_inventories();
		CHECK(kept(unit), "expiry: let go of %d ticks after the newest", EMPTY_INVENTORY_TICKS - 1);
		now = newest + EMPTY_INVENTORY_TICKS;
		distributed_client_apply_retried_inventories();
		CHECK(!kept(unit), "expiry: kept %d ticks after the newest", EMPTY_INVENTORY_TICKS);
		arrive(unit, _object_type_biped);
		arrive(first, _object_type_weapon);
		now++;
		distributed_client_apply_retried_inventories();
		CHECK(held(unit, 0) == NONE, "expiry: applied after it was let go of");
	}
	return 0;
}
