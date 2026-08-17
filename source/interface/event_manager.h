/*
EVENT_MANAGER.H

header included in hcex build.
*/

#ifndef __EVENT_MANAGER_H
#define __EVENT_MANAGER_H
#pragma once

#include "math/integer_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct event_record
{
	short type;
	short controller_index;
	union event_record_data
	{
		point2d stick;
		struct event_record_button
		{
			byte index;
			byte value;
		} button;
		long value;
	} data;
};

/* ---------- prototypes/EVENT_MANAGER.C */

void event_manager_initialize(
	void);

void event_manager_dispose(
	void);

void event_manager_suppress(
	boolean suppress);

boolean get_next_event(
	struct event_record *event,
	short local_player_index);

unsigned long event_manager_time_of_last_event(
	void);

void event_manager_flush(
	void);

void event_manager_update(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __EVENT_MANAGER_H
