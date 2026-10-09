/*
XBOX_TEXTURES_VK.C

Xbox texture decoding and the texture cache for the Vulkan renderer. A copy of
port/linux/src/xbox_textures.c, which the OpenGL renderer and
the other builds keep unchanged (this one replaces it in the Vulkan image's objects, as d3d8_vk.c replaces d3d8_gl.c): the
decoding (formats, geometry, swizzling, palettes) is the same text; what differs is where a decoded texture goes. Here the cache
keeps a number for each texture, which names an image the host makes (vk_commands.h), and a texture's texels go to the host a
level of a face at a time, put with vk_data_put when the cache uploads them, so they are the game's as they were then: BC1 to
BC3 as they are where the device samples them (host_vk_bc_supported), decoded to BGRA where not, every other format decoded to
32-bit BGRA.

A cached texture stays valid until a page it was read from is written (memory_watch.c's generations: the page protection, and
the writes the game announces).

The OpenGL version's replacements of a texture by a high-res HUD bitmap, by the text's atlas and by a menu's art
(hud_hires.h, text_hires.h, menu_files.h) are images of their own here, numbered from the same allocator
(VK_HIRES says whether they are on).
*/

#include "xgpu.h"
#include "port_config.h"
#include "vk_commands.h"
#include "vk_device.h"
#include "hud_hires.h"
#include "menu_files.h"
#include "text_hires.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the high-res HUD, text and menu replacements (step 3) */
#define VK_HIRES 1

/* ---------- formats */

enum texel_kind
{
	_texel_unknown,
	_texel_a8r8g8b8, _texel_x8r8g8b8, _texel_r5g6b5, _texel_a1r5g5b5, _texel_x1r5g5b5, _texel_a4r4g4b4,
	_texel_l8, _texel_al8, _texel_a8, _texel_a8l8, _texel_p8, _texel_g8b8, _texel_r8b8, _texel_r6g5b5,
	_texel_l16, _texel_v16u16, _texel_a8b8g8r8, _texel_b8g8r8a8, _texel_r8g8b8a8, _texel_r5g5b5a1,
	_texel_r4g4b4a4, _texel_yuy2, _texel_uyvy, _texel_d24s8, _texel_d16,
	_texel_dxt1, _texel_dxt3, _texel_dxt5,
};

struct format_information
{
	unsigned char kind;
	unsigned char bytes; /* per texel; per 4x4 block for DXT */
	unsigned char linear;
};

static struct format_information format_information(DWORD format)
{
	static const struct format_information table[0x42] =
	{
		[0x00] = { _texel_l8, 1, 0 },
		[0x01] = { _texel_al8, 1, 0 },
		[0x02] = { _texel_a1r5g5b5, 2, 0 },
		[0x03] = { _texel_x1r5g5b5, 2, 0 },
		[0x04] = { _texel_a4r4g4b4, 2, 0 },
		[0x05] = { _texel_r5g6b5, 2, 0 },
		[0x06] = { _texel_a8r8g8b8, 4, 0 },
		[0x07] = { _texel_x8r8g8b8, 4, 0 },
		[0x0b] = { _texel_p8, 1, 0 },
		[0x0c] = { _texel_dxt1, 8, 0 },
		[0x0e] = { _texel_dxt3, 16, 0 },
		[0x0f] = { _texel_dxt5, 16, 0 },
		[0x10] = { _texel_a1r5g5b5, 2, 1 },
		[0x11] = { _texel_r5g6b5, 2, 1 },
		[0x12] = { _texel_a8r8g8b8, 4, 1 },
		[0x13] = { _texel_l8, 1, 1 },
		[0x16] = { _texel_r8b8, 2, 1 },
		[0x17] = { _texel_g8b8, 2, 1 },
		[0x19] = { _texel_a8, 1, 0 },
		[0x1a] = { _texel_a8l8, 2, 0 },
		[0x1b] = { _texel_al8, 1, 1 },
		[0x1c] = { _texel_x1r5g5b5, 2, 1 },
		[0x1d] = { _texel_a4r4g4b4, 2, 1 },
		[0x1e] = { _texel_x8r8g8b8, 4, 1 },
		[0x1f] = { _texel_a8, 1, 1 },
		[0x20] = { _texel_a8l8, 2, 1 },
		[0x24] = { _texel_yuy2, 2, 1 },
		[0x25] = { _texel_uyvy, 2, 1 },
		[0x27] = { _texel_r6g5b5, 2, 0 },
		[0x28] = { _texel_g8b8, 2, 0 },
		[0x29] = { _texel_r8b8, 2, 0 },
		[0x2a] = { _texel_d24s8, 4, 0 },
		[0x2b] = { _texel_d24s8, 4, 0 },
		[0x2c] = { _texel_d16, 2, 0 },
		[0x2d] = { _texel_d16, 2, 0 },
		[0x2e] = { _texel_d24s8, 4, 1 },
		[0x2f] = { _texel_d24s8, 4, 1 },
		[0x30] = { _texel_d16, 2, 1 },
		[0x31] = { _texel_d16, 2, 1 },
		[0x32] = { _texel_l16, 2, 0 },
		[0x33] = { _texel_v16u16, 4, 0 },
		[0x35] = { _texel_l16, 2, 1 },
		[0x36] = { _texel_v16u16, 4, 1 },
		[0x37] = { _texel_r6g5b5, 2, 1 },
		[0x38] = { _texel_r5g5b5a1, 2, 0 },
		[0x39] = { _texel_r4g4b4a4, 2, 0 },
		[0x3a] = { _texel_a8b8g8r8, 4, 0 },
		[0x3b] = { _texel_b8g8r8a8, 4, 0 },
		[0x3c] = { _texel_r8g8b8a8, 4, 0 },
		[0x3d] = { _texel_r5g5b5a1, 2, 1 },
		[0x3e] = { _texel_r4g4b4a4, 2, 1 },
		[0x3f] = { _texel_a8b8g8r8, 4, 1 },
		[0x40] = { _texel_b8g8r8a8, 4, 1 },
		[0x41] = { _texel_r8g8b8a8, 4, 1 },
	};
	struct format_information unknown = { _texel_a8r8g8b8, 4, 0 };

