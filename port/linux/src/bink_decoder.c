/*
BINK_DECODER.C

Bink 1 video (revisions f to k) and Bink audio (its RDFT variant), which
the Xbox's movies use (bink_rad.c plays them). Adapted from FFmpeg 7.1:
libavcodec/bink.c, binkdsp.c and binkaudio.c, and libavformat/bink.c,
with their tables (port/third_party/ffmpeg-bink/binkdata.h) unchanged.
Only what these files need is kept: no Bink "b", no DCT audio, no
seeking. The bit reader, the Huffman tables' lookup and the audio's
inverse transform are written here in place of FFmpeg's shared ones; the
transform is the inverse real DFT FFmpeg's RDFT computes for Bink audio.
Every read is bounds-checked: a file is untrusted input.

Bink video decoder
Copyright (c) 2009 Konstantin Shishkov
Copyright (C) 2011 Peter Ross <pross@xvid.org>
Bink DSP routines
Copyright (c) 2009 Konstantin Shishkov
Bink Audio decoder
Copyright (c) 2007-2011 Peter Ross (pross@xvid.org)
Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu)
Bink demuxer
Copyright (c) 2008-2010 Peter Ross (pross@xvid.org)
Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu)

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

This file is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details (port/third_party/ffmpeg-bink/COPYING.LGPLv2.1).
*/

#include "bink_decoder.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../third_party/ffmpeg-bink/binkdata.h"

#define BINK_FLAG_ALPHA 0x00100000
#define BINK_MAXIMUM_WIDTH 7680
#define BINK_MAXIMUM_HEIGHT 4800
#define BINK_MAXIMUM_FRAMES 1000000
#define BINK_MAXIMUM_AUDIO_TRACKS 256

static unsigned long read32(const unsigned char *data)
{
	return (unsigned long)data[0] | ((unsigned long)data[1] << 8) | ((unsigned long)data[2] << 16) |
		((unsigned long)data[3] << 24);
}

/* ---------- the container (libavformat/bink.c's read_header) */

int bink_header_read(const unsigned char *data, unsigned long size, struct bink_header *header)
{
	unsigned long offset;

	memset(header, 0, sizeof(*header));
	if (size < BINK_HEADER_MINIMUM_SIZE || data[0] != 'B' || data[1] != 'I' || data[2] != 'K')
		return 0;
	header->revision = data[3];
	if (header->revision < 'f' || header->revision > 'k')
		return 0;
	header->file_size = read32(data + 4) + 8;
	header->frame_count = read32(data + 8);
	header->largest_frame_size = read32(data + 12);
	header->width = read32(data + 20);
	header->height = read32(data + 24);
	header->frame_rate = read32(data + 28);
	header->frame_rate_divisor = read32(data + 32);
	header->video_flags = read32(data + 36);
	header->audio_track_count = read32(data + 40);
	if (!header->frame_count || header->frame_count > BINK_MAXIMUM_FRAMES ||
		header->largest_frame_size > header->file_size ||
		!header->width || header->width > BINK_MAXIMUM_WIDTH ||
		!header->height || header->height > BINK_MAXIMUM_HEIGHT ||
		!header->frame_rate || !header->frame_rate_divisor ||
		header->audio_track_count > BINK_MAXIMUM_AUDIO_TRACKS)
		return 0;
	offset = 44;
	/* (an unknown field) */
	if (header->revision == 'k')
		offset += 4;
	if (header->audio_track_count)
	{
		/* each track's largest decoded size, its rate and flags, its id */
		if (size < offset + header->audio_track_count * 12)
			return 0;
		header->audio_sample_rate = (unsigned long)data[offset + header->audio_track_count * 4] |
			((unsigned long)data[offset + header->audio_track_count * 4 + 1] << 8);
		header->audio_flags = (unsigned long)data[offset + header->audio_track_count * 4 + 2] |
			((unsigned long)data[offset + header->audio_track_count * 4 + 3] << 8);
		offset += header->audio_track_count * 12;
	}
	header->index_offset = offset;
	return 1;
}

/* ---------- the bit reader: little-endian, as FFmpeg's BITSTREAM_READER_LE */

struct bit_reader
{
	const unsigned char *data;
	unsigned long size;      /* in bytes, with 8 more readable past them */
	long position;           /* in bits */
	long size_in_bits;
};

static void bits_initialize(struct bit_reader *reader, const unsigned char *data, unsigned long size)
{
	reader->data = data;
	reader->size = size;
	reader->position = 0;
	reader->size_in_bits = (long)(size * 8);
}

static long bits_left(const struct bit_reader *reader)
{
	return reader->size_in_bits - reader->position;
}

/* the next count bits (up to 25), not taken; zeros past the end */
static unsigned long bits_show(const struct bit_reader *reader, int count)
{
	unsigned long byte_index = (unsigned long)reader->position >> 3;
	const unsigned char *at;

	if (reader->position < 0 || byte_index >= reader->size)
		return 0;
	at = reader->data + byte_index;
	return (read32(at) >> (reader->position & 7)) & ((1UL << count) - 1);
}

static unsigned long bits_get(struct bit_reader *reader, int count)
{
	unsigned long value;

	if (!count)
		return 0;
	value = bits_show(reader, count);
	reader->position += count;
	return value;
}

static int bits_get1(struct bit_reader *reader)
{
	return (int)bits_get(reader, 1);
}

static void bits_skip(struct bit_reader *reader, long count)
{
	reader->position += count;
}

static void bits_align32(struct bit_reader *reader)
{
	if (reader->position & 31)
		reader->position += 32 - (reader->position & 31);
}

/* ---------- Huffman trees: the 16 fixed ones of binkdata.h */

#define TREE_MAXIMUM_BITS 7

struct tree_entry
{
	unsigned char symbol;
	unsigned char length;
};

static struct tree_entry tree_tables[16][1 << TREE_MAXIMUM_BITS];
static int tree_tables_ready;

static void trees_initialize(void)
{
	int tree;

	if (tree_tables_ready)
		return;
	for (tree = 0; tree < 16; tree++)
	{
		int maximum = bink_tree_lens[tree][15];
		int symbol;

		for (symbol = 0; symbol < 16; symbol++)
		{
			int length = bink_tree_lens[tree][symbol];
			unsigned long extension;

			/* (read least significant bit first: a code's bits are the low
			ones of every index that starts with it) */
			for (extension = 0; extension < (1UL << (maximum - length)); extension++)
			{
				struct tree_entry *entry = &tree_tables[tree][bink_tree_bits[tree][symbol] | (extension << length)];

				entry->symbol = (unsigned char)symbol;
				entry->length = (unsigned char)length;
			}
		}
	}
	tree_tables_ready = 1;
}

struct tree
{
	int number;
	unsigned char symbols[16];
};

