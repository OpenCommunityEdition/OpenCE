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

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

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
