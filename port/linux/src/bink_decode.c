/*
BINK_DECODE.C

A Bink 1 decoder (bink_decode.h). Its layout, in the order of the file:
the bit reader, the Huffman trees, the container, the video (the planes
are cut into 8x8 blocks of ten kinds, and the values the blocks need are
decoded a row ahead into "bundles"), the audio (the spectrum of a block is
coded as bands, and turned into samples by an inverse real FFT and a
cross fade), and the conversion to pixels.

The bit stream is read least significant bit first from bytes. A video
frame is made of the planes one after another, each ending on a 32-bit
boundary.
*/

#include "bink_decode.h"

#include <math.h> /* (the port's math.h: halo_math.h) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	MAXIMUM_PACKET_SIZE = 64 << 20,
	MAXIMUM_AUDIO_TRACKS = 8,
	HEADER_SIZE = 44,

	VIDEO_FLAG_ALPHA = 0x00100000,

	AUDIO_FLAG_STEREO = 0x2000,
	AUDIO_FLAG_DCT = 0x1000,

	/* a lookup of the longest code (7 bits) */
	TREE_LOOKUP_BITS = 7,

	BUNDLE_BLOCK_TYPES = 0,
	BUNDLE_SUB_BLOCK_TYPES,
	BUNDLE_COLORS,
	BUNDLE_PATTERN,
	BUNDLE_X_OFFSET,
	BUNDLE_Y_OFFSET,
	BUNDLE_INTRA_DC,
	BUNDLE_INTER_DC,
	BUNDLE_RUN,
	NUMBER_OF_BUNDLES,

	BLOCK_SKIP = 0,
	BLOCK_SCALED,
	BLOCK_MOTION,
	BLOCK_RUN,
	BLOCK_RESIDUE,
	BLOCK_INTRA,
	BLOCK_FILL,
	BLOCK_INTER,
	BLOCK_PATTERN,
	BLOCK_RAW,

	DC_START_BITS = 11,

	AUDIO_QUANTIZER_COUNT = 96,
	AUDIO_BANDS = 26
};

/* ---------- structures */

struct bits
{
	const unsigned char *data; /* with 8 zero bytes after size */
	unsigned long size_bits;
	unsigned long position;
	int overrun;
};

struct tree
{
	unsigned char lookup_number;
	unsigned char symbols[16];
};

struct bundle
{
	unsigned char *data;
	unsigned long capacity; /* bytes */
	unsigned long decoded; /* bytes filled */
	unsigned long read; /* bytes taken */
	int length_bits;
	int empty;
	struct tree tree;
};

struct plane
{
	unsigned char *current;
	unsigned char *previous;
	unsigned long width; /* the picture's, in pixels */
	unsigned long height;
	unsigned long stride; /* padded to blocks */
	unsigned long padded_height;
};

struct audio_track
{
	unsigned long sample_rate;
	int flags;
	int channels;
};

struct bink_decoder
{
	const struct bink_tables *tables;
	bink_read_proc read;
	void *context;
	unsigned long file_size;
	struct bink_info info;
	unsigned long video_flags;
	int swap_planes;
	int audio_tracks;
	struct audio_track track[MAXIMUM_AUDIO_TRACKS];

	unsigned long *index; /* frames + 1 offsets, key frame flag cleared */
	unsigned long next;
	unsigned char *packet;
	unsigned long packet_capacity;

	/* the Huffman codes: [tree][7-bit peek] is (length << 4) | symbol index */
	unsigned char lookup[16][1 << TREE_LOOKUP_BITS];

	struct plane planes[4];
	struct bundle bundles[NUMBER_OF_BUNDLES];
	struct tree color_high[16];
	int color_last;
	int failed;

	/* audio of the first track */
	int audio_enabled;
	int audio_frame_bits;
	unsigned long audio_frame_length;
	unsigned long audio_overlap;
	unsigned long audio_block_size;
	unsigned long audio_band_count;
	unsigned long audio_bands[AUDIO_BANDS];
	float audio_quantizer[AUDIO_QUANTIZER_COUNT];
	float audio_root;
	float *audio_spectrum; /* re, im of the full frame */
	float *audio_coefficients;
	float *audio_samples;
	float *audio_previous;
	float *audio_cos;
	float *audio_sin;
	unsigned short *audio_reverse;
	int audio_first;
	short *audio_output;
	unsigned long audio_output_capacity; /* in frames */
	unsigned long audio_output_head;
	unsigned long audio_output_count;
	int audio_output_channels;
};

/* ---------- bit reader */

static void bits_init(struct bits *bits, const unsigned char *data, unsigned long size_bytes)
{
	bits->data = data;
	bits->size_bits = size_bytes * 8;
	bits->position = 0;
	bits->overrun = 0;
}

/* count is at most 24 */
static unsigned long bits_peek(const struct bits *bits, int count)
{
	unsigned long byte = bits->position >> 3;
	unsigned long word;

	if (bits->position > bits->size_bits)
	{
		return 0;
	}
	word = (unsigned long)bits->data[byte] | (unsigned long)bits->data[byte + 1] << 8 |
		(unsigned long)bits->data[byte + 2] << 16 | (unsigned long)bits->data[byte + 3] << 24;
	return (word >> (bits->position & 7)) & ((1UL << count) - 1);
}

static unsigned long bits_get(struct bits *bits, int count)
{
	unsigned long value;

	if (count == 0)
	{
		return 0;
	}
	value = bits_peek(bits, count);
	bits->position += (unsigned long)count;
	if (bits->position > bits->size_bits)
	{
		bits->overrun = 1;
	}
	return value;
}

static int bits_get_one(struct bits *bits)
{
	return (int)bits_get(bits, 1);
}

static void bits_skip(struct bits *bits, unsigned long count)
{
	bits->position += count;
	if (bits->position > bits->size_bits)
	{
		bits->overrun = 1;
		bits->position = bits->size_bits + 1;
	}
}

static void bits_align32(struct bits *bits)
{
	if (bits->position & 31)
	{
		bits_skip(bits, 32 - (bits->position & 31));
	}
}

/* ---------- trees */

static void trees_build(struct bink_decoder *decoder)
{
	int tree, symbol;
	unsigned long peek;

	for (tree = 0; tree < 16; tree++)
	{
		for (symbol = 0; symbol < 16; symbol++)
		{
			unsigned long length = decoder->tables->tree_lengths[tree][symbol];
			unsigned long code = decoder->tables->tree_bits[tree][symbol];

			for (peek = code; peek < (1UL << TREE_LOOKUP_BITS); peek += 1UL << length)
			{
				decoder->lookup[tree][peek] = (unsigned char)(length << 4 | (unsigned long)symbol);
			}
		}
	}
}

static int tree_get(const struct bink_decoder *decoder, struct bits *bits, const struct tree *tree)
{
	unsigned char entry = decoder->lookup[tree->lookup_number][bits_peek(bits, TREE_LOOKUP_BITS)];

	bits_skip(bits, (unsigned long)(entry >> 4));
	return tree->symbols[entry & 15];
}

/* merges the runs of size symbols in source (in pairs) into destination,
each step chosen by a bit */
static void tree_merge(struct bits *bits, unsigned char *destination, const unsigned char *source, int size)
{
	const unsigned char *second = source + size;
	int first_left = size;
	int second_left = size;

	do
	{
		if (!bits_get_one(bits))
		{
			*destination++ = *source++;
			first_left--;
		}
		else
		{
			*destination++ = *second++;
			second_left--;
		}
	}
	while (first_left && second_left);
	while (first_left--)
	{
		*destination++ = *source++;
	}
	while (second_left--)
	{
		*destination++ = *second++;
	}
}

