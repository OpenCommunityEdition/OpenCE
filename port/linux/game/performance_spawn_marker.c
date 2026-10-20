/* A classic map has no scenery\spawn_marker_nhe. A hardcore match draws
the pad shipped with the game on the player starts. The user's
hardcore_maps prisoner.map is used when that copy is not in the game.
Without either, the line markers remain. */

#include "cseries.h"
#include "hardcore_assets.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_models.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "shaders/shader_definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void platform_log(char const *format, ...);

#define MARKER_TAG_BASE 0x40440000UL
#define MARKER_TAG_MAXIMUM 0x1700000UL
#define MARKER_VERTICES_MAXIMUM 512
#define MARKER_INDICES_MAXIMUM 1024
#define MARKER_STATE_NONE 0
#define MARKER_STATE_FAILED 1
#define MARKER_STATE_READY 2
/* An open tube. One extra side closes the texture without a seam. */
#define MARKER_COLUMN_SIDES 32
#define MARKER_COLUMN_RINGS 13
#define MARKER_BEACON_VERTICES ((MARKER_COLUMN_SIDES + 1) * MARKER_COLUMN_RINGS)
#define MARKER_BEACON_TRIANGLES (MARKER_COLUMN_SIDES * (MARKER_COLUMN_RINGS - 1) * 2)
#define MARKER_PLASMA_SIZE 64

typedef char marker_pointer_is_tag_sized[sizeof(void *) == 4 ? 1 : -1];
typedef char marker_bitmap_is_tag_sized[sizeof(struct bitmap_data) == 0x30 ? 1 : -1];
typedef char marker_vertex_is_model_sized[sizeof(struct model_vertex_uncompressed) == 0x44 ? 1 : -1];

static int marker_state;
static int marker_logged;
static struct bitmap_group marker_bitmap_group;
static struct bitmap_data marker_bitmap;
static unsigned char *marker_pixels;
static long marker_pixel_bytes;
static struct vertex_buffer marker_vertices;
static struct triangle_buffer marker_triangles;
static long marker_triangle_count;
static real_point3d marker_centroid;
static real marker_radius;
static unsigned char marker_shader[0x6C] __attribute__((aligned(8)));
static unsigned char marker_shader_map[2][0xDC] __attribute__((aligned(8)));

/* The game's free halts on a null pointer. Reset runs at startup, before
a pad has been loaded. */
static void marker_free(void *pointer)
{
	if (pointer)
		free(pointer);
}

