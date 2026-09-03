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
	unsigned long signature;
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
	unsigned long pixels_offset;
	unsigned long pixel_data_size;
	unsigned long bitmap_tag_index;
	void *bitmap_data_pointer;
	void *hardware_format;
	void *base_address;
};

struct bitmap_group
{
	short type;
	short format;
	short usage;
	unsigned short flags;
	real detail_fade;
	real sharpen_amount;
	real bump_height;
	short sprite_budget_size;
	unsigned short sprite_budget_count;
	short import_width;
	short import_height;
	struct tag_data import_bitmap;
	struct tag_data pixel_data;
	real smoothing_filter_size;
	real alpha_bias;
	unsigned short mipmap_count;
	short sprite_usage;
	short sprite_spacing;
	unsigned short unused;
	struct tag_block sequences;
	struct tag_block bitmap_data;
};

/* ---------- prototypes/BITMAP_GROUP.C */

struct bitmap_data *bitmap_group_get_bitmap_from_sequence(
	long bitmap_group_index,
	short sequence_index,
	short frame_index);

/* ---------- globals */

/* ---------- public code */

#endif // __BITMAP_GROUP_H
