/*
MARINE_FINGERS.C

The armored marines' (characters\marine_armored\marine_armored) bare
fingertips are drawn as shiny as their armour, on the Xbox too. The red of
their arms' multipurpose map (characters\marine_armored\bitmaps\
marine_arms_multipurpose), the reflection mask (rasterizer_xbox_models.c,
set_environment_shader_pixel_shader), is set under the ends of the hand's
fingers, where the mask of an armour piece painted below the hand spills
over them: the fingertips reflect the environment's cube map, pale, or red
under a10's alarm lights, as if their nails were painted. Only the hands'
triangles sample those texels.

When an Xbox map's tags load (scenario_tags_load), the texels of the mask
that the hands' triangles sample and no other triangle of the model does are
found in each of its mipmaps, from the model's own triangles: every level of
detail of every part drawn with a shader whose multipurpose map it is, a
texel being sampled where a triangle covers any of it, and its neighbours
(the filtering). Each time the mask's pixels are loaded into the texture
cache (xbox_texture_cache.c), before they are drawn, their red is cleared
there: in a DXT1 block no other triangle samples, its two colors' red (so
every texel of it); in one another triangle does, each of the hands' texels
takes the block's color of the least red. Nothing else of the mask changes,
nor anything played or sent between machines. A mod that replaces the mask
or the model, another model drawn with the mask, a shader that moves the
mask's texture coordinates, and Custom Edition and MCC maps leave it as it
is.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "shaders/shader_definitions.h"
#include "shaders/shader_texture_animation.h"
#include "tag_files/tag_groups.h"
#include "custom_edition_cache.h"
#include "mcc_cache.h"
#include "mods.h"

#include <xtl.h>
#include <stddef.h>
#include <string.h>

/* ---------- constants */

#define MARINE_MODEL_NAME "characters\\marine_armored\\marine_armored"
/* (the nodes "bip01 l hand" and "bip01 r hand") */
#define HAND_NODE_NAME_TEXT "hand"

/* the largest mask this reads (the marines' is 256 x 512), and its blocks to
clear in all its levels */
#define MARINE_FINGERS_MAXIMUM_SIDE 512
#define MARINE_FINGERS_MAXIMUM_BLOCKS 4096
#define MARINE_FINGERS_LEVEL_LONGS (MARINE_FINGERS_MAXIMUM_SIDE * MARINE_FINGERS_MAXIMUM_SIDE / LONG_BITS)

/* a compressed vertex's texture coordinate, and its weight on its first node,
at 1 (rasterizer_geometry.c) */
#define MODEL_VERTEX_COMPRESSED_ONE 32767.f
/* how much of a vertex's weight its hands' nodes have for it to be a hand's */
#define HAND_VERTEX_WEIGHT 0.999f
/* how far past a triangle's edge a texel still counts as covered by it, in
texels */
#define TEXEL_COVER_MARGIN 0.001f

enum
{
	_model_geometry_part_stripped_bit = 0,
	_model_geometry_part_local_nodes_bit,
};

enum
{
	/* (bitmaps.c's _bitmap_type_2d and _bitmap_format_dxt1) */
	_marine_fingers_bitmap_type_2d = 0,
	_marine_fingers_bitmap_format_dxt1 = 14,
	/* (shaders.c's _shader_type_model) */
	_marine_fingers_shader_type_model = 4,
	/* (periodic_functions.c's _periodic_function_one) */
	_marine_fingers_periodic_function_one = 0,
	/* a triangle buffer's kinds (rasterizer_geometry.c) */
	_marine_fingers_triangle_list = 0,
	_marine_fingers_triangle_strip = 1,
};

/* ---------- structures */

/* this build's model geometry and part, as models.c and rasterizer.c each
define them for themselves (no header declares them) */
struct model_geometry
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct model_geometry_part
{
	unsigned long flags;
	short shader_index;
	char previous_part_index;
	char next_part_index;
	short centroid_primary_node_index;
	short centroid_secondary_node_index;
	real centroid_primary_node_weight;
	real centroid_secondary_node_weight;
	real_point3d centroid;
	struct tag_block uncompressed_vertices;
	struct tag_block compressed_vertices;
	struct tag_block triangles;
	struct triangle_buffer triangle_buffer;
	struct vertex_buffer vertex_buffer;
};

