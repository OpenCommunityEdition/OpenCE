/*
D3D12_DEVICE.C

The Direct3D 12 renderer of the Xbox Direct3D 8 device (xgpu_device.h),
Windows only (display.renderer "d3d12"): the device's state at each draw
described for the renderer's host half (d3d12_renderer.h), which records it,
as d3d8_gl.c draws it with OpenGL, for the same pictures: the same shaders
(in HLSL, nv2a_vsh.c, nv2a_psh.c), the same render targets by address, the
same mirror of the contiguous window, the same pixel rounding.

This half keeps what is the device's: the shaders made for each program and
declaration and for each pixel shader key, the input layouts, the render
targets' textures (and multisampled storage, resolved when they are read),
the mip chains of targets drawn a level at a time, the constants, the
visibility tests' areas.
*/

#include "xgpu_device.h"
#include "d3d12_renderer.h"
#include "sdl_platform.h"
#include "port_config.h"
#include "screenshot.h"
#include "zlib_prefixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* d3d12_renderer.h's values are xgpu.h's and xgpu_device.h's */
#define SAME(a, b) _Static_assert((long)(a) == (long)(b), #a)
SAME(_d3d12r_texture_cube, _xgpu_texture_cube);
SAME(_d3d12r_texture_3d, _xgpu_texture_3d);
SAME(_d3d12r_format_bgra8, _xgpu_format_bgra8);
SAME(_d3d12r_format_rgba8, _xgpu_format_rgba8);
SAME(_d3d12r_format_dxt5, _xgpu_format_dxt5);
SAME(_d3d12r_sampler_2d, _xgpu_sampler_2d);
SAME(_d3d12r_sampler_3d, _xgpu_sampler_3d);
SAME(_d3d12r_sampler_cube, _xgpu_sampler_cube);
SAME(D3D12R_MIRROR_SEGMENT_SIZE, MIRROR_SEGMENT_SIZE);
SAME(D3D12R_MIRROR_SEGMENT_COUNT, MIRROR_SEGMENT_COUNT);
SAME(D3D12R_STAGES, D3DTSS_MAXSTAGES);
SAME(D3D12R_ATTRIBUTES, XGPU_VERTEX_ATTRIBUTE_COUNT);
SAME(D3D12R_VISIBILITY_SLOTS, VISIBILITY_TEST_SLOTS);

/* port/third_party/smaa, embedded by tools/embed_assets.py (xgpu_post.c) */
extern const unsigned int xgpu_smaa_shader[];
extern const unsigned long xgpu_smaa_shader_size;
extern const unsigned int xgpu_smaa_area_texture[];
extern const unsigned long xgpu_smaa_area_texture_size;
extern const unsigned int xgpu_smaa_search_texture[];
extern const unsigned long xgpu_smaa_search_texture_size;

/* ---------- shaders */

#define SHADER_BUCKETS 1024

static unsigned long next_shader_id = 1;

static void shader_dump(const char *name, const char *source)
{
	char path[512];
	FILE *file;

	if (!xgpu_debug_settings.dump_shaders || !xgpu_debug_settings.dump_shaders[0])
		return;
	snprintf(path, sizeof(path), "%s/%s.hlsl", xgpu_debug_settings.dump_shaders, name);
	if ((file = fopen(path, "w")) != NULL)
	{
		fputs(source, file);
		fclose(file);
	}
}

/* a vertex program's shader for one way of feeding its inputs, lit for each
pixel or not; by the program's id (unique to the program, whose address a
later program can have) */
struct vertex_shader_entry
{
	struct vertex_shader_entry *next;
	unsigned long program;
	BOOL lit;
	struct nv2a_vertex_inputs inputs;
	void *code;
	unsigned long id;
};

static struct vertex_shader_entry *vertex_shader_buckets[SHADER_BUCKETS];

/* NULL if it does not compile */
static struct vertex_shader_entry *vertex_shader_get(const struct vertex_shader_object *program, BOOL lit,
	const struct nv2a_vertex_inputs *inputs)
{
	static struct vertex_shader_entry *last;
	unsigned long hash;
	struct vertex_shader_entry **bucket, *entry;

	if (last && last->program == program->id && last->lit == lit && !memcmp(&last->inputs, inputs, sizeof(*inputs)))
		return last->code ? last : NULL;
	hash = (program->id * 2654435761UL) ^ (lit ? 0x7f4a7c15UL : 0) ^ hash_words(inputs, sizeof(*inputs));
	bucket = &vertex_shader_buckets[hash % SHADER_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->program == program->id && entry->lit == lit && !memcmp(&entry->inputs, inputs, sizeof(*inputs)))
			break;
	}
	if (!entry)
	{
		char *source;

		if (!(entry = calloc(1, sizeof(*entry))))
			return NULL;
		entry->program = program->id;
		entry->lit = lit;
		entry->inputs = *inputs;
		entry->id = next_shader_id++;
		source = nv2a_vertex_shader_to_hlsl(program->instructions, program->instruction_count, inputs,
			lit ? &program->lighting : NULL);
		if (source)
		{
			char name[64];

			snprintf(name, sizeof(name), "vs%03lu_%04lx%s", program->id, entry->id, lit ? "_lit" : "");
			shader_dump(name, source);
			entry->code = d3d12r_shader_compile(source, FALSE, name);
			free(source);
		}
		entry->next = *bucket;
		*bucket = entry;
	}
	last = entry;
	return entry->code ? entry : NULL;
}

struct pixel_shader_entry
{
	struct pixel_shader_entry *next;
	unsigned long hash;
	struct nv2a_pixel_shader_key key;
	void *code;
	unsigned long id;
};

static struct pixel_shader_entry *pixel_shader_buckets[SHADER_BUCKETS];

static struct pixel_shader_entry *pixel_shader_get(const struct nv2a_pixel_shader_key *key)
{
	/* consecutive draws mostly use one of a few pixel shaders (an object's
	parts take turns) */
#define RECENT_PIXEL_SHADERS 4
	static struct pixel_shader_entry *recent[RECENT_PIXEL_SHADERS];
	static unsigned long recent_next;
	unsigned long hash, index;
	struct pixel_shader_entry **bucket, *entry;

	for (index = 0; index < RECENT_PIXEL_SHADERS; index++)
	{
		if (recent[index] && !memcmp(&recent[index]->key, key, sizeof(*key)))
			return recent[index]->code ? recent[index] : NULL;
	}
	hash = hash_words(key, sizeof(*key));
	bucket = &pixel_shader_buckets[hash % SHADER_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && !memcmp(&entry->key, key, sizeof(*key)))
			break;
	}
	if (!entry)
	{
		char *source;

		if (!(entry = calloc(1, sizeof(*entry))))
			return NULL;
		entry->hash = hash;
		entry->key = *key;
		entry->id = next_shader_id++;
		source = nv2a_pixel_shader_to_hlsl(key);
		if (source)
		{
			char name[32];

			snprintf(name, sizeof(name), "ps_%08lx", hash);
			shader_dump(name, source);
			entry->code = d3d12r_shader_compile(source, TRUE, name);
			free(source);
		}
		entry->next = *bucket;
		*bucket = entry;
	}
	recent[recent_next++ % RECENT_PIXEL_SHADERS] = entry;
	return entry->code ? entry : NULL;
}

/* ---------- input layouts

Direct3D 12 reads an element only at an offset that is a multiple of its
format's size, or of 4 for formats of 4 bytes or more. The game's detail
objects (grass) have elements of three bytes one after the other, which no
format reads where they are: a stream with such an element is copied for
the draw into one where each element starts at a multiple of 4 (its
"repacked" layout). */

