/*
BINK_FUZZ.C

Feeds damaged copies of a Bink movie to the port's decoder
(port/linux/src/bink_decode.c): bytes flipped anywhere, damaged headers and
frame indexes, and truncated files. The decoder must fail cleanly or play
on; the check is that this program ends (build it with the address and
undefined behaviour sanitizers, tools/test_bink.py does) and the decoder
never reads or writes out of bounds. The copies are made in memory; the
movie and the executable the tables are read from are the player's own,
named on the command line and never kept.

    fuzz <default.xbe> <movie.bik> [iterations [seed [frames per iteration]]]

Build: cc -O1 -g -fsanitize=address,undefined -I port/linux/src -o fuzz \
    port/tools/bink_fuzz.c port/linux/src/bink_decode.c port/linux/src/bink_tables.c -lm
*/

#include "bink_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct memory_file
{
	const unsigned char *data;
	unsigned long size;
};

static int memory_read(void *context, unsigned long offset, void *buffer, unsigned long size)
{
	const struct memory_file *file = (const struct memory_file *)context;

	if (offset > file->size || size > file->size - offset)
		return 0;
	memcpy(buffer, file->data + offset, size);
	return 1;
}

static unsigned long long random_state;

static unsigned long random_next(void)
{
	random_state = random_state * 6364136223846793005ULL + 1442695040888963407ULL;
	return (unsigned long)(random_state >> 33);
}

static unsigned char *read_whole(const char *path, unsigned long *size)
{
	FILE *file = fopen(path, "rb");
	unsigned char *data;
	long length;

	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = (unsigned char *)malloc((size_t)length);
	if (data && fread(data, 1, (size_t)length, file) != (size_t)length)
	{
		free(data);
		data = NULL;
	}
	fclose(file);
	*size = (unsigned long)length;
	return data;
}

int main(int argument_count, char **arguments)
{
	static struct bink_tables tables;
	unsigned char *xbe, *movie, *copy;
	unsigned long xbe_size, movie_size, iteration, iterations = 200, frames_limit = 40;
	unsigned long opened = 0, played = 0, frames_total = 0;
	char error[128];

	if (argument_count < 3)
	{
		fprintf(stderr, "usage: %s default.xbe movie.bik [iterations [seed [frames]]]\n", arguments[0]);
		return 2;
	}
	if (argument_count > 3)
		iterations = strtoul(arguments[3], NULL, 10);
	random_state = argument_count > 4 ? strtoul(arguments[4], NULL, 10) : 1;
	if (argument_count > 5)
		frames_limit = strtoul(arguments[5], NULL, 10);
	xbe = read_whole(arguments[1], &xbe_size);
	movie = read_whole(arguments[2], &movie_size);
	if (!xbe || !movie || !bink_tables_from_xbe(xbe, xbe_size, &tables, error, sizeof(error)))
	{
		fprintf(stderr, "cannot start: %s\n", xbe && movie ? error : "unreadable input");
		return 1;
	}
	copy = (unsigned char *)malloc(movie_size);
	for (iteration = 0; iteration < iterations; iteration++)
	{
		struct memory_file file;
		struct bink_decoder *decoder;
		unsigned long kind = random_next() % 6, count, index, frame;
		unsigned long length = movie_size;

		memcpy(copy, movie, movie_size);
		switch (kind)
		{
		case 0: /* bytes flipped anywhere */
			for (count = 1 + random_next() % 64; count > 0; count--)
				copy[random_next() % movie_size] ^= (unsigned char)(1 << (random_next() % 8));
			break;
		case 1: /* the header and the start of the frame index */
			for (count = 1 + random_next() % 8; count > 0; count--)
				copy[random_next() % 0x200] = (unsigned char)random_next();
			break;
		case 2: /* truncated */
			length = random_next() % movie_size;
			break;
		case 3: /* a run of bytes replaced */
			index = random_next() % movie_size;
			for (count = random_next() % 4096; count > 0 && index < movie_size; count--, index++)
				copy[index] = (unsigned char)random_next();
			break;
		case 4: /* the first frames' data, damaged in many places */
			for (count = 1 + random_next() % 256; count > 0; count--)
				copy[0x4000 + random_next() % 0x40000] = (unsigned char)random_next();
			break;
		default: /* a few bytes of the first frames set to extremes */
			for (count = 1 + random_next() % 16; count > 0; count--)
				copy[0x4000 + random_next() % 0x10000] = (random_next() & 1) ? 0xFF : 0x00;
			break;
		}
		file.data = copy;
		file.size = length;
		decoder = bink_open(&tables, memory_read, &file, length, error, sizeof(error));
		if (!decoder)
			continue;
		opened++;
		for (frame = 0; frame < frames_limit; frame++)
		{
			short samples[2048];

			if (!bink_decode_frame(decoder, error, sizeof(error)))
				break;
			frames_total++;
			bink_take_audio(decoder, samples, 1024);
			if ((frame & 7) == 0)
			{
				/* the picture is read too, for the pixel conversion's bounds */
				static unsigned char picture[4096 * 4 * 8];

				bink_copy_to_buffer(decoder, picture, 4096 * 4, 8, 0, 0);
			}
		}
		if (frame == frames_limit)
			played++;
		bink_close(decoder);
	}
	printf("%lu damaged copies: %lu opened, %lu played to the limit, %lu frames decoded\n", iterations, opened,
		played, frames_total);
	free(copy);
	free(movie);
	free(xbe);
	return 0;
}
