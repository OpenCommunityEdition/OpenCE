/* The pad, the authored respawn meshes, and the timer recordings are linked
into the game. tools/hardcore_assets.py writes the compressed words. A map
file is still used when this copy has no matching piece. */

#include "cseries.h"
#include "memory/data_compress.h"
#include "memory/zlib/zlib.h"
#include "hardcore_assets.h"

#include <stdlib.h>

#define HARDCORE_ASSETS_MAGIC 0x4B505645UL
#define HARDCORE_ASSETS_LIMIT (32UL * 1024UL * 1024UL)

extern const unsigned long hardcore_assets_compressed_bytes;
extern const unsigned int hardcore_assets_words[];

static unsigned char *hardcore_assets_pack;
static unsigned long hardcore_assets_pack_bytes;
static int hardcore_assets_ready;
static int hardcore_assets_failed;

static unsigned long hardcore_assets_u32(unsigned char const *bytes)
{
	return (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
}

static int hardcore_assets_same(char const *left, char const *right)
{
	unsigned long index;

	for (index = 0; left[index] || right[index]; index++)
	{
		char a = left[index];
		char b = right[index];

		if (a >= 'A' && a <= 'Z')
			a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z')
			b = (char)(b - 'A' + 'a');
		if (a != b)
			return 0;
	}
	return 1;
}

static int hardcore_assets_open(void)
{
	unsigned char const *bytes;
	unsigned long compressed;
	unsigned long size;

	if (hardcore_assets_ready)
		return 1;
	if (hardcore_assets_failed)
		return 0;
	compressed = hardcore_assets_compressed_bytes;
	bytes = (unsigned char const *)hardcore_assets_words;
	size = data_decompressed_size(bytes, compressed);
	/* The game's decompress hands zlib a length that runs past the stream.
	   The call below stops at the stream, so the words are not read past. */
	if (compressed < 16 || size < 12 || size > HARDCORE_ASSETS_LIMIT)
		goto fail;
	hardcore_assets_pack = (unsigned char *)malloc(size);
	if (!hardcore_assets_pack)
		goto fail;
	hardcore_assets_pack_bytes = size;
	if (uncompress(hardcore_assets_pack, &hardcore_assets_pack_bytes,
			(Bytef const *)bytes + 4, compressed - 4) != Z_OK ||
		hardcore_assets_pack_bytes != size ||
		hardcore_assets_u32(hardcore_assets_pack) != HARDCORE_ASSETS_MAGIC ||
		hardcore_assets_u32(hardcore_assets_pack + 4) != 1)
		goto fail;
	hardcore_assets_ready = 1;
	return 1;

fail:
	if (hardcore_assets_pack)
		free(hardcore_assets_pack);
	hardcore_assets_pack = NULL;
	hardcore_assets_failed = 1;
	return 0;
}

int hardcore_assets_find(unsigned kind, char const *name,
	unsigned char const **bytes, unsigned long *size)
{
	unsigned long count;
	unsigned long index;
	unsigned long cursor;

	if (!name || !bytes || !size || !hardcore_assets_open())
		return 0;
	count = hardcore_assets_u32(hardcore_assets_pack + 8);
	cursor = 12;
	if (count > 256)
		return 0;
	for (index = 0; index < count; index++)
	{
		unsigned long entry_kind;
		unsigned long nbytes;
		char entry_name[33];
		unsigned long name_index;

		if (cursor > hardcore_assets_pack_bytes || hardcore_assets_pack_bytes - cursor < 40)
			return 0;
		entry_kind = hardcore_assets_u32(hardcore_assets_pack + cursor);
		for (name_index = 0; name_index < 32; name_index++)
			entry_name[name_index] = (char)hardcore_assets_pack[cursor + 4 + name_index];
		entry_name[32] = 0;
		nbytes = hardcore_assets_u32(hardcore_assets_pack + cursor + 36);
		cursor += 40;
		if (nbytes > hardcore_assets_pack_bytes - cursor)
			return 0;
		if (entry_kind == kind && hardcore_assets_same(entry_name, name))
		{
			*bytes = hardcore_assets_pack + cursor;
			*size = nbytes;
			return 1;
		}
		cursor += nbytes;
	}
	return 0;
}
