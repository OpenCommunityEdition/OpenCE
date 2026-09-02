/*
ACTOR_STIMULUS.C

symbols in this file:
00024E70 0030:
	_actor_stimulus_clear (0000)
00024EA0 00d0:
	_code_00024ea0 (0000)
00024F70 0060:
	_actor_stimulus_surprise (0000)
00024FD0 0060:
	_actor_stimulus_suspicion (0000)
00025030 0070:
	_actor_stimulus_prop_sighted (0000)
000250A0 0080:
	_actor_stimulus_enter_combat_found_body (0000)
00025120 0040:
	_actor_stimulus_enter_combat_perceived_enemy (0000)
00025160 0080:
	_actor_stimulus_enter_combat_friend_in_combat (0000)
000251E0 0030:
	_actor_stimulus_bumped (0000)
00025210 0050:
	_actor_stimulus_environmental_noise (0000)
00025260 0150:
	_actor_stimulus_heard_shooting (0000)
000253B0 0020:
	_actor_stimulus_was_surprised (0000)
000253D0 0070:
	_actor_stimulus_maneuvering (0000)
00025440 0020:
	_actor_stimulus_vehicle_eviction (0000)
00025460 00d0:
	_actor_stimulus_abandon_stationary_facing (0000)
00025530 0320:
	_actor_stimulus_prop_acknowledged (0000)
00025850 0170:
	_actor_stimulus_damage (0000)
000259C0 0140:
	_actor_stimulus_weapon_impact (0000)
00025B00 0140:
	_actor_stimulus_weapon_detonation (0000)
00025C40 01a0:
	_actor_stimulus_prop_just_killed (0000)
00025DE0 0110:
	_actor_stimulus_prop_fleeing (0000)
00025EF0 0110:
	_actor_stimulus_noticed_danger_zone (0000)
002462D4 0038:
	??_C@_0DI@JCJNLAKP@?$CFs?5?$CFd?3?5surprise?5?$CFs?3?5?$CFs?5?$CFsexp?5?$CFs?5@ (0000)
0024630C 0007:
	??_C@_06NMPBKCIA@noncom?$AA@ (0000)
00246314 0006:
	??_C@_05HJNNLMDJ@front?$AA@ (0000)
0024631C 0006:
	??_C@_05ILIIOMHH@flank?$AA@ (0000)
00246324 0004:
	??_C@_03NJKJADM@not?$AA@ (0000)
00246328 0004:
	??_C@_03JEOKDCFP@far?$AA@ (0000)
0024632C 0006:
	??_C@_05LBOHBHFK@close?$AA@ (0000)
00246334 0013:
	??_C@_0BD@PCBBJBIG@unexp?9behind?9shoot?$AA@ (0000)
00246348 0012:
	??_C@_0BC@IGIPMMMB@unexp?9close?9shoot?$AA@ (0000)
0024635C 0008:
	??_C@_07JBPBHDIL@grenade?$AA@ (0000)
00246364 000c:
	??_C@_0M@EODEKNJA@enemy?9close?$AA@ (0000)
00246370 000c:
	??_C@_0M@POIHHAMP@enemy?9shoot?$AA@ (0000)
0024637C 0023:
	??_C@_0CD@LMNLJKDA@c?3?2halo?2SOURCE?2ai?2actor_stimulus@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "ai/actors.h"
#include "ai/actor_definitions.h"
#include "ai/props.h"

#include "game/game.h"

#include "memory/data.h"

#include <stddef.h>

/* ---------- constants */

enum
{
	_actor_mode_asleep = 1,
	_actor_mode_combat = 3,
};

enum
{
	_actor_stimulus_combat_friend = 1,
	_actor_stimulus_combat_body = 2,
};

enum
{
	_actor_panic_platoon_retreating = 6,
};

enum
{
	_ai_communication_sighted_enemy = 4,
	_ai_communication_found_enemy = 5,
	_ai_communication_advance = 22,
	_ai_communication_retreat = 23,
};

enum
{
	_communication_hostility_enemy = 3,
};

/* ---------- macros */

/* ---------- structures */

typedef char actor_stimulus_data_size_assert[
	sizeof(struct actor_stimulus_data) == 0x64 ? 1 : -1];
typedef char actor_stimuli_offset_assert[
	offsetof(struct actor_datum, stimuli) == 0x2EC ? 1 : -1];
typedef char actor_stimulus_vehicle_eviction_offset_assert[
	offsetof(struct actor_datum, stimuli.vehicle_eviction) == 0x2ED ? 1 : -1];
