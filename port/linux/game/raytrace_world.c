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
#include "objects/objects.h"
#include "objects/object_types.h"
#include "objects/object_definitions.h"
#include "models/model_definitions.h"
#include "game/players.h"

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

/* ---------- the objects

The units (the bipeds and the vehicles) as triangles for the rays, in the
world, each frame: their drawn models, skinned as the renderer skins them
(display.ray_tracing_shapes "model", at the high level of detail), or
their collision models - the meshes the game tests its
bullets against, a mesh for each node's region as it now is (its damage
permutation), placed by the node's matrix as the animation poses it (between
the last two ticks, as the frame draws it). A unit without them is its
skeleton's bones, each an ellipsoid from its node to its parent's (bipeds),
or its bounding sphere flattened along its axes. Each triangle's group: its
object (0 to 31, the player's first) above, its kind below (2 an object, 4
the local player's body: the first person does not draw it, so only the
rays show its shadow). */

#include "physics/collision_model_definitions.h"

enum
{
	_ray_mask_object = 2,
	_ray_mask_player = 4,
};

/* the objects nearer the camera than this (world units) are in the rays */
#define RAY_TRACED_OBJECT_DISTANCE 25.0f
#define RAY_TRACED_OBJECT_GROUPS 32

/* the model geometry's layout (models.c keeps it private) */
struct model_geometry_view
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct model_geometry_part_view
{
	unsigned long flags;
	short shader_index;
	char previous_part_index;
	char next_part_index;
	short centroid_primary_node_index;
	short centroid_secondary_node_index;
	real centroid_primary_node_weight;
	real centroid_secondary_node_weight;
	real_point3d centroid;
	struct tag_block uncompressed_vertices;
	struct tag_block compressed_vertices;
	struct tag_block triangles;
	struct
	{
		short type;
		word pad;
		long count;
		void *base_address;
		void *hardware_format;
	} triangle_buffer;
	struct
	{
		short type;
		word pad;
		long count;
		long offset;
		void *base_address;
		void *hardware_format;
	} vertex_buffer;
};

/* a compressed model vertex, as the renderer reads it (rasterizer.c) */
struct model_vertex_view
{
	real_point3d position;
	unsigned long normal;
	unsigned long binormal;
	unsigned long tangent;
	short texture_coordinates[2];
	char node_indices[2];
	short node_weight;
};

typedef char model_geometry_part_view_size[sizeof(struct model_geometry_part_view) == 0x68 ? 1 : -1];
typedef char model_vertex_view_size[sizeof(struct model_vertex_view) == 32 ? 1 : -1];

/* a collision mesh's triangles, in its node's space (9 floats each), made
once for each mesh while its map is loaded */
struct mesh_triangles
{
	const struct collision_bsp *bsp;
	unsigned long generation;
	float *triangles;
	long count;
};

static struct mesh_triangles meshes[512];
static long mesh_count;