typedef char verify_marine_fingers_model_geometry_part_size[sizeof(struct model_geometry_part) == 0x68 ? 1 : -1];

/* a model's shader (model->shaders) */
struct model_shader_reference
{
	struct tag_reference shader;
	short permutation_index;
	word pad;
	long unused[3];
};

typedef char verify_marine_fingers_model_shader_reference_size[sizeof(struct model_shader_reference) == 0x20 ? 1 : -1];

/* what this reads of a model shader, as rasterizer_xbox_models.c defines it
for itself (struct shader_model_definition) */
struct marine_fingers_shader_model
{
	struct shader shader;
	byte reserved28[0x74];
	real map_u_scale;
	real map_v_scale;
	struct tag_reference base_map;
	byte reservedB4[8];
	struct tag_reference multipurpose_map;
	byte reservedCC[0x30];
	struct shader_texture_animation texture_animation;
};

typedef char verify_marine_fingers_shader_model_map_u_scale_offset[
	offsetof(struct marine_fingers_shader_model, map_u_scale) == 0x9C ? 1 : -1];
typedef char verify_marine_fingers_shader_model_multipurpose_map_offset[
	offsetof(struct marine_fingers_shader_model, multipurpose_map.index) == 0xC8 ? 1 : -1];
typedef char verify_marine_fingers_shader_model_texture_animation_offset[
	offsetof(struct marine_fingers_shader_model, texture_animation) == 0xFC ? 1 : -1];

/* a DXT1 block of the mask to clear: where it is in the pixels, the hands'
texels in it (bit 4 * row + column), and whether another triangle samples
any of its texels */
struct marine_fingers_block
{
	unsigned long offset;
	unsigned short texels;
	boolean shared;
};

/* ---------- globals */

static struct
{
	/* the mask (NONE: none to clear in the map loaded) and its first
	bitmap, as measured */
	long bitmap_index;
	struct bitmap_data const *bitmap;
	short width;
	short height;
	long pixels_size;
	/* the model, and its geometries as measured (a mod's model puts others
	in their place) */
	long model_index;
	void const *geometries;
	long geometry_count;
	short block_count;
	struct marine_fingers_block blocks[MARINE_FINGERS_MAXIMUM_BLOCKS];
} marine_fingers_globals = { NONE };

/* the texels of one level the hands' triangles sample, and the others' */
static unsigned long marine_fingers_hand_texels[MARINE_FINGERS_LEVEL_LONGS];
static unsigned long marine_fingers_other_texels[MARINE_FINGERS_LEVEL_LONGS];

/* ---------- private code */

/* whether the texel (x, y) of a level width texels wide is set */
static boolean texels_test(
	unsigned long const *texels,
	long width,
	long x,
	long y)
{
	long index = y * width + x;

	return (texels[index / LONG_BITS] & (1UL << (index % LONG_BITS))) != 0;
}

/* the texel (x, y) set, and its neighbours (bilinear filtering reads the
texels around where it samples), the level's edges wrapping */
static void texels_mark(
	unsigned long *texels,
	long width,
	long height,
	long x,
	long y)
{
	long dx;
	long dy;

	for (dy = -1; dy <= 1; dy++)
	{
		for (dx = -1; dx <= 1; dx++)
		{
			long index = (long)((unsigned long)(y + dy) & (unsigned long)(height - 1)) * width +
				(long)((unsigned long)(x + dx) & (unsigned long)(width - 1));

			texels[index / LONG_BITS] |= 1UL << (index % LONG_BITS);
		}
	}

	return;
}