struct input_layout_entry
{
	struct input_layout_entry *next;
	unsigned long hash;
	unsigned long count;
	/* the declaration's elements (the key), and as Direct3D 12 reads them */
	struct d3d12r_element declared[XGPU_VERTEX_ATTRIBUTE_COUNT];
	struct d3d12r_element elements[XGPU_VERTEX_ATTRIBUTE_COUNT];
	/* the streams repacked; each element's bytes, and each repacked
	stream's stride */
	unsigned long repacked_streams;
	unsigned char bytes[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned short repacked_strides[D3D12R_STREAMS];
	unsigned long id;
};

#define INPUT_LAYOUT_BUCKETS 256

static struct input_layout_entry *input_layout_buckets[INPUT_LAYOUT_BUCKETS];
static unsigned long next_input_layout_id = 1;

static unsigned int element_format(unsigned long type)
{
	switch (type)
	{
	case D3DVSDT_FLOAT1: return D3D12R_FORMAT_R32_FLOAT;
	case D3DVSDT_FLOAT2: return D3D12R_FORMAT_R32G32_FLOAT;
	case D3DVSDT_FLOAT3: case D3DVSDT_FLOAT2H: return D3D12R_FORMAT_R32G32B32_FLOAT;
	case D3DVSDT_FLOAT4: return D3D12R_FORMAT_R32G32B32A32_FLOAT;
	case D3DVSDT_D3DCOLOR: return D3D12R_FORMAT_B8G8R8A8_UNORM;
	case D3DVSDT_SHORT1: return D3D12R_FORMAT_R16_SINT;
	case D3DVSDT_SHORT2: return D3D12R_FORMAT_R16G16_SINT;
	/* (no format has three 16-bit components: four, w made 1, nv2a_vertex_inputs) */
	case D3DVSDT_SHORT3: case D3DVSDT_SHORT4: return D3D12R_FORMAT_R16G16B16A16_SINT;
	case D3DVSDT_NORMSHORT1: return D3D12R_FORMAT_R16_SNORM;
	case D3DVSDT_NORMSHORT2: return D3D12R_FORMAT_R16G16_SNORM;
	case D3DVSDT_NORMSHORT3: case D3DVSDT_NORMSHORT4: return D3D12R_FORMAT_R16G16B16A16_SNORM;
	case D3DVSDT_PBYTE1: return D3D12R_FORMAT_R8_UNORM;
	case D3DVSDT_PBYTE2: return D3D12R_FORMAT_R8G8_UNORM;
	case D3DVSDT_PBYTE3: case D3DVSDT_PBYTE4: return D3D12R_FORMAT_R8G8B8A8_UNORM;
	case D3DVSDT_NORMPACKED3: return D3D12R_FORMAT_R32_UINT;
	default: return D3D12R_FORMAT_R32G32B32A32_FLOAT;
	}
}

static unsigned long format_size(unsigned int format)
{
	switch (format)
	{
	case D3D12R_FORMAT_R32G32B32A32_FLOAT: return 16;
	case D3D12R_FORMAT_R32G32B32_FLOAT: return 12;
	case D3D12R_FORMAT_R32G32_FLOAT:
	case D3D12R_FORMAT_R16G16B16A16_SINT:
	case D3D12R_FORMAT_R16G16B16A16_SNORM: return 8;
	case D3D12R_FORMAT_R16_SINT:
	case D3D12R_FORMAT_R16_SNORM:
	case D3D12R_FORMAT_R8G8_UNORM: return 2;
	case D3D12R_FORMAT_R8_UNORM: return 1;
	default: return 4;
	}
}

/* an entry's repacked layout, for the streams with an element Direct3D 12
cannot read where it is */
static void input_layout_repack(struct input_layout_entry *entry)
{
	unsigned long index, stream;

	memcpy(entry->elements, entry->declared, sizeof(entry->elements));
	for (index = 0; index < entry->count; index++)
	{
		unsigned long size = format_size(entry->declared[index].format);

		if (entry->declared[index].offset % (size < 4 ? size : 4))
			entry->repacked_streams |= 1UL << entry->declared[index].stream;
	}
	for (stream = 0; stream < D3D12R_STREAMS; stream++)
	{
		unsigned long offset = 0;

		if (!(entry->repacked_streams & (1UL << stream)))
			continue;
		for (index = 0; index < entry->count; index++)
		{
			unsigned long size = format_size(entry->declared[index].format);

			if (entry->declared[index].stream != stream)
				continue;
			entry->elements[index].offset = (unsigned short)offset;
			offset += ((size > entry->bytes[index] ? size : entry->bytes[index]) + 3) & ~3UL;
		}
		entry->repacked_strides[stream] = (unsigned short)offset;
	}
}

/* the declaration's elements that the inputs take from streams; immediate
mode's: every register, floats from stream 0 */
static struct input_layout_entry *input_layout_get(BOOL immediate, const struct nv2a_vertex_inputs *inputs)
{
	static struct input_layout_entry immediate_layout, *last;
	const struct vertex_shader_object *declaration = xgpu_device.vertex_shader;
	struct d3d12r_element elements[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned char bytes[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long count = 0, index, hash;
	struct input_layout_entry **bucket, *entry;

	if (immediate)
	{
		if (!immediate_layout.id)
		{
			for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
			{
				immediate_layout.elements[index].reg = (unsigned char)index;
				immediate_layout.elements[index].stream = 0;
				immediate_layout.elements[index].offset = (unsigned short)(index * 4 * sizeof(float));
				immediate_layout.elements[index].format = D3D12R_FORMAT_R32G32B32A32_FLOAT;
			}
			memcpy(immediate_layout.declared, immediate_layout.elements, sizeof(immediate_layout.declared));
			immediate_layout.count = XGPU_VERTEX_ATTRIBUTE_COUNT;
			immediate_layout.id = next_input_layout_id++;
		}
		return &immediate_layout;
	}
	memset(elements, 0, sizeof(elements));
	memset(bytes, 0, sizeof(bytes));
	for (index = 0; declaration && index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];

		if (!(inputs->provided_mask & (1UL << element->reg)) || count >= XGPU_VERTEX_ATTRIBUTE_COUNT)
			continue;
		elements[count].reg = element->reg;
		elements[count].stream = element->stream;
		elements[count].offset = element->offset;
		elements[count].format = element_format(element->type);
		bytes[count] = element->bytes;
		count++;
	}
	if (last && last->count == count && !memcmp(last->declared, elements, sizeof(elements)))
		return last;
	hash = hash_words(elements, sizeof(elements));
	bucket = &input_layout_buckets[hash % INPUT_LAYOUT_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && entry->count == count && !memcmp(entry->declared, elements, sizeof(elements)))
			return last = entry;
	}
	if (!(entry = calloc(1, sizeof(*entry))))
		return NULL;
	entry->hash = hash;
	entry->count = count;
	memcpy(entry->declared, elements, sizeof(elements));
	memcpy(entry->bytes, bytes, sizeof(bytes));
	input_layout_repack(entry);
	entry->id = next_input_layout_id++;
	entry->next = *bucket;
	*bucket = entry;
	return last = entry;
}

/* ---------- the render states */

static unsigned char blend_factor(DWORD factor, BOOL alpha)
{
	switch (factor)
	{
	case D3DBLEND_ZERO: return D3D12R_BLEND_ZERO;
	case D3DBLEND_ONE: return D3D12R_BLEND_ONE;
	/* (a color's factor for alpha is its alpha's, as OpenGL takes it) */
	case D3DBLEND_SRCCOLOR: return alpha ? D3D12R_BLEND_SRC_ALPHA : D3D12R_BLEND_SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR: return alpha ? D3D12R_BLEND_INV_SRC_ALPHA : D3D12R_BLEND_INV_SRC_COLOR;
	case D3DBLEND_SRCALPHA: return D3D12R_BLEND_SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA: return D3D12R_BLEND_INV_SRC_ALPHA;
	case D3DBLEND_DESTALPHA: return D3D12R_BLEND_DEST_ALPHA;
	case D3DBLEND_INVDESTALPHA: return D3D12R_BLEND_INV_DEST_ALPHA;
	case D3DBLEND_DESTCOLOR: return alpha ? D3D12R_BLEND_DEST_ALPHA : D3D12R_BLEND_DEST_COLOR;
	case D3DBLEND_INVDESTCOLOR: return alpha ? D3D12R_BLEND_INV_DEST_ALPHA : D3D12R_BLEND_INV_DEST_COLOR;
	case D3DBLEND_SRCALPHASAT: return D3D12R_BLEND_SRC_ALPHA_SAT;
	/* (the blend factor is the constant alpha's for those: draw_blend_factor) */
	case D3DBLEND_CONSTANTCOLOR: case D3DBLEND_CONSTANTALPHA: return D3D12R_BLEND_BLEND_FACTOR;
	case D3DBLEND_INVCONSTANTCOLOR: case D3DBLEND_INVCONSTANTALPHA: return D3D12R_BLEND_INV_BLEND_FACTOR;
	default: return D3D12R_BLEND_ONE;
	}
}

static unsigned char blend_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DBLENDOP_SUBTRACT: return D3D12R_BLEND_OP_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT:
	case D3DBLENDOP_REVSUBTRACTSIGNED: return D3D12R_BLEND_OP_REV_SUBTRACT;
	case D3DBLENDOP_MIN: return D3D12R_BLEND_OP_MIN;
	case D3DBLENDOP_MAX: return D3D12R_BLEND_OP_MAX;
	default: return D3D12R_BLEND_OP_ADD;
	}
}