/* a tree's number, and how its 16 symbols are ordered */
static void tree_read(struct bits *bits, struct tree *tree)
{
	unsigned char first[16], second[16];
	unsigned char *input = first, *output = second, *swap;
	int index, count, round, start;

	memset(first, 0, sizeof(first));
	tree->lookup_number = (unsigned char)bits_get(bits, 4);
	if (tree->lookup_number == 0)
	{
		for (index = 0; index < 16; index++)
		{
			tree->symbols[index] = (unsigned char)index;
		}
		return;
	}
	if (bits_get_one(bits))
	{
		/* the first symbols are named, the rest follow in order */
		count = (int)bits_get(bits, 3);
		for (index = 0; index <= count; index++)
		{
			tree->symbols[index] = (unsigned char)bits_get(bits, 4);
			first[tree->symbols[index]] = 1;
		}
		for (index = 0; index < 16 && count < 15; index++)
		{
			if (!first[index])
			{
				tree->symbols[++count] = (unsigned char)index;
			}
		}
	}
	else
	{
		/* a merge sort, with each comparison given by a bit */
		count = (int)bits_get(bits, 2);
		for (index = 0; index < 16; index++)
		{
			input[index] = (unsigned char)index;
		}
		for (round = 0; round <= count; round++)
		{
			int size = 1 << round;

			for (start = 0; start < 16; start += size << 1)
			{
				tree_merge(bits, output + start, input + start, size);
			}
			swap = input;
			input = output;
			output = swap;
		}
		memcpy(tree->symbols, input, 16);
	}
}

/* ---------- utilities */

static void set_error(char *error, int error_size, const char *message)
{
	if (error && error_size > 0)
	{
		snprintf(error, (size_t)error_size, "%s", message);
	}
}

static unsigned long read_u32(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static int log2_floor(unsigned long value)
{
	int result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}
	return result;
}

static unsigned char clip_byte(int value)
{
	return value < 0 ? 0 : value > 255 ? 255 : (unsigned char)value;
}

/* ---------- bundles */

/* how many values the bundle's next batch holds, when one is due: a bundle
is refilled when everything it holds has been taken. Returns 0 to read none,
-1 for a count that does not fit, else the count (at least 1) */
static long bundle_count(struct bits *bits, struct bundle *bundle, unsigned long unit)
{
	unsigned long count;

	if (bundle->empty || bundle->decoded > bundle->read)
	{
		return 0;
	}
	count = bits_get(bits, bundle->length_bits);
	if (count == 0)
	{
		bundle->empty = 1;
		return 0;
	}
	if (bits->overrun || count * unit > bundle->capacity - bundle->decoded)
	{
		return -1;
	}
	return (long)count;
}

static int bundle_read_block_types(struct bits *bits, struct bundle *bundle, const struct tree *tree,
	const struct bink_decoder *decoder)
{
	static const unsigned char run_lengths[4] = { 4, 8, 12, 32 };
	long count = bundle_count(bits, bundle, 1);
	unsigned long end;
	int last = 0;

	if (count <= 0)
	{
		return count == 0;
	}
	end = bundle->decoded + (unsigned long)count;
	if (bits_get_one(bits))
	{
		/* one type for all of them */
		memset(bundle->data + bundle->decoded, (int)bits_get(bits, 4), (size_t)count);
		bundle->decoded = end;
		return 1;
	}
	while (bundle->decoded < end)
	{
		int value = tree_get(decoder, bits, tree);

		if (value > 11)
		{
			unsigned long run = run_lengths[value - 12];

			if (run > end - bundle->decoded)
			{
				return 0;
			}
			memset(bundle->data + bundle->decoded, last, (size_t)run);
			bundle->decoded += run;
		}
		else
		{
			last = value;
			bundle->data[bundle->decoded++] = (unsigned char)value;
		}
		if (bits->overrun)
		{
			return 0;
		}
	}
	return 1;
}

static int bundle_read_colors(struct bink_decoder *decoder, struct bits *bits, struct bundle *bundle)
{
	long count = bundle_count(bits, bundle, 1);
	unsigned long end;

	if (count <= 0)
	{
		return count == 0;
	}
	end = bundle->decoded + (unsigned long)count;
	if (bits_get_one(bits))
	{
		int value;

		decoder->color_last = tree_get(decoder, bits, &decoder->color_high[decoder->color_last]);
		value = decoder->color_last << 4 | tree_get(decoder, bits, &bundle->tree);
		if (decoder->info.revision < 'i')
		{
			int sign = (int)(signed char)value >> 7;

			value = ((value & 0x7F) ^ sign) - sign + 0x80;
		}
		memset(bundle->data + bundle->decoded, value, (size_t)count);
		bundle->decoded = end;
		return 1;
	}
	while (bundle->decoded < end)
	{
		int value;

		decoder->color_last = tree_get(decoder, bits, &decoder->color_high[decoder->color_last]);
		value = decoder->color_last << 4 | tree_get(decoder, bits, &bundle->tree);
		if (decoder->info.revision < 'i')
		{
			int sign = (int)(signed char)value >> 7;

			value = ((value & 0x7F) ^ sign) - sign + 0x80;
		}
		bundle->data[bundle->decoded++] = (unsigned char)value;
	}
	return !bits->overrun;
}

static int bundle_read_patterns(const struct bink_decoder *decoder, struct bits *bits, struct bundle *bundle)
{
	long count = bundle_count(bits, bundle, 1);
	unsigned long end;

	if (count <= 0)
	{
		return count == 0;
	}
	end = bundle->decoded + (unsigned long)count;
	while (bundle->decoded < end)
	{
		int low = tree_get(decoder, bits, &bundle->tree);
		int high = tree_get(decoder, bits, &bundle->tree);

		bundle->data[bundle->decoded++] = (unsigned char)(low | high << 4);
	}
	return !bits->overrun;
}

static int bundle_read_motion(const struct bink_decoder *decoder, struct bits *bits, struct bundle *bundle)
{
	long count = bundle_count(bits, bundle, 1);
	unsigned long end;

	if (count <= 0)
	{
		return count == 0;
	}
	end = bundle->decoded + (unsigned long)count;
	if (bits_get_one(bits))
	{
		int value = (int)bits_get(bits, 4);

		if (value)
		{
			int sign = -bits_get_one(bits);

			value = (value ^ sign) - sign;
		}
		memset(bundle->data + bundle->decoded, value, (size_t)count);
		bundle->decoded = end;
		return 1;
	}
	while (bundle->decoded < end)
	{
		int value = tree_get(decoder, bits, &bundle->tree);

		if (value)
		{
			int sign = -bits_get_one(bits);

			value = (value ^ sign) - sign;
		}
		bundle->data[bundle->decoded++] = (unsigned char)value;
	}
	return !bits->overrun;
}

/* the DC values of the blocks, 16-bit: a first value and then differences,
in groups of 8 whose bit width is given */
static int bundle_read_dc(struct bits *bits, struct bundle *bundle, int has_sign)
{
	long count = bundle_count(bits, bundle, 2);
	short *values = (short *)(void *)bundle->data;
	unsigned long position = bundle->decoded / 2;
	long index, group, size;
	int value;

	if (count <= 0)
	{
		return count == 0;
	}
	value = (int)bits_get(bits, DC_START_BITS - has_sign);
	if (value && has_sign)
	{
		int sign = -bits_get_one(bits);

		value = (value ^ sign) - sign;
	}
	values[position++] = (short)value;
	count--;
	for (index = 0; index < count; index += 8)
	{
		int width;

		size = count - index < 8 ? count - index : 8;
		width = (int)bits_get(bits, 4);
		for (group = 0; group < size; group++)
		{
			if (width)
			{
				int delta = (int)bits_get(bits, width);

				if (delta)
				{
					int sign = -bits_get_one(bits);

					delta = (delta ^ sign) - sign;
				}
				value += delta;
				if (value < -32768 || value > 32767)
				{
					return 0;
				}
			}
			values[position++] = (short)value;
		}
	}
	bundle->decoded = position * 2;
	return !bits->overrun;
}

static int bundle_read_runs(const struct bink_decoder *decoder, struct bits *bits, struct bundle *bundle)
{
	long count = bundle_count(bits, bundle, 1);
	unsigned long end;

	if (count <= 0)
	{
		return count == 0;
	}
	end = bundle->decoded + (unsigned long)count;
	if (bits_get_one(bits))
	{
		memset(bundle->data + bundle->decoded, (int)bits_get(bits, 4), (size_t)count);
		bundle->decoded = end;
		return 1;
	}
	while (bundle->decoded < end)
	{
		bundle->data[bundle->decoded++] = (unsigned char)tree_get(decoder, bits, &bundle->tree);
	}
	return !bits->overrun;
}

