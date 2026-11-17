/*
SOUND_PLAYED_BEYOND.C (test)

The real sound_definitions.c picking a pitch range's next permutation
(sound_definition_next_permutation, OpenH1 issue 1170): a range
that chooses from more than 32 permutations (a Custom Edition sound's) plays
every one of them before any repeats, its played bits past 32 kept apart
from the map's structure and reset with it as a map loads, the entries only
ever the current map's (map after map, none shares one); a range of 32 or
fewer picks as the game did, draw for draw (the same picks and the same
seed after each, against the code before, copied here); and the pieces of a
linked permutation are never picked to start a
sound. test_sound_played_beyond.py takes the code (under_test.inc) and the
game's structures (types.inc).
*/

#include "harness.h"
#include <stddef.h>
#include <stdint.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define VALID_INDEX(i, c) ((i) >= 0 && (i) < (c))
#define SET_FLAG(f, b, v) ((f) = (v) ? (f) | (1U << (b)) : (f) & ~(1U << (b)))
#define TAG_BLOCK_GET_ELEMENT(b, i, t) (&((t *)(b)->address)[i])
#define _error_silent 0
#define error(priority, ...) ((void)(priority))
typedef void *byte_swap_data_proc;

#include "types.inc"

/* the game's seeded random (real_math.c's: a linear congruential step) */
static short seed_random_range(unsigned long *seed, short lower, short upper)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (short)(lower + (short)(((*seed >> 16) & 0xFFFF) * (unsigned long)(upper - lower) >> 16));
}
static real real_seed_random(unsigned long *seed)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (real)((*seed >> 16) & 0xFFFF) / 65536.f;
}

static boolean fail_allocation;
static void *test_realloc(void *pointer, size_t size)
{
    return fail_allocation ? NULL : realloc(pointer, size);
}
#define realloc test_realloc
static unsigned long *current_seed;
static unsigned long *get_global_local_random_seed_address(void) { return current_seed; }

#include "under_test.inc"

/* ---------- the pick before issue 1170 (master 617995457), for ranges of 32 or fewer */

static void old_try_to_reset_permutations(struct sound_pitch_range *range)
{
	short permutation_count = range->actual_permutation_count;
	unsigned long all_permutations_mask = permutation_count >= 32 ? 0xFFFFFFFFUL :
		permutation_count > 0 ? (FLAG(permutation_count) - 1) : 0;

	if ((~range->played_permutation_mask & all_permutations_mask) == 0)
	{
		range->played_permutation_mask = 0;
		if (permutation_count > 1 && range->previous_permutation_index >= 0 &&
			range->previous_permutation_index < 32)
		{
			range->played_permutation_mask = FLAG(range->previous_permutation_index);
		}
	}
}

/* (its random pick: a sound's start, no forced permutation) */
static short old_pick(struct sound_pitch_range *range, unsigned long *seed)
{
	short selected_permutation_index = seed_random_range(seed, 0, range->actual_permutation_count);
	short attempt_count = 0;

	for (;;)
	{
		old_try_to_reset_permutations(range);
		if (!TEST_FLAG(range->played_permutation_mask, selected_permutation_index))
		{
			SET_FLAG(range->played_permutation_mask, selected_permutation_index, TRUE);
			if (attempt_count++ == 16)
				break;
			if (real_seed_random(seed) >=
				TAG_BLOCK_GET_ELEMENT(&range->permutations, selected_permutation_index,
					struct sound_permutation)->skip_fraction)
				break;
		}
		selected_permutation_index++;
		if (selected_permutation_index == range->actual_permutation_count)
			selected_permutation_index = 0;
	}
	range->previous_permutation_index = selected_permutation_index;
	return selected_permutation_index;
}

/* ---------- a sound */

#define MOST MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE

struct test_sound
{
	struct sound_definition definition;
	struct sound_pitch_range range;
	struct sound_permutation permutations[MOST];
};