/* (the Xbox's are OpenGL's, NEVER to ALWAYS in order; 0 is NEVER) */
static unsigned char comparison(DWORD function)
{
	if (function >= D3DCMP_NEVER && function <= D3DCMP_ALWAYS)
		return (unsigned char)(D3D12R_COMPARISON_NEVER + (function - D3DCMP_NEVER));
	return D3D12R_COMPARISON_NEVER;
}

static unsigned char stencil_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DSTENCILOP_ZERO: return D3D12R_STENCIL_OP_ZERO;
	case D3DSTENCILOP_REPLACE: return D3D12R_STENCIL_OP_REPLACE;
	case D3DSTENCILOP_INCRSAT: return D3D12R_STENCIL_OP_INCR_SAT;
	case D3DSTENCILOP_DECRSAT: return D3D12R_STENCIL_OP_DECR_SAT;
	case D3DSTENCILOP_INVERT: return D3D12R_STENCIL_OP_INVERT;
	case D3DSTENCILOP_INCR: return D3D12R_STENCIL_OP_INCR;
	case D3DSTENCILOP_DECR: return D3D12R_STENCIL_OP_DECR;
	default: return D3D12R_STENCIL_OP_KEEP;
	}
}

/* the render states' part of the pipeline state, as d3d8_gl.c's
apply_raster_state sets OpenGL's */
static void pipeline_states(struct d3d12r_pipeline *key, BOOL has_depth)
{
	DWORD *rs = D3D__RenderState;
	DWORD write = rs[D3DRS_COLORWRITEENABLE];
	BOOL depth_test = has_depth && rs[D3DRS_ZENABLE];

	key->depth_enable = (unsigned char)depth_test;
	key->depth_write = (unsigned char)(depth_test && rs[D3DRS_ZWRITEENABLE]);
	key->depth_function = depth_test ? comparison(rs[D3DRS_ZFUNC]) : D3D12R_COMPARISON_ALWAYS;
	key->stencil_enable = (unsigned char)(has_depth && rs[D3DRS_STENCILENABLE]);
	if (key->stencil_enable)
	{
		key->stencil_read_mask = (unsigned char)rs[D3DRS_STENCILMASK];
		key->stencil_write_mask = (unsigned char)rs[D3DRS_STENCILWRITEMASK];
		key->stencil_function = comparison(rs[D3DRS_STENCILFUNC]);
		key->stencil_fail = stencil_operation(rs[D3DRS_STENCILFAIL]);
		key->stencil_depth_fail = stencil_operation(rs[D3DRS_STENCILZFAIL]);
		key->stencil_pass = stencil_operation(rs[D3DRS_STENCILPASS]);
	}
	else
	{
		key->stencil_function = D3D12R_COMPARISON_ALWAYS;
		key->stencil_fail = key->stencil_depth_fail = key->stencil_pass = D3D12R_STENCIL_OP_KEEP;
	}
	key->blend_enable = rs[D3DRS_ALPHABLENDENABLE] != 0;
	if (key->blend_enable)
	{
		key->blend_source = blend_factor(rs[D3DRS_SRCBLEND], FALSE);
		key->blend_destination = blend_factor(rs[D3DRS_DESTBLEND], FALSE);
		key->blend_source_alpha = blend_factor(rs[D3DRS_SRCBLEND], TRUE);
		key->blend_destination_alpha = blend_factor(rs[D3DRS_DESTBLEND], TRUE);
		key->blend_operation = blend_operation(rs[D3DRS_BLENDOP]);
	}
	else
	{
		key->blend_source = key->blend_source_alpha = D3D12R_BLEND_ONE;
		key->blend_destination = key->blend_destination_alpha = D3D12R_BLEND_ZERO;
		key->blend_operation = D3D12R_BLEND_OP_ADD;
	}
	key->write_mask = (unsigned char)(((write & D3DCOLORWRITEENABLE_RED) ? D3D12R_COLOR_WRITE_RED : 0) |
		((write & D3DCOLORWRITEENABLE_GREEN) ? D3D12R_COLOR_WRITE_GREEN : 0) |
		((write & D3DCOLORWRITEENABLE_BLUE) ? D3D12R_COLOR_WRITE_BLUE : 0) |
		((write & D3DCOLORWRITEENABLE_ALPHA) ? D3D12R_COLOR_WRITE_ALPHA : 0));
	/* the cull mode names the winding to discard; FRONTFACE names the front
	winding */
	key->front_counter_clockwise = rs[D3DRS_FRONTFACE] == D3DFRONT_CCW;
	key->cull_mode = rs[D3DRS_CULLMODE] == D3DCULL_NONE ? D3D12R_CULL_NONE :
		rs[D3DRS_CULLMODE] == rs[D3DRS_FRONTFACE] ? D3D12R_CULL_FRONT : D3D12R_CULL_BACK;
	key->fill_mode = rs[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? D3D12R_FILL_WIREFRAME : D3D12R_FILL_SOLID;
	/* D3DRS_ZBIAS is expressed in these states (D3DDevice_SetRenderState_ZBias):
	OpenGL's polygon offset of slope and units, Direct3D 12's bias in the depth
	buffer's least units and its slope scale */
	if (rs[D3DRS_SOLIDOFFSETENABLE])
	{
		key->depth_bias = (int)dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);
		key->slope_scaled_depth_bias = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
	}
}

/* Direct3D 12 has one blend factor: a blend of the constant alpha alone
has it in every channel */
static void draw_blend_factor(float factor[4])
{
	DWORD *rs = D3D__RenderState;
	DWORD source = rs[D3DRS_SRCBLEND], destination = rs[D3DRS_DESTBLEND];

	color_to_vec4(rs[D3DRS_BLENDCOLOR], factor);
	if ((source == D3DBLEND_CONSTANTALPHA || source == D3DBLEND_INVCONSTANTALPHA ||
		destination == D3DBLEND_CONSTANTALPHA || destination == D3DBLEND_INVCONSTANTALPHA) &&
		source != D3DBLEND_CONSTANTCOLOR && source != D3DBLEND_INVCONSTANTCOLOR &&
		destination != D3DBLEND_CONSTANTCOLOR && destination != D3DBLEND_INVCONSTANTCOLOR)
	{
		factor[0] = factor[1] = factor[2] = factor[3];
	}
}

