/*
ELEVATOR.C

A platform 6 by 6 and 0.25 high on the floor, with the player's start on it,
that rises 5 units in 2 seconds when it is set to, and comes down when set
back (tests/test_elevator.py: whoever stands on it rides it).
*/

#include "../stagekit.h"

static void elevator_build(
	struct stage *stage)
{
	real_rectangle3d platform = { { -3.f, 3.f, -3.f, 3.f, 0.f, 0.25f } };
	real_point3d position = { { 0.f, 0.f, 0.f } };
	real_point3d start = { { -1.f, 0.f, 0.25f } };

	prop_elevator(stage, "elevator", &platform, &position, 5.f, 2.f);
	player_start(stage, &start);
}

struct stagekit_scenario const scenario_elevator = {
	"elevator", "a platform the player rides up and down", elevator_build };
