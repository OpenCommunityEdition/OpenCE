/*
SCENERY.C

symbols in this file:
00134D70 0010:
	_scenery_initialize (0000)
00134D80 0010:
	_scenery_initialize_for_new_map (0000)
00134D90 0010:
	_scenery_dispose_from_old_map (0000)
00134DA0 0010:
	_scenery_dispose (0000)
00134DB0 0020:
	_scenery_place (0000)
00134DD0 0080:
	_scenery_new (0000)
00134E50 0010:
	_scenery_delete (0000)
00134E60 0050:
	_scenery_update (0000)
00134EB0 00e0:
	_code_00134eb0 (0000)
00134F90 0070:
	_scenery_get_animation_time (0000)
00135000 0020:
	_scenery_animation_start (0000)
00135020 0020:
	_scenery_animation_start_at_frame (0000)
0028AC4C 0033:
	??_C@_0DD@PMOLPF@the?5animation?5?8?$CFs?8?5doesn?8t?5exist@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "object_types.h"
#include "objects.h"
#include "scenery.h"

#include "models/model_animation_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *state,
	boolean unknown);

/* ---------- globals */

/* ---------- public code */

void scenery_initialize(
	void)
{
	return;
}

void scenery_initialize_for_new_map(
	void)
{
	return;
}

void scenery_dispose_from_old_map(
	void)
{
	return;
}

void scenery_dispose(
	void)
{
	return;
}

void scenery_delete(
	long object_index)
{
	return;
}

boolean scenery_update(
	long object_index)
{
	long animation_graph_index;
	struct scenery_datum *scenery = scenery_get(object_index);

	if (TEST_FLAG(scenery->flags, 0))
	{
		animation_graph_index = scenery->object.object.animation.animation_graph_index;
		switch (animation_update_internal(
			1,
			animation_graph_index,
			&scenery->object.object.animation.state,
			FALSE))
		{
		case 2:
			scenery->object.object.animation.state.frame_index--;
			break;
		}
	}

	return TRUE;
}

short scenery_get_animation_time(
	long object_index)
{
	struct animation *animation;
	struct animation_graph *animation_graph;
	struct scenery_datum *scenery = scenery_get(object_index);

	if (TEST_FLAG(scenery->flags, 0))
	{
		animation_graph = animation_graph_definition_get(
			scenery->object.object.animation.animation_graph_index);
		animation = TAG_BLOCK_GET_ELEMENT(
			&animation_graph->animations,
			scenery->object.object.animation.state.index,
			struct animation);

		return MAX(
			animation->frame_count - scenery->object.object.animation.state.frame_index - 2,
			0);
	}

	return 0;
}

/* ---------- private code */
