#include "harness.h"
#include <limits.h>

#include "config.inc"

/* A collision bsp of four surfaces (collision_bsp_definitions.h), cut into
the triangles the host's Jolt world is built from (physics_jolt.c). Jolt's
triangles face where their corners turn counter-clockwise, so each must face
as its surface's plane does, the designator's sign bit turning the plane.

	0 a floor, a quad at z 0 whose ring turns clockwise from above, on
	  plane 0 (up): its triangles must be turned to face up
	1 a wall at x 0, a triangle on plane 1 (+x) taken backwards (the
	  designator's sign bit): it faces -x
	2 a two-sided pane at y 5: both ways
	3 a surface whose ring names a vertex the bsp does not have: left out */
struct tag_block { long count; void *address; long definition; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address + (index))
typedef struct { real_vector3d n; real d; } real_plane3d;
struct collision_surface { long plane_designator; long first_edge_index; byte flags; byte breakable_surface_index; short material_index; };
struct collision_edge { long vertex_indices[2]; long edge_indices[2]; long surface_indices[2]; };
struct collision_vertex { real_point3d point; long first_edge_index; };
struct bsp3d { struct tag_block nodes, planes; };
struct collision_bsp { struct bsp3d bsp3d; struct tag_block leaves, bsp2d_references, bsp2d_nodes, bsp2d_planes, surfaces, edges, vertices; };

static struct collision_vertex vertices[] =
{
	/* the floor: 0 1 2 3, clockwise from above */
	{{0, 0, 0}}, {{0, 4, 0}}, {{4, 4, 0}}, {{4, 0, 0}},
	/* the wall: 4 5 6 */
	{{0, 0, 1}}, {{0, 1, 1}}, {{0, 0, 2}},
	/* the pane: 7 8 9 */
	{{0, 5, 0}}, {{1, 5, 0}}, {{0, 5, 1}},
};
static real_plane3d planes[] = {{{0, 0, 1}, 0}, {{1, 0, 0}, 0}, {{0, 1, 0}, 5}};
/* each edge walked forwards by its surface (surface_indices[0]), its next
edge in edge_indices[0]; the broken surface's ring is 99 0 1 */
static struct collision_edge edges[] =
{
	{{0, 1}, {1, NONE}, {0, NONE}}, {{1, 2}, {2, NONE}, {0, NONE}},
	{{2, 3}, {3, NONE}, {0, NONE}}, {{3, 0}, {0, NONE}, {0, NONE}},
	{{4, 5}, {5, NONE}, {1, NONE}}, {{5, 6}, {6, NONE}, {1, NONE}}, {{6, 4}, {4, NONE}, {1, NONE}},
	{{7, 8}, {8, NONE}, {2, NONE}}, {{8, 9}, {9, NONE}, {2, NONE}}, {{9, 7}, {7, NONE}, {2, NONE}},
	{{99, 0}, {11, NONE}, {3, NONE}}, {{0, 1}, {12, NONE}, {3, NONE}}, {{1, 99}, {10, NONE}, {3, NONE}},
};
static struct collision_surface surfaces[] =
{
	{0, 0, 0}, {1 | LONG_MIN, 4, 0}, {2, 7, FLAG(_physics_jolt_surface_two_sided_bit)}, {0, 10, 0},
};
static struct collision_bsp bsp;

#include "under_test.inc"

#define COUNT(a) ((long)(sizeof(a) / sizeof((a)[0])))

static real out_vertices[3 * COUNT(vertices)];
static unsigned long indices[3 * 16];

/* the face normal of triangle t, as Jolt has it (counter-clockwise) */
static real_vector3d facing(long t)
{
	real const *a = out_vertices + 3 * indices[3 * t];
	real const *b = out_vertices + 3 * indices[3 * t + 1];
	real const *c = out_vertices + 3 * indices[3 * t + 2];
	real u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
	real v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
	real_vector3d n;

	n.i = u[1] * v[2] - u[2] * v[1];
	n.j = u[2] * v[0] - u[0] * v[2];
	n.k = u[0] * v[1] - u[1] * v[0];
	return n;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long count;
	long t;

	bsp.bsp3d.planes.count = COUNT(planes);
	bsp.bsp3d.planes.address = planes;
	bsp.surfaces.count = COUNT(surfaces);
	bsp.surfaces.address = surfaces;
	bsp.edges.count = COUNT(edges);
	bsp.edges.address = edges;
	bsp.vertices.count = COUNT(vertices);
	bsp.vertices.address = vertices;

	/* the floor's 2, the wall's 1, the pane's 2 */
	CASE("counted")
	{
		count = physics_jolt_level_triangles(&bsp, NULL, NULL, 0);
		CHECK(count == 5, "counted: %ld triangles", count);
		count = physics_jolt_level_triangles(&bsp, out_vertices, indices, 16);
		CHECK(count == 5, "cut: %ld triangles", count);
		for (t = 0; t < COUNT(vertices); t++)
			CHECK(out_vertices[3 * t] == vertices[t].point.x && out_vertices[3 * t + 1] == vertices[t].point.y &&
				out_vertices[3 * t + 2] == vertices[t].point.z, "vertex %ld not copied", t);
		return 0;
	}
	/* the floor's triangles face up, though its ring turns clockwise */
	CASE("floor")
	{
		physics_jolt_level_triangles(&bsp, out_vertices, indices, 16);
		for (t = 0; t < 2; t++)
		{
			real_vector3d n = facing(t);

			CHECK(n.k > 0.0f && n.i == 0.0f && n.j == 0.0f, "floor: triangle %ld faces (%g %g %g)", t, n.i, n.j, n.k);
		}
		return 0;
	}
	/* the wall, on its plane taken backwards, faces -x */
	CASE("wall")
	{
		real_vector3d n;

		physics_jolt_level_triangles(&bsp, out_vertices, indices, 16);
		n = facing(2);
		CHECK(n.i < 0.0f, "wall: faces (%g %g %g)", n.i, n.j, n.k);
		return 0;
	}
	/* the pane, both ways */
	CASE("pane")
	{
		real_vector3d front, back;

		physics_jolt_level_triangles(&bsp, out_vertices, indices, 16);
		front = facing(3);
		back = facing(4);
		CHECK(front.j > 0.0f && back.j < 0.0f, "pane: faces %g and %g", front.j, back.j);
		return 0;
	}
	/* no more triangles than there is room for */
	CASE("capacity")
	{
		memset(indices, 0xff, sizeof(indices));
		count = physics_jolt_level_triangles(&bsp, out_vertices, indices, 3);
		CHECK(count == 3 && indices[9] == 0xffffffffUL, "capacity: %ld triangles", count);
		return 0;
	}
	/* every index names a vertex the bsp has */
	CASE("indices")
	{
		count = physics_jolt_level_triangles(&bsp, out_vertices, indices, 16);
		for (t = 0; t < 3 * count; t++)
			CHECK(indices[t] < (unsigned long)COUNT(vertices), "index %lu", indices[t]);
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
