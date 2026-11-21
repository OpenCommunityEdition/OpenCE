/* The real campaign_carry.c (game.carry_weapons) over one player's unit and
two maps whose weapon tags have different indices, the second without the
needler: the weapons, rounds, battery, the one in hand and the grenades
taken as a level is won are given at the next level's first spawn in place
of its loadout, a weapon the map lacks replaced by one of the level's, and
none given on another map, a second spawn, in a network game, with the
setting off or for a player dead at the win. */
#include "harness.h"

#include "config.inc"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define csmemset memset
#define csstrncpy strncpy
#define WEAPON_DEFINITION_TAG 0x77656170L
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address + (index))

#define OBJECTS 32
#define UNIT 0x10000L

/* a unit and its weapons, one object */
struct fake_object
{
	long definition_index;
	struct { short type; long damage_flags; } object;
	struct
	{
		short current_weapon_index;
		short desired_weapon_index;
		long weapon_object_indices[MAXIMUM_WEAPONS_PER_UNIT];
		char grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES];
	} unit;
	struct { real heat; real age; struct { short rounds_total; short rounds_loaded; } magazines[2]; } weapon;
};
#define unit_datum fake_object
#define weapon_datum fake_object
#define object_datum fake_object

static struct fake_object objects[OBJECTS];
static boolean used[OBJECTS];
static struct fake_object *object_get(long index) { return &objects[index & 0xFFFF]; }
#define unit_get object_get
#define weapon_get object_get

/* the weapons' tags: their names, their indices in the old map and the new
(NONE: not in it), their magazine's maxima and a new one's rounds */
enum { AR, PISTOL, NEEDLER, SNIPER, WEAPONS };
static struct
{
	char const *name;
	long old_index, new_index;
	short total_maximum, loaded_maximum, total_initial, loaded_initial;
} definitions[WEAPONS] =
{
	{ "weapons\\assault rifle\\assault rifle", 0x100, 0x203, 600, 60, 240, 60 },
	{ "weapons\\pistol\\pistol", 0x101, 0x201, 120, 12, 48, 12 },
	{ "weapons\\needler\\needler", 0x102, NONE, 80, 20, 60, 20 },
	{ "weapons\\sniper rifle\\sniper rifle", 0x103, 0x202, 24, 4, 8, 4 },
};
static boolean on_new_map;

struct tag_block { long count; void *address; };
struct weapon_magazine_definition { short rounds_total_maximum; short rounds_loaded_maximum; };
struct weapon_definition { struct { struct tag_block magazines; } weapon; };
static struct weapon_magazine_definition magazine_definitions[WEAPONS];
static struct weapon_definition weapon_definitions[WEAPONS];

static int weapon_of(long index)
{
	int weapon;

	for (weapon = 0; weapon < WEAPONS; weapon++)
	{
		if (index != NONE && (definitions[weapon].old_index == index || definitions[weapon].new_index == index))
			return weapon;
	}
	return -1;
}
static long index_of(int weapon) { return on_new_map ? definitions[weapon].new_index : definitions[weapon].old_index; }
static char *tag_get_name(long index) { return (char *)definitions[weapon_of(index)].name; }
static long tag_loaded(long group, char const *name)
{
	int weapon;

	for (weapon = 0; weapon < WEAPONS; weapon++)
	{
		if (!strcmp(name, definitions[weapon].name))
			return index_of(weapon);
	}
	return NONE;
}
static struct weapon_definition *weapon_definition_get(long index) { return &weapon_definitions[weapon_of(index)]; }

struct object_placement_data { long definition_index; };
static void object_placement_data_new(struct object_placement_data *data, long definition_index, long owner)
{
	data->definition_index = definition_index;
}
static long object_new(struct object_placement_data *data)
{
	short slot;
	int weapon = weapon_of(data->definition_index);

	for (slot = 1; slot < OBJECTS && used[slot]; slot++)
		;
	if (slot == OBJECTS || weapon < 0)
		return NONE;
	used[slot] = TRUE;
	memset(&objects[slot], 0, sizeof(objects[slot]));
	objects[slot].definition_index = data->definition_index;
	objects[slot].object.type = _object_type_weapon;
	objects[slot].weapon.magazines[0].rounds_total = definitions[weapon].total_initial;
	objects[slot].weapon.magazines[0].rounds_loaded = definitions[weapon].loaded_initial;
	return 0x10000L | slot;
}
static void object_delete(long index) { used[index & 0xFFFF] = FALSE; }
static void unit_delete_all_weapons(long unit_index)
{
	short slot;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (objects[0].unit.weapon_object_indices[slot] != NONE)
			object_delete(objects[0].unit.weapon_object_indices[slot]);
		objects[0].unit.weapon_object_indices[slot] = NONE;
	}
	objects[0].unit.current_weapon_index = NONE;
	objects[0].unit.desired_weapon_index = NONE;
}
static boolean unit_add_weapon_to_inventory(long unit_index, long weapon_index, long mode)
{
	short slot;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT && objects[0].unit.weapon_object_indices[slot] != NONE; slot++)
		;
	if (slot == MAXIMUM_WEAPONS_PER_UNIT)
		return FALSE;
	objects[0].unit.weapon_object_indices[slot] = weapon_index;
	/* (the starting weapon is the one readied) */
	if (mode != _unit_add_weapon_normal)
		objects[0].unit.desired_weapon_index = slot;
	return TRUE;
}

