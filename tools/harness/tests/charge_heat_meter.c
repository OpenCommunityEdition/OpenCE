/*
CHARGE_HEAT_METER.C (test)

The real weapon_build_weapon_interface_state and
weapon_trigger_get_charged_fraction (weapons.c), the state the hud draws a
weapon's meters from, for a plasma pistol: while its first trigger charges,
the heat meter shows as much of the heat its charged shot adds (the second
trigger's, fired as the charge is let go) as it has charged, and full when it
is charged; it showed none of it until the shot, then jumped to full. The
weapon's own heat, which the game plays by, is not changed.
test_charge_heat_meter.py takes the enums, structures and code from the
sources (config.inc, under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;
#define TICKS_PER_SECOND 30
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define csmemset memset
struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])

#include "config.inc"

/* the plasma pistol: its triggers (the first charges in 0.5 s, its charged
shot the second's, which adds 0.8 heat) and its battery (no magazines) */
struct weapon_trigger_definition { real charging_time; real heat_generated_per_round; };
struct weapon_magazine_definition { short rounds_loaded_maximum, rounds_total_maximum; };
struct weapon_definition { struct { struct tag_block triggers, magazines; } weapon; };
static struct weapon_trigger_definition triggers[2] = { { 0.5f, 0.05f }, { 0.f, 0.8f } };
static struct weapon_definition pistol = { { { 2, triggers }, { 0, NULL } } };

struct weapon_trigger { short state; short state_timer; };
struct weapon_magazine { short state, rounds_loaded, rounds_total; };
struct weapon_datum { long definition_index; struct { real heat, age; unsigned long flags; struct weapon_trigger triggers[2]; } weapon; };
static struct weapon_datum weapon;

static struct { boolean bottomless_clip; } cheat;

static struct weapon_datum *weapon_get(long index) { return &weapon; }
static struct weapon_definition *weapon_definition_get(long index) { return &pistol; }
static short weapon_magazine_count(struct weapon_definition const *definition) { return 0; }
static struct weapon_trigger *weapon_trigger_get(struct weapon_datum *datum, short index)
{
	return index >= 0 && index < 2 ? &datum->weapon.triggers[index] : NULL;
}
static struct weapon_magazine *weapon_magazine_get(struct weapon_datum *datum, short index) { return NULL; }

#include "under_test.inc"

static real meter(void)
{
	struct weapon_interface_state state;

	weapon_build_weapon_interface_state(0, &state);
	return state.heat;
}

#define NEAR(a, b) ((a) - (b) < 1e-4f && (b) - (a) < 1e-4f)

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* idle: the weapon's heat */
	CASE("idle")
	{
		weapon.weapon.heat = 0.25f;
		CHECK(NEAR(meter(), 0.25f), "an idle weapon's meter %f, not its heat 0.25", meter());
		return 0;
	}
	/* charging: the charged shot's heat as far as it has charged */
	CASE("charging")
	{
		weapon.weapon.heat = 0.1f;
		weapon.weapon.triggers[0].state = _trigger_charging;
		weapon.weapon.triggers[0].state_timer = 15; /* (0.5 s to go of 0.5 s: none charged) */
		CHECK(NEAR(meter(), 0.1f), "a charge just begun shows %f, not 0.1", meter());
		weapon.weapon.triggers[0].state_timer = 6; /* (0.2 s to go: 60% charged) */
		CHECK(NEAR(meter(), 0.1f + 0.6f * 0.8f), "a charge 60%% through shows %f, not %f", meter(), 0.1f + 0.6f * 0.8f);
		CHECK(NEAR(weapon.weapon.heat, 0.1f), "the weapon's own heat changed to %f", weapon.weapon.heat);
		return 0;
	}
	/* charged and held: what letting it go adds, no more than full */
	CASE("charged")
	{
		weapon.weapon.heat = 0.1f;
		weapon.weapon.triggers[0].state = _trigger_charged;
		CHECK(NEAR(meter(), 0.9f), "a charged weapon shows %f, not 0.9", meter());
		weapon.weapon.heat = 0.5f;
		CHECK(NEAR(meter(), 1.0f), "a hot charged weapon shows %f, not full", meter());
		return 0;
	}
	/* bottomless clip: the shot adds no heat, so none is shown */
	CASE("bottomless")
	{
		cheat.bottomless_clip = TRUE;
		weapon.weapon.heat = 0.1f;
		weapon.weapon.triggers[0].state = _trigger_charged;
		CHECK(NEAR(meter(), 0.1f), "with a bottomless clip the meter shows %f, not 0.1", meter());
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
