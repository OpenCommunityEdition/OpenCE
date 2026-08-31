/*
BITMAP_UTILITIES.C

symbols in this file:
00065B80 0030:
	_bitmap_fill (0000)
00065BB0 0050:
	_bitmap_alpha_to_rgb (0000)
00065C00 0190:
	_bitmap_clone (0000)
00065D90 0290:
	_code_00065d90 (0000)
00066020 0320:
	_code_00066020 (0000)
00066340 0190:
	_code_00066340 (0000)
000664D0 0190:
	_bitmap_fade (0000)
00066660 0470:
	_code_00066660 (0000)
00066AD0 0720:
	_code_00066ad0 (0000)
000671F0 00b0:
	_code_000671f0 (0000)
000672A0 0480:
	_code_000672a0 (0000)
00067720 00d0:
	_code_00067720 (0000)
000677F0 00d0:
	_code_000677f0 (0000)
000678C0 0230:
	_code_000678c0 (0000)
00067AF0 0110:
	_code_00067af0 (0000)
00067C00 00a0:
	_code_00067c00 (0000)
00067CA0 00b0:
	_code_00067ca0 (0000)
00067D50 0200:
	_code_00067d50 (0000)
00067F50 02d0:
	_code_00067f50 (0000)
00068220 02c0:
	_code_00068220 (0000)
000684E0 0370:
	_code_000684e0 (0000)
00068850 02c0:
	_code_00068850 (0000)
00068B10 02b0:
	_code_00068b10 (0000)
00068DC0 0030:
	_real_rgb_color_brightness (0000)
00068DF0 01f0:
	_rgb_color_to_hsv_color (0000)
00068FE0 01e0:
	_hsv_color_to_rgb_color (0000)
000691C0 0190:
	_real_rgb_color_to_real_hsv_color (0000)
00069350 0190:
	_real_hsv_color_to_real_rgb_color (0000)
000694E0 0060:
	_argb_color_to_real_argb_color (0000)
00069540 0050:
	_rgb_color_to_real_rgb_color (0000)
00069590 0090:
	_pixel32_to_real_argb_color (0000)
00069620 0070:
	_pixel32_to_real_rgb_color (0000)
00069690 00c0:
	_valid_real_rgb_color (0000)
00069750 00d0:
	_bitmap_shrink (0000)
00069820 0160:
	_bitmap_smooth (0000)
00069980 0160:
	_bitmap_sharpen (0000)
00069AE0 00a0:
	_bitmap_alpha_bleed (0000)
00069B80 0430:
	_code_00069b80 (0000)
00069FB0 0110:
	_code_00069fb0 (0000)
0006A0C0 0260:
	_code_0006a0c0 (0000)
0006A320 00e0:
	_code_0006a320 (0000)
0006A400 00d0:
	_code_0006a400 (0000)
0006A4D0 0210:
	_bitmap_compress_to_mipmap (0000)
0006A6E0 0200:
	_bitmap_uncompress_from_mipmap (0000)
0006A8E0 0220:
	_rgb_colors_interpolate (0000)
0006AB00 0160:
	_rgb_colors_interpolate_and_scale (0000)
0006AC60 00d0:
	_bitmap_height_map (0000)
0006AD30 0090:
	_bitmap_vector_map (0000)
00253D04 0004:
	_rdata_00253d04 (0000)
00253D08 003b:
	??_C@_0DL@NLJHJPHM@bitmap_get_pixel_data_size?$CIclone@ (0000)
00253D44 002e:
	??_C@_0CO@GFGADFMM@?$CD?$CD?$CD?5ERROR?5failed?5to?5allocate?5tem@ (0000)
00253D74 001c:
	??_C@_0BM@JIFNGHMH@source_bitmap?9?$DObase_address?$AA@ (0000)
00253D90 000e:
	??_C@_0O@LLKKCKIH@source_bitmap?$AA@ (0000)
00253DA0 002a:
	??_C@_0CK@FAKBAENJ@c?3?2halo?2SOURCE?2bitmaps?2bitmap_ut@ (0000)
00253DCC 0008:
	??_C@_07HJGGGODH@scale?$DO1?$AA@ (0000)
00253DD4 0025:
	??_C@_0CF@KNCPFPIC@source_bitmap?9?$DOtype?$DN?$DN_bitmap_typ@ (0000)
00253DFC 0025:
	??_C@_0CF@KMONDFLF@source_bitmap?9?$DOtype?$DN?$DN_bitmap_typ@ (0000)
00253E24 002b:
	??_C@_0CL@EEMHJEBC@source_bitmap?9?$DOtype?$DN?$DN_bitmap_typ@ (0000)
00253E50 0004:
	__real@43800000 (0000)
00253E58 0046:
	??_C@_0EG@HMKKNAIC@?$CD?$CD?$CD?5WARNING?5tried?5to?5smooth?5a?5bi@ (0000)
00253EA0 002e:
	??_C@_0CO@OJGKHOJB@?$CD?$CD?$CD?5ERROR?5failed?5to?5allocate?5tem@ (0000)
00253ED0 0014:
	??_C@_0BE@CCCNMADO@filter_coefficients?$AA@ (0000)
00253EE4 001e:
	??_C@_0BO@HBJILGDD@bitmap?9?$DOtype?$DN?$DN_bitmap_type_2d?$AA@ (0000)
00253F04 001e:
	??_C@_0BO@HAFKNMAE@bitmap?9?$DOtype?$DN?$DN_bitmap_type_3d?$AA@ (0000)
00253F24 0027:
	??_C@_0CH@PCFOMNKE@?$CD?$CD?$CD?5WARNING?5tried?5to?5smooth?5a?5cu@ (0000)
00253F4C 0024:
	??_C@_0CE@LEKDELBL@bitmap?9?$DOtype?$DN?$DN_bitmap_type_cube_@ (0000)
00253F70 000f:
	??_C@_0P@BOMHLMLK@negative_table?$AA@ (0000)
00253F80 000f:
	??_C@_0P@BHFDFLKJ@positive_table?$AA@ (0000)
00253F90 0029:
	??_C@_0CJ@IOADGLLE@?$CD?$CD?$CD?5WARNING?5tried?5to?5sharpen?5a?53@ (0000)
00253FBC 0028:
	??_C@_0CI@MDGCOAND@?$CD?$CD?$CD?5WARNING?5tried?5to?5sharpen?5a?5c@ (0000)
00253FE4 0009:
	??_C@_08HMMJDEFO@passes?$DO0?$AA@ (0000)
00253FF0 0037:
	??_C@_0DH@PFEBJHLO@?$CD?$CD?$CD?5WARNING?5tried?5to?5alpha?9bleed@ (0000)
00254028 0036:
	??_C@_0DG@LNAIIMEN@?$CD?$CD?$CD?5WARNING?5tried?5to?5use?5a?5cube?5@ (0000)
00254060 0011:
	??_C@_0BB@IHCHECOA@bump_height?$DO0?40f?$AA@ (0000)
00254074 003d:
	??_C@_0DN@DPHCPDGM@TEST_FLAG?$CIdestination_bitmap?9?$DOfl@ (0000)
002540B8 0053:
	??_C@_0FD@ENNJNLHL@MAX?$CI1?0?5destination_bitmap?9?$DOdepth@ (0000)
00254110 0054:
	??_C@_0FE@IAGIFAMH@MAX?$CI1?0?5destination_bitmap?9?$DOheigh@ (0000)
00254168 0053:
	??_C@_0FD@KJCBOGPG@MAX?$CI1?0?5destination_bitmap?9?$DOwidth@ (0000)
002541BC 002a:
	??_C@_0CK@NKHGKAJF@destination_bitmap?9?$DOtype?$DN?$DN_bitma@ (0000)
002541E8 002a:
	??_C@_0CK@NLLEMKKC@destination_bitmap?9?$DOtype?$DN?$DN_bitma@ (0000)
00254214 0030:
	??_C@_0DA@HHMLFPEA@destination_bitmap?9?$DOtype?$DN?$DN_bitma@ (0000)
00254244 0038:
	??_C@_0DI@JCJJEBL@TEST_FLAG?$CIsource_bitmap?9?$DOflags?0?5@ (0000)
00254280 004e:
	??_C@_0EO@HMAHIFHF@MAX?$CI1?0?5source_bitmap?9?$DOdepth?5?$DO?$DOso@ (0000)
002542D0 004f:
	??_C@_0EP@GHOBKDJO@MAX?$CI1?0?5source_bitmap?9?$DOheight?$DO?$DOso@ (0000)
00254320 004e:
	??_C@_0EO@JIPPLIPI@MAX?$CI1?0?5source_bitmap?9?$DOwidth?5?$DO?$DOso@ (0000)
00254370 0004:
	__real@3e991687 (0000)
00254374 0004:
	__real@3f1645a2 (0000)
00254378 0004:
	__real@3de978d5 (0000)
0025437C 0004:
	__real@477fff00 (0000)
00254380 0004:
	__real@47800000 (0000)
00254384 0004:
	__real@3e2aaaab (0000)
00254388 0016:
	??_C@_0BG@EIAAKENO@rgb?$CB?$DN?$CIrgb_color?5?$CK?$CJhsv?$AA@ (0000)
002543A0 0004:
	??_C@_03PPLPHHG@hsv?$AA@ (0000)
002543A4 0004:
	__real@37800080 (0000)
002543A8 0004:
	??_C@_03FNOJMHE@rgb?$AA@ (0000)
002543AC 0004:
	__real@37800000 (0000)
002543B0 001b:
	??_C@_0BL@CJFFBABE@rgb?$CB?$DN?$CIreal_rgb_color?5?$CK?$CJhsv?$AA@ (0000)
002543CC 0021:
	??_C@_0CB@IFIPCKKG@?$CD?$CD?$CD?5ERROR?5unupported?5bitmap?5type@ (0000)
002543F0 0028:
	??_C@_0CI@JOAHHOB@filter_size?$DM?$DN?$CIfloat?$CJMAXIMUM_FILT@ (0000)
00254418 0004:
	__real@42ff0000 (0000)
0025441C 0004:
	__real@3c008081 (0000)
00254420 000b:
	??_C@_0L@DEFNOJIM@rgb_result?$AA@ (0000)
0025442C 0010:
	??_C@_0BA@JANHNNE@rgb_upper_bound?$AA@ (0000)
0025443C 002c:
	??_C@_0CM@MJIBIDD@?$CFs?3?5assert_valid_real_rgb_color?$CI@ (0000)
00254468 0010:
	??_C@_0BA@EHGOIBMN@rgb_lower_bound?$AA@ (0000)
00254478 000a:
	??_C@_09BIGBFPOL@rgb_scale?$AA@ (0000)
00254488 0041:
	??_C@_0EB@GJHMFKH@?$CD?$CD?$CD?5WARNING?5importing?5special?9ef@ (0000)
0031BF00 0414:
	_bss_0031bf00 (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "cseries/errors.h"

/* ---------- constants */

