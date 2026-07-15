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

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct scenery_datum
{
	struct object_datum object;
	unsigned long flags;
};

typedef char verify_scenery_extension_offset[
	sizeof(struct object_datum) == 0x1A4 ? 1 : -1];

/* ---------- prototypes */

short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *state,
	boolean unknown);

/* ---------- globals */

/* ---------- public code */

void
scenery_initialize(
	void)
{
}

void
scenery_initialize_for_new_map(
	void)
{
}

void
scenery_dispose_from_old_map(
	void)
{
}

void
scenery_dispose(
	void)
{
}

void
scenery_delete(
	long object_index)
{
}

boolean scenery_update(
	long object_index)
{
	long animation_graph_index;
	struct scenery_datum *scenery = (struct scenery_datum *)object_get_and_verify_type(
		object_index,
		FLAG(_object_type_scenery));

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

/* ---------- private code */
