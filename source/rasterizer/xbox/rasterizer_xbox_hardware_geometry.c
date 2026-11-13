/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- prototypes */

#ifdef HALO_ANDROID
/* port: the buffers made here are written once, so Android draws them
from GL buffers of their own (port/linux/src/d3d8_gl.c) */
void xgpu_static_buffer_add(const void *resource, unsigned long size, int indices);
#endif

/* ---------- globals */

/* ---------- public code */

void __stdcall code_00158450(
	void *resource,
	void *data)
{
	return;
}

void __stdcall code_00158460(
	void *resource,
	void *data)
{
	return;
}

boolean rasterizer_vertex_buffer_new(
	struct vertex_buffer *vertex_buffer,
	long vertex_type,
	long count,
	void const *vertices,
	long buffer_size)
{
	/* port: NULL, the Xbox's bug fixed: a failed CreateVertexBuffer returns
	without writing it (January 0x5de713..0x5de719), and the test below read
	it uninitialised */
	D3DVertexBuffer *d3d_vertex_buffer = NULL;
	byte *locked_vertices;
	boolean success;
	short vertex_size;
	long result;

	success = TRUE;
	vertex_size = rasterizer_geometry_get_vertex_size(vertex_type);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c", 24, vertex_buffer);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c", 25, vertex_size*count==buffer_size || !vertices);

	if (!count)
		success = FALSE;
	if (!global_d3d_device)
		success = FALSE;
	else if (success)
	{
		result = IDirect3DDevice8_CreateVertexBuffer(
			global_d3d_device,
			buffer_size,
			D3DUSAGE_WRITEONLY,
			0,
			D3DPOOL_MANAGED,
			&d3d_vertex_buffer);
		if (result >= 0)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				result,
				"IDirect3DDevice8_CreateVertexBuffer(global_d3d_device, buffer_size, RASTERIZER_STATIC_BUFFER_USAGE, 0, RASTERIZER_STATIC_BUFFER_POOL, &d3d_vertex_buffer)");
		}
		if (!d3d_vertex_buffer)
			success = FALSE;
		if (!d3d_vertex_buffer || !success)
			d3d_vertex_buffer = NULL;
	}

	if (vertices && success)
	{
		rasterizer_globals.current_lock_operation = _rasterizer_lock_vertexbuffer_new;
		IDirect3DVertexBuffer8_Lock(
			d3d_vertex_buffer,
			0,
			buffer_size,
			&locked_vertices,
			0);
		rasterizer_globals.current_lock_operation = _rasterizer_lock_none;

		if (!locked_vertices)
			success = FALSE;
		if (!success)
		{
			locked_vertices = NULL;
		}
		else
		{
			csmemcpy(locked_vertices, vertices, buffer_size);
			result = IDirect3DVertexBuffer8_Unlock(d3d_vertex_buffer);
			if (result >= 0)
			{
				success = TRUE;
#ifdef HALO_ANDROID
				xgpu_static_buffer_add(d3d_vertex_buffer, (unsigned long)buffer_size, 0);
#endif
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					result,
					"IDirect3DVertexBuffer8_Unlock(d3d_vertex_buffer)");
			}

			vertex_buffer->count = count;
			vertex_buffer->offset = 0;
			vertex_buffer->base_address = (void *)vertices;
			vertex_buffer->type = (short)vertex_type;
			vertex_buffer->hardware_format = d3d_vertex_buffer;
		}
	}

	if (!success)
	{
		csmemset(vertex_buffer, 0, sizeof(*vertex_buffer));
		error(_error_silent, "### ERROR failed to create vertex buffer hardware format");
	}

	return success;
}

void rasterizer_vertex_buffer_delete(
	struct vertex_buffer *vertex_buffer)
{
	if (vertex_buffer && vertex_buffer->hardware_format)
	{
		IDirect3DVertexBuffer8_Release(
			(D3DVertexBuffer *)vertex_buffer->hardware_format);
		vertex_buffer->hardware_format = 0;
	}

	return;
}

boolean rasterizer_triangle_buffer_new(
	struct triangle_buffer *triangle_buffer,
	short triangle_type,
	long count,
	void const *triangles)
{
	D3DIndexBuffer *d3d_index_buffer = NULL;
	byte *locked_triangles;
	boolean success;
	long buffer_size;
	long result;

	success = TRUE;
	buffer_size = 0;
	match_assert("c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c", 115, triangle_buffer);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c", 116, triangles);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c", 117, count>0);

	switch (triangle_type)
	{
	case 0:
		buffer_size = 6 * count;
		break;
	case 1:
		buffer_size = 2 * count + 4;
		break;
	default:
		display_assert(
			"### ERROR unsupported triangle buffer type",
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_hardware_geometry.c",
			128,
			TRUE);
		system_exit(-1);
		break;
	}

	if (!global_d3d_device)
	{
		success = FALSE;
		csmemset(triangle_buffer, 0, sizeof(*triangle_buffer));
		error(_error_silent, "### ERROR failed to create triangle buffer hardware format");
	}
	else
	{
		if (success)
		{
			result = IDirect3DDevice8_CreateIndexBuffer(
				global_d3d_device,
				buffer_size,
				D3DUSAGE_WRITEONLY,
				D3DFMT_INDEX16,
				D3DPOOL_MANAGED,
				&d3d_index_buffer);
			if (result >= 0)
			{
				success = TRUE;
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					result,
					"IDirect3DDevice8_CreateIndexBuffer(global_d3d_device, buffer_size, RASTERIZER_STATIC_BUFFER_USAGE, D3DFMT_INDEX16, RASTERIZER_STATIC_BUFFER_POOL, &d3d_index_buffer)");
			}
			if (!d3d_index_buffer)
				success = FALSE;
			if (!success)
				d3d_index_buffer = NULL;
		}

		if (success)
		{
			IDirect3DIndexBuffer8_Lock(
				d3d_index_buffer,
				0,
				buffer_size,
				&locked_triangles,
				0);
			if (!locked_triangles)
				success = FALSE;
			if (!success)
				locked_triangles = NULL;
		}

		if (success)
		{
			csmemcpy(locked_triangles, triangles, buffer_size);
			result = IDirect3DIndexBuffer8_Unlock(d3d_index_buffer);
			if (result >= 0)
			{
				success = TRUE;
#ifdef HALO_ANDROID
				xgpu_static_buffer_add(d3d_index_buffer, (unsigned long)buffer_size, 1);
#endif
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					result,
					"IDirect3DIndexBuffer8_Unlock(d3d_index_buffer)");
			}

			triangle_buffer->type = triangle_type;
			triangle_buffer->count = count;
			triangle_buffer->base_address = (void *)triangles;
			triangle_buffer->hardware_format = d3d_index_buffer;
		}
		else
		{
			csmemset(triangle_buffer, 0, sizeof(*triangle_buffer));
			error(_error_silent, "### ERROR failed to create triangle buffer hardware format");
		}
	}

	return success;
}

void rasterizer_triangle_buffer_delete(
	struct triangle_buffer *triangle_buffer)
{
	if (triangle_buffer && triangle_buffer->hardware_format)
	{
		IDirect3DIndexBuffer8_Release(
			(D3DIndexBuffer *)triangle_buffer->hardware_format);
		triangle_buffer->hardware_format = 0;
	}

	return;
}

/* ---------- private code */
