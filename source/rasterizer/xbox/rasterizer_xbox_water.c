/*
RASTERIZER_XBOX_WATER.C

symbols in this file:
001688F0 01b0:
	_code_001688f0 (0000)
00168AA0 0050:
	_code_00168aa0 (0000)
00168AF0 0020:
	_rasterizer_water_set_visibility_for_frame (0000)
00168B10 0010:
	_rasterizer_water_set_visibility_for_window (0000)
00168B20 0010:
	_rasterizer_water_get_visibility_for_window (0000)
00168B30 0220:
	_code_00168b30 (0000)
00168D50 0060:
	_code_00168d50 (0000)
00168DB0 0010:
	_code_00168db0 (0000)
00168DC0 0020:
	_code_00168dc0 (0000)
00168DE0 0010:
	_code_00168de0 (0000)
00168DF0 0010:
	_code_00168df0 (0000)
00168E00 0010:
	_code_00168e00 (0000)
00168E10 0820:
	_rasterizer_water_build_bumpmap (0000)
00169630 08c0:
	_rasterizer_water_draw (0000)
0029CBA0 0030:
	??_C@_0DA@GMAFCPCK@?$CD?$CD?$CD?5ERROR?5rasterizer_water_build@ (0000)
0029CBD0 0043:
	??_C@_0ED@IHGINMLN@ripples?$FL2?$FN?4contibution_factor?5?$CL?5@ (0000)
0029CC18 0043:
	??_C@_0ED@IIOIOBNN@ripples?$FL0?$FN?4contibution_factor?5?$CL?5@ (0000)
0029CC5C 0024:
	??_C@_0CE@CFGOBBBI@ripples?$FLripple_index?$FN?4map_repeat@ (0000)
0029CC80 0037:
	??_C@_0DH@OMMLHAPF@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
004662E8 0002:
	_bss_004662e8 (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct rasterizer_water_visibility_globals
{
	boolean needs_update;
	boolean visible_for_window;
};

typedef char verify_rasterizer_water_visibility_globals_size[
	sizeof(struct rasterizer_water_visibility_globals) == 2 ? 1 : -1];
typedef char verify_rasterizer_water_needs_update_offset[
	offsetof(struct rasterizer_water_visibility_globals, needs_update) == 0 ? 1 : -1];
typedef char verify_rasterizer_water_visible_for_window_offset[
	offsetof(struct rasterizer_water_visibility_globals, visible_for_window) == 1 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct rasterizer_water_visibility_globals bss_004662e8;

/* ---------- public code */

void rasterizer_water_set_visibility_for_frame(
	boolean visibility)
{
	bss_004662e8.needs_update = !visibility;
	bss_004662e8.visible_for_window = visibility;

	return;
}

void rasterizer_water_set_visibility_for_window(
	boolean visibility)
{
	bss_004662e8.visible_for_window = visibility;

	return;
}

boolean rasterizer_water_get_visibility_for_window(
	void)
{
	return bss_004662e8.visible_for_window;
}

/* ---------- private code */
