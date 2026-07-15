/*
ACTOR_TYPES.H

header included in hcex build.
*/

#ifndef __ACTOR_TYPES_H
#define __ACTOR_TYPES_H
#pragma once

/* ---------- constants */

enum
{
	_actor_elite = 0,
	_actor_jackal,
	_actor_grunt,
	_actor_hunter,
	_actor_engineer,
	_actor_assassin,
	_actor_player,
	_actor_marine,
	_actor_crew,
	_actor_combat_form,
	_actor_infection_form,
	_actor_carrier_form,
	_actor_monitor,
	_actor_sentinel,
	_actor_none,
	_actor_mounted_weapon,
	NUMBER_OF_ACTOR_TYPES,
};

/* ---------- macros */

/* ---------- structures */

typedef void (*actor_decide_action_proc)(long actor_index);
typedef void (*actor_swarm_control_proc)(long actor_index);
typedef void (*actor_swarm_aim_jump_proc)(long actor_index);

struct actor_type_definition
{
	const char *name;
	short race;
	short when_to_search_at_target;
	short when_to_pursue;
	short when_to_search_pursuit;
	long pursuit_controller;
	boolean swarm;
	byte pad[3];
	actor_decide_action_proc decide_action;
	actor_swarm_control_proc swarm_control;
	actor_swarm_aim_jump_proc swarm_aim_jump;
};

/* ---------- prototypes/ACTOR_TYPES.C */

void actor_types_initialize(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __ACTOR_TYPES_H
