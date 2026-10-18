/*
BINK_TABLES.C

Finds the tables of the Bink format in the player's own Xbox executable
(default.xbe of the Halo disc), which links RAD's Bink library and with it
the format's constants. They are read at run time from the player's copy:
no byte of the executable is in this source.

The executable is read as its headers describe it (a base address, and a
list of sections each with a name, and its place in the file). The library's
data is in two of them: ".rdata", which holds the matrices and the Huffman
trees and the block patterns, and "BINKDATA", which holds the audio's band
edges and the coefficient scan. Each table is at a fixed distance from the
start of its section. A table is taken only if

 - it lies inside its section,
 - its CRC-32 is that of the Rev 2 disc's (the CRC is of the table alone: it
   says which build of the library this is, and tells nothing of its bytes),
 - and it has the structure the format gives it: the code lengths of each
   Huffman tree fill the code space exactly and the codes are a prefix code,
   the patterns and the scan are orders of the 64 pixels of a block, the
   matrices grow with the quality level.

An executable that fails any of this (another build, a damaged file, not
Halo's) gives no tables and the movies are not played.
*/

#include "bink_decode.h"

#include <stdio.h>
#include <string.h>

enum
{
	XBE_BASE_ADDRESS = 0x104,
	XBE_SECTION_COUNT = 0x11c,
	XBE_SECTION_HEADERS = 0x120,
	XBE_SECTION_HEADER_SIZE = 56,
	XBE_MAXIMUM_SECTIONS = 64
};

enum table_index
{
	TABLE_PATTERNS,
	TABLE_INTRA_QUANTIZER,
	TABLE_INTER_QUANTIZER,
	TABLE_TREE_LENGTHS,
	TABLE_TREE_BITS,
	TABLE_FREQUENCIES,
	TABLE_RUN_LENGTHS,
	TABLE_SCAN,
	NUMBER_OF_TABLES
};

struct table_place
{
	const char *section;
	unsigned long offset;
	unsigned long size;
	unsigned long crc;
};

/* where each table is in the Rev 2 disc's executable, in the order of table_index */
static const struct table_place table_places[NUMBER_OF_TABLES] =
{
	{ ".rdata", 0x0898, 1024, 0xF9E3218AUL },
	{ ".rdata", 0x1258, 4096, 0xD53C4639UL },
	{ ".rdata", 0x3258, 4096, 0xC4F4DCD3UL },
	{ ".rdata", 0x5260, 256, 0xF2C3EC57UL },
	{ ".rdata", 0x5368, 256, 0xA2739DA1UL },
	{ "BINKDATA", 0x4014, 96, 0x62A6645BUL },
	{ "BINKDATA", 0x4140, 16, 0x7C59D03FUL },
	{ "BINKDATA", 0x4210, 64, 0x8FBDD9F2UL }
};