static int tree_get(struct bit_reader *reader, const struct tree *tree)
{
	int maximum = bink_tree_lens[tree->number][15];
	const struct tree_entry *entry = &tree_tables[tree->number][bits_show(reader, maximum)];

	bits_skip(reader, entry->length);
	return tree->symbols[entry->symbol];
}

/* ---------- video (libavcodec/bink.c) */

enum
{
	SOURCE_BLOCK_TYPES = 0,
	SOURCE_SUB_BLOCK_TYPES,
	SOURCE_COLORS,
	SOURCE_PATTERN,
	SOURCE_X_OFFSET,
	SOURCE_Y_OFFSET,
	SOURCE_INTRA_DC,
	SOURCE_INTER_DC,
	SOURCE_RUN,
	NUMBER_OF_SOURCES
};

enum
{
	SKIP_BLOCK = 0,
	SCALED_BLOCK,
	MOTION_BLOCK,
	RUN_BLOCK,
	RESIDUE_BLOCK,
	INTRA_BLOCK,
	FILL_BLOCK,
	INTER_BLOCK,
	PATTERN_BLOCK,
	RAW_BLOCK
};

struct bundle
{
	int length;
	struct tree tree;
	unsigned char *data;
	unsigned char *data_end;
	unsigned char *decoded;  /* where the next values decoded go; NULL: no more this plane */
	unsigned char *read;     /* the next value to take */
};

struct plane
{
	unsigned char *pixels;
	long stride;
	long width, height;      /* in pixels */
	long block_width, block_height;
};

struct bink_video
{
	int revision;
	int has_alpha;
	int swap_planes;
	long width, height;
	/* [0] is being decoded, [1] is the last decoded, which is shown */
	struct plane planes[2][4];
	unsigned char *memory;
	struct bundle bundles[NUMBER_OF_SOURCES];
	unsigned char *bundle_memory;
	struct tree high_colors[16];
	int last_high_color;
};

static void plane_set(struct plane *plane, long luma_width, long luma_height, int chroma)
{
	plane->width = luma_width >> chroma;
	plane->height = luma_height >> chroma;
	plane->block_width = chroma ? (luma_width + 15) >> 4 : (luma_width + 7) >> 3;
	plane->block_height = chroma ? (luma_height + 15) >> 4 : (luma_height + 7) >> 3;
	/* (a block's row past the last for a 16x16 block in an odd last
	column, rounded up) */
	plane->stride = ((plane->block_width + 1) * 8 + 15) & ~15L;
}

static unsigned long plane_bytes(const struct plane *plane)
{
	return (unsigned long)plane->stride * (unsigned long)(plane->block_height + 1) * 8;
}

struct bink_video *bink_video_new(const struct bink_header *header)
{
	struct bink_video *video = calloc(1, sizeof(*video));
	unsigned long total = 0, blocks;
	int frame, index;

	if (!video)
		return NULL;
	trees_initialize();
	video->revision = (int)header->revision;
	video->has_alpha = (header->video_flags & BINK_FLAG_ALPHA) != 0;
	video->swap_planes = video->revision >= 'h';
	video->width = (long)header->width;
	video->height = (long)header->height;
	for (frame = 0; frame < 2; frame++)
	{
		plane_set(&video->planes[frame][0], video->width, video->height, 0);
		plane_set(&video->planes[frame][1], video->width, video->height, 1);
		plane_set(&video->planes[frame][2], video->width, video->height, 1);
		plane_set(&video->planes[frame][3], video->width, video->height, 0);
		for (index = 0; index < 4; index++)
			total += plane_bytes(&video->planes[frame][index]);
	}
	video->memory = malloc(total);
	blocks = (unsigned long)((video->width + 7) >> 3) * (unsigned long)((video->height + 7) >> 3);
	/* (one bundle's room more, as FFmpeg's) */
	video->bundle_memory = calloc(blocks * 64, NUMBER_OF_SOURCES + 1);
	if (!video->memory || !video->bundle_memory)
	{
		bink_video_delete(video);
		return NULL;
	}
	total = 0;
	for (frame = 0; frame < 2; frame++)
	{
		for (index = 0; index < 4; index++)
		{
			struct plane *plane = &video->planes[frame][index];

			plane->pixels = video->memory + total;
			/* black: the luma's 16, the chroma's middle, opaque */
			memset(plane->pixels, index == 0 ? 16 : index == 3 ? 255 : 128, plane_bytes(plane));
			total += plane_bytes(plane);
		}
	}
	for (index = 0; index < NUMBER_OF_SOURCES; index++)
	{
		video->bundles[index].data = video->bundle_memory + blocks * 64 * index;
		video->bundles[index].data_end = video->bundles[index].data + blocks * 64;
	}
	return video;
}

void bink_video_delete(struct bink_video *video)
{
	if (!video)
		return;
	free(video->memory);
	free(video->bundle_memory);
	free(video);
}

void bink_video_planes(const struct bink_video *video, const unsigned char **y, const unsigned char **u,
	const unsigned char **v, long *luma_stride, long *chroma_stride)
{
	*y = video->planes[1][0].pixels;
	*u = video->planes[1][1].pixels;
	*v = video->planes[1][2].pixels;
	*luma_stride = video->planes[1][0].stride;
	*chroma_stride = video->planes[1][1].stride;
}

static int log2_floor(unsigned long value)
{
	int result = 0;

	while (value >>= 1)
		result++;
	return result;
}

static void lengths_initialize(struct bink_video *video, long width, long block_width)
{
	width = (width + 7) & ~7L;
	video->bundles[SOURCE_BLOCK_TYPES].length = log2_floor((unsigned long)(width >> 3) + 511) + 1;
	video->bundles[SOURCE_SUB_BLOCK_TYPES].length = log2_floor((unsigned long)(width >> 4) + 511) + 1;
	video->bundles[SOURCE_COLORS].length = log2_floor((unsigned long)block_width * 64 + 511) + 1;
	video->bundles[SOURCE_INTRA_DC].length =
	video->bundles[SOURCE_INTER_DC].length =
	video->bundles[SOURCE_X_OFFSET].length =
	video->bundles[SOURCE_Y_OFFSET].length = log2_floor((unsigned long)(width >> 3) + 511) + 1;
	video->bundles[SOURCE_PATTERN].length = log2_floor((unsigned long)block_width * 8 + 511) + 1;
	video->bundles[SOURCE_RUN].length = log2_floor((unsigned long)block_width * 48 + 511) + 1;
}

static void merge(struct bit_reader *reader, unsigned char *destination, const unsigned char *source, int size)
{
	const unsigned char *source2 = source + size;
	int size2 = size;

	do
	{
		if (!bits_get1(reader))
		{
			*destination++ = *source++;
			size--;
		}
		else
		{
			*destination++ = *source2++;
			size2--;
		}
	} while (size && size2);
	while (size--)
		*destination++ = *source++;
	while (size2--)
		*destination++ = *source2++;
}

