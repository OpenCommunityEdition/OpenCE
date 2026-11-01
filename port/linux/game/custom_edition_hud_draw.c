/* CE draws without a converted placement. Adapted from ChupathingyCE
 * #352, by Boom Pen / ugoboom, and #545, by Milenko (GPL-3.0-or-later). See
 * docs/custom_edition_caches.md for provenance and the map-family boundary.
 */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "custom_edition_hud_scale.h"
#include "custom_edition_hud_draw.h"

enum { _bitmap_linear_bit = 4 };

real custom_edition_hud_bitmap_scale(long bitmap_group_index)
{
	if (!custom_edition_cache_tags_loaded() || bitmap_group_index == NONE)
		return 1.0f;
	return custom_edition_hud_scale(bitmap_group_get(bitmap_group_index)->flags, FALSE);
}

real custom_edition_hud_number_scale(long bitmap_group_index, unsigned short placement_flags)
{
	boolean high_resolution;

	if (!custom_edition_cache_tags_loaded() || bitmap_group_index == NONE)
		return 1.0f;
	/* The known-map number fix already halves the digit metrics and sets
	the bitmap's half-scale flag. Its placement must not halve them again. */
	high_resolution = (placement_flags & 4) &&
		!custom_edition_cache_relies_on(_custom_edition_behaviour_hud_number_scale);
	return custom_edition_hud_scale(bitmap_group_get(bitmap_group_index)->flags, high_resolution);
}

/* Borrowed only across interface_draw_screen's synchronous screen-effect
draw. End the scope before another view, cinematic or map can draw. */
static struct bitmap_data const *hud_mask;
static real hud_mask_scale = 1.0f;

void custom_edition_hud_mask_begin(struct bitmap_data const *bitmap)
{
	hud_mask = custom_edition_cache_tags_loaded() ? bitmap : NULL;
	hud_mask_scale = hud_mask ? custom_edition_hud_bitmap_scale(hud_mask->tag_index) : 1.0f;
}

void custom_edition_hud_mask_end(void)
{
	hud_mask = NULL;
	hud_mask_scale = 1.0f;
}

void custom_edition_hud_mask_transform(struct bitmap_data const *bitmap,
	real_vector2d *size, real_vector2d *scale)
{
	if (!custom_edition_cache_tags_loaded() || !bitmap || bitmap != hud_mask || hud_mask_scale == 1.0f)
		return;
	size->i *= hud_mask_scale;
	size->j *= hud_mask_scale;
	if (TEST_FLAG(bitmap->flags, _bitmap_linear_bit))
	{
		scale->i = 1.0f / hud_mask_scale;
		scale->j = 1.0f / hud_mask_scale;
	}
	else
	{
		scale->i = 1.0f / size->i;
		scale->j = 1.0f / size->j;
	}
}