static unsigned long read_u32(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static int fail(char *error, int error_size, const char *message)
{
	if (error && error_size > 0)
	{
		snprintf(error, (size_t)error_size, "%s", message);
	}
	return 0;
}

static unsigned long crc32_of(const unsigned char *bytes, unsigned long size)
{
	unsigned long crc = 0xFFFFFFFFUL;
	unsigned long index;
	int bit;

	for (index = 0; index < size; index++)
	{
		crc ^= bytes[index];
		for (bit = 0; bit < 8; bit++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		}
	}
	return crc ^ 0xFFFFFFFFUL;
}

/* the table's bytes in the image, or NULL */
static const unsigned char *find_table(const unsigned char *image, unsigned long size, enum table_index index)
{
	const struct table_place *place = &table_places[index];
	unsigned long base, count, headers, section;

	if (size < 0x180 || memcmp(image, "XBEH", 4) != 0)
	{
		return NULL;
	}
	base = read_u32(image + XBE_BASE_ADDRESS);
	count = read_u32(image + XBE_SECTION_COUNT);
	headers = read_u32(image + XBE_SECTION_HEADERS);
	if (count == 0 || count > XBE_MAXIMUM_SECTIONS || headers < base || headers - base > size ||
		size - (headers - base) < count * XBE_SECTION_HEADER_SIZE)
	{
		return NULL;
	}
	for (section = 0; section < count; section++)
	{
		const unsigned char *header = image + (headers - base) + section * XBE_SECTION_HEADER_SIZE;
		unsigned long raw_address = read_u32(header + 12);
		unsigned long raw_size = read_u32(header + 16);
		unsigned long name_address = read_u32(header + 20);
		const char *name;
		unsigned long length;

		if (name_address < base || name_address - base >= size)
		{
			continue;
		}
		name = (const char *)image + (name_address - base);
		for (length = 0; length < size - (name_address - base) && name[length]; length++)
			;
		if (length != strlen(place->section) || memcmp(name, place->section, length) != 0)
		{
			continue;
		}
		if (raw_address > size || raw_size > size - raw_address || place->offset > raw_size ||
			place->size > raw_size - place->offset)
		{
			return NULL;
		}
		return image + raw_address + place->offset;
	}
	return NULL;
}

/* a Huffman tree of the format: lengths fill the code space exactly, and
no code begins another (codes are read least significant bit first) */
static int tree_is_prefix_code(const unsigned char *lengths, const unsigned char *bits)
{
	unsigned long space = 0;
	int symbol, other;

	for (symbol = 0; symbol < 16; symbol++)
	{
		if (lengths[symbol] < 1 || lengths[symbol] > 7 || (unsigned long)bits[symbol] >> lengths[symbol])
		{
			return 0;
		}
		space += 128UL >> lengths[symbol];
	}
	if (space != 128)
	{
		return 0;
	}
	for (symbol = 0; symbol < 16; symbol++)
	{
		for (other = 0; other < 16; other++)
		{
			int shorter = lengths[symbol] < lengths[other] ? lengths[symbol] : lengths[other];

			if (other != symbol && ((bits[symbol] ^ bits[other]) & ((1 << shorter) - 1)) == 0)
			{
				return 0;
			}
		}
	}
	return 1;
}

static int is_pixel_order(const unsigned char *order)
{
	unsigned char seen[64];
	int index;

	memset(seen, 0, sizeof(seen));
	for (index = 0; index < 64; index++)
	{
		if (order[index] > 63 || seen[order[index]])
		{
			return 0;
		}
		seen[order[index]] = 1;
	}
	return 1;
}

int bink_tables_from_xbe(const unsigned char *image, unsigned long size, struct bink_tables *tables,
	char *error, int error_size)
{
	const unsigned char *found[NUMBER_OF_TABLES];
	int index, level, entry;

	memset(tables, 0, sizeof(*tables));
	if (!image || size < 0x180 || memcmp(image, "XBEH", 4) != 0)
	{
		return fail(error, error_size, "not an Xbox executable");
	}
	for (index = 0; index < NUMBER_OF_TABLES; index++)
	{
		found[index] = find_table(image, size, (enum table_index)index);
		if (!found[index])
		{
			return fail(error, error_size, "the executable has no Bink tables where the Rev 2 disc's has them");
		}
		if (crc32_of(found[index], table_places[index].size) != table_places[index].crc)
		{
			return fail(error, error_size, "the executable's Bink library is not the Rev 2 disc's");
		}
	}

	memcpy(tables->patterns, found[TABLE_PATTERNS], sizeof(tables->patterns));
	memcpy(tables->tree_lengths, found[TABLE_TREE_LENGTHS], sizeof(tables->tree_lengths));
	memcpy(tables->tree_bits, found[TABLE_TREE_BITS], sizeof(tables->tree_bits));
	memcpy(tables->scan, found[TABLE_SCAN], sizeof(tables->scan));
	memcpy(tables->run_lengths, found[TABLE_RUN_LENGTHS], sizeof(tables->run_lengths));
	for (index = 0; index < 24; index++)
	{
		tables->critical_frequencies[index] = read_u32(found[TABLE_FREQUENCIES] + index * 4);
	}
	for (level = 0; level < 16; level++)
	{
		for (entry = 0; entry < 64; entry++)
		{
			tables->intra_quantizer[level][entry] =
				(int)read_u32(found[TABLE_INTRA_QUANTIZER] + (level * 64 + entry) * 4);
			tables->inter_quantizer[level][entry] =
				(int)read_u32(found[TABLE_INTER_QUANTIZER] + (level * 64 + entry) * 4);
		}
	}

	/* the structure the format gives them */
	if (!is_pixel_order(tables->scan))
	{
		return fail(error, error_size, "the Bink scan is not an order of the block's pixels");
	}
	for (index = 0; index < 16; index++)
	{
		if (!is_pixel_order(tables->patterns[index]))
		{
			return fail(error, error_size, "a Bink pattern is not an order of the block's pixels");
		}
		if (!tree_is_prefix_code(tables->tree_lengths[index], tables->tree_bits[index]))
		{
			return fail(error, error_size, "a Bink tree is not a prefix code");
		}
	}
	for (level = 0; level < 16; level++)
	{
		for (entry = 0; entry < 64; entry++)
		{
			int intra = tables->intra_quantizer[level][entry];
			int inter = tables->inter_quantizer[level][entry];

			if (intra <= 0 || intra > 0x1000000 || inter <= 0 || inter > 0x1000000 ||
				(level > 0 && (intra < tables->intra_quantizer[level - 1][entry] ||
				inter < tables->inter_quantizer[level - 1][entry])))
			{
				return fail(error, error_size, "a Bink quantiser does not grow with its level");
			}
		}
	}
	for (index = 1; index < 24; index++)
	{
		if (tables->critical_frequencies[index] <= tables->critical_frequencies[index - 1] ||
			tables->critical_frequencies[index] > 48000)
		{
			return fail(error, error_size, "the Bink band edges are not in order");
		}
	}
	return 1;
}
