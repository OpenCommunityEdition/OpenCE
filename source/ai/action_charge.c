/*
ACTION_CHARGE.C

symbols in this file:
00001080 0050:
	_action_charge_begin (0000)
000010D0 0110:
	_code_000010d0 (0000)
000011E0 0050:
	_action_charge_update (0000)
00001230 0220:
	_action_charge_control (0000)
00001450 0050:
	_action_charge_is_leaping (0000)
000014A0 0010:
	_square_root (0000)
000014B0 0020:
	_scale_vector2d (0000)
000014D0 0020:
	_magnitude_squared2d (0000)
000014F0 0020:
	_magnitude2d (0000)
00001510 0050:
	_normalize2d (0000)
00001560 0020:
	_dot_product2d (0000)
00001580 0030:
	_point_from_line3d (0000)
000015B0 0030:
	_scale_vector3d (0000)
000015E0 0030:
	_magnitude3d (0000)
00001610 0060:
	_normalize3d (0000)
00001670 0020:
	_dot_product3d (0000)
00001690 0030:
	_subtract_vectors3d (0000)
000016C0 0010:
	_real_random (0000)
000016D0 0050:
	_collision_test_line (0000)
00001720 0cb0:
	_action_charge_perform (0000)
000023D0 0120:
	_code_000023d0 (0000)
000024F0 03b0:
	_action_charge_setup (0000)
00242F60 0004:
	__real@00000000 (0000)
00242F64 0004:
	__real@3f333333 (0000)
00242F68 0004:
	__real@3f800000 (0000)
00242F70 0008:
	__real@3f1a36e2e0000000 (0000)
00242F78 0004:
	__real@40800000 (0000)
00242F7C 0004:
	__real@3f5db3d7 (0000)
00242F80 0004:
	__real@3cbf258c (0000)
00242F84 0004:
	__real@3e99999a (0000)
00242F88 0004:
	__real@3d4ccccd (0000)
00242F8C 0004:
	__real@3fc00000 (0000)
00242F90 0004:
	__real@3f4ccccd (0000)
00242F94 0022:
	??_C@_0CC@DPKBMBNB@c?3?2halo?2SOURCE?2ai?2action_charge?4@ (0000)
00242FB8 0028:
	??_C@_0CI@OJMAHDFE@global_current_collision_user_de@ (0000)
00242FE0 0049:
	??_C@_0EJ@EEKGOMEC@global_current_collision_user_de@ (0000)
0024302C 0030:
	??_C@_0DA@BMPMGKCG@actor?5?$CFs?5melee?5animation?5has?5no?5@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

#define square_root square_root_inline
#define scale_vector2d scale_vector2d_inline
#define magnitude_squared2d magnitude_squared2d_inline
#define magnitude2d magnitude2d_inline
#define normalize2d normalize2d_inline
#define dot_product2d dot_product2d_inline
#define point_from_line3d point_from_line3d_inline
#define scale_vector3d scale_vector3d_inline
#define magnitude3d magnitude3d_inline
#define normalize3d normalize3d_inline
#define dot_product3d dot_product3d_inline
#define subtract_vectors3d subtract_vectors3d_inline
#define real_random real_random_inline
#include "actions.h"

#include "actor_definitions.h"
#include "actors.h"
#include "game/game.h"
#include "math/real_math.h"
#undef square_root
#undef scale_vector2d
#undef magnitude_squared2d
#undef magnitude2d
#undef normalize2d
#undef dot_product2d
#undef point_from_line3d
#undef scale_vector3d
#undef magnitude3d
#undef normalize3d
#undef dot_product3d
#undef subtract_vectors3d
#undef real_random

/* ---------- constants */

enum
{
	_action_charge_idle_look_type = 4,
	_action_charge_primary_priority_exact_facing = 4,
	_action_charge_primary_priority_aiming = 5,
	_action_charge_primary_priority_locked_aiming = 7,
	_actor_special_fire_situation_strafing = 3,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

struct actor_variant_definition *actor_combat_get_firing_variant_definition(
	long actor_index);

/* ---------- globals */

/* ---------- public code */

void action_charge_begin(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->state.action_data.charge.goal == _charge_goal_vehicle_strafing &&
		actor_combat_get_firing_variant_definition(actor_index)->ranged_combat.special_fire_situation ==
			_actor_special_fire_situation_strafing)
	{
		short charge_remaining = actor->control.special_fire_deny_attempts;

		if (charge_remaining > 0)
		{
			actor->control.special_fire_deny_attempts = charge_remaining - 1;
		}
	}

	return;
}

void action_charge_update(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->state.action_data.charge.goal == _charge_goal_melee_leaping &&
		actor->state.action_data.charge.launched_leap &&
		!actor->state.action_data.charge.launched_melee_attack &&
		!actor->input.in_midair)
	{
		++actor->state.action_data.charge.leap_failure_timer;
	}

	return;
}

