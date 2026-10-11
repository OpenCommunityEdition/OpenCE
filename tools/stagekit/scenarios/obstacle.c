/*
OBSTACLE.C

A wall 0.5 thick, 20 long and 2 high, a unit in front of the player
(tests/test_obstacle.py: the biped stops at it, never inside or behind it).
*/

#include "../stagekit.h"

static void obstacle_build(
	struct stage *stage)
{
	real_rectangle3d wall = { { -0.25f, 0.25f, -10.f, 10.f, 0.f, 2.f } };
	real_point3d position = { { 1.25f, 0.f, 0.f } };

	prop_obstacle(stage, "stagekit\\obstacle", &wall, &position);
}

struct stagekit_scenario const scenario_obstacle = {
	"obstacle", "a wall the biped walks into and stops at", obstacle_build };