/* a sound of one range choosing from `actual` of `count` permutations (those
past `actual` the continuations of a linked one), as the
validator leaves it */
static void sound_make(struct test_sound *sound, short actual, short count, real skip_step)
{
	short index;

	memset(sound, 0, sizeof(*sound));
	sound->definition.pitch_ranges.count = 1;
	sound->definition.pitch_ranges.address = &sound->range;
	sound->range.actual_permutation_count = actual;
	sound->range.played_permutation_mask = 0;
	sound->range.played_beyond_index = 0;
	sound->range.previous_permutation_index = NONE;
	sound->range.forced_permutation_index = NONE;
	sound->range.permutations.count = count;
	sound->range.permutations.address = sound->permutations;
	for (index = 0; index < count; index++)
	{
		struct sound_permutation *permutation = &sound->permutations[index];

		permutation->next_permutation_index = NONE;
		permutation->skip_fraction = skip_step * (real)(index % 4);
	}
	/* (each piece follows the one before it, from the last actual one) */
	if (count > actual)
	{
		for (index = actual - 1; index < count - 1; index++)
			sound->permutations[index].next_permutation_index = (short)(index + 1);
	}
}

static short pick(struct test_sound *sound, unsigned long *seed)
{
	current_seed = seed;
	return sound_definition_next_permutation(&sound->definition, 0, NONE);
}

/* `count` picks of `sound`, each a different one of its `actual` permutations
and none `not` */
static short picks_distinct(struct test_sound *sound, unsigned long *seed, short count, short not)
{
	boolean seen[MOST] = { 0 };
	short index;
	short last = NONE;

	for (index = 0; index < count; index++)
	{
		last = pick(sound, seed);
		CHECK(VALID_INDEX(last, sound->range.actual_permutation_count), "pick %d is %d", index, last);
		CHECK(!seen[last] && last != not, "pick %d repeats permutation %d", index, last);
		seen[last] = TRUE;
	}
	return last;
}

