/*
RASTERIZER_CINEMATICS.C

symbols in this file:
0016D140 0020:
	_code_0016d140 (0000)
0016D160 0040:
	_rasterizer_screen_effects_initialize (0000)
0016D1A0 0030:
	_rasterizer_screen_effects_initialize_for_new_map (0000)
0016D1D0 0010:
	_rasterizer_screen_effects_dispose_from_old_map (0000)
0016D1E0 0010:
	_rasterizer_screen_effects_dispose (0000)
0016D1F0 0030:
	_rasterizer_script_screen_effect_set_value (0000)
0016D220 0030:
	_rasterizer_script_screen_effect_get_value (0000)
0016D250 0040:
	_rasterizer_screen_effect_start (0000)
0016D290 0070:
	_rasterizer_screen_effect_set_convolution (0000)
0016D300 0070:
	_rasterizer_screen_effect_set_filter (0000)
0016D370 0020:
	_rasterizer_screen_effect_set_filter_desaturation_tint (0000)
0016D390 0120:
	_rasterizer_screen_effect_set_video (0000)
0016D4B0 0010:
	_rasterizer_screen_effect_stop (0000)
0016D4C0 0250:
	_rasterizer_screen_effect_get_cinematic_parameters (0000)
0016D710 0020:
	_rasterizer_set_near_clip_distance (0000)
0016D730 0030:
	_rasterizer_get_near_clip_distance (0000)
0029D844 0020:
	??_C@_0CA@HAHKAAEK@cinematic_screen_effect_globals?$AA@ (0000)
0029D864 0032:
	??_C@_0DC@FCKAGPCD@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029D898 0014:
	??_C@_0BE@COKLOHBN@screen?5effect?5filth?$AA@ (0000)
0029D8B0 004a:
	??_C@_0EK@IJBENNIM@?$CD?$CD?$CD?5ERROR?5cinematics?5failed?5to?5s@ (0000)
0029D900 008d:
	??_C@_0IN@PCKCIAIJ@?$CD?$CD?$CD?5FATAL_ERROR?5screen?5effects?5c@ (0000)
004662F4 0004:
	_bss_004662f4 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct rasterizer_cinematic_screen_effect_state
{
	byte reserved00[0x38];
	boolean has_control;
	byte reserved39[0x2B];
	real script_values[4];
	real near_clip_distance;
};

typedef char rasterizer_cinematic_screen_effect_state_size_assert[
	sizeof(struct rasterizer_cinematic_screen_effect_state) == 0x78 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_has_control_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, has_control) == 0x38 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_script_values_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, script_values) == 0x64 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_near_clip_distance_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, near_clip_distance) == 0x74 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct rasterizer_cinematic_screen_effect_state *bss_004662f4;

/* ---------- public code */

void rasterizer_screen_effects_dispose_from_old_map(
	void)
{
	return;
}

void rasterizer_screen_effects_dispose(
	void)
{
	return;
}

real rasterizer_script_screen_effect_get_value(
	short effect_index)
{
	real value = 0.0f;

	if (bss_004662f4 && effect_index >= 0 && effect_index < 4)
	{
		value = bss_004662f4->script_values[effect_index];
	}

	return value;
}

void rasterizer_screen_effect_stop(
	void)
{
	if (bss_004662f4)
	{
		bss_004662f4->has_control = FALSE;
	}

	return;
}

void rasterizer_set_near_clip_distance(
	real near_clip_distance)
{
	if (bss_004662f4)
	{
		bss_004662f4->near_clip_distance = near_clip_distance;
	}

	return;
}

/* ---------- private code */
