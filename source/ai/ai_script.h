/*
AI_SCRIPT.H

header included in hcex build.
*/

#ifndef __AI_SCRIPT_H
#define __AI_SCRIPT_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct platoon_datum;

struct ai_script_vehicle_enterable
{
	long vehicle_index;
	real radius;
	short team_bitmask;
	short actor_type_bitmask;
};

struct ai_script_platoon_iterator
{
	long encounter_index;
	long platoon_index;
	long last_platoon_index;
};

/* ---------- prototypes/AI_SCRIPT.C */

void ai_script_initialize(
	void);
void ai_script_dispose(
	void);
void ai_script_initialize_for_new_map(
	void);
void ai_script_dispose_from_old_map(
	void);
void ai_scripting_reconnect(
	void);
void ai_scripting_erase_all(void);
void ai_index_platoon_iterator_new(
	long ai_reference,
	struct ai_script_platoon_iterator *iterator);
struct platoon_datum *ai_index_platoon_iterator_next(
	struct ai_script_platoon_iterator *iterator);
struct ai_script_vehicle_enterable *ai_scripting_find_vehicle_enterable(
	long vehicle_index);

/* ---------- globals */

/* ---------- public code */

#endif // __AI_SCRIPT_H
