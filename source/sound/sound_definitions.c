/*
SOUND_DEFINITIONS.C

symbols in this file:
001B8800 0040:
	_sound_definition_get_maximum_distance (0000)
001B8840 0040:
	_sound_definition_get_minimum_distance (0000)
001B8880 0050:
	_sound_permutation_get_mouth_aperture (0000)
001B88D0 0100:
	_sound_definition_find_pitch_range_by_pitch (0000)
001B89D0 0040:
	_try_to_reset_permutations (0000)
001B8A10 0060:
	_sound_permutation_get_real_mouth_aperture (0000)
001B8A70 0160:
	_sound_definition_next_permutation (0000)
002AB1D0 000c:
	_sound_sample_rate_samples_per_second (0000)
	_oo_unsigned_char_max (0008)
002AB1DC 0039:
	??_C@_0DJ@MJNMCFCL@tick_index?$DO?$DN0?5?$CG?$CG?5tick_index?$DMperm@ (0000)
002AB218 0029:
	??_C@_0CJ@KBJMLIGI@c?3?2halo?2SOURCE?2sound?2sound_defin@ (0000)
002AB248 0042:
	??_C@_0EC@IMDGFJGB@but?5how?5can?5you?5speak?5if?5you?5hav@ (0000)
002AB28C 001a:
	??_C@_0BK@DPJOOPFK@range?9?$DOpermutations?4count?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "sound_classes.h"
#include "sound_definitions.h"

#include <stdlib.h> /* port: the extra played-permutation bits */

/* ---------- constants */

enum
{
	/* port: a pitch range's played permutations past the 32 bits of its
	played_permutation_mask, up to every permutation a range can have
	(OpenH1 issue 1170) */
	SOUND_PLAYED_BEYOND_LONGS = (MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE - 32 + 31) / 32,
	/* (the ranges' played_beyond_index is a short: the most ranges of more
	than 32 a map may have, the rest picking from their first 32) */
	MAXIMUM_SOUND_PLAYED_BEYOND = 0x7FFF,
};

/* ---------- macros */

/* ---------- structures */

/* port: the played bits of a pitch range's permutations 32 on, bit n of
bits[n / 32] for permutation 32 + n (sound_pitch_range_played_beyond) */
struct sound_played_beyond
{
	struct sound_pitch_range const *range;
	unsigned long bits[SOUND_PLAYED_BEYOND_LONGS];
};

/* port: the current map's (sound_definitions_played_beyond_clear) */
struct sound_played_beyond_table
{
	struct sound_played_beyond *entries;
	long count;
	long capacity;
};

/* ---------- prototypes */

/* ---------- globals */

static struct sound_played_beyond_table sound_played_beyond_table;

unsigned long const sound_sample_rate_samples_per_second[2] =
{
	22050,
	44100
};

real const oo_unsigned_char_max = 1.f / 255.f;

/* ---------- public code */

real sound_definition_get_maximum_distance(
	long sound_index)
{
	struct sound_definition *definition = sound_definition_get(sound_index);
	real maximum_distance = definition->maximum_distance;

	if (maximum_distance == 0.f)
		maximum_distance = sound_class_get(definition->sound_class)->maximum_distance;

	return maximum_distance;
}

real sound_definition_get_minimum_distance(
	long sound_index)
{
	struct sound_definition *definition = sound_definition_get(sound_index);
	real minimum_distance = definition->minimum_distance;

	if (minimum_distance == 0.f)
		minimum_distance = sound_class_get(definition->sound_class)->minimum_distance;

	return minimum_distance;
}

byte *sound_permutation_get_mouth_aperture(
	struct sound_permutation *permutation,
	short tick_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_definitions.c",
		800,
		tick_index>=0 && tick_index<permutation->mouth_data.size);

	return (byte *)permutation->mouth_data.address + tick_index;
}

short sound_definition_find_pitch_range_by_pitch(
	struct sound_definition *definition,
	real pitch,
	short old_range_index)
{
	short result = NONE;

	if (old_range_index != NONE && old_range_index < definition->pitch_ranges.count)
	{
		struct sound_pitch_range *range = TAG_BLOCK_GET_ELEMENT(
			&definition->pitch_ranges,
			old_range_index,
			struct sound_pitch_range);

		if (range->bend_bounds.lower <= pitch &&
			pitch <= range->bend_bounds.upper &&
			range->permutations.count)
		{
			result = old_range_index;
		}
	}

	if (result == NONE)
	{
		real closest_pitch_ratio = FLT_MAX;
		short range_index;

		for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++)
		{
			struct sound_pitch_range *range = TAG_BLOCK_GET_ELEMENT(
				&definition->pitch_ranges,
				range_index,
				struct sound_pitch_range);

			if (range->permutations.count)
			{
				/* port: the first range with permutations if no pitch ratio
				picks one (a NaN pitch, or zero pitch or bounds, from the map's
				values); NONE would index the ranges at -1 */
				if (result == NONE)
					result = range_index;

				if (range->bend_bounds.lower <= pitch && pitch <= range->bend_bounds.upper)
				{
					result = range_index;
					break;
				}
				else
				{
					real pitch_ratio = range->bend_bounds.upper < pitch ?
						pitch / range->bend_bounds.upper :
						range->bend_bounds.lower / pitch;

					if (pitch_ratio < closest_pitch_ratio)
					{
						result = range_index;
						closest_pitch_ratio = pitch_ratio;
					}
				}
			}
		}
	}

	return result;
}

