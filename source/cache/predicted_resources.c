/*
PREDICTED_RESOURCES.C

symbols in this file:
001AD870 0090:
	_code_001ad870 (0000)
001AD900 0080:
	_predicted_resources_precache (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "cache/cache_files.h"
#include "cache/predicted_resources.h"
#include "cache/sound_cache.h"
#include "bitmaps/bitmap_group.h"
#include "cache/texture_cache.h"
#include "sound/sound_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static void predicted_resources_sound_precache(long sound_definition_index);
static boolean predicted_resource_valid(struct predicted_resource const *predicted_resource);

/* ---------- globals */

/* ---------- public code */

void predicted_resources_precache(
	struct tag_block *predicted_resources)
{
	/* port: long, as the count is (a short wrapped on a longer block and
	never ended) */
	long predicted_resource_index;

	for (predicted_resource_index = 0;
		predicted_resource_index < predicted_resources->count;
		predicted_resource_index++)
	{
		struct predicted_resource *predicted_resource;

		predicted_resource = TAG_BLOCK_GET_ELEMENT(
			predicted_resources,
			predicted_resource_index,
			struct predicted_resource);
		/* port: and one that names a resource of its type */
		if (!predicted_resource_valid(predicted_resource))
			continue;
		switch (predicted_resource->type)
		{
		case _predicted_resource_bitmap:
			_texture_cache_bitmap_get_hardware_format(
				TAG_BLOCK_GET_ELEMENT(
					&bitmap_group_get(predicted_resource->tag_index)->bitmaps,
					predicted_resource->resource_index,
					struct bitmap_data),
				FALSE,
				TRUE);
			break;

		case _predicted_resource_sound:
			predicted_resources_sound_precache(predicted_resource->tag_index);
			break;
		}
	}

	return;
}

/* ---------- private code */

/* port: a predicted resource's tag and bitmap are the map's, and the texture
and sound caches write through what they name: a tag of another group (or
none), or a bitmap the group doesn't have, is not precached, said once. Every
retail one names a tag of its type and one of its bitmaps. */
static boolean predicted_resource_valid(
	struct predicted_resource const *predicted_resource)
{
	static boolean bad_resource_reported = FALSE;
	boolean valid = TRUE;

	switch (predicted_resource->type)
	{
	case _predicted_resource_bitmap:
		valid = predicted_resource->tag_index != NONE &&
			tag_get_group_tag(predicted_resource->tag_index) == BITMAP_GROUP_TAG &&
			VALID_INDEX(
				predicted_resource->resource_index,
				bitmap_group_get(predicted_resource->tag_index)->bitmaps.count);
		break;

	case _predicted_resource_sound:
		valid = predicted_resource->tag_index != NONE &&
			tag_get_group_tag(predicted_resource->tag_index) == SOUND_DEFINITION_TAG;
		break;
	}

	if (!valid && !bad_resource_reported)
	{
		bad_resource_reported = TRUE;
		error(
			_error_silent,
			"predicted resource of type %d names tag %08x resource %d (not precached)",
			predicted_resource->type,
			predicted_resource->tag_index,
			predicted_resource->resource_index);
	}

	return valid;
}

static void predicted_resources_sound_precache(
	long sound_definition_index)
{
	struct sound_definition *sound_definition;
	struct tag_block *pitch_ranges;
	/* port: long, as the count is (see above) */
	long pitch_range_index;

	sound_definition = sound_definition_get(sound_definition_index);
	pitch_range_index = 0;
	if (sound_definition->pitch_ranges.count <= 0)
		return;
	pitch_ranges = &sound_definition->pitch_ranges;

pitch_range_loop:
	{
		struct sound_pitch_range *pitch_range;
		short permutation_index;

		pitch_range = TAG_BLOCK_GET_ELEMENT(pitch_ranges, pitch_range_index, struct sound_pitch_range);
		/* port: and no more than the range has (both counts are the map's) */
		for (permutation_index = 0;
			permutation_index < pitch_range->actual_permutation_count &&
				permutation_index < pitch_range->permutations.count;
			permutation_index++)
		{
			_sound_cache_sound_request(
				TAG_BLOCK_GET_ELEMENT(&pitch_range->permutations, permutation_index, struct sound_permutation),
				FALSE,
				TRUE,
				FALSE);
		}
		pitch_range_index++;
	}
	if (pitch_range_index < pitch_ranges->count)
		goto pitch_range_loop;

	return;
}