static int tree_read(struct bit_reader *reader, struct tree *tree)
{
	unsigned char present[16] = { 0 }, first[16], second[16], *in = first, *out = second, *swap;
	int index, step, length;

	if (bits_left(reader) < 4)
		return -1;
	tree->number = (int)bits_get(reader, 4);
	if (!tree->number)
	{
		for (index = 0; index < 16; index++)
			tree->symbols[index] = (unsigned char)index;
		return 0;
	}
	if (bits_get1(reader))
	{
		length = (int)bits_get(reader, 3);
		for (index = 0; index <= length; index++)
		{
			tree->symbols[index] = (unsigned char)bits_get(reader, 4);
			present[tree->symbols[index]] = 1;
		}
		for (index = 0; index < 16 && length < 16 - 1; index++)
			if (!present[index])
				tree->symbols[++length] = (unsigned char)index;
	}
	else
	{
		length = (int)bits_get(reader, 2);
		for (index = 0; index < 16; index++)
			in[index] = (unsigned char)index;
		for (index = 0; index <= length; index++)
		{
			int size = 1 << index;

			for (step = 0; step < 16; step += size << 1)
				merge(reader, out + step, in + step, size);
			swap = in;
			in = out;
			out = swap;
		}
		memcpy(tree->symbols, in, 16);
	}
	return 0;
}

static int bundle_read(struct bit_reader *reader, struct bink_video *video, int source)
{
	int index;

	if (source == SOURCE_COLORS)
	{
		for (index = 0; index < 16; index++)
			if (tree_read(reader, &video->high_colors[index]) < 0)
				return -1;
		video->last_high_color = 0;
	}
	if (source != SOURCE_INTRA_DC && source != SOURCE_INTER_DC)
		if (tree_read(reader, &video->bundles[source].tree) < 0)
			return -1;
	video->bundles[source].decoded = video->bundles[source].read = video->bundles[source].data;
	return 0;
}

/* the number of values to decode into a bundle now: 0 when it has enough
for now or none are left this plane (CHECK_READ_VAL) */
static int bundle_count(struct bit_reader *reader, struct bundle *bundle, unsigned long *count)
{
	if (!bundle->decoded || bundle->decoded > bundle->read)
		return 0;
	*count = bits_get(reader, bundle->length);
	if (!*count)
	{
		bundle->decoded = NULL;
		return 0;
	}
	return 1;
}

static int runs_read(struct bit_reader *reader, struct bundle *bundle)
{
	unsigned long count;
	const unsigned char *end;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (count > (unsigned long)(bundle->data_end - bundle->decoded) || bits_left(reader) < 1)
		return -1;
	end = bundle->decoded + count;
	if (bits_get1(reader))
	{
		memset(bundle->decoded, (int)bits_get(reader, 4), count);
		bundle->decoded += count;
	}
	else
	{
		while (bundle->decoded < end)
			*bundle->decoded++ = (unsigned char)tree_get(reader, &bundle->tree);
	}
	return 0;
}

static int motion_values_read(struct bit_reader *reader, struct bundle *bundle)
{
	unsigned long count;
	const unsigned char *end;
	int value, sign;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (count > (unsigned long)(bundle->data_end - bundle->decoded) || bits_left(reader) < 1)
		return -1;
	end = bundle->decoded + count;
	if (bits_get1(reader))
	{
		value = (int)bits_get(reader, 4);
		if (value)
		{
			sign = -bits_get1(reader);
			value = (value ^ sign) - sign;
		}
		memset(bundle->decoded, value, count);
		bundle->decoded += count;
	}
	else
	{
		while (bundle->decoded < end)
		{
			value = tree_get(reader, &bundle->tree);
			if (value)
			{
				sign = -bits_get1(reader);
				value = (value ^ sign) - sign;
			}
			*bundle->decoded++ = (unsigned char)value;
		}
	}
	return 0;
}

static const unsigned char run_lengths[4] = { 4, 8, 12, 32 };

static int block_types_read(struct bit_reader *reader, struct bink_video *video, struct bundle *bundle)
{
	unsigned long count;
	const unsigned char *end;
	int value, last = 0;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (video->revision == 'k')
	{
		count ^= 0xBB;
		if (!count)
		{
			bundle->decoded = NULL;
			return 0;
		}
	}
	if (count > (unsigned long)(bundle->data_end - bundle->decoded) || bits_left(reader) < 1)
		return -1;
	end = bundle->decoded + count;
	if (bits_get1(reader))
	{
		memset(bundle->decoded, (int)bits_get(reader, 4), count);
		bundle->decoded += count;
	}
	else
	{
		while (bundle->decoded < end)
		{
			value = tree_get(reader, &bundle->tree);
			if (value < 12)
			{
				last = value;
				*bundle->decoded++ = (unsigned char)value;
			}
			else
			{
				int run = run_lengths[value - 12];

				if (end - bundle->decoded < run)
					return -1;
				memset(bundle->decoded, last, (size_t)run);
				bundle->decoded += run;
			}
		}
	}
	return 0;
}

static int patterns_read(struct bit_reader *reader, struct bundle *bundle)
{
	unsigned long count;
	const unsigned char *end;
	int value;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (count > (unsigned long)(bundle->data_end - bundle->decoded))
		return -1;
	end = bundle->decoded + count;
	while (bundle->decoded < end)
	{
		if (bits_left(reader) < 2)
			return -1;
		value = tree_get(reader, &bundle->tree);
		value |= tree_get(reader, &bundle->tree) << 4;
		*bundle->decoded++ = (unsigned char)value;
	}
	return 0;
}

static int color_get(struct bit_reader *reader, struct bink_video *video, struct bundle *bundle)
{
	int value, sign;

	video->last_high_color = tree_get(reader, &video->high_colors[video->last_high_color]);
	value = (video->last_high_color << 4) | tree_get(reader, &bundle->tree);
	if (video->revision < 'i')
	{
		sign = ((signed char)value) >> 7;
		value = ((value & 0x7F) ^ sign) - sign;
		value += 0x80;
	}
	return value;
}

static int colors_read(struct bit_reader *reader, struct bink_video *video, struct bundle *bundle)
{
	unsigned long count;
	const unsigned char *end;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (count > (unsigned long)(bundle->data_end - bundle->decoded) || bits_left(reader) < 1)
		return -1;
	end = bundle->decoded + count;
	if (bits_get1(reader))
	{
		memset(bundle->decoded, color_get(reader, video, bundle), count);
		bundle->decoded += count;
	}
	else
	{
		while (bundle->decoded < end)
		{
			if (bits_left(reader) < 2)
				return -1;
			*bundle->decoded++ = (unsigned char)color_get(reader, video, bundle);
		}
	}
	return 0;
}

