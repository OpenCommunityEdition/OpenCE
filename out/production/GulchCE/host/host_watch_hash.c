/*
HOST_WATCH_HASH.C

Write tracking by page contents (host_watch_hash.h).
*/

#include "host_watch_hash.h"

#ifdef __wasm_simd128__
/*
FNV-style mixing in sixteen independent 32-bit lanes, four vectors of four
(WebAssembly's SIMD: the web build, web_memory_watch.c), so that no lane
waits for another and the multiplies are the vector unit's own; its 64-bit
multiplies are scalar, which made the hashing of every watched page each
frame a quarter of the web build's frame, twice what this takes. A change of
one word changes its lane's state for good (xor and an odd multiply are both
invertible), and the fold keeps every lane's bits, so a single-word write
always shows. Changes of several words could cancel each other, which is
unlikely for game data. (host_watch_hash.h)
*/
typedef uint32_t watch_hash_lanes __attribute__((vector_size(16), aligned(4), may_alias));

uint64_t watch_hash_page(const uint8_t *page)
{
	const watch_hash_lanes *words = (const watch_hash_lanes *)page;
	const watch_hash_lanes prime = { 0x01000193u, 0x01000193u, 0x01000193u, 0x01000193u };
	watch_hash_lanes lane0 = { 0x811c9dc5u, 0x9e3779b9u, 0x85ebca6bu, 0xc2b2ae35u };
	watch_hash_lanes lane1 = { 0x27d4eb2fu, 0x165667b1u, 0xd3a2646cu, 0xfd7046c5u };
	watch_hash_lanes lane2 = { 0xb55a4f09u, 0x7feb352du, 0x846ca68bu, 0x2c1b3c6du };
	watch_hash_lanes lane3 = { 0x297a2d39u, 0xe6546b64u, 0xcc9e2d51u, 0x1b873593u };
	watch_hash_lanes folded;
	uint32_t index;

	for (index = 0; index < WATCH_HASH_PAGE_SIZE / sizeof(watch_hash_lanes); index += 4)
	{
		lane0 = (lane0 ^ words[index + 0]) * prime;
		lane1 = (lane1 ^ words[index + 1]) * prime;
		lane2 = (lane2 ^ words[index + 2]) * prime;
		lane3 = (lane3 ^ words[index + 3]) * prime;
	}
	/* (each vector's lanes turned before they are combined, so that the same
	change in two of them does not cancel) */
	folded = lane0 ^ ((lane1 << 8) | (lane1 >> 24)) ^ ((lane2 << 16) | (lane2 >> 16)) ^ ((lane3 << 24) | (lane3 >> 8));
	return ((uint64_t)(folded[0] ^ ((folded[2] << 13) | (folded[2] >> 19))) << 32) |
		(folded[1] ^ ((folded[3] << 13) | (folded[3] >> 19)));
}
#else
/*
FNV-style mixing in four independent lanes, so that the multiplies of
one lane do not wait for another's. A change of one word changes its
lane's state for good (xor and an odd multiply are both invertible), so
a single-word write always shows. Changes of several words could cancel
each other, which is unlikely for game data. (host_watch_hash.h)
*/
uint64_t watch_hash_page(const uint8_t *page)
{
	const uint64_t *words = (const uint64_t *)page;
	uint64_t lane0 = 0xcbf29ce484222325ULL, lane1 = 0x9e3779b97f4a7c15ULL;
	uint64_t lane2 = 0xc2b2ae3d27d4eb4fULL, lane3 = 0x165667b19e3779f9ULL;
	uint32_t index;

	for (index = 0; index < WATCH_HASH_PAGE_SIZE / 8; index += 4)
	{
		lane0 = (lane0 ^ words[index + 0]) * 0x100000001b3ULL;
		lane1 = (lane1 ^ words[index + 1]) * 0x100000001b3ULL;
		lane2 = (lane2 ^ words[index + 2]) * 0x100000001b3ULL;
		lane3 = (lane3 ^ words[index + 3]) * 0x100000001b3ULL;
	}
	return lane0 ^ ((lane1 << 16) | (lane1 >> 48)) ^ ((lane2 << 32) | (lane2 >> 32)) ^
		((lane3 << 48) | (lane3 >> 16));
}
#endif

/* clamps last to the watched memory; 0 when nothing is left of the range */
static int clamp(const struct watch_hash *watch, uint32_t first, uint32_t *last)
{
	if (first >= watch->page_count || first > *last)
		return 0;
	if (*last >= watch->page_count)
		*last = watch->page_count - 1;
	return 1;
}

static const uint8_t *page_memory(const struct watch_hash *watch, uint32_t page)
{
	return watch->base + (uint64_t)page * WATCH_HASH_PAGE_SIZE;
}

void watch_hash_protect(struct watch_hash *watch, uint32_t first, uint32_t last)
{
	uint32_t page;

	if (!clamp(watch, first, &last))
		return;
	for (page = first; page <= last; page++)
	{
		if (watch->watched[page])
			continue;
		watch->watched[page] = 1;
		watch->hash[page] = watch_hash_page(page_memory(watch, page));
		watch->hashed_frame[page] = watch->frame;
		if (watch->unchanged)
			watch->unchanged[page] = 0;
	}
}

/* a stable page hashed recently enough: not hashed again yet
(host_watch_hash.h) */
static int stable_for_now(const struct watch_hash *watch, uint32_t page)
{
	return watch->unchanged && watch->unchanged[page] >= WATCH_HASH_STABLE_CHECKS &&
		watch->frame - watch->hashed_frame[page] < WATCH_HASH_STABLE_FRAMES;
}

uint32_t watch_hash_generation(struct watch_hash *watch, uint32_t first, uint32_t last)
{
	uint32_t page, newest = 0;

	if (!clamp(watch, first, &last))
		return 0;
	for (page = first; page <= last; page++)
	{
		if (watch->watched[page] && watch->hashed_frame[page] != watch->frame && !stable_for_now(watch, page))
		{
			uint64_t hash = watch_hash_page(page_memory(watch, page));

			watch->hashed_frame[page] = watch->frame;
			if (hash != watch->hash[page])
			{
				watch->hash[page] = hash;
				watch->generation[page] = __sync_add_and_fetch(watch->current_generation, 1);
				if (watch->unchanged)
					watch->unchanged[page] = 0;
			}
			else if (watch->unchanged && watch->unchanged[page] < WATCH_HASH_STABLE_CHECKS)
			{
				watch->unchanged[page]++;
			}
		}
		if (watch->generation[page] > newest)
			newest = watch->generation[page];
	}
	return newest;
}

void watch_hash_forget(struct watch_hash *watch, uint32_t first, uint32_t last)
{
	uint32_t page;

	if (!clamp(watch, first, &last))
		return;
	for (page = first; page <= last; page++)
	{
		watch->watched[page] = 0;
		watch->generation[page] = __sync_add_and_fetch(watch->current_generation, 1);
	}
}

void watch_hash_prepare_write(struct watch_hash *watch, uint32_t first, uint32_t last)
{
	uint32_t page;

	if (!watch->unchanged || !clamp(watch, first, &last))
		return;
	for (page = first; page <= last; page++)
		watch->unchanged[page] = 0;
}

/*
The serial changes each frame, so a cache that skips generation queries
while the serial stays the same (xbox_textures.c's recent_textures) asks
again once a frame. (host_watch_hash.h)
*/
void watch_hash_begin_frame(struct watch_hash *watch)
{
	watch->frame++;
	if (!watch->frame)
		watch->frame = 1;
	__sync_add_and_fetch(watch->current_generation, 1);
}