/* the topology of a draw of type (fans, quads and polygons drawn as lists
of triangles, loops as strips: converted_indices) */
static void primitive_topology(D3DPRIMITIVETYPE type, unsigned char *topology_type, unsigned int *topology)
{
	switch (type)
	{
	case D3DPT_POINTLIST:
		*topology_type = D3D12R_TOPOLOGY_TYPE_POINT;
		*topology = D3D12R_TOPOLOGY_POINTLIST;
		break;
	case D3DPT_LINELIST:
		*topology_type = D3D12R_TOPOLOGY_TYPE_LINE;
		*topology = D3D12R_TOPOLOGY_LINELIST;
		break;
	case D3DPT_LINESTRIP:
	case D3DPT_LINELOOP:
		*topology_type = D3D12R_TOPOLOGY_TYPE_LINE;
		*topology = D3D12R_TOPOLOGY_LINESTRIP;
		break;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_QUADSTRIP:
		*topology_type = D3D12R_TOPOLOGY_TYPE_TRIANGLE;
		*topology = D3D12R_TOPOLOGY_TRIANGLESTRIP;
		break;
	default:
		*topology_type = D3D12R_TOPOLOGY_TYPE_TRIANGLE;
		*topology = D3D12R_TOPOLOGY_TRIANGLELIST;
		break;
	}
}

/* whether a draw of type needs indices made for it */
static BOOL primitive_converted(D3DPRIMITIVETYPE type)
{
	return type == D3DPT_QUADLIST || type == D3DPT_TRIANGLEFAN || type == D3DPT_POLYGON || type == D3DPT_LINELOOP;
}

/* the indices a draw of type is drawn with (indices NULL: 0, 1, 2 ...);
malloc'd, NULL for a type drawn as it is */
static WORD *converted_indices(D3DPRIMITIVETYPE type, const WORD *indices, unsigned long count,
	unsigned long *out_count)
{
	switch (type)
	{
	case D3DPT_QUADLIST:
		return xgpu_quad_indices(indices, count, out_count);
	case D3DPT_TRIANGLEFAN:
	case D3DPT_POLYGON:
		return xgpu_fan_indices(indices, count, out_count);
	case D3DPT_LINELOOP:
	{
		WORD *result = malloc((count + 1) * sizeof(WORD) + 2);
		unsigned long index;

		if (!result)
			return NULL;
		for (index = 0; index < count; index++)
			result[index] = indices ? indices[index] : (WORD)index;
		result[count] = indices ? indices[0] : 0;
		*out_count = count + 1;
		return result;
	}
	default:
		return NULL;
	}
}

/* ---------- samplers */

static unsigned int address_mode(DWORD mode)
{
	switch (mode)
	{
	case D3DTADDRESS_MIRROR: return D3D12R_ADDRESS_MIRROR;
	case D3DTADDRESS_CLAMP:
	case D3DTADDRESS_CLAMPTOEDGE: return D3D12R_ADDRESS_CLAMP;
	case D3DTADDRESS_BORDER: return D3D12R_ADDRESS_BORDER;
	default: return D3D12R_ADDRESS_WRAP;
	}
}

/* a stage's sampler, as d3d8_gl.c's configure_sampler sets OpenGL's; hires:
a high-res HUD texture (hud_hires.h), drawn smaller than it is, so filtered
and from its mip levels whatever the game asks */
static void stage_sampler(int stage, BOOL mipmapped, BOOL hires, struct d3d12r_sampler *sampler)
{
	DWORD *state = D3D__TextureState[stage];
	DWORD min_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MINFILTER];
	DWORD mip_filter = hires ? D3DTEXF_LINEAR : mipmapped ? state[D3DTSS_MIPFILTER] : D3DTEXF_NONE;
	DWORD mag_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MAGFILTER];
	DWORD maximum_mip_level = hires ? 0 : state[D3DTSS_MAXMIPLEVEL];
	float lod_bias = hires ? 0.0f : dword_to_float(state[D3DTSS_MIPMAPLODBIAS]);

	memset(sampler, 0, sizeof(*sampler));
	if (min_filter == D3DTEXF_ANISOTROPIC && state[D3DTSS_MAXANISOTROPY] > 1)
	{
		sampler->filter = D3D12R_FILTER_ANISOTROPIC;
		sampler->maximum_anisotropy = state[D3DTSS_MAXANISOTROPY] > 16 ? 16 : (unsigned int)state[D3DTSS_MAXANISOTROPY];
	}
	else
	{
		sampler->filter = D3D12R_BASIC_FILTER(
			min_filter == D3DTEXF_POINT ? D3D12R_FILTER_TYPE_POINT : D3D12R_FILTER_TYPE_LINEAR,
			mag_filter == D3DTEXF_POINT ? D3D12R_FILTER_TYPE_POINT : D3D12R_FILTER_TYPE_LINEAR,
			mip_filter == D3DTEXF_NONE || mip_filter == D3DTEXF_POINT ? D3D12R_FILTER_TYPE_POINT :
				D3D12R_FILTER_TYPE_LINEAR);
		sampler->maximum_anisotropy = 1;
	}
	sampler->address_u = address_mode(state[D3DTSS_ADDRESSU]);
	sampler->address_v = address_mode(state[D3DTSS_ADDRESSV]);
	sampler->address_w = address_mode(state[D3DTSS_ADDRESSW]);
	sampler->mip_lod_bias = lod_bias < -16.0f ? -16.0f : lod_bias > 15.99f ? 15.99f : lod_bias;
	color_to_vec4(state[D3DTSS_BORDERCOLOR], sampler->border_color);
	/* without mip filtering, the base level (OpenGL's non-mipmap filters) */
	if (mip_filter == D3DTEXF_NONE)
	{
		sampler->minimum_lod = 0.0f;
		sampler->maximum_lod = 0.0f;
	}
	else
	{
		sampler->minimum_lod = (float)maximum_mip_level;
		sampler->maximum_lod = 3.402823466e+38f;
	}
}

/* ---------- render targets

Each target's texture is the host half's; with multisampling, draws go to
multisampled storage of its own (xgpu_render_target multisample), resolved
into the texture before the texture is read. Nothing samples a depth buffer
as a texture, so a depth buffer is never resolved. */

static void d3d12_render_target_create(struct xgpu_render_target *target)
{
	target->texture = d3d12r_target_create(target->depth, target->pixel_width, target->pixel_height, 1);
	target->multisample = 0;
	target->samples = 0;
	target->unresolved = FALSE;
}

static void render_target_resolve(struct xgpu_render_target *target)
{
	if (!target->unresolved)
		return;
	target->unresolved = FALSE;
	if (!target->depth)
		d3d12r_target_resolve(target->multisample, target->texture);
}

/* a target's multisampled storage with these samples a pixel (0: none),
its pixels drawn so far resolved into its texture first */
static void render_target_multisample(struct xgpu_render_target *target, int samples)
{
	if (target->samples == samples)
		return;
	render_target_resolve(target);
	d3d12r_texture_delete(target->multisample);
	target->multisample = samples ?
		d3d12r_target_create(target->depth, target->pixel_width, target->pixel_height, samples) : 0;
	target->samples = samples;
}

/* ---------- render targets sampled with their mip chain

The game renders some textures one mip level at a time, each level being a
surface of its own (the water's ripple map). Sampling such a texture needs
every level in one texture, so the levels' render targets are copied into a
mipmapped composite whenever it is bound; levels the game did not render
are made from the ones it did. */

struct mip_composite
{
	struct mip_composite *next;
	unsigned long data, width, height, levels;
	unsigned int texture;
};

static struct mip_composite *mip_composites;

static unsigned int mip_composite_get(const struct xgpu_texture_description *description, unsigned long data)
{
	struct mip_composite *composite;
	unsigned long level, rendered_levels = 0;

	for (composite = mip_composites; composite; composite = composite->next)
	{
		if (composite->data == data && composite->width == description->width &&
			composite->height == description->height && composite->levels == description->levels)
		{
			break;
		}
	}
	if (!composite)
	{
		if (!(composite = calloc(1, sizeof(*composite))))
			return 0;
		composite->data = data;
		composite->width = description->width;
		composite->height = description->height;
		composite->levels = description->levels;
		composite->texture = d3d12r_composite_create(description->width, description->height, description->levels);
		composite->next = mip_composites;
		mip_composites = composite;
	}
	for (level = 0; level < description->levels; level++)
	{
		unsigned long width = description->width >> level ? description->width >> level : 1;
		unsigned long height = description->height >> level ? description->height >> level : 1;
		struct xgpu_render_target *target =
			xgpu_render_target_find(data + xgpu_texture_level_offset(description, level));

		if (!target || target->width != width || target->height != height || target->pixel_width != width ||
			target->pixel_height != height)
		{
			break;
		}
		render_target_resolve(target);
		d3d12r_composite_copy_level(composite->texture, level, target->texture);
		rendered_levels++;
	}
	/* levels the game did not render come from the ones it did */
	if (rendered_levels < description->levels)
		d3d12r_composite_mipmaps(composite->texture, rendered_levels ? rendered_levels : 1);
	return composite->texture;
}

