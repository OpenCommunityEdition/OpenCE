/*
PHYSICS_JOLT.C

The game's side of the physics bridge (port/macos/include/host_physics.h,
docs/macos_host.md "Physics"). The game writes a batch of commands in its
own memory and gives it to the host with host_physics_step, which checks
it, carries it out on its Jolt world, steps the world and writes every
ragdoll's and body's pose into the results buffer before it returns: one
VM exit a batch.

Here: the batch writer, and the level. When a structure bsp loads or
switches, its collision surfaces are cut into triangles (fans of their
rings of edges, turned to face as their planes do) and sent once; the host
builds Jolt's static mesh from them.

Only the macOS app's guest has the import; elsewhere the bridge is never
enabled.
*/

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "physics/bsp3d.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"
#include "physics_jolt.h"

#include <stdlib.h>
#include <string.h>

#if defined(HALO_GUEST_VM) && defined(HALO_METAL)
#include "host_physics.h"
/* the host's import (port/android/guest/runtime/guest_host.h) */
int host_physics_step(unsigned int batch, unsigned int size, unsigned int results, unsigned int results_size);
#define PHYSICS_JOLT_HOST TRUE
#else
#define PHYSICS_JOLT_HOST FALSE
#endif

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

/* the surfaces' flags (structures.c's) */
enum
{
	_physics_jolt_surface_two_sided_bit = 0,
};

/* the most points of a surface's ring (collision_bsp_definitions.h) */
#define PHYSICS_JOLT_RING_MAX 8

static unsigned long physics_jolt_changes = (unsigned long)-1;
static boolean physics_jolt_on = FALSE;
static short physics_jolt_level_bsp_index = NONE;

boolean physics_jolt_enabled(void)
{
	if (config_changes() != physics_jolt_changes)
	{
		physics_jolt_changes = config_changes();
		physics_jolt_on = PHYSICS_JOLT_HOST &&
			(config_boolean("game.ragdolls") || config_boolean("game.physics_debris"));
	}
	return physics_jolt_on;
}

/* ---------- the level */

/* the vertex indices of a surface's ring, in its order; 0 if the ring is
broken or names a vertex the bsp does not have */
static short physics_jolt_surface_ring(
	struct collision_bsp const *bsp,
	long surface_index,
	long *ring)
{
	struct collision_surface const *surface;
	long first_edge_index;
	long edge_index;
	short count = 0;

	if (surface_index < 0 || surface_index >= bsp->surfaces.count)
		return 0;
	surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
	first_edge_index = surface->first_edge_index;
	edge_index = first_edge_index;
	do
	{
		struct collision_edge const *edge;
		boolean reverse;
		long vertex_index;

		if (edge_index < 0 || edge_index >= bsp->edges.count || count >= PHYSICS_JOLT_RING_MAX)
			return 0;
		edge = TAG_BLOCK_GET_ELEMENT(&bsp->edges, edge_index, struct collision_edge);
		reverse = edge->surface_indices[1] == surface_index;
		vertex_index = edge->vertex_indices[reverse];
		if (vertex_index < 0 || vertex_index >= bsp->vertices.count)
			return 0;
		ring[count++] = vertex_index;
		edge_index = edge->edge_indices[reverse];
	}
	while (edge_index != first_edge_index);

	return count;
}

long physics_jolt_level_triangles(
	struct collision_bsp const *bsp,
	real *vertices,
	unsigned long *indices,
	long triangle_capacity)
{
	long triangle_count = 0;
	long surface_index;
	long vertex_index;

	if (vertices)
	{
		for (vertex_index = 0; vertex_index < bsp->vertices.count; vertex_index++)
		{
			struct collision_vertex const *vertex =
				TAG_BLOCK_GET_ELEMENT(&bsp->vertices, vertex_index, struct collision_vertex);

			vertices[3 * vertex_index + 0] = vertex->point.x;
			vertices[3 * vertex_index + 1] = vertex->point.y;
			vertices[3 * vertex_index + 2] = vertex->point.z;
		}
	}
	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		struct collision_surface const *surface =
			TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
		long ring[PHYSICS_JOLT_RING_MAX];
		short count = physics_jolt_surface_ring(bsp, surface_index, ring);
		long plane_index = surface->plane_designator & LONG_MAX;
		boolean two_sided = TEST_FLAG(surface->flags, _physics_jolt_surface_two_sided_bit);
		short sides = two_sided ? 2 : 1;
		short side;
		short point;

		if (count < 3 || plane_index >= bsp->bsp3d.planes.count)
			continue;
		if (indices)
		{
			real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(&bsp->bsp3d.planes, plane_index, real_plane3d);
			real_vector3d normal;
			real facing;
			boolean flipped;

			/* the ring's normal (Newell's), against its plane's */
			normal.i = normal.j = normal.k = 0.0f;
			for (point = 0; point < count; point++)
			{
				struct collision_vertex const *a =
					TAG_BLOCK_GET_ELEMENT(&bsp->vertices, ring[point], struct collision_vertex);
				struct collision_vertex const *b =
					TAG_BLOCK_GET_ELEMENT(&bsp->vertices, ring[(point + 1) % count], struct collision_vertex);

				normal.i += (a->point.y - b->point.y) * (a->point.z + b->point.z);
				normal.j += (a->point.z - b->point.z) * (a->point.x + b->point.x);
				normal.k += (a->point.x - b->point.x) * (a->point.y + b->point.y);
			}
			facing = normal.i * plane->n.i + normal.j * plane->n.j + normal.k * plane->n.k;
			flipped = (facing < 0.0f) != ((surface->plane_designator & LONG_MIN) != 0);
			for (side = 0; side < sides; side++)
			{
				boolean reverse = flipped != (side == 1);

				for (point = 1; point + 1 < count; point++)
				{
					unsigned long *triangle;

					if (triangle_count >= triangle_capacity)
						return triangle_count;
					triangle = indices + 3 * triangle_count++;
					triangle[0] = ring[0];
					triangle[1] = ring[reverse ? point + 1 : point];
					triangle[2] = ring[reverse ? point : point + 1];
				}
			}
		}
		else
		{
			triangle_count += sides * (count - 2);
		}
	}

	return triangle_count;
}

