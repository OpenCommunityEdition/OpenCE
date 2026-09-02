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

enum
{
	NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE= 3,

	/* set in an edge designator when the triangle traverses the edge in the direction it is stored */
	_connected_geometry_edge_designator_forward_bit= 31
};

#define CONNECTED_GEOMETRY_POINT_EPSILON 0.001f
#define CONNECTED_GEOMETRY_COPLANAR_EPSILON 0.01f

/* ---------- macros */

/* ---------- structures */

/* local views of math/real_math.h real_point3d and real_vector3d (see plane3d_from_points below) */

union connected_geometry_point
{
	real n[3];
	struct { real x, y, z; };
};

union connected_geometry_vector
{
	real n[3];
	struct { real i, j, k; };
};

struct connected_geometry_plane
{
	union connected_geometry_vector normal;
	real distance;
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

struct intermediate_geometry_triangle
{
	long unused_before_vertex_indices[2];
	long vertex_indices[3];
	long unused_after_vertex_indices[8];
};

struct intermediate_geometry_vertex
{
	long unused_before_point[2];
	union connected_geometry_point point;
	long unused_after_point[15];
};

struct intermediate_geometry
{
	long unused_before_triangles[77];
	struct dynamic_array triangles;
	struct dynamic_array vertices;
};

typedef boolean (*connected_geometry_group_predicate)(
	void *predicate_data,
	struct connected_geometry *geometry,
	struct connected_geometry_triangle *triangle,
	long group_index);

typedef char connected_geometry_dynamic_array_size_assert[
	sizeof(struct dynamic_array) == 0xC ? 1 : -1];
typedef char connected_geometry_point_size_assert[
	sizeof(union connected_geometry_point) == 0xC ? 1 : -1];
typedef char connected_geometry_plane_size_assert[
	sizeof(struct connected_geometry_plane) == 0x10 ? 1 : -1];
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
typedef char intermediate_geometry_triangle_size_assert[
	sizeof(struct intermediate_geometry_triangle) == 0x34 ? 1 : -1];
typedef char intermediate_geometry_triangle_vertex_indices_offset_assert[
	offsetof(struct intermediate_geometry_triangle, vertex_indices) == 0x8 ? 1 : -1];
typedef char intermediate_geometry_vertex_size_assert[
	sizeof(struct intermediate_geometry_vertex) == 0x50 ? 1 : -1];
typedef char intermediate_geometry_vertex_point_offset_assert[
	offsetof(struct intermediate_geometry_vertex, point) == 0x8 ? 1 : -1];
typedef char intermediate_geometry_triangles_offset_assert[
	offsetof(struct intermediate_geometry, triangles) == 0x134 ? 1 : -1];
typedef char intermediate_geometry_vertices_offset_assert[
	offsetof(struct intermediate_geometry, vertices) == 0x140 ? 1 : -1];

/* ---------- prototypes */

long connected_geometry_add_triangle(
	struct connected_geometry *geometry,
	union connected_geometry_point const *point0,
	union connected_geometry_point const *point1,
	union connected_geometry_point const *point2,
	boolean report_duplicates);
struct connected_geometry_plane *plane3d_from_points(
	struct connected_geometry_plane *plane,
	union connected_geometry_point const *point0,
	union connected_geometry_point const *point1,
	union connected_geometry_point const *point2);
static boolean triangle_coplanar(
	void *predicate_data,
	struct connected_geometry *geometry,
	struct connected_geometry_triangle *triangle,
	long group_index);

/* math/real_math.h (real normalize3d(real_vector3d *v)) */
real normalize3d(
	union connected_geometry_vector *vector);

/* tool/error_geometry.h */
void error_geometry_triangle(
	union connected_geometry_point const *point0,
	union connected_geometry_point const *point1,
	union connected_geometry_point const *point2,
	union real_argb_color const *color);

/* ---------- globals */

static boolean warned_about_duplicate_triangles= FALSE;

/* ---------- public code */

void connected_geometry_new(
	struct connected_geometry *geometry)
{
	dynamic_array_new(&geometry->points, sizeof(union connected_geometry_point));
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

static long connected_geometry_find_or_add_vertex(
	struct connected_geometry *geometry,
	union connected_geometry_point const *point)
{
	long point_index;