/* ---------- a draw's textures and targets */

/* the bound targets' pixels per unit and samples a pixel (the visibility
tests' area) */
static float target_scale[2] = { 1.0f, 1.0f };
static int target_samples = 1;

static long target_pixel(float coordinate, int axis)
{
	return xgpu_scaled_pixel(coordinate, target_scale[axis]);
}

static void bind_textures(struct d3d12r_draw *draw, struct nv2a_pixel_shader_key *key, float texture_scale[4][4])
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = xgpu_device.textures[stage];
		unsigned long mode = stage_texture_mode(stage);
		struct xgpu_texture_description description;
		struct xgpu_render_target *target;
		unsigned int handle;
		int type = _xgpu_texture_2d;

		texture_scale[stage][0] = texture_scale[stage][1] = 1.0f;
		texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
		draw->textures[stage] = 0;
		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
		{
			key->sampler_type[stage] = mode == 0x11 ? _xgpu_sampler_2d : _xgpu_sampler_none;
			draw->sampler_types[stage] = key->sampler_type[stage];
			continue;
		}
		if ((target = xgpu_render_target_find(texture->Data)) != NULL)
		{
			/* (multisampled: its pixels drawn since, resolved) */
			render_target_resolve(target);
			xgpu_texture_describe(texture->Format, texture->Size, &description);
			handle = target->texture;
			if (description.linear)
			{
				texture_scale[stage][0] = 1.0f / (float)target->width;
				texture_scale[stage][1] = 1.0f / (float)target->height;
			}
			if (!description.linear && !description.cube_map && description.levels > 1 &&
				target->width == description.width && target->height == description.height)
				handle = mip_composite_get(&description, texture->Data);
			else
				description.levels = 1;
		}
		else
		{
			const D3DCOLOR *palette = xgpu_device.palettes[stage] && xgpu_device.palettes[stage]->Data ?
				(const D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(xgpu_device.palettes[stage]->Data) : NULL;

			handle = xgpu_texture_get((const DWORD *)texture, palette, &type, &description);
			if (description.linear)
			{
				texture_scale[stage][0] = 1.0f / (float)description.width;
				texture_scale[stage][1] = 1.0f / (float)description.height;
			}
		}
		draw->textures[stage] = handle;
		stage_sampler(stage, description.levels > 1, description.hires, &draw->samplers[stage]);
		if (stage == 0)
			key->coverage_alpha = description.hires_coverage != FALSE;
		key->sampler_type[stage] = type == _xgpu_texture_cube ? _xgpu_sampler_cube :
			type == _xgpu_texture_3d ? _xgpu_sampler_3d : _xgpu_sampler_2d;
		draw->sampler_types[stage] = key->sampler_type[stage];
	}
}

/* the draw's targets, as d3d8_gl.c's bind_targets binds OpenGL's
framebuffer; FALSE if there is nothing to draw into */
static BOOL bind_targets(unsigned int *color_texture, unsigned int *depth_texture)
{
	struct render_target_entry *color = xgpu_render_target_get(xgpu_device.render_target);
	struct render_target_entry *depth = xgpu_render_target_get(xgpu_device.depth_stencil);
	int samples;

	if (depth && !depth->target.depth)
		depth = NULL;
	if (!color && !depth)
		return FALSE;
	if (color)
		color->last_rendered = xgpu_device.frame + 1;
	/* viewports and clears are in the targets' units */
	target_scale[0] = color ? color->target.scale[0] : depth->target.scale[0];
	target_scale[1] = color ? color->target.scale[1] : depth->target.scale[1];
	/* with multisampling, multisampled where either is a screen buffer or
	is multisampled already */
	samples = xgpu_anti_aliasing() == _anti_aliasing_msaa ? xgpu_anti_aliasing_samples() : 0;
	if (samples && !((color && (color->screen_buffer || color->target.samples)) ||
		(depth && (depth->screen_buffer || depth->target.samples))))
	{
		samples = 0;
	}
	if (color)
		render_target_multisample(&color->target, samples);
	if (depth)
		render_target_multisample(&depth->target, samples);
	*color_texture = color ? (samples ? color->target.multisample : color->target.texture) : 0;
	*depth_texture = depth ? (samples ? depth->target.multisample : depth->target.texture) : 0;
	if (samples)
	{
		if (color)
			color->target.unresolved = TRUE;
		if (depth)
			depth->target.unresolved = TRUE;
	}
	target_samples = samples ? samples : 1;
	return (!color || *color_texture) && (!depth || *depth_texture);
}

/* the viewport and the scissor (the viewport's rectangle: the NV2A's
scissor follows it, which keeps a split-screen window's geometry from
bleeding across the divider), in the targets' pixels as OpenGL's are */
static void viewport_set(struct d3d12r_draw *draw)
{
	long x0 = target_pixel((float)xgpu_device.viewport.X, 0);
	long y0 = target_pixel((float)xgpu_device.viewport.Y, 1);
	long x1 = target_pixel((float)(xgpu_device.viewport.X + xgpu_device.viewport.Width), 0);
	long y1 = target_pixel((float)(xgpu_device.viewport.Y + xgpu_device.viewport.Height), 1);

	draw->viewport[0] = (float)x0;
	draw->viewport[1] = (float)y0;
	draw->viewport[2] = (float)(x1 > x0 ? x1 - x0 : 0);
	draw->viewport[3] = (float)(y1 > y0 ? y1 - y0 : 0);
	draw->viewport[4] = xgpu_device.viewport.MinZ;
	draw->viewport[5] = xgpu_device.viewport.MaxZ;
	if (x1 > x0 && y1 > y0)
	{
		draw->scissor[0] = x0 > 0 ? x0 : 0;
		draw->scissor[1] = y0 > 0 ? y0 : 0;
		draw->scissor[2] = x1;
		draw->scissor[3] = y1;
	}
	else
	{
		/* (no scissor test, as OpenGL's without one) */
		draw->scissor[0] = draw->scissor[1] = 0;
		draw->scissor[2] = draw->scissor[3] = 16384;
	}
}

/* ---------- constants

Each draw's are made again only when what they come from has changed since
the draw before, whose upload the host half then shares, by their serial. */

static struct xgpu_hlsl_vertex_constants vertex_constants;
static struct xgpu_hlsl_pixel_constants pixel_constants;
static unsigned long vertex_constants_serial, pixel_constants_serial;

