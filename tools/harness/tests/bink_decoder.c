/*
BINK_DECODER.C (test)

The real bink_decoder.c (port/linux/src), which plays the Xbox's movies, on
Bink data written here bit by bit (no movie is needed): a file's header; a
video frame whose blocks are filled with one colour per plane, one whose
blocks are skipped (the last frame's), and a damaged one; an audio block of
a level, of one sine wave, and the crossfade of two blocks. The decoder's
tables are FFmpeg's (port/third_party/ffmpeg-bink/binkdata.h).
test_bink_decoder.py takes the code and the tables from the sources.
*/

#include <math.h>
#include "harness.h"

#pragma clang diagnostic ignored "-Wunused-const-variable"

#include "under_test.inc"

/* ---------- writing bits, least significant first, as the decoder reads */

struct writer
{
	unsigned char data[8192];
	unsigned long bits;
};

static void put(struct writer *writer, unsigned long value, int count)
{
	int index;

	for (index = 0; index < count; index++, writer->bits++)
	{
		if ((value >> index) & 1)
			writer->data[writer->bits >> 3] |= (unsigned char)(1 << (writer->bits & 7));
	}
}

static void align32(struct writer *writer)
{
	writer->bits = (writer->bits + 31) & ~31UL;
}

static unsigned long size_of(const struct writer *writer)
{
	return (writer->bits + 7) >> 3;
}

/* a 4-bit value of Huffman tree 0, whose symbols are themselves */
static void put_symbol(struct writer *writer, int symbol)
{
	put(writer, bink_tree_bits[0][symbol], bink_tree_lens[0][symbol]);
}

static int floor_log2(unsigned long value)
{
	int result = 0;

	while (value >>= 1)
		result++;
	return result;
}

/* ---------- video */

#define WIDTH 24
#define HEIGHT 16
#define FILL 6
#define SKIP 0

/* a frame of revision i: each plane's blocks all FILL with its colour (the
luma, then the second chroma, then the first: the planes are swapped from
revision h), or all SKIP when colours is NULL */
static void video_frame(struct writer *writer, const int *colours)
{
	int plane;

	memset(writer, 0, sizeof(*writer));
	put(writer, 0, 32);
	for (plane = 0; plane < 3; plane++)
	{
		int index = plane ? plane ^ 3 : 0, chroma = plane != 0;
		long width = WIDTH >> chroma;
		long block_width = chroma ? (WIDTH + 15) >> 4 : (WIDTH + 7) >> 3;
		long block_height = chroma ? (HEIGHT + 15) >> 4 : (HEIGHT + 7) >> 3;
		long aligned = ((width > 8 ? width : 8) + 7) & ~7L;
		int types = floor_log2((unsigned long)(aligned >> 3) + 511) + 1;
		int sub_types = floor_log2((unsigned long)(aligned >> 4) + 511) + 1;
		int colours_length = floor_log2((unsigned long)block_width * 64 + 511) + 1;
		int patterns = floor_log2((unsigned long)block_width * 8 + 511) + 1;
		int runs = floor_log2((unsigned long)block_width * 48 + 511) + 1;
		long row;

		/* each bundle's tree: 0, the symbols themselves (the colours'
		sixteen for their high halves, and their own; none for the DCs) */
		put(writer, 0, 4 + 4 + 16 * 4 + 4 + 4 + 4 + 4 + 4);
		for (row = 0; row < block_height; row++)
		{
			/* the row's block types, all one */
			put(writer, (unsigned long)block_width, types);
			put(writer, 1, 1);
			put(writer, colours ? FILL : SKIP, 4);
			if (row == 0)
				put(writer, 0, sub_types);
			if (colours)
			{
				/* a colour for each block, all one */
				put(writer, (unsigned long)block_width, colours_length);
				put(writer, 1, 1);
				put_symbol(writer, colours[index] >> 4);
				put_symbol(writer, colours[index] & 15);
			}
			else if (row == 0)
			{
				put(writer, 0, colours_length);
			}
			if (row == 0)
			{
				/* no patterns, motion, DCs or runs this plane */
				put(writer, 0, patterns);
				put(writer, 0, types);
				put(writer, 0, types);
				put(writer, 0, types);
				put(writer, 0, types);
				put(writer, 0, runs);
			}
		}
		align32(writer);
	}
}

static void expect_planes(struct bink_video *video, int y, int u, int v, const char *what)
{
	const unsigned char *planes[3];
	long strides[2], row, column;
	int plane;

	bink_video_planes(video, &planes[0], &planes[1], &planes[2], &strides[0], &strides[1]);
	for (plane = 0; plane < 3; plane++)
	{
		int expected = plane == 0 ? y : plane == 1 ? u : v;
		long width = WIDTH >> (plane != 0), height = HEIGHT >> (plane != 0);

		for (row = 0; row < height; row++)
			for (column = 0; column < width; column++)
				CHECK(planes[plane][row * strides[plane != 0] + column] == expected,
					"%s: plane %d at %ld,%ld is %d, not %d", what, plane, column, row,
					planes[plane][row * strides[plane != 0] + column], expected);
	}
}

