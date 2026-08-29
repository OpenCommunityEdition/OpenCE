/*
RASTERIZER_XBOX_DETAIL_OBJECTS.C

symbols in this file:
0014B7E0 01b0:
	_code_0014b7e0 (0000)
0014B990 0050:
	_code_0014b990 (0000)
0014B9E0 0020:
	_code_0014b9e0 (0000)
0014BA00 0120:
	_code_0014ba00 (0000)
0014BB20 0070:
	_rasterizer_detail_objects_initialize (0000)
0014BB90 0220:
	_code_0014bb90 (0000)
0014BDB0 0060:
	_code_0014bdb0 (0000)
0014BE10 0010:
	_code_0014be10 (0000)
0014BE20 0010:
	_code_0014be20 (0000)
0014BE30 0010:
	_code_0014be30 (0000)
0014BE40 0010:
	__rasterizer_detail_objects_end (0000)
0014BE50 0030:
	_code_0014be50 (0000)
0014BE80 0010:
	_code_0014be80 (0000)
0014BE90 0010:
	_code_0014be90 (0000)
0014BEA0 0020:
	_code_0014bea0 (0000)
0014BEC0 0010:
	_code_0014bec0 (0000)
0014BED0 0070:
	_rasterizer_detail_objects_dispose (0000)
0014BF40 0290:
	__rasterizer_detail_objects_begin (0000)
0014C1D0 0230:
	__rasterizer_detail_objects_rebuild_vertices (0000)
0014C400 0470:
	__rasterizer_detail_objects_draw (0000)
0028E028 0036:
	??_C@_0DG@OJLPBMH@?$CD?$CD?$CD?5ERROR?5rasterizer_detail_obje@ (0000)
0028E060 010b:
	??_C@_0BAL@NENAACKL@IDirect3DDevice8_CreateVertexBuf@ (0000)
0028E170 0040:
	??_C@_0EA@LGHKKBIM@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
0028E1B0 003f:
	??_C@_0DP@PFBAPIPD@?$CD?$CD?$CD?5ERROR?5too?5many?5detail?5object@ (0000)
0028E1F0 0018:
	??_C@_0BI@NKKNLOHO@detail_object_view_data?$AA@ (0000)
0028E208 0030:
	??_C@_0DA@LEHODDDN@?$CD?$CD?$CD?5ERROR?5rasterizer_detail_obje@ (0000)
0028E238 00a5:
	??_C@_0KF@HFCLJJLO@IDirect3DDevice8_DrawVertices?$CIgl@ (0000)
0028E2E0 00ab:
	??_C@_0KL@CKHIIGPN@IDirect3DDevice8_SetVertexData4f@ (0000)
0028E390 00c7:
	??_C@_0MH@KOGEPMP@IDirect3DDevice8_SetVertexData4f@ (0000)
0028E458 0019:
	??_C@_0BJ@ODGAOKJE@cell?9?$DOz_reference_vector?$AA@ (0000)
0028E478 0098:
	??_C@_0JI@PJJHOGAJ@IDirect3DDevice8_SetVertexShader@ (0000)
0028E510 00b8:
	??_C@_0LI@CELBHMPB@IDirect3DDevice8_SetVertexShader@ (0000)
0045E904 0004:
	_bss_0045e904 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include <xtl.h>

/* ---------- constants */

enum
{
	DETAIL_OBJECT_VERTEX_BUFFER_SIZE = 0x20000,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void rasterizer_error(
	long error_result,
	char const *format,
	...);

void rasterizer_profile_end(
	short profile);

/* ---------- globals */

extern D3DDevice *global_d3d_device;
D3DVertexBuffer *bss_0045e904 = NULL;

#define local_d3d_vertex_buffer bss_0045e904

/* ---------- public code */

boolean rasterizer_detail_objects_initialize(
	void)
{
	boolean success;
	long result;

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0x62,
		global_d3d_device);
	result = IDirect3DDevice8_CreateVertexBuffer(
		global_d3d_device,
		DETAIL_OBJECT_VERTEX_BUFFER_SIZE,
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		0,
		D3DPOOL_DEFAULT,
		&local_d3d_vertex_buffer);
	if (result >= 0)
	{
		success = TRUE;
	}
	else
	{
		success = FALSE;
		rasterizer_error(
			result,
			"IDirect3DDevice8_CreateVertexBuffer(global_d3d_device, RASTERIZER_MAXIMUM_DETAIL_OBJECTS_PER_FRAME*NUMBER_OF_VERTICES_PER_QUADRILATERAL*sizeof(struct detail_object_vertex), RASTERIZER_DYNAMIC_BUFFER_USAGE, 0, RASTERIZER_DYNAMIC_BUFFER_POOL, &local_d3d_vertex_buffer)");
		error(
			_error_silent,
			"### ERROR rasterizer_detail_objects_initialize failed");
	}

	return success;
}

void _rasterizer_detail_objects_end(
	void)
{
	rasterizer_profile_end(21);
	return;
}

void rasterizer_detail_objects_dispose(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0x77,
		local_d3d_vertex_buffer);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0x78,
		global_d3d_device);
	if (local_d3d_vertex_buffer)
	{
		IDirect3DVertexBuffer8_Release(local_d3d_vertex_buffer);
		local_d3d_vertex_buffer = NULL;
	}

	return;
}

/* ---------- private code */
