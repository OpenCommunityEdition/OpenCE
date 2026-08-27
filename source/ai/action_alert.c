/*
ACTION_ALERT.C

symbols in this file:
00000600 0080:
	_action_alert_setup (0000)
00000680 0010:
	_action_alert_begin (0000)
00000690 0050:
	_action_alert_control (0000)
000006E0 0030:
	_action_alert_flush_position_indices (0000)
00000710 0030:
	_action_alert_flush_structure_indices (0000)
00000740 0030:
	_vector_from_points3d (0000)
00000770 0030:
	_magnitude_squared3d (0000)
000007A0 0040:
	_distance_squared3d (0000)
000007E0 0020:
	_real_random_range (0000)
00000800 0150:
	_action_alert_update (0000)
00000950 0310:
	_code_00000950 (0000)
00000C60 0260:
	_action_alert_perform (0000)
00242EEC 000b:
	??_C@_0L@ILHIFHGP@state_data?$AA@ (0000)
00242EF8 0021:
	??_C@_0CB@JGDDPGCE@c?3?2halo?2SOURCE?2ai?2action_alert?4c@ (0000)
00242F1C 0004:
	__real@3e800000 (0000)
00242F20 0013:
	??_C@_0BD@KDCHGMEK@?$CBactor?9?$DOmeta?4swarm?$AA@ (0000)
00242F34 0004:
	__real@41f00000 (0000)
00242F38 0004:
	__real@3f000000 (0000)
*/

/* ---------- headers */

#define vector_from_points3d vector_from_points3d_inline
#define magnitude_squared3d magnitude_squared3d_inline
#define real_random_range real_random_range_inline
#include "cseries.h"
#include "actions.h"
#include "math/real_math.h"
#undef vector_from_points3d
#undef magnitude_squared3d
#undef real_random_range

#include "actor_definitions.h"
#include "actors.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void action_alert_begin(
	long actor_index)
{
	return;
}

void action_alert_control(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition =
		actor_definition_get(actor->meta.definition_index);

	actor->orders.look.idle_look_type = 1;
	if (TEST_FLAG(definition->flags,
		_actor_definition_crouch_when_noncombat_bit))
	{
		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.move.moving_crouch = TRUE;
	}

	return;
}

real real_random_range(
	real lower_bound,
	real upper_bound)
{
	return real_seed_random_range(
		get_global_random_seed_address(),
		lower_bound,
		upper_bound);
}

real_vector3d *vector_from_points3d(
	real_point3d const *a,
	real_point3d const *b,
	real_vector3d *result)
{
	result->i = b->x - a->x;
	result->j = b->y - a->y;
	result->k = b->z - a->z;
	return result;
}

real magnitude_squared3d(
	real_vector3d const *v)
{
	return v->i * v->i + v->j * v->j + v->k * v->k;
}

/* ---------- private code */
