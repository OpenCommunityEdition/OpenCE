/*
WEB_MEMORY_WATCH.C

Write tracking for the memory the renderer caches
(port/linux/src/memory_watch.c's interface), for the web build.

WebAssembly has no page protection, so a write cannot fault: a watched
page's contents are hashed instead, and hashed again when the renderer asks
for its generation, at most once a frame, a different hash counting as a
write. The hashing is the Android host's for the x86 emulator, which cannot
catch its faults either (port/android/host/host_watch_hash.c); a write is
seen in the next frame, not at once. Pages that have long stayed the same
are hashed only every few frames, unless a write into them is announced
(memory_watch_prepare_write: the file layer's reads, the Direct3D locks).
*/

#include "platform.h"
#include "host_watch_hash.h"

#define WATCH_PAGE_COUNT (PLATFORM_CONTIGUOUS_SIZE / WATCH_HASH_PAGE_SIZE)

static uint8_t page_watched[WATCH_PAGE_COUNT];
static uint64_t page_hash[WATCH_PAGE_COUNT];
static uint32_t page_hashed_frame[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static uint8_t page_unchanged[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static pthread_mutex_t watch_lock = PTHREAD_MUTEX_INITIALIZER;

static struct watch_hash watch =
{
	(const uint8_t *)PLATFORM_CONTIGUOUS_BASE,
	WATCH_PAGE_COUNT,
	page_watched,
	page_hash,
	page_hashed_frame,
	page_generation,
	&current_generation,
	1,
	page_unchanged,
};

/* the pages of a range of the window, first to last; FALSE if it is not in
the window */
static BOOL watch_pages(unsigned long address, unsigned long size, uint32_t *first, uint32_t *last)
{
	unsigned long start = address, end = address + size;

	if (!size || end <= PLATFORM_CONTIGUOUS_BASE || start >= PLATFORM_CONTIGUOUS_BASE + PLATFORM_CONTIGUOUS_SIZE)
		return FALSE;
	if (start < PLATFORM_CONTIGUOUS_BASE)
		start = PLATFORM_CONTIGUOUS_BASE;
	*first = (uint32_t)((start - PLATFORM_CONTIGUOUS_BASE) / WATCH_HASH_PAGE_SIZE);
	*last = (uint32_t)((end - 1 - PLATFORM_CONTIGUOUS_BASE) / WATCH_HASH_PAGE_SIZE);
	return TRUE;
}

void memory_watch_initialize(void)
{
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	uint32_t first, last;

	if (!watch_pages(address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	watch_hash_protect(&watch, first, last);
	pthread_mutex_unlock(&watch_lock);
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	uint32_t first, last, generation;

	if (!watch_pages(address, size, &first, &last))
		return 0;
	pthread_mutex_lock(&watch_lock);
	generation = watch_hash_generation(&watch, first, last);
	pthread_mutex_unlock(&watch_lock);
	return generation;
}

unsigned long memory_watch_serial(void)
{
	return current_generation;
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	uint32_t first, last;

	if (!watch_pages((unsigned long)address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	watch_hash_prepare_write(&watch, first, last);
	pthread_mutex_unlock(&watch_lock);
}

void memory_watch_forget(void *address, unsigned long size)
{
	uint32_t first, last;

	if (!watch_pages((unsigned long)address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	watch_hash_forget(&watch, first, last);
	pthread_mutex_unlock(&watch_lock);
}

void memory_watch_begin_frame(void)
{
	pthread_mutex_lock(&watch_lock);
	watch_hash_begin_frame(&watch);
	pthread_mutex_unlock(&watch_lock);
}
