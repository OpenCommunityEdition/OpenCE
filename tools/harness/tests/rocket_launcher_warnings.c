/*
ROCKET_LAUNCHER_WARNINGS.C (test)

The real rocket_launcher_warning_placement (hud_weapon.c): the rocket
launcher's warnings ("Reload", the flash of its last rocket, its click when
empty), which it takes from ui\hud\master rounds 30 HUD units below its
reticle's middle, were drawn over its ring, which reaches 31 units down. They
are drawn 7 units lower for it; every other weapon's are where its HUD puts
them, as is any other crosshair.
test_rocket_launcher_warnings.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

typedef struct { short x, y; } point2d;
typedef struct { real i, j; } real_vector2d;

static char const *tag_names[] =
{
	"weapons\\rocket launcher\\rocket_launcher",
	"weapons\\assault rifle\\assault rifle",
	"weapons\\shotgun\\shotgun",
	"ui\\hud\\master rounds",
};

static char const *tag_get_name(long tag_index)
{
	return tag_names[tag_index];
}

static long csstrcmp(char const *a, char const *b)
{
	return strcmp(a, b);
}

#include "under_test.inc"

enum
{
	ROCKET_LAUNCHER = 0,
	ASSAULT_RIFLE,
	SHOTGUN,
	MASTER_ROUNDS
};

/* where a warning of the state at (x, y) is drawn for a weapon's HUD */
static short placed_y(long hud_index, short state_index, short x, short y)
{
	struct hud_placement_definition placement;

	memset(&placement, 0, sizeof(placement));
	placement.offset.x = x;
	placement.offset.y = y;
	rocket_launcher_warning_placement(hud_index, state_index, &placement);
	CHECK(placement.offset.x == x, "moved across, to %d", placement.offset.x);
	return placement.offset.y;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* master rounds' three warnings, at (-30, 30): 7 lower */
	CASE("rocket launcher")
	{
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_flash_ammo_none_for_reload, -30, 30) == 37, "Reload at %d",
			placed_y(ROCKET_LAUNCHER, _crosshair_state_flash_ammo_none_for_reload, -30, 30));
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_flash_ammo, -30, 30) == 37, "its last rocket at %d",
			placed_y(ROCKET_LAUNCHER, _crosshair_state_flash_ammo, -30, 30));
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_fired_with_no_ammo, -30, 30) == 37, "its click at %d",
			placed_y(ROCKET_LAUNCHER, _crosshair_state_fired_with_no_ammo, -30, 30));
		return 0;
	}
	/* the same warnings of other weapons (the shotgun's ring only touches
	"Reload"), and the HUD that holds them drawn for itself: where they were */
	CASE("other weapons")
	{
		CHECK(placed_y(ASSAULT_RIFLE, _crosshair_state_flash_ammo_none_for_reload, -30, 30) == 30, "assault rifle moved");
		CHECK(placed_y(SHOTGUN, _crosshair_state_flash_ammo_none_for_reload, -30, 30) == 30, "shotgun moved");
		CHECK(placed_y(MASTER_ROUNDS, _crosshair_state_flash_ammo, -30, 30) == 30, "master rounds moved");
		return 0;
	}
	/* the rocket launcher's reticle, scope and a grenade warning (master's,
	below everything), and a warning above the middle: where they were */
	CASE("other crosshairs")
	{
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_aim, 0, 0) == 0, "reticle moved");
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_zoom, 105, 105) == 105, "scope moved");
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_threw_with_no_grenade, 0, 60) == 60, "grenade warning moved");
		CHECK(placed_y(ROCKET_LAUNCHER, _crosshair_state_flash_ammo, -30, -30) == -30, "warning above moved");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
