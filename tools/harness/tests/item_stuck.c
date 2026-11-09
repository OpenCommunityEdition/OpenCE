/*
ITEM_STUCK.C (test)

The real item_stuck_count (item_stuck.c): a falling item comes to rest only
on ground no steeper than 45 degrees, met slowly; wedged where it cannot (a
crease, a wall, a steep slope), it hit the ground every tick without getting
anywhere, spinning and playing its collision sound with each hit, until it
was picked up. One that hits the ground on 10 ticks in a row, moving no
further than 3 cm in any of them, is put at rest; a tick in the air, a
further move, or a hit on anything but the ground (a vehicle, a biped)
starts the count again. test_item_stuck.py takes the code from the sources
(under_test.inc).
*/

#include "harness.h"

#include <math.h>

#include "under_test.inc"

/* the tick (1, 2, ...) of the ticks given that put the item at rest, or 0 */
static int settles(short *stuck_ticks, int ticks, boolean hit_ground, real distance)
{
	int tick;

	for (tick = 1; tick <= ticks; tick++)
	{
		if (item_stuck_count(stuck_ticks, hit_ground, distance))
		{
			return tick;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short stuck_ticks = 0;

	/* pinned where it hit (each tick's hit where the tick began), or
	trembling in a crease: at rest on the tenth tick, and counting again
	after */
	CASE("stuck")
	{
		CHECK(settles(&stuck_ticks, 30, TRUE, 0.f) == 10, "pinned: not at rest on the tenth tick");
		CHECK(stuck_ticks == 0, "the count not started again (%d)", stuck_ticks);
		CHECK(settles(&stuck_ticks, 30, TRUE, 0.009f) == 10, "trembling: not at rest on the tenth tick");
		return 0;
	}
	/* a tick in the air starts the count again: an item bouncing, however
	long, is never stopped */
	CASE("in the air")
	{
		int bounce;

		for (bounce = 0; bounce < 20; bounce++)
		{
			CHECK(!settles(&stuck_ticks, 9, TRUE, 0.f), "at rest before ten hits in a row");
			CHECK(!item_stuck_count(&stuck_ticks, FALSE, 0.f), "at rest in the air");
		}
		CHECK(settles(&stuck_ticks, 10, TRUE, 0.f) == 10, "ten in a row not at rest");
		return 0;
	}
	/* an item that hits the ground and gets somewhere (sliding down a slope,
	faster each tick) is never stopped */
	CASE("moving")
	{
		int tick;

		CHECK(!settles(&stuck_ticks, 100, TRUE, 0.011f), "at rest moving 1.1 cm a tick");
		for (tick = 0; tick < 100; tick++)
		{
			CHECK(!item_stuck_count(&stuck_ticks, TRUE, 0.0025f * (real)tick), "a slide at rest on tick %d", tick);
		}
		CHECK(!settles(&stuck_ticks, 100, TRUE, NAN), "at rest moving no number");
		return 0;
	}
	/* hits on anything but the ground (a vehicle, a biped, water) are not
	counted, and start the count again */
	CASE("not the ground")
	{
		CHECK(!settles(&stuck_ticks, 100, FALSE, 0.f), "at rest on a vehicle");
		CHECK(!settles(&stuck_ticks, 9, TRUE, 0.f), "early");
		CHECK(!item_stuck_count(&stuck_ticks, FALSE, 0.f), "at rest on a vehicle after nine ticks");
		CHECK(settles(&stuck_ticks, 10, TRUE, 0.f) == 10, "the count not started again");
		return 0;
	}
	/* a count that is not one (a saved game's, a datum's leftovers) counts
	from none */
	CASE("any count")
	{
		stuck_ticks = -7;
		CHECK(settles(&stuck_ticks, 30, TRUE, 0.f) == 10, "from -7");
		stuck_ticks = 300;
		CHECK(settles(&stuck_ticks, 30, TRUE, 0.f) == 10, "from 300");
		return 0;
	}

	return 2;
}
