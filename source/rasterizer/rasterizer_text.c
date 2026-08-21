/*
RASTERIZER_TEXT.C

symbols in this file:
00172E80 0010:
	_code_00172e80 (0000)
00172E90 0010:
	_code_00172e90 (0000)
00172EA0 0090:
	_rasterizer_text_cache_initialize (0000)
00172F30 0010:
	_rasterizer_text_set_shadow_color (0000)
00172F40 0030:
	_rasterizer_text_cache_flush (0000)
00172F70 0030:
	_rasterizer_text_cache_dispose (0000)
00172FA0 0020:
	_code_00172fa0 (0000)
00172FC0 00b0:
	_code_00172fc0 (0000)
00173070 0060:
	_code_00173070 (0000)
001730D0 0380:
	_code_001730d0 (0000)
00173450 00f0:
	_code_00173450 (0000)
00173540 0170:
	_code_00173540 (0000)
001736B0 0200:
	_rasterizer_draw_string (0000)
001738B0 0200:
	_rasterizer_draw_unicode_string (0000)
0029EEE0 0033:
	??_C@_0DD@DKOHMJNA@?$CD?$CD?$CD?5ERROR?5failed?5to?5initialize?5h@ (0000)
0029EF14 0026:
	??_C@_0CG@HPKDNNGC@?$CBhardware_character_cache?4initia@ (0000)
0029EF3C 002c:
	??_C@_0CM@KJINBGGM@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029EF68 0009:
	??_C@_08KDNNBGOA@x0?5?$CG?$CG?5y0?$AA@ (0000)
0029EF78 0054:
	??_C@_0FE@BCPFIEIE@hardware_character_index?$DO?$DN0?5?$CG?$CG?5h@ (0000)
0029EFCC 0025:
	??_C@_0CF@POBHCEMM@hardware_character_cache?4initial@ (0000)
0029EFF4 0026:
	??_C@_0CG@JHCOKPHL@font?5cache?5overwrote?5character?5i@ (0000)
0029F01C 0013:
	??_C@_0BD@PIEBJAO@hardware_character?$AA@ (0000)
0029F030 0046:
	??_C@_0EG@CBPAFGHN@font_character?9?$DObitmap_height?$DM?$DNH@ (0000)
0029F078 0044:
	??_C@_0EE@KGLMBKON@font_character?9?$DObitmap_width?$DM?$DNHA@ (0000)
0029F0C0 0068:
	??_C@_0GI@IDEJCHPO@font_character?$DN?$DNhardware_charact@ (0000)
0029F128 0074:
	??_C@_0HE@KFKECHAF@font_character?9?$DOhardware_charact@ (0000)
0030D4D0 0002:
	_data_0030d4d0 (0000)
004B82C0 0816:
	_bss_004b82c0 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "bitmaps/bitmap_group.h"
#include "math/integer_math.h"

/* ---------- constants */

enum
{
	HARDWARE_CHARACTER_CACHE_BITMAP_WIDTH = 128,
	HARDWARE_CHARACTER_CACHE_BITMAP_HEIGHT = 128,
	MAXIMUM_HARDWARE_CHARACTERS = 256,
};

/* ---------- macros */

/* ---------- structures */

struct font_character
{
	byte unused[0xC];
	short hardware_character_index;
};

struct hardware_character
{
	struct font_character *character;
	short x;
	short y;
};

struct hardware_character_cache
{
	boolean initialized;
	byte unused1;
	unsigned short read_index;
	unsigned short write_index;
	short x;
	short y;
	short maximum_character_height;
	struct bitmap_data *bitmap;
	struct hardware_character characters[MAXIMUM_HARDWARE_CHARACTERS];
	pixel32 shadow_color;
	short unused814;
};

/* ---------- prototypes */

void bitmap_delete(
	struct bitmap_data *bitmap);

/* ---------- globals */

extern struct hardware_character_cache bss_004b82c0;

/* ---------- public code */

void code_00172e80(
	void)
{
	return;
}

void code_00172e90(
	void)
{
	return;
}

void
rasterizer_text_set_shadow_color(
	pixel32 shadow_color)
{
	bss_004b82c0.shadow_color = shadow_color;

	return;
}

void
rasterizer_text_cache_flush(
	void)
{
	struct hardware_character *hardware_character;
	long hardware_character_count;

	if (bss_004b82c0.initialized)
	{
		hardware_character = bss_004b82c0.characters;
		hardware_character_count = MAXIMUM_HARDWARE_CHARACTERS;
		do
		{
			if (hardware_character->character)
				hardware_character->character->hardware_character_index = NONE;
			hardware_character->character = NULL;
			hardware_character++;
		} while (--hardware_character_count);
	}

	return;
}

void
rasterizer_text_cache_dispose(
	void)
{
	if (bss_004b82c0.initialized)
	{
		rasterizer_text_cache_flush();
		bitmap_delete(bss_004b82c0.bitmap);
		bss_004b82c0.initialized = FALSE;
	}

	return;
}

/* ---------- private code */

struct bitmap_data *
code_00172fa0(
	void)
{
	return bss_004b82c0.initialized ? bss_004b82c0.bitmap : NULL;
}
