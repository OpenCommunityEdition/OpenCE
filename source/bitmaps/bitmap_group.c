/* ---------- headers */

#include "cseries.h"

#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "cache/cache_files.h"
#include "cseries/errors.h"
#include "tag_files/tag_files.h"

/* ---------- constants */

enum
{
	_bitmap_group_type_cube_maps = 2,
	_bitmap_group_type_sprites = 3,
	_bitmap_group_type_interface_bitmaps = 4,
	_bitmap_format_a8y8 = 3,
	_bitmap_format_dxt1 = 14,
	_bitmap_format_dxt5 = 16,
	_bitmap_format_p8_bump = 17,
	_bitmap_has_power_of_two_dimensions_bit = 0,
	_bitmap_compressed_bit = 1,
	_bitmap_palettized_bit = 2,
	_bitmap_linear_bit = 4,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- declarations that belong in tag_files/tag_groups.h
   These are shared tag-system types, not bitmap_group's own: seven
   tag_field_type constants January's bitmap_fields uses, and struct
   tag_flags_definition and struct tag_group (member lists attested by
   HCEX.pdb).  They are held here because this lane may not commit shared
   cross-lane headers; moving them to tag_groups.h is an owner action and is
   data-inert (VC7 lays .data out by declaration order, not name count).
   ---------- */

enum
{
	_tag_field_tag = 5,
	_tag_field_point2d = 10,
	_tag_field_real = 14,
	_tag_field_real_fraction = 15,
	_tag_field_real_point2d = 16,
	_tag_field_explanation = 42,
	_tag_field_custom = 43,
};

typedef boolean (*postprocess_tag_proc)(
	long tag_index,
	boolean editing);

struct tag_flags_definition
{
	long count;
	char **names;
};

struct tag_group
{
	char *name;
	unsigned long flags;
	unsigned long group_tag;
	unsigned long parent_group_tag;
	short version;
	postprocess_tag_proc postprocess_tag;
	struct tag_block_definition *header_block_definition;
	unsigned long child_group_tags[16];
	short child_count;
};

typedef char tag_group_size_assert[sizeof(struct tag_group) == 0x60 ? 1 : -1];

/* ---------- END OWNER HEADER PREREQUISITE */

/* ---------- prototypes */

static boolean postprocess_bitmap(
	struct bitmap_data *bitmap,
	boolean editing);
static void delete_bitmap(
	struct tag_block *block,
	long element_index);
static boolean postprocess_bitmap_group(
	long bitmap_group_index,
	boolean editing);

/* ---------- globals */

extern boolean find_all_fucked_up_shit;

struct tag_reference_definition global_bitmap_reference =
{
	0,
	BITMAP_GROUP_TAG,
	NULL,
};

struct tag_reference_definition global_bitmap_reference_optional =
{
	0,
	BITMAP_GROUP_TAG,
	NULL,
};

static char *bitmap_types_strings[3] =
{
	"2D texture",
	"3D texture",
	"cube map",
};

static struct tag_enum_definition bitmap_types =
{
	3,
	bitmap_types_strings,
	NULL,
};

static char *bitmap_formats_strings[18] =
{
	"a8",
	"y8",
	"ay8",
	"a8y8",
	"unused1",
	"unused2",
	"r5g6b5",
	"unused3",
	"a1r5g5b5",
	"a4r4g4b4",
	"x8r8g8b8",
	"a8r8g8b8",
	"unused4",
	"unused5",
	"dxt1",
	"dxt3",
	"dxt5",
	"p8-bump",
};

static struct tag_enum_definition bitmap_formats =
{
	18,
	bitmap_formats_strings,
	NULL,
};

static char *bitmap_flags_strings[6] =
{
	"power of two dimensions",
	"compressed",
	"palettized",
	"swizzled",
	"linear",
	"v16u16",
};

static struct tag_flags_definition bitmap_flags =
{
	6,
	bitmap_flags_strings,
};

static struct tag_field bitmap_data_block_fields[16] =
{
	{ _tag_field_tag, 0, "signature*", NULL },
	{ _tag_field_short_integer, 0, "width*:pixels", NULL },
	{ _tag_field_short_integer, 0, "height*:pixels", NULL },
	{ _tag_field_short_integer, 0, "depth*:pixels#depth is 1 for 2D textures and cube maps", NULL },
	{ _tag_field_enum, 0, "type*#determines bitmap 'geometry'", &bitmap_types },
	{ _tag_field_enum, 0, "format*#determines how pixels are represented internally", &bitmap_formats },
	{ _tag_field_word_flags, 0, "flags*", &bitmap_flags },
	{ _tag_field_point2d, 0, "registration point*", NULL },
	{ _tag_field_short_integer, 0, "mipmap count*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)2 },
	{ _tag_field_long_integer, 0, "pixels offset*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)4 },
	{ _tag_field_pad, 0, NULL, (void *)4 },
	{ _tag_field_pad, 0, NULL, (void *)4 },
	{ _tag_field_pad, 0, NULL, (void *)8 },
	{ _tag_field_terminator, 0, NULL, NULL },
};

static struct tag_block_definition bitmap_data_block =
{
	"bitmap_data_block",
	0,
	2048,
	sizeof(struct bitmap_data),
	NULL,
	bitmap_data_block_fields,
	NULL,
	postprocess_bitmap,
	NULL,
	delete_bitmap,
	NULL,
};

static struct tag_field bitmap_group_sprite_block_fields[9] =
{
	{ _tag_field_short_integer, 0, "bitmap index*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)2 },
	{ _tag_field_pad, 0, NULL, (void *)4 },
	{ _tag_field_real, 0, "left*", NULL },
	{ _tag_field_real, 0, "right*", NULL },
	{ _tag_field_real, 0, "top*", NULL },
	{ _tag_field_real, 0, "bottom*", NULL },
	{ _tag_field_real_point2d, 0, "registration point*", NULL },
	{ _tag_field_terminator, 0, NULL, NULL },
};

static struct tag_block_definition bitmap_group_sprite_block =
{
	"bitmap_group_sprite_block",
	0,
	64,
	sizeof(struct bitmap_group_sprite),
	NULL,
	bitmap_group_sprite_block_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static struct tag_field bitmap_group_sequence_block_fields[6] =
{
	{ _tag_field_string, 0, "name^", NULL },
	{ _tag_field_short_integer, 0, "first bitmap index*", NULL },
	{ _tag_field_short_integer, 0, "bitmap count*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)16 },
	{ _tag_field_block, 0, "sprites*", &bitmap_group_sprite_block },
	{ _tag_field_terminator, 0, NULL, NULL },
};

static struct tag_block_definition bitmap_group_sequence_block =
{
	"bitmap_group_sequence_block",
	0,
	256,
	sizeof(struct bitmap_group_sequence),
	NULL,
	bitmap_group_sequence_block_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static char *bitmap_group_flags_strings[4] =
{
	"enable diffusion dithering",
	"disable height map compression",
	"uniform sprite sequences",
	"filthy sprite bug fix",
};

static struct tag_flags_definition bitmap_group_flags =
{
	4,
	bitmap_group_flags_strings,
};

static char *bitmap_group_types_strings[5] =
{
	"2D textures",
	"3D textures",
	"cube maps",
	"sprites",
	"interface bitmaps",
};

static struct tag_enum_definition bitmap_group_types =
{
	5,
	bitmap_group_types_strings,
	NULL,
};

static char *bitmap_group_usages_strings[6] =
{
	"alpha-blend",
	"default",
	"height map",
	"detail map",
	"light map",
	"vector map",
};

static struct tag_enum_definition bitmap_group_usages =
{
	6,
	bitmap_group_usages_strings,
	NULL,
};

static char *bitmap_group_formats_strings[6] =
{
	"compressed with color-key transparency",
	"compressed with explicit alpha",
	"compressed with interpolated alpha",
	"16-bit color",
	"32-bit color",
	"monochrome",
};

static struct tag_enum_definition bitmap_group_formats =
{
	6,
	bitmap_group_formats_strings,
	NULL,
};

static char *bitmap_group_sprite_budgets_strings[5] =
{
	"32x32",
	"64x64",
	"128x128",
	"256x256",
	"512x512",
};

static struct tag_enum_definition bitmap_group_sprite_budgets =
{
	5,
	bitmap_group_sprite_budgets_strings,
	NULL,
};

static char *bitmap_group_sprite_usages_strings[3] =
{
	"blend/add/subtract/max",
	"multiply/min",
	"double multiply",
};

static struct tag_enum_definition bitmap_group_sprite_usages =
{
	3,
	bitmap_group_sprite_usages_strings,
	NULL,
};

struct tag_data_definition bitmap_pixel_data =
{
	"bitmap_pixel_data",
	1,
	0x1000000,
	NULL,
};

struct tag_data_definition color_plate_data =
{
	"color_plate_data",
	1,
	0x1000000,
	NULL,
};

static struct tag_field bitmap_fields[32] =
{
	{ _tag_field_custom, 0, NULL, (void *)'bshw' },
	{ _tag_field_explanation, 0, "type", "Type controls bitmap 'geometry'. All dimensions must be a power of two except for SPRITES and INTERFACE BITMAPS:\n\n* 2D TEXTURES: Ordinary, 2D textures will be generated.\n* 3D TEXTURES: Volume textures will be generated from each sequence of 2D texture 'slices'.\n* CUBE MAPS: Cube maps will be generated from each consecutive set of six 2D textures in each sequence, all faces of a cube map must be square and the same size.\n* SPRITES: Sprite texture pages will be generated.\n* INTERFACE BITMAPS: Similar to 2D TEXTURES, but without mipmaps and without the power of two restriction." },
	{ _tag_field_enum, 0, "type", &bitmap_group_types },
	{ _tag_field_explanation, 0, "format", "Format controls how pixels will be stored internally:\n\n* COMPRESSED WITH COLOR-KEY TRANSPARENCY: DXT1 compression, uses 4 bits per pixel. 4x4 blocks of pixels are reduced to 2 colors and interpolated, alpha channel uses color-key transparency instead of alpha from the plate (all zero-alpha pixels also have zero-color).\n* COMPRESSED WITH EXPLICIT ALPHA: DXT2/3 compression, uses 8 bits per pixel. Same as DXT1 without the color key transparency, alpha channel uses alpha from plate quantized down to 4 bits per pixel.\n* COMPRESSED WITH INTERPOLATED ALPHA: DXT4/5 compression, uses 8 bits per pixel. Same as DXT2/3, except alpha is smoother. Better for smooth alpha gradients, worse for noisy alpha.\n* 16-BIT COLOR: Uses 16 bits per pixel. Depending on the alpha channel, bitmaps are quantized to either r5g6b5 (no alpha), a1r5g5b5 (1-bit alpha), or a4r4g4b4 (>1-bit alpha).\n* 32-BIT COLOR: Uses 32 bits per pixel. Very high quality, can have alpha at no added cost. This format takes up the most memory, however. Bitmap formats are x8r8g8b8 and a8r8g8b.\n* MONOCHROME: Uses either 8 or 16 bits per pixel. Bitmap formats are a8 (alpha), y8 (intensity), ay8 (combined alpha-intensity) and a8y8 (separate alpha-intensity).\n\nNote: Height maps (a.k.a. bump maps) should use 32-bit color; this is internally converted to a palettized format which takes less memory." },
	{ _tag_field_enum, 0, "format", &bitmap_group_formats },
	{ _tag_field_explanation, 0, "usage", "Usage controls how mipmaps are generated:\n\n* ALPHA BLEND: Pixels with zero alpha are ignored in mipmaps, to prevent bleeding the transparent color.\n* DEFAULT: Downsampling works normally, as in Photoshop.\n* HEIGHT MAP: The bitmap (normally grayscale) is a height map which gets converted to a bump map. Uses <bump height> below. Alpha is passed through unmodified.\n* DETAIL MAP: Mipmap color fades to gray, controlled by <detail fade factor> below. Alpha fades to white.\n* LIGHT MAP: Generates no mipmaps. Do not use!\n* VECTOR MAP: Used mostly for special effects; pixels are treated as XYZ vectors and normalized after downsampling. Alpha is passed through unmodified." },
	{ _tag_field_enum, 0, "usage", &bitmap_group_usages },
	{ _tag_field_word_flags, 0, "flags", &bitmap_group_flags },
	{ _tag_field_explanation, 0, "post-processing", "These properties control how mipmaps are post-processed." },
	{ _tag_field_real_fraction, 0, "detail fade factor:[0,1]#0 means fade to gray by last mipmap, 1 means fade to gray by first mipmap", NULL },
	{ _tag_field_real_fraction, 0, "sharpen amount:[0,1]#sharpens mipmap after downsampling", NULL },
	{ _tag_field_real_fraction, 0, "bump height:repeats#the apparent height of the bump map above the triangle it is textured onto, in texture repeats (i.e., 1.0 would be as high as the texture is wide)", NULL },
	{ _tag_field_explanation, 0, "sprite processing", "When creating a sprite group, specify the number and size of textures that the group is allowed to occupy. During importing, you'll receive feedback about how well the alloted space was used." },
	{ _tag_field_enum, 0, "sprite budget size", &bitmap_group_sprite_budgets },
	{ _tag_field_short_integer, 0, "sprite budget count", NULL },
	{ _tag_field_explanation, 0, "color plate", "The original TIFF file used to import the bitmap group." },
	{ _tag_field_short_integer, 0, "color plate width*:pixels", NULL },
	{ _tag_field_short_integer, 0, "color plate height*:pixels", NULL },
	{ _tag_field_data, 0, "compressed color plate data*", &color_plate_data },
	{ _tag_field_explanation, 0, "processed pixel data", "Pixel data after being processed by the tool." },
	{ _tag_field_data, 0, "processed pixel data*", &bitmap_pixel_data },
	{ _tag_field_explanation, 0, "miscellaneous", "" },
	{ _tag_field_real, 0, "blur filter size:[0,10] pixels#blurs the bitmap before generating mipmaps", NULL },
	{ _tag_field_real, 0, "alpha bias:[-1,1]#affects alpha mipmap generation", NULL },
	{ _tag_field_short_integer, 0, "mipmap count:levels#0 defaults to all levels", NULL },
	{ _tag_field_explanation, 0, "...more sprite processing", "Sprite usage controls the background color of sprite plates." },
	{ _tag_field_enum, 0, "sprite usage", &bitmap_group_sprite_usages },
	{ _tag_field_short_integer, 0, "sprite spacing*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)2 },
	{ _tag_field_block, 0, "sequences*", &bitmap_group_sequence_block },
	{ _tag_field_block, 0, "bitmaps*", &bitmap_data_block },
	{ _tag_field_terminator, 0, NULL, NULL },
};

static struct tag_block_definition bitmap_block =
{
	"bitmap",
	0,
	1,
	sizeof(struct bitmap_group),
	NULL,
	bitmap_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct tag_group bitmap_group =
{
	"bitmap",
	8,
	BITMAP_GROUP_TAG,
	NONE,
	7,
	postprocess_bitmap_group,
	&bitmap_block,
};

/* ---------- public code */

struct bitmap_data *bitmap_group_try_and_get_bitmap(
	long bitmap_group_index,
	short bitmap_index)
{
	struct bitmap_group *group = bitmap_group_get(bitmap_group_index);
	struct bitmap_data *result = NULL;

	if (group && bitmap_index >= 0 && bitmap_index < group->bitmaps.count)
	{
		result = TAG_BLOCK_GET_ELEMENT(
			&group->bitmaps,
			bitmap_index,
			struct bitmap_data);
	}

	return result;
}

struct bitmap_data *bitmap_group_get_bitmap_from_sequence(
	long bitmap_group_index,
	short sequence_index,
	short frame_index)
{
	struct bitmap_data *result = NULL;

	if (bitmap_group_index != NONE)
	{
		short bitmap_index = NONE;
		struct bitmap_group *group;

		match_assert(
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
			0x2A6,
			sequence_index>=0 && frame_index>=0);

		group = bitmap_group_get(bitmap_group_index);
		if (group)
		{
			/* port: the asserts only log in release, and % keeps a negative
			sequence negative. So a negative one (the map's, as hud elements
			give it) has no sequence, and the frame is the bitmap, as with no
			sequences at all. */
			if (group->sequences.count > 0 && sequence_index < 0)
			{
				static boolean reported = FALSE;

				if (!reported)
				{
					reported = TRUE;
					error(_error_silent, "bitmap 0x%08lX asked for sequence #%d (none used)",
						(unsigned long)bitmap_group_index, sequence_index);
				}
			}
			else if (group->sequences.count > 0)
			{
				struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
					&group->sequences,
					sequence_index % group->sequences.count,
					struct bitmap_group_sequence);

				if (sequence->bitmap_count > 0)
				{
					bitmap_index = (short)(frame_index % sequence->bitmap_count +
						sequence->first_bitmap_index);
				}
				else if (sequence->sprites.count && frame_index >= 0) /* port: not below the first sprite */
				{
					bitmap_index = TAG_BLOCK_GET_ELEMENT(
						&sequence->sprites,
						frame_index,
						struct bitmap_group_sprite)->bitmap_index;
				}
			}

			if (bitmap_index == NONE)
				bitmap_index = frame_index;

			if (bitmap_index >= 0 && bitmap_index < group->bitmaps.count)
			{
				result = TAG_BLOCK_GET_ELEMENT(
					&group->bitmaps,
					bitmap_index,
					struct bitmap_data);
			}
		}
	}

	return result;
}

short bitmap_group_add_bitmap(
	struct bitmap_group *group,
	short width,
	short height,
	short depth,
	short type,
	short format,
	short mipmap_count)
{
	struct bitmap_data fake_bitmap;
	long pixels_end = 0;
	long previous_count;
	long pixel_data_size;

	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c", 0x2DB, group);

	fake_bitmap.type = type;
	fake_bitmap.flags = 0;
	fake_bitmap.registration_point.y = 0;
	fake_bitmap.registration_point.x = 0;
	fake_bitmap.mipmap_count = mipmap_count;
	fake_bitmap.pixels_offset = 0;
	fake_bitmap.hardware_format = NULL;
	fake_bitmap.base_address = NULL;
	fake_bitmap.signature = BITMAP_GROUP_TAG;
	fake_bitmap.width = width;
	fake_bitmap.height = height;
	fake_bitmap.depth = depth;
	fake_bitmap.format = format;

	if (group->type == _bitmap_group_type_interface_bitmaps)
	{
		SET_FLAG(fake_bitmap.flags, _bitmap_linear_bit, TRUE);
	}
	else if ((width & (width - 1)) ||
		(height & (height - 1)) ||
		(depth & (depth - 1)))
	{
		fprintf(
			stdout,
			"skipping bitmap with non-power-of-two dimensions (#%dx#%d#%d)\r\n",
			width,
			height,
			depth);
		fflush(stdout);
		return NONE;
	}
	else if (group->type == _bitmap_group_type_cube_maps && width != height)
	{
		fprintf(
			stdout,
			"skipping cube map with non-square faces (#%dx#%d)\r\n",
			width,
			height);
		fflush(stdout);
		return NONE;
	}
	else
	{
		SET_FLAG(
			fake_bitmap.flags,
			_bitmap_has_power_of_two_dimensions_bit,
			TRUE);
	}

	if (format >= _bitmap_format_dxt1 && format <= _bitmap_format_dxt5)
	{
		SET_FLAG(fake_bitmap.flags, _bitmap_compressed_bit, TRUE);
	}
	if (format == _bitmap_format_p8_bump)
	{
		SET_FLAG(fake_bitmap.flags, _bitmap_palettized_bit, TRUE);
	}

	/* January repeats these validation guards after assigning format flags. */
	if (group->type == _bitmap_group_type_cube_maps && width != height)
	{
		fprintf(
			stdout,
			"skipping cube map with non-square faces (#%dx#%d)\r\n",
			width,
			height);
		fflush(stdout);
		return NONE;
	}
	if (!TEST_FLAG(
		fake_bitmap.flags,
		_bitmap_has_power_of_two_dimensions_bit) &&
		group->type != _bitmap_group_type_interface_bitmaps)
	{
		fprintf(
			stdout,
			"skipping bitmap with non power-of-two dimensions (#%dx#%d)\r\n",
			width,
			height);
		fflush(stdout);
		return NONE;
	}

	previous_count = group->bitmaps.count;
	pixel_data_size = bitmap_get_pixel_data_size(&fake_bitmap);
	if (tag_block_resize(&group->bitmaps, group->bitmaps.count + 1) &&
		tag_data_resize(&group->pixel_data, group->pixel_data.size + pixel_data_size))
	{
		struct bitmap_data *previous_bitmap = NULL;
		short bitmap_index = 0;

		for (;
			bitmap_index < group->bitmaps.count;
			bitmap_index = (short)(bitmap_index + 1))
		{
			struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
				&group->bitmaps,
				bitmap_index,
				struct bitmap_data);

			if (bitmap->base_address)
			{
				long space_between;

				match_assert(
					"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
					0x34D,
					!bitmap->hardware_format);
				bitmap->base_address =
					(byte *)group->pixel_data.address + bitmap->pixels_offset;
				match_assert(
					"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
					0x352,
					(byte*)bitmap->base_address>=(byte*)group->pixel_data.address);
				match_assert(
					"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
					0x354,
					(byte*)bitmap->base_address + bitmap_get_pixel_data_size(bitmap) <= (byte*)group->pixel_data.address + group->pixel_data.size);

				if (previous_bitmap)
				{
					space_between = bitmap->pixels_offset -
						previous_bitmap->pixels_offset -
						bitmap_get_pixel_data_size(previous_bitmap);
					match_assert(
						"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
						0x35B,
						space_between>=0);
					if (space_between != 0)
					{
						error(
							_error_silent,
							"### WARNING bitmap group pixel data isn't tight");
					}
				}

				previous_bitmap = bitmap;
				pixels_end = bitmap_get_pixel_data_size(bitmap) +
					bitmap->pixels_offset;
			}
		}

		{
			struct bitmap_data *new_bitmap = TAG_BLOCK_GET_ELEMENT(
				&group->bitmaps,
				previous_count,
				struct bitmap_data);

			match_assert(
				"c:\\halo\\SOURCE\\bitmaps\\bitmap_group.c",
				0x371,
				new_bitmap);
			csmemcpy(new_bitmap, &fake_bitmap, sizeof(fake_bitmap));
			new_bitmap->pixels_offset = pixels_end;
			new_bitmap->base_address = (byte *)group->pixel_data.address + pixels_end;
			csmemset(new_bitmap->base_address, 0, pixel_data_size);
		}

		return (short)previous_count;
	}

	error(
		_error_silent,
		"### ERROR failed to add bitmap to group (tag resize failed)");
	tag_block_resize(&group->bitmaps, previous_count);
	return NONE;
}

/* ---------- private code */

static boolean postprocess_bitmap(
	struct bitmap_data *bitmap,
	boolean editing)
{
	return TRUE;
}

static void delete_bitmap(
	struct tag_block *block,
	long element_index)
{
	bitmap_delete(TAG_BLOCK_GET_ELEMENT(block, element_index, struct bitmap_data));
	return;
}

static boolean postprocess_bitmap_group(
	long bitmap_group_index,
	boolean editing)
{
	struct bitmap_group *group = bitmap_group_get(bitmap_group_index);
	boolean result = TRUE;
	short bitmap_index;
	short sequence_index;

	for (bitmap_index = 0;
		bitmap_index < group->bitmaps.count;
		bitmap_index = (short)(bitmap_index + 1))
	{
		struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
			&group->bitmaps,
			bitmap_index,
			struct bitmap_data);

		if (group->type == _bitmap_group_type_interface_bitmaps)
			SET_FLAG(bitmap->flags, _bitmap_linear_bit, TRUE);

		if (bitmap_verify(bitmap, FALSE))
			texture_cache_bitmap_new(bitmap_group_index, bitmap);
		else
			result = FALSE;
	}

	for (sequence_index = 0;
		sequence_index < group->sequences.count;
		sequence_index = (short)(sequence_index + 1))
	{
		struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
			&group->sequences,
			sequence_index,
			struct bitmap_group_sequence);

		if (sequence_index < group->sequences.count - 1)
		{
			(void)TAG_BLOCK_GET_ELEMENT(
				&group->sequences,
				sequence_index + 1,
				struct bitmap_group_sequence);
		}

		if (group->type == _bitmap_group_type_sprites &&
			(sequence->first_bitmap_index || sequence->bitmap_count))
		{
			TAG_BLOCK_GET_ELEMENT(
				&group->sequences,
				sequence_index,
				struct bitmap_group_sequence)->first_bitmap_index = 0;
			TAG_BLOCK_GET_ELEMENT(
				&group->sequences,
				sequence_index,
				struct bitmap_group_sequence)->bitmap_count = 0;
		}
	}

	if (group->sequences.count > 0)
	{
		struct bitmap_group_sequence *last_sequence = TAG_BLOCK_GET_ELEMENT(
			&group->sequences,
			group->sequences.count - 1,
			struct bitmap_group_sequence);

		if (!last_sequence->bitmap_count && !last_sequence->sprites.count &&
			!tag_block_resize(&group->sequences, group->sequences.count - 1))
		{
			error(
				_error_immediate,
				"### FATAL_ERROR failed to fix bitmap group '%s'",
				tag_get_name(bitmap_group_index));
			result = FALSE;
		}
	}

	if (find_all_fucked_up_shit)
	{
		for (bitmap_index = 0;
			bitmap_index < group->bitmaps.count;
			bitmap_index = (short)(bitmap_index + 1))
		{
			struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
				&group->bitmaps,
				bitmap_index,
				struct bitmap_data);

			if (bitmap->format == _bitmap_format_a8y8)
			{
				error(
					_error_silent,
					"!!MUST BE FIXED: bitmap #%d of group '%s' has a8y8 format",
					bitmap_index,
					tag_get_name(bitmap_group_index));
			}
			if (TEST_FLAG(bitmap->flags, _bitmap_linear_bit) &&
				!(bitmap->width & (bitmap->width - 1)) &&
				!(bitmap->height & (bitmap->height - 1)))
			{
				error(
					_error_silent,
					"!!MUST BE FIXED: bitmap #%d of group '%s' is linear and power-of-two",
					bitmap_index,
					tag_get_name(bitmap_group_index));
			}
		}

		if (group->bitmaps.count < 1)
		{
			error(
				_error_silent,
				"!!MUST BE FIXED: ",
				"bitmap group '%s' has %d bitmaps",
				tag_get_name(bitmap_group_index),
				group->bitmaps.count);
		}
		if (group->sequences.count < 1)
		{
			error(
				_error_silent,
				"!!MUST BE FIXED: ",
				"bitmap group '%s' has %d sequences",
				tag_get_name(bitmap_group_index),
				group->sequences.count);
		}

		for (sequence_index = 0;
			sequence_index < group->sequences.count;
			sequence_index = (short)(sequence_index + 1))
		{
			struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
				&group->sequences,
				sequence_index,
				struct bitmap_group_sequence);
			struct bitmap_group_sequence *next_sequence;
			short sprite_index;

			if (sequence_index < group->sequences.count - 1)
			{
				next_sequence = TAG_BLOCK_GET_ELEMENT(
					&group->sequences,
					sequence_index + 1,
					struct bitmap_group_sequence);
			}
			else
			{
				next_sequence = NULL;
			}

			if (group->type == _bitmap_group_type_sprites)
			{
				if (sequence->first_bitmap_index || sequence->bitmap_count)
				{
					error(
						_error_silent,
						"!!MUST BE FIXED: bitmap group '%s' (type=%d) sequence #%d doesn't know it's a sprite sequence",
						tag_get_name(bitmap_group_index),
						group->type,
						sequence_index);
				}
			}
			else if (sequence->first_bitmap_index < 0 ||
				sequence->first_bitmap_index >= group->bitmaps.count ||
				sequence->bitmap_count < 1 ||
				sequence->first_bitmap_index + sequence->bitmap_count > group->bitmaps.count ||
				(sequence_index == 0 && sequence->first_bitmap_index != 0) ||
				(next_sequence && next_sequence->first_bitmap_index !=
					sequence->first_bitmap_index + sequence->bitmap_count))
			{
				error(
					_error_silent,
					"!!MUST BE FIXED: bitmap group '%s' sequence #%d references bitmaps [#%d..#%d]",
					tag_get_name(bitmap_group_index),
					sequence_index,
					sequence->first_bitmap_index,
					sequence->first_bitmap_index + sequence->bitmap_count);
			}

			if (group->type == _bitmap_group_type_sprites)
			{
				if (sequence->sprites.count < 1)
				{
					error(
						_error_silent,
						"!!MUST BE FIXED: bitmap group '%s' sequence #%d has %d sprites",
						tag_get_name(bitmap_group_index),
						sequence_index,
						sequence->sprites.count);
				}
				else
				{
					sprite_index = 0;
					for (;
						sprite_index < sequence->sprites.count;
						sprite_index = (short)(sprite_index + 1))
					{
						short sprite_bitmap_index = TAG_BLOCK_GET_ELEMENT(
							&sequence->sprites,
							sprite_index,
							struct bitmap_group_sprite)->bitmap_index;

						if (sprite_bitmap_index < 0 ||
							sprite_bitmap_index >= group->bitmaps.count)
						{
							error(
								_error_silent,
								"!!MUST BE FIXED: bitmap group '%s' sequence #%d sprite #%d references bitmap #%d",
								tag_get_name(bitmap_group_index),
								sequence_index,
								sprite_index,
								sprite_bitmap_index);
						}
					}
				}
			}
			else if (sequence->sprites.count > 0)
			{
				error(
					_error_silent,
					"!!MUST BE FIXED: bitmap group '%s' (type=%d) sequence #%d has %d sprites",
					tag_get_name(bitmap_group_index),
					group->type,
					sequence_index,
					sequence->sprites.count);
			}
		}
	}

	return result;
}
