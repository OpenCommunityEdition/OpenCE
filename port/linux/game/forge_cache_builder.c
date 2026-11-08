/*
FORGE_CACHE_BUILDER.C

Custom Edition (v609) and Halo PC (v7) cache parser, schema-driven tag cloner,
PC-to-CE bitmap/sound internalizer, gbxmodel geometry packer, credits.md
attribution generator, 2D blueprint collision extractor, and "Save Copy As..."
scenario exporter (.map, 140x114 .bmp, .txt).
*/

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries/cseries.h"
#include "forge_cache_builder.h"
#include "cache_file_formats.h"
#include "tag_schema.h"
#include "math/real_math.h"
#include "render/render.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "rasterizer/rasterizer_geometry.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "items/item_definitions.h"
#include "../src/port_tag_profiles.h"
#include "custom_edition_cache.h"
#include "tag_bundle.h"
#include "../../third_party/zlib/zlib_prefixed.h"

extern void platform_translate_path(char const *xbox_path, char *out_path, unsigned long out_size);

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtl.h>
#include "forge_engine.h"

#define FORGE_MKDIR(p) CreateDirectoryA((p), NULL)

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef remove
#undef rename
#undef _mkdir
#undef mkdir
#undef memcmp
#undef memcpy
#undef memmove
#undef memset

#define fopen(p, m) ((FILE *)forge_host_fopen((p), (m)))
#define remove(p) forge_host_remove((p))

/* ---------- constants */

#define CACHE_HEADER_SIGNATURE 'head'
#define CACHE_FOOTER_SIGNATURE 'foot'
#define TAG_INDEX_SIGNATURE 'tags'
#define CACHE_VERSION_XBOX 5
#define CACHE_VERSION_PC 7
#define CACHE_VERSION_CE 609

#define CE_TAG_BASE_ADDR 0x40440000UL
#define XBOX_TAG_BASE_ADDR 0x803A6000UL

#define TAG_INSTANCE_SIZE 0x20

/* ---------- little-endian memory helpers */

static uint16_t read_u16(void const *p)
{
	uint8_t const *b = (uint8_t const *)p;
	return (uint16_t)(b[0] | ((uint16_t)b[1] << 8));
}

static int16_t read_s16(void const *p)
{
	return (int16_t)read_u16(p);
}

