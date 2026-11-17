/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "main/main.h"
#include "rasterizer.h"
#include "rasterizer_console_vars.h"
#include "rasterizer_geometry.h"
#include "rasterizer_xbox_draw_primitives.h"
/* The January object retains out-of-line copies of the D3D inline wrappers.
 * The stock XDK definition of D3DINLINE (static __forceinline) reproduces all
 * of them; do not replace them with handwritten Microsoft dispatchers, take
 * their address or weaken __forceinline, as any of those changes the emitted
 * ABI and code shape.
 *
 * Six of them originally carried the split's address names, because their
 * bodies come in identical pairs and so cannot be told apart by bytes alone.
 * Their recovered names follow COMDAT emission order, anchored on the three
 * neighbouring wrappers that can be identified uniquely:
 *
 * code_0014e180 = D3DVertexBuffer_Unlock
 * code_0014e190 = IDirect3DVertexBuffer8_Release
 *                 (IDirect3DVertexBuffer8_Lock, named)
 * code_0014e1c0 = IDirect3DVertexBuffer8_Unlock
 *                 (D3DIndexBuffer_Lock, named)
 * code_0014e1e0 = D3DIndexBuffer_Unlock
 * code_0014e1f0 = IDirect3DIndexBuffer8_Release
 *                 (IDirect3DIndexBuffer8_Lock, named)
 * code_0014e220 = IDirect3DIndexBuffer8_Unlock
 */
#include <xtl.h>
#include "rasterizer_xbox.h"

/* ---------- constants */

enum
{
	RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND = 10000,
	RASTERIZER_MAXIMUM_DYNAMIC_DEBUG_VERTICES = 24576,
};

enum
{
	RASTERIZER_DYNAMIC_BUFFER_USAGE = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
	RASTERIZER_DYNAMIC_BUFFER_POOL = D3DPOOL_DEFAULT,
};

enum
{
	_rasterizer_stats_geometry = 2,
};

#define RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE \
	"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_draw_primitives.c"

/* ---------- macros */

/* ---------- structures */

struct rasterizer_triangle
{
	short vertex_indices[NUMBER_OF_VERTICES_PER_TRIANGLE];
};

struct dynamic_vertex_group
{
	long vertex_count;
	long maximum_vertex_count;
	long total_vertex_count;
	D3DVertexBuffer *d3d_vertex_buffer;
	boolean first_lock;
	byte pad11[3];
};

struct dynamic_vertex_buffer
{
	short type;
	word pad02;
	long vertex_start_index;
	long vertex_count;
	byte *vertices;
};

struct dynamic_vertices_globals
{
	struct dynamic_vertex_group groups[NUMBER_OF_RASTERIZER_VERTEX_TYPES];
	struct dynamic_vertex_buffer buffers[RASTERIZER_MAXIMUM_DYNAMIC_VERTEX_BUFFERS];
	long buffer_count;
};

struct dynamic_triangle_buffer
{
	long triangle_start_index;
	long triangle_count;
	short *triangles;
};

struct dynamic_triangles_globals
{
	struct dynamic_triangle_buffer buffers[RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLE_BUFFERS];
	long buffer_count;
	long triangle_count;
	D3DIndexBuffer *d3d_index_buffer;
	boolean first_lock;
	byte pad300d[3];
};

typedef char dynamic_vertex_group_size_assert[
	sizeof(struct dynamic_vertex_group) == 0x14 ? 1 : -1];
typedef char dynamic_vertex_buffer_size_assert[
	sizeof(struct dynamic_vertex_buffer) == 0x10 ? 1 : -1];
typedef char dynamic_triangle_buffer_size_assert[
	sizeof(struct dynamic_triangle_buffer) == 0xC ? 1 : -1];
typedef char rasterizer_triangle_size_assert[
	sizeof(struct rasterizer_triangle) == 0x6 ? 1 : -1];

/* ---------- prototypes */

/* (rasterizer_xbox.c) */
void rasterizer_model_part_skinning(struct vertex_buffer const *vertex_buffer);

static D3DVertexBuffer *dynamic_vertex_group_get_d3d_vertex_buffer(
	struct dynamic_vertex_group const *group);
static void draw_primitives_data_error(
	char const *problem);
static boolean draw_primitives_vertex_buffer_valid(
	struct vertex_buffer const *vertex_buffer);
static boolean draw_primitives_triangle_buffer_valid(
	struct triangle_buffer const *triangle_buffer,
	long *triangle_count);
static boolean draw_primitives_dynamic_triangle_count_valid(
	struct dynamic_triangle_buffer const *dynamic_triangle_buffer,
	long first_triangle_index,
	long *triangle_count);

/* ---------- globals */

static D3DPRIMITIVETYPE const d3d_primitive_type_table[NUMBER_OF_TRIANGLE_BUFFER_TYPES] =
{
	D3DPT_TRIANGLELIST,
	D3DPT_TRIANGLESTRIP
};


static struct dynamic_vertices_globals dynamic_vertices = {0};
static struct dynamic_triangles_globals dynamic_triangles = {0};
static D3DVertexBuffer *aux_dynamic_unlit_vb = NULL;
static boolean dynamic_triangles_overflow_warning = FALSE;
static boolean dynamic_vertices_overflow_warning = FALSE;


/* ---------- public code */

