/*
COOP_CRYO_CELL.C (test)

The collision bsp port/linux/game/coop_cryo_cell.c builds for the cryo bay's
cell, walked by the engine's own bsp code and checked against the hulls in the
cell's tables (test_coop_cryo_cell.py).
*/

#define real_point3d harness_real_point3d
#define real_vector3d harness_real_vector3d
#include "harness.h"
#undef real_point3d
#undef real_vector3d

#include <limits.h>
#include <math.h>
#include <stddef.h>

typedef unsigned short word;

#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : MIN((n), (ceiling)))
#define BIT_VECTOR_TEST_FLAG(vector, bit) (((vector)[(bit) >> 5] & (1UL << ((bit) & 31))) != 0)
#define _error_silent 0

static void error(int level, char const *format, ...)
{
}

#include "types.inc"

static void *tag_block_element(struct tag_block const *block, long index, long size)
{
	CHECK(index >= 0 && index < block->count, "element %ld of a block of %ld", index, block->count);
	return (char *)block->address + index * size;
}

#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)tag_block_element((block), (index), sizeof(type)))

/* not under test: collision's statistics, two-sided surfaces (the cell has none) */
#define collision_log_usage(function) ((void)0)
#define collision_log_start_time(time) ((void)0)
#define collision_log_end_time(function, time) ((void)0)
#define collision_surface_test_point(...) TRUE
static struct collision_bsp const *global_collision_bsp;
static boolean warned_about_bsp2d_nodes, warned_about_bsp3d_nodes, warned_about_collision_surface_edges,
	warned_about_collision_surface_index, warned_about_collision_bsp_nodes, warned_about_collision_bsp2d_nodes,
	warned_about_collision_leaf;

/* (the engine's flat initializers of its unions) */
#pragma GCC diagnostic ignored "-Wmissing-braces"
#include "under_test.inc"

/* ---------- the hulls in the cell's tables */

enum
{
	MAXIMUM_HULL_PLANES = 64,
};

#define EDGE_MARGIN 2e-3

struct hull_plane
{
	double n[3];
	double d;
};

static struct hull_plane hull_planes[CELL_PIECES][MAXIMUM_HULL_PLANES];
static short hull_plane_counts[CELL_PIECES];
static double hull_areas[CELL_PIECES];
static struct collision_bsp bsp;

