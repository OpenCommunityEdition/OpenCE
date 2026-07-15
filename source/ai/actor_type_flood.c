/*
ACTOR_TYPE_FLOOD.C

symbols in this file:
00026810 0170:
	_flood_decide_action (0000)
00026980 0510:
	_actor_type_flood_desire_shamble (0000)
002463CC 0006:
	??_C@_05ONANONLM@flood?$AA@ (0000)
002463D4 0004:
	__real@41f80000 (0000)
002463D8 0032:
	??_C@_0DC@EGHABMGG@actor?9?$DOemotions?4crouch_switching@ (0000)
0024640C 0004:
	__real@bfb33333 (0000)
00246410 0004:
	__real@3fb33333 (0000)
00246414 0025:
	??_C@_0CF@GOCGFILI@c?3?2halo?2SOURCE?2ai?2actor_type_flo@ (0000)
002B6B64 0020:
	_actor_type_flood (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "actor_definitions.h"
#include "actor_types.h"
#include "actors.h"
#include "actions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void flood_decide_action(
	long actor_index);

/* ---------- globals */

struct actor_type_definition actor_type_flood =
{
	"flood",
	8,
	0,
	0,
	0,
	1,
	FALSE,
	{ 0, 0, 0 },
	flood_decide_action,
	NULL,
	NULL
};

/* ---------- public code */

/* ---------- private code */

void flood_decide_action(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor_definition_get(actor->meta.definition_index);
	actor_action_handle_initial_action(actor_index);
	actor_action_handle_pending_command_list(actor_index);
	actor_action_handle_surprise(actor_index, 4);
	if (!actor_action_deny_transition(actor_index))
	{
		actor_action_handle_berserking_from_damage(actor_index);
		actor_action_handle_berserk_transition(actor_index, 3);
		actor_action_handle_combat_transition(actor_index);
		actor_action_handle_danger_avoidance(actor_index);
	}

	switch (actor->state.action)
	{
	case _actor_action_fight:
	case _actor_action_charge:
		if (actor_action_handle_combat_status(actor_index, TRUE, FALSE) ||
			actor_action_handle_combat_failure(actor_index))
		{
			return;
		}
		actor_action_handle_evasion(actor_index);
		return;

	case _actor_action_flee:
		if (actor->state.action_data.flee.unable_to_flee)
		{
			actor_action_handle_combat_status(actor_index, TRUE, TRUE);
		}
		else
		{
			actor_action_handle_done_fleeing(actor_index);
		}
		return;

	case _actor_action_guard:
		actor_action_handle_combat_status(
			actor_index,
			actor_action_can_stop_guarding(actor_index, _actor_action_fight, _actor_action_guard),
			FALSE);
		return;

	case _actor_action_uncover:
	case _actor_action_search:
	case _actor_action_wait:
		if (!actor_action_handle_combat_status(actor_index, TRUE, FALSE))
		{
			actor_action_handle_exit_pursuit(actor_index);
		}
		return;

	case _actor_action_obey:
		actor_action_handle_combat_status(
			actor_index,
			actor->state.action_data.obey.initiative,
			actor->state.action_data.obey.finished);
		return;

	case _actor_action_avoid:
		if (actor->danger_zone.danger_type == 0)
		{
			actor_action_handle_combat_status(actor_index, TRUE, TRUE);
		}
		return;
	}

	return;
}