static long bsp_triangulate(const struct collision_bsp *bsp, float *out, long maximum)
{
	const struct collision_surface *surfaces = bsp->surfaces.address;
	const struct collision_edge *edges = bsp->edges.address;
	const struct collision_vertex *vertices = bsp->vertices.address;
	long surface_index, count = 0;

	for (surface_index = 0; surface_index < bsp->surfaces.count && count < maximum; surface_index++)
	{
		const struct collision_surface *surface = &surfaces[surface_index];
		long ring[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
		long ring_count = 0, edge_index = surface->first_edge_index, steps, corner, plane_index;
		float normal[3] = { 0.0f, 0.0f, 0.0f };

		for (steps = 0; steps < MAXIMUM_EDGES_PER_COLLISION_SURFACE * 2; steps++)
		{
			const struct collision_edge *edge;
			int side;

			if (edge_index < 0 || edge_index >= bsp->edges.count)
				break;
			edge = &edges[edge_index];
			side = edge->surface_indices[0] == surface_index ? 0 : 1;
			if (ring_count < MAXIMUM_VERTICES_PER_COLLISION_SURFACE)
				ring[ring_count++] = edge->vertex_indices[side];
			edge_index = edge->edge_indices[side];
			if (edge_index == surface->first_edge_index)
				break;
		}
		plane_index = surface->plane_designator & LONG_MAX;
		if (plane_index < bsp->bsp3d.planes.count)
		{
			const real_plane3d *plane = (const real_plane3d *)bsp->bsp3d.planes.address + plane_index;
			float sign = (surface->plane_designator & LONG_MIN) ? -1.0f : 1.0f;

			normal[0] = plane->n.i * sign;
			normal[1] = plane->n.j * sign;
			normal[2] = plane->n.k * sign;
		}
		for (corner = 1; corner + 1 < ring_count && count < maximum; corner++)
		{
			long a = ring[0], b = ring[corner], c = ring[corner + 1];
			const float *pa, *pb, *pc;
			float u[3], v[3], facing, *t = out + count * 9;

			if (a < 0 || b < 0 || c < 0 || a >= bsp->vertices.count || b >= bsp->vertices.count ||
				c >= bsp->vertices.count)
			{
				continue;
			}
			pa = &vertices[a].point.x;
			pb = &vertices[b].point.x;
			pc = &vertices[c].point.x;
			/* wound counterclockwise seen from outside, as the level's */
			u[0] = pb[0] - pa[0];
			u[1] = pb[1] - pa[1];
			u[2] = pb[2] - pa[2];
			v[0] = pc[0] - pa[0];
			v[1] = pc[1] - pa[1];
			v[2] = pc[2] - pa[2];
			facing = (u[1] * v[2] - u[2] * v[1]) * normal[0] + (u[2] * v[0] - u[0] * v[2]) * normal[1] +
				(u[0] * v[1] - u[1] * v[0]) * normal[2];
			if (facing < 0.0f)
			{
				const float *swap = pb;

				pb = pc;
				pc = swap;
			}
			memcpy(t, pa, 3 * sizeof(float));
			memcpy(t + 3, pb, 3 * sizeof(float));
			memcpy(t + 6, pc, 3 * sizeof(float));
			count++;
		}
	}
	return count;
}

/* the mesh's triangles, made the first time it is asked for; NULL if none */
static const struct mesh_triangles *mesh_get(const struct collision_bsp *bsp)
{
	long index, capacity;
	struct mesh_triangles *mesh;

	for (index = 0; index < mesh_count; index++)
	{
		if (meshes[index].bsp == bsp && meshes[index].generation == world.generation)
			return meshes[index].count > 0 ? &meshes[index] : NULL;
	}
	/* a new map: the meshes of the last are gone */
	if (mesh_count && meshes[0].generation != world.generation)
	{
		for (index = 0; index < mesh_count; index++)
		{
			if (meshes[index].triangles)
				free(meshes[index].triangles);
		}
		mesh_count = 0;
	}
	if (mesh_count >= (long)(sizeof(meshes) / sizeof(meshes[0])))
		return NULL;
	mesh = &meshes[mesh_count++];
	mesh->bsp = bsp;
	mesh->generation = world.generation;
	mesh->triangles = NULL;
	mesh->count = 0;
	capacity = bsp->surfaces.count * (MAXIMUM_VERTICES_PER_COLLISION_SURFACE - 2);
	if (capacity <= 0)
		return NULL;
	mesh->triangles = malloc((size_t)capacity * 9 * sizeof(float));
	if (!mesh->triangles)
		return NULL;
	mesh->count = bsp_triangulate(bsp, mesh->triangles, capacity);
	return mesh->count > 0 ? mesh : NULL;
}

/* a unit sphere (an icosahedron: 20 triangles, wound as the level's) */
static const float icosahedron[20][9] = {
#define ICO_T 1.6180340f
#define ICO_V(x, y, z) (x) * 0.5257311f * 1.12f, (y) * 0.5257311f * 1.12f, (z) * 0.5257311f * 1.12f
	{ ICO_V(-1, ICO_T, 0), ICO_V(0, 1, ICO_T), ICO_V(-ICO_T, 0, 1) },
	{ ICO_V(-1, ICO_T, 0), ICO_V(1, ICO_T, 0), ICO_V(0, 1, ICO_T) },
	{ ICO_V(-1, ICO_T, 0), ICO_V(0, 1, -ICO_T), ICO_V(1, ICO_T, 0) },
	{ ICO_V(-1, ICO_T, 0), ICO_V(-ICO_T, 0, -1), ICO_V(0, 1, -ICO_T) },
	{ ICO_V(-1, ICO_T, 0), ICO_V(-ICO_T, 0, 1), ICO_V(-ICO_T, 0, -1) },
	{ ICO_V(1, ICO_T, 0), ICO_V(ICO_T, 0, 1), ICO_V(0, 1, ICO_T) },
	{ ICO_V(0, 1, ICO_T), ICO_V(0, -1, ICO_T), ICO_V(-ICO_T, 0, 1) },
	{ ICO_V(-ICO_T, 0, 1), ICO_V(-1, -ICO_T, 0), ICO_V(-ICO_T, 0, -1) },
	{ ICO_V(-ICO_T, 0, -1), ICO_V(0, -1, -ICO_T), ICO_V(0, 1, -ICO_T) },
	{ ICO_V(0, 1, -ICO_T), ICO_V(ICO_T, 0, -1), ICO_V(1, ICO_T, 0) },
	{ ICO_V(1, -ICO_T, 0), ICO_V(0, -1, ICO_T), ICO_V(ICO_T, 0, 1) },
	{ ICO_V(1, -ICO_T, 0), ICO_V(-1, -ICO_T, 0), ICO_V(0, -1, ICO_T) },
	{ ICO_V(1, -ICO_T, 0), ICO_V(0, -1, -ICO_T), ICO_V(-1, -ICO_T, 0) },
	{ ICO_V(1, -ICO_T, 0), ICO_V(ICO_T, 0, -1), ICO_V(0, -1, -ICO_T) },
	{ ICO_V(1, -ICO_T, 0), ICO_V(ICO_T, 0, 1), ICO_V(ICO_T, 0, -1) },
	{ ICO_V(0, -1, ICO_T), ICO_V(0, 1, ICO_T), ICO_V(ICO_T, 0, 1) },
	{ ICO_V(-1, -ICO_T, 0), ICO_V(-ICO_T, 0, 1), ICO_V(0, -1, ICO_T) },
	{ ICO_V(0, -1, -ICO_T), ICO_V(-ICO_T, 0, -1), ICO_V(-1, -ICO_T, 0) },
	{ ICO_V(ICO_T, 0, -1), ICO_V(0, 1, -ICO_T), ICO_V(0, -1, -ICO_T) },
	{ ICO_V(ICO_T, 0, 1), ICO_V(1, ICO_T, 0), ICO_V(ICO_T, 0, -1) },
#undef ICO_V
#undef ICO_T
};

/* an ellipsoid (a unit sphere under axes u, v, w about center) as
triangles into out; returns how many (20, or 0 without room) */
static long ellipsoid_triangles(float *out, long room, const float *center, const float *u, const float *v,
	const float *w)
{
	long triangle, corner;

	if (room < 20)
		return 0;
	for (triangle = 0; triangle < 20; triangle++)
	{
		for (corner = 0; corner < 3; corner++)
		{
			const float *p = &icosahedron[triangle][corner * 3];
			float *q = out + triangle * 9 + corner * 3;
			int axis;

			/* (the table's winding checked: each face's normal points out) */
			for (axis = 0; axis < 3; axis++)
				q[axis] = center[axis] + u[axis] * p[0] + v[axis] * p[1] + w[axis] * p[2];
		}
	}
	return 20;
}

/* a bone from a to b, r thick each way, ending in round caps */
static long bone_triangles(float *out, long room, const real_point3d *a, const real_point3d *b, float r)
{
	float center[3] = { (a->x + b->x) * 0.5f, (a->y + b->y) * 0.5f, (a->z + b->z) * 0.5f };
	float d[3] = { b->x - a->x, b->y - a->y, b->z - a->z };
	float length = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
	float u[3], v[3], w[3], half, l;

	if (length < 1e-4f)
	{
		float x[3] = { r, 0, 0 }, y[3] = { 0, r, 0 }, z[3] = { 0, 0, r };

		return ellipsoid_triangles(out, room, center, x, y, z);
	}
	d[0] /= length;
	d[1] /= length;
	d[2] /= length;
	if (fabsf(d[2]) < 0.9f)
	{
		v[0] = -d[1];
		v[1] = d[0];
		v[2] = 0.0f;
	}
	else
	{
		v[0] = 0.0f;
		v[1] = -d[2];
		v[2] = d[1];
	}
	l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
	w[0] = d[1] * v[2] - d[2] * v[1];
	w[1] = d[2] * v[0] - d[0] * v[2];
	w[2] = d[0] * v[1] - d[1] * v[0];
	half = length * 0.5f + r * 0.6f;
	u[0] = d[0] * half;
	u[1] = d[1] * half;
	u[2] = d[2] * half;
	v[0] *= r;
	v[1] *= r;
	v[2] *= r;
	w[0] *= r;
	w[1] *= r;
	w[2] *= r;
	return ellipsoid_triangles(out, room, center, u, v, w);
}

/* the shapes for the rays: the drawn models, the collision models, or
ellipsoids (display.ray_tracing_shapes) */
enum
{
	_ray_shapes_model,
	_ray_shapes_collision,
	_ray_shapes_simple,
};

/* the model's level of detail in the rays: high (of super low to super high) */
#define RAY_TRACED_MODEL_DETAIL_LEVEL 3
#define RAY_TRACED_MAXIMUM_NODES 128

/* the object's drawn model, skinned as the renderer skins it (each vertex
by its two nodes' matrices, each the node's pose times its inverse default
pose: models.c), its regions as they are now; returns how many triangles */
static long model_triangles(struct object_datum *object, const real_matrix4x3 *matrices, float *out, long room)
{
	const struct object_definition *definition = object_definition_get(object->definition_index);
	const struct model *model;
	const struct model_node *nodes;
	static real_matrix4x3 relative[RAY_TRACED_MAXIMUM_NODES];
	long count = 0, node_index, region_index;

	if (!matrices || definition->object.model.index == NONE)
		return 0;
	model = model_definition_get(definition->object.model.index);
	if (model->nodes.count <= 0 || model->nodes.count > RAY_TRACED_MAXIMUM_NODES)
		return 0;
	nodes = (const struct model_node *)model->nodes.address;
	for (node_index = 0; node_index < model->nodes.count; node_index++)
		matrix4x3_multiply(&matrices[node_index], &nodes[node_index].runtime_default_inverse_matrix, &relative[node_index]);
	for (region_index = 0; region_index < model->regions.count && count < room; region_index++)
	{
		const struct model_region *region = (const struct model_region *)model->regions.address + region_index;
		const struct model_region_permutation *permutation;
		const struct model_geometry_view *geometry;
		char permutation_index = object->object.region_permutations[region_index];
		short geometry_index, part_index;

		if (permutation_index == NONE || permutation_index >= region->permutations.count)
			continue;
		permutation = (const struct model_region_permutation *)region->permutations.address + permutation_index;
		/* (high, or the nearest level of detail the model has) */
		{
			short level;

			geometry_index = NONE;
			for (level = RAY_TRACED_MODEL_DETAIL_LEVEL; level >= 0 && geometry_index == NONE; level--)
				geometry_index = permutation->geometry_indices[level];
			for (level = RAY_TRACED_MODEL_DETAIL_LEVEL + 1; level < 5 && geometry_index == NONE; level++)
				geometry_index = permutation->geometry_indices[level];
		}
		if (geometry_index == NONE || geometry_index >= model->geometries.count)
			continue;
		geometry = (const struct model_geometry_view *)model->geometries.address + geometry_index;
		for (part_index = 0; part_index < geometry->parts.count && count < room; part_index++)
		{
			const struct model_geometry_part_view *part = (const struct model_geometry_part_view *)geometry->parts.address +
				part_index;
			/* the part's strip and vertices: in a cache file, not in the tag
			blocks but in its buffers - the strip in memory, the vertices at a
			physical address, which the CPU sees in the window at 0x80000000 */
			const unsigned short *strip = (const unsigned short *)part->triangle_buffer.base_address;
			unsigned long vertex_address = (unsigned long)part->vertex_buffer.base_address;
			const byte *vertex_data;
			boolean compressed = part->vertex_buffer.type == 5;
			long index, strip_count = part->triangle_buffer.count + 2, vertex_count = part->vertex_buffer.count;
			long vertex_size = compressed ? (long)sizeof(struct model_vertex_view) : 68;

			if (vertex_address && vertex_address < 0x80000000UL)
				vertex_address |= 0x80000000UL;
			vertex_data = (const byte *)vertex_address + part->vertex_buffer.offset * vertex_size;
			/* (or the tag blocks, where they are kept) */
			if (!strip || !vertex_address)
			{
				strip = (const unsigned short *)part->triangles.address;
				vertex_data = (const byte *)part->compressed_vertices.address;
				vertex_count = part->compressed_vertices.count;
				compressed = TRUE;
				vertex_size = (long)sizeof(struct model_vertex_view);
			}

			/* (HALO_RT_LOG_SHAPES: each model's parts' data, once) */
			if (getenv("HALO_RT_LOG_SHAPES") && part_index == 0 && region_index == 0)
			{
				extern void platform_log(const char *format, ...);
				static long logged[64];
				static int logged_count;
				int seen = 0, k;

				for (k = 0; k < logged_count; k++)
					seen |= logged[k] == definition->object.model.index;
				if (!seen && logged_count < 64)
				{
					logged[logged_count++] = definition->object.model.index;
					platform_log("ray tracing model %ld: part flags %lx, triangles block %ld at %p, compressed %ld at %p, "
						"uncompressed %ld at %p, triangle buffer type %d count %ld at %p, vertex buffer type %d count %ld "
						"offset %ld at %p", definition->object.model.index, (unsigned long)part->flags,
						(long)part->triangles.count, part->triangles.address, (long)part->compressed_vertices.count,
						part->compressed_vertices.address, (long)part->uncompressed_vertices.count,
						part->uncompressed_vertices.address, part->triangle_buffer.type, part->triangle_buffer.count,
						part->triangle_buffer.base_address, part->vertex_buffer.type, part->vertex_buffer.count,
						part->vertex_buffer.offset, part->vertex_buffer.base_address);
				}
			}
			if ((part->flags & 1) || !strip || !vertex_data || vertex_count <= 0 || part->triangle_buffer.type != 1 ||
				(part->vertex_buffer.type != 4 && part->vertex_buffer.type != 5 && part->vertex_buffer.base_address))
			{
				continue;
			}
			for (index = 0; index + 2 < strip_count && count < room; index++)
			{
				unsigned short corners[3] = { strip[index], strip[index + 1], strip[index + 2] };
				int corner;

				if (corners[0] == corners[1] || corners[1] == corners[2] || corners[0] == corners[2] ||
					corners[0] >= vertex_count || corners[1] >= vertex_count || corners[2] >= vertex_count)
				{
					continue;
				}
				for (corner = 0; corner < 3; corner++)
				{
					const byte *raw = vertex_data + corners[corner] * vertex_size;
					const real_point3d *position = (const real_point3d *)raw;
					short node0, node1;
					float weight0;
					real_point3d point0 = *position, point1 = *position;

					if (compressed)
					{
						const struct model_vertex_view *vertex = (const struct model_vertex_view *)raw;

						/* (three times the node's index, a byte: past 42 nodes, over 127) */
						node0 = (short)((unsigned char)vertex->node_indices[0] / 3);
						node1 = (short)((unsigned char)vertex->node_indices[1] / 3);
						weight0 = (float)vertex->node_weight * (1.0f / 32767.0f);
					}
					else
					{
						/* model_vertex_uncompressed: position, normal, binormal, tangent,
						texcoord (56 bytes), then two node indices and their weights */
						node0 = *(const short *)(raw + 56);
						node1 = *(const short *)(raw + 58);
						weight0 = *(const float *)(raw + 60);
					}
					float *q = out + count * 9 + corner * 3;

					if (node0 >= 0 && node0 < model->nodes.count)
						matrix4x3_transform_point(&relative[node0], position, &point0);
					if (node1 >= 0 && node1 < model->nodes.count)
						matrix4x3_transform_point(&relative[node1], position, &point1);
					else
						point1 = point0;
					q[0] = point0.x * weight0 + point1.x * (1.0f - weight0);
					q[1] = point0.y * weight0 + point1.y * (1.0f - weight0);
					q[2] = point0.z * weight0 + point1.z * (1.0f - weight0);
				}
				count++;
			}
		}
	}
	return count;
}

/* one object's triangles into out (at most room), of the shapes; returns
how many */
static long object_triangles(long object_index, struct object_datum *object, float *out, long room, long shapes)
{
	const struct object_definition *definition = object_definition_get(object->definition_index);
	const real_matrix4x3 *matrices = object_get_node_matrices(object_index);
	float radius = object->object.bounding_sphere_radius;
	long count = 0;

	/* the drawn model */
	if (shapes == _ray_shapes_model)
	{
		count = model_triangles(object, matrices, out, room);
		if (count > 0)
			return count;
	}
	/* the collision model: its meshes, where the game's bullets hit */
	if (shapes != _ray_shapes_simple && matrices && definition->object.collision_model.index != NONE)
	{
		const struct collision_model *model = collision_model_definition_get(definition->object.collision_model.index);
		const struct collision_node *nodes = (const struct collision_node *)model->nodes.address;
		short node_index;

		for (node_index = 0; node_index < model->nodes.count && count < room; node_index++)
		{
			const struct collision_node *node = &nodes[node_index];
			const struct mesh_triangles *mesh;
			short permutation;
			long triangle;

			if (node->region_index == NONE || node->bsps.count <= 0)
				continue;
			permutation = object->object.region_permutations[node->region_index];
			if (permutation == NONE)
				continue;
			permutation = PIN(permutation, 0, node->bsps.count - 1);
			mesh = mesh_get((const struct collision_bsp *)node->bsps.address + permutation);
			if (!mesh)
				continue;
			for (triangle = 0; triangle < mesh->count && count < room; triangle++, count++)
			{
				int corner;

				for (corner = 0; corner < 3; corner++)
				{
					const float *p = &mesh->triangles[triangle * 9 + corner * 3];
					real_point3d local = { p[0], p[1], p[2] }, placed;

					matrix4x3_transform_point(&matrices[node_index], &local, &placed);
					out[count * 9 + corner * 3 + 0] = placed.x;
					out[count * 9 + corner * 3 + 1] = placed.y;
					out[count * 9 + corner * 3 + 2] = placed.z;
				}
			}
		}
		if (count > 0)
			return count;
	}
	/* without (or simple shapes): a biped's bones */
	if (object->object.type == _object_type_biped && matrices && definition->object.model.index != NONE)
	{
		const struct model *model = model_definition_get(definition->object.model.index);
		const struct model_node *nodes = (const struct model_node *)model->nodes.address;
		long node_index;
		float r = PIN(radius * 0.13f, 0.02f, 0.12f);

		for (node_index = 0; node_index < model->nodes.count; node_index++)
		{
			short parent = nodes[node_index].parent_node_index;

			if (parent < 0 || parent >= model->nodes.count)
				continue;
			count += bone_triangles(out + count * 9, room - count, &matrices[parent].position,
				&matrices[node_index].position, r);
		}
		return count;
	}
	/* or its bounding sphere along its axes */
	{
		const real_vector3d *forward = &object->object.forward, *up = &object->object.up;
		float center[3] = { object->object.bounding_sphere_center.x, object->object.bounding_sphere_center.y,
			object->object.bounding_sphere_center.z };
		float u[3] = { forward->i * radius * 0.85f, forward->j * radius * 0.85f, forward->k * radius * 0.85f };
		float w[3] = { up->i * radius * 0.35f, up->j * radius * 0.35f, up->k * radius * 0.35f };
		float v[3] = { (up->j * forward->k - up->k * forward->j) * radius * 0.45f,
			(up->k * forward->i - up->i * forward->k) * radius * 0.45f,
			(up->i * forward->j - up->j * forward->i) * radius * 0.45f };

		return ellipsoid_triangles(out, room, center, u, v, w);
	}
}

/* the objects in the rays: all that the game draws as models - units,
items, projectiles, scenery, devices */
#define RAY_TRACED_OBJECT_TYPES (_object_mask_unit | _object_mask_item | _object_mask_projectile | \
	_object_mask_scenery | _object_mask_device)