	for (point_index= 0; point_index<geometry->points.count; point_index++)
	{
		if (fabs(point->x - ((union connected_geometry_point *)dynamic_array_get_element(
				&geometry->points, point_index, sizeof(union connected_geometry_point)))->x)<CONNECTED_GEOMETRY_POINT_EPSILON &&
			fabs(point->y - ((union connected_geometry_point *)dynamic_array_get_element(
				&geometry->points, point_index, sizeof(union connected_geometry_point)))->y)<CONNECTED_GEOMETRY_POINT_EPSILON &&
			fabs(point->z - ((union connected_geometry_point *)dynamic_array_get_element(
				&geometry->points, point_index, sizeof(union connected_geometry_point)))->z)<CONNECTED_GEOMETRY_POINT_EPSILON)
		{
			break;
		}
	}

	if (point_index==geometry->points.count)
	{
		point_index= dynamic_array_add_element(&geometry->points);
		if (point_index!=NONE)
		{
			*(union connected_geometry_point *)dynamic_array_get_element(
				&geometry->points, point_index, sizeof(union connected_geometry_point))= *point;
		}
	}

	return point_index;
}

static long connected_geometry_find_or_add_edge(
	struct connected_geometry *geometry,
	long triangle_index,
	long point_index0,
	long point_index1)
{
	struct connected_geometry_edge *edge;
	long edge_index;
	long edge_triangle_index;
	boolean forward= TRUE;

	for (edge_index= 0; edge_index<geometry->edges.count; edge_index++)
	{
		edge= dynamic_array_get_element(&geometry->edges, edge_index, sizeof(*edge));
		if (edge->point_indices[0]==point_index0 && edge->point_indices[1]==point_index1)
		{
			forward= TRUE;
			break;
		}
		if (edge->point_indices[0]==point_index1 && edge->point_indices[1]==point_index0)
		{
			forward= FALSE;
			break;
		}
	}

	if (edge_index==geometry->edges.count)
	{
		edge_index= dynamic_array_add_element(&geometry->edges);
		forward= TRUE;
		if (edge_index==NONE)
		{
			return NONE;
		}
		edge= dynamic_array_get_element(&geometry->edges, edge_index, sizeof(*edge));
		dynamic_array_new(&edge->triangle_indices, sizeof(long));
		edge->point_indices[0]= point_index0;
		edge->point_indices[1]= point_index1;
	}

	if (edge_index==NONE)
	{
		return NONE;
	}

	edge= dynamic_array_get_element(&geometry->edges, edge_index, sizeof(*edge));
	edge_triangle_index= dynamic_array_add_element(&edge->triangle_indices);
	if (edge_triangle_index==NONE)
	{
		return NONE;
	}
	*(long *)dynamic_array_get_element(&edge->triangle_indices, edge_triangle_index, sizeof(long))= triangle_index;

	SET_FLAG(edge_index, _connected_geometry_edge_designator_forward_bit, forward);

	return edge_index;
}

/* TU-local mirrors of the math/real_math.h vector inlines, written against this file's local point,
 * vector and plane views (this file cannot include real_math.h: its inline plane3d_from_points
 * returns the plane on the degenerate path where January returns NULL, see below). */

__inline union connected_geometry_vector *vector_from_points3d(
	union connected_geometry_point const *a,
	union connected_geometry_point const *b,
	union connected_geometry_vector *result)
{
	result->i= b->x-a->x;
	result->j= b->y-a->y;
	result->k= b->z-a->z;
	return result;
}

__inline union connected_geometry_vector *cross_product3d(
	union connected_geometry_vector const *a,
	union connected_geometry_vector const *b,
	union connected_geometry_vector *result)
{
	real k= a->i*b->j - a->j*b->i;
	real j= a->k*b->i - a->i*b->k;
	real i= a->j*b->k - a->k*b->j;
	result->i= i;
	result->j= j;
	result->k= k;

	return result;
}

__inline real dot_product3d(
	union connected_geometry_vector const *a,
	union connected_geometry_vector const *b)
{
	return a->i*b->i + a->j*b->j + a->k*b->k;
}

struct connected_geometry_plane *plane3d_from_points(
	struct connected_geometry_plane *plane,
	union connected_geometry_point const *point0,
	union connected_geometry_point const *point1,
	union connected_geometry_point const *point2)
{
	union connected_geometry_vector v0;
	union connected_geometry_vector v1;