#define DC_START_BITS 11

static int dcs_read(struct bit_reader *reader, struct bundle *bundle, int start_bits, int has_sign)
{
	unsigned long count;
	short *destination = (short *)bundle->decoded;
	short *destination_end = (short *)bundle->data_end;
	long index, length, value, step, sign;
	int size;

	if (!bundle_count(reader, bundle, &count))
		return 0;
	if (bits_left(reader) < start_bits - has_sign)
		return -1;
	value = (long)bits_get(reader, start_bits - has_sign);
	if (value && has_sign)
	{
		sign = -bits_get1(reader);
		value = (value ^ sign) - sign;
	}
	if (destination_end - destination < 1)
		return -1;
	*destination++ = (short)value;
	length = (long)count - 1;
	for (index = 0; index < length; index += 8)
	{
		long run = length - index < 8 ? length - index : 8, j;

		if (destination_end - destination < run)
			return -1;
		size = (int)bits_get(reader, 4);
		if (size)
		{
			for (j = 0; j < run; j++)
			{
				step = (long)bits_get(reader, size);
				if (step)
				{
					sign = -bits_get1(reader);
					step = (step ^ sign) - sign;
				}
				value += step;
				*destination++ = (short)value;
				if (value < -32768 || value > 32767)
					return -1;
			}
		}
		else
		{
			for (j = 0; j < run; j++)
				*destination++ = (short)value;
		}
	}
	bundle->decoded = (unsigned char *)destination;
	return 0;
}

/* the next value of a bundle (get_value); 0 past its end */
static int value_get(struct bink_video *video, int source)
{
	struct bundle *bundle = &video->bundles[source];
	int value;

	if (source == SOURCE_INTRA_DC || source == SOURCE_INTER_DC)
	{
		if (bundle->data_end - bundle->read < 2)
			return 0;
		value = *(short *)bundle->read;
		bundle->read += 2;
		return value;
	}
	if (bundle->read >= bundle->data_end)
		return 0;
	if (source == SOURCE_X_OFFSET || source == SOURCE_Y_OFFSET)
		return (signed char)*bundle->read++;
	return *bundle->read++;
}

/* (read_dct_coeffs) the quantizer's index, or negative when damaged */
static int dct_coefficients_read(struct bit_reader *reader, int32_t block[64], const unsigned char *scan,
	int *coefficient_count, int coefficient_indices[64])
{
	int coefficients[128], modes[128];
	int index, value, bits, coefficient, mode, sign, count = 0;
	int list_start = 64, list_end = 64, list_position;

	if (bits_left(reader) < 4)
		return -1;
	coefficients[list_end] = 4; modes[list_end++] = 0;
	coefficients[list_end] = 24; modes[list_end++] = 0;
	coefficients[list_end] = 44; modes[list_end++] = 0;
	coefficients[list_end] = 1; modes[list_end++] = 3;
	coefficients[list_end] = 2; modes[list_end++] = 3;
	coefficients[list_end] = 3; modes[list_end++] = 3;

	for (bits = (int)bits_get(reader, 4) - 1; bits >= 0; bits--)
	{
		list_position = list_start;
		while (list_position < list_end)
		{
			if (!(modes[list_position] | coefficients[list_position]) || !bits_get1(reader))
			{
				list_position++;
				continue;
			}
			coefficient = coefficients[list_position];
			mode = modes[list_position];
			switch (mode)
			{
			case 0:
			case 2:
				if (mode == 0)
				{
					coefficients[list_position] = coefficient + 4;
					modes[list_position] = 1;
				}
				else
				{
					coefficients[list_position] = 0;
					modes[list_position++] = 0;
				}
				for (index = 0; index < 4; index++, coefficient++)
				{
					if (bits_get1(reader))
					{
						/* (no more than the list's 64 places before its start) */
						if (list_start <= 0)
							return -1;
						coefficients[--list_start] = coefficient;
						modes[list_start] = 3;
					}
					else
					{
						if (coefficient > 63 || count >= 64)
							return -1;
						if (!bits)
						{
							value = 1 - (bits_get1(reader) << 1);
						}
						else
						{
							value = (int)bits_get(reader, bits) | 1 << bits;
							sign = -bits_get1(reader);
							value = (value ^ sign) - sign;
						}
						block[scan[coefficient]] = value;
						coefficient_indices[count++] = coefficient;
					}
				}
				break;
			case 1:
				modes[list_position] = 2;
				for (index = 0; index < 3; index++)
				{
					if (list_end >= 128)
						return -1;
					coefficient += 4;
					coefficients[list_end] = coefficient;
					modes[list_end++] = 2;
				}
				break;
			case 3:
				if (coefficient > 63 || count >= 64)
					return -1;
				if (!bits)
				{
					value = 1 - (bits_get1(reader) << 1);
				}
				else
				{
					value = (int)bits_get(reader, bits) | 1 << bits;
					sign = -bits_get1(reader);
					value = (value ^ sign) - sign;
				}
				block[scan[coefficient]] = value;
				coefficient_indices[count++] = coefficient;
				coefficients[list_position] = 0;
				modes[list_position++] = 0;
				break;
			}
		}
	}
	*coefficient_count = count;
	return (int)bits_get(reader, 4);
}

static void dct_coefficients_unquantize(int32_t block[64], const int32_t quantizer[64], int count,
	const int coefficient_indices[64], const unsigned char *scan)
{
	int index;

	block[0] = (int32_t)((uint32_t)block[0] * (uint32_t)quantizer[0]) >> 11;
	for (index = 0; index < count; index++)
	{
		int at = coefficient_indices[index];

		block[scan[at]] = (int32_t)((uint32_t)block[scan[at]] * (uint32_t)quantizer[at]) >> 11;
	}
}

