/*
INPUT_ABSTRACTION.H

header included in hcex build.
*/

#ifndef __INPUT_ABSTRACTION_H
#define __INPUT_ABSTRACTION_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct game_input_preferences;

/* ---------- prototypes/INPUT_ABSTRACTION.C */

void input_abstraction_reset_controller_detection_timer(
	void);
void input_abstraction_get_local_player_preferences(
	short local_player_index,
	struct game_input_preferences *preferences);

/* ---------- globals */

/* ---------- public code */

#endif // __INPUT_ABSTRACTION_H
