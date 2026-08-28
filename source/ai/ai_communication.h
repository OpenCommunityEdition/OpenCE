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

struct ai_information_packet;

/* The independently mapped prefix of the 0x64-byte conversation datum. */
struct ai_conversation_datum_header
{
	short identifier;
	short scenario_conversation_index;
	boolean scripted;
	boolean any_line_spoken;
	boolean begun;
	boolean finished;
	boolean waiting_to_advance;
	boolean told_to_advance;
	byte reserved0A[2];
	long creation_time;
	long triggering_player_unit_index;
};

/* ---------- prototypes/AI_COMMUNICATION.C */

void ai_communication_dispose(
	void);
void ai_communication_dispose_from_old_map(
	void);
void ai_communication_packet_new(
	struct ai_information_packet *information);
short ai_conversation_line(
	short scenario_conversation_index);
void ai_conversation_advance(
	short scenario_conversation_index);
void ai_conversation_finish(
	long conversation_index,
	boolean abort,
	boolean force);
void ai_conversation_stop(
	short scenario_conversation_index);
real ai_communication_get_player_rating(
	long unit_index,
	boolean test_line_of_sight,
	long *unit_index_reference,
	real *distance_reference);

/* ---------- globals */

extern struct data_array *conversation_data;

/* ---------- public code */

#endif // __AI_COMMUNICATION_H
