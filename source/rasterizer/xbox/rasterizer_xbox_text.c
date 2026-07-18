/*
RASTERIZER_XBOX_TEXT.C

symbols in this file:
00162EA0 01b0:
	_code_00162ea0 (0000)
00163050 0050:
	_code_00163050 (0000)
001630A0 0220:
	_code_001630a0 (0000)
001632C0 0060:
	_code_001632c0 (0000)
00163320 0010:
	_code_00163320 (0000)
00163330 0010:
	_rasterizer_text_end (0000)
00163340 0020:
	_code_00163340 (0000)
00163360 0010:
	_code_00163360 (0000)
00163370 0010:
	_code_00163370 (0000)
00163380 0010:
	_code_00163380 (0000)
00163390 0690:
	_rasterizer_text_begin (0000)
00163A20 0120:
	_rasterizer_text_draw_character (0000)
00292B28 0036:
	??_C@_0DG@NLGEIAMP@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
00292B60 0030:
	??_C@_0DA@FEBHLDDN@?$CD?$CD?$CD?5ERROR?5rasterizer_text_draw_c@ (0000)
00292B90 0087:
	??_C@_0IH@NAGGOICD@IDirect3DDevice8_SetVertexData2f@ (0000)
00292C18 007d:
	??_C@_0HN@NMADEKEL@IDirect3DDevice8_SetVertexData2f@ (0000)
00292C98 0058:
	??_C@_0FI@KKLBHBJN@IDirect3DDevice8_SetVertexDataCo@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "real_math.h"
/* The January object retains out-of-line copies of the D3D inline wrappers:
 * code_00162ea0 = D3DDevice_SetRenderState
 * code_00163050 = D3DDevice_SetTextureStageState
 * code_001630a0 = IDirect3DDevice8_SetRenderState
 * code_001632c0 = IDirect3DDevice8_SetTextureStageState
 * code_00163320 = IDirect3DDevice8_SetVertexShaderConstant
 * code_00163340 = IDirect3DDevice8_SetVertexData2f
 * code_00163360 = IDirect3DDevice8_SetVertexDataColor
 * code_00163370 = IDirect3DDevice8_Begin
 * code_00163380 = IDirect3DDevice8_End
 */
#define D3DINLINE static
#include <xtl.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct rasterizer_text_vertex
{
	real x;
	real y;
	real u;
	real v;
	unsigned long color;
};

struct rasterizer_text_debug_options
{
	byte reserved00[0x22];
	boolean render_text;
};

struct rasterizer_text_window_parameters
{
	short rasterizer_target;
};

/* ---------- prototypes */

void rasterizer_error(
	long error_result,
	char const *format,
	...);

/* ---------- globals */

extern void *global_d3d_device;
extern struct rasterizer_text_debug_options rasterizer_debug_options;
extern struct rasterizer_text_window_parameters global_window_parameters;

/* ---------- public code */

void rasterizer_text_end(
	void)
{
	return;
}

void rasterizer_text_draw_character(
	struct rasterizer_text_vertex const *vertices)
{
	boolean success;
	short vertex_index;

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_text.c",
		216,
		global_d3d_device);

	if (rasterizer_debug_options.render_text &&
		global_window_parameters.rasterizer_target == 0)
	{
		success = IDirect3DDevice8_Begin(
			global_d3d_device,
			D3DPT_TRIANGLEFAN) >= 0;
		for (vertex_index = 0; vertex_index < 4; vertex_index++)
		{
			if (IDirect3DDevice8_SetVertexDataColor(
				global_d3d_device,
				9,
				vertices[vertex_index].color) >= 0 && success)
			{
				success = TRUE;
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					0,
					"IDirect3DDevice8_SetVertexDataColor(global_d3d_device, 9, vertices[vertex_index].color)");
			}

			if (IDirect3DDevice8_SetVertexData2f(
				global_d3d_device,
				4,
				vertices[vertex_index].u,
				vertices[vertex_index].v) >= 0 && success)
			{
				success = TRUE;
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					0,
					"IDirect3DDevice8_SetVertexData2f(global_d3d_device, 4, vertices[vertex_index].texcoord.u, vertices[vertex_index].texcoord.v)");
			}

			if (IDirect3DDevice8_SetVertexData2f(
				global_d3d_device,
				0,
				vertices[vertex_index].x,
				vertices[vertex_index].y) >= 0 && success)
			{
				success = TRUE;
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					0,
					"IDirect3DDevice8_SetVertexData2f(global_d3d_device, VSDE_VERTEX, vertices[vertex_index].position.x, vertices[vertex_index].position.y)");
			}
		}

		if (IDirect3DDevice8_End(global_d3d_device) >= 0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_End(global_d3d_device)");
		}

		if (!success)
			error(2, "### ERROR rasterizer_text_draw_character failed");
	}

	return;
}

/* ---------- private code */
