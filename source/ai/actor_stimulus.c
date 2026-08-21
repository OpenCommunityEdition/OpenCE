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

#include "memory/data.h"

#include <stddef.h>

/* ---------- constants */

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

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void actor_stimulus_clear(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	csmemset(&actor->stimuli, 0, sizeof(actor->stimuli));
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

/* ---------- private code */