boolean rasterizer_dynamic_geometry_initialize(
	void)
{
	boolean success;
	long result;
	short vertex_type;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		93,
		global_d3d_device);

	result = IDirect3DDevice8_CreateIndexBuffer(
		global_d3d_device,
		sizeof(struct rasterizer_triangle)*RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLES,
		RASTERIZER_DYNAMIC_BUFFER_USAGE,
		D3DFMT_INDEX16,
		RASTERIZER_DYNAMIC_BUFFER_POOL,
		&dynamic_triangles.d3d_index_buffer);
	if (result>=0)
	{
		success = TRUE;
	}
	else
	{
		success = FALSE;
		rasterizer_error(
			result,
			"IDirect3DDevice8_CreateIndexBuffer(global_d3d_device, sizeof(struct rasterizer_triangle)*RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLES, RASTERIZER_DYNAMIC_BUFFER_USAGE, D3DFMT_INDEX16, RASTERIZER_DYNAMIC_BUFFER_POOL, &dynamic_triangles.d3d_index_buffer)");
	}
	if (!dynamic_triangles.d3d_index_buffer)
	{
		success = FALSE;
	}
	if (!success)
	{
		dynamic_triangles.d3d_index_buffer = NULL;
		error(_error_silent, "### ERROR failed to create dynamic triangle buffer");
	}

	for (vertex_type = 0;
		success && vertex_type<NUMBER_OF_RASTERIZER_VERTEX_TYPES;
		vertex_type++)
	{
		struct dynamic_vertex_group *group = &dynamic_vertices.groups[vertex_type];
		long count;

		switch (vertex_type)
		{
			case _rasterizer_vertex_type_dynamic_unlit:
				count = RASTERIZER_MAXIMUM_DYNAMIC_UNLIT_VERTICES;
				break;

			case _rasterizer_vertex_type_dynamic_lit:
			case _rasterizer_vertex_type_dynamic_screen:
				count = 0;
				break;

			case _rasterizer_vertex_type_debug:
				count = RASTERIZER_MAXIMUM_DYNAMIC_DEBUG_VERTICES;
				break;

			case _rasterizer_vertex_type_model_compressed:
				count = RASTERIZER_MAXIMUM_DYNAMIC_MODEL_VERTICES;
				break;

			default:
				count = 0;
				break;
		}

		if (count>0)
		{
			result = IDirect3DDevice8_CreateVertexBuffer(
				global_d3d_device,
				rasterizer_geometry_get_vertex_size(vertex_type)*count,
				RASTERIZER_DYNAMIC_BUFFER_USAGE,
				0,
				RASTERIZER_DYNAMIC_BUFFER_POOL,
				&group->d3d_vertex_buffer);
			if (success && result>=0)
			{
				success = TRUE;
			}
			else
			{
				success = FALSE;
				rasterizer_error(
					result,
					"IDirect3DDevice8_CreateVertexBuffer(global_d3d_device, rasterizer_geometry_get_vertex_size(vertex_type)*count, RASTERIZER_DYNAMIC_BUFFER_USAGE, 0, RASTERIZER_DYNAMIC_BUFFER_POOL, &group->d3d_vertex_buffer)");
			}
			if (!group->d3d_vertex_buffer)
			{
				success = FALSE;
			}
			if (!success)
			{
				group->d3d_vertex_buffer = NULL;
				error(_error_silent, "### ERROR failed to create dynamic vertex buffer");
			}
		}
		else
		{
			group->d3d_vertex_buffer = NULL;
		}

		group->maximum_vertex_count = count;
		group->total_vertex_count = count;
	}

	if (success)
	{
		result = IDirect3DDevice8_CreateVertexBuffer(
			global_d3d_device,
			rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_dynamic_unlit)*
				RASTERIZER_MAXIMUM_DYNAMIC_UNLIT_VERTICES,
			RASTERIZER_DYNAMIC_BUFFER_USAGE,
			0,
			RASTERIZER_DYNAMIC_BUFFER_POOL,
			&aux_dynamic_unlit_vb);
		if (result>=0)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				result,
				"IDirect3DDevice8_CreateVertexBuffer(global_d3d_device, rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_dynamic_unlit)*RASTERIZER_MAXIMUM_DYNAMIC_UNLIT_VERTICES, RASTERIZER_DYNAMIC_BUFFER_USAGE, 0, RASTERIZER_DYNAMIC_BUFFER_POOL, &aux_dynamic_unlit_vb)");
		}
		if (!aux_dynamic_unlit_vb)
		{
			success = FALSE;
		}
		if (!success)
		{
			aux_dynamic_unlit_vb = NULL;
		}
	}

	if (!success)
	{
		error(_error_silent, "### ERROR failed to initialize rasterizer dynamic geometry");
	}

	return success;
}

void rasterizer_dynamic_geometry_begin(
	void)
{
	long vertex_type;

	if (rasterizer_debug_options.splitscreen_VB_optimization_enabled)
	{
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			198,
			global_window_parameters.window_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			199,
			global_window_parameters.window_index<main_get_window_count());

		for (vertex_type = 0; vertex_type<NUMBER_OF_RASTERIZER_VERTEX_TYPES; vertex_type++)
		{
			if (global_window_parameters.window_index==0)
			{
				dynamic_vertices.groups[vertex_type].vertex_count = 0;
				dynamic_vertices.groups[vertex_type].first_lock = TRUE;
			}
			else
			{
				dynamic_vertices.groups[vertex_type].maximum_vertex_count =
					(global_window_parameters.window_index+1)*
						dynamic_vertices.groups[vertex_type].total_vertex_count/
						main_get_window_count();
			}
		}

		dynamic_triangles.triangle_count = 0;
		dynamic_triangles.first_lock = TRUE;
		if (global_window_parameters.window_index==0)
		{
			dynamic_vertices.buffer_count = 0;
		}
		dynamic_triangles.buffer_count = 0;
	}
	else
	{
		for (vertex_type = 0; vertex_type<NUMBER_OF_RASTERIZER_VERTEX_TYPES; vertex_type++)
		{
			dynamic_vertices.groups[vertex_type].vertex_count = 0;
			dynamic_vertices.groups[vertex_type].first_lock = TRUE;
		}

		dynamic_triangles.triangle_count = 0;
		dynamic_triangles.first_lock = TRUE;
		dynamic_vertices.buffer_count = 0;
		dynamic_triangles.buffer_count = 0;
	}

	return;
}

