/*
TEXTURE_DECODE_ROWS.C

decode_level (xbox_textures.c) with its row-at-a-time paths for palettized
and 32-bit textures, against itself with those paths off (each texel through
convert_texel, as before): the same texels for every uncompressed format,
swizzled and linear, 2D and volume, every mip level, with and without a
palette. test_texture_decode_rows.py takes the code from the sources.
*/

#include "harness.h"

typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned long D3DCOLOR;
#include "structures.inc"
#include "under_test.inc"

static unsigned long random_state = 31337;

static unsigned char random_byte(void)
{
	random_state = random_state * 1103515245 + 12345;
	return (unsigned char)(random_state >> 16);
}

static void case_identical(void)
{
	static unsigned char source[64 * 64 * 4 * 4 * 2];
	static unsigned long rows[64 * 64 * 4], texels[64 * 64 * 4];
	static D3DCOLOR palette[256];
	static unsigned long const sizes[][3] = {
		{ 1, 1, 1 }, { 2, 1, 1 }, { 1, 4, 1 }, { 4, 4, 1 }, { 8, 2, 1 }, { 16, 16, 1 }, { 64, 8, 1 }, { 32, 64, 1 },
		{ 4, 4, 4 }, { 8, 4, 2 }, { 16, 16, 4 },
	};
	unsigned long format, size, level, index, cases = 0;
	int linear, with_palette;

	for (index = 0; index < sizeof(source); index++)
		source[index] = random_byte();
	for (index = 0; index < 256; index++)
		palette[index] = ((unsigned long)random_byte() << 24) | ((unsigned long)random_byte() << 16) |
			((unsigned long)random_byte() << 8) | random_byte();
	/* (the formats past the table's are its unknown one's) */
	for (format = 0; format < 0x43; format++)
	{
		struct format_information information = format_information(format);

		if (kind_compressed(information.kind))
			continue;
		for (size = 0; size < sizeof(sizes) / sizeof(sizes[0]); size++)
		{
			for (linear = 0; linear < 2; linear++)
			{
				struct xgpu_texture_description description;

				if (linear && sizes[size][2] > 1)
					continue;
				memset(&description, 0, sizeof(description));
				description.format = format;
				description.width = sizes[size][0];
				description.height = sizes[size][1];
				description.depth = sizes[size][2];
				description.levels = floor_log2(sizes[size][0] > sizes[size][1] ? sizes[size][0] : sizes[size][1]) + 1;
				description.linear = linear;
				description.pitch = linear ? (sizes[size][0] * information.bytes + 3) & ~3UL : 0;
				for (level = 0; level < (linear ? 1 : description.levels); level++)
				{
					unsigned long count = level_dimension(description.width, level) *
						level_dimension(description.height, level) * level_dimension(description.depth, level);

					for (with_palette = 0; with_palette < 2; with_palette++)
					{
						D3DCOLOR const *used = with_palette ? palette : NULL;

						memset(rows, 0xA5, count * sizeof(rows[0]));
						memset(texels, 0x5A, count * sizeof(texels[0]));
						CHECK(decode_level(&description, level, source, used, rows), "decoded");
						CHECK(decode_level_reference(&description, level, source, used, texels), "decoded");
						for (index = 0; index < count; index++)
						{
							CHECK(rows[index] == texels[index],
								"format %02lx %lux%lux%lu %s level %lu %s palette: texel %lu %08lx, expected %08lx",
								format, description.width, description.height, description.depth,
								linear ? "linear" : "swizzled", level, with_palette ? "with" : "no", index,
								rows[index], texels[index]);
						}
						cases++;
					}
				}
			}
		}
	}
	CHECK(cases > 500, "only %lu cases", cases);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("identical") case_identical();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
