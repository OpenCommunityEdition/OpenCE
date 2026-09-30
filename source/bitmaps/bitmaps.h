/*
BITMAPS.H

header included in hcex build.
*/

#ifndef __BITMAPS_H
#define __BITMAPS_H
#pragma once

/* ---------- headers */

#include "math/integer_math.h"

/* ---------- constants */

enum
{
	NUMBER_OF_ENTRIES_IN_PALETTE = 256
};

/* ---------- macros */

/* ---------- structures */

struct bitmap_data;

/* ---------- prototypes/BITMAPS.C */

char const *bitmap_type_get_string(
	short type);
char const *bitmap_format_get_string(
	short format);
void bitmap_rebuild(
	struct bitmap_data *bitmap);
void bitmap_delete(
	struct bitmap_data *bitmap);

boolean bitmap_verify(
	struct bitmap_data *bitmap,
	boolean repair);
void bitmap_3d_slice_extract(
	struct bitmap_data *bitmap,
	short mipmap_index,
	short slice_index,
	struct bitmap_data *slice_bitmap);
void bitmap_3d_slice_insert(
	struct bitmap_data *slice_bitmap,
	struct bitmap_data *bitmap,
	short mipmap_index,
	short slice_index);
void bitmap_cube_map_face_extract(
	struct bitmap_data *bitmap,
	short mipmap_index,
	short face_index,
	struct bitmap_data *face_bitmap);
void bitmap_cube_map_face_insert(
	struct bitmap_data *face_bitmap,
	struct bitmap_data *bitmap,
	short mipmap_index,
	short face_index);

short bitmap_format_get_bits_per_pixel(
	short format);
struct bitmap_data *bitmap_2d_new(
	short width,
	short height,
	short mipmap_count,
	short format);
struct bitmap_data *bitmap_3d_new(
	short width,
	short height,
	short depth,
	short mipmap_count,
	short format);
struct bitmap_data *bitmap_cube_map_new(
	short width,
	short mipmap_count,
	short format);

void *bitmap_2d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short mipmap_index);
void *bitmap_3d_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short z,
	short mipmap_index);
void *bitmap_cube_map_address(
	struct bitmap_data *bitmap,
	short x,
	short y,
	short face_index,
	short mipmap_index);
void *bitmap_mipmap_address(
	struct bitmap_data *bitmap,
	short mipmap_index);
pixel32 bitmap_format_to_a8r8g8b8(
	short format,
	void const *mipmap_address,
	long pixel_index);
byte palette_find_closest_match(
	pixel32 const *palette,
	pixel32 color);
short bitmap_get_max_mipmap_count(
	struct bitmap_data *bitmap);
long bitmap_get_pixel_count(
	struct bitmap_data *bitmap);
long bitmap_get_pixel_data_size(
	struct bitmap_data *bitmap);
short bitmap_mipmap_get_width(
	struct bitmap_data *bitmap,
	short mipmap_index);
short bitmap_mipmap_get_height(
	struct bitmap_data *bitmap,
	short mipmap_index);
short bitmap_mipmap_get_depth(
	struct bitmap_data *bitmap,
	short mipmap_index);
long bitmap_mipmap_get_pixel_data_size(
	struct bitmap_data *bitmap,
	short mipmap_index);
long bitmap_mipmap_get_row_pitch(
	struct bitmap_data *bitmap,
	short mipmap_index);

/* ---------- prototypes/BITMAPS_QUANTITIZE.C */

void bitmap_quantitize(
	struct bitmap_data *bitmap,
	short const *bits_per_channel);

/* ---------- prototypes/BITMAP_UTILITIES.C */

void bitmap_vector_map(
	struct bitmap_data *bitmap);

real real_rgb_color_brightness(union real_rgb_color const *color);
union real_argb_color *pixel32_to_real_argb_color(
	pixel32 color,
	union real_argb_color *result);

boolean valid_real_rgb_color(
	union real_rgb_color const *color);

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

extern pixel32 global_vector_palette[NUMBER_OF_ENTRIES_IN_PALETTE];

/* ---------- public code */

#endif // __BITMAPS_H
