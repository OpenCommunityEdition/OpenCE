/*
ACTOR_PERCEPTION.H

header included in hcex build.
*/

#ifndef __ACTOR_PERCEPTION_H
#define __ACTOR_PERCEPTION_H
#pragma once

/* ---------- prototypes/ACTOR_PERCEPTION.C */

void actor_situation_update_target_status(
	long actor_index);
void actor_situation_combat_status_update(
	long actor_index);
boolean actor_situation_try_new_target(
	long actor_index,
	long prop_index);

#endif // __ACTOR_PERCEPTION_H
