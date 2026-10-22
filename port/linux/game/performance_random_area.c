/* A hardcore match draws the darker teammate respawn area from the mesh
shipped with the game when that map has one. Hang 'em High does. The
matching hardcore map is used when this copy has no mesh for it. Maps that
never place one, Prisoner among them, draw nothing here. A classic game
draws neither. */

#include "cseries.h"
#include "hardcore_assets.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "main/main.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_models.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "shaders/shader_definitions.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void platform_log(char const *format, ...);

#define AREA_TAG_BASE 0x40440000UL
#define AREA_TAG_MAXIMUM 0x800000UL
#define AREA_VERTICES_MAXIMUM 12288
#define AREA_TRIANGLES_MAXIMUM 40000
#define AREA_PARTS_MAXIMUM 4
#define AREA_PLACES_MAXIMUM 2
#define AREA_STATE_NONE 0
#define AREA_STATE_READY 2
#define AREA_STATE_LOADED 3

typedef char area_pointer_is_tag_sized[sizeof(void *) == 4 ? 1 : -1];
typedef char area_bitmap_is_tag_sized[sizeof(struct bitmap_data) == 0x30 ? 1 : -1];
typedef char area_vertex_is_model_sized[sizeof(struct model_vertex_uncompressed) == 0x44 ? 1 : -1];

struct area_place
{
	real_point3d position;
	real yaw;
	real pitch;
	real roll;
};

struct area_part
{
	struct vertex_buffer vertices;
	struct triangle_buffer triangles;
	long triangle_count;
	real_point3d centroid;
	real radius;
};

static int area_state;
static int area_logged;
static int area_authored_tried;
static int area_part_count;
static int area_place_count;
static int area_bounds_set;
static real area_min_x, area_min_y, area_min_z;
static real area_max_x, area_max_y, area_max_z;
static real_point3d area_offset;
static struct area_place area_places[AREA_PLACES_MAXIMUM];
static struct area_part area_parts[AREA_PARTS_MAXIMUM];
static real_point3d area_node_translation;
static struct bitmap_group area_bitmap_group;
static struct bitmap_data area_bitmap;
static unsigned char *area_pixels;
static long area_pixel_bytes;
static unsigned char area_shader[0x6C] __attribute__((aligned(8)));
static unsigned char area_shader_map[0xDC] __attribute__((aligned(8)));

/* The game's free halts on a null pointer. Reset runs before a volume exists. */
static void area_free(void *pointer)
{
	if (pointer)
		free(pointer);
}

