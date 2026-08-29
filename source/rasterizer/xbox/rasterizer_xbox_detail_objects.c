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
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "tag_files/tag_groups.h"
#include <xtl.h>

/* ---------- constants */

enum
{
	DETAIL_OBJECT_VERTEX_BUFFER_SIZE = 0x20000,
	MAXIMUM_DETAIL_OBJECTS_PER_FRAME = 0x1000,
};

/* ---------- macros */

#define detail_object_collection_definition_get(index) \
	((struct detail_object_collection_definition *)tag_get('dobc', (index)))

/* ---------- structures */

struct detail_objects_debug_options
{
	byte reserved00[0x24];
	boolean detail_objects;
	byte reserved25[0x2B];
	float detail_objects_offset_multiplier;
};

struct detail_objects_rasterizer_globals
{
	boolean initialized;
	byte reserved01;
	short current_lock_operation;
};

struct detail_object
{
	byte position[3];
	byte data;
	word color;
};

struct detail_object_vertex
{
	byte position[3];
	byte color[3];
	word sprite;
};

struct detail_object_type_definition
{
	byte reserved00[0x22];
	byte first_sprite_index;
	byte sprite_count;
	byte reserved24[0x3C];
};

struct detail_object_collection_definition
{
	byte reserved00[0x44];
	struct tag_block type_definitions;
	byte reserved50[0x30];
};

struct scenario_detail_object_collection_palette_entry
{
	struct tag_reference collection;
	byte reserved10[0x20];
};

struct structure_detail_object_data
{
	byte reserved00[0xC];
	struct tag_block detail_objects;
	byte reserved18[0x28];
};

struct detail_object_cell_data
{
	long first_detail_object_index;
	long detail_object_count;
	short cell_x;
	short cell_y;
	float cell_z;
	long first_vertex_index;
	void const *z_reference_vector;
};

struct detail_object_layer_data
{
	struct detail_object_cell_data *cells;
	short cell_count;
	short collection_definition_index;
};

struct detail_object_view_data
{
	struct detail_object_layer_data *layers;
	short layer_count;
	word pad;
};

struct pixel_shader_definition
{
	unsigned long alpha_inputs[8];
	unsigned long final_combiner_inputs_abcd;
	unsigned long final_combiner_inputs_efg;
	unsigned long constant_0[8];
	unsigned long constant_1[8];
	unsigned long alpha_outputs[8];
	unsigned long rgb_inputs[8];
	unsigned long compare_mode;
	unsigned long final_combiner_constant_0;
	unsigned long final_combiner_constant_1;
	unsigned long rgb_outputs[8];
	unsigned long combiner_count;
	unsigned long texture_modes;
	unsigned long dot_mapping;
	unsigned long input_texture;
	unsigned long c0_mapping;
	unsigned long c1_mapping;
	unsigned long final_combiner_constants;
};

typedef char detail_objects_enabled_offset_assert[
	offsetof(struct detail_objects_debug_options, detail_objects) == 0x24 ? 1 : -1];
typedef char detail_objects_multiplier_offset_assert[
	offsetof(struct detail_objects_debug_options, detail_objects_offset_multiplier) == 0x50 ? 1 : -1];
typedef char detail_objects_pixel_shader_size_assert[
	sizeof(struct pixel_shader_definition) == 0xF0 ? 1 : -1];
typedef char detail_object_size_assert[
	sizeof(struct detail_object) == 0x6 ? 1 : -1];
typedef char detail_object_vertex_size_assert[
	sizeof(struct detail_object_vertex) == 0x8 ? 1 : -1];
typedef char detail_object_type_definition_size_assert[
	sizeof(struct detail_object_type_definition) == 0x60 ? 1 : -1];
typedef char detail_object_collection_definition_size_assert[
	sizeof(struct detail_object_collection_definition) == 0x80 ? 1 : -1];
typedef char detail_object_palette_entry_size_assert[
	sizeof(struct scenario_detail_object_collection_palette_entry) == 0x30 ? 1 : -1];
typedef char structure_detail_object_data_size_assert[
	sizeof(struct structure_detail_object_data) == 0x40 ? 1 : -1];
typedef char detail_object_cell_data_size_assert[
	sizeof(struct detail_object_cell_data) == 0x18 ? 1 : -1];
