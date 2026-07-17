/*
LRAR_CACHE.C

symbols in this file:
0010BFA0 0010:
	_code_0010bfa0 (0000)
0010BFB0 0010:
	_code_0010bfb0 (0000)
0010BFC0 0020:
	_code_0010bfc0 (0000)
0010BFE0 0060:
	_code_0010bfe0 (0000)
0010C040 0050:
	_code_0010c040 (0000)
0010C090 01b0:
	_lrar_new (0000)
0010C240 0040:
	_lrar_dispose (0000)
0010C280 0050:
	_code_0010c280 (0000)
0010C2D0 0140:
	_lrar_flush (0000)
0010C410 0310:
	_lrar_allocate (0000)
0010C720 0060:
	_lrar_block_address (0000)
0010C780 0070:
	_lrar_deallocate (0000)
0027DD04 0032:
	??_C@_0DC@EAHEHBJC@lrar?5cache?5?$CFs?5?$EA?$CFp?5block?5?$EA?$CFp?5appe@ (0000)
0027DD38 0023:
	??_C@_0CD@MFNOCGKM@c?3?2halo?2SOURCE?2memory?2lrar_cache@ (0000)
0027DD5C 0028:
	??_C@_0CI@KPCLJMCF@lrar?5cache?5?$CFs?5?$EA?$CFp?5appears?5to?5be?5@ (0000)
0027DD84 000e:
	??_C@_0O@ILDLNLNB@block_count?$DO0?$AA@ (0000)
0027DD94 0026:
	??_C@_0CG@GPBJKAFO@boundary_bit?$DN?$DNNONE?5?$HM?$HM?5boundary_b@ (0000)
0027DDBC 0011:
	??_C@_0BB@NAOBPHNE@alignment_bit?$DO?$DN0?$AA@ (0000)
0027DDD0 0020:
	??_C@_0CA@IIPMMBBP@minimum_address?$DMmaximum_address?$AA@ (0000)
0027DDF0 0031:
	??_C@_0DB@KHEPPLFA@block_index?$DO?$DN0?5?$CG?$CG?5block_index?$DMca@ (0000)
0027DE28 0079:
	??_C@_0HJ@GLEHLHIE@adjusted_new_block_address?$DO?$DNtest@ (0000)
0027DEA8 006e:
	??_C@_0GO@DFHBADBA@adjusted_new_block_address?$DO?$DNcach@ (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "lrar_cache.h"

/* ---------- constants */

enum
{
	_lrar_cache_signature = 0x6C726172,
	_lrar_block_signature = 0x52626C6B,
};

/* ---------- macros */

/* ---------- structures */

struct lrar_cache_block
{
	void *user_data;
	unsigned long signature;
	unsigned long address;
	long size;
};

struct lrar_cache
{
	char name[32];
	short alignment_bit;
	short boundary_bit;
	unsigned long minimum_address;
	unsigned long maximum_address;
	long total_size;
	struct lrar_cache_block *blocks;
	short first_block_index;
	short last_block_index;
	short block_count;
	short __pad3A;
	lrar_lock_proc lock_proc;
	lrar_unlock_proc unlock_proc;
	unsigned long signature;
};

typedef char lrar_cache_size_assert[sizeof(struct lrar_cache) == 0x48 ? 1 : -1];
typedef char lrar_cache_block_size_assert[sizeof(struct lrar_cache_block) == 0x10 ? 1 : -1];

/* ---------- prototypes */

void code_0010bfa0(
	short *reference,
	short value);
void code_0010bfb0(
	short *reference);
static void code_0010bfe0(
	struct lrar_cache *cache,
	struct lrar_cache_block *block);
static void code_0010c040(
	struct lrar_cache *cache);

/* ---------- globals */

/* ---------- public code */

void code_0010bfa0(
	short *reference,
	short value)
{
	*reference = value;

	return;
}

void code_0010bfb0(
	short *reference)
{
	*reference = NONE;

	return;
}

