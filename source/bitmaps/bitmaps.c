/* ---------- headers */

#include "cseries.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/s3tc/s3tc.h"
#include "cseries/errors.h"
#include "math/integer_math.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"

/* ---------- constants */

enum
{
	MAXIMUM_BITMAP_WIDTH = 30000,
	MAXIMUM_BITMAP_HEIGHT = 30000,
	MAXIMUM_BITMAP_DEPTH = 256,
};

enum
{
	_bitmap_type_2d,
	_bitmap_type_3d,
	_bitmap_type_cube_map,
	NUMBER_OF_BITMAP_TYPES,
};

enum
{
	_bitmap_format_a8,
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
	NUMBER_OF_BITMAP_FORMATS,

	FIRST_COMPRESSED_BITMAP_FORMAT = _bitmap_format_dxt1,
	LAST_COMPRESSED_BITMAP_FORMAT = _bitmap_format_dxt5,
};

enum
{
	_bitmap_has_power_of_two_dimensions_bit,
	_bitmap_compressed_bit,
	_bitmap_palettized_bit,
	_bitmap_swizzled_bit,
	_bitmap_linear_bit,
	_bitmap_v16u16_bit,
	_bitmap_allocated_bit,
	_bitmap_cached_bit,
	NUMBER_OF_BITMAP_FLAGS,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

long bitmap_mipmap_get_pixel_count(
	struct bitmap_data *bitmap,
	short mipmap_index);
static boolean bitmap_format_type_valid_width(
	short format,
	short type,
	short width);
static boolean bitmap_format_type_valid_height(
	short format,
	short type,
	short height);
static boolean bitmap_format_type_valid_depth(
	short format,
	short type,
	short depth);

/* ---------- globals */

pixel32 global_vector_palette[NUMBER_OF_ENTRIES_IN_PALETTE] =
{
	0xFF7A19CC, 0xFF7E19CC, 0xFF8019CC, 0xFF8119CC, 0xFF8519CC, 0xFF742FE2, 0xFF7A2FE2, 0xFF7E2FE2,
	0xFF802FE2, 0xFF812FE2, 0xFF852FE2, 0xFF8B2FE2, 0xFF6B42ED, 0xFF7442EE, 0xFF7A42EF, 0xFF7E42EF,
	0xFF8042EF, 0xFF8142EF, 0xFF8542EF, 0xFF8B42EE, 0xFF9442ED, 0xFF6052F2, 0xFF6B52F5, 0xFF7452F6,
	0xFF7A52F7, 0xFF7E52F7, 0xFF8052F7, 0xFF8152F7, 0xFF8552F7, 0xFF8B52F6, 0xFF9452F5, 0xFF9F52F2,
	0xFF5260F2, 0xFF6060F7, 0xFF6B60F9, 0xFF7460FB, 0xFF7A60FB, 0xFF7E60FB, 0xFF8060FB, 0xFF8160FB,
	0xFF8560FB, 0xFF8B60FB, 0xFF9460F9, 0xFF9F60F7, 0xFFAD60F2, 0xFF426BED, 0xFF526BF5, 0xFF606BF9,
	0xFF6B6BFC, 0xFF746BFD, 0xFF7A6BFD, 0xFF7E6BFD, 0xFF806BFD, 0xFF816BFD, 0xFF856BFD, 0xFF8B6BFD,
	0xFF946BFC, 0xFF9F6BF9, 0xFFAD6BF5, 0xFFBD6BED, 0xFF2F74E2, 0xFF4274EE, 0xFF5274F6, 0xFF6074FB,
	0xFF6B74FD, 0xFF7474FE, 0xFF7A74FE, 0xFF7E74FE, 0xFF8074FE, 0xFF8174FE, 0xFF8574FE, 0xFF8B74FE,
	0xFF9474FD, 0xFF9F74FB, 0xFFAD74F6, 0xFFBD74EE, 0xFFD074E2, 0xFF197ACC, 0xFF2F7AE2, 0xFF427AEF,
	0xFF527AF7, 0xFF607AFB, 0xFF6B7AFD, 0xFF747AFE, 0xFF7A7AFF, 0xFF7E7AFF, 0xFF807AFF, 0xFF817AFF,
	0xFF857AFF, 0xFF8B7AFE, 0xFF947AFD, 0xFF9F7AFB, 0xFFAD7AF7, 0xFFBD7AEF, 0xFFD07AE2, 0xFFE57ACC,
	0xFF197ECC, 0xFF2F7EE2, 0xFF427EEF, 0xFF527EF7, 0xFF607EFB, 0xFF6B7EFD, 0xFF747EFE, 0xFF7A7EFF,
	0xFF7E7EFF, 0xFF807EFF, 0xFF817EFF, 0xFF857EFF, 0xFF8B7EFE, 0xFF947EFD, 0xFF9F7EFB, 0xFFAD7EF7,
	0xFFBD7EEF, 0xFFD07EE2, 0xFFE57ECC, 0xFF1980CC, 0xFF2F80E2, 0xFF4280EF, 0xFF5280F7, 0xFF6080FB,
	0xFF6B80FD, 0xFF7480FE, 0xFF7A80FF, 0xFF7E80FF, 0xFF8080FF, 0xFF8180FF, 0xFF8580FF, 0xFF8B80FE,
	0xFF9480FD, 0xFF9F80FB, 0xFFAD80F7, 0xFFBD80EF, 0xFFD080E2, 0xFFE580CC, 0xFF1981CC, 0xFF2F81E2,
	0xFF4281EF, 0xFF5281F7, 0xFF6081FB, 0xFF6B81FD, 0xFF7481FE, 0xFF7A81FF, 0xFF7E81FF, 0xFF8081FF,
	0xFF8181FF, 0xFF8581FF, 0xFF8B81FE, 0xFF9481FD, 0xFF9F81FB, 0xFFAD81F7, 0xFFBD81EF, 0xFFD081E2,
	0xFFE581CC, 0xFF1985CC, 0xFF2F85E2, 0xFF4285EF, 0xFF5285F7, 0xFF6085FB, 0xFF6B85FD, 0xFF7485FE,
	0xFF7A85FF, 0xFF7E85FF, 0xFF8085FF, 0xFF8185FF, 0xFF8585FF, 0xFF8B85FE, 0xFF9485FD, 0xFF9F85FB,
	0xFFAD85F7, 0xFFBD85EF, 0xFFD085E2, 0xFFE585CC, 0xFF2F8BE2, 0xFF428BEE, 0xFF528BF6, 0xFF608BFB,
	0xFF6B8BFD, 0xFF748BFE, 0xFF7A8BFE, 0xFF7E8BFE, 0xFF808BFE, 0xFF818BFE, 0xFF858BFE, 0xFF8B8BFE,
	0xFF948BFD, 0xFF9F8BFB, 0xFFAD8BF6, 0xFFBD8BEE, 0xFFD08BE2, 0xFF4294ED, 0xFF5294F5, 0xFF6094F9,
	0xFF6B94FC, 0xFF7494FD, 0xFF7A94FD, 0xFF7E94FD, 0xFF8094FD, 0xFF8194FD, 0xFF8594FD, 0xFF8B94FD,
	0xFF9494FC, 0xFF9F94F9, 0xFFAD94F5, 0xFFBD94ED, 0xFF529FF2, 0xFF609FF7, 0xFF6B9FF9, 0xFF749FFB,
	0xFF7A9FFB, 0xFF7E9FFB, 0xFF809FFB, 0xFF819FFB, 0xFF859FFB, 0xFF8B9FFB, 0xFF949FF9, 0xFF9F9FF7,
	0xFFAD9FF2, 0xFF60ADF2, 0xFF6BADF5, 0xFF74ADF6, 0xFF7AADF7, 0xFF7EADF7, 0xFF80ADF7, 0xFF81ADF7,
	0xFF85ADF7, 0xFF8BADF6, 0xFF94ADF5, 0xFF9FADF2, 0xFF6BBDED, 0xFF74BDEE, 0xFF7ABDEF, 0xFF7EBDEF,
	0xFF80BDEF, 0xFF81BDEF, 0xFF85BDEF, 0xFF8BBDEE, 0xFF94BDED, 0xFF74D0E2, 0xFF7AD0E2, 0xFF7ED0E2,
	0xFF80D0E2, 0xFF81D0E2, 0xFF85D0E2, 0xFF8BD0E2, 0xFF7AE5CC, 0xFF7EE5CC, 0xFF80E5CC, 0xFF81E5CC,
	0xFF85E5CC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x008080FF,
};

static char const *bitmap_type_string_table[NUMBER_OF_BITMAP_TYPES + 1] =
{
	"2d texture",
	"3d texture",
	"cube map",
	NULL,
};

static char const *bitmap_format_string_table[NUMBER_OF_BITMAP_FORMATS + 1] =
{
	"alpha",
	"intensity",
	"combined alpha-intensity",
	"separate alpha-intensity",
	"",
	"",
	"high-color",
	"r6g5b5",
	"high-color with 1-bit alpha",
	"high-color with alpha",
	"true-color",
	"true-color with alpha",
	"",
	"",
	"compressed with color-key transparency",
	"compressed with explicit alpha",
	"compressed with interpolated alpha",
	"palettized bump map",
	NULL,
};

static char const bitmap_format_bits_per_pixel_table[NUMBER_OF_BITMAP_FORMATS + 1] =
{
	8,
	8,
	8,
	16,
	0,
	0,
	16,
	0,
	16,
	16,
	32,
	32,
	0,
	0,
	4,
	8,
	8,
	8,
	NONE,
};

/* ---------- public code */

char const *bitmap_type_get_string(
	short type)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 80, type>=0 && type<NUMBER_OF_BITMAP_TYPES);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 81, bitmap_type_string_table[NUMBER_OF_BITMAP_TYPES]==NULL);

	return bitmap_type_string_table[type];
}