/* an object and what it carries (its children: a unit's weapon), in one
group; returns how many triangles */
static long object_family_triangles(long object_index, struct object_datum *object, float *out, long room,
	long shapes, unsigned char group, unsigned char *groups)
{
	long count = object_triangles(object_index, object, out, room, shapes), child_index, index, guard = 0;

	for (child_index = object->object.first_child_object_index; child_index != NONE && count < room && guard < 16;
		guard++)
	{
		struct object_datum *child = object_try_and_get_and_verify_type(child_index, RAY_TRACED_OBJECT_TYPES);

		if (!child)
			break;
		count += object_triangles(child_index, child, out + count * 9, room - count, shapes);
		child_index = child->object.next_object_index;
	}
	for (index = 0; index < count; index++)
		groups[index] = group;
	return count;
}

/* this frame's objects near the camera as triangles (9 floats each, at most
maximum) and each triangle's group, and the player's body's bounding sphere
(center, radius; radius 0 if none); returns how many triangles. The player
and what it carries are group 0; the other units (with what they carry)
each a group; the loose objects (items on the ground, projectiles,
scenery, devices) share the last. */
long halo_ray_tracing_objects(float *triangles, unsigned char *groups, long maximum, const float *camera,
	float *player_sphere, long shapes)
{
	struct object_iterator iterator;
	struct object_datum *object;
	long count = 0, player_unit = NONE, player_index, group = 1, index;
	const unsigned char loose = (unsigned char)((RAY_TRACED_OBJECT_GROUPS - 1) << 3 | _ray_mask_object);

	player_sphere[0] = player_sphere[1] = player_sphere[2] = player_sphere[3] = 0.0f;
	if (global_structure_bsp_index == NONE || !object_header_data)
		return 0;
	player_index = local_player_get_player_index(0);
	if (player_index != NONE)
		player_unit = player_get(player_index)->unit_index;
	if (player_unit != NONE && (object = object_try_and_get_and_verify_type(player_unit, _object_mask_unit)) != NULL &&
		object->object.bounding_sphere_radius > 0.0f)
	{
		player_sphere[0] = object->object.bounding_sphere_center.x;
		player_sphere[1] = object->object.bounding_sphere_center.y;
		player_sphere[2] = object->object.bounding_sphere_center.z;
		player_sphere[3] = object->object.bounding_sphere_radius;
		count += object_family_triangles(player_unit, object, triangles, maximum, shapes, _ray_mask_player, groups);
	}
	object_iterator_new(&iterator, RAY_TRACED_OBJECT_TYPES, 0);
	while ((object = (struct object_datum *)object_iterator_next(&iterator)) != NULL && count < maximum)
	{
		float radius = object->object.bounding_sphere_radius;
		float dx = object->object.bounding_sphere_center.x - camera[0];
		float dy = object->object.bounding_sphere_center.y - camera[1];
		float dz = object->object.bounding_sphere_center.z - camera[2];
		float reach = RAY_TRACED_OBJECT_DISTANCE + radius;
		boolean unit = ((1UL << object->object.type) & _object_mask_unit) != 0;
		long added;

		/* (what something carries goes with it) */
		if (iterator.index == player_unit || object->object.parent_object_index != NONE || !(radius > 0.0f) ||
			radius > 20.0f || dx * dx + dy * dy + dz * dz > reach * reach)
		{
			continue;
		}
		if (unit && group < RAY_TRACED_OBJECT_GROUPS - 1)
		{
			added = object_family_triangles(iterator.index, object, triangles + count * 9, maximum - count, shapes,
				(unsigned char)(group << 3 | _ray_mask_object), groups + count);
			if (added > 0)
				group++;
		}
		else
		{
			added = object_family_triangles(iterator.index, object, triangles + count * 9, maximum - count, shapes,
				loose, groups + count);
		}
		count += added;
	}
	(void)index;
	return count;
}