static void constants_update(void)
{
	static BOOL valid;
	static unsigned long long constants_serial;
	static unsigned long uniforms_serial;
	const struct draw_uniforms *uniforms = &xgpu_draw_uniforms;
	BOOL constants_changed = !valid || constants_serial != xgpu_constant_serials.serial;
	BOOL uniforms_changed = !valid || uniforms_serial != xgpu_draw_uniforms_serial;

	if (constants_changed || uniforms_changed ||
		memcmp(vertex_constants.attribute_values, xgpu_device.attributes, sizeof(vertex_constants.attribute_values)))
	{
		memcpy(vertex_constants.c, xgpu_device.constants, sizeof(vertex_constants.c));
		memcpy(vertex_constants.viewport_scale, uniforms->viewport_scale, sizeof(vertex_constants.viewport_scale));
		memcpy(vertex_constants.viewport_offset, uniforms->viewport_offset, sizeof(vertex_constants.viewport_offset));
		vertex_constants.misc[0] = uniforms->point_size;
		vertex_constants.misc[1] = uniforms->screen_offset;
		memcpy(vertex_constants.attribute_values, xgpu_device.attributes, sizeof(vertex_constants.attribute_values));
		vertex_constants_serial++;
	}
	if (constants_changed || uniforms_changed)
	{
		int light;

		memcpy(pixel_constants.ps_c0, uniforms->ps_c0, sizeof(pixel_constants.ps_c0));
		memcpy(pixel_constants.ps_c1, uniforms->ps_c1, sizeof(pixel_constants.ps_c1));
		memcpy(pixel_constants.ps_final_c0, uniforms->ps_final_c0, sizeof(pixel_constants.ps_final_c0));
		memcpy(pixel_constants.ps_final_c1, uniforms->ps_final_c1, sizeof(pixel_constants.ps_final_c1));
		memcpy(pixel_constants.fog_color, uniforms->fog_color, sizeof(pixel_constants.fog_color));
		memcpy(pixel_constants.fog_parameters, uniforms->fog_parameters, sizeof(pixel_constants.fog_parameters));
		pixel_constants.misc[0] = uniforms->alpha_reference;
		memcpy(pixel_constants.bump_matrix, uniforms->bump_matrix, sizeof(pixel_constants.bump_matrix));
		memcpy(pixel_constants.bump_luminance, uniforms->bump_luminance, sizeof(pixel_constants.bump_luminance));
		memcpy(pixel_constants.texture_scale, uniforms->texture_scale, sizeof(pixel_constants.texture_scale));
		for (light = 0; light < XGPU_MODEL_LIGHT_COUNT; light++)
		{
			memcpy(pixel_constants.model_lights[light], xgpu_device.constants[model_light_register(light)],
				sizeof(pixel_constants.model_lights[light]));
		}
		pixel_constants_serial++;
	}
	valid = TRUE;
	constants_serial = xgpu_constant_serials.serial;
	uniforms_serial = xgpu_draw_uniforms_serial;
}

/* ---------- describing a draw */

/* the input layout of the draw being described */
static const struct input_layout_entry *draw_layout;

/* the shaders, pipeline state, textures, targets and constants of a draw;
FALSE to skip it */
static BOOL prepare_draw(struct d3d12r_draw *draw, BOOL immediate, D3DPRIMITIVETYPE type)
{
	struct vertex_shader_object *program = current_program();
	struct nv2a_pixel_shader_key key;
	struct nv2a_vertex_inputs inputs;
	struct vertex_shader_entry *vertex_shader = NULL;
	struct pixel_shader_entry *pixel_shader = NULL;
	struct input_layout_entry *input_layout;
	float texture_scale[4][4];
	BOOL lit = FALSE;

	if (!xgpu_device.ready || !program || !xgpu_device.vertex_shader || !program->instructions)
	{
		xgpu_statistics.skipped_no_program++;
		return FALSE;
	}
	if (xgpu_skip_program(program))
		return FALSE;
	memset(draw, 0, sizeof(*draw));
	xgpu_pixel_shader_key_begin(&key);
	/* the textures before the targets: a render target the draw samples has
	its multisampled pixels resolved first */
	bind_textures(draw, &key, texture_scale);
	if (!bind_targets(&draw->color, &draw->depth))
	{
		xgpu_statistics.skipped_no_target++;
		return FALSE;
	}
	viewport_set(draw);
	xgpu_pixel_shader_key_finish(&key, target_samples);

	xgpu_vertex_inputs(immediate, &inputs);
	if (program->lighting.lights && !program->lighting_failed && xgpu_per_pixel_lighting())
	{
		key.per_pixel_lighting = (unsigned char)program->lighting.lights;
		vertex_shader = vertex_shader_get(program, TRUE, &inputs);
		pixel_shader = vertex_shader ? pixel_shader_get(&key) : NULL;
		lit = vertex_shader && pixel_shader;
		if (!lit)
		{
			/* drawn as the vertex shader lights it instead */
			platform_log("GPU: vertex shader %lu cannot be lit for each pixel here (refer to the shader log above): "
				"lit for each vertex", program->id);
			program->lighting_failed = TRUE;
			key.per_pixel_lighting = 0;
		}
	}
	if (!lit)
	{
		vertex_shader = vertex_shader_get(program, FALSE, &inputs);
		pixel_shader = pixel_shader_get(&key);
	}
	input_layout = input_layout_get(immediate, &inputs);
	if (!vertex_shader || !pixel_shader || !input_layout)
	{
		xgpu_statistics.skipped_link++;
		return FALSE;
	}
	draw->vertex_shader = vertex_shader->code;
	draw->pixel_shader = pixel_shader->code;
	draw->elements = input_layout->elements;
	draw->element_count = input_layout->count;
	draw_layout = input_layout;

	primitive_topology(type, &draw->pipeline.topology_type, &draw->topology);
	pipeline_states(&draw->pipeline, draw->depth != 0);
	draw->pipeline.vertex_shader = vertex_shader->id;
	draw->pipeline.pixel_shader = pixel_shader->id;
	draw->pipeline.input_layout = input_layout->id;
	draw->pipeline.samples = (unsigned char)target_samples;
	draw->pipeline.has_color = draw->color != 0;
	draw->pipeline.has_depth = draw->depth != 0;
	draw->stencil_reference = D3D__RenderState[D3DRS_STENCILREF] & 0xff;
	draw->pass = xgpu_draw_label.pass;
	draw->pass_name = xgpu_pass_name(xgpu_draw_label.pass);
	draw->window = xgpu_draw_label.view.window;
	draw_blend_factor(draw->blend_factor);

	/* the state the other uniforms come from (most draws share it with the
	draw before them, and so share its constants) */
	xgpu_draw_uniforms_update((const float (*)[4])texture_scale);
	constants_update();
	draw->vertex_constants = &vertex_constants;
	draw->vertex_constants_size = sizeof(vertex_constants);
	draw->vertex_constants_serial = vertex_constants_serial;
	draw->pixel_constants = &pixel_constants;
	draw->pixel_constants_size = sizeof(pixel_constants);
	draw->pixel_constants_serial = pixel_constants_serial;
	draw->index_segment = -1;
	if (immediate)
		xgpu_statistics.immediate_draws++;
	else
		xgpu_statistics.draws++;
	return TRUE;
}

/* a stream's vertices copied into its repacked layout (input_layout_repack),
into memory kept for each stream */
static void stream_repack(struct d3d12r_stream *stream, unsigned long stream_index, unsigned long count)
{
	static unsigned char *buffers[D3D12R_STREAMS];
	static unsigned long buffer_sizes[D3D12R_STREAMS];
	const struct input_layout_entry *layout = draw_layout;
	unsigned long stride = layout->repacked_strides[stream_index], size = stride * count, vertex, index;
	const unsigned char *source = stream->data;
	unsigned char *buffer;

	if (buffer_sizes[stream_index] < size)
	{
		free(buffers[stream_index]);
		buffer_sizes[stream_index] = 0;
		if (!(buffers[stream_index] = malloc(size + 65536)))
			return;
		buffer_sizes[stream_index] = size + 65536;
	}
	buffer = buffers[stream_index];
	memset(buffer, 0, size);
	for (vertex = 0; vertex < count; vertex++)
	{
		for (index = 0; index < layout->count; index++)
		{
			if (layout->declared[index].stream == stream_index)
			{
				memcpy(buffer + vertex * stride + layout->elements[index].offset,
					source + vertex * stream->stride + layout->declared[index].offset, layout->bytes[index]);
			}
		}
	}
	stream->data = buffer;
	stream->size = size;
	stream->stride = stride;
	stream->segment = -1;
}

