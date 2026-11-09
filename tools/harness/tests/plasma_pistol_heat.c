/*
PLASMA_PISTOL_HEAT.C (test)

The real plasma_pistol_heat_meter_value (plasma_pistol_heat.c): with
display.plasma_pistol_heat, the plasma pistol's gauge (a meter shader whose
value follows the pistol's charge) shows the larger of its charge and the
pistol's heat, so that firing it fast fills the gauge as it heats; any other
weapon's meters, and every meter with the setting off, are as they were.
test_plasma_pistol_heat.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define csstrcmp strcmp
#define _object_mask_weapon 4

/* (what the gauge reads of a weapon) */
struct weapon_datum
{
	long definition_index;
	struct
	{
		real heat;
	} weapon;
};

static int setting = 1;
static unsigned long setting_changes = 1;
static struct weapon_datum weapons[3] =
{
	{ 0, { 0.0f } }, /* a plasma pistol */
	{ 1, { 0.0f } }, /* a plasma rifle */
	{ 0, { 0.0f } }, /* another plasma pistol */
};
static char const *definition_names[] =
{
	"weapons\\plasma pistol\\plasma pistol",
	"weapons\\plasma rifle\\plasma rifle",
};

static int config_boolean(const char *name)
{
	return !strcmp(name, "display.plasma_pistol_heat") && setting;
}

static unsigned long config_changes(void)
{
	return setting_changes;
}

static char const *tag_get_name(long tag_index)
{
	return definition_names[tag_index];
}

/* objects 0..2 are the weapons; 7 is not a weapon */
static void *object_try_and_get_and_verify_type(long object_index, unsigned long mask)
{
	(void)mask;
	return object_index >= 0 && object_index < 3 ? &weapons[object_index] : NULL;
}

#include "under_test.inc"

static int near(real a, real b)
{
	return a - b < 1e-5f && b - a < 1e-5f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* fired fast: no charge, hot */
	CASE("firing fast")
	{
		weapons[0].weapon.heat = 0.6f;
		CHECK(near(plasma_pistol_heat_meter_value(0, 0.0f), 0.6f), "%g", plasma_pistol_heat_meter_value(0, 0.0f));
		weapons[2].weapon.heat = 1.4f;
		CHECK(near(plasma_pistol_heat_meter_value(2, 0.0f), 1.0f), "past 1: %g", plasma_pistol_heat_meter_value(2, 0.0f));
		return 0;
	}
	/* an overcharged shot charging, cooler than it is charged: its charge */
	CASE("charging")
	{
		weapons[0].weapon.heat = 0.3f;
		CHECK(near(plasma_pistol_heat_meter_value(0, 0.8f), 0.8f), "%g", plasma_pistol_heat_meter_value(0, 0.8f));
		return 0;
	}
	/* a plasma rifle's gauge, no object, a meter on what is not a weapon:
	as they were */
	CASE("other meters")
	{
		weapons[1].weapon.heat = 0.9f;
		CHECK(near(plasma_pistol_heat_meter_value(1, 0.2f), 0.2f), "a plasma rifle's: %g", plasma_pistol_heat_meter_value(1, 0.2f));
		CHECK(near(plasma_pistol_heat_meter_value(NONE, 0.2f), 0.2f), "no object: %g", plasma_pistol_heat_meter_value(NONE, 0.2f));
		CHECK(near(plasma_pistol_heat_meter_value(7, 0.2f), 0.2f), "not a weapon: %g", plasma_pistol_heat_meter_value(7, 0.2f));
		weapons[0].weapon.heat = 0.9f;
		CHECK(near(plasma_pistol_heat_meter_value(0, 0.2f), 0.9f), "a plasma pistol's after: %g",
			plasma_pistol_heat_meter_value(0, 0.2f));
		return 0;
	}
	/* the setting off: the stock gauge */
	CASE("setting off")
	{
		setting = 0;
		setting_changes++;
		weapons[0].weapon.heat = 0.7f;
		CHECK(near(plasma_pistol_heat_meter_value(0, 0.0f), 0.0f), "%g", plasma_pistol_heat_meter_value(0, 0.0f));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