/* ---------- the emitters

The glowing things the game draws without a light of their own - a
needle, a plasma bolt's glow, Guilty Spark's eye, a glowing panel: any
object with a light volume (the glow's sprite) attached or as a widget, and
no light - as lights for the rays: each lights what is near it in its
glow's colour, with its shadows. (Those with a light the game draws it,
and the rays shadow it: object_lights.c.) */

#include "objects/widgets/light_volumes.h"

#define RAY_TRACED_EMITTER_DISTANCE 30.0f
/* how far an emitter's light reaches (world units), and how bright */
#define RAY_TRACED_EMITTER_RADIUS 3.0f
#define RAY_TRACED_EMITTER_INTENSITY 2.0f

#define GROUP_TAG_LIGHT_VOLUME 0x6D677332 /* 'mgs2' */
#define GROUP_TAG_LIGHT 0x6C696768 /* 'ligh' */

/* a light volume's colour (its first frame's near colour); FALSE if dark */
static boolean light_volume_color(long definition_index, float *color)
{
	const struct light_volume_definition *volume = light_volume_definition_get(definition_index);
	const struct light_volume_frame *frame;

	if (volume->frames.count <= 0)
		return FALSE;
	frame = (const struct light_volume_frame *)volume->frames.address;
	color[0] = frame->color_hither.red;
	color[1] = frame->color_hither.green;
	color[2] = frame->color_hither.blue;
	return color[0] + color[1] + color[2] > 0.05f;
}