	if (format < sizeof(table) / sizeof(table[0]) && table[format].kind != _texel_unknown)
		return table[format];
	return unknown;
}

static BOOL kind_compressed(unsigned char kind)
{
	return kind == _texel_dxt1 || kind == _texel_dxt3 || kind == _texel_dxt5;
}

/* ---------- geometry of a texture in memory */

static unsigned long floor_log2(unsigned long value)
{
	unsigned long result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}
	return result;
}

static unsigned long level_dimension(unsigned long base, unsigned long level)
{
	unsigned long value = base >> level;

	return value ? value : 1;
}

void xgpu_texture_describe(DWORD format_word, DWORD size_word, struct xgpu_texture_description *description)
{
	struct format_information information;

	memset(description, 0, sizeof(*description));
	description->format = (format_word & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT;
	information = format_information(description->format);
	description->cube_map = (format_word & D3DFORMAT_CUBEMAP) != 0;
	description->compressed = kind_compressed(information.kind);
	if (size_word)
	{
		description->width = (size_word & D3DSIZE_WIDTH_MASK) + 1;
		description->height = ((size_word & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
		description->depth = 1;
		description->levels = 1;
		description->pitch = (((size_word & D3DSIZE_PITCH_MASK) >> D3DSIZE_PITCH_SHIFT) + 1) * D3DTEXTURE_PITCH_ALIGNMENT;
		description->linear = TRUE;
	}
	else
	{
		description->width = 1UL << ((format_word & D3DFORMAT_USIZE_MASK) >> D3DFORMAT_USIZE_SHIFT);
		description->height = 1UL << ((format_word & D3DFORMAT_VSIZE_MASK) >> D3DFORMAT_VSIZE_SHIFT);
		description->depth = 1UL << ((format_word & D3DFORMAT_PSIZE_MASK) >> D3DFORMAT_PSIZE_SHIFT);
		description->levels = (format_word & D3DFORMAT_MIPMAP_MASK) >> D3DFORMAT_MIPMAP_SHIFT;
		if (!description->levels)
			description->levels = 1;
		description->linear = information.linear;
		description->pitch = description->width * information.bytes;
	}
	if ((format_word & D3DFORMAT_DIMENSION_MASK) >> D3DFORMAT_DIMENSION_SHIFT != 3)
		description->depth = 1;
}

static unsigned long level_bytes(const struct xgpu_texture_description *description, unsigned long level)
{
	struct format_information information = format_information(description->format);
	unsigned long width = level_dimension(description->width, level);
	unsigned long height = level_dimension(description->height, level);
	unsigned long depth = level_dimension(description->depth, level);

	if (description->compressed)
		return ((width + 3) / 4) * ((height + 3) / 4) * information.bytes * depth;
	if (description->linear)
		return description->pitch * height;
	return width * height * depth * information.bytes;
}

unsigned long xgpu_texture_level_offset(const struct xgpu_texture_description *description, unsigned long level)
{
	unsigned long offset = 0;
	unsigned long index;

	for (index = 0; index < level && index < description->levels; index++)
		offset += level_bytes(description, index);
	return offset;
}

unsigned long xgpu_texture_face_size(const struct xgpu_texture_description *description)
{
	unsigned long size = xgpu_texture_level_offset(description, description->levels);

	if (description->cube_map)
		size = (size + D3DTEXTURE_CUBEFACE_ALIGNMENT - 1) & ~(unsigned long)(D3DTEXTURE_CUBEFACE_ALIGNMENT - 1);
	return size;
}

unsigned long xgpu_texture_level_pitch(const struct xgpu_texture_description *description, unsigned long level)
{
	struct format_information information = format_information(description->format);

	if (description->linear)
		return description->pitch;
	if (description->compressed)
		return ((level_dimension(description->width, level) + 3) / 4) * information.bytes;
	return level_dimension(description->width, level) * information.bytes;
}

/* ---------- swizzling */

struct swizzle_masks
{
	unsigned long x, y, z;
};

static struct swizzle_masks swizzle_masks(unsigned long width, unsigned long height, unsigned long depth)
{
	struct swizzle_masks masks = { 0, 0, 0 };
	unsigned long bit = 1, mask_bit = 1;
	BOOL done;

	/* bits of x, y and z alternate until each dimension runs out */
	do
	{
		done = TRUE;
		if (bit < width)
		{
			masks.x |= mask_bit;
			mask_bit <<= 1;
			done = FALSE;
		}
		if (bit < height)
		{
			masks.y |= mask_bit;
			mask_bit <<= 1;
			done = FALSE;
		}
		if (bit < depth)
		{
			masks.z |= mask_bit;
			mask_bit <<= 1;
			done = FALSE;
		}
		bit <<= 1;
	} while (!done);
	return masks;
}

static unsigned long spread(unsigned long mask, unsigned long value)
{
	unsigned long result = 0, bit = 1;

	while (value && bit)
	{
		if (mask & bit)
		{
			if (value & 1)
				result |= bit;
			value >>= 1;
		}
		bit <<= 1;
	}
	return result;
}

/* ---------- texel conversion */

static unsigned long expand5(unsigned long v) { return (v << 3) | (v >> 2); }
static unsigned long expand6(unsigned long v) { return (v << 2) | (v >> 4); }
static unsigned long expand4(unsigned long v) { return v * 0x11; }

static unsigned long argb(unsigned long a, unsigned long r, unsigned long g, unsigned long b)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static unsigned char clamp_byte(long value)
{
	return (unsigned char)(value < 0 ? 0 : value > 255 ? 255 : value);
}

static unsigned long yuv_to_argb(long y, long u, long v)
{
	long c = y - 16, d = u - 128, e = v - 128;

	return argb(255, clamp_byte((298 * c + 409 * e + 128) >> 8),
		clamp_byte((298 * c - 100 * d - 208 * e + 128) >> 8),
		clamp_byte((298 * c + 516 * d + 128) >> 8));
}

static unsigned long convert_texel(unsigned char kind, const unsigned char *source, const D3DCOLOR *palette,
	unsigned long x, const unsigned char *row)
{
	unsigned long v16 = source[0] | ((unsigned long)source[1] << 8);
	unsigned long v32 = v16 | ((unsigned long)source[2] << 16) | ((unsigned long)source[3] << 24);

	switch (kind)
	{
	case _texel_a8r8g8b8: return v32;
	case _texel_x8r8g8b8: return v32 | 0xff000000UL;
	case _texel_r5g6b5: return argb(255, expand5(v16 >> 11), expand6((v16 >> 5) & 0x3f), expand5(v16 & 0x1f));
	case _texel_a1r5g5b5: return argb((v16 & 0x8000) ? 255 : 0, expand5((v16 >> 10) & 0x1f), expand5((v16 >> 5) & 0x1f), expand5(v16 & 0x1f));
	case _texel_x1r5g5b5: return argb(255, expand5((v16 >> 10) & 0x1f), expand5((v16 >> 5) & 0x1f), expand5(v16 & 0x1f));
	case _texel_a4r4g4b4: return argb(expand4(v16 >> 12), expand4((v16 >> 8) & 0xf), expand4((v16 >> 4) & 0xf), expand4(v16 & 0xf));
	case _texel_l8: return argb(255, source[0], source[0], source[0]);
	case _texel_al8: return argb(source[0], source[0], source[0], source[0]);
	case _texel_a8: return argb(source[0], 255, 255, 255);
	case _texel_a8l8: return argb(source[1], source[0], source[0], source[0]);
	case _texel_p8: return palette ? palette[source[0]] : argb(255, source[0], source[0], source[0]);
	/* V8U8 shares this format: U (the low byte) reads as red, V as green */
	case _texel_g8b8: return argb(255, source[0], source[1], 0);
	case _texel_r8b8: return argb(255, source[1], 0, source[0]);
	case _texel_r6g5b5: return argb(255, expand6(v16 >> 10), expand5((v16 >> 5) & 0x1f), expand5(v16 & 0x1f));
	case _texel_l16: return argb(255, source[1], source[1], source[1]);
	case _texel_v16u16: return argb(255, source[1], source[3], 0);
	case _texel_a8b8g8r8: return argb(source[3], source[0], source[1], source[2]);
	case _texel_b8g8r8a8: return argb(source[0], source[1], source[2], source[3]);
	case _texel_r8g8b8a8: return argb(source[0], source[3], source[2], source[1]);
	case _texel_r5g5b5a1: return argb((v16 & 1) ? 255 : 0, expand5(v16 >> 11), expand5((v16 >> 6) & 0x1f), expand5((v16 >> 1) & 0x1f));
	case _texel_r4g4b4a4: return argb(expand4(v16 & 0xf), expand4(v16 >> 12), expand4((v16 >> 8) & 0xf), expand4((v16 >> 4) & 0xf));
	case _texel_yuy2:
	{
		const unsigned char *pair = row + (x & ~1UL) * 2;

		return yuv_to_argb(pair[(x & 1) ? 2 : 0], pair[1], pair[3]);
	}
	case _texel_uyvy:
	{
		const unsigned char *pair = row + (x & ~1UL) * 2;

		return yuv_to_argb(pair[(x & 1) ? 3 : 1], pair[0], pair[2]);
	}
	case _texel_d24s8: return argb(255, source[3], source[3], source[3]);
	case _texel_d16: return argb(255, source[1], source[1], source[1]);
	default: return v32;
	}
}

/* one level (or 3D slice set) of an uncompressed texture into BGRA */
static void decode_level(const struct xgpu_texture_description *description, unsigned long level,
	const unsigned char *source, const D3DCOLOR *palette, unsigned long *destination)
{
	struct format_information information = format_information(description->format);
	unsigned long width = level_dimension(description->width, level);
	unsigned long height = level_dimension(description->height, level);
	unsigned long depth = level_dimension(description->depth, level);
	unsigned long x, y, z;

	if (description->linear)
	{
		for (y = 0; y < height; y++)
		{
			const unsigned char *row = source + y * description->pitch;

			for (x = 0; x < width; x++)
				destination[y * width + x] = convert_texel(information.kind, row + x * information.bytes, palette, x, row);
		}
		return;
	}
	{
		struct swizzle_masks masks = swizzle_masks(width, height, depth);
		unsigned long *x_offsets = malloc(width * sizeof(unsigned long));

		for (x = 0; x < width; x++)
			x_offsets[x] = spread(masks.x, x);
		for (z = 0; z < depth; z++)
		{
			unsigned long z_offset = spread(masks.z, z);

			for (y = 0; y < height; y++)
			{
				unsigned long y_offset = spread(masks.y, y) | z_offset;

				for (x = 0; x < width; x++)
				{
					const unsigned char *texel = source + (x_offsets[x] | y_offset) * information.bytes;

					destination[(z * height + y) * width + x] = convert_texel(information.kind, texel, palette, x, texel);
				}
			}
		}
		free(x_offsets);
	}
}
/* ---------- upload */

/* the bytes of a decoded level: 32 bits a texel, whatever the format was */
static unsigned long decoded_bytes(const struct xgpu_texture_description *description, unsigned long level)
{
	return level_dimension(description->width, level) * level_dimension(description->height, level) *
		level_dimension(description->depth, level) * 4;
}

/* ---------- DXT decoding, for ES drivers without S3TC (Mali) */

static unsigned long color565(unsigned long value)
{
	return argb(255, expand5(value >> 11), expand6((value >> 5) & 0x3f), expand5(value & 0x1f));
}

static unsigned long mix(unsigned long a, unsigned long b, unsigned long weight_a, unsigned long weight_b,
	unsigned long divisor)
{
	unsigned long result = 0;
	int shift;

	for (shift = 0; shift < 24; shift += 8)
	{
		unsigned long channel = (((a >> shift) & 0xff) * weight_a + ((b >> shift) & 0xff) * weight_b) / divisor;

		result |= channel << shift;
	}
	return result | 0xff000000UL;
}

/* one 4x4 block's colors; dxt1 selects the punch-through alpha mode */
static void dxt_color_block(const unsigned char *block, BOOL dxt1, unsigned long colors[16])
{
	unsigned long c0 = block[0] | (block[1] << 8);
	unsigned long c1 = block[2] | (block[3] << 8);
	unsigned long palette[4];
	unsigned long bits = block[4] | (block[5] << 8) | ((unsigned long)block[6] << 16) | ((unsigned long)block[7] << 24);
	int index;

	palette[0] = color565(c0);
	palette[1] = color565(c1);
	if (c0 > c1 || !dxt1)
	{
		palette[2] = mix(palette[0], palette[1], 2, 1, 3);
		palette[3] = mix(palette[0], palette[1], 1, 2, 3);
	}
	else
	{
		palette[2] = mix(palette[0], palette[1], 1, 1, 2);
		palette[3] = 0;
	}
	for (index = 0; index < 16; index++)
		colors[index] = palette[(bits >> (index * 2)) & 3];
}

static void dxt_decode_level(unsigned char kind, const unsigned char *source, unsigned long width, unsigned long height,
	unsigned long depth, unsigned long *destination)
{
	unsigned long blocks_x = (width + 3) / 4, blocks_y = (height + 3) / 4;
	unsigned long block_bytes = kind == _texel_dxt1 ? 8 : 16;
	unsigned long z, bx, by, x, y;

	for (z = 0; z < depth; z++)
	{
		for (by = 0; by < blocks_y; by++)
		{
			for (bx = 0; bx < blocks_x; bx++)
			{
				const unsigned char *block = source + ((z * blocks_y + by) * blocks_x + bx) * block_bytes;
				unsigned long colors[16];
				unsigned long alpha[16];
				int index;

				if (kind == _texel_dxt1)
				{
					dxt_color_block(block, TRUE, colors);
					for (index = 0; index < 16; index++)
						alpha[index] = colors[index] >> 24;
				}
				else
				{
					dxt_color_block(block + 8, FALSE, colors);
					if (kind == _texel_dxt3)
					{
						for (index = 0; index < 16; index++)
							alpha[index] = expand4((block[index / 2] >> ((index & 1) * 4)) & 0xf);
					}
					else
					{
						unsigned long a0 = block[0], a1 = block[1], values[8];
						unsigned long long bits = 0;
						int bit;

						for (bit = 0; bit < 6; bit++)
							bits |= (unsigned long long)block[2 + bit] << (bit * 8);
						values[0] = a0;
						values[1] = a1;
						if (a0 > a1)
						{
							for (index = 2; index < 8; index++)
								values[index] = ((8 - index) * a0 + (index - 1) * a1) / 7;
						}
						else
						{
							for (index = 2; index < 6; index++)
								values[index] = ((6 - index) * a0 + (index - 1) * a1) / 5;
							values[6] = 0;
							values[7] = 255;
						}
						for (index = 0; index < 16; index++)
							alpha[index] = values[(bits >> (index * 3)) & 7];
					}
				}
				for (y = 0; y < 4; y++)
				{
					for (x = 0; x < 4; x++)
					{
						unsigned long px = bx * 4 + x, py = by * 4 + y;

						if (px < width && py < height)
						{
							destination[(z * height + py) * width + px] =
								(colors[y * 4 + x] & 0x00ffffffUL) | (alpha[y * 4 + x] << 24);
						}
					}
				}
			}
		}
	}
}

static uint32_t compressed_format(unsigned char kind)
{
	switch (kind)
	{
	case _texel_dxt1: return VK_IMAGE_FORMAT_BC1;
	case _texel_dxt3: return VK_IMAGE_FORMAT_BC2;
	default: return VK_IMAGE_FORMAT_BC3;
	}
}

uint32_t host_vk_bc_supported(void);

/* whether BC1 to BC3 go to the device as they are */
static BOOL bc_native(void)
{
	static int known = -1;

	if (known < 0)
	{
		known = host_vk_bc_supported() != 0;
		platform_log("vk: BC1 to BC3 textures %s", known ? "are sampled as they are" : "are decoded to BGRA");
	}
	return known;
}

/* sends a level of a face: its texels put, and the command that names them. rows is 0 for all of the level, else the 2D BGRA
rows [top, top + rows) of it */
static void data_send(uint32_t id, unsigned long level, unsigned long face, const void *texels, unsigned long bytes,
	unsigned long top, unsigned long rows)
{
	uint32_t put = vk_data_put(texels, bytes);
	struct vk_command_texture_data *command = vk_stream_command(VK_COMMAND_TEXTURE_DATA, sizeof(*command));

	command->id = id;
	command->level = (uint32_t)level;
	command->face = (uint32_t)face;
	command->data.id = put;
	command->data.offset = 0;
	command->top = (uint32_t)top;
	command->rows = (uint32_t)rows;
}

/* Sends a texture's texels to the host as image id: the image's description, then every level of every face. A compressed
texture is read where the game keeps it (BC) or decoded; any other is decoded to BGRA. */
static void upload(uint32_t id, uint32_t kind, const struct xgpu_texture_description *description,
	const unsigned char *base, const D3DCOLOR *palette)
{
	struct format_information information = format_information(description->format);
	unsigned long face_count = description->cube_map ? 6 : 1;
	unsigned long face_size = xgpu_texture_face_size(description);
	/* (a 3D image of a BC format is optional in Vulkan, where 2D and cube ones are not: a 3D one is decoded) */
	BOOL native = description->compressed && description->depth <= 1 && bc_native();
	struct vk_command_texture *command = vk_stream_command(VK_COMMAND_TEXTURE, sizeof(*command));
	unsigned long face, level;
	unsigned long *converted = NULL;

	command->id = id;
	command->kind = kind;
	command->format = native ? compressed_format(information.kind) : VK_IMAGE_FORMAT_BGRA;
	command->width = (uint32_t)description->width;
	command->height = (uint32_t)description->height;
	command->depth = (uint32_t)description->depth;
	command->levels = (uint32_t)description->levels;
	if (!native)
	{
		converted = malloc(decoded_bytes(description, 0));
		if (!converted)
			return;
	}
	for (face = 0; face < face_count; face++)
	{
		for (level = 0; level < description->levels; level++)
		{
			const unsigned char *source = base + face * face_size + xgpu_texture_level_offset(description, level);

			if (native)
			{
				data_send(id, level, face, source, level_bytes(description, level), 0, 0);
				continue;
			}
			if (description->compressed)
				dxt_decode_level(information.kind, source, level_dimension(description->width, level),
					level_dimension(description->height, level), level_dimension(description->depth, level), converted);
			else
				decode_level(description, level, source, palette, converted);
			data_send(id, level, face, converted, decoded_bytes(description, level), 0, 0);
		}
	}
	free(converted);
}

/* ---------- cache */

struct texture_entry
{
	struct texture_entry *next;
	DWORD data, format_word, size_word;
	unsigned long palette_hash;
	/* the image the host makes of it, 0 until the first upload is sent */
	uint32_t id;
	uint32_t kind;
	struct xgpu_texture_description description;
	unsigned long address, size;
	unsigned long generation;
	unsigned long last_used_frame;
	/* the high-res HUD bitmap that stands for it (hud_hires.h), or -1 */
	long override;
	/* its menu art's image and levels (0: none), as of menu_art_serial */
	unsigned long menu_serial;
	uint32_t menu_id;
	unsigned long menu_levels;
};

#define TEXTURE_BUCKET_COUNT 4096
#define TEXTURE_IDLE_FRAMES 1800
#define MAXIMUM_PALETTE_VARIANTS 8

static struct texture_entry *texture_buckets[TEXTURE_BUCKET_COUNT];

/* the image numbers (the host's table has VK_TEXTURE_LIMIT of them, 0 being
no texture): handed out in order, and taken back when a texture is dropped */
static uint32_t id_next = 1;
static uint32_t id_free[VK_TEXTURE_LIMIT];
static uint32_t id_free_count;

static uint32_t id_take(void)
{
	if (id_free_count)
		return id_free[--id_free_count];
	if (id_next >= VK_TEXTURE_LIMIT)
		return 0;
	return id_next++;
}

/* Draws mostly bind the textures the draws before them bound. A lookup of a
texture that is not palettized is remembered with the memory watch serial it
started at: while no watched page has been written since, and no texture
has been dropped, the same lookup finds the same current texture. */
#define RECENT_TEXTURE_COUNT 64

static struct
{
	DWORD data, format_word, size_word;
	struct texture_entry *entry;
	unsigned long watch_serial;
	unsigned long drop_serial;
} recent_textures[RECENT_TEXTURE_COUNT];
static unsigned long texture_drop_serial = 1;
static unsigned long texture_frame = 0;

static unsigned long bucket_index(DWORD data, DWORD format_word, DWORD size_word)
{
	return ((data >> 7) ^ (format_word * 2654435761UL) ^ size_word) % TEXTURE_BUCKET_COUNT;
}

/* palettized textures are cached per palette contents: the game rewrites
palettes freely, and often cycles a texture through a few of them */
static unsigned long palette_hash(const D3DCOLOR *palette)
{
	unsigned long hash = 2166136261UL, index;

	if (!palette)
		return 0;
	for (index = 0; index < 256; index++)
		hash = (hash ^ palette[index]) * 16777619UL;
	return hash ? hash : 1;
}

/* ---------- high-res replacements (step 3) */

#if VK_HIRES

/* the most bytes of rows one command names */
#define ROWS_PIECE_BYTES (1024 * 1024)

/* sends rows [top, top + rows) of a level width texels wide, whose BGRA texels are in buffer (the caller frees it: the put
copies it) */
static void rows_send(uint32_t id, unsigned long level, unsigned long width, unsigned long top, unsigned long rows,
	const unsigned char *buffer)
{
	data_send(id, level, 0, buffer, rows * width * 4, top, rows);
}

/* makes 2D BGRA image id, its texels left for rows_send and level_send */
static void image_make(uint32_t id, unsigned long width, unsigned long height, unsigned long levels)
{
	struct vk_command_texture *command = vk_stream_command(VK_COMMAND_TEXTURE, sizeof(*command));

	command->id = id;
	command->kind = VK_IMAGE_2D;
	command->format = VK_IMAGE_FORMAT_BGRA;
	command->width = (uint32_t)width;
	command->height = (uint32_t)height;
	command->depth = 1;
	command->levels = (uint32_t)levels;
}

/* sends a whole level of BGRA texels */
static BOOL level_send(uint32_t id, unsigned long level, unsigned long width, unsigned long height,
	const unsigned char *texels)
{
	data_send(id, level, 0, texels, width * height * 4, 0, 0);
	return TRUE;
}

/* an image of a PNG (8-bit RGBA) with all its mip levels, as
hud_hires_png_texture makes in GL: its number, or 0 */
static uint32_t png_image(const void *png, unsigned long size, unsigned long *levels)
{
	unsigned long width, height, level, largest, index;
	unsigned char *pixels = hud_hires_png_pixels(png, size, &width, &height);
	uint32_t id;

	if (!pixels)
		return 0;
	id = id_take();
	if (!id)
	{
		free(pixels);
		return 0;
	}
	*levels = 1;
	for (largest = width > height ? width : height; largest > 1; largest >>= 1)
		(*levels)++;
	/* (red first to blue first) */
	for (index = 0; index < width * height; index++)
	{
		unsigned char red = pixels[index * 4];

		pixels[index * 4] = pixels[index * 4 + 2];
		pixels[index * 4 + 2] = red;
	}
	image_make(id, width, height, *levels);
	for (level = 0; level < *levels; level++)
	{
		unsigned long next_width = width > 1 ? width / 2 : 1, next_height = height > 1 ? height / 2 : 1, x, y, c;
		unsigned char *next;

		level_send(id, level, width, height, pixels);
		if (level + 1 == *levels)
			break;
		/* the next level: each texel the mean of the (up to) four above it */
		next = malloc(next_width * next_height * 4);
		if (!next)
			break;
		for (y = 0; y < next_height; y++)
		{
			unsigned long y0 = y * 2, y1 = y * 2 + 1 < height ? y * 2 + 1 : y * 2;

			for (x = 0; x < next_width; x++)
			{
				unsigned long x0 = x * 2, x1 = x * 2 + 1 < width ? x * 2 + 1 : x * 2;

				for (c = 0; c < 4; c++)
					next[(y * next_width + x) * 4 + c] = (unsigned char)((pixels[(y0 * width + x0) * 4 + c] +
						pixels[(y0 * width + x1) * 4 + c] + pixels[(y1 * width + x0) * 4 + c] +
						pixels[(y1 * width + x1) * 4 + c] + 2) / 4);
			}
		}
		free(pixels);
		pixels = next;
		width = next_width;
		height = next_height;
	}
	free(pixels);
	return id;
}

/* the text's atlas (text_hires.h): its image, made and its changed rows sent
when data is its placeholder bitmap's; 0 if not */
static uint32_t atlas_image(unsigned long data)
{
	static uint32_t id;
	static BOOL failed;
	long size, top, bottom;
	const unsigned char *coverage;

	if (failed)
		return 0;
	coverage = text_hires_atlas_rows(data, &size, id == 0, &top, &bottom);
	if (!coverage)
		return 0;
	if (!id)
	{
		id = id_take();
		if (!id)
		{
			failed = TRUE;
			return 0;
		}
		image_make(id, (unsigned long)size, (unsigned long)size, 1);
	}
	while (top < bottom)
	{
		/* (white, the glyph's coverage its alpha, as the maps' fonts are) */
		long rows = ROWS_PIECE_BYTES / (size * 4), index;
		unsigned char *buffer;

		if (rows > bottom - top)
			rows = bottom - top;
		buffer = malloc((size_t)rows * size * 4);
		if (!buffer)
			break;
		for (index = 0; index < rows * size; index++)
		{
			buffer[index * 4 + 0] = buffer[index * 4 + 1] = buffer[index * 4 + 2] = 255;
			buffer[index * 4 + 3] = coverage[top * size + index];
		}
		rows_send(id, 0, (unsigned long)size, (unsigned long)top, (unsigned long)rows, buffer);
		free(buffer);
		top += rows;
	}
	return id;
}

/* a menu's art (menu_files.h), by its file: made on first use, and kept */
#define MAXIMUM_MENU_IMAGES 256

static struct
{
	char *name;
	uint32_t id;
	unsigned long levels;
} menu_images[MAXIMUM_MENU_IMAGES];
static long menu_image_count;

static uint32_t menu_image(unsigned long data, unsigned long *levels)
{
	const char *name = menu_art_name(data);
	const unsigned char *png;
	unsigned long size = 0;
	long index;

	if (!name)
		return 0;
	for (index = 0; index < menu_image_count; index++)
	{
		if (!strcmp(menu_images[index].name, name))
		{
			*levels = menu_images[index].levels;
			return menu_images[index].id;
		}
	}
	if (menu_image_count == MAXIMUM_MENU_IMAGES)
		return 0;
	/* (failed ones are remembered too, as image 0) */
	menu_images[menu_image_count].name = strdup(name);
	if (!menu_images[menu_image_count].name)
		return 0;
	png = menu_art_png(name, &size);
	menu_images[menu_image_count].id = png ? png_image(png, size, &menu_images[menu_image_count].levels) : 0;
	if (!menu_images[menu_image_count].id)
		platform_log("menus: could not draw %s", name);
	*levels = menu_images[menu_image_count].levels;
	return menu_images[menu_image_count++].id;
}

/* a high-res HUD bitmap (hud_hires.h): made on first use, and kept */
static struct
{
	uint32_t id;
	unsigned long levels;
	BOOL tried;
} *hud_images;

static uint32_t hud_image(long asset, unsigned long *levels)
{
	if (asset < 0 || asset >= (long)hud_hires_embedded_count)
		return 0;
	if (!hud_images)
	{
		hud_images = calloc(hud_hires_embedded_count, sizeof(*hud_images));
		if (!hud_images)
			return 0;
	}
	if (!hud_images[asset].tried)
	{
		const struct hud_hires_embedded *embedded = &hud_hires_embedded[asset];

		hud_images[asset].tried = TRUE;
		hud_images[asset].id = png_image(embedded->png, embedded->png_size, &hud_images[asset].levels);
		if (!hud_images[asset].id)
			platform_log("high-res hud: could not decode the texture for %s bitmap %d", embedded->tag,
				embedded->bitmap);
	}
	*levels = hud_images[asset].levels;
	return hud_images[asset].id;
}

#else

static uint32_t atlas_image(unsigned long data)
{
	(void)data;
	return 0;
}

static uint32_t menu_image(unsigned long data, unsigned long *levels)
{
	(void)data;
	*levels = 1;
	return 0;
}

static uint32_t hud_image(long asset, unsigned long *levels)
{
	(void)asset;
	*levels = 1;
	return 0;
}

#endif

/* an entry's image and description: its replacement's, if it has one (as
texture_entry_result in xbox_textures.c), with the bitmap's own size, which
its coordinates are in */
static uint32_t texture_entry_result(struct texture_entry *entry, int *kind, struct xgpu_texture_description *description)
{
	unsigned long levels = 1;
	uint32_t id;

	*kind = (int)entry->kind;
	*description = entry->description;
#if VK_HIRES
	if ((id = atlas_image(entry->data)) != 0)
	{
		*kind = VK_IMAGE_2D;
		description->levels = 1;
		return id;
	}
	/* (looked up again only when the art registered has changed: the search
	was 1.3% of the game thread, for every texture of every draw) */
	if (entry->menu_serial != menu_art_serial())
	{
		entry->menu_serial = menu_art_serial();
		entry->menu_levels = 1;
		entry->menu_id = menu_image(entry->data, &entry->menu_levels);
	}
	levels = entry->menu_levels;
	if ((id = entry->menu_id) != 0)
	{
		*kind = VK_IMAGE_2D;
		description->levels = levels;
		description->hires = TRUE;
		return id;
	}
	if (entry->override >= 0 && (id = hud_image(entry->override, &levels)) != 0)
	{
		*kind = VK_IMAGE_2D;
		description->levels = levels;
		description->hires = TRUE;
		description->hires_coverage = hud_hires_override_coverage(entry->override);
		return id;
	}
#endif
	(void)levels;
	(void)id;
	return entry->id;
}


uint32_t vk_texture_get(const DWORD *resource, const D3DCOLOR *palette, int *kind,
	struct xgpu_texture_description *description)
{
	DWORD data = resource[1], format_word = resource[3], size_word = resource[4];
	struct texture_entry **bucket = &texture_buckets[bucket_index(data, format_word, size_word)];
	struct texture_entry *entry;
	unsigned long generation;
	BOOL palettized = ((format_word & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT) == 0x0b;
	unsigned long hash = palettized ? palette_hash(palette) : 0;

	struct texture_entry *oldest_variant = NULL;
	unsigned long variant_count = 0;
	static int no_cache = -1;
	unsigned long recent = bucket_index(data, format_word, size_word) % RECENT_TEXTURE_COUNT;
	unsigned long watch_serial = memory_watch_serial();

	if (!palettized && recent_textures[recent].entry && recent_textures[recent].data == data &&
		recent_textures[recent].format_word == format_word && recent_textures[recent].size_word == size_word &&
		recent_textures[recent].watch_serial == watch_serial &&
		recent_textures[recent].drop_serial == texture_drop_serial)
	{
		entry = recent_textures[recent].entry;
		entry->last_used_frame = texture_frame;
		return texture_entry_result(entry, kind, description);
	}

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->data == data && entry->format_word == format_word && entry->size_word == size_word)
		{
			if (entry->palette_hash == hash)
				break;
			variant_count++;
			if (!oldest_variant || entry->last_used_frame < oldest_variant->last_used_frame)
				oldest_variant = entry;
		}
	}
	if (!entry && variant_count >= MAXIMUM_PALETTE_VARIANTS)
	{
		/* a palette that keeps changing reuses the stalest copy */
		entry = oldest_variant;
		entry->palette_hash = hash;
		entry->generation = 0;
	}
	if (!entry)
	{
		uint32_t id = id_take();

		if (!id)
			return 0;
		entry = calloc(1, sizeof(*entry));
		entry->data = data;
		entry->format_word = format_word;
		entry->size_word = size_word;
		entry->palette_hash = hash;
		xgpu_texture_describe(format_word, size_word, &entry->description);
		entry->kind = entry->description.cube_map ? VK_IMAGE_CUBE : entry->description.depth > 1 ? VK_IMAGE_3D :
			VK_IMAGE_2D;
		entry->address = (unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(data);
		entry->size = xgpu_texture_face_size(&entry->description) * (entry->description.cube_map ? 6 : 1);
		entry->generation = 0;
		entry->override = -1;
		entry->id = id;
		entry->next = *bucket;
		*bucket = entry;
	}

	if (no_cache < 0)
		no_cache = config_boolean("debug.texture_no_cache");
	generation = memory_watch_generation(entry->address, entry->size);
	if (!entry->generation || generation > entry->generation || no_cache)
	{
		/* mark first, so a write racing with the upload is noticed */
		memory_watch_protect(entry->address, entry->size);
		entry->generation = memory_watch_generation(entry->address, entry->size);
		if (!entry->generation)
			entry->generation = 1;
		/* (which bitmap is here may have changed with the pixels) */
		entry->override = -1;
		if (!palettized && !entry->description.cube_map && entry->description.depth == 1)
		{
			unsigned long levels;

			entry->override = hud_hires_override_find(entry->address, entry->description.width,
				entry->description.height, entry->description.levels > 1 ?
				xgpu_texture_level_offset(&entry->description, 1) : xgpu_texture_face_size(&entry->description));
			if (entry->override >= 0 && !hud_image(entry->override, &levels))
				entry->override = -1;
		}
		if (entry->override < 0 && platform_is_contiguous((void *)entry->address) &&
			platform_is_contiguous((void *)(entry->address + entry->size - 1)))
		{
			if (config_boolean("debug.texture_log"))
				platform_log("texture upload %08lx fmt %02lx %lux%lu size %lu gen %lu id %u", (unsigned long)data,
					(unsigned long)entry->description.format, entry->description.width, entry->description.height,
					entry->size, entry->generation, (unsigned)entry->id);
			upload(entry->id, entry->kind, &entry->description, (const unsigned char *)entry->address, palette);
		}
	}
	entry->last_used_frame = texture_frame;
	if (!palettized && !no_cache)
	{
		recent_textures[recent].data = data;
		recent_textures[recent].format_word = format_word;
		recent_textures[recent].size_word = size_word;
		recent_textures[recent].entry = entry;
		recent_textures[recent].watch_serial = watch_serial;
		recent_textures[recent].drop_serial = texture_drop_serial;
	}
	return texture_entry_result(entry, kind, description);
}

void vk_texture_cache_begin_frame(void)
{
	unsigned long index;

	texture_frame++;
	if (texture_frame % 600)
		return;
	/* drop textures that have not been used for a while */
	for (index = 0; index < TEXTURE_BUCKET_COUNT; index++)
	{
		struct texture_entry **link = &texture_buckets[index];

		while (*link)
		{
			struct texture_entry *entry = *link;

			if (texture_frame - entry->last_used_frame > TEXTURE_IDLE_FRAMES)
			{
				struct vk_command_texture_free *command = vk_stream_command(VK_COMMAND_TEXTURE_FREE, sizeof(*command));

				command->id = entry->id;
				*link = entry->next;
				id_free[id_free_count++] = entry->id;
				texture_drop_serial++;
				free(entry);
			}
			else
			{
				link = &entry->next;
			}
		}
	}
}

/* ---------- Custom Edition channel orders (platform.h)

xbox_textures.c samples the textures of a Halo Custom Edition map whose
channels Halo PC keeps elsewhere with each channel taken from where it is.
This cache does not: a Custom Edition map's tag data is linked at 0x40440000,
where this build's game image is (port/android/include/halo_android_abi.h),
so its tag cache is never reserved (xbox_memory.c) and no Custom Edition map
loads here. Every texture is sampled in the Xbox's order. */

void halo_custom_edition_texels_channels(const void *texels, unsigned char channel_order)
{
	(void)texels;
	(void)channel_order;
}

void halo_custom_edition_texels_forget(void)
{
}
