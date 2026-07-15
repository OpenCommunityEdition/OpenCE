/*
STRUCTURE_RUNTIME_DECALS.C

symbols in this file:
00185AE0 0040:
	_structure_decals_initialize (0000)
00185B20 0030:
	_structure_decals_initialize_for_new_map (0000)
00185B50 0030:
	_structure_decals_reconnect_to_structure_bsp (0000)
00185B80 0070:
	_structure_decals_disconnect_from_structure_bsp (0000)
00185BF0 0010:
	_structure_decals_dispose_from_old_map (0000)
00185C00 0010:
	_structure_decals_dispose (0000)
00185C10 0210:
	_structure_decals_update (0000)
002A1A8C 0019:
	??_C@_0BJ@EHFGPMAL@structure_decals_globals?$AA@ (0000)
002A1AA8 0035:
	??_C@_0DF@EMFPFLCM@c?3?2halo?2SOURCE?2structures?2struct@ (0000)
002A1AE0 0011:
	??_C@_0BB@PDFGKABJ@structure?5decals?$AA@ (0000)
004C0CE8 0004:
	_structure_decals_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "saved games/game_state.h"
#include "structures.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct structure_decals_globals
{
	boolean reconnect_to_structure_bsp;
	byte pad[3];
};

/* ---------- prototypes */

/* ---------- globals */

struct structure_decals_globals *structure_decals_globals = NULL;

/* ---------- public code */

void structure_decals_initialize(
	void)
{
	structure_decals_globals = game_state_malloc("structure decals", NULL, sizeof(*structure_decals_globals));
	match_assert("c:\\halo\\SOURCE\\structures\\structure_runtime_decals.c", 0x1c, structure_decals_globals);

	return;
}

void structure_decals_initialize_for_new_map(
	void)
{
	match_assert("c:\\halo\\SOURCE\\structures\\structure_runtime_decals.c", 0x24, structure_decals_globals);
	structure_decals_globals->reconnect_to_structure_bsp = FALSE;

	return;
}

void structure_decals_reconnect_to_structure_bsp(
	void)
{
	match_assert("c:\\halo\\SOURCE\\structures\\structure_runtime_decals.c", 0x2d, structure_decals_globals);
	structure_decals_globals->reconnect_to_structure_bsp = TRUE;

	return;
}

void structure_decals_dispose_from_old_map(
	void)
{
	return;
}

void structure_decals_dispose(
	void)
{
	return;
}

/* ---------- private code */
