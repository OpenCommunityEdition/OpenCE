/*
 * OpenCE : Forge
 * Runtime Tag Catalog, Cached Tag Folder System (OpenCE Forge/cached_tags/),
 * Dynamic Runtime Tag Table Expansion, Multiplayer Asset Guard, Custom Tag Extraction,
 * Standalone Map Baking, Dynamic Network Version & 343 Guilty Spark Hardware Model Renderer.
 */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"
#include "halo_port_limits.h" /* HALO_PORT_NETWORK_VERSION: OpenCE's protocol for standard sessions */
#include "game/game.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "objects/object_lights.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "models/models.h"
#include "models/model_definitions.h"
#include "physics/physics_definitions.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "math/real_math.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "game/players.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_models.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "render/render.h"
#include "shaders/shader_definitions.h"
#include "shaders/shaders.h"
#include "sound/sound_definitions.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "cache/cache_files.h"
#include "custom_edition_cache.h"
#include "tag_schema.h"
#include "tag_bundle.h"
#include "tag_names.h"
#include "tag_preview.h"
#include "../src/port_tag_profiles.h"
extern int compress2(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen, int level);
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"
#include "../../third_party/zlib/zlib_prefixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "forge_engine.h"

#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef remove
#undef malloc
#undef calloc
#undef free
#undef realloc

extern unsigned long system_milliseconds(void);
extern void rasterizer_set_frustum_z(real z_near, real z_far);
extern void *cache_files_tag_instances(long *count);
extern void cache_files_set_tag_instances(void *instances, long count);
extern void progress_bar_begin(boolean skip_frame_capture);
extern void progress_bar_display(real progress);
extern void progress_bar_end(void);
extern void progress_bar_eachframe(void);
extern void shell_idle(void);
extern void sound_render(void);
extern void main_present_frame(void);
extern long halo_screen_width(void);
extern void platform_translate_path(const char *xbox_path, char *host_path, unsigned long host_path_size);
extern char *main_get_multiplayer_map_name(void);
extern boolean game_engine_has_teams(void);

static int forge_float_is_finite(float x)
{
	return (x == x && x > -3.4e38f && x < 3.4e38f) ? 1 : 0;
}

static void forge_resolve_host_path(char const *path, char *out_host_path, size_t out_sz);

void *forge_host_fopen(char const *path, char const *mode)
{
	char host_path[1024];
	FILE *f;
	if (!path || !path[0] || !mode)
		return NULL;
	host_path[0] = '\0';
	forge_resolve_host_path(path, host_path, sizeof(host_path));
	f = host_path[0] ? fopen(host_path, mode) : NULL;
	return f ? f : fopen(path, mode);
}

int forge_host_remove(char const *path)
{
	char host_path[1024];
	if (!path || !path[0])
		return -1;
	host_path[0] = '\0';
	forge_resolve_host_path(path, host_path, sizeof(host_path));
	if (host_path[0] && remove(host_path) == 0)
		return 0;
	return remove(path);
}

static void forge_resolve_host_path(char const *path, char *out_host_path, size_t out_sz)
{
	char norm[1024];
	char d_xbox[1024];
	size_t i;

	if (!path || !path[0] || !out_host_path || out_sz == 0)
		return;

	for (i = 0; path[i] && i + 1 < sizeof(norm); i++)
		norm[i] = (path[i] == '\\') ? '/' : path[i];
	norm[i] = '\0';

	if ((norm[0] == 'd' || norm[0] == 'D' ||
		 norm[0] == 'h' || norm[0] == 'H' ||
		 norm[0] == 'z' || norm[0] == 'Z' ||
		 norm[0] == 't' || norm[0] == 'T' ||
		 norm[0] == 'u' || norm[0] == 'U') &&
		norm[1] == ':' && norm[2] == '/')
	{
		platform_translate_path(path, out_host_path, (unsigned long)out_sz);
		return;
	}

	if (norm[0] == '/' ||
		(((norm[0] >= 'A' && norm[0] <= 'Z') || (norm[0] >= 'a' && norm[0] <= 'z')) && norm[1] == ':'))
	{
		snprintf(out_host_path, out_sz, "%s", norm);
		return;
	}

	snprintf(d_xbox, sizeof(d_xbox), "d:\\%s", norm);
	platform_translate_path(d_xbox, out_host_path, (unsigned long)out_sz);
}

int forge_host_rename(char const *old_path, char const *new_path)
{
	char old_host[1024];
	char new_host[1024];

	if (!old_path || !old_path[0] || !new_path || !new_path[0])
		return -1;

	old_host[0] = '\0';
	new_host[0] = '\0';
	forge_resolve_host_path(old_path, old_host, sizeof(old_host));
	forge_resolve_host_path(new_path, new_host, sizeof(new_host));

#ifdef _WIN32
	remove(new_host);
#endif
	if (rename(old_host, new_host) == 0)
		return 0;

#ifdef _WIN32
	remove(new_path);
#endif
	return rename(old_path, new_path);
}

char const *forge_get_active_map_name(void)
{
	static char s_active_map[256];
	struct scenario *scen = global_scenario_try_and_get();
	char const *ce_loaded;
	char const *loaded_leaf;
	char const *mp_map;
	char const *sp_map;

	if (!scen || scen->type == 2 /* _scenario_type_main_menu */)
		return "levels\\ui\\ui";

	{
		extern boolean mcc_cache_tags_loaded(void);
		extern char const *mcc_cache_loaded_level_name(void);
		if (mcc_cache_tags_loaded())
		{
			char const *mcc_level = mcc_cache_loaded_level_name();
			if (mcc_level && mcc_level[0])
				return mcc_level;
		}
	}

	ce_loaded = custom_edition_cache_tags_loaded() ? custom_edition_cache_loaded_path() : NULL;
	if (ce_loaded && ce_loaded[0])
	{
		char const *slash = strrchr(ce_loaded, '\\');
		char const *slash2 = strrchr(ce_loaded, '/');
		char const *stem;
		char clean_stem[128];
		char *dot;
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		stem = slash ? (slash + 1) : ce_loaded;
		snprintf(clean_stem, sizeof(clean_stem), "%s", stem);
		dot = strrchr(clean_stem, '.');
		if (dot && _stricmp(dot, ".map") == 0)
			*dot = '\0';
		if (clean_stem[0])
		{
			snprintf(s_active_map, sizeof(s_active_map), "custom_maps\\%s", clean_stem);
			return s_active_map;
		}
	}

	loaded_leaf = cache_file_loaded_map_name();
	mp_map = main_get_multiplayer_map_name();
	sp_map = main_get_map_name();

	if (loaded_leaf && loaded_leaf[0] && _stricmp(loaded_leaf, "ui") != 0)
	{
		if (mp_map && mp_map[0])
		{
			char const *ms = strrchr(mp_map, '\\');
			char const *ms2 = strrchr(mp_map, '/');
			char const *mleaf;
			if (ms2 && (!ms || ms2 > ms))
				ms = ms2;
			mleaf = ms ? (ms + 1) : mp_map;
			if (_stricmp(mleaf, loaded_leaf) == 0)
				return mp_map;
		}
		if (sp_map && sp_map[0])
		{
			char const *ss = strrchr(sp_map, '\\');
			char const *ss2 = strrchr(sp_map, '/');
			char const *sleaf;
			if (ss2 && (!ss || ss2 > ss))
				ss = ss2;
			sleaf = ss ? (ss + 1) : sp_map;
			if (_stricmp(sleaf, loaded_leaf) == 0)
				return sp_map;
		}
		return loaded_leaf;
	}

	if (scen->type == 1 /* _scenario_type_multiplayer */ && mp_map && mp_map[0])
		return mp_map;

	return (sp_map && sp_map[0]) ? sp_map : "levels\\ui\\ui";
}

void forge_snap_to_scenario_placement_origin(
	uint32_t group_tag,
	char const *tag_path,
	float *io_x,
	float *io_y,
	float *io_z)
{
	struct scenario *scen;
	short object_type;
	float best_d2 = 0.0625f; /* Within 0.25 world units */
	float best_x = 0.0f, best_y = 0.0f, best_z = 0.0f;
	int found = 0;

	if (!tag_path || !tag_path[0] || !io_x || !io_y || !io_z)
		return;
	scen = global_scenario_try_and_get();
	if (!scen)
		return;

	for (object_type = 0; object_type < NUMBER_OF_OBJECT_TYPES; object_type++)
	{
		struct object_type_definition *def = object_type_definition_get(object_type);
		struct tag_block *datums;
		struct tag_block *palette;
		long elem_size = 0;
		long i;

		if (!def || def->placement_tag_block_offset == NONE || def->palette_tag_block_offset == NONE)
			continue;
		if ((uint32_t)def->group_tag != group_tag)
			continue;

		datums = scenario_get_object_type_scenario_datums(scen, object_type, &elem_size);
		palette = scenario_get_object_type_scenario_palette(scen, object_type);
		if (!datums || !palette || datums->count <= 0 || palette->count <= 0 ||
			elem_size < (long)sizeof(struct scenario_object_datum))
			continue;

		for (i = 0; i < datums->count; i++)
		{
			struct scenario_object_datum const *sobj =
				(struct scenario_object_datum const *)tag_block_get_element_fast(datums, i, elem_size);
			struct scenario_object_palette_entry const *pal_entry;
			long ref_idx;
			char const *ref_name;
			float dx, dy, dz, d2;

			if (!sobj || sobj->palette_entry_index < 0 || sobj->palette_entry_index >= palette->count)
				continue;
			pal_entry = TAG_BLOCK_GET_ELEMENT(
				palette, sobj->palette_entry_index, struct scenario_object_palette_entry);
			if (!pal_entry)
				continue;
			ref_idx = pal_entry->reference.index;
			if (ref_idx == NONE || (uint32_t)tag_get_group_tag(ref_idx) != group_tag)
				continue;
			ref_name = tag_get_name(ref_idx);
			if (!ref_name || _stricmp(ref_name, tag_path) != 0)
				continue;

			dx = sobj->position.x - *io_x;
			dy = sobj->position.y - *io_y;
			dz = sobj->position.z - *io_z;
			d2 = dx * dx + dy * dy + dz * dz;
			if (d2 < best_d2)
			{
				best_d2 = d2;
				best_x = sobj->position.x;
				best_y = sobj->position.y;
				best_z = sobj->position.z;
				found = 1;
			}
		}
	}

	if (found)
	{
		*io_x = best_x;
		*io_y = best_y;
		*io_z = best_z;
	}
}

#define fopen(p, m) ((FILE *)forge_host_fopen((p), (m)))
#define remove(p) forge_host_remove((p))
extern long bitmap_format_to_d3d_format(short format, word flags);
extern long bitmap_format_to_d3d_linear_format(short format, word flags);
extern void models_fix_transparent_part_links(void);
extern boolean custom_edition_cache_tags_loaded(void);
extern boolean mcc_cache_tags_loaded(void);
extern char const *mcc_cache_loaded_level_name(void);
extern boolean mcc_package_map_path(char const *level_name, char *out_path);
struct mcc_runtime;
extern struct mcc_runtime *mcc_forge_browser_open(FILE *map_file, char const *map_path, void *tag_buf, unsigned long tag_size);
extern void mcc_forge_browser_close(struct mcc_runtime *rt);
extern boolean mcc_forge_convert_single_bitmap(
	struct mcc_runtime *rt,
	char const *tag_name,
	unsigned int bitmap_index,
	unsigned int channel_mode,
	struct bitmap_data *inout_bmp,
	unsigned char **out_pixels,
	unsigned int *out_size);
extern boolean mcc_forge_convert_single_permutation(
	struct mcc_runtime *rt,
	char const *tag_name,
	int sound_channels,
	unsigned int sample_rate,
	unsigned int sound_rate,
	unsigned int pitch_range_index,
	unsigned int permutation_index,
	struct sound_permutation *inout_perm,
	unsigned char **out_samples,
	unsigned int *out_size);
extern long port_tag_profiles_scan(void);
extern long port_tag_profiles_count(void);
extern long port_tag_profiles_find(const char *folder);
extern long port_tag_profiles_tag_count(long profile);
extern unsigned long port_tag_profiles_tag_group(long profile, long tag);
extern const char *port_tag_profiles_tag_name(long profile, long tag);
extern int port_tag_profiles_tag_enabled(long profile, long tag);
extern const char *port_tag_profiles_tag_option(long profile, long tag, const char *key);
extern void halo_custom_edition_texels_channels(void const *address, unsigned char order);
extern boolean orig_mod_sounds_read(struct sound_permutation const *permutation, void *buffer);
extern void orig_render_model(
	long model_index,
	real level_of_detail_pixels,
	struct real_matrix4x3 const *node_matrices,
	char const *region_permutation_indices,
	union real_rgb_color const *change_colors,
	real const *function_values,
	struct render_lighting const *lighting,
	union real_point3d const *centroid,
	real radius,
	struct render_model_effect const *model_effect,
	long unique_identifier,
	short forced_shader_permutation_index,
	unsigned long flags);

struct forge_progress_bar_mode_mirror
{
	long state;
	boolean capture_frame;
	unsigned char padding0[3];
	IDirect3DTexture8 *texture0;
	IDirect3DTexture8 *texture1;
	boolean active;
	unsigned char padding1[3];
};
extern struct forge_progress_bar_mode_mirror progress_bar_mode;
extern real last_t;

static int s_forge_loading_active = 0;
static float s_forge_loading_map_base = 0.0f;
static float s_forge_loading_map_span = 0.0f;
static unsigned long s_forge_loading_last_ms = 0;

static uint8_t sample_dxt5_halo_logo_lum(uint8_t const *dxt5_raw, int sx, int sy)
{
	int bx = sx >> 2;
	int by = sy >> 2;
	int p_idx = ((sy & 3) << 2) | (sx & 3);
	size_t bidx = ((size_t)by * 256u + (size_t)bx) * 16u;
	uint8_t a0 = dxt5_raw[bidx + 0];
	uint8_t a1 = dxt5_raw[bidx + 1];
	uint64_t abits =
		(uint64_t)dxt5_raw[bidx + 2] |
		((uint64_t)dxt5_raw[bidx + 3] << 8) |
		((uint64_t)dxt5_raw[bidx + 4] << 16) |
		((uint64_t)dxt5_raw[bidx + 5] << 24) |
		((uint64_t)dxt5_raw[bidx + 6] << 32) |
		((uint64_t)dxt5_raw[bidx + 7] << 40);
	uint32_t acode = (uint32_t)((abits >> (p_idx * 3)) & 7u);
	uint32_t alpha;
	uint16_t c0 = *(uint16_t const *)(dxt5_raw + bidx + 8);
	uint16_t c1 = *(uint16_t const *)(dxt5_raw + bidx + 10);
	uint32_t cbits = *(uint32_t const *)(dxt5_raw + bidx + 12);
	uint32_t ccode = (cbits >> (p_idx * 2)) & 3u;
	uint32_t r0 = ((c0 >> 11) & 31u) * 255u / 31u;
	uint32_t g0 = ((c0 >> 5) & 63u) * 255u / 63u;
	uint32_t b0 = (c0 & 31u) * 255u / 31u;
	uint32_t r1 = ((c1 >> 11) & 31u) * 255u / 31u;
	uint32_t g1 = ((c1 >> 5) & 63u) * 255u / 63u;
	uint32_t b1 = (c1 & 31u) * 255u / 31u;
	uint32_t l0 = r0 > g0 ? (r0 > b0 ? r0 : b0) : (g0 > b0 ? g0 : b0);
	uint32_t l1 = r1 > g1 ? (r1 > b1 ? r1 : b1) : (g1 > b1 ? g1 : b1);
	uint32_t lum;

	if (acode == 0) alpha = a0;
	else if (acode == 1) alpha = a1;
	else if (a0 > a1) alpha = ((8u - acode) * a0 + (acode - 1u) * a1) / 7u;
	else if (acode < 6u) alpha = ((6u - acode) * a0 + (acode - 1u) * a1) / 5u;
	else if (acode == 6u) alpha = 0u;
	else alpha = 255u;

	if (ccode == 0u) lum = l0;
	else if (ccode == 1u) lum = l1;
	else if (ccode == 2u) lum = (2u * l0 + l1) / 3u;
	else lum = (l0 + 2u * l1) / 3u;

	return (uint8_t)((lum * alpha) / 255u);
}

static void forge_ensure_xbox_loading_tga(void)
{
	char z_tga_host[1024];
	FILE *zf;
	FILE *mf;
	uint8_t *tga_buf;
	uint8_t *dxt5_raw = NULL;
	int have_ui_logo = 0;
	int x, y, slot;

	platform_translate_path("z:\\maps\\loading.tga", z_tga_host, sizeof(z_tga_host));
	{
		char z_maps_dir[1024];
		platform_translate_path("z:\\maps", z_maps_dir, sizeof(z_maps_dir));
		CreateDirectoryA(z_maps_dir, NULL);
	}

	zf = fopen(z_tga_host, "rb");
	if (zf)
	{
		long zsz = 0;
		fseek(zf, 0, SEEK_END);
		zsz = ftell(zf);
		if (zsz == 230418L)
		{
			uint8_t *check_buf = (uint8_t *)malloc(230418u);
			int has_logo_brightness = 0;
			if (check_buf)
			{
				fseek(zf, 0, SEEK_SET);
				if (fread(check_buf, 1, 230418u, zf) == 230418u)
				{
					for (y = 75; y < 165 && !has_logo_brightness; y++)
					{
						for (x = 40; x < 280; x++)
						{
							uint8_t const *px = check_buf + 18u + ((size_t)(239 - y) * 320u + (size_t)x) * 3u;
							if (px[0] > 140 || px[1] > 140 || px[2] > 140)
							{
								has_logo_brightness = 1;
								break;
							}
						}
					}
				}
				free(check_buf);
			}
			fclose(zf);
			if (has_logo_brightness)
				return;
		}
		else
		{
			fclose(zf);
		}
	}

	mf = fopen("maps/loading.tga", "rb");
	if (mf)
	{
		long msz = 0;
		fseek(mf, 0, SEEK_END);
		msz = ftell(mf);
		if (msz == 230418L)
		{
			tga_buf = (uint8_t *)malloc(230418u);
			if (tga_buf)
			{
				fseek(mf, 0, SEEK_SET);
				if (fread(tga_buf, 1, 230418u, mf) == 230418u)
				{
					FILE *out_z = fopen(z_tga_host, "wb");
					if (out_z)
					{
						fwrite(tga_buf, 1, 230418u, out_z);
						fclose(out_z);
					}
					free(tga_buf);
					fclose(mf);
					return;
				}
				free(tga_buf);
			}
		}
		fclose(mf);
	}

	/* Extract the Halo ring + logo from the player's own Xbox ui.map (ui\shell\main_menu\halo_logo) */
	{
		long logo_tag = tag_loaded('bitm', "ui\\shell\\main_menu\\halo_logo");
		long pix_off = 0x590E00L;
		if (logo_tag != NONE)
		{
			struct bitmap_group *bgrp = (struct bitmap_group *)tag_get('bitm', logo_tag);
			if (bgrp && bgrp->bitmaps.count > 0 && bgrp->bitmaps.address)
			{
				struct bitmap_data *bmp = (struct bitmap_data *)bgrp->bitmaps.address;
				if (bmp->pixels_offset > 2048L && bmp->pixels_size >= 262144L)
					pix_off = bmp->pixels_offset;
			}
		}

		for (slot = 0; slot < 6 && !have_ui_logo; slot++)
		{
			char z_xbox[64];
			char z_cache[1024];
			FILE *cf;
			snprintf(z_xbox, sizeof(z_xbox), "z:\\cache00%d.map", slot);
			platform_translate_path(z_xbox, z_cache, sizeof(z_cache));
			cf = fopen(z_cache, "rb");
			if (cf)
			{
				uint8_t chdr[2048];
				if (fread(chdr, 1, sizeof(chdr), cf) == sizeof(chdr) &&
					*(uint32_t const *)(chdr + 0x00) == 0x68656164u &&
					_stricmp((char const *)(chdr + 0x20), "ui") == 0)
				{
					dxt5_raw = (uint8_t *)malloc(262144u);
					if (dxt5_raw && fseek(cf, pix_off, SEEK_SET) == 0 &&
						fread(dxt5_raw, 1, 262144u, cf) == 262144u)
					{
						have_ui_logo = 1;
					}
				}
				fclose(cf);
			}
		}

		if (!have_ui_logo)
		{
			static char const * const s_bmap_paths[] = {
				"maps/bitmaps.map",
				"d:\\maps\\bitmaps.map",
				"assets/maps/bitmaps.map",
				"../assets/maps/bitmaps.map"
			};
			int bp;
			for (bp = 0; bp < 4 && !have_ui_logo; bp++)
			{
				char bmap_host[1024];
				FILE *bf;
				if (bp == 1)
					platform_translate_path(s_bmap_paths[bp], bmap_host, sizeof(bmap_host));
				else
					snprintf(bmap_host, sizeof(bmap_host), "%s", s_bmap_paths[bp]);
				bf = fopen(bmap_host, "rb");
				if (bf)
				{
					if (!dxt5_raw)
						dxt5_raw = (uint8_t *)malloc(262144u);
					if (dxt5_raw && fseek(bf, pix_off, SEEK_SET) == 0 &&
						fread(dxt5_raw, 1, 262144u, bf) == 262144u)
					{
						have_ui_logo = 1;
					}
					fclose(bf);
				}
			}
		}
	}

	tga_buf = (uint8_t *)calloc(1, 230418u);
	if (!tga_buf)
	{
		free(dxt5_raw);
		return;
	}

	/* 18-byte uncompressed 24-bit 320x240 TGA header */
	tga_buf[2] = 2;
	tga_buf[12] = (uint8_t)(320 & 0xFF);
	tga_buf[13] = (uint8_t)(320 >> 8);
	tga_buf[14] = (uint8_t)(240 & 0xFF);
	tga_buf[15] = (uint8_t)(240 >> 8);
	tga_buf[16] = 24;

	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 320; x++)
		{
			uint8_t grey = 0;
			uint8_t *dst = tga_buf + 18u + ((size_t)(239 - y) * 320u + (size_t)x) * 3u;

			if (have_ui_logo && dxt5_raw && y >= 56 && y < 184)
			{
				int sx = x * 2;
				int sy = (y - 56) * 2;
				uint8_t v0 = sample_dxt5_halo_logo_lum(dxt5_raw, sx, sy);
				uint8_t v1 = sample_dxt5_halo_logo_lum(dxt5_raw, sx + 1, sy);
				uint8_t v2 = sample_dxt5_halo_logo_lum(dxt5_raw, sx, sy + 1);
				uint8_t v3 = sample_dxt5_halo_logo_lum(dxt5_raw, sx + 1, sy + 1);
				uint8_t vmax = v0 > v1 ? v0 : v1;
				if (v2 > vmax) vmax = v2;
				if (v3 > vmax) vmax = v3;
				grey = vmax;
			}
			else if (!have_ui_logo && y >= 68 && y <= 172)
			{
				/* Procedural Halo ring & horizontal banner fallback so loading.tga is never blank black */
				float dx = ((float)x - 160.0f) / 36.0f;
				float dy = ((float)y - 120.0f) / 36.0f;
				float r2 = dx * dx + dy * dy;
				if (r2 >= 0.72f && r2 <= 1.24f)
				{
					float dist = (r2 < 0.96f) ? (0.96f - r2) / 0.24f : (r2 - 0.96f) / 0.28f;
					int ring_lum = (int)((1.0f - dist) * 235.0f);
					if (ring_lum > (int)grey)
						grey = (uint8_t)(ring_lum > 255 ? 255 : ring_lum);
				}
				if (y >= 114 && y <= 126 && x >= 44 && x < 276)
				{
					if (grey < 195)
						grey = 195;
				}
			}
			/* Subtle horizontal progress track beneath the Halo logo so the sweeping glow bar is always visible */
			if (y >= 190 && y <= 194 && x >= 32 && x < 288)
			{
				if (grey < 140)
					grey = 140;
			}
			dst[0] = grey;
			dst[1] = grey;
			dst[2] = grey;
		}
	}

	{
		FILE *out_z = fopen(z_tga_host, "wb");
		if (out_z)
		{
			fwrite(tga_buf, 1, 230418u, out_z);
			fclose(out_z);
		}
	}

	free(tga_buf);
	free(dxt5_raw);
}

static void forge_xbox_loading_screen_render_frame(float progress)
{
	struct rasterizer_frame_begin_parameters frame_parameters;
	struct rasterizer_window_begin_parameters window_parameters;

	if (!global_d3d_device)
		return;
	if (progress < 0.001f)
		progress = 0.001f;
	if (progress > 1.0f)
		progress = 1.0f;

	shell_idle();
	sound_render();

	memset(&frame_parameters, 0, sizeof(frame_parameters));
	rasterizer_frame_begin(&frame_parameters);
	rasterizer_windows_begin();

	memset(&window_parameters, 0, sizeof(window_parameters));
	window_parameters.camera.position.x = 0.0f;
	window_parameters.camera.position.y = 0.0f;
	window_parameters.camera.position.z = 0.0f;
	window_parameters.camera.forward.i = 0.0f;
	window_parameters.camera.forward.j = 0.0f;
	window_parameters.camera.forward.k = 1.0f;
	window_parameters.camera.up.i = 0.0f;
	window_parameters.camera.up.j = 1.0f;
	window_parameters.camera.up.k = 0.0f;
	window_parameters.camera.mirrored = FALSE;
	window_parameters.camera.vertical_field_of_view = 1.1f;
	window_parameters.camera.viewport_bounds.x0 = 0;
	window_parameters.camera.viewport_bounds.y0 = 0;
	window_parameters.camera.viewport_bounds.x1 = (short)halo_screen_width();
	window_parameters.camera.viewport_bounds.y1 = 480;
	window_parameters.camera.window_bounds = window_parameters.camera.viewport_bounds;
	window_parameters.camera.z_near = 0.01f;
	window_parameters.camera.z_far = 1.0f;
	render_camera_build_frustum(&window_parameters.camera, NULL, &window_parameters.frustum, TRUE);
	window_parameters.rasterizer_target = 0;

	rasterizer_window_begin(&window_parameters);
	progress_bar_display(progress);
	rasterizer_window_end();
	rasterizer_windows_end();
	rasterizer_frame_end();
	main_present_frame();
	s_forge_loading_last_ms = system_milliseconds();
}

static void forge_xbox_loading_screen_begin(void)
{
	if (!global_d3d_device)
		return;
	forge_ensure_xbox_loading_tga();
	progress_bar_eachframe();
	if (progress_bar_mode.texture0)
	{
		IDirect3DTexture8_Release(progress_bar_mode.texture0);
		progress_bar_mode.texture0 = NULL;
	}
	if (progress_bar_mode.texture1)
	{
		IDirect3DTexture8_Release(progress_bar_mode.texture1);
		progress_bar_mode.texture1 = NULL;
	}
	last_t = 0.0f;
	s_forge_loading_active = 1;
	progress_bar_begin(FALSE);
	forge_xbox_loading_screen_render_frame(0.001f);
}

static void forge_xbox_loading_screen_update(float progress)
{
	int catchup = 0;
	if (!global_d3d_device || !s_forge_loading_active)
		return;
	if (progress < 0.01f)
		progress = 0.01f;
	if (progress > 1.0f)
		progress = 1.0f;

	forge_xbox_loading_screen_render_frame(progress);
	while (last_t + 0.04f < progress && catchup < 5)
	{
		Sleep(10);
		forge_xbox_loading_screen_render_frame(progress);
		catchup++;
	}
}

static void forge_xbox_loading_screen_end(void)
{
	int step;
	if (!global_d3d_device || !s_forge_loading_active)
		return;
	for (step = 0; step < 12 && last_t < 0.96f; step++)
	{
		forge_xbox_loading_screen_render_frame(1.0f);
		Sleep(10);
	}
	forge_xbox_loading_screen_render_frame(1.0f);
	progress_bar_end();
	s_forge_loading_active = 0;
	s_forge_loading_map_base = 0.0f;
	s_forge_loading_map_span = 0.0f;
}

struct forge_cache_file_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct forge_runtime_state g_forge;
int g_forge_session_enabled = 0;

long forge_get_network_version(void)
{
	return g_forge_session_enabled ? FORGE_NETWORK_VERSION : HALO_PORT_NETWORK_VERSION;
}

void forge_set_status(char const *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(g_forge.status_message, sizeof(g_forge.status_message), fmt, ap);
	va_end(ap);
	g_forge.status_message_until_ms = (uint32_t)system_milliseconds() + 6000u;
}

void forge_draw_screen_rect(short x0, short y0, short x1, short y1, uint32_t argb)
{
	if (!global_d3d_device || x1 <= x0 || y1 <= y0)
		return;

	_rasterizer_debug_immediate_begin_screenspace();
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_BLENDOP, D3DBLENDOP_ADD);
	pixel_shader.final_combiner_inputs_efg = 0x1400;
	rasterizer_set_pixel_shader(&pixel_shader);

	IDirect3DDevice8_Begin(global_d3d_device, D3DPT_TRIANGLEFAN);
	IDirect3DDevice8_SetVertexDataColor(global_d3d_device, 9, argb);
	IDirect3DDevice8_SetVertexData2s(global_d3d_device, 0, x0, y0);
	IDirect3DDevice8_SetVertexData2s(global_d3d_device, 0, x1, y0);
	IDirect3DDevice8_SetVertexData2s(global_d3d_device, 0, x1, y1);
	IDirect3DDevice8_SetVertexData2s(global_d3d_device, 0, x0, y1);
	IDirect3DDevice8_End(global_d3d_device);
	_rasterizer_debug_immediate_end_screenspace();
}

int forge_is_multiplayer_session(void)
{
	short conn = game_connection();
	return (conn == _game_connection_network_client || conn == _game_connection_network_server) ? 1 : 0;
}

static int is_allowed_forge_light_or_device_tag(uint32_t grp, char const *tag_path)
{
	static struct
	{
		uint32_t grp;
		char const *path;
	} const s_allowed[] = {
		/* Functioning Beam Emitters ('mach', animated emitter + beam effect + hum sound + light) */
		{ 'mach', "levels\\a30\\devices\\beam emitter\\beam emitter" },
		{ 'mach', "levels\\a30\\devices\\beam emitter\\beam emitter red" },
		/* 12 Light Fixtures ('lifi') with 3D fixture models, lens flares, and dynamic illumination */
		{ 'lifi', "levels\\a50\\devices\\light_ceiling\\bright_light_ceiling" },
		{ 'lifi', "levels\\a50\\devices\\light_ceiling\\light_ceiling" },
		{ 'lifi', "levels\\a50\\devices\\light_ceiling_large\\light_ceiling_large" },
		{ 'lifi', "levels\\a50\\devices\\light_ground\\light_ground" },
		{ 'lifi', "levels\\b30\\devices\\light fixtures\\light fixture ceiling1\\light fixture ceiling1" },
		{ 'lifi', "levels\\b30\\devices\\light fixtures\\light fixture floor1\\light fixture floor1" },
		{ 'lifi', "levels\\b30\\devices\\light fixtures\\light fixture wall1\\light fixture wall1" },
		{ 'lifi', "levels\\b40\\devices\\b40_spotlight\\b40_spotlight" },
		{ 'lifi', "levels\\c10\\devices\\ground_bright\\ground_bright" },
		{ 'lifi', "levels\\test\\carousel\\devices\\car_spotlight\\car_spotlight" },
		{ 'lifi', "levels\\test\\damnation\\devices\\dam_spotlight\\dam_spotlight" },
		{ 'lifi', "levels\\test\\damnation\\devices\\light_ceiling\\mp bright_light_ceiling" },
		/* 5 Dynamic Point Light Scenery tags ('scen') with 3D meshes and colored dynamic lights */
		{ 'scen', "levels\\c10\\scenery\\cyan point light\\light cyan dynamic" },
		{ 'scen', "levels\\c10\\scenery\\green point light\\light green dynamic" },
		{ 'scen', "levels\\c10\\scenery\\orange point light\\light orange dynamic" },
		{ 'scen', "levels\\c10\\scenery\\red point light\\light red dynamic" },
		{ 'scen', "levels\\b40\\scenery\\b40_ctorch\\ctorch" }
	};
	size_t i;

	if (!tag_path || !tag_path[0])
		return 0;
	if ((grp == 0 || grp == 'mach') &&
		(strstr(tag_path, "door") != NULL || strstr(tag_path, "Door") != NULL || strstr(tag_path, "DOOR") != NULL))
	{
		if (strstr(tag_path, "doorblinker") == NULL && strstr(tag_path, "door_blast_collision") == NULL)
			return 1;
	}
	for (i = 0; i < sizeof(s_allowed) / sizeof(s_allowed[0]); i++)
	{
		if ((grp == 0 || grp == s_allowed[i].grp) &&
			_stricmp(tag_path, s_allowed[i].path) == 0)
		{
			return 1;
		}
	}
	return 0;
}

static int is_forge_door_tag(uint32_t grp, char const *tag_path)
{
	if (!tag_path || !tag_path[0])
		return 0;
	if ((grp == 0 || grp == 'mach') &&
		(strstr(tag_path, "door") != NULL || strstr(tag_path, "Door") != NULL || strstr(tag_path, "DOOR") != NULL))
	{
		if (strstr(tag_path, "doorblinker") == NULL && strstr(tag_path, "door_blast_collision") == NULL)
			return 1;
	}
	return 0;
}

static int path_contains_any_ci(char const *tag_path, char const *const *needles, size_t count)
{
	char lower_buf[260];
	size_t i, k;
	if (!tag_path || !tag_path[0])
		return 0;
	for (i = 0; tag_path[i] && i + 1 < sizeof(lower_buf); i++)
		lower_buf[i] = (tag_path[i] >= 'A' && tag_path[i] <= 'Z') ? (char)(tag_path[i] - 'A' + 'a') : tag_path[i];
	lower_buf[i] = '\0';
	for (k = 0; k < count; k++)
	{
		if (strstr(lower_buf, needles[k]) != NULL)
			return 1;
	}
	return 0;
}

static int is_forge_light_beacon_emitter_tag(uint32_t grp, char const *tag_path)
{
	static char const *const s_light_substrings[] = {
		"light", "spotlight", "beacon", "emitter", "ctorch", "torch"
	};
	if (!tag_path || !tag_path[0] || is_forge_door_tag(grp, tag_path))
		return 0;
	if (grp == 'lifi')
		return 1;
	if (grp != 0 && grp != 'scen' && grp != 'mach')
		return 0;
	return path_contains_any_ci(tag_path, s_light_substrings, sizeof(s_light_substrings) / sizeof(s_light_substrings[0]));
}

static int is_excluded_scenery_or_effect_tag(char const *tag_path)
{
	static char const *const s_excluded_substrings[] = {
		"emitter", "particle", "effect", "explosion", "exhaust", "lightning",
		"point light", "lens flare", "dynamic light", "nomodel", "hologram",
		"holo control", "doorblinker", "nutblocker", "door_blast_collision",
		"countdown timer", "temp critical", "teleporter_shield", "smoke", "spark",
		"steam", "flame", "flare", "blood_pool", "floor_arrow", "cutscene_",
		"lifepod_atmosphere_entry", "\\devices\\",
		"scenery\\cortana_chip\\cortana_chip", "scenery\\baton\\baton",
		"scenery\\tubewire\\tubewire", "scenery\\debris\\rocks_smoldering\\rocks_smoldering",
		"levels\\b40\\scenery\\b40_cpanel\\b40_cpanel", "scenery\\helmet_chip\\helmet_chip",
		"scenery\\lockpick\\lockpick", "levels\\c10\\scenery\\c10_smallplant\\c10_smallplant",
		"levels\\c10\\scenery\\c10_lilypad\\c10_lilypad", "levels\\c10\\scenery\\c10_lilypadbunch\\c10_lilypadbunch",
		"levels\\c10\\scenery\\c10_lilypadcluster\\c10_lilypadcluster", "levels\\c10\\scenery\\c10_plantclump\\c10_plantclump",
		"levels\\c10\\scenery\\c10_lilypadbunchb\\c10_lilypadbunchb", "levels\\c10\\scenery\\c10_lilypadclusterb\\c10_lilypadclusterb",
		"scenery\\baton\\baton dynamic", "scenery\\index\\index", "scenery\\index_hold\\index_hold",
		"levels\\c40\\scenery\\c40_snowbushsmall_dark\\c40_snowbushsmall_dark",
		"scenery\\hilltop\\hilltop", "scenery\\captain_implant\\captain_implant", "flood_captain"
	};
	if (!tag_path || !tag_path[0])
		return 1;
	if (is_allowed_forge_light_or_device_tag(0, tag_path))
		return 0;
	if (_strnicmp(tag_path, "cinematics\\", 11) == 0)
		return 1;
	return path_contains_any_ci(tag_path, s_excluded_substrings, sizeof(s_excluded_substrings) / sizeof(s_excluded_substrings[0]));
}

static int is_excluded_weapon_or_equipment_tag(char const *tag_path)
{
	static char const *const s_excluded_weap_eqip[] = {
		"gravity rifle", "gravity_rifle", "full-spectrum", "full spectrum", "full_spectrum",
		"plasma_cannon", "plasma cannon", "fuel rod", "fuel_rod", "double speed", "double_speed",
		"target_laser", "target laser", "energy sword", "energy_sword",
		"\\flag", "flag\\", "\\ball", "ball\\"
	};
	if (!tag_path || !tag_path[0])
		return 1;
	return path_contains_any_ci(tag_path, s_excluded_weap_eqip, sizeof(s_excluded_weap_eqip) / sizeof(s_excluded_weap_eqip[0]));
}

static int compute_catalog_entry_allowed(struct forge_catalog_entry const *entry)
{
	if (!entry)
		return 0;
	if (entry->source_cache_id == 2 || entry->source_cache_id == 3 || entry->source_cache_id == 4)
		return 1;
	/* Disable default stock bipeds, AI characters, and control devices */
	if (entry->category == _forge_cat_bipeds ||
	    entry->category == _forge_cat_ai_squads ||
	    entry->group_tag == 'bipd' ||
	    entry->group_tag == 'actv' ||
	    entry->group_tag == 'ctrl')
	{
		return 0;
	}
	/* Doors have their own dedicated DOORS category (_forge_cat_barriers_devices) */
	if (entry->category == _forge_cat_barriers_devices)
	{
		return is_forge_door_tag(entry->group_tag, entry->tag_path);
	}
	if (is_forge_door_tag(entry->group_tag, entry->tag_path))
	{
		return (entry->category == _forge_cat_barriers_devices) ? 1 : 0;
	}
	/* Lights, beacons, and emitters have their own dedicated LIGHTS category (_forge_cat_lights) */
	if (entry->category == _forge_cat_lights)
	{
		if (!is_forge_light_beacon_emitter_tag(entry->group_tag, entry->tag_path))
			return 0;
	}
	else if (is_forge_light_beacon_emitter_tag(entry->group_tag, entry->tag_path))
	{
		return 0;
	}
	if (entry->group_tag == 'mach' || entry->group_tag == 'lifi')
	{
		if (entry->source_cache_id == 2 || entry->source_cache_id == 1)
			return 1;
		return is_allowed_forge_light_or_device_tag(entry->group_tag, entry->tag_path);
	}
	/* Allow all custom tag cache entries (source_cache_id == 2) and injected donor map tags (source_cache_id == 1) */
	if (entry->source_cache_id == 2 || entry->source_cache_id == 1)
		return 1;
	/* Exclude gravity rifle, full spectrum vision, fuel rod (plasma_cannon), and double speed */
	if ((entry->group_tag == 'weap' || entry->group_tag == 'eqip' ||
	     entry->category == _forge_cat_weapons || entry->category == _forge_cat_equipment) &&
	    is_excluded_weapon_or_equipment_tag(entry->tag_path))
	{
		return 0;
	}
	/* Exclude all emitters, particle effects, point lights, lens flares, and device scenery props */
	if (entry->group_tag == 'scen' && is_excluded_scenery_or_effect_tag(entry->tag_path))
	{
		return 0;
	}
	/* Allow standard vehicles, Pelican, and Wraith in the vehicle spawn list */
	if (entry->group_tag == 'vehi' || entry->category == _forge_cat_vehicles)
	{
		static char const *const s_allowed_vehicles[] = {
			"vehicles\\warthog\\warthog",
			"vehicles\\warthog\\mp_warthog",
			"vehicles\\rwarthog\\rwarthog",
			"vehicles\\ghost\\ghost",
			"vehicles\\banshee\\banshee",
			"vehicles\\banshee\\banshee_mp",
			"vehicles\\scorpion\\scorpion",
			"vehicles\\pelican\\pelican",
			"vehicles\\wraith\\wraith",
			"vehicles\\c_gun_turret\\c_gun_turret",
			"vehicles\\c_gun_turret\\c_gun_turret_mp",
			"vehicles\\c gun turret\\c gun turret",
			"vehicles\\c gun turret\\c gun turret_mp"
		};
		int v, allowed = 0;
		for (v = 0; v < (int)(sizeof(s_allowed_vehicles) / sizeof(s_allowed_vehicles[0])); v++)
		{
			if (_stricmp(entry->tag_path, s_allowed_vehicles[v]) == 0)
			{
				allowed = 1;
				break;
			}
		}
		if (!allowed)
			return 0;
	}
	return 1;
}

int forge_catalog_entry_allowed(struct forge_catalog_entry const *entry)
{
	if (!entry)
		return 0;
	if (entry->pad & 0x0100u)
		return (entry->pad & 0x0001u) ? 1 : 0;
	return compute_catalog_entry_allowed(entry);
}

static void make_short_display_name(char const *tag_path, char const *prefix, char *out, size_t out_sz)
{
	char const *leaf = tag_path;
	char const *p;
	for (p = tag_path; *p; p++)
	{
		if (*p == '\\' || *p == '/')
			leaf = p + 1;
	}
	if (prefix && (_stricmp(prefix, "[c]") == 0))
		snprintf(out, out_sz, "[C] %s", leaf);
	else if (prefix && (_stricmp(prefix, "[map]") == 0))
		snprintf(out, out_sz, "[MAP] %s", leaf);
	else if (prefix && prefix[0] == '[')
		snprintf(out, out_sz, "%s", prefix);
	else if (prefix && prefix[0])
		snprintf(out, out_sz, "%s %s", prefix, leaf);
	else
		snprintf(out, out_sz, "%s", leaf);
}

static int is_equivalent_stock_vehicle_path(char const *a, char const *b)
{
	static struct { char const *x; char const *y; } const pairs[] = {
		{ "vehicles\\warthog\\warthog", "vehicles\\warthog\\mp_warthog" },
		{ "vehicles\\ghost\\ghost", "vehicles\\ghost\\ghost_mp" },
		{ "vehicles\\scorpion\\scorpion", "vehicles\\scorpion\\scorpion_mp" },
		{ "vehicles\\banshee\\banshee", "vehicles\\banshee\\banshee_mp" },
		{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c_gun_turret\\c_gun_turret_mp" },
		{ "vehicles\\c gun turret\\c gun turret", "vehicles\\c gun turret\\c gun turret_mp" },
		{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c gun turret\\c gun turret_mp" },
		{ "vehicles\\c gun turret\\c gun turret", "vehicles\\c_gun_turret\\c_gun_turret_mp" },
		{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c gun turret\\c gun turret" },
		{ "vehicles\\c_gun_turret\\c_gun_turret_mp", "vehicles\\c gun turret\\c gun turret_mp" }
	};
	size_t k;
	if (!a || !b)
		return 0;
	for (k = 0; k < sizeof(pairs) / sizeof(pairs[0]); k++)
	{
		if ((_stricmp(a, pairs[k].x) == 0 && _stricmp(b, pairs[k].y) == 0) ||
			(_stricmp(a, pairs[k].y) == 0 && _stricmp(b, pairs[k].x) == 0))
		{
			return 1;
		}
	}
	return 0;
}

static void add_catalog_entry(
	uint32_t group_tag,
	uint8_t category,
	uint8_t source_cache_id,
	char const *tag_path,
	char const *display_prefix)
{
	int i;
	struct forge_catalog_entry *e;

	if (!tag_path || !tag_path[0] || g_forge.catalog_count >= FORGE_MAX_CATALOG_ENTRIES)
		return;

	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (g_forge.catalog[i].group_tag == group_tag &&
		    (_stricmp(g_forge.catalog[i].tag_path, tag_path) == 0 ||
		     (group_tag == 'vehi' && source_cache_id == 3 &&
		      is_equivalent_stock_vehicle_path(g_forge.catalog[i].tag_path, tag_path))))
		{
			if (source_cache_id == 2)
			{
				g_forge.catalog[i].source_cache_id = 2;
				g_forge.catalog[i].category = category;
				make_short_display_name(tag_path, display_prefix, g_forge.catalog[i].display_name, sizeof(g_forge.catalog[i].display_name));
				g_forge.catalog[i].pad = (uint16_t)(0x0100u | (compute_catalog_entry_allowed(&g_forge.catalog[i]) ? 1u : 0u));
			}
			else if (source_cache_id == 4 && g_forge.catalog[i].source_cache_id != 2)
			{
				g_forge.catalog[i].source_cache_id = 4;
				g_forge.catalog[i].category = category;
				make_short_display_name(tag_path, display_prefix, g_forge.catalog[i].display_name, sizeof(g_forge.catalog[i].display_name));
				g_forge.catalog[i].pad = (uint16_t)(0x0100u | (compute_catalog_entry_allowed(&g_forge.catalog[i]) ? 1u : 0u));
			}
			else if (source_cache_id == 3 && g_forge.catalog[i].source_cache_id == 0 &&
			         !forge_catalog_entry_allowed(&g_forge.catalog[i]))
			{
				g_forge.catalog[i].source_cache_id = 3;
				g_forge.catalog[i].category = category;
				make_short_display_name(tag_path, display_prefix, g_forge.catalog[i].display_name, sizeof(g_forge.catalog[i].display_name));
				g_forge.catalog[i].pad = (uint16_t)(0x0100u | (compute_catalog_entry_allowed(&g_forge.catalog[i]) ? 1u : 0u));
			}
			return;
		}
	}

	e = &g_forge.catalog[g_forge.catalog_count++];
	e->group_tag = group_tag;
	e->category = category;
	e->source_cache_id = source_cache_id;
	e->pad = 0;
	snprintf(e->tag_path, sizeof(e->tag_path), "%s", tag_path);
	make_short_display_name(tag_path, display_prefix, e->display_name, sizeof(e->display_name));
	e->pad = (uint16_t)(0x0100u | (compute_catalog_entry_allowed(e) ? 1u : 0u));
}

static void scan_cached_tags_manifest(char const *manifest_path)
{
	FILE *f = fopen(manifest_path, "r");
	char line[512];
	if (!f)
		return;
	while (fgets(line, sizeof(line), f))
	{
		char grp_str[16];
		int cat_id = 0;
		int src_id = 0;
		char tag_path[256];
		char custom_label[128];
		char prefix_buf[140];
		uint32_t grp = 0;
		char *nl;
		char *bar;

		nl = strchr(line, '\r');
		if (nl) *nl = '\0';
		nl = strchr(line, '\n');
		if (nl) *nl = '\0';
		if (line[0] == '#' || line[0] == '\0')
			continue;

		custom_label[0] = '\0';
		if (sscanf(line, "%4s|%d|%d|%255[^\n]", grp_str, &cat_id, &src_id, tag_path) == 4)
		{
			bar = strchr(tag_path, '|');
			if (bar)
			{
				*bar = '\0';
				snprintf(custom_label, sizeof(custom_label), "%s", bar + 1);
			}
			if (strlen(grp_str) == 4 && tag_path[0] != '\0')
			{
				grp = ((uint32_t)(uint8_t)grp_str[0] << 24) |
				      ((uint32_t)(uint8_t)grp_str[1] << 16) |
				      ((uint32_t)(uint8_t)grp_str[2] << 8) |
				      ((uint32_t)(uint8_t)grp_str[3]);
				if (grp == 'ctrl')
					continue;
				if ((grp == 'mach' || grp == 'lifi') &&
					src_id != 2 && src_id != 1 &&
					!is_allowed_forge_light_or_device_tag(grp, tag_path))
				{
					continue;
				}
				if (src_id != 2 && grp == 'scen' && is_excluded_scenery_or_effect_tag(tag_path))
					continue;
				if (src_id != 2 && (grp == 'weap' || grp == 'eqip') && is_excluded_weapon_or_equipment_tag(tag_path))
					continue;

				/* Map group tag directly to the separated WEAPONS vs EQUIPMENT vs VEHICLES vs SCENERY vs DOORS vs LIGHTS tabs */
				if (grp == 'weap')
					cat_id = _forge_cat_weapons;
				else if (grp == 'eqip')
					cat_id = _forge_cat_equipment;
				else if (grp == 'vehi')
					cat_id = _forge_cat_vehicles;
				else if (is_forge_door_tag(grp, tag_path))
					cat_id = _forge_cat_barriers_devices;
				else if (is_forge_light_beacon_emitter_tag(grp, tag_path))
					cat_id = _forge_cat_lights;
				else if (grp == 'scen' || grp == 'mach' || grp == 'lifi')
					cat_id = _forge_cat_scenery;
				else if (grp == 'bipd')
					cat_id = _forge_cat_bipeds;
				else if (grp == 'actv')
					cat_id = _forge_cat_ai_squads;
				else
					continue;

				if (src_id == 2)
				{
					if (custom_label[0])
					{
						if (_strnicmp(custom_label, "[c]", 3) == 0)
							snprintf(prefix_buf, sizeof(prefix_buf), "%s", custom_label);
						else
							snprintf(prefix_buf, sizeof(prefix_buf), "[C] %s", custom_label);
					}
					else
					{
						snprintf(prefix_buf, sizeof(prefix_buf), "[C]");
					}
					add_catalog_entry(grp, (uint8_t)cat_id, 2, tag_path, prefix_buf);
				}
				else
				{
					add_catalog_entry(
						grp,
						(uint8_t)cat_id,
						(uint8_t)src_id,
						tag_path,
						src_id ? (grp == 'actv' ? "[CUSTOM AI]" : "[CUSTOM]") : (grp == 'actv' ? "[AI]" : ""));
				}
			}
		}
	}
	fclose(f);
}

static void scan_faber_tag_vault_into_catalog(void)
{
	long prof_count, p, tag_count, t;

	port_tag_profiles_scan();
	prof_count = port_tag_profiles_count();
	for (p = 0; p < prof_count; p++)
	{
		tag_count = port_tag_profiles_tag_count(p);
		for (t = 0; t < tag_count; t++)
		{
			uint32_t grp;
			char const *tag_path;
			char const *custom_name;
			char prefix_buf[140];
			uint8_t cat_id;

			if (!port_tag_profiles_tag_enabled(p, t))
				continue;
			grp = (uint32_t)port_tag_profiles_tag_group(p, t);
			tag_path = port_tag_profiles_tag_name(p, t);
			if (!tag_path || !tag_path[0])
				continue;

			if (grp == 'weap')
				cat_id = _forge_cat_weapons;
			else if (grp == 'eqip')
				cat_id = _forge_cat_equipment;
			else if (grp == 'vehi')
				cat_id = _forge_cat_vehicles;
			else if (grp == 'bipd')
				cat_id = _forge_cat_bipeds;
			else if (grp == 'actv')
				cat_id = _forge_cat_ai_squads;
			else if (grp == 'lifi')
				cat_id = _forge_cat_lights;
			else if (grp == 'mach' || grp == 'ctrl')
				cat_id = is_forge_light_beacon_emitter_tag(grp, tag_path) ? _forge_cat_lights : _forge_cat_barriers_devices;
			else if (grp == 'scen')
				cat_id = is_forge_light_beacon_emitter_tag(grp, tag_path) ? _forge_cat_lights : _forge_cat_scenery;
			else
				continue;

			custom_name = port_tag_profiles_tag_option(p, t, "name");
			if (custom_name && custom_name[0])
			{
				if (_strnicmp(custom_name, "[c]", 3) == 0)
					snprintf(prefix_buf, sizeof(prefix_buf), "%s", custom_name);
				else
					snprintf(prefix_buf, sizeof(prefix_buf), "[C] %s", custom_name);
			}
			else
			{
				snprintf(prefix_buf, sizeof(prefix_buf), "[C]");
			}
			add_catalog_entry(grp, cat_id, 2, tag_path, prefix_buf);
		}
	}
}

static int is_excluded_map_local_helper_tag(char const *tag_path)
{
	static char const *const s_helpers[] = {
		"nomodel",
		"hologram",
		"holo control",
		"doorblinker",
		"nutblocker",
		"door_blast_collision",
		"countdown timer",
		"temp critical",
		"teleporter_shield",
		"floor_arrow",
		"cutscene_",
		"lifepod_atmosphere_entry",
		"\\devices\\"
	};
	char lower_buf[260];
	size_t i, k;

	if (!tag_path || !tag_path[0])
		return 1;
	if (_strnicmp(tag_path, "cinematics\\", 11) == 0 ||
		_strnicmp(tag_path, "ui\\", 3) == 0 ||
		_strnicmp(tag_path, "sound\\", 6) == 0 ||
		_strnicmp(tag_path, "rasterizer\\", 11) == 0 ||
		_strnicmp(tag_path, "fx\\", 3) == 0)
	{
		return 1;
	}
	for (i = 0; tag_path[i] && i + 1 < sizeof(lower_buf); i++)
	{
		char ch = tag_path[i];
		if (ch >= 'A' && ch <= 'Z')
			ch = (char)(ch - 'A' + 'a');
		lower_buf[i] = ch;
	}
	lower_buf[i] = '\0';

	for (k = 0; k < sizeof(s_helpers) / sizeof(s_helpers[0]); k++)
	{
		if (strstr(lower_buf, s_helpers[k]) != NULL)
			return 1;
	}
	return 0;
}

static void scan_loaded_map_into_catalog(void)
{
	char const *cur_map = forge_get_active_map_name();
	struct scenario *scen;
	long tag_count = 0;
	struct forge_cache_file_tag_instance *instances;
	long i;

	if (!cur_map || !cur_map[0] ||
		_stricmp(cur_map, "levels\\ui\\ui") == 0 ||
		_stricmp(cur_map, "ui") == 0)
	{
		return;
	}
	scen = global_scenario_try_and_get();
	if (!scen || scen->type == 2 /* _scenario_type_main_menu */)
		return;

	instances = (struct forge_cache_file_tag_instance *)cache_files_tag_instances(&tag_count);
	if (!instances || tag_count <= 0)
		return;

	for (i = 0; i < tag_count; i++)
	{
		struct forge_cache_file_tag_instance const *inst = &instances[i];
		uint32_t grp;
		char const *tpath;
		uint8_t cat_id;
		struct _object_definition const *odef;

		if (inst->tag_index == NONE || !inst->name || !inst->name[0] || !inst->base_address)
			continue;
		grp = (uint32_t)inst->group_tag;
		tpath = inst->name;

		if (grp != 'scen' && grp != 'vehi' && grp != 'weap' &&
			grp != 'eqip' && grp != 'mach' && grp != 'lifi')
		{
			continue;
		}
		if (is_excluded_map_local_helper_tag(tpath))
			continue;

		/* Skip built-in vehicle turret weapons and non-placeable pseudo-weapons */
		if (grp == 'weap' &&
			(_strnicmp(tpath, "vehicles\\", 9) == 0 ||
			 _strnicmp(tpath, "characters\\", 11) == 0 ||
			 is_excluded_weapon_or_equipment_tag(tpath)))
		{
			continue;
		}
		if (grp == 'eqip' && is_excluded_weapon_or_equipment_tag(tpath))
			continue;

		/* Require a valid 3D model on object definitions (or light/emitter classification)
		 * so invisible script-anchor scenery or sound-only dummy objects do not clutter the palette. */
		odef = (struct _object_definition const *)inst->base_address;
		if (grp != 'lifi' && odef->model.index == NONE)
		{
			if (!is_forge_light_beacon_emitter_tag(grp, tpath) || odef->attachments.count <= 0)
				continue;
		}

		if (grp == 'weap')
			cat_id = _forge_cat_weapons;
		else if (grp == 'eqip')
			cat_id = _forge_cat_equipment;
		else if (grp == 'vehi')
			cat_id = _forge_cat_vehicles;
		else if (grp == 'lifi')
			cat_id = _forge_cat_lights;
		else if (grp == 'mach')
			cat_id = is_forge_light_beacon_emitter_tag(grp, tpath) ? _forge_cat_lights : _forge_cat_barriers_devices;
		else if (is_forge_door_tag(grp, tpath))
			cat_id = _forge_cat_barriers_devices;
		else if (is_forge_light_beacon_emitter_tag(grp, tpath))
			cat_id = _forge_cat_lights;
		else
			cat_id = _forge_cat_scenery;

		add_catalog_entry(grp, cat_id, 3, tpath, "[map]");
	}
}

static void invalidate_credits_cache(void);
static void close_runtime_xbox_donor(void);
static void close_preview_xbox_donor(void);
static int has_active_dynamic_tags(void);

void forge_runtime_reload_catalogs(void)
{
	struct forge_catalog_entry saved_ephemeral[64];
	int saved_ephemeral_count = 0;
	int ep_i;

	if (g_forge.initialized && g_forge.catalog_count > 0 && has_active_dynamic_tags())
	{
		for (ep_i = 0; ep_i < g_forge.catalog_count && saved_ephemeral_count < 64; ep_i++)
		{
			if (g_forge.catalog[ep_i].source_cache_id == 4)
				saved_ephemeral[saved_ephemeral_count++] = g_forge.catalog[ep_i];
		}
	}

	invalidate_credits_cache();
	g_forge.catalog_count = 0;

	/* Built-in Spawns & Teleporters */
	add_catalog_entry('spwn', _forge_cat_spawns_teleporters, 0, "spawn\\player_red", "[SPAWN] Red Team Spawn");
	add_catalog_entry('spwn', _forge_cat_spawns_teleporters, 0, "spawn\\player_blue", "[SPAWN] Blue Team Spawn");
	add_catalog_entry('spwn', _forge_cat_spawns_teleporters, 0, "spawn\\player_ffa", "[SPAWN] FFA / Neutral Spawn");
	add_catalog_entry('tele', _forge_cat_spawns_teleporters, 0, "teleporter\\origin", "[TELEPORTER] Origin Node");
	add_catalog_entry('tele', _forge_cat_spawns_teleporters, 0, "teleporter\\destination", "[TELEPORTER] Destination Node");

	/* Built-in Netgame Objectives */
	add_catalog_entry('flag', _forge_cat_objectives, 0, "objective\\ctf_flag_red", "[CTF] Red Flag Base");
	add_catalog_entry('flag', _forge_cat_objectives, 0, "objective\\ctf_flag_blue", "[CTF] Blue Flag Base");
	add_catalog_entry('flag', _forge_cat_objectives, 0, "objective\\oddball_spawn", "[ODDBALL] Ball Spawn Marker");
	add_catalog_entry('flag', _forge_cat_objectives, 0, "objective\\koth_hill", "[KOTH] Hill Marker");
	add_catalog_entry('flag', _forge_cat_objectives, 0, "objective\\race_track", "[RACE] Race Marker / Checkpoint");

	/* Load custom tag vault catalog first (so [C] items appear at the top of each category),
	 * then Xbox default catalog, then any map-local tags ([MAP]) from the currently loaded map */
	scan_faber_tag_vault_into_catalog();
	for (ep_i = 0; ep_i < saved_ephemeral_count; ep_i++)
	{
		add_catalog_entry(
			saved_ephemeral[ep_i].group_tag,
			saved_ephemeral[ep_i].category,
			4,
			saved_ephemeral[ep_i].tag_path,
			saved_ephemeral[ep_i].display_name);
	}
	scan_cached_tags_manifest("OpenCE Forge/cached_tags/xbox_catalog.txt");
	scan_loaded_map_into_catalog();

	/* 3. Stable-sort custom cached entries ([C], source_cache_id == 2 or 4) first,
	 *    then map-local entries ([MAP], source_cache_id == 3), then donor/stock entries */
	if (g_forge.catalog_count > 10)
	{
		struct forge_catalog_entry *sorted =
			(struct forge_catalog_entry *)malloc((size_t)g_forge.catalog_count * sizeof(struct forge_catalog_entry));
		if (sorted)
		{
			int i, out_n = 0;
			for (i = 0; i < g_forge.catalog_count; i++)
			{
				if (g_forge.catalog[i].source_cache_id == 2 || g_forge.catalog[i].source_cache_id == 4)
					sorted[out_n++] = g_forge.catalog[i];
			}
			for (i = 0; i < g_forge.catalog_count; i++)
			{
				if (g_forge.catalog[i].source_cache_id == 3)
					sorted[out_n++] = g_forge.catalog[i];
			}
			for (i = 0; i < g_forge.catalog_count; i++)
			{
				if (g_forge.catalog[i].source_cache_id == 1)
					sorted[out_n++] = g_forge.catalog[i];
			}
			for (i = 0; i < g_forge.catalog_count; i++)
			{
				if (g_forge.catalog[i].source_cache_id != 2 &&
					g_forge.catalog[i].source_cache_id != 4 &&
					g_forge.catalog[i].source_cache_id != 3 &&
					g_forge.catalog[i].source_cache_id != 1)
				{
					sorted[out_n++] = g_forge.catalog[i];
				}
			}
			memcpy(g_forge.catalog, sorted, (size_t)out_n * sizeof(struct forge_catalog_entry));
			free(sorted);
		}
	}
}

static int is_xbox_version5_map_file(char const *map_path)
{
	FILE *fp = fopen(map_path, "rb");
	uint32_t hdr[2];
	if (!fp)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return (hdr[0] == 0x68656164u /* 'head' */ && hdr[1] == 5u) ? 1 : 0;
}

static int is_mcc_version13_map_file(char const *map_path)
{
	FILE *fp = fopen(map_path, "rb");
	uint32_t hdr[2];
	if (!fp)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return (hdr[0] == 0x68656164u /* 'head' */ && hdr[1] == 13u) ? 1 : 0;
}

/*
 * Checks if a pre-existing decompressed Xbox cache file already exists for `map_path`
 * in OpenCE's Xbox hard-drive cache (z:\cache000.map..z:\cache005.map or z:\cache_donor.map)
 * or if `map_path` itself is already uncompressed on disk.
 * If not, decompresses the Xbox v5 map payload using zlib into OpenCE's z:\cache_donor.map.
 */
static int ensure_xbox_map_decompressed_cache(
	char const *map_path,
	char *out_cache_path,
	size_t out_cache_sz)
{
	FILE *in_fp;
	uint8_t hdr[2048];
	uint32_t head_magic, map_version, decomp_len, tag_off, tag_sz;
	char map_internal_name[36];
	int slot;

	if (!map_path || !map_path[0] || !out_cache_path || out_cache_sz == 0)
		return 0;
	out_cache_path[0] = 0;

	in_fp = fopen(map_path, "rb");
	if (!in_fp)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), in_fp) != sizeof(hdr))
	{
		fclose(in_fp);
		return 0;
	}

	head_magic = *(uint32_t const *)(hdr + 0x00);
	map_version = *(uint32_t const *)(hdr + 0x04);
	decomp_len = *(uint32_t const *)(hdr + 0x08);
	tag_off = *(uint32_t const *)(hdr + 0x10);
	tag_sz = *(uint32_t const *)(hdr + 0x14);
	memset(map_internal_name, 0, sizeof(map_internal_name));
	memcpy(map_internal_name, hdr + 0x20, 31);

	if (head_magic != 0x68656164u || map_version != 5u ||
		tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
	{
		fclose(in_fp);
		return 0;
	}

	/* 0. Check if OpenCE Forge/cached_tags/xbox_cache/<map_internal_name>.map already holds this decompressed map */
	if (map_internal_name[0])
	{
		char xcache_path[512];
		FILE *xf;
		snprintf(xcache_path, sizeof(xcache_path), "OpenCE Forge/cached_tags/xbox_cache/%s.map", map_internal_name);
		xf = fopen(xcache_path, "rb");
		if (xf)
		{
			uint8_t xhdr[2048];
			long xsz = 0;
			fseek(xf, 0, SEEK_END);
			xsz = ftell(xf);
			fseek(xf, 0, SEEK_SET);
			if (xsz >= (long)(tag_off + tag_sz) && fread(xhdr, 1, sizeof(xhdr), xf) == sizeof(xhdr))
			{
				if (*(uint32_t const *)(xhdr + 0x00) == 0x68656164u &&
					*(uint32_t const *)(xhdr + 0x04) == 5u &&
					*(uint32_t const *)(xhdr + 0x10) == tag_off &&
					*(uint32_t const *)(xhdr + 0x14) == tag_sz &&
					_strnicmp((char const *)(xhdr + 0x20), map_internal_name, 31) == 0)
				{
					fclose(xf);
					fclose(in_fp);
					snprintf(out_cache_path, out_cache_sz, "%s", xcache_path);
					return 1;
				}
			}
			fclose(xf);
		}
	}

	/* 1. Check if OpenCE's Xbox hard-drive cache (z:\cache000.map .. z:\cache005.map or z:\cache_donor.map)
	 * already holds this decompressed Xbox map; if so, use it directly without duplicating! */
	for (slot = 0; slot <= 6; slot++)
	{
		char z_xbox[64];
		char z_path[512];
		FILE *zf;
		if (slot < 6)
			snprintf(z_xbox, sizeof(z_xbox), "z:\\cache00%d.map", slot);
		else
			snprintf(z_xbox, sizeof(z_xbox), "z:\\cache_donor.map");
		platform_translate_path(z_xbox, z_path, sizeof(z_path));
		zf = fopen(z_path, "rb");
		if (zf)
		{
			uint8_t zhdr[2048];
			long zsz = 0;
			fseek(zf, 0, SEEK_END);
			zsz = ftell(zf);
			fseek(zf, 0, SEEK_SET);
			if (zsz >= (long)(tag_off + tag_sz) && fread(zhdr, 1, sizeof(zhdr), zf) == sizeof(zhdr))
			{
				if (*(uint32_t const *)(zhdr + 0x00) == 0x68656164u &&
					*(uint32_t const *)(zhdr + 0x04) == 5u &&
					*(uint32_t const *)(zhdr + 0x10) == tag_off &&
					*(uint32_t const *)(zhdr + 0x14) == tag_sz &&
					_strnicmp((char const *)(zhdr + 0x20), map_internal_name, 31) == 0)
				{
					fclose(zf);
					fclose(in_fp);
					snprintf(out_cache_path, out_cache_sz, "%s", z_path);
					return 1;
				}
			}
			fclose(zf);
		}
	}

	/* 2. Check if map_path itself is already uncompressed on disk; if so, read it directly in-place! */
	{
		long in_sz = 0;
		fseek(in_fp, 0, SEEK_END);
		in_sz = ftell(in_fp);
		if (decomp_len >= tag_off + tag_sz && in_sz >= (long)decomp_len)
		{
			uint32_t tag_sig = 0;
			if (fseek(in_fp, (long)(tag_off + 0x20u), SEEK_SET) == 0 &&
				fread(&tag_sig, 4, 1, in_fp) == 1 &&
				tag_sig == 0x626C616Du /* 'blam' */)
			{
				fclose(in_fp);
				snprintf(out_cache_path, out_cache_sz, "%s", map_path);
				return 1;
			}
		}
		fseek(in_fp, 2048L, SEEK_SET);
	}

	/* 3. Stream-decompress Xbox v5 map into persistent OpenCE Forge/cached_tags/xbox_cache/<name>.map file */
	if (map_internal_name[0])
	{
		CreateDirectoryA("OpenCE Forge", NULL);
		CreateDirectoryA("OpenCE Forge/cached_tags", NULL);
		CreateDirectoryA("OpenCE Forge/cached_tags/xbox_cache", NULL);
		snprintf(out_cache_path, out_cache_sz, "OpenCE Forge/cached_tags/xbox_cache/%s.map", map_internal_name);
	}
	else
	{
		platform_translate_path("z:\\cache_donor.map", out_cache_path, out_cache_sz);
		close_runtime_xbox_donor();
		close_preview_xbox_donor();
	}
	{
		FILE *tmp_out = fopen(out_cache_path, "wb");
		z_stream strm;
		uint8_t *in_chunk;
		uint8_t *out_chunk;
		size_t const chunk_sz = 262144u;
		int zret = Z_OK;
		int decomp_ok = 0;

		if (!tmp_out)
		{
			fclose(in_fp);
			return 0;
		}
		fwrite(hdr, 1, sizeof(hdr), tmp_out);

		in_chunk = (uint8_t *)malloc(chunk_sz);
		out_chunk = (uint8_t *)malloc(chunk_sz);
		memset(&strm, 0, sizeof(strm));
		if (in_chunk && out_chunk && inflateInit(&strm) == Z_OK)
		{
			decomp_ok = 1;
			do
			{
				strm.avail_in = (uInt)fread(in_chunk, 1, chunk_sz, in_fp);
				if (ferror(in_fp))
				{
					decomp_ok = 0;
					break;
				}
				if (strm.avail_in == 0)
					break;
				strm.next_in = in_chunk;
				do
				{
					size_t have;
					strm.avail_out = (uInt)chunk_sz;
					strm.next_out = out_chunk;
					zret = inflate(&strm, Z_NO_FLUSH);
					if (zret == Z_NEED_DICT || zret == Z_DATA_ERROR || zret == Z_MEM_ERROR)
					{
						decomp_ok = 0;
						break;
					}
					have = chunk_sz - (size_t)strm.avail_out;
					if (have > 0 && fwrite(out_chunk, 1, have, tmp_out) != have)
					{
						decomp_ok = 0;
						break;
					}
					if (s_forge_loading_active && s_forge_loading_map_span > 0.0f)
					{
						unsigned long now_ms = system_milliseconds();
						if (now_ms - s_forge_loading_last_ms >= 15u)
						{
							float map_frac = (decomp_len > 2048u)
								? ((float)strm.total_out / (float)(decomp_len - 2048u))
								: 0.5f;
							if (map_frac < 0.0f) map_frac = 0.0f;
							if (map_frac > 0.99f) map_frac = 0.99f;
							forge_xbox_loading_screen_render_frame(
								s_forge_loading_map_base + map_frac * s_forge_loading_map_span);
						}
					}
				} while (strm.avail_out == 0);
			} while (decomp_ok && zret != Z_STREAM_END);
			inflateEnd(&strm);
		}
		free(in_chunk);
		free(out_chunk);
		fclose(tmp_out);
		fclose(in_fp);

		if (!decomp_ok)
		{
			remove(out_cache_path);
			out_cache_path[0] = 0;
			return 0;
		}
	}
	return 1;
}

static int forge_index_xbox_map_file_to_catalog(
	char const *map_path,
	FILE *xbox_manifest_fp,
	FILE *legacy_manifest_fp,
	FILE *credits_fp)
{
	FILE *f;
	uint8_t hdr[2048];
	uint32_t decomp_len, tag_off, tag_sz, inst_addr, tag_base, inst_off;
	int32_t tag_count, i;
	uint8_t *tag_buf;
	int indexed_count = 0;
	char map_internal_name[64];
	long in_sz = 0;
	int read_ok = 0;

	if (!map_path || !map_path[0])
		return 0;

	f = fopen(map_path, "rb");
	if (!f)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr))
	{
		fclose(f);
		return 0;
	}
	if (*(uint32_t const *)(hdr + 0x00) != 0x68656164u ||
		*(uint32_t const *)(hdr + 0x04) != 5u)
	{
		fclose(f);
		return 0;
	}
	decomp_len = *(uint32_t const *)(hdr + 0x08);
	tag_off = *(uint32_t const *)(hdr + 0x10);
	tag_sz = *(uint32_t const *)(hdr + 0x14);
	memset(map_internal_name, 0, sizeof(map_internal_name));
	memcpy(map_internal_name, hdr + 0x20, 31);

	if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
	{
		fclose(f);
		return 0;
	}
	tag_buf = (uint8_t *)malloc(tag_sz);
	if (!tag_buf)
	{
		fclose(f);
		return 0;
	}

	/* If map_path is already uncompressed on disk, read [tag_off .. tag_off + tag_sz) directly */
	fseek(f, 0, SEEK_END);
	in_sz = ftell(f);
	if (decomp_len >= tag_off + tag_sz && in_sz >= (long)decomp_len)
	{
		uint32_t tag_sig = 0;
		if (fseek(f, (long)(tag_off + 0x20u), SEEK_SET) == 0 &&
			fread(&tag_sig, 4, 1, f) == 1 &&
			tag_sig == 0x626C616Du /* 'blam' */ &&
			fseek(f, (long)tag_off, SEEK_SET) == 0 &&
			fread(tag_buf, 1, tag_sz, f) == tag_sz)
		{
			read_ok = 1;
		}
	}

	fclose(f);

	/* Otherwise stream-decompress to OpenCE Forge/cached_tags/xbox_cache/<name>.map and read [tag_off .. tag_off + tag_sz) */
	if (!read_ok)
	{
		char decomp_cache_path[512];
		if (ensure_xbox_map_decompressed_cache(map_path, decomp_cache_path, sizeof(decomp_cache_path)))
		{
			FILE *df = fopen(decomp_cache_path, "rb");
			if (df)
			{
				if (fseek(df, (long)tag_off, SEEK_SET) == 0 &&
					fread(tag_buf, 1, tag_sz, df) == tag_sz)
				{
					read_ok = 1;
				}
				fclose(df);
			}
		}
	}

	if (!read_ok)
	{
		free(tag_buf);
		return 0;
	}

	inst_addr = *(uint32_t const *)(tag_buf + 0x00);
	tag_count = *(int32_t const *)(tag_buf + 0x0C);
	tag_base = inst_addr - 0x24u;
	inst_off = 0x24u;
	if (tag_count <= 0 || tag_count > 16384 || inst_off + (uint32_t)tag_count * 32u > tag_sz)
	{
		free(tag_buf);
		return 0;
	}

	if (credits_fp)
	{
		fprintf(credits_fp, "## Xbox Map: `%s` (`%s`)\n",
			map_internal_name[0] ? map_internal_name : map_path, map_path);
	}

	for (i = 0; i < tag_count; i++)
	{
		uint32_t io = inst_off + (uint32_t)i * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t name_addr = *(uint32_t const *)(tag_buf + io + 0x10);
		uint32_t data_addr = *(uint32_t const *)(tag_buf + io + 0x14);
		char const *tname;
		int cat_id = -1;
		char grp_str[5];

		if (name_addr < tag_base || (name_addr - tag_base) >= tag_sz)
			continue;
		tname = (char const *)(tag_buf + (name_addr - tag_base));
		if (!tname[0] || is_excluded_scenery_or_effect_tag(tname))
			continue;

		if (grp == 'weap')
		{
			if (!is_excluded_weapon_or_equipment_tag(tname))
				cat_id = _forge_cat_weapons;
		}
		else if (grp == 'eqip')
		{
			if (!is_excluded_weapon_or_equipment_tag(tname))
				cat_id = _forge_cat_equipment;
		}
		else if (grp == 'vehi')
			cat_id = _forge_cat_vehicles;
		else if (is_forge_door_tag(grp, tname))
			cat_id = _forge_cat_barriers_devices;
		else if (is_forge_light_beacon_emitter_tag(grp, tname) &&
				 (grp == 'scen' || is_allowed_forge_light_or_device_tag(grp, tname)))
			cat_id = _forge_cat_lights;
		else if (grp == 'scen')
			cat_id = _forge_cat_scenery;
		else if ((grp == 'mach' || grp == 'lifi') && is_allowed_forge_light_or_device_tag(grp, tname))
			cat_id = _forge_cat_scenery;

		if (cat_id < 0 || data_addr < tag_base || (data_addr - tag_base) + 0x38u > tag_sz)
			continue;

		/* Verify the object has a valid 'mode' model reference in the Xbox map (or is a point-light scenery tag) */
		if (!is_allowed_forge_light_or_device_tag(grp, tname))
		{
			uint32_t obj_off = data_addr - tag_base;
			int32_t mod_idx = *(int32_t const *)(tag_buf + obj_off + 0x34);
			if (mod_idx == -1 || (mod_idx & 0xFFFF) >= tag_count)
				continue;
		}

		grp_str[0] = (char)((grp >> 24) & 0xFF);
		grp_str[1] = (char)((grp >> 16) & 0xFF);
		grp_str[2] = (char)((grp >> 8) & 0xFF);
		grp_str[3] = (char)(grp & 0xFF);
		grp_str[4] = '\0';

		if (xbox_manifest_fp)
			fprintf(xbox_manifest_fp, "%s|%d|0|%s\n", grp_str, cat_id, tname);
		if (credits_fp)
			fprintf(credits_fp, "- `%s` (`%s`)\n", tname, grp_str);
		indexed_count++;

		if (grp == 'mach' && _stricmp(tname, "levels\\a30\\devices\\beam emitter\\beam emitter") == 0)
		{
			char const *red_tname = "levels\\a30\\devices\\beam emitter\\beam emitter red";
			if (xbox_manifest_fp)
				fprintf(xbox_manifest_fp, "mach|%d|0|%s\n", cat_id, red_tname);
			if (credits_fp)
				fprintf(credits_fp, "- `%s` (`mach`)\n", red_tname);
			indexed_count++;
		}
	}

	if (credits_fp)
		fprintf(credits_fp, "\n");
	free(tag_buf);
	(void)legacy_manifest_fp;
	return indexed_count;
}

static int forge_rip_maps_folder_to_cached_tags(char const *maps_dir)
{
	char pattern[512];
	WIN32_FIND_DATAA fd;
	HANDLE h;
	FILE *xbox_manifest_fp;
	FILE *credits_fp;
	char map_files[64][512];
	int map_file_count = 0;
	int m;
	int ripped_maps = 0;
	int total_assets = 0;

	snprintf(pattern, sizeof(pattern), "%s\\*.map", maps_dir);
	h = FindFirstFileA(pattern, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return 0;

	do
	{
		char full_path[512];
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		if (_stricmp(fd.cFileName, "bitmaps.map") == 0 ||
			_stricmp(fd.cFileName, "sounds.map") == 0 ||
			_stricmp(fd.cFileName, "loc.map") == 0 ||
			_stricmp(fd.cFileName, "ui.map") == 0 ||
			_stricmp(fd.cFileName, "forgeasset.map") == 0 ||
			_stricmp(fd.cFileName, "customforgeasset.map") == 0 ||
			_stricmp(fd.cFileName, "_temp_xbox_decomp.map") == 0)
		{
			continue;
		}
		snprintf(full_path, sizeof(full_path), "%s/%s", maps_dir, fd.cFileName);
		/* Strictly pull default tags from Xbox maps (version == 5) ONLY! */
		if (!is_xbox_version5_map_file(full_path))
			continue;
		if (map_file_count < 64)
			snprintf(map_files[map_file_count++], sizeof(map_files[0]), "%s", full_path);
	} while (FindNextFileA(h, &fd));
	CloseHandle(h);

	if (map_file_count <= 0)
		return 0;

	remove("OpenCE Forge/cached_tags/catalog.txt");
	xbox_manifest_fp = fopen("OpenCE Forge/cached_tags/xbox_catalog.txt", "wb");
	if (!xbox_manifest_fp)
		return 0;
	fprintf(xbox_manifest_fp, "# OpenCE : Forge - Native Xbox Maps Tag Catalog (Built from Xbox maps in %s)\n", maps_dir);

	credits_fp = fopen("OpenCE Forge/credits.md", "wb");
	if (credits_fp)
	{
		fprintf(credits_fp, "# OpenCE : Forge - Native Xbox Map Tag Catalog\n\n");
		fprintf(credits_fp, "Indexed natively from Xbox maps in `%s/`.\n\n", maps_dir);
	}

	forge_xbox_loading_screen_begin();
	for (m = 0; m < map_file_count; m++)
	{
		int count_from_map;
		s_forge_loading_map_base = (float)m / (float)map_file_count;
		s_forge_loading_map_span = 1.0f / (float)map_file_count;
		forge_xbox_loading_screen_update(s_forge_loading_map_base + 0.05f * s_forge_loading_map_span);
		count_from_map = forge_index_xbox_map_file_to_catalog(
			map_files[m], xbox_manifest_fp, NULL, credits_fp);
		forge_xbox_loading_screen_update((float)(m + 1) / (float)map_file_count);
		if (count_from_map > 0)
		{
			ripped_maps++;
			total_assets += count_from_map;
		}
	}
	forge_xbox_loading_screen_end();

	{
		char donor_cache[512];
		platform_translate_path("z:\\cache_donor.map", donor_cache, sizeof(donor_cache));
		remove(donor_cache);
	}

	if (credits_fp)
		fclose(credits_fp);
	fclose(xbox_manifest_fp);
	return (ripped_maps > 0 && total_assets > 0) ? total_assets : 0;
}

static void forge_delete_files_matching(char const *dir_path, char const *pattern)
{
	char search_pat[512];
	WIN32_FIND_DATAA fd;
	HANDLE h;
	snprintf(search_pat, sizeof(search_pat), "%s\\%s", dir_path, pattern);
	h = FindFirstFileA(search_pat, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			char full[512];
			snprintf(full, sizeof(full), "%s/%s", dir_path, fd.cFileName);
			remove(full);
		}
	} while (FindNextFileA(h, &fd));
	CloseHandle(h);
}

static void close_runtime_xbox_donor(void);
static void forge_ensure_local_tag_cache_built(void);

int forge_reset_stock_tag_cache(void)
{
	char z_path[512];

	close_runtime_xbox_donor();
	remove("OpenCE Forge/cached_tags/xbox_catalog.txt");
	remove("OpenCE Forge/cached_tags/catalog.txt");
	remove("OpenCE Forge/credits.md");
	forge_delete_files_matching("OpenCE Forge/cached_tags/xbox_cache", "*.map");
	RemoveDirectoryA("OpenCE Forge/cached_tags/xbox_cache");
	forge_delete_files_matching("OpenCE Forge/cached_tags/markers", "m_*.*");

	/* Remove only z:\cache_donor.map; preserve z:\cache000..005.map (active ui.map / engine caches) */
	platform_translate_path("z:\\cache_donor.map", z_path, sizeof(z_path));
	remove(z_path);

	CreateDirectoryA("OpenCE Forge", NULL);
	CreateDirectoryA("OpenCE Forge/cached_tags", NULL);
	CreateDirectoryA("OpenCE Forge/cached_tags/xbox_cache", NULL);
	forge_ensure_local_tag_cache_built();
	forge_runtime_reload_catalogs();
	forge_set_status("Stock Tag Cache Rebuilt!");
	return 1;
}

int forge_clear_custom_tag_cache(void)
{
	/* Delete all custom cached tag bundles in the Faber Tag Vault */
	{
		long safety = 0;
		port_tag_profiles_scan();
		while (port_tag_profiles_count() > 0 && safety++ < 128)
		{
			if (!port_tag_profiles_delete(0))
				break;
		}
	}

	forge_runtime_reload_catalogs();
	forge_set_status("Cleared Custom Tag Cache!");
	return 1;
}

static void forge_ensure_local_tag_cache_built(void)
{
	{
		FILE *f = fopen("OpenCE Forge/cached_tags/xbox_catalog.txt", "r");
		if (f)
		{
			char line[512];
			int has_base_tags = 0;
			int has_lights_and_door = 0;
			int has_wraith = 0;
			while (fgets(line, sizeof(line), f))
			{
				if (strstr(line, "|0|") != NULL)
					has_base_tags = 1;
				if (strstr(line, "beam emitter red") != NULL)
					has_lights_and_door = 1;
				if (strstr(line, "vehicles\\wraith\\wraith") != NULL)
					has_wraith = 1;
			}
			fclose(f);
			if (has_base_tags && has_lights_and_door && has_wraith)
			{
				FILE *xc = fopen("OpenCE Forge/cached_tags/xbox_cache/a30.map", "rb");
				if (xc)
				{
					fclose(xc);
					return;
				}
			}
		}
	}

	CreateDirectoryA("OpenCE Forge", NULL);
	CreateDirectoryA("OpenCE Forge/cached_tags", NULL);

	{
		int extracted_total = forge_rip_maps_folder_to_cached_tags("maps");
		if (extracted_total <= 0)
		{
			char translated_maps[1024];
			platform_translate_path("d:\\maps", translated_maps, sizeof(translated_maps));
			if (translated_maps[0] && _stricmp(translated_maps, "maps") != 0)
				extracted_total = forge_rip_maps_folder_to_cached_tags(translated_maps);
		}
		if (extracted_total <= 0)
			extracted_total = forge_rip_maps_folder_to_cached_tags("assets/maps");
		if (extracted_total <= 0)
			extracted_total = forge_rip_maps_folder_to_cached_tags("../assets/maps");
	}
}

void forge_runtime_init(void)
{
	if (g_forge.initialized)
	{
		forge_ensure_local_tag_cache_built();
		return;
	}
	memset(&g_forge, 0, sizeof(g_forge));
	g_forge.initialized = 1;
	g_forge.mode = _forge_mode_play;
	g_forge.active_modal = _forge_modal_none;
	g_forge.current_category = _forge_cat_vehicles;
	g_forge.active_weld_group = 1;
	g_forge.snap_mode = _forge_snap_15_deg;
	g_forge.cam_speed = 12.0f;
	g_forge.grabbed_object_index = NONE;
	g_forge.grabbed_live_index = -1;
	g_forge.hovered_object_index = NONE;
	g_forge.hovered_live_index = -1;
	g_forge.grab_distance = 5.0f;
	snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "forge_custom_arena");
	g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);

	CreateDirectoryA("OpenCE Forge", NULL);
	CreateDirectoryA("OpenCE Forge/cached_tags", NULL);
	CreateDirectoryA("OpenCE Forge/cached_tags/xbox_cache", NULL);
	RemoveDirectoryA("OpenCE Forge/custom_tags");
	RemoveDirectoryA("custom_tags");
	forge_ensure_local_tag_cache_built();
	forge_runtime_reload_catalogs();
}

/* ============================================================================
 * NATIVE XBOX & CUSTOM EDITION MAP TAG CACHE LOADER
 * Tags are loaded natively from extracted map tag bundles and Xbox map caches
 * (using OpenCE's exact tag schema & hardware vertex/triangle/texture registration).
 * ============================================================================ */

#define FORGE_MAX_HW_PARTS       48
#define FORGE_MAX_DYNAMIC_TAGS   1024
#define FORGE_MAX_PHYS_ALLOCS    8192
#define FORGE_MAX_DYN_SOUND_PERMS 4096

struct forge_hw_part
{
	struct vertex_buffer vertex_buffer;
	struct triangle_buffer triangle_buffer;
	char shader_hint[128];
	float u_scale;
	float v_scale;
};

struct forge_dynamic_tag_entry
{
	uint32_t group_tag;
	char tag_path[256];
	long definition_index;
	float bounding_radius;
	int part_count;
	struct forge_hw_part parts[FORGE_MAX_HW_PARTS];
};

static struct
{
	void *saved_base_instances;
	long saved_base_count;
	void *allocated_instances;
	int count;
	struct forge_dynamic_tag_entry entries[FORGE_MAX_DYNAMIC_TAGS];
} s_dyn_tags;

static int has_active_dynamic_tags(void)
{
	return s_dyn_tags.count > 0;
}

static struct
{
	int count;
	void *ptrs[FORGE_MAX_PHYS_ALLOCS];
} s_forge_phys_allocs;

static struct
{
	int count;
	int cap;
	void **ptrs;
} s_forge_heap_allocs;

#define FORGE_TAG_STREAM_MAX_SESSION_BYTES (24u * 1024u * 1024u)
struct forge_staged_ephemeral_tag
{
	uint8_t active;
	uint8_t committed;
	uint8_t bundle_format;
	uint8_t cat_id;
	uint32_t expected_hash;
	uint32_t root_group;
	char tag_path[256];
	char clean_disp[64];
	uint8_t *raw_bytes;
	uint32_t raw_size;
};
static struct forge_staged_ephemeral_tag s_staged_ephemeral_tags[32];
static uint32_t s_streamed_phys_bytes = 0;

static void *forge_session_calloc(size_t bytes)
{
	size_t alloc_sz = (bytes > 0u) ? bytes : 1u;
	void *p = calloc(1, alloc_sz);
	if (!p)
		return NULL;
	if (s_forge_heap_allocs.count >= s_forge_heap_allocs.cap)
	{
		int new_cap = (s_forge_heap_allocs.cap > 0) ? (s_forge_heap_allocs.cap * 2) : 512;
		void **grown = (void **)realloc(s_forge_heap_allocs.ptrs, (size_t)new_cap * sizeof(void *));
		if (grown)
		{
			s_forge_heap_allocs.ptrs = grown;
			s_forge_heap_allocs.cap = new_cap;
		}
	}
	if (s_forge_heap_allocs.count < s_forge_heap_allocs.cap)
		s_forge_heap_allocs.ptrs[s_forge_heap_allocs.count++] = p;
	return p;
}

static struct
{
	int count;
	struct sound_permutation const *perms[FORGE_MAX_DYN_SOUND_PERMS];
} s_dyn_sound_perms;

static void *forge_phys_alloc(size_t bytes)
{
	void *p;
	size_t alloc_sz = (bytes > 64u) ? bytes : 64u;
	p = XPhysicalAlloc((unsigned long)alloc_sz, (unsigned long)-1, 0, PAGE_READWRITE);
	if (p)
	{
		memset(p, 0, alloc_sz);
		if (s_forge_phys_allocs.count < FORGE_MAX_PHYS_ALLOCS)
			s_forge_phys_allocs.ptrs[s_forge_phys_allocs.count++] = p;
		return p;
	}
	return calloc(1, alloc_sz);
}

static void forge_register_dyn_sound_perm(struct sound_permutation const *perm)
{
	int i;
	if (!perm || !perm->samples.address || perm->samples.size <= 0)
		return;
	for (i = 0; i < s_dyn_sound_perms.count; i++)
	{
		if (s_dyn_sound_perms.perms[i] == perm)
			return;
	}
	if (s_dyn_sound_perms.count < FORGE_MAX_DYN_SOUND_PERMS)
		s_dyn_sound_perms.perms[s_dyn_sound_perms.count++] = perm;
}

boolean mod_sounds_read(
	struct sound_permutation const *permutation,
	void *buffer)
{
	int i;
	if (permutation && buffer)
	{
		for (i = 0; i < s_dyn_sound_perms.count; i++)
		{
			if (s_dyn_sound_perms.perms[i] == permutation &&
				permutation->samples.address &&
				permutation->samples.size > 0)
			{
				memcpy(buffer, permutation->samples.address, (size_t)permutation->samples.size);
				return TRUE;
			}
		}
	}
	return orig_mod_sounds_read(permutation, buffer);
}

static long s_monitor_bipd_handle = NONE;
static int s_monitor_resolved = 0;
static int s_local_monitor_drawn_this_frame = 0;
static int s_last_preview_chosen_idx = -1;
static unsigned long s_last_preview_cursor_ms = 0;

struct forge_light_preset_cache_entry
{
	long base_ligh_idx;
	uint8_t color_preset;
	uint8_t intensity_preset;
	uint8_t radius_preset;
	uint8_t pad;
	long cloned_ligh_idx;
};

static struct forge_light_preset_cache_entry s_light_preset_cache[128];
static int s_light_preset_cache_count = 0;
static void const *s_light_preset_cache_scen = NULL;

#define FORGE_MAX_DONOR_CACHE_SLOTS 12

static struct
{
	char path[512];
	FILE *fp;
	uint8_t *tag_buf;
	uint32_t tag_off;
	uint32_t tag_sz;
	unsigned long last_use_seq;
} s_runtime_xbox_donors[FORGE_MAX_DONOR_CACHE_SLOTS];

static unsigned long s_runtime_xbox_donor_seq = 0;

static void close_runtime_xbox_donor(void)
{
	int i;
	for (i = 0; i < FORGE_MAX_DONOR_CACHE_SLOTS; i++)
	{
		if (s_runtime_xbox_donors[i].fp)
			fclose(s_runtime_xbox_donors[i].fp);
		if (s_runtime_xbox_donors[i].tag_buf)
			free(s_runtime_xbox_donors[i].tag_buf);
	}
	memset(s_runtime_xbox_donors, 0, sizeof(s_runtime_xbox_donors));
	s_runtime_xbox_donor_seq = 0;
}

static void forge_ui_close_map_tag_browser(void);

void forge_runtime_on_map_unload(void)
{
	int i;

	forge_ui_close_map_tag_browser();
	close_runtime_xbox_donor();

	/* Dispose injected Custom Edition model buffers FIRST while their gbxmodel
	 * part definitions in s_forge_phys_allocs are still mapped in memory! */
	custom_edition_injected_models_dispose();

	for (i = 0; i < s_forge_phys_allocs.count; i++)
	{
		if (s_forge_phys_allocs.ptrs[i])
			XPhysicalFree(s_forge_phys_allocs.ptrs[i]);
	}
	s_forge_phys_allocs.count = 0;
	for (i = 0; i < s_forge_heap_allocs.count; i++)
	{
		if (s_forge_heap_allocs.ptrs[i])
			free(s_forge_heap_allocs.ptrs[i]);
	}
	free(s_forge_heap_allocs.ptrs);
	s_forge_heap_allocs.ptrs = NULL;
	s_forge_heap_allocs.count = 0;
	s_forge_heap_allocs.cap = 0;
	for (i = 0; i < 32; i++)
	{
		if (s_staged_ephemeral_tags[i].raw_bytes)
			free(s_staged_ephemeral_tags[i].raw_bytes);
	}
	memset(s_staged_ephemeral_tags, 0, sizeof(s_staged_ephemeral_tags));
	s_streamed_phys_bytes = 0;
	s_dyn_sound_perms.count = 0;
	s_monitor_bipd_handle = NONE;
	s_monitor_resolved = 0;
	s_local_monitor_drawn_this_frame = 0;
	s_last_preview_chosen_idx = -1;
	s_last_preview_cursor_ms = 0;
	s_light_preset_cache_count = 0;
	s_light_preset_cache_scen = NULL;

	if (s_dyn_tags.saved_base_instances)
	{
		cache_files_set_tag_instances(s_dyn_tags.saved_base_instances, s_dyn_tags.saved_base_count);
		s_dyn_tags.saved_base_instances = NULL;
		s_dyn_tags.saved_base_count = 0;
	}
	if (s_dyn_tags.allocated_instances)
	{
		free(s_dyn_tags.allocated_instances);
		s_dyn_tags.allocated_instances = NULL;
	}
	s_dyn_tags.count = 0;
	if (g_forge.initialized && g_forge.catalog_count > 0)
	{
		int out_c = 0;
		for (i = 0; i < g_forge.catalog_count; i++)
		{
			if (g_forge.catalog[i].source_cache_id != 3 &&
				g_forge.catalog[i].source_cache_id != 4)
				g_forge.catalog[out_c++] = g_forge.catalog[i];
		}
		g_forge.catalog_count = out_c;
	}
}

static void build_hw_part_from_arrays(
	struct forge_hw_part *dst_part,
	uint8_t *uncomp_verts,
	int32_t vtx_count,
	uint8_t const *idx_bytes,
	int32_t tri_count,
	char const *shader_hint,
	float scale_factor,
	float z_offset)
{
	uint32_t uncomp_sz;
	uint32_t comp_sz;
	uint32_t idx_sz;
	void *hw_verts;
	void *hw_tris;
	int v, c;

	/* Strip trailing 0xFFFF padding indices (from .gbxmodel 3-index model_triangle blocks) */
	if (idx_bytes && vtx_count >= 3)
	{
		uint16_t const *raw_idx = (uint16_t const *)idx_bytes;
		while (tri_count > 1 && raw_idx[tri_count + 1] >= (uint16_t)vtx_count)
			tri_count--;
	}
	if (vtx_count < 3 || tri_count < 1)
		return;

	uncomp_sz = (uint32_t)vtx_count * 68u;
	comp_sz = (uint32_t)vtx_count * 32u;
	idx_sz = ((uint32_t)tri_count + 2u) * 2u;

	for (v = 0; v < vtx_count; v++)
	{
		uint8_t *vp = uncomp_verts + (uint32_t)v * 68u;
		if (scale_factor != 1.0f || z_offset != 0.0f)
		{
			*(float *)(vp + 0) *= scale_factor;
			*(float *)(vp + 4) *= scale_factor;
			*(float *)(vp + 8) = (*(float *)(vp + 8) * scale_factor) + z_offset;
		}
		for (c = 0; c < 9; c++)
		{
			float val = *(float *)(vp + 12 + c * 4);
			if (!(val >= -1.0f && val <= 1.0f))
				*(float *)(vp + 12 + c * 4) = (c % 3 == 0) ? 1.0f : 0.0f;
		}
		*(int16_t *)(vp + 56) = 0;
		*(int16_t *)(vp + 58) = 0;
		*(float *)(vp + 60) = 1.0f;
		*(float *)(vp + 64) = 0.0f;
	}

	hw_verts = XPhysicalAlloc(comp_sz, (unsigned long)-1, 0, PAGE_READWRITE);
	if (!hw_verts)
		hw_verts = malloc(comp_sz);
	hw_tris = XPhysicalAlloc(idx_sz, (unsigned long)-1, 0, PAGE_READWRITE);
	if (!hw_tris)
		hw_tris = malloc(idx_sz);
	if (!hw_verts || !hw_tris)
		return;

	memset(dst_part, 0, sizeof(*dst_part));
	dst_part->u_scale = 1.0f;
	dst_part->v_scale = 1.0f;
	if (shader_hint && shader_hint[0])
	{
		char const *bar1 = strchr(shader_hint, '|');
		char const *bar2 = bar1 ? strchr(bar1 + 1, '|') : NULL;
		if (bar2)
		{
			float us = 1.0f, vs = 1.0f;
			size_t copy_len = (size_t)(bar2 - shader_hint);
			if (copy_len >= sizeof(dst_part->shader_hint))
				copy_len = sizeof(dst_part->shader_hint) - 1;
			memcpy(dst_part->shader_hint, shader_hint, copy_len);
			dst_part->shader_hint[copy_len] = '\0';
			if (sscanf(bar2 + 1, "%f,%f", &us, &vs) == 2)
			{
				if (us > 0.0001f && us < 10000.0f)
					dst_part->u_scale = us;
				if (vs > 0.0001f && vs < 10000.0f)
					dst_part->v_scale = vs;
			}
		}
		else
		{
			snprintf(dst_part->shader_hint, sizeof(dst_part->shader_hint), "%s", shader_hint);
		}
	}

	rasterizer_geometry_compress_vertices(
		_rasterizer_vertex_type_model_uncompressed,
		vtx_count,
		hw_verts,
		(long)comp_sz,
		uncomp_verts,
		(long)uncomp_sz);
	memcpy(hw_tris, idx_bytes, idx_sz);
	{
		uint16_t *tris_u16 = (uint16_t *)hw_tris;
		int32_t total_idx = tri_count + 2;
		int32_t k;
		for (k = 0; k < total_idx; k++)
		{
			if (tris_u16[k] >= (uint16_t)vtx_count)
				tris_u16[k] = (k > 0 && tris_u16[k - 1] < (uint16_t)vtx_count) ? tris_u16[k - 1] : 0u;
		}
	}

	rasterizer_vertex_buffer_new(
		&dst_part->vertex_buffer,
		_rasterizer_vertex_type_model_compressed,
		vtx_count,
		hw_verts,
		(long)comp_sz);

	rasterizer_triangle_buffer_new(
		&dst_part->triangle_buffer,
		_triangle_buffer_type_precompiled_strip,
		tri_count,
		hw_tris);
}

static long append_runtime_tag_instance(
	long group_tag,
	long parent0,
	long parent1,
	char const *tag_path,
	void *base_address)
{
	long existing_count = 0;
	struct forge_cache_file_tag_instance *instances;
	struct forge_cache_file_tag_instance *grown;
	long max_salt = 0xE174;
	long i, new_abs_idx, new_tag_index;
	char *name_copy;

	instances = (struct forge_cache_file_tag_instance *)cache_files_tag_instances(&existing_count);
	if (!instances || existing_count <= 0 || existing_count >= 65000)
		return NONE;

	if (!s_dyn_tags.saved_base_instances)
	{
		s_dyn_tags.saved_base_instances = instances;
		s_dyn_tags.saved_base_count = existing_count;
	}

	grown = (struct forge_cache_file_tag_instance *)malloc((size_t)(existing_count + 1) * sizeof(*grown));
	name_copy = (char *)malloc(strlen(tag_path) + 1);
	if (!grown || !name_copy)
	{
		free(grown);
		free(name_copy);
		return NONE;
	}
	memcpy(grown, instances, (size_t)existing_count * sizeof(*grown));
	strcpy(name_copy, tag_path);

	for (i = 0; i < existing_count; i++)
	{
		if (instances[i].tag_index != NONE)
		{
			long s = (long)(((unsigned long)instances[i].tag_index >> 16) & 0x7FFFu);
			if (s > max_salt)
				max_salt = s;
		}
	}

	new_abs_idx = existing_count;
	new_tag_index = ((max_salt + 1) << 16) | (new_abs_idx & 0xFFFF);

	memset(&grown[new_abs_idx], 0, sizeof(grown[new_abs_idx]));
	grown[new_abs_idx].group_tag = group_tag;
	grown[new_abs_idx].parent_group_tags[0] = parent0;
	grown[new_abs_idx].parent_group_tags[1] = parent1;
	grown[new_abs_idx].tag_index = new_tag_index;
	grown[new_abs_idx].name = name_copy;
	grown[new_abs_idx].base_address = base_address;

	cache_files_set_tag_instances(grown, existing_count + 1);
	if (s_dyn_tags.allocated_instances)
		free(s_dyn_tags.allocated_instances);
	s_dyn_tags.allocated_instances = grown;

	return new_tag_index;
}

uint32_t forge_get_base_map_tag_count(void)
{
	long existing_count = 0;
	if (s_dyn_tags.saved_base_count > 0)
		return (uint32_t)s_dyn_tags.saved_base_count;
	if (cache_files_tag_instances(&existing_count) && existing_count > 0)
		return (uint32_t)existing_count;
	return 0;
}

static int is_valid_cyborg_driver_seat_label(char const *label)
{
	if (!label || !label[0])
		return 0;
	if (_stricmp(label, "W-driver") == 0 ||
		_stricmp(label, "G-driver") == 0 ||
		_stricmp(label, "B-driver") == 0 ||
		_stricmp(label, "S-driver") == 0 ||
		_stricmp(label, "R-driver") == 0 ||
		_stricmp(label, "scorpion-driver") == 0 ||
		_stricmp(label, "CT-driver") == 0)
		return 1;
	return 0;
}

static void patch_vehicle_tag_for_player_driving(long vehi_tag_idx, char const *tag_path)
{
	struct unit_definition *ud;
	int is_wraith = 0;

	if (vehi_tag_idx == NONE)
		return;

	ud = (struct unit_definition *)tag_get('vehi', vehi_tag_idx);
	if (ud)
	{
		if ((tag_path && (strstr(tag_path, "wraith") || strstr(tag_path, "Wraith"))) ||
			(ud->object.collision_model.name && strstr(ud->object.collision_model.name, "wraith")))
		{
			is_wraith = 1;
		}
		ud->unit.flags |= FLAG(_unit_has_entrance_points_inside_bounding_sphere_bit);
		if (ud->unit.seats.count > 0 && ud->unit.seats.address)
		{
			struct unit_seat *seat0 = (struct unit_seat *)ud->unit.seats.address;
			seat0->flags &= ~FLAG(_unit_seat_invisible_bit);
			seat0->flags |= FLAG(_unit_seat_driver_bit) |
							FLAG(_unit_seat_third_person_camera_bit) |
							FLAG(_unit_seat_third_person_on_enter_bit);
			if (is_wraith)
				seat0->flags |= FLAG(_unit_seat_gunner_bit);
			if (!is_valid_cyborg_driver_seat_label(seat0->label))
				snprintf(seat0->label, sizeof(seat0->label), "W-driver");
		}
		if (is_wraith && ud->object.collision_model.index != NONE)
		{
			uint8_t *cb = (uint8_t *)tag_get('coll', ud->object.collision_model.index);
			if (cb)
			{
				int32_t m_cnt = *(int32_t const *)(cb + 0x220u);
				uint8_t *m_ptr = *(uint8_t **)(cb + 0x224u);
				int32_t r_cnt = *(int32_t const *)(cb + 0x22Cu);
				uint8_t *r_ptr = *(uint8_t **)(cb + 0x230u);
				int32_t k;

				*(float *)(cb + 0x04u) = 0.0f; /* maximum_body_vitality = 0 (cannot take body damage) */
				*(float *)(cb + 0x08u) = 0.0f; /* body_system_shock = 0 */
				*(float *)(cb + 0x4Cu) = 0.0f; /* body_damaged_threshold = 0 */
				*(int32_t *)(cb + 0x60u) = NONE; /* body_depleted_effect.index = NONE */
				*(float *)(cb + 0x64u) = 0.0f; /* body_destroyed_threshold = 0 */
				*(int32_t *)(cb + 0x74u) = NONE; /* body_destroyed_effect.index = NONE */
				*(float *)(cb + 0x78u) = 0.0f; /* maximum_shield_vitality = 0 */

				if (m_ptr && m_cnt > 0 && m_cnt <= 64)
				{
					for (k = 0; k < m_cnt; k++)
					{
						uint8_t *mp = m_ptr + (uint32_t)k * 0x48u;
						*(float *)(mp + 0x2Cu) = 0.0f; /* shield_damage_multiplier */
						*(float *)(mp + 0x3Cu) = 0.0f; /* body_damage_multiplier */
					}
				}
				if (r_ptr && r_cnt > 0 && r_cnt <= 64)
				{
					for (k = 0; k < r_cnt; k++)
					{
						uint8_t *rp = r_ptr + (uint32_t)k * 0x54u;
						*(uint32_t *)(rp + 0x20u) &= ~0x0Eu; /* clear forces_object_to_die / dies_when_object_dies / dies_in_water */
						*(float *)(rp + 0x28u) = 0.0f; /* damage_threshold = 0 (cannot be destroyed) */
						*(int32_t *)(rp + 0x50u) = NONE; /* destroyed_effect.index = NONE */
					}
				}
			}
		}
	}
}

static void patch_light_definition_buffer(uint8_t *lbuf)
{
	if (!lbuf)
		return;
	/* Bit 0 (_light_definition_dynamic_bit = 1) must be set and radius_modifier_upper_bound > 0
	 * for Halo's object_lights.c to illuminate surrounding world geometry and objects! */
	*(uint32_t *)(lbuf + 0x00) |= 1u;
	if (*(float const *)(lbuf + 0x04) < 3.5f)
		*(float *)(lbuf + 0x04) = 4.5f;
	if (*(float const *)(lbuf + 0x08) == 0.0f && *(float const *)(lbuf + 0x0C) == 0.0f)
	{
		*(float *)(lbuf + 0x08) = 1.0f;
		*(float *)(lbuf + 0x0C) = 1.0f;
	}
	if (*(float const *)(lbuf + 0x4C) == 0.0f &&
		*(float const *)(lbuf + 0x50) == 0.0f &&
		*(float const *)(lbuf + 0x54) == 0.0f)
	{
		*(float *)(lbuf + 0x38) = 1.0f;
		*(float *)(lbuf + 0x3C) = 1.0f;
		*(float *)(lbuf + 0x40) = 1.0f;
		*(float *)(lbuf + 0x44) = 1.0f;
		*(float *)(lbuf + 0x48) = 1.0f;
		*(float *)(lbuf + 0x4C) = 1.0f;
		*(float *)(lbuf + 0x50) = 1.0f;
		*(float *)(lbuf + 0x54) = 1.0f;
	}
}

extern struct data_array *light_data;

struct forge_engine_light_datum_view
{
	struct datum_header header;
	uint16_t flags;
	long definition_index;
	long rasterizer_light_index;
	long marker;
	long cluster_reference;
	real_rgb_color color;
	uint8_t reserved20[0xC];
	long object_index;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real radius;
	long parent_light_index;
	short attachment_marker_index;
	short function_index;
	short color_function_index;
};

static float forge_clamp01(float v)
{
	if (v < 0.0f)
		return 0.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
}

static void forge_get_light_preset_rgb(uint8_t preset, float *r, float *g, float *b)
{
	switch (preset)
	{
	case 1: *r = 1.00f; *g = 1.00f; *b = 1.00f; break; /* White */
	case 2: *r = 1.00f; *g = 0.16f; *b = 0.12f; break; /* Red */
	case 3: *r = 0.18f; *g = 0.48f; *b = 1.00f; break; /* Blue */
	case 4: *r = 0.18f; *g = 1.00f; *b = 0.28f; break; /* Green */
	case 5: *r = 1.00f; *g = 0.68f; *b = 0.14f; break; /* Amber */
	case 6: *r = 0.76f; *g = 0.22f; *b = 1.00f; break; /* Purple */
	case 7: *r = 0.15f; *g = 0.92f; *b = 1.00f; break; /* Cyan */
	default: *r = 1.00f; *g = 0.96f; *b = 0.88f; break;
	}
}

static float forge_get_light_intensity_mult(uint8_t preset)
{
	switch (preset)
	{
	case 1: return 0.25f;
	case 2: return 0.50f;
	case 3: return 1.50f;
	case 4: return 2.00f;
	case 5: return 3.00f;
	default: return 1.00f;
	}
}

static float forge_get_light_radius_override(uint8_t preset)
{
	switch (preset)
	{
	case 1: return 2.0f;
	case 2: return 4.0f;
	case 3: return 8.0f;
	case 4: return 12.0f;
	case 5: return 20.0f;
	default: return 0.0f;
	}
}

static long forge_get_or_clone_light_preset_tag(
	long base_ligh_idx,
	uint8_t color_preset,
	uint8_t intensity_preset,
	uint8_t radius_preset)
{
	void const *cur_scen = global_scenario_try_and_get();
	uint8_t const *src_lbuf = NULL;
	uint8_t *dst_lbuf;
	char clone_name[128];
	float r = 1.0f, g = 0.96f, b = 0.88f;
	float mult = forge_get_light_intensity_mult(intensity_preset);
	float rad = forge_get_light_radius_override(radius_preset);
	long new_idx;
	int i;

	if (cur_scen != s_light_preset_cache_scen)
	{
		s_light_preset_cache_scen = cur_scen;
		s_light_preset_cache_count = 0;
	}

	for (i = 0; i < s_light_preset_cache_count; i++)
	{
		if (s_light_preset_cache[i].base_ligh_idx == base_ligh_idx &&
			s_light_preset_cache[i].color_preset == color_preset &&
			s_light_preset_cache[i].intensity_preset == intensity_preset &&
			s_light_preset_cache[i].radius_preset == radius_preset)
		{
			return s_light_preset_cache[i].cloned_ligh_idx;
		}
	}

	if (base_ligh_idx != NONE)
		src_lbuf = (uint8_t const *)tag_get('ligh', base_ligh_idx);

	dst_lbuf = (uint8_t *)forge_session_calloc(0x104u);
	if (!dst_lbuf)
		return base_ligh_idx;

	if (src_lbuf)
	{
		memcpy(dst_lbuf, src_lbuf, 0x104u);
		patch_light_definition_buffer(dst_lbuf);
	}
	else
	{
		*(uint32_t *)(dst_lbuf + 0x00u) = 1u; /* _light_definition_dynamic_bit */
		*(float *)(dst_lbuf + 0x04u) = 6.0f;  /* radius */
		*(float *)(dst_lbuf + 0x08u) = 1.0f;  /* radius_modifier_lower_bound */
		*(float *)(dst_lbuf + 0x0Cu) = 1.0f;  /* radius_modifier_upper_bound */
		*(float *)(dst_lbuf + 0x10u) = 3.14159265f; /* falloff_angle */
		*(float *)(dst_lbuf + 0x14u) = 3.14159265f; /* cutoff_angle */
		*(float *)(dst_lbuf + 0x1Cu) = -1.0f; /* runtime_cosine_falloff_angle */
		*(float *)(dst_lbuf + 0x20u) = -1.0f; /* runtime_cosine_cutoff_angle */
		*(float *)(dst_lbuf + 0x24u) = 1.0f;  /* specular_radius_multiplier */
		*(float *)(dst_lbuf + 0x28u) = 0.0f;  /* runtime_sine_cutoff_angle */
		*(float *)(dst_lbuf + 0x38u) = 1.0f;
		*(float *)(dst_lbuf + 0x3Cu) = 1.0f;
		*(float *)(dst_lbuf + 0x40u) = 0.96f;
		*(float *)(dst_lbuf + 0x44u) = 0.88f;
		*(float *)(dst_lbuf + 0x48u) = 1.0f;
		*(float *)(dst_lbuf + 0x4Cu) = 1.0f;
		*(float *)(dst_lbuf + 0x50u) = 0.96f;
		*(float *)(dst_lbuf + 0x54u) = 0.88f;
		*(int32_t *)(dst_lbuf + 0xB8u) = NONE; /* lens_flare.index */
	}

	*(uint32_t *)(dst_lbuf + 0x00u) |= 1u;
	*(float *)(dst_lbuf + 0x08u) = 1.0f;
	*(float *)(dst_lbuf + 0x0Cu) = 1.0f;

	if (color_preset != 0)
	{
		forge_get_light_preset_rgb(color_preset, &r, &g, &b);
	}
	else
	{
		r = *(float const *)(dst_lbuf + 0x4Cu);
		g = *(float const *)(dst_lbuf + 0x50u);
		b = *(float const *)(dst_lbuf + 0x54u);
		if (r <= 0.0f && g <= 0.0f && b <= 0.0f)
		{
			r = 1.0f; g = 0.96f; b = 0.88f;
		}
	}

	if (mult <= 1.0f)
	{
		r = forge_clamp01(r * mult);
		g = forge_clamp01(g * mult);
		b = forge_clamp01(b * mult);
	}
	else
	{
		float lift = (mult - 1.0f) * 0.12f;
		r = forge_clamp01(r * mult + lift);
		g = forge_clamp01(g * mult + lift);
		b = forge_clamp01(b * mult + lift);
		*(float *)(dst_lbuf + 0x24u) = mult;
	}

	*(float *)(dst_lbuf + 0x38u) = 1.0f;
	*(float *)(dst_lbuf + 0x3Cu) = r;
	*(float *)(dst_lbuf + 0x40u) = g;
	*(float *)(dst_lbuf + 0x44u) = b;
	*(float *)(dst_lbuf + 0x48u) = 1.0f;
	*(float *)(dst_lbuf + 0x4Cu) = r;
	*(float *)(dst_lbuf + 0x50u) = g;
	*(float *)(dst_lbuf + 0x54u) = b;

	if (rad > 0.0f)
	{
		*(float *)(dst_lbuf + 0x04u) = rad;
	}
	else if (mult > 1.0f)
	{
		*(float *)(dst_lbuf + 0x04u) = *(float const *)(dst_lbuf + 0x04u) * (1.0f + (mult - 1.0f) * 0.25f);
	}

	snprintf(clone_name, sizeof(clone_name), "forge\\lights\\preset_%ld_%u_%u_%u",
		base_ligh_idx, (unsigned)color_preset, (unsigned)intensity_preset, (unsigned)radius_preset);
	new_idx = append_runtime_tag_instance('ligh', NONE, NONE, clone_name, dst_lbuf);
	if (new_idx == NONE)
		return base_ligh_idx;

	if (s_light_preset_cache_count < 128)
	{
		struct forge_light_preset_cache_entry *ce = &s_light_preset_cache[s_light_preset_cache_count++];
		ce->base_ligh_idx = base_ligh_idx;
		ce->color_preset = color_preset;
		ce->intensity_preset = intensity_preset;
		ce->radius_preset = radius_preset;
		ce->cloned_ligh_idx = new_idx;
	}
	return new_idx;
}

int forge_instance_supports_light_props(int32_t live_idx)
{
	struct forge_live_instance const *inst;
	struct object_datum const *obj;
	int i, a;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return 0;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active || inst->is_ai_unit)
		return 0;
	if (inst->group_tag != 'lifi' && inst->group_tag != 'scen' &&
		inst->group_tag != 'mach' && inst->group_tag != 'ctrl')
		return 0;
	if (inst->group_tag == 'lifi')
		return 1;
	if (inst->light_color_preset != 0 || inst->light_intensity_preset != 0 || inst->light_radius_preset != 0)
		return 1;

	if (inst->object_index != NONE)
	{
		obj = (struct object_datum const *)object_try_and_get_and_verify_type(inst->object_index, (unsigned long)-1);
		if (obj)
		{
			if (TEST_FLAG(obj->object.flags, _object_has_attached_lights_bit))
				return 1;
			for (a = 0; a < 8; a++)
			{
				if (obj->object.attachment_types[a] == 0 && obj->object.attachment_indices[a] != NONE)
					return 1;
			}
			if (obj->definition_index != NONE)
			{
				uint8_t const *obuf = (uint8_t const *)tag_get((long)inst->group_tag, obj->definition_index);
				if (obuf)
				{
					int32_t att_cnt = *(int32_t const *)(obuf + 0x140u);
					uint8_t const *att_base = *(uint8_t const *const *)(obuf + 0x144u);
					for (a = 0; a < att_cnt && att_base; a++)
					{
						if (*(uint32_t const *)(att_base + (size_t)a * 72u) == 'ligh')
							return 1;
					}
				}
			}
		}
	}

	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (g_forge.catalog[i].category == _forge_cat_lights &&
			g_forge.catalog[i].group_tag == inst->group_tag &&
			_stricmp(g_forge.catalog[i].tag_path, inst->tag_path) == 0)
		{
			return 1;
		}
	}

	if (strstr(inst->tag_path, "light") != NULL || strstr(inst->tag_path, "Light") != NULL ||
		strstr(inst->tag_path, "lamp") != NULL || strstr(inst->tag_path, "beacon") != NULL ||
		strstr(inst->tag_path, "torch") != NULL || strstr(inst->tag_path, "emitter") != NULL)
	{
		return 1;
	}
	return 0;
}

void forge_apply_instance_light_settings(int32_t live_idx)
{
	struct forge_live_instance const *inst;
	struct object_datum *obj;
	uint8_t *obuf = NULL;
	int32_t att_cnt = 0;
	uint8_t *att_base = NULL;
	int a, found_attached = 0;
	int has_override;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active || inst->object_index == NONE || !light_data)
		return;

	has_override = (inst->light_color_preset != 0 ||
		inst->light_intensity_preset != 0 ||
		inst->light_radius_preset != 0);

	obj = (struct object_datum *)object_try_and_get_and_verify_type(inst->object_index, (unsigned long)-1);
	if (!obj)
		return;

	if (obj->definition_index != NONE)
	{
		obuf = (uint8_t *)tag_get((long)inst->group_tag, obj->definition_index);
		if (!obuf)
			obuf = (uint8_t *)object_definition_get(obj->definition_index);
		if (obuf)
		{
			att_cnt = *(int32_t const *)(obuf + 0x140u);
			att_base = *(uint8_t *const *)(obuf + 0x144u);
		}
	}

	if (inst->light_color_preset != 0)
	{
		float cr, cg, cb;
		int c;
		forge_get_light_preset_rgb(inst->light_color_preset, &cr, &cg, &cb);
		for (c = 0; c < 4; c++)
		{
			obj->object.base_change_colors[c].red = cr;
			obj->object.base_change_colors[c].green = cg;
			obj->object.base_change_colors[c].blue = cb;
			obj->object.outgoing_change_colors[c] = obj->object.base_change_colors[c];
		}
	}
	else if (forge_instance_supports_light_props(live_idx))
	{
		int c;
		for (c = 0; c < 4; c++)
		{
			obj->object.base_change_colors[c].red = 1.0f;
			obj->object.base_change_colors[c].green = 1.0f;
			obj->object.base_change_colors[c].blue = 1.0f;
			obj->object.outgoing_change_colors[c] = obj->object.base_change_colors[c];
		}
	}

	for (a = 0; a < 8; a++)
	{
		if (obj->object.attachment_types[a] == 0 && obj->object.attachment_indices[a] != NONE)
		{
			long l_idx = obj->object.attachment_indices[a];
			struct forge_engine_light_datum_view *ld =
				(struct forge_engine_light_datum_view *)datum_try_and_get(light_data, l_idx);
			if (ld)
			{
				long orig_def_idx = ld->definition_index;
				short orig_color_fn = ld->color_function_index;
				found_attached = 1;

				if (att_base && ld->attachment_marker_index >= 0 && ld->attachment_marker_index < att_cnt)
				{
					uint8_t const *att = att_base + (size_t)ld->attachment_marker_index * 72u;
					if (*(uint32_t const *)(att + 0x00u) == 'ligh' && *(int32_t const *)(att + 0x0Cu) != NONE)
					{
						orig_def_idx = *(int32_t const *)(att + 0x0Cu);
						orig_color_fn = *(int16_t const *)(att + 0x34u);
						if (orig_color_fn < 0 || orig_color_fn >= 4)
							orig_color_fn = NONE;
					}
				}

				if (has_override)
				{
					long custom_def = forge_get_or_clone_light_preset_tag(
						orig_def_idx,
						inst->light_color_preset,
						inst->light_intensity_preset,
						inst->light_radius_preset);
					if (custom_def != NONE)
						ld->definition_index = custom_def;
					if (inst->light_color_preset != 0)
						ld->color_function_index = NONE;
					ld->flags |= 3u; /* _point_light_dynamic_bit | _point_light_connects_to_map_bit */
				}
				else if (orig_def_idx != NONE)
				{
					ld->definition_index = orig_def_idx;
					ld->color_function_index = orig_color_fn;
				}
				light_disconnect_from_map(l_idx);
				light_reconnect_to_map(l_idx);
			}
		}
	}

	if (!found_attached && has_override && obuf && forge_instance_supports_light_props(live_idx))
	{
		long synth_def = forge_get_or_clone_light_preset_tag(
			NONE,
			inst->light_color_preset,
			inst->light_intensity_preset,
			inst->light_radius_preset);
		if (synth_def != NONE)
		{
			int slot = -1;
			short marker_att_idx = 0;

			for (a = 0; a < 8; a++)
			{
				if (obj->object.attachment_types[a] == NONE)
				{
					slot = a;
					break;
				}
			}
			if (slot >= 0)
			{
				if (att_cnt <= 0 || !att_base)
				{
					uint8_t *new_att = (uint8_t *)forge_session_calloc(72u);
					if (new_att)
					{
						*(uint32_t *)(new_att + 0x00u) = 'ligh';
						*(int32_t *)(new_att + 0x0Cu) = synth_def;
						*(int16_t *)(new_att + 0x30u) = NONE;
						*(int16_t *)(new_att + 0x32u) = NONE;
						*(int16_t *)(new_att + 0x34u) = NONE;
						*(int32_t *)(obuf + 0x140u) = 1;
						*(uint8_t **)(obuf + 0x144u) = new_att;
						marker_att_idx = 0;
					}
					else
					{
						slot = -1;
					}
				}
				if (slot >= 0)
				{
					long new_lidx = light_new(synth_def, inst->object_index, marker_att_idx, NONE, NONE);
					if (new_lidx != NONE)
					{
						obj->object.attachment_types[slot] = 0;
						obj->object.attachment_indices[slot] = new_lidx;
						SET_FLAG(obj->object.flags, _object_has_attached_lights_bit, TRUE);
					}
				}
			}
		}
	}
}

static void patch_forge_device_and_light_tag_runtime(long tag_idx, uint32_t grp, char const *tag_path)
{
	uint8_t *obuf;
	int32_t att_cnt, a, model_idx;
	uint8_t *att_base;

	(void)tag_path;
	if (tag_idx == NONE)
		return;
	if (grp != 'scen' && grp != 'mach' && grp != 'lifi' && grp != 'ctrl')
		return;

	obuf = (uint8_t *)tag_get((long)grp, tag_idx);
	if (!obuf)
		return;

	/* Ensure scenery, devices, and light fixtures have generous bounding and render radii so Forge raycasting
	 * and long-distance frustum/LOD visibility never cull custom scenery early */
	if (*(float const *)(obuf + 0x04u) < 1.5f)
		*(float *)(obuf + 0x04u) = 1.5f;
	if (*(float const *)(obuf + 0x104u) < 8.0f)
		*(float *)(obuf + 0x104u) = 8.0f;
	if (*(float const *)(obuf + 0x104u) < *(float const *)(obuf + 0x04u) * 2.0f)
		*(float *)(obuf + 0x104u) = *(float const *)(obuf + 0x04u) * 2.0f;

	/* Patch the object's model ('mode'/'mod2') LOD cutoffs and permutation geometry_indices[0..4]
	 * so custom scenery items with missing low-LOD geometries or positive super_low pixel cutoffs
	 * render across the entire map just like base-game scenery! */
	model_idx = *(int32_t const *)(obuf + 0x34u);
	if (model_idx != NONE)
	{
		uint8_t *mbuf = (uint8_t *)tag_get('mode', model_idx);
		if (!mbuf)
			mbuf = (uint8_t *)tag_get('mod2', model_idx);
		if (mbuf)
		{
			float *cutoffs = (float *)(mbuf + 0x08u);
			int32_t reg_cnt = *(int32_t const *)(mbuf + 0xC4u);
			uint8_t *reg_base = *(uint8_t *const *)(mbuf + 0xC8u);
			int32_t geom_cnt = *(int32_t const *)(mbuf + 0xD0u);
			uint8_t *geom_base = *(uint8_t *const *)(mbuf + 0xD4u);
			int32_t r, p, lv;

			if (cutoffs[0] > cutoffs[4] && cutoffs[4] >= 0.0f)
			{
				float t0 = cutoffs[0], t1 = cutoffs[1];
				cutoffs[0] = cutoffs[4];
				cutoffs[1] = cutoffs[3];
				cutoffs[3] = t1;
				cutoffs[4] = t0;
			}
			cutoffs[0] = 0.0f; /* Never cull at super_low distance (models.c:1000) */

			if (reg_cnt > 0 && reg_cnt <= 32 && reg_base && geom_cnt > 0 && geom_base)
			{
				for (r = 0; r < reg_cnt; r++)
				{
					uint8_t *reg = reg_base + (size_t)r * 76u;
					int32_t perm_cnt = *(int32_t const *)(reg + 0x40u);
					uint8_t *perm_base = *(uint8_t *const *)(reg + 0x44u);
					if (perm_cnt <= 0 || perm_cnt > 32 || !perm_base)
						continue;
					for (p = 0; p < perm_cnt; p++)
					{
						uint8_t *perm = perm_base + (size_t)p * 88u;
						int16_t *g_indices = (int16_t *)(perm + 0x40u);
						int16_t best_g = -1;
						for (lv = 4; lv >= 0; lv--)
						{
							int16_t gi = g_indices[lv];
							if (gi >= 0 && gi < (int16_t)geom_cnt)
							{
								int32_t part_cnt = *(int32_t const *)(geom_base + (size_t)gi * 48u + 36u);
								if (part_cnt > 0)
								{
									best_g = gi;
									break;
								}
							}
						}
						if (best_g >= 0)
						{
							for (lv = 0; lv < 5; lv++)
							{
								int16_t gi = g_indices[lv];
								if (gi < 0 || gi >= (int16_t)geom_cnt ||
									*(int32_t const *)(geom_base + (size_t)gi * 48u + 36u) <= 0)
								{
									g_indices[lv] = best_g;
								}
							}
						}
					}
				}
			}
		}
	}

	if (grp == 'mach' && tag_path &&
		(_stricmp(tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter") == 0 ||
		 _stricmp(tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0))
	{
		*(float *)(obuf + 0x04u) = 65.0f;  /* bounding_radius */
		*(float *)(obuf + 0x10u) = 0.0f;   /* bounding_offset.z */
		*(float *)(obuf + 0x104u) = 65.0f; /* render_bounding_radius */
	}

	/* Patch attached 'ligh' tags so placed lights, spotlights, and point lights cast dynamic illumination */
	att_cnt = *(int32_t const *)(obuf + 0x140u);
	att_base = *(uint8_t *const *)(obuf + 0x144u);
	for (a = 0; a < att_cnt && att_base; a++)
	{
		uint8_t *att = att_base + (size_t)a * 72u;
		uint32_t agrp = *(uint32_t const *)(att + 0x00u);
		int32_t aidx = *(int32_t const *)(att + 0x0Cu);
		if (agrp == 'ligh' && aidx != NONE)
		{
			uint8_t *lbuf = (uint8_t *)tag_get('ligh', aidx);
			if (lbuf)
				patch_light_definition_buffer(lbuf);
		}
	}

	/* Configure 'mach' doors to open/close automatically when players or vehicles approach */
	if (grp == 'mach')
	{
		int16_t mtype = *(int16_t const *)(obuf + 0x290u);
		if (mtype == 0 /* _machine_type_door */ ||
			(tag_path && (strstr(tag_path, "door") != NULL || strstr(tag_path, "Door") != NULL || strstr(tag_path, "DOOR") != NULL)))
		{
			*(int16_t *)(obuf + 0x290u) = 0;    /* _machine_type_door */
			*(float *)(obuf + 0x21Cu) = 2.5f;   /* device.automatic_activation_radius */
			*(uint16_t *)(obuf + 0x292u) |= 0x0003u; /* _machine_is_pathfinding_obstacle_bit | _machine_is_not_pathfinding_obstacle_when_open_bit */
			*(float *)(obuf + 0x294u) = 3.0f;   /* machine.door_open_time */
			*(int32_t *)(obuf + 0x320u) = 90;   /* machine.runtime_door_open_ticks (3.0f * 30) */
		}
	}
}

static long find_native_or_mp_variant_tag(uint32_t group_tag, char const *tag_path)
{
	long idx;
	long i;

	idx = tag_loaded((long)group_tag, tag_path);
	if (idx != NONE)
	{
		if (group_tag == 'vehi')
			patch_vehicle_tag_for_player_driving(idx, tag_path);
		else if (group_tag == 'scen' || group_tag == 'mach' || group_tag == 'lifi' || group_tag == 'ctrl')
			patch_forge_device_and_light_tag_runtime(idx, group_tag, tag_path);
		return idx;
	}

	/* Map between SP and MP vehicle/weapon names natively present in MP maps */
	if (group_tag == 'vehi')
	{
		static struct { char const *a; char const *b; } const vehi_pairs[] = {
			{ "vehicles\\warthog\\warthog", "vehicles\\warthog\\mp_warthog" },
			{ "vehicles\\ghost\\ghost", "vehicles\\ghost\\ghost_mp" },
			{ "vehicles\\scorpion\\scorpion", "vehicles\\scorpion\\scorpion_mp" },
			{ "vehicles\\banshee\\banshee", "vehicles\\banshee\\banshee_mp" },
			{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c_gun_turret\\c_gun_turret_mp" },
			{ "vehicles\\c gun turret\\c gun turret", "vehicles\\c gun turret\\c gun turret_mp" },
			{ "vehicles\\c_gun_turret\\c_gun_turret", "vehicles\\c gun turret\\c gun turret_mp" },
			{ "vehicles\\c gun turret\\c gun turret", "vehicles\\c_gun_turret\\c_gun_turret_mp" }
		};
		for (i = 0; i < (long)(sizeof(vehi_pairs) / sizeof(vehi_pairs[0])); i++)
		{
			if (_stricmp(tag_path, vehi_pairs[i].a) == 0)
			{
				idx = tag_loaded('vehi', vehi_pairs[i].b);
				if (idx != NONE) return idx;
			}
			else if (_stricmp(tag_path, vehi_pairs[i].b) == 0)
			{
				idx = tag_loaded('vehi', vehi_pairs[i].a);
				if (idx != NONE) return idx;
			}
		}
	}
	return NONE;
}

static struct tag_schema_group const *forge_runtime_schema_group_get(uint32_t group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	if (group_tag == 'mod2')
	{
		for (group = tag_schema_custom_edition_groups; group->group_tag != 0; group++)
		{
			if (group->group_tag == 'mod2')
				return group;
		}
	}

	for (list = tag_schema_group_lists; *list != NULL; list++)
	{
		for (group = *list; group->group_tag != 0; group++)
		{
			if (group->group_tag == (unsigned long)group_tag)
				return group;
		}
	}
	return NULL;
}

struct xbox_runtime_closure
{
	uint8_t const *tag_buf;
	uint32_t tag_sz;
	uint32_t tag_base;
	uint32_t inst_off;
	int32_t tag_count;
	uint8_t *visited;
	int32_t *order;
	int32_t order_count;
	long *runtime_handles;
	uint8_t **cloned_roots;
	int is_red_beam_variant;
	int include_all_dependencies;
};

static void xbox_collect_schema_dependencies(
	struct xbox_runtime_closure *cl,
	struct tag_schema_definition const *def,
	uint32_t base_off,
	int32_t elem_count,
	int depth);

static void xbox_visit_tag_index(struct xbox_runtime_closure *cl, int32_t abs_idx)
{
	uint32_t io, grp, name_addr, data_addr;
	char const *tname;
	struct tag_schema_group const *schema;

	if (abs_idx < 0 || abs_idx >= cl->tag_count)
		return;
	if (cl->visited[abs_idx])
		return;
	cl->visited[abs_idx] = 1;

	io = cl->inst_off + (uint32_t)abs_idx * 32u;
	if (io + 32u > cl->tag_sz)
		return;
	grp = *(uint32_t const *)(cl->tag_buf + io + 0x00);
	name_addr = *(uint32_t const *)(cl->tag_buf + io + 0x10);
	data_addr = *(uint32_t const *)(cl->tag_buf + io + 0x14);
	if (name_addr < cl->tag_base || (name_addr - cl->tag_base) >= cl->tag_sz)
		return;
	tname = (char const *)(cl->tag_buf + (name_addr - cl->tag_base));

	if (grp == 'udlg' || grp == 'sbsp' || grp == 'scnr')
	{
		cl->runtime_handles[abs_idx] = NONE;
		return;
	}

	/* Reuse any dependency tag that was already dynamically injected during this session,
	 * or reuse shared base-map dependencies (including sounds) on stock Xbox maps. */
	if (cl->order_count > 0 && !cl->is_red_beam_variant && !cl->include_all_dependencies)
	{
		char donor_name[256];
		long ex;
		snprintf(donor_name, sizeof(donor_name), "%s__donor", tname);
		ex = tag_loaded((long)grp, donor_name);
		if (ex != NONE)
		{
			cl->runtime_handles[abs_idx] = ex;
			return;
		}
		ex = tag_loaded((long)grp, tname);
		if (ex != NONE)
		{
			long base_cnt = (long)forge_get_base_map_tag_count();
			if ((base_cnt > 0 && (ex & 0xFFFF) >= base_cnt) ||
				(!custom_edition_cache_tags_loaded() && !mcc_cache_tags_loaded() &&
				 grp != 'bitm' && grp != 'mode' && grp != 'snd!' && grp != 'lsnd' &&
				 grp != 'shdr' && grp != 'senv' && grp != 'soso' && grp != 'sotr' &&
				 grp != 'schi' && grp != 'scex' && grp != 'swat' && grp != 'sgla' &&
				 grp != 'smet' && grp != 'spla'))
			{
				cl->runtime_handles[abs_idx] = ex;
				return;
			}
		}
	}

	if (data_addr < cl->tag_base || (data_addr - cl->tag_base) >= cl->tag_sz)
		return;

	schema = forge_runtime_schema_group_get(grp);
	if (!schema || !schema->definition || schema->definition->size <= 0)
		return;
	if ((data_addr - cl->tag_base) + (uint32_t)schema->definition->size > cl->tag_sz)
		return;

	cl->order[cl->order_count++] = abs_idx;
	xbox_collect_schema_dependencies(cl, schema->definition, data_addr - cl->tag_base, 1, 1);
}

static void xbox_collect_schema_dependencies(
	struct xbox_runtime_closure *cl,
	struct tag_schema_definition const *def,
	uint32_t base_off,
	int32_t elem_count,
	int depth)
{
	int32_t elem_idx;
	struct tag_schema_field const *field;
	uint32_t elem_size;

	if (!def || !def->fields || elem_count <= 0 || def->size <= 0 || depth > 10)
		return;
	elem_size = (uint32_t)def->size;
	if ((uint64_t)base_off + (uint64_t)elem_count * elem_size > cl->tag_sz)
		return;

	for (elem_idx = 0; elem_idx < elem_count; elem_idx++)
	{
		uint32_t elem_off = base_off + (uint32_t)elem_idx * elem_size;
		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t fcount = field->count > 0 ? field->count : 1;
			uint32_t fstride = field->size > 0 ? (uint32_t)field->size : 0;

			for (k = 0; k < fcount; k++)
			{
				uint32_t f_off = elem_off + (uint32_t)field->offset + (uint32_t)k * fstride;
				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def && f_off + (uint32_t)sub_def->size <= elem_off + elem_size)
						xbox_collect_schema_dependencies(cl, sub_def, f_off, 1, depth + 1);
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					if (child_def && child_def->size > 0 && f_off + 12u <= elem_off + elem_size)
					{
						int32_t cnt = *(int32_t const *)(cl->tag_buf + f_off + 0u);
						uint32_t addr = *(uint32_t const *)(cl->tag_buf + f_off + 4u);
						int32_t max_cnt = (field->maximum > 0) ? (int32_t)field->maximum : 32768;
						if (cnt > 0 && cnt <= max_cnt && addr >= cl->tag_base &&
							(uint64_t)(addr - cl->tag_base) + (uint64_t)cnt * (uint32_t)child_def->size <= cl->tag_sz)
						{
							xbox_collect_schema_dependencies(cl, child_def, addr - cl->tag_base, cnt, depth + 1);
						}
					}
				}
				else if (field->type == _tag_schema_reference)
				{
					if (f_off + 16u <= elem_off + elem_size)
					{
						int32_t ref_idx = *(int32_t const *)(cl->tag_buf + f_off + 12u);
						if (ref_idx != -1 && (ref_idx & 0xFFFF) < cl->tag_count)
							xbox_visit_tag_index(cl, ref_idx & 0xFFFF);
					}
				}
			}
		}
	}
}

static void xbox_relocate_schema_elements(
	struct xbox_runtime_closure *cl,
	struct tag_schema_definition const *def,
	uint8_t *dst_base,
	uint32_t src_base_off,
	int32_t elem_count,
	int depth)
{
	int32_t elem_idx;
	struct tag_schema_field const *field;
	uint32_t elem_size;

	if (!def || !def->fields || !dst_base || elem_count <= 0 || def->size <= 0 || depth > 10)
		return;
	elem_size = (uint32_t)def->size;

	for (elem_idx = 0; elem_idx < elem_count; elem_idx++)
	{
		uint8_t *elem_ptr = dst_base + (size_t)elem_idx * elem_size;
		uint32_t src_elem_off = src_base_off + (uint32_t)elem_idx * elem_size;

		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t fcount = field->count > 0 ? field->count : 1;
			uint32_t fstride = field->size > 0 ? (uint32_t)field->size : 0;

			for (k = 0; k < fcount; k++)
			{
				uint32_t f_rel = (uint32_t)field->offset + (uint32_t)k * fstride;
				uint32_t src_f_off = src_elem_off + f_rel;

				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def && f_rel + (uint32_t)sub_def->size <= elem_size)
					{
						xbox_relocate_schema_elements(
							cl, sub_def, elem_ptr + f_rel, src_f_off, 1, depth + 1);
					}
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					if (child_def && child_def->size > 0 && f_rel + 12u <= elem_size)
					{
						struct tag_block *blk = (struct tag_block *)(elem_ptr + f_rel);
						int32_t cnt = *(int32_t const *)(cl->tag_buf + src_f_off + 0u);
						uint32_t addr = *(uint32_t const *)(cl->tag_buf + src_f_off + 4u);
						uint32_t sub_elem_sz = (uint32_t)child_def->size;
						int32_t max_cnt = (field->maximum > 0) ? (int32_t)field->maximum : 32768;

						blk->definition = NULL;
						if (cnt > 0 && cnt <= max_cnt && addr >= cl->tag_base &&
							(uint64_t)(addr - cl->tag_base) + (uint64_t)cnt * sub_elem_sz <= cl->tag_sz)
						{
							size_t copy_bytes = (size_t)cnt * sub_elem_sz;
							uint8_t *new_sub = (uint8_t *)forge_session_calloc(copy_bytes);
							if (new_sub)
							{
								memcpy(new_sub, cl->tag_buf + (addr - cl->tag_base), copy_bytes);
								blk->count = cnt;
								blk->address = new_sub;
								xbox_relocate_schema_elements(
									cl, child_def, new_sub, addr - cl->tag_base, cnt, depth + 1);
							}
							else
							{
								blk->count = 0;
								blk->address = NULL;
							}
						}
						else
						{
							blk->count = 0;
							blk->address = NULL;
						}
					}
				}
				else if (field->type == _tag_schema_data)
				{
					if (f_rel + 20u <= elem_size)
					{
						struct tag_data *td = (struct tag_data *)(elem_ptr + f_rel);
						int32_t sz = *(int32_t const *)(cl->tag_buf + src_f_off + 0u);
						uint32_t addr = *(uint32_t const *)(cl->tag_buf + src_f_off + 12u);
						int32_t max_sz = (field->maximum > 0) ? (int32_t)field->maximum : (16 * 1024 * 1024);

						td->definition = NULL;
						if (sz > 0 && sz <= max_sz && addr >= cl->tag_base &&
							(uint64_t)(addr - cl->tag_base) + (uint32_t)sz <= cl->tag_sz)
						{
							void *new_td = forge_session_calloc((size_t)sz);
							if (new_td)
							{
								memcpy(new_td, cl->tag_buf + (addr - cl->tag_base), (size_t)sz);
								td->size = sz;
								td->address = new_td;
							}
							else
							{
								td->size = 0;
								td->address = NULL;
							}
						}
						else
						{
							td->size = 0;
							td->address = NULL;
						}
					}
				}
				else if (field->type == _tag_schema_reference)
				{
					if (f_rel + 16u <= elem_size)
					{
						struct tag_reference *ref = (struct tag_reference *)(elem_ptr + f_rel);
						uint32_t name_addr = *(uint32_t const *)(cl->tag_buf + src_f_off + 4u);
						int32_t ref_idx = *(int32_t const *)(cl->tag_buf + src_f_off + 12u);

						ref->name = NULL;
						ref->name_length = 0;
						if (name_addr >= cl->tag_base && (name_addr - cl->tag_base) < cl->tag_sz)
						{
							char const *rname = (char const *)(cl->tag_buf + (name_addr - cl->tag_base));
							size_t rlen = strlen(rname);
							char *ncopy = (char *)forge_session_calloc(rlen + 1u);
							if (ncopy)
							{
								memcpy(ncopy, rname, rlen + 1u);
								ref->name = ncopy;
							}
						}
						if (ref_idx != -1 && (ref_idx & 0xFFFF) < cl->tag_count &&
							cl->runtime_handles[ref_idx & 0xFFFF] != NONE)
						{
							long hdl = cl->runtime_handles[ref_idx & 0xFFFF];
							ref->index = hdl;
							ref->group_tag = tag_get_group_tag(hdl);
						}
						else
						{
							ref->index = NONE;
						}
					}
				}
				else if (field->type == _tag_schema_tag_index)
				{
					if (f_rel + 4u <= elem_size)
					{
						int32_t ref_idx = *(int32_t const *)(cl->tag_buf + src_f_off);
						if (ref_idx != -1 && (ref_idx & 0xFFFF) < cl->tag_count &&
							cl->runtime_handles[ref_idx & 0xFFFF] != NONE)
						{
							*(int32_t *)(elem_ptr + f_rel) = cl->runtime_handles[ref_idx & 0xFFFF];
						}
						else
						{
							*(int32_t *)(elem_ptr + f_rel) = NONE;
						}
					}
				}
			}
		}
	}
}

static void runtime_tint_dxt1_blue_to_red(uint8_t *pix, uint32_t byte_count)
{
	uint32_t off;
	if (!pix)
		return;
	for (off = 0; off + 8u <= byte_count; off += 8u)
	{
		uint16_t c0 = *(uint16_t const *)(pix + off + 0u);
		uint16_t c1 = *(uint16_t const *)(pix + off + 2u);
		uint32_t lut = *(uint32_t const *)(pix + off + 4u);
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
		*(uint16_t *)(pix + off + 0u) = nc0;
		*(uint16_t *)(pix + off + 2u) = nc1;
		*(uint32_t *)(pix + off + 4u) = lut;
	}
}

static void forge_register_d3d_bitmap_texture(struct bitmap_data *bmp, D3DBaseTexture *tex)
{
	tex->Data = 0;
	tex->Lock = 0;
	tex->Common = D3DCOMMON_TYPE_TEXTURE | 1;
	if (TEST_FLAG(bmp->flags, 4 /* _bitmap_linear_bit */))
	{
		tex->Format =
			(bitmap_format_to_d3d_linear_format(bmp->format, bmp->flags) << D3DFORMAT_FORMAT_SHIFT) |
			(1 << D3DFORMAT_MIPMAP_SHIFT) |
			(2 << D3DFORMAT_DIMENSION_SHIFT) |
			D3DFORMAT_BORDERSOURCE_COLOR |
			D3DFORMAT_DMACHANNEL_A;
		tex->Size =
			(((bitmap_mipmap_get_row_pitch(bmp, 0) + D3DTEXTURE_PITCH_ALIGNMENT - 1) / D3DTEXTURE_PITCH_ALIGNMENT - 1)
				<< D3DSIZE_PITCH_SHIFT) |
			((bmp->height - 1) << D3DSIZE_HEIGHT_SHIFT) |
			(bmp->width - 1);
	}
	else
	{
		tex->Format =
			(floor_log2(bmp->depth) << D3DFORMAT_PSIZE_SHIFT) |
			(floor_log2(bmp->height) << D3DFORMAT_VSIZE_SHIFT) |
			(floor_log2(bmp->width) << D3DFORMAT_USIZE_SHIFT) |
			(bitmap_format_to_d3d_format(bmp->format, bmp->flags) << D3DFORMAT_FORMAT_SHIFT) |
			((bmp->type == 1 ? 3 : 2) << D3DFORMAT_DIMENSION_SHIFT) |
			((rasterizer_xbox_bitmap_get_max_mipmap_count(bmp) + 1) << D3DFORMAT_MIPMAP_SHIFT) |
			(bmp->type == 2 ? D3DFORMAT_CUBEMAP : 0) |
			D3DFORMAT_BORDERSOURCE_COLOR |
			D3DFORMAT_DMACHANNEL_A;
		tex->Size = 0;
	}
	IDirect3DBaseTexture8_Register(tex, bmp->base_address);
	bmp->hardware_format = tex;
}

static long load_native_xbox_tag_from_source(
	char const *cache_map_path,
	uint8_t const *mem_image,
	uint32_t mem_image_size,
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group,
	int strict_untrusted)
{
	FILE *f = NULL;
	uint8_t hdr[2048];
	uint32_t tag_off = 0, tag_sz = 0, inst_addr;
	int32_t tag_count, i, root_abs_idx = -1;
	uint8_t *tag_buf = NULL;
	int keep_cached = 0;
	struct xbox_runtime_closure cl;
	long root_handle = NONE;
	int is_red_beam_variant = (!strict_untrusted && req_group == 'mach' &&
		_stricmp(req_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0) ? 1 : 0;
	char const *lookup_path = is_red_beam_variant
		? "levels\\a30\\devices\\beam emitter\\beam emitter"
		: req_path;

	if (mem_image != NULL)
	{
		if (mem_image_size < 2048u || mem_image_size > 64u * 1024u * 1024u)
			return NONE;
		memcpy(hdr, mem_image, sizeof(hdr));
		if (*(uint32_t const *)(hdr + 0x00) != 0x68656164u || *(uint32_t const *)(hdr + 0x04) != 5u)
			return NONE;
		tag_off = *(uint32_t const *)(hdr + 0x10);
		tag_sz = *(uint32_t const *)(hdr + 0x14);
		if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u ||
			(uint64_t)tag_off + (uint64_t)tag_sz > (uint64_t)mem_image_size)
		{
			return NONE;
		}
		tag_buf = (uint8_t *)malloc((size_t)tag_sz + 1u);
		if (!tag_buf)
			return NONE;
		memcpy(tag_buf, mem_image + tag_off, tag_sz);
		tag_buf[tag_sz] = 0;
	}
	else
	{
		int d_slot;
		if (!cache_map_path || !cache_map_path[0])
			return NONE;

		for (d_slot = 0; d_slot < FORGE_MAX_DONOR_CACHE_SLOTS; d_slot++)
		{
			if (s_runtime_xbox_donors[d_slot].fp &&
				s_runtime_xbox_donors[d_slot].tag_buf &&
				_stricmp(s_runtime_xbox_donors[d_slot].path, cache_map_path) == 0)
			{
				f = s_runtime_xbox_donors[d_slot].fp;
				tag_buf = s_runtime_xbox_donors[d_slot].tag_buf;
				tag_off = s_runtime_xbox_donors[d_slot].tag_off;
				tag_sz = s_runtime_xbox_donors[d_slot].tag_sz;
				s_runtime_xbox_donors[d_slot].last_use_seq = ++s_runtime_xbox_donor_seq;
				keep_cached = 1;
				break;
			}
		}
		if (!keep_cached)
		{
			f = fopen(cache_map_path, "rb");
			if (!f)
				return NONE;
			if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr))
			{
				fclose(f);
				return NONE;
			}
			if (*(uint32_t const *)(hdr + 0x00) != 0x68656164u || *(uint32_t const *)(hdr + 0x04) != 5u)
			{
				fclose(f);
				return NONE;
			}
			tag_off = *(uint32_t const *)(hdr + 0x10);
			tag_sz = *(uint32_t const *)(hdr + 0x14);
			if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
			{
				fclose(f);
				return NONE;
			}

			tag_buf = (uint8_t *)malloc((size_t)tag_sz + 1u);
			if (!tag_buf)
			{
				close_runtime_xbox_donor();
				tag_buf = (uint8_t *)malloc((size_t)tag_sz + 1u);
			}
			if (!tag_buf || fseek(f, (long)tag_off, SEEK_SET) != 0 || fread(tag_buf, 1, tag_sz, f) != tag_sz)
			{
				free(tag_buf);
				fclose(f);
				return NONE;
			}
			tag_buf[tag_sz] = 0;
			{
				int replace_slot = 0;
				unsigned long oldest_seq = (unsigned long)-1;
				for (d_slot = 0; d_slot < FORGE_MAX_DONOR_CACHE_SLOTS; d_slot++)
				{
					if (!s_runtime_xbox_donors[d_slot].fp)
					{
						replace_slot = d_slot;
						break;
					}
					if (s_runtime_xbox_donors[d_slot].last_use_seq < oldest_seq)
					{
						oldest_seq = s_runtime_xbox_donors[d_slot].last_use_seq;
						replace_slot = d_slot;
					}
				}
				if (s_runtime_xbox_donors[replace_slot].fp)
					fclose(s_runtime_xbox_donors[replace_slot].fp);
				if (s_runtime_xbox_donors[replace_slot].tag_buf)
					free(s_runtime_xbox_donors[replace_slot].tag_buf);
				snprintf(s_runtime_xbox_donors[replace_slot].path, sizeof(s_runtime_xbox_donors[0].path), "%s", cache_map_path);
				s_runtime_xbox_donors[replace_slot].fp = f;
				s_runtime_xbox_donors[replace_slot].tag_buf = tag_buf;
				s_runtime_xbox_donors[replace_slot].tag_off = tag_off;
				s_runtime_xbox_donors[replace_slot].tag_sz = tag_sz;
				s_runtime_xbox_donors[replace_slot].last_use_seq = ++s_runtime_xbox_donor_seq;
				keep_cached = 1;
			}
		}
	}

	inst_addr = *(uint32_t const *)(tag_buf + 0x00);
	tag_count = *(int32_t const *)(tag_buf + 0x0C);
	if (inst_addr < 0x24u || tag_count <= 0 || tag_count > (strict_untrusted ? 2048 : 16384) ||
		0x24u + (uint32_t)tag_count * 32u > tag_sz)
	{
		if (!keep_cached)
		{
			free(tag_buf);
			if (f)
				fclose(f);
		}
		return NONE;
	}

	memset(&cl, 0, sizeof(cl));
	cl.tag_buf = tag_buf;
	cl.tag_sz = tag_sz;
	cl.tag_base = inst_addr - 0x24u;
	cl.inst_off = 0x24u;
	cl.tag_count = tag_count;
	cl.is_red_beam_variant = is_red_beam_variant;

	/* Locate the requested tag in this Xbox map's tag table */
	for (i = 0; i < tag_count; i++)
	{
		uint32_t io = cl.inst_off + (uint32_t)i * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t name_addr = *(uint32_t const *)(tag_buf + io + 0x10);
		char const *tname;
		if (strict_untrusted &&
			(grp == 'scnr' || grp == 'sbsp' || grp == 'matg' || grp == 'udlg'))
		{
			free(tag_buf);
			if (f)
				fclose(f);
			return NONE;
		}
		if (name_addr < cl.tag_base || (name_addr - cl.tag_base) >= tag_sz)
			continue;
		tname = (char const *)(tag_buf + (name_addr - cl.tag_base));
		if ((grp == req_group || (req_group == 'mod2' && grp == 'mode')) &&
			_stricmp(tname, lookup_path) == 0)
		{
			root_abs_idx = i;
			if (!strict_untrusted)
				break;
		}
	}

	if (root_abs_idx < 0)
	{
		if (!keep_cached)
		{
			free(tag_buf);
			if (f)
				fclose(f);
		}
		return NONE;
	}

	cl.visited = (uint8_t *)calloc((size_t)tag_count, sizeof(uint8_t));
	cl.order = (int32_t *)calloc((size_t)tag_count, sizeof(int32_t));
	cl.runtime_handles = (long *)malloc((size_t)tag_count * sizeof(long));
	cl.cloned_roots = (uint8_t **)calloc((size_t)tag_count, sizeof(uint8_t *));
	if (!cl.visited || !cl.order || !cl.runtime_handles || !cl.cloned_roots)
	{
		free(cl.visited);
		free(cl.order);
		free(cl.runtime_handles);
		free(cl.cloned_roots);
		if (!keep_cached)
		{
			free(tag_buf);
			if (f)
				fclose(f);
		}
		return NONE;
	}
	for (i = 0; i < tag_count; i++)
		cl.runtime_handles[i] = NONE;

	/* 1. Collect transitive tag dependency closure via OpenCE's tag_schema_group_lists */
	xbox_visit_tag_index(&cl, root_abs_idx);

	/* 2. Allocate root tag definitions and register them in global_tag_instances */
	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = cl.inst_off + (uint32_t)abs_idx * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t p0 = *(uint32_t const *)(tag_buf + io + 0x04);
		uint32_t p1 = *(uint32_t const *)(tag_buf + io + 0x08);
		uint32_t name_addr = *(uint32_t const *)(tag_buf + io + 0x10);
		uint32_t data_addr = *(uint32_t const *)(tag_buf + io + 0x14);
		char const *tname = (char const *)(tag_buf + (name_addr - cl.tag_base));
		char reg_name[256];
		struct tag_schema_group const *schema = forge_runtime_schema_group_get(grp);
		size_t root_sz;
		uint8_t *root_buf;

		if (!schema || !schema->definition || schema->definition->size <= 0)
			continue;
		root_sz = (size_t)schema->definition->size;
		root_buf = (uint8_t *)forge_session_calloc(root_sz < 64u ? 64u : root_sz);
		if (!root_buf)
			continue;
		memcpy(root_buf, tag_buf + (data_addr - cl.tag_base), root_sz);
		cl.cloned_roots[abs_idx] = root_buf;
		if (is_red_beam_variant)
		{
			if (abs_idx == root_abs_idx)
				snprintf(reg_name, sizeof(reg_name), "%s", req_path);
			else
				snprintf(reg_name, sizeof(reg_name), "%s__red", tname);
		}
		else if (abs_idx != root_abs_idx && tag_loaded((long)grp, tname) != NONE)
		{
			snprintf(reg_name, sizeof(reg_name), "%s__donor", tname);
		}
		else
		{
			snprintf(reg_name, sizeof(reg_name), "%s", tname);
		}
		cl.runtime_handles[abs_idx] = append_runtime_tag_instance(
			(long)grp,
			schema->parent_group_tags[0] ? (long)schema->parent_group_tags[0] : (long)p0,
			schema->parent_group_tags[1] ? (long)schema->parent_group_tags[1] : (long)p1,
			reg_name,
			root_buf);
	}

	/* 3. Relocate all nested tag_blocks, tag_data, and tag_references */
	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = cl.inst_off + (uint32_t)abs_idx * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t data_addr = *(uint32_t const *)(tag_buf + io + 0x14);
		struct tag_schema_group const *schema = forge_runtime_schema_group_get(grp);
		if (cl.cloned_roots[abs_idx] && schema && schema->definition)
		{
			xbox_relocate_schema_elements(
				&cl,
				schema->definition,
				cl.cloned_roots[abs_idx],
				data_addr - cl.tag_base,
				1,
				1);
		}
	}

	/* 4. Initialize Xbox 'bitm' hardware textures and 'mode' hardware vertex/triangle buffers using OpenCE's native API */
	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = cl.inst_off + (uint32_t)abs_idx * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t data_addr = *(uint32_t const *)(tag_buf + io + 0x14);
		uint8_t *root_buf = cl.cloned_roots[abs_idx];

		if (!root_buf || cl.runtime_handles[abs_idx] == NONE)
			continue;

		if (grp == 'bitm')
		{
			struct bitmap_group *bgrp = (struct bitmap_group *)root_buf;
			int b_idx;
			for (b_idx = 0; b_idx < bgrp->bitmaps.count && bgrp->bitmaps.address; b_idx++)
			{
				struct bitmap_data *bmp = (struct bitmap_data *)((uint8_t *)bgrp->bitmaps.address + (size_t)b_idx * 48u);
				long req_bytes;
				uint32_t read_bytes, alloc_bytes;
				uint8_t *pix_mem;
				D3DBaseTexture *tex;

				/* Clear _bitmap_cached_bit (bit 7 = 0x80) so _texture_cache_bitmap_get_hardware_format returns bmp->hardware_format directly! */
				bmp->flags &= (uint16_t)~(1u << 7);
				bmp->tag_index = cl.runtime_handles[abs_idx];
				bmp->cache_block_index = NONE;
				bmp->base_address = NULL;
				bmp->hardware_format = NULL;

				if (bmp->width <= 0 || bmp->height <= 0 || bmp->depth <= 0 || bmp->pixels_offset < 2048)
					continue;
				req_bytes = rasterizer_xbox_bitmap_get_pixel_data_size(bmp);
				if (req_bytes <= 0)
					continue;
				read_bytes = (bmp->pixels_size > 0 && bmp->pixels_size < req_bytes) ? (uint32_t)bmp->pixels_size : (uint32_t)req_bytes;
				alloc_bytes = (uint32_t)req_bytes;
				if ((uint32_t)bmp->pixels_size > alloc_bytes && bmp->pixels_size <= 16 * 1024 * 1024)
					alloc_bytes = (uint32_t)bmp->pixels_size;

				pix_mem = (uint8_t *)forge_phys_alloc(alloc_bytes);
				tex = (D3DBaseTexture *)calloc(1, sizeof(D3DBaseTexture));
				if (!pix_mem || !tex)
				{
					free(tex);
					continue;
				}
				if (mem_image != NULL)
				{
					if ((uint64_t)(uint32_t)bmp->pixels_offset + (uint64_t)read_bytes > (uint64_t)mem_image_size)
					{
						free(tex);
						continue;
					}
					memcpy(pix_mem, mem_image + (uint32_t)bmp->pixels_offset, read_bytes);
				}
				else if (!f || fseek(f, (long)bmp->pixels_offset, SEEK_SET) != 0 ||
					fread(pix_mem, 1, read_bytes, f) != read_bytes)
				{
					free(tex);
					continue;
				}
				if (is_red_beam_variant && bmp->format == 14)
				{
					uint32_t name_addr = *(uint32_t const *)(tag_buf + io + 0x10);
					char const *btname = (name_addr >= cl.tag_base && (name_addr - cl.tag_base) < tag_sz)
						? (char const *)(tag_buf + (name_addr - cl.tag_base))
						: "";
					if (strstr(btname, "jackal shield") != NULL || strstr(btname, "static blue") != NULL)
						runtime_tint_dxt1_blue_to_red(pix_mem, read_bytes);
				}
				bmp->base_address = pix_mem;
				halo_custom_edition_texels_channels(bmp->base_address, 0 /* _custom_edition_channels_xbox */);

				forge_register_d3d_bitmap_texture(bmp, tex);
			}
		}
		else if (grp == 'senv' || grp == 'soso' || grp == 'sotr' || grp == 'schi' ||
				 grp == 'scex' || grp == 'swat' || grp == 'sgla' || grp == 'smet' || grp == 'spla')
		{
			int16_t stype = 4;
			if (grp == 'senv') stype = 3;
			else if (grp == 'soso') stype = 4;
			else if (grp == 'sotr') stype = 5;
			else if (grp == 'schi' || grp == 'scex') stype = 6;
			else if (grp == 'swat') stype = 7;
			else if (grp == 'sgla') stype = 8;
			else if (grp == 'smet') stype = 9;
			else if (grp == 'spla') stype = 10;
			*(int16_t *)(root_buf + 0x24u) = stype;
		}
		else if (grp == 'snd!')
		{
			struct sound_definition *sdef = (struct sound_definition *)root_buf;
			int pr_i, pm_i;
			sdef->compression = 1; /* _sound_compression_xbox_adpcm */
			if (sdef->encoding != 1)
				sdef->sample_rate = 0; /* mono Xbox ADPCM plays at 22050 Hz */
			if (*(uint16_t const *)((uint8_t const *)sdef + 0x0Au) == 1u)
			{
				sdef->minimum_distance = (sdef->sound_class == 22 || sdef->sound_class == 23) ? 2.0f : 1.0f;
				if (!(sdef->maximum_distance > sdef->minimum_distance))
					sdef->maximum_distance = 25.0f;
			}
			sdef->promotion_sound.index = NONE;
			sdef->promotion_count = 0;
			for (pr_i = 0; pr_i < sdef->pitch_ranges.count && sdef->pitch_ranges.address; pr_i++)
			{
				struct sound_pitch_range *pr = (struct sound_pitch_range *)((uint8_t *)sdef->pitch_ranges.address + (size_t)pr_i * sizeof(struct sound_pitch_range));
				pr->played_permutation_mask = 0xFFFFFFFFu;
				pr->previous_permutation_index = NONE;
				pr->forced_permutation_index = NONE;
				for (pm_i = 0; pm_i < pr->permutations.count && pr->permutations.address; pm_i++)
				{
					struct sound_permutation *perm = (struct sound_permutation *)((uint8_t *)pr->permutations.address + (size_t)pm_i * sizeof(struct sound_permutation));
					perm->unknown0 = NONE;
					perm->unknown1 = 0;
					perm->unknown2 = (unsigned long)cl.runtime_handles[abs_idx];
					perm->unknown3 = (unsigned long)cl.runtime_handles[abs_idx];
					perm->samples.address = NULL;
					if (perm->samples.size > 0 && perm->samples.size <= MAXIMUM_SOUND_DATA_SIZE &&
						perm->samples.file_offset >= 2048)
					{
						void *sbuf = forge_session_calloc((size_t)perm->samples.size);
						int s_ok = 0;
						if (sbuf)
						{
							if (mem_image != NULL)
							{
								if ((uint64_t)(uint32_t)perm->samples.file_offset + (uint64_t)(uint32_t)perm->samples.size <= (uint64_t)mem_image_size)
								{
									memcpy(sbuf, mem_image + (uint32_t)perm->samples.file_offset, (size_t)perm->samples.size);
									s_ok = 1;
								}
							}
							else if (f && fseek(f, (long)perm->samples.file_offset, SEEK_SET) == 0 &&
								fread(sbuf, 1, (size_t)perm->samples.size, f) == (size_t)perm->samples.size)
							{
								s_ok = 1;
							}
						}
						if (s_ok)
						{
							perm->samples.address = sbuf;
							perm->samples.file_offset = 0;
							forge_register_dyn_sound_perm(perm);
						}
						else
						{
							perm->samples.size = 0;
						}
					}
					else
					{
						perm->samples.size = 0;
					}
				}
			}
		}
		else if (grp == 'mode')
		{
			uint32_t mod_off = data_addr - cl.tag_base;
			int32_t geom_cnt = *(int32_t const *)(root_buf + 0xD0u);
			uint8_t *geom_base = *(uint8_t **)(root_buf + 0xD4u);
			int32_t shdr_cnt = *(int32_t const *)(root_buf + 0xDCu);
			uint8_t *shdr_base = *(uint8_t **)(root_buf + 0xE0u);
			uint32_t src_geom_addr = *(uint32_t const *)(tag_buf + mod_off + 0xD4u);
			int g_idx, s_idx;

			if (!(*(float const *)(root_buf + 0x30u) > 0.0001f && *(float const *)(root_buf + 0x30u) < 10000.0f))
				*(float *)(root_buf + 0x30u) = 1.0f;
			if (!(*(float const *)(root_buf + 0x34u) > 0.0001f && *(float const *)(root_buf + 0x34u) < 10000.0f))
				*(float *)(root_buf + 0x34u) = 1.0f;

			/* Shift beam emitter root node down by 6.5 world units (~10m in Forge HUD) so its flare collar sits at the placement origin */
			if (is_red_beam_variant || _stricmp(req_path, "levels\\a30\\devices\\beam emitter\\beam emitter") == 0)
			{
				int32_t node_cnt = *(int32_t const *)(root_buf + 0xB8u);
				uint8_t *node_base = *(uint8_t **)(root_buf + 0xBCu);
				if (node_cnt > 0 && node_base)
					*(float *)(node_base + 48u) = -6.5f;
			}

			for (s_idx = 0; s_idx < shdr_cnt && shdr_base; s_idx++)
			{
				int16_t *perm_idx_ptr = (int16_t *)(shdr_base + (size_t)s_idx * 32u + 16u);
				if (*perm_idx_ptr < 0)
					*perm_idx_ptr = 0;
			}

			if (geom_cnt > 0 && geom_base && src_geom_addr >= cl.tag_base &&
				(uint64_t)(src_geom_addr - cl.tag_base) + (uint64_t)geom_cnt * 48u <= tag_sz)
			{
				for (g_idx = 0; g_idx < geom_cnt; g_idx++)
				{
					uint8_t *dst_geom = geom_base + (size_t)g_idx * 48u;
					uint32_t src_g_off = (src_geom_addr - cl.tag_base) + (uint32_t)g_idx * 48u;
					int32_t part_cnt = *(int32_t const *)(dst_geom + 36u);
					uint8_t *dst_parts = *(uint8_t **)(dst_geom + 40u);
					uint32_t src_part_addr = *(uint32_t const *)(tag_buf + src_g_off + 40u);
					int p_idx;

					if (part_cnt <= 0 || !dst_parts || src_part_addr < cl.tag_base ||
						(uint64_t)(src_part_addr - cl.tag_base) + (uint64_t)part_cnt * 104u > tag_sz)
						continue;

					for (p_idx = 0; p_idx < part_cnt; p_idx++)
					{
						uint8_t *dst_part = dst_parts + (size_t)p_idx * 104u;
						uint32_t p_off = (src_part_addr - cl.tag_base) + (uint32_t)p_idx * 104u;
						int16_t tri_type = *(int16_t const *)(tag_buf + p_off + 68u);
						int32_t tri_count = *(int32_t const *)(tag_buf + p_off + 72u);
						uint32_t tb_hw_addr = *(uint32_t const *)(tag_buf + p_off + 80u);
						int16_t vtx_type = *(int16_t const *)(tag_buf + p_off + 84u);
						int32_t vtx_count = *(int32_t const *)(tag_buf + p_off + 88u);
						uint32_t vb_hw_addr = *(uint32_t const *)(tag_buf + p_off + 100u);

						memset(dst_part + 68u, 0, 36u);
						((struct vertex_buffer *)(dst_part + 84u))->type = _rasterizer_vertex_type_model_compressed;
						if (tri_count > 0 && tri_count <= 65535 && vtx_count > 0 && vtx_count <= 65535 &&
							tb_hw_addr >= cl.tag_base && (uint64_t)(tb_hw_addr - cl.tag_base) + 12u <= tag_sz &&
							vb_hw_addr >= cl.tag_base && (uint64_t)(vb_hw_addr - cl.tag_base) + 12u <= tag_sz)
						{
							uint32_t ib_data_addr = *(uint32_t const *)(tag_buf + (tb_hw_addr - cl.tag_base) + 4u);
							uint32_t vb_data_addr = *(uint32_t const *)(tag_buf + (vb_hw_addr - cl.tag_base) + 4u);
							uint32_t comp_sz = (uint32_t)vtx_count * 32u;
							uint32_t idx_sz = (tri_type == _triangle_buffer_type_precompiled_strip)
								? ((uint32_t)tri_count + 2u) * 2u
								: (uint32_t)tri_count * 6u;

							if (ib_data_addr >= cl.tag_base && (uint64_t)(ib_data_addr - cl.tag_base) + (uint64_t)idx_sz <= (uint64_t)tag_sz &&
								vb_data_addr >= cl.tag_base && (uint64_t)(vb_data_addr - cl.tag_base) + (uint64_t)comp_sz <= (uint64_t)tag_sz)
							{
								void *hw_verts = forge_phys_alloc(comp_sz);
								void *hw_tris = forge_phys_alloc(idx_sz);
								if (hw_verts && hw_tris)
								{
									memcpy(hw_verts, tag_buf + (vb_data_addr - cl.tag_base), comp_sz);
									memcpy(hw_tris, tag_buf + (ib_data_addr - cl.tag_base), idx_sz);
									rasterizer_vertex_buffer_new(
										(struct vertex_buffer *)(dst_part + 84u),
										vtx_type ? vtx_type : _rasterizer_vertex_type_model_compressed,
										vtx_count,
										hw_verts,
										(long)comp_sz);
									rasterizer_triangle_buffer_new(
										(struct triangle_buffer *)(dst_part + 68u),
										tri_type,
										tri_count,
										hw_tris);
								}
							}
						}
					}
				}
			}
			models_fix_transparent_part_links();
		}
		else if (grp == 'vehi')
		{
			struct unit_definition *ud = (struct unit_definition *)root_buf;
			ud->unit.flags |= FLAG(_unit_has_entrance_points_inside_bounding_sphere_bit);
		}
		else if (grp == 'antr')
		{
			/* Clear _animation_world_relative_bit (0x0002) on scenery/character animations (e.g. flood_captain in d20)
			 * so placed objects render at their placed world position rather than teleporting to campaign cutscene coordinates. */
			int32_t anim_cnt = *(int32_t const *)(root_buf + 0x74u);
			uint8_t *anim_base = *(uint8_t **)(root_buf + 0x78u);
			float n0_tx = 0.0f, n0_ty = 0.0f, n0_tz = 0.0f;
			int m_k, a_idx;

			for (m_k = 0; m_k < cl.order_count; m_k++)
			{
				int m_abs = cl.order[m_k];
				uint32_t m_io = cl.inst_off + (uint32_t)m_abs * 32u;
				if (*(uint32_t const *)(tag_buf + m_io + 0x00) == 'mode' && cl.cloned_roots[m_abs])
				{
					uint8_t const *mod_buf = cl.cloned_roots[m_abs];
					int32_t n_cnt = *(int32_t const *)(mod_buf + 0xB8u);
					uint8_t const *n_base = *(uint8_t *const *)(mod_buf + 0xBCu);
					if (n_cnt > 0 && n_base)
					{
						float tx = *(float const *)(n_base + 40u);
						float ty = *(float const *)(n_base + 44u);
						float tz = *(float const *)(n_base + 48u);
						if (tx > -10.0f && tx < 10.0f && ty > -10.0f && ty < 10.0f && tz > -10.0f && tz < 10.0f)
						{
							n0_tx = tx;
							n0_ty = ty;
							n0_tz = tz;
						}
					}
					break;
				}
			}

			for (a_idx = 0; a_idx < anim_cnt && anim_base; a_idx++)
			{
				uint8_t *anim = anim_base + (size_t)a_idx * 180u;
				uint16_t aflags = *(uint16_t const *)(anim + 0x3Au);
				if (aflags & 0x0002u)
				{
					*(uint16_t *)(anim + 0x3Au) = (uint16_t)(aflags & ~0x0002u);
					if (!(aflags & 0x0001u))
					{
						uint32_t trans_flags0 = *(uint32_t const *)(anim + 0x5Cu);
						uint32_t rot_flags0 = *(uint32_t const *)(anim + 0x64u);
						uint32_t n0_trans_off = (rot_flags0 & 1u) ? 0u : 8u;
						if (!(trans_flags0 & 1u))
						{
							int32_t def_sz = *(int32_t const *)(anim + 0x80u);
							uint8_t *def_ptr = *(uint8_t **)(anim + 0x88u);
							if (def_ptr && def_sz >= (int32_t)(n0_trans_off + 12u))
							{
								*(float *)(def_ptr + n0_trans_off + 0u) = n0_tx;
								*(float *)(def_ptr + n0_trans_off + 4u) = n0_ty;
								*(float *)(def_ptr + n0_trans_off + 8u) = n0_tz;
							}
						}
						else
						{
							int16_t frame_cnt = *(int16_t const *)(anim + 0x24u);
							int16_t frame_sz = *(int16_t const *)(anim + 0x26u);
							int32_t fdata_sz = *(int32_t const *)(anim + 0x94u);
							uint8_t *fdata_ptr = *(uint8_t **)(anim + 0x9Cu);
							int f_i;
							for (f_i = 0; f_i < frame_cnt && frame_sz >= (int16_t)(n0_trans_off + 12u) && fdata_ptr; f_i++)
							{
								uint32_t f_off = (uint32_t)f_i * (uint32_t)frame_sz + n0_trans_off;
								if (f_off + 12u <= (uint32_t)fdata_sz)
								{
									*(float *)(fdata_ptr + f_off + 0u) = n0_tx;
									*(float *)(fdata_ptr + f_off + 4u) = n0_ty;
									*(float *)(fdata_ptr + f_off + 8u) = n0_tz;
								}
							}
						}
					}
				}
			}
		}
		else if (grp == 'ligh')
		{
			patch_light_definition_buffer(root_buf);
			if (is_red_beam_variant)
			{
				*(float *)(root_buf + 0x38u) = 1.0f;
				*(float *)(root_buf + 0x3Cu) = 1.0f;
				*(float *)(root_buf + 0x40u) = 0.12f;
				*(float *)(root_buf + 0x44u) = 0.08f;
				*(float *)(root_buf + 0x48u) = 1.0f;
				*(float *)(root_buf + 0x4Cu) = 1.0f;
				*(float *)(root_buf + 0x50u) = 0.18f;
				*(float *)(root_buf + 0x54u) = 0.10f;
			}
		}
		else if (grp == 'lens' && is_red_beam_variant)
		{
			int32_t refl_cnt = *(int32_t const *)(root_buf + 0xC4u);
			uint8_t *refl_base = *(uint8_t *const *)(root_buf + 0xC8u);
			int r_idx;
			for (r_idx = 0; r_idx < refl_cnt && refl_base; r_idx++)
			{
				uint8_t *rf = refl_base + (size_t)r_idx * 128u;
				*(float *)(rf + 0x40u) = 1.0f;
				*(float *)(rf + 0x44u) = 1.0f;
				*(float *)(rf + 0x48u) = 0.12f;
				*(float *)(rf + 0x4Cu) = 0.08f;
				*(float *)(rf + 0x50u) = 1.0f;
				*(float *)(rf + 0x54u) = 1.0f;
				*(float *)(rf + 0x58u) = 0.18f;
				*(float *)(rf + 0x5Cu) = 0.10f;
				*(float *)(rf + 0x60u) = 1.0f;
				*(float *)(rf + 0x64u) = 1.0f;
				*(float *)(rf + 0x68u) = 0.12f;
				*(float *)(rf + 0x6Cu) = 0.08f;
			}
		}
	}

	root_handle = cl.runtime_handles[root_abs_idx];
	if (root_handle != NONE)
	{
		if (req_group == 'vehi')
			patch_vehicle_tag_for_player_driving(root_handle, req_path);
		else if (req_group == 'scen' || req_group == 'mach' || req_group == 'lifi' || req_group == 'ctrl')
			patch_forge_device_and_light_tag_runtime(root_handle, req_group, req_path);
		if (s_dyn_tags.count < FORGE_MAX_DYNAMIC_TAGS)
		{
			struct forge_dynamic_tag_entry *entry = &s_dyn_tags.entries[s_dyn_tags.count++];
			memset(entry, 0, sizeof(*entry));
			entry->group_tag = req_group;
			snprintf(entry->tag_path, sizeof(entry->tag_path), "%s", req_path);
			entry->definition_index = root_handle;
			entry->part_count = 0; /* 0 = rendered natively by Halo's render_model! */
			entry->bounding_radius = cl.cloned_roots[root_abs_idx]
				? *(float const *)(cl.cloned_roots[root_abs_idx] + 0x04u)
				: 1.5f;
			if (!(entry->bounding_radius > 0.1f && entry->bounding_radius < 100.0f))
				entry->bounding_radius = 1.5f;
		}
		if (out_actual_group)
			*out_actual_group = (uint32_t)tag_get_group_tag(root_handle);
	}

	free(cl.visited);
	free(cl.order);
	free(cl.runtime_handles);
	free(cl.cloned_roots);
	if (!keep_cached)
	{
		free(tag_buf);
		if (f)
			fclose(f);
	}
	return root_handle;
}

static long load_native_xbox_tag_from_decompressed_map(
	char const *cache_map_path,
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group)
{
	return load_native_xbox_tag_from_source(
		cache_map_path,
		NULL,
		0,
		req_group,
		req_path,
		out_actual_group,
		0);
}

struct forge_credits_entry
{
	uint32_t group_tag;
	char stem[32];
	char tag_path[200];
};

static struct forge_credits_entry *s_credits_entries = NULL;
static int s_credits_entry_count = 0;
static int s_credits_entry_cap = 0;
static int s_credits_loaded = 0;

static void invalidate_credits_cache(void)
{
	s_credits_loaded = 0;
	s_credits_entry_count = 0;
}

static void ensure_credits_loaded(void)
{
	FILE *fp;
	char line[512];
	char cur_stem[32];

	if (s_credits_loaded)
		return;
	s_credits_loaded = 1;
	s_credits_entry_count = 0;

	fp = fopen("OpenCE Forge/credits.md", "r");
	if (!fp)
		return;

	cur_stem[0] = '\0';
	while (fgets(line, sizeof(line), fp))
	{
		if (strncmp(line, "## Xbox Map: `", 14) == 0)
		{
			char const *p = line + 14;
			char const *q = strchr(p, '`');
			if (q && q > p)
			{
				size_t len = (size_t)(q - p);
				if (len >= sizeof(cur_stem))
					len = sizeof(cur_stem) - 1;
				memcpy(cur_stem, p, len);
				cur_stem[len] = '\0';
			}
		}
		else if (cur_stem[0] && strncmp(line, "- `", 3) == 0)
		{
			char const *p1 = line + 3;
			char const *q1 = strchr(p1, '`');
			if (q1 && q1 > p1)
			{
				size_t tlen = (size_t)(q1 - p1);
				char const *p2 = strchr(q1 + 1, '`');
				char const *q2 = p2 ? strchr(p2 + 1, '`') : NULL;
				if (tlen < 200u && p2 && q2 && (q2 - (p2 + 1)) == 4)
				{
					uint8_t const *gb = (uint8_t const *)(p2 + 1);
					uint32_t grp = ((uint32_t)gb[0] << 24) | ((uint32_t)gb[1] << 16) | ((uint32_t)gb[2] << 8) | (uint32_t)gb[3];
					if (s_credits_entry_count >= s_credits_entry_cap)
					{
						int new_cap = s_credits_entry_cap ? (s_credits_entry_cap * 2) : 2048;
						struct forge_credits_entry *grown = (struct forge_credits_entry *)realloc(
							s_credits_entries, (size_t)new_cap * sizeof(*grown));
						if (!grown)
							break;
						s_credits_entries = grown;
						s_credits_entry_cap = new_cap;
					}
					s_credits_entries[s_credits_entry_count].group_tag = grp;
					snprintf(s_credits_entries[s_credits_entry_count].stem, sizeof(s_credits_entries[0].stem), "%s", cur_stem);
					memcpy(s_credits_entries[s_credits_entry_count].tag_path, p1, tlen);
					s_credits_entries[s_credits_entry_count].tag_path[tlen] = '\0';
					s_credits_entry_count++;
				}
			}
		}
	}
	fclose(fp);
}

static int find_donor_map_stems_from_credits(
	uint32_t req_group,
	char const *req_path,
	char out_stems[][64],
	int max_stems)
{
	char const *lookup_path;
	int count = 0, idx, i;

	if (!req_path || !req_path[0] || !out_stems || max_stems <= 0)
		return 0;

	ensure_credits_loaded();
	if (s_credits_entry_count <= 0 || !s_credits_entries)
		return 0;

	lookup_path = (req_group == 'mach' &&
		_stricmp(req_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0)
		? "levels\\a30\\devices\\beam emitter\\beam emitter"
		: req_path;

	for (idx = 0; idx < s_credits_entry_count; idx++)
	{
		uint32_t g = s_credits_entries[idx].group_tag;
		if ((g == req_group || (req_group == 'mod2' && g == 'mode')) &&
			_stricmp(s_credits_entries[idx].tag_path, lookup_path) == 0)
		{
			int dup = 0;
			for (i = 0; i < count; i++)
			{
				if (_stricmp(out_stems[i], s_credits_entries[idx].stem) == 0)
				{
					dup = 1;
					break;
				}
			}
			if (!dup && count < max_stems)
			{
				snprintf(out_stems[count++], 64, "%s", s_credits_entries[idx].stem);
				if (count >= max_stems)
					break;
			}
		}
	}
	return count;
}

static long load_native_xbox_tag_runtime(
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group)
{
	static char const *const s_xbox_map_dirs[] = {
		"maps",
		"assets/maps",
		"../assets/maps"
	};
	char candidate_stems[8][64];
	int stem_count, s, slot, d;

	if (!req_path || !req_path[0])
		return NONE;

	/* 1. Check exact donor map(s) indexed in OpenCE Forge/credits.md FIRST so we never scan unrelated maps */
	stem_count = find_donor_map_stems_from_credits(req_group, req_path, candidate_stems, 8);
	for (s = 0; s < stem_count; s++)
	{
		char xcache_path[512];
		long xhdl;
		snprintf(xcache_path, sizeof(xcache_path), "OpenCE Forge/cached_tags/xbox_cache/%s.map", candidate_stems[s]);
		xhdl = load_native_xbox_tag_from_decompressed_map(xcache_path, req_group, req_path, out_actual_group);
		if (xhdl != NONE)
			return xhdl;

		for (d = 0; d < (int)(sizeof(s_xbox_map_dirs) / sizeof(s_xbox_map_dirs[0])); d++)
		{
			char full_path[512];
			char cache_path[512];
			snprintf(full_path, sizeof(full_path), "%s/%s.map", s_xbox_map_dirs[d], candidate_stems[s]);
			if (is_xbox_version5_map_file(full_path) &&
				ensure_xbox_map_decompressed_cache(full_path, cache_path, sizeof(cache_path)))
			{
				long hdl = load_native_xbox_tag_from_decompressed_map(cache_path, req_group, req_path, out_actual_group);
				if (hdl != NONE)
					return hdl;
			}
		}
	}

	/* 1b. Also check any already-open in-memory donor maps in s_runtime_xbox_donors */
	for (slot = 0; slot < FORGE_MAX_DONOR_CACHE_SLOTS; slot++)
	{
		if (s_runtime_xbox_donors[slot].fp && s_runtime_xbox_donors[slot].tag_buf && s_runtime_xbox_donors[slot].path[0])
		{
			long hdl = load_native_xbox_tag_from_decompressed_map(
				s_runtime_xbox_donors[slot].path, req_group, req_path, out_actual_group);
			if (hdl != NONE)
				return hdl;
		}
	}

	/* 2. Check OpenCE's Xbox hard-drive cache (z:\cache000.map .. z:\cache005.map and z:\cache_donor.map) */
	for (slot = 0; slot <= 6; slot++)
	{
		char z_xbox[64];
		char z_path[512];
		long hdl;
		if (slot < 6)
			snprintf(z_xbox, sizeof(z_xbox), "z:\\cache00%d.map", slot);
		else
			snprintf(z_xbox, sizeof(z_xbox), "z:\\cache_donor.map");
		platform_translate_path(z_xbox, z_path, sizeof(z_path));
		hdl = load_native_xbox_tag_from_decompressed_map(z_path, req_group, req_path, out_actual_group);
		if (hdl != NONE)
			return hdl;
	}

	/* 3. Fallback: scan Xbox maps in maps/ */
	for (d = 0; d < (int)(sizeof(s_xbox_map_dirs) / sizeof(s_xbox_map_dirs[0])); d++)
	{
		char pattern[512];
		WIN32_FIND_DATAA fd;
		HANDLE h;
		snprintf(pattern, sizeof(pattern), "%s\\*.map", s_xbox_map_dirs[d]);
		h = FindFirstFileA(pattern, &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				char full_path[512];
				char cache_path[512];
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					continue;
				if (_stricmp(fd.cFileName, "bitmaps.map") == 0 ||
					_stricmp(fd.cFileName, "sounds.map") == 0 ||
					_stricmp(fd.cFileName, "loc.map") == 0 ||
					_stricmp(fd.cFileName, "ui.map") == 0)
					continue;
				snprintf(full_path, sizeof(full_path), "%s/%s", s_xbox_map_dirs[d], fd.cFileName);
				if (is_xbox_version5_map_file(full_path) &&
					ensure_xbox_map_decompressed_cache(full_path, cache_path, sizeof(cache_path)))
				{
					long hdl = load_native_xbox_tag_from_decompressed_map(cache_path, req_group, req_path, out_actual_group);
					if (hdl != NONE)
					{
						CloseHandle(h);
						return hdl;
					}
				}
			} while (FindNextFileA(h, &fd));
			CloseHandle(h);
		}
	}
	return NONE;
}

struct faber_bundle_read_ctx
{
	long handle;
	byte const *mem_file_data;
	unsigned long file_data_offset;
	unsigned long file_data_size;
};

static boolean faber_bundle_file_read_cb(
	void *context,
	unsigned long offset,
	unsigned long size,
	void *buffer)
{
	struct faber_bundle_read_ctx const *ctx = (struct faber_bundle_read_ctx const *)context;
	if (!ctx || !buffer)
		return FALSE;
	if (offset > ctx->file_data_size || size > ctx->file_data_size - offset)
		return FALSE;
	if (ctx->mem_file_data)
	{
		if (size > 0)
			memcpy(buffer, ctx->mem_file_data + offset, size);
		return TRUE;
	}
	if (ctx->handle == NONE)
		return FALSE;
	return port_tag_profiles_bundle_read_at(
		ctx->handle,
		ctx->file_data_offset + offset,
		buffer,
		size) ? TRUE : FALSE;
}

static void faber_retarget_schema_fields(
	struct tag_bundle_loaded const *loaded,
	long const *datums,
	uint8_t *element,
	struct tag_schema_definition const *definition,
	int depth)
{
	struct tag_schema_field const *field;

	if (depth >= 24 || !element || !definition || !definition->fields)
		return;
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		int16_t item;
		int16_t fcount = (field->count > 0) ? field->count : 1;
		uint32_t fstride = (field->size > 0) ? (uint32_t)field->size : 0;

		for (item = 0; item < fcount; item++)
		{
			uint8_t *place = element + (uint32_t)field->offset + (uint32_t)item * fstride;

			switch (field->type)
			{
			case _tag_schema_reference:
			{
				struct tag_reference *ref = (struct tag_reference *)place;
				long b_idx = ref->index;
				long datum = (b_idx >= 0 && b_idx < loaded->tag_count) ? datums[b_idx] : NONE;

				ref->index = datum;
				if (ref->group_tag == 'mod2')
					ref->group_tag = 'mode';
				else if (ref->group_tag == 'scex')
					ref->group_tag = 'schi';
				if (datum == NONE)
				{
					ref->name = (char *)loaded->names + loaded->names_size - 1;
					ref->name_length = 0;
				}
				else
				{
					char const *tname = tag_get_name(datum);
					ref->group_tag = tag_get_group_tag(datum);
					ref->name = (char *)tname;
					ref->name_length = tname ? (long)strlen(tname) : 0;
				}
				break;
			}
			case _tag_schema_tag_index:
			{
				long b_idx = *(long const *)place;
				*(long *)place = (b_idx >= 0 && b_idx < loaded->tag_count) ? datums[b_idx] : NONE;
				break;
			}
			case _tag_schema_struct:
				faber_retarget_schema_fields(
					loaded,
					datums,
					place,
					(struct tag_schema_definition const *)field->definition,
					depth + 1);
				break;
			case _tag_schema_block:
			{
				struct tag_block *block = (struct tag_block *)place;
				struct tag_schema_definition const *elem_def =
					(struct tag_schema_definition const *)field->definition;
				long k;
				for (k = 0; elem_def && block->address && k < block->count; k++)
				{
					faber_retarget_schema_fields(
						loaded,
						datums,
						(uint8_t *)block->address + (size_t)k * (size_t)elem_def->size,
						elem_def,
						depth + 1);
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

static long install_faber_bundle_from_source(
	long profile_handle,
	byte const *raw_bundle,
	unsigned long bundle_size,
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group,
	int strict_untrusted)
{
	byte header_bytes[TAG_BUNDLE_HEADER_BYTES];
	unsigned long tags_size;
	byte *bundle_bytes = NULL;
	struct tag_bundle_loaded loaded;
	long *datums = NULL;
	uint8_t *is_new_tag = NULL;
	struct faber_bundle_read_ctx read_ctx;
	long ti;
	long root_handle = NONE;

	if (!req_path || !req_path[0] || bundle_size < TAG_BUNDLE_HEADER_BYTES || bundle_size > 64u * 1024u * 1024u)
		return NONE;

	if (raw_bundle != NULL)
	{
		memcpy(header_bytes, raw_bundle, TAG_BUNDLE_HEADER_BYTES);
	}
	else if (profile_handle == NONE ||
		!port_tag_profiles_bundle_read_at(profile_handle, 0, header_bytes, TAG_BUNDLE_HEADER_BYTES))
	{
		return NONE;
	}

	tags_size = tag_bundle_file_data_offset(header_bytes);
	if (tags_size < TAG_BUNDLE_HEADER_BYTES || tags_size > bundle_size || tags_size > 64u * 1024u * 1024u)
		return NONE;

	if (strict_untrusted)
	{
		byte *probe_buf = (byte *)malloc(tags_size);
		struct tag_bundle_loaded probe_loaded;
		long k;
		uint32_t root0_grp;
		char const *root0_name;

		if (!probe_buf)
			return NONE;
		if (raw_bundle != NULL)
			memcpy(probe_buf, raw_bundle, tags_size);
		else if (!port_tag_profiles_bundle_read_at(profile_handle, 0, probe_buf, tags_size))
		{
			free(probe_buf);
			return NONE;
		}
		memset(&probe_loaded, 0, sizeof(probe_loaded));
		if (tag_bundle_load_tags(probe_buf, tags_size, bundle_size, &probe_loaded) != _tag_bundle_status_ok ||
			probe_loaded.tag_count <= 0 || probe_loaded.tag_count > 512)
		{
			free(probe_buf);
			return NONE;
		}
		root0_grp = *(uint32_t const *)(probe_loaded.tags + TAG_BUNDLE_TAG_GROUP_OFFSET);
		root0_name = probe_loaded.names + *(uint32_t const *)(probe_loaded.tags + TAG_BUNDLE_TAG_NAME_OFFSET);
		if (!((root0_grp == req_group || (req_group == 'mode' && root0_grp == 'mod2')) &&
			  _stricmp(root0_name, req_path) == 0))
		{
			free(probe_buf);
			return NONE;
		}
		for (k = 0; k < probe_loaded.tag_count; k++)
		{
			uint32_t k_grp = *(uint32_t const *)(probe_loaded.tags + k * TAG_BUNDLE_TAG_BYTES + TAG_BUNDLE_TAG_GROUP_OFFSET);
			if (k_grp == 'scnr' || k_grp == 'sbsp' || k_grp == 'matg' || k_grp == 'udlg')
			{
				free(probe_buf);
				return NONE;
			}
		}
		free(probe_buf);
	}

	bundle_bytes = (byte *)forge_phys_alloc(tags_size);
	if (!bundle_bytes)
		return NONE;
	if (raw_bundle != NULL)
	{
		memcpy(bundle_bytes, raw_bundle, tags_size);
	}
	else if (!port_tag_profiles_bundle_read_at(profile_handle, 0, bundle_bytes, tags_size))
	{
		return NONE;
	}
	memset(&loaded, 0, sizeof(loaded));
	if (tag_bundle_load_tags(bundle_bytes, tags_size, bundle_size, &loaded) != _tag_bundle_status_ok ||
		loaded.tag_count <= 0)
	{
		return NONE;
	}

	datums = (long *)malloc((size_t)loaded.tag_count * sizeof(long));
	is_new_tag = (uint8_t *)calloc((size_t)loaded.tag_count, 1);
	if (!datums || !is_new_tag)
	{
		free(datums);
		free(is_new_tag);
		return NONE;
	}
	for (ti = 0; ti < loaded.tag_count; ti++)
		datums[ti] = NONE;

	read_ctx.handle = profile_handle;
	read_ctx.mem_file_data = (raw_bundle != NULL) ? (raw_bundle + loaded.file_data_offset) : NULL;
	read_ctx.file_data_offset = loaded.file_data_offset;
	read_ctx.file_data_size = loaded.file_data_size;

	/* 1. Convert Custom Edition 'mod2' models and register all bundle tags */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		byte const *entry = loaded.tags + ti * TAG_BUNDLE_TAG_BYTES;
		uint32_t grp = *(uint32_t const *)(entry + TAG_BUNDLE_TAG_GROUP_OFFSET);
		uint32_t p0 = *(uint32_t const *)(entry + TAG_BUNDLE_TAG_PARENT_GROUPS_OFFSET);
		uint32_t p1 = *(uint32_t const *)(entry + TAG_BUNDLE_TAG_PARENT_GROUPS_OFFSET + 4);
		char const *tname = loaded.names + *(uint32_t const *)(entry + TAG_BUNDLE_TAG_NAME_OFFSET);
		uint8_t *root_buf = loaded.data + *(uint32_t const *)(entry + TAG_BUNDLE_TAG_ROOT_OFFSET);
		uint32_t actual_grp = (grp == 'mod2') ? 'mode' : ((grp == 'scex') ? 'schi' : grp);

		if (grp == 'scnr' || grp == 'sbsp' || grp == 'matg' || grp == 'udlg')
			continue;

		if (ti > 0 && (custom_edition_cache_tags_loaded() || mcc_cache_tags_loaded()) &&
			grp != 'bitm' && grp != 'mod2' && grp != 'mode' && grp != 'snd!' && grp != 'lsnd' &&
			grp != 'shdr' && grp != 'senv' && grp != 'soso' && grp != 'sotr' &&
			grp != 'schi' && grp != 'scex' && grp != 'swat' && grp != 'sgla' &&
			grp != 'smet' && grp != 'spla')
		{
			long ex = tag_loaded((long)actual_grp, tname);
			if (ex != NONE)
			{
				datums[ti] = ex;
				continue;
			}
		}

		if (grp == 'mod2')
		{
			if (!custom_edition_injected_model_convert(
					(struct model *)root_buf,
					tname,
					loaded.file_data_size,
					faber_bundle_file_read_cb,
					&read_ctx))
			{
				continue;
			}
		}
		else if (grp == 'scex')
		{
			p0 = 'shdr';
			p1 = NONE;
		}

		{
			char reg_name[256];
			if (ti > 0 && tag_loaded((long)actual_grp, tname) != NONE)
			{
				long ex_cnt = 0;
				cache_files_tag_instances(&ex_cnt);
				snprintf(reg_name, sizeof(reg_name), "%s__f%ld", tname, ex_cnt);
			}
			else
			{
				snprintf(reg_name, sizeof(reg_name), "%s", tname);
			}
			datums[ti] = append_runtime_tag_instance(
				(long)actual_grp,
				(long)p0,
				(long)p1,
				reg_name,
				root_buf);
		}
		if (datums[ti] != NONE)
			is_new_tag[ti] = 1;
	}

	/* 2. Retarget all tag_references and tag_indices across the bundle */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		byte const *entry = loaded.tags + ti * TAG_BUNDLE_TAG_BYTES;
		uint32_t grp = *(uint32_t const *)(entry + TAG_BUNDLE_TAG_GROUP_OFFSET);
		uint8_t *root_buf = loaded.data + *(uint32_t const *)(entry + TAG_BUNDLE_TAG_ROOT_OFFSET);
		struct tag_schema_group const *schema_grp;

		if (!is_new_tag[ti] || datums[ti] == NONE)
			continue;
		schema_grp = forge_runtime_schema_group_get(grp == 'mod2' ? 'mode' : grp);
		if (schema_grp && schema_grp->definition)
			faber_retarget_schema_fields(&loaded, datums, root_buf, schema_grp->definition, 0);
	}

	/* 3. Initialize bitmaps, sounds, shaders, lights, and vehicles */
	for (ti = 0; ti < loaded.tag_count; ti++)
	{
		byte const *entry = loaded.tags + ti * TAG_BUNDLE_TAG_BYTES;
		uint32_t grp = *(uint32_t const *)(entry + TAG_BUNDLE_TAG_GROUP_OFFSET);
		uint8_t *root_buf = loaded.data + *(uint32_t const *)(entry + TAG_BUNDLE_TAG_ROOT_OFFSET);

		if (!is_new_tag[ti] || datums[ti] == NONE)
			continue;

		if (grp == 'bitm')
		{
			struct bitmap_group *bgrp = (struct bitmap_group *)root_buf;
			unsigned char chan_order = 0;
			long k;
			int bi;

			*(uint32_t *)(root_buf + 0x38u) = 0;
			*(void **)(root_buf + 0x44u) = NULL;

			{
				int used_mp = 0, used_rgba = 0;
				for (k = 0; k < loaded.tag_count; k++)
				{
					byte const *s_ent = loaded.tags + k * TAG_BUNDLE_TAG_BYTES;
					uint32_t s_grp = *(uint32_t const *)(s_ent + TAG_BUNDLE_TAG_GROUP_OFFSET);
					uint8_t const *s_buf = loaded.data + *(uint32_t const *)(s_ent + TAG_BUNDLE_TAG_ROOT_OFFSET);
					if (s_grp != 'soso')
						continue;
					if (*(int32_t const *)(s_buf + 0xC8u) == (int32_t)datums[ti])
						used_mp = 1;
					if (*(int32_t const *)(s_buf + 0xB0u) == (int32_t)datums[ti] ||
						*(int32_t const *)(s_buf + 0xE8u) == (int32_t)datums[ti] ||
						*(int32_t const *)(s_buf + 0x170u) == (int32_t)datums[ti])
						used_rgba = 1;
				}
				if (used_mp && !used_rgba)
					chan_order = 1;
			}

			for (bi = 0; bi < bgrp->bitmaps.count && bgrp->bitmaps.address; bi++)
			{
				struct bitmap_data *bmp = (struct bitmap_data *)((uint8_t *)bgrp->bitmaps.address + (size_t)bi * sizeof(struct bitmap_data));
				unsigned long pix_off = (unsigned long)bmp->pixels_offset;
				size_t src_len = (bmp->pixels_size > 0) ? (size_t)bmp->pixels_size : 0u;
				short max_mips;
				long req_bytes;
				size_t alloc_bytes;
				void *pix_copy;
				D3DBaseTexture *tex;

				bmp->base_address = NULL;
				bmp->hardware_format = NULL;
				bmp->cache_block_index = NONE;
				bmp->tag_index = datums[ti];
				bmp->signature = 'bitm';
				bmp->flags &= (unsigned short)~(0x80u | 0x08u | 0x10u | 0x100u);
				if (bmp->width > 0 && (bmp->width & (bmp->width - 1)) == 0 &&
					bmp->height > 0 && (bmp->height & (bmp->height - 1)) == 0)
					bmp->flags |= 0x01u;
				if (bmp->format >= 14 && bmp->format <= 16)
					bmp->flags |= 0x02u;
				else
					bmp->flags &= (unsigned short)~0x02u;
				if (bmp->format == 17)
					bmp->flags |= 0x04u;

				if (src_len == 0 || bmp->width <= 0 || bmp->height <= 0 || bmp->depth <= 0 ||
					pix_off > loaded.file_data_size || src_len > loaded.file_data_size - pix_off)
					continue;

				max_mips = rasterizer_xbox_bitmap_get_max_mipmap_count(bmp);
				if (bmp->mipmap_count < 0)
					bmp->mipmap_count = 0;
				if (bmp->mipmap_count > max_mips)
					bmp->mipmap_count = max_mips;
				while (bmp->mipmap_count > 0 && (size_t)bitmap_get_pixel_data_size(bmp) > src_len)
					bmp->mipmap_count--;

				req_bytes = rasterizer_xbox_bitmap_get_pixel_data_size(bmp);
				if (req_bytes <= 0)
					continue;
				alloc_bytes = ((size_t)req_bytes > src_len) ? (size_t)req_bytes : src_len;
				pix_copy = forge_phys_alloc(alloc_bytes);
				tex = (D3DBaseTexture *)calloc(1, sizeof(D3DBaseTexture));
				if (!pix_copy || !tex ||
					!faber_bundle_file_read_cb(&read_ctx, pix_off, (unsigned long)src_len, pix_copy))
				{
					free(tex);
					continue;
				}
				bmp->base_address = pix_copy;
				bmp->pixels_offset = 0;
				bmp->pixels_size = (long)alloc_bytes;

				halo_custom_edition_texels_channels(bmp->base_address, chan_order);
				rasterizer_xbox_bitmap_rebuild_hardware_format(bmp);

				forge_register_d3d_bitmap_texture(bmp, tex);
			}
		}
		else if (grp == 'snd!')
		{
			struct sound_definition *sdef = (struct sound_definition *)root_buf;
			int pr_i, pm_i;
			sdef->compression = 1; /* _sound_compression_xbox_adpcm */
			if (sdef->encoding != 1)
				sdef->sample_rate = 0; /* mono Xbox ADPCM plays at 22050 Hz */
			if (*(uint16_t const *)((uint8_t const *)sdef + 0x0Au) == 1u)
			{
				sdef->minimum_distance = (sdef->sound_class == 22 || sdef->sound_class == 23) ? 2.0f : 1.0f;
				if (!(sdef->maximum_distance > sdef->minimum_distance))
					sdef->maximum_distance = 25.0f;
			}
			sdef->promotion_sound.index = NONE;
			sdef->promotion_count = 0;
			for (pr_i = 0; pr_i < sdef->pitch_ranges.count && sdef->pitch_ranges.address; pr_i++)
			{
				struct sound_pitch_range *pr = (struct sound_pitch_range *)((uint8_t *)sdef->pitch_ranges.address + (size_t)pr_i * sizeof(struct sound_pitch_range));
				pr->played_permutation_mask = 0xFFFFFFFFu;
				pr->previous_permutation_index = NONE;
				pr->forced_permutation_index = NONE;
				for (pm_i = 0; pm_i < pr->permutations.count && pr->permutations.address; pm_i++)
				{
					struct sound_permutation *perm = (struct sound_permutation *)((uint8_t *)pr->permutations.address + (size_t)pm_i * sizeof(struct sound_permutation));
					unsigned long s_off = (unsigned long)perm->samples.file_offset;
					unsigned long s_sz = (perm->samples.size > 0) ? (unsigned long)perm->samples.size : 0u;
					perm->unknown0 = NONE;
					perm->unknown1 = 0;
					perm->unknown2 = (unsigned long)datums[ti];
					perm->unknown3 = (unsigned long)datums[ti];
					perm->samples.address = NULL;
					if (s_sz > 0 && s_sz <= MAXIMUM_SOUND_DATA_SIZE &&
						s_off <= loaded.file_data_size && s_sz <= loaded.file_data_size - s_off)
					{
						void *sbuf = forge_phys_alloc(s_sz);
						if (sbuf && faber_bundle_file_read_cb(&read_ctx, s_off, s_sz, sbuf))
						{
							perm->samples.address = sbuf;
							perm->samples.file_offset = 0;
							forge_register_dyn_sound_perm(perm);
						}
						else
						{
							perm->samples.size = 0;
						}
					}
					else
					{
						perm->samples.size = 0;
					}
				}
			}
		}
		else if (grp == 'ligh')
		{
			patch_light_definition_buffer(root_buf);
		}
	}

	root_handle = datums[0];
	if (root_handle != NONE)
	{
		if (req_group == 'vehi')
			patch_vehicle_tag_for_player_driving(root_handle, req_path);
		else if (req_group == 'scen' || req_group == 'mach' || req_group == 'lifi' || req_group == 'ctrl')
			patch_forge_device_and_light_tag_runtime(root_handle, req_group, req_path);
		if (s_dyn_tags.count < FORGE_MAX_DYNAMIC_TAGS)
		{
			struct forge_dynamic_tag_entry *d_entry = &s_dyn_tags.entries[s_dyn_tags.count++];
			uint8_t const *root0 = loaded.data + *(uint32_t const *)(loaded.tags + TAG_BUNDLE_TAG_ROOT_OFFSET);
			memset(d_entry, 0, sizeof(*d_entry));
			d_entry->group_tag = req_group;
			snprintf(d_entry->tag_path, sizeof(d_entry->tag_path), "%s", req_path);
			d_entry->definition_index = root_handle;
			d_entry->part_count = 0;
			d_entry->bounding_radius = *(float const *)(root0 + 0x04u);
			if (!(d_entry->bounding_radius > 0.1f && d_entry->bounding_radius < 100.0f))
				d_entry->bounding_radius = 1.5f;
		}
		if (out_actual_group)
			*out_actual_group = (uint32_t)tag_get_group_tag(root_handle);
	}

	free(datums);
	free(is_new_tag);
	return root_handle;
}

static long load_faber_vault_tag_bundle_runtime(
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group)
{
	long prof_count, p, t, found_p = -1, found_t = -1;
	unsigned long bundle_size = 0;
	long b_handle = NONE;
	long root_handle;

	if (!req_path || !req_path[0])
		return NONE;

	prof_count = port_tag_profiles_count();
	if (prof_count <= 0)
	{
		port_tag_profiles_scan();
		prof_count = port_tag_profiles_count();
	}
	for (p = 0; p < prof_count && found_p < 0; p++)
	{
		long tc = port_tag_profiles_tag_count(p);
		for (t = 0; t < tc; t++)
		{
			unsigned long grp;
			char const *tname;
			if (!port_tag_profiles_tag_enabled(p, t))
				continue;
			grp = port_tag_profiles_tag_group(p, t);
			tname = port_tag_profiles_tag_name(p, t);
			if ((grp == (unsigned long)req_group || (req_group == 'mode' && grp == 'mod2')) &&
				tname && _stricmp(tname, req_path) == 0)
			{
				found_p = p;
				found_t = t;
				break;
			}
		}
	}
	if (found_p < 0)
		return NONE;

	b_handle = port_tag_profiles_bundle_open(found_p, found_t, &bundle_size);
	if (b_handle == NONE)
		return NONE;
	root_handle = NONE;
	if (bundle_size >= 4u)
	{
		uint32_t magic = 0;
		if (port_tag_profiles_bundle_read_at(b_handle, 0, &magic, sizeof(magic)))
		{
			if (magic == 0x68656164u /* 'head' Xbox/MCC v5 bundle in Faber Tag Vault */)
			{
				if (bundle_size >= 2048u && bundle_size <= 64u * 1024u * 1024u)
				{
					uint8_t *uncomp_buf = (uint8_t *)malloc(bundle_size);
					if (uncomp_buf && port_tag_profiles_bundle_read_at(b_handle, 0, uncomp_buf, bundle_size))
					{
						root_handle = load_native_xbox_tag_from_source(
							NULL,
							uncomp_buf,
							(uint32_t)bundle_size,
							req_group,
							req_path,
							out_actual_group,
							0);
					}
					free(uncomp_buf);
				}
			}
			else
			{
				root_handle = install_faber_bundle_from_source(
					b_handle,
					NULL,
					bundle_size,
					req_group,
					req_path,
					out_actual_group,
					0);
			}
		}
	}
	port_tag_profiles_bundle_close(b_handle);
	return root_handle;
}

static long register_dynamic_cached_tag(uint32_t group_tag, char const *tag_path, uint32_t *out_actual_group)
{
	int i;
	long native_idx;

	/* Check if already registered (or negative-cached) for the current map */
	for (i = 0; i < s_dyn_tags.count; i++)
	{
		if (s_dyn_tags.entries[i].group_tag == group_tag &&
			_stricmp(s_dyn_tags.entries[i].tag_path, tag_path) == 0)
		{
			if (out_actual_group && s_dyn_tags.entries[i].definition_index != NONE)
				*out_actual_group = (uint32_t)tag_get_group_tag(s_dyn_tags.entries[i].definition_index);
			return s_dyn_tags.entries[i].definition_index;
		}
	}

	if (s_dyn_tags.count >= FORGE_MAX_DYNAMIC_TAGS)
		return NONE;

	/* 1. Check compressed Faber Tag Vault (.fbrtag + faber_vault.bin) in OpenCE Forge/tag_vault/ */
	native_idx = load_faber_vault_tag_bundle_runtime(group_tag, tag_path, out_actual_group);
	if (native_idx != NONE)
		return native_idx;

	/* 2. Load default Xbox tag natively from Xbox map cache using OpenCE's exact tag schema & hardware format! */
	native_idx = load_native_xbox_tag_runtime(group_tag, tag_path, out_actual_group);
	if (native_idx != NONE)
		return native_idx;

	/* Negative-cache missing tag so we never re-scan disk on every frame */
	if (s_dyn_tags.count < FORGE_MAX_DYNAMIC_TAGS)
	{
		struct forge_dynamic_tag_entry *entry = &s_dyn_tags.entries[s_dyn_tags.count++];
		memset(entry, 0, sizeof(*entry));
		entry->group_tag = group_tag;
		snprintf(entry->tag_path, sizeof(entry->tag_path), "%s", tag_path);
		entry->definition_index = NONE;
	}

	return NONE;
}

static long forge_try_get_loaded_engine_tag(
	uint32_t group_tag,
	char const *tag_path,
	uint32_t *out_actual_group,
	int *out_known_missing)
{
	long idx;
	int i;

	if (out_actual_group)
		*out_actual_group = group_tag;
	if (out_known_missing)
		*out_known_missing = 0;
	if (!tag_path || !tag_path[0])
		return NONE;

	for (i = 0; i < s_dyn_tags.count; i++)
	{
		if (s_dyn_tags.entries[i].group_tag == group_tag &&
			_stricmp(s_dyn_tags.entries[i].tag_path, tag_path) == 0)
		{
			if (s_dyn_tags.entries[i].definition_index == NONE)
			{
				if (out_known_missing)
					*out_known_missing = 1;
				return NONE;
			}
			if (out_actual_group)
				*out_actual_group = (uint32_t)tag_get_group_tag(s_dyn_tags.entries[i].definition_index);
			return s_dyn_tags.entries[i].definition_index;
		}
	}

	idx = find_native_or_mp_variant_tag(group_tag, tag_path);
	if (idx != NONE)
	{
		if (group_tag == 'vehi')
			patch_vehicle_tag_for_player_driving(idx, tag_path);
		return idx;
	}
	return NONE;
}

long forge_resolve_engine_tag(uint32_t group_tag, char const *tag_path, uint32_t *out_actual_group)
{
	long idx;
	int known_missing = 0;

	idx = forge_try_get_loaded_engine_tag(group_tag, tag_path, out_actual_group, &known_missing);
	if (idx != NONE || known_missing)
		return idx;

	if (!tag_path || !tag_path[0])
		return NONE;

	/* Dynamically load from cached map tag bundles or Xbox map cache into global_tag_instances! */
	return register_dynamic_cached_tag(group_tag, tag_path, out_actual_group);
}

static int is_ce_v609_map_file(char const *path)
{
	FILE *f = fopen(path, "rb");
	uint32_t hdr[2];
	if (!f)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr))
	{
		fclose(f);
		return 0;
	}
	fclose(f);
	return (hdr[0] == 0x68656164u && hdr[1] == 609u) ? 1 : 0;
}

static int extract_embedded_forge_base_map_stem(char const *map_path, char *out_stem, size_t out_sz)
{
	FILE *f;
	uint8_t hdr[0x200];
	uint32_t state_off = 0, state_sz = 0, sig = 0;

	if (!map_path || !out_stem || out_sz == 0)
		return 0;
	out_stem[0] = 0;
	f = fopen(map_path, "rb");
	if (!f)
		return 0;
	if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr))
	{
		fclose(f);
		return 0;
	}
	memcpy(&state_off, hdr + 0x1E4u, 4);
	memcpy(&state_sz, hdr + 0x1E8u, 4);
	memcpy(&sig, hdr + 0x1FCu, 4);
	if (sig == 0x464C4731u && state_off >= 0x800u && state_sz > 0 && state_sz < 4u * 1024u * 1024u)
	{
		uint32_t read_sz = (state_sz < 1024u) ? state_sz : 1024u;
		char buf[1025];
		if (fseek(f, (long)state_off, SEEK_SET) == 0 && fread(buf, 1, read_sz, f) == read_sz)
		{
			char const *p;
			buf[read_sz] = 0;
			p = strstr(buf, "Base Map: ");
			if (p)
			{
				size_t i = 0;
				p += 10;
				while (p[i] && p[i] != '\r' && p[i] != '\n' && i + 1 < out_sz)
				{
					out_stem[i] = p[i];
					i++;
				}
				out_stem[i] = 0;
			}
		}
	}
	fclose(f);
	return out_stem[0] ? 1 : 0;
}

static int map_file_has_equal_tag_count(char const *path_a, char const *path_b)
{
	FILE *fa;
	FILE *fb;
	unsigned char hdr_a[0x40];
	unsigned char hdr_b[0x40];
	unsigned char idx_a[0x28];
	unsigned char idx_b[0x28];
	unsigned long off_a, off_b;
	long cnt_a = -1, cnt_b = -2;

	fa = fopen(path_a, "rb");
	if (!fa)
		return 0;
	fb = fopen(path_b, "rb");
	if (!fb)
	{
		fclose(fa);
		return 0;
	}
	if (fread(hdr_a, 1, sizeof(hdr_a), fa) == sizeof(hdr_a) &&
		fread(hdr_b, 1, sizeof(hdr_b), fb) == sizeof(hdr_b))
	{
		off_a = *(unsigned long const *)(hdr_a + 0x10);
		off_b = *(unsigned long const *)(hdr_b + 0x10);
		if (fseek(fa, (long)off_a, SEEK_SET) == 0 &&
			fseek(fb, (long)off_b, SEEK_SET) == 0 &&
			fread(idx_a, 1, sizeof(idx_a), fa) == sizeof(idx_a) &&
			fread(idx_b, 1, sizeof(idx_b), fb) == sizeof(idx_b))
		{
			cnt_a = *(long const *)(idx_a + 0x0C);
			cnt_b = *(long const *)(idx_b + 0x0C);
		}
	}
	fclose(fa);
	fclose(fb);
	return (cnt_a > 0 && cnt_a == cnt_b) ? 1 : 0;
}

static int try_resolve_clean_ce_map_by_stem(char const *stem, char *out_path, size_t out_sz)
{
	static char const *const s_fmts[] = {
		"C:/Program Files (x86)/Microsoft Games/Halo Custom Edition/maps/%s.map",
		"C:/Program Files/Microsoft Games/Halo Custom Edition/maps/%s.map",
		"custom_maps/%s.map",
		"maps/%s.map"
	};
	char candidate[512], h_xbox[128], dummy_stem[64];
	size_t i;
	if (!stem || !stem[0])
		return 0;
	snprintf(h_xbox, sizeof(h_xbox), "h:\\maps\\%s.map", stem);
	platform_translate_path(h_xbox, candidate, sizeof(candidate));
	if (is_ce_v609_map_file(candidate) && !extract_embedded_forge_base_map_stem(candidate, dummy_stem, sizeof(dummy_stem)))
	{
		snprintf(out_path, out_sz, "%s", candidate);
		return 1;
	}
	for (i = 0; i < sizeof(s_fmts) / sizeof(s_fmts[0]); i++)
	{
		snprintf(candidate, sizeof(candidate), s_fmts[i], stem);
		if (is_ce_v609_map_file(candidate) && !extract_embedded_forge_base_map_stem(candidate, dummy_stem, sizeof(dummy_stem)))
		{
			snprintf(out_path, out_sz, "%s", candidate);
			return 1;
		}
	}
	return 0;
}

static void resolve_current_base_map_path(char *out_path, size_t out_sz)
{
	boolean is_ce_active = custom_edition_cache_tags_loaded();
	char const *ce_loaded = is_ce_active ? custom_edition_cache_loaded_path() : NULL;
	char const *map_leaf;
	char base_stem[64];
	char clean_candidate[512];
	FILE *f;

	if (is_ce_active && ce_loaded && ce_loaded[0])
	{
		char const *rel_ce = ce_loaded;
		if ((rel_ce[0] == 'd' || rel_ce[0] == 'D') && rel_ce[1] == ':' && (rel_ce[2] == '\\' || rel_ce[2] == '/'))
			rel_ce += 3;
		if (is_ce_v609_map_file(rel_ce))
		{
			if (extract_embedded_forge_base_map_stem(rel_ce, base_stem, sizeof(base_stem)) &&
				_stricmp(base_stem, rel_ce) != 0 &&
				try_resolve_clean_ce_map_by_stem(base_stem, clean_candidate, sizeof(clean_candidate)) &&
				map_file_has_equal_tag_count(rel_ce, clean_candidate))
			{
				snprintf(out_path, out_sz, "%s", clean_candidate);
				return;
			}
			snprintf(out_path, out_sz, "%s", rel_ce);
			return;
		}
	}

	map_leaf = forge_get_active_map_name();
	if (map_leaf && map_leaf[0])
	{
		char const *slash = strrchr(map_leaf, '\\');
		char const *slash2 = strrchr(map_leaf, '/');
		char candidate[512];
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		if (slash)
			map_leaf = slash + 1;

		if (is_ce_active)
		{
			snprintf(candidate, sizeof(candidate), "custom_maps/%s.map", map_leaf);
			if (is_ce_v609_map_file(candidate))
			{
				if (extract_embedded_forge_base_map_stem(candidate, base_stem, sizeof(base_stem)) &&
					_stricmp(base_stem, map_leaf) != 0 &&
					try_resolve_clean_ce_map_by_stem(base_stem, clean_candidate, sizeof(clean_candidate)) &&
					map_file_has_equal_tag_count(candidate, clean_candidate))
				{
					snprintf(out_path, out_sz, "%s", clean_candidate);
					return;
				}
				snprintf(out_path, out_sz, "%s", candidate);
				return;
			}
			if (try_resolve_clean_ce_map_by_stem(map_leaf, out_path, out_sz))
				return;
		}
		else
		{
			/* Active map is an Xbox v5 map: check maps/<leaf>.map and d:\maps\<leaf>.map first,
			 * then custom_maps/<leaf>.map for an already-saved Xbox FLG1 map. Never substitute a Custom Edition map! */
			char d_path[160];
			snprintf(candidate, sizeof(candidate), "maps/%s.map", map_leaf);
			if ((f = fopen(candidate, "rb")) != NULL)
			{
				fclose(f);
				snprintf(out_path, out_sz, "%s", candidate);
				return;
			}
			snprintf(d_path, sizeof(d_path), "d:\\maps\\%s.map", map_leaf);
			candidate[0] = '\0';
			platform_translate_path(d_path, candidate, sizeof(candidate));
			if (candidate[0] && (f = fopen(candidate, "rb")) != NULL)
			{
				fclose(f);
				snprintf(out_path, out_sz, "%s", candidate);
				return;
			}
			snprintf(d_path, sizeof(d_path), "d:\\custom_maps\\%s.map", map_leaf);
			candidate[0] = '\0';
			platform_translate_path(d_path, candidate, sizeof(candidate));
			if (candidate[0] && !is_ce_v609_map_file(candidate) && (f = fopen(candidate, "rb")) != NULL)
			{
				fclose(f);
				snprintf(out_path, out_sz, "%s", candidate);
				return;
			}
			snprintf(candidate, sizeof(candidate), "custom_maps/%s.map", map_leaf);
			if (!is_ce_v609_map_file(candidate) && (f = fopen(candidate, "rb")) != NULL)
			{
				fclose(f);
				snprintf(out_path, out_sz, "%s", candidate);
				return;
			}
		}

		snprintf(candidate, sizeof(candidate), "maps/%s.map", map_leaf);
		if ((f = fopen(candidate, "rb")) != NULL)
		{
			fclose(f);
			snprintf(out_path, out_sz, "%s", candidate);
			return;
		}
	}

	if (is_ce_active && try_resolve_clean_ce_map_by_stem("bloodgulch", out_path, out_sz))
		return;
	if ((f = fopen("maps/bloodgulch.map", "rb")) != NULL)
	{
		fclose(f);
		snprintf(out_path, out_sz, "maps/bloodgulch.map");
		return;
	}
	snprintf(out_path, out_sz, "maps/bloodgulch.map");
}

extern void network_game_server_port_refresh_map_version(void);
extern int halo_server_mode(void);

static int s_forge_server_dirty_txt = 0;
static int s_forge_server_dirty_map = 0;
static unsigned long s_forge_server_last_edit_ms = 0;
static int s_forge_server_prev_player_count = 0;
static char s_pending_sidecar_leaf[64] = "";
static char s_active_sidecar_base_map[128] = "";
static char s_deferred_client_save_name[64] = "";

static void forge_commit_session_ephemeral_tags_to_disk(void);

void forge_set_pending_sidecar_leaf(char const *leaf)
{
	if (leaf && leaf[0])
	{
		snprintf(s_pending_sidecar_leaf, sizeof(s_pending_sidecar_leaf), "%s", leaf);
	}
	else
	{
		s_pending_sidecar_leaf[0] = '\0';
		s_active_sidecar_base_map[0] = '\0';
	}
}

int forge_has_pending_or_active_sidecar(void)
{
	return (s_pending_sidecar_leaf[0] != '\0' || g_forge.live_instance_count > 0) ? 1 : 0;
}

static int forge_write_sidecar_state_file(char const *leaf);

static int is_existing_custom_map_file_flg1(char const *leaf, int *out_file_exists)
{
	char d_map[160], host_map[512];
	FILE *f;
	uint8_t hdr[0x200];
	uint32_t sig = 0;

	if (out_file_exists)
		*out_file_exists = 0;
	if (!leaf || !leaf[0])
		return 0;

	snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.map", leaf);
	host_map[0] = '\0';
	platform_translate_path(d_map, host_map, sizeof(host_map));
	f = host_map[0] ? fopen(host_map, "rb") : NULL;
	if (!f)
	{
		snprintf(host_map, sizeof(host_map), "custom_maps/%s.map", leaf);
		f = fopen(host_map, "rb");
	}
	if (!f)
		return 0;

	if (out_file_exists)
		*out_file_exists = 1;
	if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr))
		memcpy(&sig, hdr + 0x1FCu, 4);
	fclose(f);
	return (sig == 0x464C4731u) ? 1 : 0;
}

void forge_deferred_client_save_tick(void)
{
	char dl_status[128];
	char pending_name[64];

	if (!s_deferred_client_save_name[0])
		return;
	if (forge_network_get_tag_download_status(dl_status, sizeof(dl_status)))
		return;

	snprintf(pending_name, sizeof(pending_name), "%s", s_deferred_client_save_name);
	s_deferred_client_save_name[0] = '\0';
	forge_ui_save_copy_as(pending_name);
}

int forge_ui_save_copy_as(char const *new_map_name)
{
	char base_map_path[512];
	char clean_name[64];
	int was_already_saved = g_forge.has_saved_map_once;
	int map_exists = 0;
	int is_legacy_flg1_map = 0;
	int use_sidecar_only = 1;
	int ok;

	if (!new_map_name || !new_map_name[0])
	{
		char const *cur = forge_get_active_map_name();
		char const *slash = cur ? strrchr(cur, '\\') : NULL;
		char const *slash2 = cur ? strrchr(cur, '/') : NULL;
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		if (halo_server_mode())
			snprintf(clean_name, sizeof(clean_name), "%s", slash ? (slash + 1) : (cur && cur[0] ? cur : "bloodgulch"));
		else
			snprintf(clean_name, sizeof(clean_name), "%s_forge", slash ? (slash + 1) : (cur && cur[0] ? cur : "bloodgulch"));
	}
	else
	{
		snprintf(clean_name, sizeof(clean_name), "%s", new_map_name);
	}

	{
		char dl_status[128];
		if (forge_network_get_tag_download_status(dl_status, sizeof(dl_status)))
		{
			snprintf(s_deferred_client_save_name, sizeof(s_deferred_client_save_name), "%s", clean_name);
			forge_set_status("Waiting for custom tags to finish streaming before saving '%s'...", clean_name);
			return 1;
		}
	}
	s_deferred_client_save_name[0] = '\0';
	forge_commit_session_ephemeral_tags_to_disk();

	resolve_current_base_map_path(base_map_path, sizeof(base_map_path));

	{
		char d_dir[512];
		d_dir[0] = '\0';
		platform_translate_path("d:\\custom_maps", d_dir, sizeof(d_dir));
		if (d_dir[0])
			CreateDirectoryA(d_dir, NULL);
	}
	CreateDirectoryA("d:\\custom_maps", NULL);
	CreateDirectoryA("custom_maps", NULL);

	is_legacy_flg1_map = is_existing_custom_map_file_flg1(clean_name, &map_exists);
	if (map_exists && !is_legacy_flg1_map && !halo_server_mode())
	{
		/* Never overwrite or shadow a base non-Forge Custom Edition .map in custom_maps/ */
		char suffixed[64];
		snprintf(suffixed, sizeof(suffixed), "%s_forge", clean_name);
		snprintf(clean_name, sizeof(clean_name), "%s", suffixed);
	}
	(void)use_sidecar_only;

	{
		char const *cur = forge_get_active_map_name();
		char const *slash = cur ? strrchr(cur, '\\') : NULL;
		char const *slash2 = cur ? strrchr(cur, '/') : NULL;
		char const *active_leaf;
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		active_leaf = slash ? (slash + 1) : (cur && cur[0] ? cur : "");
		ok = forge_write_sidecar_state_file(clean_name);
		if (ok)
			snprintf(s_pending_sidecar_leaf, sizeof(s_pending_sidecar_leaf), "%s", clean_name);
		if (ok && active_leaf[0] && _stricmp(active_leaf, clean_name) != 0)
		{
			/* Remove any leftover default-named sidecar save (e.g. custom_maps/<active_leaf>.txt)
			 * so saving with a custom name never leaves two saves or pollutes the stock base map. */
			char d_map[160], d_txt[160], host_map[512], host_txt[512];
			FILE *f_check;
			snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.map", active_leaf);
			host_map[0] = '\0';
			platform_translate_path(d_map, host_map, sizeof(host_map));
			f_check = host_map[0] ? fopen(host_map, "rb") : NULL;
			if (!f_check)
			{
				snprintf(host_map, sizeof(host_map), "custom_maps/%s.map", active_leaf);
				f_check = fopen(host_map, "rb");
			}
			if (f_check)
			{
				fclose(f_check);
			}
			else
			{
				snprintf(d_txt, sizeof(d_txt), "d:\\custom_maps\\%s.txt", active_leaf);
				host_txt[0] = '\0';
				platform_translate_path(d_txt, host_txt, sizeof(host_txt));
				if (host_txt[0])
					remove(host_txt);
				snprintf(host_txt, sizeof(host_txt), "custom_maps/%s.txt", active_leaf);
				remove(host_txt);
			}
		}
	}

	if (ok)
	{
		char const *saved_ext = "txt";
		s_forge_server_dirty_txt = 0;
		s_forge_server_dirty_map = 0;
		forge_menu_custom_edition_maps_look_again();
		network_game_server_port_refresh_map_version();
		snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s", clean_name);
		g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
		g_forge.has_saved_map_once = 1;

		if (was_already_saved)
		{
			forge_set_status("Updated saved map 'custom_maps/%s.%s' (%d placed, %d default edits)!",
				clean_name,
				saved_ext,
				(int)g_forge.edit_state.placed_object_count,
				(int)g_forge.edit_state.default_edit_count);
		}
		else
		{
			forge_set_status("Initial save complete: 'custom_maps/%s.%s'! Press F5 anytime to quick-update.",
				clean_name,
				saved_ext);
		}
		return 1;
	}
	forge_set_status("ERROR: Failed to export custom_maps/%s.txt", clean_name);
	return 0;
}

void forge_trigger_save_map_action(void)
{
	/* If connected as a client in a multiplayer Forge game, also tell a dedicated host to flush its map file */
	if (game_connection() == _game_connection_network_client)
	{
		extern void forge_send_packet_to_host(char const *ascii_payload);
		forge_send_packet_to_host("!FW");
	}

	/* If an initial save has already been done in this session (or loaded from Edit Maps), quick-update immediately! */
	if (g_forge.has_saved_map_once && g_forge.save_map_name[0])
	{
		g_forge.active_modal = _forge_modal_none;
		forge_ui_save_copy_as(g_forge.save_map_name);
		return;
	}

	/* Otherwise, open the initial save prompt so the user can name their custom map */
	if (!g_forge.save_map_name[0])
	{
		char const *cur = forge_get_active_map_name();
		char const *slash = cur ? strrchr(cur, '\\') : NULL;
		char const *slash2 = cur ? strrchr(cur, '/') : NULL;
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s_forge", slash ? (slash + 1) : (cur && cur[0] ? cur : "custom"));
		g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
	}
	g_forge.active_modal = _forge_modal_save_copy_as;
}

extern void sync_live_player_spawns_to_scenario(void);
extern void sync_live_netgame_flags_to_scenario(void);

static void parse_forge_state_text_buffer(char const *text_buf, char const *leaf)
{
	char const *cur = text_buf;
	char line[512];
	int in_forge_state = 0;
	int cleared_default_spawns = 0;
	int cleared_default_flags = 0;
	int32_t last_object_li = -1;
	int32_t last_object_po = -1;
	int32_t last_default_li = -1;
	int32_t last_default_de = -1;
	int32_t last_ai_li = -1;
	int32_t last_ai_idx = -1;
	int16_t placed_to_li[FORGE_MAX_PLACED_OBJECTS];
	int16_t def_to_li[FORGE_MAX_DEFAULT_OBJECT_EDITS];
	int16_t ai_to_li[FORGE_MAX_AI_SPAWNS];
	int map_i;

	if (!cur)
		return;

	for (map_i = 0; map_i < FORGE_MAX_PLACED_OBJECTS; map_i++)
		placed_to_li[map_i] = -1;
	for (map_i = 0; map_i < FORGE_MAX_DEFAULT_OBJECT_EDITS; map_i++)
		def_to_li[map_i] = -1;
	for (map_i = 0; map_i < FORGE_MAX_AI_SPAWNS; map_i++)
		ai_to_li[map_i] = -1;

	while (*cur)
	{
		size_t len = 0;
		while (cur[len] && cur[len] != '\r' && cur[len] != '\n')
			len++;
		if (len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, cur, len);
		line[len] = 0;
		cur += len;
		while (*cur == '\r' || *cur == '\n')
			cur++;

		if (strcmp(line, "[OpenCE Forge Map]") == 0)
		{
			/* This map is already a saved Forge map: enable quick-update on F5! */
			snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s", leaf);
			g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
			g_forge.has_saved_map_once = 1;
		}
		else if (strcmp(line, "[ForgeState]") == 0)
		{
			in_forge_state = 1;
		}
		else if (in_forge_state && strncmp(line, "OBJECT|", 7) == 0)
		{
			unsigned int grp = 0;
			int src_cache = 0, baked = 0, team = 0;
			float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
			char tpath[256];
			tpath[0] = 0;
			if (sscanf(line + 7, "%u|%d|%d|%f|%f|%f|%f|%f|%f|%d|%255[^\r\n]",
					&grp, &src_cache, &baked, &x, &y, &z, &yaw, &pitch, &roll, &team, tpath) == 11 && tpath[0])
			{
				if (!baked)
				{
					/* Cached tag object not baked into base_map: spawn live now! */
					last_object_li = forge_3d_spawn_object_at((uint32_t)grp, tpath, (uint8_t)src_cache, x, y, z, yaw, pitch, roll, (int16_t)team, 0);
					last_object_po = (last_object_li >= 0) ? g_forge.live_instances[last_object_li].edit_entry_index : -1;
					if (last_object_po >= 0 && last_object_po < FORGE_MAX_PLACED_OBJECTS)
						placed_to_li[last_object_po] = (int16_t)last_object_li;
				}
				else
				{
					struct object_iterator iter;
					struct object_datum *obj;
					long found_oidx = NONE;
					int32_t existing_default_li = -1;
					float best_d2 = 100.0f;

					object_iterator_new(&iter, (unsigned long)-1, 0);
					while ((obj = object_iterator_next(&iter)) != NULL)
					{
						int li, already_claimed = 0;
						int32_t match_default_li = -1;
						float dx, dy, dz, d2;
						char const *oname = NULL;
						if (obj->object.parent_object_index != NONE)
							continue;
						if (obj->object.type == _object_type_biped &&
							((struct unit_datum *)obj)->unit.player_index != NONE)
							continue;
						for (li = 0; li < g_forge.live_instance_count; li++)
						{
							if (g_forge.live_instances[li].active &&
								g_forge.live_instances[li].object_index == iter.index)
							{
								if (g_forge.live_instances[li].is_default_scenario_object)
								{
									match_default_li = li;
								}
								else
								{
									already_claimed = 1;
									break;
								}
							}
						}
						if (already_claimed)
							continue;
						if (obj->definition_index != NONE)
							oname = tag_get_name(obj->definition_index);
						if (oname && oname[0])
						{
							size_t tlen = strlen(tpath);
							if (_stricmp(oname, tpath) != 0 &&
								_strnicmp(oname, tpath, tlen) != 0 &&
								!(grp == 'vehi' && _strnicmp(oname, "forge\\vehicles\\placed_unit_", 27) == 0))
								continue;
						}
						dx = obj->object.position.x - x;
						dy = obj->object.position.y - y;
						dz = obj->object.position.z - z;
						/* Physics objects (vehicles/weapons) spawn with +bounding_sphere_radius*0.5f on Z */
						if (dx * dx + dy * dy > 16.0f || (float)fabs((double)dz) > 15.0f)
							continue;
						d2 = dx * dx + dy * dy + dz * dz * 0.25f;
						if (d2 < best_d2)
						{
							best_d2 = d2;
							found_oidx = iter.index;
							existing_default_li = match_default_li;
						}
					}

					if (found_oidx == NONE)
					{
						/* Not yet spawned by scenario: spawn live and record in placed_objects */
						last_object_li = forge_3d_spawn_object_at((uint32_t)grp, tpath, (uint8_t)src_cache, x, y, z, yaw, pitch, roll, (int16_t)team, 0);
						last_object_po = (last_object_li >= 0) ? g_forge.live_instances[last_object_li].edit_entry_index : -1;
						if (last_object_po >= 0 && last_object_po < FORGE_MAX_PLACED_OBJECTS)
							placed_to_li[last_object_po] = (int16_t)last_object_li;
					}
					else if (g_forge.edit_state.placed_object_count < FORGE_MAX_PLACED_OBJECTS)
					{
						/* Record baked object in edit_state and link to its live instance */
						int32_t po_idx = g_forge.edit_state.placed_object_count++;
						struct forge_placed_object_entry *po = &g_forge.edit_state.placed_objects[po_idx];
						struct object_datum *fobj = (struct object_datum *)object_try_and_get_and_verify_type(found_oidx, (unsigned long)-1);

						last_object_po = po_idx;
						last_object_li = -1;

						memset(po, 0, sizeof(*po));
						po->active = 1;
						po->group_tag = (uint32_t)grp;
						po->source_cache_id = (uint8_t)src_cache;
						po->category = (grp == 'vehi') ? _forge_object_vehicle :
							(grp == 'weap') ? _forge_object_weapon :
							(grp == 'bipd') ? _forge_object_biped :
							(grp == 'eqip') ? _forge_object_equipment : _forge_object_scenery;
						snprintf(po->tag_path, sizeof(po->tag_path), "%s", tpath);
						po->x = x; po->y = y; po->z = z;
						po->yaw = yaw; po->pitch = pitch; po->roll = roll;
						po->team_index = (int16_t)team;

						if (fobj && fobj->definition_index != NONE)
						{
							if ((uint32_t)grp == 'vehi')
								patch_vehicle_tag_for_player_driving(fobj->definition_index, tpath);
							else
								patch_forge_device_and_light_tag_runtime(fobj->definition_index, (uint32_t)grp, tpath);
						}

						if (existing_default_li >= 0 || g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
						{
							int32_t li_idx = (existing_default_li >= 0) ? existing_default_li : g_forge.live_instance_count++;
							struct forge_live_instance *inst = &g_forge.live_instances[li_idx];
							memset(inst, 0, sizeof(*inst));
							inst->active = 1;
							inst->object_index = found_oidx;
							inst->actor_index = NONE;
							inst->edit_entry_index = po_idx;
							inst->group_tag = (uint32_t)grp;
							inst->source_cache_id = (uint8_t)src_cache;
							inst->is_default_scenario_object = 0;
							inst->spawned_once = 1;
							inst->death_tick = -1;
							inst->team_designator = (int16_t)team;
							inst->x = x; inst->y = y; inst->z = z;
							inst->yaw = yaw; inst->pitch = pitch; inst->roll = roll;
							snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tpath);
							forge_3d_move_instance(li_idx, x, y, z, yaw, pitch, roll, 0);
							last_object_li = li_idx;
							placed_to_li[po_idx] = (int16_t)li_idx;
						}
					}
				}
			}
		}
		else if (in_forge_state && strncmp(line, "AI|", 3) == 0)
		{
			float x = 0, y = 0, z = 0, yaw = 0;
			int team = 0;
			char tpath[256];
			tpath[0] = 0;
			if (sscanf(line + 3, "%f|%f|%f|%f|%d|%255[^\r\n]", &x, &y, &z, &yaw, &team, tpath) == 6 && tpath[0])
			{
				last_ai_li = forge_3d_spawn_object_at('actv', tpath, 0, x, y, z, yaw, 0.0f, 0.0f, (int16_t)team, 0);
				last_ai_idx = (last_ai_li >= 0) ? g_forge.live_instances[last_ai_li].edit_entry_index : (g_forge.edit_state.ai_spawn_count - 1);
				if (last_ai_idx >= 0 && last_ai_idx < FORGE_MAX_AI_SPAWNS)
					ai_to_li[last_ai_idx] = (int16_t)last_ai_li;
			}
		}
		else if (in_forge_state && strncmp(line, "AI_RESPAWN|", 11) == 0)
		{
			int aidx = -1, rsec = 0;
			int nread = sscanf(line + 11, "%d|%d", &aidx, &rsec);
			if (nread == 1)
			{
				rsec = aidx;
				aidx = last_ai_idx;
			}
			if (nread >= 1 && rsec >= 0)
			{
				int32_t target_li = (aidx >= 0 && aidx < FORGE_MAX_AI_SPAWNS && ai_to_li[aidx] >= 0) ? ai_to_li[aidx] : last_ai_li;
				if (aidx >= 0 && aidx < g_forge.edit_state.ai_spawn_count)
					g_forge.edit_state.ai_spawns[aidx].respawn_timer_sec = (int16_t)rsec;
				else if (g_forge.edit_state.ai_spawn_count > 0)
					g_forge.edit_state.ai_spawns[g_forge.edit_state.ai_spawn_count - 1].respawn_timer_sec = (int16_t)rsec;
				if (target_li >= 0 && target_li < g_forge.live_instance_count && g_forge.live_instances[target_li].is_ai_unit)
					g_forge.live_instances[target_li].respawn_timer_sec = (int16_t)rsec;
			}
		}
		else if (in_forge_state && strncmp(line, "AI_ORDER|", 9) == 0)
		{
			int aidx = -1;
			unsigned int aorder = 0, agroup = 0;
			int nread = sscanf(line + 9, "%d|%u|%u", &aidx, &aorder, &agroup);
			if (nread == 1)
			{
				aorder = (unsigned int)aidx;
				aidx = last_ai_idx;
			}
			if (nread >= 1)
			{
				int32_t target_li = (aidx >= 0 && aidx < FORGE_MAX_AI_SPAWNS && ai_to_li[aidx] >= 0) ? ai_to_li[aidx] : last_ai_li;
				if (aorder >= NUMBER_OF_FORGE_AI_ORDERS)
					aorder = 0;
				if (agroup > FORGE_MAX_WELD_GROUPS)
					agroup = 0;
				if (aidx >= 0 && aidx < g_forge.edit_state.ai_spawn_count)
				{
					g_forge.edit_state.ai_spawns[aidx].ai_order = (uint8_t)aorder;
					if (nread == 3)
						g_forge.edit_state.ai_spawns[aidx].group_id = (uint8_t)agroup;
				}
				if (target_li >= 0 && target_li < g_forge.live_instance_count && g_forge.live_instances[target_li].is_ai_unit)
				{
					struct forge_live_instance *ainst = &g_forge.live_instances[target_li];
					ainst->ai_order = (uint8_t)aorder;
					if (nread == 3)
						ainst->group_id = (uint8_t)agroup;
					forge_ai_apply_order(ainst->actor_index, ainst->object_index, ainst->ai_order,
						ainst->x, ainst->y, ainst->z, game_time_get());
				}
			}
		}
		else if (in_forge_state && strncmp(line, "AI_GROUP|", 9) == 0)
		{
			int aidx = -1;
			unsigned int agroup = 0;
			if (sscanf(line + 9, "%d|%u", &aidx, &agroup) == 2 && agroup <= FORGE_MAX_WELD_GROUPS)
			{
				int32_t target_li = (aidx >= 0 && aidx < FORGE_MAX_AI_SPAWNS && ai_to_li[aidx] >= 0) ? ai_to_li[aidx] : last_ai_li;
				if (aidx >= 0 && aidx < g_forge.edit_state.ai_spawn_count)
					g_forge.edit_state.ai_spawns[aidx].group_id = (uint8_t)agroup;
				if (target_li >= 0 && target_li < g_forge.live_instance_count && g_forge.live_instances[target_li].is_ai_unit)
					g_forge.live_instances[target_li].group_id = (uint8_t)agroup;
			}
		}
		else if (in_forge_state && strncmp(line, "DEFAULT_EDIT|", 13) == 0)
		{
			unsigned int grp = 0;
			int deleted = 0;
			float ox = 0, oy = 0, oz = 0, nx = 0, ny = 0, nz = 0, nyaw = 0, npitch = 0, nroll = 0;
			char tpath[256];
			tpath[0] = 0;
			if (sscanf(line + 13, "%u|%d|%f|%f|%f|%f|%f|%f|%f|%f|%f|%255[^\r\n]",
					&grp, &deleted, &ox, &oy, &oz, &nx, &ny, &nz, &nyaw, &npitch, &nroll, tpath) == 12 &&
				g_forge.edit_state.default_edit_count < FORGE_MAX_DEFAULT_OBJECT_EDITS)
			{
				int32_t de_idx = g_forge.edit_state.default_edit_count++;
				struct forge_default_object_edit *de = &g_forge.edit_state.default_edits[de_idx];
				struct object_iterator iter;
				struct object_datum *obj;
				long found_oidx = NONE;
				float best_d2 = 36.0f;

				memset(de, 0, sizeof(*de));
				de->active = 1;
				de->group_tag = (uint32_t)grp;
				de->deleted = (uint8_t)deleted;
				de->orig_x = ox; de->orig_y = oy; de->orig_z = oz;
				de->new_x = nx; de->new_y = ny; de->new_z = nz;
				de->new_yaw = nyaw; de->new_pitch = npitch; de->new_roll = nroll;
				snprintf(de->tag_path, sizeof(de->tag_path), "%s", tpath);

				/* Also update scenario placement blocks in RAM (for Xbox v5 FLG1 containers where disk scenario is unedited) */
				{
					struct scenario *scen_ram = global_scenario_try_and_get();
					if (scen_ram)
					{
						long si;
						for (si = 0; si < scen_ram->netgame_equipment.count; si++)
						{
							struct scenario_netgame_equipment *eq = TAG_BLOCK_GET_ELEMENT(
								&scen_ram->netgame_equipment, si, struct scenario_netgame_equipment);
							float dx = eq->position.x - ox;
							float dy = eq->position.y - oy;
							float dz = eq->position.z - oz;
							if (dx * dx + dy * dy + dz * dz < 16.0f)
							{
								if (deleted)
								{
									eq->item_collection.index = NONE;
									eq->position.x = -99999.0f;
									eq->position.y = -99999.0f;
									eq->position.z = -99999.0f;
								}
								else
								{
									eq->position.x = nx;
									eq->position.y = ny;
									eq->position.z = nz;
									eq->facing = nyaw;
								}
								break;
							}
						}
						for (si = 0; si < scen_ram->vehicles.count; si++)
						{
							struct scenario_object_datum *sv = (struct scenario_object_datum *)tag_block_get_element_fast(
								&scen_ram->vehicles, si, 120);
							if (sv)
							{
								float dx = sv->position.x - ox;
								float dy = sv->position.y - oy;
								float dz = sv->position.z - oz;
								if (dx * dx + dy * dy < 16.0f && (float)fabs((double)dz) < 12.0f)
								{
									if (deleted)
									{
										sv->palette_entry_index = NONE;
										sv->position.x = -99999.0f;
										sv->position.y = -99999.0f;
										sv->position.z = -99999.0f;
									}
									else
									{
										sv->position.x = nx;
										sv->position.y = ny;
										sv->position.z = nz;
										sv->rotation.yaw = nyaw;
										sv->rotation.pitch = npitch;
										sv->rotation.roll = nroll;
									}
									break;
								}
							}
						}
					}
				}

				/* Apply default edit live if the scenario object is still at its unedited origin (e.g. Xbox v5 FLG1 container) */
				{
					int li;
					int32_t matched_li = -1;
					float max_xy_d2 = (grp == 'scen' || grp == 'mach' || grp == 'lifi' || grp == 'ctrl') ? 0.04f : 16.0f;
					float best_li_d2 = max_xy_d2;

					for (li = 0; li < g_forge.live_instance_count; li++)
					{
						struct forge_live_instance *inst = &g_forge.live_instances[li];
						float dx, dy, dz, d2;
						if (!inst->active || !inst->is_default_scenario_object || inst->edit_entry_index >= 0)
							continue;
						if (inst->group_tag != (uint32_t)grp || _stricmp(inst->tag_path, tpath) != 0)
							continue;
						dx = inst->orig_x - ox;
						dy = inst->orig_y - oy;
						dz = inst->orig_z - oz;
						d2 = dx * dx + dy * dy + dz * dz * 0.25f;
						if (dx * dx + dy * dy <= max_xy_d2 && (float)fabs((double)dz) <= 12.0f && d2 < best_li_d2)
						{
							best_li_d2 = d2;
							matched_li = li;
							found_oidx = inst->object_index;
						}
					}

					if (matched_li < 0)
					{
						best_d2 = max_xy_d2;
						object_iterator_new(&iter, (unsigned long)-1, 0);
						while ((obj = object_iterator_next(&iter)) != NULL)
						{
							float dx = obj->object.position.x - ox;
							float dy = obj->object.position.y - oy;
							float dz = obj->object.position.z - oz;
							float d2 = dx * dx + dy * dy + dz * dz * 0.25f;
							int claimed = 0;
							if (obj->object.parent_object_index != NONE)
								continue;
							if (obj->object.type == _object_type_biped &&
								((struct unit_datum *)obj)->unit.player_index != NONE)
								continue;
							if (obj->definition_index != NONE)
							{
								char const *oname = tag_get_name(obj->definition_index);
								if (oname && oname[0] && _stricmp(oname, tpath) != 0)
									continue;
							}
							for (li = 0; li < g_forge.live_instance_count; li++)
							{
								if (g_forge.live_instances[li].active &&
									g_forge.live_instances[li].object_index == iter.index &&
									(!g_forge.live_instances[li].is_default_scenario_object ||
									 g_forge.live_instances[li].edit_entry_index >= 0))
								{
									claimed = 1;
									break;
								}
							}
							if (claimed)
								continue;
							if (dx * dx + dy * dy <= max_xy_d2 && (float)fabs((double)dz) <= 12.0f && d2 < best_d2)
							{
								best_d2 = d2;
								found_oidx = iter.index;
							}
						}
					}
				}
				if (found_oidx != NONE)
				{
					if (deleted)
					{
						int li, recorded_del = 0;
						for (li = 0; li < g_forge.live_instance_count; li++)
						{
							if (g_forge.live_instances[li].active &&
								g_forge.live_instances[li].object_index == found_oidx)
							{
								g_forge.live_instances[li].active = 0;
								g_forge.live_instances[li].object_index = NONE;
								g_forge.live_instances[li].edit_entry_index = de_idx;
								recorded_del = 1;
							}
						}
						if (!recorded_del && g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
						{
							struct forge_live_instance *dinst = &g_forge.live_instances[g_forge.live_instance_count++];
							memset(dinst, 0, sizeof(*dinst));
							dinst->active = 0;
							dinst->object_index = NONE;
							dinst->actor_index = NONE;
							dinst->edit_entry_index = de_idx;
							dinst->group_tag = (uint32_t)grp;
							dinst->is_default_scenario_object = 1;
							dinst->death_tick = -1;
							dinst->orig_x = ox;
							dinst->orig_y = oy;
							dinst->orig_z = oz;
							snprintf(dinst->tag_path, sizeof(dinst->tag_path), "%s", tpath);
						}
						object_delete(found_oidx);
					}
					else
					{
						int32_t li_idx = forge_3d_adopt_or_find_object_instance(found_oidx);
						last_default_de = de_idx;
						last_default_li = li_idx;
						if (de_idx >= 0 && de_idx < FORGE_MAX_DEFAULT_OBJECT_EDITS)
							def_to_li[de_idx] = (int16_t)li_idx;
						if (li_idx >= 0)
						{
							g_forge.live_instances[li_idx].edit_entry_index = de_idx;
							forge_3d_move_instance(li_idx, nx, ny, nz, nyaw, npitch, nroll, 0);
						}
					}
				}
			}
		}
		else if (in_forge_state && strncmp(line, "DEFAULT_GAMETYPE|", 17) == 0)
		{
			int didx = -1;
			unsigned int gflags = 0;
			int nread = sscanf(line + 17, "%d|%u", &didx, &gflags);
			if (nread == 1)
			{
				gflags = (unsigned int)didx;
				didx = last_default_de;
			}
			if (nread >= 1)
			{
				int32_t target_li = (didx >= 0 && didx < FORGE_MAX_DEFAULT_OBJECT_EDITS && def_to_li[didx] >= 0) ? def_to_li[didx] : last_default_li;
				if (didx >= 0 && didx < g_forge.edit_state.default_edit_count)
					g_forge.edit_state.default_edits[didx].gametype_flags = (uint16_t)gflags;
				if (target_li >= 0 && target_li < g_forge.live_instance_count)
					g_forge.live_instances[target_li].gametype_flags = (uint16_t)gflags;
			}
		}
		else if (in_forge_state && strncmp(line, "DEFAULT_PROPS|", 14) == 0)
		{
			int didx = -1, rsec = 0;
			unsigned int pmode = 0, gid = 0, lcol = 0, lint = 0, lrad = 0;
			if (sscanf(line + 14, "%d|%u|%d|%u|%u|%u|%u",
					&didx, &pmode, &rsec, &gid, &lcol, &lint, &lrad) >= 4)
			{
				int32_t target_de = (didx >= 0 && didx < g_forge.edit_state.default_edit_count) ? didx : last_default_de;
				int32_t target_li = (target_de >= 0 && target_de < FORGE_MAX_DEFAULT_OBJECT_EDITS && def_to_li[target_de] >= 0)
					? def_to_li[target_de] : last_default_li;

				if (pmode >= NUMBER_OF_FORGE_PHYSICS_MODES) pmode = 0;
				if (gid > FORGE_MAX_WELD_GROUPS) gid = 0;
				if (lcol >= NUMBER_OF_FORGE_LIGHT_COLORS) lcol = 0;
				if (lint >= NUMBER_OF_FORGE_LIGHT_INTENSITIES) lint = 0;
				if (lrad >= NUMBER_OF_FORGE_LIGHT_RADII) lrad = 0;
				if (rsec < 0) rsec = 0;

				if (target_de >= 0 && target_de < g_forge.edit_state.default_edit_count)
				{
					struct forge_default_object_edit *de = &g_forge.edit_state.default_edits[target_de];
					de->physics_mode = (uint8_t)pmode;
					de->respawn_timer_sec = (int16_t)rsec;
					de->group_id = (uint8_t)gid;
					de->light_color_preset = (uint8_t)lcol;
					de->light_intensity_preset = (uint8_t)lint;
					de->light_radius_preset = (uint8_t)lrad;
				}
				if (target_li >= 0 && target_li < g_forge.live_instance_count)
				{
					struct forge_live_instance *inst = &g_forge.live_instances[target_li];
					inst->physics_mode = (uint8_t)pmode;
					inst->respawn_timer_sec = (int16_t)rsec;
					inst->group_id = (uint8_t)gid;
					inst->light_color_preset = (uint8_t)lcol;
					inst->light_intensity_preset = (uint8_t)lint;
					inst->light_radius_preset = (uint8_t)lrad;
					forge_apply_instance_light_settings(target_li);
					forge_3d_move_instance_ext(target_li, inst->x, inst->y, inst->z,
						inst->yaw, inst->pitch, inst->roll, 0, 0);
				}
			}
		}
		else if (in_forge_state && strncmp(line, "OBJECT_GAMETYPE|", 16) == 0)
		{
			unsigned int gflags = 0;
			if (sscanf(line + 16, "%u", &gflags) == 1)
			{
				if (last_object_po >= 0 && last_object_po < g_forge.edit_state.placed_object_count)
					g_forge.edit_state.placed_objects[last_object_po].gametype_flags = (uint16_t)gflags;
				else if (g_forge.edit_state.placed_object_count > 0)
					g_forge.edit_state.placed_objects[g_forge.edit_state.placed_object_count - 1].gametype_flags = (uint16_t)gflags;
				if (last_object_li >= 0 && last_object_li < g_forge.live_instance_count)
					g_forge.live_instances[last_object_li].gametype_flags = (uint16_t)gflags;
				else if (g_forge.live_instance_count > 0)
					g_forge.live_instances[g_forge.live_instance_count - 1].gametype_flags = (uint16_t)gflags;
			}
		}
		else if (in_forge_state && strncmp(line, "OBJECT_PROPS|", 13) == 0)
		{
			int pidx = -1, rsec = 0;
			unsigned int pmode = 0, gid = 0, lcol = 0, lint = 0, lrad = 0;
			int parsed = 0;
			if (sscanf(line + 13, "%d|%u|%d|%u|%u|%u|%u",
					&pidx, &pmode, &rsec, &gid, &lcol, &lint, &lrad) == 7)
			{
				parsed = 1;
			}
			else if (sscanf(line + 13, "%u|%d|%u|%u|%u|%u",
					&pmode, &rsec, &gid, &lcol, &lint, &lrad) >= 3)
			{
				pidx = last_object_po;
				parsed = 1;
			}
			if (parsed)
			{
				int32_t target_po = (pidx >= 0 && pidx < g_forge.edit_state.placed_object_count) ? pidx : last_object_po;
				int32_t target_li = (target_po >= 0 && target_po < FORGE_MAX_PLACED_OBJECTS && placed_to_li[target_po] >= 0)
					? placed_to_li[target_po] : last_object_li;

				if (pmode >= NUMBER_OF_FORGE_PHYSICS_MODES) pmode = 0;
				if (gid > FORGE_MAX_WELD_GROUPS) gid = 0;
				if (lcol >= NUMBER_OF_FORGE_LIGHT_COLORS) lcol = 0;
				if (lint >= NUMBER_OF_FORGE_LIGHT_INTENSITIES) lint = 0;
				if (lrad >= NUMBER_OF_FORGE_LIGHT_RADII) lrad = 0;
				if (rsec < 0) rsec = 0;

				if (target_po >= 0 && target_po < g_forge.edit_state.placed_object_count)
				{
					struct forge_placed_object_entry *po = &g_forge.edit_state.placed_objects[target_po];
					po->physics_mode = (uint8_t)pmode;
					po->respawn_timer_sec = (int16_t)rsec;
					po->group_id = (uint8_t)gid;
					po->light_color_preset = (uint8_t)lcol;
					po->light_intensity_preset = (uint8_t)lint;
					po->light_radius_preset = (uint8_t)lrad;
				}
				if (target_li >= 0 && target_li < g_forge.live_instance_count)
				{
					struct forge_live_instance *inst = &g_forge.live_instances[target_li];
					inst->physics_mode = (uint8_t)pmode;
					inst->respawn_timer_sec = (int16_t)rsec;
					inst->group_id = (uint8_t)gid;
					inst->light_color_preset = (uint8_t)lcol;
					inst->light_intensity_preset = (uint8_t)lint;
					inst->light_radius_preset = (uint8_t)lrad;
					forge_apply_instance_light_settings(target_li);
					forge_3d_move_instance_ext(target_li, inst->x, inst->y, inst->z,
						inst->yaw, inst->pitch, inst->roll, 0, 0);
				}
			}
		}
		else if (in_forge_state && strncmp(line, "SPAWN|", 6) == 0)
		{
			float sx = 0, sy = 0, sz = 0, syaw = 0;
			int steam = 0;
			unsigned int sgflags = 0;
			if (sscanf(line + 6, "%f|%f|%f|%f|%d|%u", &sx, &sy, &sz, &syaw, &steam, &sgflags) >= 5)
			{
				if (!cleared_default_spawns)
				{
					int li;
					cleared_default_spawns = 1;
					g_forge.edit_state.replace_player_spawns = 1;
					g_forge.edit_state.player_spawn_count = 0;
					for (li = 0; li < g_forge.live_instance_count; li++)
					{
						if (g_forge.live_instances[li].group_tag == 'spwn')
							g_forge.live_instances[li].active = 0;
					}
				}
				if (g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
				{
					int32_t li_idx = g_forge.live_instance_count++;
					struct forge_live_instance *inst = &g_forge.live_instances[li_idx];
					memset(inst, 0, sizeof(*inst));
					inst->active = 1;
					inst->object_index = NONE;
					inst->actor_index = NONE;
					inst->edit_entry_index = -1;
					inst->group_tag = 'spwn';
					inst->team_designator = (int16_t)steam;
					inst->gametype_flags = (uint16_t)sgflags;
					inst->netgame_flag_type = (steam == 2) ? 13 : 12;
					inst->x = sx; inst->y = sy; inst->z = sz;
					inst->yaw = syaw;
					snprintf(inst->tag_path, sizeof(inst->tag_path), "%s",
						steam == 1 ? "spawn\\player_blue" : (steam == 2 ? "spawn\\player_ffa" : "spawn\\player_red"));
				}
			}
		}
		else if (in_forge_state && strncmp(line, "NETGAME_FLAG|", 13) == 0)
		{
			float fx = 0, fy = 0, fz = 0, fyaw = 0;
			int ftype = 0, fusage = 0;
			if (sscanf(line + 13, "%f|%f|%f|%f|%d|%d", &fx, &fy, &fz, &fyaw, &ftype, &fusage) >= 6)
			{
				if (!cleared_default_flags)
				{
					int li;
					cleared_default_flags = 1;
					g_forge.edit_state.replace_netgame_flags = 1;
					g_forge.edit_state.netgame_flag_count = 0;
					for (li = 0; li < g_forge.live_instance_count; li++)
					{
						if (g_forge.live_instances[li].group_tag == 'tele' ||
							g_forge.live_instances[li].group_tag == 'flag')
						{
							g_forge.live_instances[li].active = 0;
						}
					}
				}
				if (g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
				{
					int32_t li_idx = g_forge.live_instance_count++;
					struct forge_live_instance *inst = &g_forge.live_instances[li_idx];
					memset(inst, 0, sizeof(*inst));
					inst->active = 1;
					inst->object_index = NONE;
					inst->actor_index = NONE;
					inst->edit_entry_index = -1;
					inst->group_tag = (ftype == 6 || ftype == 7) ? 'tele' : 'flag';
					inst->netgame_flag_type = (int16_t)ftype;
					inst->team_designator = (int16_t)fusage;
					inst->x = fx; inst->y = fy; inst->z = fz;
					inst->yaw = fyaw;
					if (ftype == 6)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "teleporter\\origin");
					else if (ftype == 7)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "teleporter\\destination");
					else if (ftype == 2)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "objective\\oddball_spawn");
					else if (ftype == 3)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "objective\\race_track");
					else if (ftype == 4)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "objective\\race_vehicle");
					else if (ftype == 5)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "objective\\vegas_bank");
					else if (ftype == 8)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "objective\\koth_hill");
					else if (ftype == 1)
						snprintf(inst->tag_path, sizeof(inst->tag_path), "%s",
							fusage == 1 ? "objective\\ctf_vehicle_blue" : "objective\\ctf_vehicle_red");
					else
						snprintf(inst->tag_path, sizeof(inst->tag_path), "%s",
							fusage == 1 ? "objective\\ctf_flag_blue" : "objective\\ctf_flag_red");
				}
			}
		}
	}
	if (cleared_default_spawns)
	{
		sync_live_player_spawns_to_scenario();
		if (game_connection() != _game_connection_network_client &&
			g_forge.edit_state.player_spawn_count > 0 && player_data)
		{
			long p_idx;
			int sp_cursor = 0;
			boolean teams = game_engine_has_teams();
			for (p_idx = data_next_index(player_data, NONE);
				 p_idx != NONE;
				 p_idx = data_next_index(player_data, p_idx))
			{
				struct player_datum *player = (struct player_datum *)datum_get(player_data, p_idx);
				if (player && player->unit_index != NONE)
				{
					int chosen_sp = -1;
					int s;
					for (s = 0; s < g_forge.edit_state.player_spawn_count; s++)
					{
						int idx = (sp_cursor + s) % g_forge.edit_state.player_spawn_count;
						int16_t td = g_forge.edit_state.player_spawns[idx].team_index;
						if (!teams || td < 0 || td == player->team_index)
						{
							chosen_sp = idx;
							break;
						}
					}
					if (chosen_sp < 0)
						chosen_sp = sp_cursor % g_forge.edit_state.player_spawn_count;
					sp_cursor++;
					{
						struct forge_player_spawn_entry const *sp = &g_forge.edit_state.player_spawns[chosen_sp];
						real_point3d pos;
						real_vector3d fwd, up;
						pos.x = sp->x;
						pos.y = sp->y;
						pos.z = sp->z + 0.05f;
						fwd.i = (float)cosine(sp->yaw);
						fwd.j = (float)sine(sp->yaw);
						fwd.k = 0.0f;
						up.i = 0.0f;
						up.j = 0.0f;
						up.k = 1.0f;
						object_set_position(player->unit_index, &pos, &fwd, &up);
					}
				}
			}
		}
	}
	if (cleared_default_flags)
		sync_live_netgame_flags_to_scenario();
}

void forge_load_saved_map_state_if_present(char const *map_name)
{
	char const *slash;
	char leaf[64];
	char map_path[512];
	char txt_path[512];
	FILE *f;

	if (!map_name || !map_name[0])
		return;

	forge_undo_clear();
	forge_ui_close_map_tag_browser();

	/* Patch any baked vehicles and custom scenery/devices/lights in the loaded map so vehicle seats and long-distance LOD rendering work */
	{
		long tag_cnt = 0, ti;
		struct forge_cache_file_tag_instance *instances =
			(struct forge_cache_file_tag_instance *)cache_files_tag_instances(&tag_cnt);
		if (instances && tag_cnt > 0)
		{
			for (ti = 0; ti < tag_cnt; ti++)
			{
				uint32_t g = (uint32_t)instances[ti].group_tag;
				if (!instances[ti].base_address)
					continue;
				if (g == 'vehi')
					patch_vehicle_tag_for_player_driving(instances[ti].tag_index, instances[ti].name);
				else if (g == 'scen' || g == 'mach' || g == 'lifi' || g == 'ctrl')
					patch_forge_device_and_light_tag_runtime(instances[ti].tag_index, g, instances[ti].name);
			}
		}
		if (object_header_data)
		{
			unsigned long scen_mask =
				_object_mask_scenery |
				_object_mask_machine |
				_object_mask_control |
				_object_mask_light_fixture;
			long o_idx;
			for (o_idx = data_next_index(object_header_data, NONE);
				 o_idx != NONE;
				 o_idx = data_next_index(object_header_data, o_idx))
			{
				struct object_datum *obj = (struct object_datum *)object_try_and_get_and_verify_type(o_idx, scen_mask);
				if (obj && obj->definition_index != NONE)
				{
					uint8_t const *obuf = (uint8_t const *)object_definition_get(obj->definition_index);
					if (obuf)
					{
						float def_br = *(float const *)(obuf + 0x04u);
						if (obj->object.bounding_sphere_radius < def_br)
							obj->object.bounding_sphere_radius = def_br;
					}
				}
			}
		}
	}
	if (g_forge.initialized)
		forge_runtime_reload_catalogs();

	/* Clients receive the live Forge object state from the Host via !FH: / !FM: / !FD: packets */
	if (game_connection() == _game_connection_network_client)
		return;

	slash = strrchr(map_name, '\\');
	if (!slash)
		slash = strrchr(map_name, '/');
	if (_stricmp(slash ? (slash + 1) : map_name, "ui") == 0)
	{
		s_pending_sidecar_leaf[0] = '\0';
		s_active_sidecar_base_map[0] = '\0';
		return;
	}
	{
		int has_pending_sidecar = (s_pending_sidecar_leaf[0] != '\0');
		int is_flg1_map = 0;
		if (has_pending_sidecar)
		{
			snprintf(leaf, sizeof(leaf), "%s", s_pending_sidecar_leaf);
			snprintf(s_active_sidecar_base_map, sizeof(s_active_sidecar_base_map), "%s", forge_get_active_map_name());
		}
		else
		{
			snprintf(leaf, sizeof(leaf), "%s", slash ? (slash + 1) : map_name);
			s_active_sidecar_base_map[0] = '\0';
			is_flg1_map = is_existing_custom_map_file_flg1(leaf, NULL);
		}

		s_forge_server_dirty_txt = 0;
		s_forge_server_dirty_map = 0;
		s_forge_server_prev_player_count = 0;

		if (halo_server_mode())
		{
			forge_runtime_init();
			snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s", leaf);
			g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
			g_forge.has_saved_map_once = 1;
		}

		/* 0. If a sidecar custom_maps/<leaf>.txt exists (either explicitly selected via Edit Maps / Forge Maps
		 *    or crash-recovery sidecar for a custom FLG1 .map), load it now.
		 *    When launching a stock Xbox, CE, or MCC map directly without selecting a custom sidecar, do NOT auto-load a sidecar. */
		f = NULL;
		if (has_pending_sidecar || is_flg1_map || halo_server_mode())
		{
			char d_txt[160];
			snprintf(d_txt, sizeof(d_txt), "d:\\custom_maps\\%s.txt", leaf);
			txt_path[0] = '\0';
			platform_translate_path(d_txt, txt_path, sizeof(txt_path));
			f = txt_path[0] ? fopen(txt_path, "rb") : NULL;
			if (!f)
			{
				snprintf(txt_path, sizeof(txt_path), "custom_maps/%s.txt", leaf);
				f = fopen(txt_path, "rb");
			}
		}
		if (f)
		{
			long sz;
			fseek(f, 0, SEEK_END);
			sz = ftell(f);
			fseek(f, 0, SEEK_SET);
			if (sz > 0 && sz < 4 * 1024 * 1024)
			{
				char *buf = (char *)malloc((size_t)sz + 1u);
				if (buf)
				{
					size_t rd = fread(buf, 1, (size_t)sz, f);
					buf[rd] = 0;
					fclose(f);
					forge_runtime_init();
					snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s", leaf);
					g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
					if (halo_server_mode() || has_pending_sidecar || is_flg1_map)
						g_forge.has_saved_map_once = 1;
					parse_forge_state_text_buffer(buf, leaf);
					free(buf);
					if (halo_server_mode())
						forge_ui_save_copy_as(leaf);
					return;
				}
			}
			fclose(f);
		}
	}

	/* 1. Read embedded [OpenCE Forge Map] / [ForgeState] directly from custom_maps/<leaf>.map header (0x1E4 / 0x1E8) */
	f = NULL;
	{
		char const *ce_loaded = custom_edition_cache_tags_loaded() ? custom_edition_cache_loaded_path() : NULL;
		if (ce_loaded && ce_loaded[0])
		{
			map_path[0] = '\0';
			platform_translate_path(ce_loaded, map_path, sizeof(map_path));
			if (map_path[0])
				f = fopen(map_path, "rb");
			if (!f)
			{
				char const *rel_ce = ce_loaded;
				if ((rel_ce[0] == 'd' || rel_ce[0] == 'D') && rel_ce[1] == ':' && (rel_ce[2] == '\\' || rel_ce[2] == '/'))
					rel_ce += 3;
				f = fopen(rel_ce, "rb");
			}
		}
		if (!f)
		{
			char d_map[160];
			snprintf(d_map, sizeof(d_map), "d:\\custom_maps\\%s.map", leaf);
			map_path[0] = '\0';
			platform_translate_path(d_map, map_path, sizeof(map_path));
			if (map_path[0])
				f = fopen(map_path, "rb");
		}
		if (!f)
		{
			snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", leaf);
			f = fopen(map_path, "rb");
		}
	}
	if (f)
	{
		uint8_t hdr[0x200];
		if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr))
		{
			uint32_t state_off = 0, state_sz = 0, sig = 0;
			memcpy(&state_off, hdr + 0x1E4u, 4);
			memcpy(&state_sz, hdr + 0x1E8u, 4);
			memcpy(&sig, hdr + 0x1FCu, 4);
			if (sig == 0x464C4731u)
			{
				forge_runtime_init();
				snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s", leaf);
				g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
				g_forge.has_saved_map_once = 1;
				if (state_off >= 0x800u && state_sz > 0 && state_sz < 4u * 1024u * 1024u)
				{
					char *buf = (char *)malloc(state_sz + 1u);
					if (buf)
					{
						if (fseek(f, (long)state_off, SEEK_SET) == 0 && fread(buf, 1, state_sz, f) == state_sz)
						{
							buf[state_sz] = 0;
							parse_forge_state_text_buffer(buf, leaf);
							free(buf);
							fclose(f);
							return;
						}
						free(buf);
					}
				}
			}
		}
		fclose(f);
	}
}

static int forge_write_sidecar_state_file(char const *leaf)
{
	char txt_path[512];
	char tmp_path[520];
	FILE *f;
	int i;
	int def_emit_idx = 0;
	int placed_emit_idx = 0;
	int ai_emit_idx = 0;

	if (!leaf || !leaf[0])
		return 0;

	{
		char d_dir[512];
		d_dir[0] = '\0';
		platform_translate_path("d:\\custom_maps", d_dir, sizeof(d_dir));
		if (d_dir[0])
			CreateDirectoryA(d_dir, NULL);
	}
	CreateDirectoryA("d:\\custom_maps", NULL);
	CreateDirectoryA("custom_maps", NULL);

	{
		char d_txt[160];
		snprintf(d_txt, sizeof(d_txt), "d:\\custom_maps\\%s.txt", leaf);
		txt_path[0] = '\0';
		platform_translate_path(d_txt, txt_path, sizeof(txt_path));
		if (!txt_path[0])
			snprintf(txt_path, sizeof(txt_path), "custom_maps/%s.txt", leaf);
	}
	snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", txt_path);
	f = fopen(tmp_path, "wb");
	if (!f)
	{
		snprintf(txt_path, sizeof(txt_path), "custom_maps/%s.txt", leaf);
		snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", txt_path);
		f = fopen(tmp_path, "wb");
	}
	if (!f)
		return 0;

	{
		char base_map_str[128];
		int has_flg1_map = is_existing_custom_map_file_flg1(leaf, NULL);
		int is_baked_val = (!mcc_cache_tags_loaded() && has_flg1_map) ? 1 : 0;

		if (s_active_sidecar_base_map[0] != '\0')
		{
			snprintf(base_map_str, sizeof(base_map_str), "%s", s_active_sidecar_base_map);
		}
		else if (custom_edition_cache_tags_loaded())
		{
			char const *ce_loaded = custom_edition_cache_loaded_path();
			char const *rel_ce = ce_loaded ? ce_loaded : "";
			char base_stem[64];
			if ((rel_ce[0] == 'd' || rel_ce[0] == 'D') && rel_ce[1] == ':' && (rel_ce[2] == '\\' || rel_ce[2] == '/'))
				rel_ce += 3;
			if (rel_ce[0] && extract_embedded_forge_base_map_stem(rel_ce, base_stem, sizeof(base_stem)) && base_stem[0])
				snprintf(base_map_str, sizeof(base_map_str), "custom_maps\\%s", base_stem);
			else
				snprintf(base_map_str, sizeof(base_map_str), "%s", forge_get_active_map_name());
		}
		else
		{
			snprintf(base_map_str, sizeof(base_map_str), "%s", forge_get_active_map_name());
		}

		fprintf(f,
			"[OpenCE Forge Map]\r\n"
			"ForgeSource=UserCreated\r\n"
			"Map Name: %s\r\n"
			"Base Map: %s\r\n"
			"Author: OpenCE : Forge Creator\r\n"
			"Objects Placed: %d\r\n"
			"AI Squads: %d\r\n"
			"[ForgeState]\r\n",
			leaf,
			base_map_str,
			(int)g_forge.edit_state.placed_object_count,
			(int)g_forge.edit_state.ai_spawn_count);

		for (i = 0; i < g_forge.edit_state.default_edit_count; i++)
		{
			struct forge_default_object_edit const *de = &g_forge.edit_state.default_edits[i];
			if (!de->active)
				continue;
			fprintf(f,
				"DEFAULT_EDIT|%u|%d|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%s\r\n",
				(unsigned)de->group_tag, (int)de->deleted,
				de->orig_x, de->orig_y, de->orig_z,
				de->new_x, de->new_y, de->new_z,
				de->new_yaw, de->new_pitch, de->new_roll,
				de->tag_path);
			if (!de->deleted && de->gametype_flags != 0 && de->gametype_flags != 0x0FFFu)
			{
				fprintf(f, "DEFAULT_GAMETYPE|%d|%u\r\n", def_emit_idx, (unsigned)de->gametype_flags);
			}
			if (!de->deleted &&
				(de->physics_mode != 0 || de->respawn_timer_sec > 0 || de->group_id != 0 ||
				 de->light_color_preset != 0 || de->light_intensity_preset != 0 || de->light_radius_preset != 0))
			{
				fprintf(f,
					"DEFAULT_PROPS|%d|%u|%d|%u|%u|%u|%u\r\n",
					def_emit_idx,
					(unsigned)de->physics_mode,
					(int)de->respawn_timer_sec,
					(unsigned)de->group_id,
					(unsigned)de->light_color_preset,
					(unsigned)de->light_intensity_preset,
					(unsigned)de->light_radius_preset);
			}
			def_emit_idx++;
		}
		for (i = 0; i < g_forge.edit_state.placed_object_count; i++)
		{
			struct forge_placed_object_entry const *po = &g_forge.edit_state.placed_objects[i];
			if (!po->active || !po->tag_path[0] || po->x < -90000.0f)
				continue;
			fprintf(f,
				"OBJECT|%u|%d|%d|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%d|%s\r\n",
				(unsigned)po->group_tag,
				(int)po->source_cache_id,
				is_baked_val,
				po->x, po->y, po->z,
				po->yaw, po->pitch, po->roll,
				(int)po->team_index,
				po->tag_path);
		if (po->gametype_flags != 0 && po->gametype_flags != 0x0FFFu)
		{
			fprintf(f, "OBJECT_GAMETYPE|%u\r\n", (unsigned)po->gametype_flags);
		}
		if (po->physics_mode != 0 || po->respawn_timer_sec > 0 || po->group_id != 0 ||
			po->light_color_preset != 0 || po->light_intensity_preset != 0 || po->light_radius_preset != 0)
		{
			fprintf(f,
				"OBJECT_PROPS|%d|%u|%d|%u|%u|%u|%u\r\n",
				placed_emit_idx,
				(unsigned)po->physics_mode,
				(int)po->respawn_timer_sec,
				(unsigned)po->group_id,
				(unsigned)po->light_color_preset,
				(unsigned)po->light_intensity_preset,
				(unsigned)po->light_radius_preset);
		}
		placed_emit_idx++;
	}
	}
	if (g_forge.edit_state.replace_player_spawns)
	{
		for (i = 0; i < g_forge.edit_state.player_spawn_count; i++)
		{
			struct forge_player_spawn_entry const *sp = &g_forge.edit_state.player_spawns[i];
			int steam = (sp->type0 == 13) ? 2 : (int)sp->team_index;
			int smode = (sp->type0 == 13) ? (int)sp->type1 : (int)sp->type0;
			unsigned int sgflags =
				(smode == 1) ? 0x0001u :
				(smode == 2) ? 0x0002u :
				(smode == 3) ? 0x0004u :
				(smode == 4) ? 0x0008u :
				(smode == 5) ? 0x0010u : 0u;
			fprintf(f,
				"SPAWN|%.4f|%.4f|%.4f|%.4f|%d|%u\r\n",
				sp->x, sp->y, sp->z, sp->yaw,
				steam,
				sgflags);
		}
	}
	if (g_forge.edit_state.replace_netgame_flags)
	{
		for (i = 0; i < g_forge.edit_state.netgame_flag_count; i++)
		{
			struct forge_netgame_flag_entry const *nf = &g_forge.edit_state.netgame_flags[i];
			fprintf(f,
				"NETGAME_FLAG|%.4f|%.4f|%.4f|%.4f|%d|%d\r\n",
				nf->x, nf->y, nf->z, nf->yaw,
				(int)nf->type,
				(int)nf->usage_id);
		}
	}
	for (i = 0; i < g_forge.edit_state.ai_spawn_count; i++)
	{
		struct forge_ai_spawn_entry const *ai = &g_forge.edit_state.ai_spawns[i];
		if (!ai->actor_variant_path[0])
			continue;
		fprintf(f,
			"AI|%.4f|%.4f|%.4f|%.4f|%d|%s\r\n",
			ai->x, ai->y, ai->z, ai->yaw,
			(int)ai->team_designator,
			ai->actor_variant_path);
		if (ai->respawn_timer_sec > 0)
		{
			fprintf(f, "AI_RESPAWN|%d|%d\r\n", ai_emit_idx, (int)ai->respawn_timer_sec);
		}
		if (ai->ai_order != 0)
		{
			fprintf(f, "AI_ORDER|%d|%u\r\n", ai_emit_idx, (unsigned)ai->ai_order);
		}
		if (ai->group_id != 0)
		{
			fprintf(f, "AI_GROUP|%d|%u\r\n", ai_emit_idx, (unsigned)ai->group_id);
		}
		ai_emit_idx++;
	}

	fclose(f);
	if (forge_host_rename(tmp_path, txt_path) != 0)
	{
		remove(tmp_path);
		return 0;
	}
	return 1;
}

void forge_server_mark_dirty(void)
{
	s_forge_server_dirty_txt = 1;
	s_forge_server_dirty_map = 1;
	s_forge_server_last_edit_ms = GetTickCount() | 1u;
}

int forge_server_save_map_if_dirty(int force_full_export)
{
	char leaf[64];

	if (!g_forge_session_enabled || game_connection() == _game_connection_network_client)
		return 0;
	if (!halo_server_mode() && !g_forge.has_saved_map_once)
		return 0;

	if (g_forge.save_map_name[0])
	{
		snprintf(leaf, sizeof(leaf), "%s", g_forge.save_map_name);
	}
	else
	{
		char const *cur = forge_get_active_map_name();
		char const *slash = cur ? strrchr(cur, '\\') : NULL;
		char const *slash2 = cur ? strrchr(cur, '/') : NULL;
		if (slash2 && (!slash || slash2 > slash))
			slash = slash2;
		snprintf(leaf, sizeof(leaf), "%s", slash ? (slash + 1) : (cur ? cur : ""));
	}

	if (!leaf[0] || _stricmp(leaf, "ui") == 0)
		return 0;

	if (force_full_export)
	{
		if (!s_forge_server_dirty_map)
			return 0;
		return forge_ui_save_copy_as(leaf);
	}

	if (s_forge_server_dirty_txt)
	{
		if (forge_write_sidecar_state_file(leaf))
		{
			s_forge_server_dirty_txt = 0;
			return 1;
		}
	}
	return 0;
}

void forge_server_autosave_tick(void)
{
	unsigned long now;
	int player_cnt = 0;

	if (!halo_server_mode() || !g_forge_session_enabled || game_connection() == _game_connection_network_client)
		return;

	if (player_data)
	{
		long p_idx;
		for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
			player_cnt++;
	}

	now = GetTickCount() | 1u;

	/* 1. Flush lightweight crash-recovery sidecar (<4 KB) 5 seconds after the last edit while players are online */
	if (s_forge_server_dirty_txt && s_forge_server_last_edit_ms && (now - s_forge_server_last_edit_ms) >= 5000u)
	{
		forge_server_save_map_if_dirty(0);
	}

	/* 2. When the last player leaves the server after making edits, overwrite custom_maps/<leaf>.map in full! */
	if (s_forge_server_prev_player_count > 0 && player_cnt == 0 && s_forge_server_dirty_map)
	{
		forge_server_save_map_if_dirty(1);
	}

	s_forge_server_prev_player_count = player_cnt;
}

/* ============================================================================
 * MONITOR ("characters\monitor\monitor") & NATIVE MODEL RENDERER
 * - Renders 343 Guilty Spark ("characters\monitor\monitor") and 3D Spawn Menu
 *   previews directly through OpenCE's native orig_render_model!
 * ============================================================================ */

static int draw_native_object_model_at(
	long obj_def_idx,
	long unique_id,
	void const *lighting_ptr,
	union real_point3d const *pos,
	union real_vector3d const *forward,
	union real_vector3d const *up,
	int is_monitor,
	int is_preview,
	unsigned long flags)
{
	uint8_t const *obj_def;
	float radius;
	long model_idx;
	struct model const *mdef;
	struct real_matrix4x3 node_matrices[64];
	static struct render_lighting studio_lighting;
	struct render_lighting const *use_lighting;
	static union real_rgb_color change_colors_mon[4];
	static union real_rgb_color change_colors_std[4];
	static real function_values[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	union real_rgb_color *change_colors = is_monitor ? change_colors_mon : change_colors_std;
	union real_point3d draw_centroid = *pos;
	int node_count, n;

	if (obj_def_idx == NONE)
		return 0;
	obj_def = (uint8_t const *)tag_get(tag_get_group_tag(obj_def_idx), obj_def_idx);
	if (!obj_def)
		return 0;

	radius = *(float const *)(obj_def + 0x04u);
	if (!(radius > 0.1f && radius < 100.0f))
		radius = 1.5f;

	model_idx = *(int32_t const *)(obj_def + 0x34u);
	if (model_idx == NONE)
		return 0;
	mdef = (struct model const *)tag_get('mode', model_idx);
	if (!mdef)
		return 0;

	node_count = (int)mdef->nodes.count;
	if (node_count <= 0)
		return 0;
	if (node_count > 64)
		node_count = 64;

	memset(node_matrices, 0, sizeof(node_matrices));
	if (is_preview)
	{
		float preview_scale = 0.52f / ((radius > 0.15f) ? radius : 0.52f);
		struct real_matrix4x3 scale_pos_mat;
		if (preview_scale < 0.035f) preview_scale = 0.035f;
		if (preview_scale > 1.35f) preview_scale = 1.35f;

		model_get_node_matrices(mdef, node_matrices, global_origin3d, forward, up);
		matrix4x3_from_point_and_vectors(&scale_pos_mat, pos, global_forward3d, global_up3d);
		scale_pos_mat.scale = preview_scale;
		for (n = 0; n < node_count; n++)
		{
			struct real_matrix4x3 tmp = node_matrices[n];
			matrix4x3_multiply(&scale_pos_mat, &tmp, &node_matrices[n]);
		}
		radius *= preview_scale;
	}
	else if (is_monitor)
	{
		union real_point3d mpos = *pos;
		struct real_matrix4x3 scale_pos_mat;
		mpos.z += 0.55f;
		draw_centroid = mpos;
		model_get_node_matrices(mdef, node_matrices, global_origin3d, forward, up);
		matrix4x3_from_point_and_vectors(&scale_pos_mat, &mpos, global_forward3d, global_up3d);
		scale_pos_mat.scale = 1.35f;
		for (n = 0; n < node_count; n++)
		{
			struct real_matrix4x3 tmp = node_matrices[n];
			matrix4x3_multiply(&scale_pos_mat, &tmp, &node_matrices[n]);
		}
		radius *= 1.35f;
	}
	else
	{
		model_get_node_matrices(mdef, node_matrices, pos, forward, up);
	}

	if (is_monitor)
	{
		change_colors[0].red = 0.28f;
		change_colors[0].green = 0.72f;
		change_colors[0].blue = 1.0f;
		change_colors[1].red = 0.65f;
		change_colors[1].green = 0.90f;
		change_colors[1].blue = 1.0f;
	}
	else
	{
		change_colors[0].red = 0.85f;
		change_colors[0].green = 0.85f;
		change_colors[0].blue = 0.85f;
		change_colors[1].red = 0.75f;
		change_colors[1].green = 0.75f;
		change_colors[1].blue = 0.75f;
	}
	change_colors[2] = change_colors[0];
	change_colors[3] = change_colors[1];

	function_values[0] = 1.0f;
	function_values[1] = 1.0f;
	function_values[2] = 1.0f;
	function_values[3] = 1.0f;

	if (lighting_ptr)
	{
		use_lighting = (struct render_lighting const *)lighting_ptr;
	}
	else
	{
		memset(&studio_lighting, 0, sizeof(studio_lighting));
		studio_lighting.ambient_color.red = 0.72f;
		studio_lighting.ambient_color.green = 0.74f;
		studio_lighting.ambient_color.blue = 0.78f;
		studio_lighting.distant_light_count = 1;
		studio_lighting.distant_lights[0].color.red = 0.95f;
		studio_lighting.distant_lights[0].color.green = 0.95f;
		studio_lighting.distant_lights[0].color.blue = 0.92f;
		studio_lighting.distant_lights[0].direction.i = -0.577f;
		studio_lighting.distant_lights[0].direction.j = -0.577f;
		studio_lighting.distant_lights[0].direction.k = -0.577f;
		use_lighting = &studio_lighting;
	}

	orig_render_model(
		model_idx,
		10000.0f,
		node_matrices,
		NULL,
		change_colors,
		function_values,
		use_lighting,
		&draw_centroid,
		radius,
		NULL,
		(is_monitor || is_preview) ? NONE : unique_id,
		0,
		is_preview ? (flags | (1u << 3)) /* _render_model_first_person_bit */ : flags);
	return 1;
}

enum forge_marker_color_preset
{
	_marker_color_ffa_yellow = 0,
	_marker_color_red_team,
	_marker_color_blue_team,
	_marker_color_tele_green,
	_marker_color_race_orange,
	_marker_color_oddball_purple,
	_marker_color_koth_gold,
	NUMBER_OF_MARKER_COLOR_PRESETS
};

enum forge_marker_mesh_kind
{
	_marker_mesh_player_spawn = 0,
	_marker_mesh_tele_origin,
	_marker_mesh_tele_dest,
	_marker_mesh_race,
	_marker_mesh_oddball,
	_marker_mesh_ctf_flag,
	_marker_mesh_koth,
	_marker_mesh_koth_beam,
	NUMBER_OF_MARKER_MESH_KINDS
};

static struct
{
	int loaded;
	struct
	{
		int valid;
		int part_count;
		float radius;
		struct forge_hw_part parts[FORGE_MAX_HW_PARTS];
	} meshes[NUMBER_OF_MARKER_MESH_KINDS];
} s_marker_meshes = { 0 };

static long get_or_create_translucent_marker_shader(int color_preset, int is_highlighted, int is_z_prime)
{
	static uint32_t const s_preset_rgb[NUMBER_OF_MARKER_COLOR_PRESETS] = {
		0x00FFE618u, /* FFA Player Spawn     - Translucent Yellow */
		0x00FF2424u, /* Red Team Spawn/Flag  - Translucent Red    */
		0x002488FFu, /* Blue Team Spawn/Flag - Translucent Blue   */
		0x0024FF54u, /* Teleporter In/Out    - Translucent Green  */
		0x00FF8C14u, /* Race Marker          - Translucent Orange */
		0x00BC38FFu, /* Oddball Marker       - Translucent Purple */
		0x00FFC824u  /* KOTH Hill Marker     - Translucent Gold   */
	};
	char shdr_name[96];
	char bitm_name[96];
	long shdr_idx, tmpl_soso_idx, bitm_idx;
	struct tag_iterator iter;
	void *tmpl_ptr;
	struct bitmap_data *bmp;
	struct bitmap_group *bgrp;
	uint8_t *soso_def;
	uint32_t rgb, alpha_byte, argb;
	uint32_t *pix;
	float r_f, g_f, b_f;
	int i;

	if (color_preset < 0 || color_preset >= NUMBER_OF_MARKER_COLOR_PRESETS)
		color_preset = _marker_color_ffa_yellow;

	snprintf(shdr_name, sizeof(shdr_name), "forge\\markers\\shdr_%d_%d_%d",
		color_preset, is_highlighted ? 1 : 0, is_z_prime ? 1 : 0);
	shdr_idx = tag_loaded('soso', shdr_name);
	if (shdr_idx != NONE)
		return shdr_idx;

	rgb = s_preset_rgb[color_preset];
	alpha_byte = is_z_prime ? 0xFFu : (is_highlighted ? 0xB8u : 0x8Cu);
	argb = (alpha_byte << 24) | rgb;
	r_f = (float)((rgb >> 16) & 0xFFu) / 255.0f;
	g_f = (float)((rgb >> 8) & 0xFFu) / 255.0f;
	b_f = (float)(rgb & 0xFFu) / 255.0f;

	snprintf(bitm_name, sizeof(bitm_name), "forge\\markers\\bitm_%d_%d_%d",
		color_preset, is_highlighted ? 1 : 0, is_z_prime ? 1 : 0);
	bitm_idx = tag_loaded('bitm', bitm_name);
	if (bitm_idx == NONE)
	{
		/* Format 11 = _bitmap_format_a8r8g8b8 (32-bit ARGB) */
		bmp = bitmap_2d_new(16, 16, 0, 11);
		if (!bmp || !bmp->base_address)
			return NONE;
		pix = (uint32_t *)bmp->base_address;
		for (i = 0; i < 16 * 16; i++)
			pix[i] = argb;
		bitmap_rebuild(bmp);

		bgrp = (struct bitmap_group *)calloc(1, sizeof(struct bitmap_group));
		if (!bgrp)
			return NONE;
		bgrp->bitmaps.count = 1;
		bgrp->bitmaps.address = bmp;
		bitm_idx = append_runtime_tag_instance('bitm', NONE, NONE, bitm_name, bgrp);
		if (bitm_idx == NONE)
			return NONE;
	}

	tag_iterator_new(&iter, 'soso');
	tmpl_soso_idx = tag_iterator_next(&iter);
	if (tmpl_soso_idx == NONE)
		return NONE;
	tmpl_ptr = shader_definition_get(tmpl_soso_idx);
	if (!tmpl_ptr)
		return NONE;

	soso_def = (uint8_t *)calloc(1, 512);
	if (!soso_def)
		return NONE;
	memcpy(soso_def, tmpl_ptr, 440);

	*(int16_t *)(soso_def + 0x24) = 4; /* _shader_type_model */
	/* Bit 1 = _shader_model_two_sided_bit, Bit 2 = _shader_model_not_alpha_tested_bit, Bit 3 = _shader_model_alpha_blended_decal_bit */
	*(uint16_t *)(soso_def + 0x28) = is_z_prime ? ((1u << 1) | (1u << 2)) : ((1u << 1) | (1u << 2) | (1u << 3));
	*(float *)(soso_def + 0x38) = is_highlighted ? 0.72f : 0.55f;
	*(int16_t *)(soso_def + 0x4C) = 0;
	*(uint16_t *)(soso_def + 0x6C) = 1u; /* _shader_model_self_illumination_no_random_phase_bit */
	*(int16_t *)(soso_def + 0x70) = 0;
	*(int16_t *)(soso_def + 0x72) = 0;
	*(float *)(soso_def + 0x74) = 1.0f;
	*(float *)(soso_def + 0x78) = r_f * 0.34f;
	*(float *)(soso_def + 0x7C) = g_f * 0.34f;
	*(float *)(soso_def + 0x80) = b_f * 0.34f;
	*(float *)(soso_def + 0x84) = r_f * 0.34f;
	*(float *)(soso_def + 0x88) = g_f * 0.34f;
	*(float *)(soso_def + 0x8C) = b_f * 0.34f;
	*(float *)(soso_def + 0x9C) = 1.0f;
	*(float *)(soso_def + 0xA0) = 1.0f;
	*(int32_t *)(soso_def + 0xA4) = 'bitm';
	*(int32_t *)(soso_def + 0xA8) = 0;
	*(int32_t *)(soso_def + 0xAC) = 0;
	*(int32_t *)(soso_def + 0xB0) = bitm_idx;
	*(int32_t *)(soso_def + 0xC8) = NONE;
	*(int16_t *)(soso_def + 0xD4) = 0;
	*(int16_t *)(soso_def + 0xD6) = 0;
	*(float *)(soso_def + 0xD8) = 1.0f;
	*(int32_t *)(soso_def + 0xE8) = NONE;
	*(float *)(soso_def + 0xEC) = 1.0f;
	*(float *)(soso_def + 0x144) = 0.0f;
	*(float *)(soso_def + 0x154) = 0.0f;
	*(int32_t *)(soso_def + 0x170) = NONE;

	return append_runtime_tag_instance('soso', 'shdr', NONE, shdr_name, soso_def);
}

struct forge_proc_mesh_builder
{
	uint8_t *verts;
	int32_t vtx_count;
	int32_t vtx_cap;
	uint16_t *strip;
	int32_t idx_count;
	int32_t idx_cap;
};

static uint16_t proc_mesh_add_vtx(
	struct forge_proc_mesh_builder *b,
	float x, float y, float z,
	float nx, float ny, float nz)
{
	uint8_t *vp;
	uint16_t idx;

	if (!b || b->vtx_count >= b->vtx_cap)
		return 0;
	idx = (uint16_t)b->vtx_count++;
	vp = b->verts + (uint32_t)idx * 68u;
	memset(vp, 0, 68u);
	*(float *)(vp + 0) = x;
	*(float *)(vp + 4) = y;
	*(float *)(vp + 8) = z;
	*(float *)(vp + 12) = nx;
	*(float *)(vp + 16) = ny;
	*(float *)(vp + 20) = nz;
	*(float *)(vp + 24) = 0.0f;
	*(float *)(vp + 28) = 1.0f;
	*(float *)(vp + 32) = 0.0f;
	*(float *)(vp + 36) = 1.0f;
	*(float *)(vp + 40) = 0.0f;
	*(float *)(vp + 44) = 0.0f;
	*(float *)(vp + 48) = 0.5f;
	*(float *)(vp + 52) = 0.5f;
	*(int16_t *)(vp + 56) = 0;
	*(int16_t *)(vp + 58) = 0;
	*(float *)(vp + 60) = 1.0f;
	*(float *)(vp + 64) = 0.0f;
	return idx;
}

static void proc_mesh_add_tri(
	struct forge_proc_mesh_builder *b,
	uint16_t i0, uint16_t i1, uint16_t i2)
{
	if (!b || b->idx_count + 6 > b->idx_cap)
		return;
	if (b->idx_count == 0)
	{
		b->strip[b->idx_count++] = i0;
		b->strip[b->idx_count++] = i1;
		b->strip[b->idx_count++] = i2;
	}
	else
	{
		uint16_t last = b->strip[b->idx_count - 1];
		b->strip[b->idx_count++] = last;
		b->strip[b->idx_count++] = last;
		b->strip[b->idx_count++] = i0;
		b->strip[b->idx_count++] = i0;
		b->strip[b->idx_count++] = i1;
		b->strip[b->idx_count++] = i2;
	}
}

static void proc_mesh_add_quad(
	struct forge_proc_mesh_builder *b,
	uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3)
{
	proc_mesh_add_tri(b, i0, i1, i2);
	proc_mesh_add_tri(b, i0, i2, i3);
}

static void proc_mesh_add_box(
	struct forge_proc_mesh_builder *b,
	float x0, float y0, float z0,
	float x1, float y1, float z1)
{
	uint16_t v0, v1, v2, v3;

	/* Top (+Z) */
	v0 = proc_mesh_add_vtx(b, x0, y0, z1, 0.0f, 0.0f, 1.0f);
	v1 = proc_mesh_add_vtx(b, x1, y0, z1, 0.0f, 0.0f, 1.0f);
	v2 = proc_mesh_add_vtx(b, x1, y1, z1, 0.0f, 0.0f, 1.0f);
	v3 = proc_mesh_add_vtx(b, x0, y1, z1, 0.0f, 0.0f, 1.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);

	/* Bottom (-Z) */
	v0 = proc_mesh_add_vtx(b, x0, y1, z0, 0.0f, 0.0f, -1.0f);
	v1 = proc_mesh_add_vtx(b, x1, y1, z0, 0.0f, 0.0f, -1.0f);
	v2 = proc_mesh_add_vtx(b, x1, y0, z0, 0.0f, 0.0f, -1.0f);
	v3 = proc_mesh_add_vtx(b, x0, y0, z0, 0.0f, 0.0f, -1.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);

	/* Front (+X) */
	v0 = proc_mesh_add_vtx(b, x1, y0, z0, 1.0f, 0.0f, 0.0f);
	v1 = proc_mesh_add_vtx(b, x1, y1, z0, 1.0f, 0.0f, 0.0f);
	v2 = proc_mesh_add_vtx(b, x1, y1, z1, 1.0f, 0.0f, 0.0f);
	v3 = proc_mesh_add_vtx(b, x1, y0, z1, 1.0f, 0.0f, 0.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);

	/* Back (-X) */
	v0 = proc_mesh_add_vtx(b, x0, y1, z0, -1.0f, 0.0f, 0.0f);
	v1 = proc_mesh_add_vtx(b, x0, y0, z0, -1.0f, 0.0f, 0.0f);
	v2 = proc_mesh_add_vtx(b, x0, y0, z1, -1.0f, 0.0f, 0.0f);
	v3 = proc_mesh_add_vtx(b, x0, y1, z1, -1.0f, 0.0f, 0.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);

	/* Left (+Y) */
	v0 = proc_mesh_add_vtx(b, x1, y1, z0, 0.0f, 1.0f, 0.0f);
	v1 = proc_mesh_add_vtx(b, x0, y1, z0, 0.0f, 1.0f, 0.0f);
	v2 = proc_mesh_add_vtx(b, x0, y1, z1, 0.0f, 1.0f, 0.0f);
	v3 = proc_mesh_add_vtx(b, x1, y1, z1, 0.0f, 1.0f, 0.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);

	/* Right (-Y) */
	v0 = proc_mesh_add_vtx(b, x0, y0, z0, 0.0f, -1.0f, 0.0f);
	v1 = proc_mesh_add_vtx(b, x1, y0, z0, 0.0f, -1.0f, 0.0f);
	v2 = proc_mesh_add_vtx(b, x1, y0, z1, 0.0f, -1.0f, 0.0f);
	v3 = proc_mesh_add_vtx(b, x0, y0, z1, 0.0f, -1.0f, 0.0f);
	proc_mesh_add_quad(b, v0, v1, v2, v3);
}

static void proc_mesh_add_frustum_z(
	struct forge_proc_mesh_builder *b,
	float cx, float cy,
	float z0, float r0,
	float z1, float r1,
	int segs,
	int cap_bottom,
	int cap_top)
{
	int s;
	uint16_t bot_center = 0, top_center = 0;

	if (segs < 3)
		segs = 3;
	if (cap_bottom && r0 > 0.001f)
		bot_center = proc_mesh_add_vtx(b, cx, cy, z0, 0.0f, 0.0f, -1.0f);
	if (cap_top && r1 > 0.001f)
		top_center = proc_mesh_add_vtx(b, cx, cy, z1, 0.0f, 0.0f, 1.0f);

	for (s = 0; s < segs; s++)
	{
		float a0 = (2.0f * _pi * (float)s) / (float)segs;
		float a1 = (2.0f * _pi * (float)(s + 1)) / (float)segs;
		float c0 = cosine(a0), s0 = sine(a0);
		float c1 = cosine(a1), s1 = sine(a1);
		uint16_t b0 = proc_mesh_add_vtx(b, cx + r0 * c0, cy + r0 * s0, z0, c0, s0, 0.25f);
		uint16_t b1 = proc_mesh_add_vtx(b, cx + r0 * c1, cy + r0 * s1, z0, c1, s1, 0.25f);
		uint16_t t1 = proc_mesh_add_vtx(b, cx + r1 * c1, cy + r1 * s1, z1, c1, s1, 0.25f);
		uint16_t t0 = proc_mesh_add_vtx(b, cx + r1 * c0, cy + r1 * s0, z1, c0, s0, 0.25f);
		proc_mesh_add_quad(b, b0, b1, t1, t0);
		if (cap_bottom && r0 > 0.001f)
			proc_mesh_add_tri(b, bot_center, b1, b0);
		if (cap_top && r1 > 0.001f)
			proc_mesh_add_tri(b, top_center, t0, t1);
	}
}

static void proc_mesh_add_arrow_x(
	struct forge_proc_mesh_builder *b,
	float x_tail, float x_neck, float x_tip,
	float half_shaft_w, float half_head_w,
	float z0, float z1)
{
	uint16_t t_tip, t_hl, t_hr, b_tip, b_hl, b_hr;

	proc_mesh_add_box(b, x_tail, -half_shaft_w, z0, x_neck, half_shaft_w, z1);

	t_tip = proc_mesh_add_vtx(b, x_tip, 0.0f, z1, 0.0f, 0.0f, 1.0f);
	t_hl  = proc_mesh_add_vtx(b, x_neck, half_head_w, z1, 0.0f, 0.0f, 1.0f);
	t_hr  = proc_mesh_add_vtx(b, x_neck, -half_head_w, z1, 0.0f, 0.0f, 1.0f);
	proc_mesh_add_tri(b, t_hr, t_tip, t_hl);

	b_tip = proc_mesh_add_vtx(b, x_tip, 0.0f, z0, 0.0f, 0.0f, -1.0f);
	b_hl  = proc_mesh_add_vtx(b, x_neck, half_head_w, z0, 0.0f, 0.0f, -1.0f);
	b_hr  = proc_mesh_add_vtx(b, x_neck, -half_head_w, z0, 0.0f, 0.0f, -1.0f);
	proc_mesh_add_tri(b, b_hl, b_tip, b_hr);

	proc_mesh_add_quad(b, b_hr, b_tip, t_tip, t_hr);
	proc_mesh_add_quad(b, b_tip, b_hl, t_hl, t_tip);
	proc_mesh_add_quad(b, b_hl, b_hr, t_hr, t_hl);
}

static void proc_mesh_add_torus_yz(
	struct forge_proc_mesh_builder *b,
	float cx, float cy, float cz,
	float major_r, float minor_r,
	int maj_segs, int min_segs,
	int half_arch_only)
{
	int i, j;
	float max_angle = half_arch_only ? _pi : (2.0f * _pi);

	for (i = 0; i < maj_segs; i++)
	{
		float u0 = (max_angle * (float)i) / (float)maj_segs;
		float u1 = (max_angle * (float)(i + 1)) / (float)maj_segs;
		float cu0 = cosine(u0), su0 = sine(u0);
		float cu1 = cosine(u1), su1 = sine(u1);

		for (j = 0; j < min_segs; j++)
		{
			float v0 = (2.0f * _pi * (float)j) / (float)min_segs;
			float v1 = (2.0f * _pi * (float)(j + 1)) / (float)min_segs;
			float cv0 = cosine(v0), sv0 = sine(v0);
			float cv1 = cosine(v1), sv1 = sine(v1);

			uint16_t p00 = proc_mesh_add_vtx(b,
				cx + minor_r * sv0,
				cy + (major_r + minor_r * cv0) * cu0,
				cz + (major_r + minor_r * cv0) * su0,
				sv0, cv0 * cu0, cv0 * su0);
			uint16_t p10 = proc_mesh_add_vtx(b,
				cx + minor_r * sv0,
				cy + (major_r + minor_r * cv0) * cu1,
				cz + (major_r + minor_r * cv0) * su1,
				sv0, cv0 * cu1, cv0 * su1);
			uint16_t p11 = proc_mesh_add_vtx(b,
				cx + minor_r * sv1,
				cy + (major_r + minor_r * cv1) * cu1,
				cz + (major_r + minor_r * cv1) * su1,
				sv1, cv1 * cu1, cv1 * su1);
			uint16_t p01 = proc_mesh_add_vtx(b,
				cx + minor_r * sv1,
				cy + (major_r + minor_r * cv1) * cu0,
				cz + (major_r + minor_r * cv1) * su0,
				sv1, cv1 * cu0, cv1 * su0);
			proc_mesh_add_quad(b, p00, p10, p11, p01);
		}
	}
}

static void ensure_objective_marker_meshes_loaded(void)
{
	int k, all_valid = 1;

	if (s_marker_meshes.loaded)
	{
		for (k = 0; k < NUMBER_OF_MARKER_MESH_KINDS; k++)
			if (!s_marker_meshes.meshes[k].valid) { all_valid = 0; break; }
		if (all_valid)
			return;
	}
	s_marker_meshes.loaded = 1;

	for (k = 0; k < NUMBER_OF_MARKER_MESH_KINDS; k++)
	{
		struct forge_proc_mesh_builder mb;

		if (s_marker_meshes.meshes[k].valid && s_marker_meshes.meshes[k].part_count > 0)
			continue;
		s_marker_meshes.meshes[k].valid = 0;
		s_marker_meshes.meshes[k].part_count = 0;
		s_marker_meshes.meshes[k].radius = 0.45f;

		memset(&mb, 0, sizeof(mb));
		mb.vtx_cap = 2048;
		mb.idx_cap = 16384;
		mb.verts = (uint8_t *)malloc((size_t)mb.vtx_cap * 68u);
		mb.strip = (uint16_t *)malloc((size_t)mb.idx_cap * sizeof(uint16_t));
		if (!mb.verts || !mb.strip)
		{
			free(mb.verts);
			free(mb.strip);
			continue;
		}

		switch (k)
		{
		case _marker_mesh_player_spawn:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.032f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.032f, 0.122f, 0.042f, 0.114f, 20, 0, 1);
			proc_mesh_add_arrow_x(&mb, -0.07f, 0.11f, 0.234f, 0.034f, 0.076f, 0.032f, 0.059f);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_tele_origin:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, -0.115f, 0.034f, 0.022f, 0.084f, 0.020f, 10, 0, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.115f, 0.034f, 0.022f, 0.084f, 0.020f, 10, 0, 0);
			proc_mesh_add_torus_yz(&mb, 0.0f, 0.0f, 0.084f, 0.115f, 0.020f, 14, 8, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.068f, 0.004f, 0.110f, 0.036f, 8, 0, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.110f, 0.036f, 0.152f, 0.004f, 8, 0, 0);
			proc_mesh_add_arrow_x(&mb, -0.04f, 0.09f, 0.180f, 0.028f, 0.064f, 0.034f, 0.056f);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_tele_dest:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, -0.115f, 0.034f, 0.022f, 0.084f, 0.020f, 10, 0, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.115f, 0.034f, 0.022f, 0.084f, 0.020f, 10, 0, 0);
			proc_mesh_add_torus_yz(&mb, 0.0f, 0.0f, 0.084f, 0.115f, 0.020f, 14, 8, 1);
			proc_mesh_add_arrow_x(&mb, -0.02f, 0.11f, 0.220f, 0.034f, 0.076f, 0.034f, 0.060f);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_race:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.034f, 0.028f, 0.082f, 0.022f, 10, 0, 0);
			proc_mesh_add_torus_yz(&mb, 0.0f, 0.0f, 0.195f, 0.102f, 0.022f, 18, 8, 0);
			proc_mesh_add_arrow_x(&mb, -0.05f, 0.09f, 0.195f, 0.028f, 0.064f, 0.034f, 0.055f);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_oddball:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.034f, 0.064f, 0.130f, 0.026f, 12, 0, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.165f, 0.025f, 0.210f, 0.075f, 14, 1, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.210f, 0.075f, 0.265f, 0.086f, 14, 0, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.265f, 0.086f, 0.315f, 0.062f, 14, 0, 0);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.315f, 0.062f, 0.339f, 0.020f, 14, 0, 1);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_ctf_flag:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_arrow_x(&mb, 0.0f, 0.11f, 0.234f, 0.032f, 0.072f, 0.034f, 0.056f);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.034f, 0.016f, 0.545f, 0.014f, 10, 0, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.545f, 0.024f, 0.565f, 0.006f, 10, 1, 1);
			proc_mesh_add_box(&mb, -0.220f, -0.008f, 0.385f, 0.0f, 0.008f, 0.535f);
			s_marker_meshes.meshes[k].radius = 0.60f;
			break;

		case _marker_mesh_koth:
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.012f, 0.164f, 0.034f, 0.156f, 20, 1, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.034f, 0.044f, 0.215f, 0.028f, 12, 0, 1);
			proc_mesh_add_frustum_z(&mb, 0.0f, 0.0f, 0.195f, 0.078f, 0.263f, 0.108f, 12, 1, 1);
			s_marker_meshes.meshes[k].radius = 0.45f;
			break;

		case _marker_mesh_koth_beam:
		default:
			proc_mesh_add_box(&mb, 0.0f, -0.014f, 0.025f, 1.0f, 0.014f, 0.053f);
			s_marker_meshes.meshes[k].radius = 1.00f;
			break;
		}

		if (mb.vtx_count >= 3 && mb.idx_count >= 3)
		{
			build_hw_part_from_arrays(
				&s_marker_meshes.meshes[k].parts[0],
				mb.verts,
				mb.vtx_count,
				(uint8_t const *)mb.strip,
				mb.idx_count - 2,
				"",
				1.0f,
				0.0f);
			s_marker_meshes.meshes[k].part_count = 1;
			s_marker_meshes.meshes[k].valid = 1;
		}

		free(mb.verts);
		free(mb.strip);
	}
}

static void draw_translucent_marker_hw_parts(
	union real_point3d const *pos,
	union real_vector3d const *forward,
	union real_vector3d const *up,
	float radius,
	float scale,
	struct forge_hw_part *parts,
	int part_count,
	int color_preset,
	int is_highlighted,
	int is_preview)
{
	struct rasterizer_model_begin_parameters model_parameters;
	static union real_rgb_color change_colors[4];
	static real function_values[4];
	struct real_matrix4x3 node_matrix;
	long col_shdr_idx;
	struct shader *col_shdr_def;
	int p;

	if (!parts || part_count <= 0)
		return;

	col_shdr_idx = get_or_create_translucent_marker_shader(color_preset, is_highlighted, 0);
	if (col_shdr_idx == NONE)
		return;
	col_shdr_def = shader_definition_get(col_shdr_idx);
	if (!col_shdr_def)
		return;

	matrix4x3_from_point_and_vectors(&node_matrix, pos, forward, up);
	if (scale > 0.001f && scale != 1.0f)
		node_matrix.scale = scale;

	change_colors[0].red = 1.0f;
	change_colors[0].green = 1.0f;
	change_colors[0].blue = 1.0f;
	change_colors[1] = change_colors[0];
	change_colors[2] = change_colors[0];
	change_colors[3] = change_colors[0];

	function_values[0] = 1.0f;
	function_values[1] = 1.0f;
	function_values[2] = 1.0f;
	function_values[3] = 1.0f;

	memset(&model_parameters, 0, sizeof(model_parameters));
	model_parameters.unique_identifier = NONE;
	model_parameters.lighting.ambient_color.red = is_highlighted ? 0.88f : 0.72f;
	model_parameters.lighting.ambient_color.green = is_highlighted ? 0.88f : 0.72f;
	model_parameters.lighting.ambient_color.blue = is_highlighted ? 0.88f : 0.72f;
	model_parameters.lighting.distant_light_count = 1;
	model_parameters.lighting.distant_lights[0].color.red = 0.95f;
	model_parameters.lighting.distant_lights[0].color.green = 0.95f;
	model_parameters.lighting.distant_lights[0].color.blue = 0.95f;
	model_parameters.lighting.distant_lights[0].direction.i = -0.45f;
	model_parameters.lighting.distant_lights[0].direction.j = -0.45f;
	model_parameters.lighting.distant_lights[0].direction.k = -0.77f;
	model_parameters.centroid = *pos;
	model_parameters.radius = (radius > 0.1f) ? (radius * ((scale > 0.001f) ? scale : 1.0f)) : 0.6f;
	model_parameters.animation.colors = change_colors;
	model_parameters.animation.values = function_values;
	model_parameters.skinning.node_matrices = &node_matrix;
	model_parameters.skinning.node_matrix_count = 1;
	model_parameters.geometry_flags = (1u << 2) | (is_preview ? (1u << 4) : 0u); /* _rasterizer_geometry_no_fog_bit */
	model_parameters.base_map_scale.i = 1.0f;
	model_parameters.base_map_scale.j = 1.0f;

	rasterizer_model_begin(&model_parameters, FALSE);

	for (p = 0; p < part_count; p++)
	{
		rasterizer_model_draw(
			col_shdr_def,
			0,
			&parts[p].triangle_buffer,
			NONE,
			parts[p].triangle_buffer.count,
			&parts[p].vertex_buffer,
			NONE);
	}
	rasterizer_model_end();
	if (is_preview)
	{
		rasterizer_set_stencil_mode(0);
		rasterizer_set_frustum_z(0.0f, 0.0f);
	}
}

static int resolve_marker_kind_and_color(
	uint32_t group_tag,
	char const *tag_path,
	int16_t netgame_flag_type,
	int16_t team_designator,
	int *out_mesh_kind,
	int *out_color_preset)
{
	if (group_tag == 'spwn' || (tag_path && strncmp(tag_path, "spawn\\", 6) == 0))
	{
		*out_mesh_kind = _marker_mesh_player_spawn;
		if (team_designator == 1 || (tag_path && strstr(tag_path, "blue")))
			*out_color_preset = _marker_color_blue_team;
		else if (team_designator == 2 || (tag_path && strstr(tag_path, "ffa")))
			*out_color_preset = _marker_color_ffa_yellow;
		else
			*out_color_preset = _marker_color_red_team;
		return 1;
	}
	if (group_tag == 'tele' || (tag_path && strncmp(tag_path, "teleporter\\", 11) == 0))
	{
		int is_sender = (netgame_flag_type == 6 /* _netgame_flag_teleporter_source */ ||
			(tag_path && (strstr(tag_path, "origin") || strstr(tag_path, "sender"))));
		*out_mesh_kind = is_sender ? _marker_mesh_tele_origin : _marker_mesh_tele_dest;
		*out_color_preset = _marker_color_tele_green;
		return 1;
	}
	if (group_tag == 'flag' || (tag_path && strncmp(tag_path, "objective\\", 10) == 0))
	{
		if (netgame_flag_type == 2 /* _netgame_flag_oddball_ball_spawn */ ||
			(tag_path && strstr(tag_path, "oddball")))
		{
			*out_mesh_kind = _marker_mesh_oddball;
			*out_color_preset = _marker_color_oddball_purple;
			return 1;
		}
		if (netgame_flag_type == 3 /* _netgame_flag_race_track */ ||
			netgame_flag_type == 4 /* _netgame_flag_race_vehicle */ ||
			(tag_path && strstr(tag_path, "race")))
		{
			*out_mesh_kind = _marker_mesh_race;
			*out_color_preset = _marker_color_race_orange;
			return 1;
		}
		if (netgame_flag_type == 0 /* _netgame_flag_ctf_flag */ ||
			netgame_flag_type == 1 /* _netgame_flag_ctf_vehicle */ ||
			(tag_path && strstr(tag_path, "ctf")))
		{
			*out_mesh_kind = _marker_mesh_ctf_flag;
			*out_color_preset = (team_designator == 1 || (tag_path && strstr(tag_path, "blue")))
				? _marker_color_blue_team
				: _marker_color_red_team;
			return 1;
		}
		*out_mesh_kind = _marker_mesh_koth;
		*out_color_preset = _marker_color_koth_gold;
		return 1;
	}
	return 0;
}

static void draw_translucent_koth_beam_3d(
	union real_point3d const *p0,
	union real_point3d const *p1)
{
	struct rasterizer_model_begin_parameters model_parameters;
	union real_rgb_color change_colors[4];
	real function_values[4];
	struct real_matrix4x3 node_matrix;
	long col_shdr_idx;
	struct shader *col_shdr_def;
	float dx, dy, dz, len_xy, len_3d;
	int p;

	if (!s_marker_meshes.meshes[_marker_mesh_koth_beam].valid ||
		s_marker_meshes.meshes[_marker_mesh_koth_beam].part_count <= 0)
		return;

	dx = p1->x - p0->x;
	dy = p1->y - p0->y;
	dz = p1->z - p0->z;
	len_xy = square_root(dx * dx + dy * dy);
	len_3d = square_root(dx * dx + dy * dy + dz * dz);
	if (len_3d < 0.01f)
		return;

	col_shdr_idx = get_or_create_translucent_marker_shader(_marker_color_koth_gold, 1, 0);
	if (col_shdr_idx == NONE)
		return;
	col_shdr_def = shader_definition_get(col_shdr_idx);
	if (!col_shdr_def)
		return;

	/*
	 * Stretch the unit 3D beam (x in [0..1], y in [-0.015..+0.015], z in [0.025..0.055])
	 * along forward = (p1 - p0) while keeping left and up unit-length so cross-section
	 * thickness stays constant regardless of segment length!
	 */
	memset(&node_matrix, 0, sizeof(node_matrix));
	node_matrix.scale = 1.0f;
	node_matrix.position = *p0;
	node_matrix.forward.i = dx;
	node_matrix.forward.j = dy;
	node_matrix.forward.k = dz;
	if (len_xy > 0.001f)
	{
		node_matrix.left.i = -dy / len_xy;
		node_matrix.left.j = dx / len_xy;
		node_matrix.left.k = 0.0f;
	}
	else
	{
		node_matrix.left.i = 0.0f;
		node_matrix.left.j = 1.0f;
		node_matrix.left.k = 0.0f;
	}
	node_matrix.up.i = 0.0f;
	node_matrix.up.j = 0.0f;
	node_matrix.up.k = 1.0f;

	change_colors[0].red = 1.0f;
	change_colors[0].green = 1.0f;
	change_colors[0].blue = 1.0f;
	change_colors[1] = change_colors[0];
	change_colors[2] = change_colors[0];
	change_colors[3] = change_colors[0];

	function_values[0] = 1.0f;
	function_values[1] = 1.0f;
	function_values[2] = 1.0f;
	function_values[3] = 1.0f;

	memset(&model_parameters, 0, sizeof(model_parameters));
	model_parameters.unique_identifier = NONE;
	model_parameters.lighting.ambient_color.red = 0.90f;
	model_parameters.lighting.ambient_color.green = 0.90f;
	model_parameters.lighting.ambient_color.blue = 0.90f;
	model_parameters.lighting.distant_light_count = 1;
	model_parameters.lighting.distant_lights[0].color.red = 0.95f;
	model_parameters.lighting.distant_lights[0].color.green = 0.95f;
	model_parameters.lighting.distant_lights[0].color.blue = 0.95f;
	model_parameters.lighting.distant_lights[0].direction.i = -0.45f;
	model_parameters.lighting.distant_lights[0].direction.j = -0.45f;
	model_parameters.lighting.distant_lights[0].direction.k = -0.77f;
	model_parameters.centroid.x = 0.5f * (p0->x + p1->x);
	model_parameters.centroid.y = 0.5f * (p0->y + p1->y);
	model_parameters.centroid.z = 0.5f * (p0->z + p1->z);
	model_parameters.radius = 0.5f * len_3d + 0.5f;
	model_parameters.animation.colors = change_colors;
	model_parameters.animation.values = function_values;
	model_parameters.skinning.node_matrices = &node_matrix;
	model_parameters.skinning.node_matrix_count = 1;
	model_parameters.geometry_flags = (1u << 2); /* _rasterizer_geometry_no_fog_bit */
	model_parameters.base_map_scale.i = 1.0f;
	model_parameters.base_map_scale.j = 1.0f;

	rasterizer_model_begin(&model_parameters, FALSE);
	for (p = 0; p < s_marker_meshes.meshes[_marker_mesh_koth_beam].part_count; p++)
	{
		rasterizer_model_draw(
			col_shdr_def,
			0,
			&s_marker_meshes.meshes[_marker_mesh_koth_beam].parts[p].triangle_buffer,
			NONE,
			s_marker_meshes.meshes[_marker_mesh_koth_beam].parts[p].triangle_buffer.count,
			&s_marker_meshes.meshes[_marker_mesh_koth_beam].parts[p].vertex_buffer,
			NONE);
	}
	rasterizer_model_end();
}

static void render_world_objective_and_spawn_markers(void)
{
	int i, hill_id;

	ensure_objective_marker_meshes_loaded();

	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		struct forge_live_instance const *inst = &g_forge.live_instances[i];
		int mesh_kind = 0, color_preset = 0;
		int is_hi;
		int is_stream_proxy = 0;
		float mesh_scale = 1.0f;
		float dx, dy, dz, d2;
		union real_point3d pos;
		union real_vector3d fwd, up;

		if (!inst->active)
			continue;
		if ((inst->pad_inst & 0x40u) != 0 && inst->object_index == NONE &&
			inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
		{
			is_stream_proxy = 1;
			mesh_kind = _marker_mesh_tele_origin;
			color_preset = _marker_color_blue_team;
			mesh_scale = 0.85f;
		}
		else
		{
			if (inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
				continue;
			if (!resolve_marker_kind_and_color(
					inst->group_tag,
					inst->tag_path,
					inst->netgame_flag_type,
					inst->team_designator,
					&mesh_kind,
					&color_preset))
				continue;
		}
		if (mesh_kind < 0 || mesh_kind >= NUMBER_OF_MARKER_MESH_KINDS ||
			!s_marker_meshes.meshes[mesh_kind].valid)
			continue;

		/* Player spawns, CTF flags, KOTH, Oddball, and Race markers are only visible while Forge Mode is active */
		if (!is_stream_proxy && g_forge.mode != _forge_mode_3d_monitor &&
			(mesh_kind == _marker_mesh_player_spawn ||
			 mesh_kind == _marker_mesh_ctf_flag ||
			 mesh_kind == _marker_mesh_koth ||
			 mesh_kind == _marker_mesh_oddball ||
			 mesh_kind == _marker_mesh_race))
		{
			continue;
		}

		if (mesh_kind == _marker_mesh_koth)
		{
			/* KOTH markers only highlight when actively clicked/grabbed (or during 10-node placement), never on hover/proximity */
			is_hi = (i == g_forge.grabbed_live_index) ||
					(g_forge.koth_group_placing && inst->team_designator == g_forge.koth_group_hill_id);
		}
		else
		{
			is_hi = (i == g_forge.grabbed_live_index || i == g_forge.hovered_live_index);
		}

		dx = inst->x - render.camera.position.x;
		dy = inst->y - render.camera.position.y;
		dz = inst->z - render.camera.position.z;
		d2 = dx * dx + dy * dy + dz * dz;
		if (d2 > 300.0f * 300.0f && !is_hi)
			continue;

		pos.x = inst->x;
		pos.y = inst->y;
		pos.z = inst->z;
		fwd.i = (float)cosine(inst->yaw);
		fwd.j = (float)sine(inst->yaw);
		fwd.k = 0.0f;
		up.i = 0.0f;
		up.j = 0.0f;
		up.k = 1.0f;

		draw_translucent_marker_hw_parts(
			&pos,
			&fwd,
			&up,
			s_marker_meshes.meshes[mesh_kind].radius * mesh_scale,
			mesh_scale,
			s_marker_meshes.meshes[mesh_kind].parts,
			s_marker_meshes.meshes[mesh_kind].part_count,
			color_preset,
			is_hi,
			0);
	}

	/* Render 3D translucent KOTH boundary beams ONLY in Forge mode when a node of that hill is actually clicked/grabbed
	 * (or actively being placed as a 10-point hill), connecting the exact 3D marker positions. */
	if (g_forge.mode != _forge_mode_3d_monitor)
		return;
	for (hill_id = 0; hill_id < 16; hill_id++)
	{
		int node_indices[32];
		float node_angles[32];
		int ncount = 0;
		int hill_grabbed = (g_forge.koth_group_placing && g_forge.koth_group_hill_id == hill_id) ? 1 : 0;
		float cx_h = 0.0f, cy_h = 0.0f;
		int a, b;

		for (i = 0; i < g_forge.live_instance_count && ncount < 32; i++)
		{
			struct forge_live_instance const *inst = &g_forge.live_instances[i];
			if (!inst->active || inst->group_tag != 'flag')
				continue;
			if (inst->netgame_flag_type != 8 /* _netgame_flag_hill */ && strstr(inst->tag_path, "koth") == NULL)
				continue;
			if (inst->team_designator != hill_id)
				continue;
			node_indices[ncount++] = i;
			cx_h += inst->x;
			cy_h += inst->y;
			if (i == g_forge.grabbed_live_index)
				hill_grabbed = 1;
		}

		if (ncount >= 2 && hill_grabbed)
		{
			cx_h /= (float)ncount;
			cy_h /= (float)ncount;

			for (a = 0; a < ncount; a++)
			{
				struct forge_live_instance const *na = &g_forge.live_instances[node_indices[a]];
				node_angles[a] = (float)atan2((double)(na->y - cy_h), (double)(na->x - cx_h));
			}
			for (a = 0; a < ncount - 1; a++)
			{
				for (b = a + 1; b < ncount; b++)
				{
					if (node_angles[b] < node_angles[a])
					{
						float ta = node_angles[a];
						int ti = node_indices[a];
						node_angles[a] = node_angles[b];
						node_indices[a] = node_indices[b];
						node_angles[b] = ta;
						node_indices[b] = ti;
					}
				}
			}

			for (a = 0; a < ncount; a++)
			{
				int nb = (a + 1) % ncount;
				struct forge_live_instance const *inst0 = &g_forge.live_instances[node_indices[a]];
				struct forge_live_instance const *inst1 = &g_forge.live_instances[node_indices[nb]];
				union real_point3d p0, p1;
				p0.x = inst0->x;
				p0.y = inst0->y;
				p0.z = inst0->z;
				p1.x = inst1->x;
				p1.y = inst1->y;
				p1.z = inst1->z;
				draw_translucent_koth_beam_3d(&p0, &p1);
			}
		}
	}
}

static void render_spawn_palette_3d_preview(long object_index, void const *lighting_ptr)
{
	int i, match_count = 0, chosen_idx = -1;
	struct forge_catalog_entry const *entry;
	union real_point3d preview_pos;
	union real_vector3d preview_fwd, preview_up, cam_right;
	float dist, angle;
	int marker_kind = -1, marker_color = 0;
	uint32_t actual_grp = 0;
	long obj_def_idx;

	if (g_forge.active_modal != _forge_modal_spawn_palette || g_forge.h3_menu_stage != 1)
		return;

	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (g_forge.catalog[i].category == g_forge.current_category &&
			forge_catalog_entry_allowed(&g_forge.catalog[i]))
		{
			if (match_count == g_forge.palette_cursor)
			{
				chosen_idx = i;
				break;
			}
			match_count++;
		}
	}
	if (chosen_idx < 0)
		return;

	entry = &g_forge.catalog[chosen_idx];

	/* Check if this catalog entry is one of our 3D Translucent Spawn/Teleporter/Objective Markers! */
	if (resolve_marker_kind_and_color(entry->group_tag, entry->tag_path, -1, -1, &marker_kind, &marker_color))
	{
		ensure_objective_marker_meshes_loaded();
		if (marker_kind >= 0 && marker_kind < NUMBER_OF_MARKER_MESH_KINDS &&
			s_marker_meshes.meshes[marker_kind].valid)
		{
			dist = 1.45f;
			cross_product3d(&render.camera.forward, &render.camera.up, &cam_right);
			preview_pos.x = render.camera.position.x + render.camera.forward.i * dist + cam_right.i * 0.42f - render.camera.up.i * 0.18f;
			preview_pos.y = render.camera.position.y + render.camera.forward.j * dist + cam_right.j * 0.42f - render.camera.up.j * 0.18f;
			preview_pos.z = render.camera.position.z + render.camera.forward.k * dist + cam_right.k * 0.42f - render.camera.up.k * 0.18f;

			angle = (float)(system_milliseconds() % 6000u) * (6.2831853f / 6000.0f);
			preview_fwd.i = (float)cosine(angle);
			preview_fwd.j = (float)sine(angle);
			preview_fwd.k = 0.0f;
			preview_up.i = 0.0f;
			preview_up.j = 0.0f;
			preview_up.k = 1.0f;

			draw_translucent_marker_hw_parts(
				&preview_pos,
				&preview_fwd,
				&preview_up,
				s_marker_meshes.meshes[marker_kind].radius,
				1.15f,
				s_marker_meshes.meshes[marker_kind].parts,
				s_marker_meshes.meshes[marker_kind].part_count,
				marker_color,
				1,
				1);
			return;
		}
	}

	{
		int known_missing = 0;
		obj_def_idx = forge_try_get_loaded_engine_tag(
			entry->group_tag,
			entry->tag_path,
			&actual_grp,
			&known_missing);
		if (obj_def_idx == NONE && !known_missing)
		{
			unsigned long now_ms = system_milliseconds();
			if (chosen_idx != s_last_preview_chosen_idx)
			{
				s_last_preview_chosen_idx = chosen_idx;
				s_last_preview_cursor_ms = now_ms;
				return;
			}
			if ((unsigned long)(now_ms - s_last_preview_cursor_ms) < 120u)
				return;
			obj_def_idx = forge_resolve_engine_tag(entry->group_tag, entry->tag_path, &actual_grp);
		}
		else
		{
			s_last_preview_chosen_idx = chosen_idx;
		}
	}
	if (obj_def_idx == NONE)
		return;

	dist = 1.55f;
	cross_product3d(&render.camera.forward, &render.camera.up, &cam_right);
	preview_pos.x = render.camera.position.x + render.camera.forward.i * dist + cam_right.i * 0.42f - render.camera.up.i * 0.14f;
	preview_pos.y = render.camera.position.y + render.camera.forward.j * dist + cam_right.j * 0.42f - render.camera.up.j * 0.14f;
	preview_pos.z = render.camera.position.z + render.camera.forward.k * dist + cam_right.k * 0.42f - render.camera.up.k * 0.14f;

	angle = (float)(system_milliseconds() % 6000u) * (6.2831853f / 6000.0f);
	preview_fwd.i = (float)cosine(angle);
	preview_fwd.j = (float)sine(angle);
	preview_fwd.k = 0.0f;
	preview_up.i = 0.0f;
	preview_up.j = 0.0f;
	preview_up.k = 1.0f;

	draw_native_object_model_at(
		obj_def_idx,
		object_index,
		lighting_ptr,
		&preview_pos,
		&preview_fwd,
		&preview_up,
		0,
		1,
		0);
}

void forge_render_world_3d_overlays(void const *lighting_ptr)
{
	if (!g_forge.initialized || (!g_forge_session_enabled && g_forge.live_instance_count <= 0))
		return;
	if (g_forge_session_enabled && g_forge.mode == _forge_mode_3d_monitor && g_forge.third_person_view && !s_local_monitor_drawn_this_frame)
	{
		long local_unit = player_control_get_unit_index(0);
		if (local_unit != NONE)
			forge_monitor_render_player_unit(local_unit, lighting_ptr, 0);
	}
	s_local_monitor_drawn_this_frame = 0;
	render_world_objective_and_spawn_markers();
	if (g_forge_session_enabled)
		render_spawn_palette_3d_preview(NONE, lighting_ptr);
}

int forge_monitor_render_player_unit(long object_index, void const *lighting_ptr, unsigned long flags)
{
	struct unit_datum *unit;

	if (!g_forge_session_enabled || object_index == NONE)
		return 0;

	/* Check if this object is a player biped in Forge Monitor mode (per-player in MP!) */
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_biped);
	if (!unit || unit->object.type != _object_type_biped || unit->unit.player_index == NONE)
		return 0;

	if (!forge_is_unit_in_monitor_mode(object_index))
		return 0;

	if (flags & (1u << 1)) /* Skip shadow pass for Monitor */
		return 1;

	if (!s_monitor_resolved || s_monitor_bipd_handle == NONE)
	{
		uint32_t actual_grp = 'bipd';
		s_monitor_resolved = 1;
		s_monitor_bipd_handle = forge_resolve_engine_tag('bipd', "characters\\monitor\\monitor", &actual_grp);
	}
	if (s_monitor_bipd_handle == NONE)
		return 0;

	if (object_index == player_control_get_unit_index(0))
	{
		union real_point3d pos;
		union real_vector3d fwd, up;
		float cy, sy, cp, sp;

		if (!g_forge.third_person_view)
			return 1;
		s_local_monitor_drawn_this_frame = 1;

		/* Place local Monitor at exact live camera anchor (draw_native_object_model_at adds +0.55f to Z) */
		pos.x = g_forge.cam_x;
		pos.y = g_forge.cam_y;
		pos.z = g_forge.cam_z - 0.55f;

		cy = (float)cosine(g_forge.cam_yaw);
		sy = (float)sine(g_forge.cam_yaw);
		cp = (float)cosine(g_forge.cam_pitch);
		sp = (float)sine(g_forge.cam_pitch);
		fwd.i = cy * cp;
		fwd.j = sy * cp;
		fwd.k = sp;
		up.i = -cy * sp;
		up.j = -sy * sp;
		up.k = cp;

		return draw_native_object_model_at(
			s_monitor_bipd_handle,
			NONE,
			lighting_ptr,
			&pos,
			&fwd,
			&up,
			1,
			0,
			flags);
	}

	{
		union real_point3d pos = unit->object.position;
		union real_vector3d fwd = unit->object.forward;
		union real_vector3d up = unit->object.up;
		int p_abs = (int)((uint32_t)unit->unit.player_index & 0xFFFFu);

		if (p_abs >= 0 && p_abs < 128 && g_forge.player_has_monitor_pos[p_abs])
		{
			float rx = g_forge.player_has_render_pos[p_abs]
				? g_forge.player_monitor_render_pos[p_abs][0]
				: g_forge.player_monitor_pos[p_abs][0];
			float ry = g_forge.player_has_render_pos[p_abs]
				? g_forge.player_monitor_render_pos[p_abs][1]
				: g_forge.player_monitor_pos[p_abs][1];
			float rz = g_forge.player_has_render_pos[p_abs]
				? g_forge.player_monitor_render_pos[p_abs][2]
				: g_forge.player_monitor_pos[p_abs][2];
			float ryaw = g_forge.player_has_render_pos[p_abs]
				? g_forge.player_monitor_render_yaw[p_abs]
				: g_forge.player_monitor_yaw[p_abs];
			float rpitch = g_forge.player_has_render_pos[p_abs]
				? g_forge.player_monitor_render_pitch[p_abs]
				: g_forge.player_monitor_pitch[p_abs];
			float cy = (float)cosine(ryaw);
			float sy = (float)sine(ryaw);
			float cp = (float)cosine(rpitch);
			float sp = (float)sine(rpitch);

			pos.x = rx;
			pos.y = ry;
			pos.z = rz - 0.55f;
			fwd.i = cy * cp;
			fwd.j = sy * cp;
			fwd.k = sp;
			up.i = -cy * sp;
			up.j = -sy * sp;
			up.k = cp;
		}

		return draw_native_object_model_at(
			s_monitor_bipd_handle,
			NONE,
			lighting_ptr,
			&pos,
			&fwd,
			&up,
			1,
			0,
			flags);
	}
}


/* ============================================================================
 * FORGE TAG OPTIONS: ADD TAGS (MAP TAG BROWSER), INSTALLED TAGS & 3D PREVIEW
 * ============================================================================ */

#define FORGE_BROWSER_NUM_CATS 5

struct browser_ce_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct browser_ce_map_header
{
	struct browser_ce_tag_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long model_part_count;
	unsigned long model_data_offset;
	long model_part_count_again;
	unsigned long model_index_data_offset;
	unsigned long model_data_size;
	unsigned long signature;
};

static struct
{
	int is_open;
	int is_ce_map;
	int is_xbox_map;
	int is_mcc_map;
	char map_path[512];
	char decomp_xbox_path[512];
	char map_name[64];
	struct browser_ce_map_header *ce_header;
	unsigned char *names_made;
	FILE *xbox_fp;
	uint8_t *xbox_tag_buf;
	uint32_t xbox_tag_sz;
	uint32_t xbox_tag_base;
	uint32_t xbox_inst_off;
	int32_t xbox_tag_count;
	struct mcc_runtime *mcc_rt;
	int32_t *cat_tags[FORGE_BROWSER_NUM_CATS];
	int32_t *cat_sizes[FORGE_BROWSER_NUM_CATS];
	int cat_counts[FORGE_BROWSER_NUM_CATS];
} s_map_browser;

static struct
{
	char map_path[512];
	FILE *fp;
	uint8_t *tag_buf;
	uint32_t tag_sz;
	uint32_t tag_base;
	uint32_t inst_off;
	int32_t tag_count;
} s_preview_xbox_donor;

static void close_preview_xbox_donor(void)
{
	if (s_preview_xbox_donor.fp)
		fclose(s_preview_xbox_donor.fp);
	if (s_preview_xbox_donor.tag_buf)
		free(s_preview_xbox_donor.tag_buf);
	memset(&s_preview_xbox_donor, 0, sizeof(s_preview_xbox_donor));
}

static int open_preview_xbox_donor_for_tag(uint32_t req_group, char const *req_path)
{
	char stems[16][64];
	int stem_count, s, d;
	static char const *const map_dirs[] = {
		"maps",
		"custom_maps",
		"assets/maps",
		"../assets/maps"
	};

	if (s_preview_xbox_donor.fp && s_preview_xbox_donor.tag_buf)
	{
		int32_t i;
		char const *lookup = (req_group == 'mach' &&
			_stricmp(req_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0)
			? "levels\\a30\\devices\\beam emitter\\beam emitter"
			: req_path;
		for (i = 0; i < s_preview_xbox_donor.tag_count; i++)
		{
			uint32_t io = s_preview_xbox_donor.inst_off + (uint32_t)i * 32u;
			uint32_t grp = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x00);
			uint32_t name_addr = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x10);
			if (name_addr >= s_preview_xbox_donor.tag_base &&
				(name_addr - s_preview_xbox_donor.tag_base) < s_preview_xbox_donor.tag_sz)
			{
				char const *tname = (char const *)(s_preview_xbox_donor.tag_buf + (name_addr - s_preview_xbox_donor.tag_base));
				if (grp == req_group && _stricmp(tname, lookup) == 0)
					return 1;
			}
		}
	}

	stem_count = find_donor_map_stems_from_credits(req_group, req_path, stems, 16);
	for (s = 0; s < stem_count; s++)
	{
		for (d = 0; d < 4; d++)
		{
			char cand[512];
			char decomp[512];
			FILE *f;
			uint8_t hdr[2048];
			uint32_t tag_off, tag_sz, inst_addr;
			int32_t tag_count;
			uint8_t *tbuf;

			snprintf(cand, sizeof(cand), "%s/%s.map", map_dirs[d], stems[s]);
			f = fopen(cand, "rb");
			if (!f)
				continue;
			fclose(f);
			if (!is_xbox_version5_map_file(cand))
				continue;
			if (!ensure_xbox_map_decompressed_cache(cand, decomp, sizeof(decomp)))
				continue;
			f = fopen(decomp, "rb");
			if (!f)
				continue;
			if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) ||
				*(uint32_t const *)(hdr + 0x00) != 0x68656164u ||
				*(uint32_t const *)(hdr + 0x04) != 5u)
			{
				fclose(f);
				continue;
			}
			tag_off = *(uint32_t const *)(hdr + 0x10);
			tag_sz = *(uint32_t const *)(hdr + 0x14);
			if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
			{
				fclose(f);
				continue;
			}
			tbuf = (uint8_t *)malloc(tag_sz);
			if (!tbuf || fseek(f, (long)tag_off, SEEK_SET) != 0 || fread(tbuf, 1, tag_sz, f) != tag_sz)
			{
				free(tbuf);
				fclose(f);
				continue;
			}
			inst_addr = *(uint32_t const *)(tbuf + 0x00);
			tag_count = *(int32_t const *)(tbuf + 0x0C);
			if (tag_count <= 0 || tag_count > 16384 || 0x24u + (uint32_t)tag_count * 32u > tag_sz)
			{
				free(tbuf);
				fclose(f);
				continue;
			}
			close_preview_xbox_donor();
			snprintf(s_preview_xbox_donor.map_path, sizeof(s_preview_xbox_donor.map_path), "%s", decomp);
			s_preview_xbox_donor.fp = f;
			s_preview_xbox_donor.tag_buf = tbuf;
			s_preview_xbox_donor.tag_sz = tag_sz;
			s_preview_xbox_donor.tag_base = inst_addr - 0x24u;
			s_preview_xbox_donor.inst_off = 0x24u;
			s_preview_xbox_donor.tag_count = tag_count;
			return 1;
		}
	}
	return 0;
}

static int browser_group_matches_category(uint32_t grp, int category_tab)
{
	switch (category_tab)
	{
	case 0: /* SCENERY */
		return grp == 'scen';
	case 1: /* VEHICLES */
		return grp == 'vehi';
	case 2: /* DEVICES */
		return grp == 'mach' || grp == 'lifi' || grp == 'ctrl';
	case 3: /* WEAPONS */
		return grp == 'weap';
	case 4: /* EQUIPMENT */
		return grp == 'eqip';
	default:
		return 0;
	}
}

static char const *browser_group_type_label(uint32_t grp)
{
	switch (grp)
	{
	case 'scen': return "SCENERY (scen)";
	case 'vehi': return "VEHICLE (vehi)";
	case 'mach': return "DEVICE MACHINE (mach)";
	case 'lifi': return "LIGHT FIXTURE (lifi)";
	case 'ctrl': return "DEVICE CONTROL (ctrl)";
	case 'weap': return "WEAPON (weap)";
	case 'eqip': return "EQUIPMENT (eqip)";
	default:     return "OBJECT TAG";
	}
}

static void format_tag_byte_size(int32_t bytes, char *out, size_t out_sz)
{
	if (!out || out_sz == 0)
		return;
	if (bytes <= 0)
		snprintf(out, out_sz, "N/A");
	else if (bytes >= 1024 * 1024)
		snprintf(out, out_sz, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
	else if (bytes >= 1024)
		snprintf(out, out_sz, "%.1f KB", (double)bytes / 1024.0);
	else
		snprintf(out, out_sz, "%d B", (int)bytes);
}

static char const *browser_tag_name_at(int32_t abs_idx)
{
	if (s_map_browser.is_ce_map && s_map_browser.ce_header)
	{
		if (abs_idx >= 0 && abs_idx < s_map_browser.ce_header->tag_count &&
			s_map_browser.ce_header->instances[abs_idx].name)
			return s_map_browser.ce_header->instances[abs_idx].name;
	}
	else if (s_map_browser.is_xbox_map && s_map_browser.xbox_tag_buf)
	{
		if (abs_idx >= 0 && abs_idx < s_map_browser.xbox_tag_count)
		{
			uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)abs_idx * 32u;
			uint32_t name_addr = *(uint32_t const *)(s_map_browser.xbox_tag_buf + io + 0x10);
			if (name_addr >= s_map_browser.xbox_tag_base &&
				(name_addr - s_map_browser.xbox_tag_base) < s_map_browser.xbox_tag_sz)
				return (char const *)(s_map_browser.xbox_tag_buf + (name_addr - s_map_browser.xbox_tag_base));
		}
	}
	return "";
}

static uint32_t browser_tag_group_at(int32_t abs_idx)
{
	if (s_map_browser.is_ce_map && s_map_browser.ce_header)
	{
		if (abs_idx >= 0 && abs_idx < s_map_browser.ce_header->tag_count)
			return (uint32_t)s_map_browser.ce_header->instances[abs_idx].group_tag;
	}
	else if (s_map_browser.is_xbox_map && s_map_browser.xbox_tag_buf)
	{
		if (abs_idx >= 0 && abs_idx < s_map_browser.xbox_tag_count)
		{
			uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)abs_idx * 32u;
			return *(uint32_t const *)(s_map_browser.xbox_tag_buf + io + 0x00);
		}
	}
	return 0;
}

static int browser_sort_cmp(void const *a, void const *b)
{
	int32_t ia = *(int32_t const *)a;
	int32_t ib = *(int32_t const *)b;
	return _stricmp(browser_tag_name_at(ia), browser_tag_name_at(ib));
}

static void forge_ui_close_map_tag_browser(void)
{
	int c;
	for (c = 0; c < FORGE_BROWSER_NUM_CATS; c++)
	{
		if (s_map_browser.cat_tags[c])
			free(s_map_browser.cat_tags[c]);
		if (s_map_browser.cat_sizes[c])
			free(s_map_browser.cat_sizes[c]);
		s_map_browser.cat_tags[c] = NULL;
		s_map_browser.cat_sizes[c] = NULL;
		s_map_browser.cat_counts[c] = 0;
	}
	if (s_map_browser.is_ce_map)
		custom_edition_cache_bundle_close();
	tag_names_dispose();
	if (s_map_browser.names_made)
		free(s_map_browser.names_made);
	if (s_map_browser.mcc_rt)
		mcc_forge_browser_close(s_map_browser.mcc_rt);
	if (s_map_browser.xbox_fp)
		fclose(s_map_browser.xbox_fp);
	if (s_map_browser.xbox_tag_buf)
		free(s_map_browser.xbox_tag_buf);
	memset(&s_map_browser, 0, sizeof(s_map_browser));
	close_preview_xbox_donor();
}

int forge_ui_open_map_for_tag_browser(char const *map_path)
{
	char const *s1;
	char const *s2;
	char const *leaf;
	char *dot;
	struct browser_ce_map_header *ce_hdr;
	int c;

	forge_ui_close_map_tag_browser();
	if (!map_path || !map_path[0])
		return 0;

	snprintf(s_map_browser.map_path, sizeof(s_map_browser.map_path), "%s", map_path);
	s1 = strrchr(map_path, '/');
	s2 = strrchr(map_path, '\\');
	leaf = (s1 > s2) ? (s1 + 1) : (s2 ? (s2 + 1) : map_path);
	snprintf(s_map_browser.map_name, sizeof(s_map_browser.map_name), "%s", leaf);
	dot = strrchr(s_map_browser.map_name, '.');
	if (dot)
		*dot = '\0';

	/* 1. Try opening as a Custom Edition / PC cache map first */
	ce_hdr = (is_xbox_version5_map_file(map_path) || is_mcc_version13_map_file(map_path)) ? NULL : (struct browser_ce_map_header *)custom_edition_cache_bundle_open(map_path, NULL, 0);
	if (ce_hdr && ce_hdr->tag_count > 0 && ce_hdr->instances)
	{
		int32_t i;

		s_map_browser.is_open = 1;
		s_map_browser.is_ce_map = 1;
		s_map_browser.ce_header = ce_hdr;
		tag_names_restore(ce_hdr, s_map_browser.map_name, NULL);
		s_map_browser.names_made = NULL;

		for (c = 0; c < FORGE_BROWSER_NUM_CATS; c++)
		{
			int cnt = 0;
			for (i = 0; i < ce_hdr->tag_count; i++)
			{
				if (browser_group_matches_category((uint32_t)ce_hdr->instances[i].group_tag, c))
					cnt++;
			}
			if (cnt > 0)
			{
				int idx = 0;
				s_map_browser.cat_tags[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				s_map_browser.cat_sizes[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				if (s_map_browser.cat_tags[c] && s_map_browser.cat_sizes[c])
				{
					for (i = 0; i < ce_hdr->tag_count; i++)
					{
						if (browser_group_matches_category((uint32_t)ce_hdr->instances[i].group_tag, c))
						{
							s_map_browser.cat_tags[c][idx] = i;
							s_map_browser.cat_sizes[c][idx] = -1;
							idx++;
						}
					}
					s_map_browser.cat_counts[c] = idx;
					qsort(s_map_browser.cat_tags[c], (size_t)idx, sizeof(int32_t), browser_sort_cmp);
				}
			}
		}
		return 1;
	}
	if (ce_hdr)
		custom_edition_cache_bundle_close();

	/* 2. Open as a Stock Xbox v5 map (compressed or decompressed) */
	if (is_xbox_version5_map_file(map_path))
	{
		char decomp_path[512];
		FILE *fp;
		uint8_t hdr[2048];
		uint32_t tag_off, tag_sz, inst_addr;
		int32_t tag_count, i;
		uint8_t *tbuf;

		CreateDirectoryA("OpenCE Forge", NULL);
		CreateDirectoryA("OpenCE Forge/cached_tags", NULL);
		if (!ensure_xbox_map_decompressed_cache(map_path, decomp_path, sizeof(decomp_path)))
			return 0;

		fp = fopen(decomp_path, "rb");
		if (!fp)
			return 0;
		if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr) ||
			*(uint32_t const *)(hdr + 0x00) != 0x68656164u ||
			*(uint32_t const *)(hdr + 0x04) != 5u)
		{
			fclose(fp);
			return 0;
		}
		tag_off = *(uint32_t const *)(hdr + 0x10);
		tag_sz = *(uint32_t const *)(hdr + 0x14);
		if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
		{
			fclose(fp);
			return 0;
		}
		tbuf = (uint8_t *)malloc(tag_sz);
		if (!tbuf || fseek(fp, (long)tag_off, SEEK_SET) != 0 || fread(tbuf, 1, tag_sz, fp) != tag_sz)
		{
			free(tbuf);
			fclose(fp);
			return 0;
		}
		inst_addr = *(uint32_t const *)(tbuf + 0x00);
		tag_count = *(int32_t const *)(tbuf + 0x0C);
		if (tag_count <= 0 || tag_count > 16384 || 0x24u + (uint32_t)tag_count * 32u > tag_sz)
		{
			free(tbuf);
			fclose(fp);
			return 0;
		}

		s_map_browser.is_open = 1;
		s_map_browser.is_xbox_map = 1;
		snprintf(s_map_browser.decomp_xbox_path, sizeof(s_map_browser.decomp_xbox_path), "%s", decomp_path);
		s_map_browser.xbox_fp = fp;
		s_map_browser.xbox_tag_buf = tbuf;
		s_map_browser.xbox_tag_sz = tag_sz;
		s_map_browser.xbox_tag_base = inst_addr - 0x24u;
		s_map_browser.xbox_inst_off = 0x24u;
		s_map_browser.xbox_tag_count = tag_count;

		for (c = 0; c < FORGE_BROWSER_NUM_CATS; c++)
		{
			int cnt = 0;
			for (i = 0; i < tag_count; i++)
			{
				uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)i * 32u;
				uint32_t grp = *(uint32_t const *)(tbuf + io + 0x00);
				if (browser_group_matches_category(grp, c))
					cnt++;
			}
			if (cnt > 0)
			{
				int idx = 0;
				s_map_browser.cat_tags[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				s_map_browser.cat_sizes[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				if (s_map_browser.cat_tags[c] && s_map_browser.cat_sizes[c])
				{
					for (i = 0; i < tag_count; i++)
					{
						uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)i * 32u;
						uint32_t grp = *(uint32_t const *)(tbuf + io + 0x00);
						if (browser_group_matches_category(grp, c))
						{
							s_map_browser.cat_tags[c][idx] = i;
							s_map_browser.cat_sizes[c][idx] = -1;
							idx++;
						}
					}
					s_map_browser.cat_counts[c] = idx;
					qsort(s_map_browser.cat_tags[c], (size_t)idx, sizeof(int32_t), browser_sort_cmp);
				}
			}
		}
		return 1;
	}
	else if (is_mcc_version13_map_file(map_path))
	{
		FILE *fp;
		uint8_t hdr[2048];
		uint32_t tag_off, tag_sz, inst_addr;
		int32_t tag_count, i;
		uint8_t *tbuf;

		CreateDirectoryA("OpenCE Forge", NULL);
		CreateDirectoryA("OpenCE Forge/cached_tags", NULL);

		fp = fopen(map_path, "rb");
		if (!fp)
			return 0;
		if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr) ||
			*(uint32_t const *)(hdr + 0x00) != 0x68656164u ||
			*(uint32_t const *)(hdr + 0x04) != 13u)
		{
			fclose(fp);
			return 0;
		}
		tag_off = *(uint32_t const *)(hdr + 0x10);
		tag_sz = *(uint32_t const *)(hdr + 0x14);
		if (tag_off < 2048u || tag_sz < 64u || tag_sz > 64u * 1024u * 1024u)
		{
			fclose(fp);
			return 0;
		}
		tbuf = (uint8_t *)malloc(tag_sz);
		if (!tbuf || fseek(fp, (long)tag_off, SEEK_SET) != 0 || fread(tbuf, 1, tag_sz, fp) != tag_sz)
		{
			free(tbuf);
			fclose(fp);
			return 0;
		}
		inst_addr = *(uint32_t const *)(tbuf + 0x00);
		tag_count = *(int32_t const *)(tbuf + 0x0C);
		if (tag_count <= 0 || tag_count > 16384 || 0x28u + (uint32_t)tag_count * 32u > tag_sz)
		{
			free(tbuf);
			fclose(fp);
			return 0;
		}

		s_map_browser.is_open = 1;
		s_map_browser.is_xbox_map = 1;
		s_map_browser.is_mcc_map = 1;
		snprintf(s_map_browser.decomp_xbox_path, sizeof(s_map_browser.decomp_xbox_path), "%s", map_path);
		s_map_browser.xbox_fp = fp;
		s_map_browser.xbox_tag_buf = tbuf;
		s_map_browser.xbox_tag_sz = tag_sz;
		s_map_browser.xbox_tag_base = inst_addr - 0x28u;
		s_map_browser.xbox_inst_off = 0x28u;
		s_map_browser.xbox_tag_count = tag_count;
		s_map_browser.mcc_rt = mcc_forge_browser_open(fp, map_path, tbuf, tag_sz);

		for (c = 0; c < FORGE_BROWSER_NUM_CATS; c++)
		{
			int cnt = 0;
			for (i = 0; i < tag_count; i++)
			{
				uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)i * 32u;
				uint32_t grp = *(uint32_t const *)(tbuf + io + 0x00);
				if (browser_group_matches_category(grp, c))
					cnt++;
			}
			if (cnt > 0)
			{
				int idx = 0;
				s_map_browser.cat_tags[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				s_map_browser.cat_sizes[c] = (int32_t *)malloc((size_t)cnt * sizeof(int32_t));
				if (s_map_browser.cat_tags[c] && s_map_browser.cat_sizes[c])
				{
					for (i = 0; i < tag_count; i++)
					{
						uint32_t io = s_map_browser.xbox_inst_off + (uint32_t)i * 32u;
						uint32_t grp = *(uint32_t const *)(tbuf + io + 0x00);
						if (browser_group_matches_category(grp, c))
						{
							s_map_browser.cat_tags[c][idx] = i;
							s_map_browser.cat_sizes[c][idx] = -1;
							idx++;
						}
					}
					s_map_browser.cat_counts[c] = idx;
					qsort(s_map_browser.cat_tags[c], (size_t)idx, sizeof(int32_t), browser_sort_cmp);
				}
			}
		}
		return 1;
	}

	return 0;
}

int forge_ui_map_tag_count(int category_tab)
{
	if (!s_map_browser.is_open || category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS)
		return 0;
	return s_map_browser.cat_counts[category_tab];
}

static boolean bundle_count_size_cb(void *context, void const *bytes, unsigned long size)
{
	unsigned long *total = (unsigned long *)context;
	(void)bytes;
	*total += size;
	return TRUE;
}

static int32_t compute_xbox_tag_closure_bytes(
	uint8_t const *tag_buf,
	uint32_t tag_sz,
	uint32_t tag_base,
	uint32_t inst_off,
	int32_t tag_count,
	int32_t root_abs_idx)
{
	struct xbox_runtime_closure cl;
	int32_t i;
	uint32_t total = 2048u;

	if (!tag_buf || root_abs_idx < 0 || root_abs_idx >= tag_count)
		return 0;
	memset(&cl, 0, sizeof(cl));
	cl.tag_buf = tag_buf;
	cl.tag_sz = tag_sz;
	cl.tag_base = tag_base;
	cl.inst_off = inst_off;
	cl.tag_count = tag_count;
	cl.include_all_dependencies = 1;
	cl.visited = (uint8_t *)calloc((size_t)tag_count, 1);
	cl.order = (int32_t *)calloc((size_t)tag_count, sizeof(int32_t));
	cl.runtime_handles = (long *)malloc((size_t)tag_count * sizeof(long));
	if (!cl.visited || !cl.order || !cl.runtime_handles)
	{
		free(cl.visited);
		free(cl.order);
		free(cl.runtime_handles);
		return 0;
	}
	for (i = 0; i < tag_count; i++)
		cl.runtime_handles[i] = NONE;
	xbox_visit_tag_index(&cl, root_abs_idx);

	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = inst_off + (uint32_t)abs_idx * 32u;
		uint32_t grp = *(uint32_t const *)(tag_buf + io + 0x00);
		uint32_t data_addr = *(uint32_t const *)(tag_buf + io + 0x14);
		struct tag_schema_group const *schema = forge_runtime_schema_group_get(grp);

		total += 64u;
		if (schema && schema->definition && schema->definition->size > 0)
			total += (uint32_t)schema->definition->size * 2u;
		if (grp == 'bitm' && data_addr >= tag_base && (data_addr - tag_base) + 0x6Cu <= tag_sz)
		{
			uint32_t b_off = data_addr - tag_base;
			int32_t b_cnt = *(int32_t const *)(tag_buf + b_off + 0x60u);
			uint32_t b_addr = *(uint32_t const *)(tag_buf + b_off + 0x64u);
			int b_i;
			if (b_cnt > 0 && b_cnt <= 256 && b_addr >= tag_base &&
				(b_addr - tag_base) + (uint32_t)b_cnt * 48u <= tag_sz)
			{
				for (b_i = 0; b_i < b_cnt; b_i++)
				{
					int32_t p_sz = *(int32_t const *)(tag_buf + (b_addr - tag_base) + (uint32_t)b_i * 48u + 28u);
					if (p_sz > 0 && p_sz <= 16 * 1024 * 1024)
						total += (uint32_t)p_sz;
				}
			}
		}
		else if ((grp == 'mode' || grp == 'mod2') && data_addr >= tag_base && (data_addr - tag_base) + 0xE8u <= tag_sz)
		{
			uint32_t m_off = data_addr - tag_base;
			int32_t g_cnt = *(int32_t const *)(tag_buf + m_off + 0xD0u);
			uint32_t g_addr = *(uint32_t const *)(tag_buf + m_off + 0xD4u);
			uint32_t part_stride = (grp == 'mod2') ? 132u : 104u;
			int g_i;
			if (g_cnt > 0 && g_cnt <= 64 && g_addr >= tag_base &&
				(g_addr - tag_base) + (uint32_t)g_cnt * 48u <= tag_sz)
			{
				for (g_i = 0; g_i < g_cnt; g_i++)
				{
					uint32_t go = (g_addr - tag_base) + (uint32_t)g_i * 48u;
					int32_t p_cnt = *(int32_t const *)(tag_buf + go + 36u);
					uint32_t p_addr = *(uint32_t const *)(tag_buf + go + 40u);
					int p_i;
					if (p_cnt > 0 && p_cnt <= 128 && p_addr >= tag_base &&
						(p_addr - tag_base) + (uint32_t)p_cnt * part_stride <= tag_sz)
					{
						for (p_i = 0; p_i < p_cnt; p_i++)
						{
							uint32_t po = (p_addr - tag_base) + (uint32_t)p_i * part_stride;
							int32_t t_cnt = *(int32_t const *)(tag_buf + po + 72u);
							int32_t v_cnt = *(int32_t const *)(tag_buf + po + 88u);
							if (v_cnt > 0 && v_cnt <= 65536)
								total += (uint32_t)v_cnt * 32u;
							if (t_cnt > 0 && t_cnt <= 65536)
								total += ((uint32_t)t_cnt + 2u) * 2u;
						}
					}
				}
			}
		}
	}

	free(cl.visited);
	free(cl.order);
	free(cl.runtime_handles);
	return (int32_t)total;
}

int forge_ui_map_tag_get_info(
	int category_tab,
	int list_index,
	char *out_short_name,
	size_t short_sz,
	char *out_full_path,
	size_t full_sz,
	char *out_type_str,
	size_t type_sz,
	char *out_size_str,
	size_t size_sz)
{
	int32_t abs_idx;
	char const *full_path;
	char const *leaf;
	char const *p;
	uint32_t grp;
	int32_t byte_size;

	if (!s_map_browser.is_open || category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS ||
		list_index < 0 || list_index >= s_map_browser.cat_counts[category_tab])
		return 0;

	abs_idx = s_map_browser.cat_tags[category_tab][list_index];
	full_path = browser_tag_name_at(abs_idx);
	grp = browser_tag_group_at(abs_idx);
	leaf = full_path;
	for (p = full_path; *p; p++)
	{
		if (*p == '\\' || *p == '/')
			leaf = p + 1;
	}

	if (out_short_name && short_sz > 0)
		snprintf(out_short_name, short_sz, "%s", leaf);
	if (out_full_path && full_sz > 0)
		snprintf(out_full_path, full_sz, "%s", full_path);
	if (out_type_str && type_sz > 0)
		snprintf(out_type_str, type_sz, "%s", browser_group_type_label(grp));

	byte_size = s_map_browser.cat_sizes[category_tab][list_index];
	if (byte_size < 0 && out_size_str && size_sz > 0)
	{
		if (s_map_browser.is_ce_map)
		{
			struct tag_bundle_source source;
			struct tag_bundle_report report;
			unsigned long counted = 0;
			custom_edition_cache_bundle_source(&source);
			memset(&report, 0, sizeof(report));
			if (tag_bundle_write(&source, abs_idx, bundle_count_size_cb, &counted, &report) == _tag_bundle_status_ok)
				byte_size = (int32_t)report.size;
			else
				byte_size = 0;
		}
		else if (s_map_browser.is_xbox_map)
		{
			byte_size = compute_xbox_tag_closure_bytes(
				s_map_browser.xbox_tag_buf,
				s_map_browser.xbox_tag_sz,
				s_map_browser.xbox_tag_base,
				s_map_browser.xbox_inst_off,
				s_map_browser.xbox_tag_count,
				abs_idx);
		}
		s_map_browser.cat_sizes[category_tab][list_index] = byte_size;
	}
	if (out_size_str && size_sz > 0)
		format_tag_byte_size(byte_size, out_size_str, size_sz);
	return 1;
}

struct forge_grow_buf
{
	byte *bytes;
	unsigned long size;
	unsigned long capacity;
};

static boolean forge_grow_buf_write(void *context, void const *bytes, unsigned long size)
{
	struct forge_grow_buf *buffer = (struct forge_grow_buf *)context;
	if (size > 0x7FFFFFFFUL - buffer->size)
		return FALSE;
	if (buffer->size + size > buffer->capacity)
	{
		unsigned long cap = buffer->capacity ? buffer->capacity : 0x100000UL;
		byte *grown;
		while (cap < buffer->size + size)
		{
			if (cap > 0x3FFFFFFFUL)
				return FALSE;
			cap *= 2;
		}
		grown = buffer->bytes ? (byte *)realloc(buffer->bytes, cap) : (byte *)malloc(cap);
		if (!grown)
			return FALSE;
		buffer->bytes = grown;
		buffer->capacity = cap;
	}
	memcpy(buffer->bytes + buffer->size, bytes, size);
	buffer->size += size;
	return TRUE;
}

static void xbox_mark_schema_spans(
	struct xbox_runtime_closure const *cl,
	uint8_t *used_mask,
	struct tag_schema_definition const *def,
	uint32_t base_off,
	int32_t elem_count,
	int depth)
{
	int32_t elem_idx;
	struct tag_schema_field const *field;
	uint32_t elem_size;
	uint64_t span_bytes;

	if (!def || !def->fields || elem_count <= 0 || def->size <= 0 || depth > 12)
		return;
	elem_size = (uint32_t)def->size;
	span_bytes = (uint64_t)elem_count * elem_size;
	if ((uint64_t)base_off + span_bytes > cl->tag_sz)
		return;

	memset(used_mask + base_off, 1, (size_t)span_bytes);

	for (elem_idx = 0; elem_idx < elem_count; elem_idx++)
	{
		uint32_t elem_off = base_off + (uint32_t)elem_idx * elem_size;
		for (field = def->fields; field->type != _tag_schema_terminator; field++)
		{
			int16_t k;
			int16_t count = (field->count > 0) ? field->count : 1;
			uint32_t stride = (field->size > 0) ? (uint32_t)field->size : 0;

			for (k = 0; k < count; k++)
			{
				uint32_t f_off = elem_off + (uint32_t)field->offset + (uint32_t)k * stride;
				if (field->type == _tag_schema_struct)
				{
					struct tag_schema_definition const *sub_def =
						(struct tag_schema_definition const *)field->definition;
					if (sub_def && f_off + (uint32_t)sub_def->size <= cl->tag_sz)
						xbox_mark_schema_spans(cl, used_mask, sub_def, f_off, 1, depth + 1);
				}
				else if (field->type == _tag_schema_block)
				{
					struct tag_schema_definition const *child_def =
						(struct tag_schema_definition const *)field->definition;
					if (child_def && f_off + 12u <= cl->tag_sz)
					{
						int32_t child_cnt = *(int32_t const *)(cl->tag_buf + f_off + 0);
						uint32_t child_addr = *(uint32_t const *)(cl->tag_buf + f_off + 4);
						if (child_cnt > 0 && child_cnt <= 65536 &&
							child_addr >= cl->tag_base &&
							(uint64_t)(child_addr - cl->tag_base) + (uint64_t)child_cnt * (uint32_t)child_def->size <= cl->tag_sz)
						{
							xbox_mark_schema_spans(
								cl,
								used_mask,
								child_def,
								child_addr - cl->tag_base,
								child_cnt,
								depth + 1);
						}
					}
				}
				else if (field->type == _tag_schema_data)
				{
					if (f_off + 20u <= cl->tag_sz)
					{
						int32_t d_sz = *(int32_t const *)(cl->tag_buf + f_off + 0);
						uint32_t d_addr = *(uint32_t const *)(cl->tag_buf + f_off + 12);
						if (d_sz > 0 && d_sz <= 16 * 1024 * 1024 &&
							d_addr >= cl->tag_base &&
							(uint64_t)(d_addr - cl->tag_base) + (uint64_t)d_sz <= cl->tag_sz)
						{
							memset(used_mask + (d_addr - cl->tag_base), 1, (size_t)d_sz);
						}
					}
				}
				else if (field->type == _tag_schema_reference)
				{
					if (f_off + 16u <= cl->tag_sz)
					{
						uint32_t rname_addr = *(uint32_t const *)(cl->tag_buf + f_off + 4);
						int32_t rname_len = *(int32_t const *)(cl->tag_buf + f_off + 8);
						if (rname_len > 0 && rname_len < 256 &&
							rname_addr >= cl->tag_base &&
							(rname_addr - cl->tag_base) + (uint32_t)rname_len + 1u <= cl->tag_sz)
						{
							memset(used_mask + (rname_addr - cl->tag_base), 1, (size_t)rname_len + 1u);
						}
					}
				}
			}
		}
	}
}

struct xbox_vault_ext_patch
{
	uint32_t *offset_ptr;
	uint32_t rel_ext_off;
};

static int xbox_vault_append_ext_region(
	struct forge_grow_buf *ext_buf,
	struct tag_bundle_file_region **regions,
	long *region_count,
	long *region_cap,
	struct xbox_vault_ext_patch **patches,
	long *patch_count,
	long *patch_cap,
	void const *bytes,
	unsigned long size,
	uint32_t *offset_ptr)
{
	unsigned long rel_off;

	if (!ext_buf || !bytes || size == 0 || !offset_ptr)
		return 0;
	rel_off = ext_buf->size;
	if (*region_count >= *region_cap)
	{
		long new_cap = *region_cap ? (*region_cap * 2) : 64;
		struct tag_bundle_file_region *grown_r = (struct tag_bundle_file_region *)realloc(
			*regions, (size_t)new_cap * sizeof(*grown_r));
		if (!grown_r)
			return 0;
		*regions = grown_r;
		*region_cap = new_cap;
	}
	if (*patch_count >= *patch_cap)
	{
		long new_cap = *patch_cap ? (*patch_cap * 2) : 64;
		struct xbox_vault_ext_patch *grown_p = (struct xbox_vault_ext_patch *)realloc(
			*patches, (size_t)new_cap * sizeof(*grown_p));
		if (!grown_p)
			return 0;
		*patches = grown_p;
		*patch_cap = new_cap;
	}
	if (!forge_grow_buf_write(ext_buf, bytes, size))
		return 0;
	(*regions)[*region_count].offset = rel_off;
	(*regions)[*region_count].size = size;
	(*region_count)++;
	(*patches)[*patch_count].offset_ptr = offset_ptr;
	(*patches)[*patch_count].rel_ext_off = (uint32_t)rel_off;
	(*patch_count)++;
	return 1;
}

static int extract_xbox_map_tag_to_vault(
	int32_t root_abs_idx,
	uint32_t grp,
	char const *tag_path,
	char const *clean_name)
{
	struct xbox_runtime_closure cl;
	uint8_t *used_mask = NULL;
	uint8_t *masked_tag_buf = NULL;
	struct forge_grow_buf ext_buf = { NULL, 0, 0 };
	struct forge_grow_buf model_hw_buf = { NULL, 0, 0 };
	struct tag_bundle_file_region *regions = NULL;
	long region_count = 0, region_cap = 0;
	struct xbox_vault_ext_patch *patches = NULL;
	long patch_count = 0, patch_cap = 0;
	static struct tag_bundle_file_region const no_region = { 0, 0 };
	uint32_t final_tag_sz, uncomp_size, new_tag_off, tags_size;
	uint8_t *uncomp_img = NULL;
	long prof = 0, p_idx;
	int added = 0;
	int32_t i;
	size_t c_idx;

	if (!s_map_browser.xbox_fp || !s_map_browser.xbox_tag_buf ||
		root_abs_idx < 0 || root_abs_idx >= s_map_browser.xbox_tag_count)
		return 0;

	memset(&cl, 0, sizeof(cl));
	cl.tag_buf = s_map_browser.xbox_tag_buf;
	cl.tag_sz = s_map_browser.xbox_tag_sz;
	cl.tag_base = s_map_browser.xbox_tag_base;
	cl.inst_off = s_map_browser.xbox_inst_off;
	cl.tag_count = s_map_browser.xbox_tag_count;
	cl.include_all_dependencies = 1;
	cl.visited = (uint8_t *)calloc((size_t)cl.tag_count, 1);
	cl.order = (int32_t *)calloc((size_t)cl.tag_count, sizeof(int32_t));
	cl.runtime_handles = (long *)malloc((size_t)cl.tag_count * sizeof(long));
	used_mask = (uint8_t *)calloc(1, cl.tag_sz);
	masked_tag_buf = (uint8_t *)calloc(1, cl.tag_sz);
	if (!cl.visited || !cl.order || !cl.runtime_handles || !used_mask || !masked_tag_buf)
	{
		free(cl.visited);
		free(cl.order);
		free(cl.runtime_handles);
		free(used_mask);
		free(masked_tag_buf);
		return 0;
	}
	for (i = 0; i < cl.tag_count; i++)
		cl.runtime_handles[i] = NONE;

	/* Visit ALL dependencies of root_abs_idx */
	xbox_visit_tag_index(&cl, root_abs_idx);

	/* Mark the tag table header (0x24 or 0x28 bytes) */
	memset(used_mask, 1, s_map_browser.is_mcc_map ? 0x28u : 0x24u);

	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = cl.inst_off + (uint32_t)abs_idx * 32u;
		uint32_t t_grp = *(uint32_t const *)(cl.tag_buf + io + 0x00);
		uint32_t name_addr = *(uint32_t const *)(cl.tag_buf + io + 0x10);
		uint32_t data_addr = *(uint32_t const *)(cl.tag_buf + io + 0x14);
		struct tag_schema_group const *schema = forge_runtime_schema_group_get(t_grp);

		memset(used_mask + io, 1, 32u);
		if (name_addr >= cl.tag_base && (name_addr - cl.tag_base) < cl.tag_sz)
		{
			char const *tn = (char const *)(cl.tag_buf + (name_addr - cl.tag_base));
			size_t tn_len = strlen(tn) + 1u;
			if ((name_addr - cl.tag_base) + tn_len <= cl.tag_sz)
				memset(used_mask + (name_addr - cl.tag_base), 1, tn_len);
		}
		if (schema && schema->definition && data_addr >= cl.tag_base &&
			(data_addr - cl.tag_base) + (uint32_t)schema->definition->size <= cl.tag_sz)
		{
			xbox_mark_schema_spans(&cl, used_mask, schema->definition, data_addr - cl.tag_base, 1, 1);
		}
		if (t_grp == 'mode' && data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0xE8u <= cl.tag_sz)
		{
			uint32_t m_off = data_addr - cl.tag_base;
			int32_t g_cnt = *(int32_t const *)(cl.tag_buf + m_off + 0xD0u);
			uint32_t g_addr = *(uint32_t const *)(cl.tag_buf + m_off + 0xD4u);
			int g_i;
			if (g_cnt > 0 && g_cnt <= 64 && g_addr >= cl.tag_base &&
				(g_addr - cl.tag_base) + (uint32_t)g_cnt * 48u <= cl.tag_sz)
			{
				for (g_i = 0; g_i < g_cnt; g_i++)
				{
					uint32_t go = (g_addr - cl.tag_base) + (uint32_t)g_i * 48u;
					int32_t p_cnt = *(int32_t const *)(cl.tag_buf + go + 36u);
					uint32_t p_addr = *(uint32_t const *)(cl.tag_buf + go + 40u);
					int p_i;
					if (p_cnt > 0 && p_cnt <= 128 && p_addr >= cl.tag_base &&
						(p_addr - cl.tag_base) + (uint32_t)p_cnt * 104u <= cl.tag_sz)
					{
						for (p_i = 0; p_i < p_cnt; p_i++)
						{
							uint32_t po = (p_addr - cl.tag_base) + (uint32_t)p_i * 104u;
							int16_t tri_type = *(int16_t const *)(cl.tag_buf + po + 68u);
							int32_t tri_cnt = *(int32_t const *)(cl.tag_buf + po + 72u);
							uint32_t tb_hw = *(uint32_t const *)(cl.tag_buf + po + 80u);
							int32_t vtx_cnt = *(int32_t const *)(cl.tag_buf + po + 88u);
							uint32_t vb_hw = *(uint32_t const *)(cl.tag_buf + po + 100u);
							if (tri_cnt > 0 && vtx_cnt > 0 &&
								tb_hw >= cl.tag_base && (tb_hw - cl.tag_base) + 12u <= cl.tag_sz &&
								vb_hw >= cl.tag_base && (vb_hw - cl.tag_base) + 12u <= cl.tag_sz)
							{
								uint32_t ib_addr = *(uint32_t const *)(cl.tag_buf + (tb_hw - cl.tag_base) + 4u);
								uint32_t vb_addr = *(uint32_t const *)(cl.tag_buf + (vb_hw - cl.tag_base) + 4u);
								uint32_t comp_sz = (uint32_t)vtx_cnt * 32u;
								uint32_t idx_sz = (tri_type == _triangle_buffer_type_precompiled_strip)
									? ((uint32_t)tri_cnt + 2u) * 2u
									: (uint32_t)tri_cnt * 6u;
								memset(used_mask + (tb_hw - cl.tag_base), 1, 12u);
								memset(used_mask + (vb_hw - cl.tag_base), 1, 12u);
								if (ib_addr >= cl.tag_base && (ib_addr - cl.tag_base) + idx_sz <= cl.tag_sz)
									memset(used_mask + (ib_addr - cl.tag_base), 1, idx_sz);
								if (vb_addr >= cl.tag_base && (vb_addr - cl.tag_base) + comp_sz <= cl.tag_sz)
									memset(used_mask + (vb_addr - cl.tag_base), 1, comp_sz);
							}
						}
					}
				}
			}
		}
	}

	/* Copy only marked bytes into masked_tag_buf (all other bytes remain 0 for maximum zlib compression!) */
	for (c_idx = 0; c_idx < cl.tag_sz; c_idx++)
	{
		if (used_mask[c_idx])
			masked_tag_buf[c_idx] = cl.tag_buf[c_idx];
	}

	/* Pack external 'bitm' pixels and 'snd!' samples contiguously starting at file offset 2048,
	 * and convert MCC 'mod2'/'scex'/'weap' tags into native Xbox v5 layout when extracting from MCC v13 maps. */
	for (i = 0; i < cl.order_count; i++)
	{
		int32_t abs_idx = cl.order[i];
		uint32_t io = cl.inst_off + (uint32_t)abs_idx * 32u;
		uint32_t t_grp = *(uint32_t const *)(masked_tag_buf + io + 0x00);
		uint32_t name_addr = *(uint32_t const *)(masked_tag_buf + io + 0x10);
		uint32_t data_addr = *(uint32_t const *)(masked_tag_buf + io + 0x14);
		char const *t_name = (name_addr >= cl.tag_base && (name_addr - cl.tag_base) < cl.tag_sz)
			? (char const *)(masked_tag_buf + (name_addr - cl.tag_base))
			: "";

		if (t_grp == 'bitm' && data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0x6Cu <= cl.tag_sz)
		{
			uint32_t b_off = data_addr - cl.tag_base;
			int32_t b_cnt = *(int32_t const *)(masked_tag_buf + b_off + 0x60u);
			uint32_t b_addr = *(uint32_t const *)(masked_tag_buf + b_off + 0x64u);
			int b_i;
			unsigned chan_mode = 1u;

			if (s_map_browser.is_mcc_map)
			{
				int32_t s_k;
				for (s_k = 0; s_k < cl.order_count; s_k++)
				{
					int32_t s_abs = cl.order[s_k];
					uint32_t s_io = cl.inst_off + (uint32_t)s_abs * 32u;
					uint32_t s_grp = *(uint32_t const *)(masked_tag_buf + s_io + 0x00);
					uint32_t s_data = *(uint32_t const *)(masked_tag_buf + s_io + 0x14);
					if (s_grp == 'soso' && s_data >= cl.tag_base && (s_data - cl.tag_base) + 0xDCu <= cl.tag_sz)
					{
						uint16_t s_flags = *(uint16_t const *)(masked_tag_buf + (s_data - cl.tag_base) + 0x28u);
						int32_t multi_ref = *(int32_t const *)(masked_tag_buf + (s_data - cl.tag_base) + 0xC8u + 12u);
						if (!(s_flags & 0x40u) && multi_ref != -1 && (multi_ref & 0xFFFF) == abs_idx)
						{
							chan_mode = 2u;
							break;
						}
					}
				}
			}

			if (b_cnt > 0 && b_cnt <= 256 && b_addr >= cl.tag_base &&
				(b_addr - cl.tag_base) + (uint32_t)b_cnt * 48u <= cl.tag_sz)
			{
				for (b_i = 0; b_i < b_cnt; b_i++)
				{
					uint8_t *bmp_raw = masked_tag_buf + (b_addr - cl.tag_base) + (uint32_t)b_i * 48u;
					struct bitmap_data tmp_bmp;

					memcpy(&tmp_bmp, bmp_raw, sizeof(tmp_bmp));
					if (s_map_browser.is_mcc_map && s_map_browser.mcc_rt)
					{
						unsigned char *conv_pix = NULL;
						uint32_t conv_sz = 0;
						if (mcc_forge_convert_single_bitmap(
								s_map_browser.mcc_rt,
								t_name,
								(uint32_t)b_i,
								chan_mode,
								&tmp_bmp,
								&conv_pix,
								&conv_sz) && conv_pix && conv_sz > 0)
						{
							memcpy(bmp_raw, &tmp_bmp, sizeof(tmp_bmp));
							xbox_vault_append_ext_region(
								&ext_buf,
								&regions,
								&region_count,
								&region_cap,
								&patches,
								&patch_count,
								&patch_cap,
								conv_pix,
								conv_sz,
								(uint32_t *)(bmp_raw + 24u));
						}
						free(conv_pix);
					}
					else
					{
						long req_bytes;
						uint32_t orig_off = (uint32_t)tmp_bmp.pixels_offset;
						uint32_t p_sz = (tmp_bmp.pixels_size > 0) ? (uint32_t)tmp_bmp.pixels_size : 0u;
						uint32_t read_bytes;
						uint8_t *pix_tmp;

						if (tmp_bmp.width <= 0 || tmp_bmp.height <= 0 || tmp_bmp.depth <= 0 || orig_off < 2048u)
							continue;
						req_bytes = rasterizer_xbox_bitmap_get_pixel_data_size(&tmp_bmp);
						if (req_bytes <= 0)
							continue;
						read_bytes = (p_sz > 0 && p_sz < (uint32_t)req_bytes) ? p_sz : (uint32_t)req_bytes;
						pix_tmp = (uint8_t *)malloc(read_bytes);
						if (pix_tmp &&
							fseek(s_map_browser.xbox_fp, (long)orig_off, SEEK_SET) == 0 &&
							fread(pix_tmp, 1, read_bytes, s_map_browser.xbox_fp) == read_bytes)
						{
							xbox_vault_append_ext_region(
								&ext_buf,
								&regions,
								&region_count,
								&region_cap,
								&patches,
								&patch_count,
								&patch_cap,
								pix_tmp,
								read_bytes,
								(uint32_t *)(bmp_raw + 24u));
						}
						free(pix_tmp);
					}
				}
			}
		}
		else if (t_grp == 'snd!' && data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0xA4u <= cl.tag_sz)
		{
			uint32_t s_off = data_addr - cl.tag_base;
			struct sound_definition *sdef_raw = (struct sound_definition *)(masked_tag_buf + s_off);
			int32_t pr_cnt = *(int32_t const *)(masked_tag_buf + s_off + 0x98u);
			uint32_t pr_addr = *(uint32_t const *)(masked_tag_buf + s_off + 0x9Cu);
			int s_channels = (sdef_raw->encoding == 1) ? 2 : 1;
			uint32_t s_rate = (sdef_raw->sample_rate == 1) ? 44100u : 22050u;
			uint32_t s_out_rate = (s_channels == 1) ? 22050u : s_rate;
			int pr_i, pm_i;

			if (s_map_browser.is_mcc_map)
			{
				sdef_raw->compression = 1; /* _sound_compression_xbox_adpcm */
				sdef_raw->sample_rate = (s_out_rate == 44100u) ? 1 : 0;
			}

			if (pr_cnt > 0 && pr_cnt <= 64 && pr_addr >= cl.tag_base &&
				(pr_addr - cl.tag_base) + (uint32_t)pr_cnt * 72u <= cl.tag_sz)
			{
				for (pr_i = 0; pr_i < pr_cnt; pr_i++)
				{
					uint32_t pro = (pr_addr - cl.tag_base) + (uint32_t)pr_i * 72u;
					int32_t pm_cnt = *(int32_t const *)(masked_tag_buf + pro + 60u);
					uint32_t pm_addr = *(uint32_t const *)(masked_tag_buf + pro + 64u);
					if (pm_cnt > 0 && pm_cnt <= 256 && pm_addr >= cl.tag_base &&
						(pm_addr - cl.tag_base) + (uint32_t)pm_cnt * 124u <= cl.tag_sz)
					{
						for (pm_i = 0; pm_i < pm_cnt; pm_i++)
						{
							uint8_t *perm_raw = masked_tag_buf + (pm_addr - cl.tag_base) + (uint32_t)pm_i * 124u;
							if (s_map_browser.is_mcc_map && s_map_browser.mcc_rt)
							{
								struct sound_permutation tmp_perm;
								unsigned char *conv_snd = NULL;
								uint32_t conv_sz = 0;
								memcpy(&tmp_perm, perm_raw, sizeof(tmp_perm));
								if (mcc_forge_convert_single_permutation(
										s_map_browser.mcc_rt,
										t_name,
										s_channels,
										s_rate,
										s_out_rate,
										(uint32_t)pr_i,
										(uint32_t)pm_i,
										&tmp_perm,
										&conv_snd,
										&conv_sz) && conv_snd && conv_sz > 0)
								{
									memcpy(perm_raw, &tmp_perm, sizeof(tmp_perm));
									xbox_vault_append_ext_region(
										&ext_buf,
										&regions,
										&region_count,
										&region_cap,
										&patches,
										&patch_count,
										&patch_cap,
										conv_snd,
										conv_sz,
										(uint32_t *)(perm_raw + 72u));
								}
								free(conv_snd);
							}
							else
							{
								int32_t s_sz = *(int32_t const *)(perm_raw + 64u);
								uint32_t s_foff = *(uint32_t const *)(perm_raw + 72u);
								if (s_sz > 0 && s_sz <= MAXIMUM_SOUND_DATA_SIZE && s_foff >= 2048u)
								{
									uint8_t *stmp = (uint8_t *)malloc((size_t)s_sz);
									if (stmp &&
										fseek(s_map_browser.xbox_fp, (long)s_foff, SEEK_SET) == 0 &&
										fread(stmp, 1, (size_t)s_sz, s_map_browser.xbox_fp) == (size_t)s_sz)
									{
										xbox_vault_append_ext_region(
											&ext_buf,
											&regions,
											&region_count,
											&region_cap,
											&patches,
											&patch_count,
											&patch_cap,
											stmp,
											(unsigned long)s_sz,
											(uint32_t *)(perm_raw + 72u));
									}
									free(stmp);
								}
							}
						}
					}
				}
			}
		}
		else if (s_map_browser.is_mcc_map && t_grp == 'mod2' &&
				 data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0xE8u <= cl.tag_sz)
		{
			uint32_t model_file_off = *(uint32_t const *)(cl.tag_buf + 0x14u);
			uint32_t model_vtx_bytes = *(uint32_t const *)(cl.tag_buf + 0x1Cu);
			uint32_t m_off = data_addr - cl.tag_base;
			uint32_t m_flags = *(uint32_t const *)(masked_tag_buf + m_off);
			int local_nodes = (m_flags & 2u) != 0;
			int32_t node_cnt = *(int32_t const *)(masked_tag_buf + m_off + 0xB8u);
			int32_t g_cnt = *(int32_t const *)(masked_tag_buf + m_off + 0xD0u);
			uint32_t g_addr = *(uint32_t const *)(masked_tag_buf + m_off + 0xD4u);
			int32_t shdr_cnt = *(int32_t const *)(masked_tag_buf + m_off + 0xDCu);
			int g_i;

			*(uint32_t *)(masked_tag_buf + io + 0x00u) = 'mode';
			*(uint32_t *)(masked_tag_buf + m_off) = m_flags & ~2u;

			if (g_cnt > 0 && g_cnt <= 256 && g_addr >= cl.tag_base &&
				(g_addr - cl.tag_base) + (uint32_t)g_cnt * 48u <= cl.tag_sz)
			{
				for (g_i = 0; g_i < g_cnt; g_i++)
				{
					uint32_t go = (g_addr - cl.tag_base) + (uint32_t)g_i * 48u;
					int32_t p_cnt = *(int32_t const *)(masked_tag_buf + go + 36u);
					uint32_t p_addr = *(uint32_t const *)(masked_tag_buf + go + 40u);
					int p_i;
					if (p_cnt <= 0 || p_cnt > 128 || p_addr < cl.tag_base ||
						(p_addr - cl.tag_base) + (uint32_t)p_cnt * 132u > cl.tag_sz)
						continue;

					for (p_i = 0; p_i < p_cnt; p_i++)
					{
						uint8_t const *wire = cl.tag_buf + (p_addr - cl.tag_base) + (uint32_t)p_i * 132u;
						uint8_t *dst_part = masked_tag_buf + (p_addr - cl.tag_base) + (uint32_t)p_i * 104u;
						uint32_t vtx_cnt = *(uint32_t const *)(wire + 0x58u);
						uint32_t tri_cnt = *(uint32_t const *)(wire + 0x48u);
						uint32_t vtx_off = *(uint32_t const *)(wire + 0x64u);
						uint32_t idx_off = *(uint32_t const *)(wire + 0x4Cu);
						uint8_t pal_cnt = wire[0x6Bu];
						uint8_t const *pal = wire + 0x6Cu;
						uint32_t raw_vtx_sz = vtx_cnt * 68u;
						uint32_t comp_vtx_sz = vtx_cnt * 32u;
						uint32_t idx_sz = (tri_cnt + 2u) * 2u;
						struct model_vertex_uncompressed *verts68 = NULL;
						uint8_t *comp32 = NULL;
						uint16_t *indices = NULL;
						uint8_t part_prefix[0x20];

						memcpy(part_prefix, wire, 0x20u);
						memset(dst_part, 0, 104u);
						memcpy(dst_part, part_prefix, 0x20u);
						*(uint32_t *)dst_part &= ~2u;
						if (*(uint16_t const *)(dst_part + 4u) != 0xFFFFu &&
							shdr_cnt > 0 && *(uint16_t const *)(dst_part + 4u) >= (uint16_t)shdr_cnt)
						{
							*(uint16_t *)(dst_part + 4u) = 0;
						}
						if (local_nodes && pal_cnt > 0 && pal_cnt <= 22)
						{
							int c_n;
							for (c_n = 0; c_n < 2; c_n++)
							{
								int16_t cn = *(int16_t const *)(dst_part + 8u + c_n * 2);
								if (cn >= 0 && cn < (int16_t)pal_cnt)
									*(int16_t *)(dst_part + 8u + c_n * 2) = (int16_t)pal[cn];
							}
						}

						if (vtx_cnt == 0 || vtx_cnt > 65535 || tri_cnt == 0 || tri_cnt > 65535)
							continue;

						verts68 = (struct model_vertex_uncompressed *)malloc(raw_vtx_sz);
						comp32 = (uint8_t *)calloc(1, comp_vtx_sz);
						indices = (uint16_t *)malloc(idx_sz);
						if (verts68 && comp32 && indices &&
							fseek(s_map_browser.xbox_fp, (long)(model_file_off + vtx_off), SEEK_SET) == 0 &&
							fread(verts68, 1, raw_vtx_sz, s_map_browser.xbox_fp) == raw_vtx_sz &&
							fseek(s_map_browser.xbox_fp, (long)(model_file_off + model_vtx_bytes + idx_off), SEEK_SET) == 0 &&
							fread(indices, 1, idx_sz, s_map_browser.xbox_fp) == idx_sz)
						{
							uint32_t v_i;
							uint32_t tb_hw_vaddr, ib_data_vaddr, vb_hw_vaddr, vb_data_vaddr;
							uint32_t hw_hdr[3];
							uint8_t pad_zero[4] = { 0, 0, 0, 0 };

							for (v_i = 0; v_i < vtx_cnt; v_i++)
							{
								float *fv = (float *)&verts68[v_i];
								int f_j;
								for (f_j = 0; f_j < 14; f_j++)
								{
									if (!forge_float_is_finite(fv[f_j]))
										fv[f_j] = (f_j == 3 || f_j == 7 || f_j == 11) ? 1.0f : 0.0f;
									else if (f_j >= 3 && f_j < 12)
									{
										if (fv[f_j] > 1.0f) fv[f_j] = 1.0f;
										else if (fv[f_j] < -1.0f) fv[f_j] = -1.0f;
									}
								}
								for (f_j = 0; f_j < 2; f_j++)
								{
									int16_t nd = verts68[v_i].nodes[f_j];
									if (local_nodes && pal_cnt > 0 && pal_cnt <= 22)
										nd = (nd >= 0 && nd < (int16_t)pal_cnt) ? (int16_t)pal[nd] : 0;
									if (nd < 0 || (node_cnt > 0 && nd >= (int16_t)node_cnt))
										nd = 0;
									verts68[v_i].nodes[f_j] = nd;
									if (!forge_float_is_finite(verts68[v_i].node_weights[f_j]))
										verts68[v_i].node_weights[f_j] = (f_j == 0) ? 1.0f : 0.0f;
								}
							}

							rasterizer_geometry_compress_vertices(
								_rasterizer_vertex_type_model_uncompressed,
								(long)vtx_cnt,
								comp32,
								(long)comp_vtx_sz,
								verts68,
								(long)raw_vtx_sz);

							if (model_hw_buf.size & 3u)
								forge_grow_buf_write(&model_hw_buf, pad_zero, 4u - (model_hw_buf.size & 3u));
							tb_hw_vaddr = cl.tag_base + cl.tag_sz + (uint32_t)model_hw_buf.size;
							ib_data_vaddr = tb_hw_vaddr + 12u;
							hw_hdr[0] = 0;
							hw_hdr[1] = ib_data_vaddr;
							hw_hdr[2] = 0;
							forge_grow_buf_write(&model_hw_buf, hw_hdr, 12u);
							forge_grow_buf_write(&model_hw_buf, indices, idx_sz);

							if (model_hw_buf.size & 3u)
								forge_grow_buf_write(&model_hw_buf, pad_zero, 4u - (model_hw_buf.size & 3u));
							vb_hw_vaddr = cl.tag_base + cl.tag_sz + (uint32_t)model_hw_buf.size;
							vb_data_vaddr = vb_hw_vaddr + 12u;
							hw_hdr[0] = 0;
							hw_hdr[1] = vb_data_vaddr;
							hw_hdr[2] = 0;
							forge_grow_buf_write(&model_hw_buf, hw_hdr, 12u);
							forge_grow_buf_write(&model_hw_buf, comp32, comp_vtx_sz);

							*(int16_t *)(dst_part + 68u) = _triangle_buffer_type_precompiled_strip;
							*(int32_t *)(dst_part + 72u) = (int32_t)tri_cnt;
							*(uint32_t *)(dst_part + 80u) = tb_hw_vaddr;
							*(int16_t *)(dst_part + 84u) = _rasterizer_vertex_type_model_compressed;
							*(int32_t *)(dst_part + 88u) = (int32_t)vtx_cnt;
							*(uint32_t *)(dst_part + 100u) = vb_hw_vaddr;
						}
						free(verts68);
						free(comp32);
						free(indices);
					}
					if ((uint32_t)p_cnt * 132u > (uint32_t)p_cnt * 104u)
					{
						memset(
							masked_tag_buf + (p_addr - cl.tag_base) + (uint32_t)p_cnt * 104u,
							0,
							(size_t)p_cnt * (132u - 104u));
					}
				}
			}
		}
		else if (s_map_browser.is_mcc_map && t_grp == 'scex' &&
				 data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0x78u <= cl.tag_sz)
		{
			uint8_t *p = masked_tag_buf + (data_addr - cl.tag_base);
			uint32_t flags = *(uint32_t const *)(p + 0x6Cu);
			if (!*(uint32_t const *)(p + 0x54u))
				memcpy(p + 0x54u, p + 0x60u, 12u);
			memset(p + 0x60u, 0, 12u);
			memcpy(p + 0x60u, &flags, 4u);
			*(uint32_t *)(masked_tag_buf + io + 0x00u) = 'schi';
			*(uint32_t *)(masked_tag_buf + io + 0x04u) = 'shdr';
		}
		else if (s_map_browser.is_mcc_map && t_grp == 'weap' &&
				 data_addr >= cl.tag_base && (data_addr - cl.tag_base) + 0x508u <= cl.tag_sz)
		{
			uint8_t *p = masked_tag_buf + (data_addr - cl.tag_base);
			unsigned w_j;
			for (w_j = 0; w_j < 4u; w_j++)
			{
				uint16_t val = *(uint16_t const *)(p + 0x330u + w_j * 2u);
				if (val == 17u || val == 18u)
					*(uint16_t *)(p + 0x330u + w_j * 2u) = (uint16_t)(val - 2u);
			}
		}
	}

	if (s_map_browser.is_mcc_map)
	{
		size_t scan_off;
		for (scan_off = 0x28u + (size_t)cl.tag_count * 32u; scan_off + 16u <= cl.tag_sz; scan_off += 4u)
		{
			if (!used_mask[scan_off])
				continue;
			if (*(uint32_t const *)(masked_tag_buf + scan_off) == 'mod2')
				*(uint32_t *)(masked_tag_buf + scan_off) = 'mode';
			else if (*(uint32_t const *)(masked_tag_buf + scan_off) == 'scex')
				*(uint32_t *)(masked_tag_buf + scan_off) = 'schi';
		}
		memmove(masked_tag_buf + 0x24u, masked_tag_buf + 0x28u, (size_t)cl.tag_count * 32u);
		memset(masked_tag_buf + 0x24u + (size_t)cl.tag_count * 32u, 0, 4u);
		*(uint32_t *)(masked_tag_buf + 0x00u) = cl.tag_base + 0x24u;
		*(uint32_t *)(masked_tag_buf + 0x20u) = 0x74616773u; /* 'tags' */
	}

	final_tag_sz = cl.tag_sz + (uint32_t)model_hw_buf.size;
	new_tag_off = 2048u;
	tags_size = 2048u + ((final_tag_sz + 31u) & ~31u);
	for (p_idx = 0; p_idx < patch_count; p_idx++)
		*patches[p_idx].offset_ptr = tags_size + patches[p_idx].rel_ext_off;
	free(patches);
	patches = NULL;

	uncomp_size = tags_size + (uint32_t)ext_buf.size;
	uncomp_img = (uint8_t *)calloc(1, uncomp_size);
	if (!uncomp_img)
	{
		free(regions);
		free(ext_buf.bytes);
		free(model_hw_buf.bytes);
		free(cl.visited);
		free(cl.order);
		free(cl.runtime_handles);
		free(used_mask);
		free(masked_tag_buf);
		return 0;
	}

	*(uint32_t *)(uncomp_img + 0x00) = 0x68656164u; /* 'head' */
	*(uint32_t *)(uncomp_img + 0x04) = 5u;          /* Xbox v5 */
	*(uint32_t *)(uncomp_img + 0x08) = uncomp_size;
	*(uint32_t *)(uncomp_img + 0x10) = new_tag_off;
	*(uint32_t *)(uncomp_img + 0x14) = final_tag_sz;
	*(uint32_t *)(uncomp_img + 0x7FC) = 0x666F6F74u; /* 'foot' */
	memcpy(uncomp_img + new_tag_off, masked_tag_buf, cl.tag_sz);
	if (model_hw_buf.bytes && model_hw_buf.size > 0)
		memcpy(uncomp_img + new_tag_off + cl.tag_sz, model_hw_buf.bytes, model_hw_buf.size);
	if (ext_buf.bytes && ext_buf.size > 0)
		memcpy(uncomp_img + tags_size, ext_buf.bytes, ext_buf.size);

	free(ext_buf.bytes);
	free(model_hw_buf.bytes);
	free(cl.visited);
	free(cl.order);
	free(cl.runtime_handles);
	free(used_mask);
	free(masked_tag_buf);

	port_tag_profiles_scan();
	if (port_tag_profiles_count() <= 0)
		prof = port_tag_profiles_create("Forge Custom Tags");
	else
		prof = 0;

	if (prof >= 0)
	{
		added = port_tag_profiles_tag_add(
			prof,
			s_map_browser.map_name[0] ? s_map_browser.map_name : "xbox_map",
			(unsigned long)grp,
			tag_path,
			uncomp_img,
			uncomp_size,
			tags_size,
			(struct port_tag_store_region const *)(regions ? regions : &no_region),
			region_count);
		if (added)
		{
			long t_slot = port_tag_profiles_tag_find(prof, (unsigned long)grp, tag_path);
			if (t_slot >= 0)
			{
				port_tag_profiles_tag_set_enabled(prof, t_slot, 1);
				port_tag_profiles_tag_set_option(prof, t_slot, "name", clean_name);
			}
		}
	}

	free(regions);
	free(uncomp_img);
	return added;
}

int forge_ui_extract_selected_map_tag(int category_tab, int list_index, char const *custom_name)
{
	int32_t abs_idx;
	uint32_t grp;
	char tag_path[256];
	char clean_name[64];
	char const *name_src;
	char const *leaf;
	char const *p;

	if (!s_map_browser.is_open || category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS ||
		list_index < 0 || list_index >= s_map_browser.cat_counts[category_tab])
		return 0;

	abs_idx = s_map_browser.cat_tags[category_tab][list_index];
	grp = browser_tag_group_at(abs_idx);
	snprintf(tag_path, sizeof(tag_path), "%s", browser_tag_name_at(abs_idx));
	if (!tag_path[0])
		return 0;

	leaf = tag_path;
	for (p = tag_path; *p; p++)
	{
		if (*p == '\\' || *p == '/')
			leaf = p + 1;
	}
	name_src = (custom_name && custom_name[0]) ? custom_name : leaf;
	while (*name_src == ' ')
		name_src++;
	if (_strnicmp(name_src, "[c]", 3) == 0)
	{
		name_src += 3;
		while (*name_src == ' ')
			name_src++;
	}
	snprintf(clean_name, sizeof(clean_name), "%s", name_src[0] ? name_src : leaf);

	if (s_map_browser.is_ce_map && s_map_browser.ce_header)
	{
		long prof = 0;
		struct tag_bundle_source source;
		struct tag_bundle_report report;
		struct forge_grow_buf buffer = { NULL, 0, 0 };
		long *indices = NULL;
		long count = 0;
		long problem = NONE;
		long tag_count = s_map_browser.ce_header->tag_count;
		byte *sounds = NULL;
		long t_i;
		struct browser_ce_map_header *reopened;
		unsigned long tags_size;
		byte *copy_tags = NULL;
		struct tag_bundle_loaded loaded;
		struct tag_bundle_file_region *regions = NULL;
		long region_count = 0;
		static struct tag_bundle_file_region const no_region = { 0, 0 };
		int added = 0;

		port_tag_profiles_scan();
		if (port_tag_profiles_count() <= 0)
			prof = port_tag_profiles_create("Forge Custom Tags");
		else
			prof = 0;
		if (prof < 0)
			return 0;

		custom_edition_cache_bundle_source(&source);
		if (tag_bundle_tags(&source, abs_idx, &indices, &count, &problem) != _tag_bundle_status_ok || !indices)
		{
			free(indices);
			return 0;
		}
		sounds = (byte *)calloc((size_t)tag_count, 1);
		if (!sounds)
		{
			free(indices);
			return 0;
		}
		for (t_i = 0; t_i < count; t_i++)
		{
			if ((uint32_t)s_map_browser.ce_header->instances[indices[t_i]].group_tag == 'snd!')
				sounds[indices[t_i]] = TRUE;
		}
		free(indices);

		reopened = (struct browser_ce_map_header *)custom_edition_cache_bundle_open(
			s_map_browser.map_path, sounds, tag_count);
		free(sounds);
		if (!reopened)
			return 0;
		s_map_browser.ce_header = reopened;
		if (s_map_browser.names_made)
			free(s_map_browser.names_made);
		s_map_browser.names_made = NULL;
		tag_names_restore(reopened, s_map_browser.map_name, NULL);

		custom_edition_cache_bundle_source(&source);
		memset(&report, 0, sizeof(report));
		if (tag_bundle_write(&source, abs_idx, forge_grow_buf_write, &buffer, &report) != _tag_bundle_status_ok ||
			!buffer.bytes || buffer.size < TAG_BUNDLE_HEADER_BYTES)
		{
			free(buffer.bytes);
			return 0;
		}

		tags_size = tag_bundle_file_data_offset(buffer.bytes);
		if (tags_size >= TAG_BUNDLE_HEADER_BYTES && tags_size <= buffer.size)
		{
			copy_tags = (byte *)malloc(tags_size);
			if (copy_tags)
			{
				memcpy(copy_tags, buffer.bytes, tags_size);
				if (tag_bundle_load_tags_regions(
						copy_tags,
						tags_size,
						buffer.size,
						&loaded,
						&regions,
						&region_count) == _tag_bundle_status_ok)
				{
					added = port_tag_profiles_tag_add(
						prof,
						s_map_browser.map_name,
						(unsigned long)grp,
						tag_path,
						buffer.bytes,
						buffer.size,
						tags_size,
						(struct port_tag_store_region const *)(regions ? regions : &no_region),
						region_count);
				}
				free(copy_tags);
				free(regions);
			}
		}
		free(buffer.bytes);

		if (!added)
			return 0;

		{
			long t_slot = port_tag_profiles_tag_find(prof, (unsigned long)grp, tag_path);
			if (t_slot >= 0)
			{
				port_tag_profiles_tag_set_enabled(prof, t_slot, 1);
				port_tag_profiles_tag_set_option(prof, t_slot, "name", clean_name);
			}
		}
		forge_runtime_reload_catalogs();
		return 1;
	}
	else if (s_map_browser.is_xbox_map)
	{
		if (!extract_xbox_map_tag_to_vault(abs_idx, grp, tag_path, clean_name))
			return 0;
		forge_runtime_reload_catalogs();
		return 1;
	}

	return 0;
}

static boolean active_ce_map_bundle_read_cb(
	void *context,
	unsigned long offset,
	unsigned long size,
	void *buffer)
{
	(void)context;
	custom_edition_cache_read(NONE, (long)offset, (long)size, buffer);
	return TRUE;
}

int forge_is_tag_map_local_only(uint32_t group_tag, char const *tag_path)
{
	int i;

	if (!tag_path || !tag_path[0] ||
		group_tag == 'spwn' || group_tag == 'tele' || group_tag == 'flag')
	{
		return 0;
	}

	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (g_forge.catalog[i].group_tag == group_tag &&
			(_stricmp(g_forge.catalog[i].tag_path, tag_path) == 0 ||
			 (group_tag == 'vehi' && is_equivalent_stock_vehicle_path(g_forge.catalog[i].tag_path, tag_path))))
		{
			if (g_forge.catalog[i].source_cache_id == 2 || g_forge.catalog[i].source_cache_id == 4)
				return 0;
			if (g_forge.catalog[i].source_cache_id == 3)
				return 1;
			if (forge_catalog_entry_allowed(&g_forge.catalog[i]))
				return 0;
			return 1;
		}
	}

	return (tag_loaded((long)group_tag, tag_path) != NONE) ? 1 : 0;
}

int forge_cache_active_map_tag(uint32_t group_tag, char const *tag_path)
{
	char clean_name[64];
	char const *leaf;
	char const *p;

	if (!tag_path || !tag_path[0] ||
		group_tag == 'spwn' || group_tag == 'tele' || group_tag == 'flag')
	{
		return 0;
	}
	if (!forge_is_tag_map_local_only(group_tag, tag_path))
		return 1;

	leaf = tag_path;
	for (p = tag_path; *p; p++)
	{
		if (*p == '\\' || *p == '/')
			leaf = p + 1;
	}
	snprintf(clean_name, sizeof(clean_name), "%s", leaf[0] ? leaf : "custom_tag");

	/* Case 1: Active Custom Edition (v609) map loaded in 0x40440000 */
	if (custom_edition_cache_tags_loaded())
	{
		char const *ce_path = custom_edition_cache_loaded_path();
		char host_ce_path[512];
		struct browser_ce_map_header *ce_hdr = (struct browser_ce_map_header *)0x40440000UL;
		FILE *map_fp = NULL;
		uint8_t map_hdr[2048];
		uint32_t tag_data_off, tag_data_sz;
		char map_name[36], map_build[36];
		uint8_t *raw_tag_data = NULL;
		uint8_t *backup_tag_data = NULL;
		long abs_idx = -1, ti;
		long prof = 0;
		struct tag_bundle_source source;
		struct tag_bundle_report report;
		struct forge_grow_buf buffer = { NULL, 0, 0 };
		unsigned long tags_size;
		byte *copy_tags = NULL;
		struct tag_bundle_loaded loaded;
		struct tag_bundle_file_region *regions = NULL;
		long region_count = 0;
		static struct tag_bundle_file_region const no_region = { 0, 0 };
		int added = 0;

		if (!ce_path || !ce_path[0] || !ce_hdr || ce_hdr->tag_count <= 0 || !ce_hdr->instances)
			return 0;

		for (ti = 0; ti < ce_hdr->tag_count; ti++)
		{
			if ((uint32_t)ce_hdr->instances[ti].group_tag == group_tag &&
				ce_hdr->instances[ti].name &&
				_stricmp(ce_hdr->instances[ti].name, tag_path) == 0)
			{
				abs_idx = ti;
				break;
			}
		}
		if (abs_idx < 0)
			return 0;

		host_ce_path[0] = '\0';
		platform_translate_path(ce_path, host_ce_path, sizeof(host_ce_path));
		if (host_ce_path[0])
			map_fp = fopen(host_ce_path, "rb");
		if (!map_fp && (ce_path[0] == 'd' || ce_path[0] == 'D') && ce_path[1] == ':' && (ce_path[2] == '\\' || ce_path[2] == '/'))
			map_fp = fopen(ce_path + 3, "rb");
		if (!map_fp)
			map_fp = fopen(ce_path, "rb");
		if (!map_fp)
		{
			char const *m_slash = strrchr(ce_path, '\\');
			if (!m_slash)
				m_slash = strrchr(ce_path, '/');
			if (m_slash && m_slash[1])
			{
				snprintf(host_ce_path, sizeof(host_ce_path), "custom_maps/%s", m_slash + 1);
				map_fp = fopen(host_ce_path, "rb");
			}
		}
		if (!map_fp)
			return 0;
		if (fread(map_hdr, 1, sizeof(map_hdr), map_fp) != sizeof(map_hdr))
		{
			fclose(map_fp);
			return 0;
		}
		tag_data_off = *(uint32_t const *)(map_hdr + 0x10);
		tag_data_sz = *(uint32_t const *)(map_hdr + 0x14);
		memset(map_name, 0, sizeof(map_name));
		memset(map_build, 0, sizeof(map_build));
		memcpy(map_name, map_hdr + 0x20, 31);
		memcpy(map_build, map_hdr + 0x40, 31);

		if (tag_data_off < 2048u || tag_data_sz < 64u || tag_data_sz > 0x1700000u)
		{
			fclose(map_fp);
			return 0;
		}
		raw_tag_data = (uint8_t *)malloc(tag_data_sz);
		backup_tag_data = (uint8_t *)malloc(tag_data_sz);
		if (!raw_tag_data || !backup_tag_data ||
			fseek(map_fp, (long)tag_data_off, SEEK_SET) != 0 ||
			fread(raw_tag_data, 1, tag_data_sz, map_fp) != tag_data_sz)
		{
			free(raw_tag_data);
			free(backup_tag_data);
			fclose(map_fp);
			return 0;
		}
		fclose(map_fp);

		port_tag_profiles_scan();
		if (port_tag_profiles_count() <= 0)
			prof = port_tag_profiles_create("Forge Custom Tags");
		else
			prof = 0;
		if (prof < 0)
		{
			free(raw_tag_data);
			free(backup_tag_data);
			return 0;
		}

		/* Back up live [0x40440000, 0x40440000 + tag_data_sz) and temporarily restore
		 * raw 'mod2' group tags (regrouped to 'mode' by custom_edition_models_convert),
		 * geometry->parts (0x84-byte custom_edition_model_part), and model->flags
		 * so tag_bundle_write & tag_bundle_load_tags_regions walk GBXMODEL_PART_DEFINITION_NAME cleanly. */
		memcpy(backup_tag_data, (void const *)0x40440000UL, tag_data_sz);
		for (ti = 0; ti < ce_hdr->tag_count; ti++)
		{
			uint32_t inst_grp = (uint32_t)ce_hdr->instances[ti].group_tag;
			if ((inst_grp == 'mode' || inst_grp == 'mod2') && ce_hdr->instances[ti].base_address)
			{
				uint32_t m_addr = (uint32_t)(uintptr_t)ce_hdr->instances[ti].base_address;
				ce_hdr->instances[ti].group_tag = 'mod2';
				if (m_addr >= 0x40440000UL && (m_addr - 0x40440000UL) + 0xE8u <= tag_data_sz)
				{
					uint32_t m_off = m_addr - 0x40440000UL;
					int32_t g_cnt = *(int32_t const *)((uint8_t const *)0x40440000UL + m_off + 0xD0u);
					uint32_t g_addr = *(uint32_t const *)((uint8_t const *)0x40440000UL + m_off + 0xD4u);
					int32_t s_cnt = *(int32_t const *)((uint8_t const *)0x40440000UL + m_off + 0xDCu);
					int32_t g;
					memcpy((uint8_t *)0x40440000UL + m_off, raw_tag_data + m_off, 4u);
					if (g_cnt > 0 && g_cnt <= 256 && g_addr >= 0x40440000UL &&
						(g_addr - 0x40440000UL) + (uint32_t)g_cnt * 0x30u <= tag_data_sz)
					{
						for (g = 0; g < g_cnt; g++)
						{
							uint32_t go = (g_addr - 0x40440000UL) + (uint32_t)g * 0x30u;
							int32_t p_cnt = *(int32_t const *)((uint8_t const *)0x40440000UL + go + 0x24u);
							uint32_t p_addr = *(uint32_t const *)((uint8_t const *)0x40440000UL + go + 0x28u);
							if (p_cnt > 0 && p_cnt <= 256 && p_addr >= 0x40440000UL &&
								(p_addr - 0x40440000UL) + (uint32_t)p_cnt * 0x84u <= tag_data_sz)
							{
								uint32_t po = p_addr - 0x40440000UL;
								int32_t pi;
								memcpy((uint8_t *)0x40440000UL + po, raw_tag_data + po, (size_t)p_cnt * 0x84u);
								for (pi = 0; pi < p_cnt; pi++)
								{
									uint8_t *part_p = (uint8_t *)0x40440000UL + po + (uint32_t)pi * 0x84u;
									int16_t sh_idx = *(int16_t const *)(part_p + 0x04u);
									if (s_cnt > 0 && (sh_idx < 0 || sh_idx >= s_cnt))
										*(int16_t *)(part_p + 0x04u) = (int16_t)(s_cnt - 1);
									/* Clear empty uncompressed/compressed/triangle tag_blocks at +0x20, +0x2C, +0x38 */
									memset(part_p + 0x20u, 0, 36u);
								}
							}
						}
					}
				}
			}
		}

		memset(&source, 0, sizeof(source));
		source.tag_header = ce_hdr;
		source.loaded_size = 0x1700000L;
		source.read = active_ce_map_bundle_read_cb;
		source.context = NULL;
		source.map_name = map_name[0] ? map_name : "custom_map";
		source.map_build = map_build[0] ? map_build : "01.00.00.0609";

		memset(&report, 0, sizeof(report));
		if (tag_bundle_write(&source, abs_idx, forge_grow_buf_write, &buffer, &report) != _tag_bundle_status_ok ||
			!buffer.bytes || buffer.size < TAG_BUNDLE_HEADER_BYTES)
		{
			memcpy((void *)0x40440000UL, backup_tag_data, tag_data_sz);
			free(backup_tag_data);
			free(raw_tag_data);
			free(buffer.bytes);
			return 0;
		}

		/* Immediately restore live memory in 0x40440000 before any further work */
		memcpy((void *)0x40440000UL, backup_tag_data, tag_data_sz);
		free(backup_tag_data);
		free(raw_tag_data);

		tags_size = tag_bundle_file_data_offset(buffer.bytes);
		if (tags_size >= TAG_BUNDLE_HEADER_BYTES && tags_size <= buffer.size)
		{
			copy_tags = (byte *)malloc(tags_size);
			if (copy_tags)
			{
				memcpy(copy_tags, buffer.bytes, tags_size);
				if (tag_bundle_load_tags_regions(
						copy_tags,
						tags_size,
						buffer.size,
						&loaded,
						&regions,
						&region_count) == _tag_bundle_status_ok)
				{
					added = port_tag_profiles_tag_add(
						prof,
						source.map_name,
						(unsigned long)group_tag,
						tag_path,
						buffer.bytes,
						buffer.size,
						tags_size,
						(struct port_tag_store_region const *)(regions ? regions : &no_region),
						region_count);
				}
				free(copy_tags);
				free(regions);
			}
		}
		free(buffer.bytes);

		if (!added)
			return 0;

		{
			long t_slot = port_tag_profiles_tag_find(prof, (unsigned long)group_tag, tag_path);
			if (t_slot >= 0)
			{
				port_tag_profiles_tag_set_enabled(prof, t_slot, 1);
				port_tag_profiles_tag_set_option(prof, t_slot, "name", clean_name);
			}
		}
		forge_runtime_reload_catalogs();
		return 1;
	}

	/* Case 2: Active Xbox (v5) map (stock, custom Xbox, or Forge-saved map) */
	{
		char const *cur_map = forge_get_active_map_name();
		char const *m_slash;
		char map_leaf[64];
		char resolved_map[512];
		static char const *const s_dirs[] = {
			"maps",
			"custom_maps",
			"assets/maps",
			"../assets/maps"
		};
		int d, found_map = 0, ok = 0;

		if (!cur_map || !cur_map[0])
			return 0;
		m_slash = strrchr(cur_map, '\\');
		if (!m_slash)
			m_slash = strrchr(cur_map, '/');
		snprintf(map_leaf, sizeof(map_leaf), "%s", m_slash ? (m_slash + 1) : cur_map);

		/* If the active map is an MCC map, resolve its MCC package path FIRST so we don't
		 * accidentally open a retail/Xbox map with the same leaf stem (e.g. bloodgulch.map). */
		if (_strnicmp(cur_map, "mcc_maps\\", 9) == 0 ||
			_strnicmp(cur_map, "mcc_maps/", 9) == 0 ||
			mcc_cache_tags_loaded())
		{
			char mcc_rel[128];
			if (mcc_package_map_path(cur_map, mcc_rel) && mcc_rel[0])
			{
				if (is_mcc_version13_map_file(mcc_rel))
				{
					snprintf(resolved_map, sizeof(resolved_map), "%s", mcc_rel);
					found_map = 1;
				}
				else
				{
					char xbox_vpath[160];
					snprintf(xbox_vpath, sizeof(xbox_vpath), "d:\\%s", mcc_rel);
					platform_translate_path(xbox_vpath, resolved_map, sizeof(resolved_map));
					if (is_mcc_version13_map_file(resolved_map))
						found_map = 1;
				}
			}
		}

		for (d = 0; !found_map && d < 4; d++)
		{
			snprintf(resolved_map, sizeof(resolved_map), "%s/%s.map", s_dirs[d], map_leaf);
			if (is_xbox_version5_map_file(resolved_map))
			{
				found_map = 1;
				break;
			}
		}
		if (!found_map)
		{
			char xbox_vpath[128];
			snprintf(xbox_vpath, sizeof(xbox_vpath), "d:\\maps\\%s.map", map_leaf);
			platform_translate_path(xbox_vpath, resolved_map, sizeof(resolved_map));
			if (is_xbox_version5_map_file(resolved_map))
				found_map = 1;
		}
		if (!found_map)
		{
			char xbox_vpath[128];
			snprintf(xbox_vpath, sizeof(xbox_vpath), "d:\\custom_maps\\%s.map", map_leaf);
			platform_translate_path(xbox_vpath, resolved_map, sizeof(resolved_map));
			if (is_xbox_version5_map_file(resolved_map))
				found_map = 1;
		}
		if (!found_map)
		{
			char mcc_rel[128];
			if (mcc_package_map_path(cur_map, mcc_rel) && mcc_rel[0])
			{
				if (is_mcc_version13_map_file(mcc_rel))
				{
					snprintf(resolved_map, sizeof(resolved_map), "%s", mcc_rel);
					found_map = 1;
				}
				else
				{
					char xbox_vpath[160];
					snprintf(xbox_vpath, sizeof(xbox_vpath), "d:\\%s", mcc_rel);
					platform_translate_path(xbox_vpath, resolved_map, sizeof(resolved_map));
					if (is_mcc_version13_map_file(resolved_map))
						found_map = 1;
				}
			}
		}
		if (!found_map)
			return 0;

		if (!forge_ui_open_map_for_tag_browser(resolved_map))
			return 0;

		if (s_map_browser.is_xbox_map && s_map_browser.xbox_tag_buf)
		{
			int32_t abs_idx = -1, i;
			for (i = 0; i < s_map_browser.xbox_tag_count; i++)
			{
				if (browser_tag_group_at(i) == group_tag &&
					_stricmp(browser_tag_name_at(i), tag_path) == 0)
				{
					abs_idx = i;
					break;
				}
			}
			if (abs_idx >= 0)
				ok = extract_xbox_map_tag_to_vault(abs_idx, group_tag, tag_path, clean_name);
		}
		forge_ui_close_map_tag_browser();
		if (ok)
			forge_runtime_reload_catalogs();
		return ok;
	}
}

static int catalog_entry_matches_installed_tab(struct forge_catalog_entry const *e, int category_tab)
{
	if (!e || e->source_cache_id == 3 || e->group_tag == 'spwn' || e->group_tag == 'tele' || e->group_tag == 'flag')
		return 0;
	return browser_group_matches_category(e->group_tag, category_tab);
}

static int get_installed_catalog_index(int category_tab, int list_index)
{
	int i, found = 0;
	if (category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS || list_index < 0)
		return -1;
	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (catalog_entry_matches_installed_tab(&g_forge.catalog[i], category_tab))
		{
			if (found == list_index)
				return i;
			found++;
		}
	}
	return -1;
}

int forge_ui_installed_tag_count(int category_tab)
{
	int i, count = 0;
	if (category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS)
		return 0;
	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (catalog_entry_matches_installed_tab(&g_forge.catalog[i], category_tab))
			count++;
	}
	return count;
}

int forge_ui_installed_tag_get_info(
	int category_tab,
	int list_index,
	char *out_display_name,
	size_t disp_sz,
	char *out_full_path,
	size_t full_sz,
	char *out_type_str,
	size_t type_sz,
	char *out_size_str,
	size_t size_sz,
	int *out_is_custom)
{
	int cat_idx = get_installed_catalog_index(category_tab, list_index);
	struct forge_catalog_entry const *e;
	int32_t byte_size = 0;

	if (cat_idx < 0 || cat_idx >= g_forge.catalog_count)
		return 0;
	e = &g_forge.catalog[cat_idx];

	if (out_display_name && disp_sz > 0)
		snprintf(out_display_name, disp_sz, "%s", e->display_name);
	if (out_full_path && full_sz > 0)
		snprintf(out_full_path, full_sz, "%s", e->tag_path);
	if (out_type_str && type_sz > 0)
		snprintf(out_type_str, type_sz, "%s", browser_group_type_label(e->group_tag));
	if (out_is_custom)
		*out_is_custom = (_strnicmp(e->display_name, "[c] ", 4) == 0) ? 1 : 0;

	if (out_size_str && size_sz > 0)
	{
		long p_cnt = port_tag_profiles_count();
		long p;

		for (p = 0; p < p_cnt && byte_size <= 0; p++)
		{
			long t = port_tag_profiles_tag_find(p, (unsigned long)e->group_tag, e->tag_path);
			if (t >= 0)
				byte_size = (int32_t)port_tag_profiles_tag_size(p, t);
		}
		if (byte_size <= 0 && s_preview_xbox_donor.fp && s_preview_xbox_donor.tag_buf)
		{
			int32_t i;
			char const *lookup = (e->group_tag == 'mach' &&
				_stricmp(e->tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0)
				? "levels\\a30\\devices\\beam emitter\\beam emitter"
				: e->tag_path;
			for (i = 0; i < s_preview_xbox_donor.tag_count; i++)
			{
				uint32_t io = s_preview_xbox_donor.inst_off + (uint32_t)i * 32u;
				uint32_t grp = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x00);
				uint32_t name_addr = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x10);
				if (grp == e->group_tag && name_addr >= s_preview_xbox_donor.tag_base &&
					(name_addr - s_preview_xbox_donor.tag_base) < s_preview_xbox_donor.tag_sz)
				{
					char const *tn = (char const *)(s_preview_xbox_donor.tag_buf + (name_addr - s_preview_xbox_donor.tag_base));
					if (_stricmp(tn, lookup) == 0)
					{
						byte_size = compute_xbox_tag_closure_bytes(
							s_preview_xbox_donor.tag_buf,
							s_preview_xbox_donor.tag_sz,
							s_preview_xbox_donor.tag_base,
							s_preview_xbox_donor.inst_off,
							s_preview_xbox_donor.tag_count,
							i);
						break;
					}
				}
			}
		}
		format_tag_byte_size(byte_size, out_size_str, size_sz);
	}
	return 1;
}

static void remove_manifest_entry_from_file(char const *manifest_path, uint32_t grp, char const *tag_path)
{
	FILE *in_fp = fopen(manifest_path, "r");
	char (*lines)[512];
	int line_count = 0, i;
	char grp_str[5];
	FILE *out_fp;

	if (!in_fp)
		return;
	lines = (char (*)[512])malloc(2048u * sizeof(*lines));
	if (!lines)
	{
		fclose(in_fp);
		return;
	}
	grp_str[0] = (char)((grp >> 24) & 0xFF);
	grp_str[1] = (char)((grp >> 16) & 0xFF);
	grp_str[2] = (char)((grp >> 8) & 0xFF);
	grp_str[3] = (char)(grp & 0xFF);
	grp_str[4] = '\0';

	{
		char buf[512];
		while (line_count < 2048 && fgets(buf, sizeof(buf), in_fp))
		{
			char l_grp[16], l_path[256];
			int l_cat = 0, l_src = 0;
			char *nl = strchr(buf, '\r');
			if (nl) *nl = '\0';
			nl = strchr(buf, '\n');
			if (nl) *nl = '\0';
			if (sscanf(buf, "%4s|%d|%d|%255[^|\n]", l_grp, &l_cat, &l_src, l_path) == 4 &&
				_stricmp(l_grp, grp_str) == 0 && _stricmp(l_path, tag_path) == 0)
			{
				continue;
			}
			snprintf(lines[line_count++], sizeof(lines[0]), "%s", buf);
		}
	}
	fclose(in_fp);

	out_fp = fopen(manifest_path, "wb");
	if (out_fp)
	{
		for (i = 0; i < line_count; i++)
			fprintf(out_fp, "%s\n", lines[i]);
		fclose(out_fp);
	}
	free(lines);
}

int forge_ui_installed_tag_delete(int category_tab, int list_index)
{
	int cat_idx = get_installed_catalog_index(category_tab, list_index);
	uint32_t grp;
	char tag_path[256];
	long p_cnt, p;

	if (cat_idx < 0 || cat_idx >= g_forge.catalog_count)
		return 0;
	grp = g_forge.catalog[cat_idx].group_tag;
	snprintf(tag_path, sizeof(tag_path), "%s", g_forge.catalog[cat_idx].tag_path);

	/* 1. Remove from Faber Tag Vault profiles if present */
	port_tag_profiles_scan();
	p_cnt = port_tag_profiles_count();
	for (p = 0; p < p_cnt; p++)
	{
		long t = port_tag_profiles_tag_find(p, (unsigned long)grp, tag_path);
		if (t >= 0)
			port_tag_profiles_tag_remove(p, t);
	}

	/* 2. Remove from xbox_catalog.txt if present */
	remove_manifest_entry_from_file("OpenCE Forge/cached_tags/xbox_catalog.txt", grp, tag_path);

	forge_runtime_reload_catalogs();
	return 1;
}

/* ============================================================================
 * 3D SOFTWARE MESH PREVIEW BUILDER (Supports CE 'mod2', Faber '.fbrtag',
 * and Xbox/MCC 'mode' tags!)
 * ============================================================================ */

#define FORGE_PREVIEW_MAX_VERTICES   16384
#define FORGE_PREVIEW_MAX_TRIANGLES  24576
#define FORGE_PREVIEW_MAX_SHADERS    64
#define FORGE_PREVIEW_TEXTURE_SIZE   128

struct forge_preview_node_xform
{
	float matrix[3][3];
	float point[3];
};

static void compute_preview_world_nodes(
	int32_t node_count,
	uint8_t const *nodes_base,
	struct forge_preview_node_xform *out_nodes)
{
	int32_t i;
	for (i = 0; i < node_count; i++)
	{
		uint8_t const *n = nodes_base + (size_t)i * 156u;
		int16_t parent = *(int16_t const *)(n + 36u);
		float tx = *(float const *)(n + 40u);
		float ty = *(float const *)(n + 44u);
		float tz = *(float const *)(n + 48u);
		float qi = *(float const *)(n + 52u);
		float qj = *(float const *)(n + 56u);
		float qk = *(float const *)(n + 60u);
		float qw = *(float const *)(n + 64u);
		float norm = qi * qi + qj * qj + qk * qk + qw * qw;
		float s = (norm > 0.0f) ? (2.0f / norm) : 0.0f;
		float xx = qi * qi * s, xy = qi * qj * s, xz = qi * qk * s, xw = qi * qw * s;
		float yy = qj * qj * s, yz = qj * qk * s, yw = qj * qw * s;
		float zz = qk * qk * s, zw = qk * qw * s;
		float loc[3][3];

		loc[0][0] = 1.0f - (yy + zz);
		loc[0][1] = xy + zw;
		loc[0][2] = xz - yw;
		loc[1][0] = xy - zw;
		loc[1][1] = 1.0f - (xx + zz);
		loc[1][2] = yz + xw;
		loc[2][0] = xz + yw;
		loc[2][1] = yz - xw;
		loc[2][2] = 1.0f - (xx + yy);

		if (parent >= 0 && parent < i)
		{
			struct forge_preview_node_xform const *p = &out_nodes[parent];
			int r, c;
			for (r = 0; r < 3; r++)
			{
				for (c = 0; c < 3; c++)
				{
					out_nodes[i].matrix[r][c] =
						loc[r][0] * p->matrix[0][c] +
						loc[r][1] * p->matrix[1][c] +
						loc[r][2] * p->matrix[2][c];
				}
			}
			for (c = 0; c < 3; c++)
			{
				out_nodes[i].point[c] = p->point[c] +
					tx * p->matrix[0][c] +
					ty * p->matrix[1][c] +
					tz * p->matrix[2][c];
			}
		}
		else
		{
			memcpy(out_nodes[i].matrix, loc, sizeof(loc));
			out_nodes[i].point[0] = tx;
			out_nodes[i].point[1] = ty;
			out_nodes[i].point[2] = tz;
		}
	}
}

static void preview_append_part_vertices_and_strip(
	struct tag_injector_mesh *mesh,
	uint8_t const *verts68,
	int32_t vtx_count,
	uint16_t const *strip_indices,
	int32_t strip_tri_count,
	struct forge_preview_node_xform const *world_nodes,
	int32_t node_count,
	int has_local_nodes,
	int16_t local_node_count,
	uint8_t const *local_node_indices,
	short material_index,
	float model_u_scale,
	float model_v_scale)
{
	long base_vtx = mesh->vertex_count;
	int32_t v, t;

	if (vtx_count <= 0 || strip_tri_count <= 0 ||
		base_vtx + vtx_count > FORGE_PREVIEW_MAX_VERTICES ||
		mesh->triangle_count + strip_tri_count > FORGE_PREVIEW_MAX_TRIANGLES)
		return;

	for (v = 0; v < vtx_count; v++)
	{
		uint8_t const *vb = verts68 + (size_t)v * 68u;
		float px = *(float const *)(vb + 0u);
		float py = *(float const *)(vb + 4u);
		float pz = *(float const *)(vb + 8u);
		float nx = *(float const *)(vb + 12u);
		float ny = *(float const *)(vb + 16u);
		float nz = *(float const *)(vb + 20u);
		float tu = *(float const *)(vb + 48u) * model_u_scale;
		float tv = *(float const *)(vb + 52u) * model_v_scale;
		int16_t n0 = *(int16_t const *)(vb + 56u);
		int16_t n1 = *(int16_t const *)(vb + 58u);
		float w0 = *(float const *)(vb + 60u);
		float w1 = *(float const *)(vb + 64u);

		if (!(px > -1000.0f && px < 1000.0f && py > -1000.0f && py < 1000.0f && pz > -1000.0f && pz < 1000.0f))
			px = py = pz = 0.0f;
		if (!(nx > -1000.0f && nx < 1000.0f && ny > -1000.0f && ny < 1000.0f && nz > -1000.0f && nz < 1000.0f))
		{
			nx = 0.0f; ny = 0.0f; nz = 1.0f;
		}
		if (!(tu > -1000.0f && tu < 1000.0f && tv > -1000.0f && tv < 1000.0f))
			tu = tv = 0.0f;

		if (world_nodes && node_count > 0)
		{
			if (has_local_nodes && local_node_indices)
			{
				n0 = (n0 >= 0 && n0 < local_node_count) ? (int16_t)local_node_indices[n0] : -1;
				n1 = (n1 >= 0 && n1 < local_node_count) ? (int16_t)local_node_indices[n1] : -1;
			}
			if (n0 >= 0 && n0 < node_count)
			{
				struct forge_preview_node_xform const *xf0 = &world_nodes[n0];
				float p0x = xf0->point[0] + px * xf0->matrix[0][0] + py * xf0->matrix[1][0] + pz * xf0->matrix[2][0];
				float p0y = xf0->point[1] + px * xf0->matrix[0][1] + py * xf0->matrix[1][1] + pz * xf0->matrix[2][1];
				float p0z = xf0->point[2] + px * xf0->matrix[0][2] + py * xf0->matrix[1][2] + pz * xf0->matrix[2][2];
				float n0x = nx * xf0->matrix[0][0] + ny * xf0->matrix[1][0] + nz * xf0->matrix[2][0];
				float n0y = nx * xf0->matrix[0][1] + ny * xf0->matrix[1][1] + nz * xf0->matrix[2][1];
				float n0z = nx * xf0->matrix[0][2] + ny * xf0->matrix[1][2] + nz * xf0->matrix[2][2];

				if (n1 >= 0 && n1 < node_count && w0 >= 0.0f && w0 <= 1.0f && w1 > 0.0f && w1 <= 1.0f)
				{
					struct forge_preview_node_xform const *xf1 = &world_nodes[n1];
					float p1x = xf1->point[0] + px * xf1->matrix[0][0] + py * xf1->matrix[1][0] + pz * xf1->matrix[2][0];
					float p1y = xf1->point[1] + px * xf1->matrix[0][1] + py * xf1->matrix[1][1] + pz * xf1->matrix[2][1];
					float p1z = xf1->point[2] + px * xf1->matrix[0][2] + py * xf1->matrix[1][2] + pz * xf1->matrix[2][2];
					float n1x = nx * xf1->matrix[0][0] + ny * xf1->matrix[1][0] + nz * xf1->matrix[2][0];
					float n1y = nx * xf1->matrix[0][1] + ny * xf1->matrix[1][1] + nz * xf1->matrix[2][1];
					float n1z = nx * xf1->matrix[0][2] + ny * xf1->matrix[1][2] + nz * xf1->matrix[2][2];
					px = w0 * p0x + w1 * p1x;
					py = w0 * p0y + w1 * p1y;
					pz = w0 * p0z + w1 * p1z;
					nx = w0 * n0x + w1 * n1x;
					ny = w0 * n0y + w1 * n1y;
					nz = w0 * n0z + w1 * n1z;
				}
				else
				{
					px = p0x; py = p0y; pz = p0z;
					nx = n0x; ny = n0y; nz = n0z;
				}
			}
		}

		mesh->positions[base_vtx + v][0] = px;
		mesh->positions[base_vtx + v][1] = py;
		mesh->positions[base_vtx + v][2] = pz;
		mesh->normals[base_vtx + v][0] = nx;
		mesh->normals[base_vtx + v][1] = ny;
		mesh->normals[base_vtx + v][2] = nz;
		mesh->texcoords[base_vtx + v][0] = tu;
		mesh->texcoords[base_vtx + v][1] = tv;
	}
	mesh->vertex_count += vtx_count;

	for (t = 0; t < strip_tri_count; t++)
	{
		uint16_t a = strip_indices[t];
		uint16_t b = strip_indices[t + 1];
		uint16_t c = strip_indices[t + 2];
		if (a == b || b == c || a == c ||
			a >= (uint16_t)vtx_count || b >= (uint16_t)vtx_count || c >= (uint16_t)vtx_count)
			continue;
		mesh->triangles[mesh->triangle_count][0] = base_vtx + (long)((t & 1) ? b : a);
		mesh->triangles[mesh->triangle_count][1] = base_vtx + (long)((t & 1) ? a : b);
		mesh->triangles[mesh->triangle_count][2] = base_vtx + (long)c;
		mesh->triangle_materials[mesh->triangle_count] = material_index;
		mesh->triangle_count++;
	}
}

enum
{
	_bitmap_format_a8 = 0,
	_bitmap_format_y8,
	_bitmap_format_ay8,
	_bitmap_format_a8y8,
	_bitmap_format_unused1,
	_bitmap_format_unused2,
	_bitmap_format_r5g6b5,
	_bitmap_format_unused3,
	_bitmap_format_a1r5g5b5,
	_bitmap_format_a4r4g4b4,
	_bitmap_format_x8r8g8b8,
	_bitmap_format_a8r8g8b8,
	_bitmap_format_unused4,
	_bitmap_format_unused5,
	_bitmap_format_dxt1,
	_bitmap_format_dxt3,
	_bitmap_format_dxt5,
	_bitmap_format_p8_bump,
	_bitmap_swizzled_bit = 3
};

static unsigned long preview_bitmap_level_bytes(short format, short bpp, short width, short height)
{
	unsigned long w = (width > 0) ? (unsigned long)width : 1ul;
	unsigned long h = (height > 0) ? (unsigned long)height : 1ul;
	if (bpp > 0)
		return w * h * (unsigned long)bpp;
	if (format == _bitmap_format_dxt1)
		return ((w + 3ul) / 4ul) * ((h + 3ul) / 4ul) * 8ul;
	if (format == _bitmap_format_dxt3 || format == _bitmap_format_dxt5)
		return ((w + 3ul) / 4ul) * ((h + 3ul) / 4ul) * 16ul;
	return 0ul;
}

static int preview_decode_bitmap_texture(
	struct bitmap_data const *bitmap,
	boolean (*read_fn)(void *ctx, unsigned long offset, unsigned long size, void *buf),
	void *read_ctx,
	struct tag_injector_texture *out_tex)
{
	short mip_w, mip_h;
	short level = 0;
	unsigned long offset;
	unsigned long size;
	unsigned char *pixels;
	short bpp = 0;

	if (!bitmap || !read_fn || !out_tex)
		return 0;
	if (bitmap->type != 0 /* _bitmap_type_2d */ ||
		bitmap->width <= 0 || bitmap->width > 4096 ||
		bitmap->height <= 0 || bitmap->height > 4096 ||
		bitmap->depth != 1 || bitmap->pixels_offset < 0 || bitmap->pixels_size <= 0)
		return 0;

	switch (bitmap->format)
	{
	case _bitmap_format_a8:
	case _bitmap_format_y8:
	case _bitmap_format_ay8:
		bpp = 1;
		break;
	case _bitmap_format_a8y8:
	case _bitmap_format_r5g6b5:
	case _bitmap_format_a1r5g5b5:
	case _bitmap_format_a4r4g4b4:
		bpp = 2;
		break;
	case _bitmap_format_x8r8g8b8:
	case _bitmap_format_a8r8g8b8:
		bpp = 4;
		break;
	case _bitmap_format_dxt1:
	case _bitmap_format_dxt3:
	case _bitmap_format_dxt5:
		break;
	default:
		return 0;
	}

	mip_w = bitmap->width;
	mip_h = bitmap->height;
	offset = (unsigned long)bitmap->pixels_offset;
	while (level < bitmap->mipmap_count &&
		   (mip_w > FORGE_PREVIEW_TEXTURE_SIZE || mip_h > FORGE_PREVIEW_TEXTURE_SIZE))
	{
		offset += preview_bitmap_level_bytes(bitmap->format, bpp, mip_w, mip_h);
		mip_w = (short)((mip_w >> 1) > 1 ? (mip_w >> 1) : 1);
		mip_h = (short)((mip_h >> 1) > 1 ? (mip_h >> 1) : 1);
		level++;
	}
	size = preview_bitmap_level_bytes(bitmap->format, bpp, mip_w, mip_h);
	if (!size || offset - (unsigned long)bitmap->pixels_offset > (unsigned long)bitmap->pixels_size ||
		size > (unsigned long)bitmap->pixels_size - (offset - (unsigned long)bitmap->pixels_offset))
		return 0;

	pixels = (unsigned char *)malloc(size);
	if (!pixels)
		return 0;
	if (!read_fn(read_ctx, offset, size, pixels))
	{
		free(pixels);
		return 0;
	}
	if (bpp > 0 && TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit))
	{
		unsigned char *unswizzled = (unsigned char *)malloc(size);
		if (!unswizzled)
		{
			free(pixels);
			return 0;
		}
		{
			short sx, sy;
			for (sy = 0; sy < mip_h; sy++)
			{
				for (sx = 0; sx < mip_w; sx++)
				{
					long swz = 0;
					bitmap_swizzle_vector2d(mip_w, mip_h, sx, sy, &swz);
					if (swz >= 0 && (unsigned long)(swz + 1) * (unsigned long)bpp <= size)
						memcpy(unswizzled + ((long)sy * mip_w + sx) * bpp, pixels + swz * bpp, (size_t)bpp);
				}
			}
		}
		free(pixels);
		pixels = unswizzled;
	}

	out_tex->width = mip_w;
	out_tex->height = mip_h;
	out_tex->format = bitmap->format;
	out_tex->size = size;
	out_tex->pixels = pixels;
	return 1;
}

static boolean preview_xbox_file_read_cb(void *ctx, unsigned long offset, unsigned long size, void *buf)
{
	FILE *fp = (FILE *)ctx;
	if (!fp || offset < 2048u)
		return FALSE;
	return (fseek(fp, (long)offset, SEEK_SET) == 0 && fread(buf, 1, size, fp) == size) ? TRUE : FALSE;
}

struct preview_mem_slice
{
	unsigned char const *bytes;
	unsigned long size;
};

static boolean preview_mem_read_cb(void *ctx, unsigned long offset, unsigned long size, void *buf)
{
	struct preview_mem_slice const *slice = (struct preview_mem_slice const *)ctx;
	if (!slice || !slice->bytes || offset > slice->size || size > slice->size - offset)
		return FALSE;
	memcpy(buf, slice->bytes + offset, size);
	return TRUE;
}

static int preview_mesh_alloc_buffers(struct tag_injector_mesh *mesh, short *shader_to_mat)
{
	int i;
	mesh->positions = (float (*)[3])malloc(FORGE_PREVIEW_MAX_VERTICES * sizeof(*mesh->positions));
	mesh->normals = (float (*)[3])malloc(FORGE_PREVIEW_MAX_VERTICES * sizeof(*mesh->normals));
	mesh->texcoords = (float (*)[2])malloc(FORGE_PREVIEW_MAX_VERTICES * sizeof(*mesh->texcoords));
	mesh->triangles = (long (*)[3])malloc(FORGE_PREVIEW_MAX_TRIANGLES * sizeof(*mesh->triangles));
	mesh->triangle_materials = (short *)malloc(FORGE_PREVIEW_MAX_TRIANGLES * sizeof(short));
	mesh->materials = (struct tag_injector_material *)calloc(FORGE_PREVIEW_MAX_SHADERS, sizeof(*mesh->materials));
	mesh->textures = (struct tag_injector_texture *)calloc(FORGE_PREVIEW_MAX_SHADERS, sizeof(*mesh->textures));
	if (!mesh->positions || !mesh->normals || !mesh->texcoords ||
		!mesh->triangles || !mesh->triangle_materials || !mesh->materials || !mesh->textures)
	{
		tag_preview_mesh_free(mesh);
		return 0;
	}
	mesh->color_a[0] = mesh->color_a[1] = mesh->color_a[2] = 1.0f;
	mesh->color_b[0] = mesh->color_b[1] = mesh->color_b[2] = 1.0f;
	for (i = 0; i < FORGE_PREVIEW_MAX_SHADERS; i++)
		shader_to_mat[i] = -2;
	return 1;
}

static int build_preview_mesh_from_xbox_map_data(
	FILE *fp,
	uint8_t const *tag_buf,
	uint32_t tag_sz,
	uint32_t tag_base,
	uint32_t inst_off,
	int32_t tag_count,
	int32_t obj_abs_idx,
	struct tag_injector_mesh *mesh)
{
	uint32_t obj_io, obj_data_addr, obj_off;
	int32_t mod_ref_idx, mod_abs_idx;
	uint32_t mod_io, mod_grp, mod_data_addr, mod_off;
	uint32_t part_stride;
	int has_local_nodes = 0;
	float u_scale, v_scale;
	int32_t node_cnt, reg_cnt, geom_cnt, shdr_cnt;
	uint32_t node_addr, reg_addr, geom_addr, shdr_addr;
	struct forge_preview_node_xform *world_nodes = NULL;
	short shader_to_mat[FORGE_PREVIEW_MAX_SHADERS];
	int r_i, s_i;

	if (!tag_buf || obj_abs_idx < 0 || obj_abs_idx >= tag_count || !mesh)
		return 0;

	obj_io = inst_off + (uint32_t)obj_abs_idx * 32u;
	obj_data_addr = *(uint32_t const *)(tag_buf + obj_io + 0x14);
	if (obj_data_addr < tag_base || (obj_data_addr - tag_base) + 0x40u > tag_sz)
		return 0;
	obj_off = obj_data_addr - tag_base;

	/* object.model reference is at +0x28 (index at +0x34) */
	mod_ref_idx = *(int32_t const *)(tag_buf + obj_off + 0x34u);
	if (mod_ref_idx == -1)
		return 0;
	mod_abs_idx = mod_ref_idx & 0xFFFF;
	if (mod_abs_idx < 0 || mod_abs_idx >= tag_count)
		return 0;

	mod_io = inst_off + (uint32_t)mod_abs_idx * 32u;
	mod_grp = *(uint32_t const *)(tag_buf + mod_io + 0x00);
	if (mod_grp != 'mode' && mod_grp != 'mod2')
		return 0;
	part_stride = (mod_grp == 'mod2') ? 132u : 104u;
	mod_data_addr = *(uint32_t const *)(tag_buf + mod_io + 0x14);
	if (mod_data_addr < tag_base || (mod_data_addr - tag_base) + 0xE8u > tag_sz)
		return 0;
	mod_off = mod_data_addr - tag_base;
	if (mod_grp == 'mod2' && (*(uint32_t const *)(tag_buf + mod_off) & 2u))
		has_local_nodes = 1;

	u_scale = *(float const *)(tag_buf + mod_off + 0x30u);
	v_scale = *(float const *)(tag_buf + mod_off + 0x34u);
	if (!(u_scale > 0.0001f && u_scale < 1000.0f)) u_scale = 1.0f;
	if (!(v_scale > 0.0001f && v_scale < 1000.0f)) v_scale = 1.0f;

	node_cnt = *(int32_t const *)(tag_buf + mod_off + 0xB8u);
	node_addr = *(uint32_t const *)(tag_buf + mod_off + 0xBCu);
	reg_cnt = *(int32_t const *)(tag_buf + mod_off + 0xC4u);
	reg_addr = *(uint32_t const *)(tag_buf + mod_off + 0xC8u);
	geom_cnt = *(int32_t const *)(tag_buf + mod_off + 0xD0u);
	geom_addr = *(uint32_t const *)(tag_buf + mod_off + 0xD4u);
	shdr_cnt = *(int32_t const *)(tag_buf + mod_off + 0xDCu);
	shdr_addr = *(uint32_t const *)(tag_buf + mod_off + 0xE0u);

	if (geom_cnt <= 0 || geom_addr < tag_base || (geom_addr - tag_base) + (uint32_t)geom_cnt * 48u > tag_sz)
		return 0;

	(void)s_i;
	if (!preview_mesh_alloc_buffers(mesh, shader_to_mat))
		return 0;

	if (node_cnt > 0 && node_cnt <= 64 && node_addr >= tag_base &&
		(node_addr - tag_base) + (uint32_t)node_cnt * 156u <= tag_sz)
	{
		world_nodes = (struct forge_preview_node_xform *)malloc((size_t)node_cnt * sizeof(*world_nodes));
		if (world_nodes)
			compute_preview_world_nodes(node_cnt, tag_buf + (node_addr - tag_base), world_nodes);
	}

	/* Iterate regions (or geometry 0 if no regions) */
	for (r_i = 0; r_i < (reg_cnt > 0 ? reg_cnt : 1); r_i++)
	{
		int16_t geom_idx = 0;
		uint32_t go;
		int32_t part_cnt;
		uint32_t part_addr;
		int p_i;

		if (reg_cnt > 0 && reg_addr >= tag_base && (reg_addr - tag_base) + (uint32_t)reg_cnt * 76u <= tag_sz)
		{
			uint32_t ro = (reg_addr - tag_base) + (uint32_t)r_i * 76u;
			int32_t perm_cnt = *(int32_t const *)(tag_buf + ro + 64u);
			uint32_t perm_addr = *(uint32_t const *)(tag_buf + ro + 68u);
			int pm_i;
			geom_idx = -1;
			if (perm_cnt > 0 && perm_addr >= tag_base && (perm_addr - tag_base) + (uint32_t)perm_cnt * 88u <= tag_sz)
			{
				for (pm_i = 0; pm_i < perm_cnt && geom_idx < 0; pm_i++)
				{
					uint32_t pmo = (perm_addr - tag_base) + (uint32_t)pm_i * 88u;
					int lod;
					for (lod = 4; lod >= 0; lod--)
					{
						int16_t g = *(int16_t const *)(tag_buf + pmo + 64u + (uint32_t)lod * 2u);
						if (g >= 0 && g < geom_cnt)
						{
							geom_idx = g;
							break;
						}
					}
				}
			}
		}
		if (geom_idx < 0 || geom_idx >= geom_cnt)
			continue;

		go = (geom_addr - tag_base) + (uint32_t)geom_idx * 48u;
		part_cnt = *(int32_t const *)(tag_buf + go + 36u);
		part_addr = *(uint32_t const *)(tag_buf + go + 40u);
		if (part_cnt <= 0 || part_addr < tag_base || (part_addr - tag_base) + (uint32_t)part_cnt * part_stride > tag_sz)
			continue;

		for (p_i = 0; p_i < part_cnt; p_i++)
		{
			uint32_t po = (part_addr - tag_base) + (uint32_t)p_i * part_stride;
			int16_t shdr_idx = *(int16_t const *)(tag_buf + po + 4u);
			short mat_idx = -1;

			if (shdr_idx >= 0 && shdr_idx < shdr_cnt && shdr_idx < FORGE_PREVIEW_MAX_SHADERS &&
				shdr_addr >= tag_base && (shdr_addr - tag_base) + (uint32_t)shdr_cnt * 32u <= tag_sz)
			{
				if (shader_to_mat[shdr_idx] == -2)
				{
					uint32_t so = (shdr_addr - tag_base) + (uint32_t)shdr_idx * 32u;
					int32_t s_ref = *(int32_t const *)(tag_buf + so + 12u);
					int32_t s_abs = (s_ref != -1) ? (s_ref & 0xFFFF) : -1;
					shader_to_mat[shdr_idx] = -1;
					if (s_abs >= 0 && s_abs < tag_count && fp)
					{
						uint32_t sio = inst_off + (uint32_t)s_abs * 32u;
						uint32_t sgrp = *(uint32_t const *)(tag_buf + sio + 0x00);
						uint32_t sdata = *(uint32_t const *)(tag_buf + sio + 0x14);
						int32_t bitm_ref = -1;
						float su = 1.0f, sv = 1.0f;
						if (sgrp == 'soso' && sdata >= tag_base && (sdata - tag_base) + 0xB4u <= tag_sz)
						{
							su = *(float const *)(tag_buf + (sdata - tag_base) + 0x9Cu);
							sv = *(float const *)(tag_buf + (sdata - tag_base) + 0xA0u);
							bitm_ref = *(int32_t const *)(tag_buf + (sdata - tag_base) + 0xB0u);
						}
						else if (sgrp == 'senv' && sdata >= tag_base && (sdata - tag_base) + 0x98u <= tag_sz)
						{
							bitm_ref = *(int32_t const *)(tag_buf + (sdata - tag_base) + 0x94u);
						}
						if (bitm_ref != -1 && (bitm_ref & 0xFFFF) < tag_count)
						{
							int32_t b_abs = bitm_ref & 0xFFFF;
							uint32_t bio = inst_off + (uint32_t)b_abs * 32u;
							uint32_t bname_addr = *(uint32_t const *)(tag_buf + bio + 0x10);
							uint32_t bdata = *(uint32_t const *)(tag_buf + bio + 0x14);
							char const *b_tname = (bname_addr >= tag_base && (bname_addr - tag_base) < tag_sz)
								? (char const *)(tag_buf + (bname_addr - tag_base))
								: "";
							if (*(uint32_t const *)(tag_buf + bio + 0x00) == 'bitm' &&
								bdata >= tag_base && (bdata - tag_base) + 0x6Cu <= tag_sz)
							{
								int32_t b_cnt = *(int32_t const *)(tag_buf + (bdata - tag_base) + 0x60u);
								uint32_t b_addr = *(uint32_t const *)(tag_buf + (bdata - tag_base) + 0x64u);
								if (b_cnt > 0 && b_addr >= tag_base && (b_addr - tag_base) + 48u <= tag_sz)
								{
									struct bitmap_data b_elem;
									int tex_ok = 0;
									memcpy(&b_elem, tag_buf + (b_addr - tag_base), sizeof(b_elem));
									if (mod_grp == 'mod2' && s_map_browser.is_mcc_map && s_map_browser.mcc_rt)
									{
										unsigned char *conv_pix = NULL;
										uint32_t conv_sz = 0;
										b_elem.flags |= 16u;
										if (mcc_forge_convert_single_bitmap(
												s_map_browser.mcc_rt,
												b_tname,
												0,
												1,
												&b_elem,
												&conv_pix,
												&conv_sz) && conv_pix && conv_sz > 0)
										{
											struct preview_mem_slice slice;
											slice.bytes = conv_pix;
											slice.size = conv_sz;
											b_elem.pixels_offset = 0;
											b_elem.pixels_size = (long)conv_sz;
											tex_ok = preview_decode_bitmap_texture(
												&b_elem,
												preview_mem_read_cb,
												&slice,
												&mesh->textures[mesh->texture_count]);
										}
										free(conv_pix);
									}
									else
									{
										tex_ok = preview_decode_bitmap_texture(
											&b_elem,
											preview_xbox_file_read_cb,
											fp,
											&mesh->textures[mesh->texture_count]);
									}
									if (tex_ok)
									{
										short m_slot = (short)mesh->material_count++;
										short t_slot = (short)mesh->texture_count++;
										mesh->materials[m_slot].texture_index = t_slot;
										mesh->materials[m_slot].color_source = 0;
										mesh->materials[m_slot].u_scale = (su > 0.0001f && su < 1000.0f) ? su : 1.0f;
										mesh->materials[m_slot].v_scale = (sv > 0.0001f && sv < 1000.0f) ? sv : 1.0f;
										mesh->materials[m_slot].base_map_alpha_Replicates = 0;
										shader_to_mat[shdr_idx] = m_slot;
									}
								}
							}
						}
					}
				}
				mat_idx = shader_to_mat[shdr_idx];
			}

			if (mod_grp == 'mod2')
			{
				uint32_t model_file_off = *(uint32_t const *)(tag_buf + 0x14u);
				uint32_t model_vtx_bytes = *(uint32_t const *)(tag_buf + 0x1Cu);
				int32_t tri_cnt = *(int32_t const *)(tag_buf + po + 0x48u);
				uint32_t strip_off = *(uint32_t const *)(tag_buf + po + 0x4Cu);
				int32_t vtx_cnt = *(int32_t const *)(tag_buf + po + 0x58u);
				uint32_t vtx_off = *(uint32_t const *)(tag_buf + po + 0x64u);
				int16_t loc_node_cnt = (int16_t)tag_buf[po + 0x6Bu];
				uint8_t const *loc_nodes = tag_buf + po + 0x6Cu;

				if (fp && tri_cnt > 0 && tri_cnt <= 65536 && vtx_cnt > 0 && vtx_cnt <= 16384 &&
					model_file_off >= 2048u)
				{
					uint32_t vtx_bytes = (uint32_t)vtx_cnt * 68u;
					uint32_t idx_bytes = ((uint32_t)tri_cnt + 2u) * 2u;
					uint8_t *verts68 = (uint8_t *)malloc(vtx_bytes);
					uint16_t *strip_idx = (uint16_t *)malloc(idx_bytes);
					if (verts68 && strip_idx &&
						fseek(fp, (long)(model_file_off + vtx_off), SEEK_SET) == 0 &&
						fread(verts68, 1, vtx_bytes, fp) == vtx_bytes &&
						fseek(fp, (long)(model_file_off + model_vtx_bytes + strip_off), SEEK_SET) == 0 &&
						fread(strip_idx, 1, idx_bytes, fp) == idx_bytes)
					{
						preview_append_part_vertices_and_strip(
							mesh,
							verts68,
							vtx_cnt,
							strip_idx,
							tri_cnt,
							world_nodes,
							node_cnt,
							has_local_nodes,
							loc_node_cnt,
							loc_nodes,
							mat_idx,
							u_scale,
							v_scale);
					}
					free(verts68);
					free(strip_idx);
				}
			}
			else
			{
				int16_t tri_type = *(int16_t const *)(tag_buf + po + 68u);
				int32_t tri_cnt = *(int32_t const *)(tag_buf + po + 72u);
				uint32_t tb_hw = *(uint32_t const *)(tag_buf + po + 80u);
				int32_t vtx_cnt = *(int32_t const *)(tag_buf + po + 88u);
				uint32_t vb_hw = *(uint32_t const *)(tag_buf + po + 100u);

				if (tri_type == _triangle_buffer_type_precompiled_strip &&
					tri_cnt > 0 && tri_cnt <= 65536 && vtx_cnt > 0 && vtx_cnt <= 16384 &&
					tb_hw >= tag_base && (tb_hw - tag_base) + 12u <= tag_sz &&
					vb_hw >= tag_base && (vb_hw - tag_base) + 12u <= tag_sz)
				{
					uint32_t ib_addr = *(uint32_t const *)(tag_buf + (tb_hw - tag_base) + 4u);
					uint32_t vb_addr = *(uint32_t const *)(tag_buf + (vb_hw - tag_base) + 4u);
					uint32_t comp_sz = (uint32_t)vtx_cnt * 32u;
					uint32_t idx_sz = ((uint32_t)tri_cnt + 2u) * 2u;
					if (ib_addr >= tag_base && (ib_addr - tag_base) + idx_sz <= tag_sz &&
						vb_addr >= tag_base && (vb_addr - tag_base) + comp_sz <= tag_sz)
					{
						uint8_t *verts68 = (uint8_t *)malloc((size_t)vtx_cnt * 68u);
						if (verts68)
						{
							rasterizer_geometry_uncompress_vertices(
								_rasterizer_vertex_type_model_compressed,
								vtx_cnt,
								verts68,
								(int)vtx_cnt * 68,
								(void *)(tag_buf + (vb_addr - tag_base)),
								(int)comp_sz);
							preview_append_part_vertices_and_strip(
								mesh,
								verts68,
								vtx_cnt,
								(uint16_t const *)(tag_buf + (ib_addr - tag_base)),
								tri_cnt,
								world_nodes,
								node_cnt,
								0,
								0,
								NULL,
								mat_idx,
								u_scale,
								v_scale);
							free(verts68);
						}
					}
				}
			}
		}
	}

	free(world_nodes);
	return mesh->triangle_count > 0 ? 1 : 0;
}

struct preview_ce_source
{
	int is_bundle;
	struct tag_bundle_source map_src;
	struct browser_ce_map_header const *map_hdr;
	struct tag_bundle_loaded const *bundle_loaded;
	struct faber_bundle_read_ctx bundle_read;
};

static uint8_t const *preview_ce_get_tag(
	struct preview_ce_source const *src,
	long index,
	uint32_t *out_group)
{
	if (!src || index == NONE)
		return NULL;
	if (src->is_bundle)
	{
		if (index >= 0 && index < src->bundle_loaded->tag_count)
		{
			byte const *ent = src->bundle_loaded->tags + index * TAG_BUNDLE_TAG_BYTES;
			if (out_group)
				*out_group = *(uint32_t const *)(ent + TAG_BUNDLE_TAG_GROUP_OFFSET);
			return src->bundle_loaded->data + *(uint32_t const *)(ent + TAG_BUNDLE_TAG_ROOT_OFFSET);
		}
	}
	else if (src->map_hdr)
	{
		long abs_i = index & 0xFFFF;
		if (abs_i >= 0 && abs_i < src->map_hdr->tag_count)
		{
			if (out_group)
				*out_group = (uint32_t)src->map_hdr->instances[abs_i].group_tag;
			return (uint8_t const *)src->map_hdr->instances[abs_i].base_address;
		}
	}
	return NULL;
}

static boolean preview_ce_read_file_cb(void *ctx, unsigned long offset, unsigned long size, void *buf)
{
	struct preview_ce_source const *src = (struct preview_ce_source const *)ctx;
	if (!src)
		return FALSE;
	if (src->is_bundle)
		return faber_bundle_file_read_cb((void *)&src->bundle_read, offset, size, buf);
	return src->map_src.read(src->map_src.context, offset, size, buf);
}

static int build_preview_mesh_from_ce_source(
	struct preview_ce_source const *src,
	uint8_t const *obj_root,
	uint32_t obj_group,
	struct tag_injector_mesh *mesh)
{
	struct object_definition const *obj;
	uint32_t mod_group = 0;
	struct model const *model;
	float u_scale, v_scale;
	struct forge_preview_node_xform *world_nodes = NULL;
	short shader_to_mat[FORGE_PREVIEW_MAX_SHADERS];
	int has_local_nodes;
	long r_i, s_i;

	(void)obj_group;
	if (!src || !obj_root || !mesh)
		return 0;
	obj = (struct object_definition const *)obj_root;
	if (obj->object.model.index == NONE)
		return 0;
	model = (struct model const *)preview_ce_get_tag(src, obj->object.model.index, &mod_group);
	if (!model || (mod_group != 'mod2' && mod_group != 'mode') ||
		model->geometries.count <= 0 || !model->geometries.address)
		return 0;

	u_scale = model->base_map_scale.i;
	v_scale = model->base_map_scale.j;
	if (!(u_scale > 0.0001f && u_scale < 1000.0f)) u_scale = 1.0f;
	if (!(v_scale > 0.0001f && v_scale < 1000.0f)) v_scale = 1.0f;

	(void)s_i;
	if (!preview_mesh_alloc_buffers(mesh, shader_to_mat))
		return 0;

	if (model->nodes.count > 0 && model->nodes.count <= 256 && model->nodes.address)
	{
		world_nodes = (struct forge_preview_node_xform *)malloc((size_t)model->nodes.count * sizeof(*world_nodes));
		if (world_nodes)
			compute_preview_world_nodes(model->nodes.count, (uint8_t const *)model->nodes.address, world_nodes);
	}
	has_local_nodes = TEST_FLAG(model->flags, 1) ? 1 : 0;

	for (r_i = 0; r_i < (model->regions.count > 0 ? model->regions.count : 1); r_i++)
	{
		short geom_idx = 0;
		uint8_t const *geom_ptr;
		struct tag_block const *parts_blk;
		long p_i;

		if (model->regions.count > 0 && model->regions.address)
		{
			struct model_region const *reg = (struct model_region const *)model->regions.address + r_i;
			long pm_i;
			geom_idx = NONE;
			for (pm_i = 0; reg->permutations.address && pm_i < reg->permutations.count && geom_idx == NONE; pm_i++)
			{
				struct model_region_permutation const *perm =
					(struct model_region_permutation const *)reg->permutations.address + pm_i;
				int lod;
				for (lod = 4; lod >= 0; lod--)
				{
					short g = perm->geometry_indices[lod];
					if (g >= 0 && g < model->geometries.count)
					{
						geom_idx = g;
						break;
					}
				}
			}
		}
		if (geom_idx < 0 || geom_idx >= model->geometries.count)
			continue;

		geom_ptr = (uint8_t const *)model->geometries.address + (size_t)geom_idx * 48u;
		parts_blk = (struct tag_block const *)(geom_ptr + 36u);
		if (parts_blk->count <= 0 || !parts_blk->address)
			continue;

		for (p_i = 0; p_i < parts_blk->count; p_i++)
		{
			uint8_t const *part = (uint8_t const *)parts_blk->address + (size_t)p_i * 132u /* GBXMODEL_PART_BYTES */;
			short shdr_idx = *(short const *)(part + 0x04u);
			int32_t tri_cnt = *(int32_t const *)(part + 0x48u);
			uint32_t strip_off = *(uint32_t const *)(part + 0x4Cu);
			int32_t vtx_cnt = *(int32_t const *)(part + 0x58u);
			uint32_t vtx_off = *(uint32_t const *)(part + 0x64u);
			int16_t loc_node_cnt = (int16_t)part[0x6Bu];
			uint8_t const *loc_nodes = part + 0x6Cu;
			short mat_idx = -1;

			if (shdr_idx >= 0 && shdr_idx < model->shaders.count &&
				shdr_idx < FORGE_PREVIEW_MAX_SHADERS && model->shaders.address)
			{
				if (shader_to_mat[shdr_idx] == -2)
				{
					uint8_t const *shdr_ref = (uint8_t const *)model->shaders.address + (size_t)shdr_idx * 32u;
					long s_idx = *(long const *)(shdr_ref + 12u);
					uint32_t s_grp = 0;
					uint8_t const *s_def = preview_ce_get_tag(src, s_idx, &s_grp);
					shader_to_mat[shdr_idx] = -1;
					if (s_def)
					{
						long bitm_idx = NONE;
						float su = 1.0f, sv = 1.0f;
						if (s_grp == 'soso')
						{
							su = *(float const *)(s_def + 0x9Cu);
							sv = *(float const *)(s_def + 0xA0u);
							bitm_idx = *(long const *)(s_def + 0xB0u);
						}
						else if (s_grp == 'senv')
						{
							bitm_idx = *(long const *)(s_def + 0x94u);
						}
						if (bitm_idx != NONE)
						{
							uint32_t b_grp = 0;
							struct bitmap_group const *bgrp =
								(struct bitmap_group const *)preview_ce_get_tag(src, bitm_idx, &b_grp);
							if (bgrp && b_grp == 'bitm' && bgrp->bitmaps.count > 0 && bgrp->bitmaps.address)
							{
								struct bitmap_data const *bmp = (struct bitmap_data const *)bgrp->bitmaps.address;
								if (preview_decode_bitmap_texture(
										bmp,
										preview_ce_read_file_cb,
										(void *)src,
										&mesh->textures[mesh->texture_count]))
								{
									short m_slot = (short)mesh->material_count++;
									short t_slot = (short)mesh->texture_count++;
									mesh->materials[m_slot].texture_index = t_slot;
									mesh->materials[m_slot].color_source = 0;
									mesh->materials[m_slot].u_scale = (su > 0.0001f && su < 1000.0f) ? su : 1.0f;
									mesh->materials[m_slot].v_scale = (sv > 0.0001f && sv < 1000.0f) ? sv : 1.0f;
									mesh->materials[m_slot].base_map_alpha_Replicates = 0;
									shader_to_mat[shdr_idx] = m_slot;
								}
							}
						}
					}
				}
				mat_idx = shader_to_mat[shdr_idx];
			}

			if (vtx_cnt > 0 && vtx_cnt <= 16384 && tri_cnt > 0 && tri_cnt <= 65536)
			{
				unsigned long vtx_bytes = (unsigned long)vtx_cnt * 68u;
				unsigned long strip_bytes = ((unsigned long)tri_cnt + 2u) * 2u;
				unsigned long real_vtx_off = src->is_bundle
					? vtx_off
					: (src->map_hdr->model_data_offset + vtx_off);
				unsigned long real_strip_off = src->is_bundle
					? strip_off
					: (src->map_hdr->model_data_offset + src->map_hdr->model_index_data_offset + strip_off);
				uint8_t *verts68 = (uint8_t *)malloc(vtx_bytes);
				uint16_t *strip_idx = (uint16_t *)malloc(strip_bytes);

				if (verts68 && strip_idx &&
					preview_ce_read_file_cb((void *)src, real_vtx_off, vtx_bytes, verts68) &&
					preview_ce_read_file_cb((void *)src, real_strip_off, strip_bytes, strip_idx))
				{
					preview_append_part_vertices_and_strip(
						mesh,
						verts68,
						vtx_cnt,
						strip_idx,
						tri_cnt,
						world_nodes,
						model->nodes.count,
						has_local_nodes,
						loc_node_cnt,
						loc_nodes,
						mat_idx,
						u_scale,
						v_scale);
				}
				free(verts68);
				free(strip_idx);
			}
		}
	}

	free(world_nodes);
	return mesh->triangle_count > 0 ? 1 : 0;
}

int forge_ui_build_tag_preview_mesh(
	int is_installed_mode,
	int category_tab,
	int list_index,
	struct tag_injector_mesh *mesh)
{
	if (!mesh || category_tab < 0 || category_tab >= FORGE_BROWSER_NUM_CATS || list_index < 0)
		return 0;
	memset(mesh, 0, sizeof(*mesh));

	/* Mode 0: ADD TAGS (Map Tag Browser) */
	if (!is_installed_mode)
	{
		int32_t abs_idx;
		if (!s_map_browser.is_open || list_index >= s_map_browser.cat_counts[category_tab])
			return 0;
		abs_idx = s_map_browser.cat_tags[category_tab][list_index];
		if (s_map_browser.is_ce_map && s_map_browser.ce_header)
		{
			struct preview_ce_source src;
			memset(&src, 0, sizeof(src));
			src.is_bundle = 0;
			src.map_hdr = s_map_browser.ce_header;
			custom_edition_cache_bundle_source(&src.map_src);
			return build_preview_mesh_from_ce_source(
				&src,
				(uint8_t const *)s_map_browser.ce_header->instances[abs_idx].base_address,
				(uint32_t)s_map_browser.ce_header->instances[abs_idx].group_tag,
				mesh);
		}
		else if (s_map_browser.is_xbox_map && s_map_browser.xbox_tag_buf)
		{
			return build_preview_mesh_from_xbox_map_data(
				s_map_browser.xbox_fp,
				s_map_browser.xbox_tag_buf,
				s_map_browser.xbox_tag_sz,
				s_map_browser.xbox_tag_base,
				s_map_browser.xbox_inst_off,
				s_map_browser.xbox_tag_count,
				abs_idx,
				mesh);
		}
		return 0;
	}

	/* Mode 1: INSTALLED TAGS (Cache Tag Browser) */
	{
		int cat_idx = get_installed_catalog_index(category_tab, list_index);
		struct forge_catalog_entry const *e;
		long p_cnt, p;

		if (cat_idx < 0 || cat_idx >= g_forge.catalog_count)
			return 0;
		e = &g_forge.catalog[cat_idx];

		/* 1. Check Faber Tag Vault (.fbrtag + faber_vault.bin) */
		p_cnt = port_tag_profiles_count();
		for (p = 0; p < p_cnt; p++)
		{
			long t = port_tag_profiles_tag_find(p, (unsigned long)e->group_tag, e->tag_path);
			if (t >= 0)
			{
				unsigned long bundle_sz = 0;
				long bh = port_tag_profiles_bundle_open(p, t, &bundle_sz);
				if (bh != NONE && bundle_sz >= TAG_BUNDLE_HEADER_BYTES)
				{
					uint32_t magic = 0;
					if (port_tag_profiles_bundle_read_at(bh, 0, &magic, sizeof(magic)) &&
						magic == 0x68656164u && bundle_sz >= 2048u && bundle_sz <= 128u * 1024u * 1024u)
					{
						uint8_t *uncomp = (uint8_t *)malloc(bundle_sz);
						if (uncomp && port_tag_profiles_bundle_read_at(bh, 0, uncomp, bundle_sz))
						{
							char donor_path[512];
							FILE *dfp;
							port_tag_profiles_bundle_close(bh);
							platform_translate_path("z:\\cache_vault_tmp.map", donor_path, sizeof(donor_path));
							dfp = fopen(donor_path, "wb+");
							if (dfp)
							{
								uint32_t tag_off = *(uint32_t const *)(uncomp + 0x10);
								uint32_t tag_sz = *(uint32_t const *)(uncomp + 0x14);
								fwrite(uncomp, 1, bundle_sz, dfp);
								fflush(dfp);
								if (tag_off >= 2048u && tag_off + tag_sz <= bundle_sz)
								{
									uint8_t const *tbuf = uncomp + tag_off;
									uint32_t inst_addr = *(uint32_t const *)(tbuf + 0x00);
									int32_t t_cnt = *(int32_t const *)(tbuf + 0x0C);
									uint32_t t_base = inst_addr - 0x24u;
									int32_t i, root_abs = -1;
									for (i = 0; i < t_cnt && 0x24u + (uint32_t)t_cnt * 32u <= tag_sz; i++)
									{
										uint32_t io = 0x24u + (uint32_t)i * 32u;
										uint32_t grp = *(uint32_t const *)(tbuf + io + 0x00);
										uint32_t n_addr = *(uint32_t const *)(tbuf + io + 0x10);
										if (grp == e->group_tag && n_addr >= t_base && (n_addr - t_base) < tag_sz)
										{
											if (_stricmp((char const *)(tbuf + (n_addr - t_base)), e->tag_path) == 0)
											{
												root_abs = i;
												break;
											}
										}
									}
									if (root_abs >= 0)
									{
										int ok = build_preview_mesh_from_xbox_map_data(
											dfp, tbuf, tag_sz, t_base, 0x24u, t_cnt, root_abs, mesh);
										fclose(dfp);
										remove(donor_path);
										free(uncomp);
										return ok;
									}
								}
								fclose(dfp);
								remove(donor_path);
							}
							free(uncomp);
							return 0;
						}
						free(uncomp);
					}
					else
					{
						byte hdr[TAG_BUNDLE_HEADER_BYTES];
						if (port_tag_profiles_bundle_read_at(bh, 0, hdr, TAG_BUNDLE_HEADER_BYTES))
						{
							unsigned long tags_sz = tag_bundle_file_data_offset(hdr);
							if (tags_sz >= TAG_BUNDLE_HEADER_BYTES && tags_sz <= bundle_sz)
							{
								byte *b_bytes = (byte *)malloc(tags_sz);
								struct tag_bundle_loaded loaded;
								if (b_bytes && port_tag_profiles_bundle_read_at(bh, 0, b_bytes, tags_sz) &&
									tag_bundle_load_tags(b_bytes, tags_sz, bundle_sz, &loaded) == _tag_bundle_status_ok &&
									loaded.tag_count > 0)
								{
									struct preview_ce_source src;
									int ok;
									memset(&src, 0, sizeof(src));
									src.is_bundle = 1;
									src.bundle_loaded = &loaded;
									src.bundle_read.handle = bh;
									src.bundle_read.file_data_offset = loaded.file_data_offset;
									src.bundle_read.file_data_size = loaded.file_data_size;
									ok = build_preview_mesh_from_ce_source(
										&src,
										loaded.data + *(uint32_t const *)(loaded.tags + TAG_BUNDLE_TAG_ROOT_OFFSET),
										*(uint32_t const *)(loaded.tags + TAG_BUNDLE_TAG_GROUP_OFFSET),
										mesh);
									free(b_bytes);
									port_tag_profiles_bundle_close(bh);
									return ok;
								}
								free(b_bytes);
							}
						}
					}
					port_tag_profiles_bundle_close(bh);
				}
			}
		}

		/* 2. Stock Xbox cached tag: open donor Xbox map (cached across scroll steps) */
		if (open_preview_xbox_donor_for_tag(e->group_tag, e->tag_path))
		{
			int32_t i;
			char const *lookup = (e->group_tag == 'mach' &&
				_stricmp(e->tag_path, "levels\\a30\\devices\\beam emitter\\beam emitter red") == 0)
				? "levels\\a30\\devices\\beam emitter\\beam emitter"
				: e->tag_path;
			for (i = 0; i < s_preview_xbox_donor.tag_count; i++)
			{
				uint32_t io = s_preview_xbox_donor.inst_off + (uint32_t)i * 32u;
				uint32_t grp = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x00);
				uint32_t n_addr = *(uint32_t const *)(s_preview_xbox_donor.tag_buf + io + 0x10);
				if (grp == e->group_tag && n_addr >= s_preview_xbox_donor.tag_base &&
					(n_addr - s_preview_xbox_donor.tag_base) < s_preview_xbox_donor.tag_sz)
				{
					char const *tn = (char const *)(s_preview_xbox_donor.tag_buf + (n_addr - s_preview_xbox_donor.tag_base));
					if (_stricmp(tn, lookup) == 0)
					{
						return build_preview_mesh_from_xbox_map_data(
							s_preview_xbox_donor.fp,
							s_preview_xbox_donor.tag_buf,
							s_preview_xbox_donor.tag_sz,
							s_preview_xbox_donor.tag_base,
							s_preview_xbox_donor.inst_off,
							s_preview_xbox_donor.tag_count,
							i,
							mesh);
					}
				}
			}
		}
	}

	return 0;
}

/* ---------- MCC-Converted Custom Edition HUD Tag Normalization */

static int s_mcc_hud_active = 0;

int forge_is_mcc_hud_active(void)
{
	return s_mcc_hud_active;
}

static void halve_hud_placement_offset(uint8_t *pl)
{
	int16_t ox = *(int16_t const *)(pl + 0x00);
	int16_t oy = *(int16_t const *)(pl + 0x02);
	*(int16_t *)(pl + 0x00) = (int16_t)((ox >= 0 ? (ox + 1) : (ox - 1)) / 2);
	*(int16_t *)(pl + 0x02) = (int16_t)((oy >= 0 ? (oy + 1) : (oy - 1)) / 2);
}

static void halve_hud_placement_scale(uint8_t *pl)
{
	float sx = *(float const *)(pl + 0x04);
	float sy = *(float const *)(pl + 0x08);
	*(float *)(pl + 0x04) = (sx == 0.0f) ? 0.5f : (sx * 0.5f);
	*(float *)(pl + 0x08) = (sy == 0.0f) ? 0.5f : (sy * 0.5f);
}

static int hud_mto_block_follows_zoom(uint8_t const *mto_block_ptr)
{
	int32_t mto_count = *(int32_t const *)(mto_block_ptr + 0x00);
	uint8_t const *mto_arr = *(uint8_t const *const *)(mto_block_ptr + 0x04);
	int32_t m;

	if (mto_count <= 0 || mto_count > 64 || !mto_arr)
		return 0;
	for (m = 0; m < mto_count; m++)
	{
		uint8_t const *mto = mto_arr + (size_t)m * 0x1E0u;
		int32_t eff_count = *(int32_t const *)(mto + 0x168);
		uint8_t const *eff_arr = *(uint8_t const *const *)(mto + 0x16C);
		int32_t e;
		if (eff_count <= 0 || eff_count > 64 || !eff_arr)
			continue;
		for (e = 0; e < eff_count; e++)
		{
			uint8_t const *eff = eff_arr + (size_t)e * 0xDCu;
			int16_t src = *(int16_t const *)(eff + 0x44);
			if (src == 7) /* _hud_multitexture_overlay_effector_source_zoom_level */
				return 1;
		}
	}
	return 0;
}

void forge_normalize_mcc_hud_tags(void)
{
	long total = 0;
	long i;
	struct forge_cache_file_tag_instance *instances;
	int found_mcc_hud = 0;
	int already_normalized = 0;
	int32_t msg_icons_tag = NONE;

	s_mcc_hud_active = 0;
	if (!custom_edition_cache_tags_loaded())
		return;

	instances = (struct forge_cache_file_tag_instance *)cache_files_tag_instances(&total);
	if (!instances || total <= 0)
		return;

	for (i = 0; i < total; i++)
	{
		uint8_t *base = (uint8_t *)instances[i].base_address;
		if (!base || instances[i].tag_index == NONE)
			continue;
		if (instances[i].group_tag == 'hud#')
		{
			int8_t char_w = *(int8_t const *)(base + 0x10);
			int8_t scr_w = *(int8_t const *)(base + 0x11);
			uint32_t sentinel = *(uint32_t const *)(base + 0x60);

			if (sentinel == 0x4D434348u)
			{
				found_mcc_hud = 1;
				already_normalized = 1;
				break;
			}
			if (char_w >= 20 || scr_w >= 16)
			{
				int b;
				int32_t bm_tag = *(int32_t const *)(base + 0x0C);
				for (b = 0; b < 6; b++)
				{
					int8_t v = *(int8_t const *)(base + 0x10 + b);
					*(int8_t *)(base + 0x10 + b) = (int8_t)((v >= 0 ? (v + 1) : (v - 1)) / 2);
				}
				/* Clear _bitmap_group_half_hud_scale_bit (0x10) on the 4x number bitmap
				 * so hud_draw_bitmap_direct can apply a single 0.25f scale uniformly. */
				if (bm_tag != NONE)
				{
					long bm_abs = bm_tag & 0xFFFF;
					if (bm_abs >= 0 && bm_abs < total &&
						instances[bm_abs].group_tag == 'bitm' &&
						instances[bm_abs].base_address)
					{
						uint8_t *bm_def = (uint8_t *)instances[bm_abs].base_address;
						uint16_t bm_flags = *(uint16_t const *)(bm_def + 0x06);
						*(uint16_t *)(bm_def + 0x06) = (uint16_t)(bm_flags & ~0x0090u);
					}
				}
				*(uint32_t *)(base + 0x60) = 0x4D434348u;
				found_mcc_hud = 1;
			}
		}
	}

	if (!found_mcc_hud)
		return;

	s_mcc_hud_active = 1;
	if (already_normalized)
		return;

	/* First pass: locate hudg messaging_icons tag index and normalize hudg */
	for (i = 0; i < total; i++)
	{
		uint8_t *base = (uint8_t *)instances[i].base_address;
		if (!base || instances[i].tag_index == NONE || instances[i].group_tag != 'hudg')
			continue;

		/* messaging.placement at +0x24 */
		halve_hud_placement_offset(base + 0x24);

		/* messaging.messaging_icons tag_reference at +0xA4 (index at +0xB0) */
		msg_icons_tag = *(int32_t const *)(base + 0xA4 + 0x0C);

		/* waypoint offsets at +0x120 */
		{
			int w;
			for (w = 0; w < 4; w++)
			{
				float *wo = (float *)(base + 0x120 + w * 4);
				if (*wo > 20.0f)
					*wo = 20.0f;
			}
		}

		/* damage_indicators offsets at +0x310 */
		{
			int d;
			for (d = 0; d < 4; d++)
			{
				int16_t *doff = (int16_t *)(base + 0x310 + d * 2);
				*doff = (int16_t)(*doff / 2);
			}
		}
	}

	/* Second pass: normalize unhi, grhi, and wphi */
	for (i = 0; i < total; i++)
	{
		uint8_t *base = (uint8_t *)instances[i].base_address;
		uint32_t grp;
		if (!base || instances[i].tag_index == NONE)
			continue;
		grp = instances[i].group_tag;

		if (grp == 'unhi')
		{
			int32_t cnt, k;
			uint8_t *arr;

			/* unit_hud_background (+0x24), shield_panel_background (+0x8C), health_panel_background (+0x17C) */
			halve_hud_placement_offset(base + 0x24);
			halve_hud_placement_scale(base + 0x24);
			halve_hud_placement_offset(base + 0x8C);
			halve_hud_placement_scale(base + 0x8C);
			halve_hud_placement_offset(base + 0x17C);
			halve_hud_placement_scale(base + 0x17C);

			/* shield_panel_meter (+0xF4), health_panel_meter (+0x1E4) */
			halve_hud_placement_offset(base + 0xF4);
			halve_hud_placement_scale(base + 0xF4);
			halve_hud_placement_offset(base + 0x1E4);
			halve_hud_placement_scale(base + 0x1E4);

			/* motion_sensor.background (+0x26C) & motion_sensor.foreground (+0x2D4) */
			{
				int16_t ms_bg_x = *(int16_t const *)(base + 0x26C + 0x00);
				int16_t ms_bg_y = *(int16_t const *)(base + 0x26C + 0x02);
				int16_t ms_fg_x = *(int16_t const *)(base + 0x2D4 + 0x00);
				int16_t ms_fg_y = *(int16_t const *)(base + 0x2D4 + 0x02);
				int shifted_20 = (ms_bg_x == 20 && ms_bg_y == 20);

				if (shifted_20)
				{
					*(int16_t *)(base + 0x26C + 0x00) = 0;
					*(int16_t *)(base + 0x26C + 0x02) = 0;
				}
				else
				{
					halve_hud_placement_offset(base + 0x26C);
				}
				halve_hud_placement_scale(base + 0x26C);

				if (ms_fg_x == 20 && ms_fg_y == 20)
				{
					*(int16_t *)(base + 0x2D4 + 0x00) = 0;
					*(int16_t *)(base + 0x2D4 + 0x02) = 0;
				}
				else
				{
					halve_hud_placement_offset(base + 0x2D4);
				}
				halve_hud_placement_scale(base + 0x2D4);

				/* blip_placement (+0x35C) */
				{
					int16_t bx = *(int16_t const *)(base + 0x35C + 0x00);
					int16_t by = *(int16_t const *)(base + 0x35C + 0x02);
					if (shifted_20 && bx >= 95 && by >= 95)
					{
						*(int16_t *)(base + 0x35C + 0x00) = 41;
						*(int16_t *)(base + 0x35C + 0x02) = 41;
					}
					else
					{
						halve_hud_placement_offset(base + 0x35C);
					}
					halve_hud_placement_scale(base + 0x35C);
				}
			}

			/* auxilary_panel.auxilary_overlays (+0x3A4, stride 0x84) */
			cnt = *(int32_t const *)(base + 0x3A4);
			arr = *(uint8_t *const *)(base + 0x3A8);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *el = arr + (size_t)k * 0x84u;
					halve_hud_placement_offset(el);
					halve_hud_placement_scale(el);
				}
			}

			/* auxilary_meters (+0x3CC, stride 0x144) */
			cnt = *(int32_t const *)(base + 0x3CC);
			arr = *(uint8_t *const *)(base + 0x3D0);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *am = arr + (size_t)k * 0x144u;
					halve_hud_placement_offset(am + 0x14);
					halve_hud_placement_scale(am + 0x14);
					halve_hud_placement_offset(am + 0x7C);
					halve_hud_placement_scale(am + 0x7C);
				}
			}
		}
		else if (grp == 'grhi')
		{
			int32_t cnt, k;
			uint8_t *arr;

			/* background (+0x24) & total_grenades_background (+0x8C) */
			halve_hud_placement_offset(base + 0x24);
			halve_hud_placement_scale(base + 0x24);
			halve_hud_placement_offset(base + 0x8C);
			halve_hud_placement_scale(base + 0x8C);

			/* total_grenades_numbers (+0xF4) */
			halve_hud_placement_offset(base + 0xF4);

			/* overlays.items (+0x15C, stride 0x88) */
			cnt = *(int32_t const *)(base + 0x15C);
			arr = *(uint8_t *const *)(base + 0x160);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *ov = arr + (size_t)k * 0x88u;
					halve_hud_placement_offset(ov);
					halve_hud_placement_scale(ov);
				}
			}
		}
		else if (grp == 'wphi')
		{
			int32_t cnt, k;
			uint8_t *arr;

			/* statics (+0x60, stride 0xB4) */
			cnt = *(int32_t const *)(base + 0x60);
			arr = *(uint8_t *const *)(base + 0x64);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *st = arr + (size_t)k * 0xB4u;
					if (hud_mto_block_follows_zoom(st + 0x7C))
						continue;
					halve_hud_placement_offset(st + 0x24);
					halve_hud_placement_scale(st + 0x24);
				}
			}

			/* meters (+0x6C, stride 0xB4) */
			cnt = *(int32_t const *)(base + 0x6C);
			arr = *(uint8_t *const *)(base + 0x70);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *mt = arr + (size_t)k * 0xB4u;
					halve_hud_placement_offset(mt + 0x24);
					halve_hud_placement_scale(mt + 0x24);
				}
			}

			/* numbers (+0x78, stride 0xA0) */
			cnt = *(int32_t const *)(base + 0x78);
			arr = *(uint8_t *const *)(base + 0x7C);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *nm = arr + (size_t)k * 0xA0u;
					uint8_t nflags = *(uint8_t const *)(nm + 0x69);
					if (nflags & (1 << 1)) /* _hud_number_show_only_when_zoomed_bit */
						continue;
					halve_hud_placement_offset(nm + 0x24);
				}
			}

			/* crosshairs (+0x84, stride 0x68) -> items (+0x34, stride 0x6C) */
			cnt = *(int32_t const *)(base + 0x84);
			arr = *(uint8_t *const *)(base + 0x88);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *ch = arr + (size_t)k * 0x68u;
					int32_t icnt = *(int32_t const *)(ch + 0x34);
					uint8_t *iarr = *(uint8_t *const *)(ch + 0x38);
					int32_t j;
					if (icnt <= 0 || icnt > 64 || !iarr)
						continue;
					for (j = 0; j < icnt; j++)
					{
						uint8_t *ci = iarr + (size_t)j * 0x6Cu;
						float sx = *(float const *)(ci + 0x04);
						if (sx <= 0.35f)
							continue;
						halve_hud_placement_offset(ci);
						halve_hud_placement_scale(ci);
					}
				}
			}

			/* overlays (+0x90, stride 0x68) -> items (+0x34, stride 0x88) */
			cnt = *(int32_t const *)(base + 0x90);
			arr = *(uint8_t *const *)(base + 0x94);
			if (cnt > 0 && cnt <= 64 && arr)
			{
				for (k = 0; k < cnt; k++)
				{
					uint8_t *oe = arr + (size_t)k * 0x68u;
					int32_t icnt = *(int32_t const *)(oe + 0x34);
					uint8_t *iarr = *(uint8_t *const *)(oe + 0x38);
					int32_t j;
					if (icnt <= 0 || icnt > 64 || !iarr)
						continue;
					for (j = 0; j < icnt; j++)
					{
						uint8_t *oi = iarr + (size_t)j * 0x88u;
						halve_hud_placement_offset(oi);
						halve_hud_placement_scale(oi);
					}
				}
			}

			/* messaging_icon (+0x13C) */
			{
				int16_t seq_idx = *(int16_t const *)(base + 0x13C);
				int16_t w_off = *(int16_t const *)(base + 0x13E);
				int16_t ox = *(int16_t const *)(base + 0x140);
				int16_t oy = *(int16_t const *)(base + 0x142);
				uint8_t flags = *(uint8_t const *)(base + 0x14B);

				*(int16_t *)(base + 0x140) = (int16_t)(ox / 2);
				*(int16_t *)(base + 0x142) = (int16_t)(oy / 2);

				if (!(flags & 2) && msg_icons_tag != NONE && seq_idx >= 0)
				{
					long bm_abs = msg_icons_tag & 0xFFFF;
					if (bm_abs >= 0 && bm_abs < total &&
						instances[bm_abs].group_tag == 'bitm' &&
						instances[bm_abs].base_address)
					{
						uint8_t const *bm_def = (uint8_t const *)instances[bm_abs].base_address;
						int32_t seq_cnt = *(int32_t const *)(bm_def + 0x54);
						uint8_t const *seq_arr = *(uint8_t const *const *)(bm_def + 0x58);
						int32_t bmp_cnt = *(int32_t const *)(bm_def + 0x60);
						uint8_t const *bmp_arr = *(uint8_t const *const *)(bm_def + 0x64);
						if (seq_idx < seq_cnt && seq_arr && bmp_cnt > 0 && bmp_arr)
						{
							uint8_t const *seq = seq_arr + (size_t)seq_idx * 0x40u;
							int32_t spr_cnt = *(int32_t const *)(seq + 0x34);
							uint8_t const *spr_arr = *(uint8_t const *const *)(seq + 0x38);
							if (spr_cnt > 0 && spr_arr)
							{
								int16_t bm_idx = *(int16_t const *)(spr_arr + 0x00);
								float left = *(float const *)(spr_arr + 0x08);
								float right = *(float const *)(spr_arr + 0x0C);
								if (bm_idx >= 0 && bm_idx < bmp_cnt)
								{
									uint8_t const *bd = bmp_arr + (size_t)bm_idx * 0x30u;
									int16_t bw = *(int16_t const *)(bd + 0x04);
									float sprite_px = (right - left) * (float)bw * 0.25f;
									*(int16_t *)(base + 0x13E) = (int16_t)(sprite_px + (float)(w_off / 2) + 0.5f);
									*(uint8_t *)(base + 0x14B) = (uint8_t)(flags | 2); /* _hud_icon_absolute_width_bit */
								}
							}
						}
					}
				}
			}
		}
	}
}

/* ============================================================================
 * ZERO-TRUST SESSION-RAM-ONLY CUSTOM TAG STREAMING HELPERS
 * ============================================================================ */

uint32_t forge_compute_catalog_hash(uint32_t group_tag, char const *tag_path)
{
	uint32_t h = 2166136261u;
	char const *p;

	h = (h ^ ((group_tag >> 24) & 0xFFu)) * 16777619u;
	h = (h ^ ((group_tag >> 16) & 0xFFu)) * 16777619u;
	h = (h ^ ((group_tag >> 8) & 0xFFu)) * 16777619u;
	h = (h ^ (group_tag & 0xFFu)) * 16777619u;

	if (tag_path)
	{
		for (p = tag_path; *p; p++)
		{
			unsigned char c = (unsigned char)*p;
			if (c == '/')
				c = '\\';
			else if (c >= 'A' && c <= 'Z')
				c = (unsigned char)(c - 'A' + 'a');
			h = (h ^ c) * 16777619u;
		}
	}
	return h ? h : 1u;
}

int forge_zlib_compress_buffer(uint8_t const *src, uint32_t src_len, uint8_t **out_dst, uint32_t *out_dst_len)
{
	unsigned long bound;
	unsigned long comp_len;
	uint8_t *dst;

	if (!src || src_len == 0 || src_len > 16u * 1024u * 1024u || !out_dst || !out_dst_len)
		return 0;

	bound = (unsigned long)src_len + ((unsigned long)src_len >> 10) + 128ul;
	dst = (uint8_t *)malloc((size_t)bound);
	if (!dst)
		return 0;

	comp_len = bound;
	{
#undef compress2
		extern int compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
		if (compress2(dst, &comp_len, src, (unsigned long)src_len, 6) != Z_OK || comp_len == 0)
		{
			free(dst);
			return 0;
		}
	}

	*out_dst = dst;
	*out_dst_len = (uint32_t)comp_len;
	return 1;
}

int forge_zlib_uncompress_exact(uint8_t const *src, uint32_t src_len, uint8_t *dst, uint32_t exact_dst_len)
{
	unsigned long out_len;

	if (!src || src_len == 0 || !dst || exact_dst_len == 0 || exact_dst_len > 16u * 1024u * 1024u)
		return 0;

	out_len = (unsigned long)exact_dst_len;
	if (z_uncompress(dst, &out_len, src, (unsigned long)src_len) != Z_OK ||
		out_len != (unsigned long)exact_dst_len)
	{
		return 0;
	}
	return 1;
}

uint32_t forge_get_streamed_tag_ram_used(void)
{
	return s_streamed_phys_bytes;
}

static int forge_stream_tag_path_is_safe(char const *tag_path)
{
	size_t i;
	int prev_sep = 1;

	if (!tag_path || !tag_path[0])
		return 0;

	for (i = 0; tag_path[i] != '\0'; i++)
	{
		unsigned char c = (unsigned char)tag_path[i];
		if (i >= 240u)
			return 0;
		if (c < 0x20u || c > 0x7Eu)
			return 0;
		if (c == ':' || c == '|' || c == '"' || c == '*' || c == '?' || c == '<' || c == '>')
			return 0;
		if (c == '\\' || c == '/')
		{
			if (prev_sep)
				return 0;
			prev_sep = 1;
		}
		else
		{
			if (c == '.' && tag_path[i + 1] == '.')
				return 0;
			prev_sep = 0;
		}
	}
	if (prev_sep || i == 0)
		return 0;
	return 1;
}

static int forge_stream_group_is_allowed_root(uint32_t grp)
{
	return (grp == 'scen' || grp == 'vehi' || grp == 'weap' || grp == 'eqip' ||
			grp == 'mach' || grp == 'lifi' || grp == 'ctrl' || grp == 'bipd') ? 1 : 0;
}

int forge_export_custom_tag_bundle(
	uint32_t hash,
	uint8_t *out_format,
	uint32_t *out_group_tag,
	char *out_tag_path,
	size_t path_sz,
	char *out_display_name,
	size_t disp_sz,
	uint8_t **out_raw_bytes,
	uint32_t *out_raw_size)
{
	long prof_count, p, t;
	int i;

	if (!hash || !out_format || !out_group_tag || !out_tag_path || path_sz < 4 ||
		!out_display_name || disp_sz < 4 || !out_raw_bytes || !out_raw_size)
	{
		return 0;
	}

	*out_format = 0;
	*out_group_tag = 0;
	out_tag_path[0] = '\0';
	out_display_name[0] = '\0';
	*out_raw_bytes = NULL;
	*out_raw_size = 0;

	/* 1. Check enabled Faber Tag Vault custom bundles (FABB format = 1) */
	prof_count = port_tag_profiles_count();
	if (prof_count <= 0)
	{
		port_tag_profiles_scan();
		prof_count = port_tag_profiles_count();
	}
	for (p = 0; p < prof_count; p++)
	{
		long tc = port_tag_profiles_tag_count(p);
		for (t = 0; t < tc; t++)
		{
			uint32_t grp;
			char const *tname;
			unsigned long bundle_sz = 0;
			long b_hdl;
			uint8_t *raw;
			char const *opt_name;

			if (!port_tag_profiles_tag_enabled(p, t))
				continue;
			grp = (uint32_t)port_tag_profiles_tag_group(p, t);
			tname = port_tag_profiles_tag_name(p, t);
			if (!tname || !tname[0] || !forge_stream_group_is_allowed_root(grp))
				continue;
			if (forge_compute_catalog_hash(grp, tname) != hash)
				continue;

			b_hdl = port_tag_profiles_bundle_open(p, t, &bundle_sz);
			if (b_hdl == NONE)
				continue;
			if (bundle_sz < TAG_BUNDLE_HEADER_BYTES || bundle_sz > 14u * 1024u * 1024u)
			{
				port_tag_profiles_bundle_close(b_hdl);
				continue;
			}
			raw = (uint8_t *)malloc((size_t)bundle_sz);
			if (!raw || !port_tag_profiles_bundle_read_at(b_hdl, 0, raw, bundle_sz))
			{
				free(raw);
				port_tag_profiles_bundle_close(b_hdl);
				continue;
			}
			port_tag_profiles_bundle_close(b_hdl);

			opt_name = port_tag_profiles_tag_option(p, t, "name");
			*out_format = (*(uint32_t const *)raw == 0x68656164u) ? 2u : 1u;
			*out_group_tag = grp;
			snprintf(out_tag_path, path_sz, "%s", tname);
			snprintf(out_display_name, disp_sz, "%s", (opt_name && opt_name[0]) ? opt_name : tname);
			*out_raw_bytes = raw;
			*out_raw_size = (uint32_t)bundle_sz;
			return 1;
		}
	}

	/* 2. Also check session-staged ephemeral tags received in this session */
	for (i = 0; i < 32; i++)
	{
		struct forge_staged_ephemeral_tag const *st = &s_staged_ephemeral_tags[i];
		uint8_t *raw_copy;
		if (!st->active || st->expected_hash != hash || !st->raw_bytes || st->raw_size == 0)
			continue;
		raw_copy = (uint8_t *)malloc(st->raw_size);
		if (!raw_copy)
			continue;
		memcpy(raw_copy, st->raw_bytes, st->raw_size);
		*out_format = st->bundle_format;
		*out_group_tag = st->root_group;
		snprintf(out_tag_path, path_sz, "%s", st->tag_path);
		snprintf(out_display_name, disp_sz, "%s", st->clean_disp[0] ? st->clean_disp : st->tag_path);
		*out_raw_bytes = raw_copy;
		*out_raw_size = st->raw_size;
		return 1;
	}

	return 0;
}

static void forge_commit_session_ephemeral_tags_to_disk(void)
{
	int i, k;

	for (i = 0; i < 32; i++)
	{
		struct forge_staged_ephemeral_tag *st = &s_staged_ephemeral_tags[i];
		int committed_ok = 0;

		if (!st->active || st->committed || !st->raw_bytes || st->raw_size < TAG_BUNDLE_HEADER_BYTES)
			continue;

		if (st->bundle_format == 2u)
		{
			if (st->raw_size >= 2048u && *(uint32_t const *)st->raw_bytes == 0x68656164u)
			{
				long prof = 0;
				uint32_t tag_off = *(uint32_t const *)(st->raw_bytes + 0x10);
				uint32_t tag_sz = *(uint32_t const *)(st->raw_bytes + 0x14);
				unsigned long tags_size = (tag_off >= 2048u && tag_off <= st->raw_size && tag_sz <= st->raw_size - tag_off)
					? (unsigned long)(tag_off + ((tag_sz + 31u) & ~31u))
					: 2048ul;
				struct tag_bundle_file_region ext_reg;
				long reg_cnt = 0;

				if (tags_size > st->raw_size)
					tags_size = st->raw_size;
				if (st->raw_size > tags_size)
				{
					ext_reg.offset = 0;
					ext_reg.size = (unsigned long)(st->raw_size - tags_size);
					reg_cnt = 1;
				}
				else
				{
					ext_reg.offset = 0;
					ext_reg.size = 0;
				}

				port_tag_profiles_scan();
				if (port_tag_profiles_count() <= 0)
					prof = port_tag_profiles_create("Forge Custom Tags");
				else
					prof = 0;

				if (prof >= 0 &&
					port_tag_profiles_tag_add(
						prof,
						"network_stream",
						(unsigned long)st->root_group,
						st->tag_path,
						st->raw_bytes,
						st->raw_size,
						tags_size,
						(struct port_tag_store_region const *)&ext_reg,
						reg_cnt))
				{
					long t_slot = port_tag_profiles_tag_find(prof, (unsigned long)st->root_group, st->tag_path);
					if (t_slot >= 0)
					{
						port_tag_profiles_tag_set_enabled(prof, t_slot, 1);
						port_tag_profiles_tag_set_option(prof, t_slot, "name", st->clean_disp);
					}
					committed_ok = 1;
				}
			}
		}
		else if (st->bundle_format == 1u)
		{
			unsigned long tags_size = tag_bundle_file_data_offset(st->raw_bytes);
			if (tags_size >= TAG_BUNDLE_HEADER_BYTES && tags_size <= st->raw_size)
			{
				byte *copy_tags = (byte *)malloc(tags_size);
				if (copy_tags)
				{
					long prof = 0;
					struct tag_bundle_loaded loaded;
					struct tag_bundle_file_region *regions = NULL;
					long region_count = 0;
					static struct tag_bundle_file_region const no_region = { 0, 0 };

					port_tag_profiles_scan();
					if (port_tag_profiles_count() <= 0)
						prof = port_tag_profiles_create("Forge Custom Tags");
					else
						prof = 0;

					memcpy(copy_tags, st->raw_bytes, tags_size);
					if (prof >= 0 &&
						tag_bundle_load_tags_regions(
							copy_tags,
							tags_size,
							st->raw_size,
							&loaded,
							&regions,
							&region_count) == _tag_bundle_status_ok)
					{
						if (port_tag_profiles_tag_add(
								prof,
								"network_stream",
								(unsigned long)st->root_group,
								st->tag_path,
								st->raw_bytes,
								st->raw_size,
								tags_size,
								(struct port_tag_store_region const *)(regions ? regions : &no_region),
								region_count))
						{
							long t_slot = port_tag_profiles_tag_find(prof, (unsigned long)st->root_group, st->tag_path);
							if (t_slot >= 0)
							{
								port_tag_profiles_tag_set_enabled(prof, t_slot, 1);
								port_tag_profiles_tag_set_option(prof, t_slot, "name", st->clean_disp);
							}
							committed_ok = 1;
						}
					}
					free(copy_tags);
					free(regions);
				}
			}
		}

		if (committed_ok)
		{
			st->committed = 1;
			for (k = 0; k < g_forge.catalog_count; k++)
			{
				if (g_forge.catalog[k].group_tag == st->root_group &&
					_stricmp(g_forge.catalog[k].tag_path, st->tag_path) == 0 &&
					g_forge.catalog[k].source_cache_id == 4)
				{
					g_forge.catalog[k].source_cache_id = 2;
				}
			}
			for (k = 0; k < g_forge.live_instance_count; k++)
			{
				if (g_forge.live_instances[k].active &&
					g_forge.live_instances[k].group_tag == st->root_group &&
					_stricmp(g_forge.live_instances[k].tag_path, st->tag_path) == 0 &&
					g_forge.live_instances[k].source_cache_id == 4)
				{
					g_forge.live_instances[k].source_cache_id = 2;
				}
			}
			for (k = 0; k < g_forge.edit_state.placed_object_count; k++)
			{
				if (g_forge.edit_state.placed_objects[k].active &&
					g_forge.edit_state.placed_objects[k].group_tag == st->root_group &&
					_stricmp(g_forge.edit_state.placed_objects[k].tag_path, st->tag_path) == 0 &&
					g_forge.edit_state.placed_objects[k].source_cache_id == 4)
				{
					g_forge.edit_state.placed_objects[k].source_cache_id = 2;
				}
			}
		}
	}
}

long forge_install_ephemeral_tag_bundle(
	uint32_t expected_hash,
	uint8_t bundle_format,
	uint32_t root_group,
	char const *root_tag_path,
	char const *display_name,
	uint8_t const *uncomp_bytes,
	uint32_t uncomp_size,
	uint32_t *out_actual_group)
{
	long existing_hdl;
	long root_hdl = NONE;
	uint32_t actual_grp = root_group;
	uint8_t cat_id;
	char clean_disp[64];
	char prefix_buf[80];
	size_t di = 0, wi = 0;

	if (!expected_hash || (bundle_format != 1u && bundle_format != 2u))
		return NONE;
	if (!forge_stream_group_is_allowed_root(root_group))
		return NONE;
	if (!forge_stream_tag_path_is_safe(root_tag_path))
		return NONE;
	if (forge_compute_catalog_hash(root_group, root_tag_path) != expected_hash)
		return NONE;
	if (!uncomp_bytes || uncomp_size < TAG_BUNDLE_HEADER_BYTES || uncomp_size > 14u * 1024u * 1024u)
		return NONE;

	/* Never shadow or overwrite an existing base-map tag */
	existing_hdl = tag_loaded((long)root_group, root_tag_path);
	if (existing_hdl != NONE)
	{
		uint32_t base_cnt = forge_get_base_map_tag_count();
		if (base_cnt > 0 && (uint32_t)DATUM_INDEX_TO_ABSOLUTE_INDEX(existing_hdl) < base_cnt)
			return NONE;
		if (out_actual_group)
			*out_actual_group = (uint32_t)tag_get_group_tag(existing_hdl);
		return existing_hdl;
	}

	/* Enforce strict per-session RAM cap and slot headroom */
	if (s_streamed_phys_bytes > FORGE_TAG_STREAM_MAX_SESSION_BYTES ||
		uncomp_size > FORGE_TAG_STREAM_MAX_SESSION_BYTES - s_streamed_phys_bytes)
	{
		return NONE;
	}
	if (s_dyn_tags.count >= FORGE_MAX_DYNAMIC_TAGS - 8 ||
		s_forge_phys_allocs.count >= FORGE_MAX_PHYS_ALLOCS - 64)
	{
		return NONE;
	}

	if (bundle_format == 1u)
	{
		root_hdl = install_faber_bundle_from_source(
			NONE,
			uncomp_bytes,
			(unsigned long)uncomp_size,
			root_group,
			root_tag_path,
			&actual_grp,
			1);
	}
	else
	{
		root_hdl = load_native_xbox_tag_from_source(
			NULL,
			uncomp_bytes,
			uncomp_size,
			root_group,
			root_tag_path,
			&actual_grp,
			1);
	}

	if (root_hdl == NONE)
		return NONE;

	s_streamed_phys_bytes += uncomp_size;

	if (display_name)
	{
		char const *s = display_name;
		while (*s == ' ')
			s++;
		if (_strnicmp(s, "[c]", 3) == 0)
		{
			s += 3;
			while (*s == ' ')
				s++;
		}
		for (di = 0; s[di] != '\0' && wi + 1u < sizeof(clean_disp); di++)
		{
			unsigned char ch = (unsigned char)s[di];
			if (ch >= 0x20u && ch <= 0x7Eu && ch != '|')
				clean_disp[wi++] = (char)ch;
		}
	}
	clean_disp[wi] = '\0';
	if (!clean_disp[0])
	{
		char const *leaf = root_tag_path;
		char const *p;
		for (p = root_tag_path; *p; p++)
		{
			if (*p == '\\' || *p == '/')
				leaf = p + 1;
		}
		snprintf(clean_disp, sizeof(clean_disp), "%s", leaf);
	}

	if (root_group == 'weap') cat_id = _forge_cat_weapons;
	else if (root_group == 'eqip') cat_id = _forge_cat_equipment;
	else if (root_group == 'vehi') cat_id = _forge_cat_vehicles;
	else if (root_group == 'bipd') cat_id = _forge_cat_bipeds;
	else if (is_forge_door_tag(root_group, root_tag_path)) cat_id = _forge_cat_barriers_devices;
	else if (is_forge_light_beacon_emitter_tag(root_group, root_tag_path)) cat_id = _forge_cat_lights;
	else cat_id = _forge_cat_scenery;

	snprintf(prefix_buf, sizeof(prefix_buf), "[C] %s", clean_disp);
	add_catalog_entry(root_group, cat_id, 4, root_tag_path, prefix_buf);

	{
		int st_i, st_slot = -1;
		for (st_i = 0; st_i < 32; st_i++)
		{
			if (s_staged_ephemeral_tags[st_i].active &&
				s_staged_ephemeral_tags[st_i].expected_hash == expected_hash)
			{
				st_slot = st_i;
				break;
			}
			if (!s_staged_ephemeral_tags[st_i].active && st_slot < 0)
				st_slot = st_i;
		}
		if (st_slot >= 0)
		{
			uint8_t *raw_copy = (uint8_t *)malloc(uncomp_size);
			if (raw_copy)
			{
				memcpy(raw_copy, uncomp_bytes, uncomp_size);
				if (s_staged_ephemeral_tags[st_slot].raw_bytes)
					free(s_staged_ephemeral_tags[st_slot].raw_bytes);
				s_staged_ephemeral_tags[st_slot].active = 1;
				s_staged_ephemeral_tags[st_slot].committed = 0;
				s_staged_ephemeral_tags[st_slot].bundle_format = bundle_format;
				s_staged_ephemeral_tags[st_slot].cat_id = cat_id;
				s_staged_ephemeral_tags[st_slot].expected_hash = expected_hash;
				s_staged_ephemeral_tags[st_slot].root_group = root_group;
				snprintf(s_staged_ephemeral_tags[st_slot].tag_path, sizeof(s_staged_ephemeral_tags[st_slot].tag_path), "%s", root_tag_path);
				snprintf(s_staged_ephemeral_tags[st_slot].clean_disp, sizeof(s_staged_ephemeral_tags[st_slot].clean_disp), "%s", clean_disp);
				s_staged_ephemeral_tags[st_slot].raw_bytes = raw_copy;
				s_staged_ephemeral_tags[st_slot].raw_size = uncomp_size;
			}
		}
	}

	if (out_actual_group)
		*out_actual_group = actual_grp;
	return root_hdl;
}
