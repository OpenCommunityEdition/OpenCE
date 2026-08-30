/*
AI_PROFILE.H

header included in hcex build.
*/

#ifndef __AI_PROFILE_H
#define __AI_PROFILE_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/AI_PROFILE.C */

void ai_profile_initialize(
	void);
void ai_profile_initialize_for_new_map(
	void);

void ai_profile_dispose(
	void);
void ai_profile_dispose_from_old_map(
	void);
short ai_profile_change_render_spray(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __AI_PROFILE_H