/* whether the texel whose corner is (x, y) has any of the triangle (u, v, in
texels) on it: no edge of the triangle has the whole texel on its outside */
static boolean triangle_covers_texel(
	real const *u,
	real const *v,
	long x,
	long y)
{
	short edge;

	for (edge = 0; edge < 3; edge++)
	{
		short next = (short)((edge + 1) % 3);
		short other = (short)((edge + 2) % 3);
		real normal_u = v[edge] - v[next];
		real normal_v = u[next] - u[edge];
		real inside = normal_u * (u[other] - u[edge]) + normal_v * (v[other] - v[edge]);
		real margin = TEXEL_COVER_MARGIN * ((normal_u < 0.f ? -normal_u : normal_u) + (normal_v < 0.f ? -normal_v : normal_v));
		real least = 0.f;
		real most = 0.f;
		short corner;

		for (corner = 0; corner < 4; corner++)
		{
			real distance = normal_u * ((real)(x + (corner & 1)) - u[edge]) +
				normal_v * ((real)(y + (corner >> 1)) - v[edge]);

			if (!corner || distance < least)
				least = distance;
			if (!corner || distance > most)
				most = distance;
		}
		if ((inside > 0.f && most < -margin) || (inside < 0.f && least > margin))
			return FALSE;
	}

	return TRUE;
}

/* the texels a triangle (u, v, in texels of a level width by height) samples
marked; FALSE (none marked) when it spans more than the level twice over */
static boolean triangle_mark(
	unsigned long *texels,
	long width,
	long height,
	real const *u,
	real const *v)
{
	real least_u = MIN(u[0], MIN(u[1], u[2]));
	real most_u = MAX(u[0], MAX(u[1], u[2]));
	real least_v = MIN(v[0], MIN(v[1], v[2]));
	real most_v = MAX(v[0], MAX(v[1], v[2]));
	long first_x;
	long last_x;
	long first_y;
	long last_y;
	long x;
	long y;

	if (!(most_u - least_u <= (real)(2 * width)) || !(most_v - least_v <= (real)(2 * height)) ||
		!(least_u > -(real)(4 * MARINE_FINGERS_MAXIMUM_SIDE) && most_u < (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)) ||
		!(least_v > -(real)(4 * MARINE_FINGERS_MAXIMUM_SIDE) && most_v < (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)))
	{
		return FALSE;
	}
	/* (every texel the triangle's bounds touch, an edge included) */
	first_x = (long)(least_u + (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)) - 4 * MARINE_FINGERS_MAXIMUM_SIDE;
	last_x = (long)(most_u + (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)) - 4 * MARINE_FINGERS_MAXIMUM_SIDE;
	first_y = (long)(least_v + (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)) - 4 * MARINE_FINGERS_MAXIMUM_SIDE;
	last_y = (long)(most_v + (real)(4 * MARINE_FINGERS_MAXIMUM_SIDE)) - 4 * MARINE_FINGERS_MAXIMUM_SIDE;
	for (y = first_y; y <= last_y; y++)
	{
		for (x = first_x; x <= last_x; x++)
		{
			if (triangle_covers_texel(u, v, x, y))
				texels_mark(texels, width, height, x, y);
		}
	}

	return TRUE;
}

/* whether a shader's texture animation leaves its maps where its texture
coordinates are, to a whole texture (the maps wrap): each offset a constant
whole number, the turn a constant whole number of turns */
static boolean texture_animation_still(
	struct shader_texture_animation const *animation)
{
	real offsets[3];
	short axis;

	offsets[0] = animation->u_scale;
	offsets[1] = animation->v_scale;
	offsets[2] = animation->r_scale / 360.f;
	for (axis = 0; axis < 3; axis++)
	{
		short source = axis == 0 ? animation->u_source : axis == 1 ? animation->v_source : animation->r_source;
		short function = axis == 0 ? animation->u_function : axis == 1 ? animation->v_function : animation->r_function;

		if (offsets[axis] == 0.f)
			continue;
		if (source != 0 || function != _marine_fingers_periodic_function_one ||
			!(offsets[axis] > -1000.f && offsets[axis] < 1000.f) ||
			(real)(long)offsets[axis] != offsets[axis])
		{
			return FALSE;
		}
	}

	return TRUE;
}

/* the model shader of a model's shader reference, or NULL */
static struct marine_fingers_shader_model const *model_shader_get(
	struct model const *model,
	short shader_index)
{
	struct model_shader_reference const *reference;
	struct shader const *shader;

	if (!VALID_INDEX(shader_index, model->shaders.count) || !model->shaders.address)
		return NULL;
	reference = TAG_BLOCK_GET_ELEMENT(&model->shaders, shader_index, struct model_shader_reference);
	if (reference->shader.index == NONE)
		return NULL;
	shader = shader_definition_get(reference->shader.index);
	if (!shader || shader->base.type != _marine_fingers_shader_type_model)
		return NULL;

	return (struct marine_fingers_shader_model const *)shader;
}