/* the next value of a bundle; one that is not there fails the frame */
static int bundle_take_byte(struct bink_decoder *decoder, int number)
{
	struct bundle *bundle = &decoder->bundles[number];

	if (bundle->read >= bundle->decoded)
	{
		decoder->failed = 1;
		return 0;
	}
	return bundle->data[bundle->read++];
}

static int bundle_take_dc(struct bink_decoder *decoder, int number)
{
	struct bundle *bundle = &decoder->bundles[number];

	if (bundle->read + 2 > bundle->decoded)
	{
		decoder->failed = 1;
		return 0;
	}
	bundle->read += 2;
	return ((short *)(void *)bundle->data)[(bundle->read - 2) / 2];
}

/* ---------- transforms */

/* the DCT's constants: 4096 times 1/sqrt(2), the sine and cosine of
pi/8 scaled by sqrt(2) and the same with a sign, as the format's integer
transform has them */
#define TRANSFORM_A1 2896
#define TRANSFORM_A2 2217
#define TRANSFORM_A3 3784
#define TRANSFORM_A4 (-5352)
/* (a damaged stream can give values that overflow; the arithmetic is
unsigned, so that it wraps, as the library's 32-bit arithmetic does, the same on every port) */
#define TRANSFORM_MULTIPLY(constant, value) ((unsigned)((int)((unsigned)(constant) * (value)) >> 11))

/* one dimension of the inverse transform, from the 8 values of source at
step apart to those of destination; round is added to each result before
it is shifted down by shift */
static void transform_line(const int *source, int *destination, int step, int round, int shift)
{
	unsigned s0 = (unsigned)source[0], s1 = (unsigned)source[step], s2 = (unsigned)source[2 * step];
	unsigned s3 = (unsigned)source[3 * step], s4 = (unsigned)source[4 * step], s5 = (unsigned)source[5 * step];
	unsigned s6 = (unsigned)source[6 * step], s7 = (unsigned)source[7 * step];
	unsigned bias = (unsigned)round;
	unsigned a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4;

	/* only the first value: the transform gives it back in all 8 (most of
	the lines of a block are so) */
	if (!(s1 | s2 | s3 | s4 | s5 | s6 | s7))
	{
		int value = (int)(s0 + bias) >> shift;

		destination[0] = destination[step] = destination[2 * step] = destination[3 * step] = value;
		destination[4 * step] = destination[5 * step] = destination[6 * step] = destination[7 * step] = value;
		return;
	}
	a0 = s0 + s4;
	a1 = s0 - s4;
	a2 = s2 + s6;
	a3 = TRANSFORM_MULTIPLY(TRANSFORM_A1, s2 - s6);
	a4 = s5 + s3;
	a5 = s5 - s3;
	a6 = s1 + s7;
	a7 = s1 - s7;
	b0 = a4 + a6;
	b1 = TRANSFORM_MULTIPLY(TRANSFORM_A3, a5 + a7);
	b2 = TRANSFORM_MULTIPLY(TRANSFORM_A4, a5) - b0 + b1;
	b3 = TRANSFORM_MULTIPLY(TRANSFORM_A1, a6 - a4) - b2;
	b4 = TRANSFORM_MULTIPLY(TRANSFORM_A2, a7) + b3 - b1;

	destination[0] = (int)(a0 + a2 + b0 + bias) >> shift;
	destination[step] = (int)(a1 + a3 - a2 + b2 + bias) >> shift;
	destination[2 * step] = (int)(a1 - a3 - a2 + b3 + bias) >> shift;
	destination[3 * step] = (int)(a0 - a2 - b4 + bias) >> shift;
	destination[4 * step] = (int)(a0 - a2 + b4 + bias) >> shift;
	destination[5 * step] = (int)(a1 - a3 + a2 - b3 + bias) >> shift;
	destination[6 * step] = (int)(a1 + a3 + a2 - b2 + bias) >> shift;
	destination[7 * step] = (int)(a0 + a2 - b0 + bias) >> shift;
}

/* the whole 8x8 inverse transform of block, to pixel offsets in output */
static void transform_block(const int *block, int *output)
{
	int middle[64];
	int index;

	for (index = 0; index < 8; index++)
	{
		transform_line(block + index, middle + index, 8, 0, 0);
	}
	for (index = 0; index < 8; index++)
	{
		transform_line(middle + index * 8, output + index * 8, 1, 0x7F, 8);
	}
}

static void transform_put(unsigned char *destination, unsigned long stride, const int *block)
{
	int output[64];
	int row, column;

	transform_block(block, output);
	for (row = 0; row < 8; row++, destination += stride)
	{
		for (column = 0; column < 8; column++)
		{
			destination[column] = clip_byte(output[row * 8 + column]);
		}
	}
}

static void transform_add(unsigned char *destination, unsigned long stride, const int *block)
{
	int output[64];
	int row, column;

	transform_block(block, output);
	for (row = 0; row < 8; row++, destination += stride)
	{
		for (column = 0; column < 8; column++)
		{
			destination[column] = clip_byte((int)((unsigned)destination[column] + (unsigned)output[row * 8 + column]));
		}
	}
}

/* ---------- block coefficients */

/* a coefficient times a 16.16 factor, in 32-bit arithmetic that wraps (as
the library's does) so that every port gets the same result for any stream */
static int dequantize(int value, int factor)
{
	return (int)((unsigned)value * (unsigned)factor) >> 11;
}

/* The coefficients of a DCT block: in bit planes from the highest, each
plane going over a list of coefficients (or groups of 4, 16 or 64) that
are still undecided. A list entry's mode: 0, a group of four whose
significance is not known; 1, a group of 16 (split into 4 groups of 4 when
significant); 2, one of those groups of 4 (unknown); 3, a single coefficient
known to be significant. */
static int read_coefficients(struct bink_decoder *decoder, struct bits *bits, int *block,
	const int (*quantizers)[64])
{
	int coefficient_list[128], mode_list[128];
	int coefficient_index[64];
	int list_start = 64, list_end = 64, list_position;
	int coefficient_count = 0;
	int plane_bits, mask, quality, index;
	const unsigned char *scan = decoder->tables->scan;
	const int *quantizer;

	coefficient_list[list_end] = 4;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 24;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 44;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 1;
	mode_list[list_end++] = 3;
	coefficient_list[list_end] = 2;
	mode_list[list_end++] = 3;
	coefficient_list[list_end] = 3;
	mode_list[list_end++] = 3;

	plane_bits = (int)bits_get(bits, 4) - 1;
	for (mask = plane_bits >= 0 ? 1 << plane_bits : 0; plane_bits >= 0; mask >>= 1, plane_bits--)
	{
		list_position = list_start;
		while (list_position < list_end)
		{
			int coefficient, mode, group;

			if (!(mode_list[list_position] | coefficient_list[list_position]) || !bits_get_one(bits))
			{
				list_position++;
				continue;
			}
			coefficient = coefficient_list[list_position];
			mode = mode_list[list_position];
			switch (mode)
			{
			case 0:
			case 2:
				if (mode == 0)
				{
					coefficient_list[list_position] = coefficient + 4;
					mode_list[list_position] = 1;
				}
				else
				{
					coefficient_list[list_position] = 0;
					mode_list[list_position++] = 0;
				}
				for (group = 0; group < 4; group++, coefficient++)
				{
					if (bits_get_one(bits))
					{
						if (list_start == 0)
						{
							return 0;
						}
						coefficient_list[--list_start] = coefficient;
						mode_list[list_start] = 3;
					}
					else
					{
						int value;

						if (!plane_bits)
						{
							value = 1 - (bits_get_one(bits) << 1);
						}
						else
						{
							int sign;

							value = (int)bits_get(bits, plane_bits) | mask;
							sign = -bits_get_one(bits);
							value = (value ^ sign) - sign;
						}
						if (coefficient > 63 || coefficient_count >= 64)
						{
							return 0;
						}
						block[scan[coefficient]] = value;
						coefficient_index[coefficient_count++] = coefficient;
					}
				}
				break;
			case 1:
				mode_list[list_position] = 2;
				for (group = 0; group < 3; group++)
				{
					coefficient += 4;
					if (list_end >= 128)
					{
						return 0;
					}
					coefficient_list[list_end] = coefficient;
					mode_list[list_end++] = 2;
				}
				break;
			default:
				{
					int value;

					if (!plane_bits)
					{
						value = 1 - (bits_get_one(bits) << 1);
					}
					else
					{
						int sign;

						value = (int)bits_get(bits, plane_bits) | mask;
						sign = -bits_get_one(bits);
						value = (value ^ sign) - sign;
					}
					if (coefficient > 63 || coefficient_count >= 64)
					{
						return 0;
					}
					block[scan[coefficient]] = value;
					coefficient_index[coefficient_count++] = coefficient;
					coefficient_list[list_position] = 0;
					mode_list[list_position++] = 0;
				}
				break;
			}
			if (bits->overrun)
			{
				return 0;
			}
		}
	}

	quality = (int)bits_get(bits, 4);
	quantizer = quantizers[quality];
	block[0] = dequantize(block[0], quantizer[0]);
	for (index = 0; index < coefficient_count; index++)
	{
		int position = scan[coefficient_index[index]];

		block[position] = dequantize(block[position], quantizer[position]);
	}
	return !bits->overrun;
}

