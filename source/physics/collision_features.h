/*
COLLISION_FEATURES.H

header included in hcex build.
*/

#ifndef __COLLISION_FEATURES_H
#define __COLLISION_FEATURES_H
#pragma once

/* ---------- constants */

enum
{
	MAXIMUM_COLLISION_FEATURES_PER_TYPE = 256,
};

/* ---------- macros */

/* ---------- structures */

struct collision_feature_reference
{
	long object_index;
	long feature_index;
	byte type;
	byte flags;
	short material_index;
};

struct collision_point_feature
{
	struct collision_feature_reference reference;
	real_point3d point;
	long plane_designator;
};

struct collision_line_feature
{
	struct collision_feature_reference reference;
	real_point3d point0;
	real_point3d point1;
	long plane_designator;
};

struct collision_polygon_feature
{
	struct collision_feature_reference reference;
	real_plane3d plane;
	long plane_designator;
	short projection;
	byte projection_sign;
	byte pad;
	long point_count;
	real_point2d points[8];
};

struct collision_feature_list
{
	short point_count;
	short line_count;
	short polygon_count;
	short pad;
	struct collision_point_feature points[MAXIMUM_COLLISION_FEATURES_PER_TYPE];
	struct collision_line_feature lines[MAXIMUM_COLLISION_FEATURES_PER_TYPE];
	struct collision_polygon_feature polygons[MAXIMUM_COLLISION_FEATURES_PER_TYPE];
};

typedef char collision_feature_reference_size_assert[
	sizeof(struct collision_feature_reference) == 0x0C ? 1 : -1];
typedef char collision_point_feature_size_assert[
	sizeof(struct collision_point_feature) == 0x1C ? 1 : -1];
typedef char collision_line_feature_size_assert[
	sizeof(struct collision_line_feature) == 0x28 ? 1 : -1];
typedef char collision_polygon_feature_size_assert[
	sizeof(struct collision_polygon_feature) == 0x68 ? 1 : -1];
typedef char collision_feature_list_lines_offset_assert[
	offsetof(struct collision_feature_list, lines) == 0x1C08 ? 1 : -1];
typedef char collision_feature_list_polygons_offset_assert[
	offsetof(struct collision_feature_list, polygons) == 0x4408 ? 1 : -1];
typedef char collision_feature_list_size_assert[
	sizeof(struct collision_feature_list) == 0xAC08 ? 1 : -1];

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __COLLISION_FEATURES_H
