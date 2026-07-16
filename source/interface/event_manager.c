/*
EVENT_MANAGER.C

symbols in this file:
000CB700 0030:
	_event_manager_initialize (0000)
000CB730 0020:
	_event_manager_dispose (0000)
000CB750 0020:
	_event_manager_flush (0000)
000CB770 0010:
	_event_manager_suppress (0000)
000CB780 00c0:
	_get_next_event (0000)
000CB840 0010:
	_event_manager_time_of_last_event (0000)
000CB850 0300:
	_code_000cb850 (0000)
000CBB50 0110:
	_event_manager_update (0000)
002706D8 0066:
	??_C@_0GG@GKBBBOFI@event?5?$CG?$CG?5?$CI?$CIlocal_player_index?$DO?$DN0@ (0000)
00270740 0029:
	??_C@_0CJ@HBEODFJG@c?3?2halo?2SOURCE?2interface?2event_m@ (0000)
00453B60 0168:
	_event_manager_globals (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "event_manager.h"
#include "input.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct event_manager_state
{
	boolean initialized;
	boolean suppressed;
	short __unknown2;
	unsigned long time_of_last_event;
	struct event_record events[MAXIMUM_GAMEPADS][8];
};

struct event_manager_globals
{
	struct event_manager_state state;
	byte __unknown108[0x60];
};

typedef char verify_event_manager_state_size[
	sizeof(struct event_manager_state) == 0x108 ? 1 : -1];
typedef char verify_event_manager_globals_size[
	sizeof(struct event_manager_globals) == 0x168 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct event_manager_globals event_manager_globals;

/* ---------- public code */

void event_manager_initialize(
	void)
{
	csmemset(&event_manager_globals.state, 0, sizeof(event_manager_globals.state));
	event_manager_globals.state.time_of_last_event = system_milliseconds();
	event_manager_globals.state.initialized = TRUE;
	return;
}

void event_manager_dispose(
	void)
{
	csmemset(&event_manager_globals.state, 0, sizeof(event_manager_globals.state));
	return;
}

void event_manager_flush(
	void)
{
	csmemset(event_manager_globals.state.events, 0, sizeof(event_manager_globals.state.events));
	return;
}

void event_manager_suppress(
	boolean suppress)
{
	event_manager_globals.state.suppressed = suppress;
	return;
}

boolean get_next_event(
	struct event_record *event,
	short local_player_index)
{
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\event_manager.c",
		211,
		event && ((local_player_index>=0 && local_player_index<MAXIMUM_GAMEPADS) || local_player_index==NONE));

	if (event_manager_globals.state.initialized)
	{
		if (local_player_index == NONE)
		{
			local_player_index = 0;
			while (!result)
			{
				if (local_player_index >= MAXIMUM_GAMEPADS)
					break;
				result = get_next_event(event, local_player_index);
				local_player_index++;
			}
		}
		else
		{
			long event_index;

			for (event_index = 7; event_index >= 0; event_index--)
			{
				if (event_manager_globals.state.events[local_player_index][event_index].type != 0)
				{
					*event = event_manager_globals.state.events[local_player_index][event_index];
					event_manager_globals.state.events[local_player_index][event_index].type = 0;
					return TRUE;
				}
			}
		}
	}

	return result;
}

unsigned long event_manager_time_of_last_event(
	void)
{
	return event_manager_globals.state.time_of_last_event;
}

/* ---------- private code */
