#ifndef CUSTOM_EDITION_HUD_DRAW_H
#define CUSTOM_EDITION_HUD_DRAW_H

#include "math/real_math.h"

struct bitmap_data;

real custom_edition_hud_bitmap_scale(long bitmap_group_index);
real custom_edition_hud_number_scale(long bitmap_group_index, unsigned short placement_flags);
void custom_edition_hud_mask_begin(struct bitmap_data const *bitmap);
void custom_edition_hud_mask_end(void);
void custom_edition_hud_mask_transform(struct bitmap_data const *bitmap,
	real_vector2d *size, real_vector2d *scale);

#endif
