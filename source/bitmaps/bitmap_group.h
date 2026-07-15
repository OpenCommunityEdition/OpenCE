/*
BITMAP_GROUP.H

header included in hcex build.
*/

#ifndef __BITMAP_GROUP_H
#define __BITMAP_GROUP_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	BITMAP_GROUP_TAG = 'bitm',
};

/* ---------- macros */

#define bitmap_group_get(index) ((struct bitmap_group *)tag_get(BITMAP_GROUP_TAG, (index)))

/* ---------- structures */

struct bitmap_data
{
	unsigned long bitmap_class;
	short width;
	short height;
	unsigned short depth;
	short type;
	short format;
	unsigned short flags;
	short registration_point_x;
	short registration_point_y;
	unsigned short mipmap_count;
	unsigned short pixel_data;
	unsigned long pixel_data_offset;
	unsigned long pixel_data_size;
	unsigned long bitmap_tag_index;
	void *bitmap_data_pointer;
	void *hardware_format;
	void *base_address;
};

struct bitmap_group
{
	short type;
	short encoding_format;
	short usage;
	unsigned short flags;
	real detail_fade_factor;
	real sharpen_amount;
	real bump_height;
	short sprite_budget_size;
	unsigned short sprite_budget_count;
	unsigned short color_plate_width;
	unsigned short color_plate_height;
	struct tag_data compressed_color_plate_data;
	struct tag_data processed_pixel_data;
	real blur_filter_size;
	real alpha_bias;
	unsigned short mipmap_count;
	short sprite_usage;
	unsigned short sprite_spacing;
	unsigned short unused;
	struct tag_block sequences;
	struct tag_block bitmap_data;
};

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __BITMAP_GROUP_H
