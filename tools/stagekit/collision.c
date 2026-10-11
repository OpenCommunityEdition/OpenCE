/*
COLLISION.C

Collision geometry for StageKit's tags: a box, hollow (the chamber, walls
facing in) or solid (an obstacle, faces out). Each convention is the engine's:
bsp3d_test_point takes a node's second child in front of its plane, a child
with the sign bit a leaf (NONE solid); a leaf's bsp2d references name the
planes the collision tests push for its side (the front: no sign bit), each a
surface itself (a bsp2d root with the sign bit); a surface's ring takes each
edge's vertex and next edge on its side (collision_surface_polygon).
*/

/* ---------- headers */

#include "stagekit.h"

/* ---------- constants */

/* (an index with its high bit set names a leaf or a surface, not a node:
collision_bsp.c) */
#define LEAF_BIT ((long)LONG_MIN)

enum
{
	NUMBER_OF_BOX_WALLS = 6,
	NUMBER_OF_BOX_CORNERS = 8,
	NUMBER_OF_BOX_EDGES = 12,
};

/* ---------- structures */

/* (collision_bsp.c keeps these to itself: its layout, which its size checks fix) */
struct collision_leaf
{
	word flags;
	short bsp2d_reference_count;
	long first_bsp2d_reference_index;
};

struct bsp2d_reference
{
	long plane_designator;
	long root_index;
};

typedef char collision_leaf_size_check[sizeof(struct collision_leaf) == 0x08 ? 1 : -1];
typedef char bsp2d_reference_size_check[sizeof(struct bsp2d_reference) == 0x08 ? 1 : -1];

/* ---------- public code */

