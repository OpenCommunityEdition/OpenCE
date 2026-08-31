/*
CONNECTED_GEOMETRY.C

symbols in this file:
000F2CD0 0030:
	_connected_geometry_new (0000)
000F2D00 0050:
	_connected_geometry_delete (0000)
000F2D50 00d0:
	_connected_geometry_group_recursive (0000)
000F2E20 00c0:
	_code_000f2e20 (0000)
000F2EE0 00f0:
	_code_000f2ee0 (0000)
000F2FD0 00b0:
	_plane3d_from_points (0000)
000F3080 01a0:
	_connected_geometry_add_triangle (0000)
000F3220 0180:
	_code_000f3220 (0000)
000F33A0 0080:
	_connected_geometry_add_intermediate_triangle (0000)
000F3420 0130:
	_connected_geometry_group_coplanar (0000)
00279CB0 0079:
	??_C@_0HJ@CCKALPEO@?$CD?$CD?$CD?5WARNING?3?5found?5duplicate?5tri@ (0000)
00279D30 0008:
	__real@3f847ae140000000 (0000)
004561B3 0001:
	_bss_004561b3 (0000)
*/

/* ---------- headers */

#include "memory/array.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct connected_geometry_point
{
	float x;
	float y;
	float z;
};

struct connected_geometry_edge
{
	struct dynamic_array triangle_indices;
	long point_indices[2];
	long unused[2];
};

struct connected_geometry_triangle
{
	long edge_designators[3];
	long coplanar_group_index;
	long unused[2];
};

struct connected_geometry
{
	struct dynamic_array points;
	struct dynamic_array edges;
	struct dynamic_array triangles;
};

typedef boolean (*connected_geometry_group_predicate)(
	void *predicate_data,
	struct connected_geometry *geometry,
	struct connected_geometry_triangle *triangle,
	long group_index);

typedef char connected_geometry_dynamic_array_size_assert[
	sizeof(struct dynamic_array) == 0xC ? 1 : -1];
typedef char connected_geometry_point_size_assert[
	sizeof(struct connected_geometry_point) == 0xC ? 1 : -1];
typedef char connected_geometry_edge_size_assert[
	sizeof(struct connected_geometry_edge) == 0x1C ? 1 : -1];
typedef char connected_geometry_edge_point_indices_offset_assert[
	offsetof(struct connected_geometry_edge, point_indices) == 0xC ? 1 : -1];
typedef char connected_geometry_triangle_size_assert[
	sizeof(struct connected_geometry_triangle) == 0x18 ? 1 : -1];
typedef char connected_geometry_size_assert[
	sizeof(struct connected_geometry) == 0x24 ? 1 : -1];
typedef char connected_geometry_edges_offset_assert[
	offsetof(struct connected_geometry, edges) == 0xC ? 1 : -1];
typedef char connected_geometry_triangles_offset_assert[
	offsetof(struct connected_geometry, triangles) == 0x18 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void connected_geometry_new(
	struct connected_geometry *geometry)
{
	dynamic_array_new(&geometry->points, sizeof(struct connected_geometry_point));
	dynamic_array_new(&geometry->edges, sizeof(struct connected_geometry_edge));
	dynamic_array_new(&geometry->triangles, sizeof(struct connected_geometry_triangle));

	return;
}

void connected_geometry_delete(
	struct connected_geometry *geometry)
{
	long edge_index;

	for (edge_index = 0; edge_index < geometry->edges.count; edge_index++)
	{
		struct connected_geometry_edge *edge = dynamic_array_get_element(
			&geometry->edges,
			edge_index,
			sizeof(*edge));
		dynamic_array_delete(&edge->triangle_indices);
	}

	dynamic_array_delete(&geometry->points);
	dynamic_array_delete(&geometry->edges);
	dynamic_array_delete(&geometry->triangles);

	return;
}

/* ---------- private code */

void connected_geometry_group_recursive(
	struct connected_geometry *geometry,
	connected_geometry_group_predicate predicate,
	void *predicate_data,
	long group_index,
	long triangle_index)
{
	struct connected_geometry_triangle *triangle;
	long *edge_designator;
	struct connected_geometry_edge *edge;
	long edge_triangle_index;
	short edge_triangle_counter;
	long edge_designator_count;

	triangle = dynamic_array_get_element(
		&geometry->triangles,
		triangle_index,
		sizeof(*triangle));
	if (triangle->coplanar_group_index == NONE &&
		(predicate == NULL ||
			predicate(predicate_data, geometry, triangle, group_index)))
	{
		triangle->coplanar_group_index = group_index;
		edge_designator = triangle->edge_designators;
		edge_designator_count = 3;
		do
		{
			if (*edge_designator != NONE)
			{
				edge = dynamic_array_get_element(
					&geometry->edges,
					*edge_designator & LONG_MAX,
					sizeof(*edge));
				edge_triangle_counter = 0;
				if (edge->triangle_indices.count > 0)
				{
					edge_triangle_index = 0;
					do
					{
						connected_geometry_group_recursive(
							geometry,
							predicate,
							predicate_data,
							group_index,
							*(long *)dynamic_array_get_element(
								&edge->triangle_indices,
								edge_triangle_index,
								sizeof(long)));
						edge_triangle_counter++;
						edge_triangle_index = (long)edge_triangle_counter;
					}
					while (edge_triangle_index < edge->triangle_indices.count);
				}
			}
			edge_designator++;
			edge_designator_count--;
		}
		while (edge_designator_count != 0);
	}

	return;
}
