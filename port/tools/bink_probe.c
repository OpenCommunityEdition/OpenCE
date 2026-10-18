/*
BINK_PROBE.C

Decodes a Bink movie with the port's decoder (port/linux/src/bink_decode.c)
and reports how it went, for checking the decoder and timing it. The
Bink tables are read from a Halo Xbox executable that the caller names: the
executable and the movies are the player's own and are never part of this
repository.

    probe <default.xbe> <movie.bik> [frames [dump_prefix [dump_every]]]

With a dump prefix, every dump_every'th frame (default 100) is written as a
PPM picture (prefix_<frame>.ppm), and the audio as prefix.wav (16-bit PCM).
Build (any host; the maths functions are the C library's here):

    cc -O2 -I port/linux/src -I port/include -o probe \
       port/tools/bink_probe.c port/linux/src/bink_decode.c port/linux/src/bink_tables.c -lm
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

static void put_u32(unsigned char *at, unsigned long value)
{
	at[0] = (unsigned char)value;
	at[1] = (unsigned char)(value >> 8);
	at[2] = (unsigned char)(value >> 16);
	at[3] = (unsigned char)(value >> 24);
}

int main(int argument_count, char **arguments)
{
	FILE *file, *xbe_file, *wave = NULL;
	unsigned char *xbe;
	long xbe_size, file_size;
	static struct bink_tables tables;
	struct bink_decoder *decoder;
	const struct bink_info *info;
	char error[256];
	unsigned long limit = 0xFFFFFFFFUL, frame = 0, dump_every = 100, audio_frames = 0;
	const char *prefix = NULL;
	unsigned char *picture = NULL;
	short *audio = NULL;
	double start, decode_time = 0, convert_time = 0, worst = 0;
	unsigned long worst_frame = 0;

	if (argument_count < 3)
	{
		fprintf(stderr, "usage: %s default.xbe movie.bik [frames [dump_prefix [dump_every]]]\n", arguments[0]);
		return 2;
	}
	if (argument_count > 3)
		limit = strtoul(arguments[3], NULL, 10);
	if (argument_count > 4)
		prefix = arguments[4];
	if (argument_count > 5)
		dump_every = strtoul(arguments[5], NULL, 10);

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
	if (fread(xbe, 1, (size_t)xbe_size, xbe_file) != (size_t)xbe_size)
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
	printf("%lux%lu, %lu frames, %lu/%lu fps, revision %c, audio %lu Hz x %d\n", info->width, info->height,
		info->frames, info->frame_rate, info->frame_rate_divisor, info->revision, info->sample_rate,
		info->channels);
	picture = (unsigned char *)malloc(info->width * info->height * 4);
	audio = (short *)malloc(sizeof(short) * 65536 * 2);
	if (prefix && info->sample_rate)
	{
		char name[512];
		unsigned char header[44];

		snprintf(name, sizeof(name), "%s.wav", prefix);
		wave = fopen(name, "wb");
		memset(header, 0, sizeof(header));
		fwrite(header, 1, sizeof(header), wave);
	}

	while (frame < limit && frame < info->frames)
	{
		double began = now_milliseconds(), elapsed;

		if (!bink_decode_frame(decoder, error, sizeof(error)))
		{
			printf("frame %lu: %s\n", frame, error);
			break;
		}
		elapsed = now_milliseconds() - began;
		decode_time += elapsed;
		if (elapsed > worst)
		{
			worst = elapsed;
			worst_frame = frame;
		}
		start = now_milliseconds();
		bink_copy_to_buffer(decoder, picture, (long)info->width * 4, info->height, 0, 0);
		convert_time += now_milliseconds() - start;
		while (bink_audio_available(decoder))
		{
			unsigned long taken = bink_take_audio(decoder, audio, 65536);

			audio_frames += taken;
			if (wave)
				fwrite(audio, sizeof(short) * (size_t)info->channels, taken, wave);
		}
		if (prefix && dump_every && frame % dump_every == 0)
		{
			char name[512];
			FILE *out;
			unsigned long pixel;

			snprintf(name, sizeof(name), "%s_%05lu.ppm", prefix, frame);
			out = fopen(name, "wb");
			fprintf(out, "P6\n%lu %lu\n255\n", info->width, info->height);
			for (pixel = 0; pixel < info->width * info->height; pixel++)
			{
				unsigned char rgb[3];

				rgb[0] = picture[pixel * 4 + 2];
				rgb[1] = picture[pixel * 4 + 1];
				rgb[2] = picture[pixel * 4];
				fwrite(rgb, 1, 3, out);
			}
			fclose(out);
		}
		frame++;
	}
	printf("decoded %lu frames; video %.3f ms/frame (worst %.2f ms, frame %lu), convert %.3f ms/frame\n", frame,
		frame ? decode_time / (double)frame : 0.0, worst, worst_frame, frame ? convert_time / (double)frame : 0.0);
	printf("audio: %lu frames = %.2f s (video %.2f s)\n", audio_frames,
		info->sample_rate ? (double)audio_frames / (double)info->sample_rate : 0.0,
		(double)frame * (double)info->frame_rate_divisor / (double)info->frame_rate);
	if (wave)
	{
		unsigned char header[44];
		unsigned long bytes = audio_frames * (unsigned long)info->channels * 2;

		memcpy(header, "RIFF", 4);
		put_u32(header + 4, 36 + bytes);
		memcpy(header + 8, "WAVEfmt ", 8);
		put_u32(header + 16, 16);
		header[20] = 1;
		header[21] = 0;
		header[22] = (unsigned char)info->channels;
		header[23] = 0;
		put_u32(header + 24, info->sample_rate);
		put_u32(header + 28, info->sample_rate * (unsigned long)info->channels * 2);
		header[32] = (unsigned char)(info->channels * 2);
		header[33] = 0;
		header[34] = 16;
		header[35] = 0;
		memcpy(header + 36, "data", 4);
		put_u32(header + 40, bytes);
		fseek(wave, 0, SEEK_SET);
		fwrite(header, 1, sizeof(header), wave);
		fclose(wave);
	}
	bink_close(decoder);
	fclose(file);
	return 0;
}
