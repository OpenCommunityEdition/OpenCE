/*
XBOX_TEXTURE_CACHE.C

symbols in this file:
001AE410 0020:
	_texture_cache_delete (0000)
001AE430 0010:
	_texture_cache_open (0000)
001AE440 0010:
	_texture_cache_idle (0000)
001AE450 0090:
	_texture_cache_bitmap_new (0000)
001AE4E0 0040:
	_texture_cache_bitmap_delete (0000)
001AE520 00e0:
	_texture_cache_steal_memory (0000)
001AE600 0060:
	_texture_cache_return_memory (0000)
001AE660 0030:
	_code_001ae660 (0000)
001AE690 0090:
	_bitmap_format_to_d3d_format (0000)
001AE720 0090:
	_bitmap_format_to_d3d_linear_format (0000)
001AE7B0 0030:
	_code_001ae7b0 (0000)
001AE7E0 0010:
	_code_001ae7e0 (0000)
001AE7F0 0010:
	_code_001ae7f0 (0000)
001AE800 0010:
	_code_001ae800 (0000)
001AE810 0010:
	_code_001ae810 (0000)
001AE820 0020:
	_texture_cache_flush (0000)
001AE840 0040:
	_code_001ae840 (0000)
001AE880 00a0:
	_code_001ae880 (0000)
001AE920 0150:
	_code_001ae920 (0000)
001AEA70 0100:
	_code_001aea70 (0000)
001AEB70 00b0:
	_texture_cache_new (0000)
001AEC20 0050:
	_texture_cache_close (0000)
001AEC70 00e0:
	_code_001aec70 (0000)
001AED50 0310:
	_texture_cache_debug_render (0000)
001AF060 01f0:
	__texture_cache_bitmap_get_hardware_format (0000)
002A7BD0 0090:
	_rdata_002a7bd0 (0000)
002A7C60 002e:
	??_C@_0CO@EKKOPCBA@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
002A7C90 002a:
	??_C@_0CK@BDDINKJM@c?3?2halo?2SOURCE?2cache?2xbox_textur@ (0000)
002A7CBC 002a:
	??_C@_0CK@MBCIMJKB@?$CBxbox_texture_cache_globals?4stol@ (0000)
002A7CE8 0017:
	??_C@_0BH@KHBOCJGF@remaining_page_count?$DO0?$AA@ (0000)
002A7D00 0029:
	??_C@_0CJ@MMJPDHMC@xbox_texture_cache_globals?4stole@ (0000)
002A7D2C 0014:
	??_C@_0BE@ILIIMHBE@table?$FLformat?$FN?$CB?$DNNONE?$AA@ (0000)
002A7D40 0030:
	??_C@_0DA@KFGIBCJF@texture?9?$DObitmap?9?$DOcache_block_ind@ (0000)
002A7D70 0028:
	??_C@_0CI@OLENPGPB@xbox_texture_cache_globals?4base_@ (0000)
002A7D98 0021:
	??_C@_0CB@NHIMPHNC@xbox_texture_cache_globals?4cache@ (0000)
002A7DBC 0013:
	??_C@_0BD@GPJOJANJ@xbox?5texture?5cache?$AA@ (0000)
002A7DD0 0024:
	??_C@_0CE@IHAAKOAF@xbox_texture_cache_globals?4textu@ (0000)
002A7DF4 000d:
	??_C@_0N@HJPPIHML@xbox?5texture?$AA@ (0000)
002A7E04 0025:
	??_C@_0CF@LJFHGILO@new_texture_index?$DN?$DNcache_block_i@ (0000)
002A7E2C 000b:
	??_C@_0L@PCGBDGNI@?$HMt?$CFd?$HMt?$CFs?$CFs?$AA@ (0000)
002A7E38 0044:
	??_C@_0EE@BCDFBEJK@YOU?5GOT?5STABBED?$CB?$CB?$CB?$CB?5double?9click@ (0000)
004D1198 1618:
	_bss_004d1198 (0000)
	_texture_cache_debug_options (1610)
	_debug_texture_cache (1612)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cache/physical_memory_map.h"
#include "memory/data.h"
#include "memory/lruv_cache.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	_bitmap_cached_bit = 7,
	XBOX_TEXTURE_CACHE_PAGE_COUNT = 0x580,
	XBOX_TEXTURE_CACHE_PAGE_SIZE_BITS = 14,
	XBOX_TEXTURE_CACHE_ENTRY_SIZE = 0x20,
	XBOX_TEXTURE_CACHE_SIZE = 0x1600000,
	XBOX_TEXTURE_CACHE_PROTECTION = 0x404,
};

/* ---------- macros */