char const *bitmap_format_get_string(
	short format)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 134, format>=0 && format<NUMBER_OF_BITMAP_FORMATS);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 135, bitmap_format_string_table[NUMBER_OF_BITMAP_FORMATS]==NULL);

	return bitmap_format_string_table[format];
}

short bitmap_format_get_bits_per_pixel(
	short format)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xA6, format>=0 && format<NUMBER_OF_BITMAP_FORMATS);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xA7, bitmap_format_bits_per_pixel_table[format]!=0);

	return bitmap_format_bits_per_pixel_table[format];
}

struct bitmap_data *bitmap_2d_new(
	short width,
	short height,
	short mipmap_count,
	short format)
{
	struct bitmap_data *bitmap;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xB5, bitmap_format_type_valid_width (format, _bitmap_type_2d, width));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xB6, bitmap_format_type_valid_height(format, _bitmap_type_2d, height));

	bitmap = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xB8, sizeof(struct bitmap_data));
	if (bitmap)
	{
		csmemset(bitmap, 0, sizeof(struct bitmap_data));
		bitmap->signature = BITMAP_GROUP_TAG;
		bitmap->width = width;
		bitmap->height = height;
		bitmap->depth = 1;
		bitmap->type = _bitmap_type_2d;
		bitmap->format = format;
		bitmap->flags = FLAG(_bitmap_allocated_bit);
		bitmap->mipmap_count = mipmap_count;
		if ((width&(width-1))==0 && (height&(height-1))==0)
		{
			SET_FLAG(bitmap->flags, _bitmap_has_power_of_two_dimensions_bit, TRUE);
		}
		if (format>=FIRST_COMPRESSED_BITMAP_FORMAT && format<=LAST_COMPRESSED_BITMAP_FORMAT)
		{
			SET_FLAG(bitmap->flags, _bitmap_compressed_bit, TRUE);
		}
		if (format==_bitmap_format_p8_bump)
		{
			SET_FLAG(bitmap->flags, _bitmap_palettized_bit, TRUE);
		}

		bitmap->base_address = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xD5, bitmap_get_pixel_data_size(bitmap));
		if (bitmap->base_address)
		{
			match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xD9, bitmap_verify(bitmap, FALSE));
		}
		else
		{
			error(_error_silent, "### ERROR failed to allocate bitmap->base_address");
		}
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate bitmap");
	}

	return bitmap;
}