/* a vertex's weight on the hands' nodes (hands: a flag a node) */
static real vertex_hand_weight(
	boolean const *hands,
	long node_count,
	long first_node,
	long second_node,
	real first_weight)
{
	real weight = 0.f;

	if (VALID_INDEX(first_node, node_count) && hands[first_node])
		weight += first_weight;
	if (VALID_INDEX(second_node, node_count) && hands[second_node])
		weight += 1.f - first_weight;

	return weight;
}

/* a part's triangles marked on the level's texels: the hands' (each of whose
vertices the hands move) on the hands', every other on the others'; FALSE
when one of the others can't be marked (the level is then not cleared) */
static boolean part_mark(
	struct model const *model,
	struct model_geometry_part const *part,
	boolean const *hands,
	real u_scale,
	real v_scale,
	long width,
	long height)
{
	struct vertex_buffer const *vertex_buffer = &part->vertex_buffer;
	struct triangle_buffer const *triangle_buffer = &part->triangle_buffer;
	byte *vertices = NULL;
	BYTE *indices = NULL;
	long stride;
	long index_count;
	long index;
	boolean marked = TRUE;

	if (TEST_FLAG(part->flags, _model_geometry_part_stripped_bit))
		return TRUE;
	if (!vertex_buffer->hardware_format || vertex_buffer->offset || vertex_buffer->count <= 0 ||
		vertex_buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART ||
		!triangle_buffer->hardware_format || triangle_buffer->count <= 0)
	{
		return TRUE;
	}
	if (vertex_buffer->type == _rasterizer_vertex_type_model_compressed)
		stride = sizeof(struct model_vertex_compressed);
	else if (vertex_buffer->type == _rasterizer_vertex_type_model_uncompressed)
		stride = sizeof(struct model_vertex_uncompressed);
	else
		return FALSE;
	if (triangle_buffer->type == _marine_fingers_triangle_strip)
		index_count = triangle_buffer->count + 2;
	else if (triangle_buffer->type == _marine_fingers_triangle_list)
		index_count = triangle_buffer->count * 3;
	else
		return FALSE;

	IDirect3DVertexBuffer8_Lock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);
	IDirect3DIndexBuffer8_Lock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format, 0, 0, &indices, D3DLOCK_READONLY);
	if (!vertices || !indices)
	{
		if (vertices)
			IDirect3DVertexBuffer8_Unlock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format);
		if (indices)
			IDirect3DIndexBuffer8_Unlock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format);
		return FALSE;
	}

	for (index = 0; index + 2 < index_count; index += (triangle_buffer->type == _marine_fingers_triangle_strip) ? 1 : 3)
	{
		unsigned short const *list = (unsigned short const *)indices;
		real u[3];
		real v[3];
		boolean hand = TRUE;
		boolean valid = TRUE;
		short corner;

		if (list[index] == list[index + 1] || list[index + 1] == list[index + 2] || list[index] == list[index + 2])
			continue;
		for (corner = 0; corner < 3; corner++)
		{
			long vertex_index = list[index + corner];
			real texcoord_u;
			real texcoord_v;
			real weight;

			if (vertex_index >= vertex_buffer->count)
			{
				valid = FALSE;
				break;
			}
			if (vertex_buffer->type == _rasterizer_vertex_type_model_compressed)
			{
				struct model_vertex_compressed const *vertex =
					(struct model_vertex_compressed const *)(vertices + vertex_index * stride);

				texcoord_u = (real)vertex->texcoord.x / MODEL_VERTEX_COMPRESSED_ONE;
				texcoord_v = (real)vertex->texcoord.y / MODEL_VERTEX_COMPRESSED_ONE;
				weight = vertex_hand_weight(hands, model->nodes.count, vertex->nodes[0] / 3, vertex->nodes[1] / 3,
					(real)vertex->node_weight / MODEL_VERTEX_COMPRESSED_ONE);
			}
			else
			{
				struct model_vertex_uncompressed const *vertex =
					(struct model_vertex_uncompressed const *)(vertices + vertex_index * stride);

				texcoord_u = vertex->texcoord.x;
				texcoord_v = vertex->texcoord.y;
				weight = vertex_hand_weight(hands, model->nodes.count, vertex->nodes[0], vertex->nodes[1],
					vertex->node_weights[0]);
			}
			/* (a part with its own node map, which this build's part doesn't
			carry, is no hand's) */
			if (TEST_FLAG(part->flags, _model_geometry_part_local_nodes_bit) || !(weight >= HAND_VERTEX_WEIGHT))
				hand = FALSE;
			u[corner] = texcoord_u * u_scale * (real)width;
			v[corner] = texcoord_v * v_scale * (real)height;
			if (!valid_real(u[corner]) || !valid_real(v[corner]))
				valid = FALSE;
		}
		if (!valid)
		{
			/* (a triangle that can't be placed: the level is not cleared) */
			marked = FALSE;
			break;
		}
		if (hand)
			triangle_mark(marine_fingers_hand_texels, width, height, u, v);
		else if (!triangle_mark(marine_fingers_other_texels, width, height, u, v))
		{
			marked = FALSE;
			break;
		}
	}

	IDirect3DVertexBuffer8_Unlock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format);
	IDirect3DIndexBuffer8_Unlock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format);

	return marked;
}

