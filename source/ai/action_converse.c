/*
ACTION_CONVERSE.C

symbols in this file:
000028A0 00b0:
	_action_converse_setup (0000)
00002950 0010:
	_action_converse_begin (0000)
00002960 0100:
	_action_converse_perform (0000)
00002A60 0020:
	_action_converse_update (0000)
00002A80 0090:
	_action_converse_control (0000)
00002B10 0030:
	_action_converse_replace_prop (0000)
00002B40 0080:
	_actor_conversation_control (0000)
00002BC0 0030:
	_actor_conversation_end (0000)
00002BF0 0030:
	_action_converse_end (0000)
0024305C 0024:
	??_C@_0CE@NNMIMHKJ@c?3?2halo?2SOURCE?2ai?2action_convers@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actions.h"

#include "actors.h"
#include "ai_communication.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void action_converse_begin(
	long actor_index)
{
	return;
}

void action_converse_update(
	long actor_index)
{
	actor_get(actor_index);

	return;
}

void action_converse_replace_prop(
	long actor_index,
	long old_prop_index,
	long new_prop_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct converse_state_data *state_data = &actor->state.action_data.converse;

	if (state_data->run_to_prop_index == old_prop_index)
		state_data->run_to_prop_index = new_prop_index;

	return;
}

void actor_conversation_end(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->external_orders.conversation_index != NONE)
	{
		ai_conversation_finish(
			actor->external_orders.conversation_index,
			FALSE,
			FALSE);
	}

	return;
}

void action_converse_end(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->external_orders.conversation_index != NONE)
	{
		ai_conversation_finish(
			actor->external_orders.conversation_index,
			FALSE,
			FALSE);
	}

	return;
}

/* ---------- private code */
