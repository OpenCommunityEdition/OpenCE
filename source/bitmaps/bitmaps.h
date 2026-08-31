/*
BITMAPS.H

header included in hcex build.
*/

#ifndef __BITMAPS_H
#define __BITMAPS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct bitmap_data;

/* ---------- prototypes/BITMAPS.C */

boolean bitmap_verify(
	struct bitmap_data *bitmap,
	boolean repair);

void *bitmap_2d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short mipmap_index);

/* ---------- prototypes/BITMAPS_QUANTITIZE.C */

void bitmap_quantitize(
	struct bitmap_data *bitmap,
	short const *bits_per_channel);

/* ---------- prototypes/BITMAP_UTILITIES.C */

real real_rgb_color_brightness(union real_rgb_color const *color);

union real_rgb_color *rgb_colors_interpolate(
	union real_rgb_color *rgb_result,
	unsigned long flags,
	union real_rgb_color const *rgb_lower_bound,
	union real_rgb_color const *rgb_upper_bound,
	real u);
union real_rgb_color *rgb_colors_interpolate_and_scale(
	union real_rgb_color *rgb_result,
	unsigned long flags,
	union real_argb_color const *argb_lower_bound,
	union real_argb_color const *argb_upper_bound,
	union real_rgb_color const *rgb_scale,
	real u);

/* ---------- globals */

/* ---------- public code */

#endif // __BITMAPS_H