#if defined(HALO_GUEST_VM) && defined(HALO_METAL)

/* ---------- the batch */

#define PHYSICS_JOLT_BATCH_WORDS 65536
#define PHYSICS_JOLT_RESULTS_WORDS (HOST_PHYSICS_RESULTS_HEADER_WORDS + \
	HOST_PHYSICS_RAGDOLLS_MAX * (HOST_PHYSICS_RESULT_RAGDOLL_WORDS + HOST_PHYSICS_PARTS_MAX * HOST_PHYSICS_POSE_WORDS) + \
	HOST_PHYSICS_BODIES_MAX * HOST_PHYSICS_RESULT_BODY_WORDS)

static unsigned long physics_jolt_batch[PHYSICS_JOLT_BATCH_WORDS];
static unsigned long physics_jolt_results[PHYSICS_JOLT_RESULTS_WORDS];
static long physics_jolt_batch_length;
static long physics_jolt_batch_commands;

static void physics_jolt_batch_begin(void)
{
	physics_jolt_batch[0] = HOST_PHYSICS_BATCH_MAGIC;
	physics_jolt_batch[1] = HOST_PHYSICS_VERSION;
	physics_jolt_batch_length = HOST_PHYSICS_BATCH_HEADER_WORDS;
	physics_jolt_batch_commands = 0;
}

/* room for a command of this many words, its first written; NULL if the
batch is full */
static unsigned long *physics_jolt_command(
	unsigned short op,
	unsigned short words)
{
	unsigned long *command;

	if (physics_jolt_batch_length + words > PHYSICS_JOLT_BATCH_WORDS)
		return NULL;
	command = physics_jolt_batch + physics_jolt_batch_length;
	command[0] = HOST_PHYSICS_COMMAND(op, words);
	physics_jolt_batch_length += words;
	physics_jolt_batch_commands++;
	return command;
}

static unsigned long physics_jolt_word(
	real value)
{
	unsigned long word;

	memcpy(&word, &value, sizeof(word));
	return word;
}

/* gives the batch to the host, which steps the world `seconds` after its
commands; the commands refused, or -1 */
static int physics_jolt_batch_send(
	real seconds)
{
	physics_jolt_batch[2] = physics_jolt_batch_length * 4;
	physics_jolt_batch[3] = physics_jolt_batch_commands;
	physics_jolt_batch[4] = physics_jolt_word(seconds);
	return host_physics_step((unsigned int)(unsigned long)physics_jolt_batch, physics_jolt_batch_length * 4,
		(unsigned int)(unsigned long)physics_jolt_results, sizeof(physics_jolt_results));
}

/* ---------- the level */

static void physics_jolt_level_send(void)
{
	struct collision_bsp const *bsp = global_collision_bsp_get();
	unsigned long *command;
	real *vertices = NULL;
	unsigned long *indices = NULL;
	long triangle_count = 0;

	physics_jolt_level_bsp_index = global_structure_bsp_index_get();
	if (bsp && bsp->vertices.count > 0)
	{
		triangle_count = physics_jolt_level_triangles(bsp, NULL, NULL, 0);
		if (triangle_count > (long)HOST_PHYSICS_LEVEL_TRIANGLES_MAX)
			triangle_count = HOST_PHYSICS_LEVEL_TRIANGLES_MAX;
		vertices = malloc(sizeof(real) * 3 * bsp->vertices.count);
		indices = malloc(sizeof(unsigned long) * 3 * (triangle_count ? triangle_count : 1));
		if (vertices && indices)
			triangle_count = physics_jolt_level_triangles(bsp, vertices, indices, triangle_count);
		else
			triangle_count = 0;
	}
	physics_jolt_batch_begin();
	command = physics_jolt_command(HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);
	command[1] = (unsigned long)vertices;
	command[2] = triangle_count ? bsp->vertices.count : 0;
	command[3] = (unsigned long)indices;
	command[4] = triangle_count;
	if (physics_jolt_batch_send(0.0f) != 0)
		error(_error_silent, "physics: the host refused the level's %ld triangles", triangle_count);
	/* (the host copies them as it reads the command) */
	free(vertices);
	free(indices);
}

void physics_jolt_structure_bsp_reconnect(void)
{
	if (physics_jolt_enabled())
		physics_jolt_level_send();
	else
		physics_jolt_level_bsp_index = NONE;
}

void physics_jolt_dispose_from_old_map(void)
{
	if (physics_jolt_level_bsp_index == NONE)
		return;
	physics_jolt_batch_begin();
	physics_jolt_command(HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
	physics_jolt_batch_send(0.0f);
	physics_jolt_level_bsp_index = NONE;
}

#else

void physics_jolt_structure_bsp_reconnect(void)
{
}

void physics_jolt_dispose_from_old_map(void)
{
}

#endif
