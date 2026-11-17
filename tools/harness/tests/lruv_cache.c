/*
LRUV_CACHE.C

The lruv cache (source/memory/lruv_cache.c), compiled whole over the harness's
data arrays: blocks made, evicted, deleted, flushed and cut off by a resize
keep the cache's list in page order and every block's links whole, and a
delete that reaches a block whose neighbours do not link to it stops at its
check (match_assert, which jumps back to the case here).
*/

#include <setjmp.h>
#include "harness.h"

#pragma clang diagnostic ignored "-Wmultichar"
#pragma clang diagnostic ignored "-Wformat"

#undef match_assert
static jmp_buf assert_jump;
static int assert_line;
#define match_assert(file, line, condition) \
	do { if (!(condition)) { assert_line = (line); longjmp(assert_jump, 1); } } while (0)
#define csmemset(destination, value, size) memset(destination, value, size)
#define csstrncpy(destination, source, size) strncpy(destination, source, size)
#define match_malloc(file, line, size) malloc(size)
#define match_free(file, line, pointer) free(pointer)
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SHORT_BITS 16
#define actual_count count

static long data_allocation_size(short maximum_count, long size)
{
	(void)maximum_count;
	(void)size;
	return sizeof(struct data_array);
}

static void data_initialize(struct data_array *array, const char *name, short maximum_count, long size)
{
	struct data_array *made = game_state_data_new(name, maximum_count, size);

	*array = *made;
	free(made);
}

#define data_make_valid(array) ((void)(array))
#define data_verify(array) ((void)(array))

static void data_dispose(struct data_array *array)
{
	free(array->data);
}

struct data_iterator
{
	struct data_array *array;
	long next;
	long datum_index;
};

static void data_iterator_new(struct data_iterator *iterator, struct data_array *array)
{
	iterator->array = array;
	iterator->next = 0;
	iterator->datum_index = NONE;
}

static void *data_iterator_next(struct data_iterator *iterator)
{
	while (iterator->next < iterator->array->maximum_count)
	{
		long absolute = iterator->next++;
		struct datum_header *header =
			(struct datum_header *)(iterator->array->data + absolute * iterator->array->size);

		if (header->identifier)
		{
			iterator->datum_index = ((long)(unsigned short)header->identifier << 16) | absolute;
			return header;
		}
	}
	iterator->datum_index = NONE;
	return NULL;
}

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

#define MAXIMUM_BLOCKS 48
#define PAGES 256
#define PAGE_BITS 4

static struct lruv_cache *cache;
/* each block the cache has, as its callbacks saw it (by absolute index) */
static long live_index[MAXIMUM_BLOCKS];
static boolean locked[MAXIMUM_BLOCKS];
static long deleted_count;
static unsigned long random_state = 2024;

static unsigned long next_random(void)
{
	random_state = random_state * 1103515245 + 12345;
	return (random_state >> 8) & 0xFFFFFF;
}

static void block_deleted(long block_index)
{
	long absolute = DATUM_INDEX_TO_ABSOLUTE_INDEX(block_index);

	CHECK(live_index[absolute] == block_index, "deleted a block not made: %08lx", block_index);
	CHECK(!locked[absolute], "deleted a locked block: %08lx", block_index);
	live_index[absolute] = NONE;
	deleted_count++;
}

static boolean block_locked(long block_index)
{
	return locked[DATUM_INDEX_TO_ABSOLUTE_INDEX(block_index)];
}

static void make_cache(void)
{
	int index;

	for (index = 0; index < MAXIMUM_BLOCKS; index++)
	{
		live_index[index] = NONE;
		locked[index] = FALSE;
	}
	cache = malloc(lruv_allocation_size(MAXIMUM_BLOCKS));
	EXPECT_OK(lruv_initialize(cache, "test", PAGES, PAGE_BITS, MAXIMUM_BLOCKS, block_deleted, block_locked));
}

/* the cache's list against the blocks made and not deleted: in page order,
not overlapping, inside the cache, each linked both ways */
static void check_cache(void)
{
	long block_index = cache->first_block_index;
	long previous_index = NONE;
	long previous_end = 0;
	long listed = 0;
	long live = 0;
	int index;

	EXPECT_OK(lruv_cache_verify(cache, TRUE));
	while (block_index != NONE)
	{
		struct lruv_cache_block *block = datum_get(cache->blocks, block_index);

		CHECK(live_index[DATUM_INDEX_TO_ABSOLUTE_INDEX(block_index)] == block_index, "listed %08lx not made", block_index);
		CHECK(block->previous_block_index == previous_index, "%08lx's previous", block_index);
		CHECK(block->first_page_index >= previous_end, "%08lx overlaps", block_index);
		CHECK(block->page_count > 0 && block->first_page_index + block->page_count <= cache->page_count,
			"%08lx outside the cache", block_index);
		previous_end = block->first_page_index + block->page_count;
		previous_index = block_index;
		block_index = block->next_block_index;
		listed++;
		CHECK(listed <= MAXIMUM_BLOCKS, "the list loops");
	}
	CHECK(cache->last_block_index == previous_index, "last block");
	for (index = 0; index < MAXIMUM_BLOCKS; index++)
		live += live_index[index] != NONE;
	CHECK(listed == live && live == cache->blocks->count, "%ld listed, %ld made, %ld in the array",
		listed, live, cache->blocks->count);
}