void rasterizer_dynamic_geometry_end(
	void)
{
	return;
}

void rasterizer_dynamic_geometry_dispose(
	void)
{
	struct dynamic_vertex_group *group;
	long vertex_type;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		263,
		global_d3d_device);

	for (vertex_type = 0, group = dynamic_vertices.groups;
		vertex_type<NUMBER_OF_RASTERIZER_VERTEX_TYPES;
		vertex_type++, group++)
	{
		if (group->d3d_vertex_buffer)
		{
			IDirect3DVertexBuffer8_Release(group->d3d_vertex_buffer);
			group->d3d_vertex_buffer = NULL;
		}
	}

	if (aux_dynamic_unlit_vb)
	{
		IDirect3DVertexBuffer8_Release(aux_dynamic_unlit_vb);
		aux_dynamic_unlit_vb = NULL;
	}

	if (dynamic_triangles.d3d_index_buffer)
	{
		IDirect3DIndexBuffer8_Release(dynamic_triangles.d3d_index_buffer);
		dynamic_triangles.d3d_index_buffer = NULL;
	}

	return;
}

long _rasterizer_dynamic_triangles_new(
	long count)
{
	long dynamic_triangle_buffer_index = NONE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		284,
		count>=0);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		285,
		dynamic_triangles.d3d_index_buffer);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		286,
		global_d3d_device);

	if (count>0)
	{
		if (dynamic_triangles.triangle_count<RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLES-count &&
			dynamic_triangles.buffer_count<RASTERIZER_MAXIMUM_DYNAMIC_TRIANGLE_BUFFERS-1)
		{
			struct dynamic_triangle_buffer *dynamic_triangle_buffer;

			dynamic_triangle_buffer_index = dynamic_triangles.buffer_count;
			dynamic_triangle_buffer =
				&dynamic_triangles.buffers[dynamic_triangle_buffer_index];
			dynamic_triangle_buffer->triangle_start_index = dynamic_triangles.triangle_count;
			dynamic_triangle_buffer->triangle_count = count;
			dynamic_triangles.triangle_count += count;
			dynamic_triangles.buffer_count++;

			if (rasterizer_debug_options.statistics_mode==_rasterizer_stats_geometry)
			{
				rasterizer_frame_statistics.dynamic_triangle_count += count;
				rasterizer_frame_statistics.dynamic_triangle_buffer_count++;
			}
		}
		else if (!dynamic_triangles_overflow_warning)
		{
			error(_error_silent, "### ERROR too many dynamic triangles requested from rasterizer");
			dynamic_triangles_overflow_warning = TRUE;
		}
	}

	return dynamic_triangle_buffer_index;
}

short *_rasterizer_dynamic_triangles_lock(
	long dynamic_triangle_buffer_index)
{
	short *triangles = NULL;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		331,
		global_d3d_device);

	if (dynamic_triangle_buffer_index!=NONE)
	{
		struct dynamic_triangle_buffer *dynamic_triangle_buffer;

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			337,
			dynamic_triangle_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			338,
			dynamic_triangle_buffer_index<dynamic_triangles.buffer_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			340,
			dynamic_triangles.d3d_index_buffer);

		dynamic_triangle_buffer = &dynamic_triangles.buffers[dynamic_triangle_buffer_index];

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			344,
			dynamic_triangle_buffer->triangle_count>0);

		IDirect3DIndexBuffer8_Lock(
			dynamic_triangles.d3d_index_buffer,
			sizeof(struct rasterizer_triangle)*dynamic_triangle_buffer->triangle_start_index,
			sizeof(struct rasterizer_triangle)*dynamic_triangle_buffer->triangle_count,
			(byte **)&dynamic_triangle_buffer->triangles,
			dynamic_triangles.first_lock ? 0 : D3DLOCK_READONLY);
		dynamic_triangles.first_lock = FALSE;

		triangles = dynamic_triangle_buffer->triangles;
	}
	else
	{
		error(_error_silent, "### WARNING tried to lock dynamic triangles with index=NONE");
	}

	return triangles;
}

void _rasterizer_dynamic_triangles_unlock(
	long dynamic_triangle_buffer_index)
{
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		373,
		global_d3d_device);

	if (dynamic_triangle_buffer_index!=NONE)
	{
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			377,
			dynamic_triangle_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			378,
			dynamic_triangle_buffer_index<dynamic_triangles.buffer_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			380,
			dynamic_triangles.d3d_index_buffer);

		IDirect3DIndexBuffer8_Unlock(dynamic_triangles.d3d_index_buffer);
	}
	else
	{
		error(_error_silent, "### WARNING tried to unlock dynamic triangles with index=NONE");
	}

	return;
}

void _rasterizer_dynamic_triangles_delete(
	long triangle_buffer_index)
{
	return;
}

