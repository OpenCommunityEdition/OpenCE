/*
ANTENNA.C

symbols in this file:
00120710 0030:
	_antennas_initialize (0000)
00120740 0010:
	_antennas_initialize_for_new_map (0000)
00120750 0010:
	_antennas_dispose_from_old_map (0000)
00120760 0020:
	_antennas_dispose (0000)
00120780 0200:
	_antenna_new (0000)
00120980 0020:
	_antenna_delete (0000)
001209A0 0130:
	_code_001209a0 (0000)
00120AD0 0170:
	_code_00120ad0 (0000)
00120C40 0310:
	_code_00120c40 (0000)
00120F50 0090:
	_antenna_render (0000)
00120FE0 00b0:
	_antennas_update (0000)
00288EEC 0022:
	??_C@_0CC@JBAGLFPJ@couldn?8t?5allocate?5antenna?5global@ (0000)
00288F10 0008:
	??_C@_07HJCCDMBN@antenna?$AA@ (0000)
*/

/* ---------- headers */

#include "objects/widgets/antenna.h"

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "saved games/game_state.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void antennas_initialize(
	void)
{
	antenna_data = game_state_data_new("antenna", 12, 0x2BC);
	if (!antenna_data)
		error(_error_immediate, "couldn't allocate antenna globals");

	return;
}

void antennas_initialize_for_new_map(
	void)
{
	data_make_valid(antenna_data);

	return;
}

void antennas_dispose_from_old_map(
	void)
{
	data_make_invalid(antenna_data);

	return;
}

void antennas_dispose(
	void)
{
	if (antenna_data)
		antenna_data = NULL;

	return;
}

void antenna_delete(
	long antenna_index)
{
	datum_delete(antenna_data, antenna_index);
	return;
}

/* ---------- private code */