enum
{
	_bitmap_type_2d = 0,
	_bitmap_type_3d,
	_bitmap_type_cube_map,
};

/* ---------- macros */

#define match_assert_valid_real_rgb_color(file, line, rgb) \
match_vassert( \
	file, \
	line, \
	valid_real_rgb_color(rgb), \
	csprintf( \
		temporary, \
		"%s: assert_valid_real_rgb_color(%f, %f, %f)", \
		#rgb, (*rgb).red, (*rgb).green, (*rgb).blue \
	) \
)

/* ---------- structures */

struct rgb_color
{
	word red;
	word green;
	word blue;
};

struct hsv_color
{
	word hue;
	word saturation;
	word value;
};

struct argb_color
{
	word alpha;
	word red;
	word green;
	word blue;
};

union real_hsv_color
{
	real n[3];
	struct
	{
		real hue;
		real saturation;
		real value;
	};
};

/* ---------- prototypes */

long bitmap_get_pixel_count(
	struct bitmap_data *bitmap);
long bitmap_get_pixel_data_size(
	struct bitmap_data *bitmap);
void *bitmap_mipmap_address(
	struct bitmap_data *bitmap,
	short mipmap_index);
struct bitmap_data *bitmap_2d_new(
	short width,
	short height,
	short mipmap_count,
	short format);
