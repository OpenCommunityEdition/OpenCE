/*
MAIN.C

StageKit (README.md): writes a scenario's map, in code.

usage: stagekit --list
       stagekit <scenario> <map file>
*/

/* ---------- headers */

#include "stagekit.h"

/* ---------- globals */

extern struct stagekit_scenario const scenario_walk;
extern struct stagekit_scenario const scenario_obstacle;
extern struct stagekit_scenario const scenario_elevator;

static struct stagekit_scenario const *const scenarios[] = { &scenario_walk, &scenario_obstacle, &scenario_elevator };

/* ---------- public code */

int main(
	int argc,
	char **argv)
{
	struct stage stage = { 0 };
	struct stagekit_scenario const *scenario = NULL;
	long tag_count;
	unsigned long index;

	if (argc == 2 && !strcmp(argv[1], "--list"))
	{
		for (index = 0; index < NUMBEROF(scenarios); index++)
			printf("%-12s %s\n", scenarios[index]->name, scenarios[index]->description);
		return 0;
	}
	for (index = 0; argc == 3 && index < NUMBEROF(scenarios); index++)
	{
		if (!strcmp(scenarios[index]->name, argv[1]))
			scenario = scenarios[index];
	}
	if (!scenario)
	{
		fprintf(stderr, "usage: stagekit --list\n       stagekit <scenario> <map file>\n");
		return 2;
	}
	stage_build(&stage, scenario);
	cache_write(argv[2], &tag_count);
	printf("stagekit: %s: %s, %ld tags\n", scenario->name, argv[2], tag_count);
	return 0;
}
