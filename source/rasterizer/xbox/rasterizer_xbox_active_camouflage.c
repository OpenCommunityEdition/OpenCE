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
#include "rasterizer/rasterizer_active_camouflage.h"
#include "rasterizer/rasterizer_geometry.h"

#include <xtl.h>

#include "rasterizer/xbox/rasterizer_xbox.h"
#include "rasterizer/xbox/rasterizer_xbox_internal.h"
#include "rasterizer/rasterizer_debug_options.h"
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"

/* ---------- constants */

enum
{
	_rasterizer_target_render_primary = 0,
	_rasterizer_target_render_secondary
};

enum
{
	RASTERIZER_STENCIL_MODE_NONE = 0,
	RASTERIZER_STENCIL_MODE_REJECT = 2,
};

enum
{
	ACTIVE_CAMOUFLAGE_SCREEN_WIDTH = 320,
	ACTIVE_CAMOUFLAGE_SCREEN_HEIGHT = 240
};

/* ---------- macros */

/* ---------- structures */


/* ---------- prototypes */

/* ---------- globals */

extern struct rasterizer_window_begin_parameters global_window_parameters;

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

void rasterizer_active_camouflage_cache_primary_render_target(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_active_camouflage.c",
		41,
		global_d3d_device);

	if (rasterizer_debug_options.active_camouflage &&
		local_active_camouflage_visibility_flag)
	{
		real screen_projection[5][4];
		rectangle2d bounds;

		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_active_camouflage.c",
			46,
			global_window_parameters.rasterizer_target==_rasterizer_target_render_primary);

		rasterizer_set_target_as_texture(
			0,
			_rasterizer_target_render_primary,
			0);
		IDirect3DDevice8_SetTextureStageState(
			global_d3d_device,
			0,
			D3DTSS_ADDRESSU,
			D3DTADDRESS_CLAMP);
		IDirect3DDevice8_SetTextureStageState(
			global_d3d_device,
			0,
			D3DTSS_ADDRESSV,
			D3DTADDRESS_CLAMP);
		IDirect3DDevice8_SetTextureStageState(
			global_d3d_device,
			0,
			D3DTSS_MAGFILTER,
			D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(
			global_d3d_device,
			0,
			D3DTSS_MINFILTER,
			D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(
			global_d3d_device,
			0,
			D3DTSS_MIPFILTER,
			D3DTEXF_POINT);

		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_CULLMODE,
			D3DCULL_CCW);
		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_COLORWRITEENABLE,
			D3DCOLORWRITEENABLE_RED |
			D3DCOLORWRITEENABLE_GREEN |
			D3DCOLORWRITEENABLE_BLUE);
		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_ALPHABLENDENABLE,
			FALSE);
		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_ALPHATESTENABLE,
			FALSE);
		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_ZENABLE,
			FALSE);
		IDirect3DDevice8_SetRenderState(
			global_d3d_device,
			D3DRS_ZBIAS,
			0);

		rasterizer_set_vertex_shader_permutation(
			4,
			_rasterizer_vertex_type_dynamic_screen,
			FALSE);

		screen_projection[0][0] =
			2.0f / (real)ACTIVE_CAMOUFLAGE_SCREEN_WIDTH;
		screen_projection[0][1] = 0.0f;
		screen_projection[0][2] = 0.0f;
		screen_projection[0][3] =
			-1.0f - 1.0f / (real)ACTIVE_CAMOUFLAGE_SCREEN_WIDTH;
		screen_projection[1][0] = 0.0f;
		screen_projection[1][1] =
			-2.0f / (real)ACTIVE_CAMOUFLAGE_SCREEN_HEIGHT;
		screen_projection[1][2] = 0.0f;
		screen_projection[1][3] =
			1.0f + 1.0f / (real)ACTIVE_CAMOUFLAGE_SCREEN_HEIGHT;
		screen_projection[2][0] = 0.0f;
		screen_projection[2][1] = 0.0f;
		screen_projection[2][2] = 0.0f;
		screen_projection[2][3] = 0.5f;
		screen_projection[3][0] = 0.0f;
		screen_projection[3][1] = 0.0f;
		screen_projection[3][2] = 0.0f;
		screen_projection[3][3] = 1.0f;
		screen_projection[4][0] = 1.0f;
		screen_projection[4][1] = 1.0f;
		screen_projection[4][2] = 0.0f;
		screen_projection[4][3] = 1.0f;
		IDirect3DDevice8_SetVertexShaderConstant(
			global_d3d_device,
			-68,
			screen_projection,
			5);

		csmemset(&pixel_shader, 0, sizeof(pixel_shader));
		pixel_shader.texture_modes = PS_TEXTUREMODES(
			PS_TEXTUREMODES_PROJECT2D,
			PS_TEXTUREMODES_NONE,
			PS_TEXTUREMODES_NONE,
			PS_TEXTUREMODES_NONE);
		pixel_shader.combiner_count = PS_COMBINERCOUNT(1, 0);
		pixel_shader.final_combiner_inputs_abcd = PS_COMBINERINPUTS(
			PS_REGISTER_ZERO,
			PS_REGISTER_ZERO,
			PS_REGISTER_ZERO,
			PS_REGISTER_T0);
		rasterizer_set_pixel_shader(&pixel_shader);

		rasterizer_set_target(
			_rasterizer_target_render_secondary,
			0,
			0,
			FALSE,
			FALSE);
		rasterizer_set_stencil_mode(RASTERIZER_STENCIL_MODE_NONE);

		IDirect3DDevice8_Begin(global_d3d_device, D3DPT_TRIANGLEFAN);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			4,
			global_window_parameters.camera.viewport_bounds.x0,
			global_window_parameters.camera.viewport_bounds.y1);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			0,
			0,
			0);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			4,
			global_window_parameters.camera.viewport_bounds.x1,
			global_window_parameters.camera.viewport_bounds.y1);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			0,
			ACTIVE_CAMOUFLAGE_SCREEN_WIDTH,
			0);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			4,
			global_window_parameters.camera.viewport_bounds.x1,
			global_window_parameters.camera.viewport_bounds.y0);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			0,
			ACTIVE_CAMOUFLAGE_SCREEN_WIDTH,
			ACTIVE_CAMOUFLAGE_SCREEN_HEIGHT);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			4,
			global_window_parameters.camera.viewport_bounds.x0,
			global_window_parameters.camera.viewport_bounds.y0);
		IDirect3DDevice8_SetVertexData2s(
			global_d3d_device,
			0,
			0,
			ACTIVE_CAMOUFLAGE_SCREEN_HEIGHT);
		IDirect3DDevice8_End(global_d3d_device);

		rasterizer_set_target(
			global_window_parameters.rasterizer_target,
			0,
			0,
			FALSE,
			TRUE);
		rasterizer_set_stencil_mode(RASTERIZER_STENCIL_MODE_REJECT);

		bounds.x0 = 512;
		bounds.x1 = 640;
		bounds.y0 = (local_active_camouflage_debug_cache_count + 1) * 96;
		bounds.y1 = (local_active_camouflage_debug_cache_count + 2) * 96;
		rasterizer_secondary_render_target_debug(&bounds);

		local_active_camouflage_debug_cache_count++;
		if (!rasterizer_debug_options.active_camouflage_multipass)
		{
			local_active_camouflage_visibility_flag = FALSE;
		}
		local_active_camouflage_debug_cached_flag = TRUE;
	}

	return;
}

/* ---------- private code */