/* The corrections of a residue block: like the coefficients, bit planes
from a count of 8 (3 bits), and values of one bit plane each; at most
budget values are given in all. */
static int read_residue(struct bink_decoder *decoder, struct bits *bits, int *block, int budget)
{
	int coefficient_list[128], mode_list[128];
	int nonzero[64];
	int list_start = 64, list_end = 64, list_position;
	int nonzero_count = 0;
	int mask, index;
	const unsigned char *scan = decoder->tables->scan;

	coefficient_list[list_end] = 4;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 24;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 44;
	mode_list[list_end++] = 0;
	coefficient_list[list_end] = 0;
	mode_list[list_end++] = 2;

	for (mask = 1 << bits_get(bits, 3); mask; mask >>= 1)
	{
		for (index = 0; index < nonzero_count; index++)
		{
			if (!bits_get_one(bits))
			{
				continue;
			}
			if (block[nonzero[index]] < 0)
			{
				block[nonzero[index]] -= mask;
			}
			else
			{
				block[nonzero[index]] += mask;
			}
			if (--budget < 0)
			{
				return 1;
			}
		}
		list_position = list_start;
		while (list_position < list_end)
		{
			int coefficient, mode, group;

			if (!(coefficient_list[list_position] | mode_list[list_position]) || !bits_get_one(bits))
			{
				list_position++;
				continue;
			}
			coefficient = coefficient_list[list_position];
			mode = mode_list[list_position];
			switch (mode)
			{
			case 0:
			case 2:
				if (mode == 0)
				{
					coefficient_list[list_position] = coefficient + 4;
					mode_list[list_position] = 1;
				}
				else
				{
					coefficient_list[list_position] = 0;
					mode_list[list_position++] = 0;
				}
				for (group = 0; group < 4; group++, coefficient++)
				{
					if (bits_get_one(bits))
					{
						if (list_start == 0)
						{
							return 0;
						}
						coefficient_list[--list_start] = coefficient;
						mode_list[list_start] = 3;
					}
					else
					{
						int sign;

						if (coefficient > 63 || nonzero_count >= 64)
						{
							return 0;
						}
						nonzero[nonzero_count++] = scan[coefficient];
						sign = -bits_get_one(bits);
						block[scan[coefficient]] = (mask ^ sign) - sign;
						if (--budget < 0)
						{
							return 1;
						}
					}
				}
				break;
			case 1:
				mode_list[list_position] = 2;
				for (group = 0; group < 3; group++)
				{
					coefficient += 4;
					if (list_end >= 128)
					{
						return 0;
					}
					coefficient_list[list_end] = coefficient;
					mode_list[list_end++] = 2;
				}
				break;
			default:
				{
					int sign;

					if (coefficient > 63 || nonzero_count >= 64)
					{
						return 0;
					}
					nonzero[nonzero_count++] = scan[coefficient];
					sign = -bits_get_one(bits);
					block[scan[coefficient]] = (mask ^ sign) - sign;
					coefficient_list[list_position] = 0;
					mode_list[list_position++] = 0;
					if (--budget < 0)
					{
						return 1;
					}
				}
				break;
			}
			if (bits->overrun)
			{
				return 0;
			}
		}
	}
	return !bits->overrun;
}

/* ---------- video blocks */

static void copy_block(unsigned char *destination, unsigned long destination_stride,
	const unsigned char *source, unsigned long source_stride)
{
	int row;

	for (row = 0; row < 8; row++)
	{
		memcpy(destination + row * destination_stride, source + row * source_stride, 8);
	}
}

static void fill_block(unsigned char *destination, unsigned long stride, int value, int size)
{
	int row;

	for (row = 0; row < size; row++)
	{
		memset(destination + row * stride, value, (size_t)size);
	}
}

/* copies the block at the offsets the motion bundles give from the
previous picture. The source must lie in the picture */
static int copy_motion_block(struct bink_decoder *decoder, const struct plane *plane, unsigned long bx,
	unsigned long by, unsigned char *destination)
{
	int x_offset = (signed char)bundle_take_byte(decoder, BUNDLE_X_OFFSET);
	int y_offset = (signed char)bundle_take_byte(decoder, BUNDLE_Y_OFFSET);
	long x = (long)(bx * 8) + x_offset;
	long y = (long)(by * 8) + y_offset;

	if (decoder->failed || x < 0 || y < 0 || (unsigned long)x + 8 > plane->stride ||
		(unsigned long)y + 8 > plane->padded_height)
	{
		decoder->failed = 1;
		return 0;
	}
	copy_block(destination, plane->stride, plane->previous + (unsigned long)y * plane->stride + (unsigned long)x,
		plane->stride);
	return 1;
}

/* a block of one of the kinds that make pixels from nothing, into an area
of stride apart rows */
static int decode_new_block(struct bink_decoder *decoder, struct bits *bits, int type, unsigned char *destination,
	unsigned long stride)
{
	int row, column;

	switch (type)
	{
	case BLOCK_RUN:
		{
			const unsigned char *scan = decoder->tables->patterns[bits_get(bits, 4)];
			int total = 0;

			do
			{
				int run = bundle_take_byte(decoder, BUNDLE_RUN) + 1;
				int index;

				total += run;
				if (total > 64 || decoder->failed)
				{
					decoder->failed = 1;
					return 0;
				}
				if (bits_get_one(bits))
				{
					int value = bundle_take_byte(decoder, BUNDLE_COLORS);

					for (index = 0; index < run; index++, scan++)
					{
						destination[(*scan >> 3) * stride + (*scan & 7)] = (unsigned char)value;
					}
				}
				else
				{
					for (index = 0; index < run; index++, scan++)
					{
						destination[(*scan >> 3) * stride + (*scan & 7)] =
							(unsigned char)bundle_take_byte(decoder, BUNDLE_COLORS);
					}
				}
			}
			while (total < 63);
			if (total == 63)
			{
				destination[(*scan >> 3) * stride + (*scan & 7)] =
					(unsigned char)bundle_take_byte(decoder, BUNDLE_COLORS);
			}
		}
		break;
	case BLOCK_INTRA:
		{
			int block[64];

			memset(block, 0, sizeof(block));
			block[0] = bundle_take_dc(decoder, BUNDLE_INTRA_DC);
			if (!read_coefficients(decoder, bits, block, decoder->tables->intra_quantizer))
			{
				decoder->failed = 1;
				return 0;
			}
			transform_put(destination, stride, block);
		}
		break;
	case BLOCK_PATTERN:
		{
			unsigned char colors[2];

			colors[0] = (unsigned char)bundle_take_byte(decoder, BUNDLE_COLORS);
			colors[1] = (unsigned char)bundle_take_byte(decoder, BUNDLE_COLORS);
			for (row = 0; row < 8; row++)
			{
				int mask = bundle_take_byte(decoder, BUNDLE_PATTERN);

				for (column = 0; column < 8; column++, mask >>= 1)
				{
					destination[row * stride + column] = colors[mask & 1];
				}
			}
		}
		break;
	case BLOCK_RAW:
		{
			struct bundle *colors = &decoder->bundles[BUNDLE_COLORS];

			if (colors->read + 64 > colors->decoded)
			{
				decoder->failed = 1;
				return 0;
			}
			for (row = 0; row < 8; row++)
			{
				memcpy(destination + row * stride, colors->data + colors->read, 8);
				colors->read += 8;
			}
		}
		break;
	default:
		decoder->failed = 1;
		return 0;
	}
	return !decoder->failed && !bits->overrun;
}