/* (read_residue) */
static int residue_read(struct bit_reader *reader, short block[64], int masks)
{
	int coefficients[128], modes[128], nonzero[64];
	int index, sign, mask, coefficient, mode, nonzero_count = 0;
	int list_start = 64, list_end = 64, list_position;

	coefficients[list_end] = 4; modes[list_end++] = 0;
	coefficients[list_end] = 24; modes[list_end++] = 0;
	coefficients[list_end] = 44; modes[list_end++] = 0;
	coefficients[list_end] = 0; modes[list_end++] = 2;

	for (mask = 1 << bits_get(reader, 3); mask; mask >>= 1)
	{
		for (index = 0; index < nonzero_count; index++)
		{
			if (!bits_get1(reader))
				continue;
			if (block[nonzero[index]] < 0)
				block[nonzero[index]] -= mask;
			else
				block[nonzero[index]] += mask;
			if (--masks < 0)
				return 0;
		}
		list_position = list_start;
		while (list_position < list_end)
		{
			if (!(coefficients[list_position] | modes[list_position]) || !bits_get1(reader))
			{
				list_position++;
				continue;
			}
			coefficient = coefficients[list_position];
			mode = modes[list_position];
			switch (mode)
			{
			case 0:
			case 2:
				if (mode == 0)
				{
					coefficients[list_position] = coefficient + 4;
					modes[list_position] = 1;
				}
				else
				{
					coefficients[list_position] = 0;
					modes[list_position++] = 0;
				}
				for (index = 0; index < 4; index++, coefficient++)
				{
					if (bits_get1(reader))
					{
						if (list_start <= 0)
							return -1;
						coefficients[--list_start] = coefficient;
						modes[list_start] = 3;
					}
					else
					{
						if (coefficient > 63 || nonzero_count >= 64)
							return -1;
						nonzero[nonzero_count++] = bink_scan[coefficient];
						sign = -bits_get1(reader);
						block[bink_scan[coefficient]] = (short)((mask ^ sign) - sign);
						if (--masks < 0)
							return 0;
					}
				}
				break;
			case 1:
				modes[list_position] = 2;
				for (index = 0; index < 3; index++)
				{
					if (list_end >= 128)
						return -1;
					coefficient += 4;
					coefficients[list_end] = coefficient;
					modes[list_end++] = 2;
				}
				break;
			case 3:
				if (coefficient > 63 || nonzero_count >= 64)
					return -1;
				nonzero[nonzero_count++] = bink_scan[coefficient];
				sign = -bits_get1(reader);
				block[bink_scan[coefficient]] = (short)((mask ^ sign) - sign);
				coefficients[list_position] = 0;
				modes[list_position++] = 0;
				if (--masks < 0)
					return 0;
				break;
			}
		}
	}
	return 0;
}

/* ---------- binkdsp.c: the inverse DCT, unclamped as Bink's own */

#define A1 2896 /* (1/sqrt(2))<<12 */
#define A2 2217
#define A3 3784
#define A4 -5352

#define MUL(X, Y) ((int32_t)((uint32_t)(X) * (uint32_t)(Y)) >> 11)

#define IDCT_TRANSFORM(dest, s0, s1, s2, s3, s4, s5, s6, s7, d0, d1, d2, d3, d4, d5, d6, d7, munge, src) { \
	const int32_t a0 = (src)[s0] + (src)[s4]; \
	const int32_t a1 = (src)[s0] - (src)[s4]; \
	const int32_t a2 = (src)[s2] + (src)[s6]; \
	const int32_t a3 = MUL(A1, (src)[s2] - (src)[s6]); \
	const int32_t a4 = (src)[s5] + (src)[s3]; \
	const int32_t a5 = (src)[s5] - (src)[s3]; \
	const int32_t a6 = (src)[s1] + (src)[s7]; \
	const int32_t a7 = (src)[s1] - (src)[s7]; \
	const int32_t b0 = a4 + a6; \
	const int32_t b1 = MUL(A3, a5 + a7); \
	const int32_t b2 = MUL(A4, a5) - b0 + b1; \
	const int32_t b3 = MUL(A1, a6 - a4) - b2; \
	const int32_t b4 = MUL(A2, a7) + b3 - b1; \
	(dest)[d0] = munge(a0 + a2 + b0); \
	(dest)[d1] = munge(a1 + a3 - a2 + b2); \
	(dest)[d2] = munge(a1 - a3 + a2 + b3); \
	(dest)[d3] = munge(a0 - a2 - b4); \
	(dest)[d4] = munge(a0 - a2 + b4); \
	(dest)[d5] = munge(a1 - a3 + a2 - b3); \
	(dest)[d6] = munge(a1 + a3 - a2 - b2); \
	(dest)[d7] = munge(a0 + a2 - b0); \
}

#define MUNGE_NONE(x) (x)
#define IDCT_COLUMN(dest, src) IDCT_TRANSFORM(dest, 0, 8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48, 56, MUNGE_NONE, src)
#define MUNGE_ROW(x) (((x) + 0x7F) >> 8)
#define IDCT_ROW(dest, src) IDCT_TRANSFORM(dest, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, MUNGE_ROW, src)

static void idct_column(int32_t *destination, const int32_t *source)
{
	if ((source[8] | source[16] | source[24] | source[32] | source[40] | source[48] | source[56]) == 0)
	{
		destination[0] = destination[8] = destination[16] = destination[24] =
		destination[32] = destination[40] = destination[48] = destination[56] = source[0];
	}
	else
	{
		IDCT_COLUMN(destination, source);
	}
}

static void idct(int32_t *block)
{
	int32_t temporary[64];
	int index;

	for (index = 0; index < 8; index++)
		idct_column(&temporary[index], &block[index]);
	for (index = 0; index < 8; index++)
	{
		IDCT_ROW((&block[8 * index]), (&temporary[8 * index]));
	}
}

static void idct_add(unsigned char *destination, long stride, int32_t *block)
{
	int i, j;

	idct(block);
	for (i = 0; i < 8; i++, destination += stride, block += 8)
		for (j = 0; j < 8; j++)
			destination[j] = (unsigned char)(destination[j] + block[j]);
}

static void idct_put(unsigned char *destination, long stride, const int32_t *block)
{
	int32_t temporary[64];
	int index;

	for (index = 0; index < 8; index++)
		idct_column(&temporary[index], &block[index]);
	for (index = 0; index < 8; index++)
	{
		unsigned char *row = &destination[index * stride];
		const int32_t *source = &temporary[8 * index];
		int32_t values[8];

		IDCT_ROW(values, source);
		for (int column = 0; column < 8; column++)
			row[column] = (unsigned char)values[column];
	}
}

static void block_scale(const unsigned char source[64], unsigned char *destination, long stride)
{
	int i, j;

	for (j = 0; j < 8; j++)
	{
		unsigned char *first = destination + stride * 2 * j;
		unsigned char *second = first + stride;

		for (i = 0; i < 8; i++)
			first[2 * i] = first[2 * i + 1] = second[2 * i] = second[2 * i + 1] = source[i];
		source += 8;
	}
}

static void pixels_add(unsigned char *pixels, const short *block, long stride)
{
	int i, j;

	for (i = 0; i < 8; i++, pixels += stride, block += 8)
		for (j = 0; j < 8; j++)
			pixels[j] = (unsigned char)(pixels[j] + block[j]);
}

static void pixels_put(unsigned char *destination, const unsigned char *source, long stride)
{
	int i;

	for (i = 0; i < 8; i++)
		memcpy(destination + i * stride, source + i * stride, 8);
}

static void block_fill(unsigned char *destination, int value, long stride, int size)
{
	int i;

	for (i = 0; i < size; i++)
		memset(destination + i * stride, value, (size_t)size);
}

