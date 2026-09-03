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
struct scenario;
struct squad_datum;

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

struct ai_script_squad_iterator
{
	long encounter_index;
	long required_platoon_index;
	long squad_index;
	long next_squad_index;
	long last_squad_index;
};

struct ai_script_actor_reference_iterator
{
	long encounter_index;
	long squad_index;
	long platoon_index;
	long actor_encounter_index;
	long actor_index;
	long next_actor_index;
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
void ai_scripting_erase_all(
	void);
void ai_index_to_string(
	long ai_reference,
	struct scenario *scenario,
	char *buffer,
	long buffer_size);
boolean ai_index_from_string(
	struct scenario *scenario,
	char const *ai_string,
	long *ai_reference);
void ai_index_squad_iterator_new(
	long ai_reference,
	struct ai_script_squad_iterator *iterator);
struct squad_datum *ai_index_squad_iterator_next(
	struct ai_script_squad_iterator *iterator);
void ai_index_platoon_iterator_new(
	long ai_reference,
	struct ai_script_platoon_iterator *iterator);
struct platoon_datum *ai_index_platoon_iterator_next(
	struct ai_script_platoon_iterator *iterator);
long object_list_from_ai_reference(
	long ai_reference);
struct ai_script_vehicle_enterable *ai_scripting_find_vehicle_enterable(
	long vehicle_index);
void ai_scripting_attach_unit(
	long unit_index,
	long ai_reference);
void ai_scripting_magically_see_unit(
	long ai_reference,
	long unit_index);
void ai_scripting_magically_see_players(
	long ai_reference);
void ai_index_actor_iterator_new(
	long ai_reference,
	struct ai_script_actor_reference_iterator *iterator);
struct actor_datum *ai_index_actor_iterator_next(
	struct ai_script_actor_reference_iterator *iterator);
void ai_scripting_go_to_vehicle(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name);
void ai_scripting_go_to_vehicle_override(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name);
void ai_scripting_attack(
	long ai_reference);
void ai_scripting_defend(
	long ai_reference);
void ai_scripting_maneuver(
	long ai_reference);
void ai_scripting_maneuver_enable(
	long ai_reference,
	boolean enable);
void ai_scripting_berserk(
	long ai_reference,
	boolean berserk);
void ai_scripting_playfight(
	long ai_reference,
	boolean playfight);
void ai_scripting_braindead(
	long ai_reference,
	boolean braindead);
void ai_scripting_allow_charge(
	long ai_reference,
	boolean allow);
void ai_scripting_allow_dormant(
	long ai_reference,
	boolean allow);
void ai_scripting_timer_start(
	long ai_reference);
void ai_scripting_timer_expire(
	long ai_reference);
void ai_scripting_follow_target_disable(
	long ai_reference);
void ai_scripting_automatic_migration_target(
	long ai_reference,
	boolean automatic);
void ai_scripting_follow_target_players(
	long ai_reference);
void ai_scripting_follow_distance(
	long ai_reference,
	real distance);
void ai_scripting_follow_target_unit(
	long ai_reference,
	long unit_index);
void ai_scripting_force_active(
	long ai_reference,
	boolean force);

/* ---------- globals */

/* ---------- public code */

#endif // __AI_SCRIPT_H