typedef char actor_stimulus_was_surprised_offset_assert[
	offsetof(struct actor_datum, stimuli.was_surprised) == 0x2F0 ? 1 : -1];
typedef char actor_stimulus_suspicion_combat_status_offset_assert[
	offsetof(struct actor_datum, stimuli.suspicion_combat_status) == 0x34A ? 1 : -1];
typedef char actor_stimulus_suspicion_timer_offset_assert[
	offsetof(struct actor_datum, stimuli.suspicion_timer) == 0x34C ? 1 : -1];
typedef char actor_stimulus_actor_mode_offset_assert[
	offsetof(struct actor_datum, state.mode) == 0x6A ? 1 : -1];
typedef char actor_stimulus_actor_unit_index_offset_assert[
	offsetof(struct actor_datum, meta.unit_index) == 0x18 ? 1 : -1];
typedef char actor_stimulus_panic_type_offset_assert[
	offsetof(struct actor_datum, stimuli.panic_type) == 0x308 ? 1 : -1];
typedef char actor_stimulus_panic_prop_index_offset_assert[
	offsetof(struct actor_datum, stimuli.panic_prop_index) == 0x30C ? 1 : -1];
typedef char actor_stimulus_direction_size_assert[
	sizeof(struct direction_specification) == 0x10 ? 1 : -1];
typedef char actor_stimulus_direction_prop_index_offset_assert[
	offsetof(struct direction_specification, prop_index) == 0x4 ? 1 : -1];
typedef char actor_stimulus_direction_point_offset_assert[
	offsetof(struct direction_specification, point) == 0x4 ? 1 : -1];
typedef char actor_stimulus_prop_unit_index_offset_assert[
	offsetof(struct prop_datum, unit_index) == 0x18 ? 1 : -1];
typedef char actor_stimulus_prop_enemy_offset_assert[
	offsetof(struct prop_datum, enemy) == 0x60 ? 1 : -1];
typedef char actor_stimulus_prop_dead_offset_assert[
	offsetof(struct prop_datum, dead) == 0x127 ? 1 : -1];

/* ---------- prototypes */

void ai_communication_event(
	short type,
	long unit_index,
	long prop_index,
	long object_index,
	long position_index,
	long structure_index,
	void const *context);

/* ---------- globals */

/* ---------- public code */

void actor_stimulus_clear(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	csmemset(&actor->stimuli, 0, sizeof(actor->stimuli));
	return;
}

static void actor_stimulus_combat(
	long actor_index,
	short transition_type,
	real_point3d const *guard_point,
	long guard_point_surface_index,
	real guard_distance,
	long guard_timer,
	real_vector3d const *transition_vector,
	long prop_index,
	long prop_look_timer,
	boolean prop_look_while_moving)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->state.mode >= _actor_mode_combat)
	{
		return;
	}
	if (transition_type < actor->stimuli.combat_transition)
	{
		return;
	}

	actor->stimuli.combat_transition = transition_type;
	if (!guard_point)
	{
		actor->stimuli.combat_transition_guard_at_point = FALSE;
	}
	else
	{
		actor->stimuli.combat_transition_guard_at_point = TRUE;
		actor->stimuli.combat_transition_guard_point = *guard_point;
		actor->stimuli.combat_transition_guard_point_surface_index =
			guard_point_surface_index;
		actor->stimuli.combat_transition_guard_point_distance = guard_distance;
	}

	if (!transition_vector)
	{
		actor->stimuli.combat_transition_has_vector = FALSE;
	}
	else
	{
		actor->stimuli.combat_transition_has_vector = TRUE;
		actor->stimuli.combat_transition_vector = *transition_vector;
	}

	actor->stimuli.combat_transition_prop_index = prop_index;
	actor->stimuli.combat_transition_prop_look_timer = prop_look_timer;
	actor->stimuli.combat_transition_prop_look_while_moving =
		prop_look_while_moving;
	actor->stimuli.combat_transition_guard_timer = guard_timer;

	return;
}

void actor_stimulus_suspicion(
	long actor_index,
	short suspicion_combat_status,
	long suspicion_timer)
{
	struct actor_datum *actor = actor_get(actor_index);
	short current_status = actor->stimuli.suspicion_combat_status;

	if (current_status < suspicion_combat_status)
	{
		actor->stimuli.suspicion_combat_status = suspicion_combat_status;
		actor->stimuli.suspicion_timer = suspicion_timer;
	}
	else if (current_status == suspicion_combat_status)
	{
		actor->stimuli.suspicion_timer = MAX(
			actor->stimuli.suspicion_timer,
			suspicion_timer);
	}

	return;
}

