/*
BMP_FILE_REPORT.C

Reports whether the native builds read a picture file
(port/linux/game/bmp_files.c), for the tests (tools/test_bmp_files.py).

	bmp_file_report [--fit WIDTH HEIGHT SHAPE_WIDTH SHAPE_HEIGHT OUTPUT] FILE

The file gets a block of "key: value" lines. With --fit, a picture it reads
is fitted to WIDTH by HEIGHT pixels of the shape SHAPE_WIDTH by SHAPE_HEIGHT
(bmp_file_fit), and the pixels are written to OUTPUT as 32-bit little-endian
0xFFRRGGBB values, the top row first. The exit status is 0 when the file was
read.
*/

/* ---------- headers */

#include "bmp_files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* larger than any picture bmp_file_open accepts: 8192 by 8192 at 32 bits */
#define MAXIMUM_FILE_BYTES 0x10001000UL
/* the largest picture --fit makes */
#define MAXIMUM_FIT_DIMENSION 4096UL

/* ---------- private code */

/* reads the file at `path` into memory, or returns NULL */
static uint8_t *file_read(
	char const *path,
	uint32_t *size)
{
	FILE *stream = fopen(path, "rb");
	uint8_t *bytes = NULL;
	long length;

	if (!stream)
	{
		return NULL;
	}
	if (fseek(stream, 0, SEEK_END) == 0 &&
		(length = ftell(stream)) >= 0 &&
		(unsigned long)length <= MAXIMUM_FILE_BYTES &&
		fseek(stream, 0, SEEK_SET) == 0)
	{
		/* one more byte, so that an empty file is not a failed allocation */
		bytes = malloc((size_t)length + 1);
		if (bytes && fread(bytes, 1, (size_t)length, stream) != (size_t)length)
		{
			free(bytes);
			bytes = NULL;
		}
		*size = (uint32_t)length;
	}
	fclose(stream);

	return bytes;
}

static int fit_write(
	uint8_t const *file,
	struct bmp_file_picture const *picture,
	unsigned long const dimensions[4],
	char const *output_path)
{
	uint32_t width = (uint32_t)dimensions[0];
	uint32_t height = (uint32_t)dimensions[1];
	uint32_t *pixels = malloc((size_t)width * height * sizeof(uint32_t));
	FILE *output;
	size_t index;
	int written = 1;

	if (!pixels)
	{
		return 0;
	}
	bmp_file_fit(file, picture, (uint32_t)dimensions[2], (uint32_t)dimensions[3], width, height, pixels);
	output = fopen(output_path, "wb");
	if (!output)
	{
		free(pixels);
		return 0;
	}
	for (index = 0; written && index < (size_t)width * height; index++)
	{
		uint8_t bytes[4];

		bytes[0] = (uint8_t)pixels[index];
		bytes[1] = (uint8_t)(pixels[index] >> 8);
		bytes[2] = (uint8_t)(pixels[index] >> 16);
		bytes[3] = (uint8_t)(pixels[index] >> 24);
		written = fwrite(bytes, 1, sizeof(bytes), output) == sizeof(bytes);
	}
	written = fclose(output) == 0 && written;
	free(pixels);

	return written;
}

static int usage(
	void)
{
	fprintf(stderr, "usage: bmp_file_report [--fit WIDTH HEIGHT SHAPE_WIDTH SHAPE_HEIGHT OUTPUT] FILE\n");

	return 2;
}

/* ---------- public code */

int main(
	int argc,
	char **argv)
{
	unsigned long dimensions[4];
	char const *output_path = NULL;
	char const *path;
	struct bmp_file_picture picture;
	enum bmp_file_status status;
	uint8_t *file;
	uint32_t file_size = 0;
	int argument = 1;
	int index;
	int result = 1;

	if (argc > argument && !strcmp(argv[argument], "--fit"))
	{
		if (argc != argument + 7)
		{
			return usage();
		}
		for (index = 0; index < 4; index++)
		{
			char *end;

			dimensions[index] = strtoul(argv[argument + 1 + index], &end, 10);
			if (*end || (index < 2 && (dimensions[index] == 0 || dimensions[index] > MAXIMUM_FIT_DIMENSION)))
			{
				return usage();
			}
		}
		output_path = argv[argument + 5];
		argument += 6;
	}
	if (argc != argument + 1)
	{
		return usage();
	}
	path = argv[argument];

	printf("file: %s\n", path);
	file = file_read(path, &file_size);
	if (!file)
	{
		printf("status: the file could not be read\n");
		return 1;
	}
	status = bmp_file_open(file, file_size, &picture);
	printf("status: %s\n", bmp_file_status_describe(status));
	if (status == _bmp_file_status_ok)
	{
		printf("width: %lu\n", (unsigned long)picture.width);
		printf("height: %lu\n", (unsigned long)picture.height);
		printf("bits_per_pixel: %u\n", (unsigned)picture.bits_per_pixel);
		printf("top_down: %d\n", picture.top_down);
		result = 0;
		if (output_path && !fit_write(file, &picture, dimensions, output_path))
		{
			printf("fit: the pixels could not be written\n");
			result = 1;
		}
	}
	free(file);

	return result;
}
