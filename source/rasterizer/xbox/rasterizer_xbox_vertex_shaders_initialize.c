/*
RASTERIZER_XBOX_VERTEX_SHADERS_INITIALIZE.C

symbols in this file:
00168070 0020:
	_code_00168070 (0000)
00168090 0010:
	_code_00168090 (0000)
001680A0 0260:
	_rasterizer_vertex_shaders_initialize (0000)
00168300 0050:
	_rasterizer_vertex_shaders_dispose (0000)
0029BF94 00e0:
	_rdata_0029bf94 (0000)
0029C074 0036:
	??_C@_0DG@DJPOAFHM@?$CD?$CD?$CD?5ERROR?5rasterizer_vertex_shad@ (0000)
0029C0B0 00e9:
	??_C@_0OJ@IGEKCPEI@IDirect3DDevice8_CreateVertexSha@ (0000)
0029C19C 002e:
	??_C@_0CO@JPPOFKGN@vertex_shader_table?$FLvertex_shade@ (0000)
0029C1CC 0035:
	??_C@_0DF@NLIMIMDJ@vertex_shader_table?$FLvertex_shade@ (0000)
0029C208 004b:
	??_C@_0EL@FLPDNHEI@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
0029C254 0033:
	??_C@_0DD@EOLJNOFA@?$CD?$CD?$CD?5ERROR?5rasterizer_vertex_shad@ (0000)
0029C288 006f:
	??_C@_0GP@DNJKFNGM@IDirect3DDevice8_DeleteVertexSha@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct vertex_shader_entry
{
	void const *declaration;
	void const *function;
	unsigned long handle;
	long instruction_count;
};

/* ---------- prototypes */

void __stdcall D3DDevice_DeleteVertexShader(
	unsigned long handle);

void rasterizer_error(
	long error_result,
	char const *format,
	...);

static __inline long delete_vertex_shader(
	void *device,
	unsigned long handle)
{
	D3DDevice_DeleteVertexShader(handle);
	return 0;
}

/* ---------- globals */

extern void *global_d3d_device;
extern struct vertex_shader_entry vertex_shader_table[67];

/* ---------- public code */

void rasterizer_vertex_shaders_dispose(
	void)
{
	boolean success;
	short vertex_shader_index;

	success = TRUE;
	for (vertex_shader_index = 0; vertex_shader_index < 67; vertex_shader_index++)
	{
		if (delete_vertex_shader(global_d3d_device, vertex_shader_table[vertex_shader_index].handle) >= 0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DeleteVertexShader(global_d3d_device, (DWORD)vertex_shader_table[vertex_shader_index].handle)");
		}
	}

	if (!success)
	{
		error(2, "### ERROR rasterizer_vertex_shaders_dispose failed");
	}
	return;
}

/* ---------- private code */