static boolean mcc_active;
static short mcc_counts[4];
static short mcc_grenades_get(long unit, short type) { return mcc_active ? mcc_counts[type] : 0; }
static void mcc_grenades_set(long unit, short type, short count) { if (mcc_active) mcc_counts[type] = count; }

static boolean carry_setting = TRUE;
static int config_boolean(const char *name) { return !strcmp(name, "game.carry_weapons") ? carry_setting : 0; }
static short connection = _game_connection_local;
static short game_connection(void) { return connection; }
static boolean game_engine_running(void) { return FALSE; }
struct player_datum { long unit_index; };
static struct player_datum player = { UNIT };
static long local_player_get_player_index(short local_player_index) { return local_player_index == 0 ? 0 : NONE; }
static struct player_datum *player_get(long player_index) { return &player; }

#include "under_test.inc"

/* a weapon of the map being played, with these rounds */
static long weapon_new(int weapon, short loaded, short total)
{
	struct object_placement_data placement;
	long index;

	object_placement_data_new(&placement, index_of(weapon), UNIT);
	index = object_new(&placement);
	objects[index & 0xFFFF].weapon.magazines[0].rounds_loaded = loaded;
	objects[index & 0xFFFF].weapon.magazines[0].rounds_total = total;
	return index;
}

/* the player on the old map: these weapons in slots 0 and 1 (-1 none), the
one in the slot given in hand, these grenades */
static void playing(int first, int second, short in_hand, char frags, char plasmas)
{
	memset(used, 0, sizeof(used));
	memset(objects, 0, sizeof(objects));
	used[0] = TRUE;
	on_new_map = FALSE;
	unit_delete_all_weapons(UNIT);
	if (first >= 0)
		objects[0].unit.weapon_object_indices[0] = weapon_new(first, 37, 250);
	if (second >= 0)
		objects[0].unit.weapon_object_indices[1] = weapon_new(second, 5, 40);
	objects[0].unit.current_weapon_index = in_hand;
	objects[0].unit.grenade_counts[0] = frags;
	objects[0].unit.grenade_counts[1] = plasmas;
}

/* a map begun, and the player spawning with its loadout (the pistol and
the sniper rifle, a frag), as players.c gives it, then what is carried */
static void spawn_on(char const *map_name)
{
	on_new_map = TRUE;
	campaign_carry_new_map(map_name);
	unit_delete_all_weapons(UNIT);
	unit_add_weapon_to_inventory(UNIT, weapon_new(PISTOL, 12, 48), _unit_add_weapon_starting);
	unit_add_weapon_to_inventory(UNIT, weapon_new(SNIPER, 4, 8), _unit_add_weapon_normal);
	objects[0].unit.grenade_counts[0] = 1;
	objects[0].unit.grenade_counts[1] = 0;
	campaign_carry_give(UNIT, 0);
}