void try_to_reset_permutations(
	struct sound_pitch_range *range)
{
	short permutation_count = range->actual_permutation_count;
	/* port: the mask of no more permutations than it has bits, and the
	previous one's bit only if there was one (NONE shifted by 255) */
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

	return;
}

/* port: a new map's ranges have none of their played bits past 32 yet
(sound_initialize_for_new_map): the entries are only ever the ranges of the
map loaded, so they never run out as map follows map, and an index is never
another range's */
void sound_definitions_played_beyond_clear(
	void)
{
	sound_played_beyond_table.count = 0;

	return;
}

/* port: the played bits of `range`'s permutations 32 on, for a range that
chooses from more than 32 (a Custom Edition sound's, or a mod's: OpenH1
issue 1170), or NULL if there is no room for them (past
MAXIMUM_SOUND_PLAYED_BEYOND ranges in a map, or no memory). The range is
the map's structure, its layout the tags', so they are kept here: the
range's played_beyond_index is its entry, one on, 0 for none. The validator
(tag_schema_effects.c) and the mods' loader (mod_sounds.c) set it to 0 as
they set the range's played_permutation_mask, so a range loaded starts with
none played, as its first 32 do; the entries are the current map's
(sound_definitions_played_beyond_clear). */
static unsigned long *sound_pitch_range_played_beyond(
	struct sound_pitch_range *range)
{
	struct sound_played_beyond_table *table = &sound_played_beyond_table;
	long index = (long)range->played_beyond_index - 1;
	long word;

	if (index >= 0 && index < table->count && table->entries[index].range == range)
		return table->entries[index].bits;

	if (table->count == MAXIMUM_SOUND_PLAYED_BEYOND)
		return NULL;
	if (table->count == table->capacity)
	{
		long capacity = table->capacity ? MIN(table->capacity * 2, (long)MAXIMUM_SOUND_PLAYED_BEYOND) : 16;
		struct sound_played_beyond *grown = realloc(table->entries, (size_t)capacity * sizeof(*grown));

		if (!grown)
			return NULL;
		table->entries = grown;
		table->capacity = capacity;
	}
	index = table->count++;
	table->entries[index].range = range;
	for (word = 0; word < SOUND_PLAYED_BEYOND_LONGS; word++)
		table->entries[index].bits[word] = 0;
	range->played_beyond_index = (short)(index + 1);

	return table->entries[index].bits;
}

/* port: whether permutation `index` of `range` has played: one of the first
32 in its played_permutation_mask, as the game kept them, one past them in
`beyond` (sound_pitch_range_played_beyond: NULL for a range of 32 or fewer,
which never asks past 32) */
static boolean sound_pitch_range_permutation_played(
	struct sound_pitch_range const *range,
	unsigned long const *beyond,
	short index)
{
	return index < 32 ?
		TEST_FLAG(range->played_permutation_mask, index) :
		((beyond[(index - 32) >> 5] >> ((index - 32) & 31)) & 1) != 0;
}

static void sound_pitch_range_permutation_set_played(
	struct sound_pitch_range *range,
	unsigned long *beyond,
	short index)
{
	if (index < 32)
		SET_FLAG(range->played_permutation_mask, index, TRUE);
	else
		beyond[(index - 32) >> 5] |= 1UL << ((index - 32) & 31);

	return;
}

/* port: try_to_reset_permutations for a range of more than 32 permutations:
once every one of them has played, none has but the one played last */
static void try_to_reset_permutations_beyond(
	struct sound_pitch_range *range,
	unsigned long *beyond)
{
	short permutation_count = range->actual_permutation_count;
	short index;

	for (index = 0; index < permutation_count; index++)
	{
		if (!sound_pitch_range_permutation_played(range, beyond, index))
			return;
	}
	range->played_permutation_mask = 0;
	for (index = 0; index < SOUND_PLAYED_BEYOND_LONGS; index++)
		beyond[index] = 0;
	if (range->previous_permutation_index >= 0 && range->previous_permutation_index < permutation_count)
		sound_pitch_range_permutation_set_played(range, beyond, range->previous_permutation_index);

	return;
}

