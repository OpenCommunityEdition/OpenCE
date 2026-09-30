/*
HOST_WATCH_HASH.H

Write tracking by page contents, for processes that cannot catch their own
page protection faults: the ARM translation of the x86 Android emulator
cannot deliver SIGSEGV to the app, so the protection-based tracking of
host_memory.c would stop the game at its first write to a cached page.

A watched page's hash is kept. Asking for its generation hashes it again,
at most once a frame (watch_hash_begin_frame), and a different hash counts
as a write. A write is therefore seen in the next frame, not at once.
*/

#ifndef HOST_WATCH_HASH_H
#define HOST_WATCH_HASH_H

#include <stdint.h>

#define WATCH_HASH_PAGE_SIZE 0x1000u

struct watch_hash
{
	/* the memory of page 0 */
	const uint8_t *base;
	uint32_t page_count;
	/* page_count entries each */
	uint8_t *watched;
	uint64_t *hash;
	uint32_t *hashed_frame;
	/* shared with the protection-based tracking (host_memory.c) */
	uint32_t *generation;
	volatile uint32_t *current_generation;
	uint32_t frame;
};

/* a hash of one page (WATCH_HASH_PAGE_SIZE bytes, 8-byte aligned) that
changes whenever a word of it changes */
uint64_t watch_hash_page(const uint8_t *page);

/* Starts watching pages first to last (inclusive), remembering their hashes.
Pages already watched keep their hash; pages beyond page_count are
ignored. */
void watch_hash_protect(struct watch_hash *watch, uint32_t first, uint32_t last);

/* The newest generation among pages first to last (inclusive), after hashing
again the watched ones not yet hashed this frame; a page whose hash
changed gets a new generation. 0 if the range is outside the watched
memory. */
uint32_t watch_hash_generation(struct watch_hash *watch, uint32_t first, uint32_t last);

/* The pages were remapped: treat them as written and unwatched. */
void watch_hash_forget(struct watch_hash *watch, uint32_t first, uint32_t last);

/* A new frame: pages may be hashed again, and the serial changes. */
void watch_hash_begin_frame(struct watch_hash *watch);

#endif