struct bitmap_data *bitmap_3d_new(
	short width,
	short height,
	short depth,
	short mipmap_count,
	short format)
{
	struct bitmap_data *bitmap;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xF1, bitmap_format_type_valid_width (format, _bitmap_type_3d, width));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xF2, bitmap_format_type_valid_height(format, _bitmap_type_3d, height));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xF3, bitmap_format_type_valid_depth (format, _bitmap_type_3d, depth));

	bitmap = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0xF5, sizeof(struct bitmap_data));
	if (bitmap)
	{
		csmemset(bitmap, 0, sizeof(struct bitmap_data));
		bitmap->signature = BITMAP_GROUP_TAG;
		bitmap->width = width;
		bitmap->height = height;
		bitmap->depth = depth;
		bitmap->type = _bitmap_type_3d;
		bitmap->format = format;
		bitmap->flags = FLAG(_bitmap_allocated_bit);
		bitmap->mipmap_count = mipmap_count;
		if ((width&(width-1))==0 && (height&(height-1))==0 && (depth&(depth-1))==0)
		{
			SET_FLAG(bitmap->flags, _bitmap_has_power_of_two_dimensions_bit, TRUE);
		}
		if (format>=FIRST_COMPRESSED_BITMAP_FORMAT && format<=LAST_COMPRESSED_BITMAP_FORMAT)
		{
			SET_FLAG(bitmap->flags, _bitmap_compressed_bit, TRUE);
		}
		if (format==_bitmap_format_p8_bump)
		{
			SET_FLAG(bitmap->flags, _bitmap_palettized_bit, TRUE);
		}

		bitmap->base_address = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x112, bitmap_get_pixel_data_size(bitmap));
		if (bitmap->base_address)
		{
			match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x116, bitmap_verify(bitmap, FALSE));
		}
		else
		{
			error(_error_silent, "### ERROR failed to allocate bitmap->base_address");
		}
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate bitmap");
	}

	return bitmap;
}

struct bitmap_data *bitmap_cube_map_new(
	short width,
	short mipmap_count,
	short format)
{
	struct bitmap_data *bitmap;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x12C, bitmap_format_type_valid_width(format, _bitmap_type_cube_map, width));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x12D, (width&(width-1))==0);

	bitmap = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x12F, sizeof(struct bitmap_data));
	if (bitmap)
	{
		csmemset(bitmap, 0, sizeof(struct bitmap_data));
		bitmap->signature = BITMAP_GROUP_TAG;
		bitmap->width = width;
		bitmap->height = width;
		bitmap->depth = 1;
		bitmap->type = _bitmap_type_cube_map;
		bitmap->format = format;
		bitmap->mipmap_count = mipmap_count;
		bitmap->hardware_format = NULL;
		bitmap->flags = FLAG(_bitmap_has_power_of_two_dimensions_bit)|FLAG(_bitmap_allocated_bit);
		if (format>=FIRST_COMPRESSED_BITMAP_FORMAT && format<=LAST_COMPRESSED_BITMAP_FORMAT)
		{
			SET_FLAG(bitmap->flags, _bitmap_compressed_bit, TRUE);
		}
		if (format==_bitmap_format_p8_bump)
		{
			SET_FLAG(bitmap->flags, _bitmap_palettized_bit, TRUE);
		}

		bitmap->base_address = match_malloc("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x14D, bitmap_get_pixel_data_size(bitmap));
		if (bitmap->base_address)
		{
			match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x151, bitmap_verify(bitmap, FALSE));
		}
		else
		{
			error(_error_silent, "### ERROR failed to allocate bitmap->base_address");
		}
	}
	else
	{
		error(_error_silent, "### ERROR failed to allocate bitmap");
	}

	return bitmap;
}

void bitmap_rebuild(
	struct bitmap_data *bitmap)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x163, bitmap);

	if (!bitmap->hardware_format)
	{
		rasterizer_bitmap_new(bitmap);
	}
	rasterizer_bitmap_changed(bitmap);

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x171, bitmap_verify(bitmap, FALSE));

	return;
}

void bitmap_changed(
	struct bitmap_data *bitmap)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x179, bitmap);
	rasterizer_bitmap_changed(bitmap);

	return;
}