static unsigned long marker_u32(unsigned char const *bytes)
{
	return (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
}

static long marker_s32(unsigned char const *bytes)
{
	return (long)marker_u32(bytes);
}

static short marker_s16(unsigned char const *bytes)
{
	return (short)((unsigned short)bytes[0] | ((unsigned short)bytes[1] << 8));
}

static int marker_span(unsigned long offset, unsigned long size, unsigned long limit)
{
	return size > 0 && offset < limit && size <= limit - offset;
}

static int marker_tag_place(unsigned long virtual_address, unsigned long tag_size, unsigned long *offset)
{
	if (virtual_address < MARKER_TAG_BASE)
		return 0;
	*offset = virtual_address - MARKER_TAG_BASE;
	return *offset < tag_size;
}

static int marker_name_is(unsigned char const *tag, unsigned long tag_size,
	unsigned long virtual_address, char const *name)
{
	unsigned long offset;
	unsigned long length = 0;
	unsigned long expect;

	if (!name || !marker_tag_place(virtual_address, tag_size, &offset))
		return 0;
	expect = (unsigned long)strlen(name);
	while (offset + length < tag_size && tag[offset + length] && length < 256)
		length++;
	if (offset + length >= tag_size || tag[offset + length] || length != expect)
		return 0;
	return memcmp(tag + offset, name, length) == 0;
}

static int marker_find(unsigned char const *tag, unsigned long tag_size,
	char const *name, unsigned long *address)
{
	unsigned long table;
	unsigned long count;
	unsigned long index;

	if (tag_size < 0x28)
		return 0;
	count = marker_u32(tag + 0x0C);
	if (count == 0 || count > 65535 || !marker_tag_place(marker_u32(tag), tag_size, &table) ||
		!marker_span(table, count * 0x20UL, tag_size))
		return 0;
	for (index = 0; index < count; index++)
	{
		unsigned char const *instance = tag + table + index * 0x20UL;

		if (marker_name_is(tag, tag_size, marker_u32(instance + 0x10), name))
		{
			*address = marker_u32(instance + 0x14);
			return 1;
		}
	}
	return 0;
}

static unsigned long marker_pixel_span(short width, short height, short mipmaps)
{
	unsigned long total = 0;
	short level;

	if (width < 1 || height < 1 || mipmaps < 0)
		return 0;
	for (level = 0; level <= mipmaps; level++)
	{
		long mip_width = width >> level;
		long mip_height = height >> level;

		if (mip_width < 1)
			mip_width = 1;
		if (mip_height < 1)
			mip_height = 1;
		total += (unsigned long)mip_width * (unsigned long)mip_height * 4UL;
	}
	return total;
}

static int marker_power_of_two(short value)
{
	return value > 0 && (value & (value - 1)) == 0;
}

/* Two scrolling copies of one noise, the way the menu plasma is built.
Double multiply keeps the interference bright. Slide is periodic function 6. */
static void marker_map_write(unsigned char *map, long bitmap_tag, short function,
	float u_period, float v_period, float u_scroll, float v_scroll)
{
	unsigned long group = BITMAP_GROUP_TAG;
	float scale = 2.0f;
	short slide = 6;
	short none = 0;

	memset(map, 0, 0xDC);
	memcpy(map + 0x2C, &function, sizeof(function));
	memcpy(map + 0x54, &scale, sizeof(scale));
	memcpy(map + 0x58, &scale, sizeof(scale));
	memcpy(map + 0x6C, &group, sizeof(group));
	memcpy(map + 0x78, &bitmap_tag, sizeof(bitmap_tag));
	memcpy(map + 0xA4, &none, sizeof(none));
	memcpy(map + 0xA6, &slide, sizeof(slide));
	memcpy(map + 0xA8, &u_period, sizeof(u_period));
	memcpy(map + 0xB0, &u_scroll, sizeof(u_scroll));
	memcpy(map + 0xB4, &none, sizeof(none));
	memcpy(map + 0xB6, &slide, sizeof(slide));
	memcpy(map + 0xB8, &v_period, sizeof(v_period));
	memcpy(map + 0xC0, &v_scroll, sizeof(v_scroll));
}

static void marker_shader_set(long bitmap_tag)
{
	long count = 2;
	void *maps = marker_shader_map;
	short type = 6;
	short blend = 0;

	/* Chicago is type 6. Alpha blend uses the texture alpha, so the mark
	stays a bit see-through. Bit 2 draws both sides, bit 4 draws before water. */
	memset(marker_shader, 0, sizeof(marker_shader));
	memcpy(marker_shader + 0x24, &type, sizeof(type));
	marker_shader[0x29] = 0x14;
	memcpy(marker_shader + 0x2C, &blend, sizeof(blend));
	memcpy(marker_shader + 0x54, &count, sizeof(count));
	memcpy(marker_shader + 0x58, &maps, sizeof(maps));
	/* The menu's two layers drift at about these rates, one against the other. */
	marker_map_write(marker_shader_map[0], bitmap_tag, 3, 31.1f, 37.3f, 1.0f, 1.0f);
	marker_map_write(marker_shader_map[1], bitmap_tag, 0, 28.1f, 32.2f, -1.0f, -1.0f);
}

static unsigned int marker_hash(int x, int y)
{
	unsigned int n = (unsigned int)(x * 374761393 + y * 668265263);

	n = (n ^ (n >> 13)) * 1274126177u;
	return n ^ (n >> 16);
}

static int marker_lattice(int x, int y)
{
	x &= 7;
	y &= 7;
	return (int)(marker_hash(x, y) & 255);
}

/* Wrapped so the scroll has no seam. x and y are in eighths of a cell. */
static int marker_noise(int x, int y)
{
	int x0 = x >> 3;
	int y0 = y >> 3;
	int fx = x & 7;
	int fy = y & 7;
	int v00 = marker_lattice(x0, y0);
	int v10 = marker_lattice(x0 + 1, y0);
	int v01 = marker_lattice(x0, y0 + 1);
	int v11 = marker_lattice(x0 + 1, y0 + 1);
	int v0 = v00 + ((v10 - v00) * fx) / 8;
	int v1 = v01 + ((v11 - v01) * fx) / 8;

	return v0 + ((v1 - v0) * fy) / 8;
}

/* x8r8g8b8 is blue, green, red. The menu plasma is noise, not a flat
color. Green carries it, and the alpha leaves the disc see-through. */
static int marker_plasma_fill(void)
{
	unsigned char *pixels;
	long bytes;
	int x;
	int y;

	bytes = MARKER_PLASMA_SIZE * MARKER_PLASMA_SIZE * 4;
	pixels = (unsigned char *)malloc((unsigned long)bytes);
	if (!pixels)
		return 0;
	for (y = 0; y < MARKER_PLASMA_SIZE; y++)
	{
		for (x = 0; x < MARKER_PLASMA_SIZE; x++)
		{
			int coarse = marker_noise(x, y);
			int fine = marker_noise(x * 2, y * 2);
			int n = (coarse * 2 + fine) / 3;
			unsigned char *pixel = pixels + (y * MARKER_PLASMA_SIZE + x) * 4;
			unsigned int green = 70u + ((unsigned int)n * 185u) / 255u;
			unsigned int alpha = 55u + ((unsigned int)n * 45u) / 255u;

			pixel[0] = 0;
			pixel[1] = (unsigned char)green;
			pixel[2] = 0;
			pixel[3] = (unsigned char)alpha;
		}
	}
	marker_free(marker_pixels);
	marker_pixels = pixels;
	marker_pixel_bytes = bytes;
	marker_bitmap.width = MARKER_PLASMA_SIZE;
	marker_bitmap.height = MARKER_PLASMA_SIZE;
	marker_bitmap.depth = 1;
	marker_bitmap.type = 0;
	/* Format 11 is a8r8g8b8. Format 10 drops the alpha byte. */
	marker_bitmap.format = 11;
	marker_bitmap.mipmap_count = 0;
	marker_bitmap.pixels_size = bytes;
	return 1;
}

static void marker_set_vertex(struct model_vertex_uncompressed *vertex,
	real x, real y, real z, real nx, real ny, real nz, real u, real v)
{
	memset(vertex, 0, sizeof(*vertex));
	vertex->position.x = x;
	vertex->position.y = y;
	vertex->position.z = z;
	vertex->normal.i = nx;
	vertex->normal.j = ny;
	vertex->normal.k = nz;
	vertex->tangent.i = 1.0f;
	vertex->binormal.j = 1.0f;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
	vertex->node_weights[0] = 1.0f;
}

/* Wide at both ends and narrow through the middle. The floor stays put. */
static long marker_build_beacon(struct model_vertex_uncompressed *vertices,
	unsigned short *list)
{
	long ring;
	long side;
	long triangles;
	long stride;
	real floor_z;
	real height;
	real flare;
	real waist;
	real turn;

	if (!vertices || !list)
		return 0;
	floor_z = 0.05f;
	height = 0.31f;
	flare = 0.16f;
	waist = 0.065f;
	turn = 6.2831855f;
	stride = MARKER_COLUMN_SIDES + 1;
	triangles = 0;
	for (ring = 0; ring < MARKER_COLUMN_RINGS; ring++)
	{
		real t = (real)ring / (real)(MARKER_COLUMN_RINGS - 1);
		real wave = cosine(turn * t);
		real radius = waist + (flare - waist) * 0.5f * (1.0f + wave);
		real n_radial = height;
		real n_up = (flare - waist) * 3.1415927f * sine(turn * t);
		real n_length = square_root(n_radial * n_radial + n_up * n_up);
		real z = floor_z + t * height;

		n_radial /= n_length;
		n_up /= n_length;
		for (side = 0; side <= MARKER_COLUMN_SIDES; side++)
		{
			real u = (real)side / (real)MARKER_COLUMN_SIDES;
			real angle = turn * u;
			real around = cosine(angle);
			real across = sine(angle);

			marker_set_vertex(&vertices[ring * stride + side],
				around * radius, across * radius, z,
				around * n_radial, across * n_radial, n_up, u, t);
		}
	}
	for (ring = 0; ring < MARKER_COLUMN_RINGS - 1; ring++)
	{
		for (side = 0; side < MARKER_COLUMN_SIDES; side++)
		{
			long next = side + 1;
			long at = triangles * 3;
			long base = ring * stride;

			list[at++] = (unsigned short)(base + side);
			list[at++] = (unsigned short)(base + next);
			list[at++] = (unsigned short)(base + stride + side);
			list[at++] = (unsigned short)(base + next);
			list[at++] = (unsigned short)(base + stride + next);
			list[at++] = (unsigned short)(base + stride + side);
			triangles += 2;
		}
	}
	return triangles;
}

/* The mesh and the 32-bit texture are already in memory. */
static int marker_commit(struct model_vertex_uncompressed *uncompressed, long vertex_count,
	unsigned short *indices, long index_count)
{
	struct model_vertex_uncompressed *drawn = NULL;
	struct model_vertex_compressed *compressed = NULL;
	unsigned short *column = NULL;
	long triangle_count;
	long column_triangles;
	long drawn_count;
	long index;
	long bitmap_tag;
	short width;
	short height;
	short mipmaps;
	unsigned long pixel_bytes;
	int ready = 0;

	triangle_count = index_count - 2;
	width = marker_bitmap.width;
	height = marker_bitmap.height;
	mipmaps = marker_bitmap.mipmap_count;
	pixel_bytes = marker_pixel_span(width, height, mipmaps);
	if (triangle_count < 1 || vertex_count < 3 || !uncompressed || !indices || !marker_pixels ||
		marker_bitmap.signature != BITMAP_GROUP_TAG || marker_bitmap.type != 0 ||
		marker_bitmap.format != 10 || marker_bitmap.depth != 1 ||
		!marker_power_of_two(width) || !marker_power_of_two(height) ||
		pixel_bytes == 0 || pixel_bytes > 1024UL * 1024UL ||
		(unsigned long)marker_bitmap.pixels_size != pixel_bytes ||
		marker_pixel_bytes != (long)pixel_bytes)
		return 0;
	for (index = 0; index < index_count; index++)
	{
		if (indices[index] >= (unsigned short)vertex_count)
			return 0;
	}
	drawn_count = MARKER_BEACON_VERTICES;
	drawn = (struct model_vertex_uncompressed *)malloc(
		(unsigned long)drawn_count * sizeof(*drawn));
	column = (unsigned short *)malloc(
		(unsigned long)MARKER_BEACON_TRIANGLES * 3UL * sizeof(*column));
	compressed = (struct model_vertex_compressed *)malloc(
		(unsigned long)drawn_count * sizeof(*compressed));
	if (!drawn || !column || !compressed)
		goto done;
	column_triangles = marker_build_beacon(drawn, column);
	if (column_triangles != MARKER_BEACON_TRIANGLES)
		goto done;
	if (!marker_plasma_fill())
		goto done;
	marker_centroid.x = marker_centroid.y = marker_centroid.z = 0.0f;
	for (index = 0; index < column_triangles * 3; index++)
	{
		struct model_vertex_uncompressed *vertex = &drawn[column[index]];

		marker_centroid.x += vertex->position.x;
		marker_centroid.y += vertex->position.y;
		marker_centroid.z += vertex->position.z;
	}
	marker_centroid.x /= (real)(column_triangles * 3);
	marker_centroid.y /= (real)(column_triangles * 3);
	marker_centroid.z /= (real)(column_triangles * 3);
	/* The mesh is well under a world unit. This only keeps the transparent
	sort from treating the pad as a point. */
	marker_radius = 0.5f;
	rasterizer_geometry_compress_vertices(
		_rasterizer_vertex_type_model_uncompressed,
		drawn_count,
		compressed,
		drawn_count * (long)sizeof(*compressed),
		drawn,
		drawn_count * (long)sizeof(*drawn));
	memset(&marker_vertices, 0, sizeof(marker_vertices));
	memset(&marker_triangles, 0, sizeof(marker_triangles));
	if (!rasterizer_vertex_buffer_new(&marker_vertices, _rasterizer_vertex_type_model_compressed,
			drawn_count, compressed, drawn_count * (long)sizeof(*compressed)) ||
		!rasterizer_triangle_buffer_new(&marker_triangles, _triangle_buffer_type_triangles,
			column_triangles, column))
		goto done;
	marker_triangle_count = column_triangles;
	/* The cache bit is runtime. The file's copy would make the texture
	cache refuse to load these pixels. */
	marker_bitmap.flags = (unsigned short)(marker_bitmap.flags & ~(1u << 7) & ~(1u << 6) & ~(1u << 3));
	marker_bitmap.pixels_offset = 0;
	marker_bitmap.pixels_size = marker_pixel_bytes;
	marker_bitmap.tag_index = NONE;
	marker_bitmap.cache_block_index = NONE;
	marker_bitmap.hardware_format = NULL;
	marker_bitmap.base_address = NULL;
	memset(&marker_bitmap_group, 0, sizeof(marker_bitmap_group));
	marker_bitmap_group.bitmaps.count = 1;
	marker_bitmap_group.bitmaps.address = &marker_bitmap;
	bitmap_tag = cache_files_register_runtime_ui_tag(BITMAP_GROUP_TAG,
		"scenery\\spawn_marker_nhe\\bitmaps\\spawn_marker_nhe", &marker_bitmap_group);
	if (bitmap_tag == NONE)
		goto done;
	texture_cache_bitmap_new(bitmap_tag, &marker_bitmap);
	if (marker_bitmap.pixels_size != marker_pixel_bytes || marker_bitmap.pixels_offset != 0)
		goto done;
	marker_shader_set(bitmap_tag);
	ready = 1;

done:
	marker_free(drawn);
	marker_free(column);
	marker_free(compressed);
	return ready;
}

/* 1 when the pad shipped with the game is ready, 0 when it is absent,
-1 when that copy is there and cannot be used. */
static int marker_from_pack(void)
{
	unsigned char const *payload;
	unsigned long size;
	unsigned long vertex_count;
	unsigned long index_count;
	unsigned long pixel_bytes;
	unsigned long mesh_at;
	struct model_vertex_uncompressed *uncompressed = NULL;
	unsigned short *indices = NULL;
	int ready = 0;

	if (!hardcore_assets_find(HARDCORE_ASSET_MARKER, "marker", &payload, &size))
		return 0;
	if (size < 12UL + 0x30UL)
		goto done;
	vertex_count = marker_u32(payload);
	index_count = marker_u32(payload + 4);
	pixel_bytes = marker_u32(payload + 8);
	if (vertex_count < 3 || vertex_count > MARKER_VERTICES_MAXIMUM ||
		index_count < 3 || index_count > MARKER_INDICES_MAXIMUM ||
		pixel_bytes == 0 || pixel_bytes > 1024UL * 1024UL)
		goto done;
	mesh_at = 12UL + 0x30UL;
	if (size < mesh_at + vertex_count * 0x44UL + index_count * 2UL + pixel_bytes)
		goto done;
	uncompressed = (struct model_vertex_uncompressed *)malloc(vertex_count * sizeof(*uncompressed));
	indices = (unsigned short *)malloc(index_count * sizeof(*indices));
	marker_pixels = (unsigned char *)malloc(pixel_bytes);
	if (!uncompressed || !indices || !marker_pixels)
		goto done;
	memcpy(&marker_bitmap, payload + 12, sizeof(marker_bitmap));
	memcpy(uncompressed, payload + mesh_at, vertex_count * 0x44UL);
	memcpy(indices, payload + mesh_at + vertex_count * 0x44UL, index_count * 2UL);
	memcpy(marker_pixels, payload + mesh_at + vertex_count * 0x44UL + index_count * 2UL, pixel_bytes);
	marker_pixel_bytes = (long)pixel_bytes;
	if (marker_commit(uncompressed, (long)vertex_count, indices, (long)index_count))
		ready = 1;

done:
	marker_free(uncompressed);
	marker_free(indices);
	if (!ready)
	{
		marker_free(marker_pixels);
		marker_pixels = NULL;
		marker_pixel_bytes = 0;
		rasterizer_vertex_buffer_delete(&marker_vertices);
		rasterizer_triangle_buffer_delete(&marker_triangles);
		memset(&marker_vertices, 0, sizeof(marker_vertices));
		memset(&marker_triangles, 0, sizeof(marker_triangles));
		return -1;
	}
	return 1;
}

static void marker_prepare(void)
{
	FILE *file = NULL;
	unsigned char *tag = NULL;
	struct model_vertex_uncompressed *uncompressed = NULL;
	struct model_vertex_compressed *compressed = NULL;
	unsigned short *indices = NULL;
	unsigned short *column = NULL;
	unsigned char header[0x18];
	unsigned char part[0x84];
	unsigned long file_size;
	unsigned long tag_offset;
	unsigned long tag_size;
	unsigned long vertex_file;
	unsigned long index_relative;
	unsigned long model_address;
	unsigned long model_offset;
	unsigned long bitmap_address;
	unsigned long bitmap_offset;
	unsigned long geometry_address;
	unsigned long geometry_offset;
	long geometry_count;
	long geometry_index;
	long best_vertices = 0;
	long vertex_count = 0;
	long triangle_count = 0;
	long column_triangles = 0;
	long drawn_count = 0;
	unsigned long vertex_at = 0;
	unsigned long index_at = 0;
	long bitmap_tag;
	long index;
	long told;
	int found_part = 0;
	int packed;

	packed = marker_from_pack();
	if (packed > 0)
	{
		marker_state = MARKER_STATE_READY;
		return;
	}
	if (packed < 0)
		goto fail;
	file = fopen("d:\\hardcore_maps\\prisoner.map", "rb");
	if (!file)
		goto fail;
	if (fseek(file, 0, SEEK_END) != 0 || (told = ftell(file)) < 0x800)
		goto fail;
	file_size = (unsigned long)told;
	if (fseek(file, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), file) != sizeof(header) ||
		header[0] != 'd' || header[1] != 'a' || header[2] != 'e' || header[3] != 'h' ||
		marker_s32(header + 4) != 7)
		goto fail;
	tag_offset = marker_u32(header + 0x10);
	tag_size = marker_u32(header + 0x14);
	if (tag_offset < 0x800 || tag_size < 0x28 || tag_size > MARKER_TAG_MAXIMUM ||
		!marker_span(tag_offset, tag_size, file_size))
		goto fail;
	tag = (unsigned char *)malloc(tag_size);
	if (!tag || fseek(file, (long)tag_offset, SEEK_SET) != 0 ||
		fread(tag, 1, tag_size, file) != tag_size)
		goto fail;
	vertex_file = marker_u32(tag + 0x14);
	index_relative = marker_u32(tag + 0x1C);
	if (!marker_find(tag, tag_size, "scenery\\spawn_marker_nhe\\overflow_spawn_marker", &model_address) ||
		!marker_find(tag, tag_size, "scenery\\spawn_marker_nhe\\bitmaps\\spawn_marker_nhe", &bitmap_address) ||
		!marker_tag_place(model_address, tag_size, &model_offset) ||
		!marker_span(model_offset, 0xE8, tag_size))
		goto fail;
	geometry_count = marker_s32(tag + model_offset + 0xD0);
	geometry_address = marker_u32(tag + model_offset + 0xD4);
	if (geometry_count < 1 || geometry_count > 8 ||
		!marker_tag_place(geometry_address, tag_size, &geometry_offset) ||
		!marker_span(geometry_offset, (unsigned long)geometry_count * 0x30UL, tag_size))
		goto fail;
	for (geometry_index = 0; geometry_index < geometry_count; geometry_index++)
	{
		unsigned char const *geometry = tag + geometry_offset + (unsigned long)geometry_index * 0x30UL;
		long part_count = marker_s32(geometry + 0x24);
		unsigned long part_address = marker_u32(geometry + 0x28);
		unsigned long part_offset;
		long part_vertices;
		long part_triangles;

		if (part_count < 1 || !marker_tag_place(part_address, tag_size, &part_offset) ||
			!marker_span(part_offset, 0x84, tag_size))
			continue;
		part_vertices = marker_s32(tag + part_offset + 0x58);
		part_triangles = marker_s32(tag + part_offset + 0x48);
		if (part_vertices < 3 || part_vertices > MARKER_VERTICES_MAXIMUM ||
			part_triangles < 1 || part_triangles + 2 > MARKER_INDICES_MAXIMUM ||
			marker_s16(tag + part_offset + 0x44) != 1 ||
			marker_s16(tag + part_offset + 0x54) != 4)
			continue;
		if (part_vertices > best_vertices)
		{
			best_vertices = part_vertices;
			memcpy(part, tag + part_offset, sizeof(part));
			found_part = 1;
		}
	}
	if (!found_part)
		goto fail;
	vertex_count = marker_s32(part + 0x58);
	triangle_count = marker_s32(part + 0x48);
	if (vertex_file > file_size || index_relative > file_size - vertex_file ||
		marker_u32(part + 0x64) > file_size - vertex_file ||
		marker_u32(part + 0x4C) > file_size - (vertex_file + index_relative))
		goto fail;
	vertex_at = vertex_file + marker_u32(part + 0x64);
	index_at = vertex_file + index_relative + marker_u32(part + 0x4C);
	if (!marker_span(vertex_at, (unsigned long)vertex_count * 0x44UL, file_size) ||
		!marker_span(index_at, (unsigned long)(triangle_count + 2) * 2UL, file_size))
		goto fail;
	uncompressed = (struct model_vertex_uncompressed *)malloc(
		(unsigned long)vertex_count * sizeof(*uncompressed));
	compressed = (struct model_vertex_compressed *)malloc(
		(unsigned long)vertex_count * sizeof(*compressed));
	indices = (unsigned short *)malloc((unsigned long)(triangle_count + 2) * sizeof(*indices));
	if (!uncompressed || !compressed || !indices ||
		fseek(file, (long)vertex_at, SEEK_SET) != 0 ||
		fread(uncompressed, sizeof(*uncompressed), (size_t)vertex_count, file) != (size_t)vertex_count ||
		fseek(file, (long)index_at, SEEK_SET) != 0 ||
		fread(indices, sizeof(*indices), (size_t)(triangle_count + 2), file) != (size_t)(triangle_count + 2))
		goto fail;
	for (index = 0; index < triangle_count + 2; index++)
	{
		if (indices[index] >= (unsigned short)vertex_count)
			goto fail;
	}
	marker_free(uncompressed);
	marker_free(compressed);
	uncompressed = NULL;
	compressed = NULL;
	drawn_count = MARKER_BEACON_VERTICES;
	uncompressed = (struct model_vertex_uncompressed *)malloc(
		(unsigned long)drawn_count * sizeof(*uncompressed));
	compressed = (struct model_vertex_compressed *)malloc(
		(unsigned long)drawn_count * sizeof(*compressed));
	column = (unsigned short *)malloc(
		(unsigned long)MARKER_BEACON_TRIANGLES * 3UL * sizeof(*column));
	if (!uncompressed || !compressed || !column)
		goto fail;
	column_triangles = marker_build_beacon(uncompressed, column);
	if (column_triangles != MARKER_BEACON_TRIANGLES)
		goto fail;
	marker_centroid.x = marker_centroid.y = marker_centroid.z = 0.0f;
	for (index = 0; index < column_triangles * 3; index++)
	{
		struct model_vertex_uncompressed *vertex = &uncompressed[column[index]];

		marker_centroid.x += vertex->position.x;
		marker_centroid.y += vertex->position.y;
		marker_centroid.z += vertex->position.z;
	}
	marker_centroid.x /= (real)(column_triangles * 3);
	marker_centroid.y /= (real)(column_triangles * 3);
	marker_centroid.z /= (real)(column_triangles * 3);
	/* The mesh is well under a world unit. This only keeps the transparent
	sort from treating the pad as a point. */
	marker_radius = 0.5f;
	rasterizer_geometry_compress_vertices(
		_rasterizer_vertex_type_model_uncompressed,
		drawn_count,
		compressed,
		drawn_count * (long)sizeof(*compressed),
		uncompressed,
		drawn_count * (long)sizeof(*uncompressed));
	memset(&marker_vertices, 0, sizeof(marker_vertices));
	memset(&marker_triangles, 0, sizeof(marker_triangles));
	if (!rasterizer_vertex_buffer_new(&marker_vertices, _rasterizer_vertex_type_model_compressed,
			drawn_count, compressed, drawn_count * (long)sizeof(*compressed)) ||
		!rasterizer_triangle_buffer_new(&marker_triangles, _triangle_buffer_type_triangles,
			column_triangles, column))
		goto fail;
	marker_triangle_count = column_triangles;

	if (!marker_tag_place(bitmap_address, tag_size, &bitmap_offset) ||
		!marker_span(bitmap_offset, 0x6C, tag_size))
		goto fail;
	{
		unsigned char const *group = tag + bitmap_offset;
		long bitmap_count = marker_s32(group + 0x60);
		unsigned long bitmaps_address = marker_u32(group + 0x64);
		unsigned long bitmaps_offset;
		unsigned long pixel_file;
		unsigned long pixel_bytes;
		short width;
		short height;
		short mipmaps;

		if (bitmap_count < 1 || !marker_tag_place(bitmaps_address, tag_size, &bitmaps_offset) ||
			!marker_span(bitmaps_offset, 0x30, tag_size))
			goto fail;
		memcpy(&marker_bitmap, tag + bitmaps_offset, sizeof(marker_bitmap));
		width = marker_bitmap.width;
		height = marker_bitmap.height;
		mipmaps = marker_bitmap.mipmap_count;
		pixel_file = (unsigned long)marker_bitmap.pixels_offset;
		pixel_bytes = marker_pixel_span(width, height, mipmaps);
		if (marker_bitmap.signature != BITMAP_GROUP_TAG || marker_bitmap.type != 0 ||
			marker_bitmap.format != 10 || marker_bitmap.depth != 1 ||
			!marker_power_of_two(width) || !marker_power_of_two(height) ||
			pixel_bytes == 0 || pixel_bytes > 1024UL * 1024UL ||
			(unsigned long)marker_bitmap.pixels_size != pixel_bytes ||
			!marker_span(pixel_file, pixel_bytes, file_size))
			goto fail;
		marker_pixels = (unsigned char *)malloc(pixel_bytes);
		if (!marker_pixels || fseek(file, (long)pixel_file, SEEK_SET) != 0 ||
			fread(marker_pixels, 1, pixel_bytes, file) != pixel_bytes)
			goto fail;
		marker_pixel_bytes = (long)pixel_bytes;
		if (!marker_plasma_fill())
			goto fail;
		/* The cache bit is runtime. The file's copy would make the texture
		cache refuse to load these pixels. */
		marker_bitmap.flags = (unsigned short)(marker_bitmap.flags & ~(1u << 7) & ~(1u << 6) & ~(1u << 3));
		marker_bitmap.pixels_offset = 0;
		marker_bitmap.pixels_size = marker_pixel_bytes;
		marker_bitmap.tag_index = NONE;
		marker_bitmap.cache_block_index = NONE;
		marker_bitmap.hardware_format = NULL;
		marker_bitmap.base_address = NULL;
		memset(&marker_bitmap_group, 0, sizeof(marker_bitmap_group));
		marker_bitmap_group.bitmaps.count = 1;
		marker_bitmap_group.bitmaps.address = &marker_bitmap;
		bitmap_tag = cache_files_register_runtime_ui_tag(BITMAP_GROUP_TAG,
			"scenery\\spawn_marker_nhe\\bitmaps\\spawn_marker_nhe", &marker_bitmap_group);
		if (bitmap_tag == NONE)
			goto fail;
		texture_cache_bitmap_new(bitmap_tag, &marker_bitmap);
		if (marker_bitmap.pixels_size != marker_pixel_bytes || marker_bitmap.pixels_offset != 0)
			goto fail;
		marker_shader_set(bitmap_tag);
	}

	marker_free(uncompressed);
	marker_free(compressed);
	marker_free(indices);
	marker_free(column);
	marker_free(tag);
	fclose(file);
	marker_state = MARKER_STATE_READY;
	return;

fail:
	if (file)
		fclose(file);
	marker_free(tag);
	marker_free(uncompressed);
	marker_free(compressed);
	marker_free(indices);
	marker_free(column);
	marker_free(marker_pixels);
	marker_pixels = NULL;
	marker_pixel_bytes = 0;
	rasterizer_vertex_buffer_delete(&marker_vertices);
	rasterizer_triangle_buffer_delete(&marker_triangles);
	memset(&marker_vertices, 0, sizeof(marker_vertices));
	memset(&marker_triangles, 0, sizeof(marker_triangles));
	marker_state = MARKER_STATE_FAILED;
	if (!marker_logged)
	{
		marker_logged = 1;
		platform_log("hardcore: spawn pads were not loaded from the game or hardcore_maps/prisoner.map");
	}
}

