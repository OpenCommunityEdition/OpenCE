/*
FOG_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __FOG_DEFINITIONS_H
#define __FOG_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	FOG_TAG = 'fog ',
};

/* ---------- macros */

#define fog_definition_get(index) ((struct fog_definition *)tag_get(FOG_TAG, (index)))

/* ---------- structures */

struct fog_definition
{
	byte flags;
	byte unused1[0x73];
	real plane_distance;
};

typedef char fog_definition_size_assert[
	sizeof(struct fog_definition) == 0x78 ? 1 : -1];
typedef char fog_definition_plane_distance_offset_assert[
	offsetof(struct fog_definition, plane_distance) == 0x74 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __FOG_DEFINITIONS_H