void bitmap_delete(
	struct bitmap_data *bitmap)
{
	if (bitmap)
	{
		rasterizer_bitmap_delete(bitmap);
		if (TEST_FLAG(bitmap->flags, _bitmap_allocated_bit))
		{
			if (bitmap->base_address)
			{
				debug_free(
					bitmap->base_address,
					"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
					0x18B);
			}
			debug_free(
				bitmap,
				"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
				0x18E);
		}
	}

	return;
}

void *bitmap_2d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short mipmap_index)
{
	long pixel_offset = 0;
	short width, height, minimum_dimension;
	long bits_per_pixel;
	short i;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A1, bitmap);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A2, bitmap->base_address);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A3, bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A4, x>=0 && x<bitmap->width);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A5, y>=0 && y<bitmap->height);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A6, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A7, !TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) || (x==0 && y==0));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1A8, !TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit) || (x==0 && y==0));

	width = bitmap->width;
	height = bitmap->height;
	minimum_dimension = TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) ? 4 : 1;
	bits_per_pixel = bitmap_format_get_bits_per_pixel(bitmap->format);
	for (i = 0; i<mipmap_index; i++)
	{
		pixel_offset += width*height;
		width = MAX(minimum_dimension, width>>1);
		height = MAX(minimum_dimension, height>>1);
	}

	return (byte *)bitmap->base_address + (pixel_offset + width*y + x)*bits_per_pixel/8;
}

void *bitmap_3d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short z,
	short mipmap_index)
{
	long pixel_offset = 0;
	short width, height, depth, minimum_dimension;
	long bits_per_pixel;
	short i;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1C7, bitmap);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1C8, bitmap->base_address);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1C9, bitmap->type==_bitmap_type_3d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CA, x>=0 && x<bitmap->width);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CB, y>=0 && y<bitmap->height);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CC, z>=0 && z<(short)bitmap->depth, "z>=0 && z<bitmap->depth");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CD, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CE, !TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) || (x==0 && y==0 && z==0));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1CF, !TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit) || (x==0 && y==0 && z==0));

	width = bitmap->width;
	height = bitmap->height;
	depth = bitmap->depth;
	minimum_dimension = TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) ? 4 : 1;
	bits_per_pixel = bitmap_format_get_bits_per_pixel(bitmap->format);
	for (i = 0; i<mipmap_index; i++)
	{
		pixel_offset += width*height*depth;
		width = MAX(minimum_dimension, width>>1);
		height = MAX(minimum_dimension, height>>1);
		depth = MAX(1, depth>>1);
	}

	return (byte *)bitmap->base_address + (pixel_offset + (height*z + y)*width + x)*bits_per_pixel/8;
}

void *bitmap_cube_map_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short face_index,
	short mipmap_index)
{
	long pixel_offset = 0;
	short width, minimum_dimension;
	long bits_per_pixel;
	short i;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F0, bitmap);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F1, bitmap->base_address);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F2, bitmap->type==_bitmap_type_cube_map);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F3, x>=0 && x<bitmap->width);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F4, y>=0 && y<bitmap->height);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F5, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F6, !TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) || (x==0 && y==0));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x1F7, !TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit) || (x==0 && y==0));

	width = bitmap->width;
	minimum_dimension = TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) ? 4 : 1;
	bits_per_pixel = bitmap_format_get_bits_per_pixel(bitmap->format);
	for (i = 0; i<mipmap_index; i++)
	{
		pixel_offset += width*width*6;
		width = MAX(minimum_dimension, width>>1);
	}

	return (byte *)bitmap->base_address + (pixel_offset + (face_index*width + y)*width + x)*bits_per_pixel/8;
}

void *bitmap_mipmap_address(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	/* January relies on the fatal assertion for unsupported types; there is
	 * no fallback address if system_exit unexpectedly returns. */
	void *mipmap_address;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x20D, bitmap);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x20E, bitmap->base_address);

	switch (bitmap->type)
	{
	case _bitmap_type_2d:
		mipmap_address = bitmap_2d_address(bitmap, 0, 0, mipmap_index);
		break;

	case _bitmap_type_3d:
		mipmap_address = bitmap_3d_address(bitmap, 0, 0, 0, mipmap_index);
		break;

	case _bitmap_type_cube_map:
		mipmap_address = bitmap_cube_map_address(bitmap, 0, 0, 0, mipmap_index);
		break;

	default:
		match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x21C, FALSE, "### ERROR unsupported bitmap type");
		break;
	}

	return mipmap_address;
}