/* the blocks of a level (at offset in the pixels) whose texels the hands
sample and nothing else does, added; FALSE when they don't all fit */
static boolean level_blocks_add(
	long width,
	long height,
	unsigned long offset)
{
	long blocks_wide = MAX(1, width / 4);
	long blocks_high = MAX(1, height / 4);
	long block_x;
	long block_y;

	for (block_y = 0; block_y < blocks_high; block_y++)
	{
		for (block_x = 0; block_x < blocks_wide; block_x++)
		{
			unsigned short texels = 0;
			boolean shared = FALSE;
			short texel;

			for (texel = 0; texel < 16; texel++)
			{
				long x = block_x * 4 + (texel & 3);
				long y = block_y * 4 + (texel >> 2);

				if (x >= width || y >= height)
					continue;
				if (texels_test(marine_fingers_other_texels, width, x, y))
					shared = TRUE;
				else if (texels_test(marine_fingers_hand_texels, width, x, y))
					texels |= (unsigned short)(1 << texel);
			}
			if (texels)
			{
				struct marine_fingers_block *block;

				if (marine_fingers_globals.block_count >= MARINE_FINGERS_MAXIMUM_BLOCKS)
					return FALSE;
				block = &marine_fingers_globals.blocks[marine_fingers_globals.block_count++];
				block->offset = offset + (unsigned long)(block_y * blocks_wide + block_x) * 8;
				block->texels = texels;
				block->shared = shared;
			}
		}
	}

	return TRUE;
}

/* a DXT1 color's red, green or blue (0 to 255) */
static long dxt1_color_channel(
	unsigned short color,
	short channel)
{
	long value;

	if (channel == 0)
	{
		value = (color >> 11) & 0x1F;
		return (value << 3) | (value >> 2);
	}
	if (channel == 1)
	{
		value = (color >> 5) & 0x3F;
		return (value << 2) | (value >> 4);
	}
	value = color & 0x1F;
	return (value << 3) | (value >> 2);
}

/* a channel of the colors a DXT1 block's indices name (four: the block's
first color is the greater; else index 3 is transparent and named nothing) */
static void dxt1_palette_channel(
	unsigned short first,
	unsigned short second,
	short channel,
	long *palette)
{
	long a = dxt1_color_channel(first, channel);
	long b = dxt1_color_channel(second, channel);

	palette[0] = a;
	palette[1] = b;
	if (first > second)
	{
		palette[2] = (2 * a + b) / 3;
		palette[3] = (a + 2 * b) / 3;
	}
	else
	{
		palette[2] = (a + b) / 2;
		palette[3] = 0;
	}

	return;
}