void box_build(
	struct collision_bsp *collision,
	void *(*block_new)(struct tag_block *, long, unsigned long),
	real_rectangle3d const *bounds,
	boolean solid)
{
	/* each wall's plane facing into the box: a solid box's face out */
	real const walls[NUMBER_OF_BOX_WALLS][4] = {
		{ 0.f, 0.f, 1.f, bounds->z0 }, { 0.f, 0.f, -1.f, -bounds->z1 },
		{ 1.f, 0.f, 0.f, bounds->x0 }, { -1.f, 0.f, 0.f, -bounds->x1 },
		{ 0.f, 1.f, 0.f, bounds->y0 }, { 0.f, -1.f, 0.f, -bounds->y1 } };
	real const facing = solid ? -1.f : 1.f;
	real_plane3d *planes = block_new(&collision->bsp3d.planes, NUMBER_OF_BOX_WALLS, sizeof(real_plane3d));
	struct bsp3d_node *nodes = block_new(&collision->bsp3d.nodes, NUMBER_OF_BOX_WALLS, sizeof(struct bsp3d_node));
	struct collision_leaf *leaf = block_new(&collision->leaves, 1, sizeof(struct collision_leaf));
	struct bsp2d_reference *references = block_new(&collision->bsp2d_references, NUMBER_OF_BOX_WALLS,
		sizeof(struct bsp2d_reference));
	struct collision_surface *surfaces = block_new(&collision->surfaces, NUMBER_OF_BOX_WALLS,
		sizeof(struct collision_surface));
	struct collision_edge *edges = block_new(&collision->edges, NUMBER_OF_BOX_EDGES, sizeof(struct collision_edge));
	struct collision_vertex *vertices = block_new(&collision->vertices, NUMBER_OF_BOX_CORNERS,
		sizeof(struct collision_vertex));
	long edge_count = 0;
	long wall;
	long corner;

	/* (a box of some size every way: each wall then has four corners) */
	if (!(bounds->x0 < bounds->x1 && bounds->y0 < bounds->y1 && bounds->z0 < bounds->z1))
	{
		fprintf(stderr, "stagekit: a box is %g by %g by %g\n", bounds->x1 - bounds->x0, bounds->y1 - bounds->y0,
			bounds->z1 - bounds->z0);
		exit(1);
	}
	for (corner = 0; corner < NUMBER_OF_BOX_CORNERS; corner++)
	{
		vertices[corner].point.x = corner & 1 ? bounds->x1 : bounds->x0;
		vertices[corner].point.y = corner & 2 ? bounds->y1 : bounds->y0;
		vertices[corner].point.z = corner & 4 ? bounds->z1 : bounds->z0;
		vertices[corner].first_edge_index = NONE;
	}
	leaf->bsp2d_reference_count = NUMBER_OF_BOX_WALLS;
	leaf->first_bsp2d_reference_index = 0;
	for (wall = 0; wall < NUMBER_OF_BOX_WALLS; wall++)
	{
		long ring[4];
		long ring_edges[4];
		long count = 0;
		real winding;
		long point;

		planes[wall].n.i = facing * walls[wall][0];
		planes[wall].n.j = facing * walls[wall][1];
		planes[wall].n.k = facing * walls[wall][2];
		planes[wall].d = facing * walls[wall][3];
		nodes[wall].plane_designator = wall;
		/* a hollow box: in front of every wall, the open leaf; a solid one: in
		front of any face, the open leaf, behind them all, solid */
		if (solid)
		{
			nodes[wall].children[1] = LEAF_BIT;
			nodes[wall].children[0] = wall + 1 < NUMBER_OF_BOX_WALLS ? wall + 1 : NONE;
		}
		else
		{
			nodes[wall].children[0] = NONE;
			nodes[wall].children[1] = wall + 1 < NUMBER_OF_BOX_WALLS ? wall + 1 : LEAF_BIT;
		}
		references[wall].plane_designator = wall;
		references[wall].root_index = LEAF_BIT | wall;
		surfaces[wall].plane_designator = wall;
		surfaces[wall].flags = 0;
		surfaces[wall].breakable_surface_index = 0xFF;
		surfaces[wall].material_index = NONE;

		/* the wall's corners (on its plane), in order round it, then turned so
		that they go counterclockwise seen from in front */
		for (corner = 0; corner < NUMBER_OF_BOX_CORNERS; corner++)
		{
			if (plane3d_distance_to_point(&planes[wall], &vertices[corner].point) == 0.f)
				ring[count++] = corner;
		}
		/* (the box's corners on a face, by bit order, go 0 1 3 2 round it) */
		{
			long ordered[4] = { ring[0], ring[1], ring[3], ring[2] };
			real_vector3d u, v;

			u.i = vertices[ordered[1]].point.x - vertices[ordered[0]].point.x;
			u.j = vertices[ordered[1]].point.y - vertices[ordered[0]].point.y;
			u.k = vertices[ordered[1]].point.z - vertices[ordered[0]].point.z;
			v.i = vertices[ordered[2]].point.x - vertices[ordered[1]].point.x;
			v.j = vertices[ordered[2]].point.y - vertices[ordered[1]].point.y;
			v.k = vertices[ordered[2]].point.z - vertices[ordered[1]].point.z;
			winding = (u.j * v.k - u.k * v.j) * planes[wall].n.i + (u.k * v.i - u.i * v.k) * planes[wall].n.j +
				(u.i * v.j - u.j * v.i) * planes[wall].n.k;
			for (point = 0; point < 4; point++)
				ring[point] = winding > 0.f ? ordered[point] : ordered[3 - point];
		}
		/* each side of the ring an edge: the other wall's (the other way round) or a new one */
		for (point = 0; point < 4; point++)
		{
			long from = ring[point];
			long to = ring[(point + 1) % 4];
			long edge_index;

			for (edge_index = 0; edge_index < edge_count; edge_index++)
			{
				if (edges[edge_index].vertex_indices[0] == to && edges[edge_index].vertex_indices[1] == from)
					break;
			}
			if (edge_index == edge_count)
			{
				edges[edge_index].vertex_indices[0] = from;
				edges[edge_index].vertex_indices[1] = to;
				edges[edge_index].surface_indices[0] = wall;
				edges[edge_index].surface_indices[1] = NONE;
				edges[edge_index].edge_indices[0] = edges[edge_index].edge_indices[1] = NONE;
				edge_count++;
			}
			else
			{
				edges[edge_index].surface_indices[1] = wall;
			}
			if (vertices[from].first_edge_index == NONE)
				vertices[from].first_edge_index = edge_index;
			ring_edges[point] = edge_index;
		}
		for (point = 0; point < 4; point++)
		{
			struct collision_edge *edge = &edges[ring_edges[point]];

			edge->edge_indices[edge->surface_indices[1] == wall] = ring_edges[(point + 1) % 4];
		}
		surfaces[wall].first_edge_index = ring_edges[0];
	}
	collision->edges.count = edge_count;
}