/* one plane of the frame */
static int decode_plane(struct bink_decoder *decoder, struct bits *bits, int index)
{
	struct plane *plane = &decoder->planes[index];
	unsigned long block_width = (plane->width + 7) >> 3;
	unsigned long block_height = (plane->height + 7) >> 3;
	unsigned long width = plane->width > 8 ? plane->width : 8;
	unsigned long stride = plane->stride;
	unsigned long bx, by;
	int number;

	decoder->bundles[BUNDLE_BLOCK_TYPES].length_bits = log2_floor((width >> 3) + 511) + 1;
	decoder->bundles[BUNDLE_SUB_BLOCK_TYPES].length_bits = log2_floor((width >> 4) + 511) + 1;
	decoder->bundles[BUNDLE_COLORS].length_bits = log2_floor(block_width * 64 + 511) + 1;
	decoder->bundles[BUNDLE_INTRA_DC].length_bits = log2_floor((width >> 3) + 511) + 1;
	decoder->bundles[BUNDLE_INTER_DC].length_bits = log2_floor((width >> 3) + 511) + 1;
	decoder->bundles[BUNDLE_X_OFFSET].length_bits = log2_floor((width >> 3) + 511) + 1;
	decoder->bundles[BUNDLE_Y_OFFSET].length_bits = log2_floor((width >> 3) + 511) + 1;
	decoder->bundles[BUNDLE_PATTERN].length_bits = log2_floor((block_width << 3) + 511) + 1;
	decoder->bundles[BUNDLE_RUN].length_bits = log2_floor(block_width * 48 + 511) + 1;

	for (number = 0; number < NUMBER_OF_BUNDLES; number++)
	{
		struct bundle *bundle = &decoder->bundles[number];

		if (number == BUNDLE_COLORS)
		{
			int high;

			for (high = 0; high < 16; high++)
			{
				tree_read(bits, &decoder->color_high[high]);
			}
			decoder->color_last = 0;
		}
		if (number != BUNDLE_INTRA_DC && number != BUNDLE_INTER_DC)
		{
			tree_read(bits, &bundle->tree);
		}
		bundle->decoded = 0;
		bundle->read = 0;
		bundle->empty = 0;
		bundle->capacity = block_width * block_height * 64;
	}
	if (bits->overrun)
	{
		return 0;
	}

	decoder->failed = 0;
	for (by = 0; by < block_height; by++)
	{
		unsigned char *row_current = plane->current + by * 8 * stride;

		if (!bundle_read_block_types(bits, &decoder->bundles[BUNDLE_BLOCK_TYPES],
				&decoder->bundles[BUNDLE_BLOCK_TYPES].tree, decoder) ||
			!bundle_read_block_types(bits, &decoder->bundles[BUNDLE_SUB_BLOCK_TYPES],
				&decoder->bundles[BUNDLE_SUB_BLOCK_TYPES].tree, decoder) ||
			!bundle_read_colors(decoder, bits, &decoder->bundles[BUNDLE_COLORS]) ||
			!bundle_read_patterns(decoder, bits, &decoder->bundles[BUNDLE_PATTERN]) ||
			!bundle_read_motion(decoder, bits, &decoder->bundles[BUNDLE_X_OFFSET]) ||
			!bundle_read_motion(decoder, bits, &decoder->bundles[BUNDLE_Y_OFFSET]) ||
			!bundle_read_dc(bits, &decoder->bundles[BUNDLE_INTRA_DC], 0) ||
			!bundle_read_dc(bits, &decoder->bundles[BUNDLE_INTER_DC], 1) ||
			!bundle_read_runs(decoder, bits, &decoder->bundles[BUNDLE_RUN]))
		{
			return 0;
		}

		for (bx = 0; bx < block_width; bx++)
		{
			unsigned char *destination = row_current + bx * 8;
			int type = bundle_take_byte(decoder, BUNDLE_BLOCK_TYPES);

			if (decoder->failed)
			{
				return 0;
			}
			/* the second row of a 16x16 block is already done */
			if ((by & 1) && type == BLOCK_SCALED)
			{
				bx++;
				continue;
			}
			switch (type)
			{
			case BLOCK_SKIP:
				copy_block(destination, stride, plane->previous + (destination - plane->current), stride);
				break;
			case BLOCK_SCALED:
				{
					unsigned char small[64];
					int sub_type = bundle_take_byte(decoder, BUNDLE_SUB_BLOCK_TYPES);

					if (decoder->failed || bx + 1 >= block_width || by + 1 >= block_height)
					{
						return 0;
					}
					if (sub_type == BLOCK_FILL)
					{
						fill_block(destination, stride, bundle_take_byte(decoder, BUNDLE_COLORS), 16);
					}
					else
					{
						int row, column;

						if (!decode_new_block(decoder, bits, sub_type, small, 8))
						{
							return 0;
						}
						for (row = 0; row < 8; row++)
						{
							for (column = 0; column < 8; column++)
							{
								unsigned char *pixel = destination + row * 2 * stride + column * 2;

								pixel[0] = pixel[1] = pixel[stride] = pixel[stride + 1] = small[row * 8 + column];
							}
						}
					}
					bx++;
				}
				break;
			case BLOCK_MOTION:
				if (!copy_motion_block(decoder, plane, bx, by, destination))
				{
					return 0;
				}
				break;
			case BLOCK_RESIDUE:
				{
					int block[64];
					int row, column;
					int budget;

					if (!copy_motion_block(decoder, plane, bx, by, destination))
					{
						return 0;
					}
					budget = (int)bits_get(bits, 7);
					memset(block, 0, sizeof(block));
					if (!read_residue(decoder, bits, block, budget))
					{
						return 0;
					}
					for (row = 0; row < 8; row++)
					{
						for (column = 0; column < 8; column++)
						{
							unsigned char *pixel = destination + row * stride + column;

							*pixel = clip_byte((int)((unsigned)*pixel + (unsigned)block[row * 8 + column]));
						}
					}
				}
				break;
			case BLOCK_INTER:
				{
					int block[64];

					if (!copy_motion_block(decoder, plane, bx, by, destination))
					{
						return 0;
					}
					memset(block, 0, sizeof(block));
					block[0] = bundle_take_dc(decoder, BUNDLE_INTER_DC);
					if (decoder->failed || !read_coefficients(decoder, bits, block, decoder->tables->inter_quantizer))
					{
						return 0;
					}
					transform_add(destination, stride, block);
				}
				break;
			case BLOCK_FILL:
				fill_block(destination, stride, bundle_take_byte(decoder, BUNDLE_COLORS), 8);
				break;
			default:
				if (!decode_new_block(decoder, bits, type, destination, stride))
				{
					return 0;
				}
				break;
			}
			if (decoder->failed || bits->overrun)
			{
				return 0;
			}
		}
	}
	bits_align32(bits);
	return !bits->overrun;
}

static int decode_video(struct bink_decoder *decoder, struct bits *bits)
{
	int plane, index;
	struct plane *swap_planes[4];

	if (decoder->info.has_alpha)
	{
		if (decoder->info.revision >= 'i')
		{
			bits_skip(bits, 32);
		}
		if (!decode_plane(decoder, bits, 3))
		{
			return 0;
		}
	}
	if (decoder->info.revision >= 'i')
	{
		bits_skip(bits, 32);
	}
	for (plane = 0; plane < 3; plane++)
	{
		index = (plane == 0 || !decoder->swap_planes) ? plane : (plane ^ 3);
		if (!decode_plane(decoder, bits, index))
		{
			return 0;
		}
		if (bits->position >= bits->size_bits)
		{
			break;
		}
	}
	/* what was drawn is the reference of the next frame */
	(void)swap_planes;
	for (plane = 0; plane < 4; plane++)
	{
		unsigned char *current = decoder->planes[plane].current;

		decoder->planes[plane].current = decoder->planes[plane].previous;
		decoder->planes[plane].previous = current;
	}
	return 1;
}