int main(int argc, char **argv)
{
	static struct test_sound sound, old;
	char const *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "forty"))
	{
		/* every one of 40 before any repeats, round after round; the first
		of a round is never the last of the one before */
		unsigned long seed, initial_seed;

		for (initial_seed = 1; initial_seed <= 64; initial_seed++)
		{
			short last;
			short round;

			seed = initial_seed;
			sound_make(&sound, 40, 40, 0.f);
			last = picks_distinct(&sound, &seed, 40, NONE);
			for (round = 0; round < 4; round++)
				last = picks_distinct(&sound, &seed, 39, last);
		}
		/* and of the most a range can have */
		seed = 7;
		sound_make(&sound, MOST, MOST, 0.f);
		picks_distinct(&sound, &seed, MOST, NONE);
	}
	else if (!strcmp(name, "same-as-old"))
	{
		/* 32 or fewer: the same picks, and the same seed after each, as before */
		short count;

		for (count = 1; count <= 32; count++)
		{
			int variant;

			for (variant = 0; variant < 6; variant++)
			{
				unsigned long seed = 0x1234567UL * (unsigned long)(variant + 1) + (unsigned long)count;
				unsigned long old_seed = seed;
				real skip = variant % 3 == 0 ? 0.f : variant % 3 == 1 ? 0.3f : 0.6f;
				short pieces = variant >= 3 ? 3 : 0;
				int index;

				sound_make(&sound, count, (short)(count + pieces), skip);
				sound_make(&old, count, (short)(count + pieces), skip);
				for (index = 0; index < 400; index++)
				{
					short new_pick = pick(&sound, &seed);
					short old_pick_index = old_pick(&old.range, &old_seed);

					CHECK(new_pick == old_pick_index && seed == old_seed &&
						sound.range.played_permutation_mask == old.range.played_permutation_mask,
						"count %d, variant %d, pick %d: %d (seed %08lx) where the game picked %d (seed %08lx)",
						count, variant, index, new_pick, seed, old_pick_index, old_seed);
				}
				CHECK(sound.range.played_beyond_index == 0, "a range of %d keeps bits past 32", count);
			}
		}
	}
	else if (!strcmp(name, "pieces"))
	{
		/* a split permutation's pieces (past the actual ones) never start a
		sound, in a range of more than 32 or fewer */
		static short const actuals[] = { 40, 20, 33 };
		int which;

		for (which = 0; which < 3; which++)
		{
			unsigned long seed = 99;
			int index;

			sound_make(&sound, actuals[which], (short)(actuals[which] + 6), 0.f);
			for (index = 0; index < 1000; index++)
			{
				short picked = pick(&sound, &seed);

				CHECK(VALID_INDEX(picked, actuals[which]), "%d of %d picks a piece: %d", index, actuals[which], picked);
			}
			/* (and a piece is followed by the next piece, as before) */
			sound.definition.flags |= FLAG(1);
			CHECK(sound_definition_next_permutation(&sound.definition, 0, (short)(actuals[which] - 1)) == actuals[which], "the last actual one is not followed by its first piece");
		}
	}
	else if (!strcmp(name, "reloaded"))
	{
		/* a range whose map loads again (the validator resets its played mask
		and index, then sound_initialize_for_new_map) starts with none
		played, past 32 too */
		unsigned long seed = 5;
		int index;

		sound_definitions_played_beyond_clear();
		sound_make(&sound, 40, 40, 0.f);
		for (index = 0; index < 25; index++)
			pick(&sound, &seed);
		sound.range.played_permutation_mask = 0;
		sound.range.played_beyond_index = 0;
		sound.range.previous_permutation_index = NONE;
		sound_definitions_played_beyond_clear();
		picks_distinct(&sound, &seed, 40, NONE);
	}
	else if (!strcmp(name, "map-after-map"))
	{
		/* map after map, more than the indices a range has (a short): each
		map's two ranges of 40, at addresses earlier maps' had, have
		indices of their own, each its own entry, and each plays all 40
		before any repeats */
		static struct test_sound sounds[8];
		unsigned long seed = 11;
		long map;

		for (map = 0; map < 3L * MAXIMUM_SOUND_PLAYED_BEYOND; map++)
		{
			struct test_sound *first = &sounds[map % 8];
			struct test_sound *second = &sounds[(map + 3) % 8];

			sound_make(first, 40, 40, 0.f);
			sound_make(second, 40, 40, 0.f);
			sound_definitions_played_beyond_clear();
			picks_distinct(first, &seed, 40, NONE);
			picks_distinct(second, &seed, 40, NONE);
			CHECK(first->range.played_beyond_index > 0 && second->range.played_beyond_index > 0 &&
				first->range.played_beyond_index != second->range.played_beyond_index,
				"map %ld: its ranges have indices %d and %d", map, first->range.played_beyond_index,
				second->range.played_beyond_index);
			CHECK(sound_played_beyond_table.count == 2 &&
				sound_played_beyond_table.entries[first->range.played_beyond_index - 1].range == &first->range &&
				sound_played_beyond_table.entries[second->range.played_beyond_index - 1].range == &second->range,
				"map %ld: %ld entries, not its two ranges'", map, sound_played_beyond_table.count);
		}
	}
    else if (!strcmp(name, "allocation-failure"))
    {
        unsigned long seed = 17;
        sound_make(&sound, 40, 40, 0.f);
        fail_allocation = TRUE;
        picks_distinct(&sound, &seed, 32, NONE);
        CHECK(sound.range.actual_permutation_count == 32 && sound.range.played_beyond_index == 0,
            "allocation failure must use only the original mask");
        CHECK(sound_played_beyond_table.count == 0, "failed allocation retained an entry");
    }
    else if (!strcmp(name, "table-full"))
    {
        struct sound_pitch_range *ranges = calloc(MAXIMUM_SOUND_PLAYED_BEYOND, sizeof(*ranges));
        unsigned long seed = 19;
        long index;
        CHECK(ranges, "no memory for ranges");
        for (index = 0; index < MAXIMUM_SOUND_PLAYED_BEYOND; index++)
            CHECK(sound_pitch_range_played_beyond(&ranges[index]), "entry %ld was refused", index);
        CHECK(ranges[MAXIMUM_SOUND_PLAYED_BEYOND - 1].played_beyond_index == MAXIMUM_SOUND_PLAYED_BEYOND,
            "last index wrapped");
        sound_make(&sound, 40, 40, 0.f);
        picks_distinct(&sound, &seed, 32, NONE);
        CHECK(sound.range.actual_permutation_count == 32, "full table did not fall back");
        CHECK(sound_pitch_range_played_beyond(&ranges[0]) == sound_played_beyond_table.entries[0].bits,
            "full table lost an existing range");
        sound_definitions_played_beyond_clear();
        free(ranges);
    }

	else
	{
		CHECK(FALSE, "no case %s", name);
	}
	return 0;
}
