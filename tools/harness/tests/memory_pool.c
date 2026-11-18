/*
MEMORY_POOL.C

The memory pool (source/memory/memory_pool.c), compiled whole: blocks
allocated, freed, resized and compacted keep their contents, sizes and links,
and an allocate, free or resize that reaches a damaged pool or block stops at
its check (match_assert, which jumps back to the case here).
*/

#include <setjmp.h>
#include "harness.h"

#pragma clang diagnostic ignored "-Wmultichar"

#undef match_assert
static jmp_buf assert_jump;
static int assert_line;
#define match_assert(file, line, condition) \
	do { if (!(condition)) { assert_line = (line); longjmp(assert_jump, 1); } } while (0)
#define match_vassert(file, line, condition, message) match_assert(file, line, condition)
static char temporary[16];
#define csprintf(buffer, ...) (buffer)
#define csmemset(destination, value, size) memset(destination, value, size)
#define csmemmove(destination, source, size) memmove(destination, source, size)
#define csmemcpy(destination, source, size) memcpy(destination, source, size)
#define csstrncpy(destination, source, size) strncpy(destination, source, size)
#define match_malloc(file, line, size) malloc(size)
#define match_free(file, line, pointer) free(pointer)

#include "under_test.inc"

#define EXPECT_OK(statement) \
	do { if (!setjmp(assert_jump)) { statement; } else CHECK(0, "check at line %d failed: %s", assert_line, #statement); } while (0)
#define EXPECT_CHECK(line, statement) \
	do \
	{ \
		assert_line = 0; \
		if (!setjmp(assert_jump)) \
		{ \
			statement; \
			CHECK(0, "no check failed, expected line %d: %s", (line), #statement); \
		} \
		CHECK(assert_line == (line), "check at line %d failed, expected line %d: %s", assert_line, (line), #statement); \
	} while (0)

#define SLOTS 64
#define POOL_SIZE 0x10000

static struct memory_pool *pool;
static void *slot_data[SLOTS];
static long slot_size[SLOTS];
static unsigned char slot_fill[SLOTS];
static unsigned long random_state = 12345;

static unsigned long next_random(void)
{
	random_state = random_state * 1103515245 + 12345;
	return (random_state >> 8) & 0xFFFFFF;
}

static void fill(int slot, long from)
{
	long index;

	for (index = from; index < slot_size[slot]; index++)
		((unsigned char *)slot_data[slot])[index] = (unsigned char)(slot_fill[slot] + index);
}

static void check_slots(void)
{
	long used = 0;
	int slot;
	long index;

	for (slot = 0; slot < SLOTS; slot++)
	{
		if (!slot_data[slot])
			continue;
		used += memory_pool_block_compute_actual_size(slot_size[slot]);
		for (index = 0; index < slot_size[slot]; index++)
		{
			CHECK(((unsigned char *)slot_data[slot])[index] == (unsigned char)(slot_fill[slot] + index),
				"slot %d byte %ld changed", slot, index);
		}
	}
	CHECK(pool->free_size == pool->size - used, "free size %ld, expected %ld", pool->free_size, pool->size - used);
	EXPECT_OK(memory_pool_verify(pool));
}

static boolean allocate(int slot, long size)
{
	boolean done = FALSE;

	EXPECT_OK(done = memory_pool_block_allocate(pool, &slot_data[slot], size));
	if (!done)
	{
		slot_data[slot] = NULL;
		return FALSE;
	}
	slot_size[slot] = size;
	slot_fill[slot] = (unsigned char)next_random();
	fill(slot, 0);
	return TRUE;
}

/* random allocations, frees, resizes and compactions, each slot's contents
checked after every one */
static void case_sequence(void)
{
	int step;

	pool = memory_pool_new("test", POOL_SIZE);
	for (step = 0; step < 20000; step++)
	{
		int slot = (int)(next_random() % SLOTS);
		unsigned long choice = next_random() % 3;

		if (!slot_data[slot])
		{
			long size = (long)(next_random() % 1200);

			if (!allocate(slot, size))
			{
				EXPECT_OK(memory_pool_compact(pool));
				allocate(slot, size);
			}
		}
		else if (choice == 0)
		{
			EXPECT_OK(memory_pool_block_free(pool, &slot_data[slot]));
			slot_data[slot] = NULL;
		}
		else if (choice == 1)
		{
			long size = (long)(next_random() % 1200);
			boolean done = FALSE;

			EXPECT_OK(done = memory_pool_block_reallocate(pool, &slot_data[slot], size));
			if (done)
			{
				long old_size = slot_size[slot];

				slot_size[slot] = size;
				if (size > old_size)
					fill(slot, old_size);
			}
		}
		if (step % 97 == 0)
			EXPECT_OK(memory_pool_compact(pool));
		check_slots();
	}
	memory_pool_delete(pool);
}

/* the first, last and only block freed and resized, and a pool emptied and
used again */
static void case_ends(void)
{
	int slot;

	pool = memory_pool_new("test", 4096);
	for (slot = 0; slot < 3; slot++)
		CHECK(allocate(slot, 100), "allocate %d", slot);
	EXPECT_OK(memory_pool_block_free(pool, &slot_data[0]));
	slot_data[0] = NULL;
	check_slots();
	EXPECT_OK(memory_pool_block_free(pool, &slot_data[2]));
	slot_data[2] = NULL;
	check_slots();
	CHECK(pool->first_block == pool->last_block, "one block left");
	EXPECT_OK(memory_pool_block_reallocate(pool, &slot_data[1], 3000));
	slot_size[1] = 3000;
	fill(1, 100);
	check_slots();
	EXPECT_OK(memory_pool_block_free(pool, &slot_data[1]));
	slot_data[1] = NULL;
	CHECK(!pool->first_block && !pool->last_block && pool->free_size == pool->size, "empty");
	CHECK(allocate(0, 4096 - (long)sizeof(struct memory_pool_block)), "the whole empty pool");
	check_slots();
	memory_pool_delete(pool);
}

/* an allocation into an empty pool still checks the pool */
static void case_empty_pool_damaged(void)
{
	void *reference;

	pool = memory_pool_new("test", 4096);
	CHECK(allocate(0, 100), "allocate");
	EXPECT_OK(memory_pool_block_free(pool, &slot_data[0]));
	pool->signature = 0;
	EXPECT_CHECK(340, memory_pool_block_allocate(pool, &reference, 100));
	pool->signature = POOL_SIGNATURE;
	pool->size = 0;
	EXPECT_CHECK(341, memory_pool_block_allocate(pool, &reference, 100));
}

/* a damaged block, or one not linked as its neighbours say, stops a call
that reaches it */
static void case_block_damaged(void)
{
	struct memory_pool_block *first, *middle, *last;
	void *reference;
	void *stale;
	int slot;

	pool = memory_pool_new("test", 4096);
	for (slot = 0; slot < 3; slot++)
		CHECK(allocate(slot, 100), "allocate %d", slot);
	first = (struct memory_pool_block *)slot_data[0] - 1;
	middle = (struct memory_pool_block *)slot_data[1] - 1;
	last = (struct memory_pool_block *)slot_data[2] - 1;

	middle->trailer_signature = 0;
	EXPECT_CHECK(356, memory_pool_block_free(pool, &slot_data[1]));
	middle->trailer_signature = BLOCK_TRAILER_SIGNATURE;

	last->header_signature = 0;
	EXPECT_CHECK(355, memory_pool_block_allocate(pool, &reference, 100));
	last->header_signature = BLOCK_HEADER_SIGNATURE;

	first->next_block = last;
	EXPECT_CHECK(352, memory_pool_block_free(pool, &slot_data[1]));
	first->next_block = middle;

	last->previous_block = first;
	EXPECT_CHECK(353, memory_pool_block_reallocate(pool, &slot_data[1], 50));
	last->previous_block = middle;

	check_slots();
	stale = slot_data[1];
	EXPECT_OK(memory_pool_block_free(pool, &slot_data[1]));
	slot_data[1] = stale;
	EXPECT_CHECK(379, memory_pool_block_free(pool, &slot_data[1]));
}

int main(int argc, char **argv)
{
	const char *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "sequence"))
		case_sequence();
	else if (!strcmp(name, "ends"))
		case_ends();
	else if (!strcmp(name, "empty-pool-damaged"))
		case_empty_pool_damaged();
	else if (!strcmp(name, "block-damaged"))
		case_block_damaged();
	else
	{
		fprintf(stderr, "unknown case %s\n", name);
		return 2;
	}
	(void)temporary;
	return 0;
}