static long make_block(long size)
{
	long block_index = NONE;

	EXPECT_OK(block_index = lruv_block_new(cache, size));
	if (block_index != NONE)
	{
		long absolute = DATUM_INDEX_TO_ABSOLUTE_INDEX(block_index);

		CHECK(live_index[absolute] == NONE, "made over a live block");
		live_index[absolute] = block_index;
	}
	return block_index;
}

static long random_live(void)
{
	int tries;

	for (tries = 0; tries < 4 * MAXIMUM_BLOCKS; tries++)
	{
		long index = live_index[next_random() % MAXIMUM_BLOCKS];

		if (index != NONE)
			return index;
	}
	return NONE;
}

/* random blocks made (evicting the oldest unlocked, and the oldest when the
array is full), touched, locked, deleted, flushed and resized */
static void case_sequence(void)
{
	int step;
	long evicted;

	make_cache();
	for (step = 0; step < 30000; step++)
	{
		unsigned long choice = next_random() % 100;
		long block_index;

		if (choice < 55)
		{
			make_block((long)(next_random() % (24 << PAGE_BITS)) + 1);
		}
		else if (choice < 70)
		{
			if ((block_index = random_live()) != NONE)
				EXPECT_OK(lruv_block_touch(cache, block_index));
		}
		else if (choice < 76)
		{
			if ((block_index = random_live()) != NONE)
				locked[DATUM_INDEX_TO_ABSOLUTE_INDEX(block_index)] ^= 1;
		}
		else if (choice < 90)
		{
			if ((block_index = random_live()) != NONE && !block_locked(block_index))
				EXPECT_OK(lruv_block_delete(cache, block_index));
		}
		else if (choice < 98)
		{
			EXPECT_OK(lruv_idle(cache));
		}
		else if (choice < 99)
		{
			int index;

			for (index = 0; index < MAXIMUM_BLOCKS; index++)
				locked[index] = FALSE;
			EXPECT_OK(lruv_flush(cache));
			CHECK(cache->first_block_index == NONE && cache->blocks->count == 0, "flushed");
		}
		else
		{
			long pages = 64 + (long)(next_random() % (PAGES - 63));
			int index;

			for (index = 0; index < MAXIMUM_BLOCKS; index++)
				locked[index] = FALSE;
			EXPECT_OK(lruv_resize(cache, pages));
			check_cache();
			CHECK(cache->last_block_index == NONE ||
				((struct lruv_cache_block *)datum_get(cache->blocks, cache->last_block_index))->first_page_index +
				((struct lruv_cache_block *)datum_get(cache->blocks, cache->last_block_index))->page_count <= pages,
				"a block past the new end");
		}
		check_cache();
	}
	evicted = deleted_count;
	CHECK(evicted > 1000, "only %ld deleted: the evictions were not exercised", evicted);
	EXPECT_OK(lruv_delete(cache));
}

/* the first, last and only block deleted, and blocks made before the first
and after the last */
static void case_ends(void)
{
	long first, middle, last, front;

	make_cache();
	first = make_block(16 << PAGE_BITS);
	middle = make_block(16 << PAGE_BITS);
	last = make_block(16 << PAGE_BITS);
	check_cache();
	CHECK(cache->first_block_index == first && cache->last_block_index == last, "made in page order");
	EXPECT_OK(lruv_block_delete(cache, first));
	check_cache();
	CHECK(cache->first_block_index == middle, "the next one first");
	EXPECT_OK(lruv_idle(cache));
	front = make_block(8 << PAGE_BITS);
	check_cache();
	CHECK(cache->first_block_index == front, "made before the first: %08lx, first %08lx", front, cache->first_block_index);
	EXPECT_OK(lruv_block_delete(cache, last));
	check_cache();
	CHECK(cache->last_block_index == middle, "the one before last");
	EXPECT_OK(lruv_block_delete(cache, front));
	EXPECT_OK(lruv_block_delete(cache, middle));
	check_cache();
	CHECK(cache->first_block_index == NONE && cache->last_block_index == NONE, "empty");
	first = make_block(PAGES << PAGE_BITS);
	check_cache();
	CHECK(first != NONE, "the whole empty cache");
}

/* a delete reaching a block its neighbours do not link to stops at the check */
static void case_damaged(void)
{
	long first, middle, last;
	struct lruv_cache_block *block;

	make_cache();
	first = make_block(4 << PAGE_BITS);
	middle = make_block(4 << PAGE_BITS);
	last = make_block(4 << PAGE_BITS);

	block = datum_get(cache->blocks, first);
	block->next_block_index = last;
	EXPECT_CHECK(776, lruv_block_delete(cache, middle));
	block->next_block_index = middle;

	block = datum_get(cache->blocks, last);
	block->previous_block_index = first;
	EXPECT_CHECK(790, lruv_block_delete(cache, middle));
	block->previous_block_index = middle;

	cache->first_block_index = middle;
	EXPECT_CHECK(770, lruv_block_delete(cache, first));
	cache->first_block_index = first;

	cache->last_block_index = middle;
	EXPECT_CHECK(784, lruv_block_delete(cache, last));
	cache->last_block_index = last;

	block = datum_get(cache->blocks, middle);
	block->first_page_index = 2;
	EXPECT_CHECK(778, lruv_block_delete(cache, middle));
	block->first_page_index = 4;
	check_cache();
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("sequence") case_sequence();
	else CASE("ends") case_ends();
	else CASE("damaged") case_damaged();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
