/*
AI_RUNTIME.H
*/

#ifndef __AI_RUNTIME_H
#define __AI_RUNTIME_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- prototypes/AI.C */

void ai_handle_allegiance_status_changed(
	short team1_index,
	short team2_index,
	boolean currently_broken,
	boolean permanently_broken);

#endif // __AI_RUNTIME_H
