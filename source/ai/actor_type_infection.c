/*
ACTOR_TYPE_INFECTION.C

symbols in this file:
00027280 0130:
	_infection_decide_action (0000)
000273B0 0060:
	_code_000273b0 (0000)
00027410 0060:
	_code_00027410 (0000)
00027470 0e20:
	_code_00027470 (0000)
00028290 02b0:
	_infection_swarm_aim_jump (0000)
0024644C 000a:
	??_C@_09BGMFGBDN@infection?$AA@ (0000)
00246458 0004:
	__real@bfc90fdb (0000)
0024645C 0004:
	__real@3fc90fdb (0000)
00246460 0004:
	__real@bf666666 (0000)
00246464 003d:
	??_C@_0DN@KOBFBIIN@TEST_FLAG?$CIswarm_component?9?$DOflags@ (0000)
002464A8 004a:
	??_C@_0EK@OLPJENC@swarm_component?9?$DOwander?4move_tic@ (0000)
002464F4 0029:
	??_C@_0CJ@BNPOEG@c?3?2halo?2SOURCE?2ai?2actor_type_inf@ (0000)
00246520 0004:
	__real@40490fdb (0000)
00246524 0004:
	__real@bd888889 (0000)
00246528 0004:
	__real@3d99999a (0000)
0024652C 0004:
	__real@3df5c28f (0000)
002B6BC4 0020:
	_actor_type_infection (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "actions.h"
#include "actor_definitions.h"
#include "actors.h"
#include "actor_types.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void infection_decide_action(
	long actor_index);
void code_00027470(
	long actor_index);
void infection_swarm_aim_jump(
	long actor_index,
	void *arg1,
	void *arg2,
	void *arg3);

/* ---------- globals */

struct actor_type_definition actor_type_infection =
{
	"infection",
	32,
	2,
	2,
	2,
	0,
	TRUE,
	{ 0, 0 },
	NULL,
	infection_decide_action,
	code_00027470,
	infection_swarm_aim_jump
};

/* ---------- public code */

/* ---------- private code */

void infection_decide_action(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor_definition_get(actor->meta.definition_index);
	actor_action_handle_initial_action(actor_index);
	actor_action_handle_pending_command_list(actor_index);
	if (!actor_action_deny_transition(actor_index))
		actor_action_handle_combat_transition(actor_index);

	switch (actor->state.action)
	{
	case _actor_action_fight:
	case _actor_action_charge:
		if (!actor_action_handle_combat_status(actor_index, TRUE, FALSE))
		{
			actor_action_handle_combat_failure(actor_index);
			return;
		}
		break;

	case _actor_action_flee:
		if (actor->state.action_data.flee.unable_to_flee)
			actor_action_handle_combat_status(actor_index, TRUE, TRUE);
		else
			actor_action_handle_done_fleeing(actor_index);
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
			actor_action_handle_exit_pursuit(actor_index);
		return;

	case _actor_action_obey:
		actor_action_handle_combat_status(
			actor_index,
			actor->state.action_data.obey.initiative,
			actor->state.action_data.obey.finished);
		break;
	}

	return;
}
