/*
RASTERIZER_XBOX_ACTIVE_CAMOUFLAGE.C

symbols in this file:
001488C0 0070:
	_real_alpha_to_pixel32 (0000)
00148930 01b0:
	_D3DDevice_SetRenderState (0000)
00148AE0 0050:
	_D3DDevice_SetTextureStageState (0000)
00148B30 0020:
	_rasterizer_active_camouflage_set_visibility (0000)
00148B50 0220:
	_IDirect3DDevice8_SetRenderState@12 (0000)
00148D70 0060:
	_IDirect3DDevice8_SetTextureStageState@16 (0000)
00148DD0 0010:
	_IDirect3DDevice8_SetVertexShaderConstant@16 (0000)
00148DE0 0010:
	_IDirect3DDevice8_SetVertexData2s@16 (0000)
00148DF0 0010:
	_IDirect3DDevice8_Begin@8 (0000)
00148E00 0010:
	_IDirect3DDevice8_End@4 (0000)
00148E10 0340:
	_rasterizer_active_camouflage_cache_primary_render_target (0000)
00149150 07e0:
	_rasterizer_active_camouflage_draw (0000)
0028DA50 004e:
	??_C@_0EO@HDJOIPE@global_window_parameters?4rasteri@ (0000)
0028DAA0 0043:
	??_C@_0ED@HJIAGIIM@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
0028DAE8 0045:
	??_C@_0EF@JPIBDDAJ@?$CBTEST_FLAG?$CIgroup?9?$DOgeometry_flags@ (0000)
0028DB30 002a:
	??_C@_0CK@KJCGLLHP@local_active_camouflage_debug_ca@ (0000)
0028DB5C 001e:
	??_C@_0BO@PEAJKHII@group?9?$DOeffect?4intensity?$DM?$DN1?40f?$AA@ (0000)
0028DB7C 001d:
	??_C@_0BN@DOLIELKA@group?9?$DOeffect?4intensity?$DO0?40f?$AA@ (0000)
0028DBA0 0040:
	??_C@_0EA@OILAHNPH@group?9?$DOeffect?4type?$DN?$DN_render_mode@ (0000)
0028DBE0 000e:
	??_C@_0O@KHNLKNGC@group?9?$DOshader?$AA@ (0000)
0045E8E0 0006:
	_local_active_camouflage_visibility_flag (0000)
	_local_active_camouflage_debug_cached_flag (0001)
	_local_active_camouflage_debug_cache_count (0004)
*/

/* ---------- headers */

#include "cseries.h"
#include "rasterizer/rasterizer.h"

#include <xtl.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

boolean local_active_camouflage_visibility_flag = FALSE;
boolean local_active_camouflage_debug_cached_flag = FALSE;
short local_active_camouflage_debug_cache_count = 0;

/* ---------- public code */

void rasterizer_active_camouflage_set_visibility(
	boolean visibility)
{
	local_active_camouflage_visibility_flag = visibility;

	if (!visibility)
	{
		local_active_camouflage_debug_cache_count = 0;
		local_active_camouflage_debug_cached_flag = FALSE;
	}

	return;
}

/* ---------- private code */