/* ---------- audio */

#ifndef AUDIO_IMAGINARY_SIGN
#define AUDIO_IMAGINARY_SIGN 1.0f
#endif

/* the packed spectrum of audio_frame_length values, as the format has it:
the first two are the real values at frequency 0 and at half the sampling
rate, and the rest are real and imaginary parts of the frequencies between;
the samples are the real part of its inverse transform (halved, as the
coefficients are scaled for that). The transform is a complex FFT of the
whole symmetric spectrum */
static void audio_transform(struct bink_decoder *decoder, float *coefficients)
{
	unsigned long length = decoder->audio_frame_length;
	unsigned long half = length / 2;
	float *real = decoder->audio_spectrum;
	float *imaginary = decoder->audio_spectrum + length;
	unsigned long index, size, start, offset;

	real[0] = coefficients[0];
	imaginary[0] = 0.0f;
	real[half] = coefficients[1];
	imaginary[half] = 0.0f;
	for (index = 1; index < half; index++)
	{
		real[index] = coefficients[index * 2];
		imaginary[index] = AUDIO_IMAGINARY_SIGN * coefficients[index * 2 + 1];
		real[length - index] = coefficients[index * 2];
		imaginary[length - index] = -AUDIO_IMAGINARY_SIGN * coefficients[index * 2 + 1];
	}
	/* bit reversed order */
	for (index = 0; index < length; index++)
	{
		unsigned long other = decoder->audio_reverse[index];

		if (other > index)
		{
			float swap = real[index];

			real[index] = real[other];
			real[other] = swap;
			swap = imaginary[index];
			imaginary[index] = imaginary[other];
			imaginary[other] = swap;
		}
	}
	for (size = 2; size <= length; size <<= 1)
	{
		unsigned long step = length / size;

		for (start = 0; start < length; start += size)
		{
			for (offset = 0; offset < size / 2; offset++)
			{
				float cosine = decoder->audio_cos[offset * step];
				float sine = decoder->audio_sin[offset * step];
				unsigned long low = start + offset;
				unsigned long high = low + size / 2;
				float high_real = real[high] * cosine - imaginary[high] * sine;
				float high_imaginary = real[high] * sine + imaginary[high] * cosine;

				real[high] = real[low] - high_real;
				imaginary[high] = imaginary[low] - high_imaginary;
				real[low] += high_real;
				imaginary[low] += high_imaginary;
			}
		}
	}
	for (index = 0; index < length; index++)
	{
		decoder->audio_samples[index] = real[index] * 0.5f;
	}
}

/* a number stored as a 5-bit exponent, 23-bit mantissa and a sign */
static float audio_get_float(struct bits *bits)
{
	int power = (int)bits_get(bits, 5);
	unsigned long mantissa = bits_get(bits, 23);
	double value = (double)mantissa;
	int exponent = power - 23;
	int sign = bits_get_one(bits);

	while (exponent > 0)
	{
		value *= 2.0;
		exponent--;
	}
	while (exponent < 0)
	{
		value *= 0.5;
		exponent++;
	}
	return (float)(sign ? -value : value);
}

static void audio_push(struct bink_decoder *decoder, const float *samples, unsigned long count)
{
	unsigned long ring = decoder->audio_output_capacity * (unsigned long)decoder->audio_output_channels;
	unsigned long room = ring - decoder->audio_output_count;
	unsigned long at = decoder->audio_output_head + decoder->audio_output_count;
	unsigned long index;

	if (at >= ring)
	{
		at -= ring;
	}
	if (count > room)
	{
		/* (nobody is taking the audio: the rest is dropped) */
		count = room;
	}
	for (index = 0; index < count; index++)
	{
		float scaled = samples[index] * 32768.0f;

		decoder->audio_output[at] = scaled >= 32767.0f ? 32767 : scaled <= -32768.0f ? -32768 :
			(short)(scaled + (scaled < 0 ? -0.5f : 0.5f));
		if (++at == ring)
		{
			at = 0;
		}
	}
	decoder->audio_output_count += count;
}

/* one block of audio from bits: block_size samples out */
static int audio_decode_block(struct bink_decoder *decoder, struct bits *bits)
{
	unsigned long length = decoder->audio_frame_length;
	float *coefficients = decoder->audio_coefficients;
	float quantizer[AUDIO_BANDS];
	unsigned long band, position, end, k;
	float q;

	/* (the two values at the ends of the spectrum are in the units of the samples) */
	coefficients[0] = audio_get_float(bits) * decoder->audio_root;
	coefficients[1] = audio_get_float(bits) * decoder->audio_root;
	for (band = 0; band < decoder->audio_band_count; band++)
	{
		unsigned long value = bits_get(bits, 8);

		quantizer[band] = decoder->audio_quantizer[value < AUDIO_QUANTIZER_COUNT - 1 ? value : AUDIO_QUANTIZER_COUNT - 1];
	}
	k = 0;
	q = quantizer[0];
	position = 2;
	while (position < length)
	{
		int width;

		if (bits_get_one(bits))
		{
			end = position + (unsigned long)decoder->tables->run_lengths[bits_get(bits, 4)] * 8;
		}
		else
		{
			end = position + 8;
		}
		if (end > length)
		{
			end = length;
		}
		width = (int)bits_get(bits, 4);
		if (width == 0)
		{
			while (position < end)
			{
				coefficients[position++] = 0.0f;
			}
			while (k < decoder->audio_band_count && decoder->audio_bands[k] < position)
			{
				q = quantizer[k++];
			}
		}
		else
		{
			while (position < end)
			{
				unsigned long value;

				if (k < decoder->audio_band_count && decoder->audio_bands[k] == position)
				{
					q = quantizer[k++];
				}
				value = bits_get(bits, width);
				if (value)
				{
					coefficients[position] = bits_get_one(bits) ? -q * (float)value : q * (float)value;
				}
				else
				{
					coefficients[position] = 0.0f;
				}
				position++;
			}
		}
		if (bits->overrun)
		{
			return 0;
		}
	}
	bits_align32(bits);
	if (bits->overrun)
	{
		return 0;
	}
	audio_transform(decoder, coefficients);
	{
		unsigned long overlap = decoder->audio_overlap;
		float *samples = decoder->audio_samples;
		unsigned long index;

		/* the start of this block fades in as the end of the last fades out */
		if (!decoder->audio_first)
		{
			for (index = 0; index < overlap; index++)
			{
#ifdef AUDIO_FADE_SWAP
				samples[index] = (decoder->audio_previous[index] * (float)index +
					samples[index] * (float)(overlap - index)) / (float)overlap;
#else
				samples[index] = (decoder->audio_previous[index] * (float)(overlap - index) +
					samples[index] * (float)index) / (float)overlap;
#endif
			}
		}
		audio_push(decoder, samples, decoder->audio_block_size);
		memcpy(decoder->audio_previous, samples + decoder->audio_block_size, overlap * sizeof(float));
		decoder->audio_first = 0;
	}
	return 1;
}

/* ---------- container */

static void audio_free(struct bink_decoder *decoder)
{
	free(decoder->audio_coefficients);
	free(decoder->audio_samples);
	free(decoder->audio_spectrum);
	free(decoder->audio_previous);
	free(decoder->audio_cos);
	free(decoder->audio_sin);
	free(decoder->audio_reverse);
	free(decoder->audio_output);
	decoder->audio_coefficients = NULL;
	decoder->audio_samples = NULL;
	decoder->audio_spectrum = NULL;
	decoder->audio_previous = NULL;
	decoder->audio_cos = NULL;
	decoder->audio_sin = NULL;
	decoder->audio_reverse = NULL;
	decoder->audio_output = NULL;
	decoder->audio_enabled = 0;
}