static int marker_in_bsp(struct player_starting_location const *start)
{
	return start->structure_bsp_reference_index == NONE ||
		start->structure_bsp_reference_index == global_structure_bsp_index_get();
}

static void marker_draw(struct player_starting_location const *start, long identifier)
{
	real_matrix4x3 matrix;
	real_point3d centroid;
	struct rasterizer_model_begin_parameters parameters;
	static real_rgb_color colors[4];
	static real values[4];
	short color_index;

	for (color_index = 0; color_index < 4; color_index++)
	{
		colors[color_index].red = colors[color_index].green = colors[color_index].blue = 1.0f;
		values[color_index] = 0.0f;
	}
	matrix4x3_rotation_from_angles(&matrix, start->facing, 0.0f, 0.0f);
	matrix.position = start->position;
	/* The frame node is an identity rotation lifted 0.05, and the vertices
	are in that node's space. hangemhigh places this same pad at the start,
	so the extra lift is the node's and the mesh bottom stays about a tenth
	above the floor. */
	matrix.position.z += 0.05f;
	matrix4x3_transform_point(&matrix, &marker_centroid, &centroid);
	memset(&parameters, 0, sizeof(parameters));
	parameters.unique_identifier = identifier;
	parameters.skinning.node_matrices = &matrix;
	parameters.skinning.node_matrix_count = 1;
	parameters.lighting.ambient_color = *global_real_rgb_white;
	parameters.lighting.reflection_tint_color = *global_real_argb_white;
	parameters.lighting.shadow_vector.j = 1.0f;
	parameters.lighting.shadow_color = *global_real_rgb_black;
	parameters.animation.colors = colors;
	parameters.animation.values = values;
	parameters.centroid = centroid;
	parameters.radius = marker_radius;
	parameters.base_map_scale.i = 1.0f;
	parameters.base_map_scale.j = 1.0f;
	rasterizer_profile_enable(FALSE);
	rasterizer_models_begin(FALSE);
	rasterizer_model_begin(&parameters, TRUE);
	rasterizer_model_transparent_geometry_submit(
		(struct shader *)marker_shader,
		0,
		&marker_triangles,
		NONE,
		marker_triangle_count,
		&marker_vertices,
		NONE,
		&centroid,
		NULL);
	rasterizer_model_end();
	rasterizer_models_end();
	rasterizer_profile_enable(TRUE);
}

