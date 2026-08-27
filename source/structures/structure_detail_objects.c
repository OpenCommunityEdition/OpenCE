/*
STRUCTURE_DETAIL_OBJECTS.C

symbols in this file:
00183160 0020:
	_code_00183160 (0000)
00183180 0040:
	_code_00183180 (0000)
001831C0 0050:
	_structure_detail_objects_initialize (0000)
00183210 0010:
	_structure_detail_objects_dispose_from_old_map (0000)
00183220 0010:
	_structure_detail_objects_dispose (0000)
00183230 0010:
	_structure_detail_objects_flush (0000)
00183240 0030:
	_detail_object_offset (0000)
00183270 0030:
	_code_00183270 (0000)
001832A0 0030:
	_code_001832a0 (0000)
001832D0 0080:
	_code_001832d0 (0000)
00183350 0080:
	_code_00183350 (0000)
001833D0 0030:
	_dot_product4d (0000)
00183400 0050:
	_structure_detail_objects_initialize_for_new_map (0000)
00183450 0470:
	_structure_render_detail_objects (0000)
001838C0 02f0:
	_render_debug_detail_objects (0000)
002A12BC 0016:
	??_C@_0BG@BEHEKGEA@local_player_index?$DN?$DN0?$AA@ (0000)
002A12D4 0035:
	??_C@_0DF@DGEAAFAE@c?3?2halo?2SOURCE?2structures?2struct@ (0000)
002A130C 0019:
	??_C@_0BJ@IOENOFIB@structure?5detail?5objects?$AA@ (0000)
002A1328 0022:
	??_C@_0CC@GPGHNCNI@detail_object_global_runtime_dat@ (0000)
002A134C 0023:
	??_C@_0CD@OMDCMBBI@lower_bound_cell?$DM?$DNupper_bound_ce@ (0000)
004C0CBC 0014:
	_debug_detail_objects (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#define dot_product4d dot_product4d_inline
#include "math/real_math.h"
#undef dot_product4d
#include "structures.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct detail_object_runtime_data
{
	byte reserved0000[0x520E];
	boolean initialized;
	byte reserved520F;
};

struct detail_object_global_runtime_data
{
	struct detail_object_runtime_data local_player_data[2];
	real_vector4d default_z_reference_vector;
};

struct detail_object_globals
{
	boolean enabled;
	byte pad01[3];
	struct detail_object_global_runtime_data *runtime_data;
	boolean fudge_vector;
	byte pad09[3];
	real fudge_offset;
	real final_offset;
};

typedef char detail_object_runtime_data_size[
	sizeof(struct detail_object_runtime_data) == 0x5210 ? 1 : -1];
typedef char detail_object_global_runtime_data_size[
	sizeof(struct detail_object_global_runtime_data) == 0xA430 ? 1 : -1];
typedef char detail_object_globals_size[
	sizeof(struct detail_object_globals) == 0x14 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct detail_object_globals debug_detail_objects = { 0 };

#define detail_object_global_runtime_data debug_detail_objects.runtime_data

/* ---------- public code */

void structure_detail_objects_dispose_from_old_map(
	void)
{
	return;
}

void structure_detail_objects_dispose(
	void)
{
	return;
}

void structure_detail_objects_flush(
	void)
{
	debug_detail_objects.runtime_data->local_player_data[0].initialized = FALSE;

	return;
}

/* ---------- private code */

real dot_product4d(
	real_vector4d const *a,
	real_vector4d const *b)
{
	return a->i*b->i + a->j*b->j + a->k*b->k + a->l*b->l;
}

void structure_detail_objects_initialize_for_new_map(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\structures\\structure_detail_objects.c",
		0x6D,
		detail_object_global_runtime_data);
	csmemset(
		detail_object_global_runtime_data,
		0,
		sizeof(*detail_object_global_runtime_data));
	detail_object_global_runtime_data->local_player_data[0].initialized = FALSE;

	return;
}
