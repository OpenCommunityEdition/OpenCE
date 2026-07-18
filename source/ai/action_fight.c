/*
ACTION_FIGHT.C

symbols in this file:
00002C20 0040:
	_action_fight_setup (0000)
00002C60 0010:
	_action_fight_begin (0000)
00002C70 0010:
	_action_fight_end (0000)
00002C80 0070:
	_action_fight_update (0000)
00002CF0 0080:
	_action_fight_control (0000)
00002D70 03d0:
	_action_fight_perform (0000)
00243080 0021:
	??_C@_0CB@KJFGPOLC@c?3?2halo?2SOURCE?2ai?2action_fight?4c@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actions.h"

#include "actors.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void actor_discard_firing_position(
	long actor_index,
	short firing_position_index,
	boolean temporary);

/* ---------- globals */

/* ---------- public code */

boolean action_fight_setup(
	long actor_index,
	struct fight_state_data *state_data)
{
	match_assert("c:\\halo\\SOURCE\\ai\\action_fight.c", 30, state_data);
	memset(state_data, 0, sizeof(*state_data));
	return TRUE;
}

void action_fight_begin(
	long actor_index)
{
	return;
}

void action_fight_end(
	long actor_index)
{
	return;
}

/* ---------- private code */

void
action_fight_update(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct fight_state_data *state_data = &actor->state.action_data.fight;

	if (state_data->firing_position_timer > 0 && actor->control.path.at_destination)
	{
		state_data->firing_position_timer--;
		if (state_data->firing_position_timer == 0 &&
			actor->firing_positions.current_position_index != NONE &&
			!actor->firing_positions.current_position_found_outside_range)
		{
			actor_discard_firing_position(
				actor_index,
				actor->firing_positions.current_position_index,
				FALSE);
		}
	}

	return;
}

void
action_fight_control(
	long actor_index)
{
	/* NonMatching: target and candidate are both 0x80 bytes with both
	   relocations exact. January reuses CL for defensive_crouch and then
	   FALSE, while this TU selects DL for the first value. Five legal-C
	   lifetime and statement-order shapes did not remove the register tie. */
	struct actor_datum *actor = actor_get(actor_index);

	actor->orders.move.stationary_crouch = actor->emotions.defensive_crouch;
	actor->orders.look.primary_priority = 5;
	actor->orders.look.primary_direction.type = 2;
	actor->orders.look.idle_look_type = 4;
	actor->orders.move.moving_crouch = FALSE;
	actor->orders.move.panicked = FALSE;
	actor->orders.move.dive_into_cover = FALSE;
	actor->orders.move.emerge_from_cover = FALSE;

	if (actor->input.vehicle_driver_type != 4 && actor->state.combat_status >= 5)
	{
		actor->orders.combat.shoot_at_target = TRUE;
		actor->orders.look.primary_priority = 7;
	}

	return;
}