/* (bink_put_pixels) a block of the last frame, moved */
static int motion_put(struct bink_video *video, unsigned char *destination, const unsigned char *previous,
	long stride, const unsigned char *start, const unsigned char *end)
{
	int x = value_get(video, SOURCE_X_OFFSET);
	int y = value_get(video, SOURCE_Y_OFFSET);
	const unsigned char *reference = previous + x + y * stride;

	if (reference < start || reference > end)
		return -1;
	pixels_put(destination, reference, stride);
	return 0;
}

static int run_fill(struct bit_reader *reader, struct bink_video *video, unsigned char *destination,
	const int map[64])
{
	const unsigned char *scan = bink_patterns[bits_get(reader, 4)];
	int index = 0, j, value;

	do
	{
		int run = value_get(video, SOURCE_RUN) + 1;

		index += run;
		if (index > 64)
			return -1;
		if (bits_get1(reader))
		{
			value = value_get(video, SOURCE_COLORS);
			for (j = 0; j < run; j++)
				destination[map[*scan++]] = (unsigned char)value;
		}
		else
		{
			for (j = 0; j < run; j++)
				destination[map[*scan++]] = (unsigned char)value_get(video, SOURCE_COLORS);
		}
	} while (index < 63);
	if (index == 63)
		destination[map[*scan++]] = (unsigned char)value_get(video, SOURCE_COLORS);
	return 0;
}

/* (bink_decode_plane) */
static int plane_decode(struct bink_video *video, struct bit_reader *reader, int plane_index, int chroma)
{
	struct plane *plane = &video->planes[0][plane_index];
	const struct plane *last = &video->planes[1][plane_index];
	const long stride = plane->stride;
	const long block_width = plane->block_width, block_height = plane->block_height;
	int block, index, j, bx, by, value, colors[2], quantizer, coefficient_count, coefficient_indices[64];
	int map[64], map8[64];
	unsigned char *destination;
	const unsigned char *previous, *start, *end;
	short residue[64];
	unsigned char scaled[64];
	int32_t coefficients[64];

	if (video->revision == 'k' && bits_get1(reader))
	{
		int fill = (int)bits_get(reader, 8);

		for (index = 0; index < plane->height; index++)
			memset(plane->pixels + index * stride, fill, (size_t)plane->width);
		goto end;
	}

	lengths_initialize(video, plane->width > 8 ? plane->width : 8, block_width);
	for (index = 0; index < NUMBER_OF_SOURCES; index++)
		if (bundle_read(reader, video, index) < 0)
			return -1;

	start = last->pixels;
	end = start + (block_width - 1 + stride * (block_height - 1)) * 8;
	for (index = 0; index < 64; index++)
	{
		map[index] = (index & 7) + (index >> 3) * (int)stride;
		map8[index] = index;
	}
	(void)chroma;

	for (by = 0; by < block_height; by++)
	{
		if (block_types_read(reader, video, &video->bundles[SOURCE_BLOCK_TYPES]) < 0 ||
			block_types_read(reader, video, &video->bundles[SOURCE_SUB_BLOCK_TYPES]) < 0 ||
			colors_read(reader, video, &video->bundles[SOURCE_COLORS]) < 0 ||
			patterns_read(reader, &video->bundles[SOURCE_PATTERN]) < 0 ||
			motion_values_read(reader, &video->bundles[SOURCE_X_OFFSET]) < 0 ||
			motion_values_read(reader, &video->bundles[SOURCE_Y_OFFSET]) < 0 ||
			dcs_read(reader, &video->bundles[SOURCE_INTRA_DC], DC_START_BITS, 0) < 0 ||
			dcs_read(reader, &video->bundles[SOURCE_INTER_DC], DC_START_BITS, 1) < 0 ||
			runs_read(reader, &video->bundles[SOURCE_RUN]) < 0)
			return -1;

		destination = plane->pixels + 8 * by * stride;
		previous = last->pixels + 8 * by * stride;
		for (bx = 0; bx < block_width; bx++, destination += 8, previous += 8)
		{
			block = value_get(video, SOURCE_BLOCK_TYPES);
			/* a 16x16 block's other three quarters: already drawn */
			if (((by & 1) || (bx & 1)) && block == SCALED_BLOCK)
			{
				bx++;
				destination += 8;
				previous += 8;
				continue;
			}
			switch (block)
			{
			case SKIP_BLOCK:
				pixels_put(destination, previous, stride);
				break;
			case SCALED_BLOCK:
				block = value_get(video, SOURCE_SUB_BLOCK_TYPES);
				switch (block)
				{
				case RUN_BLOCK:
					if (bits_left(reader) < 4 || run_fill(reader, video, scaled, map8) < 0)
						return -1;
					break;
				case INTRA_BLOCK:
					memset(coefficients, 0, sizeof(coefficients));
					coefficients[0] = value_get(video, SOURCE_INTRA_DC);
					quantizer = dct_coefficients_read(reader, coefficients, bink_scan, &coefficient_count,
						coefficient_indices);
					if (quantizer < 0)
						return -1;
					dct_coefficients_unquantize(coefficients, bink_intra_quant[quantizer], coefficient_count,
						coefficient_indices, bink_scan);
					idct_put(scaled, 8, coefficients);
					break;
				case FILL_BLOCK:
					block_fill(destination, value_get(video, SOURCE_COLORS), stride, 16);
					break;
				case PATTERN_BLOCK:
					for (index = 0; index < 2; index++)
						colors[index] = value_get(video, SOURCE_COLORS);
					for (j = 0; j < 8; j++)
					{
						value = value_get(video, SOURCE_PATTERN);
						for (index = 0; index < 8; index++, value >>= 1)
							scaled[index + j * 8] = (unsigned char)colors[value & 1];
					}
					break;
				case RAW_BLOCK:
					for (j = 0; j < 8; j++)
						for (index = 0; index < 8; index++)
							scaled[index + j * 8] = (unsigned char)value_get(video, SOURCE_COLORS);
					break;
				default:
					return -1;
				}
				if (block != FILL_BLOCK)
					block_scale(scaled, destination, stride);
				bx++;
				destination += 8;
				previous += 8;
				break;
			case MOTION_BLOCK:
				if (motion_put(video, destination, previous, stride, start, end) < 0)
					return -1;
				break;
			case RUN_BLOCK:
				if (run_fill(reader, video, destination, map) < 0)
					return -1;
				break;
			case RESIDUE_BLOCK:
				if (motion_put(video, destination, previous, stride, start, end) < 0)
					return -1;
				memset(residue, 0, sizeof(residue));
				value = (int)bits_get(reader, 7);
				if (residue_read(reader, residue, value) < 0)
					return -1;
				pixels_add(destination, residue, stride);
				break;
			case INTRA_BLOCK:
				memset(coefficients, 0, sizeof(coefficients));
				coefficients[0] = value_get(video, SOURCE_INTRA_DC);
				quantizer = dct_coefficients_read(reader, coefficients, bink_scan, &coefficient_count,
					coefficient_indices);
				if (quantizer < 0)
					return -1;
				dct_coefficients_unquantize(coefficients, bink_intra_quant[quantizer], coefficient_count,
					coefficient_indices, bink_scan);
				idct_put(destination, stride, coefficients);
				break;
			case FILL_BLOCK:
				block_fill(destination, value_get(video, SOURCE_COLORS), stride, 8);
				break;
			case INTER_BLOCK:
				if (motion_put(video, destination, previous, stride, start, end) < 0)
					return -1;
				memset(coefficients, 0, sizeof(coefficients));
				coefficients[0] = value_get(video, SOURCE_INTER_DC);
				quantizer = dct_coefficients_read(reader, coefficients, bink_scan, &coefficient_count,
					coefficient_indices);
				if (quantizer < 0)
					return -1;
				dct_coefficients_unquantize(coefficients, bink_inter_quant[quantizer], coefficient_count,
					coefficient_indices, bink_scan);
				idct_add(destination, stride, coefficients);
				break;
			case PATTERN_BLOCK:
				for (index = 0; index < 2; index++)
					colors[index] = value_get(video, SOURCE_COLORS);
				for (index = 0; index < 8; index++)
				{
					value = value_get(video, SOURCE_PATTERN);
					for (j = 0; j < 8; j++, value >>= 1)
						destination[index * stride + j] = (unsigned char)colors[value & 1];
				}
				break;
			case RAW_BLOCK:
				{
					struct bundle *colors_bundle = &video->bundles[SOURCE_COLORS];

					if (colors_bundle->data_end - colors_bundle->read < 64)
						return -1;
					for (index = 0; index < 8; index++)
						memcpy(destination + index * stride, colors_bundle->read + index * 8, 8);
					colors_bundle->read += 64;
				}
				break;
			default:
				return -1;
			}
		}
	}

end:
	/* (the next plane's data starts at a 32-bit boundary) */
	bits_align32(reader);
	return 0;
}