long _rasterizer_dynamic_vertices_new(
	short type,
	long count)
{
	long dynamic_vertex_buffer_index = NONE;
	struct dynamic_vertex_group *group;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		426,
		count>=0);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		427,
		type>=0 && type<NUMBER_OF_RASTERIZER_VERTEX_TYPES);

	group = &dynamic_vertices.groups[type];

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		429,
		dynamic_vertices.groups[type].d3d_vertex_buffer);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		430,
		global_d3d_device);

	if (count>0)
	{
		if (group->vertex_count<group->maximum_vertex_count-count &&
			dynamic_vertices.buffer_count<RASTERIZER_MAXIMUM_DYNAMIC_VERTEX_BUFFERS-1)
		{
			struct dynamic_vertex_buffer *dynamic_vertex_buffer;
			long vertex_size = rasterizer_geometry_get_vertex_size(type);

			dynamic_vertex_buffer_index = dynamic_vertices.buffer_count;
			dynamic_vertex_buffer = &dynamic_vertices.buffers[dynamic_vertices.buffer_count];
			dynamic_vertex_buffer->type = type;
			dynamic_vertex_buffer->vertex_start_index = group->vertex_count;
			dynamic_vertex_buffer->vertex_count = count;
			group->vertex_count += count;
			dynamic_vertices.buffer_count++;

			if (rasterizer_debug_options.statistics_mode==_rasterizer_stats_geometry)
			{
				rasterizer_frame_statistics.dynamic_vertex_count += count;
				rasterizer_frame_statistics.dynamic_vertex_buffer_count++;
			}
		}
		else if (!dynamic_vertices_overflow_warning)
		{
			error(_error_silent, "### ERROR too many dynamic vertices requested from rasterizer");
			dynamic_vertices_overflow_warning = TRUE;
		}
	}

	return dynamic_vertex_buffer_index;
}

short _rasterizer_dynamic_vertices_get_type(
	long dynamic_vertex_buffer_index)
{
	short type = NONE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		478,
		global_d3d_device);

	if (dynamic_vertex_buffer_index!=NONE)
	{
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			484,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			485,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);

		type = dynamic_vertices.buffers[dynamic_vertex_buffer_index].type;
	}
	else
	{
		error(_error_silent, "### WARNING tried to query dynamic vertices with index=NONE");
	}

	return type;
}

void *_rasterizer_dynamic_vertices_lock(
	long dynamic_vertex_buffer_index)
{
	void *vertices = NULL;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		519,
		global_d3d_device);

	if (!rasterizer_globals.current_lock_operation)
	{
		error(_error_silent, "### WARNING: tried to lock dynamic vertices without specifying a lock operation");
	}

	if (dynamic_vertex_buffer_index!=NONE)
	{
		struct dynamic_vertex_buffer *dynamic_vertex_buffer;
		struct dynamic_vertex_group *group;
		D3DVertexBuffer *d3d_vertex_buffer;
		long vertex_size;

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			535,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			536,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			539,
			dynamic_triangles.d3d_index_buffer);

		dynamic_vertex_buffer = &dynamic_vertices.buffers[dynamic_vertex_buffer_index];
		vertex_size = rasterizer_geometry_get_vertex_size(dynamic_vertex_buffer->type);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			544,
			dynamic_vertex_buffer->type>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			545,
			dynamic_vertex_buffer->type<NUMBER_OF_RASTERIZER_VERTEX_TYPES);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			546,
			dynamic_vertex_buffer->vertex_count>0);

		group = &dynamic_vertices.groups[dynamic_vertex_buffer->type];
		d3d_vertex_buffer = dynamic_vertex_group_get_d3d_vertex_buffer(group);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			551,
			d3d_vertex_buffer);

		IDirect3DVertexBuffer8_Lock(
			d3d_vertex_buffer,
			vertex_size*dynamic_vertex_buffer->vertex_start_index,
			vertex_size*dynamic_vertex_buffer->vertex_count,
			&dynamic_vertex_buffer->vertices,
			group->first_lock ? 0 : D3DLOCK_READONLY);
		group->first_lock = FALSE;

		vertices = dynamic_vertex_buffer->vertices;
	}
	else
	{
		error(_error_silent, "### WARNING tried to lock dynamic vertices with index=NONE");
	}

	return vertices;
}

void _rasterizer_dynamic_vertices_unlock(
	long dynamic_vertex_buffer_index)
{
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		582,
		global_d3d_device);

	if (dynamic_vertex_buffer_index!=NONE)
	{
		struct dynamic_vertex_buffer *buffer;
		D3DVertexBuffer *d3d_vertex_buffer;

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			588,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			589,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			591,
			dynamic_triangles.d3d_index_buffer);

		buffer = &dynamic_vertices.buffers[dynamic_vertex_buffer_index];

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			595,
			buffer->type>=0 && buffer->type<NUMBER_OF_RASTERIZER_VERTEX_TYPES);

		d3d_vertex_buffer = dynamic_vertex_group_get_d3d_vertex_buffer(
			&dynamic_vertices.groups[buffer->type]);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			600,
			d3d_vertex_buffer);

		IDirect3DVertexBuffer8_Unlock(d3d_vertex_buffer);
	}
	else
	{
		error(_error_silent, "### WARNING tried to unlock dynamic vertices with index=NONE");
	}

	return;
}

void _rasterizer_dynamic_vertices_delete(
	long dynamic_vertex_buffer_index)
{
	return;
}