static struct bink_video *video_new(void)
{
	struct bink_header header;

	memset(&header, 0, sizeof(header));
	header.revision = 'i';
	header.width = WIDTH;
	header.height = HEIGHT;
	return bink_video_new(&header);
}

/* ---------- audio: stereo at 44.1 kHz, one transform of 4096 samples */

#define AUDIO_BANDS 25
#define AUDIO_FRAME 4096
#define AUDIO_BLOCK (AUDIO_FRAME - AUDIO_FRAME / 16)

/* a value of 23 bits times 2^(power - 23) */
static void put_float(struct writer *writer, unsigned long mantissa, int power)
{
	put(writer, (unsigned long)power, 5);
	put(writer, mantissa, 23);
	put(writer, 0, 1);
}

/* a block: the level (the first coefficient, 64 times the samples it
gives), and one coefficient of 15 bits at index, none if it is 0 */
static void audio_block(struct writer *writer, unsigned long level, int index, unsigned long value)
{
	int at, band;

	put_float(writer, level, 23);
	put_float(writer, 0, 0);
	/* each band's quantizer: the first, the root */
	for (band = 0; band < AUDIO_BANDS; band++)
		put(writer, 0, 8);
	for (at = 2; at < AUDIO_FRAME; at += 8)
	{
		int end = at + 8 < AUDIO_FRAME ? at + 8 : AUDIO_FRAME, k;

		/* (no run: eight coefficients) */
		put(writer, 0, 1);
		if (index >= at && index < end)
		{
			put(writer, 15, 4);
			for (k = at; k < end; k++)
			{
				put(writer, k == index ? value : 0, 15);
				if (k == index)
					put(writer, 0, 1);
			}
		}
		else
		{
			put(writer, 0, 4);
		}
	}
	align32(writer);
}

static struct bink_audio *audio_new(void)
{
	struct bink_header header;

	memset(&header, 0, sizeof(header));
	header.revision = 'i';
	header.audio_track_count = 1;
	header.audio_sample_rate = 44100;
	header.audio_flags = 0x4000 | BINK_AUDIO_STEREO;
	return bink_audio_new(&header);
}

static short samples[AUDIO_BLOCK * 4];

static unsigned long audio_decode(struct bink_audio *audio, struct writer *writer)
{
	return bink_audio_decode(audio, writer->data, size_of(writer), samples, AUDIO_BLOCK * 4);
}

static void expect_sample(unsigned long index, double expected, const char *what)
{
	CHECK(fabs(samples[index] - expected) <= 2.0, "%s: sample %lu is %d, not %.0f", what, index, samples[index],
		expected);
}

/* ---------- header */