/* a DXT1 block's red cleared: not shared, its colors' (every texel's, the
block kept in the mode it was); shared, each of texels (bit 4 * row +
column) takes the block's opaque color of the least red, of those the one
nearest its own in green and blue */
static void dxt1_block_clear(
	byte *block,
	unsigned short texels,
	boolean shared)
{
	unsigned short first = (unsigned short)(block[0] | (block[1] << 8));
	unsigned short second = (unsigned short)(block[2] | (block[3] << 8));
	unsigned long indices = (unsigned long)block[4] | ((unsigned long)block[5] << 8) |
		((unsigned long)block[6] << 16) | ((unsigned long)block[7] << 24);
	boolean four = first > second;
	short texel;

	if (!shared)
	{
		unsigned short new_first = (unsigned short)(first & 0x07FF);
		unsigned short new_second = (unsigned short)(second & 0x07FF);

		if (four && new_first < new_second)
		{
			/* (the colors swapped, to stay four: each index names the other
			of its pair) */
			unsigned short swap = new_first;

			new_first = new_second;
			new_second = swap;
			indices ^= 0x55555555UL;
		}
		else if (four && new_first == new_second)
		{
			/* (one color now: every texel names it) */
			indices = 0;
		}
		else if (!four && new_first > new_second)
		{
			/* (the colors swapped, to stay three: the first two indices
			swapped, the middle and transparent ones kept) */
			unsigned short swap = new_first;

			new_first = new_second;
			new_second = swap;
			for (texel = 0; texel < 16; texel++)
			{
				if (((indices >> (2 * texel)) & 3) < 2)
					indices ^= 1UL << (2 * texel);
			}
		}
		first = new_first;
		second = new_second;
	}
	else
	{
		long red[4];
		long green[4];
		long blue[4];
		short opaque = four ? 4 : 3;

		dxt1_palette_channel(first, second, 0, red);
		dxt1_palette_channel(first, second, 1, green);
		dxt1_palette_channel(first, second, 2, blue);
		for (texel = 0; texel < 16; texel++)
		{
			unsigned long current = (indices >> (2 * texel)) & 3;
			unsigned long best = current;
			short entry;

			if (!(texels & (1 << texel)) || (long)current >= opaque)
				continue;
			for (entry = 0; entry < opaque; entry++)
			{
				long distance = (green[entry] > green[current] ? green[entry] - green[current] : green[current] - green[entry]) +
					(blue[entry] > blue[current] ? blue[entry] - blue[current] : blue[current] - blue[entry]);
				long best_distance = (green[best] > green[current] ? green[best] - green[current] : green[current] - green[best]) +
					(blue[best] > blue[current] ? blue[best] - blue[current] : blue[current] - blue[best]);

				if (red[entry] < red[best] || (red[entry] == red[best] && distance < best_distance))
					best = (unsigned long)entry;
			}
			indices = (indices & ~(3UL << (2 * texel))) | (best << (2 * texel));
		}
	}

	block[0] = (byte)(first & 0xFF);
	block[1] = (byte)(first >> 8);
	block[2] = (byte)(second & 0xFF);
	block[3] = (byte)(second >> 8);
	block[4] = (byte)(indices & 0xFF);
	block[5] = (byte)((indices >> 8) & 0xFF);
	block[6] = (byte)((indices >> 16) & 0xFF);
	block[7] = (byte)((indices >> 24) & 0xFF);

	return;
}

/* whether a side is a power of two the masks hold */
static boolean side_valid(
	long side)
{
	return side > 0 && side <= MARINE_FINGERS_MAXIMUM_SIDE && !(side & (side - 1));
}

/* the mask's blocks to clear found, from the model's triangles drawn with
it; FALSE when it is not one to clear */
static boolean mask_measure(
	struct model const *model,
	boolean const *hands,
	long bitmap_index,
	struct bitmap_data const *bitmap)
{
	unsigned long offset = 0;
	short level;

