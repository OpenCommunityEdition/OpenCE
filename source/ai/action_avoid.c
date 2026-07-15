/*
ACTION_AVOID.C

symbols in this file:
00000EC0 0040:
	_action_avoid_setup (0000)
00000F00 0010:
	_action_avoid_begin (0000)
00000F10 0010:
	_action_avoid_end (0000)
00000F20 00c0:
	_action_avoid_perform (0000)
00000FE0 0010:
	_action_avoid_update (0000)
00000FF0 0090:
	_action_avoid_control (0000)
00242F3C 0021:
	??_C@_0CB@LDCPOHMO@c?3?2halo?2SOURCE?2ai?2action_avoid?4c@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actions.h"

#include "actors.h"

/* ---------- constants */

enum
{
	_action_avoid_firing_position_group = 6,
	_action_avoid_primary_target_priority = 7,
	_action_avoid_primary_danger_priority = 5,
	_action_avoid_idle_look_type = 4,
};

/* ---------- macros */

/* ---------- structures */

/*
 * These are work records owned by actor_firing_position.c.  That object has
 * not yet been reconstructed, so only the field consumed here is named; the
 * remaining bytes are an evidence-bounded temporary gap (0x670 bytes total).
 */
struct firing_position_search_definition
{
	long allowed_groups;
	short firing_position_group;
	byte unresolved[0x66A];
};

struct firing_position_search_workspace
{
	byte unresolved[0x1408C];
};

struct firing_position_candidate
{
	byte unresolved[0x3C];
};

/* ---------- prototypes */

long actor_active_select_firing_position(
	long actor_index,
	struct firing_position_search_definition *search,
	struct firing_position_candidate *candidate,
	long *previous_owner_actor_index,
	struct firing_position_search_workspace *workspace,
	long *position_flags);

short actor_change_firing_position(
	long actor_index,
	long firing_position_index,
	struct firing_position_candidate *candidate,
	long previous_owner_actor_index,
	struct firing_position_search_workspace *workspace,
	long position_flags);

/* ---------- globals */

/* ---------- public code */

boolean action_avoid_setup(
	long actor_index,
	struct avoid_state_data *state_data)
{
	match_assert("c:\\halo\\SOURCE\\ai\\action_avoid.c", 30, state_data);
	memset(state_data, 0, sizeof(*state_data));
	return TRUE;
}

void
action_avoid_begin(
	long actor_index)
{
}

void
action_avoid_end(
	long actor_index)
{
}

boolean action_avoid_perform(
	long actor_index)
{
	long position_flags;
	long previous_owner_actor_index;
	long firing_position_index;
	struct firing_position_candidate candidate;
	struct firing_position_search_definition search;
	struct firing_position_search_workspace workspace;
	struct actor_datum *actor = actor_get(actor_index);

	match_assert("c:\\halo\\SOURCE\\ai\\action_avoid.c", 55, !actor->meta.swarm);
	if (actor->meta.timeslice)
	{
		memset(&search, 0, sizeof(search));
		search.firing_position_group = _action_avoid_firing_position_group;
		firing_position_index = actor_active_select_firing_position(
			actor_index,
			&search,
			&candidate,
			&previous_owner_actor_index,
			&workspace,
			&position_flags);
		actor_change_firing_position(
			actor_index,
			firing_position_index,
			&candidate,
			previous_owner_actor_index,
			&workspace,
			position_flags);
	}

	return actor->danger_zone.danger_type == 0;
}

void
action_avoid_update(
	long actor_index)
{
}

void
action_avoid_control(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->target.target_type >= _actor_target_uninspected_orphan)
	{
		actor->orders.combat.shoot_at_target = TRUE;
		actor->orders.look.primary_priority = _action_avoid_primary_target_priority;
	}
	else
	{
		actor->orders.look.primary_priority = _action_avoid_primary_danger_priority;
		if (actor->danger_zone.danger_type > 0)
		{
			actor->orders.look.primary_direction.type = _direction_specification_danger;
			goto finish_control;
		}
	}
	actor->orders.look.primary_direction.type = _direction_specification_target;

finish_control:
	actor->orders.look.idle_look_type = _action_avoid_idle_look_type;
	actor->orders.move.stationary_crouch = actor->emotions.defensive_crouch;
	actor->orders.move.moving_crouch = FALSE;
	actor->orders.move.panicked = FALSE;
	actor->orders.move.dive_into_cover = FALSE;
	actor->orders.move.emerge_from_cover = FALSE;
}

/* ---------- private code */
