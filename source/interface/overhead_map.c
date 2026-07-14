/*
OVERHEAD_MAP.C

symbols in this file:
000CFB20 0010:
	_overhead_map_initialize (0000)
000CFB30 0010:
	_overhead_map_initialize_for_new_map (0000)
000CFB40 0010:
	_overhead_map_dispose_from_old_map (0000)
000CFB50 00e0:
	_overhead_map_post_rasterize (0000)
00270BD0 000c:
	_rdata_00270bd0 (0000)
00453CD0 0030:
	_bss_00453cd0 (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct overhead_map_globals
{
	boolean enabled;
	byte runtime_state[0x2F];
};

/* ---------- prototypes */

/* ---------- globals */

static struct overhead_map_globals overhead_map_globals;

/* ---------- public code */

void overhead_map_initialize(void)
{
}

/* ---------- private code */
