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
#include "props.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_actor_mode_combat = 3,
};

enum
{
	_actor_combat_status_clear_los = 5,
};

enum
{
	_actor_movement_switching_always_run = 0,
	_actor_movement_switching_always_crouch,
	_actor_movement_switching_switch_types,
};

enum
{
	_actor_variant_definition_movement_switching_try_to_stay_with_friends_bit = 3,
};

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
	{ 0, 0 },
	NULL,
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

boolean actor_type_flood_desire_shamble(
	long actor_index)
{
	boolean new_value;
	real crouch_chance;
	struct actor_datum *actor = actor_get(actor_index);
	struct unit_datum *unit;
	struct prop_datum *target_prop;
	struct actor_variant_definition const *variant =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	struct actor_variant_definition const *firing_variant =
		actor_combat_get_firing_variant_definition(actor_index);

	if ((!unit_is_busy(actor->meta.unit_index) && !actor_path_has_path(actor_index)) ||
		actor->state.mode < _actor_mode_combat)
		goto desire_shamble;

	if (actor->state.combat_status < _actor_combat_status_clear_los)
		goto do_not_shamble;

	unit = unit_get(actor->meta.unit_index);
	target_prop = actor->target.target_prop_index == NONE
		? NULL
		: prop_get(actor->target.target_prop_index);

	if (unit->unit.animation.state == _unit_state_hard_ping && !actor->emotions.berserk)
		goto desire_shamble;
	if (target_prop &&
		target_prop->distance > firing_variant->ranged_combat.maximum_firing_range)
		goto do_not_shamble;
	if (actor->emotions.berserk && target_prop &&
		target_prop->distance > firing_variant->ranged_combat.berserk_firing_range_upper_bound)
		goto do_not_shamble;
	if (TEST_FLAG(unit->object.damage_flags, _object_melee_attack_inhibited_bit))
		goto desire_shamble;

	if (actor->emotions.berserk)
		goto do_not_shamble;
	if ((actor->state.action == _actor_action_charge &&
			(actor->state.action_data.charge.goal == _charge_goal_melee ||
				actor->state.action_data.charge.goal == _charge_goal_melee_leaping)) ||
		!actor_has_ranged_weapon(actor_index) || actor->input.underwater)
		goto do_not_shamble;

	if (variant->movement_switching.movement_type != _actor_movement_switching_always_run)
	{
		if (variant->movement_switching.movement_type == _actor_movement_switching_always_crouch)
			goto desire_shamble;
		if (!target_prop ||
			!(target_prop->distance < firing_variant->ranged_combat.combat_range_upper_bound))
			goto update_shamble;

desire_shamble:
		actor->emotions.crouch_switching_active = FALSE;
		return TRUE;
	}

do_not_shamble:
	actor->emotions.crouch_switching_active = FALSE;
	return FALSE;

update_shamble:
	if (actor->emotions.crouch_switching_active)
	{
		if (actor->emotions.crouch_switching_stay_timer > 0)
		{
			actor->emotions.crouch_switching_stay_timer--;
		}
		else if (TEST_FLAG(
			variant->flags,
			_actor_variant_definition_movement_switching_try_to_stay_with_friends_bit) &&
			actor->situation.close_friends > 0)
		{
			short forward_count = 0;
			short middle_count = 0;
			short backward_count = 0;
			struct prop_iterator iterator;
			struct prop_datum *prop;

			match_assert(
				"c:\\halo\\SOURCE\\ai\\actor_type_flood.c",
				201,
				actor->target.target_prop_index != NONE);
			target_prop = prop_get(actor->target.target_prop_index);
			prop_iterator_new(&iterator, actor_index);
			while ((prop = prop_iterator_next(&iterator)) != NULL)
			{
				if (prop->state >= _prop_state_becoming_unacknowledged &&
					prop->state <= _prop_state_acknowledged &&
					!prop->enemy && !prop->dead && prop->distance <= 15.0f &&
					prop->actor_index != NONE)
				{
					struct actor_datum *friend_actor = actor_get(prop->actor_index);

					if (friend_actor->emotions.crouch_switching_active)
					{
						real_vector3d direction;
						real projection;

						vector_from_points3d(
							&actor->input.position.body_position,
							&friend_actor->input.position.body_position,
							&direction);
						projection = dot_product3d(&direction, &target_prop->actor_to_prop);
						if (projection > 1.4f)
							forward_count++;
						else if (projection < -1.4f)
							backward_count++;
						else
							middle_count++;
					}
				}
			}

			if (actor->emotions.crouch_switching_current_value)
			{
				if (backward_count == 0 && forward_count > middle_count)
				{
					new_value = !actor->emotions.crouch_switching_current_value;
					goto changed;
				}
			}
			else if (forward_count == 0 && backward_count > middle_count)
			{
				goto change_crouch;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\ai\\actor_type_flood.c",
			256,
			actor->emotions.crouch_switching_change_timer > 0);
		actor->emotions.crouch_switching_change_timer--;
		if (actor->emotions.crouch_switching_change_timer != 0)
			goto return_current;

change_crouch:
		new_value = !actor->emotions.crouch_switching_current_value;
	}
	else
	{
		crouch_chance = variant->movement_switching.initial_crouch_chance;

		if (actor->situation.area_friends > 0)
		{
			short standing_count = 0;
			short crouching_count = 0;
			struct prop_iterator iterator;
			struct prop_datum *prop;

			prop_iterator_new(&iterator, actor_index);
			while ((prop = prop_iterator_next(&iterator)) != NULL)
			{
				if (prop->state >= _prop_state_becoming_unacknowledged &&
					prop->state <= _prop_state_acknowledged &&
					!prop->enemy && !prop->dead && prop->actor_index != NONE)
				{
					struct actor_datum *friend_actor = actor_get(prop->actor_index);

					if (friend_actor->meta.type == actor->meta.type &&
						friend_actor->state.combat_status >= _actor_combat_status_clear_los)
					{
						if (friend_actor->emotions.defensive_crouch)
							crouching_count++;
						else
							standing_count++;
					}
				}
			}

			{
				real negative_crouch_chance = -crouch_chance;

				crouch_chance -=
					(negative_crouch_chance * standing_count +
						(1.0f - crouch_chance) * crouching_count) * 0.5f;
			}
		}

		{
			real random_value = real_seed_random(get_global_random_seed_address());

			actor->emotions.crouch_switching_active = TRUE;
			new_value = random_value < crouch_chance;
		}
	}

changed:
	{
		real lower_bound;
		real upper_bound;
		real change_time;

		actor->emotions.crouch_switching_current_value = new_value;
		if (new_value)
		{
			upper_bound = variant->movement_switching.crouch_time_upper_bound;
			lower_bound = variant->movement_switching.crouch_time_lower_bound;
		}
		else
		{
			upper_bound = variant->movement_switching.run_time_upper_bound;
			lower_bound = variant->movement_switching.run_time_lower_bound;
		}
		change_time = real_seed_random_range(
			get_global_random_seed_address(),
			lower_bound,
			upper_bound) * 30.0f;
		actor->emotions.crouch_switching_change_timer = (short)MAX(change_time, 31.0f);
		actor->emotions.crouch_switching_stay_timer = 30;

return_current:
		return actor->emotions.crouch_switching_current_value;
	}
}