static void header_bytes(unsigned char *bytes, char revision, unsigned long width, unsigned long tracks)
{
	unsigned long words[11] = { 0, 1000000 - 8, 10, 100, 10, 0, 480, 2997, 100, 0, 0 };
	int index;

	memset(bytes, 0, 64);
	words[5] = width;
	words[10] = tracks;
	for (index = 1; index < 11; index++)
	{
		bytes[index * 4] = (unsigned char)words[index];
		bytes[index * 4 + 1] = (unsigned char)(words[index] >> 8);
		bytes[index * 4 + 2] = (unsigned char)(words[index] >> 16);
		bytes[index * 4 + 3] = (unsigned char)(words[index] >> 24);
	}
	bytes[0] = 'B';
	bytes[1] = 'I';
	bytes[2] = 'K';
	bytes[3] = (unsigned char)revision;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	static struct writer writer;

	CASE("header")
	{
		unsigned char bytes[64];
		struct bink_header header;
		int skip = 0;

		for (skip = 0; skip < 2; skip++)
		{
			/* one audio track: its largest size, its rate and flags, its id
			(after an unknown field in revision k) */
			header_bytes(bytes, skip ? 'k' : 'i', 640, 1);
			bytes[44 + skip * 4 + 4] = 0x44;
			bytes[44 + skip * 4 + 5] = 0xAC;
			bytes[44 + skip * 4 + 7] = 0x60;
			CHECK(bink_header_read(bytes, 64, &header), "revision %c refused", skip ? 'k' : 'i');
			CHECK(header.width == 640 && header.height == 480 && header.frame_count == 10 &&
				header.frame_rate == 2997 && header.frame_rate_divisor == 100 && header.file_size == 1000000,
				"revision %c: %lux%lu, %lu frames", skip ? 'k' : 'i', header.width, header.height, header.frame_count);
			CHECK(header.audio_sample_rate == 44100 && header.audio_flags == 0x6000,
				"revision %c: audio %lu Hz, flags %#lx", skip ? 'k' : 'i', header.audio_sample_rate, header.audio_flags);
			CHECK(header.index_offset == 56UL + skip * 4, "revision %c: the index at %lu", skip ? 'k' : 'i',
				header.index_offset);
		}
		header_bytes(bytes, 'b', 640, 1);
		CHECK(!bink_header_read(bytes, 64, &header), "revision b taken");
		header_bytes(bytes, 'i', 0, 1);
		CHECK(!bink_header_read(bytes, 64, &header), "no width taken");
		header_bytes(bytes, 'i', 640, 300);
		CHECK(!bink_header_read(bytes, 64, &header), "300 audio tracks taken");
		header_bytes(bytes, 'i', 640, 1);
		CHECK(!bink_header_read(bytes, 40, &header), "a short header taken");
		return 0;
	}
	CASE("video fill")
	{
		struct bink_video *video = video_new();
		const int colours[3] = { 200, 50, 180 };

		expect_planes(video, 16, 128, 128, "before a frame");
		video_frame(&writer, colours);
		CHECK(bink_video_decode(video, writer.data, size_of(&writer)) == 0, "the frame refused");
		expect_planes(video, 200, 50, 180, "filled");
		bink_video_delete(video);
		return 0;
	}
	CASE("video skip")
	{
		struct bink_video *video = video_new();
		const int first[3] = { 200, 50, 180 }, second[3] = { 30, 140, 90 };

		video_frame(&writer, first);
		CHECK(bink_video_decode(video, writer.data, size_of(&writer)) == 0, "the first frame refused");
		video_frame(&writer, NULL);
		CHECK(bink_video_decode(video, writer.data, size_of(&writer)) == 0, "the skipped frame refused");
		expect_planes(video, 200, 50, 180, "skipped");
		video_frame(&writer, second);
		CHECK(bink_video_decode(video, writer.data, size_of(&writer)) == 0, "the third frame refused");
		expect_planes(video, 30, 140, 90, "filled again");
		bink_video_delete(video);
		return 0;
	}
	CASE("video damaged")
	{
		struct bink_video *video = video_new();
		const int colours[3] = { 200, 50, 180 };

		video_frame(&writer, colours);
		CHECK(bink_video_decode(video, writer.data, size_of(&writer)) == 0, "the frame refused");
		/* (cut off in the luma's bundles) */
		CHECK(bink_video_decode(video, writer.data, 6) < 0, "a damaged frame taken");
		expect_planes(video, 200, 50, 180, "after a damaged frame");
		bink_video_delete(video);
		return 0;
	}
	CASE("audio dc")
	{
		struct bink_audio *audio = audio_new();
		unsigned long count, index;

		CHECK(audio && bink_audio_channels(audio) == 2, "no stereo decoder");
		memset(&writer, 0, sizeof(writer));
		put(&writer, AUDIO_BLOCK * 2, 32);
		audio_block(&writer, 64000, 0, 0);
		count = audio_decode(audio, &writer);
		CHECK(count == AUDIO_BLOCK, "%lu samples, not %d", count, AUDIO_BLOCK);
		for (index = 0; index < count; index++)
			expect_sample(index, 1000.0, "a level");
		bink_audio_delete(audio);
		return 0;
	}
	CASE("audio tone")
	{
		struct bink_audio *audio = audio_new();
		unsigned long count, index;

		memset(&writer, 0, sizeof(writer));
		put(&writer, AUDIO_BLOCK * 2, 32);
		/* the sine of 8 cycles over the transform: index 2 * 8 + 1 */
		audio_block(&writer, 0, 17, 32000);
		count = audio_decode(audio, &writer);
		CHECK(count == AUDIO_BLOCK, "%lu samples", count);
		for (index = 0; index < count; index++)
			expect_sample(index, 1000.0 * sin(2.0 * 3.14159265358979 * 8.0 * index / AUDIO_FRAME), "a sine");
		bink_audio_delete(audio);
		return 0;
	}
	CASE("audio overlap")
	{
		struct bink_audio *audio = audio_new();
		unsigned long count;

		/* two blocks in a packet: the second's first 256 samples go from the
		first's level to its own */
		memset(&writer, 0, sizeof(writer));
		put(&writer, AUDIO_BLOCK * 4, 32);
		audio_block(&writer, 64000, 0, 0);
		audio_block(&writer, 128000, 0, 0);
		count = audio_decode(audio, &writer);
		CHECK(count == AUDIO_BLOCK * 2, "%lu samples", count);
		expect_sample(AUDIO_BLOCK - 1, 1000.0, "the first block's last");
		expect_sample(AUDIO_BLOCK, 1000.0, "the crossfade's start");
		expect_sample(AUDIO_BLOCK + 128, 1500.0, "the crossfade's middle");
		expect_sample(AUDIO_BLOCK + 256, 2000.0, "past the crossfade");
		bink_audio_delete(audio);
		return 0;
	}
	fprintf(stderr, "no case named %s\n", case_name);
	return 2;
}
