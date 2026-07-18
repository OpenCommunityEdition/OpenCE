/*
STACK_MEMORY_POOL.C

symbols in this file:
0010E1E0 0090:
	_stack_memory_pool_reset (0000)
0010E270 0040:
	_code_0010e270 (0000)
0010E2B0 0030:
	_code_0010e2b0 (0000)
0010E2E0 0050:
	_code_0010e2e0 (0000)
0010E330 0030:
	_code_0010e330 (0000)
0010E360 0080:
	_code_0010e360 (0000)
0010E3E0 0050:
	_code_0010e3e0 (0000)
0010E430 0060:
	_code_0010e430 (0000)
0010E490 0080:
	_code_0010e490 (0000)
0010E510 00a0:
	_code_0010e510 (0000)
0010E5B0 0040:
	_code_0010e5b0 (0000)
0010E5F0 0040:
	_code_0010e5f0 (0000)
0010E630 0040:
	_code_0010e630 (0000)
0010E670 0030:
	_code_0010e670 (0000)
0010E6A0 00d0:
	_code_0010e6a0 (0000)
0010E770 0080:
	_code_0010e770 (0000)
0010E7F0 00a0:
	_code_0010e7f0 (0000)
0010E890 00d0:
	_code_0010e890 (0000)
0010E960 00a0:
	_code_0010e960 (0000)
0010EA00 0280:
	_code_0010ea00 (0000)
0010EC80 0080:
	_dispose_handle (0000)
0010ED00 0070:
	_lock_handle (0000)
0010ED70 00d0:
	_unlock_handle (0000)
0010EE40 00b0:
	_dispose_pointer (0000)
0010EEF0 0020:
	_code_0010eef0 (0000)
0010EF10 0060:
	_code_0010ef10 (0000)
0010EF70 00c0:
	_code_0010ef70 (0000)
0010F030 0070:
	_pool_new_handle (0000)
0010F0A0 00b0:
	_pool_new_handle_clear (0000)
0010F150 0110:
	_pool_resize_handle (0000)
0010F260 00b0:
	_pool_new_pointer (0000)
0010F310 00f0:
	_pool_new_pointer_clear (0000)
0010F400 0130:
	_pool_resize_pointer (0000)
0027E6B4 0005:
	??_C@_04CJIJKHMO@pool?$AA@ (0000)
0027E6BC 002a:
	??_C@_0CK@POCGLLGP@c?3?2halo?2SOURCE?2memory?2stack_memo@ (0000)
0027E6E8 0006:
	??_C@_05PIBOEKAB@block?$AA@ (0000)
0027E6F0 001b:
	??_C@_0BL@DBFCIPGG@pool?5?$CG?$CG?5pool?9?$DObase_address?$AA@ (0000)
0027E70C 0037:
	??_C@_0DH@EDIODODK@?$CB?$CCwrote?5beyond?5the?5valid?5address@ (0000)
0027E744 0022:
	??_C@_0CC@ECLAPKCC@?$CB?$CCthis?5memory?5has?5been?5corrupted@ (0000)
0027E768 001c:
	??_C@_0BM@JILEAMIL@?$CB?$CCpointer?5has?5invalid?5size?$CC?$AA@ (0000)
0027E784 001a:
	??_C@_0BK@LDONEMGA@memory_block_valid?$CIblock?$CJ?$AA@ (0000)
0027E7A0 002b:
	??_C@_0CL@HLEBFHCG@pool?5?$CG?$CG?5pool?9?$DObase_address?5?$CG?$CG?5po@ (0000)
0027E7CC 002f:
	??_C@_0CP@BNFGAJLL@stack_memory_pool_valid_block?$CIpo@ (0000)
0027E800 0055:
	??_C@_0FF@JCHHGMKP@stack_memory_pool_valid_block?$CIpo@ (0000)
0027E858 0054:
	??_C@_0FE@NGEIOJCH@stack_memory_pool_valid_block?$CIpo@ (0000)
0027E8AC 000d:
	??_C@_0N@CECKNFDB@invalid?5size?$AA@ (0000)
0027E8BC 001c:
	??_C@_0BM@OEEFEPIM@free_space_in_pool_previous?$AA@ (0000)
0027E8D8 003c:
	??_C@_0DM@JDDKECPN@?$CIpool?9?$DOlast_block?5?$DN?$DN?5NULL?$CJ?5?$CG?$CG?5?$CIp@ (0000)
0027E914 0011:
	??_C@_0BB@LKLKPKKP@pool?9?$DOlast_block?$AA@ (0000)
0027E928 0053:
	??_C@_0FD@KNOHBMJO@the?5memory?5pool?5has?5no?5more?5unsu@ (0000)
0027E980 0050:
	??_C@_0FA@LKCLAHNE@allocation?5from?5memory?5pool?5fail@ (0000)
0027E9D0 0028:
	??_C@_0CI@IMGJKFFB@not?5a?5valid?5handle?0?5or?5handle?5is@ (0000)
0027E9F8 0030:
	??_C@_0DA@KBONCLF@not?5a?5valid?5handle?0?5or?5handle?5is@ (0000)
0027EA28 0002:
	??_C@_01BDACAMKP@h?$AA@ (0000)
0027EA2C 0029:
	??_C@_0CJ@NEONIHNA@invalid?5handle?0?5or?5handle?5was?5no@ (0000)
0027EA58 0010:
	??_C@_0BA@KKNICBIG@invalid?5pointer?$AA@ (0000)
0027EA68 0038:
	??_C@_0DI@NCMLNKNJ@not?5a?5valid?5handle?0?5or?5trying?5to@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct stack_memory_pool_block
{
	unsigned long size_and_flags;
	long slot_index;
	struct stack_memory_pool_block *previous;
	struct stack_memory_pool_block *next;
	void **handle;
	long lock_count;
	unsigned long header_signature;
	byte data[1];
};

struct stack_memory_pool
{
	char const *name;
	byte *base_address;
	long size;
	long maximum_block_count;
	long next_block_index;
	long bytes_used;
	long maximum_bytes_used;
	long block_count;
	long maximum_block_count_used;
	long largest_block_size;
	boolean disable_compaction;
	byte unused29[3];
	struct stack_memory_pool_block *first_block;
	struct stack_memory_pool_block *last_block;
	struct stack_memory_pool_block *blocks[1];
};

/* ---------- prototypes */