static uint32_t read_u32(void const *p)
{
	uint8_t const *b = (uint8_t const *)p;
	return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int32_t read_s32(void const *p)
{
	return (int32_t)read_u32(p);
}

static float read_f32(void const *p)
{
	uint32_t bits = read_u32(p);
	float val;
	memcpy(&val, &bits, sizeof(val));
	return val;
}

static void write_u16(void *p, uint16_t v)
{
	uint8_t *b = (uint8_t *)p;
	b[0] = (uint8_t)(v & 0xFF);
	b[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void write_s16(void *p, int16_t v)
{
	write_u16(p, (uint16_t)v);
}

static void write_u32(void *p, uint32_t v)
{
	uint8_t *b = (uint8_t *)p;
	b[0] = (uint8_t)(v & 0xFF);
	b[1] = (uint8_t)((v >> 8) & 0xFF);
	b[2] = (uint8_t)((v >> 16) & 0xFF);
	b[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void write_s32(void *p, int32_t v)
{
	write_u32(p, (uint32_t)v);
}

static void write_f32(void *p, float v)
{
	uint32_t bits;
	memcpy(&bits, &v, sizeof(bits));
	write_u32(p, bits);
}

/* ---------- CRC-32 (IEEE 802.3, matching OpenCE cache_file_formats.c) */

static uint32_t forge_crc32_update(uint32_t crc, void const *data, size_t size)
{
	uint8_t const *bytes = (uint8_t const *)data;
	size_t i;
	int bit;

	for (i = 0; i < size; i++)
	{
		crc ^= bytes[i];
		for (bit = 0; bit < 8; bit++)
		{
			crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
		}
	}
	return crc;
}

/* ---------- growable byte buffer */

struct forge_byte_buf
{
	uint8_t *data;
	uint32_t size;
	uint32_t capacity;
};

static void buf_init(struct forge_byte_buf *b, uint32_t initial_cap)
{
	b->size = 0;
	b->capacity = initial_cap ? initial_cap : 65536;
	b->data = (uint8_t *)calloc(1, b->capacity);
}

static void buf_free(struct forge_byte_buf *b)
{
	if (b->data)
		free(b->data);
	b->data = NULL;
	b->size = 0;
	b->capacity = 0;
}

static uint32_t buf_alloc(struct forge_byte_buf *b, uint32_t bytes, uint32_t align)
{
	uint32_t offset = b->size;
	uint32_t needed;

	if (align > 1)
		offset = (offset + (align - 1)) & ~(align - 1);
	needed = offset + bytes;
	if (needed > b->capacity)
	{
		uint32_t new_cap = b->capacity ? b->capacity : 65536;
		uint8_t *new_data;
		while (new_cap < needed)
			new_cap *= 2;
		new_data = (uint8_t *)realloc(b->data, new_cap);
		if (!new_data)
			return 0;
		memset(new_data + b->capacity, 0, new_cap - b->capacity);
		b->data = new_data;
		b->capacity = new_cap;
	}
	if (offset > b->size)
		memset(b->data + b->size, 0, offset - b->size);
	memset(b->data + offset, 0, bytes);
	b->size = needed;
	return offset;
}

static void buf_append_bytes(struct forge_byte_buf *b, void const *src, int len)
{
	if (len > 0 && b->data)
	{
		uint32_t o = buf_alloc(b, (uint32_t)len, 1);
		if (b->data) memcpy(b->data + o, src, (size_t)len);
	}
}

/* ---------- schema lookup */

static struct tag_schema_group const *forge_schema_group_get(uint32_t group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	for (group = tag_schema_custom_edition_groups; group->group_tag; group++)
	{
		if (group->group_tag == group_tag)
			return group;
	}
	for (list = tag_schema_group_lists; *list; list++)
	{
		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}
	return NULL;
}

static int forge_groups_compatible(uint32_t group_a, uint32_t group_b)
{
	if (group_a == group_b)
		return 1;
	if ((group_a == 'mod2' && group_b == 'mode') || (group_a == 'mode' && group_b == 'mod2'))
		return 1;
	if ((group_a == 'scex' && group_b == 'schi') || (group_a == 'schi' && group_b == 'scex'))
		return 1;
	return 0;
}

/* ---------- source .map loader */

struct forge_source_map
{
	char path[512];
	char filename[128];
	char map_name[36];
	uint8_t *data;
	uint32_t file_size;
	int32_t version;
	int16_t scenario_type;
	uint32_t tag_base_addr;
	uint32_t tag_data_offset;
	uint32_t tag_data_size;
	uint32_t instances_offset;
	int32_t scenario_tag_index;
	int32_t tag_count;
	uint32_t model_vertex_data_offset;
	uint32_t model_index_data_offset;
	uint32_t model_data_size;
};

static void forge_extract_basename(char const *path, char *out_name, size_t max_len)
{
	char const *slash1 = strrchr(path, '/');
	char const *slash2 = strrchr(path, '\\');
	char const *base = slash1 > slash2 ? slash1 + 1 : (slash2 ? slash2 + 1 : path);
	snprintf(out_name, max_len, "%s", base);
}

static int forge_source_map_open(char const *path, struct forge_source_map *map)
{
	FILE *f;
	long sz;
	uint32_t tag_hdr_off;
	uint32_t inst_addr;
	uint8_t hdr_peek[0x800];

	memset(map, 0, sizeof(*map));
	if (!path || !path[0])
		return 0;
	snprintf(map->path, sizeof(map->path), "%s", path);
	forge_extract_basename(path, map->filename, sizeof(map->filename));

	f = fopen(path, "rb");
	if (!f)
		return 0;
	if (fseek(f, 0, SEEK_END) != 0 || (sz = ftell(f)) < 0x800)
	{
		fclose(f);
		return 0;
	}
	fseek(f, 0, SEEK_SET);
	if (fread(hdr_peek, 1, 0x800, f) != 0x800)
	{
		fclose(f);
		return 0;
	}
	if (read_u32(hdr_peek) != CACHE_HEADER_SIGNATURE ||
		read_u32(hdr_peek + 0x7FC) != CACHE_FOOTER_SIGNATURE)
	{
		fclose(f);
		return 0;
	}
	map->version = read_s32(hdr_peek + 0x04);
	if (map->version != CACHE_VERSION_XBOX &&
		map->version != CACHE_VERSION_PC &&
		map->version != CACHE_VERSION_CE)
	{
		fclose(f);
		return 0;
	}
	map->tag_data_offset = read_u32(hdr_peek + 0x10);
	map->tag_data_size = read_u32(hdr_peek + 0x14);
	memcpy(map->map_name, hdr_peek + 0x20, 32);
	map->map_name[31] = 0;
	map->scenario_type = read_s16(hdr_peek + 0x60);

	/* Transparently handle zlib-compressed Xbox v5 maps (or compressed CE v609 base maps in OpenCE Forge/forge_maps/) */
	if ((map->version == CACHE_VERSION_XBOX || map->version == CACHE_VERSION_CE) &&
		(uint64_t)map->tag_data_offset + (uint64_t)map->tag_data_size > (uint64_t)sz)
	{
		uint32_t decomp_len = read_u32(hdr_peek + 0x08);
		char cache_cand[512];
		FILE *cf = NULL;

		if (decomp_len < map->tag_data_offset + map->tag_data_size)
			decomp_len = map->tag_data_offset + map->tag_data_size;
		if (decomp_len < 0x800u || decomp_len > 512u * 1024u * 1024u)
		{
			fclose(f);
			return 0;
		}

		if (map->version == CACHE_VERSION_XBOX)
		{
			int slot;
			for (slot = 0; slot <= 6; slot++)
			{
				char z_xbox[64];
				if (slot < 6)
				{
					snprintf(z_xbox, sizeof(z_xbox), "z:\\cache00%d.map", slot);
					platform_translate_path(z_xbox, cache_cand, sizeof(cache_cand));
				}
				else
				{
					snprintf(z_xbox, sizeof(z_xbox), "z:\\cache_donor.map");
					platform_translate_path(z_xbox, cache_cand, sizeof(cache_cand));
				}
				cf = fopen(cache_cand, "rb");
				if (cf)
				{
					uint8_t zhdr[0x800];
					long csz = 0;
					if (fseek(cf, 0, SEEK_END) == 0 &&
						(csz = ftell(cf)) >= (long)(map->tag_data_offset + map->tag_data_size) &&
						fseek(cf, 0, SEEK_SET) == 0 &&
						fread(zhdr, 1, sizeof(zhdr), cf) == sizeof(zhdr) &&
						read_u32(zhdr) == CACHE_HEADER_SIGNATURE &&
						read_s32(zhdr + 0x04) == CACHE_VERSION_XBOX &&
						read_u32(zhdr + 0x10) == map->tag_data_offset &&
						read_u32(zhdr + 0x14) == map->tag_data_size &&
						_strnicmp((char const *)(zhdr + 0x20), map->map_name, 31) == 0)
					{
						map->file_size = (uint32_t)csz;
						map->data = (uint8_t *)malloc(map->file_size);
						if (map->data)
						{
							fseek(cf, 0, SEEK_SET);
							if (fread(map->data, 1, map->file_size, cf) == map->file_size)
							{
								fclose(cf);
								fclose(f);
								goto parse_tag_header;
							}
							free(map->data);
							map->data = NULL;
						}
					}
					fclose(cf);
				}
			}
		}

		{
			uint32_t comp_len = (uint32_t)sz - 0x800u;
			uint8_t *comp_buf = (uint8_t *)malloc(comp_len);
			uLongf dst_len = (uLongf)(decomp_len - 0x800u);
			if (!comp_buf)
			{
				fclose(f);
				return 0;
			}
			fseek(f, 0x800, SEEK_SET);
			if (fread(comp_buf, 1, comp_len, f) != comp_len)
			{
				free(comp_buf);
				fclose(f);
				return 0;
			}
			fclose(f);
			f = NULL;

			map->file_size = decomp_len;
			map->data = (uint8_t *)malloc(decomp_len);
			if (!map->data)
			{
				free(comp_buf);
				return 0;
			}
			memcpy(map->data, hdr_peek, 0x800u);
			if (uncompress(map->data + 0x800u, &dst_len, comp_buf, (uLong)comp_len) != Z_OK ||
				0x800u + (uint32_t)dst_len < map->tag_data_offset + map->tag_data_size)
			{
				free(comp_buf);
				free(map->data);
				map->data = NULL;
				return 0;
			}
			free(comp_buf);
			map->file_size = 0x800u + (uint32_t)dst_len;
		}
	}
	else
	{
		map->file_size = (uint32_t)sz;
		map->data = (uint8_t *)malloc(map->file_size);
		if (!map->data)
		{
			fclose(f);
			return 0;
		}
		fseek(f, 0, SEEK_SET);
		if (fread(map->data, 1, map->file_size, f) != map->file_size)
		{
			free(map->data);
			map->data = NULL;
			fclose(f);
			return 0;
		}
		fclose(f);
	}

parse_tag_header:
	if (map->tag_data_offset < 0x800 ||
		map->tag_data_size < 0x24u ||
		map->tag_data_offset + map->tag_data_size > map->file_size)
	{
		free(map->data);
		map->data = NULL;
		return 0;
	}
	tag_hdr_off = map->tag_data_offset;
	if (map->version == CACHE_VERSION_XBOX)
	{
		if (read_u32(map->data + tag_hdr_off + 0x20) != TAG_INDEX_SIGNATURE)
		{
			free(map->data);
			map->data = NULL;
			return 0;
		}
	}
	else if (read_u32(map->data + tag_hdr_off + 0x24) != TAG_INDEX_SIGNATURE)
	{
		free(map->data);
		map->data = NULL;
		return 0;
	}
	inst_addr = read_u32(map->data + tag_hdr_off + 0x00);
	map->scenario_tag_index = read_s32(map->data + tag_hdr_off + 0x04) & 0xFFFF;
	map->tag_count = read_s32(map->data + tag_hdr_off + 0x0C);
	map->model_vertex_data_offset = read_u32(map->data + tag_hdr_off + 0x14);
	map->model_index_data_offset = read_u32(map->data + tag_hdr_off + 0x1C);
	map->model_data_size = read_u32(map->data + tag_hdr_off + 0x20);
	map->tag_base_addr = (map->version == CACHE_VERSION_XBOX)
		? ((inst_addr >= 0x24u) ? (inst_addr - 0x24u) : XBOX_TAG_BASE_ADDR)
		: CE_TAG_BASE_ADDR;

	if (inst_addr < map->tag_base_addr ||
		map->tag_count <= 0 ||
		map->tag_count > 65535 ||
		(inst_addr - map->tag_base_addr) + (uint32_t)map->tag_count * TAG_INSTANCE_SIZE > map->tag_data_size)
	{
		free(map->data);
		map->data = NULL;
		return 0;
	}
	map->instances_offset = inst_addr - map->tag_base_addr;
	return 1;
}

static void forge_source_map_close(struct forge_source_map *map)
{
	if (map->data)
		free(map->data);
	memset(map, 0, sizeof(*map));
}

static uint8_t const *src_tag_instance(struct forge_source_map const *map, int32_t idx)
{
	if (idx < 0 || idx >= map->tag_count)
		return NULL;
	return map->data + map->tag_data_offset + map->instances_offset + (uint32_t)idx * TAG_INSTANCE_SIZE;
}

static char const *src_tag_name(struct forge_source_map const *map, int32_t idx)
{
	uint8_t const *inst = src_tag_instance(map, idx);
	uint32_t name_addr;
	uint32_t off;
	if (!inst)
		return NULL;
	name_addr = read_u32(inst + 0x10);
	if (name_addr < map->tag_base_addr)
		return NULL;
	off = name_addr - map->tag_base_addr;
	if (off >= map->tag_data_size)
		return NULL;
	if (!memchr(map->data + map->tag_data_offset + off, 0, map->tag_data_size - off))
		return NULL;
	return (char const *)(map->data + map->tag_data_offset + off);
}

static int32_t src_map_find_tag(struct forge_source_map const *map, uint32_t group_tag, char const *name)
{
	int32_t i;
	if (!map || !map->data || !name)
		return -1;
	for (i = 0; i < map->tag_count; i++)
	{
		uint8_t const *inst = src_tag_instance(map, i);
		char const *tname = src_tag_name(map, i);
		if (inst && tname &&
			forge_groups_compatible(read_u32(inst + 0x00), group_tag) &&
			_stricmp(tname, name) == 0)
		{
			return i;
		}
	}
	return -1;
}

/* ---------- 2D Blueprint Floorplan & Scenario Exporter ("Save Copy As...") */

struct forge_bsp_edge_2d
{
	float x0, y0, z0;
	float x1, y1, z1;
};

struct forge_map_blueprint_data
{
	char map_name[64];
	int16_t scenario_type; /* 0 = solo/co-op, 1 = multiplayer */
	float min_x, max_x;
	float min_y, max_y;
	float min_z, max_z;

	int32_t edge_count;
	struct forge_bsp_edge_2d *edges; /* heap-allocated array */

	struct forge_scenario_edit_state initial_state;
};

static int forge_load_map_blueprint(
	char const *map_path,
	struct forge_map_blueprint_data *out_blueprint)
{
	struct forge_source_map sm;
	uint8_t const *scnr_inst;
	uint32_t scnr_addr, scnr_off;
	int32_t bsp_count;
	uint32_t bsp_addr, ref_off, foff, bsp_sz, bsp_base, bsp_ptr, bsp_root_off;
	int32_t cbsp_count, edge_count, vert_count, e;
	uint32_t cbsp_addr, cbsp_off, edge_addr, vert_addr, edge_off, vert_off;

	if (!map_path || !out_blueprint)
		return 0;
	memset(out_blueprint, 0, sizeof(*out_blueprint));

	if (!forge_source_map_open(map_path, &sm))
		return 0;

	snprintf(out_blueprint->map_name, sizeof(out_blueprint->map_name), "%s", sm.map_name);
	out_blueprint->scenario_type = sm.scenario_type;

	scnr_inst = src_tag_instance(&sm, sm.scenario_tag_index);
	if (!scnr_inst)
	{
		forge_source_map_close(&sm);
		return 0;
	}
	scnr_addr = read_u32(scnr_inst + 0x14);
	if (scnr_addr < sm.tag_base_addr)
	{
		forge_source_map_close(&sm);
		return 0;
	}
	scnr_off = scnr_addr - sm.tag_base_addr;
	if (scnr_off + sizeof(struct scenario) > sm.tag_data_size)
	{
		forge_source_map_close(&sm);
		return 0;
	}

	/* Extract existing player spawns (offsetof(struct scenario, players), 52 bytes each) */
	{
		uint32_t pl_field = (uint32_t)offsetof(struct scenario, players);
		int32_t sp_count = read_s32(sm.data + sm.tag_data_offset + scnr_off + pl_field + 0);
		uint32_t sp_addr = read_u32(sm.data + sm.tag_data_offset + scnr_off + pl_field + 4);
		if (sp_count > 0 && sp_addr >= sm.tag_base_addr)
		{
			uint32_t sp_off = sp_addr - sm.tag_base_addr;
			int32_t i;
			for (i = 0; i < sp_count && out_blueprint->initial_state.player_spawn_count < FORGE_MAX_PLAYER_SPAWNS; i++)
			{
				uint32_t eo = sp_off + (uint32_t)i * 52u;
				struct forge_player_spawn_entry *dst;
				if (eo + 52u > sm.tag_data_size)
					break;
				dst = &out_blueprint->initial_state.player_spawns[out_blueprint->initial_state.player_spawn_count++];
				dst->x = read_f32(sm.data + sm.tag_data_offset + eo + 0x00u);
				dst->y = read_f32(sm.data + sm.tag_data_offset + eo + 0x04u);
				dst->z = read_f32(sm.data + sm.tag_data_offset + eo + 0x08u);
				dst->yaw = read_f32(sm.data + sm.tag_data_offset + eo + 0x0Cu);
				dst->team_index = read_s16(sm.data + sm.tag_data_offset + eo + 0x10u);
				dst->bsp_index = read_s16(sm.data + sm.tag_data_offset + eo + 0x12u);
				dst->type0 = read_s16(sm.data + sm.tag_data_offset + eo + 0x14u);
			}
		}
	}

	/* Extract existing netgame flags (offsetof(struct scenario, netgame_flags), 148 bytes each) */
	{
		uint32_t nf_field = (uint32_t)offsetof(struct scenario, netgame_flags);
		int32_t nf_count = read_s32(sm.data + sm.tag_data_offset + scnr_off + nf_field + 0);
		uint32_t nf_addr = read_u32(sm.data + sm.tag_data_offset + scnr_off + nf_field + 4);
		if (nf_count > 0 && nf_addr >= sm.tag_base_addr)
		{
			uint32_t nf_off = nf_addr - sm.tag_base_addr;
			int32_t i;
			for (i = 0; i < nf_count && out_blueprint->initial_state.netgame_flag_count < FORGE_MAX_NETGAME_FLAGS; i++)
			{
				uint32_t eo = nf_off + (uint32_t)i * 148u;
				struct forge_netgame_flag_entry *dst;
				if (eo + 148u > sm.tag_data_size)
					break;
				dst = &out_blueprint->initial_state.netgame_flags[out_blueprint->initial_state.netgame_flag_count++];
				dst->x = read_f32(sm.data + sm.tag_data_offset + eo + 0x00u);
				dst->y = read_f32(sm.data + sm.tag_data_offset + eo + 0x04u);
				dst->z = read_f32(sm.data + sm.tag_data_offset + eo + 0x08u);
				dst->yaw = read_f32(sm.data + sm.tag_data_offset + eo + 0x0Cu);
				dst->type = read_s16(sm.data + sm.tag_data_offset + eo + 0x10u);
				dst->usage_id = read_s16(sm.data + sm.tag_data_offset + eo + 0x12u);
			}
		}
	}

	/* Extract collision BSP wireframe edges */
	bsp_count = read_s32(sm.data + sm.tag_data_offset + scnr_off + (uint32_t)offsetof(struct scenario, structure_bsp_references) + 0);
	bsp_addr = read_u32(sm.data + sm.tag_data_offset + scnr_off + (uint32_t)offsetof(struct scenario, structure_bsp_references) + 4);
	if (bsp_count > 0 && bsp_addr >= sm.tag_base_addr)
	{
		ref_off = bsp_addr - sm.tag_base_addr;
		foff = read_u32(sm.data + sm.tag_data_offset + ref_off + 0x00u);
		bsp_sz = read_u32(sm.data + sm.tag_data_offset + ref_off + 0x04u);
		bsp_base = read_u32(sm.data + sm.tag_data_offset + ref_off + 0x08u);
		if (bsp_sz >= 0x100u && (uint64_t)foff + bsp_sz <= sm.file_size)
		{
			/* Xbox v5 structure_bsp_header has 'sbsp' signature at +0x14 (24 bytes), whereas CE v609 has 'sbsp' at +0x10 (20 bytes) */
			bsp_ptr = read_u32(sm.data + foff);
			if (bsp_ptr >= bsp_base)
			{
				bsp_root_off = foff + (bsp_ptr - bsp_base);
				if ((uint64_t)bsp_root_off + sizeof(struct structure_bsp) <= sm.file_size)
				{
					uint32_t wb_off = bsp_root_off + (uint32_t)offsetof(struct structure_bsp, world_bounds);
					uint32_t cb_field = bsp_root_off + (uint32_t)offsetof(struct structure_bsp, collision_bsp);
					out_blueprint->min_x = read_f32(sm.data + wb_off + 0x00u);
					out_blueprint->max_x = read_f32(sm.data + wb_off + 0x04u);
					out_blueprint->min_y = read_f32(sm.data + wb_off + 0x08u);
					out_blueprint->max_y = read_f32(sm.data + wb_off + 0x0Cu);
					out_blueprint->min_z = read_f32(sm.data + wb_off + 0x10u);
					out_blueprint->max_z = read_f32(sm.data + wb_off + 0x14u);

					cbsp_count = read_s32(sm.data + cb_field + 0);
					cbsp_addr = read_u32(sm.data + cb_field + 4);
					if (cbsp_count > 0 && cbsp_addr >= bsp_base)
					{
						cbsp_off = foff + (cbsp_addr - bsp_base);
						if ((uint64_t)cbsp_off + sizeof(struct collision_bsp) <= sm.file_size)
						{
							edge_count = read_s32(sm.data + cbsp_off + (uint32_t)offsetof(struct collision_bsp, edges) + 0);
							edge_addr = read_u32(sm.data + cbsp_off + (uint32_t)offsetof(struct collision_bsp, edges) + 4);
							vert_count = read_s32(sm.data + cbsp_off + (uint32_t)offsetof(struct collision_bsp, vertices) + 0);
							vert_addr = read_u32(sm.data + cbsp_off + (uint32_t)offsetof(struct collision_bsp, vertices) + 4);
							if (edge_count > 0 && vert_count > 0 && edge_addr >= bsp_base && vert_addr >= bsp_base)
							{
								edge_off = foff + (edge_addr - bsp_base);
								vert_off = foff + (vert_addr - bsp_base);
								if ((uint64_t)edge_off + (uint64_t)edge_count * 24u <= sm.file_size &&
									(uint64_t)vert_off + (uint64_t)vert_count * 16u <= sm.file_size)
								{
									out_blueprint->edges = (struct forge_bsp_edge_2d *)malloc(
										(size_t)edge_count * sizeof(struct forge_bsp_edge_2d));
									if (out_blueprint->edges)
									{
										for (e = 0; e < edge_count; e++)
										{
											uint32_t eo = edge_off + (uint32_t)e * 24u;
											int32_t v0 = read_s32(sm.data + eo + 0x00u);
											int32_t v1 = read_s32(sm.data + eo + 0x04u);
											if (v0 >= 0 && v0 < vert_count && v1 >= 0 && v1 < vert_count)
											{
												uint32_t vo0 = vert_off + (uint32_t)v0 * 16u;
												uint32_t vo1 = vert_off + (uint32_t)v1 * 16u;
												struct forge_bsp_edge_2d *ed =
													&out_blueprint->edges[out_blueprint->edge_count++];
												ed->x0 = read_f32(sm.data + vo0 + 0x00u);
												ed->y0 = read_f32(sm.data + vo0 + 0x04u);
												ed->z0 = read_f32(sm.data + vo0 + 0x08u);
												ed->x1 = read_f32(sm.data + vo1 + 0x00u);
												ed->y1 = read_f32(sm.data + vo1 + 0x04u);
												ed->z1 = read_f32(sm.data + vo1 + 0x08u);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	forge_source_map_close(&sm);
	return 1;
}

static void forge_free_map_blueprint(struct forge_map_blueprint_data *blueprint)
{
	if (!blueprint)
		return;
	if (blueprint->edges)
		free(blueprint->edges);
	memset(blueprint, 0, sizeof(*blueprint));
}

static uint8_t *build_blueprint_bmp_buffer(
	char const *custom_bmp_hint_path,
	struct forge_map_blueprint_data const *bp,
	struct forge_scenario_edit_state const *edits,
	uint32_t *out_bmp_size)
{
	enum { W = 140, H = 114, IMG_BYTES = 54 + 140 * 114 * 3 };
	uint8_t *img;
	uint8_t *px;
	int x, y, i;
	float span_x = 1.0f, span_y = 1.0f;

	if (out_bmp_size)
		*out_bmp_size = 0;

	/* Allow a user-supplied custom .bmp (if present) to be embedded into the .map */
	if (custom_bmp_hint_path && custom_bmp_hint_path[0])
	{
		FILE *f = fopen(custom_bmp_hint_path, "rb");
		if (f)
		{
			uint8_t hdr[14];
			size_t nr = fread(hdr, 1, sizeof(hdr), f);
			if (nr == sizeof(hdr) && hdr[0] == 'B' && hdr[1] == 'M')
			{
				uint32_t sig = read_u32(hdr + 6);
				uint32_t fsz = read_u32(hdr + 2);
				if (sig != 0x504D4246u && fsz >= 54u && fsz <= 4u * 1024u * 1024u)
				{
					uint8_t *custom_buf = (uint8_t *)malloc(fsz);
					if (custom_buf)
					{
						fseek(f, 0, SEEK_SET);
						if (fread(custom_buf, 1, fsz, f) == fsz)
						{
							fclose(f);
							if (out_bmp_size)
								*out_bmp_size = fsz;
							return custom_buf;
						}
						free(custom_buf);
					}
				}
			}
			fclose(f);
		}
	}

	img = (uint8_t *)calloc(1, IMG_BYTES);
	if (!img)
		return NULL;
	px = img + 54;
	img[0] = 'B';
	img[1] = 'M';
	write_u32(img + 2, IMG_BYTES);
	write_u32(img + 6, 0x504D4246u); /* 'FBMP' auto-generated blueprint marker in bfReserved1/2 */
	write_u32(img + 10, 54);
	write_u32(img + 14, 40);
	write_s32(img + 18, W);
	write_s32(img + 22, H);
	write_u16(img + 26, 1);
	write_u16(img + 28, 24);
	write_u32(img + 34, W * H * 3);

	for (y = 0; y < H; y++)
	{
		for (x = 0; x < W; x++)
		{
			uint8_t *p = px + (y * W + x) * 3;
			int grid = ((x % 14) == 0) || ((y % 14) == 0);
			p[0] = grid ? 62 : 34;
			p[1] = grid ? 42 : 22;
			p[2] = grid ? 20 : 10;
		}
	}

	if (bp && bp->edge_count > 0)
	{
		span_x = bp->max_x - bp->min_x;
		span_y = bp->max_y - bp->min_y;
		if (span_x < 0.001f) span_x = 1.0f;
		if (span_y < 0.001f) span_y = 1.0f;

		for (i = 0; i < bp->edge_count; i++)
		{
			int x0 = 6 + (int)(((bp->edges[i].x0 - bp->min_x) / span_x) * (W - 12));
			int y0 = 6 + (int)(((bp->edges[i].y0 - bp->min_y) / span_y) * (H - 12));
			int x1 = 6 + (int)(((bp->edges[i].x1 - bp->min_x) / span_x) * (W - 12));
			int y1 = 6 + (int)(((bp->edges[i].y1 - bp->min_y) / span_y) * (H - 12));
			int steps = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
			int s;
			if (steps > 200) steps = 200;
			for (s = 0; s <= steps; s++)
			{
				int cx = steps ? (x0 + (x1 - x0) * s / steps) : x0;
				int cy = steps ? (y0 + (y1 - y0) * s / steps) : y0;
				if (cx >= 0 && cx < W && cy >= 0 && cy < H)
				{
					uint8_t *p = px + (cy * W + cx) * 3;
					p[0] = 215; p[1] = 175; p[2] = 95;
				}
			}
		}
	}

	if (bp && edits)
	{
		for (i = 0; i < edits->placed_object_count; i++)
		{
			int cx = 6 + (int)(((edits->placed_objects[i].x - bp->min_x) / span_x) * (W - 12));
			int cy = 6 + (int)(((edits->placed_objects[i].y - bp->min_y) / span_y) * (H - 12));
			int dx, dy;
			for (dy = -1; dy <= 1; dy++)
				for (dx = -1; dx <= 1; dx++)
					if (cx + dx >= 0 && cx + dx < W && cy + dy >= 0 && cy + dy < H)
					{
						uint8_t *p = px + ((cy + dy) * W + (cx + dx)) * 3;
						p[0] = 80; p[1] = 240; p[2] = 255;
					}
		}
	}

	if (out_bmp_size)
		*out_bmp_size = (uint32_t)IMG_BYTES;
	return img;
}

static int32_t src_map_find_tag_or_variant(
	struct forge_source_map const *map,
	uint32_t group_tag,
	char const *tag_path)
{
	int32_t idx;
	size_t i;

	if (!map || !tag_path || !tag_path[0])
		return -1;

	idx = src_map_find_tag(map, group_tag, tag_path);
	if (idx >= 0)
		return idx;

	if (group_tag == 'mach' && _stricmp(tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0)
	{
		idx = src_map_find_tag(map, 'mach', "levels\\a30\\devices\\beam emitter\\beam emitter");
		if (idx >= 0)
			return idx;
	}

	if (group_tag == 'vehi')
	{
		static struct { char const *a; char const *b; } const vehi_pairs[] = {
			{ "vehicles\\warthog\\warthog", "vehicles\\warthog\\mp_warthog" },
			{ "vehicles\\ghost\\ghost", "vehicles\\ghost\\ghost_mp" },
			{ "vehicles\\scorpion\\scorpion", "vehicles\\scorpion\\scorpion_mp" },
			{ "vehicles\\banshee\\banshee", "vehicles\\banshee\\banshee_mp" },
			{ "vehicles\\c gun turret\\c gun turret", "vehicles\\c gun turret\\c gun turret_mp" },
			{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c gun turret\\c gun turret_mp" },
			{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c_gun_turret\\c_gun_turret_mp" }
		};
		for (i = 0; i < sizeof(vehi_pairs) / sizeof(vehi_pairs[0]); i++)
		{
			if (_stricmp(tag_path, vehi_pairs[i].a) == 0)
			{
				idx = src_map_find_tag(map, 'vehi', vehi_pairs[i].b);
				if (idx >= 0) return idx;
			}
			else if (_stricmp(tag_path, vehi_pairs[i].b) == 0)
			{
				idx = src_map_find_tag(map, 'vehi', vehi_pairs[i].a);
				if (idx >= 0) return idx;
			}
		}
	}
	return -1;
}

static uint32_t inplace_append_scenario_element(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_foff,
	uint32_t block_field_off,
	uint32_t elem_size)
{
	int32_t count;
	uint32_t addr;
	uint32_t new_foff;
	uint32_t old_foff = 0;

	if (!out || !out->data || scnr_foff + block_field_off + 12u > out->size)
		return 0;

	count = read_s32(out->data + scnr_foff + block_field_off + 0u);
	addr = read_u32(out->data + scnr_foff + block_field_off + 4u);
	if (count < 0 || count > 8192)
		count = 0;
	if (count > 0 && addr >= CE_TAG_BASE_ADDR)
	{
		old_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
		if ((uint64_t)old_foff + (uint64_t)count * elem_size > out->size)
			count = 0;
	}
	else
	{
		count = 0;
	}

	new_foff = buf_alloc(out, (uint32_t)(count + 1) * elem_size, 4);
	if (!new_foff || !out->data)
		return 0;

	if (count > 0 && old_foff > 0)
	{
		memcpy(out->data + new_foff, out->data + old_foff, (size_t)count * elem_size);
	}

	write_s32(out->data + scnr_foff + block_field_off + 0u, count + 1);
	write_u32(out->data + scnr_foff + block_field_off + 4u, CE_TAG_BASE_ADDR + (new_foff - tag_data_offset));
	write_u32(out->data + scnr_foff + block_field_off + 8u, 0);
	return new_foff + (uint32_t)count * elem_size;
}

static int16_t inplace_ensure_scenario_palette_entry(
	struct forge_byte_buf *out,
	struct forge_source_map const *base_map,
	uint32_t tag_data_offset,
	uint32_t scnr_foff,
	uint32_t pal_field_off,
	uint32_t elem_size,
	int32_t tag_idx)
{
	uint8_t const *inst;
	uint32_t inst_foff;
	uint32_t target_group, target_name_addr, target_handle;
	int32_t count, i, cur_tag_count;
	uint32_t addr;
	uint32_t new_eo;

	(void)base_map;
	if (!out || !out->data || tag_idx < 0 || scnr_foff + pal_field_off + 12u > out->size)
		return -1;
	cur_tag_count = read_s32(out->data + tag_data_offset + 0x0Cu);
	inst_foff = tag_data_offset + 0x28u + (uint32_t)tag_idx * 32u;
	if (tag_idx >= cur_tag_count || inst_foff + 32u > out->size)
		return -1;
	inst = out->data + inst_foff;

	target_group = read_u32(inst + 0x00u);
	target_handle = read_u32(inst + 0x0Cu);
	target_name_addr = read_u32(inst + 0x10u);

	count = read_s32(out->data + scnr_foff + pal_field_off + 0u);
	addr = read_u32(out->data + scnr_foff + pal_field_off + 4u);
	if (count > 0 && addr >= CE_TAG_BASE_ADDR)
	{
		uint32_t arr_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
		for (i = 0; i < count; i++)
		{
			uint32_t eo = arr_foff + (uint32_t)i * elem_size;
			if (eo + 16u <= out->size && read_u32(out->data + eo + 12u) == target_handle)
				return (int16_t)i;
		}
	}
	else
	{
		count = 0;
	}

	new_eo = inplace_append_scenario_element(out, tag_data_offset, scnr_foff, pal_field_off, elem_size);
	if (!new_eo || !out->data)
		return -1;

	write_u32(out->data + new_eo + 0u, target_group);
	write_u32(out->data + new_eo + 4u, target_name_addr);
	write_s32(out->data + new_eo + 8u, 0);
	write_u32(out->data + new_eo + 12u, target_handle);
	return (int16_t)count;
}

struct forge_pending_extra_gpart
{
	uint32_t tag_idx;
	uint32_t geom_idx;
	uint32_t part_idx;
	uint32_t vtx_count;
	uint32_t idx_count;
	uint32_t extra_vtx_rel_off;
	uint32_t extra_idx_rel_off;
};

struct forge_pending_bake_obj
{
	uint32_t group_tag;
	char tag_path[256];
	long faber_profile_idx;
	long faber_tag_idx;
	char xbox_map_path[512];
	int extra_gpart_count;
	struct forge_pending_extra_gpart extra_gparts[256];
	int32_t baked_tag_idx;
};

struct cb_deferred_media_patch
{
	uint32_t field_foff;
	uint32_t media_rel_off;
};

static struct forge_byte_buf s_cb_raw_media;
static struct cb_deferred_media_patch s_cb_media_patches[4096];
static int s_cb_media_patch_count = 0;

static void cb_reset_deferred_media(void)
{
	buf_free(&s_cb_raw_media);
	buf_init(&s_cb_raw_media, 65536u);
	s_cb_media_patch_count = 0;
}

static void cb_alloc_deferred_media(uint8_t const *bytes, uint32_t size, uint32_t align, uint32_t field_foff)
{
	uint32_t rel_off;
	if (!bytes || size == 0 || s_cb_media_patch_count >= 4096)
		return;
	rel_off = buf_alloc(&s_cb_raw_media, size, align);
	if (!s_cb_raw_media.data)
		return;
	memcpy(s_cb_raw_media.data + rel_off, bytes, size);
	s_cb_media_patches[s_cb_media_patch_count].field_foff = field_foff;
	s_cb_media_patches[s_cb_media_patch_count].media_rel_off = rel_off;
	s_cb_media_patch_count++;
}

static float cb_clamp_unit_float(float v)
{
	if (v != v) return 0.0f;
	if (v < -1.0f) return -1.0f;
	if (v > 1.0f) return 1.0f;
	return v;
}

static void cb_sanitize_faber_vertex_68(uint8_t *v, int32_t node_limit)
{
	float nx, ny, nz, bx, by, bz, tx, ty, tz;
	int16_t n0, n1;
	float w0, w1;

	if (node_limit <= 0)
		node_limit = 1;
	nx = cb_clamp_unit_float(read_f32(v + 0x0Cu));
	ny = cb_clamp_unit_float(read_f32(v + 0x10u));
	nz = cb_clamp_unit_float(read_f32(v + 0x14u));
	if (nx == 0.0f && ny == 0.0f && nz == 0.0f)
		nz = 1.0f;
	bx = cb_clamp_unit_float(read_f32(v + 0x18u));
	by = cb_clamp_unit_float(read_f32(v + 0x1Cu));
	bz = cb_clamp_unit_float(read_f32(v + 0x20u));
	tx = cb_clamp_unit_float(read_f32(v + 0x24u));
	ty = cb_clamp_unit_float(read_f32(v + 0x28u));
	tz = cb_clamp_unit_float(read_f32(v + 0x2Cu));
	write_f32(v + 0x0Cu, nx);
	write_f32(v + 0x10u, ny);
	write_f32(v + 0x14u, nz);
	write_f32(v + 0x18u, bx);
	write_f32(v + 0x1Cu, by);
	write_f32(v + 0x20u, bz);
	write_f32(v + 0x24u, tx);
	write_f32(v + 0x28u, ty);
	write_f32(v + 0x2Cu, tz);

	n0 = read_s16(v + 0x38u);
	n1 = read_s16(v + 0x3Au);
	w0 = read_f32(v + 0x3Cu);
	w1 = read_f32(v + 0x40u);
	if (n0 < 0 || n0 >= node_limit)
		n0 = 0;
	if (n1 < 0 || n1 >= node_limit)
	{
		n1 = n0;
		w0 = 1.0f;
		w1 = 0.0f;
	}
	if (w0 != w0 || w0 < 0.0f || w0 > 1.0f)
		w0 = 1.0f;
	if (w1 != w1 || w1 < 0.0f || w1 > 1.0f)
		w1 = 0.0f;
	write_s16(v + 0x38u, n0);
	write_s16(v + 0x3Au, n1);
	write_f32(v + 0x3Cu, w0);
	write_f32(v + 0x40u, w1);
}

static int32_t out_map_find_tag(
	struct forge_byte_buf const *out,
	uint32_t tag_data_offset,
	uint32_t group_tag,
	char const *tag_path)
{
	int32_t count, i;
	if (!out || !out->data || !tag_path || !tag_path[0])
		return -1;
	count = read_s32(out->data + tag_data_offset + 0x0Cu);
	for (i = 0; i < count; i++)
	{
		uint32_t io = tag_data_offset + 0x28u + (uint32_t)i * 32u;
		uint32_t g, naddr;
		if (io + 32u > out->size)
			break;
		g = read_u32(out->data + io + 0x00u);
		if (group_tag != 0 && g != group_tag)
			continue;
		naddr = read_u32(out->data + io + 0x10u);
		if (naddr >= CE_TAG_BASE_ADDR && tag_data_offset + (naddr - CE_TAG_BASE_ADDR) < out->size)
		{
			char const *nm = (char const *)(out->data + tag_data_offset + (naddr - CE_TAG_BASE_ADDR));
			if (_stricmp(nm, tag_path) == 0)
				return i;
		}
	}
	return -1;
}

static int32_t out_map_alloc_tag_instance(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	uint32_t g0,
	uint32_t g1,
	uint32_t g2,
	char const *tag_path,
	uint32_t def_foff,
	uint32_t in_res_map,
	uint32_t *out_handle,
	uint32_t *out_name_addr)
{
	int32_t idx;
	uint32_t io, name_foff, handle, name_addr;
	size_t nlen;

	if (!out || !out->data || !tag_path)
		return -1;
	idx = read_s32(out->data + tag_data_offset + 0x0Cu);
	if (idx < 0 || 0x28u + (uint32_t)(idx + 1) * 32u > scnr_off)
		return -1;

	nlen = strlen(tag_path);
	name_foff = buf_alloc(out, (uint32_t)nlen + 1u, 1);
	if (!name_foff || !out->data)
		return -1;
	memcpy(out->data + name_foff, tag_path, nlen + 1u);

	io = tag_data_offset + 0x28u + (uint32_t)idx * 32u;
	handle = (((uint32_t)(0xE174 + idx) & 0xFFFFu) << 16) | ((uint32_t)idx & 0xFFFFu);
	name_addr = CE_TAG_BASE_ADDR + (name_foff - tag_data_offset);

	write_u32(out->data + io + 0x00u, g0);
	write_u32(out->data + io + 0x04u, g1);
	write_u32(out->data + io + 0x08u, g2);
	write_u32(out->data + io + 0x0Cu, handle);
	write_u32(out->data + io + 0x10u, name_addr);
	write_u32(out->data + io + 0x14u, def_foff ? (CE_TAG_BASE_ADDR + (def_foff - tag_data_offset)) : 0u);
	write_u32(out->data + io + 0x18u, in_res_map);
	write_u32(out->data + io + 0x1Cu, 0u);

	write_s32(out->data + tag_data_offset + 0x0Cu, idx + 1);
	if (out_handle) *out_handle = handle;
	if (out_name_addr) *out_name_addr = name_addr;
	return idx;
}

static void out_map_init_schema_null_refs(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	struct tag_schema_definition const *def,
	uint32_t elem_foff,
	int32_t elem_count)
{
	int32_t elem;
	struct tag_schema_field const *field;

	if (!out || !out->data || !def || !def->fields || elem_count <= 0)
		return;

	for (elem = 0; elem < elem_count; elem++)
	{
		uint32_t base_foff = elem_foff + (uint32_t)elem * (uint32_t)def->size;
		if (base_foff + (uint32_t)def->size > out->size)
			break;

		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t fcount = field->count > 0 ? field->count : 1;
			uint32_t fstride = field->size > 0 ? (uint32_t)field->size : 0;

			for (k = 0; k < fcount; k++)
			{
				uint32_t f_foff = base_foff + (uint32_t)field->offset + (uint32_t)k * fstride;

				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def)
						out_map_init_schema_null_refs(out, tag_data_offset, sub_def, f_foff, 1);
				}
				else if (field->type == _tag_schema_reference)
				{
					if (f_foff + 16u <= out->size &&
						read_u32(out->data + f_foff + 0u) == 0u &&
						read_u32(out->data + f_foff + 12u) == 0u)
					{
						write_u32(out->data + f_foff + 0u, 0xFFFFFFFFu);
						write_u32(out->data + f_foff + 4u, 0u);
						write_u32(out->data + f_foff + 8u, 0u);
						write_u32(out->data + f_foff + 12u, 0xFFFFFFFFu);
					}
				}
				else if (field->type == _tag_schema_tag_index)
				{
					if (f_foff + 4u <= out->size && read_u32(out->data + f_foff) == 0u)
						write_u32(out->data + f_foff, 0xFFFFFFFFu);
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					int32_t cnt = read_s32(out->data + f_foff + 0u);
					uint32_t addr = read_u32(out->data + f_foff + 4u);
					if (child_def && cnt > 0 && addr >= CE_TAG_BASE_ADDR)
					{
						uint32_t child_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
						if ((uint64_t)child_foff + (uint64_t)(uint32_t)cnt * (uint32_t)child_def->size <= out->size)
							out_map_init_schema_null_refs(out, tag_data_offset, child_def, child_foff, cnt);
					}
				}
			}
		}
	}
}

static void out_map_clone_schema_blocks(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	struct tag_schema_definition const *def,
	uint32_t elem_foff,
	int32_t elem_count)
{
	int32_t elem;
	struct tag_schema_field const *field;

	if (!out || !out->data || !def || !def->fields || elem_count <= 0)
		return;

	for (elem = 0; elem < elem_count; elem++)
	{
		uint32_t base_foff = elem_foff + (uint32_t)elem * (uint32_t)def->size;
		if (base_foff + (uint32_t)def->size > out->size)
			break;

		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t fcount = field->count > 0 ? field->count : 1;
			uint32_t fstride = field->size > 0 ? (uint32_t)field->size : 0;

			for (k = 0; k < fcount; k++)
			{
				uint32_t f_foff = base_foff + (uint32_t)field->offset + (uint32_t)k * fstride;

				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def)
						out_map_clone_schema_blocks(out, tag_data_offset, sub_def, f_foff, 1);
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					int32_t cnt = read_s32(out->data + f_foff + 0u);
					uint32_t addr = read_u32(out->data + f_foff + 4u);
					write_u32(out->data + f_foff + 8u, 0u);

					if (cnt <= 0 || !child_def || child_def->size <= 0 || addr < CE_TAG_BASE_ADDR)
					{
						write_s32(out->data + f_foff + 0u, 0);
						write_u32(out->data + f_foff + 4u, 0u);
					}
					else
					{
						uint32_t child_src_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
						uint64_t total_bytes;
						if (field->maximum > 0 && !(field->flags & (1 << _tag_schema_tool_maximum_bit)) &&
							cnt > field->maximum)
						{
							cnt = field->maximum;
						}
						total_bytes = (uint64_t)(uint32_t)cnt * (uint64_t)(uint32_t)child_def->size;
						if ((uint64_t)child_src_foff + total_bytes > out->size)
						{
							write_s32(out->data + f_foff + 0u, 0);
							write_u32(out->data + f_foff + 4u, 0u);
						}
						else
						{
							uint32_t child_dst_foff = buf_alloc(out, (uint32_t)total_bytes, 4);
							if (child_dst_foff && out->data)
							{
								memcpy(out->data + child_dst_foff, out->data + child_src_foff, (size_t)total_bytes);
								write_s32(out->data + f_foff + 0u, cnt);
								write_u32(out->data + f_foff + 4u, CE_TAG_BASE_ADDR + (child_dst_foff - tag_data_offset));
								out_map_clone_schema_blocks(out, tag_data_offset, child_def, child_dst_foff, cnt);
							}
							else
							{
								write_s32(out->data + f_foff + 0u, 0);
								write_u32(out->data + f_foff + 4u, 0u);
							}
						}
					}
				}
				else if (field->type == _tag_schema_data)
				{
					int32_t sz = read_s32(out->data + f_foff + 0u);
					uint32_t addr = read_u32(out->data + f_foff + 12u);
					write_u32(out->data + f_foff + 16u, 0u);

					if (sz <= 0 || addr < CE_TAG_BASE_ADDR)
					{
						memset(out->data + f_foff, 0, 20u);
					}
					else
					{
						uint32_t td_src_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
						if ((uint64_t)td_src_foff + (uint32_t)sz > out->size)
						{
							memset(out->data + f_foff, 0, 20u);
						}
						else
						{
							uint32_t td_dst_foff = buf_alloc(out, (uint32_t)sz, 4);
							if (td_dst_foff && out->data)
							{
								memcpy(out->data + td_dst_foff, out->data + td_src_foff, (size_t)sz);
								write_s32(out->data + f_foff + 0u, sz);
								write_u32(out->data + f_foff + 12u, CE_TAG_BASE_ADDR + (td_dst_foff - tag_data_offset));
							}
							else
							{
								memset(out->data + f_foff, 0, 20u);
							}
						}
					}
				}
			}
		}
	}
}

/* =========================================================================
 * Native Xbox (.map v5) Tag Closure Baking into Custom Edition (.map v609)
 * ========================================================================= */

struct cb_xbox_bake_tag
{
	int32_t src_tag_idx;
	uint32_t fourcc;
	char name[256];
	uint32_t src_base_addr;
	uint32_t def_foff;
	int32_t baked_tag_idx;
	uint32_t baked_handle;
	uint32_t baked_name_addr;
	int is_reused_existing;
};

struct cb_faber_bake_tag
{
	uint32_t fourcc;
	uint32_t p0;
	uint32_t p1;
	uint32_t root_off;
	uint32_t root_sz;
	char name[256];
	uint32_t def_foff;
	int32_t baked_tag_idx;
	uint32_t baked_handle;
	uint32_t baked_name_addr;
};

static int cb_find_faber_vault_bundle_for_tag(
	uint32_t req_group,
	char const *req_path,
	long *out_profile,
	long *out_tag)
{
	long pcount, p, tcount, t;

	if (out_profile) *out_profile = -1;
	if (out_tag) *out_tag = -1;
	if (!req_path || !req_path[0])
		return 0;

	pcount = port_tag_profiles_count();
	if (pcount <= 0)
	{
		port_tag_profiles_scan();
		pcount = port_tag_profiles_count();
	}
	for (p = 0; p < pcount; p++)
	{
		tcount = port_tag_profiles_tag_count(p);
		for (t = 0; t < tcount; t++)
		{
			uint32_t tgrp = (uint32_t)port_tag_profiles_tag_group(p, t);
			char const *tnm = port_tag_profiles_tag_name(p, t);
			if (tnm && tnm[0] && forge_groups_compatible(tgrp, req_group) &&
				_stricmp(tnm, req_path) == 0)
			{
				if (out_profile) *out_profile = p;
				if (out_tag) *out_tag = t;
				return 1;
			}
		}
	}
	return 0;
}

static void cb_prescan_faber_vault_models(
	long prof_idx,
	long tag_idx,
	struct forge_pending_bake_obj *pobj,
	struct forge_byte_buf *extra_verts,
	struct forge_byte_buf *extra_strips)
{
	unsigned long bundle_size = 0;
	long b_handle = -1;
	uint8_t header_bytes[TAG_BUNDLE_HEADER_BYTES];
	uint32_t tags_size = 0;
	uint8_t *bundle_bytes = NULL;
	struct tag_bundle_loaded loaded;
	int32_t ti;

	if (!pobj || !extra_verts || !extra_strips || prof_idx < 0 || tag_idx < 0)
		return;
	b_handle = port_tag_profiles_bundle_open(prof_idx, tag_idx, &bundle_size);
	if (b_handle < 0 || bundle_size < TAG_BUNDLE_HEADER_BYTES)
	{
		if (b_handle >= 0) port_tag_profiles_bundle_close(b_handle);
		return;
	}
	if (!port_tag_profiles_bundle_read_at(b_handle, 0, header_bytes, TAG_BUNDLE_HEADER_BYTES))
	{
		port_tag_profiles_bundle_close(b_handle);
		return;
	}
	tags_size = tag_bundle_file_data_offset(header_bytes);
	if (!tags_size || tags_size > bundle_size)
	{
		port_tag_profiles_bundle_close(b_handle);
		return;
	}
	bundle_bytes = (uint8_t *)malloc(tags_size);
	if (!bundle_bytes || !port_tag_profiles_bundle_read_at(b_handle, 0, bundle_bytes, tags_size))
	{
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return;
	}
	memset(&loaded, 0, sizeof(loaded));
	if (tag_bundle_load_tags(bundle_bytes, tags_size, (uint32_t)bundle_size, &loaded) != _tag_bundle_status_ok)
	{
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return;
	}

	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		uint8_t const *ti_rec = loaded.tags + (uint32_t)ti * 32u;
		uint32_t grp = read_u32(ti_rec + 0u);
		uint32_t root_off = read_u32(ti_rec + 16u);
		uint32_t root_sz = read_u32(ti_rec + 20u);

		if (grp == 'mod2' && root_sz >= 0xE8u && root_off + 0xE8u <= loaded.data_size)
		{
			uint8_t const *mod2 = loaded.data + root_off;
			uint32_t mod2_flags = read_u32(mod2 + 0x00u);
			int local_nodes = (mod2_flags & 2u) != 0;
			int32_t mnode_cnt = *(int32_t const *)(mod2 + 0xB8u);
			int32_t geom_cnt = *(int32_t const *)(mod2 + 0xD0u);
			uint8_t const *geom_ptr = *(uint8_t *const *)(mod2 + 0xD4u);
			int32_t g_idx;

			if (geom_cnt > 0 && geom_ptr >= loaded.data &&
				geom_ptr + (uint64_t)(uint32_t)geom_cnt * 48u <= loaded.data + loaded.data_size)
			{
				for (g_idx = 0; g_idx < geom_cnt; g_idx++)
				{
					uint8_t const *geom = geom_ptr + (uint32_t)g_idx * 48u;
					int32_t part_cnt = *(int32_t const *)(geom + 0x24u);
					uint8_t const *part_ptr = *(uint8_t *const *)(geom + 0x28u);
					int32_t p_idx;

					if (part_cnt > 0 && part_ptr >= loaded.data &&
						part_ptr + (uint64_t)(uint32_t)part_cnt * 132u <= loaded.data + loaded.data_size)
					{
						for (p_idx = 0; p_idx < part_cnt && pobj->extra_gpart_count < 256; p_idx++)
						{
							uint8_t const *part = part_ptr + (uint32_t)p_idx * 132u;
							int32_t tri_cnt = read_s32(part + 0x48u);
							uint32_t strip_off = read_u32(part + 0x4Cu);
							int32_t vtx_cnt = read_s32(part + 0x58u);
							uint32_t vtx_off = read_u32(part + 0x64u);
							int32_t node_limit = local_nodes ? (int32_t)part[0x67u] : mnode_cnt;

							if (node_limit <= 0)
								node_limit = 1;
							if (vtx_cnt > 0 && tri_cnt > 0)
							{
								uint32_t vc = (uint32_t)vtx_cnt;
								uint32_t ic = (uint32_t)tri_cnt + 2u;
								uint32_t v_bytes = vc * 68u;
								uint32_t i_bytes = ic * 2u;

								if ((uint64_t)vtx_off + v_bytes <= loaded.file_data_size &&
									(uint64_t)strip_off + i_bytes <= loaded.file_data_size)
								{
									uint32_t v_rel = buf_alloc(extra_verts, v_bytes, 4);
									uint32_t i_rel = buf_alloc(extra_strips, (ic + 2u) * 2u, 4);
									if (extra_verts->data && extra_strips->data &&
										port_tag_profiles_bundle_read_at(b_handle, loaded.file_data_offset + vtx_off, extra_verts->data + v_rel, v_bytes) &&
										port_tag_profiles_bundle_read_at(b_handle, loaded.file_data_offset + strip_off, extra_strips->data + i_rel, i_bytes))
									{
										uint32_t k;
										struct forge_pending_extra_gpart *ogp =
											&pobj->extra_gparts[pobj->extra_gpart_count++];
										for (k = 0; k < vc; k++)
											cb_sanitize_faber_vertex_68(extra_verts->data + v_rel + k * 68u, node_limit);
										for (k = 0; k < ic; k++)
										{
											uint16_t idx_val = read_u16(extra_strips->data + i_rel + k * 2u);
											if (idx_val >= vc)
											{
												uint16_t safe_val = (k > 0) ? read_u16(extra_strips->data + i_rel + (k - 1u) * 2u) : 0u;
												if (safe_val >= vc) safe_val = 0u;
												write_u16(extra_strips->data + i_rel + k * 2u, safe_val);
											}
										}
										write_u16(extra_strips->data + i_rel + ic * 2u, 0xFFFFu);
										write_u16(extra_strips->data + i_rel + (ic + 1u) * 2u, 0xFFFFu);
										ogp->tag_idx = (uint32_t)ti;
										ogp->geom_idx = (uint32_t)g_idx;
										ogp->part_idx = (uint32_t)p_idx;
										ogp->vtx_count = vc;
										ogp->idx_count = ic;
										ogp->extra_vtx_rel_off = v_rel;
										ogp->extra_idx_rel_off = i_rel;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	free(bundle_bytes);
	port_tag_profiles_bundle_close(b_handle);
}

static void cb_faber_rebase_and_link_schema(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	struct tag_bundle_loaded const *loaded,
	uint32_t bundle_data_foff,
	struct tag_schema_definition const *def,
	uint32_t elem_rel_off,
	int32_t elem_count,
	struct cb_faber_bake_tag const *fb_tags,
	int32_t fb_tag_count,
	int depth)
{
	int32_t e;
	struct tag_schema_field const *field;
	uint32_t elem_size;

	if (!out || !out->data || !loaded || !loaded->data || !def || !def->fields ||
		elem_count <= 0 || def->size <= 0 || depth > 24)
		return;

	elem_size = (uint32_t)def->size;
	for (e = 0; e < elem_count; e++)
	{
		uint32_t base_rel_off = elem_rel_off + (uint32_t)e * elem_size;
		uint32_t base_foff = bundle_data_foff + base_rel_off;
		if ((uint64_t)base_rel_off + elem_size > loaded->data_size ||
			(uint64_t)base_foff + elem_size > out->size)
			break;

		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t fcount = (field->count > 0) ? field->count : 1;
			uint32_t fstride = (field->size > 0) ? (uint32_t)field->size : 0;

			for (k = 0; k < fcount; k++)
			{
				uint32_t f_rel_off = base_rel_off + (uint32_t)field->offset + (uint32_t)k * fstride;
				uint32_t f_foff = bundle_data_foff + f_rel_off;

				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def && (uint32_t)field->offset + (uint32_t)k * fstride + (uint32_t)sub_def->size <= elem_size)
					{
						cb_faber_rebase_and_link_schema(
							out, tag_data_offset, loaded, bundle_data_foff,
							sub_def, f_rel_off, 1, fb_tags, fb_tag_count, depth + 1);
					}
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					if (child_def && child_def->size > 0 &&
						f_rel_off + 12u <= loaded->data_size && f_foff + 12u <= out->size)
					{
						int32_t cnt = *(int32_t const *)(loaded->data + f_rel_off + 0u);
						uint8_t const *ptr = *(uint8_t *const *)(loaded->data + f_rel_off + 4u);
						uint32_t child_sz = (uint32_t)child_def->size;
						if (cnt > 0 && ptr >= loaded->data &&
							ptr + (uint64_t)(uint32_t)cnt * child_sz <= loaded->data + loaded->data_size)
						{
							uint32_t child_rel = (uint32_t)(ptr - loaded->data);
							write_s32(out->data + f_foff + 0u, cnt);
							write_u32(out->data + f_foff + 4u,
								CE_TAG_BASE_ADDR + (bundle_data_foff + child_rel - tag_data_offset));
							write_u32(out->data + f_foff + 8u, 0u);
							cb_faber_rebase_and_link_schema(
								out, tag_data_offset, loaded, bundle_data_foff,
								child_def, child_rel, cnt, fb_tags, fb_tag_count, depth + 1);
						}
						else
						{
							memset(out->data + f_foff, 0, 12u);
						}
					}
				}
				else if (field->type == _tag_schema_data)
				{
					if (f_rel_off + 20u <= loaded->data_size && f_foff + 20u <= out->size)
					{
						int32_t sz = *(int32_t const *)(loaded->data + f_rel_off + 0u);
						uint8_t const *ptr = *(uint8_t *const *)(loaded->data + f_rel_off + 12u);
						if (sz > 0 && ptr >= loaded->data &&
							ptr + (uint32_t)sz <= loaded->data + loaded->data_size)
						{
							uint32_t td_rel = (uint32_t)(ptr - loaded->data);
							write_s32(out->data + f_foff + 0u, sz);
							write_u32(out->data + f_foff + 4u, 0u);
							write_u32(out->data + f_foff + 8u, 0u);
							write_u32(out->data + f_foff + 12u,
								CE_TAG_BASE_ADDR + (bundle_data_foff + td_rel - tag_data_offset));
							write_u32(out->data + f_foff + 16u, 0u);
						}
						else
						{
							memset(out->data + f_foff, 0, 20u);
						}
					}
				}
				else if (field->type == _tag_schema_reference)
				{
					if (f_rel_off + 16u <= loaded->data_size && f_foff + 16u <= out->size)
					{
						int32_t b_idx = *(int32_t const *)(loaded->data + f_rel_off + 12u);
						if (b_idx >= 0 && b_idx < fb_tag_count && fb_tags[b_idx].baked_tag_idx >= 0)
						{
							write_u32(out->data + f_foff + 0u, fb_tags[b_idx].fourcc);
							write_u32(out->data + f_foff + 4u, fb_tags[b_idx].baked_name_addr);
							write_s32(out->data + f_foff + 8u, 0);
							write_u32(out->data + f_foff + 12u, fb_tags[b_idx].baked_handle);
						}
						else
						{
							write_u32(out->data + f_foff + 0u, 0xFFFFFFFFu);
							write_u32(out->data + f_foff + 4u, 0u);
							write_s32(out->data + f_foff + 8u, 0);
							write_u32(out->data + f_foff + 12u, 0xFFFFFFFFu);
						}
					}
				}
				else if (field->type == _tag_schema_tag_index)
				{
					if (f_rel_off + 4u <= loaded->data_size && f_foff + 4u <= out->size)
					{
						int32_t b_idx = *(int32_t const *)(loaded->data + f_rel_off);
						if (b_idx >= 0 && b_idx < fb_tag_count && fb_tags[b_idx].baked_tag_idx >= 0)
							write_u32(out->data + f_foff, fb_tags[b_idx].baked_handle);
						else
							write_u32(out->data + f_foff, 0xFFFFFFFFu);
					}
				}
			}
		}
	}
}

static int32_t out_map_bake_faber_vault_bundle(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	uint32_t orig_vtx_size,
	uint32_t orig_idx_size,
	struct forge_pending_bake_obj const *pobj)
{
	unsigned long bundle_size = 0;
	long b_handle = -1;
	uint8_t header_bytes[TAG_BUNDLE_HEADER_BYTES];
	uint32_t tags_size = 0;
	uint8_t *bundle_bytes = NULL;
	struct tag_bundle_loaded loaded;
	struct cb_faber_bake_tag *fb_tags = NULL;
	uint32_t bundle_data_foff;
	int32_t ti;
	int32_t root_baked_idx = -1;

	if (!out || !out->data || !pobj || pobj->faber_profile_idx < 0 || pobj->faber_tag_idx < 0)
		return -1;
	b_handle = port_tag_profiles_bundle_open(pobj->faber_profile_idx, pobj->faber_tag_idx, &bundle_size);
	if (b_handle < 0 || bundle_size < TAG_BUNDLE_HEADER_BYTES)
	{
		if (b_handle >= 0) port_tag_profiles_bundle_close(b_handle);
		return -1;
	}
	if (!port_tag_profiles_bundle_read_at(b_handle, 0, header_bytes, TAG_BUNDLE_HEADER_BYTES))
	{
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}
	tags_size = tag_bundle_file_data_offset(header_bytes);
	if (!tags_size || tags_size > bundle_size)
	{
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}
	bundle_bytes = (uint8_t *)malloc(tags_size);
	if (!bundle_bytes || !port_tag_profiles_bundle_read_at(b_handle, 0, bundle_bytes, tags_size))
	{
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}
	memset(&loaded, 0, sizeof(loaded));
	if (tag_bundle_load_tags(bundle_bytes, tags_size, (uint32_t)bundle_size, &loaded) != _tag_bundle_status_ok ||
		loaded.tag_count <= 0 || loaded.data_size == 0)
	{
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}

	fb_tags = (struct cb_faber_bake_tag *)calloc((size_t)loaded.tag_count, sizeof(*fb_tags));
	if (!fb_tags)
	{
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}

	bundle_data_foff = buf_alloc(out, loaded.data_size, 16);
	if (!bundle_data_foff || !out->data)
	{
		free(fb_tags);
		free(bundle_bytes);
		port_tag_profiles_bundle_close(b_handle);
		return -1;
	}
	memcpy(out->data + bundle_data_foff, loaded.data, loaded.data_size);

	/* 1. Allocate tag_instances in out for all tags in the bundle */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		uint8_t const *ti_rec = loaded.tags + (uint32_t)ti * 32u;
		struct cb_faber_bake_tag *ft = &fb_tags[ti];
		uint32_t name_off = read_u32(ti_rec + 12u);
		char const *tname = (name_off < loaded.names_size)
			? (char const *)(loaded.names + name_off) : "forge\\custom_tag";
		char alloc_name[256];

		ft->fourcc = read_u32(ti_rec + 0u);
		ft->p0 = read_u32(ti_rec + 4u);
		ft->p1 = read_u32(ti_rec + 8u);
		ft->root_off = read_u32(ti_rec + 16u);
		ft->root_sz = read_u32(ti_rec + 20u);
		ft->def_foff = bundle_data_foff + ft->root_off;
		snprintf(ft->name, sizeof(ft->name), "%s", tname);

		if (ft->fourcc == 'scex')
		{
			ft->p0 = 'shdr';
			ft->p1 = 0xFFFFFFFFu;
		}
		if (ti > 0 && out_map_find_tag(out, tag_data_offset, ft->fourcc, ft->name) >= 0)
		{
			snprintf(alloc_name, sizeof(alloc_name), "%.220s__fb%d_%d",
				ft->name, (int)pobj->faber_profile_idx, (int)pobj->faber_tag_idx);
		}
		else
		{
			snprintf(alloc_name, sizeof(alloc_name), "%s", ft->name);
		}

		ft->baked_tag_idx = out_map_alloc_tag_instance(
			out, tag_data_offset, scnr_off,
			ft->fourcc, ft->p0, ft->p1,
			alloc_name, ft->def_foff, 0u,
			&ft->baked_handle, &ft->baked_name_addr);
	}

	/* 2. Rebase all tag_blocks and tag_data to CE_TAG_BASE_ADDR and link all tag_references */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		struct cb_faber_bake_tag *ft = &fb_tags[ti];
		struct tag_schema_group const *sgrp = forge_schema_group_get(ft->fourcc);
		if (ft->baked_tag_idx < 0 || !sgrp || !sgrp->definition)
			continue;
		cb_faber_rebase_and_link_schema(
			out, tag_data_offset, &loaded, bundle_data_foff,
			sgrp->definition, ft->root_off, 1, fb_tags, loaded.tag_count, 0);
	}

	/* 3. Post-process mod2 geometry offsets, bitm pixels, snd! samples, shader types, and vehicle seats */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		struct cb_faber_bake_tag *ft = &fb_tags[ti];
		uint32_t def_foff = ft->def_foff;
		if (ft->baked_tag_idx < 0 || def_foff + ft->root_sz > out->size)
			continue;

		if (ft->fourcc == 'mod2' && ft->root_sz >= 0xE8u)
		{
			uint32_t mod2_flags = read_u32(out->data + def_foff + 0x00u);
			int local_nodes = (mod2_flags & 2u) != 0;
			int32_t mnode_cnt = read_s32(out->data + def_foff + 0xB8u);
			int32_t geom_cnt = read_s32(out->data + def_foff + 0xD0u);
			uint32_t geom_addr = read_u32(out->data + def_foff + 0xD4u);
			int32_t shdr_cnt = read_s32(out->data + def_foff + 0xDCu);
			int32_t g_idx;

			if (geom_cnt > 0 && geom_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t geom_base = tag_data_offset + (geom_addr - CE_TAG_BASE_ADDR);
				for (g_idx = 0; g_idx < geom_cnt; g_idx++)
				{
					uint32_t g_foff = geom_base + (uint32_t)g_idx * 48u;
					int32_t part_cnt, p_idx;
					uint32_t part_addr, part_base;

					if (g_foff + 48u > out->size)
						break;
					part_cnt = read_s32(out->data + g_foff + 0x24u);
					part_addr = read_u32(out->data + g_foff + 0x28u);
					if (part_cnt <= 0 || part_addr < CE_TAG_BASE_ADDR)
						continue;
					part_base = tag_data_offset + (part_addr - CE_TAG_BASE_ADDR);
					for (p_idx = 0; p_idx < part_cnt; p_idx++)
					{
						uint32_t p_foff = part_base + (uint32_t)p_idx * 132u;
						int16_t s_idx, cp_node, cs_node;
						int8_t prev_p, next_p;
						int gp_i;

						if (p_foff + 132u > out->size)
							break;
						s_idx = read_s16(out->data + p_foff + 0x04u);
						if (s_idx < 0 || (shdr_cnt > 0 && s_idx >= shdr_cnt))
							write_s16(out->data + p_foff + 0x04u, 0);
						prev_p = (int8_t)out->data[p_foff + 0x06u];
						next_p = (int8_t)out->data[p_foff + 0x07u];
						if (prev_p < -1 || prev_p >= part_cnt)
							out->data[p_foff + 0x06u] = 0xFFu;
						if (next_p < -1 || next_p >= part_cnt)
							out->data[p_foff + 0x07u] = 0xFFu;
						cp_node = read_s16(out->data + p_foff + 0x08u);
						cs_node = read_s16(out->data + p_foff + 0x0Au);
						if (cp_node < 0 || (mnode_cnt > 0 && cp_node >= mnode_cnt))
							write_s16(out->data + p_foff + 0x08u, 0);
						if (cs_node < 0 || (mnode_cnt > 0 && cs_node >= mnode_cnt))
							write_s16(out->data + p_foff + 0x0Au, 0);
						if (local_nodes)
						{
							uint8_t lnc = out->data[p_foff + 0x67u];
							uint8_t ni;
							if (lnc == 0 || lnc > 22)
							{
								lnc = 1;
								out->data[p_foff + 0x67u] = 1;
								out->data[p_foff + 0x68u] = 0;
							}
							for (ni = 0; ni < lnc; ni++)
							{
								if (mnode_cnt > 0 && out->data[p_foff + 0x68u + ni] >= (uint8_t)mnode_cnt)
									out->data[p_foff + 0x68u + ni] = 0;
							}
						}
						memset(out->data + p_foff + 0x20u, 0, 36u);

						for (gp_i = 0; gp_i < pobj->extra_gpart_count; gp_i++)
						{
							struct forge_pending_extra_gpart const *ogp = &pobj->extra_gparts[gp_i];
							if (ogp->tag_idx == (uint32_t)ti &&
								ogp->geom_idx == (uint32_t)g_idx &&
								ogp->part_idx == (uint32_t)p_idx)
							{
								write_s16(out->data + p_foff + 0x44u, 1);
								write_s32(out->data + p_foff + 0x48u,
									(int32_t)(ogp->idx_count >= 2u ? (ogp->idx_count - 2u) : 1u));
								write_u32(out->data + p_foff + 0x4Cu, orig_idx_size + ogp->extra_idx_rel_off);
								write_u32(out->data + p_foff + 0x50u, orig_idx_size + ogp->extra_idx_rel_off);
								write_s16(out->data + p_foff + 0x54u, 4);
								write_s32(out->data + p_foff + 0x58u, (int32_t)ogp->vtx_count);
								write_u32(out->data + p_foff + 0x5Cu, 0u);
								write_u32(out->data + p_foff + 0x60u, orig_vtx_size + ogp->extra_vtx_rel_off);
								write_u32(out->data + p_foff + 0x64u, orig_vtx_size + ogp->extra_vtx_rel_off);
								break;
							}
						}
					}
				}
			}
		}
		else if (ft->fourcc == 'bitm' && ft->root_sz >= 0x6Cu)
		{
			uint16_t bgrp_flags = read_u16(out->data + def_foff + 0x06u);
			int32_t b_cnt = read_s32(out->data + def_foff + 0x60u);
			uint32_t b_addr = read_u32(out->data + def_foff + 0x64u);
			int32_t b_i;

			/* Clear BITMAP_HALF_HUD_SCALE_BIT | BITMAP_FORCE_HUD_HIGH_RESOLUTION_SCALE_BIT so HUD scales are not halved twice */
			write_u16(out->data + def_foff + 0x06u, (uint16_t)(bgrp_flags & ~0x0090u));
			memset(out->data + def_foff + 0x1Cu, 0, 20u);

			if (b_cnt > 0 && b_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t b_base = tag_data_offset + (b_addr - CE_TAG_BASE_ADDR);
				for (b_i = 0; b_i < b_cnt; b_i++)
				{
					uint32_t bd_foff = b_base + (uint32_t)b_i * 48u;
					uint16_t bd_flags;
					uint32_t pix_off, pix_sz;

					if (bd_foff + 48u > out->size)
						break;
					bd_flags = read_u16(out->data + bd_foff + 0x0Eu);
					pix_off = read_u32(out->data + bd_foff + 0x18u);
					pix_sz = read_u32(out->data + bd_foff + 0x1Cu);
					bd_flags = (uint16_t)((bd_flags & ~0x0100u) | 0x0080u);
					write_u16(out->data + bd_foff + 0x0Eu, bd_flags);
					write_u32(out->data + bd_foff + 0x20u, ft->baked_handle);
					write_u32(out->data + bd_foff + 0x24u, 0xFFFFFFFFu);
					write_u32(out->data + bd_foff + 0x28u, 0u);
					write_u32(out->data + bd_foff + 0x2Cu, 0u);

					if (pix_sz > 0 && (uint64_t)pix_off + pix_sz <= loaded.file_data_size)
					{
						uint8_t *tmp_pix = (uint8_t *)malloc(pix_sz);
						if (tmp_pix && port_tag_profiles_bundle_read_at(b_handle, loaded.file_data_offset + pix_off, tmp_pix, pix_sz))
						{
							write_u32(out->data + bd_foff + 0x18u, 0u);
							cb_alloc_deferred_media(tmp_pix, pix_sz, 32, bd_foff + 0x18u);
						}
						free(tmp_pix);
					}
					else
					{
						write_u32(out->data + bd_foff + 0x18u, 0u);
						write_u32(out->data + bd_foff + 0x1Cu, 0u);
					}
				}
			}
		}
		else if (ft->fourcc == 'snd!' && ft->root_sz >= 0xA4u)
		{
			int32_t pr_cnt = read_s32(out->data + def_foff + 0x98u);
			uint32_t pr_addr = read_u32(out->data + def_foff + 0x9Cu);
			int32_t pr_i;

			write_u32(out->data + def_foff + 0x70u, 0xFFFFFFFFu);
			write_u32(out->data + def_foff + 0x74u, 0u);
			write_u32(out->data + def_foff + 0x78u, 0u);
			write_u32(out->data + def_foff + 0x7Cu, 0xFFFFFFFFu);
			write_s16(out->data + def_foff + 0x80u, 0);

			if (pr_cnt > 0 && pr_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t pr_base = tag_data_offset + (pr_addr - CE_TAG_BASE_ADDR);
				for (pr_i = 0; pr_i < pr_cnt; pr_i++)
				{
					uint32_t pr_foff = pr_base + (uint32_t)pr_i * 72u;
					int32_t pm_cnt, pm_i;
					uint32_t pm_addr, pm_base;

					if (pr_foff + 72u > out->size)
						break;
					pm_cnt = read_s32(out->data + pr_foff + 0x3Cu);
					pm_addr = read_u32(out->data + pr_foff + 0x40u);
					if (pm_cnt <= 0 || pm_addr < CE_TAG_BASE_ADDR)
						continue;
					pm_base = tag_data_offset + (pm_addr - CE_TAG_BASE_ADDR);
					for (pm_i = 0; pm_i < pm_cnt; pm_i++)
					{
						uint32_t pm_foff = pm_base + (uint32_t)pm_i * 124u;
						int32_t smp_sz;
						uint32_t smp_off;

						if (pm_foff + 124u > out->size)
							break;
						smp_sz = read_s32(out->data + pm_foff + 0x40u);
						smp_off = read_u32(out->data + pm_foff + 0x48u);
						write_u32(out->data + pm_foff + 0x2Cu, 0xFFFFFFFFu);
						write_u32(out->data + pm_foff + 0x30u, 0u);
						write_u32(out->data + pm_foff + 0x34u, ft->baked_handle);
						write_u32(out->data + pm_foff + 0x3Cu, ft->baked_handle);

						if (smp_sz > 0 && (uint64_t)smp_off + (uint32_t)smp_sz <= loaded.file_data_size)
						{
							uint8_t *tmp_snd = (uint8_t *)malloc((uint32_t)smp_sz);
							if (tmp_snd && port_tag_profiles_bundle_read_at(
								b_handle, loaded.file_data_offset + smp_off, tmp_snd, (uint32_t)smp_sz))
							{
								write_s32(out->data + pm_foff + 0x40u, smp_sz);
								write_u32(out->data + pm_foff + 0x44u, 0u);
								write_u32(out->data + pm_foff + 0x48u, 0u);
								write_u32(out->data + pm_foff + 0x4Cu, 0u);
								write_u32(out->data + pm_foff + 0x50u, 0u);
								cb_alloc_deferred_media(tmp_snd, (uint32_t)smp_sz, 16, pm_foff + 0x48u);
							}
							free(tmp_snd);
						}
						else
						{
							memset(out->data + pm_foff + 0x40u, 0, 20u);
						}
					}
				}
			}
		}
		else if (ft->fourcc == 'swat' && ft->root_sz >= 0x28u)
			write_u16(out->data + def_foff + 0x24u, 8u);
		else if (ft->fourcc == 'sgla' && ft->root_sz >= 0x28u)
			write_u16(out->data + def_foff + 0x24u, 9u);
		else if (ft->fourcc == 'smet' && ft->root_sz >= 0x28u)
			write_u16(out->data + def_foff + 0x24u, 10u);
		else if (ft->fourcc == 'spla' && ft->root_sz >= 0x28u)
			write_u16(out->data + def_foff + 0x24u, 11u);
		else if (ft->fourcc == 'ligh' && ft->root_sz >= 0x58u)
		{
			uint32_t flags = read_u32(out->data + def_foff + 0x00u);
			float rad = read_f32(out->data + def_foff + 0x04u);
			write_u32(out->data + def_foff + 0x00u, flags | 0x01u);
			if (rad < 4.5f)
				write_f32(out->data + def_foff + 0x04u, 4.5f);
		}
		else if ((ft->fourcc == 'scen' || ft->fourcc == 'lifi' || ft->fourcc == 'mach') && ft->root_sz >= 0x17Cu)
		{
			if (read_f32(out->data + def_foff + 0x04u) < 1.5f)
				write_f32(out->data + def_foff + 0x04u, 1.5f);
			if (read_f32(out->data + def_foff + 0x104u) < 8.0f)
				write_f32(out->data + def_foff + 0x104u, 8.0f);
		}
		else if (ft->fourcc == 'vehi' && ft->root_sz >= 0x3F0u)
		{
			int32_t seat_cnt = read_s32(out->data + def_foff + 0x2DCu);
			uint32_t seat_addr = read_u32(out->data + def_foff + 0x2E0u);
			write_u32(out->data + def_foff + 0x17Cu,
				read_u32(out->data + def_foff + 0x17Cu) | (1u << 29) | (1u << 4));
			if (seat_cnt > 0 && seat_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t s0_foff = tag_data_offset + (seat_addr - CE_TAG_BASE_ADDR);
				if (s0_foff + 284u <= out->size)
				{
					uint32_t sflags = read_u32(out->data + s0_foff + 0x00u);
					char const *slabel = (char const *)(out->data + s0_foff + 0x04u);
					sflags &= ~0x01u;
					sflags |= (1u << 2) | (1u << 4) | (1u << 5);
					if (strstr(ft->name, "wraith") != NULL)
						sflags |= (1u << 3);
					write_u32(out->data + s0_foff + 0x00u, sflags);
					if (_stricmp(slabel, "W-driver") != 0 &&
						_stricmp(slabel, "G-driver") != 0 &&
						_stricmp(slabel, "B-driver") != 0 &&
						_stricmp(slabel, "S-driver") != 0 &&
						_stricmp(slabel, "R-driver") != 0 &&
						_stricmp(slabel, "scorpion-driver") != 0 &&
						_stricmp(slabel, "CT-driver") != 0)
					{
						memset(out->data + s0_foff + 0x04u, 0, 32u);
						memcpy(out->data + s0_foff + 0x04u, "W-driver", 8u);
					}
				}
			}
		}
		else if (ft->fourcc == 'coll' && ft->root_sz >= 0x24Cu && strstr(ft->name, "wraith") != NULL)
		{
			int32_t mat_cnt = read_s32(out->data + def_foff + 0x234u);
			uint32_t mat_addr = read_u32(out->data + def_foff + 0x238u);
			int32_t reg_cnt = read_s32(out->data + def_foff + 0x240u);
			uint32_t reg_addr = read_u32(out->data + def_foff + 0x244u);
			int32_t k;
			write_f32(out->data + def_foff + 0x08u, 0.0f);
			write_u32(out->data + def_foff + 0xB4u, 0xFFFFFFFFu);
			write_f32(out->data + def_foff + 0xB8u, 0.0f);
			write_u32(out->data + def_foff + 0xC8u, 0xFFFFFFFFu);
			write_f32(out->data + def_foff + 0xCCu, 0.0f);
			if (mat_cnt > 0 && mat_cnt <= 64 && mat_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t m_base = tag_data_offset + (mat_addr - CE_TAG_BASE_ADDR);
				for (k = 0; k < mat_cnt && m_base + (uint32_t)(k + 1) * 0x48u <= out->size; k++)
				{
					write_f32(out->data + m_base + (uint32_t)k * 0x48u + 0x2Cu, 0.0f);
					write_f32(out->data + m_base + (uint32_t)k * 0x48u + 0x3Cu, 0.0f);
				}
			}
			if (reg_cnt > 0 && reg_cnt <= 64 && reg_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t r_base = tag_data_offset + (reg_addr - CE_TAG_BASE_ADDR);
				for (k = 0; k < reg_cnt && r_base + (uint32_t)(k + 1) * 0x54u <= out->size; k++)
				{
					uint32_t r_foff = r_base + (uint32_t)k * 0x54u;
					write_u32(out->data + r_foff + 0x20u, read_u32(out->data + r_foff + 0x20u) & ~0x0Eu);
					write_f32(out->data + r_foff + 0x28u, 0.0f);
					write_u32(out->data + r_foff + 0x44u, 0xFFFFFFFFu);
				}
			}
		}
	}

	root_baked_idx = fb_tags[0].baked_tag_idx;
	free(fb_tags);
	free(bundle_bytes);
	port_tag_profiles_bundle_close(b_handle);
	return root_baked_idx;
}

static int cb_xbox_map_contains_tag_fast(
	char const *cand_path,
	uint32_t group_tag,
	char const *tag_path)
{
	FILE *f;
	uint8_t hdr[0x800];
	long fsz;
	uint32_t tag_data_off, tag_data_sz;
	uint8_t *tag_buf;
	uint32_t inst_addr, base_addr, inst_off;
	int32_t tag_count, i;
	int found = 0;

	f = fopen(cand_path, "rb");
	if (!f)
		return 0;
	if (fseek(f, 0, SEEK_END) != 0 || (fsz = ftell(f)) < 0x800L ||
		fseek(f, 0, SEEK_SET) != 0 || fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr))
	{
		fclose(f);
		return 0;
	}
	if (read_u32(hdr) != CACHE_HEADER_SIGNATURE || read_s32(hdr + 0x04) != CACHE_VERSION_XBOX)
	{
		fclose(f);
		return 0;
	}
	tag_data_off = read_u32(hdr + 0x10);
	tag_data_sz = read_u32(hdr + 0x14);
	if ((uint64_t)tag_data_off + (uint64_t)tag_data_sz > (uint64_t)fsz)
	{
		struct forge_source_map xmap;
		fclose(f);
		if (!forge_source_map_open(cand_path, &xmap))
			return 0;
		found = (xmap.version == CACHE_VERSION_XBOX && src_map_find_tag_or_variant(&xmap, group_tag, tag_path) >= 0);
		forge_source_map_close(&xmap);
		return found;
	}
	if (tag_data_sz < 0x28u || tag_data_sz > 64u * 1024u * 1024u)
	{
		fclose(f);
		return 0;
	}
	tag_buf = (uint8_t *)malloc(tag_data_sz);
	if (!tag_buf)
	{
		fclose(f);
		return 0;
	}
	if (fseek(f, (long)tag_data_off, SEEK_SET) != 0 || fread(tag_buf, 1, tag_data_sz, f) != tag_data_sz)
	{
		free(tag_buf);
		fclose(f);
		return 0;
	}
	fclose(f);

	inst_addr = read_u32(tag_buf + 0x00);
	tag_count = read_s32(tag_buf + 0x0C);
	base_addr = (inst_addr >= 0x24u) ? (inst_addr - 0x24u) : XBOX_TAG_BASE_ADDR;
	inst_off = inst_addr - base_addr;
	if (tag_count > 0 && tag_count <= 65535 && inst_off + (uint32_t)tag_count * 32u <= tag_data_sz)
	{
		for (i = 0; i < tag_count; i++)
		{
			uint8_t const *inst = tag_buf + inst_off + (uint32_t)i * 32u;
			uint32_t grp = read_u32(inst + 0x00);
			uint32_t naddr = read_u32(inst + 0x10);
			if (forge_groups_compatible(grp, group_tag) && naddr >= base_addr && (naddr - base_addr) < tag_data_sz)
			{
				char const *nm = (char const *)(tag_buf + (naddr - base_addr));
				if (_stricmp(nm, tag_path) == 0)
				{
					found = 1;
					break;
				}
			}
		}
	}
	free(tag_buf);
	return found;
}

static int cb_find_xbox_donor_map_for_tag(
	uint32_t group_tag,
	char const *tag_path,
	char *out_map_path,
	size_t out_map_path_sz)
{
	static char const *const map_files[] = {
		"a10.map", "a30.map", "a50.map", "b30.map", "b40.map",
		"c10.map", "c20.map", "c40.map", "d20.map", "d40.map",
		"beavercreek.map", "boardingaction.map", "bloodgulch.map",
		"carousel.map", "chillout.map", "damnation.map", "hangemhigh.map",
		"longest.map", "prisoner.map", "putput.map", "ratrace.map",
		"sidewinder.map", "wizard.map"
	};
	static char const *const map_dirs[] = {
		"maps",
		"assets/maps",
		"../assets/maps"
	};
	size_t m, d;

	if (!tag_path || !tag_path[0] || !out_map_path || out_map_path_sz == 0)
		return 0;
	out_map_path[0] = '\0';

	/* 1. Check .fxtg compressed Xbox tag bundles in OpenCE Forge/cached_tags/ */
	{
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA("OpenCE Forge/cached_tags/*.fxtg", &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					char fxtg_path[512];
					FILE *fp;
					snprintf(fxtg_path, sizeof(fxtg_path), "OpenCE Forge/cached_tags/%s", fd.cFileName);
					fp = fopen(fxtg_path, "rb");
					if (fp)
					{
						uint8_t fhdr[304];
						if (fread(fhdr, 1, sizeof(fhdr), fp) == sizeof(fhdr) &&
							memcmp(fhdr, "FXTG", 4) == 0 && read_u32(fhdr + 4) == 1u)
						{
							uint32_t fgrp = read_u32(fhdr + 8);
							char const *fpath = (char const *)(fhdr + 12);
							uint32_t uncomp_sz = read_u32(fhdr + 296);
							uint32_t comp_sz = read_u32(fhdr + 300);
							if (forge_groups_compatible(fgrp, group_tag) &&
								_stricmp(fpath, tag_path) == 0 &&
								uncomp_sz >= 0x800u && uncomp_sz <= 512u * 1024u * 1024u && comp_sz > 0)
							{
								uint8_t *cbuf = (uint8_t *)malloc(comp_sz);
								uint8_t *ubuf = (uint8_t *)malloc(uncomp_sz);
								uLongf dst_len = (uLongf)uncomp_sz;
								if (cbuf && ubuf &&
									fread(cbuf, 1, comp_sz, fp) == comp_sz &&
									uncompress(ubuf, &dst_len, cbuf, (uLong)comp_sz) == Z_OK &&
									dst_len == (uLongf)uncomp_sz)
								{
									char tmp_donor[512];
									FILE *df;
									platform_translate_path("z:\\cache_donor.map", tmp_donor, sizeof(tmp_donor));
									df = fopen(tmp_donor, "wb");
									if (df)
									{
										fwrite(ubuf, 1, uncomp_sz, df);
										fclose(df);
										snprintf(out_map_path, out_map_path_sz, "%s", tmp_donor);
										free(cbuf);
										free(ubuf);
										fclose(fp);
										CloseHandle(h);
										return 1;
									}
								}
								free(cbuf);
								free(ubuf);
							}
						}
						fclose(fp);
					}
				}
			} while (FindNextFileA(h, &fd));
			CloseHandle(h);
		}
	}

	for (d = 0; d < sizeof(map_dirs) / sizeof(map_dirs[0]); d++)
	{
		for (m = 0; m < sizeof(map_files) / sizeof(map_files[0]); m++)
		{
			char cand[512];

			snprintf(cand, sizeof(cand), "%s/%s", map_dirs[d], map_files[m]);
			if (cb_xbox_map_contains_tag_fast(cand, group_tag, tag_path))
			{
				snprintf(out_map_path, out_map_path_sz, "%s", cand);
				return 1;
			}
		}
	}
	return 0;
}

static real_vector3d cb_uncompress_int32_to_real_vector3d(uint32_t compressed)
{
	real_vector3d v;
	float value = (float)(int32_t)(compressed << 21);

	v.i = (value * (1.0f / 1048576.0f) + 1.0f) * (1.0f / 2047.0f);
	compressed >>= 11;
	value = (float)(int32_t)(compressed << 21);
	v.j = (value * (1.0f / 1048576.0f) + 1.0f) * (1.0f / 2047.0f);
	compressed >>= 11;
	value = (float)(int32_t)(compressed << 22);
	v.k = (value * (1.0f / 2097152.0f) + 1.0f) * (1.0f / 1023.0f);
	return v;
}

static void cb_uncompress_xbox_model_vertex(uint8_t const *src32, uint8_t *dst68, int32_t node_limit)
{
	real_vector3d v;
	int16_t u_s16, v_s16, w_s16;
	int16_t n0, n1;
	float w0, w1;
	uint32_t k;

	memset(dst68, 0, 68u);
	memcpy(dst68 + 0x00u, src32 + 0x00u, 12u); /* position (3 floats) */
	for (k = 0; k < 3u; k++)
	{
		v = cb_uncompress_int32_to_real_vector3d(read_u32(src32 + 0x0Cu + k * 4u));
		write_f32(dst68 + 0x0Cu + k * 12u, PIN(v.i, -1.0f, 1.0f));
		write_f32(dst68 + 0x10u + k * 12u, PIN(v.j, -1.0f, 1.0f));
		write_f32(dst68 + 0x14u + k * 12u, PIN(v.k, -1.0f, 1.0f));
	}

	u_s16 = read_s16(src32 + 0x18u);
	v_s16 = read_s16(src32 + 0x1Au);
	write_f32(dst68 + 0x30u, ((float)u_s16 * 2.0f + 1.0f) / 65535.0f);
	write_f32(dst68 + 0x34u, ((float)v_s16 * 2.0f + 1.0f) / 65535.0f);

	if (node_limit <= 0) node_limit = 1;
	n0 = (int16_t)(src32[0x1Cu] / 3u);
	n1 = (int16_t)(src32[0x1Du] / 3u);
	if (n0 < 0 || n0 >= node_limit) n0 = 0;
	if (n1 < 0 || n1 >= node_limit) n1 = 0;
	write_s16(dst68 + 0x38u, n0);
	write_s16(dst68 + 0x3Au, n1);

	w_s16 = read_s16(src32 + 0x1Eu);
	w0 = ((float)w_s16 * 2.0f + 1.0f) / 65535.0f;
	w0 = PIN(w0, 0.0f, 1.0f);
	w1 = 1.0f - w0;
	write_f32(dst68 + 0x3Cu, w0);
	write_f32(dst68 + 0x40u, w1);
}

static void cb_prescan_xbox_models(
	struct forge_source_map const *xmap,
	int32_t root_tag_idx,
	struct forge_pending_bake_obj *pobj,
	struct forge_byte_buf *extra_verts,
	struct forge_byte_buf *extra_strips)
{
	uint8_t visited[8192];
	int32_t queue[2048];
	int q_head = 0, q_tail = 0;

	if (!xmap || root_tag_idx < 0 || root_tag_idx >= xmap->tag_count || root_tag_idx >= 8192)
		return;
	memset(visited, 0, sizeof(visited));
	visited[root_tag_idx] = 1;
	queue[q_tail++] = root_tag_idx;

	while (q_head < q_tail)
	{
		int32_t tidx = queue[q_head++];
		uint8_t const *inst = src_tag_instance(xmap, tidx);
		uint32_t fourcc, daddr;
		struct tag_schema_group const *sgrp;

		if (!inst) continue;
		fourcc = read_u32(inst + 0x00u);
		daddr = read_u32(inst + 0x14u);
		if (daddr < xmap->tag_base_addr || daddr >= xmap->tag_base_addr + xmap->tag_data_size)
			continue;

		if (fourcc == 'mode' || fourcc == 'mod2')
		{
			uint32_t m_foff = xmap->tag_data_offset + (daddr - xmap->tag_base_addr);
			if (m_foff + 0xE8u <= xmap->file_size)
			{
				int32_t node_cnt = read_s32(xmap->data + m_foff + 0xB8u);
				int32_t geom_cnt = read_s32(xmap->data + m_foff + 0xD0u);
				uint32_t geom_addr = read_u32(xmap->data + m_foff + 0xD4u);
				int g_idx;

				if (node_cnt <= 0) node_cnt = 1;
				if (geom_cnt > 0 && geom_cnt <= 64 && geom_addr >= xmap->tag_base_addr &&
					(geom_addr - xmap->tag_base_addr) + (uint32_t)geom_cnt * 48u <= xmap->tag_data_size)
				{
					uint32_t g_base = xmap->tag_data_offset + (geom_addr - xmap->tag_base_addr);
					for (g_idx = 0; g_idx < geom_cnt; g_idx++)
					{
						uint32_t g_foff = g_base + (uint32_t)g_idx * 48u;
						int32_t part_cnt = read_s32(xmap->data + g_foff + 36u);
						uint32_t part_addr = read_u32(xmap->data + g_foff + 40u);
						int p_idx;
						if (part_cnt <= 0 || part_cnt > 64 || part_addr < xmap->tag_base_addr ||
							(part_addr - xmap->tag_base_addr) + (uint32_t)part_cnt * 104u > xmap->tag_data_size)
							continue;
						for (p_idx = 0; p_idx < part_cnt && pobj->extra_gpart_count < 256; p_idx++)
						{
							uint32_t p_foff = xmap->tag_data_offset + (part_addr - xmap->tag_base_addr) + (uint32_t)p_idx * 104u;
							int16_t tri_type = read_s16(xmap->data + p_foff + 68u);
							int32_t tri_cnt = read_s32(xmap->data + p_foff + 72u);
							uint32_t tb_hw_addr = read_u32(xmap->data + p_foff + 80u);
							int16_t vtx_type = read_s16(xmap->data + p_foff + 84u);
							int32_t vtx_cnt = read_s32(xmap->data + p_foff + 88u);
							uint32_t vb_hw_addr = read_u32(xmap->data + p_foff + 100u);
							uint32_t vtx_stride = (vtx_type == 5) ? 32u : 68u;

							if (tri_type == 1 && tri_cnt > 0 && vtx_cnt >= 3 &&
								tb_hw_addr >= xmap->tag_base_addr && vb_hw_addr >= xmap->tag_base_addr &&
								(tb_hw_addr - xmap->tag_base_addr) + 12u <= xmap->tag_data_size &&
								(vb_hw_addr - xmap->tag_base_addr) + 12u <= xmap->tag_data_size)
							{
								uint32_t tb_hdr_foff = xmap->tag_data_offset + (tb_hw_addr - xmap->tag_base_addr);
								uint32_t vb_hdr_foff = xmap->tag_data_offset + (vb_hw_addr - xmap->tag_base_addr);
								uint32_t ib_data_addr = read_u32(xmap->data + tb_hdr_foff + 4u);
								uint32_t vb_data_addr = read_u32(xmap->data + vb_hdr_foff + 4u);
								if (ib_data_addr >= xmap->tag_base_addr && vb_data_addr >= xmap->tag_base_addr)
								{
									uint32_t i_file_off = xmap->tag_data_offset + (ib_data_addr - xmap->tag_base_addr);
									uint32_t v_file_off = xmap->tag_data_offset + (vb_data_addr - xmap->tag_base_addr);
									uint32_t ic = (uint32_t)tri_cnt + 2u;

									if ((uint64_t)v_file_off + (uint64_t)vtx_cnt * vtx_stride <= xmap->file_size &&
										(uint64_t)i_file_off + (uint64_t)ic * 2u <= xmap->file_size)
									{
										uint32_t v_rel = buf_alloc(extra_verts, (uint32_t)vtx_cnt * 68u, 4);
										uint32_t i_rel = buf_alloc(extra_strips, (ic + 2u) * 2u, 4);
										if (extra_verts->data && extra_strips->data)
										{
											int32_t vi;
											uint32_t k;
											struct forge_pending_extra_gpart *ogp = &pobj->extra_gparts[pobj->extra_gpart_count++];
											for (vi = 0; vi < vtx_cnt; vi++)
											{
												uint8_t const *vsrc = xmap->data + v_file_off + (uint32_t)vi * vtx_stride;
												uint8_t *vdst = extra_verts->data + v_rel + (uint32_t)vi * 68u;
												if (vtx_stride == 32u)
													cb_uncompress_xbox_model_vertex(vsrc, vdst, node_cnt);
												else
												{
													int16_t n0, n1;
													memcpy(vdst, vsrc, 68u);
													n0 = read_s16(vdst + 0x38u);
													n1 = read_s16(vdst + 0x3Au);
													if (n0 < 0 || n0 >= node_cnt) write_s16(vdst + 0x38u, 0);
													if (n1 < 0 || n1 >= node_cnt) write_s16(vdst + 0x3Au, 0);
												}
											}
											memcpy(extra_strips->data + i_rel, xmap->data + i_file_off, ic * 2u);
											while (ic > 3u && read_u16(extra_strips->data + i_rel + (ic - 1u) * 2u) >= (uint32_t)vtx_cnt)
												ic--;
											for (k = 0; k < ic; k++)
											{
												uint16_t idx_val = read_u16(extra_strips->data + i_rel + k * 2u);
												if (idx_val >= (uint32_t)vtx_cnt)
												{
													uint16_t safe_val = (k > 0) ? read_u16(extra_strips->data + i_rel + (k - 1u) * 2u) : 0u;
													if (safe_val >= (uint32_t)vtx_cnt) safe_val = 0u;
													write_u16(extra_strips->data + i_rel + k * 2u, safe_val);
												}
											}
											write_u16(extra_strips->data + i_rel + ic * 2u, 0xFFFFu);
											write_u16(extra_strips->data + i_rel + (ic + 1u) * 2u, 0xFFFFu);
											ogp->tag_idx = (uint32_t)tidx;
											ogp->geom_idx = (uint32_t)g_idx;
											ogp->part_idx = (uint32_t)p_idx;
											ogp->vtx_count = (uint32_t)vtx_cnt;
											ogp->idx_count = ic;
											ogp->extra_vtx_rel_off = v_rel;
											ogp->extra_idx_rel_off = i_rel;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		/* Walk schema references in this tag to discover all dependent tags */
		sgrp = forge_schema_group_get(fourcc);
		if (sgrp && sgrp->definition)
		{
			struct
			{
				struct tag_schema_definition const *def;
				uint32_t foff;
				int32_t count;
			} stack[64];
			int sp = 0;
			stack[sp].def = sgrp->definition;
			stack[sp].foff = xmap->tag_data_offset + (daddr - xmap->tag_base_addr);
			stack[sp].count = 1;
			sp++;

			while (sp > 0)
			{
				struct tag_schema_definition const *def = stack[sp - 1].def;
				uint32_t base_foff = stack[sp - 1].foff;
				int32_t count = stack[sp - 1].count;
				uint32_t elem_sz = (def && def->size > 0) ? (uint32_t)def->size : 0u;
				int32_t e;
				sp--;
				if (!def || !def->fields || elem_sz == 0) continue;

				for (e = 0; e < count; e++)
				{
					uint32_t e_foff = base_foff + (uint32_t)e * elem_sz;
					struct tag_schema_field const *f;
					if (e_foff + elem_sz > xmap->file_size) break;
					for (f = def->fields; f->type != _tag_schema_terminator; f++)
					{
						uint32_t f_off = e_foff + (uint32_t)f->offset;
						struct tag_schema_definition const *fdef = (struct tag_schema_definition const *)f->definition;
						if (f->type == _tag_schema_struct && fdef)
						{
							if (sp < 64)
							{
								stack[sp].def = fdef;
								stack[sp].foff = f_off;
								stack[sp].count = 1;
								sp++;
							}
						}
						else if (f->type == _tag_schema_block && fdef && fdef->size > 0)
						{
							if (f_off + 12u <= xmap->file_size)
							{
								int32_t cnt = read_s32(xmap->data + f_off + 0u);
								uint32_t addr = read_u32(xmap->data + f_off + 4u);
								uint32_t c_sz = (uint32_t)fdef->size;
								if (cnt > 0 && cnt <= 32768 && addr >= xmap->tag_base_addr &&
									(uint64_t)(addr - xmap->tag_base_addr) + (uint64_t)cnt * c_sz <= xmap->tag_data_size &&
									sp < 64)
								{
									stack[sp].def = fdef;
									stack[sp].foff = xmap->tag_data_offset + (addr - xmap->tag_base_addr);
									stack[sp].count = cnt;
									sp++;
								}
							}
						}
						else if (f->type == _tag_schema_reference && f_off + 16u <= xmap->file_size)
						{
							uint32_t ref_h = read_u32(xmap->data + f_off + 12u);
							int32_t ref_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
							if (ref_idx >= 0 && ref_idx < xmap->tag_count && ref_idx < 8192 &&
								!visited[ref_idx] && q_tail < 2048)
							{
								uint8_t const *rinst = src_tag_instance(xmap, ref_idx);
								uint32_t rgrp = rinst ? read_u32(rinst) : 0u;
								if (rgrp != 'udlg' && rgrp != 'sbsp' && rgrp != 'scnr')
								{
									visited[ref_idx] = 1;
									queue[q_tail++] = ref_idx;
								}
							}
						}
						else if (f->type == _tag_schema_tag_index && f_off + 4u <= xmap->file_size)
						{
							uint32_t ref_h = read_u32(xmap->data + f_off);
							int32_t ref_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
							if (ref_idx >= 0 && ref_idx < xmap->tag_count && ref_idx < 8192 &&
								!visited[ref_idx] && q_tail < 2048)
							{
								uint8_t const *rinst = src_tag_instance(xmap, ref_idx);
								uint32_t rgrp = rinst ? read_u32(rinst) : 0u;
								if (rgrp != 'udlg' && rgrp != 'sbsp' && rgrp != 'scnr')
								{
									visited[ref_idx] = 1;
									queue[q_tail++] = ref_idx;
								}
							}
						}
					}
				}
			}
		}
	}
}

static void cb_xbox_rebase_and_link_schema(
	struct forge_byte_buf *out,
	struct forge_source_map const *xmap,
	uint32_t tag_data_offset,
	struct tag_schema_definition const *def,
	uint32_t elem_base_foff,
	int32_t count,
	struct cb_xbox_bake_tag const *xtags,
	int xtag_count,
	int depth)
{
	int32_t i;
	uint32_t elem_sz;

	if (!out || !out->data || !xmap || !def || !def->fields || count <= 0 || depth > 28)
		return;
	elem_sz = (uint32_t)def->size;
	if (elem_sz == 0)
		return;

	for (i = 0; i < count; i++)
	{
		uint32_t e_foff = elem_base_foff + (uint32_t)i * elem_sz;
		struct tag_schema_field const *f;
		if (e_foff + elem_sz > out->size)
			break;

		for (f = def->fields; f->type != _tag_schema_terminator; f++)
		{
			uint32_t f_off = e_foff + (uint32_t)f->offset;
			struct tag_schema_definition const *fdef = (struct tag_schema_definition const *)f->definition;

			if (f->type == _tag_schema_struct && fdef)
			{
				cb_xbox_rebase_and_link_schema(
					out, xmap, tag_data_offset, fdef, f_off, 1,
					xtags, xtag_count, depth + 1);
			}
			else if (f->type == _tag_schema_block && fdef)
			{
				int32_t cnt;
				uint32_t addr;
				uint32_t child_elem_sz = (uint32_t)fdef->size;
				if (f_off + 12u > out->size)
					continue;
				cnt = read_s32(out->data + f_off + 0u);
				addr = read_u32(out->data + f_off + 4u);
				if (cnt <= 0 || child_elem_sz == 0)
				{
					write_s32(out->data + f_off + 0u, 0);
					write_u32(out->data + f_off + 4u, 0u);
					write_u32(out->data + f_off + 8u, 0u);
					continue;
				}
				if (f->maximum > 0 && cnt > f->maximum)
					cnt = f->maximum;

				if (addr >= xmap->tag_base_addr &&
					(uint64_t)(addr - xmap->tag_base_addr) + (uint64_t)cnt * child_elem_sz <= xmap->tag_data_size)
				{
					uint32_t total_bytes = (uint32_t)cnt * child_elem_sz;
					uint32_t child_foff = buf_alloc(out, total_bytes, 4);
					if (child_foff && out->data)
					{
						memcpy(out->data + child_foff,
							xmap->data + xmap->tag_data_offset + (addr - xmap->tag_base_addr),
							total_bytes);
						write_s32(out->data + f_off + 0u, cnt);
						write_u32(out->data + f_off + 4u, CE_TAG_BASE_ADDR + (child_foff - tag_data_offset));
						write_u32(out->data + f_off + 8u, 0u);
						cb_xbox_rebase_and_link_schema(
							out, xmap, tag_data_offset, fdef, child_foff, cnt,
							xtags, xtag_count, depth + 1);
					}
					else
					{
						write_s32(out->data + f_off + 0u, 0);
						write_u32(out->data + f_off + 4u, 0u);
						write_u32(out->data + f_off + 8u, 0u);
					}
				}
				else
				{
					write_s32(out->data + f_off + 0u, 0);
					write_u32(out->data + f_off + 4u, 0u);
					write_u32(out->data + f_off + 8u, 0u);
				}
			}
			else if (f->type == _tag_schema_data)
			{
				int32_t sz;
				uint32_t addr;
				if (f_off + 20u > out->size)
					continue;
				sz = read_s32(out->data + f_off + 0u);
				addr = read_u32(out->data + f_off + 12u);
				if (sz <= 0 || (f->maximum > 0 && sz > f->maximum))
				{
					memset(out->data + f_off, 0, 20u);
					continue;
				}
				if (addr >= xmap->tag_base_addr &&
					(uint64_t)(addr - xmap->tag_base_addr) + (uint64_t)sz <= xmap->tag_data_size)
				{
					uint32_t data_foff = buf_alloc(out, (uint32_t)sz, 4);
					if (data_foff && out->data)
					{
						memcpy(out->data + data_foff,
							xmap->data + xmap->tag_data_offset + (addr - xmap->tag_base_addr),
							(size_t)sz);
						write_u32(out->data + f_off + 4u, 0u);
						write_u32(out->data + f_off + 8u, 0u);
						write_u32(out->data + f_off + 12u, CE_TAG_BASE_ADDR + (data_foff - tag_data_offset));
						write_u32(out->data + f_off + 16u, 0u);
					}
					else
					{
						memset(out->data + f_off, 0, 20u);
					}
				}
				else
				{
					memset(out->data + f_off, 0, 20u);
				}
			}
			else if (f->type == _tag_schema_reference)
			{
				uint32_t ref_h;
				int32_t ref_src_idx;
				int k, linked = 0;
				if (f_off + 16u > out->size)
					continue;
				ref_h = read_u32(out->data + f_off + 12u);
				ref_src_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
				if (ref_src_idx >= 0)
				{
					for (k = 0; k < xtag_count; k++)
					{
						if (xtags[k].src_tag_idx == ref_src_idx && xtags[k].baked_tag_idx >= 0)
						{
							uint32_t dst_grp = (xtags[k].fourcc == 'mode') ? 'mod2' : xtags[k].fourcc;
							write_u32(out->data + f_off + 0u, dst_grp);
							write_u32(out->data + f_off + 4u, xtags[k].baked_name_addr);
							write_u32(out->data + f_off + 8u, 0u);
							write_u32(out->data + f_off + 12u, xtags[k].baked_handle);
							linked = 1;
							break;
						}
					}
				}
				if (!linked)
				{
					if (read_u32(out->data + f_off + 0u) == 0u)
						write_u32(out->data + f_off + 0u, 0xFFFFFFFFu);
					write_u32(out->data + f_off + 4u, 0u);
					write_u32(out->data + f_off + 8u, 0u);
					write_u32(out->data + f_off + 12u, 0xFFFFFFFFu);
				}
			}
			else if (f->type == _tag_schema_tag_index)
			{
				uint32_t ref_h;
				int32_t ref_src_idx;
				int k, linked = 0;
				if (f_off + 4u > out->size)
					continue;
				ref_h = read_u32(out->data + f_off);
				ref_src_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
				if (ref_src_idx >= 0)
				{
					for (k = 0; k < xtag_count; k++)
					{
						if (xtags[k].src_tag_idx == ref_src_idx && xtags[k].baked_tag_idx >= 0)
						{
							write_u32(out->data + f_off, xtags[k].baked_handle);
							linked = 1;
							break;
						}
					}
				}
				if (!linked)
					write_u32(out->data + f_off, 0xFFFFFFFFu);
			}
		}
	}
}

static void cb_compute_swizzle_masks(
	int16_t width,
	int16_t height,
	int16_t depth,
	uint32_t *out_x_mask,
	uint32_t *out_y_mask,
	uint32_t *out_z_mask)
{
	uint32_t x_mask = 0, y_mask = 0, z_mask = 0;
	uint32_t bit = 1, swiz_bit = 1;
	int i;

	for (i = 0; i < 16; i++)
	{
		if (bit < (uint32_t)width)  { x_mask |= swiz_bit; swiz_bit <<= 1; }
		if (bit < (uint32_t)height) { y_mask |= swiz_bit; swiz_bit <<= 1; }
		if (bit < (uint32_t)depth)  { z_mask |= swiz_bit; swiz_bit <<= 1; }
		bit <<= 1;
	}
	*out_x_mask = x_mask;
	*out_y_mask = y_mask;
	*out_z_mask = z_mask;
}

static uint8_t *cb_unpack_xbox_bitmap_to_ce(
	uint8_t const *xbox_pixels,
	uint32_t xbox_pixel_bytes,
	int16_t width,
	int16_t height,
	int16_t depth,
	int16_t type,
	int16_t format,
	uint16_t flags,
	int16_t mipmap_count,
	uint32_t *out_ce_size)
{
	uint32_t ce_total_bytes = 0;
	uint8_t *ce_buf;
	uint8_t *unpacked_buf = NULL;
	uint8_t const *src_pixels = xbox_pixels;
	boolean is_compressed;
	boolean is_swizzled;
	uint32_t bits_per_pix;
	int16_t mips;
	int m_idx;

	if (out_ce_size) *out_ce_size = 0;
	if (!xbox_pixels || xbox_pixel_bytes == 0 || width <= 0 || height <= 0 || depth <= 0)
		return NULL;
	if ((uint16_t)type >= 4u || (uint16_t)format >= 18u)
		return NULL;

	/* Formats: 0=a8, 1=y8, 2=ay8, 3=a8y8, 6=r5g6b5, 8=a1r5g5b5, 9=a4r4g4b4,
	 * 10=x8r8g8b8, 11=a8r8g8b8, 14=dxt1, 15=dxt3, 16=dxt5, 17=p8-bump */
	is_compressed = (format >= 14 && format <= 16) ? TRUE : FALSE;
	is_swizzled = ((flags & 0x0008u) != 0 && !is_compressed) ? TRUE : FALSE;
	bits_per_pix = (format == 14) ? 4u :
		((format == 15 || format == 16) ? 8u :
		((format == 0 || format == 1 || format == 2 || format == 17) ? 8u :
		((format == 10 || format == 11) ? 32u : 16u)));
	mips = (mipmap_count >= 0) ? mipmap_count : 0;

	for (m_idx = 0; m_idx <= (int)mips; m_idx++)
	{
		uint32_t mw = (uint32_t)width >> m_idx;
		uint32_t mh = (uint32_t)height >> m_idx;
		uint32_t md = (uint32_t)depth >> m_idx;
		uint32_t mpix;
		if (mw == 0) mw = 1;
		if (mh == 0) mh = 1;
		if (md == 0) md = 1;
		if (is_compressed)
		{
			mw = (mw + 3u) & ~3u;
			mh = (mh + 3u) & ~3u;
		}
		mpix = mw * mh * md;
		if (type == 2) /* _bitmap_type_cube_map */
			mpix *= 6u;
		ce_total_bytes += (mpix * bits_per_pix) >> 3;
	}
	if (ce_total_bytes == 0 || ce_total_bytes > 16u * 1024u * 1024u)
		return NULL;

	ce_buf = (uint8_t *)calloc(1, ce_total_bytes);
	if (!ce_buf)
		return NULL;

	if (type == 2) /* _bitmap_type_cube_map */
	{
		static int const face_inv[6] = { 0, 2, 1, 3, 4, 5 };
		uint32_t src_off = 0;
		int face, mip;
		int mip_total = (int)mips + 1;

		unpacked_buf = (uint8_t *)calloc(1, ce_total_bytes);
		if (!unpacked_buf)
		{
			free(ce_buf);
			return NULL;
		}
		for (face = 0; face < 6; face++)
		{
			uint32_t dst_off = 0;
			int ce_face = face_inv[face];
			for (mip = 0; mip < mip_total; mip++)
			{
				uint32_t mw = (uint32_t)width >> mip;
				uint32_t mh = (uint32_t)height >> mip;
				uint32_t face_bytes;
				if (mw == 0) mw = 1;
				if (mh == 0) mh = 1;
				if (is_compressed)
				{
					uint32_t bw = (mw + 3u) & ~3u;
					uint32_t bh = (mh + 3u) & ~3u;
					face_bytes = (bw * bh * bits_per_pix) >> 3;
				}
				else
				{
					face_bytes = (mw * mh * bits_per_pix) >> 3;
				}
				if (src_off + face_bytes <= xbox_pixel_bytes &&
					dst_off + (uint32_t)(ce_face + 1) * face_bytes <= ce_total_bytes)
				{
					memcpy(unpacked_buf + dst_off + (uint32_t)ce_face * face_bytes,
						xbox_pixels + src_off, face_bytes);
				}
				src_off += face_bytes;
				dst_off += face_bytes * 6u;
			}
			src_off = (src_off + 127u) & ~127u;
		}
		src_pixels = unpacked_buf;
	}
	else if (!is_compressed && !is_swizzled && type == 0) /* _bitmap_type_2d_textures */
	{
		uint32_t row_bytes = ((uint32_t)width * bits_per_pix) >> 3;
		uint32_t pitch = (row_bytes + 63u) & ~63u;
		if (pitch != row_bytes && pitch * (uint32_t)height <= xbox_pixel_bytes)
		{
			int y;
			for (y = 0; y < (int)height; y++)
			{
				if ((uint32_t)(y + 1) * row_bytes <= ce_total_bytes)
					memcpy(ce_buf + (uint32_t)y * row_bytes, xbox_pixels + (uint32_t)y * pitch, row_bytes);
			}
			if (out_ce_size) *out_ce_size = ce_total_bytes;
			return ce_buf;
		}
	}

	if (is_swizzled)
	{
		uint32_t bytes_per_pix = bits_per_pix >> 3;
		int mip, mip_total = (int)mips + 1;
		int faces = (type == 2) ? 6 : 1;
		uint32_t mip_off = 0;

		for (mip = 0; mip < mip_total; mip++)
		{
			int16_t mw = (int16_t)(width >> mip);
			int16_t mh = (int16_t)(height >> mip);
			int16_t md = (int16_t)(depth >> mip);
			uint32_t face_bytes;
			int f_idx;
			if (mw <= 0) mw = 1;
			if (mh <= 0) mh = 1;
			if (md <= 0) md = 1;
			face_bytes = (uint32_t)mw * (uint32_t)mh * (uint32_t)md * bytes_per_pix;

			for (f_idx = 0; f_idx < faces; f_idx++)
			{
				uint32_t sub_off = mip_off + (uint32_t)f_idx * face_bytes;
				uint32_t src_limit = (src_pixels == unpacked_buf) ? ce_total_bytes : xbox_pixel_bytes;
				if (sub_off + face_bytes <= ce_total_bytes && sub_off + face_bytes <= src_limit)
				{
					uint32_t x_mask = 0, y_mask = 0, z_mask = 0;
					int x, y, z;
					uint32_t z_swiz, y_swiz, x_swiz, src_idx = 0;
					uint8_t const *s_base = src_pixels + sub_off;
					uint8_t *d_base = ce_buf + sub_off;

					cb_compute_swizzle_masks(mw, mh, md, &x_mask, &y_mask, &z_mask);
					for (z = 0, z_swiz = 0; z < (int)md; z++, z_swiz = (z_swiz - z_mask) & z_mask)
					{
						for (y = 0, y_swiz = 0; y < (int)mh; y++, y_swiz = (y_swiz - y_mask) & y_mask)
						{
							for (x = 0, x_swiz = 0; x < (int)mw; x++, x_swiz = (x_swiz - x_mask) & x_mask)
							{
								uint32_t sw_i = z_swiz | y_swiz | x_swiz;
								if (bytes_per_pix == 1u)
									d_base[src_idx] = s_base[sw_i];
								else if (bytes_per_pix == 2u)
									((uint16_t *)d_base)[src_idx] = ((uint16_t const *)s_base)[sw_i];
								else if (bytes_per_pix == 4u)
									((uint32_t *)d_base)[src_idx] = ((uint32_t const *)s_base)[sw_i];
								src_idx++;
							}
						}
					}
				}
			}
			mip_off += face_bytes * (uint32_t)faces;
		}
	}
	else
	{
		uint32_t src_limit = (src_pixels == unpacked_buf) ? ce_total_bytes : xbox_pixel_bytes;
		uint32_t copy_n = (src_limit < ce_total_bytes) ? src_limit : ce_total_bytes;
		memcpy(ce_buf, src_pixels, copy_n);
	}

	free(unpacked_buf);
	if (out_ce_size) *out_ce_size = ce_total_bytes;
	return ce_buf;
}

static void cb_tint_dxt1_blue_to_red(uint8_t *pix, uint32_t byte_count)
{
	uint32_t off;
	if (!pix)
		return;
	for (off = 0; off + 8u <= byte_count; off += 8u)
	{
		uint16_t c0 = read_u16(pix + off + 0u);
		uint16_t c1 = read_u16(pix + off + 2u);
		uint32_t lut = read_u32(pix + off + 4u);
		uint16_t r0 = (uint16_t)((c0 >> 11) & 0x1Fu);
		uint16_t g0 = (uint16_t)((c0 >> 5) & 0x3Fu);
		uint16_t b0 = (uint16_t)(c0 & 0x1Fu);
		uint16_t r1 = (uint16_t)((c1 >> 11) & 0x1Fu);
		uint16_t g1 = (uint16_t)((c1 >> 5) & 0x3Fu);
		uint16_t b1 = (uint16_t)(c1 & 0x1Fu);
		uint16_t nr0 = (b0 > r0) ? b0 : r0;
		uint16_t nb0 = (r0 < b0) ? (uint16_t)(r0 >> 1) : (uint16_t)(b0 >> 1);
		uint16_t ng0 = (uint16_t)(g0 >> 2);
		uint16_t nr1 = (b1 > r1) ? b1 : r1;
		uint16_t nb1 = (r1 < b1) ? (uint16_t)(r1 >> 1) : (uint16_t)(b1 >> 1);
		uint16_t ng1 = (uint16_t)(g1 >> 2);
		uint16_t nc0 = (uint16_t)((nr0 << 11) | (ng0 << 5) | nb0);
		uint16_t nc1 = (uint16_t)((nr1 << 11) | (ng1 << 5) | nb1);

		if (c0 > c1)
		{
			if (nc0 < nc1)
			{
				uint16_t tmp = nc0;
				nc0 = nc1;
				nc1 = tmp;
				lut ^= 0x55555555u;
			}
			else if (nc0 == nc1)
			{
				if (nc0 < 0xFFFFu) nc0++;
				else nc1--;
			}
		}
		else
		{
			if (nc0 > nc1)
			{
				uint16_t tmp = nc0;
				uint32_t new_lut = 0;
				int p;
				nc0 = nc1;
				nc1 = tmp;
				for (p = 0; p < 16; p++)
				{
					uint32_t idx = (lut >> (p * 2)) & 3u;
					if (idx == 0u) idx = 1u;
					else if (idx == 1u) idx = 0u;
					new_lut |= (idx << (p * 2));
				}
				lut = new_lut;
			}
		}
		write_u16(pix + off + 0u, nc0);
		write_u16(pix + off + 2u, nc1);
		write_u32(pix + off + 4u, lut);
	}
}

static int32_t out_map_bake_xbox_tag_closure(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	uint32_t orig_vtx_size,
	uint32_t orig_idx_size,
	struct forge_pending_bake_obj const *pobj)
{
	struct forge_source_map xmap;
	int32_t root_src_idx;
	struct cb_xbox_bake_tag *xtags = NULL;
	int xtag_count = 0;
	uint8_t visited[8192];
	int32_t queue[2048];
	int q_head = 0, q_tail = 0;
	int i;
	int32_t root_baked_idx = -1;
	int is_red_beam_variant = 0;

	if (!out || !out->data || !pobj || !pobj->xbox_map_path[0])
		return -1;
	is_red_beam_variant = (pobj->group_tag == 'mach' &&
		_stricmp(pobj->tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0) ? 1 : 0;
	if (!forge_source_map_open(pobj->xbox_map_path, &xmap) || xmap.version != CACHE_VERSION_XBOX)
	{
		if (xmap.data) forge_source_map_close(&xmap);
		return -1;
	}

	root_src_idx = src_map_find_tag_or_variant(&xmap, pobj->group_tag, pobj->tag_path);
	if (root_src_idx < 0 || root_src_idx >= 8192)
	{
		forge_source_map_close(&xmap);
		return -1;
	}

	xtags = (struct cb_xbox_bake_tag *)calloc(2048, sizeof(struct cb_xbox_bake_tag));
	if (!xtags)
	{
		forge_source_map_close(&xmap);
		return -1;
	}

	/* 1. Discover transitive closure of tags starting at root_src_idx */
	memset(visited, 0, sizeof(visited));
	visited[root_src_idx] = 1;
	queue[q_tail++] = root_src_idx;

	while (q_head < q_tail && xtag_count < 2048)
	{
		int32_t tidx = queue[q_head++];
		uint8_t const *inst = src_tag_instance(&xmap, tidx);
		uint32_t fourcc, daddr;
		char const *nm;
		struct cb_xbox_bake_tag *xt;
		struct tag_schema_group const *sgrp;

		if (!inst) continue;
		fourcc = read_u32(inst + 0x00u);
		daddr = read_u32(inst + 0x14u);
		if (daddr < xmap.tag_base_addr || daddr >= xmap.tag_base_addr + xmap.tag_data_size)
			continue;
		nm = src_tag_name(&xmap, tidx);

		xt = &xtags[xtag_count++];
		memset(xt, 0, sizeof(*xt));
		xt->src_tag_idx = tidx;
		xt->fourcc = fourcc;
		snprintf(xt->name, sizeof(xt->name), "%s", nm ? nm : "");
		xt->src_base_addr = daddr;
		xt->baked_tag_idx = -1;
		xt->baked_handle = 0xFFFFFFFFu;

		sgrp = forge_schema_group_get(fourcc);
		if (sgrp && sgrp->definition)
		{
			struct
			{
				struct tag_schema_definition const *def;
				uint32_t foff;
				int32_t count;
			} stack[64];
			int sp = 0;
			stack[sp].def = sgrp->definition;
			stack[sp].foff = xmap.tag_data_offset + (daddr - xmap.tag_base_addr);
			stack[sp].count = 1;
			sp++;

			while (sp > 0)
			{
				struct tag_schema_definition const *def = stack[sp - 1].def;
				uint32_t base_foff = stack[sp - 1].foff;
				int32_t count = stack[sp - 1].count;
				uint32_t elem_sz = (def && def->size > 0) ? (uint32_t)def->size : 0u;
				int32_t e;
				sp--;
				if (!def || !def->fields || elem_sz == 0) continue;

				for (e = 0; e < count; e++)
				{
					uint32_t e_foff = base_foff + (uint32_t)e * elem_sz;
					struct tag_schema_field const *f;
					if (e_foff + elem_sz > xmap.file_size) break;
					for (f = def->fields; f->type != _tag_schema_terminator; f++)
					{
						uint32_t f_off = e_foff + (uint32_t)f->offset;
						struct tag_schema_definition const *fdef = (struct tag_schema_definition const *)f->definition;
						if (f->type == _tag_schema_struct && fdef)
						{
							if (sp < 64)
							{
								stack[sp].def = fdef;
								stack[sp].foff = f_off;
								stack[sp].count = 1;
								sp++;
							}
						}
						else if (f->type == _tag_schema_block && fdef && fdef->size > 0)
						{
							if (f_off + 12u <= xmap.file_size)
							{
								int32_t cnt = read_s32(xmap.data + f_off + 0u);
								uint32_t addr = read_u32(xmap.data + f_off + 4u);
								uint32_t c_sz = (uint32_t)fdef->size;
								if (cnt > 0 && cnt <= 32768 && addr >= xmap.tag_base_addr &&
									(uint64_t)(addr - xmap.tag_base_addr) + (uint64_t)cnt * c_sz <= xmap.tag_data_size &&
									sp < 64)
								{
									stack[sp].def = fdef;
									stack[sp].foff = xmap.tag_data_offset + (addr - xmap.tag_base_addr);
									stack[sp].count = cnt;
									sp++;
								}
							}
						}
						else if (f->type == _tag_schema_reference && f_off + 16u <= xmap.file_size)
						{
							uint32_t ref_h = read_u32(xmap.data + f_off + 12u);
							int32_t ref_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
							if (ref_idx >= 0 && ref_idx < xmap.tag_count && ref_idx < 8192 &&
								!visited[ref_idx] && q_tail < 2048)
							{
								uint8_t const *rinst = src_tag_instance(&xmap, ref_idx);
								uint32_t rgrp = rinst ? read_u32(rinst) : 0u;
								if (rgrp != 'udlg' && rgrp != 'sbsp' && rgrp != 'scnr')
								{
									visited[ref_idx] = 1;
									queue[q_tail++] = ref_idx;
								}
							}
						}
						else if (f->type == _tag_schema_tag_index && f_off + 4u <= xmap.file_size)
						{
							uint32_t ref_h = read_u32(xmap.data + f_off);
							int32_t ref_idx = (ref_h != 0xFFFFFFFFu) ? (int32_t)(ref_h & 0xFFFFu) : -1;
							if (ref_idx >= 0 && ref_idx < xmap.tag_count && ref_idx < 8192 &&
								!visited[ref_idx] && q_tail < 2048)
							{
								uint8_t const *rinst = src_tag_instance(&xmap, ref_idx);
								uint32_t rgrp = rinst ? read_u32(rinst) : 0u;
								if (rgrp != 'udlg' && rgrp != 'sbsp' && rgrp != 'scnr')
								{
									visited[ref_idx] = 1;
									queue[q_tail++] = ref_idx;
								}
							}
						}
					}
				}
			}
		}
	}

	/* 2. Allocate root tag definitions (or reuse existing tags in out) */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		uint32_t dst_grp = (xt->fourcc == 'mode') ? 'mod2' : xt->fourcc;
		uint32_t p1 = 0xFFFFFFFFu, p2 = 0xFFFFFFFFu;
		int32_t existing;
		struct tag_schema_group const *sgrp;
		uint32_t root_sz;
		char alloc_name[256];

		if (i == 0 && is_red_beam_variant)
			snprintf(xt->name, sizeof(xt->name), "%s", pobj->tag_path);

		existing = (i > 0 && dst_grp != 'bitm' && dst_grp != 'snd!')
			? out_map_find_tag(out, tag_data_offset, dst_grp, xt->name) : -1;
		if (is_red_beam_variant &&
			(dst_grp == 'mod2' || dst_grp == 'sotr' || dst_grp == 'ligh' || dst_grp == 'lens' || dst_grp == 'bitm'))
		{
			existing = -1;
		}

		if (existing >= 0)
		{
			uint32_t eio = tag_data_offset + 0x28u + (uint32_t)existing * 32u;
			xt->is_reused_existing = 1;
			xt->baked_tag_idx = existing;
			xt->baked_handle = read_u32(out->data + eio + 0x0Cu);
			xt->baked_name_addr = read_u32(out->data + eio + 0x10u);
			continue;
		}

		sgrp = forge_schema_group_get(dst_grp);
		if (!sgrp || !sgrp->definition || sgrp->definition->size <= 0)
			continue;
		root_sz = (uint32_t)sgrp->definition->size;
		if ((uint64_t)(xt->src_base_addr - xmap.tag_base_addr) + root_sz > xmap.tag_data_size)
			continue;

		xt->def_foff = buf_alloc(out, root_sz, 4);
		if (!xt->def_foff || !out->data)
			continue;
		memcpy(out->data + xt->def_foff,
			xmap.data + xmap.tag_data_offset + (xt->src_base_addr - xmap.tag_base_addr),
			root_sz);

		if (dst_grp == 'vehi' || dst_grp == 'bipd') { p1 = 'unit'; p2 = 'obje'; }
		else if (dst_grp == 'weap' || dst_grp == 'eqip' || dst_grp == 'garb') { p1 = 'item'; p2 = 'obje'; }
		else if (dst_grp == 'scen' || dst_grp == 'proj') { p1 = 'obje'; p2 = 0xFFFFFFFFu; }
		else if (dst_grp == 'mach' || dst_grp == 'ctrl' || dst_grp == 'lifi') { p1 = 'devi'; p2 = 'obje'; }
		else if (dst_grp == 'soso' || dst_grp == 'senv' || dst_grp == 'sgla' || dst_grp == 'schi' ||
				 dst_grp == 'scex' || dst_grp == 'swat' || dst_grp == 'smet' || dst_grp == 'spla' || dst_grp == 'strn' || dst_grp == 'sotr')
		{
			p1 = 'shdr'; p2 = 0xFFFFFFFFu;
		}

		if (is_red_beam_variant && i > 0 &&
			(dst_grp == 'mod2' || dst_grp == 'sotr' || dst_grp == 'ligh' || dst_grp == 'lens' || dst_grp == 'bitm'))
		{
			snprintf(alloc_name, sizeof(alloc_name), "%s__red", xt->name);
		}
		else if (dst_grp == 'bitm' || dst_grp == 'snd!')
			snprintf(alloc_name, sizeof(alloc_name), "%s__xbox", xt->name);
		else
			snprintf(alloc_name, sizeof(alloc_name), "%s", xt->name);

		xt->baked_tag_idx = out_map_alloc_tag_instance(
			out, tag_data_offset, scnr_off,
			dst_grp, p1, p2,
			alloc_name, xt->def_foff, 0u,
			&xt->baked_handle, &xt->baked_name_addr);
	}

	/* 3. Rebase & clone all nested tag_blocks and tag_data, and link tag_references */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		struct tag_schema_group const *sgrp;
		if (!xt->def_foff || xt->is_reused_existing || xt->baked_tag_idx < 0)
			continue;
		/* Note: for 'mode', walk the Xbox 'mode' schema first (where parts are 104 bytes),
		 * then step 4 converts each 104-byte Xbox part block into a 132-byte CE 'mod2' part block! */
		sgrp = forge_schema_group_get(xt->fourcc);
		if (!sgrp || !sgrp->definition || sgrp->definition->size <= 0)
			continue;
		cb_xbox_rebase_and_link_schema(
			out, &xmap, tag_data_offset, sgrp->definition,
			xt->def_foff, 1, xtags, xtag_count, 0);
	}

	/* 4. Convert Xbox 'mode' (104-byte parts) to Custom Edition 'mod2' (132-byte parts) */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		int32_t geom_cnt, shdr_cnt, node_cnt;
		uint32_t geom_addr;
		int g_idx;

		if (!xt->def_foff || xt->is_reused_existing || xt->baked_tag_idx < 0 || xt->fourcc != 'mode')
			continue;

		/* Clear local_nodes_bit (bit 2) and ensure non-zero UV scales */
		write_u32(out->data + xt->def_foff + 0x00u, read_u32(out->data + xt->def_foff + 0x00u) & ~0x04u);
		if (!(read_f32(out->data + xt->def_foff + 0x30u) > 0.0001f))
			write_f32(out->data + xt->def_foff + 0x30u, 1.0f);
		if (!(read_f32(out->data + xt->def_foff + 0x34u) > 0.0001f))
			write_f32(out->data + xt->def_foff + 0x34u, 1.0f);

		node_cnt = read_s32(out->data + xt->def_foff + 0xB8u);
		geom_cnt = read_s32(out->data + xt->def_foff + 0xD0u);
		geom_addr = read_u32(out->data + xt->def_foff + 0xD4u);
		shdr_cnt = read_s32(out->data + xt->def_foff + 0xDCu);

		/* Shift beam emitter root node down by 6.5 world units (~10m in Forge HUD) so its flare collar sits at the placement origin */
		if (is_red_beam_variant || strstr(xt->name, "beam emitter") != NULL)
		{
			uint32_t node_addr = read_u32(out->data + xt->def_foff + 0xBCu);
			if (node_cnt > 0 && node_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t node0_foff = tag_data_offset + (node_addr - CE_TAG_BASE_ADDR);
				if (node0_foff + 156u <= out->size)
					write_f32(out->data + node0_foff + 48u, -6.5f);
			}
		}

		if (geom_cnt > 0 && geom_addr >= CE_TAG_BASE_ADDR)
		{
			uint32_t geom_base_foff = tag_data_offset + (geom_addr - CE_TAG_BASE_ADDR);
			for (g_idx = 0; g_idx < geom_cnt; g_idx++)
			{
				uint32_t g_foff = geom_base_foff + (uint32_t)g_idx * 48u;
				int32_t part_cnt = read_s32(out->data + g_foff + 36u);
				uint32_t old_part_addr = read_u32(out->data + g_foff + 40u);
				uint32_t new_parts_foff;
				int p_idx;

				if (part_cnt <= 0 || old_part_addr < CE_TAG_BASE_ADDR)
				{
					write_s32(out->data + g_foff + 36u, 0);
					write_u32(out->data + g_foff + 40u, 0u);
					continue;
				}
				new_parts_foff = buf_alloc(out, (uint32_t)part_cnt * 132u, 4);
				if (!new_parts_foff || !out->data)
					continue;
				memset(out->data + new_parts_foff, 0, (size_t)part_cnt * 132u);
				geom_base_foff = tag_data_offset + (geom_addr - CE_TAG_BASE_ADDR);
				g_foff = geom_base_foff + (uint32_t)g_idx * 48u;

				for (p_idx = 0; p_idx < part_cnt; p_idx++)
				{
					uint32_t old_p = tag_data_offset + (old_part_addr - CE_TAG_BASE_ADDR) + (uint32_t)p_idx * 104u;
					uint32_t new_p = new_parts_foff + (uint32_t)p_idx * 132u;
					int16_t s_idx = read_s16(out->data + old_p + 0x04u);
					int16_t c_pri = read_s16(out->data + old_p + 0x08u);
					int16_t c_sec = read_s16(out->data + old_p + 0x0Au);
					int gp, matched = 0;

					/* Copy first 0x48 bytes (flags, shader_index, prev/next, centroid, bounds) */
					memcpy(out->data + new_p, out->data + old_p, 0x48u);
					write_u32(out->data + new_p + 0x00u, read_u32(out->data + new_p + 0x00u) & ~0x04u);
					if (s_idx < 0 || s_idx >= shdr_cnt)
						write_s16(out->data + new_p + 0x04u, (shdr_cnt > 0) ? 0 : -1);
					out->data[new_p + 0x06u] = 0xFFu;
					out->data[new_p + 0x07u] = 0xFFu;
					if (c_pri < 0 || c_pri >= node_cnt)
						write_s16(out->data + new_p + 0x08u, 0);
					if (c_sec < 0 || c_sec >= node_cnt)
						write_s16(out->data + new_p + 0x0Au, 0);

					for (gp = 0; gp < pobj->extra_gpart_count; gp++)
					{
						struct forge_pending_extra_gpart const *ogp = &pobj->extra_gparts[gp];
						if (ogp->tag_idx == (uint32_t)xt->src_tag_idx &&
							ogp->geom_idx == (uint32_t)g_idx &&
							ogp->part_idx == (uint32_t)p_idx)
						{
							int32_t tri_cnt = (ogp->idx_count >= 2u) ? (int32_t)(ogp->idx_count - 2u) : 1;
							write_s16(out->data + new_p + 0x44u, 1); /* TRIANGLE_BUFFER_TYPE_PRECOMPILED_STRIP */
							write_s32(out->data + new_p + 0x48u, tri_cnt);
							write_u32(out->data + new_p + 0x4Cu, orig_idx_size + ogp->extra_idx_rel_off);
							write_u32(out->data + new_p + 0x50u, orig_idx_size + ogp->extra_idx_rel_off);
							write_s16(out->data + new_p + 0x54u, 4); /* VERTEX_TYPE_MODEL_UNCOMPRESSED */
							write_s32(out->data + new_p + 0x58u, (int32_t)ogp->vtx_count);
							write_u32(out->data + new_p + 0x60u, orig_vtx_size + ogp->extra_vtx_rel_off);
							write_u32(out->data + new_p + 0x64u, orig_vtx_size + ogp->extra_vtx_rel_off);
							matched = 1;
							break;
						}
					}
					if (!matched)
					{
						write_s16(out->data + new_p + 0x44u, 1);
						write_s32(out->data + new_p + 0x48u, 0);
						write_s16(out->data + new_p + 0x54u, 4);
						write_s32(out->data + new_p + 0x58u, 0);
					}
				}
				write_u32(out->data + g_foff + 40u, CE_TAG_BASE_ADDR + (new_parts_foff - tag_data_offset));
			}
		}
	}

	/* 5. Convert Xbox 'bitm' raw pixel data into CE format and embed in out */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		int32_t b_cnt;
		uint32_t b_addr;
		int b_idx;

		if (!xt->def_foff || xt->is_reused_existing || xt->baked_tag_idx < 0 || xt->fourcc != 'bitm')
			continue;

		b_cnt = read_s32(out->data + xt->def_foff + 0x60u);
		b_addr = read_u32(out->data + xt->def_foff + 0x64u);
		if (b_cnt <= 0 || b_addr < CE_TAG_BASE_ADDR)
			continue;

		for (b_idx = 0; b_idx < b_cnt; b_idx++)
		{
			uint32_t bd_foff = tag_data_offset + (b_addr - CE_TAG_BASE_ADDR) + (uint32_t)b_idx * 48u;
			int16_t w = read_s16(out->data + bd_foff + 0x04u);
			int16_t h = read_s16(out->data + bd_foff + 0x06u);
			int16_t d = read_s16(out->data + bd_foff + 0x08u);
			int16_t type = read_s16(out->data + bd_foff + 0x0Au);
			int16_t fmt = read_s16(out->data + bd_foff + 0x0Cu);
			uint16_t flg = read_u16(out->data + bd_foff + 0x0Eu);
			int16_t mips = read_s16(out->data + bd_foff + 0x14u);
			uint32_t raw_off = read_u32(out->data + bd_foff + 0x18u) & 0x7FFFFFFFu;
			uint32_t ce_sz = 0;
			uint8_t *ce_pix = NULL;

			if (raw_off < xmap.file_size)
			{
				ce_pix = cb_unpack_xbox_bitmap_to_ce(
					xmap.data + raw_off, xmap.file_size - raw_off,
					w, h, d, type, fmt, flg, mips, &ce_sz);
			}
			if (ce_pix && ce_sz > 0)
			{
				uint16_t new_flg = (uint16_t)((flg & 0x00FFu) & ~0x0008u) | 0x0080u;
				if (is_red_beam_variant && fmt == 14 &&
					(strstr(xt->name, "jackal shield") != NULL || strstr(xt->name, "static blue") != NULL))
				{
					cb_tint_dxt1_blue_to_red(ce_pix, ce_sz);
				}
				bd_foff = tag_data_offset + (b_addr - CE_TAG_BASE_ADDR) + (uint32_t)b_idx * 48u;
				write_u16(out->data + bd_foff + 0x0Eu, new_flg);
				write_u32(out->data + bd_foff + 0x18u, 0u);
				write_u32(out->data + bd_foff + 0x1Cu, ce_sz);
				write_u32(out->data + bd_foff + 0x20u, xt->baked_handle);
				write_u32(out->data + bd_foff + 0x24u, 0u);
				write_u32(out->data + bd_foff + 0x28u, 0u);
				cb_alloc_deferred_media(ce_pix, ce_sz, 32, bd_foff + 0x18u);
			}
			free(ce_pix);
		}

		/* Ensure OpenCE's reordered_bitmaps_find sees this Xbox bitmap as drawn_otherwise
		 * so its Xbox multipurpose/meter RGBA channels are never reordered as Custom Edition! */
		{
			uint32_t guard_foff = buf_alloc(out, 440u, 4);
			if (guard_foff && out->data)
			{
				struct tag_schema_group const *soso_grp = forge_schema_group_get('soso');
				char guard_name[256];
				memset(out->data + guard_foff, 0, 440u);
				if (soso_grp && soso_grp->definition)
					out_map_init_schema_null_refs(out, tag_data_offset, soso_grp->definition, guard_foff, 1);
				write_s16(out->data + guard_foff + 0x24u, 4); /* _shader_type_model */
				write_u32(out->data + guard_foff + 0xA4u + 0u, 'bitm');
				write_u32(out->data + guard_foff + 0xA4u + 4u, xt->baked_name_addr);
				write_u32(out->data + guard_foff + 0xA4u + 8u, 0u);
				write_u32(out->data + guard_foff + 0xA4u + 12u, xt->baked_handle);
				snprintf(guard_name, sizeof(guard_name), "%s__xbox_ch_guard", xt->name);
				out_map_alloc_tag_instance(
					out, tag_data_offset, scnr_off,
					'soso', 'shdr', 0xFFFFFFFFu,
					guard_name, guard_foff, 0u,
					NULL, NULL);
			}
		}
	}

	/* 6. Copy Xbox 'snd!' raw audio samples (already Xbox ADPCM) and embed in out */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		int32_t pr_cnt;
		uint32_t pr_addr;
		int pr_i;

		if (!xt->def_foff || xt->is_reused_existing || xt->baked_tag_idx < 0 || xt->fourcc != 'snd!')
			continue;

		pr_cnt = read_s32(out->data + xt->def_foff + 0x98u);
		pr_addr = read_u32(out->data + xt->def_foff + 0x9Cu);
		if (pr_cnt <= 0 || pr_addr < CE_TAG_BASE_ADDR)
			continue;

		for (pr_i = 0; pr_i < pr_cnt; pr_i++)
		{
			uint32_t pr_foff = tag_data_offset + (pr_addr - CE_TAG_BASE_ADDR) + (uint32_t)pr_i * 72u;
			int32_t pm_cnt = read_s32(out->data + pr_foff + 0x3Cu);
			uint32_t pm_addr = read_u32(out->data + pr_foff + 0x40u);
			int pm_i;

			if (pm_cnt <= 0 || pm_addr < CE_TAG_BASE_ADDR)
				continue;
			for (pm_i = 0; pm_i < pm_cnt; pm_i++)
			{
				uint32_t pm_foff = tag_data_offset + (pm_addr - CE_TAG_BASE_ADDR) + (uint32_t)pm_i * 124u;
				int32_t smp_sz = read_s32(out->data + pm_foff + 0x40u);
				uint32_t smp_off = read_u32(out->data + pm_foff + 0x48u) & 0x7FFFFFFFu;

				/* Set tag_index0 (+0x34) and tag_index1 (+0x3C) to baked_handle */
				write_u32(out->data + pm_foff + 0x34u, xt->baked_handle);
				write_u32(out->data + pm_foff + 0x3Cu, xt->baked_handle);
				/* Clear mouth_data (+0x54) and subtitle_data (+0x68) */
				memset(out->data + pm_foff + 0x54u, 0, 40u);

				if (smp_sz > 0 && (uint64_t)smp_off + (uint64_t)smp_sz <= xmap.file_size)
				{
					uint32_t use_sz = (uint32_t)smp_sz;
					write_s32(out->data + pm_foff + 0x40u, (int32_t)use_sz);
					/* Bit 0 (1u) is SOUND_SAMPLES_IN_RESOURCE_MAP_BIT; 0u means embedded in this .map! */
					write_u32(out->data + pm_foff + 0x44u, 0u);
					write_u32(out->data + pm_foff + 0x48u, 0u);
					write_u32(out->data + pm_foff + 0x4Cu, 0u);
					write_u32(out->data + pm_foff + 0x50u, 0u);
					cb_alloc_deferred_media(xmap.data + smp_off, use_sz, 16, pm_foff + 0x48u);
				}
				else
				{
					memset(out->data + pm_foff + 0x40u, 0, 20u);
				}
			}
		}
	}

	/* 7. Apply light / door / vehicle patches so baked tags work identically in halo.exe */
	for (i = 0; i < xtag_count; i++)
	{
		struct cb_xbox_bake_tag *xt = &xtags[i];
		if (!xt->def_foff || xt->is_reused_existing || xt->baked_tag_idx < 0)
			continue;

		if (xt->fourcc == 'ligh')
		{
			uint32_t flags = read_u32(out->data + xt->def_foff + 0x00u);
			float rad = read_f32(out->data + xt->def_foff + 0x04u);
			flags |= 0x01u; /* _light_definition_dynamic_bit */
			write_u32(out->data + xt->def_foff + 0x00u, flags);
			if (rad < 4.5f)
				write_f32(out->data + xt->def_foff + 0x04u, 4.5f);
			write_f32(out->data + xt->def_foff + 0x08u, 1.0f);
			write_f32(out->data + xt->def_foff + 0x0Cu, 1.0f);
			if (is_red_beam_variant)
			{
				write_f32(out->data + xt->def_foff + 0x38u, 1.0f);
				write_f32(out->data + xt->def_foff + 0x3Cu, 1.0f);
				write_f32(out->data + xt->def_foff + 0x40u, 0.12f);
				write_f32(out->data + xt->def_foff + 0x44u, 0.08f);
				write_f32(out->data + xt->def_foff + 0x48u, 1.0f);
				write_f32(out->data + xt->def_foff + 0x4Cu, 1.0f);
				write_f32(out->data + xt->def_foff + 0x50u, 0.18f);
				write_f32(out->data + xt->def_foff + 0x54u, 0.10f);
			}
			else if (read_f32(out->data + xt->def_foff + 0x3Cu) <= 0.01f &&
				read_f32(out->data + xt->def_foff + 0x40u) <= 0.01f &&
				read_f32(out->data + xt->def_foff + 0x44u) <= 0.01f)
			{
				write_f32(out->data + xt->def_foff + 0x38u, 1.0f);
				write_f32(out->data + xt->def_foff + 0x3Cu, 1.0f);
				write_f32(out->data + xt->def_foff + 0x40u, 0.96f);
				write_f32(out->data + xt->def_foff + 0x44u, 0.88f);
				write_f32(out->data + xt->def_foff + 0x48u, 1.0f);
				write_f32(out->data + xt->def_foff + 0x4Cu, 1.0f);
				write_f32(out->data + xt->def_foff + 0x50u, 0.96f);
				write_f32(out->data + xt->def_foff + 0x54u, 0.88f);
			}
		}
		else if (xt->fourcc == 'lens' && is_red_beam_variant)
		{
			int32_t refl_cnt = read_s32(out->data + xt->def_foff + 0xC4u);
			uint32_t refl_addr = read_u32(out->data + xt->def_foff + 0xC8u);
			if (refl_cnt > 0 && refl_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t refl_base = tag_data_offset + (refl_addr - CE_TAG_BASE_ADDR);
				int r_idx;
				for (r_idx = 0; r_idx < refl_cnt; r_idx++)
				{
					uint32_t rf = refl_base + (uint32_t)r_idx * 128u;
					if (rf + 128u <= out->size)
					{
						write_f32(out->data + rf + 0x40u, 1.0f);
						write_f32(out->data + rf + 0x44u, 1.0f);
						write_f32(out->data + rf + 0x48u, 0.12f);
						write_f32(out->data + rf + 0x4Cu, 0.08f);
						write_f32(out->data + rf + 0x50u, 1.0f);
						write_f32(out->data + rf + 0x54u, 1.0f);
						write_f32(out->data + rf + 0x58u, 0.18f);
						write_f32(out->data + rf + 0x5Cu, 0.10f);
						write_f32(out->data + rf + 0x60u, 1.0f);
						write_f32(out->data + rf + 0x64u, 1.0f);
						write_f32(out->data + rf + 0x68u, 0.12f);
						write_f32(out->data + rf + 0x6Cu, 0.08f);
					}
				}
			}
		}
		else if (xt->fourcc == 'scen' || xt->fourcc == 'lifi')
		{
			if (read_f32(out->data + xt->def_foff + 0x04u) < 1.5f)
				write_f32(out->data + xt->def_foff + 0x04u, 1.5f);
			if (read_f32(out->data + xt->def_foff + 0x104u) < 8.0f)
				write_f32(out->data + xt->def_foff + 0x104u, 8.0f);
		}
		else if (xt->fourcc == 'mach')
		{
			int16_t mtype = read_s16(out->data + xt->def_foff + 0x290u);
			if (read_f32(out->data + xt->def_foff + 0x04u) < 1.5f)
				write_f32(out->data + xt->def_foff + 0x04u, 1.5f);
			if (read_f32(out->data + xt->def_foff + 0x104u) < 8.0f)
				write_f32(out->data + xt->def_foff + 0x104u, 8.0f);
			if (is_red_beam_variant || strstr(xt->name, "beam emitter") != NULL)
			{
				write_f32(out->data + xt->def_foff + 0x04u, 65.0f);  /* bounding_radius */
				write_f32(out->data + xt->def_foff + 0x10u, 0.0f);   /* bounding_offset.z */
				write_f32(out->data + xt->def_foff + 0x104u, 65.0f); /* render_bounding_radius */
			}
			if (mtype == 0 || strstr(xt->name, "door") != NULL)
			{
				write_s16(out->data + xt->def_foff + 0x290u, 0);    /* _machine_type_door */
				write_f32(out->data + xt->def_foff + 0x21Cu, 2.5f); /* device.automatic_activation_radius */
				write_u16(out->data + xt->def_foff + 0x292u,
					(uint16_t)(read_u16(out->data + xt->def_foff + 0x292u) | 0x0003u));
				write_f32(out->data + xt->def_foff + 0x294u, 3.0f); /* machine.door_open_time */
				write_s32(out->data + xt->def_foff + 0x320u, 90);   /* machine.runtime_door_open_ticks */
			}
		}
		else if (xt->fourcc == 'antr')
		{
			/* Clear _animation_world_relative_bit (0x0002) on scenery/character animations (e.g. flood_captain in d20)
			 * so placed objects render at their placed world position rather than teleporting to campaign cutscene coordinates. */
			int32_t anim_cnt = read_s32(out->data + xt->def_foff + 0x74u);
			uint32_t anim_addr = read_u32(out->data + xt->def_foff + 0x78u);
			float n0_tx = 0.0f, n0_ty = 0.0f, n0_tz = 0.0f;
			int m_k, a_idx;

			for (m_k = 0; m_k < xtag_count; m_k++)
			{
				if (xtags[m_k].fourcc == 'mode' && xtags[m_k].def_foff)
				{
					int32_t n_cnt = read_s32(out->data + xtags[m_k].def_foff + 0xB8u);
					uint32_t n_addr = read_u32(out->data + xtags[m_k].def_foff + 0xBCu);
					if (n_cnt > 0 && n_addr >= CE_TAG_BASE_ADDR)
					{
						uint32_t n0_foff = tag_data_offset + (n_addr - CE_TAG_BASE_ADDR);
						if (n0_foff + 156u <= out->size)
						{
							float tx = read_f32(out->data + n0_foff + 40u);
							float ty = read_f32(out->data + n0_foff + 44u);
							float tz = read_f32(out->data + n0_foff + 48u);
							if (tx > -10.0f && tx < 10.0f && ty > -10.0f && ty < 10.0f && tz > -10.0f && tz < 10.0f)
							{
								n0_tx = tx;
								n0_ty = ty;
								n0_tz = tz;
							}
						}
					}
					break;
				}
			}

			if (anim_cnt > 0 && anim_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t anim_base = tag_data_offset + (anim_addr - CE_TAG_BASE_ADDR);
				for (a_idx = 0; a_idx < anim_cnt; a_idx++)
				{
					uint32_t af = anim_base + (uint32_t)a_idx * 180u;
					uint16_t aflags;
					if (af + 180u > out->size)
						break;
					aflags = read_u16(out->data + af + 0x3Au);
					if (aflags & 0x0002u)
					{
						write_u16(out->data + af + 0x3Au, (uint16_t)(aflags & ~0x0002u));
						if (!(aflags & 0x0001u))
						{
							uint32_t trans_flags0 = read_u32(out->data + af + 0x5Cu);
							uint32_t rot_flags0 = read_u32(out->data + af + 0x64u);
							uint32_t n0_trans_off = (rot_flags0 & 1u) ? 0u : 8u;
							if (!(trans_flags0 & 1u))
							{
								int32_t def_sz = read_s32(out->data + af + 0x80u);
								uint32_t def_addr = read_u32(out->data + af + 0x88u);
								if (def_sz >= (int32_t)(n0_trans_off + 12u) && def_addr >= CE_TAG_BASE_ADDR)
								{
									uint32_t d_foff = tag_data_offset + (def_addr - CE_TAG_BASE_ADDR) + n0_trans_off;
									if (d_foff + 12u <= out->size)
									{
										write_f32(out->data + d_foff + 0u, n0_tx);
										write_f32(out->data + d_foff + 4u, n0_ty);
										write_f32(out->data + d_foff + 8u, n0_tz);
									}
								}
							}
							else
							{
								int16_t frame_cnt = read_s16(out->data + af + 0x24u);
								int16_t frame_sz = read_s16(out->data + af + 0x26u);
								int32_t fdata_sz = read_s32(out->data + af + 0x94u);
								uint32_t fdata_addr = read_u32(out->data + af + 0x9Cu);
								if (frame_cnt > 0 && frame_sz >= (int16_t)(n0_trans_off + 12u) &&
									fdata_sz > 0 && fdata_addr >= CE_TAG_BASE_ADDR)
								{
									uint32_t fb_foff = tag_data_offset + (fdata_addr - CE_TAG_BASE_ADDR);
									int f_i;
									for (f_i = 0; f_i < frame_cnt; f_i++)
									{
										uint32_t f_off = (uint32_t)f_i * (uint32_t)frame_sz + n0_trans_off;
										if (f_off + 12u <= (uint32_t)fdata_sz && fb_foff + f_off + 12u <= out->size)
										{
											write_f32(out->data + fb_foff + f_off + 0u, n0_tx);
											write_f32(out->data + fb_foff + f_off + 4u, n0_ty);
											write_f32(out->data + fb_foff + f_off + 8u, n0_tz);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else if (xt->fourcc == 'vehi')
		{
			int32_t seat_cnt = read_s32(out->data + xt->def_foff + 0x2DCu);
			uint32_t seat_addr = read_u32(out->data + xt->def_foff + 0x2E0u);
			write_u32(out->data + xt->def_foff + 0x17Cu,
				read_u32(out->data + xt->def_foff + 0x17Cu) | (1u << 29) | (1u << 4));
			if (seat_cnt > 0 && seat_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t s0_foff = tag_data_offset + (seat_addr - CE_TAG_BASE_ADDR);
				if (s0_foff + 284u <= out->size)
				{
					uint32_t sflags = read_u32(out->data + s0_foff + 0x00u);
					char const *slabel = (char const *)(out->data + s0_foff + 0x04u);
					sflags &= ~0x01u; /* clear _unit_seat_invisible_bit */
					sflags |= (1u << 2) | (1u << 4) | (1u << 5); /* driver | third_person_camera | third_person_on_enter */
					if (strstr(xt->name, "wraith") != NULL)
						sflags |= (1u << 3); /* _unit_seat_gunner_bit */
					write_u32(out->data + s0_foff + 0x00u, sflags);
					if (_stricmp(slabel, "W-driver") != 0 &&
						_stricmp(slabel, "G-driver") != 0 &&
						_stricmp(slabel, "B-driver") != 0 &&
						_stricmp(slabel, "S-driver") != 0 &&
						_stricmp(slabel, "R-driver") != 0 &&
						_stricmp(slabel, "scorpion-driver") != 0 &&
						_stricmp(slabel, "CT-driver") != 0)
					{
						memset(out->data + s0_foff + 0x04u, 0, 32u);
						memcpy(out->data + s0_foff + 0x04u, "W-driver", 8u);
					}
				}
			}
		}
		else if (xt->fourcc == 'coll' && strstr(xt->name, "wraith") != NULL && xt->def_foff + 0x24Cu <= out->size)
		{
			int32_t mat_cnt = read_s32(out->data + xt->def_foff + 0x234u);
			uint32_t mat_addr = read_u32(out->data + xt->def_foff + 0x238u);
			int32_t reg_cnt = read_s32(out->data + xt->def_foff + 0x240u);
			uint32_t reg_addr = read_u32(out->data + xt->def_foff + 0x244u);
			int32_t k;
			write_f32(out->data + xt->def_foff + 0x08u, 0.0f);
			write_u32(out->data + xt->def_foff + 0xB4u, 0xFFFFFFFFu);
			write_f32(out->data + xt->def_foff + 0xB8u, 0.0f);
			write_u32(out->data + xt->def_foff + 0xC8u, 0xFFFFFFFFu);
			write_f32(out->data + xt->def_foff + 0xCCu, 0.0f);
			if (mat_cnt > 0 && mat_cnt <= 64 && mat_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t m_base = tag_data_offset + (mat_addr - CE_TAG_BASE_ADDR);
				for (k = 0; k < mat_cnt && m_base + (uint32_t)(k + 1) * 0x48u <= out->size; k++)
				{
					write_f32(out->data + m_base + (uint32_t)k * 0x48u + 0x2Cu, 0.0f);
					write_f32(out->data + m_base + (uint32_t)k * 0x48u + 0x3Cu, 0.0f);
				}
			}
			if (reg_cnt > 0 && reg_cnt <= 64 && reg_addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t r_base = tag_data_offset + (reg_addr - CE_TAG_BASE_ADDR);
				for (k = 0; k < reg_cnt && r_base + (uint32_t)(k + 1) * 0x54u <= out->size; k++)
				{
					uint32_t r_foff = r_base + (uint32_t)k * 0x54u;
					write_u32(out->data + r_foff + 0x20u, read_u32(out->data + r_foff + 0x20u) & ~0x0Eu);
					write_f32(out->data + r_foff + 0x28u, 0.0f);
					write_u32(out->data + r_foff + 0x44u, 0xFFFFFFFFu);
				}
			}
		}
	}

	root_baked_idx = xtags[0].baked_tag_idx;
	free(xtags);
	forge_source_map_close(&xmap);
	return root_baked_idx;
}

static int32_t out_map_clone_vehicle_tag_for_mp(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	int32_t src_vehi_tag_idx,
	int32_t unique_seq)
{
	uint32_t io, daddr, new_foff;
	struct tag_schema_group const *sgrp = forge_schema_group_get('vehi');
	uint32_t root_sz = (sgrp && sgrp->definition && sgrp->definition->size > 0)
		? (uint32_t)sgrp->definition->size : 1008u;
	char neutral_name[128];

	if (!out || !out->data || src_vehi_tag_idx < 0)
		return src_vehi_tag_idx;
	io = tag_data_offset + 0x28u + (uint32_t)src_vehi_tag_idx * 32u;
	if (io + 32u > out->size)
		return src_vehi_tag_idx;
	daddr = read_u32(out->data + io + 0x14u);
	if (daddr < CE_TAG_BASE_ADDR || tag_data_offset + (daddr - CE_TAG_BASE_ADDR) + root_sz > out->size)
		return src_vehi_tag_idx;

	new_foff = buf_alloc(out, root_sz, 4);
	if (!new_foff || !out->data)
		return src_vehi_tag_idx;
	memcpy(out->data + new_foff, out->data + tag_data_offset + (daddr - CE_TAG_BASE_ADDR), root_sz);
	if (sgrp && sgrp->definition)
		out_map_clone_schema_blocks(out, tag_data_offset, sgrp->definition, new_foff, 1);

	/* Give the placed vehicle a neutral tag path that does not match any of the 6 substring checks
	 * in game_engine_variant_vehicle_type (warthog, ghost, scorpion, rwarthog, banshee, turret)
	 * so standard halo.exe spawns it unconditionally in all multiplayer game modes! */
	snprintf(
		neutral_name,
		sizeof(neutral_name),
		"forge\\vehicles\\placed_unit_%d_%d",
		(int)read_s32(out->data + tag_data_offset + 0x0Cu),
		(int)unique_seq);
	return out_map_alloc_tag_instance(
		out, tag_data_offset, scnr_off,
		'vehi', 'unit', 'obje',
		neutral_name, new_foff, 0u,
		NULL, NULL);
}

static int32_t out_map_create_item_collection_for_tag(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	uint32_t item_group,
	int32_t item_tag_idx,
	int32_t unique_seq,
	uint32_t *out_itmc_handle,
	uint32_t *out_itmc_name_addr)
{
	uint32_t io, item_handle, item_name_addr;
	uint32_t itmc_foff, perm_foff;
	char itmc_name[128];

	if (out_itmc_handle) *out_itmc_handle = 0xFFFFFFFFu;
	if (out_itmc_name_addr) *out_itmc_name_addr = 0u;
	if (!out || !out->data || item_tag_idx < 0)
		return -1;
	io = tag_data_offset + 0x28u + (uint32_t)item_tag_idx * 32u;
	if (io + 32u > out->size)
		return -1;
	item_handle = read_u32(out->data + io + 0x0Cu);
	item_name_addr = read_u32(out->data + io + 0x10u);

	itmc_foff = buf_alloc(out, (uint32_t)sizeof(struct item_collection_definition), 4);
	perm_foff = buf_alloc(out, (uint32_t)sizeof(struct item_permutation_definition), 4);
	if (!itmc_foff || !perm_foff || !out->data)
		return -1;

	memset(out->data + itmc_foff, 0, sizeof(struct item_collection_definition));
	write_s32(out->data + itmc_foff + 0x00u, 1);
	write_u32(out->data + itmc_foff + 0x04u, CE_TAG_BASE_ADDR + (perm_foff - tag_data_offset));
	write_u32(out->data + itmc_foff + 0x08u, 0u);
	write_s16(out->data + itmc_foff + 0x0Cu, 30); /* spawn_time = 30s */

	memset(out->data + perm_foff, 0, sizeof(struct item_permutation_definition));
	write_f32(out->data + perm_foff + 0x20u, 100.0f); /* weight = 100.0f */
	write_u32(out->data + perm_foff + 0x24u, item_group);
	write_u32(out->data + perm_foff + 0x28u, item_name_addr);
	write_s32(out->data + perm_foff + 0x2Cu, 0);
	write_u32(out->data + perm_foff + 0x30u, item_handle);

	snprintf(itmc_name, sizeof(itmc_name), "forge\\items\\placed_item_%d", (int)unique_seq);
	return out_map_alloc_tag_instance(
		out, tag_data_offset, scnr_off,
		'itmc', 0xFFFFFFFFu, 0xFFFFFFFFu,
		itmc_name, itmc_foff, 0u,
		out_itmc_handle, out_itmc_name_addr);
}

static int32_t out_map_bake_non_native_object(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_off,
	uint32_t orig_vtx_size,
	uint32_t orig_idx_size,
	struct forge_pending_bake_obj const *pobj)
{
	if (!pobj)
		return -1;
	if (pobj->faber_profile_idx >= 0 && pobj->faber_tag_idx >= 0)
	{
		return out_map_bake_faber_vault_bundle(
			out, tag_data_offset, scnr_off, orig_vtx_size, orig_idx_size, pobj);
	}
	if (pobj->xbox_map_path[0])
	{
		return out_map_bake_xbox_tag_closure(
			out, tag_data_offset, scnr_off, orig_vtx_size, orig_idx_size, pobj);
	}
	return -1;
}

static uint32_t compute_ce_map_checksum(uint8_t const *map_bytes, uint32_t map_size, uint32_t tag_data_offset, uint32_t scnr_foff)
{
	uint32_t crc = 0xFFFFFFFFu;
	uint32_t tag_data_size;
	uint32_t vtx_off, model_sz;
	int32_t bsp_count, i;
	uint32_t bsp_addr;

	if (!map_bytes || tag_data_offset + 0x28u > map_size)
		return 0;

	/* 1. Structure BSPs */
	if (scnr_foff + 0x5B0u <= map_size)
	{
		bsp_count = read_s32(map_bytes + scnr_foff + 0x5A4u);
		bsp_addr = read_u32(map_bytes + scnr_foff + 0x5A8u);
		if (bsp_count > 0 && bsp_count <= 32 && bsp_addr >= CE_TAG_BASE_ADDR)
		{
			uint32_t barr_foff = tag_data_offset + (bsp_addr - CE_TAG_BASE_ADDR);
			for (i = 0; i < bsp_count; i++)
			{
				uint32_t beo = barr_foff + (uint32_t)i * 32u;
				if (beo + 32u <= map_size)
				{
					uint32_t boff = read_u32(map_bytes + beo + 0x00u);
					uint32_t bsz = read_u32(map_bytes + beo + 0x04u);
					if (boff >= 0x800u && (uint64_t)boff + bsz <= map_size)
						crc = forge_crc32_update(crc, map_bytes + boff, bsz);
				}
			}
		}
	}

	/* 2. Model data (vertices + triangles) */
	vtx_off = read_u32(map_bytes + tag_data_offset + 0x14u);
	model_sz = read_u32(map_bytes + tag_data_offset + 0x20u);
	if (vtx_off >= 0x800u && (uint64_t)vtx_off + model_sz <= map_size)
		crc = forge_crc32_update(crc, map_bytes + vtx_off, model_sz);

	/* 3. Tag data */
	tag_data_size = read_u32(map_bytes + 0x14u);
	if ((uint64_t)tag_data_offset + tag_data_size <= map_size)
		crc = forge_crc32_update(crc, map_bytes + tag_data_offset, tag_data_size);

	return crc;
}

static int forge_mem_source_read(void *context, uint32_t offset, uint32_t size, void *buffer)
{
	struct forge_byte_buf const *buf = (struct forge_byte_buf const *)context;
	if (!buf || !buf->data || (uint64_t)offset + size > buf->size)
		return 0;
	if (size > 0)
		memcpy(buffer, buf->data + offset, size);
	return 1;
}

static void inplace_apply_default_object_edit(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_foff,
	struct forge_default_object_edit const *de)
{
	static struct
	{
		uint32_t blk_off;
		uint32_t elem_sz;
	} const obj_blocks[8] = {
		{ (uint32_t)offsetof(struct scenario, scenery), 72u },
		{ (uint32_t)offsetof(struct scenario, bipeds), 120u },
		{ (uint32_t)offsetof(struct scenario, vehicles), 120u },
		{ (uint32_t)offsetof(struct scenario, equipment), 40u },
		{ (uint32_t)offsetof(struct scenario, weapons), 92u },
		{ (uint32_t)offsetof(struct scenario, machines), 64u },
		{ (uint32_t)offsetof(struct scenario, controls), 64u },
		{ (uint32_t)offsetof(struct scenario, light_fixtures), 88u }
	};
	uint32_t best_obj_foff = 0;
	float best_obj_d2 = 36.0f; /* Within 6.0 world units of original spawn position */
	uint32_t best_ne_foff = 0;
	float best_ne_d2 = 36.0f;
	size_t b;
	int32_t i;

	if (!out || !out->data || !de || !de->active)
		return;

	/* 1. Search standard scenario object placement blocks */
	for (b = 0; b < 8; b++)
	{
		uint32_t f_off = scnr_foff + obj_blocks[b].blk_off;
		int32_t cnt;
		uint32_t addr, arr_foff;
		if (f_off + 12u > out->size)
			continue;
		cnt = read_s32(out->data + f_off + 0u);
		addr = read_u32(out->data + f_off + 4u);
		if (cnt <= 0 || addr < CE_TAG_BASE_ADDR)
			continue;
		arr_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
		for (i = 0; i < cnt; i++)
		{
			uint32_t eo = arr_foff + (uint32_t)i * obj_blocks[b].elem_sz;
			float ex, ey, ez, dx, dy, dz, d2;
			if (eo + obj_blocks[b].elem_sz > out->size)
				break;
			ex = read_f32(out->data + eo + 0x08u);
			ey = read_f32(out->data + eo + 0x0Cu);
			ez = read_f32(out->data + eo + 0x10u);
			dx = ex - de->orig_x;
			dy = ey - de->orig_y;
			dz = ez - de->orig_z;
			d2 = dx * dx + dy * dy + dz * dz;
			if (d2 < best_obj_d2)
			{
				best_obj_d2 = d2;
				best_obj_foff = eo;
			}
		}
	}

	if (best_obj_foff > 0)
	{
		if (de->deleted)
		{
			write_s16(out->data + best_obj_foff + 0x00u, -1);
			write_f32(out->data + best_obj_foff + 0x08u, -99999.0f);
			write_f32(out->data + best_obj_foff + 0x0Cu, -99999.0f);
			write_f32(out->data + best_obj_foff + 0x10u, -99999.0f);
		}
		else
		{
			write_f32(out->data + best_obj_foff + 0x08u, de->new_x);
			write_f32(out->data + best_obj_foff + 0x0Cu, de->new_y);
			write_f32(out->data + best_obj_foff + 0x10u, de->new_z);
			write_f32(out->data + best_obj_foff + 0x14u, de->new_yaw);
			write_f32(out->data + best_obj_foff + 0x18u, de->new_pitch);
			write_f32(out->data + best_obj_foff + 0x1Cu, de->new_roll);
		}
	}

	/* 2. Search multiplayer netgame_equipment block (MP weapons, powerups, grenades, vehicles) */
	{
		uint32_t ne_field = scnr_foff + (uint32_t)offsetof(struct scenario, netgame_equipment);
		if (ne_field + 12u <= out->size)
		{
			int32_t cnt = read_s32(out->data + ne_field + 0u);
			uint32_t addr = read_u32(out->data + ne_field + 4u);
			if (cnt > 0 && addr >= CE_TAG_BASE_ADDR)
			{
				uint32_t arr_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
				for (i = 0; i < cnt; i++)
				{
					uint32_t eo = arr_foff + (uint32_t)i * 144u;
					float ex, ey, ez, dx, dy, dz, d2;
					if (eo + 144u > out->size)
						break;
					ex = read_f32(out->data + eo + 0x40u);
					ey = read_f32(out->data + eo + 0x44u);
					ez = read_f32(out->data + eo + 0x48u);
					dx = ex - de->orig_x;
					dy = ey - de->orig_y;
					dz = ez - de->orig_z;
					d2 = dx * dx + dy * dy + dz * dz;
					if (d2 < best_ne_d2)
					{
						best_ne_d2 = d2;
						best_ne_foff = eo;
					}
				}
			}
		}
	}

	if (best_ne_foff > 0 && (best_obj_foff == 0 || best_ne_d2 <= best_obj_d2))
	{
		if (de->deleted)
		{
			write_s16(out->data + best_ne_foff + 0x04u, 0);
			write_s16(out->data + best_ne_foff + 0x06u, 0);
			write_s16(out->data + best_ne_foff + 0x08u, 0);
			write_s16(out->data + best_ne_foff + 0x0Au, 0);
			write_s32(out->data + best_ne_foff + 0x5Cu, -1);
			write_f32(out->data + best_ne_foff + 0x40u, -99999.0f);
			write_f32(out->data + best_ne_foff + 0x44u, -99999.0f);
			write_f32(out->data + best_ne_foff + 0x48u, -99999.0f);
		}
		else
		{
			write_f32(out->data + best_ne_foff + 0x40u, de->new_x);
			write_f32(out->data + best_ne_foff + 0x44u, de->new_y);
			write_f32(out->data + best_ne_foff + 0x48u, de->new_z);
			write_f32(out->data + best_ne_foff + 0x4Cu, de->new_yaw);
		}
	}
}

static int append_forge_map_metadata_and_preview_trailer(
	struct forge_byte_buf *out,
	char const *output_map_name,
	char const *base_map_label,
	char const *blueprint_map_path,
	struct forge_scenario_edit_state const *edits,
	struct forge_source_map const *ce_base_map,
	struct forge_pending_bake_obj const *pending_objs,
	int pending_count)
{
	struct forge_byte_buf state_buf;
	struct forge_map_blueprint_data *bp = NULL;
	char custom_bmp_hint[512];
	char desc_buf[512];
	char line[512];
	uint8_t *bmp_mem = NULL;
	uint32_t bmp_size = 0, desc_len = 0;
	uint32_t state_foff = 0, state_size = 0, bmp_foff = 0, desc_foff = 0;
	int line_len, i, j;

	buf_init(&state_buf, 16384u);
	line_len = snprintf(line, sizeof(line),
		"[OpenCE Forge Map]\r\n"
		"ForgeSource=UserCreated\r\n"
		"Map Name: %s\r\n"
		"Base Map: %s\r\n"
		"Author: OpenCE : Forge Creator\r\n"
		"Objects Placed: %d\r\n"
		"AI Squads: %d\r\n"
		"[ForgeState]\r\n",
		output_map_name,
		base_map_label,
		edits ? (int)edits->placed_object_count : 0,
		edits ? (int)edits->ai_spawn_count : 0);
	buf_append_bytes(&state_buf, line, line_len);
	if (edits)
	{
		for (i = 0; i < edits->default_edit_count; i++)
		{
			struct forge_default_object_edit const *de = &edits->default_edits[i];
			if (!de->active)
				continue;
			line_len = snprintf(line, sizeof(line),
				"DEFAULT_EDIT|%u|%d|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%s\r\n",
				(unsigned)de->group_tag, (int)de->deleted,
				de->orig_x, de->orig_y, de->orig_z,
				de->new_x, de->new_y, de->new_z,
				de->new_yaw, de->new_pitch, de->new_roll,
				de->tag_path);
			buf_append_bytes(&state_buf, line, line_len);
		}
		for (i = 0; i < edits->placed_object_count; i++)
		{
			struct forge_placed_object_entry const *po = &edits->placed_objects[i];
			int baked = 0;
			if (!po->tag_path[0] || po->x < -90000.0f)
				continue;
			if (ce_base_map)
			{
				uint32_t g = po->group_tag;
				if (g == 'mach' || g == 'lifi' || g == 'ctrl')
					;
				else if (g == 'scen' || po->category == _forge_object_scenery) g = 'scen';
				else if (g == 'bipd' || po->category == _forge_object_biped) g = 'bipd';
				else if (g == 'vehi' || po->category == _forge_object_vehicle) g = 'vehi';
				else if (g == 'weap' || po->category == _forge_object_weapon) g = 'weap';
				else if (g == 'eqip' || po->category == _forge_object_equipment) g = 'eqip';
				if (src_map_find_tag_or_variant(ce_base_map, g, po->tag_path) >= 0)
					baked = 1;
				else
				{
					for (j = 0; j < pending_count; j++)
					{
						if (pending_objs[j].group_tag == g &&
							_stricmp(pending_objs[j].tag_path, po->tag_path) == 0 &&
							pending_objs[j].baked_tag_idx >= 0)
						{
							baked = 1;
							break;
						}
					}
				}
			}
			line_len = snprintf(line, sizeof(line),
				"OBJECT|%u|%d|%d|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%d|%s\r\n",
				(unsigned)po->group_tag,
				(int)po->source_cache_id,
				baked,
				po->x, po->y, po->z,
				po->yaw, po->pitch, po->roll,
				(int)po->team_index,
				po->tag_path);
			buf_append_bytes(&state_buf, line, line_len);
			if (po->gametype_flags != 0 && po->gametype_flags != 0x0FFFu)
			{
				line_len = snprintf(line, sizeof(line),
					"OBJECT_GAMETYPE|%u\r\n",
					(unsigned)po->gametype_flags);
				buf_append_bytes(&state_buf, line, line_len);
			}
		}
		if (edits->replace_player_spawns)
		{
			for (i = 0; i < edits->player_spawn_count; i++)
			{
				struct forge_player_spawn_entry const *sp = &edits->player_spawns[i];
				int steam = (sp->type0 == 13) ? 2 : (int)sp->team_index;
				int smode = (sp->type0 == 13) ? (int)sp->type1 : (int)sp->type0;
				unsigned int sgflags =
					(smode == 1) ? 0x0001u :
					(smode == 2) ? 0x0002u :
					(smode == 3) ? 0x0004u :
					(smode == 4) ? 0x0008u :
					(smode == 5) ? 0x0010u : 0u;
				line_len = snprintf(line, sizeof(line),
					"SPAWN|%.4f|%.4f|%.4f|%.4f|%d|%u\r\n",
					sp->x, sp->y, sp->z, sp->yaw,
					steam,
					sgflags);
				buf_append_bytes(&state_buf, line, line_len);
			}
		}
		if (edits->replace_netgame_flags)
		{
			for (i = 0; i < edits->netgame_flag_count; i++)
			{
				struct forge_netgame_flag_entry const *nf = &edits->netgame_flags[i];
				line_len = snprintf(line, sizeof(line),
					"NETGAME_FLAG|%.4f|%.4f|%.4f|%.4f|%d|%d\r\n",
					nf->x, nf->y, nf->z, nf->yaw,
					(int)nf->type,
					(int)nf->usage_id);
				buf_append_bytes(&state_buf, line, line_len);
			}
		}
		for (i = 0; i < edits->ai_spawn_count; i++)
		{
			struct forge_ai_spawn_entry const *ai = &edits->ai_spawns[i];
			if (!ai->actor_variant_path[0])
				continue;
			line_len = snprintf(line, sizeof(line),
				"AI|%.4f|%.4f|%.4f|%.4f|%d|%s\r\n",
				ai->x, ai->y, ai->z, ai->yaw,
				(int)ai->team_designator,
				ai->actor_variant_path);
			buf_append_bytes(&state_buf, line, line_len);
		}
	}

	desc_len = (uint32_t)snprintf(desc_buf, sizeof(desc_buf),
		"OpenCE : Forge Map: %s\r\n"
		"Base Map: %s\r\n"
		"Author: OpenCE : Forge Creator\r\n"
		"Objects Placed: %d\r\n",
		output_map_name,
		base_map_label,
		edits ? (int)edits->placed_object_count : 0);

	snprintf(custom_bmp_hint, sizeof(custom_bmp_hint), "OpenCE Forge/custom_bmp/%s.bmp", output_map_name);
	bp = (struct forge_map_blueprint_data *)calloc(1, sizeof(*bp));
	if (bp)
	{
		forge_load_map_blueprint(blueprint_map_path, bp);
		bmp_mem = build_blueprint_bmp_buffer(custom_bmp_hint, bp, edits, &bmp_size);
		forge_free_map_blueprint(bp);
		free(bp);
		bp = NULL;
	}

	if (state_buf.data && state_buf.size > 0)
	{
		state_size = state_buf.size;
		state_foff = buf_alloc(out, state_size + 1u, ce_base_map ? 4u : 1u);
		if (state_foff && out->data)
		{
			memcpy(out->data + state_foff, state_buf.data, state_size);
			out->data[state_foff + state_size] = 0;
		}
	}
	buf_free(&state_buf);

	if (bmp_mem && bmp_size > 0)
	{
		bmp_foff = buf_alloc(out, bmp_size, 4);
		if (bmp_foff && out->data)
			memcpy(out->data + bmp_foff, bmp_mem, bmp_size);
		free(bmp_mem);
	}

	if (desc_len > 0)
	{
		desc_foff = buf_alloc(out, desc_len + 1u, 4);
		if (desc_foff && out->data)
		{
			memcpy(out->data + desc_foff, desc_buf, desc_len);
			out->data[desc_foff + desc_len] = 0;
		}
	}

	if (ce_base_map)
		buf_alloc(out, 0, 32);
	else
		buf_alloc(out, 0, 512);

	if (!out->data)
		return 0;

	memset(out->data + 0x20u, 0, 32);
	snprintf((char *)(out->data + 0x20u), 32, "%s", output_map_name);
	write_u32(out->data + 0x1E4u, state_foff);
	write_u32(out->data + 0x1E8u, state_size);
	write_u32(out->data + 0x1ECu, bmp_foff);
	write_u32(out->data + 0x1F0u, bmp_size);
	write_u32(out->data + 0x1F4u, desc_foff);
	write_u32(out->data + 0x1F8u, desc_len);
	write_u32(out->data + 0x1FCu, 0x464C4731u); /* 'FLG1' OpenCE : Forge map signature */
	return 1;
}

static int forge_build_xbox_flg1_map(
	char const *want_file,
	char const *base_map_path,
	char const *output_map_name,
	struct forge_scenario_edit_state const *edits)
{
	char xbox_path[512];
	char base_stem[64];
	char map_path[512];
	FILE *in_f = NULL;
	FILE *out_f = NULL;
	long raw_sz = 0;
	struct forge_byte_buf out;

	snprintf(base_stem, sizeof(base_stem), "%s", want_file);
	{
		char *dot = strrchr(base_stem, '.');
		if (dot)
			*dot = '\0';
	}
	if (!base_stem[0])
		snprintf(base_stem, sizeof(base_stem), "bloodgulch");

	snprintf(xbox_path, sizeof(xbox_path), "maps/%s.map", base_stem);
	in_f = fopen(xbox_path, "rb");
	if (!in_f)
	{
		char d_xbox[160];
		snprintf(d_xbox, sizeof(d_xbox), "d:\\maps\\%s.map", base_stem);
		platform_translate_path(d_xbox, xbox_path, sizeof(xbox_path));
		in_f = fopen(xbox_path, "rb");
	}
	if (!in_f && base_map_path && base_map_path[0])
	{
		snprintf(xbox_path, sizeof(xbox_path), "%s", base_map_path);
		in_f = fopen(xbox_path, "rb");
	}
	if (!in_f)
	{
		char d_custom[160];
		snprintf(d_custom, sizeof(d_custom), "d:\\custom_maps\\%s.map", base_stem);
		platform_translate_path(d_custom, xbox_path, sizeof(xbox_path));
		in_f = fopen(xbox_path, "rb");
	}
	if (!in_f)
	{
		snprintf(xbox_path, sizeof(xbox_path), "custom_maps/%s.map", base_stem);
		in_f = fopen(xbox_path, "rb");
	}
	if (!in_f)
		return 0;

	if (fseek(in_f, 0, SEEK_END) != 0 || (raw_sz = ftell(in_f)) < 0x800L || fseek(in_f, 0, SEEK_SET) != 0)
	{
		fclose(in_f);
		return 0;
	}

	/* If in_f is an already-saved FLG1 Xbox map, strip its previous trailer at 0x1E4 and preserve its Base Map stem */
	{
		uint8_t hdr_peek[0x200];
		if (fread(hdr_peek, 1, sizeof(hdr_peek), in_f) == sizeof(hdr_peek))
		{
			uint32_t prev_sig = read_u32(hdr_peek + 0x1FCu);
			uint32_t prev_st = read_u32(hdr_peek + 0x1E4u);
			uint32_t prev_st_sz = read_u32(hdr_peek + 0x1E8u);
			if (prev_sig == 0x464C4731u && prev_st >= 0x800u && (long)prev_st < raw_sz)
			{
				if (prev_st_sz > 0 && prev_st_sz < 262144u && (long)(prev_st + prev_st_sz) <= raw_sz)
				{
					char *st_buf = (char *)malloc(prev_st_sz + 1u);
					if (st_buf)
					{
						if (fseek(in_f, (long)prev_st, SEEK_SET) == 0 &&
							fread(st_buf, 1, prev_st_sz, in_f) == prev_st_sz)
						{
							char const *bm;
							st_buf[prev_st_sz] = '\0';
							bm = strstr(st_buf, "Base Map: ");
							if (bm)
							{
								size_t bi = 0;
								bm += 10;
								while (bm[bi] && bm[bi] != '\r' && bm[bi] != '\n' && bi + 1 < sizeof(base_stem))
								{
									base_stem[bi] = bm[bi];
									bi++;
								}
								base_stem[bi] = '\0';
							}
						}
						free(st_buf);
					}
				}
				raw_sz = (long)prev_st;
			}
		}
		fseek(in_f, 0, SEEK_SET);
	}

	buf_init(&out, (uint32_t)raw_sz + 65536u);
	if (!out.data || fread(out.data, 1, (size_t)raw_sz, in_f) != (size_t)raw_sz)
	{
		fclose(in_f);
		buf_free(&out);
		return 0;
	}
	fclose(in_f);
	out.size = (uint32_t)raw_sz;

	if (read_u32(out.data + 0x00u) != CACHE_HEADER_SIGNATURE ||
		read_s32(out.data + 0x04u) != CACHE_VERSION_XBOX ||
		read_s16(out.data + 0x60u) != 1)
	{
		buf_free(&out);
		return 0;
	}

	if (!append_forge_map_metadata_and_preview_trailer(
			&out,
			output_map_name,
			base_stem,
			xbox_path,
			edits,
			NULL,
			NULL,
			0))
	{
		buf_free(&out);
		return 0;
	}

	{
		char d_dir[512];
		char d_map[160];
		char tmp_map_path[520];
		char legacy_path[512];
		d_dir[0] = '\0';
		platform_translate_path("d:\\custom_maps", d_dir, sizeof(d_dir));
		if (d_dir[0])
			FORGE_MKDIR(d_dir);
		FORGE_MKDIR("custom_maps");
		snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.map", output_map_name);
		map_path[0] = '\0';
		platform_translate_path(d_map, map_path, sizeof(map_path));
		if (!map_path[0])
			snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", output_map_name);
		snprintf(tmp_map_path, sizeof(tmp_map_path), "%s.tmp", map_path);
		out_f = fopen(tmp_map_path, "wb");
		if (!out_f)
		{
			snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", output_map_name);
			snprintf(tmp_map_path, sizeof(tmp_map_path), "%s.tmp", map_path);
			out_f = fopen(tmp_map_path, "wb");
		}
		if (!out_f)
		{
			buf_free(&out);
			return 0;
		}
		if (fwrite(out.data, 1, out.size, out_f) != out.size)
		{
			fclose(out_f);
			remove(tmp_map_path);
			buf_free(&out);
			return 0;
		}
		fclose(out_f);
		if (forge_host_rename(tmp_map_path, map_path) != 0)
		{
			remove(tmp_map_path);
			buf_free(&out);
			return 0;
		}
		snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.txt", output_map_name);
		legacy_path[0] = '\0';
		platform_translate_path(d_map, legacy_path, sizeof(legacy_path));
		if (legacy_path[0])
			remove(legacy_path);
		snprintf(legacy_path, sizeof(legacy_path), "custom_maps/%s.txt", output_map_name);
		remove(legacy_path);
	}
	buf_free(&out);
	return 1;
}

static void cb_neutralize_baked_object_placement(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_foff,
	uint32_t group_tag,
	float x,
	float y,
	float z)
{
	uint32_t place_field_off = 0, place_elem_sz = 0;
	int is_netgame_eq = 0;
	int32_t cnt, k, best_k = -1;
	uint32_t addr, arr_foff;
	float best_d2 = 0.5f;

	if (!out || !out->data)
		return;

	if (group_tag == 0x77656170u || group_tag == 0x65716970u)
	{
		is_netgame_eq = 1;
		place_field_off = (uint32_t)offsetof(struct scenario, netgame_equipment);
		place_elem_sz = 144u;
	}
	else if (group_tag == 0x76656869u)
	{
		place_field_off = (uint32_t)offsetof(struct scenario, vehicles);
		place_elem_sz = 120u;
	}
	else if (group_tag == 0x62697064u)
	{
		place_field_off = (uint32_t)offsetof(struct scenario, bipeds);
		place_elem_sz = 120u;
	}
	else if (group_tag == 0x6D616368u)
	{
		place_field_off = (uint32_t)offsetof(struct scenario, machines);
		place_elem_sz = 64u;
	}
	else if (group_tag == 0x6C696669u)
	{
		place_field_off = (uint32_t)offsetof(struct scenario, light_fixtures);
		place_elem_sz = 88u;
	}
	else if (group_tag == 0x6374726Cu)
	{
		place_field_off = (uint32_t)offsetof(struct scenario, controls);
		place_elem_sz = 64u;
	}
	else
	{
		place_field_off = (uint32_t)offsetof(struct scenario, scenery);
		place_elem_sz = 72u;
	}

	if (scnr_foff + place_field_off + 12u > out->size)
		return;
	cnt = read_s32(out->data + scnr_foff + place_field_off + 0u);
	addr = read_u32(out->data + scnr_foff + place_field_off + 4u);
	if (cnt <= 0 || addr < CE_TAG_BASE_ADDR)
		return;
	arr_foff = tag_data_offset + (addr - CE_TAG_BASE_ADDR);
	if (arr_foff + (uint32_t)cnt * place_elem_sz > out->size)
		return;

	for (k = 0; k < cnt; k++)
	{
		uint32_t eo = arr_foff + (uint32_t)k * place_elem_sz;
		uint32_t pos_off = is_netgame_eq ? 0x40u : 0x08u;
		float ex, ey, ez, dx, dy, dz, d2;
		if (is_netgame_eq)
		{
			if (read_s32(out->data + eo + 0x5Cu) == -1)
				continue;
		}
		else
		{
			if (read_s16(out->data + eo + 0x00u) < 0)
				continue;
		}
		ex = read_f32(out->data + eo + pos_off + 0u);
		ey = read_f32(out->data + eo + pos_off + 4u);
		ez = read_f32(out->data + eo + pos_off + 8u);
		dx = ex - x;
		dy = ey - y;
		dz = ez - z;
		d2 = dx * dx + dy * dy + dz * dz;
		if (d2 <= best_d2)
		{
			best_d2 = d2;
			best_k = k;
		}
	}

	if (best_k >= 0)
	{
		uint32_t eo = arr_foff + (uint32_t)best_k * place_elem_sz;
		if (is_netgame_eq)
		{
			memset(out->data + eo + 0x50u, 0, 16u);
			write_s32(out->data + eo + 0x5Cu, -1);
			write_f32(out->data + eo + 0x40u, -99999.0f);
			write_f32(out->data + eo + 0x44u, -99999.0f);
			write_f32(out->data + eo + 0x48u, -99999.0f);
		}
		else
		{
			write_s16(out->data + eo + 0x00u, -1);
			write_s16(out->data + eo + 0x02u, -1);
			write_f32(out->data + eo + 0x08u, -99999.0f);
			write_f32(out->data + eo + 0x0Cu, -99999.0f);
			write_f32(out->data + eo + 0x10u, -99999.0f);
		}
	}
}

static void cb_undo_previous_flg1_edits(
	struct forge_byte_buf *out,
	uint32_t tag_data_offset,
	uint32_t scnr_foff,
	char const *prev_state_text)
{
	char const *cur = prev_state_text;
	char line[512];

	if (!out || !out->data || !prev_state_text)
		return;

	while (*cur)
	{
		size_t len = 0;
		while (cur[len] && cur[len] != '\n' && cur[len] != '\r')
			len++;
		if (len > 0 && len < sizeof(line))
		{
			memcpy(line, cur, len);
			line[len] = '\0';
			if (strncmp(line, "DEFAULT_EDIT|", 13) == 0)
			{
				unsigned int grp = 0;
				int del_flag = 0;
				float ox = 0.0f, oy = 0.0f, oz = 0.0f;
				float nx = 0.0f, ny = 0.0f, nz = 0.0f;
				float nyaw = 0.0f, npitch = 0.0f, nroll = 0.0f;
				if (sscanf(line + 13, "%u|%d|%f|%f|%f|%f|%f|%f|%f|%f|%f",
						&grp, &del_flag, &ox, &oy, &oz, &nx, &ny, &nz, &nyaw, &npitch, &nroll) >= 8 &&
					!del_flag)
				{
					struct forge_default_object_edit rev;
					memset(&rev, 0, sizeof(rev));
					rev.group_tag = (uint32_t)grp;
					rev.orig_x = nx;
					rev.orig_y = ny;
					rev.orig_z = nz;
					rev.new_x = ox;
					rev.new_y = oy;
					rev.new_z = oz;
					rev.new_yaw = nyaw;
					rev.new_pitch = npitch;
					rev.new_roll = nroll;
					rev.deleted = 0;
					inplace_apply_default_object_edit(out, tag_data_offset, scnr_foff, &rev);
				}
			}
			else if (strncmp(line, "OBJECT|", 7) == 0)
			{
				char const *p1 = strchr(line + 7, '|');
				char const *p2 = p1 ? strchr(p1 + 1, '|') : NULL;
				if (p1 && p2)
				{
					unsigned int grp = 0;
					int baked_flag = 0;
					float ox = 0.0f, oy = 0.0f, oz = 0.0f;
					sscanf(line + 7, "%u", &grp);
					if (sscanf(p2 + 1, "%d|%f|%f|%f", &baked_flag, &ox, &oy, &oz) == 4 && baked_flag == 1)
					{
						cb_neutralize_baked_object_placement(out, tag_data_offset, scnr_foff, (uint32_t)grp, ox, oy, oz);
					}
				}
			}
		}
		cur += len;
		while (*cur == '\r' || *cur == '\n')
			cur++;
	}
}

int forge_export_scenario_map(
	char const *base_map_path,
	struct forge_scenario_edit_state const *edits,
	char const *output_map_name)
{
	struct forge_source_map base_map;
	struct forge_byte_buf out;
	struct forge_byte_buf extra_verts;
	struct forge_byte_buf extra_strips;
	struct forge_pending_bake_obj *pending_objs = NULL;
	int pending_count = 0;
	char map_path[512];
	uint8_t const *scnr_inst;
	uint32_t scnr_addr, scnr_off, scnr_foff;
	uint32_t orig_vtx_off, orig_vtx_size, orig_model_size, orig_idx_off, orig_idx_size;
	uint32_t tag_data_offset;
	uint32_t orig_tag_end = 0;
	uint8_t *prev_media_buf = NULL;
	uint32_t prev_media_size = 0;
	char *prev_state_text = NULL;
	int32_t i, j;
	FILE *f;

	if (!base_map_path || !output_map_name)
		return 0;

#undef _mkdir
#undef mkdir
	FORGE_MKDIR("d:\\custom_maps");
	FORGE_MKDIR("custom_maps");

	/* Preserve the active map's container format: Xbox v5 maps stay Xbox v5 FLG1 maps,
	 * and Custom Edition v609 maps stay Custom Edition v609 maps. Never substitute a
	 * Custom Edition map when saving an Xbox map. */
	if (!custom_edition_cache_tags_loaded())
	{
		char want_file[128];
		forge_extract_basename(base_map_path, want_file, sizeof(want_file));
		if (!want_file[0]) snprintf(want_file, sizeof(want_file), "bloodgulch.map");
		return forge_build_xbox_flg1_map(want_file, base_map_path, output_map_name, edits);
	}

	if (!forge_source_map_open(base_map_path, &base_map) || base_map.version != CACHE_VERSION_CE)
	{
		static char const *const s_ce_fallback_fmts[] = {
			"C:/Program Files (x86)/Microsoft Games/Halo Custom Edition/maps/%s",
			"C:/Program Files/Microsoft Games/Halo Custom Edition/maps/%s",
			"custom_maps/%s"
		};
		char want_file[128], h_xbox[160], cand_path[512];
		size_t fi;
		int opened_ce = 0;

		forge_extract_basename(base_map_path, want_file, sizeof(want_file));
		if (base_map.data && base_map.version == CACHE_VERSION_XBOX)
		{
			forge_source_map_close(&base_map);
			if (!want_file[0]) snprintf(want_file, sizeof(want_file), "bloodgulch.map");
			return forge_build_xbox_flg1_map(want_file, base_map_path, output_map_name, edits);
		}
		if (base_map.data) forge_source_map_close(&base_map);
		if (!want_file[0]) snprintf(want_file, sizeof(want_file), "bloodgulch.map");

		snprintf(h_xbox, sizeof(h_xbox), "h:\\maps\\%s", want_file);
		platform_translate_path(h_xbox, cand_path, sizeof(cand_path));
		if (forge_source_map_open(cand_path, &base_map) && base_map.version == CACHE_VERSION_CE)
			opened_ce = 1;
		for (fi = 0; !opened_ce && fi < sizeof(s_ce_fallback_fmts) / sizeof(s_ce_fallback_fmts[0]); fi++)
		{
			if (base_map.data) forge_source_map_close(&base_map);
			snprintf(cand_path, sizeof(cand_path), s_ce_fallback_fmts[fi], want_file);
			if (forge_source_map_open(cand_path, &base_map) && base_map.version == CACHE_VERSION_CE)
				opened_ce = 1;
		}
		if (!opened_ce)
		{
			if (base_map.data) forge_source_map_close(&base_map);
			return forge_build_xbox_flg1_map(want_file, base_map_path, output_map_name, edits);
		}
	}

	scnr_inst = src_tag_instance(&base_map, base_map.scenario_tag_index);
	if (!scnr_inst)
	{
		forge_source_map_close(&base_map);
		return 0;
	}
	scnr_addr = read_u32(scnr_inst + 0x14u);
	if (scnr_addr < CE_TAG_BASE_ADDR)
	{
		forge_source_map_close(&base_map);
		return 0;
	}
	scnr_off = scnr_addr - CE_TAG_BASE_ADDR;
	if (scnr_off + sizeof(struct scenario) > base_map.tag_data_size)
	{
		forge_source_map_close(&base_map);
		return 0;
	}

	orig_vtx_off = read_u32(base_map.data + base_map.tag_data_offset + 0x14u);
	orig_vtx_size = read_u32(base_map.data + base_map.tag_data_offset + 0x1Cu);
	orig_model_size = read_u32(base_map.data + base_map.tag_data_offset + 0x20u);
	orig_idx_off = orig_vtx_off + orig_vtx_size;
	orig_idx_size = (orig_model_size >= orig_vtx_size) ? (orig_model_size - orig_vtx_size) : 0u;
	orig_tag_end = base_map.tag_data_offset + base_map.tag_data_size;

	/* If base_map is already an FLG1 v609 map, strip its previous trailer and preserve any deferred raw media */
	if (read_u32(base_map.data + 0x1FCu) == 0x464C4731u)
	{
		uint32_t prev_st_foff = read_u32(base_map.data + 0x1E4u);
		uint32_t prev_st_size = read_u32(base_map.data + 0x1E8u);
		if (prev_st_foff >= orig_tag_end && prev_st_foff <= base_map.file_size)
		{
			if (prev_st_size > 0 && prev_st_size < 262144u && prev_st_foff + prev_st_size <= base_map.file_size)
			{
				prev_state_text = (char *)malloc(prev_st_size + 1u);
				if (prev_state_text)
				{
					char const *bm;
					memcpy(prev_state_text, base_map.data + prev_st_foff, prev_st_size);
					prev_state_text[prev_st_size] = '\0';
					bm = strstr(prev_state_text, "Base Map: ");
					if (bm)
					{
						size_t bi = 0;
						bm += 10;
						while (bm[bi] && bm[bi] != '\r' && bm[bi] != '\n' && bi + 1 < sizeof(base_map.map_name))
						{
							base_map.map_name[bi] = bm[bi];
							bi++;
						}
						base_map.map_name[bi] = '\0';
					}
				}
			}
			if (prev_st_foff > orig_tag_end)
			{
				prev_media_size = prev_st_foff - orig_tag_end;
				prev_media_buf = (uint8_t *)malloc(prev_media_size);
				if (prev_media_buf)
					memcpy(prev_media_buf, base_map.data + orig_tag_end, prev_media_size);
			}
			base_map.file_size = orig_tag_end;
		}
	}

	buf_init(&extra_verts, 65536u);
	buf_init(&extra_strips, 32768u);
	cb_reset_deferred_media();

	/* Pre-scan non-native placed objects and collect their model vertices & triangle strips */
	if (edits && edits->placed_object_count > 0 && orig_vtx_off >= 0x800u && orig_idx_off <= base_map.tag_data_offset)
	{
		pending_objs = (struct forge_pending_bake_obj *)calloc(128, sizeof(struct forge_pending_bake_obj));
		if (pending_objs)
		{
			for (i = 0; i < edits->placed_object_count && pending_count < 128; i++)
			{
				struct forge_placed_object_entry const *po = &edits->placed_objects[i];
				uint32_t g = po->group_tag;
				int already = 0;

				if (!po->tag_path[0] || po->x < -90000.0f)
					continue;
				if (g == 'mach' || g == 'lifi' || g == 'ctrl')
					;
				else if (g == 'scen' || po->category == _forge_object_scenery) g = 'scen';
				else if (g == 'bipd' || po->category == _forge_object_biped) g = 'bipd';
				else if (g == 'vehi' || po->category == _forge_object_vehicle) g = 'vehi';
				else if (g == 'weap' || po->category == _forge_object_weapon) g = 'weap';
				else if (g == 'eqip' || po->category == _forge_object_equipment) g = 'eqip';
				else continue;

				for (j = 0; j < pending_count; j++)
				{
					if (pending_objs[j].group_tag == g && _stricmp(pending_objs[j].tag_path, po->tag_path) == 0)
					{
						already = 1;
						break;
					}
				}
				if (already)
					continue;

				if (src_map_find_tag_or_variant(&base_map, g, po->tag_path) >= 0)
					continue;

				/* 2. Try baking from a Faber Tag Vault bundle (.fbrtag / faber_vault.bin) */
				{
					long fb_prof = -1, fb_tag = -1;
					if (cb_find_faber_vault_bundle_for_tag(g, po->tag_path, &fb_prof, &fb_tag) &&
						fb_prof >= 0 && fb_tag >= 0)
					{
						struct forge_pending_bake_obj *pobj = &pending_objs[pending_count];
						memset(pobj, 0, sizeof(*pobj));
						pobj->group_tag = g;
						snprintf(pobj->tag_path, sizeof(pobj->tag_path), "%s", po->tag_path);
						pobj->faber_profile_idx = fb_prof;
						pobj->faber_tag_idx = fb_tag;
						pobj->baked_tag_idx = -1;
						cb_prescan_faber_vault_models(fb_prof, fb_tag, pobj, &extra_verts, &extra_strips);
						pending_count++;
						continue;
					}
				}

				/* 3. Otherwise locate the tag in an Xbox donor map (campaign, MP map, or .fxtg bundle)
				 * and pre-scan its 'mode' geometry so the full Xbox tag closure is baked into the saved .map! */
				{
					char xbox_path[512];
					if (cb_find_xbox_donor_map_for_tag(g, po->tag_path, xbox_path, sizeof(xbox_path)))
					{
						struct forge_source_map xmap;
						if (forge_source_map_open(xbox_path, &xmap))
						{
							int32_t root_idx = src_map_find_tag_or_variant(&xmap, g, po->tag_path);
							if (root_idx >= 0)
							{
								struct forge_pending_bake_obj *pobj = &pending_objs[pending_count];
								memset(pobj, 0, sizeof(*pobj));
								pobj->group_tag = g;
								snprintf(pobj->tag_path, sizeof(pobj->tag_path), "%s", po->tag_path);
								snprintf(pobj->xbox_map_path, sizeof(pobj->xbox_map_path), "%s", xbox_path);
								pobj->faber_profile_idx = -1;
								pobj->faber_tag_idx = -1;
								pobj->baked_tag_idx = -1;
								cb_prescan_xbox_models(&xmap, root_idx, pobj, &extra_verts, &extra_strips);
								pending_count++;
							}
							forge_source_map_close(&xmap);
						}
					}
				}
			}
		}
	}

	buf_alloc(&extra_verts, 0, 4);
	buf_alloc(&extra_strips, 0, 4);

	/* Assemble out buffer with extra model vertices and triangle strips spliced before tag_data */
	buf_init(&out, base_map.file_size + extra_verts.size + extra_strips.size + 524288u);
	if (!out.data)
	{
		buf_free(&extra_verts);
		buf_free(&extra_strips);
		free(pending_objs);
		free(prev_media_buf);
		free(prev_state_text);
		forge_source_map_close(&base_map);
		return 0;
	}

	if (pending_count > 0 && (extra_verts.size > 0 || extra_strips.size > 0))
	{
		uint32_t pos = 0;
		uint32_t orig_model_end = orig_idx_off + orig_idx_size;
		memcpy(out.data + pos, base_map.data, orig_idx_off);
		pos += orig_idx_off;
		if (extra_verts.size > 0)
		{
			memcpy(out.data + pos, extra_verts.data, extra_verts.size);
			pos += extra_verts.size;
		}
		if (orig_idx_size > 0)
		{
			memcpy(out.data + pos, base_map.data + orig_idx_off, orig_idx_size);
			pos += orig_idx_size;
		}
		if (extra_strips.size > 0)
		{
			memcpy(out.data + pos, extra_strips.data, extra_strips.size);
			pos += extra_strips.size;
		}
		if (base_map.tag_data_offset > orig_model_end)
		{
			uint32_t pad_bytes = base_map.tag_data_offset - orig_model_end;
			memcpy(out.data + pos, base_map.data + orig_model_end, pad_bytes);
			pos += pad_bytes;
		}
		tag_data_offset = pos;
		memcpy(out.data + pos, base_map.data + base_map.tag_data_offset, base_map.file_size - base_map.tag_data_offset);
		pos += (base_map.file_size - base_map.tag_data_offset);
		out.size = pos;

		write_u32(out.data + tag_data_offset + 0x1Cu, orig_vtx_size + extra_verts.size);
		write_u32(out.data + tag_data_offset + 0x20u, orig_model_size + extra_verts.size + extra_strips.size);
	}
	else
	{
		memcpy(out.data, base_map.data, base_map.file_size);
		out.size = base_map.file_size;
		tag_data_offset = base_map.tag_data_offset;
	}

	buf_free(&extra_verts);
	buf_free(&extra_strips);
	scnr_foff = tag_data_offset + scnr_off;

	if (prev_media_buf && prev_media_size > 0)
	{
		uint32_t prev_media_rel = buf_alloc(&s_cb_raw_media, prev_media_size, 32);
		if (s_cb_raw_media.data)
		{
			memcpy(s_cb_raw_media.data + prev_media_rel, prev_media_buf, prev_media_size);
			for (i = 0; i < base_map.tag_count; i++)
			{
				uint32_t io = tag_data_offset + 0x28u + (uint32_t)i * 32u;
				uint32_t gtag, taddr, t_foff;
				if (io + 32u > out.size || read_u32(out.data + io + 0x18u) != 0)
					continue;
				gtag = read_u32(out.data + io + 0x00u);
				taddr = read_u32(out.data + io + 0x14u);
				if (taddr < CE_TAG_BASE_ADDR)
					continue;
				t_foff = tag_data_offset + (taddr - CE_TAG_BASE_ADDR);
				if (gtag == 0x6269746Du && t_foff + 0x6Cu <= out.size)
				{
					int32_t bd_cnt = read_s32(out.data + t_foff + 0x60u);
					uint32_t bd_addr = read_u32(out.data + t_foff + 0x64u);
					if (bd_cnt > 0 && bd_addr >= CE_TAG_BASE_ADDR)
					{
						uint32_t bd_arr = tag_data_offset + (bd_addr - CE_TAG_BASE_ADDR);
						int32_t bi;
						for (bi = 0; bi < bd_cnt && bd_arr + (uint32_t)(bi + 1) * 48u <= out.size; bi++)
						{
							uint32_t bd_foff = bd_arr + (uint32_t)bi * 48u;
							uint16_t bflags = read_u16(out.data + bd_foff + 0x0Eu);
							uint32_t px_off = read_u32(out.data + bd_foff + 0x18u);
							if (!(bflags & (1u << 8)) && px_off >= orig_tag_end && px_off < orig_tag_end + prev_media_size &&
								s_cb_media_patch_count < 4096)
							{
								s_cb_media_patches[s_cb_media_patch_count].field_foff = bd_foff + 0x18u;
								s_cb_media_patches[s_cb_media_patch_count].media_rel_off = prev_media_rel + (px_off - orig_tag_end);
								s_cb_media_patch_count++;
							}
						}
					}
				}
				else if (gtag == 0x736E6421u && t_foff + 0xA4u <= out.size)
				{
					int32_t pr_cnt = read_s32(out.data + t_foff + 0x98u);
					uint32_t pr_addr = read_u32(out.data + t_foff + 0x9Cu);
					if (pr_cnt > 0 && pr_addr >= CE_TAG_BASE_ADDR)
					{
						uint32_t pr_arr = tag_data_offset + (pr_addr - CE_TAG_BASE_ADDR);
						int32_t pri;
						for (pri = 0; pri < pr_cnt && pr_arr + (uint32_t)(pri + 1) * 72u <= out.size; pri++)
						{
							uint32_t pr_foff = pr_arr + (uint32_t)pri * 72u;
							int32_t pm_cnt = read_s32(out.data + pr_foff + 0x3Cu);
							uint32_t pm_addr = read_u32(out.data + pr_foff + 0x40u);
							if (pm_cnt > 0 && pm_addr >= CE_TAG_BASE_ADDR)
							{
								uint32_t pm_arr = tag_data_offset + (pm_addr - CE_TAG_BASE_ADDR);
								int32_t pmi;
								for (pmi = 0; pmi < pm_cnt && pm_arr + (uint32_t)(pmi + 1) * 124u <= out.size; pmi++)
								{
									uint32_t pm_foff = pm_arr + (uint32_t)pmi * 124u;
									uint32_t ext_fl = read_u32(out.data + pm_foff + 0x44u + 4u);
									uint32_t smp_off = read_u32(out.data + pm_foff + 0x44u + 8u);
									if (!(ext_fl & 1u) && smp_off >= orig_tag_end && smp_off < orig_tag_end + prev_media_size &&
										s_cb_media_patch_count < 4096)
									{
										s_cb_media_patches[s_cb_media_patch_count].field_foff = pm_foff + 0x44u + 8u;
										s_cb_media_patches[s_cb_media_patch_count].media_rel_off = prev_media_rel + (smp_off - orig_tag_end);
										s_cb_media_patch_count++;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (prev_media_buf)
	{
		free(prev_media_buf);
		prev_media_buf = NULL;
	}
	if (prev_state_text)
	{
		cb_undo_previous_flg1_edits(&out, tag_data_offset, scnr_foff, prev_state_text);
		free(prev_state_text);
		prev_state_text = NULL;
	}

	/* Relocate existing tag_instance name strings to the end of tag_data
	 * to free contiguous 32-byte tag_instance slots after base_map.tag_count, then bake each object! */
	if (pending_count > 0 || (edits && edits->placed_object_count > 0))
	{
		for (i = 0; i < base_map.tag_count; i++)
		{
			uint32_t io = tag_data_offset + 0x28u + (uint32_t)i * 32u;
			uint32_t naddr = read_u32(out.data + io + 0x10u);
			if (naddr >= CE_TAG_BASE_ADDR && tag_data_offset + (naddr - CE_TAG_BASE_ADDR) < out.size)
			{
				char const *nm = (char const *)(out.data + tag_data_offset + (naddr - CE_TAG_BASE_ADDR));
				size_t nlen = strlen(nm);
				uint32_t new_nfoff = buf_alloc(&out, (uint32_t)nlen + 1u, 1);
				if (new_nfoff && out.data)
				{
					memcpy(out.data + new_nfoff, out.data + tag_data_offset + (naddr - CE_TAG_BASE_ADDR), nlen + 1u);
					write_u32(out.data + io + 0x10u, CE_TAG_BASE_ADDR + (new_nfoff - tag_data_offset));
				}
			}
		}

		for (j = 0; j < pending_count; j++)
		{
			pending_objs[j].baked_tag_idx = out_map_bake_non_native_object(
				&out, tag_data_offset, scnr_off,
				orig_vtx_size, orig_idx_size, &pending_objs[j]);
		}
	}

	if (edits)
	{
		/* 1. Apply edits/deletions to default scenario objects */
		for (i = 0; i < edits->default_edit_count; i++)
		{
			inplace_apply_default_object_edit(&out, tag_data_offset, scnr_foff, &edits->default_edits[i]);
		}

		/* 2. Apply player spawn and netgame flag additions/replacements */
		if (edits->replace_player_spawns)
		{
			uint32_t pl_field = (uint32_t)offsetof(struct scenario, players);
			if (edits->player_spawn_count > 0)
			{
				uint32_t arr_foff = buf_alloc(&out, (uint32_t)edits->player_spawn_count * 52u, 4);
				if (arr_foff > 0 && out.data)
				{
					write_s32(out.data + scnr_foff + pl_field + 0u, edits->player_spawn_count);
					write_u32(out.data + scnr_foff + pl_field + 4u, CE_TAG_BASE_ADDR + (arr_foff - tag_data_offset));
					write_u32(out.data + scnr_foff + pl_field + 8u, 0);
					for (i = 0; i < edits->player_spawn_count; i++)
					{
						struct forge_player_spawn_entry const *sp = &edits->player_spawns[i];
						uint32_t eo = arr_foff + (uint32_t)i * 52u;
						write_f32(out.data + eo + 0x00u, sp->x);
						write_f32(out.data + eo + 0x04u, sp->y);
						write_f32(out.data + eo + 0x08u, sp->z);
						write_f32(out.data + eo + 0x0Cu, sp->yaw);
						write_s16(out.data + eo + 0x10u, sp->team_index);
						write_s16(out.data + eo + 0x12u, sp->bsp_index);
						write_s16(out.data + eo + 0x14u, sp->type0);
						write_s16(out.data + eo + 0x16u, sp->type1);
						write_s16(out.data + eo + 0x18u, sp->type2);
						write_s16(out.data + eo + 0x1Au, sp->type3);
					}
				}
			}
			else
			{
				write_s32(out.data + scnr_foff + pl_field + 0u, 0);
			}
		}
		else
		{
			for (i = 0; i < edits->player_spawn_count; i++)
			{
				struct forge_player_spawn_entry const *sp = &edits->player_spawns[i];
				uint32_t eo = inplace_append_scenario_element(
					&out, tag_data_offset, scnr_foff, (uint32_t)offsetof(struct scenario, players), 52u);
				if (eo > 0 && out.data)
				{
					write_f32(out.data + eo + 0x00u, sp->x);
					write_f32(out.data + eo + 0x04u, sp->y);
					write_f32(out.data + eo + 0x08u, sp->z);
					write_f32(out.data + eo + 0x0Cu, sp->yaw);
					write_s16(out.data + eo + 0x10u, sp->team_index);
					write_s16(out.data + eo + 0x12u, sp->bsp_index);
					write_s16(out.data + eo + 0x14u, sp->type0 ? sp->type0 : 12);
					write_s16(out.data + eo + 0x16u, sp->type1);
					write_s16(out.data + eo + 0x18u, sp->type2);
					write_s16(out.data + eo + 0x1Au, sp->type3);
				}
			}
		}

		if (edits->replace_netgame_flags)
		{
			uint32_t nf_field = (uint32_t)offsetof(struct scenario, netgame_flags);
			if (edits->netgame_flag_count > 0)
			{
				uint32_t arr_foff = buf_alloc(&out, (uint32_t)edits->netgame_flag_count * 148u, 4);
				if (arr_foff > 0 && out.data)
				{
					write_s32(out.data + scnr_foff + nf_field + 0u, edits->netgame_flag_count);
					write_u32(out.data + scnr_foff + nf_field + 4u, CE_TAG_BASE_ADDR + (arr_foff - tag_data_offset));
					write_u32(out.data + scnr_foff + nf_field + 8u, 0);
					for (i = 0; i < edits->netgame_flag_count; i++)
					{
						struct forge_netgame_flag_entry const *nf = &edits->netgame_flags[i];
						uint32_t eo = arr_foff + (uint32_t)i * 148u;
						write_f32(out.data + eo + 0x00u, nf->x);
						write_f32(out.data + eo + 0x04u, nf->y);
						write_f32(out.data + eo + 0x08u, nf->z);
						write_f32(out.data + eo + 0x0Cu, nf->yaw);
						write_s16(out.data + eo + 0x10u, nf->type);
						write_s16(out.data + eo + 0x12u, nf->usage_id);
						write_s32(out.data + eo + 0x1Cu, -1);
					}
				}
			}
			else
			{
				write_s32(out.data + scnr_foff + nf_field + 0u, 0);
			}
		}
		else
		{
			for (i = 0; i < edits->netgame_flag_count; i++)
			{
				struct forge_netgame_flag_entry const *nf = &edits->netgame_flags[i];
				uint32_t eo = inplace_append_scenario_element(
					&out, tag_data_offset, scnr_foff, (uint32_t)offsetof(struct scenario, netgame_flags), 148u);
				if (eo > 0 && out.data)
				{
					write_f32(out.data + eo + 0x00u, nf->x);
					write_f32(out.data + eo + 0x04u, nf->y);
					write_f32(out.data + eo + 0x08u, nf->z);
					write_f32(out.data + eo + 0x0Cu, nf->yaw);
					write_s16(out.data + eo + 0x10u, nf->type);
					write_s16(out.data + eo + 0x12u, nf->usage_id);
					write_s32(out.data + eo + 0x1Cu, -1);
				}
			}
		}

		/* 3. Append all placed objects (both native base_map tags and newly baked non-native tags) */
		for (i = 0; i < edits->placed_object_count; i++)
		{
			struct forge_placed_object_entry const *po = &edits->placed_objects[i];
			uint32_t g = po->group_tag;
			uint32_t pal_off = 0, blk_off = 0, elem_sz = 0;
			int32_t tag_idx;

			if (!po->active && po->tag_path[0] == 0)
				continue;
			if (po->active == 0 && (po->x < -90000.0f))
				continue;

			if (g == 'mach')
			{
				blk_off = (uint32_t)offsetof(struct scenario, machines);
				pal_off = (uint32_t)offsetof(struct scenario, machine_palette);
				elem_sz = 64u;
			}
			else if (g == 'lifi')
			{
				blk_off = (uint32_t)offsetof(struct scenario, light_fixtures);
				pal_off = (uint32_t)offsetof(struct scenario, light_fixtures_palette);
				elem_sz = 88u;
			}
			else if (g == 'ctrl')
			{
				blk_off = (uint32_t)offsetof(struct scenario, controls);
				pal_off = (uint32_t)offsetof(struct scenario, control_palette);
				elem_sz = 64u;
			}
			else if (g == 'scen' || po->category == _forge_object_scenery)
			{
				g = 'scen'; blk_off = (uint32_t)offsetof(struct scenario, scenery); pal_off = (uint32_t)offsetof(struct scenario, scenery_palette); elem_sz = 72u;
			}
			else if (g == 'bipd' || po->category == _forge_object_biped)
			{
				g = 'bipd'; blk_off = (uint32_t)offsetof(struct scenario, bipeds); pal_off = (uint32_t)offsetof(struct scenario, biped_palette); elem_sz = 120u;
			}
			else if (g == 'vehi' || po->category == _forge_object_vehicle)
			{
				g = 'vehi'; blk_off = (uint32_t)offsetof(struct scenario, vehicles); pal_off = (uint32_t)offsetof(struct scenario, vehicle_palette); elem_sz = 120u;
			}
			else if (g == 'weap' || po->category == _forge_object_weapon)
			{
				g = 'weap'; blk_off = (uint32_t)offsetof(struct scenario, weapons); pal_off = (uint32_t)offsetof(struct scenario, weapon_palette); elem_sz = 92u;
			}
			else if (g == 'eqip' || po->category == _forge_object_equipment)
			{
				g = 'eqip'; blk_off = (uint32_t)offsetof(struct scenario, equipment); pal_off = (uint32_t)offsetof(struct scenario, equipment_palette); elem_sz = 40u;
			}

			if (elem_sz > 0)
			{
				tag_idx = -1;
				for (j = 0; j < pending_count; j++)
				{
					if (pending_objs[j].group_tag == g && _stricmp(pending_objs[j].tag_path, po->tag_path) == 0)
					{
						tag_idx = pending_objs[j].baked_tag_idx;
						break;
					}
				}
				if (tag_idx < 0)
					tag_idx = src_map_find_tag_or_variant(&base_map, g, po->tag_path);
				if (tag_idx >= 0)
				{
					int16_t pal;
					if (g == 'weap' || g == 'eqip')
					{
						uint32_t itmc_handle = 0xFFFFFFFFu, itmc_name_addr = 0u;
						int32_t itmc_idx = out_map_create_item_collection_for_tag(
							&out, tag_data_offset, scnr_off, g, tag_idx, i,
							&itmc_handle, &itmc_name_addr);
						if (itmc_idx >= 0 && itmc_handle != 0xFFFFFFFFu)
						{
							uint32_t ne_eo = inplace_append_scenario_element(
								&out, tag_data_offset, scnr_foff,
								(uint32_t)offsetof(struct scenario, netgame_equipment), 144u);
							if (ne_eo > 0 && out.data)
							{
								memset(out.data + ne_eo, 0, 144u);
								write_u32(out.data + ne_eo + 0x00u, 0u);
								write_s16(out.data + ne_eo + 0x04u, 12); /* _game_engine_all */
								write_s16(out.data + ne_eo + 0x06u, 0);
								write_s16(out.data + ne_eo + 0x08u, 0);
								write_s16(out.data + ne_eo + 0x0Au, 0);
								write_s16(out.data + ne_eo + 0x0Cu, po->team_index);
								write_s16(out.data + ne_eo + 0x0Eu, 30); /* spawn_time = 30s */
								write_f32(out.data + ne_eo + 0x40u, po->x);
								write_f32(out.data + ne_eo + 0x44u, po->y);
								write_f32(out.data + ne_eo + 0x48u, po->z);
								write_f32(out.data + ne_eo + 0x4Cu, po->yaw);
								write_u32(out.data + ne_eo + 0x50u, 'itmc');
								write_u32(out.data + ne_eo + 0x54u, itmc_name_addr);
								write_s32(out.data + ne_eo + 0x58u, 0);
								write_u32(out.data + ne_eo + 0x5Cu, itmc_handle);
								continue;
							}
						}
					}
					if (g == 'vehi')
						tag_idx = out_map_clone_vehicle_tag_for_mp(&out, tag_data_offset, scnr_off, tag_idx, i);
					pal = inplace_ensure_scenario_palette_entry(
						&out, &base_map, tag_data_offset, scnr_foff, pal_off, 48u, tag_idx);
					if (pal >= 0)
					{
						uint32_t eo = inplace_append_scenario_element(
							&out, tag_data_offset, scnr_foff, blk_off, elem_sz);
						if (eo > 0 && out.data)
						{
							write_s16(out.data + eo + 0x00u, pal);
							write_s16(out.data + eo + 0x02u, -1);
							write_u16(out.data + eo + 0x04u, po->placement_flags);
							write_s16(out.data + eo + 0x06u, 0);
							write_f32(out.data + eo + 0x08u, po->x);
							write_f32(out.data + eo + 0x0Cu, po->y);
							write_f32(out.data + eo + 0x10u, po->z);
							write_f32(out.data + eo + 0x14u, po->yaw);
							write_f32(out.data + eo + 0x18u, po->pitch);
							write_f32(out.data + eo + 0x1Cu, po->roll);
							if (g == 'vehi')
							{
								write_u16(out.data + eo + 0x5Au, 0x3F3Fu);
							}
							else if (g == 'weap')
							{
								write_s16(out.data + eo + 0x28u, 9999);
								write_s16(out.data + eo + 0x2Au, 9999);
							}
							else if (g == 'mach' || g == 'ctrl')
							{
								write_s16(out.data + eo + 0x28u, -1);
								write_s16(out.data + eo + 0x2Au, -1);
							}
							else if (g == 'lifi')
							{
								write_s16(out.data + eo + 0x28u, -1);
								write_s16(out.data + eo + 0x2Au, -1);
								write_f32(out.data + eo + 0x40u, 1.0f);
								write_f32(out.data + eo + 0x44u, 1.0f);
								write_f32(out.data + eo + 0x48u, 1.0f);
								write_f32(out.data + eo + 0x4Cu, 1.0f);
								write_f32(out.data + eo + 0x50u, 1.0f);
								write_f32(out.data + eo + 0x54u, 1.2f);
							}
						}
					}
				}
			}
		}
	}

	/* Build internal [OpenCE Forge Map] + [ForgeState] text, menu description text, and 140x114 BMP preview in memory,
	 * and embed them inside custom_maps/<output_map_name>.map (recording their offsets at 0x1E4..0x1F8 in the header)
	 * so no external .bmp or .txt files are ever created! */
	{
		uint32_t tag_data_end_foff = buf_alloc(&out, 0, 32);

		/* Append deferred raw bitmap pixels and sound samples AFTER tag_data_end_foff so
		 * tag_data_size never exceeds lowest_structure_bsp_address - CE_TAG_BASE_ADDR */
		if (s_cb_raw_media.data && s_cb_raw_media.size > 0)
		{
			uint32_t media_base_foff = buf_alloc(&out, s_cb_raw_media.size, 32);
			if (media_base_foff && out.data)
			{
				int mp_i;
				memcpy(out.data + media_base_foff, s_cb_raw_media.data, s_cb_raw_media.size);
				for (mp_i = 0; mp_i < s_cb_media_patch_count; mp_i++)
				{
					uint32_t f_foff = s_cb_media_patches[mp_i].field_foff;
					if (f_foff + 4u <= out.size)
						write_u32(out.data + f_foff, media_base_foff + s_cb_media_patches[mp_i].media_rel_off);
				}
			}
		}
		buf_free(&s_cb_raw_media);
		s_cb_media_patch_count = 0;

		if (!append_forge_map_metadata_and_preview_trailer(
				&out,
				output_map_name,
				base_map.map_name,
				base_map_path,
				edits,
				&base_map,
				pending_objs,
				pending_count))
		{
			free(pending_objs);
			forge_source_map_close(&base_map);
			return 0;
		}

		write_s32(out.data + 0x04u, CACHE_VERSION_CE);
		write_u32(out.data + 0x08u, out.size);
		write_u32(out.data + 0x0Cu, 0);
		write_u32(out.data + 0x10u, tag_data_offset);
		write_u32(out.data + 0x14u, tag_data_end_foff - tag_data_offset);
		write_s16(out.data + 0x60u, 1); /* _scenario_type_multiplayer */
		if (scnr_foff + 4u <= out.size)
			write_s16(out.data + scnr_foff + 0x00u, 1);
		write_u32(out.data + 0x7FCu, CACHE_FOOTER_SIGNATURE);

		/* Update Custom Edition CRC-32 checksum (+0x64) across BSPs + model data + tag data */
		write_u32(out.data + 0x64u, compute_ce_map_checksum(out.data, out.size, tag_data_offset, scnr_foff));

		/* Verify baked Custom Edition cache container via OpenCE's built-in cache_file_identify */
		{
			struct cache_file_source mem_src;
			struct cache_file_identity verify_id;
			mem_src.context = &out;
			mem_src.read = forge_mem_source_read;
			mem_src.size = out.size;
			memset(&verify_id, 0, sizeof(verify_id));
			if (cache_file_identify(&mem_src, &verify_id) != _cache_file_status_ok)
			{
				free(pending_objs);
				buf_free(&out);
				forge_source_map_close(&base_map);
				return 0;
			}
		}
	}

	free(pending_objs);

	{
		char d_dir[512];
		char d_map[160];
		char tmp_map_path[520];
		d_dir[0] = '\0';
		platform_translate_path("d:\\custom_maps", d_dir, sizeof(d_dir));
		if (d_dir[0])
			FORGE_MKDIR(d_dir);
		FORGE_MKDIR("custom_maps");
		snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.map", output_map_name);
		map_path[0] = '\0';
		platform_translate_path(d_map, map_path, sizeof(map_path));
		if (!map_path[0])
			snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", output_map_name);
		snprintf(tmp_map_path, sizeof(tmp_map_path), "%s.tmp", map_path);
		f = fopen(tmp_map_path, "wb");
		if (!f)
		{
			snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", output_map_name);
			snprintf(tmp_map_path, sizeof(tmp_map_path), "%s.tmp", map_path);
			f = fopen(tmp_map_path, "wb");
		}
		if (!f)
		{
			buf_free(&out);
			forge_source_map_close(&base_map);
			return 0;
		}
		if (fwrite(out.data, 1, out.size, f) != out.size)
		{
			fclose(f);
			remove(tmp_map_path);
			buf_free(&out);
			forge_source_map_close(&base_map);
			return 0;
		}
		fclose(f);
		if (forge_host_rename(tmp_map_path, map_path) != 0)
		{
			remove(tmp_map_path);
			buf_free(&out);
			forge_source_map_close(&base_map);
			return 0;
		}
	}
	/* Remove any legacy external .bmp / .txt / sidecar files for this map once the new .map is safely in place */
	{
		char d_txt[160];
		char legacy_path[512];
		snprintf(d_txt, sizeof(d_txt), "d:\\custom_maps\\%s.txt", output_map_name);
		legacy_path[0] = '\0';
		platform_translate_path(d_txt, legacy_path, sizeof(legacy_path));
		if (legacy_path[0])
			remove(legacy_path);
		snprintf(legacy_path, sizeof(legacy_path), "custom_maps/%s.bmp", output_map_name);
		remove(legacy_path);
		snprintf(legacy_path, sizeof(legacy_path), "custom_maps/%s.txt", output_map_name);
		remove(legacy_path);
	}

	buf_free(&out);
	forge_source_map_close(&base_map);
	return 1;
}