/* ---------- structures */

struct bitmap_data
{
	unsigned long bitmap_class;
	short width;
	short height;
	unsigned short depth;
	short type;
	short format;
	unsigned short flags;
	short registration_point_x;
	short registration_point_y;
	unsigned short mipmap_count;
	unsigned short reserved16;
	unsigned long pixel_data_offset;
	unsigned long pixel_data_size;
	long bitmap_tag_index;
	long cache_block_index;
	void *hardware_format;
	void *base_address;
};

struct xbox_bitmap_group_prefix
{
	byte reserved0000[0x30];
	struct tag_data processed_pixel_data;
};

struct xbox_texture_cache_globals_prefix
{
	byte reserved0000[0x1600];
	struct data_array *textures;
	void *base_address;
	struct lruv_cache *cache;
	boolean stolen_memory;
	byte reserved160D[3];
};

typedef char verify_bitmap_data_flags_offset[
	offsetof(struct bitmap_data, flags) == 0xE ? 1 : -1];
typedef char verify_bitmap_data_pixel_data_offset_offset[
	offsetof(struct bitmap_data, pixel_data_offset) == 0x18 ? 1 : -1];
typedef char verify_bitmap_data_pixel_data_size_offset[
	offsetof(struct bitmap_data, pixel_data_size) == 0x1C ? 1 : -1];
typedef char verify_bitmap_data_bitmap_tag_index_offset[
	offsetof(struct bitmap_data, bitmap_tag_index) == 0x20 ? 1 : -1];
typedef char verify_bitmap_data_cache_block_index_offset[
	offsetof(struct bitmap_data, cache_block_index) == 0x24 ? 1 : -1];
typedef char verify_bitmap_data_hardware_format_offset[
	offsetof(struct bitmap_data, hardware_format) == 0x28 ? 1 : -1];
typedef char verify_bitmap_data_base_address_offset[
	offsetof(struct bitmap_data, base_address) == 0x2C ? 1 : -1];
typedef char verify_bitmap_data_size[
	sizeof(struct bitmap_data) == 0x30 ? 1 : -1];
typedef char verify_xbox_bitmap_group_pixel_data_file_offset[
	(offsetof(
		struct xbox_bitmap_group_prefix,
		processed_pixel_data) +
	 offsetof(struct tag_data, file_offset)) == 0x38 ? 1 : -1];

typedef char verify_xbox_texture_cache_textures_offset[
	offsetof(
		struct xbox_texture_cache_globals_prefix,
		textures) == 0x1600 ? 1 : -1];
typedef char verify_xbox_texture_cache_base_address_offset[
	offsetof(
		struct xbox_texture_cache_globals_prefix,
		base_address) == 0x1604 ? 1 : -1];
typedef char verify_xbox_texture_cache_cache_offset[
	offsetof(
		struct xbox_texture_cache_globals_prefix,
		cache) == 0x1608 ? 1 : -1];
typedef char verify_xbox_texture_cache_stolen_memory_offset[
	offsetof(
		struct xbox_texture_cache_globals_prefix,
		stolen_memory) == 0x160C ? 1 : -1];
typedef char verify_xbox_texture_cache_globals_prefix_size[
	sizeof(struct xbox_texture_cache_globals_prefix) == 0x1610 ? 1 : -1];

/* ---------- prototypes */

int __stdcall D3DDevice_IsBusy(
	void);
void __stdcall D3DDevice_KickPushBuffer(
	void);
void __stdcall XPhysicalProtect(
	void *address,
	unsigned long size,
	unsigned long protection);

long bitmap_get_pixel_data_size(
	struct bitmap_data *bitmap);

boolean code_001ae840(
	long block_index);
void code_001ae880(
	long block_index);

/* ---------- globals */

