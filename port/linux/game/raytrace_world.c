/*
RAYTRACE_WORLD.C

The level's geometry for the ray-traced lighting of the macOS port
(port/linux/src/raytrace_gl.c, port/macos/host/host_metal_rt.m): the active
structure BSP's collision surfaces as a triangle mesh in world units. The
collision surfaces are the level's solid shape - its floors, walls and
ceilings - without the detail of its rendered geometry; the invisible ones
(player clip) are left out.

Each surface is a convex polygon whose edges form a ring: an edge belongs
to two surfaces, and for each it names the next edge around it
(collision_edge.edge_indices, by the side the surface is on). The polygon
is split into a fan of triangles.
*/

#include "cseries.h"
#include "physics/collision_bsp_definitions.h"
#include "scenario/scenario.h"

enum
{
	_collision_surface_invisible_flag = 1 << 1,
};

static struct
{
	const struct collision_bsp *bsp;
	unsigned long generation;
	float *vertices;
	long vertex_count;
	unsigned long *indices;
	long triangle_count;
} world;

static void world_build(const struct collision_bsp *bsp)
{
	const struct collision_surface *surfaces = bsp->surfaces.address;
	const struct collision_edge *edges = bsp->edges.address;
	const struct collision_vertex *vertices = bsp->vertices.address;
	long surface_index, vertex_index, capacity;

	/* (the game's allocator refuses NULL) */
	if (world.vertices)
		free(world.vertices);
	if (world.indices)
		free(world.indices);
	world.vertices = NULL;
	world.indices = NULL;
	world.vertex_count = 0;
	world.triangle_count = 0;
	world.bsp = bsp;
	world.generation++;
	if (!bsp || bsp->vertices.count <= 0 || bsp->surfaces.count <= 0 || bsp->edges.count <= 0)
		return;

	world.vertices = malloc((size_t)bsp->vertices.count * 3 * sizeof(float));
	/* at most MAXIMUM_VERTICES_PER_COLLISION_SURFACE - 2 triangles each */
	capacity = bsp->surfaces.count * (MAXIMUM_VERTICES_PER_COLLISION_SURFACE - 2);
	world.indices = malloc((size_t)capacity * 3 * sizeof(unsigned long));
	if (!world.vertices || !world.indices)
		return;
	for (vertex_index = 0; vertex_index < bsp->vertices.count; vertex_index++)
	{
		world.vertices[vertex_index * 3 + 0] = vertices[vertex_index].point.x;
		world.vertices[vertex_index * 3 + 1] = vertices[vertex_index].point.y;
		world.vertices[vertex_index * 3 + 2] = vertices[vertex_index].point.z;
	}
	world.vertex_count = bsp->vertices.count;

	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		const struct collision_surface *surface = &surfaces[surface_index];
		long ring[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
		long count = 0, edge_index = surface->first_edge_index, steps, corner;

		if (surface->flags & _collision_surface_invisible_flag)
			continue;
		/* the ring, guarded against a malformed one */
		for (steps = 0; steps < MAXIMUM_EDGES_PER_COLLISION_SURFACE * 2; steps++)
		{
			const struct collision_edge *edge;
			int side;

			if (edge_index < 0 || edge_index >= bsp->edges.count)
				break;
			edge = &edges[edge_index];
			side = edge->surface_indices[0] == surface_index ? 0 : 1;
			if (count < MAXIMUM_VERTICES_PER_COLLISION_SURFACE)
				ring[count++] = edge->vertex_indices[side];
			edge_index = edge->edge_indices[side];
			if (edge_index == surface->first_edge_index)
				break;
		}
		for (corner = 1; corner + 1 < count && world.triangle_count < capacity; corner++)
		{
			long a = ring[0], b = ring[corner], c = ring[corner + 1];

			if (a < 0 || b < 0 || c < 0 || a >= world.vertex_count || b >= world.vertex_count ||
				c >= world.vertex_count)
			{
				continue;
			}
			world.indices[world.triangle_count * 3 + 0] = (unsigned long)a;
			world.indices[world.triangle_count * 3 + 1] = (unsigned long)b;
			world.indices[world.triangle_count * 3 + 2] = (unsigned long)c;
			world.triangle_count++;
		}
	}
}

/* the active BSP's mesh: returns its generation, which changes when the
BSP does; 0 while there is none */
unsigned long halo_ray_tracing_world(const float **vertices, long *vertex_count, const unsigned long **indices,
	long *triangle_count)
{
	const struct collision_bsp *bsp = global_structure_bsp_index != NONE ? global_collision_bsp : NULL;

	if (bsp != world.bsp)
		world_build(bsp);
	if (!bsp || !world.triangle_count)
		return 0;
	*vertices = world.vertices;
	*vertex_count = world.vertex_count;
	*indices = world.indices;
	*triangle_count = world.triangle_count;
	return world.generation;
}
