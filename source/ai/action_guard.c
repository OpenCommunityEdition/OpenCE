/*
ACTION_GUARD.C

symbols in this file:
00003E80 0080:
	_action_guard_setup_current_position (0000)
00003F00 00d0:
	_action_guard_setup_find_position (0000)
00003FD0 0160:
	_action_guard_setup_postcombat (0000)
00004130 0040:
	_action_guard_begin (0000)
00004170 0040:
	_action_guard_end (0000)
000041B0 0140:
	_code_000041b0 (0000)
000042F0 01d0:
	_action_guard_update (0000)
000044C0 0080:
	_action_guard_flush_position_indices (0000)
00004540 0030:
	_action_guard_flush_structure_indices (0000)
00004570 00a0:
	_action_guard_modify_color (0000)
00004610 0050:
	_action_guard_replace_prop (0000)
00004660 01c0:
	_action_guard_setup_from_combat_transition (0000)
00004820 01c0:
	_action_guard_setup_from_fleeing (0000)
000049E0 01c0:
	_action_guard_perform (0000)
00004BA0 03d0:
	_action_guard_control (0000)
002431D8 0021:
	??_C@_0CB@GJJCDDDH@c?3?2halo?2SOURCE?2ai?2action_guard?4c@ (0000)
002431FC 0018:
	??_C@_0BI@NGADMAPA@state_data?9?$DOpost_combat?$AA@ (0000)
00243214 0034:
	??_C@_0DE@PILBLNDH@actor?9?$DOstimuli?4combat_transition@ (0000)
00243248 0021:
	??_C@_0CB@CNNLKLII@actor?9?$DOstimuli?4combat_transition@ (0000)
0024326C 0004:
	__real@41100000 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actions.h"

#include "actors.h"
#include "props.h"

/* ---------- constants */

/* ---------- macros */

/*
 * January cachebeta layout evidence.  The inherited HCEX prop and actor
 * control layouts differ at these fields; keep the accessors local until the
 * shared structures are corrected independently.
 */
#define action_guard_prop_is_dead(prop) (*(boolean *)((byte *)(prop) + 0x60))
#define action_guard_actor_is_moving(actor) (*(boolean *)((byte *)(actor) + 0x484))
#define action_guard_cower_retreat_timer(actor) (*(short *)((byte *)(actor) + 0x3A8))

/* ---------- structures */

/* ---------- prototypes */

boolean game_team_is_ally(
	short team_a,
	short team_b);

void ai_communication_event(
	short type,
	long unit_index,
	long prop_index,
	long object_index,
	long position_index,
	long structure_index,
	boolean allow_reply);

void actor_set_dormant(
	long actor_index,
	boolean dormant);

long actor_target_unit_index(
	long actor_index);

void actor_discard_firing_position(
	long actor_index,
	short firing_position_index,
	boolean temporary);

/* ---------- globals */

/* ---------- public code */

/* ---------- private code */

/* action_guard_post_combat */
static boolean code_000041b0(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct guard_state_data *state_data = &actor->state.action_data.guard;
	boolean result = FALSE;

	match_assert("c:\\halo\\SOURCE\\ai\\action_guard.c", 344, state_data->post_combat);
	if (!state_data->post_combat_vocalized)
	{
		short communication_type = NONE;

		switch (actor->external_orders.postcombat_type)
		{
		case _actor_postcombat_speak_alone:
			communication_type = 48;
			break;
		case _actor_postcombat_speak_unscathed:
			communication_type = 49;
			break;
		case _actor_postcombat_speak_wounded:
			communication_type = 50;
			break;
		case _actor_postcombat_speak_massacre:
			communication_type = 51;
			break;
		case _actor_postcombat_speak_triumph:
			communication_type = 52;
			break;
		case _actor_postcombat_check_enemy:
			communication_type = 53;
			break;
		case _actor_postcombat_check_friend:
			communication_type = 54;
			break;
		case _actor_postcombat_shoot_corpse:
			communication_type = 55;
			break;
		case _actor_postcombat_celebrate:
			communication_type = 56;
			break;
		}

		if (communication_type != NONE)
		{
			long prop_unit_index = NONE;
			long prop_type = NONE;

			if (state_data->guard_look_prop_index != NONE)
			{
				struct prop_datum *prop = prop_get(state_data->guard_look_prop_index);

				prop_unit_index = prop->unit_index;
				if (action_guard_prop_is_dead(prop))
				{
					prop_type = game_team_is_ally(actor->meta.team_index, prop->team_index) ? 4 : 3;
				}
				else
				{
					prop_type = 2;
				}
			}

			ai_communication_event(
				communication_type,
				actor->meta.unit_index,
				prop_unit_index,
				prop_type,
				NONE,
				NONE,
				FALSE);
			result = TRUE;
		}

		state_data->post_combat_vocalized = TRUE;
	}

	return result;
}

void
action_guard_update(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct guard_state_data *state_data = &actor->state.action_data.guard;

	if (!actor->meta.dormant &&
		action_guard_actor_is_moving(actor) &&
		state_data->wait_ticks > 0)
	{
		state_data->wait_ticks--;
		if (state_data->wait_ticks == 0 &&
			!actor->input.vehicle_passenger &&
			!actor->meta.swarm)
		{
			if (state_data->post_combat)
			{
				code_000041b0(actor_index);
				actor->external_orders.postcombat_type = _actor_postcombat_none;
				actor->external_orders.postcombat_prop_index = NONE;
				state_data->post_combat = FALSE;
				state_data->post_combat_shooting = FALSE;
				state_data->guard_look_prop_index = NONE;
			}
			state_data->find_new_guard_position = TRUE;
		}
	}

	if (state_data->look_ticks > 0 &&
		(!state_data->guard_look_until_reached_point || action_guard_actor_is_moving(actor)))
	{
		state_data->look_ticks--;
		if (state_data->look_ticks == 0)
		{
			state_data->guard_look_prop_index = NONE;
		}
	}

	if (state_data->path_begun && action_guard_actor_is_moving(actor))
	{
		boolean cower_finished;

		if (state_data->cower_from_retreat)
		{
			state_data->cower_from_retreat = action_guard_cower_retreat_timer(actor) > 0;
			cower_finished = !state_data->cower_from_retreat;
		}
		else if (state_data->cower_ticks > 0)
		{
			state_data->cower_ticks--;
			cower_finished = state_data->cower_ticks == 0;
		}
		else
		{
			cower_finished = FALSE;
		}

		if (cower_finished)
		{
			actor_set_dormant(actor_index, FALSE);
			state_data->cower = FALSE;
			state_data->cower_panicked = FALSE;
			state_data->cower_from_retreat = FALSE;
			state_data->cower_ticks = 0;

			if (actor->state.combat_status >= 2 && actor->meta.unit_index != NONE)
			{
				ai_communication_event(
					35,
					actor->meta.unit_index,
					actor_target_unit_index(actor_index),
					NONE,
					NONE,
					NONE,
					FALSE);
			}

			if (state_data->guard_location_type == 3)
			{
				actor_discard_firing_position(
					actor_index,
					state_data->guard_firing_position_index,
					FALSE);
				state_data->guard_firing_position_index = NONE;
			}

			actor->firing_positions.current_position_index = NONE;
			if (actor->input.vehicle_passenger)
			{
				state_data->guard_location_type = 1;
				return;
			}

			state_data->guard_location_type = 0;
			state_data->find_new_guard_position = TRUE;
		}
	}

	return;
}