	for (level = 0; level <= bitmap->mipmap_count; level++)
	{
		long width = MAX(1, bitmap->width >> level);
		long height = MAX(1, bitmap->height >> level);
		long geometry_index;

		csmemset(marine_fingers_hand_texels, 0, sizeof(marine_fingers_hand_texels));
		csmemset(marine_fingers_other_texels, 0, sizeof(marine_fingers_other_texels));
		for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
		{
			struct model_geometry const *geometry =
				TAG_BLOCK_GET_ELEMENT(&model->geometries, geometry_index, struct model_geometry);
			long part_index;

			if (geometry->parts.count < 0 || geometry->parts.count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY ||
				(geometry->parts.count && !geometry->parts.address))
			{
				return FALSE;
			}
			for (part_index = 0; part_index < geometry->parts.count; part_index++)
			{
				struct model_geometry_part const *part =
					TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part);
				struct marine_fingers_shader_model const *shader = model_shader_get(model, part->shader_index);

				if (!shader || shader->multipurpose_map.index != bitmap_index)
					continue;
				if (!texture_animation_still(&shader->texture_animation) ||
					!part_mark(model, part, hands, model->base_map_scale.i * shader->map_u_scale,
						model->base_map_scale.j * shader->map_v_scale, width, height))
				{
					return FALSE;
				}
			}
		}
		if (!level_blocks_add(width, height, offset))
			return FALSE;
		offset += (unsigned long)(MAX(1, width / 4) * MAX(1, height / 4) * 8);
	}

	return offset <= (unsigned long)bitmap->pixels_size;
}

/* the multipurpose map the model's hands are drawn with (one only), or NONE */
static long hands_mask_find(
	struct model const *model,
	boolean const *hands)
{
	long found = NONE;
	long geometry_index;

	for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
	{
		struct model_geometry const *geometry =
			TAG_BLOCK_GET_ELEMENT(&model->geometries, geometry_index, struct model_geometry);
		long part_index;

		if (geometry->parts.count < 0 || geometry->parts.count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY ||
			(geometry->parts.count && !geometry->parts.address))
		{
			return NONE;
		}
		for (part_index = 0; part_index < geometry->parts.count; part_index++)
		{
			struct model_geometry_part const *part =
				TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part);
			struct vertex_buffer const *buffer = &part->vertex_buffer;
			struct marine_fingers_shader_model const *shader = model_shader_get(model, part->shader_index);
			byte *vertices = NULL;
			boolean hand = FALSE;
			long index;

			if (!shader || shader->multipurpose_map.index == NONE ||
				TEST_FLAG(part->flags, _model_geometry_part_stripped_bit) ||
				TEST_FLAG(part->flags, _model_geometry_part_local_nodes_bit) ||
				buffer->type != _rasterizer_vertex_type_model_compressed || !buffer->hardware_format ||
				buffer->offset || buffer->count <= 0 || buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART)
			{
				continue;
			}
			IDirect3DVertexBuffer8_Lock((IDirect3DVertexBuffer8 *)buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);
			if (!vertices)
				continue;
			for (index = 0; index < buffer->count && !hand; index++)
			{
				struct model_vertex_compressed const *vertex = (struct model_vertex_compressed const *)vertices + index;

				hand = vertex_hand_weight(hands, model->nodes.count, vertex->nodes[0] / 3, vertex->nodes[1] / 3,
					(real)vertex->node_weight / MODEL_VERTEX_COMPRESSED_ONE) >= HAND_VERTEX_WEIGHT;
			}
			IDirect3DVertexBuffer8_Unlock((IDirect3DVertexBuffer8 *)buffer->hardware_format);
			if (hand)
			{
				if (found != NONE && found != shader->multipurpose_map.index)
					return NONE;
				found = shader->multipurpose_map.index;
			}
		}
	}

	return found;
}

