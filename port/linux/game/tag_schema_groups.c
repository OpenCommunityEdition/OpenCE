/*
TAG_SCHEMA_GROUPS.C

The tag groups' schemas (tag_schema.h), from each tag_schema_*.c.
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"

/* ---------- globals */

struct tag_schema_group const *const tag_schema_group_lists[] =
{
	tag_schema_object_groups,
	tag_schema_model_groups,
	tag_schema_collision_groups,
	tag_schema_render_groups,
	tag_schema_effect_groups,
	tag_schema_scenario_groups,
	NULL
};