pixel32 bitmap_format_to_a8r8g8b8(
	short format,
	void const *mipmap_address,
	long pixel_index)
{
	/* January relies on the fatal assertion for unsupported formats, as
	 * bitmap_mipmap_address does for unsupported types; there is no fallback
	 * pixel if system_exit unexpectedly returns. */
	pixel32 result;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x22B, mipmap_address);

	switch (format)
	{
	case _bitmap_format_r5g6b5:
	{
		word pixel = ((word const *)mipmap_address)[pixel_index];

		result = 0xFF000000 |
			(((((pixel >> 11) & 0x1F) << 3) | (((pixel >> 11) & 0x1F) >> 2)) << 16) |
			(((((pixel >> 5) & 0x3F) << 2) | (((pixel >> 5) & 0x3F) >> 4)) << 8) |
			(((pixel & 0x1F) << 3) | ((pixel & 0x1F) >> 2));
		break;
	}

	case _bitmap_format_a1r5g5b5:
	{
		word pixel = ((word const *)mipmap_address)[pixel_index];

		result = (((pixel >> 15) * 0xFF) << 24) |
			(((((pixel >> 10) & 0x1F) << 3) | (((pixel >> 10) & 0x1F) >> 2)) << 16) |
			(((((pixel >> 5) & 0x1F) << 3) | (((pixel >> 5) & 0x1F) >> 2)) << 8) |
			(((pixel & 0x1F) << 3) | ((pixel & 0x1F) >> 2));
		break;
	}

	case _bitmap_format_a4r4g4b4:
	{
		word pixel = ((word const *)mipmap_address)[pixel_index];

		result = (((((pixel >> 12) & 0xF) << 4) | ((pixel >> 12) & 0xF)) << 24) |
			(((((pixel >> 8) & 0xF) << 4) | ((pixel >> 8) & 0xF)) << 16) |
			(((((pixel >> 4) & 0xF) << 4) | ((pixel >> 4) & 0xF)) << 8) |
			(((pixel & 0xF) << 4) | (pixel & 0xF));
		break;
	}

	case _bitmap_format_x8r8g8b8:
		result = ((pixel32 const *)mipmap_address)[pixel_index];
		break;

	case _bitmap_format_a8r8g8b8:
		result = ((pixel32 const *)mipmap_address)[pixel_index];
		break;

	case _bitmap_format_a8:
		result = ((byte const *)mipmap_address)[pixel_index] << 24;
		break;

	case _bitmap_format_y8:
	{
		byte intensity = ((byte const *)mipmap_address)[pixel_index];

		result = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
		break;
	}

	case _bitmap_format_ay8:
	{
		byte intensity = ((byte const *)mipmap_address)[pixel_index];

		result = (intensity << 24) | (intensity << 16) | (intensity << 8) | intensity;
		break;
	}

	case _bitmap_format_a8y8:
	{
		word pixel = ((word const *)mipmap_address)[pixel_index];
		byte intensity = (byte)pixel;

		result = ((pixel >> 8) << 24) | (intensity << 16) | (intensity << 8) | intensity;
		break;
	}

	case _bitmap_format_p8_bump:
		result = global_vector_palette[((byte const *)mipmap_address)[pixel_index]];
		break;

	default:
		match_vassert(
			"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
			0x254,
			FALSE,
			"### ERROR unsupported bitmap format");
		break;
	}

	return result;
}

byte palette_find_closest_match(
	pixel32 const *palette,
	pixel32 color)
{
	short closest_match_index = NONE;
	long closest_distance = 0;
	short palette_index;

	if ((color & 0xFF000000) <= 0x80000000)
	{
		closest_match_index = 255;
	}
	else
	{
		for (palette_index = 0; palette_index < NUMBER_OF_ENTRIES_IN_PALETTE; palette_index++)
		{
			long red_delta;
			long green_delta;
			long blue_delta;
			long distance;

			if (palette[palette_index] == 0)
			{
				break;
			}

			red_delta = ABS((long)((palette[palette_index] >> 16) & 0xFF) - (long)((color >> 16) & 0xFF));
			green_delta = ABS((long)((palette[palette_index] >> 8) & 0xFF) - (long)((color >> 8) & 0xFF));
			blue_delta = ABS((long)(palette[palette_index] & 0xFF) - (long)(color & 0xFF));
			distance = red_delta * red_delta + green_delta * green_delta + blue_delta * blue_delta;
			if (palette_index == 0 || closest_distance > distance)
			{
				closest_distance = distance;
				closest_match_index = palette_index;
			}
		}

		match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x44D, closest_match_index!=NONE);
	}

	return (byte)closest_match_index;
}

short bitmap_get_max_mipmap_count(
	struct bitmap_data *bitmap)
{
	short mipmap_count = 0;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x368, bitmap_verify(bitmap, FALSE));

	if (TEST_FLAG(bitmap->flags, _bitmap_has_power_of_two_dimensions_bit))
	{
		mipmap_count = floor_log2(
			MAX(bitmap->width, MAX(bitmap->height, (short)bitmap->depth)));
	}

	return mipmap_count;
}

long bitmap_get_pixel_count(
	struct bitmap_data *bitmap)
{
	long pixel_count = 0;
	short mipmap_index;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x378, bitmap_verify(bitmap, FALSE));

	for (mipmap_index = 0; mipmap_index <= (short)bitmap->mipmap_count; mipmap_index++)
	{
		pixel_count += bitmap_mipmap_get_pixel_count(bitmap, mipmap_index);
	}

	return pixel_count;
}

long bitmap_get_pixel_data_size(
	struct bitmap_data *bitmap)
{
	long pixel_count;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x38A, bitmap_verify(bitmap, FALSE));

	pixel_count = bitmap_get_pixel_count(bitmap);
	return pixel_count * bitmap_format_get_bits_per_pixel(bitmap->format) / 8;
}

short bitmap_mipmap_get_width(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	short width;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x39B, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x39C, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");

	width = MAX(bitmap->width>>mipmap_index, 1);
	if (TEST_FLAG(bitmap->flags, _bitmap_compressed_bit))
	{
		width += (-width)&3;
	}

	return width;
}

short bitmap_mipmap_get_height(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	short height;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3AE, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3AF, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");

	height = MAX(bitmap->height>>mipmap_index, 1);
	if (TEST_FLAG(bitmap->flags, _bitmap_compressed_bit))
	{
		height += (-height)&3;
	}

	return height;
}