/* whether a model other than model_index is drawn with the mask */
static boolean mask_drawn_by_another_model(
	long model_index,
	long bitmap_index)
{
	struct tag_iterator iterator;
	long other_index;

	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((other_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct model const *other = model_definition_get(other_index);
		long shader_index;

		if (other_index == model_index || !other || other->shaders.count <= 0 || !other->shaders.address)
			continue;
		for (shader_index = 0; shader_index < other->shaders.count && shader_index < 0x7FFF; shader_index++)
		{
			struct marine_fingers_shader_model const *shader = model_shader_get(other, (short)shader_index);

			if (shader && shader->multipurpose_map.index == bitmap_index)
				return TRUE;
		}
	}

	return FALSE;
}

/* ---------- public code */

/* (scenario_tags_load, an Xbox map's tags loaded) */
void marine_fingers_tags_loaded(
	void)
{
	static boolean hands[MAXIMUM_NODES_PER_MODEL];
	long model_index;
	long bitmap_index;
	struct model const *model;
	struct bitmap_group const *group;
	struct bitmap_data const *bitmap;
	short node_index;

	marine_fingers_globals.bitmap_index = NONE;
	marine_fingers_globals.bitmap = NULL;
	marine_fingers_globals.block_count = 0;
	if (custom_edition_cache_tags_loaded() || mcc_cache_tags_loaded())
		return;
	model_index = tag_loaded(MODELS_GROUP_TAG, MARINE_MODEL_NAME);
	if (model_index == NONE)
		return;
	model = model_definition_get(model_index);
	if (!model || !model->nodes.address || model->nodes.count <= 0 || model->nodes.count > MAXIMUM_NODES_PER_MODEL ||
		!model->geometries.address || model->geometries.count <= 0 ||
		model->geometries.count > MAXIMUM_GEOMETRIES_PER_MODEL || !model->shaders.address || model->shaders.count <= 0)
	{
		return;
	}
	for (node_index = 0; node_index < model->nodes.count; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);
		char name[TAG_STRING_LENGTH + 1];

		csstrncpy(name, node->name, TAG_STRING_LENGTH);
		name[TAG_STRING_LENGTH] = 0;
		hands[node_index] = strstr(name, HAND_NODE_NAME_TEXT) != NULL;
	}

	bitmap_index = hands_mask_find(model, hands);
	if (bitmap_index == NONE || mask_drawn_by_another_model(model_index, bitmap_index))
		return;
	group = bitmap_group_get(bitmap_index);
	if (!group || group->bitmaps.count <= 0 || !group->bitmaps.address)
		return;
	bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
	if (bitmap->type != _marine_fingers_bitmap_type_2d || bitmap->format != _marine_fingers_bitmap_format_dxt1 ||
		!side_valid(bitmap->width) || !side_valid(bitmap->height) || bitmap->mipmap_count < 0 ||
		bitmap->mipmap_count > 15 || bitmap->pixels_size <= 0)
	{
		return;
	}
	if (!mask_measure(model, hands, bitmap_index, bitmap) || !marine_fingers_globals.block_count)
	{
		marine_fingers_globals.block_count = 0;
		return;
	}

	marine_fingers_globals.bitmap_index = bitmap_index;
	marine_fingers_globals.bitmap = bitmap;
	marine_fingers_globals.width = bitmap->width;
	marine_fingers_globals.height = bitmap->height;
	marine_fingers_globals.pixels_size = bitmap->pixels_size;
	marine_fingers_globals.model_index = model_index;
	marine_fingers_globals.geometries = model->geometries.address;
	marine_fingers_globals.geometry_count = model->geometries.count;

	return;
}

/* (xbox_texture_cache.c, a bitmap's pixels loaded, before they are drawn) */
void marine_fingers_bitmap_loaded(
	struct bitmap_data *bitmap)
{
	struct model const *model;
	byte *pixels;
	short index;

	if (!bitmap || bitmap != marine_fingers_globals.bitmap || marine_fingers_globals.bitmap_index == NONE)
		return;
	model = model_definition_get(marine_fingers_globals.model_index);
	pixels = (byte *)bitmap->base_address;
	if (!pixels || mod_bitmaps_contain(bitmap) || !model ||
		model->geometries.address != marine_fingers_globals.geometries ||
		model->geometries.count != marine_fingers_globals.geometry_count ||
		bitmap->width != marine_fingers_globals.width || bitmap->height != marine_fingers_globals.height ||
		bitmap->format != _marine_fingers_bitmap_format_dxt1 || bitmap->pixels_size != marine_fingers_globals.pixels_size)
	{
		return;
	}

	for (index = 0; index < marine_fingers_globals.block_count; index++)
	{
		struct marine_fingers_block const *block = &marine_fingers_globals.blocks[index];

		dxt1_block_clear(pixels + block->offset, block->texels, block->shared);
	}

	return;
}