void rasterizer_draw_dynamic_vertices(
	long first_primitive_index,
	long primitive_count,
	long dynamic_vertex_buffer_index,
	short vertices_per_primitive)
{
	boolean success = TRUE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		651,
		global_d3d_device);

	while (primitive_count>0)
	{
		struct dynamic_vertex_buffer *dynamic_vertex_buffer;
		struct dynamic_vertex_group *group;
		D3DVertexBuffer *d3d_vertex_buffer;
		D3DPRIMITIVETYPE d3d_primitive_type;
		long vertex_size;
		long local_primitive_count;

		if (dynamic_vertex_buffer_index==NONE)
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			666,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			667,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);

		switch (vertices_per_primitive)
		{
			case NUMBER_OF_VERTICES_PER_LINE:
				d3d_primitive_type = D3DPT_LINELIST;
				break;

			case NUMBER_OF_VERTICES_PER_TRIANGLE:
				d3d_primitive_type = D3DPT_TRIANGLELIST;
				break;

			case NUMBER_OF_VERTICES_PER_QUADRILATERAL:
				d3d_primitive_type = D3DPT_QUADLIST;
				break;

			default:
				match_assert(
					RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
					682,
					primitive_count==1);

				primitive_count = vertices_per_primitive-2;

				match_assert(
					RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
					686,
					first_primitive_index==0);
				match_assert(
					RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
					687,
					vertices_per_primitive<=RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND);

				d3d_primitive_type = D3DPT_TRIANGLESTRIP;
				break;
		}

		dynamic_vertex_buffer = &dynamic_vertices.buffers[dynamic_vertex_buffer_index];
		vertex_size = rasterizer_geometry_get_vertex_size(dynamic_vertex_buffer->type);
		group = &dynamic_vertices.groups[dynamic_vertex_buffer->type];
		d3d_vertex_buffer = dynamic_vertex_group_get_d3d_vertex_buffer(group);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			701,
			d3d_vertex_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			704,
			dynamic_vertex_buffer->vertex_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			705,
			dynamic_vertex_buffer->vertex_start_index<=group->vertex_count - dynamic_vertex_buffer->vertex_count);

		local_primitive_count = primitive_count;
		if (local_primitive_count>RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND)
		{
			local_primitive_count = RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND;
		}

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			d3d_vertex_buffer,
			vertex_size)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, d3d_vertex_buffer, vertex_size)");
		}

		if (IDirect3DDevice8_DrawPrimitive(
			global_d3d_device,
			d3d_primitive_type,
			first_primitive_index*vertices_per_primitive + dynamic_vertex_buffer->vertex_start_index,
			local_primitive_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawPrimitive(global_d3d_device, d3d_primitive_type, first_primitive_index*vertices_per_primitive + dynamic_vertex_buffer->vertex_start_index, local_primitive_count)");
		}

		first_primitive_index += local_primitive_count;
		primitive_count -= local_primitive_count;
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_dynamic_vertices failed");
	}

	return;
}

void rasterizer_draw_dynamic_triangles_dynamic_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	long dynamic_vertex_buffer_index)
{
	boolean success = TRUE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		741,
		global_d3d_device);

	while (triangle_count>0)
	{
		struct dynamic_triangle_buffer *dynamic_triangle_buffer;
		struct dynamic_vertex_buffer *dynamic_vertex_buffer;
		struct dynamic_vertex_group *group;
		D3DVertexBuffer *d3d_vertex_buffer;
		long vertex_size;
		long local_triangle_count;

		if (dynamic_triangle_buffer_index==NONE)
		{
			break;
		}
		if (dynamic_vertex_buffer_index==NONE)
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			756,
			dynamic_triangles.d3d_index_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			759,
			dynamic_triangle_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			760,
			dynamic_triangle_buffer_index<dynamic_triangles.buffer_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			761,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			762,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);

		dynamic_vertex_buffer = &dynamic_vertices.buffers[dynamic_vertex_buffer_index];
		dynamic_triangle_buffer = &dynamic_triangles.buffers[dynamic_triangle_buffer_index];
		vertex_size = rasterizer_geometry_get_vertex_size(dynamic_vertex_buffer->type);
		group = &dynamic_vertices.groups[dynamic_vertex_buffer->type];
		d3d_vertex_buffer = dynamic_vertex_group_get_d3d_vertex_buffer(group);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			773,
			d3d_vertex_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			776,
			dynamic_vertex_buffer->vertex_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			777,
			dynamic_vertex_buffer->vertex_start_index<=group->vertex_count - dynamic_vertex_buffer->vertex_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			780,
			dynamic_triangle_buffer->triangle_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			781,
			triangle_count>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			782,
			triangle_count<=dynamic_triangle_buffer->triangle_count - first_triangle_index);

		local_triangle_count = MIN(triangle_count, RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND);

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			d3d_vertex_buffer,
			vertex_size)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, d3d_vertex_buffer, vertex_size)");
		}

		if (IDirect3DDevice8_SetIndices(
			global_d3d_device,
			dynamic_triangles.d3d_index_buffer,
			dynamic_vertex_buffer->vertex_start_index)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetIndices(global_d3d_device, dynamic_triangles.d3d_index_buffer, dynamic_vertex_buffer->vertex_start_index)");
		}

		if (IDirect3DDevice8_DrawIndexedPrimitive(
			global_d3d_device,
			D3DPT_TRIANGLELIST,
			0,
			dynamic_vertex_buffer->vertex_count,
			NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index),
			local_triangle_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawIndexedPrimitive(global_d3d_device, D3DPT_TRIANGLELIST, 0, dynamic_vertex_buffer->vertex_count, NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index), local_triangle_count)");
		}

		first_triangle_index += local_triangle_count;
		triangle_count -= local_triangle_count;
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_dynamic_triangles_dynamic_vertices failed");
	}

	return;
}

