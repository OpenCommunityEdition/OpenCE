/*
RENDER_MEDIA.C

symbols in this file:
0017A6F0 0020:
	_render_media_initialize_for_new_map (0000)
0017A710 0010:
	_render_media_dispose_from_old_map (0000)
0017A720 0010:
	_render_media_build_render_target (0000)
0017A730 0010:
	_render_media (0000)
0029FEAC 0004:
	_media_wave_amplitude (0000)
0030D4D8 00ac:
	_standard_frame_level_of_detail (0000)
	_render_target_frame_level_of_detail (0058)
004C004C 002c:
	_bss_004c004c (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct render_media_globals
{
	boolean initialized;
	byte pad[3];
	long render_target_index;
	byte runtime_state[0x24];
};

/* ---------- prototypes */

/* ---------- globals */

static struct render_media_globals render_media_globals;

/* ---------- public code */

void render_media_initialize_for_new_map(void)
{
	render_media_globals.initialized = TRUE;
	render_media_globals.render_target_index = 0;
}

/* ---------- private code */