int bink_video_decode(struct bink_video *video, const unsigned char *data, unsigned long size)
{
	struct bit_reader reader;
	struct plane swap[4];
	int plane, index;

	bits_initialize(&reader, data, size);
	if (video->has_alpha)
	{
		if (video->revision >= 'i')
			bits_skip(&reader, 32);
		if (plane_decode(video, &reader, 3, 0) < 0)
			return -1;
	}
	if (video->revision >= 'i')
		bits_skip(&reader, 32);
	for (plane = 0; plane < 3; plane++)
	{
		index = (!plane || !video->swap_planes) ? plane : (plane ^ 3);
		if (plane_decode(video, &reader, index, plane != 0) < 0)
			return -1;
		if (bits_left(&reader) <= 0)
		{
			/* (the planes the packet stops before keep the last frame's) */
			for (plane++; plane < 3; plane++)
			{
				index = (!plane || !video->swap_planes) ? plane : (plane ^ 3);
				memcpy(video->planes[0][index].pixels, video->planes[1][index].pixels,
					plane_bytes(&video->planes[0][index]));
			}
			break;
		}
	}
	memcpy(swap, video->planes[0], sizeof(swap));
	memcpy(video->planes[0], video->planes[1], sizeof(swap));
	memcpy(video->planes[1], swap, sizeof(swap));
	return 0;
}

/* ---------- audio (libavcodec/binkaudio.c, its RDFT variant) */

/* the bands' upper edges (libavcodec/wma_freqs.c) */
static const unsigned short critical_frequencies[25] = {
	100, 200, 300, 400, 510, 630, 770, 920,
	1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150,
	3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500,
	24500,
};

#define AUDIO_MAXIMUM_FRAME_LENGTH 4096

struct bink_audio
{
	int channels;           /* the file's; the RDFT variant interleaves them in one */
	int frame_length;       /* the transform's size, in samples */
	int overlap_length;
	int block_size;         /* samples each block gives */
	int band_count;
	float root;
	unsigned long bands[26];
	float quantizers[96];
	float previous[AUDIO_MAXIMUM_FRAME_LENGTH / 16];
	int first;
	float coefficients[AUDIO_MAXIMUM_FRAME_LENGTH + 2];
	float output[AUDIO_MAXIMUM_FRAME_LENGTH];
	/* the inverse transform's complex FFT */
	float *real, *imaginary;
	float *cosines, *sines;
	unsigned short *reversed;
};

struct bink_audio *bink_audio_new(const struct bink_header *header)
{
	struct bink_audio *audio;
	long sample_rate, half_rate;
	int frame_length_bits, index, bits;

	if (!header->audio_track_count || !header->audio_sample_rate || (header->audio_flags & BINK_AUDIO_USE_DCT))
		return NULL;
	audio = calloc(1, sizeof(*audio));
	if (!audio)
		return NULL;
	audio->channels = (header->audio_flags & BINK_AUDIO_STEREO) ? 2 : 1;
	sample_rate = (long)header->audio_sample_rate;
	if (sample_rate < 22050)
		frame_length_bits = 9;
	else if (sample_rate < 44100)
		frame_length_bits = 10;
	else
		frame_length_bits = 11;
	/* (the channels interleaved in one transform) */
	sample_rate *= audio->channels;
	if (audio->channels == 2)
		frame_length_bits++;
	audio->frame_length = 1 << frame_length_bits;
	audio->overlap_length = audio->frame_length / 16;
	audio->block_size = audio->frame_length - audio->overlap_length;
	half_rate = (sample_rate + 1) / 2;
	audio->root = (float)(2.0 / (sqrt((double)audio->frame_length) * 32768.0));
	for (index = 0; index < 96; index++)
	{
		/* (0.066399999/log10(e)) */
		audio->quantizers[index] = (float)exp(index * 0.15289164787221953823) * audio->root;
	}
	for (audio->band_count = 1; audio->band_count < 25; audio->band_count++)
		if (half_rate <= critical_frequencies[audio->band_count - 1])
			break;
	audio->bands[0] = 2;
	for (index = 1; index < audio->band_count; index++)
		audio->bands[index] = ((unsigned long)critical_frequencies[index - 1] * (unsigned long)audio->frame_length /
			(unsigned long)half_rate) & ~1UL;
	audio->bands[audio->band_count] = (unsigned long)audio->frame_length;
	audio->first = 1;