extern struct xbox_texture_cache_globals_prefix bss_004d1198;

#define xbox_texture_cache_globals bss_004d1198

/* ---------- public code */

void texture_cache_delete(
	void)
{
	data_dispose(bss_004d1198.textures);
	lruv_delete(bss_004d1198.cache);

	return;
}

void texture_cache_open(
	void)
{
	data_make_valid(bss_004d1198.textures);

	return;
}

void texture_cache_idle(
	void)
{
	lruv_idle(bss_004d1198.cache);

	return;
}

void texture_cache_bitmap_new(
	long bitmap_tag_index,
	struct bitmap_data *bitmap)
{
	struct xbox_bitmap_group_prefix *bitmap_group;

	match_assert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		157,
		!TEST_FLAG(bitmap->flags, _bitmap_cached_bit));
	SET_FLAG(bitmap->flags, _bitmap_cached_bit, TRUE);
	bitmap->cache_block_index = NONE;
	bitmap->base_address = NULL;
	bitmap->hardware_format = NULL;
	bitmap_group = tag_get('bitm', bitmap_tag_index);
	bitmap->pixel_data_offset +=
		bitmap_group->processed_pixel_data.file_offset;
	bitmap->pixel_data_size = bitmap_get_pixel_data_size(bitmap);
	bitmap->bitmap_tag_index = bitmap_tag_index;
	bitmap->base_address = NULL;
	bitmap->hardware_format = NULL;
	bitmap->cache_block_index = NONE;

	return;
}

void texture_cache_bitmap_delete(
	struct bitmap_data *bitmap)
{
	long cache_block_index;

	if (TEST_FLAG(bitmap->flags, _bitmap_cached_bit))
	{
		cache_block_index = bitmap->cache_block_index;
		if (cache_block_index != NONE)
		{
			lruv_block_delete(
				xbox_texture_cache_globals.cache,
				cache_block_index);
		}
		SET_FLAG(bitmap->flags, _bitmap_cached_bit, FALSE);
		bitmap->cache_block_index = NONE;
		bitmap->base_address = NULL;
	}

	return;
}

void texture_cache_return_memory(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		345,
		xbox_texture_cache_globals.stolen_memory);
	lruv_resize(
		xbox_texture_cache_globals.cache,
		XBOX_TEXTURE_CACHE_PAGE_COUNT);
	XPhysicalProtect(
		physical_memory_get_texture_cache_base_address(),
		XBOX_TEXTURE_CACHE_SIZE,
		XBOX_TEXTURE_CACHE_PROTECTION);
	xbox_texture_cache_globals.stolen_memory = FALSE;

	return;
}

void texture_cache_flush(
	void)
{
	D3DDevice_KickPushBuffer();
	D3DDevice_IsBusy();
	lruv_flush(xbox_texture_cache_globals.cache);

	return;
}

void texture_cache_new(
	void)
{
	xbox_texture_cache_globals.textures = data_new(
		"xbox texture",
		XBOX_TEXTURE_CACHE_PAGE_COUNT,
		XBOX_TEXTURE_CACHE_ENTRY_SIZE);
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		98,
		xbox_texture_cache_globals.textures != NULL,
		"xbox_texture_cache_globals.textures");
	xbox_texture_cache_globals.cache = lruv_new(
		"xbox texture cache",
		XBOX_TEXTURE_CACHE_PAGE_COUNT,
		XBOX_TEXTURE_CACHE_PAGE_SIZE_BITS,
		XBOX_TEXTURE_CACHE_PAGE_COUNT,
		code_001ae880,
		code_001ae840);
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		102,
		xbox_texture_cache_globals.cache != NULL,
		"xbox_texture_cache_globals.cache");
	xbox_texture_cache_globals.base_address =
		physical_memory_get_texture_cache_base_address();
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		105,
		xbox_texture_cache_globals.base_address != NULL,
		"xbox_texture_cache_globals.base_address");

	return;
}

void texture_cache_close(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\cache\\xbox_texture_cache.c",
		133,
		!xbox_texture_cache_globals.stolen_memory);
	texture_cache_flush();
	data_make_invalid(xbox_texture_cache_globals.textures);

	return;
}

/* ---------- private code */
