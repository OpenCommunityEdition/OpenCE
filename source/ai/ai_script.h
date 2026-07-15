/*
AI_SCRIPT.H

header included in hcex build.
*/

#ifndef __AI_SCRIPT_H
#define __AI_SCRIPT_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

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

/* ---------- globals */

/* ---------- public code */

#endif // __AI_SCRIPT_H