static unsigned long area_u32(unsigned char const *bytes)
{
	return (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
}

static long area_s32(unsigned char const *bytes)
{
	return (long)area_u32(bytes);
}

static short area_s16(unsigned char const *bytes)
{
	return (short)((unsigned short)bytes[0] | ((unsigned short)bytes[1] << 8));
}

static float area_f32(unsigned char const *bytes)
{
	float value;

	memcpy(&value, bytes, sizeof(value));
	return value;
}

static int area_span(unsigned long offset, unsigned long size, unsigned long limit)
{
	return size > 0 && offset < limit && size <= limit - offset;
}

static int area_tag_place(unsigned long virtual_address, unsigned long tag_size, unsigned long *offset)
{
	if (virtual_address < AREA_TAG_BASE)
		return 0;
	*offset = virtual_address - AREA_TAG_BASE;
	return *offset < tag_size;
}

static int area_name_is(unsigned char const *tag, unsigned long tag_size,
	unsigned long virtual_address, char const *name)
{
	unsigned long offset;
	unsigned long length = 0;
	unsigned long expect;

	if (!name || !area_tag_place(virtual_address, tag_size, &offset))
		return 0;
	expect = (unsigned long)strlen(name);
	while (offset + length < tag_size && tag[offset + length] && length < 256)
		length++;
	if (offset + length >= tag_size || tag[offset + length] || length != expect)
		return 0;
	return memcmp(tag + offset, name, length) == 0;
}

static int area_copy_name(unsigned char const *tag, unsigned long tag_size,
	unsigned long virtual_address, char *out, unsigned long out_size)
{
	unsigned long offset;
	unsigned long length = 0;

	if (!out || out_size < 2 || !area_tag_place(virtual_address, tag_size, &offset))
		return 0;
	while (offset + length < tag_size && tag[offset + length] && length + 1 < out_size)
		length++;
	if (offset + length >= tag_size || tag[offset + length] || length == 0)
		return 0;
	memcpy(out, tag + offset, length);
	out[length] = 0;
	return 1;
}

static int area_has_randoms(char const *text)
{
	unsigned long index;

	if (!text)
		return 0;
	for (index = 0; text[index]; index++)
	{
		if (strncmp(text + index, "randoms", 7) == 0)
			return 1;
	}
	return 0;
}

static int area_find(unsigned char const *tag, unsigned long tag_size,
	char const *group, char const *name, unsigned long *address)
{
	unsigned long table;
	unsigned long count;
	unsigned long index;

	if (!group || tag_size < 0x28)
		return 0;
	count = area_u32(tag + 0x0C);
	if (count == 0 || count > 65535 || !area_tag_place(area_u32(tag), tag_size, &table) ||
		!area_span(table, count * 0x20UL, tag_size))
		return 0;
	for (index = 0; index < count; index++)
	{
		unsigned char const *instance = tag + table + index * 0x20UL;

		if (instance[0] != group[0] || instance[1] != group[1] ||
			instance[2] != group[2] || instance[3] != group[3])
			continue;
		if (name && !area_name_is(tag, tag_size, area_u32(instance + 0x10), name))
			continue;
		*address = area_u32(instance + 0x14);
		return 1;
	}
	return 0;
}

static int area_leaf(char *leaf, unsigned long leaf_size)
{
	char const *name = main_get_multiplayer_map_name();
	char const *start;
	unsigned long length;
	unsigned long index;

	if (!name || !leaf || leaf_size < 2)
		return 0;
	start = name;
	for (index = 0; name[index]; index++)
	{
		if (name[index] == '\\' || name[index] == '/')
			start = name + index + 1;
	}
	length = (unsigned long)strlen(start);
	if (length < 1 || length + 1 > leaf_size)
		return 0;
	for (index = 0; index < length; index++)
	{
		char character = start[index];

		if (!((character >= 'a' && character <= 'z') ||
			(character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') ||
			character == '_' || character == '-'))
			return 0;
	}
	memcpy(leaf, start, length);
	leaf[length] = 0;
	return 1;
}

static int area_part_fits(unsigned char const *part, long *vertices, long *triangles)
{
	long verts = area_s32(part + 0x58);
	long tris = area_s32(part + 0x48);

	if (verts < 3 || verts > AREA_VERTICES_MAXIMUM ||
		tris < 1 || tris > AREA_TRIANGLES_MAXIMUM ||
		area_s16(part + 0x44) != 1 ||
		area_s16(part + 0x54) != 4)
		return 0;
	*vertices = verts;
	*triangles = tris;
	return 1;
}

static void area_shader_set(long bitmap_tag)
{
	long count = 1;
	void *maps = area_shader_map;
	short type = 6;
	short blend = 1;
	unsigned long group = BITMAP_GROUP_TAG;
	float one = 1.0f;

	/* Chicago is type 6. Multiply is blend 1, and the engine's combiner
	already inverts the fade for that blend, so the volume darkens instead
	of drawing white. */
	memset(area_shader, 0, sizeof(area_shader));
	memset(area_shader_map, 0, sizeof(area_shader_map));
	memcpy(area_shader + 0x24, &type, sizeof(type));
	/* Two sided. The sheet lies on the floor, and a one-sided face is
	gone when the camera is on the other side of it. */
	area_shader[0x29] = 1u << 2;
	memcpy(area_shader + 0x2C, &blend, sizeof(blend));
	memcpy(area_shader + 0x54, &count, sizeof(count));
	memcpy(area_shader + 0x58, &maps, sizeof(maps));
	memcpy(area_shader_map + 0x54, &one, sizeof(one));
	memcpy(area_shader_map + 0x58, &one, sizeof(one));
	memcpy(area_shader_map + 0x6C, &group, sizeof(group));
	memcpy(area_shader_map + 0x78, &bitmap_tag, sizeof(bitmap_tag));
}

static int area_bitmap_make(void)
{
	long bitmap_tag;
	long index;
	unsigned long pixel_bytes = 4UL * 4UL * 4UL;

	/* hangemhigh's randoms shader multiplies by the snow treadmarks
	bitmap. That bitmap averages 123, so the drawn mesh uses that grey. */
	area_pixels = (unsigned char *)malloc(pixel_bytes);
	if (!area_pixels)
		return 0;
	for (index = 0; index < (long)pixel_bytes; index += 4)
	{
		area_pixels[index] = 123;
		area_pixels[index + 1] = 123;
		area_pixels[index + 2] = 123;
		area_pixels[index + 3] = 0xFF;
	}
	area_pixel_bytes = (long)pixel_bytes;
	memset(&area_bitmap, 0, sizeof(area_bitmap));
	area_bitmap.signature = BITMAP_GROUP_TAG;
	area_bitmap.width = 4;
	area_bitmap.height = 4;
	area_bitmap.depth = 1;
	area_bitmap.type = 0;
	area_bitmap.format = 10;
	/* Bit 0 is the power-of-two flag. A 4x4 texel is one, and the texture
	cache uses that bit when it builds the hardware copy. */
	area_bitmap.flags = 1;
	area_bitmap.pixels_size = area_pixel_bytes;
	area_bitmap.tag_index = NONE;
	area_bitmap.cache_block_index = NONE;
	memset(&area_bitmap_group, 0, sizeof(area_bitmap_group));
	area_bitmap_group.bitmaps.count = 1;
	area_bitmap_group.bitmaps.address = &area_bitmap;
	bitmap_tag = cache_files_register_runtime_ui_tag(BITMAP_GROUP_TAG,
		"scenery\\randoms\\bitmaps\\hardcore_random_area", &area_bitmap_group);
	if (bitmap_tag == NONE)
		return 0;
	texture_cache_bitmap_new(bitmap_tag, &area_bitmap);
	if (area_bitmap.pixels_size != area_pixel_bytes || area_bitmap.pixels_offset != 0)
		return 0;
	area_shader_set(bitmap_tag);
	return 1;
}

static void area_discard(void)
{
	int index;

	for (index = 0; index < AREA_PARTS_MAXIMUM; index++)
	{
		rasterizer_vertex_buffer_delete(&area_parts[index].vertices);
		rasterizer_triangle_buffer_delete(&area_parts[index].triangles);
	}
	memset(area_parts, 0, sizeof(area_parts));
	area_free(area_pixels);
	area_pixels = NULL;
	area_pixel_bytes = 0;
	area_part_count = 0;
	area_place_count = 0;
	area_bounds_set = 0;
	area_offset.x = area_offset.y = area_offset.z = 0.0f;
}

static int area_take_part(struct model_vertex_uncompressed *uncompressed, long vertex_count,
	unsigned short *indices, long triangle_count)
{
	struct model_vertex_compressed *compressed = NULL;
	struct area_part *slot;
	long index;
	real radius_squared = 0.0f;
	int loaded = 0;

	if (!uncompressed || !indices || vertex_count < 3 || triangle_count < 1)
		return 0;
	compressed = (struct model_vertex_compressed *)malloc(
		(unsigned long)vertex_count * sizeof(*compressed));
	if (!compressed)
		return 0;
	slot = &area_parts[area_part_count];
	slot->centroid.x = slot->centroid.y = slot->centroid.z = 0.0f;
	for (index = 0; index < vertex_count; index++)
	{
		uncompressed[index].nodes[0] = 0;
		uncompressed[index].nodes[1] = 0;
		uncompressed[index].node_weights[0] = 1.0f;
		uncompressed[index].node_weights[1] = 0.0f;
		if (!area_bounds_set)
		{
			area_min_x = area_max_x = uncompressed[index].position.x;
			area_min_y = area_max_y = uncompressed[index].position.y;
			area_min_z = area_max_z = uncompressed[index].position.z;
			area_bounds_set = 1;
		}
		else
		{
			if (uncompressed[index].position.x < area_min_x)
				area_min_x = uncompressed[index].position.x;
			if (uncompressed[index].position.y < area_min_y)
				area_min_y = uncompressed[index].position.y;
			if (uncompressed[index].position.z < area_min_z)
				area_min_z = uncompressed[index].position.z;
			if (uncompressed[index].position.x > area_max_x)
				area_max_x = uncompressed[index].position.x;
			if (uncompressed[index].position.y > area_max_y)
				area_max_y = uncompressed[index].position.y;
			if (uncompressed[index].position.z > area_max_z)
				area_max_z = uncompressed[index].position.z;
		}
		slot->centroid.x += uncompressed[index].position.x;
		slot->centroid.y += uncompressed[index].position.y;
		slot->centroid.z += uncompressed[index].position.z;
	}
	for (index = 0; index < triangle_count + 2; index++)
	{
		if (indices[index] >= (unsigned short)vertex_count)
			goto done;
	}
	slot->centroid.x /= (real)vertex_count;
	slot->centroid.y /= (real)vertex_count;
	slot->centroid.z /= (real)vertex_count;
	for (index = 0; index < vertex_count; index++)
	{
		real dx = uncompressed[index].position.x - slot->centroid.x;
		real dy = uncompressed[index].position.y - slot->centroid.y;
		real dz = uncompressed[index].position.z - slot->centroid.z;
		real span = dx * dx + dy * dy + dz * dz;

		if (span > radius_squared)
			radius_squared = span;
	}
	/* The transparent sort drops a mesh whose radius is under a world unit.
	These volumes are the size of the respawn zone, so the radius is the
	real extent. */
	slot->radius = sqrtf(radius_squared);
	if (slot->radius < 1.0f)
		slot->radius = 1.0f;
	rasterizer_geometry_compress_vertices(
		_rasterizer_vertex_type_model_uncompressed,
		vertex_count,
		compressed,
		vertex_count * (long)sizeof(*compressed),
		uncompressed,
		vertex_count * (long)sizeof(*uncompressed));
	if (!rasterizer_vertex_buffer_new(&slot->vertices, _rasterizer_vertex_type_model_compressed,
			vertex_count, compressed, vertex_count * (long)sizeof(*compressed)) ||
		!rasterizer_triangle_buffer_new(&slot->triangles, _triangle_buffer_type_precompiled_strip,
			triangle_count, indices))
		goto done;
	slot->triangle_count = triangle_count;
	area_part_count++;
	loaded = 1;

done:
	area_free(compressed);
	return loaded;
}

static int area_load_part(FILE *file, unsigned long file_size, unsigned long vertex_file,
	unsigned long index_relative, unsigned char const *part)
{
	struct model_vertex_uncompressed *uncompressed = NULL;
	unsigned short *indices = NULL;
	long vertex_count = 0;
	long triangle_count = 0;
	unsigned long vertex_at;
	unsigned long index_at;
	int loaded = 0;

	if (area_part_count >= AREA_PARTS_MAXIMUM)
		return 1;
	if (!area_part_fits(part, &vertex_count, &triangle_count))
		return 1;
	if (vertex_file > file_size || index_relative > file_size - vertex_file ||
		area_u32(part + 0x64) > file_size - vertex_file ||
		area_u32(part + 0x4C) > file_size - (vertex_file + index_relative))
		goto done;
	vertex_at = vertex_file + area_u32(part + 0x64);
	index_at = vertex_file + index_relative + area_u32(part + 0x4C);
	if (!area_span(vertex_at, (unsigned long)vertex_count * 0x44UL, file_size) ||
		!area_span(index_at, (unsigned long)(triangle_count + 2) * 2UL, file_size))
		goto done;
	uncompressed = (struct model_vertex_uncompressed *)malloc(
		(unsigned long)vertex_count * sizeof(*uncompressed));
	indices = (unsigned short *)malloc((unsigned long)(triangle_count + 2) * sizeof(*indices));
	if (!uncompressed || !indices ||
		fseek(file, (long)vertex_at, SEEK_SET) != 0 ||
		fread(uncompressed, sizeof(*uncompressed), (size_t)vertex_count, file) != (size_t)vertex_count ||
		fseek(file, (long)index_at, SEEK_SET) != 0 ||
		fread(indices, sizeof(*indices), (size_t)(triangle_count + 2), file) != (size_t)(triangle_count + 2))
		goto done;
	loaded = area_take_part(uncompressed, vertex_count, indices, triangle_count);

done:
	area_free(uncompressed);
	area_free(indices);
	return loaded;
}

/* 1 when at least one randoms placement was recorded, 0 when the map has
none, -1 when a placement was there and its model could not be followed. */
static int area_collect(unsigned char const *tag, unsigned long tag_size,
	unsigned long scenario_offset, char *model_name, unsigned long model_name_size)
{
	long scenery_count;
	long palette_count;
	long index;
	unsigned long scenery_offset;
	unsigned long palette_offset;
	int palette_index = -1;
	int broken = 0;

	scenery_count = area_s32(tag + scenario_offset + 0x210);
	palette_count = area_s32(tag + scenario_offset + 0x21C);
	if (scenery_count < 1 || scenery_count > 8192 || palette_count < 1 || palette_count > 512 ||
		!area_tag_place(area_u32(tag + scenario_offset + 0x214), tag_size, &scenery_offset) ||
		!area_tag_place(area_u32(tag + scenario_offset + 0x220), tag_size, &palette_offset) ||
		!area_span(scenery_offset, (unsigned long)scenery_count * 72UL, tag_size) ||
		!area_span(palette_offset, (unsigned long)palette_count * 48UL, tag_size))
		return 0;
	model_name[0] = 0;
	for (index = 0; index < scenery_count; index++)
	{
		unsigned char const *datum = tag + scenery_offset + (unsigned long)index * 72UL;
		short palette = area_s16(datum);
		char path[256];
		unsigned long scenery_address;
		unsigned long scenery_placed;

		if (palette < 0 || palette >= palette_count)
			continue;
		if (!area_copy_name(tag, tag_size,
				area_u32(tag + palette_offset + (unsigned long)palette * 48UL + 4),
				path, sizeof(path)) || !area_has_randoms(path))
			continue;
		if (palette_index < 0)
		{
			palette_index = palette;
			if (!area_find(tag, tag_size, "necs", path, &scenery_address) ||
				!area_tag_place(scenery_address, tag_size, &scenery_placed) ||
				!area_span(scenery_placed, 0x40, tag_size) ||
				!area_copy_name(tag, tag_size, area_u32(tag + scenery_placed + 0x2C),
					model_name, model_name_size))
				broken = 1;
		}
		if (broken || palette != palette_index || area_place_count >= AREA_PLACES_MAXIMUM)
			continue;
		area_places[area_place_count].position.x = area_f32(datum + 8);
		area_places[area_place_count].position.y = area_f32(datum + 12);
		area_places[area_place_count].position.z = area_f32(datum + 16);
		area_places[area_place_count].yaw = area_f32(datum + 0x14);
		area_places[area_place_count].pitch = area_f32(datum + 0x18);
		area_places[area_place_count].roll = area_f32(datum + 0x1C);
		area_place_count++;
	}
	if (broken)
		return -1;
	return area_place_count > 0 ? 1 : 0;
}

/* 1 when the mesh shipped for this map is loaded, 0 when this copy has
none, -1 when that copy is there and cannot be used. */
static int area_from_pack(char const *leaf)
{
	unsigned char const *payload;
	unsigned long size;
	unsigned long cursor;
	unsigned long place_count;
	unsigned long part_count;
	unsigned long place_index;
	unsigned long part_index;

	if (!leaf || !hardcore_assets_find(HARDCORE_ASSET_AREA, leaf, &payload, &size))
		return 0;
	if (size < 4)
		return -1;
	place_count = area_u32(payload);
	if (place_count < 1 || place_count > AREA_PLACES_MAXIMUM)
		return -1;
	cursor = 4;
	if (size - cursor < place_count * 24UL + 16UL)
		return -1;
	for (place_index = 0; place_index < place_count; place_index++)
	{
		area_places[place_index].position.x = area_f32(payload + cursor);
		area_places[place_index].position.y = area_f32(payload + cursor + 4);
		area_places[place_index].position.z = area_f32(payload + cursor + 8);
		area_places[place_index].yaw = area_f32(payload + cursor + 12);
		area_places[place_index].pitch = area_f32(payload + cursor + 16);
		area_places[place_index].roll = area_f32(payload + cursor + 20);
		cursor += 24;
	}
	area_place_count = (int)place_count;
	area_node_translation.x = area_f32(payload + cursor);
	area_node_translation.y = area_f32(payload + cursor + 4);
	area_node_translation.z = area_f32(payload + cursor + 8);
	part_count = area_u32(payload + cursor + 12);
	cursor += 16;
	if (part_count < 1 || part_count > AREA_PARTS_MAXIMUM)
		return -1;
	for (part_index = 0; part_index < part_count; part_index++)
	{
		unsigned long vertex_count;
		unsigned long index_count;
		unsigned long mesh_bytes;
		long triangle_count;
		struct model_vertex_uncompressed *uncompressed;
		unsigned short *indices;
		int taken;

		if (size - cursor < 8)
			return -1;
		vertex_count = area_u32(payload + cursor);
		index_count = area_u32(payload + cursor + 4);
		cursor += 8;
		triangle_count = (long)index_count - 2;
		if (vertex_count < 3 || vertex_count > AREA_VERTICES_MAXIMUM ||
			triangle_count < 1 || triangle_count > AREA_TRIANGLES_MAXIMUM)
			return -1;
		mesh_bytes = vertex_count * 0x44UL + index_count * 2UL;
		if (size - cursor < mesh_bytes)
			return -1;
		uncompressed = (struct model_vertex_uncompressed *)malloc(vertex_count * sizeof(*uncompressed));
		indices = (unsigned short *)malloc(index_count * sizeof(*indices));
		if (!uncompressed || !indices)
		{
			area_free(uncompressed);
			area_free(indices);
			return -1;
		}
		memcpy(uncompressed, payload + cursor, vertex_count * 0x44UL);
		memcpy(indices, payload + cursor + vertex_count * 0x44UL, index_count * 2UL);
		cursor += mesh_bytes;
		taken = area_take_part(uncompressed, (long)vertex_count, indices, triangle_count);
		area_free(uncompressed);
		area_free(indices);
		if (!taken)
			return -1;
	}
	if (area_part_count < 1)
		return -1;
	return 1;
}

static void area_prepare(void)
{
	FILE *file = NULL;
	unsigned char *tag = NULL;
	char leaf[64];
	char path[96];
	char model_name[256];
	unsigned char header[0x18];
	unsigned long file_size;
	unsigned long tag_offset;
	unsigned long tag_size;
	unsigned long scenario_address;
	unsigned long scenario_offset;
	unsigned long model_address;
	unsigned long model_offset;
	unsigned long vertex_file;
	unsigned long index_relative;
	unsigned long part_offset;
	long part_count;
	long part_index;
	long geometry_count;
	long geometry_index;
	long best_vertices;
	long told;
	int collected;
	int best_count;
	int packed;
	unsigned long best_address;

	if (!area_leaf(leaf, sizeof(leaf)))
		goto fail;
	packed = area_from_pack(leaf);
	if (packed > 0)
	{
		area_state = AREA_STATE_LOADED;
		return;
	}
	if (packed < 0)
		goto missing;
	if (snprintf(path, sizeof(path), "d:\\hardcore_maps\\%s.map", leaf) >= (int)sizeof(path))
		goto fail;
	file = fopen(path, "rb");
	if (!file)
		goto fail;
	if (fseek(file, 0, SEEK_END) != 0 || (told = ftell(file)) < 0x800)
		goto fail;
	file_size = (unsigned long)told;
	if (fseek(file, 0, SEEK_SET) != 0 || fread(header, 1, sizeof(header), file) != sizeof(header) ||
		header[0] != 'd' || header[1] != 'a' || header[2] != 'e' || header[3] != 'h' ||
		area_s32(header + 4) != 7)
		goto fail;
	tag_offset = area_u32(header + 0x10);
	tag_size = area_u32(header + 0x14);
	if (tag_offset < 0x800 || tag_size < 0x28 || tag_size > AREA_TAG_MAXIMUM ||
		!area_span(tag_offset, tag_size, file_size))
		goto fail;
	tag = (unsigned char *)malloc(tag_size);
	if (!tag || fseek(file, (long)tag_offset, SEEK_SET) != 0 ||
		fread(tag, 1, tag_size, file) != tag_size)
		goto fail;
	if (!area_find(tag, tag_size, "rncs", NULL, &scenario_address) ||
		!area_tag_place(scenario_address, tag_size, &scenario_offset) ||
		!area_span(scenario_offset, 0x228, tag_size))
		goto fail;
	collected = area_collect(tag, tag_size, scenario_offset, model_name, sizeof(model_name));
	if (collected == 0)
		goto fail;
	if (collected < 0 ||
		!area_find(tag, tag_size, "2dom", model_name, &model_address) ||
		!area_tag_place(model_address, tag_size, &model_offset) ||
		!area_span(model_offset, 0xE8, tag_size))
		goto missing;
	/* The node pivot is kept and applied only if it lands the mesh on the
	spawns. Several of these volumes were exported already in world space,
	and adding the pivot there moves them off the respawn area. */
	area_node_translation.x = area_node_translation.y = area_node_translation.z = 0.0f;
	{
		long node_count = area_s32(tag + model_offset + 0xB8);
		unsigned long node_offset;

		if (node_count >= 1 &&
			area_tag_place(area_u32(tag + model_offset + 0xBC), tag_size, &node_offset) &&
			area_span(node_offset, 52, tag_size))
		{
			area_node_translation.x = area_f32(tag + node_offset + 40);
			area_node_translation.y = area_f32(tag + node_offset + 44);
			area_node_translation.z = area_f32(tag + node_offset + 48);
		}
	}
	vertex_file = area_u32(tag + 0x14);
	index_relative = area_u32(tag + 0x1C);
	geometry_count = area_s32(tag + model_offset + 0xD0);
	best_vertices = 0;
	best_count = 0;
	best_address = 0;
	if (geometry_count < 1 || geometry_count > 8 ||
		!area_tag_place(area_u32(tag + model_offset + 0xD4), tag_size, &part_offset) ||
		!area_span(part_offset, (unsigned long)geometry_count * 0x30UL, tag_size))
		goto missing;
	for (geometry_index = 0; geometry_index < geometry_count; geometry_index++)
	{
		unsigned char const *geometry = tag + part_offset + (unsigned long)geometry_index * 0x30UL;
		long count = area_s32(geometry + 0x24);
		unsigned long address = area_u32(geometry + 0x28);
		unsigned long offset;
		long total = 0;
		long candidate;

		if (count < 1 || count > 32 || !area_tag_place(address, tag_size, &offset) ||
			!area_span(offset, (unsigned long)count * 0x84UL, tag_size))
			continue;
		for (candidate = 0; candidate < count; candidate++)
		{
			long verts;
			long tris;

			if (area_part_fits(tag + offset + (unsigned long)candidate * 0x84UL, &verts, &tris))
				total += verts;
		}
		if (total > best_vertices)
		{
			best_vertices = total;
			best_count = count;
			best_address = address;
		}
	}
	if (best_vertices < 3 || !area_tag_place(best_address, tag_size, &part_offset))
		goto missing;
	part_count = best_count;
	for (part_index = 0; part_index < part_count && area_part_count < AREA_PARTS_MAXIMUM; part_index++)
	{
		if (!area_load_part(file, file_size, vertex_file, index_relative,
				tag + part_offset + (unsigned long)part_index * 0x84UL))
			goto missing;
	}
	if (area_part_count < 1)
		goto missing;
	area_free(tag);
	fclose(file);
	area_state = AREA_STATE_LOADED;
	return;

missing:
	if (!area_logged)
	{
		area_logged = 1;
		platform_log("hardcore: random areas were not loaded from the game or hardcore_maps/%s.map", leaf);
	}
fail:
	if (file)
		fclose(file);
	area_free(tag);
	area_discard();
}

static int area_count_inside(real ox, real oy, real oz,
	struct player_starting_location const *starts, short count)
{
	short index;
	int inside = 0;

	if (!area_bounds_set || !starts || count < 1)
		return 0;
	for (index = 0; index < count; index++)
	{
		real x = starts[index].position.x;
		real y = starts[index].position.y;
		real z = starts[index].position.z;

		if (x < area_min_x + ox - 1.5f || x > area_max_x + ox + 1.5f ||
			y < area_min_y + oy - 1.5f || y > area_max_y + oy + 1.5f ||
			z < area_min_z + oz - 2.0f || z > area_max_z + oz + 2.0f)
			continue;
		inside++;
	}
	return inside;
}

/* The mesh is kept when this offset puts it on the starts. A speck that
covers one start is not a respawn area. */
static int area_place_on_starts(struct player_starting_location const *starts, short count)
{
	real ox[4];
	real oy[4];
	real oz[4];
	int scores[4];
	int best = 0;
	int index;
	int need;

	ox[0] = oy[0] = oz[0] = 0.0f;
	ox[1] = area_place_count > 0 ? area_places[0].position.x : 0.0f;
	oy[1] = area_place_count > 0 ? area_places[0].position.y : 0.0f;
	oz[1] = area_place_count > 0 ? area_places[0].position.z : 0.0f;
	ox[2] = area_node_translation.x;
	oy[2] = area_node_translation.y;
	oz[2] = area_node_translation.z;
	ox[3] = ox[1] + ox[2];
	oy[3] = oy[1] + oy[2];
	oz[3] = oz[1] + oz[2];
	for (index = 0; index < 4; index++)
		scores[index] = area_count_inside(ox[index], oy[index], oz[index], starts, count);
	for (index = 1; index < 4; index++)
	{
		if (scores[index] > scores[best])
			best = index;
	}
	need = count < 4 ? (int)count : 4;
	if (need < 1 || scores[best] < need)
		return 0;
	area_offset.x = ox[best];
	area_offset.y = oy[best];
	area_offset.z = oz[best];
	return 1;
}


static void area_draw(struct area_part const *part, long identifier)
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
	matrix4x3_rotation_from_angles(&matrix, 0.0f, 0.0f, 0.0f);
	matrix.position = area_offset;
	centroid.x = part->centroid.x + area_offset.x;
	centroid.y = part->centroid.y + area_offset.y;
	centroid.z = part->centroid.z + area_offset.z;
	memset(&parameters, 0, sizeof(parameters));
	parameters.unique_identifier = (unsigned long)identifier;
	parameters.skinning.node_matrices = &matrix;
	parameters.skinning.node_matrix_count = 1;
	parameters.lighting.ambient_color = *global_real_rgb_white;
	parameters.lighting.reflection_tint_color = *global_real_argb_white;
	parameters.lighting.shadow_vector.j = 1.0f;
	parameters.lighting.shadow_color = *global_real_rgb_black;
	parameters.animation.colors = colors;
	parameters.animation.values = values;
	parameters.centroid = centroid;
	parameters.radius = part->radius;
	parameters.base_map_scale.i = 1.0f;
	parameters.base_map_scale.j = 1.0f;
	rasterizer_profile_enable(FALSE);
	rasterizer_models_begin(FALSE);
	rasterizer_model_begin(&parameters, TRUE);
	rasterizer_model_transparent_geometry_submit(
		(struct shader *)area_shader,
		0,
		&part->triangles,
		NONE,
		part->triangle_count,
		&part->vertices,
		NONE,
		&centroid,
		NULL);
	rasterizer_model_end();
	rasterizer_models_end();
	rasterizer_profile_enable(TRUE);
}

void performance_random_area_reset(void)
{
	area_discard();
	area_logged = 0;
	area_authored_tried = 0;
	area_state = AREA_STATE_NONE;
}

void performance_random_area_render(struct player_starting_location const *starts, short count)
{
	int part_index;

	if (area_state == AREA_STATE_NONE && !area_authored_tried)
	{
		area_authored_tried = 1;
		area_prepare();
	}
	if (area_state == AREA_STATE_LOADED)
	{
		if (!starts || count < 1)
			return;
		if (!area_place_on_starts(starts, count) || !area_bitmap_make())
		{
			area_discard();
			area_state = AREA_STATE_NONE;
		}
		else
			area_state = AREA_STATE_READY;
	}
	if (area_state != AREA_STATE_READY)
		return;
	for (part_index = 0; part_index < area_part_count; part_index++)
		area_draw(&area_parts[part_index], part_index + 1);
}

boolean performance_random_area_bitmap(struct bitmap_data *bitmap, void *buffer)
{
	if (bitmap != &area_bitmap)
		return FALSE;
	if (!buffer || !area_pixels || area_pixel_bytes < 1 ||
		bitmap->pixels_size != area_pixel_bytes)
	{
		if (buffer && bitmap->pixels_size > 0 && bitmap->pixels_size <= 1024 * 1024)
			memset(buffer, 0, (size_t)bitmap->pixels_size);
		return TRUE;
	}
	memcpy(buffer, area_pixels, (size_t)area_pixel_bytes);
	return TRUE;
}
