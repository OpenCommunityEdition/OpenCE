/*
AI_COMMUNICATION.H

header included in hcex build.
*/

#ifndef __AI_COMMUNICATION_H
#define __AI_COMMUNICATION_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

#define ai_conversation_header_get(index) \
	((struct ai_conversation_datum_header *)datum_get(conversation_data, (index)))

/* ---------- structures */

/* The independently mapped prefix of the 0x64-byte conversation datum. */
struct ai_conversation_datum_header
{
	short identifier;
	short scenario_conversation_index;
	byte priority;
	boolean status_flags[5];
	long start_time;
	long unit_index;
};

/* ---------- prototypes/AI_COMMUNICATION.C */

void ai_communication_dispose(
	void);
short ai_conversation_line(
	short scenario_conversation_index);
void ai_conversation_finish(
	long conversation_index,
	boolean abort,
	boolean force);
real ai_communication_get_player_rating(real unit_index, boolean test_line_of_sight, long *unit_index_reference, real *distance_reference);

/* ---------- globals */

extern struct data_array *conversation_data;

/* ---------- public code */

#endif // __AI_COMMUNICATION_H
