/*
GUN_GAME_WEAPON_EMPTY.C (test)

The real gun_game_weapon_empty (port/linux/game/gun_game.c): Gun Game gives a
player a new weapon of their level when theirs is spent. Halo CE's plasma
rifle and plasma pistol have a magazine block that holds no rounds (every
count 0: they run on a battery), so a check that looks only at the
magazines takes them for spent from the start, and gun_game_equip replaces
them on every update: they never fire. Weapons here are a plasma rifle (one
magazine holding nothing), a pistol (12 loaded, 120 carried at most) and a
weapon with no magazines at all. test_gun_game_weapon_empty.py takes the
code (under_test.inc).
*/

#include "harness.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))

struct tag_block { long count; void *address; };
struct weapon_magazine_definition { short rounds_total_maximum; short rounds_loaded_maximum; };
struct weapon_definition { struct { struct tag_block magazines; } weapon; };
struct weapon_magazine { short rounds_total; short rounds_loaded; };
struct weapon_datum
{
	long definition_index;
	struct { real age; struct weapon_magazine magazines[2]; } weapon;
};

enum { PLASMA_RIFLE, PISTOL, NO_MAGAZINES, NUMBER_OF_DEFINITIONS };

static struct weapon_magazine_definition battery_magazine = { 0, 0 };
static struct weapon_magazine_definition pistol_magazine = { 120, 12 };
static struct weapon_definition definitions[NUMBER_OF_DEFINITIONS] =
{
	{ { { 1, &battery_magazine } } },
	{ { { 1, &pistol_magazine } } },
	{ { { 0, NULL } } },
};
static struct weapon_datum held;

#define weapon_try_and_get(index) ((index) == NONE ? NULL : &held)
#define weapon_definition_get(index) (&definitions[index])
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])

#include "under_test.inc"

static void hold(long definition_index, real age, short loaded, short total)
{
	memset(&held, 0, sizeof(held));
	held.definition_index = definition_index;
	held.weapon.age = age;
	held.weapon.magazines[0].rounds_loaded = loaded;
	held.weapon.magazines[0].rounds_total = total;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("battery-full")
	{
		/* a plasma rifle as it is given: its magazine block holds nothing, its battery is full */
		hold(PLASMA_RIFLE, 0.0f, 0, 0);
		CHECK(!gun_game_weapon_empty(0), "a full plasma rifle is taken for spent");
		hold(PLASMA_RIFLE, 0.6f, 0, 0);
		CHECK(!gun_game_weapon_empty(0), "a plasma rifle with battery left is taken for spent");
		return 0;
	}
	CASE("battery-spent")
	{
		hold(PLASMA_RIFLE, 1.0f, 0, 0);
		CHECK(gun_game_weapon_empty(0), "a plasma rifle with a dead battery is not spent");
		return 0;
	}
	CASE("rounds")
	{
		/* a weapon whose magazine holds rounds: spent when none is loaded or carried, whatever its age */
		hold(PISTOL, 0.0f, 5, 0);
		CHECK(!gun_game_weapon_empty(0), "a pistol with rounds loaded is taken for spent");
		hold(PISTOL, 0.0f, 0, 24);
		CHECK(!gun_game_weapon_empty(0), "a pistol with rounds carried is taken for spent");
		hold(PISTOL, 0.0f, 0, 0);
		CHECK(gun_game_weapon_empty(0), "a dry pistol is not spent");
		return 0;
	}
	CASE("no-magazines")
	{
		hold(NO_MAGAZINES, 0.5f, 0, 0);
		CHECK(!gun_game_weapon_empty(0), "a weapon without magazines, battery left, is taken for spent");
		hold(NO_MAGAZINES, 1.0f, 0, 0);
		CHECK(gun_game_weapon_empty(0), "a weapon without magazines and a dead battery is not spent");
		CHECK(!gun_game_weapon_empty(NONE), "no weapon is taken for a spent one");
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
