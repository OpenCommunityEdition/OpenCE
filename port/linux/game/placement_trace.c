/* placement_trace.c: the reference traces of the native engine's placements
(conductor/tracks/dev_tooling_20261021, tools/engine_placement_traces.py).

With debug.placement_trace on, each object the scenario places is logged
as it is made (object_new_from_scenario, after its type has placed it),
after a line that names the level's scenario,

	placement trace: begin <scenario tag>

as

	placement trace: <type> <block index> <x> <y> <z> <forward i j k>
		<up i j k> <regions> <parent> <tag>

with its object type (0 biped to 11 sound scenery), its index in its type's
block of the scenario, its position, forward and up, its eight regions'
permutations joined by dots, its parent's object index (-1 for none) and
its definition's tag path last (it has spaces in some maps). At the first
tick after them

	placement trace: done <structure bsp> <difficulty> <count>

ends the level's placements; any made after it (by the scripts) are not the
level's. A level that loads later (the menus' first) begins another. Usually with debug.network_test "host:<map>" (a game with no
menus). */

#include "cseries.h"
#include "game/game.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "objects/object_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"

#include "placement_trace.h"

#include <stdio.h>
#include <string.h>

/* the platform layer's (port/linux/src/port_config.c) */
int config_boolean(char const *name);
void platform_log(char const *format, ...);

static struct
{
	boolean read;
	boolean on;
	boolean done;
	long count;
	/* the level's scenario (its tag index is the same in every map) */
	char scenario[256];
} placement_trace = { FALSE, FALSE, TRUE, 0, "" };

static boolean placement_trace_on(void)
{
	if (!placement_trace.read)
	{
		placement_trace.read = TRUE;
		placement_trace.on = config_boolean("debug.placement_trace") ? TRUE : FALSE;
	}
	return placement_trace.on;
}

/* a level's start: when its scenario is not the last one's */
static void placement_trace_begin(void)
{
	char const *scenario = tag_get_name(global_scenario_index);

	if (!scenario)
	{
		scenario = "?";
	}
	if (strncmp(placement_trace.scenario, scenario, sizeof(placement_trace.scenario) - 1))
	{
		strncpy(placement_trace.scenario, scenario, sizeof(placement_trace.scenario) - 1);
		placement_trace.done = FALSE;
		placement_trace.count = 0;
		platform_log("placement trace: begin %s", scenario);
	}
}

/* the placement's index in its type's block, or NONE */
static long placement_trace_block_index(short object_type, struct scenario_object_datum const *scenario_object)
{
	struct scenario *scenario = global_scenario_get();
	struct tag_block *datums;
	long size = 0;
	long offset;

	if (!scenario || object_type<0 || object_type>=NUMBER_OF_OBJECT_TYPES ||
		object_type==_object_type_garbage || object_type==_object_type_projectile)
	{
		return NONE;
	}
	datums = scenario_get_object_type_scenario_datums(scenario, object_type, &size);
	if (!datums || size<=0 || !datums->address)
	{
		return NONE;
	}
	offset = (long)((char const *)scenario_object - (char const *)datums->address);
	if (offset<0 || offset%size!=0 || offset/size>=datums->count)
	{
		return NONE;
	}
	return offset/size;
}

void placement_trace_placed(long object_index, struct scenario_object_datum const *scenario_object)
{
	struct object_datum *object;
	char regions[8 * 4 + 1];
	char *tag;
	short region;
	int length = 0;

	if (!placement_trace_on())
	{
		return;
	}
	placement_trace_begin();
	if (placement_trace.done)
	{
		/* (a script's, after the level's) */
		return;
	}
	object = object_get(object_index);
	for (region = 0; region<MAXIMUM_REGIONS_PER_OBJECT; region++)
	{
		length += sprintf(regions + length, region ? ".%d" : "%d", object->object.region_permutations[region]);
	}
	tag = tag_get_name(object->definition_index);
	platform_log("placement trace: %d %ld %.5f %.5f %.5f %.6f %.6f %.6f %.6f %.6f %.6f %s %ld %s",
		object->object.type,
		placement_trace_block_index(object->object.type, scenario_object),
		object->object.position.x, object->object.position.y, object->object.position.z,
		object->object.forward.i, object->object.forward.j, object->object.forward.k,
		object->object.up.i, object->object.up.j, object->object.up.k,
		regions,
		object->object.parent_object_index,
		tag ? tag : "?");
	placement_trace.count++;
}

void placement_trace_update(void)
{
	if (!placement_trace_on())
	{
		return;
	}
	/* (a level that places nothing) */
	placement_trace_begin();
	if (placement_trace.done)
	{
		return;
	}
	placement_trace.done = TRUE;
	platform_log("placement trace: done %d %d %ld",
		global_structure_bsp_index_get(), game_difficulty_level_get(), placement_trace.count);
}
