/*
WALK.C

The stage alone: a biped spawns in the chamber, walks and jumps
(tests/test_walk.py: it stays on the floor, moves and jumps).
*/

#include "../stagekit.h"

static void walk_build(
	struct stage *stage)
{
	(void)stage;
}

struct stagekit_scenario const scenario_walk = {
	"walk", "a biped spawns in the chamber, walks and jumps", walk_build };