static int audio_initialize(struct bink_decoder *decoder, const struct audio_track *track)
{
	unsigned long sample_rate = track->sample_rate;
	unsigned long length, half, index, count;
	int channels = track->channels;
	int frame_bits = sample_rate < 22050 ? 9 : sample_rate < 44100 ? 10 : 11;
	double root;
	int bit;

	/* the two channels are one stream, interleaved, at twice the rate */
	if (channels > 1)
	{
		frame_bits += 1;
		sample_rate *= (unsigned long)channels;
	}
	length = 1UL << frame_bits;
	decoder->audio_frame_bits = frame_bits;
	decoder->audio_frame_length = length;
	decoder->audio_overlap = length / 16;
	decoder->audio_block_size = length - decoder->audio_overlap;
	half = (sample_rate + 1) / 2;

	root = 2.0 / (sqrt((double)length) * 32768.0);
	decoder->audio_root = (float)root;
	for (index = 0; index < AUDIO_QUANTIZER_COUNT; index++)
	{
		decoder->audio_quantizer[index] = (float)(exp((double)index * 0.15289164787221953823) * root);
	}
	for (count = 1; count < 25; count++)
	{
		if (half <= decoder->tables->critical_frequencies[count - 1])
		{
			break;
		}
	}
	decoder->audio_band_count = count;
	decoder->audio_bands[0] = 2;
	for (index = 1; index < count; index++)
	{
		decoder->audio_bands[index] = (decoder->tables->critical_frequencies[index - 1] * length / half) & ~1UL;
	}
	decoder->audio_bands[count] = length;

	decoder->audio_coefficients = (float *)calloc(length, sizeof(float));
	decoder->audio_samples = (float *)calloc(length, sizeof(float));
	decoder->audio_spectrum = (float *)calloc(length * 2, sizeof(float));
	decoder->audio_previous = (float *)calloc(decoder->audio_overlap, sizeof(float));
	decoder->audio_cos = (float *)calloc(length / 2, sizeof(float));
	decoder->audio_sin = (float *)calloc(length / 2, sizeof(float));
	decoder->audio_reverse = (unsigned short *)calloc(length, sizeof(unsigned short));
	decoder->audio_output_channels = channels;
	decoder->audio_output_capacity = track->sample_rate * 2;
	decoder->audio_output = (short *)calloc(decoder->audio_output_capacity * (unsigned long)channels, sizeof(short));
	if (!decoder->audio_coefficients || !decoder->audio_samples || !decoder->audio_spectrum ||
		!decoder->audio_previous || !decoder->audio_cos || !decoder->audio_sin || !decoder->audio_reverse ||
		!decoder->audio_output)
	{
		audio_free(decoder);
		return 0;
	}
	for (index = 0; index < length / 2; index++)
	{
		double angle = 6.283185307179586 * (double)index / (double)length;

		decoder->audio_cos[index] = (float)cos(angle);
		decoder->audio_sin[index] = (float)sin(angle);
	}
	for (index = 0; index < length; index++)
	{
		unsigned long reversed = 0;

		for (bit = 0; bit < frame_bits; bit++)
		{
			if (index & (1UL << bit))
			{
				reversed |= 1UL << (frame_bits - 1 - bit);
			}
		}
		decoder->audio_reverse[index] = (unsigned short)reversed;
	}
	decoder->audio_first = 1;
	decoder->audio_enabled = 1;
	return 1;
}

static void planes_free(struct bink_decoder *decoder)
{
	int index;

	for (index = 0; index < 4; index++)
	{
		free(decoder->planes[index].current);
		free(decoder->planes[index].previous);
		decoder->planes[index].current = NULL;
		decoder->planes[index].previous = NULL;
	}
	for (index = 0; index < NUMBER_OF_BUNDLES; index++)
	{
		free(decoder->bundles[index].data);
		decoder->bundles[index].data = NULL;
	}
}

void bink_close(struct bink_decoder *decoder)
{
	if (decoder)
	{
		audio_free(decoder);
		planes_free(decoder);
		free(decoder->index);
		free(decoder->packet);
		free(decoder);
	}
}

struct bink_decoder *bink_open(const struct bink_tables *tables, bink_read_proc read, void *context,
	unsigned long file_size, char *error, int error_size)
{
	struct bink_decoder *decoder;
	unsigned char header[HEADER_SIZE];
	unsigned char *buffer = NULL;
	unsigned long frames, tracks, index_offset, position, largest = 0, count;
	unsigned long track;
	int revision, plane;

	if (file_size < HEADER_SIZE || !read(context, 0, header, HEADER_SIZE))
	{
		set_error(error, error_size, "the movie is too short to be one");
		return NULL;
	}
	if (header[0] != 'B' || header[1] != 'I' || header[2] != 'K')
	{
		set_error(error, error_size, "not a Bink movie");
		return NULL;
	}
	revision = header[3];
	if (revision < 'f' || revision > 'i')
	{
		set_error(error, error_size, "the Bink movie's revision is not supported");
		return NULL;
	}
	frames = read_u32(header + 8);
	tracks = read_u32(header + 40);
	if (frames == 0 || frames > BINK_MAXIMUM_FRAMES || tracks > MAXIMUM_AUDIO_TRACKS ||
		read_u32(header + 20) == 0 || read_u32(header + 20) > BINK_MAXIMUM_DIMENSION ||
		read_u32(header + 24) == 0 || read_u32(header + 24) > BINK_MAXIMUM_DIMENSION ||
		read_u32(header + 28) == 0 || read_u32(header + 32) == 0)
	{
		set_error(error, error_size, "the Bink movie's header is not valid");
		return NULL;
	}

	decoder = (struct bink_decoder *)calloc(1, sizeof(*decoder));
	if (!decoder)
	{
		set_error(error, error_size, "out of memory");
		return NULL;
	}
	decoder->tables = tables;
	decoder->read = read;
	decoder->context = context;
	decoder->file_size = file_size;
	decoder->info.revision = revision;
	decoder->info.frames = frames;
	decoder->info.width = read_u32(header + 20);
	decoder->info.height = read_u32(header + 24);
	decoder->info.frame_rate = read_u32(header + 28);
	decoder->info.frame_rate_divisor = read_u32(header + 32);
	decoder->video_flags = read_u32(header + 36);
	decoder->info.has_alpha = (decoder->video_flags & VIDEO_FLAG_ALPHA) != 0;
	decoder->swap_planes = revision >= 'h';
	decoder->audio_tracks = (int)tracks;

	/* the audio tracks' descriptions, then the frame index */
	index_offset = HEADER_SIZE + tracks * 12;
	if (tracks)
	{
		buffer = (unsigned char *)malloc(tracks * 12);
		if (!buffer || !read(context, HEADER_SIZE, buffer, tracks * 12))
		{
			set_error(error, error_size, "the Bink movie's audio tracks are cut short");
			goto fail;
		}
		for (track = 0; track < tracks; track++)
		{
			unsigned long rate_flags = read_u32(buffer + tracks * 4 + track * 4);

			decoder->track[track].sample_rate = rate_flags & 0xFFFF;
			decoder->track[track].flags = (int)(rate_flags >> 16);
			decoder->track[track].channels = (rate_flags >> 16) & AUDIO_FLAG_STEREO ? 2 : 1;
		}
		free(buffer);
		buffer = NULL;
	}
	if ((frames + 1) * 4 > file_size || index_offset > file_size - (frames + 1) * 4)
	{
		set_error(error, error_size, "the Bink movie's frame index is cut short");
		goto fail;
	}
	decoder->index = (unsigned long *)malloc((frames + 1) * sizeof(unsigned long));
	buffer = (unsigned char *)malloc((frames + 1) * 4);
	if (!decoder->index || !buffer || !read(context, index_offset, buffer, (frames + 1) * 4))
	{
		set_error(error, error_size, "the Bink movie's frame index could not be read");
		goto fail;
	}
	position = index_offset + (frames + 1) * 4;
	for (count = 0; count <= frames; count++)
	{
		unsigned long offset = read_u32(buffer + count * 4) & ~1UL;

		if (offset < position || offset > file_size)
		{
			set_error(error, error_size, "the Bink movie's frame index is not valid");
			goto fail;
		}
		if (count > 0 && offset - position > largest)
		{
			largest = offset - position;
		}
		decoder->index[count] = offset;
		position = offset;
	}
	free(buffer);
	buffer = NULL;
	if (largest == 0 || largest > MAXIMUM_PACKET_SIZE)
	{
		set_error(error, error_size, "the Bink movie's frames are too large");
		goto fail;
	}
	decoder->packet_capacity = largest;
	decoder->packet = (unsigned char *)calloc(largest + 8, 1);
	if (!decoder->packet)
	{
		set_error(error, error_size, "out of memory");
		goto fail;
	}

	/* the pictures */
	for (plane = 0; plane < 4; plane++)
	{
		struct plane *entry = &decoder->planes[plane];
		unsigned long width = decoder->info.width;
		unsigned long height = decoder->info.height;
		unsigned long size;

		if (plane == 1 || plane == 2)
		{
			width = (width + 1) >> 1;
			height = (height + 1) >> 1;
		}
		entry->width = width;
		entry->height = height;
		entry->stride = ((width + 7) >> 3) * 8;
		entry->padded_height = ((height + 7) >> 3) * 8;
		size = entry->stride * entry->padded_height;
		if (plane == 3 && !decoder->info.has_alpha)
		{
			continue;
		}
		entry->current = (unsigned char *)calloc(size, 1);
		entry->previous = (unsigned char *)calloc(size, 1);
		if (!entry->current || !entry->previous)
		{
			set_error(error, error_size, "out of memory");
			goto fail;
		}
	}
	for (plane = 0; plane < NUMBER_OF_BUNDLES; plane++)
	{
		unsigned long capacity = decoder->planes[0].stride * decoder->planes[0].padded_height;

		decoder->bundles[plane].data = (unsigned char *)calloc(capacity + 16, 1);
		if (!decoder->bundles[plane].data)
		{
			set_error(error, error_size, "out of memory");
			goto fail;
		}
	}
	trees_build(decoder);

	if (tracks && decoder->track[0].sample_rate && !(decoder->track[0].flags & AUDIO_FLAG_DCT))
	{
		if (audio_initialize(decoder, &decoder->track[0]))
		{
			decoder->info.sample_rate = decoder->track[0].sample_rate;
			decoder->info.channels = decoder->track[0].channels;
		}
	}
	return decoder;

fail:
	free(buffer);
	bink_close(decoder);
	return NULL;
}

