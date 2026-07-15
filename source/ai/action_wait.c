/*
ACTION_WAIT.C

symbols in this file:
0000A4B0 0010:
	_action_wait_begin (0000)
0000A4C0 0180:
	_action_wait_perform (0000)
0000A640 00b0:
	_action_wait_control (0000)
0000A6F0 00b0:
	_action_wait_setup (0000)
0000A7A0 00e0:
	_action_wait_update (0000)
00243B18 0004:
	__real@41000000 (0000)
00243B1C 0020:
	??_C@_0CA@JCBCAIHB@c?3?2halo?2SOURCE?2ai?2action_wait?4c?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actions.h"

#include "actors.h"
#include "game/game.h"

/* ---------- constants */

/* ---------- macros */

/*
 * actors.h currently overstates the path-control block by four bytes.  The
 * target reads the `moving` flag at actor+0x504; keep this evidence-bounded
 * accessor local until the shared path layout is corrected as its own change.
 */
#define actor_wait_is_moving(actor) (*(boolean *)((byte *)(actor) + 0x504))

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void
action_wait_begin(
	long actor_index)
{
	return;
}

void
action_wait_control(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor_wait_is_moving(actor))
	{
		actor->orders.look.primary_priority = 3;
		actor->orders.look.primary_direction.type = _direction_specification_movement;
	}
	else if (!actor->external_orders.pursuit_is_coordinator &&
		actor->external_orders.pursuit_group_prop_index != NONE &&
		actor->state.action_data.wait.look_timer > 0)
	{
		actor->orders.look.primary_priority = 5;
		actor->orders.look.primary_direction.type = _direction_specification_prop;
		actor->orders.look.primary_direction.prop_index = actor->external_orders.pursuit_group_prop_index;
	}
	else
	{
		actor->orders.look.primary_priority = 1;
	}

	actor->orders.look.idle_look_type = 3;
	actor->orders.combat.shoot_at_target = FALSE;
	actor->orders.move.stationary_crouch = FALSE;
	actor->orders.move.moving_crouch = FALSE;
	actor->orders.move.panicked = FALSE;
	actor->orders.move.dive_into_cover = FALSE;
	actor->orders.move.emerge_from_cover = FALSE;
	return;
}

boolean action_wait_setup(
	long actor_index,
	boolean was_actively_searching,
	struct wait_state_data *state_data)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	match_assert("c:\\halo\\SOURCE\\ai\\action_wait.c", 33, state_data);
	memset(state_data, 0, sizeof(*state_data));
	if (!actor->input.vehicle_passenger)
	{
		state_data->waiting_as_coordinator = actor->external_orders.pursuit_is_coordinator;
		state_data->was_actively_searching = was_actively_searching;
		state_data->entry_time = game_time_get();
		state_data->exit_timer = 0;
		state_data->look_timer = 120;
		state_data->desire_move = TRUE;
		state_data->query_timer = seed_random_range(get_global_random_seed_address(), 300, 600);
		result = TRUE;
	}

	return result;
}

/* ---------- private code */