	audio->real = malloc(sizeof(float) * (size_t)audio->frame_length);
	audio->imaginary = malloc(sizeof(float) * (size_t)audio->frame_length);
	audio->cosines = malloc(sizeof(float) * (size_t)audio->frame_length / 2);
	audio->sines = malloc(sizeof(float) * (size_t)audio->frame_length / 2);
	audio->reversed = malloc(sizeof(unsigned short) * (size_t)audio->frame_length);
	if (!audio->real || !audio->imaginary || !audio->cosines || !audio->sines || !audio->reversed)
	{
		bink_audio_delete(audio);
		return NULL;
	}
	for (index = 0; index < audio->frame_length / 2; index++)
	{
		double angle = 2.0 * 3.14159265358979323846 * index / audio->frame_length;

		audio->cosines[index] = (float)cos(angle);
		audio->sines[index] = (float)sin(angle);
	}
	for (index = 0; index < audio->frame_length; index++)
	{
		int reversed = 0;

		for (bits = 0; bits < frame_length_bits; bits++)
			if (index & (1 << bits))
				reversed |= 1 << (frame_length_bits - 1 - bits);
		audio->reversed[index] = (unsigned short)reversed;
	}
	return audio;
}

void bink_audio_delete(struct bink_audio *audio)
{
	if (!audio)
		return;
	free(audio->real);
	free(audio->imaginary);
	free(audio->cosines);
	free(audio->sines);
	free(audio->reversed);
	free(audio);
}

int bink_audio_channels(const struct bink_audio *audio)
{
	return audio->channels;
}

unsigned long bink_audio_maximum_samples(const struct bink_audio *audio, unsigned long size)
{
	/* (a block takes at least its two 29-bit values and bands, past the
	packet's 32-bit size) */
	unsigned long blocks = size * 8 / (58 + (unsigned long)audio->band_count * 8) + 1;

	return blocks * (unsigned long)audio->block_size;
}

/* x[n] = c[0]/2 + c[1]/2 (-1)^n + sum c[2k] cos(2 pi k n/N) + c[2k+1]
sin(2 pi k n/N): what FFmpeg's inverse RDFT (scale 0.5, the imaginary parts
negated) makes of a block's coefficients, through a complex FFT of N */
static void inverse_transform(struct bink_audio *audio, const float *coefficients, float *output)
{
	const int n = audio->frame_length;
	float *re = audio->real, *im = audio->imaginary;
	int size, half, step, start, k;

	for (k = 0; k < n; k++)
	{
		re[k] = 0.0f;
		im[k] = 0.0f;
	}
	re[0] = 0.5f * coefficients[0];
	re[audio->reversed[n / 2]] = 0.5f * coefficients[1];
	for (k = 1; k < n / 2; k++)
	{
		re[audio->reversed[k]] = coefficients[2 * k];
		im[audio->reversed[k]] = -coefficients[2 * k + 1];
	}
	/* (radix 2, in place, e^{+i}) */
	for (size = 2; size <= n; size <<= 1)
	{
		half = size >> 1;
		step = n / size;
		for (start = 0; start < n; start += size)
		{
			for (k = 0; k < half; k++)
			{
				float wr = audio->cosines[k * step], wi = audio->sines[k * step];
				int a = start + k, b = a + half;
				float tr = re[b] * wr - im[b] * wi;
				float ti = re[b] * wi + im[b] * wr;

				re[b] = re[a] - tr;
				im[b] = im[a] - ti;
				re[a] += tr;
				im[a] += ti;
			}
		}
	}
	for (k = 0; k < n; k++)
		output[k] = re[k];
}

static float float_get(struct bit_reader *reader)
{
	int power = (int)bits_get(reader, 5);
	float value = (float)ldexp((double)bits_get(reader, 23), power - 23);

	if (bits_get1(reader))
		value = -value;
	return value;
}

static const unsigned char rle_lengths[16] = {
	2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64
};

/* (decode_block) one block into audio->output; negative when the packet is
short */
static int audio_block_decode(struct bink_audio *audio, struct bit_reader *reader)
{
	float quantizer, quantizers[25];
	int index, j, k, width, coefficient;

	if (bits_left(reader) < 58)
		return -1;
	audio->coefficients[0] = float_get(reader) * audio->root;
	audio->coefficients[1] = float_get(reader) * audio->root;
	if (bits_left(reader) < audio->band_count * 8)
		return -1;
	for (index = 0; index < audio->band_count; index++)
	{
		int value = (int)bits_get(reader, 8);

		quantizers[index] = audio->quantizers[value < 95 ? value : 95];
	}
	k = 0;
	quantizer = quantizers[0];
	index = 2;
	while (index < audio->frame_length)
	{
		if (bits_get1(reader))
			j = index + rle_lengths[bits_get(reader, 4)] * 8;
		else
			j = index + 8;
		if (j > audio->frame_length)
			j = audio->frame_length;
		width = (int)bits_get(reader, 4);
		if (width == 0)
		{
			memset(audio->coefficients + index, 0, (size_t)(j - index) * sizeof(float));
			index = j;
			while (audio->bands[k] < (unsigned long)index)
				quantizer = quantizers[k++];
		}
		else
		{
			while (index < j)
			{
				if (audio->bands[k] == (unsigned long)index)
					quantizer = quantizers[k++];
				coefficient = (int)bits_get(reader, width);
				if (coefficient)
					audio->coefficients[index] = bits_get1(reader) ? -quantizer * coefficient : quantizer * coefficient;
				else
					audio->coefficients[index] = 0.0f;
				index++;
			}
		}
	}
	inverse_transform(audio, audio->coefficients, audio->output);
	if (!audio->first)
	{
		int count = audio->overlap_length;

		for (index = 0; index < audio->overlap_length; index++)
			audio->output[index] = (audio->previous[index] * (float)(count - index) +
				audio->output[index] * (float)index) / (float)count;
	}
	memcpy(audio->previous, &audio->output[audio->frame_length - audio->overlap_length],
		(size_t)audio->overlap_length * sizeof(float));
	audio->first = 0;
	return 0;
}

unsigned long bink_audio_decode(struct bink_audio *audio, const unsigned char *data, unsigned long size,
	short *samples, unsigned long capacity)
{
	struct bit_reader reader;
	unsigned long written = 0;
	int index;

	if (size < 4)
		return 0;
	bits_initialize(&reader, data, size);
	/* (the decoded size the packet reports) */
	bits_skip(&reader, 32);
	while (bits_left(&reader) > 0 && written + (unsigned long)audio->block_size <= capacity)
	{
		if (audio_block_decode(audio, &reader) < 0)
			break;
		for (index = 0; index < audio->block_size; index++)
		{
			float value = audio->output[index] * 32768.0f;

			samples[written++] = value >= 32767.0f ? 32767 : value <= -32768.0f ? -32768 :
				(short)(value < 0.0f ? value - 0.5f : value + 0.5f);
		}
		bits_align32(&reader);
	}
	return written;
}