static void code_0010e270(
	long slot_index,
	struct stack_memory_pool_block *block,
	long block_size);
static unsigned long code_0010e2b0(
	struct stack_memory_pool_block *block);
static unsigned long code_0010e2e0(
	struct stack_memory_pool_block *block);
static long code_0010e330(
	struct stack_memory_pool_block *block);
static unsigned long code_0010e360(
	struct stack_memory_pool *pool);
static long code_0010e3e0(
	struct stack_memory_pool *pool);
static void code_0010e430(
	struct stack_memory_pool *pool);
static void *code_0010e490(
	struct stack_memory_pool *pool,
	long allocation_size,
	struct stack_memory_pool_block **previous_block);
static boolean code_0010e510(
	struct stack_memory_pool_block *block);

/* ---------- globals */

/* ---------- public code */

void stack_memory_pool_reset(
	struct stack_memory_pool *pool)
{
	unsigned long *pool_data = (unsigned long *)pool;
	unsigned long saved_name;
	unsigned long saved_base_address;
	unsigned long saved_size;
	unsigned long saved_maximum_block_count;
	unsigned long *blocks;
	unsigned long blocks_address;

	if (!pool)
	{
		display_assert("pool", "c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x1B0, TRUE);
		system_exit(-1);
	}

	saved_name = pool_data[0];
	saved_base_address = pool_data[1];
	saved_maximum_block_count = pool_data[3];
	blocks = pool_data + 13;
	blocks_address = (unsigned long)blocks;
	saved_size = pool_data[2];

	csmemset(blocks, 0, saved_maximum_block_count * sizeof(*blocks));
	csmemset(pool, 0, 0x34);

	pool_data[0] = saved_name;
	pool_data[1] = saved_base_address;
	pool_data[3] = saved_maximum_block_count;
	pool_data[2] = saved_size;
	csmemcpy(blocks, &blocks_address, sizeof(blocks_address));

	/* Retain the private block-header helpers until their allocator callers land. */
	if (FALSE)
	{
		struct stack_memory_pool_block *block = pool->first_block;
		code_0010e270(0, block, sizeof(*block));
		code_0010e2b0(block);
		code_0010e2e0(block);
		code_0010e330(block);
		code_0010e360(pool);
		code_0010e3e0(pool);
		code_0010e430(pool);
		code_0010e490(pool, sizeof(*block), &block);
		code_0010e510(block);
	}

	return;
}

/* ---------- private code */

static void code_0010e270(
	long slot_index,
	struct stack_memory_pool_block *block,
	long block_size)
{
	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x21F, block);
	block->size_and_flags = block_size;
	block->slot_index = slot_index;
	block->header_signature = 'fryd';
	*(unsigned long *)((byte *)block+block_size-sizeof(unsigned long)) = 'chkn';
	return;
}