/* the colour of the glow the definition attaches (or has as a widget), if
it attaches no light; FALSE if none */
static boolean emitter_color(const struct object_definition *definition, float *color)
{
	const struct object_attachment_definition *attachments =
		(const struct object_attachment_definition *)definition->object.attachments.address;
	const struct object_definition_widget *widgets =
		(const struct object_definition_widget *)definition->object.widgets.address;
	long index;
	boolean found = FALSE;

	for (index = 0; index < definition->object.widgets.count && !found; index++)
	{
		if (widgets[index].type.group_tag == GROUP_TAG_LIGHT_VOLUME && widgets[index].type.index != NONE)
			found = light_volume_color(widgets[index].type.index, color);
	}
	for (index = 0; index < definition->object.attachments.count; index++)
	{
		const struct tag_reference *type = &attachments[index].type;

		if (type->group_tag == GROUP_TAG_LIGHT && type->index != NONE)
			return FALSE;
		if (!found && type->group_tag == GROUP_TAG_LIGHT_VOLUME && type->index != NONE)
			found = light_volume_color(type->index, color);
	}
	return found;
}

/* this frame's emitters near the camera, 8 floats each (the position, the
radius, the colour, the intensity); returns how many, at most maximum */
long halo_ray_tracing_emitters(float *emitters, long maximum, const float *camera)
{
	struct object_iterator iterator;
	struct object_datum *object;
	long count = 0;

	if (global_structure_bsp_index == NONE || !object_header_data)
		return 0;
	object_iterator_new(&iterator, RAY_TRACED_OBJECT_TYPES, 0);
	while ((object = (struct object_datum *)object_iterator_next(&iterator)) != NULL && count < maximum)
	{
		/* (a projectile at its point; anything larger at its middle) */
		const real_point3d *at = object->object.type == _object_type_projectile ? &object->object.position :
			&object->object.bounding_sphere_center;
		float dx = at->x - camera[0], dy = at->y - camera[1], dz = at->z - camera[2], color[3];
		float *out = emitters + count * 8;

		if (dx * dx + dy * dy + dz * dz > RAY_TRACED_EMITTER_DISTANCE * RAY_TRACED_EMITTER_DISTANCE ||
			!emitter_color(object_definition_get(object->definition_index), color))
		{
			continue;
		}
		out[0] = at->x;
		out[1] = at->y;
		out[2] = at->z;
		/* (a large object's glow reaches a little farther, not across the level) */
		out[3] = PIN(object->object.bounding_sphere_radius * 2.0f, RAY_TRACED_EMITTER_RADIUS, 12.0f);
		out[4] = color[0];
		out[5] = color[1];
		out[6] = color[2];
		out[7] = RAY_TRACED_EMITTER_INTENSITY;
		count++;
	}
	return count;
}