short bitmap_mipmap_get_depth(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3BF, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3C0, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");

	return MAX((short)bitmap->depth>>mipmap_index, 1);
}

long bitmap_mipmap_get_pixel_count(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	long pixel_count;
	short width, height, depth;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3CB, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3CC, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");

	width = bitmap_mipmap_get_width(bitmap, mipmap_index);
	height = bitmap_mipmap_get_height(bitmap, mipmap_index);
	depth = bitmap_mipmap_get_depth(bitmap, mipmap_index);
	pixel_count = width*height*depth;
	if (bitmap->type==_bitmap_type_cube_map)
	{
		pixel_count *= 6;
	}

	return pixel_count;
}

long bitmap_mipmap_get_pixel_data_size(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	long pixel_count;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3E3, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3E4, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");

	pixel_count = bitmap_mipmap_get_pixel_count(bitmap, mipmap_index);
	return pixel_count*bitmap_format_get_bits_per_pixel(bitmap->format)/8;
}

long bitmap_mipmap_get_row_pitch(
	struct bitmap_data *bitmap,
	short mipmap_index)
{
	short width;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3F5, bitmap_verify(bitmap, FALSE));
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3F6, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3F7, !TEST_FLAG(bitmap->flags, _bitmap_compressed_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x3F8, !TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit));

	width = bitmap_mipmap_get_width(bitmap, mipmap_index);
	return width*bitmap_format_get_bits_per_pixel(bitmap->format)/8;
}

pixel32 bitmap_2d_get_pixel(
	struct bitmap_data *bitmap,
	union real_point2d const *point,
	real lod)
{
	short mipmap_index;
	short width;
	short height;
	short x;
	short y;
	void *mipmap_address;
	long offset;
	pixel32 pixel;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x261, bitmap);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x262, bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x263, !TEST_FLAG(bitmap->flags, _bitmap_linear_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x264, point);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x265, lod>=0.0f && lod<=1.0f, "lod>=0.0f && lod<=1.0f");

	if (bitmap->base_address)
	{
		if (lod < 1.0f && (short)bitmap->mipmap_count > 0)
		{
			mipmap_index = (short)fast_ftol((1.0f - lod) * (short)bitmap->mipmap_count);
			match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x26F, mipmap_index>=0 && mipmap_index<=(short)bitmap->mipmap_count, "mipmap_index>=0 && mipmap_index<=bitmap->mipmap_count");
		}
		else
		{
			mipmap_index = 0;
		}

		width = bitmap_mipmap_get_width(bitmap, mipmap_index);
		height = bitmap_mipmap_get_height(bitmap, mipmap_index);
		if ((width & (width - 1)) == 0)
		{
			x = (short)(fast_ftol((real)width * point->x - 0.5f) & (width - 1));
		}
		else
		{
			x = (short)(((fast_ftol((real)width * point->x - 0.5f) % width) + width) % width);
		}
		if ((height & (height - 1)) == 0)
		{
			y = (short)(fast_ftol((real)height * point->y - 0.5f) & (height - 1));
		}
		else
		{
			y = (short)(((fast_ftol((real)height * point->y - 0.5f) % height) + height) % height);
		}

		mipmap_address = bitmap_mipmap_address(bitmap, mipmap_index);
		if (TEST_FLAG(bitmap->flags, _bitmap_compressed_bit))
		{
			short bytes_per_block = S3TC_BLOCK_PIXELS * bitmap_format_get_bits_per_pixel(bitmap->format) / CHAR_BITS;
			short block_x = x / 4;
			short block_y = y / 4;
			byte *block_address = (byte *)mipmap_address + (block_y * width / 4 + block_x) * bytes_per_block;

			x &= 3;
			y &= 3;

			match_vassert(
				"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
				0x2A0,
				block_address >= (byte *)bitmap->base_address,
				csprintf(
					temporary,
					"bitmap_2d_get_pixel tried to access compressed block @ -%d bytes from address start (w=%d, h=%d, m=%d, x=%d, y=%d, lod=%f)",
					(byte *)bitmap->base_address - block_address,
					bitmap->width,
					bitmap->height,
					(short)bitmap->mipmap_count,
					fast_ftol((real)width * point->x - 0.5f) % width,
					fast_ftol((real)height * point->y - 0.5f) % height,
					lod));
			match_vassert(
				"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
				0x2A9,
				block_address < (byte *)bitmap->base_address + bitmap->pixels_size,
				csprintf(
					temporary,
					"bitmap_2d_get_pixel tried to access compressed block @ -%d bytes from address end (w=%d, h=%d, m=%d, x=%d, y=%d, lod=%f)",
					block_address - ((byte *)bitmap->base_address + bitmap->pixels_size),
					bitmap->width,
					bitmap->height,
					(short)bitmap->mipmap_count,
					fast_ftol((real)width * point->x - 0.5f) % width,
					fast_ftol((real)height * point->y - 0.5f) % height,
					lod));

			switch (bitmap->format)
			{
			case _bitmap_format_dxt1:
				DecodeBlockRGB__single_pixel(
					(struct s3tc_block_rgb const *)block_address,
					(struct s3tc_color *)&pixel,
					x,
					y);
				break;

			case _bitmap_format_dxt3:
				DecodeBlockAlpha4__single_pixel(
					(struct s3tc_block_alpha4 const *)block_address,
					(struct s3tc_color *)&pixel,
					x,
					y);
				break;

			case _bitmap_format_dxt5:
				DecodeBlockAlpha3__single_pixel(
					(struct s3tc_block_alpha3 const *)block_address,
					(struct s3tc_color *)&pixel,
					x,
					y);
				break;

			default:
				match_vassert(
					"c:\\halo\\SOURCE\\bitmaps\\bitmaps.c",
					0x2B7,
					FALSE,
					"### ERROR unsupported bitmap format");
				pixel = 0;
				break;
			}

			return pixel;
		}

		if (TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit))
		{
			long result[2];

			match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2C1, x>=0 && x<4096);
			match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2C2, y>=0 && y<4096);
			bitmap_swizzle_vector2d(width, height, x, y, result);

			offset = result[0] | result[1];
		}
		else
		{
			offset = y * width + x;
		}

		return bitmap_format_to_a8r8g8b8(bitmap->format, mipmap_address, offset);
	}

	return (pixel32)NONE;
}

