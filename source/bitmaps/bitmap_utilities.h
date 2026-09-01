/*
BITMAP_UTILITIES.H

Recovered public interface additions owned by BITMAP_UTILITIES.C.
*/

#ifndef __BITMAP_UTILITIES_H
#define __BITMAP_UTILITIES_H
#pragma once

#include "bitmaps/bitmaps.h"

void bitmap_fill(
	struct bitmap_data *bitmap,
	pixel32 fill_color);
void bitmap_alpha_to_rgb(
	struct bitmap_data *bitmap);
struct bitmap_data *bitmap_clone(
	struct bitmap_data *source_bitmap);
void bitmap_fade(
	struct bitmap_data *bitmap,
	pixel32 fade_color,
	real fade_amount);
struct bitmap_data *bitmap_shrink(
	struct bitmap_data *source_bitmap,
	short scale,
	short alpha_bias,
	boolean ignore_transparent_pixels);
void bitmap_alpha_bleed(
	struct bitmap_data *bitmap,
	short passes);
void bitmap_height_map(
	struct bitmap_data *bitmap,
	real bump_height);
void bitmap_compress_to_mipmap(
	struct bitmap_data *source_bitmap,
	struct bitmap_data *destination_bitmap,
	short destination_mipmap_index,
	boolean dither);
void bitmap_uncompress_from_mipmap(
	struct bitmap_data *source_bitmap,
	struct bitmap_data *destination_bitmap,
	short source_mipmap_index);

#endif /* __BITMAP_UTILITIES_H */