void rasterizer_draw_dynamic_triangles_static_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	boolean success = TRUE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		823,
		global_d3d_device);
	/* port: a part of a model of many nodes, its own nodes' matrices (rasterizer_xbox.c) */
	rasterizer_model_part_skinning(vertex_buffer);

	while (triangle_count>0)
	{
		struct dynamic_triangle_buffer *dynamic_triangle_buffer;
		long vertex_size;
		long local_triangle_count;

		if (dynamic_triangle_buffer_index==NONE)
		{
			break;
		}
		if (!vertex_buffer)
		{
			break;
		}
		if (!vertex_buffer->hardware_format)
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			834,
			dynamic_triangles.d3d_index_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			837,
			dynamic_triangle_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			838,
			dynamic_triangle_buffer_index<dynamic_triangles.buffer_count);

		dynamic_triangle_buffer = &dynamic_triangles.buffers[dynamic_triangle_buffer_index];
		/* port: a vertex type the tables have (a map's), and no more triangles
		than the dynamic buffer was filled with */
		if (!draw_primitives_vertex_buffer_valid(vertex_buffer) ||
			!draw_primitives_dynamic_triangle_count_valid(dynamic_triangle_buffer, first_triangle_index, &triangle_count))
		{
			break;
		}
		vertex_size = rasterizer_geometry_get_vertex_size(vertex_buffer->type);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			845,
			dynamic_triangle_buffer->triangle_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			846,
			triangle_count>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			847,
			triangle_count<=dynamic_triangle_buffer->triangle_count - first_triangle_index);

		local_triangle_count = triangle_count;
		if (local_triangle_count>RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND)
		{
			local_triangle_count = RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND;
		}

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			(IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format,
			vertex_size)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, (IDirect3DVertexBuffer8*)vertex_buffer->hardware_format, vertex_size)");
		}

		if (IDirect3DDevice8_SetIndices(
			global_d3d_device,
			dynamic_triangles.d3d_index_buffer,
			0)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetIndices(global_d3d_device, dynamic_triangles.d3d_index_buffer, 0)");
		}

		if (IDirect3DDevice8_DrawIndexedPrimitive(
			global_d3d_device,
			D3DPT_TRIANGLELIST,
			0,
			vertex_buffer->count,
			NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index),
			local_triangle_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawIndexedPrimitive(global_d3d_device, D3DPT_TRIANGLELIST, 0, vertex_buffer->count, NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index), local_triangle_count)");
		}

		first_triangle_index += local_triangle_count;
		triangle_count -= local_triangle_count;
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_dynamic_triangles_static_vertices failed");
	}

	return;
}

void rasterizer_draw_dynamic_triangles_static_vertices2(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer0,
	struct vertex_buffer const *vertex_buffer1)
{
	boolean success = TRUE;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		890,
		global_d3d_device);
	/* port: a part of a model of many nodes, its own nodes' matrices (rasterizer_xbox.c) */
	rasterizer_model_part_skinning(vertex_buffer0);

	while (triangle_count>0)
	{
		struct dynamic_triangle_buffer *dynamic_triangle_buffer;
		long vertex_size0;
		long vertex_size1;
		long local_triangle_count;

		if (dynamic_triangle_buffer_index==NONE)
		{
			break;
		}
		if (!vertex_buffer0)
		{
			break;
		}
		if (!vertex_buffer0->hardware_format)
		{
			break;
		}
		if (!vertex_buffer1)
		{
			break;
		}
		if (!vertex_buffer1->hardware_format)
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			903,
			dynamic_triangles.d3d_index_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			906,
			dynamic_triangle_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			907,
			dynamic_triangle_buffer_index<dynamic_triangles.buffer_count);

		dynamic_triangle_buffer = &dynamic_triangles.buffers[dynamic_triangle_buffer_index];
		/* port: vertex types the tables have (a map's), and no more triangles
		than the dynamic buffer was filled with */
		if (!draw_primitives_vertex_buffer_valid(vertex_buffer0) ||
			!draw_primitives_vertex_buffer_valid(vertex_buffer1) ||
			!draw_primitives_dynamic_triangle_count_valid(dynamic_triangle_buffer, first_triangle_index, &triangle_count))
		{
			break;
		}
		vertex_size0 = rasterizer_geometry_get_vertex_size(vertex_buffer0->type);
		vertex_size1 = rasterizer_geometry_get_vertex_size(vertex_buffer1->type);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			915,
			dynamic_triangle_buffer->triangle_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			916,
			triangle_count>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			917,
			triangle_count<=dynamic_triangle_buffer->triangle_count - first_triangle_index);

		local_triangle_count = MIN(triangle_count, RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND);

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			(IDirect3DVertexBuffer8 *)vertex_buffer0->hardware_format,
			vertex_size0)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, (IDirect3DVertexBuffer8*)vertex_buffer0->hardware_format, vertex_size0)");
		}

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			1,
			(IDirect3DVertexBuffer8 *)vertex_buffer1->hardware_format,
			vertex_size1)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 1, (IDirect3DVertexBuffer8*)vertex_buffer1->hardware_format, vertex_size1)");
		}

		if (IDirect3DDevice8_SetIndices(
			global_d3d_device,
			dynamic_triangles.d3d_index_buffer,
			0)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetIndices(global_d3d_device, dynamic_triangles.d3d_index_buffer, 0)");
		}

		if (IDirect3DDevice8_DrawIndexedPrimitive(
			global_d3d_device,
			D3DPT_TRIANGLELIST,
			0,
			vertex_buffer0->count,
			NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index),
			local_triangle_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawIndexedPrimitive(global_d3d_device, D3DPT_TRIANGLELIST, 0, vertex_buffer0->count, NUMBER_OF_VERTICES_PER_TRIANGLE*(first_triangle_index + dynamic_triangle_buffer->triangle_start_index), local_triangle_count)");
		}

		first_triangle_index += local_triangle_count;
		triangle_count -= local_triangle_count;
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_dynamic_triangles_static_vertices2 failed");
	}

	return;
}