static boolean holds(short slot, int weapon)
{
	long index = objects[0].unit.weapon_object_indices[slot];

	return weapon < 0 ? index == NONE :
		index != NONE && objects[index & 0xFFFF].definition_index == definitions[weapon].new_index;
}
static struct fake_object *in_slot(short slot) { return &objects[objects[0].unit.weapon_object_indices[slot] & 0xFFFF]; }
static boolean level_loadout(void)
{
	return holds(0, PISTOL) && holds(1, SNIPER) && holds(2, -1) && objects[0].unit.grenade_counts[0] == 1;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	int weapon;

	for (weapon = 0; weapon < WEAPONS; weapon++)
	{
		magazine_definitions[weapon].rounds_total_maximum = definitions[weapon].total_maximum;
		magazine_definitions[weapon].rounds_loaded_maximum = definitions[weapon].loaded_maximum;
		weapon_definitions[weapon].weapon.magazines.count = 1;
		weapon_definitions[weapon].weapon.magazines.address = &magazine_definitions[weapon];
	}

	/* the pistol in slot 0, the rifle in hand in slot 1: the next level's
	map's rifle (another index) first, in hand, with its rounds and battery,
	then the pistol, and the grenades */
	CASE("carried")
	{
		playing(PISTOL, AR, 1, 2, 3);
		objects[0].weapon.age = 0.0f;
		objects[objects[0].unit.weapon_object_indices[1] & 0xFFFF].weapon.age = 0.25f;
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		CHECK(holds(0, AR) && holds(1, PISTOL) && holds(2, -1), "the carried weapons not given, in hand first");
		CHECK(objects[0].unit.desired_weapon_index == 0, "the rifle not the one readied");
		CHECK(in_slot(0)->weapon.magazines[0].rounds_loaded == 5 && in_slot(0)->weapon.magazines[0].rounds_total == 40 &&
			in_slot(0)->weapon.age == 0.25f, "the rifle's rounds or battery not carried");
		CHECK(in_slot(1)->weapon.magazines[0].rounds_loaded == 12 && in_slot(1)->weapon.magazines[0].rounds_total == 120,
			"the pistol's rounds not carried within its magazine's maxima");
		CHECK(objects[0].unit.grenade_counts[0] == 2 && objects[0].unit.grenade_counts[1] == 3, "the grenades not carried");
	}
	/* the needler the next map lacks: the rifle, and the first of the
	level's own weapons in its place */
	CASE("missing")
	{
		playing(NEEDLER, AR, 0, 0, 0);
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		CHECK(holds(0, AR) && holds(1, PISTOL) && holds(2, -1), "a missing weapon's place not taken by the level's");
	}
	/* none the next map has: the level's own loadout, the grenades carried */
	CASE("none in map")
	{
		playing(NEEDLER, -1, 0, 4, 0);
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		CHECK(holds(0, PISTOL) && holds(1, SNIPER) && holds(2, -1), "the level's loadout not kept");
		CHECK(objects[0].unit.grenade_counts[0] == 4, "the grenades not carried");
	}
	/* a second spawn on the level (split screen): its loadout */
	CASE("once")
	{
		playing(AR, -1, 0, 0, 0);
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		campaign_carry_give(UNIT, 0);
		on_new_map = TRUE;
		unit_delete_all_weapons(UNIT);
		unit_add_weapon_to_inventory(UNIT, weapon_new(PISTOL, 12, 48), _unit_add_weapon_starting);
		unit_add_weapon_to_inventory(UNIT, weapon_new(SNIPER, 4, 8), _unit_add_weapon_normal);
		objects[0].unit.grenade_counts[0] = 1;
		campaign_carry_give(UNIT, 0);
		CHECK(level_loadout(), "given again at a second spawn");
	}
	/* the level restarted: given again; then another map: none */
	CASE("restart")
	{
		playing(AR, -1, 0, 0, 0);
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		spawn_on("levels\\b30\\b30");
		CHECK(holds(0, AR) && holds(1, -1), "not given again on a restart");
		spawn_on("levels\\b40\\b40");
		CHECK(level_loadout(), "given on another map");
	}
	/* the map after the win is not a level (the main menu after the last):
	the next one begun gets none */
	CASE("menu between")
	{
		playing(AR, -1, 0, 0, 0);
		campaign_carry_take();
		on_new_map = TRUE;
		campaign_carry_new_map("ui\\shell\\main_menu");
		spawn_on("levels\\a10\\a10");
		CHECK(level_loadout(), "given on a level begun from the menu");
	}
	/* a network game (co-op), the setting off, a player dead at the win:
	the level's loadout */
	CASE("network")
	{
		playing(AR, -1, 0, 0, 0);
		connection = 1;
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		CHECK(level_loadout(), "carried in a network game");
	}
	CASE("off")
	{
		playing(AR, -1, 0, 0, 0);
		carry_setting = FALSE;
		campaign_carry_take();
		spawn_on("levels\\b30\\b30");
		CHECK(level_loadout(), "carried with the setting off");
	}
	CASE("dead")
	{
		playing(AR, -1, 0, 0, 0);
		objects[0].object.damage_flags = 1L << _object_dead_bit;
		campaign_carry_take();
		objects[0].object.damage_flags = 0;
		spawn_on("levels\\b30\\b30");
		CHECK(level_loadout(), "carried for a player dead at the win");
	}
	/* an MCC map's two more grenade types */
	CASE("mcc grenades")
	{
		playing(AR, -1, 0, 1, 1);
		mcc_active = TRUE;
		mcc_counts[2] = 3;
		mcc_counts[3] = 2;
		campaign_carry_take();
		mcc_counts[2] = mcc_counts[3] = 0;
		spawn_on("levels\\b30\\b30");
		CHECK(mcc_counts[2] == 3 && mcc_counts[3] == 2, "MCC's grenades not carried");
	}
	return 0;
}