boolean bitmap_verify(
	struct bitmap_data *bitmap,
	boolean import)
{
	boolean valid = TRUE;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x40F, bitmap);

	if (bitmap->signature==BITMAP_GROUP_TAG &&
		(bitmap->flags&~(FLAG(NUMBER_OF_BITMAP_FLAGS)-1))==0 &&
		VALID_INDEX(bitmap->type, NUMBER_OF_BITMAP_TYPES) &&
		VALID_INDEX(bitmap->format, NUMBER_OF_BITMAP_FORMATS) &&
		bitmap_format_type_valid_width(bitmap->format, bitmap->type, bitmap->width) &&
		bitmap_format_type_valid_height(bitmap->format, bitmap->type, bitmap->height) &&
		bitmap_format_type_valid_depth(bitmap->format, bitmap->type, (short)bitmap->depth) &&
		(short)bitmap->mipmap_count>=0 &&
		(short)bitmap->mipmap_count<=floor_log2(MAX(bitmap->width, MAX(bitmap->height, (short)bitmap->depth))))
	{
		if (import &&
			!(bitmap->format==_bitmap_format_a8r8g8b8 &&
				bitmap->base_address &&
				bitmap->mipmap_count==0 &&
				!TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) &&
				!TEST_FLAG(bitmap->flags, _bitmap_palettized_bit) &&
				!TEST_FLAG(bitmap->flags, _bitmap_swizzled_bit)))
		{
			error(_error_silent, "### ERROR bitmap @%p (#%dx#%d) appears to be invalid for import", bitmap, bitmap->width, bitmap->height);
			valid = FALSE;
		}
	}
	else
	{
		error(_error_silent, "### ERROR bitmap @%p (#%dx#%d) appears to be invalid", bitmap, bitmap->width, bitmap->height);
		valid = FALSE;
	}

	return valid;
}

void bitmap_byte_swap_pixels(
	struct bitmap_data *bitmap)
{
	return;
}

void bitmap_3d_slice_extract(
	struct bitmap_data *source_bitmap,
	short source_mipmap_index,
	short source_slice_index,
	struct bitmap_data *slice_bitmap)
{
	long pixel_data_size;
	void *source_address;
	void *slice_address;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E3, bitmap_verify(source_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E4, source_bitmap->type==_bitmap_type_3d);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E5, source_mipmap_index>=0 && source_mipmap_index<=(short)source_bitmap->mipmap_count, "source_mipmap_index>=0 && source_mipmap_index<=source_bitmap->mipmap_count");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E6, source_slice_index>=0 && source_slice_index<(short)source_bitmap->depth, "source_slice_index>=0 && source_slice_index<source_bitmap->depth");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E7, MAX(1, source_bitmap->width>>source_mipmap_index)==slice_bitmap->width, "MAX(1, source_bitmap->width >>source_mipmap_index)==slice_bitmap->width");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E8, MAX(1, source_bitmap->height>>source_mipmap_index)==slice_bitmap->height, "MAX(1, source_bitmap->height>>source_mipmap_index)==slice_bitmap->height");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2E9, !TEST_FLAG(source_bitmap->flags, _bitmap_swizzled_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2EB, bitmap_verify(slice_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2EC, slice_bitmap->mipmap_count==0);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2ED, slice_bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2EE, slice_bitmap->format==source_bitmap->format);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x2EF, !TEST_FLAG(slice_bitmap->flags, _bitmap_swizzled_bit));

	pixel_data_size = bitmap_get_pixel_data_size(slice_bitmap);
	source_address = bitmap_3d_address(source_bitmap, 0, 0, source_slice_index, source_mipmap_index);
	slice_address = bitmap_mipmap_address(slice_bitmap, 0);
	csmemcpy(slice_address, source_address, pixel_data_size);

	return;
}