void rasterizer_draw_static_triangles_dynamic_vertices(
	struct triangle_buffer const *triangle_buffer,
	long first_triangle_index,
	long triangle_count,
	long dynamic_vertex_buffer_index)
{
	boolean success = TRUE;
	long local_triangle_vertex_indices_offset = 0;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		963,
		global_d3d_device);

	while (triangle_count>0)
	{
		struct dynamic_vertex_buffer *dynamic_vertex_buffer;
		struct dynamic_vertex_group *group;
		D3DVertexBuffer *d3d_vertex_buffer;
		long vertex_size;
		long local_triangle_count;

		if (!triangle_buffer)
		{
			break;
		}
		if (!triangle_buffer->hardware_format)
		{
			break;
		}
		if (dynamic_vertex_buffer_index==NONE)
		{
			break;
		}
		/* port: a triangle buffer type the table has, and no more triangles
		than the buffer has (a map's) */
		if (!draw_primitives_triangle_buffer_valid(triangle_buffer, &triangle_count))
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			977,
			dynamic_triangles.d3d_index_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			980,
			dynamic_vertex_buffer_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			981,
			dynamic_vertex_buffer_index<dynamic_vertices.buffer_count);

		dynamic_vertex_buffer = &dynamic_vertices.buffers[dynamic_vertex_buffer_index];
		vertex_size = rasterizer_geometry_get_vertex_size(dynamic_vertex_buffer->type);
		group = &dynamic_vertices.groups[dynamic_vertex_buffer->type];
		d3d_vertex_buffer = dynamic_vertex_group_get_d3d_vertex_buffer(group);

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			991,
			d3d_vertex_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			994,
			dynamic_vertex_buffer->vertex_start_index>=0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			995,
			dynamic_vertex_buffer->vertex_start_index<=group->vertex_count - dynamic_vertex_buffer->vertex_count);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			998,
			first_triangle_index==0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			999,
			triangle_buffer->type>=0 && triangle_buffer->type<NUMBER_OF_TRIANGLE_BUFFER_TYPES);

		local_triangle_count = triangle_count;
		if (local_triangle_count>RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND)
		{
			local_triangle_count = RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND;
		}

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			d3d_vertex_buffer,
			vertex_size)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, d3d_vertex_buffer, vertex_size)");
		}

		if (IDirect3DDevice8_SetIndices(
			global_d3d_device,
			(IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format,
			dynamic_vertex_buffer->vertex_start_index)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetIndices(global_d3d_device, (IDirect3DIndexBuffer8*)triangle_buffer->hardware_format, dynamic_vertex_buffer->vertex_start_index)");
		}

		if (IDirect3DDevice8_DrawIndexedPrimitive(
			global_d3d_device,
			d3d_primitive_type_table[triangle_buffer->type],
			0,
			dynamic_vertex_buffer->vertex_count,
			local_triangle_vertex_indices_offset,
			local_triangle_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawIndexedPrimitive(global_d3d_device, d3d_primitive_type_table[triangle_buffer->type], 0, dynamic_vertex_buffer->vertex_count, local_triangle_vertex_indices_offset, local_triangle_count)");
		}

		triangle_count -= local_triangle_count;

		switch (triangle_buffer->type)
		{
			case _triangle_buffer_type_triangles:
				local_triangle_vertex_indices_offset +=
					NUMBER_OF_VERTICES_PER_TRIANGLE*local_triangle_count;
				break;

			case _triangle_buffer_type_precompiled_strip:
				local_triangle_vertex_indices_offset += local_triangle_count;
				break;

			default:
				match_vassert(
					RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
					1030,
					FALSE,
					"### ERROR unsupported triangle buffer type");
				break;
		}
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_static_triangles_dynamic_vertices failed");
	}

	return;
}

void rasterizer_draw_static_triangles_static_vertices(
	struct triangle_buffer const *triangle_buffer,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer)
{
	boolean success = TRUE;
	long local_triangle_vertex_indices_offset = 0;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		1063,
		global_d3d_device);
	/* port: a part of a model of many nodes, its own nodes' matrices (rasterizer_xbox.c) */
	rasterizer_model_part_skinning(vertex_buffer);

	while (triangle_count>0)
	{
		long vertex_size;
		long local_triangle_count;

		if (!triangle_buffer)
		{
			break;
		}
		if (!triangle_buffer->hardware_format)
		{
			break;
		}
		if (!vertex_buffer)
		{
			break;
		}
		if (!vertex_buffer->hardware_format)
		{
			break;
		}
		/* port: a triangle buffer and vertex type the tables have, and no
		more triangles than the buffer has (a map's) */
		if (!draw_primitives_vertex_buffer_valid(vertex_buffer) ||
			!draw_primitives_triangle_buffer_valid(triangle_buffer, &triangle_count))
		{
			break;
		}

		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			1073,
			dynamic_triangles.d3d_index_buffer);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			1078,
			first_triangle_index==0);
		match_assert(
			RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
			1079,
			triangle_buffer->type>=0 && triangle_buffer->type<NUMBER_OF_TRIANGLE_BUFFER_TYPES);

		vertex_size = rasterizer_geometry_get_vertex_size(vertex_buffer->type);

		local_triangle_count = triangle_count;
		if (local_triangle_count>RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND)
		{
			local_triangle_count = RASTERIZER_MAXIMUM_PRIMITIVES_PER_DRAW_COMMAND;
		}

		if (IDirect3DDevice8_SetStreamSource(
			global_d3d_device,
			0,
			(IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format,
			vertex_size)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetStreamSource(global_d3d_device, 0, (IDirect3DVertexBuffer8*)vertex_buffer->hardware_format, vertex_size)");
		}

		if (IDirect3DDevice8_SetIndices(
			global_d3d_device,
			(IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format,
			0)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_SetIndices(global_d3d_device, (IDirect3DIndexBuffer8*)triangle_buffer->hardware_format, 0)");
		}

		if (IDirect3DDevice8_DrawIndexedPrimitive(
			global_d3d_device,
			d3d_primitive_type_table[triangle_buffer->type],
			0,
			vertex_buffer->count,
			local_triangle_vertex_indices_offset,
			local_triangle_count)>=0 && success)
		{
			success = TRUE;
		}
		else
		{
			success = FALSE;
			rasterizer_error(
				0,
				"IDirect3DDevice8_DrawIndexedPrimitive(global_d3d_device, d3d_primitive_type_table[triangle_buffer->type], 0, vertex_buffer->count, local_triangle_vertex_indices_offset, local_triangle_count)");
		}

		triangle_count -= local_triangle_count;

		switch (triangle_buffer->type)
		{
			case _triangle_buffer_type_triangles:
				local_triangle_vertex_indices_offset +=
					NUMBER_OF_VERTICES_PER_TRIANGLE*local_triangle_count;
				break;

			case _triangle_buffer_type_precompiled_strip:
				local_triangle_vertex_indices_offset += local_triangle_count;
				break;

			default:
				match_vassert(
					RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
					1113,
					FALSE,
					"### ERROR unsupported triangle buffer type");
				break;
		}
	}

	if (!success)
	{
		error(_error_silent, "### ERROR rasterizer_draw_static_triangles_static_vertices failed");
	}

	return;
}