static double dot(double const *a, double const *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void cell_new(void)
{
	short piece;

	for (piece = 0; piece < CELL_PIECES; piece++)
	{
		short first_corner = cell_collision_pieces[piece][0];
		short triangle;

		for (triangle = cell_collision_pieces[piece][2];
			triangle < cell_collision_pieces[piece][2] + cell_collision_pieces[piece][3]; triangle++)
		{
			struct hull_plane *plane = &hull_planes[piece][hull_plane_counts[piece]];
			double p[3][3], u[3], v[3], length;
			int corner, axis;

			for (corner = 0; corner < 3; corner++)
			{
				for (axis = 0; axis < 3; axis++)
					p[corner][axis] = cell_collision_corners[first_corner + cell_collision_triangles[triangle][corner]][axis];
			}
			for (axis = 0; axis < 3; axis++)
			{
				u[axis] = p[1][axis] - p[0][axis];
				v[axis] = p[2][axis] - p[0][axis];
			}
			plane->n[0] = u[1] * v[2] - u[2] * v[1];
			plane->n[1] = u[2] * v[0] - u[0] * v[2];
			plane->n[2] = u[0] * v[1] - u[1] * v[0];
			length = sqrt(dot(plane->n, plane->n));
			CHECK(length > 1e-9, "piece %d's triangle %d is a line", piece, triangle);
			hull_areas[piece] += length / 2.0;
			for (axis = 0; axis < 3; axis++)
				plane->n[axis] /= length;
			plane->d = dot(plane->n, p[0]);
			CHECK(hull_plane_counts[piece] < MAXIMUM_HULL_PLANES, "piece %d has too many triangles", piece);
			hull_plane_counts[piece]++;
		}
	}
	CHECK(cell_collision_bsp(&bsp), "the cell's collision bsp was not built");
}

/* how far a point is out of a hull's planes (negative: inside) */
static double hull_distance(short piece, double const *point)
{
	double distance = -1e9;
	short index;

	for (index = 0; index < hull_plane_counts[piece]; index++)
		distance = MAX(distance, dot(hull_planes[piece][index].n, point) - hull_planes[piece][index].d);
	return distance;
}

/* where a segment from outside the hulls first enters one: FALSE when it
enters none, or too near an edge, another hull or a graze to say which face */
static boolean hulls_test_segment(double const *origin, double const *vector, boolean *hit, double *t, double *normal)
{
	double length = sqrt(dot(vector, vector));
	double best = 2.0;
	short best_piece = NONE;
	short best_plane = NONE;
	boolean clear = TRUE;
	short piece;

	for (piece = 0; piece < CELL_PIECES; piece++)
	{
		double enter = 0.0, exit = 1.0;
		short enter_plane = NONE;
		boolean apart = FALSE;
		short index;

		for (index = 0; index < hull_plane_counts[piece]; index++)
		{
			struct hull_plane const *plane = &hull_planes[piece][index];
			double distance = dot(plane->n, origin) - plane->d;
			double rate = dot(plane->n, vector);

			if (fabs(rate) < 1e-12)
			{
				apart |= distance >= 0.0;
				continue;
			}
			if (rate < 0.0 && -distance / rate > enter)
			{
				enter = -distance / rate;
				enter_plane = index;
			}
			else if (rate > 0.0)
			{
				exit = MIN(exit, -distance / rate);
			}
		}
		if (apart || enter_plane == NONE)
			continue;
		if (fabs(enter - exit) * length < EDGE_MARGIN)
			clear = FALSE;
		if (enter > exit)
			continue;
		for (index = 0; index < hull_plane_counts[piece]; index++)
		{
			struct hull_plane const *plane = &hull_planes[piece][index];
			double rate = dot(plane->n, vector);

			if (rate < 0.0 && dot(plane->n, hull_planes[piece][enter_plane].n) < 0.9999 &&
				(enter - (plane->d - dot(plane->n, origin)) / rate) * length < EDGE_MARGIN)
			{
				clear = FALSE;
			}
		}
		if (best_piece != NONE && fabs(enter - best) * length < EDGE_MARGIN)
			clear = FALSE;
		if (enter < best)
		{
			best = enter;
			best_piece = piece;
			best_plane = enter_plane;
		}
	}
	*hit = best_piece != NONE;
	if (*hit)
	{
		*t = best;
		memcpy(normal, hull_planes[best_piece][best_plane].n, sizeof(hull_planes[best_piece][best_plane].n));
	}
	return clear;
}

static unsigned long random_seed = 12345;

static double random_real(double low, double high)
{
	random_seed = random_seed * 1664525UL + 1013904223UL;
	return low + (high - low) * ((random_seed >> 8) & 0xFFFFFF) / (double)0x1000000;
}

static void random_point(double *point)
{
	point[0] = random_real(-1.5, 1.4);
	point[1] = random_real(-1.2, 1.2);
	point[2] = random_real(-0.3, 1.6);
}

static boolean outside_hulls(double const *point)
{
	short piece;

	for (piece = 0; piece < CELL_PIECES; piece++)
	{
		if (hull_distance(piece, point) < EDGE_MARGIN)
			return FALSE;
	}
	return TRUE;
}

/* a segment, as the engine and as the hulls see it: TRUE when the hulls
are clear about it */
static boolean segment_compare(double const *origin, double const *vector, boolean *hulls_hit)
{
	struct collision_bsp_test_vector_result result;
	real_point3d point = { { (real)origin[0], (real)origin[1], (real)origin[2] } };
	real_vector3d engine_vector = { { (real)vector[0], (real)vector[1], (real)vector[2] } };
	double length = sqrt(dot(vector, vector));
	double t, normal[3];
	boolean hit;

	if (!hulls_test_segment(origin, vector, &hit, &t, normal))
		return FALSE;
	*hulls_hit = hit;
	if (collision_bsp_test_vector(FLAG(_collision_test_front_facing_surfaces_bit), &bsp, 0, NULL, &point, &engine_vector,
		1.0f, &result))
	{
		double engine_normal[3] = { result.plane->n.i, result.plane->n.j, result.plane->n.k };

		CHECK(hit, "a segment from %.4f %.4f %.4f along %.4f %.4f %.4f hits the bsp at %.4f, but no hull", origin[0],
			origin[1], origin[2], vector[0], vector[1], vector[2], result.t);
		CHECK(fabs(result.t - t) * length < 1e-3, "a segment from %.4f %.4f %.4f along %.4f %.4f %.4f hits the bsp at "
			"%.4f, a hull at %.4f", origin[0], origin[1], origin[2], vector[0], vector[1], vector[2], result.t, t);
		CHECK(dot(engine_normal, normal) > 0.999, "a segment from %.4f %.4f %.4f along %.4f %.4f %.4f hits the bsp "
			"through a plane facing %.3f %.3f %.3f, not %.3f %.3f %.3f", origin[0], origin[1], origin[2], vector[0],
			vector[1], vector[2], engine_normal[0], engine_normal[1], engine_normal[2], normal[0], normal[1], normal[2]);
	}
	else
	{
		CHECK(!hit, "a segment from %.4f %.4f %.4f along %.4f %.4f %.4f enters a hull at %.4f, but misses the bsp",
			origin[0], origin[1], origin[2], vector[0], vector[1], vector[2], t);
	}
	return TRUE;
}

static boolean listed(long const *indices, long count, long index)
{
	long at;

	for (at = 0; at < count; at++)
	{
		if (indices[at] == index)
			return TRUE;
	}
	return FALSE;
}

/* ---------- the cases */

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	cell_new();

	/* every surface the outline of a face of its hull, counterclockwise round
	its outward normal, the faces covering each hull */
	CASE("faces")
	{
		double areas[CELL_PIECES] = { 0 };
		long surface_index;
		short piece;

		for (surface_index = 0; surface_index < bsp.surfaces.count; surface_index++)
		{
			struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(&bsp.surfaces, surface_index,
				struct collision_surface);
			real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(&bsp.bsp3d.planes, surface->plane_designator, real_plane3d);
			double n[3] = { plane->n.i, plane->n.j, plane->n.k };
			real_point3d points[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
			short count = collision_surface_polygon(&bsp, surface_index, points);
			double newell[3] = { 0.0, 0.0, 0.0 };
			short owner = NONE;
			long edge_index = surface->first_edge_index;
			short k;

			CHECK(count >= 3, "surface %ld has %d corners", surface_index, count);
			for (k = 0; k < count; k++)
			{
				real_point3d const *a = &points[k];
				real_point3d const *b = &points[(k + 1) % count];
				real_point3d const *c = &points[(k + 2) % count];
				double ab[3] = { b->x - a->x, b->y - a->y, b->z - a->z };
				double bc[3] = { c->x - b->x, c->y - b->y, c->z - b->z };
				double turn[3] = { ab[1] * bc[2] - ab[2] * bc[1], ab[2] * bc[0] - ab[0] * bc[2], ab[0] * bc[1] - ab[1] * bc[0] };

				CHECK(fabs(plane3d_distance_to_point(plane, a)) < 1e-4, "surface %ld's corner %d is off its plane",
					surface_index, k);
				CHECK(dot(turn, n) > -1e-6, "surface %ld turns clockwise at its corner %d", surface_index, (k + 1) % count);
				newell[0] += ((double)a->y - b->y) * ((double)a->z + b->z);
				newell[1] += ((double)a->z - b->z) * ((double)a->x + b->x);
				newell[2] += ((double)a->x - b->x) * ((double)a->y + b->y);
			}
			CHECK(dot(newell, n) > 0.0, "surface %ld winds clockwise round its normal", surface_index);
			for (piece = 0; piece < CELL_PIECES && owner == NONE; piece++)
			{
				boolean on_hull = TRUE;
				short index;

				for (k = 0; k < count; k++)
				{
					double point[3] = { points[k].x, points[k].y, points[k].z };

					on_hull &= hull_distance(piece, point) < 1e-4;
				}
				for (index = 0; on_hull && index < hull_plane_counts[piece]; index++)
				{
					if (dot(hull_planes[piece][index].n, n) > 0.9999 && fabs(hull_planes[piece][index].d - plane->d) < 1e-3)
						owner = piece;
				}
			}
			CHECK(owner != NONE, "surface %ld is on no hull's face", surface_index);
			areas[owner] += sqrt(dot(newell, newell)) / 2.0;

			for (k = 0; k < count; k++)
			{
				struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(&bsp.edges, edge_index, struct collision_edge);
				boolean reverse = edge->surface_indices[1] == surface_index;

				CHECK(edge->surface_indices[reverse] == surface_index, "surface %ld's ring has another's edge %ld",
					surface_index, edge_index);
				CHECK(edge->surface_indices[!reverse] >= 0 && edge->surface_indices[!reverse] < bsp.surfaces.count &&
					edge->surface_indices[!reverse] != surface_index, "surface %ld's edge %ld has one side",
					surface_index, edge_index);
				edge_index = edge->edge_indices[reverse];
			}
			CHECK(edge_index == surface->first_edge_index, "surface %ld's ring does not close", surface_index);
		}
		for (piece = 0; piece < CELL_PIECES; piece++)
		{
			CHECK(fabs(areas[piece] - hull_areas[piece]) < 1e-4 * hull_areas[piece], "piece %d's surfaces cover %.5f m2 "
				"of its hull's %.5f", piece, areas[piece], hull_areas[piece]);
		}
		return 0;
	}

	/* solid inside the hulls, empty outside */
	CASE("points")
	{
		long tested = 0, solid = 0;
		long index;

		for (index = 0; index < 1000000; index++)
		{
			double point[3];
			real_point3d engine_point;
			boolean inside = FALSE, near = FALSE;
			short piece;

			random_point(point);
			for (piece = 0; piece < CELL_PIECES; piece++)
			{
				double distance = hull_distance(piece, point);

				inside |= distance < 0.0;
				near |= fabs(distance) < 1e-4;
			}
			if (near)
				continue;
			set_real_point3d(&engine_point, (real)point[0], (real)point[1], (real)point[2]);
			CHECK((bsp3d_test_point(&bsp.bsp3d, 0, &engine_point) == NONE) == inside, "%.4f %.4f %.4f is %s a hull, "
				"but the bsp has it %s", point[0], point[1], point[2], inside ? "in" : "out of", inside ? "empty" : "solid");
			tested++;
			solid += inside;
		}
		CHECK(solid > tested / 50, "only %ld of %ld points were in the hulls", solid, tested);
		return 0;
	}

	/* segments from outside every way, entering where the hulls are entered */
	CASE("rays")
	{
		long tested = 0, hits = 0;
		long index;

		for (index = 0; index < 200000; index++)
		{
			double origin[3], vector[3], length;
			boolean hit;

			random_point(origin);
			if (!outside_hulls(origin))
				continue;
			do
			{
				vector[0] = random_real(-1.0, 1.0);
				vector[1] = random_real(-1.0, 1.0);
				vector[2] = random_real(-1.0, 1.0);
				length = sqrt(dot(vector, vector));
			}
			while (length < 0.1 || length > 1.0);
			vector[0] *= 4.0 / length;
			vector[1] *= 4.0 / length;
			vector[2] *= 4.0 / length;
			if (segment_compare(origin, vector, &hit))
			{
				tested++;
				hits += hit;
			}
		}
		CHECK(hits > tested / 10, "only %ld of %ld segments hit a hull", hits, tested);
		return 0;
	}

	/* straight down, as a biped finds the ground: the tops stood on */
	CASE("ground")
	{
		long tested = 0, hits = 0;
		double x, y;

		for (x = -1.3; x < 1.2; x += 0.0071)
		{
			for (y = -1.0; y < 1.0; y += 0.0071)
			{
				double origin[3] = { x, y, 2.0 };
				double vector[3] = { 0.0, 0.0, -3.0 };
				boolean hit;

				if (segment_compare(origin, vector, &hit))
				{
					tested++;
					hits += hit;
				}
			}
		}
		CHECK(hits > tested / 8, "only %ld of %ld drops landed on a hull", hits, tested);
		return 0;
	}

	/* a small sphere on any surface or edge touches it; one away touches nothing */
	CASE("spheres")
	{
		static struct collision_bsp_test_sphere_result result;
		real const radius = 0.02f;
		real_point3d center;
		long index;

		for (index = 0; index < bsp.surfaces.count; index++)
		{
			struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(&bsp.surfaces, index, struct collision_surface);
			real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(&bsp.bsp3d.planes, surface->plane_designator, real_plane3d);
			real_point3d points[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
			short count = collision_surface_polygon(&bsp, index, points);
			short k;

			CHECK(count >= 3, "surface %ld has %d corners", index, count);
			set_real_point3d(&center, 0.0f, 0.0f, 0.0f);
			for (k = 0; k < count; k++)
			{
				center.x += points[k].x / count;
				center.y += points[k].y / count;
				center.z += points[k].z / count;
			}
			center.x += plane->n.i * radius / 2.0f;
			center.y += plane->n.j * radius / 2.0f;
			center.z += plane->n.k * radius / 2.0f;
			collision_bsp_test_sphere(&bsp, 0, NULL, &center, radius, &result);
			CHECK(listed(result.surface_indices, result.surface_count, index),
				"a sphere on surface %ld at %.4f %.4f %.4f does not touch it", index, center.x, center.y, center.z);
		}
		for (index = 0; index < bsp.edges.count; index++)
		{
			struct collision_edge const *edge = TAG_BLOCK_GET_ELEMENT(&bsp.edges, index, struct collision_edge);
			struct collision_vertex const *a = TAG_BLOCK_GET_ELEMENT(&bsp.vertices, edge->vertex_indices[0],
				struct collision_vertex);
			struct collision_vertex const *b = TAG_BLOCK_GET_ELEMENT(&bsp.vertices, edge->vertex_indices[1],
				struct collision_vertex);
			real_vector3d out;
			short side;

			set_real_vector3d(&out, 0.0f, 0.0f, 0.0f);
			for (side = 0; side < 2; side++)
			{
				struct collision_surface const *surface = TAG_BLOCK_GET_ELEMENT(&bsp.surfaces,
					edge->surface_indices[side], struct collision_surface);
				real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(&bsp.bsp3d.planes, surface->plane_designator,
					real_plane3d);

				out.i += plane->n.i;
				out.j += plane->n.j;
				out.k += plane->n.k;
			}
			normalize3d(&out);
			set_real_point3d(&center, (a->point.x + b->point.x) / 2.0f + out.i * radius / 2.0f,
				(a->point.y + b->point.y) / 2.0f + out.j * radius / 2.0f, (a->point.z + b->point.z) / 2.0f + out.k * radius / 2.0f);
			collision_bsp_test_sphere(&bsp, 0, NULL, &center, radius, &result);
			CHECK(listed(result.edge_indices, result.edge_count, index),
				"a sphere on edge %ld at %.4f %.4f %.4f does not touch it", index, center.x, center.y, center.z);
		}
		set_real_point3d(&center, 0.0f, 0.0f, 2.0f);
		CHECK(!collision_bsp_test_sphere(&bsp, 0, NULL, &center, 0.5f, &result) && !result.vertex_count,
			"a sphere over the cell touches it");
		return 0;
	}

	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
