/*
SERGEANT_JOHNSON.C (test)

The real sergeant_johnson_map, sergeant_johnson_leader_variant_in and
sergeant_johnson_scenario_variant_in (sergeant_johnson.c): a marine squad's
leader is Sergeant Johnson (the armored marine's variant 100) or Sergeant
Lehto (101) at random, and so Johnson led marines where a10's and a50's
story does not have him. In a10 a leader is never Johnson; in a50 the last
marine out of the opening's Pelican (ini_marine_7) is, and the gravity lift
cinematic's first marine and the gravity lift room's leader are Johnson if
he made it to the lift, the room's leader Lehto if not, and no other
leader is Johnson. Other maps keep the game's leaders.
test_sergeant_johnson.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define csstrcmp strcmp

#include "under_test.inc"

#define JOHNSON 100
#define LEHTO 101
#define ARMORED_MARINE "characters\\marine_armored\\marine_armored"
#define MARINE "characters\\marine\\marine"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short a10 = sergeant_johnson_map("levels\\a10\\a10");
	short a50 = sergeant_johnson_map("levels\\a50\\a50");
	short other = sergeant_johnson_map("levels\\b30\\b30");

	CASE("maps")
	{
		CHECK(a10 != other && a50 != other && a10 != a50, "a10 %d a50 %d other %d", a10, a50, other);
		CHECK(sergeant_johnson_map(NULL) == other, "no name: %d", sergeant_johnson_map(NULL));
		CHECK(sergeant_johnson_map("levels\\a10\\a10_custom") == other, "a map named like a10");
		return 0;
	}
	/* a10: Lehto for Johnson, the rest as the game has them */
	CASE("a10 leaders")
	{
		CHECK(sergeant_johnson_leader_variant_in(a10, "lifepod_2", JOHNSON, TRUE) == LEHTO, "Johnson led lifepod_2");
		CHECK(sergeant_johnson_leader_variant_in(a10, "stairs", JOHNSON, FALSE) == LEHTO, "Johnson led stairs");
		CHECK(sergeant_johnson_leader_variant_in(a10, "lifepod_2", LEHTO, TRUE) == LEHTO, "Lehto not kept");
		CHECK(sergeant_johnson_leader_variant_in(a10, "lifepod_2", 0, TRUE) == 0, "a marine made a leader");
		CHECK(sergeant_johnson_leader_variant_in(a10, "gravity_pad_marines", LEHTO, TRUE) == LEHTO,
			"a10's encounter of a50's name");
		return 0;
	}
	/* a50: the gravity lift room's leader is Johnson if he made it, Lehto if
	not; no other leader is Johnson; a marine is no leader */
	CASE("a50 leaders")
	{
		CHECK(sergeant_johnson_leader_variant_in(a50, "gravity_pad_marines", LEHTO, TRUE) == JOHNSON,
			"Johnson made it, Lehto leads the room");
		CHECK(sergeant_johnson_leader_variant_in(a50, "gravity_pad_marines", JOHNSON, TRUE) == JOHNSON,
			"Johnson made it, and does not lead the room");
		CHECK(sergeant_johnson_leader_variant_in(a50, "gravity_pad_marines", JOHNSON, FALSE) == LEHTO,
			"Johnson died, and leads the room");
		CHECK(sergeant_johnson_leader_variant_in(a50, "gravity_pad_marines", 0, TRUE) == 0,
			"a marine of the room made Johnson");
		CHECK(sergeant_johnson_leader_variant_in(a50, "hangar_marines", JOHNSON, TRUE) == LEHTO,
			"Johnson leads the hangar's marines");
		CHECK(sergeant_johnson_leader_variant_in(a50, "marines_area4", JOHNSON, TRUE) == LEHTO,
			"another Johnson outside");
		CHECK(sergeant_johnson_leader_variant_in(a50, "hangar_marines", LEHTO, FALSE) == LEHTO, "Lehto not kept");
		return 0;
	}
	CASE("other maps")
	{
		CHECK(sergeant_johnson_leader_variant_in(other, "gravity_pad_marines", JOHNSON, FALSE) == JOHNSON,
			"another map's Johnson");
		CHECK(sergeant_johnson_leader_variant_in(other, "x", LEHTO, TRUE) == LEHTO, "another map's Lehto");
		CHECK(sergeant_johnson_scenario_variant_in(other, "ini_marine_7", ARMORED_MARINE, 0, TRUE) == 0,
			"another map's ini_marine_7");
		CHECK(sergeant_johnson_scenario_variant_in(a10, "lift_marine_1", ARMORED_MARINE, 0, TRUE) == 0,
			"a10's lift_marine_1");
		return 0;
	}
	/* a50's marines placed from the scenario */
	CASE("a50 objects")
	{
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", ARMORED_MARINE, 0, FALSE) == JOHNSON,
			"the opening's last marine not Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "lift_marine_1", ARMORED_MARINE, 0, TRUE) == JOHNSON,
			"the lift's first marine not Johnson, though he made it");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "lift_marine_1", ARMORED_MARINE, 0, FALSE) == 0,
			"the lift's first marine Johnson, though he died");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "lift_marine_2", ARMORED_MARINE, 0, TRUE) == 0,
			"another lift marine Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_1", ARMORED_MARINE, 0, TRUE) == 0,
			"another opening marine Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", MARINE, 3, TRUE) == 3,
			"another biped of the name");
		CHECK(sergeant_johnson_scenario_variant_in(a50, NULL, ARMORED_MARINE, 0, TRUE) == 0, "no name");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", NULL, 0, TRUE) == 0, "no definition");
		return 0;
	}

	return 2;
}
