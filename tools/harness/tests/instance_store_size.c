#include "harness.h"

/* The sizes of batching's stores of the instances' registers and blocks
(d3d8_gl.c): twice what a recent frame wrote, in steps of the least, to the
ceiling; larger as soon as a frame writes more than half a store, smaller
only after INSTANCE_STORE_SHRINK_MILLISECONDS in which no frame wrote a
quarter, at any frame rate. A store is made again at its wanted size, as
instances_buffer_ready does. */

#include "under_test.inc"

#define LEAST INSTANCE_STORE_LEAST
#define MOST INSTANCE_STORE_MOST
#define WAIT INSTANCE_STORE_SHRINK_MILLISECONDS

static struct instance_store_size store;
/* the clock (milliseconds, far from 0 as a steady clock is), and the time
between frames */
static unsigned long now = 5000000;
static unsigned long step = 16;

/* a store, its wait counted from now */
static void reset(unsigned long size, unsigned long ceiling)
{
	memset(&store, 0, sizeof(store));
	store.size = size;
	store.ceiling = ceiling;
	store.since = now;
}

/* a frame writing bytes, a step later; the store made again where it wants
another size */
static void frame(unsigned long bytes)
{
	now += step;
	store.frame = bytes;
	instance_store_frame(&store, now);
	CHECK(store.wanted == 0 || (store.wanted % LEAST == 0 && store.wanted >= LEAST &&
		store.wanted <= store.ceiling), "wanted %lu: not a step of %lu within [%lu, %lu]", store.wanted,
		(unsigned long)LEAST, (unsigned long)LEAST, store.ceiling);
	if (store.wanted)
	{
		store.size = store.wanted;
		store.wanted = 0;
	}
}

/* frames writing bytes until the store's size changes: the milliseconds
from start it took */
static unsigned long until_changed(unsigned long bytes, unsigned long start)
{
	unsigned long size = store.size;
	long count;

	for (count = 0; count < 1000000 && store.size == size; count++)
		frame(bytes);
	CHECK(store.size != size, "a store of %lu never changed in %ld frames of %lu bytes", size, count, bytes);
	return now - start;
}

/* twice bytes in steps of the least, at least the least, at most the
ceiling */
static unsigned long expected(unsigned long bytes, unsigned long ceiling)
{
	unsigned long size = (bytes * 2 + LEAST - 1) / LEAST * LEAST;

	if (size < LEAST)
		size = LEAST;
	return size < ceiling ? size : ceiling;
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	CASE("grow")
	{
		/* a frame writing more than half: twice its bytes, at once */
		unsigned long bytes;

		for (bytes = LEAST / 2 + 1; bytes <= MOST; bytes += 77777)
		{
			reset(LEAST, MOST);
			frame(bytes);
			CHECK(store.size == expected(bytes, MOST), "%lu bytes a frame: a store of %lu, not %lu", bytes,
				store.size, expected(bytes, MOST));
			CHECK(store.size >= bytes, "%lu bytes a frame: a store of %lu cannot hold them", bytes, store.size);
		}
		/* half or less: as it is */
		reset(4 * LEAST, MOST);
		frame(2 * LEAST);
		CHECK(store.size == 4 * LEAST, "half the store written: made %lu, not kept at %lu", store.size, 4 * LEAST);
	}
	CASE("ceiling")
	{
		/* no more than the ceiling (the budget, or what the GPU could map),
		however much a frame writes */
		reset(LEAST, 3 * LEAST);
		frame(10 * MOST);
		CHECK(store.size == 3 * LEAST, "a store of %lu over a ceiling of %lu", store.size, 3 * LEAST);
		frame(10 * MOST);
		CHECK(store.size == 3 * LEAST, "at the ceiling: made %lu", store.size);
		reset(LEAST, MOST);
		frame(10 * MOST);
		CHECK(store.size == MOST, "a store of %lu over the budget %lu", store.size, (unsigned long)MOST);
	}
	CASE("shrink")
	{
		/* a store of 8 steps; frames writing under a quarter of it for the
		wait: twice the most they wrote */
		unsigned long start = now, took;
		long count;

		reset(8 * LEAST, MOST);
		frame(LEAST + 5);
		took = until_changed(LEAST / 3, start);
		CHECK(took >= WAIT && took < WAIT + step, "made smaller after %lu ms, not %lu", took, (unsigned long)WAIT);
		CHECK(store.size == expected(LEAST + 5, MOST), "after the wait: a store of %lu, not %lu", store.size,
			expected(LEAST + 5, MOST));
		/* a frame writing a quarter or more every 10 s: kept */
		reset(8 * LEAST, MOST);
		for (count = 0; count < 4 * (long)(WAIT / step); count++)
			frame(count % (long)(10000 / step) == 0 ? 2 * LEAST : LEAST / 3);
		CHECK(store.size == 8 * LEAST, "a quarter written every 10 s: made %lu, not kept", store.size);
		/* the least is never made smaller */
		reset(LEAST, MOST);
		for (count = 0; count < 4 * (long)(WAIT / step); count++)
			frame(0);
		CHECK(store.size == LEAST, "idle at the least: made %lu", store.size);
	}
	CASE("frame rates")
	{
		/* the same wait at 300, 60 and 20 frames a second, and at one */
		static const unsigned long steps[] = { 3, 16, 50, 1000 };
		int index;

		for (index = 0; index < (int)(sizeof(steps) / sizeof(steps[0])); index++)
		{
			unsigned long start, took;

			step = steps[index];
			reset(8 * LEAST, MOST);
			start = now;
			took = until_changed(LEAST / 3, start);
			CHECK(took >= WAIT && took < WAIT + step, "%lu ms a frame: made smaller after %lu ms, not %lu",
				step, took, (unsigned long)WAIT);
		}
	}
	CASE("scenes")
	{
		/* a crowd, a small scene for long, the crowd again (as 32x, 1x, 32x
		in a game): grown at the crowd's first frame both times, shrunk in
		between, once a whole wait has passed with only the small scene's
		frames (the wait the crowd's frame is in keeps the store) */
		unsigned long crowd = 1100 * 1024, small = 100 * 1024, start = now, took;

		reset(LEAST, MOST);
		frame(crowd);
		CHECK(store.size == expected(crowd, MOST), "the crowd's first frame: %lu", store.size);
		took = until_changed(small, start);
		CHECK(took >= 2 * WAIT && took < 2 * WAIT + step, "made smaller %lu ms after the crowd, not %lu", took,
			2 * (unsigned long)WAIT);
		CHECK(store.size == expected(small, MOST), "after the small scene: %lu, not %lu", store.size,
			expected(small, MOST));
		frame(crowd);
		CHECK(store.size == expected(crowd, MOST), "the crowd again: %lu", store.size);
	}
	return 0;
}