void bitmap_3d_slice_extract(
	struct bitmap_data *bitmap,
	short mipmap_index,
	short slice_index,
	struct bitmap_data *slice_bitmap);
void bitmap_3d_slice_insert(
	struct bitmap_data *slice_bitmap,
	struct bitmap_data *bitmap,
	short mipmap_index,
	short slice_index);
void bitmap_cube_map_face_extract(
	struct bitmap_data *bitmap,
	short mipmap_index,
	short face_index,
	struct bitmap_data *face_bitmap);
void bitmap_cube_map_face_insert(
	struct bitmap_data *face_bitmap,
	struct bitmap_data *bitmap,
	short mipmap_index,
	short face_index);
void bitmap_delete(
	struct bitmap_data *bitmap);

static void bitmap_2d_vector_map(
	struct bitmap_data *bitmap);
static void bitmap_3d_vector_map(
	struct bitmap_data *bitmap);
static void bitmap_cm_vector_map(
	struct bitmap_data *bitmap);

/* ---------- globals */

real const rdata_00253d04 = 1.0f / 65535.0f;

/* ---------- public code */

void bitmap_fill(
	struct bitmap_data *bitmap,
	pixel32 fill_color)
{
	pixel32 *pixels = bitmap_2d_address(bitmap, 0, 0, 0);
	long pixel_count = bitmap_get_pixel_count(bitmap);

	while (pixel_count-- > 0)
		*pixels++ = fill_color;
	return;
}