void rasterizer_draw(
	struct triangle_buffer const *triangle_buffer,
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count,
	struct vertex_buffer const *vertex_buffer,
	long dynamic_vertex_buffer_index)
{
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		1145,
		triangle_buffer || dynamic_triangle_buffer_index!=NONE);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		1146,
		!triangle_buffer || dynamic_triangle_buffer_index==NONE);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		1149,
		vertex_buffer || dynamic_vertex_buffer_index!=NONE);
	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		1150,
		!vertex_buffer || dynamic_vertex_buffer_index==NONE);

	if (triangle_buffer)
	{
		if (vertex_buffer)
		{
			rasterizer_draw_static_triangles_static_vertices(
				triangle_buffer,
				first_triangle_index,
				triangle_count,
				vertex_buffer);
		}
		else
		{
			rasterizer_draw_static_triangles_dynamic_vertices(
				triangle_buffer,
				first_triangle_index,
				triangle_count,
				dynamic_vertex_buffer_index);
		}
	}
	else
	{
		if (vertex_buffer)
		{
			rasterizer_draw_dynamic_triangles_static_vertices(
				dynamic_triangle_buffer_index,
				first_triangle_index,
				triangle_count,
				vertex_buffer);
		}
		else
		{
			rasterizer_draw_dynamic_triangles_dynamic_vertices(
				dynamic_triangle_buffer_index,
				first_triangle_index,
				triangle_count,
				dynamic_vertex_buffer_index);
		}
	}

	return;
}

/* ---------- private code */

/* port: a map's buffer with a type past the tables or more triangles than
it has; said once */
static void draw_primitives_data_error(
	char const *problem)
{
	static boolean reported = FALSE;

	if (!reported)
	{
		error(_error_silent, "### ERROR a draw has a bad %s; it is cut short", problem);
		reported = TRUE;
	}

	return;
}

static boolean draw_primitives_vertex_buffer_valid(
	struct vertex_buffer const *vertex_buffer)
{
	if (VALID_INDEX(vertex_buffer->type, NUMBER_OF_RASTERIZER_VERTEX_TYPES))
	{
		return TRUE;
	}
	draw_primitives_data_error("vertex buffer type");

	return FALSE;
}

/* (the static draws start at triangle 0) */
static boolean draw_primitives_triangle_buffer_valid(
	struct triangle_buffer const *triangle_buffer,
	long *triangle_count)
{
	if (!VALID_INDEX(triangle_buffer->type, NUMBER_OF_TRIANGLE_BUFFER_TYPES))
	{
		draw_primitives_data_error("triangle buffer type");
		return FALSE;
	}
	if (*triangle_count>triangle_buffer->count)
	{
		draw_primitives_data_error("triangle count");
		*triangle_count = triangle_buffer->count;
	}

	return *triangle_count>0;
}

static boolean draw_primitives_dynamic_triangle_count_valid(
	struct dynamic_triangle_buffer const *dynamic_triangle_buffer,
	long first_triangle_index,
	long *triangle_count)
{
	long available_count = dynamic_triangle_buffer->triangle_count - first_triangle_index;

	if (first_triangle_index<0 || *triangle_count>available_count)
	{
		draw_primitives_data_error("dynamic triangle count");
		*triangle_count = first_triangle_index<0 ? 0 : available_count;
	}

	return *triangle_count>0;
}

static D3DVertexBuffer *dynamic_vertex_group_get_d3d_vertex_buffer(
	struct dynamic_vertex_group const *group)
{
	D3DVertexBuffer *d3d_vertex_buffer;

	match_assert(
		RASTERIZER_XBOX_DRAW_PRIMITIVES_FILE,
		504,
		group);

	if (group==&dynamic_vertices.groups[_rasterizer_vertex_type_dynamic_unlit] &&
		TEST_FLAG(rasterizer_globals.fps_accumulation_frame_index, 0))
	{
		d3d_vertex_buffer = aux_dynamic_unlit_vb;
	}
	else
	{
		d3d_vertex_buffer = group->d3d_vertex_buffer;
	}

	return d3d_vertex_buffer;
}