	vector_from_points3d(point0, point1, &v0);
	vector_from_points3d(point0, point2, &v1);

	cross_product3d(&v0, &v1, &plane->normal);

	if (normalize3d(&plane->normal)!=0.f)
	{
		plane->distance= dot_product3d((union connected_geometry_vector const *)point0, &plane->normal);
		return plane;
	}

	plane->distance= 0.f;

	return NULL;
}

long connected_geometry_add_triangle(
	struct connected_geometry *geometry,
	union connected_geometry_point const *point0,
	union connected_geometry_point const *point1,
	union connected_geometry_point const *point2,
	boolean report_duplicates)
{
	struct connected_geometry_triangle *triangle;
	long point_indices[NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE];
	long triangle_index;
	long edge_index;

	triangle_index= dynamic_array_add_element(&geometry->triangles);
	if (triangle_index!=NONE)
	{
		triangle= dynamic_array_get_element(&geometry->triangles, triangle_index, sizeof(*triangle));

		point_indices[0]= connected_geometry_find_or_add_vertex(geometry, point0);
		point_indices[1]= connected_geometry_find_or_add_vertex(geometry, point1);
		point_indices[2]= connected_geometry_find_or_add_vertex(geometry, point2);
		if (point_indices[0]==NONE || point_indices[1]==NONE || point_indices[2]==NONE)
		{
			triangle_index= NONE;
		}

		for (edge_index= 0; edge_index<NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE; edge_index++)
		{
			triangle->edge_designators[edge_index]= connected_geometry_find_or_add_edge(
				geometry,
				triangle_index,
				point_indices[edge_index],
				point_indices[(edge_index+1)%NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE]);
			if (triangle->edge_designators[edge_index]==NONE)
			{
				triangle_index= NONE;
			}
		}

		triangle->coplanar_group_index= NONE;
		memset(triangle->unused, 0, sizeof(triangle->unused));

		if (report_duplicates)
		{
			long other_triangle_index;

			for (other_triangle_index= 0; other_triangle_index<geometry->triangles.count-1; other_triangle_index++)
			{
				struct connected_geometry_triangle *other_triangle= dynamic_array_get_element(
					&geometry->triangles, other_triangle_index, sizeof(*other_triangle));
				short i, j;

				for (i= 0; i<NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE; i++)
				{
					for (j= 0; j<NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE; j++)
					{
						if ((other_triangle->edge_designators[j] & LONG_MAX)==(triangle->edge_designators[i] & LONG_MAX))
						{
							break;
						}
					}
					if (j==NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE)
					{
						break;
					}
				}

				if (i==NUMBER_OF_EDGES_PER_CONNECTED_GEOMETRY_TRIANGLE)
				{
					error_geometry_triangle(point0, point1, point2, global_real_argb_orange);
					if (!warned_about_duplicate_triangles)
					{
						printf("### WARNING: found duplicate triangle building connected geometry. YOU SHOULD FIX THIS. (see orange in error geometry)\r\n");
						warned_about_duplicate_triangles= TRUE;
					}
					return NONE;
				}
			}
		}
	}

	return triangle_index;
}

static boolean triangle_coplanar(
	void *predicate_data,
	struct connected_geometry *geometry,
	struct connected_geometry_triangle *triangle,
	long group_index)
{
	struct connected_geometry_plane *plane= predicate_data;
	union connected_geometry_point *point0;
	union connected_geometry_point *point1;
	union connected_geometry_point *point2;
	struct connected_geometry_plane triangle_plane;

	point0= dynamic_array_get_element(
		&geometry->points,
		((struct connected_geometry_edge *)dynamic_array_get_element(
			&geometry->edges,
			triangle->edge_designators[0] & LONG_MAX,
			sizeof(struct connected_geometry_edge)))->point_indices[(triangle->edge_designators[0] & LONG_MIN)!=0],
		sizeof(*point0));
	point1= dynamic_array_get_element(
		&geometry->points,
		((struct connected_geometry_edge *)dynamic_array_get_element(
			&geometry->edges,
			triangle->edge_designators[1] & LONG_MAX,
			sizeof(struct connected_geometry_edge)))->point_indices[(triangle->edge_designators[1] & LONG_MIN)!=0],
		sizeof(*point1));
	point2= dynamic_array_get_element(
		&geometry->points,
		((struct connected_geometry_edge *)dynamic_array_get_element(
			&geometry->edges,
			triangle->edge_designators[2] & LONG_MAX,
			sizeof(struct connected_geometry_edge)))->point_indices[(triangle->edge_designators[2] & LONG_MIN)!=0],
		sizeof(*point2));

	if (fabs(point0->x*plane->normal.i + point0->y*plane->normal.j + point0->z*plane->normal.k - plane->distance)<CONNECTED_GEOMETRY_COPLANAR_EPSILON &&
		fabs(point1->x*plane->normal.i + point1->y*plane->normal.j + point1->z*plane->normal.k - plane->distance)<CONNECTED_GEOMETRY_COPLANAR_EPSILON &&
		fabs(point2->x*plane->normal.i + point2->y*plane->normal.j + point2->z*plane->normal.k - plane->distance)<CONNECTED_GEOMETRY_COPLANAR_EPSILON)
	{
		if (plane3d_from_points(&triangle_plane, point0, point2, point1)!=NULL &&
			triangle_plane.normal.i*plane->normal.i + triangle_plane.normal.j*plane->normal.j + triangle_plane.normal.k*plane->normal.k>0.f)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void connected_geometry_add_intermediate_triangle(
	struct connected_geometry *geometry,
	struct intermediate_geometry *intermediate_geometry,
	long triangle_index,
	boolean report_duplicates)
{
	struct intermediate_geometry_triangle *triangle;

	triangle = dynamic_array_get_element(
		&intermediate_geometry->triangles,
		triangle_index,
		sizeof(*triangle));
	connected_geometry_add_triangle(
		geometry,
		&((struct intermediate_geometry_vertex *)dynamic_array_get_element(
			&intermediate_geometry->vertices,
			triangle->vertex_indices[0],
			sizeof(struct intermediate_geometry_vertex)))->point,
		&((struct intermediate_geometry_vertex *)dynamic_array_get_element(
			&intermediate_geometry->vertices,
			triangle->vertex_indices[1],
			sizeof(struct intermediate_geometry_vertex)))->point,
		&((struct intermediate_geometry_vertex *)dynamic_array_get_element(
			&intermediate_geometry->vertices,
			triangle->vertex_indices[2],
			sizeof(struct intermediate_geometry_vertex)))->point,
		report_duplicates);

	return;
}

long connected_geometry_group_coplanar(
	struct connected_geometry *geometry)
{
	struct dynamic_array *edges;
	struct connected_geometry_triangle *triangle;
	long group_count;
	long triangle_index;
	struct connected_geometry_plane plane;

	group_count = 0;
	triangle_index = 0;
	if (geometry->triangles.count > 0)
	{
		edges = &geometry->edges;
		do
		{
			triangle = dynamic_array_get_element(
				&geometry->triangles,
				triangle_index,
				sizeof(*triangle));
			plane3d_from_points(
				&plane,
				dynamic_array_get_element(
					&geometry->points,
					((struct connected_geometry_edge *)dynamic_array_get_element(
						edges,
						triangle->edge_designators[0] & LONG_MAX,
						sizeof(struct connected_geometry_edge)))->point_indices[
							(triangle->edge_designators[0] & LONG_MIN) != 0],
					sizeof(union connected_geometry_point)),
				dynamic_array_get_element(
					&geometry->points,
					((struct connected_geometry_edge *)dynamic_array_get_element(
						edges,
						triangle->edge_designators[2] & LONG_MAX,
						sizeof(struct connected_geometry_edge)))->point_indices[
							(triangle->edge_designators[2] & LONG_MIN) != 0],
					sizeof(union connected_geometry_point)),
				dynamic_array_get_element(
					&geometry->points,
					((struct connected_geometry_edge *)dynamic_array_get_element(
						edges,
						triangle->edge_designators[1] & LONG_MAX,
						sizeof(struct connected_geometry_edge)))->point_indices[
							(triangle->edge_designators[1] & LONG_MIN) != 0],
					sizeof(union connected_geometry_point)));
			if (triangle->coplanar_group_index == NONE)
			{
				connected_geometry_group_recursive(
					geometry,
					triangle_coplanar,
					&plane,
					group_count,
					triangle_index);
				group_count++;
			}
			triangle_index++;
		}
		while (triangle_index < geometry->triangles.count);
	}

	return group_count;
}
