/*
CINEMATICS.C

symbols in this file:
000813F0 0040:
	_cinematic_initialize (0000)
00081430 0010:
	_cinematic_dispose (0000)
00081440 0030:
	_cinematic_initialize_for_new_map (0000)
00081470 0020:
	_cinematic_dispose_from_old_map (0000)
00081490 0040:
	_cinematic_start (0000)
000814D0 0010:
	_cinematic_can_be_skipped (0000)
000814E0 0010:
	_cinematic_skip_start (0000)
000814F0 0010:
	_cinematic_skip_stop (0000)
00081500 0030:
	_cinematic_show_letterbox (0000)
00081530 0150:
	_draw_quad (0000)
00081680 0020:
	_cinematic_force_title (0000)
000816A0 0020:
	_cinematic_suppress_bsp_object_creation (0000)
000816C0 0050:
	_cinematic_stop (0000)
00081710 0010:
	_cinematic_in_progress (0000)
00081720 0090:
	_cinematic_set_title_delayed (0000)
000817B0 0500:
	_cinematic_render (0000)
00081CB0 0020:
	_cinematic_set_title (0000)
002589FC 0012:
	??_C@_0BC@NGABDHNK@cinematic_globals?$AA@ (0000)
00258A10 0025:
	??_C@_0CF@PIGNLDBH@c?3?2halo?2SOURCE?2cutscene?2cinemati@ (0000)
00258A38 0012:
	??_C@_0BC@PJBLDLNO@cinematic?5globals?$AA@ (0000)
00258A4C 0032:
	??_C@_0DC@GMMMKPOA@no?5free?5chapter?5title?5slots?5to?5d@ (0000)
00258A80 0004:
	__real@3e000000 (0000)
00435CA0 0004:
	_cinematic_globals (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cinematics.h"
#include "game/game.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct cinematic_title
{
	short title_index;
	short time;
};

struct cinematic_global_data
{
	long title_index;
	long start_tick;
	boolean show_letterbox;
	boolean in_progress;
	boolean can_be_skipped;
	boolean suppress_bsp_object_creation;
	struct cinematic_title queued_titles[4];
};

typedef char verify_cinematic_title_size[
	sizeof(struct cinematic_title) == 0x4 ? 1 : -1];
typedef char verify_cinematic_global_data_size[
	sizeof(struct cinematic_global_data) == 0x1C ? 1 : -1];

/* ---------- prototypes */

void cinematic_set_title_delayed(
	short title_index,
	real delay);

/* ---------- globals */

struct cinematic_global_data *cinematic_globals;

/* ---------- public code */

void cinematic_dispose(
	void)
{
	return;
}

void cinematic_skip_start(
	void)
{
	cinematic_globals->can_be_skipped = TRUE;

	return;
}

void cinematic_skip_stop(
	void)
{
	cinematic_globals->can_be_skipped = FALSE;

	return;
}

void cinematic_dispose_from_old_map(
	void)
{
	cinematic_globals->show_letterbox = FALSE;
	cinematic_globals->in_progress = FALSE;

	return;
}

boolean cinematic_can_be_skipped(
	void)
{
	return cinematic_globals->can_be_skipped;
}

void cinematic_show_letterbox(
	boolean show)
{
	cinematic_globals->show_letterbox = show;
	if (show)
	{
		cinematic_globals->start_tick = game_time_get();
	}

	return;
}

void cinematic_force_title(
	unsigned short title_index)
{
	cinematic_globals->queued_titles[0].title_index = title_index;
	cinematic_globals->queued_titles[0].time = 0;

	return;
}

void cinematic_suppress_bsp_object_creation(
	boolean suppress)
{
	cinematic_globals->suppress_bsp_object_creation = suppress;

	return;
}

boolean cinematic_in_progress(
	void)
{
	return cinematic_globals->in_progress;
}

void cinematic_set_title(
	unsigned short title_index)
{
	cinematic_set_title_delayed(title_index, 0.0f);

	return;
}

/* ---------- private code */