void bitmap_alpha_to_rgb(
	struct bitmap_data *bitmap)
{
	pixel32 *pixels = bitmap_2d_address(bitmap, 0, 0, 0);
	long pixel_count = bitmap_get_pixel_count(bitmap);

	while (pixel_count-- > 0)
	{
		/* byte is a character type, so this representation access is defined. */
		byte alpha = ((byte *)pixels)[3];
		unsigned long expanded = alpha;
		expanded = (expanded << 8) | alpha;
		expanded = (expanded << 8) | alpha;
		expanded = (expanded << 8) | alpha;
		*pixels++ = expanded;
	}
	return;
}

real real_rgb_color_brightness(
	union real_rgb_color const *color)
{
	return
		color->red * 0.299f +
		color->green * 0.587f +
		color->blue * 0.114f;
}

union real_hsv_color *real_rgb_color_to_real_hsv_color(
	union real_rgb_color const *rgb,
	union real_hsv_color *hsv)
{
	real value;
	real minimum;
	real delta;
	real saturation;

	if (rgb->green > rgb->blue)
		value = rgb->green;
	else
		value = rgb->blue;
	if (rgb->red > value)
		value = rgb->red;
	else
	{
		if (rgb->green > rgb->blue)
			value = rgb->green;
		else
			value = rgb->blue;
	}

	if (rgb->green > rgb->blue)
		minimum = rgb->blue;
	else
		minimum = rgb->green;
	if (rgb->red > minimum)
	{
		if (rgb->green > rgb->blue)
			minimum = rgb->blue;
		else
			minimum = rgb->green;
	}
	else
		minimum = rgb->red;

	delta = value - minimum;

