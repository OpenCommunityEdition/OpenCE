/*
BINK_BENCH.C

Times the port's Bink decoder (port/linux/src/bink_decode.c) on a movie, for
machines the game must play it on (the PowerPC Macs: build it with the compiler
of the Mac's toolchain and run it there). It decodes the movie's frames in order,
converts each to pixels, takes the audio, and prints the milliseconds a
frame costs, a verdict against the movie's frame time, and checksums of the
pixels and the audio, which are the same on every machine (little or big
endian) that decodes correctly.

    bench <default.xbe> <movie.bik> [frames]    (0 or none: the whole movie)

The Bink tables are read from the player's own executable and the movie is the
player's own; nothing of either is kept.
*/

#include "bink_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static int read_file_proc(void *context, unsigned long offset, void *buffer, unsigned long size)
{
	FILE *file = (FILE *)context;

	return fseek(file, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
}

static double now_milliseconds(void)
{
	struct timeval time;

	gettimeofday(&time, NULL);
	return (double)time.tv_sec * 1000.0 + (double)time.tv_usec / 1000.0;
}

static unsigned int hash_bytes(unsigned int hash, const unsigned char *bytes, unsigned long count)
{
	unsigned long index;

	for (index = 0; index < count; index++)
	{
		hash = (hash ^ bytes[index]) * 16777619U;
	}
	return hash;
}

static int compare_doubles(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;

	return x < y ? -1 : x > y ? 1 : 0;
}

int main(int argument_count, char **arguments)
{
	FILE *xbe_file, *file;
	unsigned char *xbe, *picture;
	long xbe_size, file_size;
	static struct bink_tables tables;
	struct bink_decoder *decoder;
	const struct bink_info *info;
	char error[256];
	unsigned long limit = 0xFFFFFFFFUL, frame = 0;
	unsigned int pixel_hash = 2166136261U, audio_hash = 2166136261U;
	unsigned long audio_frames = 0;
	double *decode_times, *convert_times;
	double decode_total = 0, convert_total = 0, budget, p95;
	short *audio;

	if (argument_count < 3)
	{
		fprintf(stderr, "usage: %s default.xbe movie.bik [frames]\n", arguments[0]);
		return 2;
	}
	if (argument_count > 3)
		limit = strtoul(arguments[3], NULL, 10);
	xbe_file = fopen(arguments[1], "rb");
	if (!xbe_file)
	{
		perror(arguments[1]);
		return 1;
	}
	fseek(xbe_file, 0, SEEK_END);
	xbe_size = ftell(xbe_file);
	fseek(xbe_file, 0, SEEK_SET);
	xbe = (unsigned char *)malloc((size_t)xbe_size);
	if (!xbe || fread(xbe, 1, (size_t)xbe_size, xbe_file) != (size_t)xbe_size)
		return 1;
	fclose(xbe_file);
	if (!bink_tables_from_xbe(xbe, (unsigned long)xbe_size, &tables, error, sizeof(error)))
	{
		fprintf(stderr, "tables: %s\n", error);
		return 1;
	}
	free(xbe);
	file = fopen(arguments[2], "rb");
	if (!file)
	{
		perror(arguments[2]);
		return 1;
	}
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	decoder = bink_open(&tables, read_file_proc, file, (unsigned long)file_size, error, sizeof(error));
	if (!decoder)
	{
		fprintf(stderr, "open: %s\n", error);
		return 1;
	}
	info = bink_get_info(decoder);
	if (limit == 0 || limit > info->frames)
		limit = info->frames;
	budget = 1000.0 * (double)info->frame_rate_divisor / (double)info->frame_rate;
	picture = (unsigned char *)malloc(info->width * info->height * 4);
	audio = (short *)malloc(sizeof(short) * 65536 * 2);
	decode_times = (double *)malloc(sizeof(double) * limit);
	convert_times = (double *)malloc(sizeof(double) * limit);
	if (!picture || !audio || !decode_times || !convert_times)
		return 1;
	printf("%lux%lu, %lu of %lu frames, %.2f fps (%.2f ms a frame), audio %lu Hz x %d\n", info->width,
		info->height, limit, info->frames, (double)info->frame_rate / (double)info->frame_rate_divisor, budget,
		info->sample_rate, info->channels);
	while (frame < limit)
	{
		double began = now_milliseconds(), decoded, converted;

		if (!bink_decode_frame(decoder, error, sizeof(error)))
		{
			printf("frame %lu: %s\n", frame, error);
			return 1;
		}
		decoded = now_milliseconds();
		bink_copy_to_buffer(decoder, picture, (long)info->width * 4, info->height, 0, 0);
		converted = now_milliseconds();
		decode_times[frame] = decoded - began;
		convert_times[frame] = converted - decoded;
		decode_total += decoded - began;
		convert_total += converted - decoded;
		pixel_hash = hash_bytes(pixel_hash, picture, info->width * info->height * 4);
		while (bink_audio_available(decoder))
		{
			unsigned long taken = bink_take_audio(decoder, audio, 65536);
			unsigned long index;

			audio_frames += taken;
			/* the samples as little endian bytes, so the checksum is the same on every machine */
			for (index = 0; index < taken * (unsigned long)info->channels; index++)
			{
				unsigned char bytes[2];

				bytes[0] = (unsigned char)(audio[index] & 0xFF);
				bytes[1] = (unsigned char)((audio[index] >> 8) & 0xFF);
				audio_hash = hash_bytes(audio_hash, bytes, 2);
			}
		}
		frame++;
	}
	qsort(decode_times, limit, sizeof(double), compare_doubles);
	p95 = decode_times[(limit * 95) / 100];
	printf("decode  %7.3f ms a frame (median %.3f, 95%% under %.3f, worst %.3f)\n", decode_total / (double)limit,
		decode_times[limit / 2], p95, decode_times[limit - 1]);
	printf("convert %7.3f ms a frame (the picture's pixels)\n", convert_total / (double)limit);
	printf("total   %7.3f ms a frame against %.3f ms: %s (%.0f%% of a core)\n",
		(decode_total + convert_total) / (double)limit, budget,
		(decode_total + convert_total) / (double)limit <= budget ? "keeps up" : "TOO SLOW",
		100.0 * (decode_total + convert_total) / ((double)limit * budget));
	printf("worst frame decode+convert: %.3f ms; frames over the budget: ", decode_times[limit - 1]);
	{
		unsigned long over = 0, index;

		/* (decode times are sorted: the count of those that alone pass the budget) */
		for (index = 0; index < limit; index++)
			if (decode_times[index] + convert_total / (double)limit > budget)
				over++;
		printf("%lu of %lu\n", over, limit);
	}
	printf("checksums: pixels %08x, audio %08x (%lu audio frames)\n", pixel_hash, audio_hash, audio_frames);
	bink_close(decoder);
	fclose(file);
	return 0;
}
