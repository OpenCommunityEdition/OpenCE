/*
EFFECT_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __EFFECT_DEFINITIONS_H
#define __EFFECT_DEFINITIONS_H
#pragma once

#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	EFFECT_DEFINITION_TAG = 'effe',
	EFFECT_DEFINITION_VERSION = 4,
	MAXIMUM_EFFECT_PARTICLES_PER_EVENT = 32,
	MAXIMUM_EFFECT_PARTS_PER_EVENT = 32,
	MAXIMUM_EFFECT_LOCATIONS = 32,
	MAXIMUM_EFFECT_EVENTS = 32,
};

/* ---------- macros */

/* ---------- structures */

struct effect_definition
{
	long flags;
	short loop_start_index;
	short loop_stop_index;
	real runtime_danger_radius;
	real unused00c[7];
	struct tag_block locations;
	struct tag_block events;
};

typedef char effect_definition_size_assert[
	sizeof(struct effect_definition) == 0x40 ? 1 : -1];

struct effect_event_definition
{
	long flags;
	real skip_fraction;
	real delay_lower_bound;
	real delay_upper_bound;
	real duration_lower_bound;
	real duration_upper_bound;
	real unused018[5];
	struct tag_block parts;
	struct tag_block particles;
};

typedef char effect_event_definition_size_assert[
	sizeof(struct effect_event_definition) == 0x44 ? 1 : -1];

struct effect_part_definition
{
	short environment;
	short disposition;
	short location_index;
	word flags;
	long unused008[3];
	unsigned long runtime_base_class_tag;
	struct tag_reference reference;
	long unused028[6];
	real velocity_lower_bound;
	real velocity_upper_bound;
	real velocity_cone_angle;
	real angular_velocity_lower_bound;
	real angular_velocity_upper_bound;
	real radius_modifier_lower_bound;
	real radius_modifier_upper_bound;
	long unused05c;
	unsigned long scale_a_flags;
	unsigned long scale_b_flags;
};

typedef char effect_part_definition_size_assert[
	sizeof(struct effect_part_definition) == 0x68 ? 1 : -1];
typedef char effect_part_definition_reference_offset_assert[
	offsetof(struct effect_part_definition, reference) == 0x18 ? 1 : -1];
typedef char effect_part_definition_velocity_offset_assert[
	offsetof(struct effect_part_definition, velocity_lower_bound) == 0x40 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __EFFECT_DEFINITIONS_H
