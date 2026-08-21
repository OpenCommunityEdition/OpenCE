/*
ACTION_FLEE.C

symbols in this file:
00003140 0030:
	_action_flee_end (0000)
00003170 0030:
	_action_flee_flush_position_indices (0000)
000031A0 0070:
	_action_flee_modify_color (0000)
00003210 0280:
	_code_00003210 (0000)
00003490 0160:
	_code_00003490 (0000)
000035F0 0030:
	_action_flee_replace_prop (0000)
00003620 0020:
	_action_flee_blind_panic (0000)
00003640 0110:
	_action_flee_setup (0000)
00003750 0060:
	_action_flee_begin (0000)
000037B0 00a0:
	_action_flee_update (0000)
00003850 0190:
	_action_flee_control (0000)
000039E0 0140:
	_code_000039e0 (0000)
00003B20 0360:
	_action_flee_perform (0000)
002430A4 0020:
	??_C@_0CA@MCHAFBCO@c?3?2halo?2SOURCE?2ai?2action_flee?4c?$AA@ (0000)
002430C4 0004:
	__real@3ecccccd (0000)
002430C8 00d8:
	??_C@_0NI@KCCMPDJP@?$CI?$CBactor?9?$DOmeta?4timeslice?5?$CG?$CG?5state@ (0000)
002431A0 0037:
	??_C@_0DH@GFKJLMEI@actor?9?$DOfiring_positions?4current_@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actors.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_actor_panic_grenade_attached_to_us = 9,
	_actor_panic_burning_to_death = 12,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void action_flee_end(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->meta.unit_index != NONE)
	{
		unit_stop_running_blindly(actor->meta.unit_index);
	}

	return;
}

boolean action_flee_blind_panic(
	short panic_type)
{
	return panic_type >= _actor_panic_grenade_attached_to_us &&
		panic_type <= _actor_panic_burning_to_death;
}

/* ---------- private code */