/* the declaration's streams, vertices [first, first + count) of each, from
the mirror where it holds them (xgpu_device.h) */
static void streams_set_up(struct d3d12r_draw *draw, unsigned long first, unsigned long count)
{
	const struct vertex_shader_object *declaration = xgpu_device.vertex_shader;
	unsigned long index;

	for (index = 0; index < D3D12R_STREAMS; index++)
		draw->streams[index].segment = -1;
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		struct d3d12r_stream *stream = &draw->streams[element->stream];
		unsigned long stride = xgpu_device.streams[element->stream].stride;
		unsigned long bytes = stride ? stride * count : 64;
		unsigned long base, segment, offset;

		if (!xgpu_device.streams[element->stream].data || element->type == D3DVSDT_NONE || stream->size)
			continue;
		base = (unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(xgpu_device.streams[element->stream].data) + first * stride;
		stream->data = (const void *)base;
		stream->size = bytes;
		stream->stride = stride;
		if (draw_layout->repacked_streams & (1UL << element->stream))
		{
			stream_repack(stream, element->stream, count);
			xgpu_statistics.streamed_bytes += stream->size;
		}
		else if (xgpu_mirror_range(base, bytes, &segment, &offset, NULL))
		{
			stream->segment = (long)segment;
			stream->offset = offset;
		}
		else
		{
			xgpu_statistics.streamed_bytes += bytes;
		}
	}
}

/* ---------- the draws */

static void d3d12_draw_vertices(D3DPRIMITIVETYPE type, UINT start_vertex, UINT vertex_count)
{
	struct d3d12r_draw draw;
	WORD *indices;
	unsigned long index_count = 0;

	if (!vertex_count || !prepare_draw(&draw, FALSE, type))
		return;
	xgpu_trace_draw("draw", type, vertex_count, NULL);
	streams_set_up(&draw, start_vertex, vertex_count);
	indices = converted_indices(type, NULL, vertex_count, &index_count);
	if (indices)
	{
		draw.indices = indices;
		draw.index_count = index_count;
	}
	else
	{
		draw.vertex_count = vertex_count;
	}
	d3d12r_draw(&draw);
	free(indices);
}

static void d3d12_draw_indexed_vertices(D3DPRIMITIVETYPE type, UINT vertex_count, const WORD *index_data)
{
	struct d3d12r_draw draw;
	unsigned long minimum, maximum, generation = 0, segment = 0, offset = 0, count = vertex_count;
	WORD *indices = NULL;
	BOOL mirrored;

	if (!vertex_count || !index_data || !prepare_draw(&draw, FALSE, type))
		return;
	/* converted primitives are drawn from indices made for the draw */
	mirrored = !primitive_converted(type) &&
		xgpu_mirror_range((unsigned long)index_data, vertex_count * sizeof(WORD), &segment, &offset, &generation);
	xgpu_index_extent(index_data, vertex_count, generation, mirrored, &minimum, &maximum);
	xgpu_trace_draw("indexed", type, vertex_count, NULL);
	/* (the streams from the base vertex on: index i is vertex base + i; the
	attributes start at vertex minimum) */
	streams_set_up(&draw, xgpu_device.base_vertex_index + minimum, maximum - minimum + 1);
	draw.base_vertex = -(long)minimum;
	if (mirrored)
	{
		draw.index_segment = (long)segment;
		draw.index_offset = offset;
		draw.indices = index_data;
	}
	else
	{
		indices = converted_indices(type, index_data, vertex_count, &count);
		draw.indices = indices ? indices : index_data;
		xgpu_statistics.streamed_bytes += vertex_count * sizeof(WORD);
	}
	draw.index_count = count;
	if (count)
		d3d12r_draw(&draw);
	free(indices);
}

static void d3d12_draw_immediate(D3DPRIMITIVETYPE type, const float *vertices, unsigned long count)
{
	unsigned long stride = XGPU_VERTEX_ATTRIBUTE_COUNT * 4 * sizeof(float);
	struct d3d12r_draw draw;
	WORD *indices;
	unsigned long index_count = 0, index;

	if (!count || !prepare_draw(&draw, TRUE, type))
		return;
	xgpu_trace_draw("immediate", type, count, vertices);
	for (index = 0; index < D3D12R_STREAMS; index++)
		draw.streams[index].segment = -1;
	draw.streams[0].data = vertices;
	draw.streams[0].size = count * stride;
	draw.streams[0].stride = stride;
	indices = converted_indices(type, NULL, count, &index_count);
	if (indices)
	{
		draw.indices = indices;
		draw.index_count = index_count;
	}
	else
	{
		draw.vertex_count = count;
	}
	d3d12r_draw(&draw);
	free(indices);
}

/* ---------- clearing */

static void d3d12_clear(DWORD count, const D3DRECT *rectangles, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
	long rects[16][4];
	unsigned int rect_count = 0, channels;
	unsigned int color_texture = 0, depth_texture = 0;
	float rgba[4];
	DWORD index;

	if (!bind_targets(&color_texture, &depth_texture))
		return;
	if (xgpu_trace_frame())
	{
		platform_log("clear flags %lx color %08lx z %g count %lu target %08lx depth %08lx", (unsigned long)flags,
			(unsigned long)color, z, (unsigned long)count,
			xgpu_device.render_target ? (unsigned long)xgpu_device.render_target->Data : 0,
			xgpu_device.depth_stencil ? (unsigned long)xgpu_device.depth_stencil->Data : 0);
	}
	xgpu_statistics.clears++;
	color_to_vec4(color, rgba);
	if (!count || !rectangles)
	{
		/* the NV2A clips a viewport-less clear to the viewport, which is what
		keeps a split-screen window's clear from wiping the other window */
		long x0 = target_pixel((float)xgpu_device.viewport.X, 0);
		long y0 = target_pixel((float)xgpu_device.viewport.Y, 1);

		rects[0][0] = x0 > 0 ? x0 : 0;
		rects[0][1] = y0 > 0 ? y0 : 0;
		rects[0][2] = target_pixel((float)(xgpu_device.viewport.X + xgpu_device.viewport.Width), 0);
		rects[0][3] = target_pixel((float)(xgpu_device.viewport.Y + xgpu_device.viewport.Height), 1);
		rect_count = 1;
	}
	else
	{
		for (index = 0; index < count && rect_count < 16; index++)
		{
			INT left = rectangles[index].x1 > (INT)xgpu_device.viewport.X ?
				rectangles[index].x1 : (INT)xgpu_device.viewport.X;
			INT top = rectangles[index].y1 > (INT)xgpu_device.viewport.Y ?
				rectangles[index].y1 : (INT)xgpu_device.viewport.Y;
			INT right = rectangles[index].x2 < (INT)(xgpu_device.viewport.X + xgpu_device.viewport.Width) ?
				rectangles[index].x2 : (INT)(xgpu_device.viewport.X + xgpu_device.viewport.Width);
			INT bottom = rectangles[index].y2 < (INT)(xgpu_device.viewport.Y + xgpu_device.viewport.Height) ?
				rectangles[index].y2 : (INT)(xgpu_device.viewport.Y + xgpu_device.viewport.Height);
			long ui_offset = xgpu_ui_offset();

			if (left >= right || top >= bottom)
				continue;
			rects[rect_count][0] = target_pixel((float)(left + ui_offset), 0);
			rects[rect_count][1] = target_pixel((float)top, 1);
			rects[rect_count][2] = target_pixel((float)(right + ui_offset), 0);
			rects[rect_count][3] = target_pixel((float)bottom, 1);
			if (rects[rect_count][0] < 0)
				rects[rect_count][0] = 0;
			if (rects[rect_count][1] < 0)
				rects[rect_count][1] = 0;
			rect_count++;
		}
	}
	/* the Xbox clears the channels named (D3DCLEAR_TARGET_R, _G, _B, _A): the
	fog screen clears only alpha, leaving the picture under the fog */
	channels = ((flags & D3DCLEAR_TARGET_R) ? D3D12R_COLOR_WRITE_RED : 0) |
		((flags & D3DCLEAR_TARGET_G) ? D3D12R_COLOR_WRITE_GREEN : 0) |
		((flags & D3DCLEAR_TARGET_B) ? D3D12R_COLOR_WRITE_BLUE : 0) |
		((flags & D3DCLEAR_TARGET_A) ? D3D12R_COLOR_WRITE_ALPHA : 0);
	d3d12r_clear(color_texture, depth_texture, channels, rgba, (flags & D3DCLEAR_ZBUFFER) != 0, z,
		(flags & D3DCLEAR_STENCIL) != 0, stencil, (const long (*)[4])rects, rect_count);
}