void action_charge_control(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct charge_state_data *state_data = &actor->state.action_data.charge;
	struct actor_definition *definition = actor_definition_get(actor->meta.definition_index);

	actor->orders.look.primary_direction.type = _direction_specification_target;
	actor->orders.look.idle_look_type = _action_charge_idle_look_type;

	if ((state_data->goal == _charge_goal_melee ||
		state_data->goal == _charge_goal_melee_leaping) &&
		state_data->alignment_incorrect &&
		!actor->control.moving &&
		!actor_path_has_path(actor_index))
	{
		actor->orders.look.primary_priority = _action_charge_primary_priority_exact_facing;
	}
	else
	{
		short priority = _action_charge_primary_priority_aiming;

		if (actor->state.combat_status >= 5 &&
			state_data->goal != _charge_goal_stalking)
		{
			actor->orders.look.primary_priority = _action_charge_primary_priority_locked_aiming;
		}
		else
		{
			actor->orders.look.primary_priority = priority;
		}
	}

	if (state_data->goal == _charge_goal_stalking)
	{
		actor->orders.move.stationary_crouch = !state_data->stalking_catch_target;
		actor->orders.move.moving_crouch = !state_data->stalking_catch_target;
	}
	else if (!actor->orders.move.panicked &&
		TEST_FLAG(definition->flags, _actor_definition_defensive_crouch_while_charging_bit))
	{
		actor->orders.move.stationary_crouch = actor->emotions.defensive_crouch;
		actor->orders.move.moving_crouch = actor->emotions.defensive_crouch;
	}
	else
	{
		actor->orders.move.stationary_crouch = FALSE;
		actor->orders.move.moving_crouch = FALSE;
	}

	if (state_data->leap_pending)
	{
		actor->orders.move.jump = TRUE;
		actor->orders.move.jump_leap = state_data->leap_vertical_velocity < state_data->leap_horizontal_velocity * 0.7f;
		actor->orders.move.jump_targeted = TRUE;
		actor->orders.move.jump_alignment_vector = state_data->leap_alignment_vector;
		actor->orders.move.jump_target_horizontal_vel = state_data->leap_horizontal_velocity;
		actor->orders.move.jump_target_vertical_vel = state_data->leap_vertical_velocity;
		state_data->launched_leap = TRUE;
		state_data->leap_pending = FALSE;
		state_data->leap_start_time = game_time_get();
		state_data->leap_failure_timer = 0;
	}

	if (TEST_FLAG(definition->flags, _actor_definition_berserk_use_panic_movement_bit) &&
		(actor->emotions.berserk ||
		state_data->goal == _charge_goal_melee ||
		state_data->goal == _charge_goal_melee_leaping))
	{
		actor->orders.move.panicked = state_data->advancing && !actor->orders.move.moving_crouch;
	}

	actor->orders.move.move_face_exactly = TRUE;
	actor->orders.move.dive_into_cover = FALSE;
	actor->orders.move.emerge_from_cover = FALSE;
	actor->orders.combat.shoot_at_target = state_data->goal != _charge_goal_stalking;

	return;
}

boolean action_charge_is_leaping(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct charge_state_data *state_data = &actor->state.action_data.charge;
	boolean result = FALSE;

	if (state_data->goal == _charge_goal_melee_leaping &&
		state_data->launched_leap)
	{
		result = state_data->leap_start_time + 30 >= game_time_get();
	}

	return result;
}

real square_root(
	real x)
{
	return sqrt(x);
}

real_vector2d *scale_vector2d(
	real_vector2d const *a,
	real c,
	real_vector2d *result)
{
	result->i = c*a->i;
	result->j = c*a->j;

	return result;
}

real magnitude_squared2d(
	real_vector2d const *v)
{
	return v->i*v->i + v->j*v->j;
}

real magnitude2d(
	real_vector2d const *v)
{
	return square_root(magnitude_squared2d(v));
}

real normalize2d(
	real_vector2d *v)
{
	real magnitude = magnitude2d(v);

	if (!(_real_epsilon>fabs(magnitude-0.f)))
	{
		scale_vector2d(v, 1.f / magnitude, v);
	}
	else
	{
		magnitude = 0.f;
	}

	return magnitude;
}

real dot_product2d(
	real_vector2d const *a,
	real_vector2d const *b)
{
	return a->i*b->i + a->j*b->j;
}

real_point3d *point_from_line3d(
	real_point3d const *p,
	real_vector3d const *v,
	real t,
	real_point3d *result)
{
	result->x = v->i*t + p->x;
	result->y = v->j*t + p->y;
	result->z = v->k*t + p->z;

	return result;
}

real_vector3d *scale_vector3d(
	real_vector3d const *a,
	real c,
	real_vector3d *result)
{
	result->i = c*a->i;
	result->j = c*a->j;
	result->k = c*a->k;

	return result;
}

real magnitude3d(
	real_vector3d const *v)
{
	return square_root(v->i*v->i + v->j*v->j + v->k*v->k);
}

real normalize3d(
	real_vector3d *v)
{
	real magnitude = magnitude3d(v);

	if (!(_real_epsilon>fabs(magnitude-0.f)))
	{
		scale_vector3d(v, 1.f / magnitude, v);
	}
	else
	{
		magnitude = 0.f;
	}

	return magnitude;
}

real dot_product3d(
	real_vector3d const *a,
	real_vector3d const *b)
{
	return a->i*b->i + a->j*b->j + a->k*b->k;
}

real_vector3d *subtract_vectors3d(
	real_vector3d const *a,
	real_vector3d const *b,
	real_vector3d *result)
{
	result->i = a->i-b->i;
	result->j = a->j-b->j;
	result->k = a->k-b->k;

	return result;
}

real real_random(
	void)
{
	return real_seed_random(
		get_global_random_seed_address());
}

/* ---------- private code */