boolean performance_spawn_marker_ready(void)
{
	return marker_state == MARKER_STATE_READY;
}

void performance_spawn_marker_reset(void)
{
	rasterizer_vertex_buffer_delete(&marker_vertices);
	rasterizer_triangle_buffer_delete(&marker_triangles);
	memset(&marker_vertices, 0, sizeof(marker_vertices));
	memset(&marker_triangles, 0, sizeof(marker_triangles));
	marker_free(marker_pixels);
	marker_pixels = NULL;
	marker_pixel_bytes = 0;
	marker_triangle_count = 0;
	marker_state = MARKER_STATE_NONE;
}

void performance_spawn_marker_render(struct player_starting_location const *starts, short count)
{
	short index;

	if (!starts || count < 1)
		return;
	if (marker_state == MARKER_STATE_NONE)
		marker_prepare();
	if (marker_state != MARKER_STATE_READY)
		return;
	for (index = 0; index < count; index++)
	{
		if (marker_in_bsp(&starts[index]))
			marker_draw(&starts[index], index + 1);
	}
}

boolean performance_spawn_marker_bitmap(struct bitmap_data *bitmap, void *buffer)
{
	if (bitmap != &marker_bitmap)
		return FALSE;
	if (!buffer || !marker_pixels || marker_pixel_bytes < 1 ||
		bitmap->pixels_size != marker_pixel_bytes)
	{
		if (buffer && bitmap->pixels_size > 0 && bitmap->pixels_size <= 1024 * 1024)
			memset(buffer, 0, (size_t)bitmap->pixels_size);
		return TRUE;
	}
	memcpy(buffer, marker_pixels, (size_t)marker_pixel_bytes);
	return TRUE;
}