/* ---------- visibility (occlusion) tests */

static BOOL query_pending[VISIBILITY_TEST_SLOTS];
static float query_area[VISIBILITY_TEST_SLOTS];
/* debug.visibility_wait: each test's own count, waited for */
static BOOL visibility_wait;

static void d3d12_visibility_begin(void)
{
	d3d12r_visibility_begin();
}

static void d3d12_visibility_end(DWORD index)
{
	d3d12r_visibility_end(index);
	/* the target's samples to a game pixel: the result is a count of the
	game's pixels, which the game divides by its own test's area (lens
	flares, rasterizer_lights.c) */
	query_area[index] = target_scale[0] * target_scale[1] * (float)target_samples;
	query_pending[index] = TRUE;
}

static HRESULT d3d12_visibility_result(DWORD index, UINT *result)
{
	unsigned long long samples;
	float area;

	if (!query_pending[index])
	{
		if (result)
			*result = 0;
		return S_OK;
	}
	samples = d3d12r_visibility_samples(index, visibility_wait);
	area = query_area[index];
	if (result)
		*result = area > 1.0f ? (UINT)((float)samples / area + 0.5f) : (UINT)samples;
	return S_OK;
}

/* ---------- anti-aliasing */

/* a lookup texture of SMAA's from its zlib stream (malloc'd); NULL if it does
not inflate */
static unsigned char *smaa_lookup(const unsigned int *stream, unsigned long stream_size, unsigned long bytes)
{
	uLongf size = (uLongf)bytes;
	unsigned char *texels = malloc(bytes);
	int result;

	if (!texels)
		return NULL;
	result = uncompress(texels, &size, (const Bytef *)stream, (uLong)stream_size);
	/* (the size says whether it is whole, as with the game's zlib 1.1,
	which could stop short of saying the stream had ended) */
	if ((result != Z_OK && result != Z_BUF_ERROR) || size != (uLongf)bytes)
	{
		free(texels);
		return NULL;
	}
	return texels;
}

static void d3d12_anti_aliasing_prepare(int mode)
{
	int prepared = 1;

	if (mode == _anti_aliasing_fxaa)
	{
		prepared = d3d12r_anti_aliasing_prepare(0, NULL, NULL, NULL);
	}
	else if (mode == _anti_aliasing_smaa)
	{
		unsigned char *area = smaa_lookup(xgpu_smaa_area_texture, xgpu_smaa_area_texture_size, 160 * 560 * 2);
		unsigned char *search = smaa_lookup(xgpu_smaa_search_texture, xgpu_smaa_search_texture_size, 64 * 16);

		prepared = d3d12r_anti_aliasing_prepare(1, xgpu_smaa_shader_size ? (const char *)xgpu_smaa_shader : NULL,
			area, search);
		free(area);
		free(search);
	}
	if (!prepared)
		platform_log("anti-aliasing: its programs do not build, so the 3D view is not antialiased");
}

static void d3d12_anti_alias(short x0, short y0, short x1, short y1)
{
	/* (the primary target's view: the back buffer's) */
	struct render_target_entry *target = xgpu_render_target_get(&xgpu_device.back_buffer);
	long corners[4];

	if (!target)
		return;
	/* (no longer multisampled, if it was before the setting changed) */
	render_target_multisample(&target->target, 0);
	corners[0] = xgpu_scaled_pixel(x0, target->target.scale[0]);
	corners[1] = xgpu_scaled_pixel(y0, target->target.scale[1]);
	corners[2] = xgpu_scaled_pixel(x1, target->target.scale[0]);
	corners[3] = xgpu_scaled_pixel(y1, target->target.scale[1]);
	d3d12r_anti_alias(xgpu_anti_aliasing() == _anti_aliasing_smaa, target->target.texture, corners);
}

/* ---------- presentation */

static int d3d12_save_screenshot(struct render_target_entry *target, const char *path)
{
	unsigned long width = target->target.pixel_width, height = target->target.pixel_height;
	unsigned char *pixels;
	int saved = 0;

	render_target_resolve(&target->target);
	if (!(pixels = malloc(width * height * 4)))
		return 0;
	/* (rows from the top, as screenshot_write_bmp takes them) */
	if (d3d12r_read_pixels(target->target.texture, pixels, width, height))
		saved = screenshot_write_bmp(path, pixels, width, height);
	free(pixels);
	return saved;
}

static void d3d12_present(struct render_target_entry *back_buffer)
{
	int width = 0, height = 0;

	render_target_resolve(&back_buffer->target);
	platform_video_drawable_size(&width, &height);
	d3d12r_present(back_buffer->target.texture, width, height, config_boolean("display.vsync"));
	platform_video_pace();
}

/* ---------- what the game draws */

static void d3d12_view_begin(const struct xgpu_view *view)
{
	struct d3d12r_view host;

	host.window = view->window;
	host.mirrored = view->mirrored;
	memcpy(host.position, view->position, sizeof(host.position));
	memcpy(host.forward, view->forward, sizeof(host.forward));
	memcpy(host.up, view->up, sizeof(host.up));
	host.vertical_field_of_view = view->vertical_field_of_view;
	host.z_near = view->z_near;
	host.z_far = view->z_far;
	memcpy(host.viewport, view->viewport, sizeof(host.viewport));
	d3d12r_view_begin(&host);
}

static void d3d12_flush(void)
{
	/* (the GPU gets the frame's work when it is presented) */
}

/* ---------- the device */

static BOOL d3d12_initialize(unsigned long width, unsigned long height)
{
	int drawable_width = 0, drawable_height = 0;
	void *window;

	if (!platform_video_initialize_window(width, height))
		return FALSE;
	window = platform_video_native_window();
	platform_video_drawable_size(&drawable_width, &drawable_height);
	visibility_wait = config_boolean("debug.visibility_wait") != 0;
	if (!window || !d3d12r_initialize(window, drawable_width, drawable_height, config_boolean("debug.d3d12_debug"),
		config_boolean("debug.d3d12_gpu_validation"), &xgpu_device.maximum_samples, &xgpu_device.maximum_target_size))
	{
		platform_video_shutdown();
		return FALSE;
	}
	return TRUE;
}

static void d3d12_mirror_upload(unsigned long segment, unsigned long offset, unsigned long size, const void *data,
	BOOL unused)
{
	(void)unused;
	d3d12r_mirror_upload(segment, offset, size, data);
}

static BOOL d3d12_texture_compressed_supported(unsigned long width, unsigned long height)
{
	return d3d12r_texture_compressed_supported(width, height) != 0;
}

const struct xgpu_backend xgpu_backend_d3d12 =
{
	.name = "Direct3D 12",
	.initialize = d3d12_initialize,
	.anti_aliasing_prepare = d3d12_anti_aliasing_prepare,
	.render_target_create = d3d12_render_target_create,
	.draw_vertices = d3d12_draw_vertices,
	.draw_indexed_vertices = d3d12_draw_indexed_vertices,
	.draw_immediate = d3d12_draw_immediate,
	.clear = d3d12_clear,
	.visibility_begin = d3d12_visibility_begin,
	.visibility_end = d3d12_visibility_end,
	.visibility_result = d3d12_visibility_result,
	.anti_alias = d3d12_anti_alias,
	.save_screenshot = d3d12_save_screenshot,
	.present = d3d12_present,
	.flush = d3d12_flush,
	.mirror_upload = d3d12_mirror_upload,
	.texture_new = d3d12r_texture_new,
	.texture_write = d3d12r_texture_write,
	.texture_write_rows = d3d12r_texture_write_rows,
	.texture_mipmaps = d3d12r_texture_mipmaps,
	.texture_delete = d3d12r_texture_delete,
	.texture_compressed_supported = d3d12_texture_compressed_supported,
	.texture_dump = NULL,
	.view_begin = d3d12_view_begin,
	.opaque_done = d3d12r_opaque_done,
};