real sound_permutation_get_real_mouth_aperture(
	struct sound_permutation *permutation,
	long tick_index)
{
	/* port: and a size above none (the map's: a negative one pinned the
	tick to before the data) */
	if (permutation->mouth_data.size > 0)
	{
		long aperture = *sound_permutation_get_mouth_aperture(
			permutation,
			PIN((short)tick_index, 0, permutation->mouth_data.size - 1));

		return aperture * oo_unsigned_char_max;
	}

	/* port: each permutation once (asked every frame of every sound of a
	speech class, a few such sounds logged hundreds of lines a second, a
	line written and flushed each, and slowed the game) */
	{
		enum
		{
			MAXIMUM_SILENT_MOUTHS = 32,
		};
		static struct sound_permutation *silent_mouths[MAXIMUM_SILENT_MOUTHS];
		static long silent_mouth_count = 0;
		long index;

		for (index = 0; index < silent_mouth_count; index++)
		{
			if (silent_mouths[index] == permutation)
				return 0.f;
		}
		if (silent_mouth_count >= MAXIMUM_SILENT_MOUTHS)
			return 0.f;
		silent_mouths[silent_mouth_count++] = permutation;
	}
	error(
		_error_silent,
		"but how can you speak if you have no mouth data? (permutation %s)",
		permutation->name);

	return 0.f;
}

short sound_definition_next_permutation(
	struct sound_definition *definition,
	short pitch_range_index,
	short permutation_index)
{
	struct sound_pitch_range *range;
	short selected_permutation_index;
	short attempt_count = 0;
	unsigned long *beyond = NULL;

	/* port: no range (NONE, from a sound none of whose ranges has
	permutations), no permutation */
	if (!VALID_INDEX(pitch_range_index, definition->pitch_ranges.count))
		return NONE;

	range = TAG_BLOCK_GET_ELEMENT(
		&definition->pitch_ranges,
		pitch_range_index,
		struct sound_pitch_range);

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_definitions.c",
		892,
		range->permutations.count);

	/* port: every index this returns is one of the range's permutations
	(the counts and indices are the map's, and the sound cache writes
	through the permutation). A bad one is fixed in the tag, so it is said
	once. */
	if (range->permutations.count <= 0)
		return NONE;
	if (range->actual_permutation_count < 1 ||
		range->actual_permutation_count > range->permutations.count)
	{
		error(
			_error_silent,
			"pitch range %s picks from %d of its %d permutations",
			range->name,
			range->actual_permutation_count,
			range->permutations.count);
		range->actual_permutation_count = (short)PIN(
			range->actual_permutation_count,
			1,
			range->permutations.count);
	}
	if (range->forced_permutation_index != NONE &&
		!VALID_INDEX(range->forced_permutation_index, range->permutations.count))
	{
		error(
			_error_silent,
			"pitch range %s forces permutation %d of %d",
			range->name,
			range->forced_permutation_index,
			range->permutations.count);
		range->forced_permutation_index = NONE;
	}

	if (range->forced_permutation_index != NONE)
	{
		selected_permutation_index = range->forced_permutation_index;
		range->forced_permutation_index = NONE;
		range->previous_permutation_index = selected_permutation_index;
		return selected_permutation_index;
	}

	/* port: and the permutation is one of this range's */
	if (TEST_FLAG(definition->flags, 1) &&
		VALID_INDEX(permutation_index, range->permutations.count))
	{
		struct sound_permutation *permutation = TAG_BLOCK_GET_ELEMENT(
			&range->permutations,
			permutation_index,
			struct sound_permutation);

		/* port: NONE ends the chain; any other index past the range ends it too */
		if (permutation->next_permutation_index != NONE &&
			!VALID_INDEX(permutation->next_permutation_index, range->permutations.count))
		{
			error(
				_error_silent,
				"permutation %s is followed by permutation %d of %d",
				permutation->name,
				permutation->next_permutation_index,
				range->permutations.count);
			permutation->next_permutation_index = NONE;
		}

		return permutation->next_permutation_index;
	}

	/* port: a range of more than 32 permutations keeps the played bits of
	those past 32 apart (sound_pitch_range_played_beyond); one of 32 or
	fewer picks as the game did, draw for draw. Without room for them, it
	picks from its first 32. */
	if (range->actual_permutation_count > 32)
	{
		beyond = sound_pitch_range_played_beyond(range);
		if (!beyond)
		{
			error(
				_error_silent,
				"pitch range %s picks from 32 of its %d permutations: no room for the rest",
				range->name,
				range->actual_permutation_count);
			range->actual_permutation_count = 32;
		}
	}

	selected_permutation_index = seed_random_range(
		get_global_local_random_seed_address(),
		0,
		range->actual_permutation_count);

	for (;;)
	{
		if (beyond)
			try_to_reset_permutations_beyond(range, beyond);
		else
			try_to_reset_permutations(range);

		if (!sound_pitch_range_permutation_played(range, beyond, selected_permutation_index))
		{
			struct sound_permutation *permutation;

			sound_pitch_range_permutation_set_played(range, beyond, selected_permutation_index);
			if (attempt_count++ == 16)
				break;

			{
				real random = real_seed_random(get_global_local_random_seed_address());

				permutation = TAG_BLOCK_GET_ELEMENT(
				&range->permutations,
				selected_permutation_index,
				struct sound_permutation);
				if (random >= permutation->skip_fraction)
					break;
			}
		}

		selected_permutation_index++;
		if (selected_permutation_index == range->actual_permutation_count)
			selected_permutation_index = 0;
	}

	range->previous_permutation_index = selected_permutation_index;
	return selected_permutation_index;
}

/* ---------- private code */