void bitmap_3d_slice_insert(
	struct bitmap_data *slice_bitmap,
	struct bitmap_data *destination_bitmap,
	short destination_mipmap_index,
	short destination_slice_index)
{
	long pixel_data_size;
	void *source_address;
	void *destination_address;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x306, bitmap_verify(slice_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x307, slice_bitmap->mipmap_count==0);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x308, slice_bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x309, slice_bitmap->format==destination_bitmap->format);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x30A, !TEST_FLAG(slice_bitmap->flags, _bitmap_swizzled_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x30C, bitmap_verify(destination_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x30D, destination_bitmap->type==_bitmap_type_3d);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x30E, destination_mipmap_index>=0 && destination_mipmap_index<=(short)destination_bitmap->mipmap_count, "destination_mipmap_index>=0 && destination_mipmap_index<=destination_bitmap->mipmap_count");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x30F, destination_slice_index>=0 && destination_slice_index<(short)destination_bitmap->depth, "destination_slice_index>=0 && destination_slice_index<destination_bitmap->depth");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x310, MAX(1, destination_bitmap->width>>destination_mipmap_index)==slice_bitmap->width, "MAX(1, destination_bitmap->width >>destination_mipmap_index)==slice_bitmap->width");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x311, MAX(1, destination_bitmap->height>>destination_mipmap_index)==slice_bitmap->height, "MAX(1, destination_bitmap->height>>destination_mipmap_index)==slice_bitmap->height");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x312, !TEST_FLAG(destination_bitmap->flags, _bitmap_swizzled_bit));

	pixel_data_size = bitmap_get_pixel_data_size(slice_bitmap);
	destination_address = bitmap_3d_address(destination_bitmap, 0, 0, destination_slice_index, destination_mipmap_index);
	source_address = bitmap_mipmap_address(slice_bitmap, 0);
	csmemcpy(destination_address, source_address, pixel_data_size);

	return;
}

void bitmap_cube_map_face_extract(
	struct bitmap_data *source_bitmap,
	short source_mipmap_index,
	short source_face_index,
	struct bitmap_data *face_bitmap)
{
	long pixel_data_size;
	void *source_address;
	void *face_address;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x329, bitmap_verify(source_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32A, source_bitmap->type==_bitmap_type_cube_map);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32B, source_mipmap_index>=0 && source_mipmap_index<=(short)source_bitmap->mipmap_count, "source_mipmap_index>=0 && source_mipmap_index<=source_bitmap->mipmap_count");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32C, source_face_index>=0 && source_face_index<NUMBER_OF_FACES_PER_CUBE, "source_face_index>=0 && source_face_index<NUMBER_OF_FACES_PER_CUBE");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32D, MAX(1, source_bitmap->width>>source_mipmap_index)==face_bitmap->width, "MAX(1, source_bitmap->width >>source_mipmap_index)==face_bitmap->width");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32E, MAX(1, source_bitmap->height>>source_mipmap_index)==face_bitmap->height, "MAX(1, source_bitmap->height>>source_mipmap_index)==face_bitmap->height");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x32F, !TEST_FLAG(source_bitmap->flags, _bitmap_swizzled_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x331, bitmap_verify(face_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x332, face_bitmap->mipmap_count==0);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x333, face_bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x334, face_bitmap->format==source_bitmap->format);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x335, !TEST_FLAG(face_bitmap->flags, _bitmap_swizzled_bit));

	pixel_data_size = bitmap_get_pixel_data_size(face_bitmap);
	source_address = bitmap_cube_map_address(source_bitmap, 0, 0, source_face_index, source_mipmap_index);
	face_address = bitmap_mipmap_address(face_bitmap, 0);
	csmemcpy(face_address, source_address, pixel_data_size);

	return;
}

void bitmap_cube_map_face_insert(
	struct bitmap_data *face_bitmap,
	struct bitmap_data *destination_bitmap,
	short destination_mipmap_index,
	short destination_face_index)
{
	long pixel_data_size;
	void *source_address;
	void *destination_address;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x34C, bitmap_verify(face_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x34D, face_bitmap->mipmap_count==0);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x34E, face_bitmap->type==_bitmap_type_2d);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x34F, face_bitmap->format==destination_bitmap->format);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x350, !TEST_FLAG(face_bitmap->flags, _bitmap_swizzled_bit));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x352, bitmap_verify(destination_bitmap, FALSE));
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x353, destination_bitmap->type==_bitmap_type_cube_map);
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x354, destination_mipmap_index>=0 && destination_mipmap_index<=(short)destination_bitmap->mipmap_count, "destination_mipmap_index>=0 && destination_mipmap_index<=destination_bitmap->mipmap_count");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x355, destination_face_index>=0 && destination_face_index<NUMBER_OF_FACES_PER_CUBE, "destination_face_index>=0 && destination_face_index<NUMBER_OF_FACES_PER_CUBE");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x356, MAX(1, destination_bitmap->width>>destination_mipmap_index)==face_bitmap->width, "MAX(1, destination_bitmap->width >>destination_mipmap_index)==face_bitmap->width");
	match_vassert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x357, MAX(1, destination_bitmap->height>>destination_mipmap_index)==face_bitmap->height, "MAX(1, destination_bitmap->height>>destination_mipmap_index)==face_bitmap->height");
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 0x358, !TEST_FLAG(destination_bitmap->flags, _bitmap_swizzled_bit));

	pixel_data_size = bitmap_get_pixel_data_size(face_bitmap);
	destination_address = bitmap_cube_map_address(destination_bitmap, 0, 0, destination_face_index, destination_mipmap_index);
	source_address = bitmap_mipmap_address(face_bitmap, 0);
	csmemcpy(destination_address, source_address, pixel_data_size);

	return;
}

/* ---------- private code */

static boolean bitmap_format_type_valid_width(
	short format,
	short type,
	short width)
{
	return width>0 && width<=MAXIMUM_BITMAP_WIDTH;
}

static boolean bitmap_format_type_valid_height(
	short format,
	short type,
	short height)
{
	return height>0 && height<=MAXIMUM_BITMAP_HEIGHT;
}

static boolean bitmap_format_type_valid_depth(
	short format,
	short type,
	short depth)
{
	return depth>0 && depth<=MAXIMUM_BITMAP_DEPTH && (depth==1 || type==_bitmap_type_3d);
}
