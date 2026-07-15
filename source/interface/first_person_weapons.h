/*
FIRST_PERSON_WEAPONS.H

header included in hcex build.
*/

#ifndef __FIRST_PERSON_WEAPONS_H
#define __FIRST_PERSON_WEAPONS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct real_matrix4x3;

/* ---------- prototypes/FIRST_PERSON_WEAPONS.C */

void first_person_weapons_dispose(
	void);
void first_person_weapons_dispose_from_old_map(
	void);
struct real_matrix4x3 *first_person_weapon_get_node_matrix(
	short local_player_index,
	short node_index);
void first_person_weapon_message_from_weapon(long weapon_index, short message_type);

/* ---------- globals */

/* ---------- public code */

#endif // __FIRST_PERSON_WEAPONS_H