	if (!hsv)
	{
		display_assert(
			"hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8B2,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)hsv)
	{
		display_assert(
			"rgb!=(real_rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8B3,
			TRUE);
		system_exit(-1);
	}

	hsv->value = value;
	if (value == 0.0f)
		saturation = 0.0f;
	else
		saturation = delta / value;
	hsv->saturation = saturation;

	if (saturation == 0.0f)
	{
		hsv->hue = 0.0f;
		return hsv;
	}
	if (rgb->red == value)
		hsv->hue = (rgb->green - rgb->blue) / delta;
	else if (rgb->green == value)
		hsv->hue = (rgb->blue - rgb->red) / delta + 2.0f;
	else
		hsv->hue = (rgb->red - rgb->green) / delta + 4.0f;

	hsv->hue *= 1.0f / 6.0f;
	if (hsv->hue < 0.0f)
		hsv->hue += 1.0f;
	return hsv;
}

union real_rgb_color *real_hsv_color_to_real_rgb_color(
	union real_hsv_color *hsv,
	union real_rgb_color *rgb)
{
	union real_hsv_color *source = hsv;
	real scaled_hue = source->hue * 6.0f;
	real p;
	real q;
	real t;
	long truncated_sector;
	long sector;

	if (!rgb)
	{
		display_assert(
			"rgb",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8DF,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)source)
	{
		display_assert(
			"rgb!=(real_rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8E1,
			TRUE);
		system_exit(-1);
	}

	if (source->saturation == 0.0f)
	{
		rgb->red = rgb->green = rgb->blue = source->value;
		return rgb;
	}

	truncated_sector = (long)scaled_hue;
	sector = (real)truncated_sector > scaled_hue ? truncated_sector - 1 : truncated_sector;
	scaled_hue -= sector;
	p = (1.0f - source->saturation) * source->value;
	q = (1.0f - scaled_hue * source->saturation) * source->value;
	t = (1.0f - (1.0f - scaled_hue) * source->saturation) * source->value;

	switch (sector)
	{
	case 0:
		rgb->red = source->value;
		rgb->green = t;
		rgb->blue = p;
		return rgb;
	case 1:
		rgb->red = q;
		rgb->green = source->value;
		rgb->blue = p;
		return rgb;
	case 2:
		rgb->red = p;
		rgb->green = source->value;
		rgb->blue = t;
		return rgb;
	case 3:
		rgb->red = p;
		rgb->green = q;
		rgb->blue = source->value;
		return rgb;
	case 4:
		rgb->red = t;
		rgb->green = p;
		rgb->blue = source->value;
		return rgb;
	case 5:
		rgb->red = source->value;
		rgb->green = p;
		rgb->blue = q;
		return rgb;
	default:
		return rgb;
	}
}

struct hsv_color *rgb_color_to_hsv_color(
	struct rgb_color const *rgb,
	struct hsv_color *hsv)
{
	real red = (real)(long)rgb->red * (1.0f / 65535.0f);
	real green = (real)(long)rgb->green * (1.0f / 65535.0f);
	real blue = (real)(long)rgb->blue * (1.0f / 65535.0f);
	real value;
	real minimum;
	real delta;
	real hue;
	real saturation;

	if (green > blue)
		value = green;
	else
		value = blue;
	if (red > value)
		value = red;
	else
	{
		if (green > blue)
			value = green;
		else
			value = blue;
	}

	if (green > blue)
		minimum = blue;
	else
		minimum = green;
	if (red > minimum)
	{
		if (green > blue)
			minimum = blue;
		else
			minimum = green;
	}
	else
		minimum = red;

	delta = value - minimum;

	if (!hsv)
	{
		display_assert(
			"hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x852,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)hsv)
	{
		display_assert(
			"rgb!=(rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x853,
			TRUE);
		system_exit(-1);
	}

	if (value == 0.0f)
		saturation = 0.0f;
	else
		saturation = delta / value;

	if (saturation == 0.0f)
		hue = 0.0f;
	else
	{
		if (red == value)
			hue = (green - blue) / delta;
		else if (green == value)
			hue = (blue - red) / delta + 2.0f;
		else
			hue = (red - green) / delta + 4.0f;

		hue *= 1.0f / 6.0f;
		if (hue < 0.0f)
			hue += 1.0f;
	}

	hsv->hue = (word)(long)(hue * 65536.0f);
	hsv->saturation = (word)(long)(saturation * 65535.0f);
	hsv->value = (word)(long)(value * 65535.0f);
	return hsv;
}

struct rgb_color *hsv_color_to_rgb_color(
	struct hsv_color const *hsv,
	struct rgb_color *rgb)
{
	real scaled_hue;
	real saturation;
	real value;
	real fraction;
	real p;
	real q;
	real t;
	real red;
	real green;
	real blue;
	long truncated_sector;
	long sector;

	scaled_hue = (real)(long)hsv->hue * (1.0f / 65536.0f);
	scaled_hue *= 6.0f;
	saturation = (real)(long)hsv->saturation * (1.0f / 65535.0f);
	value = (real)(long)hsv->value * (1.0f / 65535.0f);

	if (!rgb)
	{
		display_assert(
			"rgb",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x886,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)hsv)
	{
		display_assert(
			"rgb!=(rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x888,
			TRUE);
		system_exit(-1);
	}

	if (saturation == 0.0f)
		red = green = blue = value;
	else
	{
		truncated_sector = (long)scaled_hue;
		sector = (real)truncated_sector > scaled_hue ? truncated_sector - 1 : truncated_sector;
		fraction = scaled_hue - (real)sector;
		p = (1.0f - saturation) * value;
		q = (1.0f - saturation * fraction) * value;
		t = (1.0f - (1.0f - fraction) * saturation) * value;

		switch (sector)
		{
		case 0:
			red = value;
			green = t;
			blue = p;
			break;
		case 1:
			red = q;
			green = value;
			blue = p;
			break;
		case 2:
			red = p;
			green = value;
			blue = t;
			break;
		case 3:
			red = p;
			green = q;
			blue = value;
			break;
		case 4:
			red = t;
			green = p;
			blue = value;
			break;
		case 5:
			red = value;
			green = p;
			blue = q;
			break;
		}
	}

	rgb->red = (word)(long)(red * 65535.0f);
	rgb->green = (word)(long)(green * 65535.0f);
	rgb->blue = (word)(long)(blue * 65535.0f);
	return rgb;
}

union real_argb_color *argb_color_to_real_argb_color(
	struct argb_color const *source,
	union real_argb_color *result)
{
	result->alpha = (real)(long)source->alpha * rdata_00253d04;
	result->red = (real)(long)source->red * rdata_00253d04;
	result->green = (real)(long)source->green * rdata_00253d04;
	result->blue = (real)(long)source->blue * rdata_00253d04;
	return result;
}

union real_rgb_color *rgb_color_to_real_rgb_color(
	struct rgb_color const *source,
	union real_rgb_color *result)
{
	result->red = (real)(long)source->red * rdata_00253d04;
	result->green = (real)(long)source->green * rdata_00253d04;
	result->blue = (real)(long)source->blue * rdata_00253d04;
	return result;
}

union real_argb_color *pixel32_to_real_argb_color(
	pixel32 color,
	union real_argb_color *result)
{
	unsigned long alpha = color >> 24;
	unsigned long red = (color >> 16) & 0xFF;
	unsigned long green = (color >> 8) & 0xFF;
	unsigned long blue = color & 0xFF;

	result->alpha = alpha * (1.0f / 255.0f);
	result->red = red * (1.0f / 255.0f);
	result->green = green * (1.0f / 255.0f);
	result->blue = blue * (1.0f / 255.0f);
	return result;
}

union real_rgb_color *pixel32_to_real_rgb_color(
	pixel32 color,
	union real_rgb_color *result)
{
	unsigned long red = (color >> 16) & 0xFF;
	unsigned long green = (color >> 8) & 0xFF;
	unsigned long blue = color & 0xFF;

	result->red = red * (1.0f / 255.0f);
	result->green = green * (1.0f / 255.0f);
	result->blue = blue * (1.0f / 255.0f);
	return result;
}

boolean valid_real_rgb_color(
	union real_rgb_color const *color)
{
	return
		valid_real(color->red) &&
		valid_real(color->green) &&
		valid_real(color->blue) &&
		color->red>=0.f && color->red<=1.f &&
		color->green>=0.f && color->green<=1.f &&
		color->blue>=0.f && color->blue<=1.f;
}

union real_rgb_color *rgb_colors_interpolate_and_scale(
	union real_rgb_color *rgb_result,
	unsigned long flags,
	union real_argb_color const *argb_lower_bound,
	union real_argb_color const *argb_upper_bound,
	union real_rgb_color const *rgb_scale,
	real u)
{
	rgb_colors_interpolate(
		rgb_result,
		flags,
		&argb_lower_bound->rgb,
		&argb_upper_bound->rgb,
		u);

	if (rgb_scale)
	{
		match_assert_valid_real_rgb_color("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x96E, rgb_scale);

		if (argb_lower_bound->alpha>_real_epsilon ||
			argb_upper_bound->alpha>_real_epsilon)
		{
			real alpha=
				(1.f-u)*argb_lower_bound->alpha +
				u*argb_upper_bound->alpha;

			rgb_result->red= alpha*rgb_result->red + (1.f-alpha)*rgb_scale->red;
			rgb_result->green= alpha*rgb_result->green + (1.f-alpha)*rgb_scale->green;
			rgb_result->blue= alpha*rgb_result->blue + (1.f-alpha)*rgb_scale->blue;
		}
		else
		{
			rgb_result->red*= rgb_scale->red;
			rgb_result->green*= rgb_scale->green;
			rgb_result->blue*= rgb_scale->blue;
		}
	}

	match_assert_valid_real_rgb_color("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x982, rgb_result);

	return rgb_result;
}

void bitmap_vector_map(
	struct bitmap_data *bitmap)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x569, bitmap_verify(bitmap, TRUE));

	switch (bitmap->type)
	{
		case _bitmap_type_2d:
			bitmap_2d_vector_map(bitmap);
			break;

		case _bitmap_type_3d:
			bitmap_3d_vector_map(bitmap);
			break;

		case _bitmap_type_cube_map:
			bitmap_cm_vector_map(bitmap);
			break;

		default:
			match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x577, FALSE, "### ERROR unsupported bitmap type");
			break;
	}

	return;
}

/* ---------- private code */

static void bitmap_2d_vector_map(
	struct bitmap_data *bitmap)
{
	long pixel_data_size;
	pixel32 *temporary_pixels;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x583, bitmap_verify(bitmap, TRUE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x584, bitmap->type==_bitmap_type_2d);

	pixel_data_size= bitmap_get_pixel_data_size(bitmap);
	temporary_pixels= match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x587, pixel_data_size);
	if (temporary_pixels)
	{
		short x, y;

		for (y= 0; y<bitmap->height; y++)
		{
			for (x= 0; x<bitmap->width; x++)
			{
				pixel32 pixel= *(pixel32 *)bitmap_2d_address(bitmap, x, y, 0);
				real_vector3d vector;

				vector.i= ((pixel>>16)&0xFF)*(2.f/255.f) - 1.f;
				vector.j= ((pixel>>8)&0xFF)*(2.f/255.f) - 1.f;
				vector.k= (pixel&0xFF)*(2.f/255.f) - 1.f;
				normalize3d(&vector);

				temporary_pixels[y*bitmap->width + x]=
					(pixel&0xFF000000) |
					(fast_ftol((vector.i+1.f)*127.5f + 0.5f)<<16) |
					(fast_ftol((vector.j+1.f)*127.5f + 0.5f)<<8) |
					fast_ftol((vector.k+1.f)*127.5f + 0.5f);
			}
		}

		csmemcpy(bitmap_mipmap_address(bitmap, 0), temporary_pixels, pixel_data_size);
		match_free("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x5A8, temporary_pixels);
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate temporary buffer");
	}

	return;
}

static void bitmap_3d_vector_map(
	struct bitmap_data *bitmap)
{
	struct bitmap_data *slice_bitmap;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x5B7, bitmap_verify(bitmap, TRUE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x5B8, bitmap->type==_bitmap_type_3d);

	slice_bitmap= bitmap_2d_new(bitmap->width, bitmap->height, 0, bitmap->format);
	if (slice_bitmap && slice_bitmap->base_address)
	{
		short slice_index;

		for (slice_index= 0; slice_index<(short)bitmap->depth; slice_index++)
		{
			bitmap_3d_slice_extract(bitmap, 0, slice_index, slice_bitmap);
			bitmap_2d_vector_map(slice_bitmap);
			bitmap_3d_slice_insert(slice_bitmap, bitmap, 0, slice_index);
		}
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate temporary bitmap");
	}

	bitmap_delete(slice_bitmap);

	return;
}

static void bitmap_cm_vector_map(
	struct bitmap_data *bitmap)
{
	struct bitmap_data *face_bitmap;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x5E5, bitmap_verify(bitmap, TRUE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x5E6, bitmap->type==_bitmap_type_cube_map);

	face_bitmap= bitmap_2d_new(bitmap->width, bitmap->height, 0, bitmap->format);
	if (face_bitmap && face_bitmap->base_address)
	{
		short face_index;

		for (face_index= 0; face_index<NUMBER_OF_FACES_PER_CUBE; face_index++)
		{
			bitmap_cube_map_face_extract(bitmap, 0, face_index, face_bitmap);
			bitmap_2d_vector_map(face_bitmap);
			bitmap_cube_map_face_insert(face_bitmap, bitmap, 0, face_index);
		}
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate temporary bitmap");
	}

	bitmap_delete(face_bitmap);

	return;
}
