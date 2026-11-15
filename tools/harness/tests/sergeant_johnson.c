/*
SERGEANT_JOHNSON.C (test)

The real sergeant_johnson_map, sergeant_johnson_leader_variant_in and
sergeant_johnson_scenario_variant_in (sergeant_johnson.c): a marine squad's
leader is Sergeant Johnson (the armored marine's variant 100) or Sergeant
Lehto (101) at random, and so Johnson led marines where a10's and a50's
story does not have him. In a10 and a50 a leader is never Johnson; in a50
the last marine out of the opening's Pelican (ini_marine_7) is, and no
other of a50's marines placed from the scenario (the gravity lift
cinematic's) is. Other maps keep the game's leaders.
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
	/* a10 and a50: Lehto for Johnson, the rest as the game has them */
	CASE("leaders")
	{
		CHECK(sergeant_johnson_leader_variant_in(a10, JOHNSON) == LEHTO, "Johnson led a10's marines");
		CHECK(sergeant_johnson_leader_variant_in(a50, JOHNSON) == LEHTO, "Johnson led a50's marines");
		CHECK(sergeant_johnson_leader_variant_in(a10, LEHTO) == LEHTO, "Lehto not kept");
		CHECK(sergeant_johnson_leader_variant_in(a50, 0) == 0, "a marine made a leader");
		return 0;
	}
	CASE("other maps")
	{
		CHECK(sergeant_johnson_leader_variant_in(other, JOHNSON) == JOHNSON, "another map's Johnson");
		CHECK(sergeant_johnson_leader_variant_in(other, LEHTO) == LEHTO, "another map's Lehto");
		CHECK(sergeant_johnson_scenario_variant_in(other, "ini_marine_7", ARMORED_MARINE, 0) == 0,
			"another map's ini_marine_7");
		CHECK(sergeant_johnson_scenario_variant_in(a10, "ini_marine_7", ARMORED_MARINE, 0) == 0,
			"a10's ini_marine_7");
		return 0;
	}
	/* a50's marines placed from the scenario */
	CASE("a50 objects")
	{
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", ARMORED_MARINE, 0) == JOHNSON,
			"the opening's last marine not Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "lift_marine_1", ARMORED_MARINE, 0) == 0,
			"the lift's first marine Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_1", ARMORED_MARINE, 0) == 0,
			"another opening marine Johnson");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", MARINE, 3) == 3,
			"another biped of the name");
		CHECK(sergeant_johnson_scenario_variant_in(a50, NULL, ARMORED_MARINE, 0) == 0, "no name");
		CHECK(sergeant_johnson_scenario_variant_in(a50, "ini_marine_7", NULL, 0) == 0, "no definition");
		return 0;
	}

	return 2;
}