typedef char detail_object_layer_data_size_assert[
	sizeof(struct detail_object_layer_data) == 0x8 ? 1 : -1];
typedef char detail_object_view_data_size_assert[
	sizeof(struct detail_object_view_data) == 0x8 ? 1 : -1];

/* ---------- prototypes */

void rasterizer_error(
	long error_result,
	char const *format,
	...);

void rasterizer_profile_end(
	short profile);

void rasterizer_profile_begin(
	short profile);

short main_get_window_count(
	void);

void rasterizer_set_pixel_shader(
	struct pixel_shader_definition const *definition);

/* ---------- globals */

extern D3DDevice *global_d3d_device;
extern struct detail_objects_debug_options rasterizer_debug_options;
extern struct detail_objects_rasterizer_globals rasterizer_globals;
extern struct pixel_shader_definition pixel_shader;
D3DVertexBuffer *bss_0045e904 = NULL;

#define local_d3d_vertex_buffer bss_0045e904

/* ---------- private code */

static void detail_object_build_vertices(
	long detail_object_count,
	struct detail_object_collection_definition const *collection,
	struct detail_object const *detail_objects,
	struct detail_object_vertex *vertices)
{
	struct detail_object_vertex vertex;
	struct detail_object_type_definition const *type_definition;
	long type_index;
	word color;
	byte color_low;
	byte color_high;

	if (detail_object_count > 0)
	{
		struct tag_block const *type_definitions = &collection->type_definitions;
		long remaining_count;
		struct detail_object const *detail_object = detail_objects;

		for (
			remaining_count = detail_object_count;
			remaining_count != 0;
			remaining_count--)
		{
			vertex.position[0] = detail_object->position[0];
			vertex.position[1] = detail_object->position[1];
			vertex.position[2] = detail_object->position[2];
			color = detail_object->color;
			color_high = (byte)(color >> 8);
			color_low = (byte)color;
			vertex.color[0] = (byte)((color_high & ~7) | ((color >> 13) & 7));
			vertex.color[1] = (byte)((((color >> 9) ^ (color_low >> 3)) & 3) ^ (color >> 3));
			vertex.color[2] = (byte)(((color_low >> 2) & 7) | (color_low << 3));

			type_index = (detail_object->data >> 4) % type_definitions->count;
			type_definition = TAG_BLOCK_GET_ELEMENT(
				type_definitions,
				(short)type_index,
				struct detail_object_type_definition);
			vertex.sprite = (word)(
				(type_index << 4) |
				((type_definition->first_sprite_index +
					((detail_object->data & 0xF) % type_definition->sprite_count)) << 8));

			*vertices++ = vertex;
			vertex.sprite++;
			*vertices++ = vertex;
			vertex.sprite++;
			*vertices++ = vertex;
			vertex.sprite++;
			*vertices++ = vertex;

			detail_object++;
		}
	}

	return;
}

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

void _rasterizer_detail_objects_begin(
	void)
{
	rasterizer_profile_begin(21);
	if (!rasterizer_debug_options.detail_objects)
	{
		return;
	}
	if (main_get_window_count() > 1)
	{
		return;
	}

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0x88,
		local_d3d_vertex_buffer);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0x89,
		global_d3d_device);

	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice8_SetRenderState(
		global_d3d_device,
		D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_BLENDOP, D3DBLENDOP_ADD);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ALPHATESTENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZENABLE, TRUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZWRITEENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZBIAS, 0);

	{
		float vertex_constants[24] =
		{
			0.00390625f,
			16.0f,
			541.0f,
			659.0f,
			8.0f,
			8.0f,
			8.0f,
			rasterizer_debug_options.detail_objects_offset_multiplier,
			1.0f,
			1.0f,
			0.5f,
			0.0f,
			0.0f,
			1.0f,
			-0.5f,
			0.0f,
			0.0f,
			0.0f,
			-0.5f,
			1.0f,
			1.0f,
			0.0f,
			0.5f,
			1.0f,
		};
		IDirect3DDevice8_SetVertexShaderConstant(
			global_d3d_device,
			-0x51,
			vertex_constants,
			6);
	}

	csmemset(&pixel_shader, 0, sizeof(pixel_shader));
	pixel_shader.texture_modes = 1;
	pixel_shader.combiner_count = 1;
	pixel_shader.rgb_inputs[0] = 0x08040000;
	pixel_shader.rgb_outputs[0] = 0xC0;
	pixel_shader.alpha_inputs[0] = 0x18140000;
	pixel_shader.alpha_outputs[0] = 0xC0;
	pixel_shader.final_combiner_inputs_abcd = 0xC;
	pixel_shader.final_combiner_inputs_efg = 0x1C00;
	rasterizer_set_pixel_shader(&pixel_shader);

	IDirect3DDevice8_SetStreamSource(
		global_d3d_device,
		0,
		local_d3d_vertex_buffer,
		8);

	return;
}

