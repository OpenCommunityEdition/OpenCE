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
#include "render/render.h"
#include "tag_files/tag_files.h"

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
		float normal[3];
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
		/* the surface's outward normal: its plane, negated by the
		designator's sign bit */
		{
			long plane_index = surface->plane_designator & LONG_MAX;

			if (plane_index < bsp->bsp3d.planes.count)
			{
				const real_plane3d *plane = (const real_plane3d *)bsp->bsp3d.planes.address + plane_index;
				float sign = (surface->plane_designator & LONG_MIN) ? -1.0f : 1.0f;

				normal[0] = plane->n.i * sign;
				normal[1] = plane->n.j * sign;
				normal[2] = plane->n.k * sign;
			}
			else
			{
				normal[0] = normal[1] = normal[2] = 0.0f;
			}
		}
		for (corner = 1; corner + 1 < count && world.triangle_count < capacity; corner++)
		{
			long a = ring[0], b = ring[corner], c = ring[corner + 1];

			if (a < 0 || b < 0 || c < 0 || a >= world.vertex_count || b >= world.vertex_count ||
				c >= world.vertex_count)
			{
				continue;
			}
			/* wound counterclockwise seen from the outside, so the rays can
			pass out of the level's surfaces from behind them: where the
			rendered surface lies behind its collision surface, a ray
			leaving it does not find the collision surface's back */
			{
				const float *pa = &world.vertices[a * 3], *pb = &world.vertices[b * 3], *pc = &world.vertices[c * 3];
				float u[3] = { pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2] };
				float v[3] = { pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2] };
				float facing = (u[1] * v[2] - u[2] * v[1]) * normal[0] + (u[2] * v[0] - u[0] * v[2]) * normal[1] +
					(u[0] * v[1] - u[1] * v[0]) * normal[2];

				if (facing < 0.0f)
				{
					long swap = b;

					b = c;
					c = swap;
				}
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

/* ---------- the sun

The first light of the visible sky with a direction (the one whose lens
flare the sky draws: render_sky.c) is taken as the sun. */


struct sky_light_view
{
	struct tag_reference lens_flare;
	char marker_name[TAG_STRING_LENGTH + 1];
	byte pad31[0x37];
	real_euler_angles2d direction;
	byte pad70[4];
};

struct sky_view
{
	struct tag_reference model;
	struct tag_reference animation_graph;
	byte pad20[0x8C];
	struct tag_block render_model_regions;
	struct tag_block animations;
	struct tag_block lights;
};

typedef char sky_light_view_size_assert[sizeof(struct sky_light_view) == 0x74 ? 1 : -1];
typedef char sky_view_lights_offset_assert[offsetof(struct sky_view, lights) == 0xC4 ? 1 : -1];

struct sky *scenario_get_sky(short sky_index);

/* the direction towards the sun in the world (unit length); FALSE if the
visible sky has none */
boolean halo_ray_tracing_sun(float *direction)
{
	const struct sky_view *sky;
	long index;

	if (render.visible_sky_index == NONE)
		return FALSE;
	sky = (const struct sky_view *)scenario_get_sky(render.visible_sky_index);
	if (!sky)
		return FALSE;
	for (index = 0; index < sky->lights.count; index++)
	{
		const struct sky_light_view *light = (const struct sky_light_view *)sky->lights.address + index;
		real_vector3d vector;

		if (light->lens_flare.index == NONE)
			continue;
		vector3d_from_euler_angles2d(&vector, &light->direction);
		direction[0] = vector.i;
		direction[1] = vector.j;
		direction[2] = vector.k;
		return TRUE;
	}
	return FALSE;
}
