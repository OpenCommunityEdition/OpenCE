/*
SOUND_DEFINITIONS.H

file has inline function assertions.
*/

#ifndef __SOUND_DEFINITIONS_H
#define __SOUND_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	SOUND_DEFINITION_TAG = 'snd!',
	SOUND_DEFINITION_VERSION = 4,
	MAXIMUM_PROMOTION_RULES_PER_SOUND = 4,
	MAXIMUM_PITCH_RANGES_PER_SOUND = 8,
	MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE = 256,
	MAXIMUM_PERMUTATIONS_PER_RANDOM_PITCH_RANGE = 32,
	MAXIMUM_SOUND_DATA_SIZE = 0x400000,
	MAXIMUM_SOUND_MOUTH_DATA_SIZE = 8192,
	SOUND_MOUTH_SAMPLES_PER_SECOND = 30,
	MAXIMUM_SOUND_SUBTITLE_DATA_SIZE = 512,
	SOUND_COMPRESSION_BLOCK_SIZE = 64,
};

enum
{
	LOOPING_SOUND_DEFINITION_TAG = 'lsnd',
	LOOPING_SOUND_DEFINITION_VERSION = 3,
	CUSTOM_MUSIC_PLAY_ID = 'mply',
	MAXIMUM_TRACKS_PER_LOOPING_SOUND = 4,
	MAXIMUM_DETAIL_SOUNDS_PER_LOOPING_SOUND = 32,
};

/* ---------- macros */

#define sound_definition_get(index) ((struct sound_definition *)tag_get(SOUND_DEFINITION_TAG, (index)))
#define looping_sound_definition_get(index) ((struct looping_sound_definition *)tag_get(LOOPING_SOUND_DEFINITION_TAG, (index)))

/* ---------- structures */

struct real_bounds
{
	real lower;
	real upper;
};

struct sound_permutation
{
	char name[32];
	real skip_fraction;
	real gain;
	short compression;
	short next_permutation_index;
	long unknown0;
	unsigned long unknown1;
	unsigned long unknown2;
	unsigned long sample_buffer_size;
	unsigned long unknown3;
	struct tag_data samples;
	struct tag_data mouth_data;
	struct tag_data subtitle_data;
};

struct sound_pitch_range
{
	char name[32];
	real natural_pitch;
	struct real_bounds bend_bounds;
	short actual_permutation_count;
	short unused;
	real playback_rate;
	unsigned long played_permutation_mask;
	short previous_permutation_index;
	short forced_permutation_index;
	struct tag_block permutations;
};

struct sound_definition
{
	unsigned long flags;
	short sound_class;
	short sample_rate;
	real minimum_distance;
	real maximum_distance;
	real skip_fraction;
	struct real_bounds random_pitch_bounds;
	real inner_cone_angle;
	real outer_cone_angle;
	real outer_cone_gain;
	real gain_modifier;
	real maximum_bend_per_second;
	byte unused0[12];
	real zero_skip_fraction_modifier;
	real zero_gain_modifier;
	real zero_pitch_modifier;
	byte unused1[12];
	real one_skip_fraction_modifier;
	real one_gain_modifier;
	real one_pitch_modifier;
	byte unused2[12];
	short encoding;
	short compression;
	struct tag_reference promotion_sound;
	short promotion_count;
	short unknown0;
	unsigned long longest_permutation_length;
	unsigned long promotion_counter;
	unsigned long promotion_time;
	unsigned long scripting_time;
	long scripting_sound_index;
	struct tag_block pitch_ranges;
};

struct looping_sound_definition
{
	unsigned long flags;
	byte unused04[0x18];
	long scripting_sound_index;
};

typedef char looping_sound_definition_scripting_sound_index_offset_assert[
	offsetof(struct looping_sound_definition, scripting_sound_index) == 0x1C ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

real sound_definition_get_maximum_distance(
	long sound_index);
real sound_definition_get_minimum_distance(
	long sound_index);

short sound_definition_find_pitch_range_by_pitch(
	struct sound_definition *definition,
	real pitch,
	long pitch_range_index);

short sound_definition_next_permutation(
	struct sound_definition *definition,
	short pitch_range_index,
	short permutation_index);

#endif // __SOUND_DEFINITIONS_H