void _rasterizer_detail_objects_rebuild_vertices(
	struct detail_object_view_data const *detail_object_view_data)
{
	struct scenario *scenario;
	struct structure_detail_object_data *detail_object_data;
	struct detail_object *detail_objects;
	struct detail_object_layer_data *layer;
	struct detail_object_cell_data *cell;
	struct scenario_detail_object_collection_palette_entry *palette_entry;
	struct detail_object_collection_definition *collection;
	BYTE *locked_vertices;
	long vertex_index;
	long submitted_count;
	long detail_object_count;
	long layer_index;
	long cell_index;
	boolean warned;

	if (!rasterizer_debug_options.detail_objects)
	{
		return;
	}
	if (main_get_window_count() > 1)
	{
		return;
	}

	scenario = global_scenario_get();
	locked_vertices = NULL;
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0xD7,
		detail_object_view_data);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0xD8,
		local_d3d_vertex_buffer);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
		0xD9,
		global_d3d_device);

	rasterizer_globals.current_lock_operation = 3;
	IDirect3DVertexBuffer8_Lock(
		local_d3d_vertex_buffer,
		0,
		DETAIL_OBJECT_VERTEX_BUFFER_SIZE,
		&locked_vertices,
		0);
	rasterizer_globals.current_lock_operation = 0;
	if (!locked_vertices)
	{
		return;
	}
	if (global_structure_bsp_get()->detail_object_data.count)
	{
		detail_object_data = TAG_BLOCK_GET_ELEMENT(
			&global_structure_bsp_get()->detail_object_data,
			0,
			struct structure_detail_object_data);
	}
	else
	{
		detail_object_data = NULL;
	}
	detail_objects = TAG_BLOCK_GET_ELEMENT(
		&detail_object_data->detail_objects,
		0,
		struct detail_object);

	vertex_index = 0;
	submitted_count = 0;
	warned = FALSE;
	layer_index = 0;
	if (detail_object_view_data->layer_count > 0)
	{
		do
		{
			layer = &detail_object_view_data->layers[(short)layer_index];
			palette_entry = TAG_BLOCK_GET_ELEMENT(
				&scenario->detail_object_collection_palette,
				layer->collection_definition_index,
				struct scenario_detail_object_collection_palette_entry);
			collection = detail_object_collection_definition_get(
				palette_entry->collection.index);

			cell_index = 0;
			if (layer->cell_count > 0)
			{
				do
				{
					cell = &layer->cells[(short)cell_index];
					detail_object_count = cell->detail_object_count;
					if (detail_object_count > MAXIMUM_DETAIL_OBJECTS_PER_FRAME - submitted_count)
					{
						detail_object_count = MAXIMUM_DETAIL_OBJECTS_PER_FRAME - submitted_count;
					}

					detail_object_build_vertices(
						detail_object_count,
						collection,
						&detail_objects[cell->first_detail_object_index],
						&((struct detail_object_vertex *)locked_vertices)[vertex_index]);
					cell->first_vertex_index = vertex_index;
					vertex_index += cell->detail_object_count * 4;
					if (cell->detail_object_count > detail_object_count)
					{
						cell->detail_object_count = detail_object_count;
						if (!warned)
						{
							warned = TRUE;
							error(
								_error_silent,
								"### ERROR too many detail object submitted for frame (max=#%d)",
								MAXIMUM_DETAIL_OBJECTS_PER_FRAME);
						}
					}
					submitted_count += detail_object_count;
					cell_index++;
				}
				while ((short)cell_index < layer->cell_count);
			}

			layer_index++;
		}
		while ((short)layer_index < detail_object_view_data->layer_count);
	}

	return;
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