void actor_stimulus_enter_combat_found_body(
	long actor_index,
	long prop_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);

	actor->emotions.corpse_ignore_time = game_time_get();
	actor_perception_find_prop_pathfinding_location(actor_index, prop_index);
	actor_stimulus_combat(
		actor_index,
		_actor_stimulus_combat_body,
		&prop->pathfinding_point,
		prop->pathfinding_surface_index,
		1.5f,
		90,
		NULL,
		prop_index,
		90,
		TRUE);

	return;
}

void actor_stimulus_enter_combat_friend_in_combat(
	long actor_index,
	long prop_index)
{
	struct prop_datum *prop = prop_get(prop_index);

	actor_stimulus_combat(
		actor_index,
		_actor_stimulus_combat_friend,
		NULL,
		NONE,
		0.0f,
		90,
		&prop->actor_to_prop,
		prop_index,
		150,
		FALSE);

	if (prop->actor_index != NONE)
	{
		struct actor_datum *friend_actor = actor_get(prop->actor_index);

		if (friend_actor->state.suspicion_combat_status > 0)
		{
			actor_stimulus_suspicion(
				actor_index,
				friend_actor->state.suspicion_combat_status,
				450);
		}
	}

	return;
}

void actor_stimulus_was_surprised(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->stimuli.was_surprised = TRUE;
	return;
}

void actor_stimulus_vehicle_eviction(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->stimuli.vehicle_eviction = TRUE;
	return;
}

void actor_stimulus_bumped(
	long actor_index,
	long prop_index)
{
	struct direction_specification direction;

	direction.type = _direction_specification_prop;
	direction.prop_index = prop_index;
	actor_look_secondary(
		actor_index,
		_secondary_look_bumped_prop,
		_secondary_look_priority_default,
		&direction);

	return;
}

void actor_stimulus_environmental_noise(
	long actor_index,
	long object_index,
	real_point3d const *position,
	short count)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct direction_specification direction;

	if (actor->state.mode == _actor_mode_asleep)
	{
		return;
	}

	direction.type = _direction_specification_point;
	direction.point = *position;
	actor_look_secondary(
		actor_index,
		_secondary_look_environmental_noise,
		_secondary_look_priority_default,
		&direction);

	return;
}

void actor_stimulus_prop_sighted(
	long actor_index,
	long prop_index,
	boolean initial_acknowledgement)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);

	if (!prop->dead &&
		actor->meta.unit_index != NONE &&
		prop->enemy)
	{
		ai_communication_event(
			initial_acknowledgement ?
				_ai_communication_sighted_enemy :
				_ai_communication_found_enemy,
			actor->meta.unit_index,
			prop->unit_index,
			_communication_hostility_enemy,
			NONE,
			NONE,
			NULL);
	}

	return;
}

void actor_stimulus_maneuvering(
	long actor_index,
	boolean advancing,
	boolean flee)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->meta.unit_index != NONE)
	{
		ai_communication_event(
			advancing ?
				_ai_communication_advance :
				_ai_communication_retreat,
			actor->meta.unit_index,
			NONE,
			NONE,
			NONE,
			NONE,
			NULL);
	}

	if (flee &&
		actor->stimuli.panic_type < _actor_panic_platoon_retreating)
	{
		actor->stimuli.panic_type = _actor_panic_platoon_retreating;
		actor->stimuli.panic_prop_index = NONE;
	}

	return;
}

void actor_stimulus_abandon_stationary_facing(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition =
		actor_definition_get(actor->meta.definition_index);

	if (actor->emotions.defensive_crouch)
	{
		if (definition->defensive.change_facing_stand_time > 0.0f)
		{
			actor->emotions.defensive_crouch = FALSE;
			actor->emotions.defensive_crouch_timer =
				(short)(definition->defensive.change_facing_stand_time *
					TICKS_PER_SECOND);
			{
				long prop_index = actor->target.target_prop_index;

				if (prop_index != NONE && prop_get(prop_index)->distance < 4.0f)
				{
					actor->emotions.perceived_danger = MAX(
						actor->emotions.perceived_danger,
						1.8f);
				}
			}
		}
	}

	return;
}

/* ---------- private code */