struct lrar_cache *lrar_new(
	char const *name,
	unsigned long minimum_address,
	unsigned long maximum_address,
	short block_count,
	short alignment_bit,
	short boundary_bit,
	lrar_lock_proc lock_proc,
	lrar_unlock_proc unlock_proc)
{
	struct lrar_cache *cache;
	struct lrar_cache_block *blocks;
	long blocks_size;
	unsigned long alignment_mask;

	cache = debug_malloc(
		sizeof(*cache),
		FALSE,
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x56);

	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x58,
		minimum_address<maximum_address);

	alignment_mask = FLAG(alignment_bit)-1;
	if (minimum_address&alignment_mask)
	{
		minimum_address = (minimum_address|alignment_mask)+1;
	}

	if (!lock_proc || !unlock_proc)
	{
		lock_proc = code_0010bfa0;
		unlock_proc = code_0010bfb0;
	}

	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x66,
		alignment_bit>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x67,
		boundary_bit==NONE || boundary_bit>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x68,
		block_count>0);

	if (cache)
	{
		blocks_size = block_count*sizeof(*blocks);
		blocks = debug_malloc(
			blocks_size,
			FALSE,
			"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
			0x6C);
		if (blocks)
		{
			csmemset(cache, 0, sizeof(*cache));
			csmemset(blocks, 0, blocks_size);
			csstrncpy(cache->name, name, 31);
			cache->name[31] = 0;
			cache->minimum_address = minimum_address;
			cache->maximum_address = maximum_address;
			cache->total_size = maximum_address-minimum_address;
			cache->alignment_bit = alignment_bit;
			cache->boundary_bit = boundary_bit;
			cache->first_block_index = NONE;
			cache->last_block_index = NONE;
			cache->blocks = blocks;
			cache->block_count = block_count;
			cache->unlock_proc = unlock_proc;
			cache->lock_proc = lock_proc;
			cache->signature = _lrar_cache_signature;
			code_0010c040(cache);
		}
		else
		{
			debug_free(
				cache,
				"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
				0x8A);
			cache = NULL;
		}
	}

	return cache;
}

void lrar_dispose(
	struct lrar_cache *cache)
{
	code_0010c040(cache);
	debug_free(
		cache->blocks,
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x98);
	debug_free(
		cache,
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x99);

	return;
}

unsigned long lrar_block_address(
	struct lrar_cache *cache,
	short block_index)
{
	struct lrar_cache_block *block;

	code_0010c040(cache);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x16E,
		block_index>=0 && block_index<cache->block_count);
	block = &cache->blocks[block_index];
	code_0010bfe0(cache, block);

	return block->address;
}

void lrar_deallocate(
	struct lrar_cache *cache,
	short block_index)
{
	struct lrar_cache_block *block;

	code_0010c040(cache);
	code_0010c040(cache);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x16E,
		block_index>=0 && block_index<cache->block_count);
	block = &cache->blocks[block_index];
	code_0010bfe0(cache, block);
	if (block->user_data)
	{
		cache->unlock_proc(block->user_data);
		block->user_data = NULL;
	}

	return;
}

/* ---------- private code */

static void code_0010bfe0(
	struct lrar_cache *cache,
	struct lrar_cache_block *block)
{
	boolean valid =
		block->signature == _lrar_block_signature &&
		block->size >= 0 &&
		block->size < cache->total_size &&
		block->address >= cache->minimum_address &&
		block->address + block->size <= cache->maximum_address;

	match_vassert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x186,
		valid,
		csprintf(
			temporary,
			"lrar cache %s @%p block @%p appears to be corrupt",
			cache,
			cache,
			block));

	return;
}

static void code_0010c040(
	struct lrar_cache *cache)
{
	boolean valid =
		cache->signature == _lrar_cache_signature &&
		cache->minimum_address < cache->maximum_address &&
		cache->total_size > 0 &&
		cache->block_count > 0;

	match_vassert(
		"c:\\halo\\SOURCE\\memory\\lrar_cache.c",
		0x199,
		valid,
		csprintf(
			temporary,
			"lrar cache %s @%p appears to be corrupt",
			cache,
			cache));

	return;
}