const struct bink_info *bink_get_info(const struct bink_decoder *decoder)
{
	return &decoder->info;
}

unsigned long bink_next_frame(const struct bink_decoder *decoder)
{
	return decoder->next;
}

void bink_rewind(struct bink_decoder *decoder)
{
	decoder->next = 0;
	decoder->audio_first = 1;
	decoder->audio_output_head = 0;
	decoder->audio_output_count = 0;
}

int bink_decode_frame(struct bink_decoder *decoder, char *error, int error_size)
{
	unsigned long offset, size, position;
	int track;
	struct bits bits;

	if (decoder->next >= decoder->info.frames)
	{
		set_error(error, error_size, "the movie has ended");
		return 0;
	}
	offset = decoder->index[decoder->next];
	size = decoder->index[decoder->next + 1] - offset;
	decoder->next++;
	if (size == 0 || size > decoder->packet_capacity ||
		!decoder->read(decoder->context, offset, decoder->packet, size))
	{
		decoder->next = decoder->info.frames;
		set_error(error, error_size, "a frame of the movie could not be read");
		return 0;
	}
	memset(decoder->packet + size, 0, 8);

	position = 0;
	for (track = 0; track < decoder->audio_tracks; track++)
	{
		unsigned long length;

		if (size - position < 4)
		{
			decoder->next = decoder->info.frames;
			set_error(error, error_size, "a frame's audio is cut short");
			return 0;
		}
		length = read_u32(decoder->packet + position);
		position += 4;
		if (length > size - position)
		{
			decoder->next = decoder->info.frames;
			set_error(error, error_size, "a frame's audio is cut short");
			return 0;
		}
		if (track == 0 && decoder->audio_enabled && length > 4)
		{
			bits_init(&bits, decoder->packet + position + 4, length - 4);
			while (bits.position + 32 <= bits.size_bits)
			{
				if (!audio_decode_block(decoder, &bits))
				{
					break;
				}
			}
		}
		position += length;
	}
	bits_init(&bits, decoder->packet + position, size - position);
	if (!decode_video(decoder, &bits))
	{
		decoder->next = decoder->info.frames;
		set_error(error, error_size, "a frame of the movie is damaged");
		return 0;
	}
	return 1;
}

unsigned long bink_audio_available(const struct bink_decoder *decoder)
{
	return decoder->audio_enabled ? decoder->audio_output_count / (unsigned long)decoder->audio_output_channels : 0;
}

unsigned long bink_take_audio(struct bink_decoder *decoder, short *samples, unsigned long max_frames)
{
	unsigned long frames = bink_audio_available(decoder);
	unsigned long channels = (unsigned long)decoder->audio_output_channels;
	unsigned long index, total, ring, at;

	if (frames > max_frames)
	{
		frames = max_frames;
	}
	total = frames * channels;
	ring = decoder->audio_output_capacity * channels;
	at = decoder->audio_output_head;
	for (index = 0; index < total; index++)
	{
		samples[index] = decoder->audio_output[at];
		if (++at == ring)
		{
			at = 0;
		}
	}
	decoder->audio_output_head = at;
	decoder->audio_output_count -= total;
	return frames;
}

/* ---------- pixels */

void bink_copy_to_buffer(const struct bink_decoder *decoder, void *destination, long pitch,
	unsigned long destination_height, unsigned long x, unsigned long y)
{
	const struct plane *luma = &decoder->planes[0];
	const struct plane *blue = &decoder->planes[1];
	const struct plane *red = &decoder->planes[2];
	unsigned long width = decoder->info.width;
	unsigned long height = decoder->info.height;
	unsigned long row, column;
	int red_from_v[256], green_from_u[256], green_from_v[256], blue_from_u[256];
	unsigned char clamp[768];
	int value;

	/* full-range YCbCr (Rec. 601): what each chroma value adds to the three
	colours, in 16.16 fixed point where it is not a whole number */
	for (value = 0; value < 256; value++)
	{
		int centered = value - 128;

		red_from_v[value] = (91881 * centered + 32768) >> 16;
		green_from_u[value] = 22554 * centered;
		green_from_v[value] = 46802 * centered;
		blue_from_u[value] = (116130 * centered + 32768) >> 16;
	}
	/* a colour is the luma and the chroma's addition, from -256 to 511, held to 0 to 255 */
	for (value = 0; value < 768; value++)
	{
		clamp[value] = clip_byte(value - 256);
	}
	if (pitch < 4 || x >= (unsigned long)pitch / 4)
	{
		return;
	}
	if (width > (unsigned long)pitch / 4 - x)
	{
		width = (unsigned long)pitch / 4 - x;
	}
	if (y >= destination_height)
	{
		return;
	}
	if (height > destination_height - y)
	{
		height = destination_height - y;
	}
	for (row = 0; row < height; row++)
	{
		/* (the last decoded picture is the reference now) */
		const unsigned char *y_row = luma->previous + row * luma->stride;
		const unsigned char *u_row = blue->previous + (row >> 1) * blue->stride;
		const unsigned char *v_row = red->previous + (row >> 1) * red->stride;
		unsigned char *out = (unsigned char *)destination + (long)(y + row) * pitch + (long)x * 4;

		for (column = 0; column < width; column += 2)
		{
			int u = u_row[column >> 1];
			int v = v_row[column >> 1];
			int add_red = red_from_v[v] + 256;
			int add_green = 256 - ((green_from_u[u] + green_from_v[v] + 32768) >> 16);
			int add_blue = blue_from_u[u] + 256;
			int luminance = y_row[column];

			out[0] = clamp[luminance + add_blue];
			out[1] = clamp[luminance + add_green];
			out[2] = clamp[luminance + add_red];
			out[3] = 0;
			if (column + 1 < width)
			{
				luminance = y_row[column + 1];
				out[4] = clamp[luminance + add_blue];
				out[5] = clamp[luminance + add_green];
				out[6] = clamp[luminance + add_red];
				out[7] = 0;
			}
			out += 8;
		}
	}
}