static unsigned long code_0010e2b0(
	struct stack_memory_pool_block *block)
{
	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x22F, block);
	return block->size_and_flags&0x7FFFFFFF;
}

static unsigned long code_0010e2e0(
	struct stack_memory_pool_block *block)
{
	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x237, block);
	return code_0010e2b0(block)-sizeof(*block);
}

static long code_0010e330(
	struct stack_memory_pool_block *block)
{
	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x24A, block);
	return block->slot_index;
}

static unsigned long code_0010e360(
	struct stack_memory_pool *pool)
{
	struct stack_memory_pool_block *block;

	match_assert(
		"c:\\halo\\SOURCE\\memory\\stack_memory_pool.c",
		0x2FC,
		pool && pool->base_address);
	if (!pool->first_block)
		return pool->size;

	block = pool->last_block;
	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x22F, block);
	return pool->size-(block->size_and_flags&0x7FFFFFFF)+(unsigned long)pool->base_address-(unsigned long)block;
}

static long code_0010e3e0(
	struct stack_memory_pool *pool)
{
	long block_index;

	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x310, pool);
	for (block_index = 0; block_index < pool->maximum_block_count; block_index++)
	{
		if (!pool->blocks[block_index])
			return block_index;
	}
	return NONE;
}

static void code_0010e430(
	struct stack_memory_pool *pool)
{
	long block_index;

	match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x321, pool);
	block_index = pool->next_block_index;
	if (block_index != NONE)
	{
		pool->next_block_index = NONE;
		for (block_index++; block_index < pool->maximum_block_count; block_index++)
		{
			if (!pool->blocks[block_index])
			{
				pool->next_block_index = block_index;
				break;
			}
		}
	}
	return;
}

static void *code_0010e490(
	struct stack_memory_pool *pool,
	long allocation_size,
	struct stack_memory_pool_block **previous_block)
{
	struct stack_memory_pool_block *block;
	struct stack_memory_pool_block *next_block;

	block = pool->first_block;
	if (!block)
		return NULL;
	if (allocation_size <= (byte *)block-pool->base_address)
		return pool->base_address;

	next_block = block->next;
	while (next_block)
	{
		match_assert("c:\\halo\\SOURCE\\memory\\stack_memory_pool.c", 0x22F, block);
		if (allocation_size <= (byte *)next_block-((byte *)block+(block->size_and_flags&0x7FFFFFFF)))
		{
			*previous_block = block;
			return (byte *)block+(block->size_and_flags&0x7FFFFFFF);
		}
		block = next_block;
		next_block = next_block->next;
	}
	return NULL;
}

static boolean code_0010e510(
	struct stack_memory_pool_block *block)
{
	unsigned long block_size;

	if (!block)
		return FALSE;
	block_size = block->size_and_flags&0x7FFFFFFF;
	if (!(block_size-sizeof(*block)))
	{
		display_assert(
			"!\"pointer has invalid size\"",
			"c:\\halo\\SOURCE\\memory\\stack_memory_pool.c",
			0x1E4,
			TRUE);
		system_exit(-1);
		return FALSE;
	}
	if (block->header_signature != 'fryd')
	{
		display_assert(
			"!\"this memory has been corrupted\"",
			"c:\\halo\\SOURCE\\memory\\stack_memory_pool.c",
			0x1E9,
			TRUE);
		system_exit(-1);
		return FALSE;
	}
	if (*(unsigned long *)((byte *)block+block_size-sizeof(unsigned long)) != 'chkn')
	{
		display_assert(
			"!\"wrote beyond the valid address space for this block\"",
			"c:\\halo\\SOURCE\\memory\\stack_memory_pool.c",
			0x1EE,
			TRUE);
		system_exit(-1);
		return FALSE;
	}
	return TRUE;
}
