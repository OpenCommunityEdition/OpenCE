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
#include "objects/object_types.h"
#include "objects/objects.h"
#include "physics/bsp3d.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "physics_jolt.h"
#include "ragdoll_bodies.h"
#include "ragdolls.h"

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
/* a dead body's on the level */
#define PHYSICS_JOLT_RAGDOLL_FRICTION 0.8f
#define PHYSICS_JOLT_RAGDOLL_RESTITUTION 0.05f
/* a stand-in's, where the ragdolls meet it */
#define PHYSICS_JOLT_STAND_IN_FRICTION 0.6f
#define PHYSICS_JOLT_STAND_IN_RESTITUTION 0.1f

static unsigned long physics_jolt_changes = (unsigned long)-1;
static boolean physics_jolt_on = FALSE;
static boolean physics_jolt_ragdolls_on = FALSE;
static short physics_jolt_level_bsp_index = NONE;

boolean physics_jolt_enabled(void)
{
	if (config_changes() != physics_jolt_changes)
	{
		physics_jolt_changes = config_changes();
		physics_jolt_ragdolls_on = PHYSICS_JOLT_HOST && config_boolean("game.ragdolls");
		physics_jolt_on = physics_jolt_ragdolls_on || (PHYSICS_JOLT_HOST && config_boolean("game.physics_debris"));
	}
	return physics_jolt_on;
}

boolean physics_jolt_ragdolls_enabled(void)
{
	physics_jolt_enabled();
	return physics_jolt_ragdolls_on;
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

/* ---------- ragdolls */

/* the arrays a tick's commands name, which the host copies as it reads them:
each new ragdoll's parts and poses, each driven ragdoll's targets */
static unsigned long physics_jolt_parts[RAGDOLL_DEATHS_MAX][RAGDOLL_BODY_PARTS_MAX * HOST_PHYSICS_PART_WORDS];
static unsigned long physics_jolt_poses[RAGDOLL_DEATHS_MAX][RAGDOLL_BODY_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];
static unsigned long physics_jolt_targets[RAGDOLLS_MAX][RAGDOLL_BODY_PARTS_MAX * HOST_PHYSICS_POSE_WORDS];

static void physics_jolt_pose_words(
	real_matrix4x3 const *matrix,
	unsigned long *words)
{
	struct ragdoll_pose pose;

	ragdoll_pose_from_matrix(matrix, &pose);
	words[0] = physics_jolt_word(pose.position.x);
	words[1] = physics_jolt_word(pose.position.y);
	words[2] = physics_jolt_word(pose.position.z);
	words[3] = physics_jolt_word(pose.rotation.v.i);
	words[4] = physics_jolt_word(pose.rotation.v.j);
	words[5] = physics_jolt_word(pose.rotation.v.k);
	words[6] = physics_jolt_word(pose.rotation.w);
}

/* a body's parts as the host reads them: the twist about the node's x, the
plane axis its z (the normal its y) */
static void physics_jolt_part_words(
	struct ragdoll_part const *part,
	unsigned long *words)
{
	long index;

	memset(words, 0, HOST_PHYSICS_PART_WORDS * sizeof(*words));
	words[0] = (unsigned long)(long)part->parent_part_index;
	words[1] = part->shape;
	for (index = 0; index < 3; index++)
		words[2 + index] = physics_jolt_word(part->size[index]);
	words[5] = physics_jolt_word(part->center.x);
	words[6] = physics_jolt_word(part->center.y);
	words[7] = physics_jolt_word(part->center.z);
	words[8] = words[9] = words[10] = physics_jolt_word(0.0f);
	words[11] = physics_jolt_word(1.0f);
	words[12] = physics_jolt_word(part->mass);
	words[13] = physics_jolt_word(1.0f);
	words[14] = words[15] = words[16] = words[17] = physics_jolt_word(0.0f);
	words[18] = physics_jolt_word(1.0f);
	words[19] = physics_jolt_word(part->twist_min);
	words[20] = physics_jolt_word(part->twist_max);
	words[21] = physics_jolt_word(part->swing_normal);
	words[22] = physics_jolt_word(part->swing_plane);
}

static void physics_jolt_ragdoll_remove(
	long id)
{
	unsigned long *command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_REMOVE, HOST_PHYSICS_RAGDOLL_REMOVE_WORDS);

	if (command)
		command[1] = id;
	ragdolls_stop(id);
}

/* A death's ragdoll, built with its joints at rest in the pose the biped
died in (the limits centre on it) and placed there, moving as the biped
was. */
static void physics_jolt_ragdoll_add(
	struct ragdoll_death const *death,
	long death_index)
{
	struct ragdoll_body const *body = death->body;
	unsigned long *parts = physics_jolt_parts[death_index];
	unsigned long *poses = physics_jolt_poses[death_index];
	unsigned long *command;
	long replaced;
	long id = ragdolls_start(death, &replaced);
	long index;

	if (id == NONE)
		return;
	if (replaced != NONE)
	{
		command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_REMOVE, HOST_PHYSICS_RAGDOLL_REMOVE_WORDS);
		if (command)
			command[1] = replaced;
	}
	for (index = 0; index < body->part_count; index++)
	{
		physics_jolt_part_words(&body->parts[index], parts + index * HOST_PHYSICS_PART_WORDS);
		physics_jolt_pose_words(&death->nodes[body->parts[index].node_index], poses + index * HOST_PHYSICS_POSE_WORDS);
	}
	command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_ADD, HOST_PHYSICS_RAGDOLL_ADD_WORDS);
	if (!command)
	{
		ragdolls_stop(id);
		return;
	}
	command[1] = id;
	command[2] = body->part_count;
	command[3] = (unsigned long)parts;
	command[4] = (unsigned long)poses;
	command[5] = (unsigned long)poses;
	command[6] = physics_jolt_word(PHYSICS_JOLT_RAGDOLL_FRICTION);
	command[7] = physics_jolt_word(PHYSICS_JOLT_RAGDOLL_RESTITUTION);
	/* (the game's velocities are a tick's) */
	command[8] = physics_jolt_word(death->velocity.i * TICKS_PER_SECOND);
	command[9] = physics_jolt_word(death->velocity.j * TICKS_PER_SECOND);
	command[10] = physics_jolt_word(death->velocity.k * TICKS_PER_SECOND);
}

/* the motors toward the biped's node matrices now: its death animation */
static void physics_jolt_ragdoll_drive(
	long id)
{
	struct ragdoll const *ragdoll = ragdolls_get(id);
	unsigned long *targets = physics_jolt_targets[id - 1];
	real_matrix4x3 const *nodes = object_get_node_matrices(ragdoll->object_index);
	unsigned long *command;
	long index;

	for (index = 0; index < ragdoll->body->part_count; index++)
		physics_jolt_pose_words(&nodes[ragdoll->body->parts[index].node_index], targets + index * HOST_PHYSICS_POSE_WORDS);
	command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_DRIVE, HOST_PHYSICS_RAGDOLL_DRIVE_WORDS);
	if (!command)
		return;
	command[1] = id;
	command[2] = physics_jolt_word(RAGDOLL_DRIVE_FREQUENCY);
	command[3] = (unsigned long)targets;
	command[4] = ragdoll->body->part_count;
}

static void physics_jolt_ragdoll_impulse(
	struct ragdoll_impulse const *impulse)
{
	unsigned long *command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_IMPULSE, HOST_PHYSICS_RAGDOLL_IMPULSE_WORDS);

	if (!command)
		return;
	command[1] = impulse->id;
	command[2] = impulse->part;
	command[3] = physics_jolt_word(impulse->impulse.i);
	command[4] = physics_jolt_word(impulse->impulse.j);
	command[5] = physics_jolt_word(impulse->impulse.k);
	command[6] = physics_jolt_word(impulse->point.x);
	command[7] = physics_jolt_word(impulse->point.y);
	command[8] = physics_jolt_word(impulse->point.z);
}

/* ---------- stand-ins */

/* A stand-in's shape where its object is now: a vehicle's hull, the box
of its collision model's first node in that node's frame; a living biped's
pill (bipeds.c's), standing, a capsule up its frame's x, or a sphere. An AI's
pill has no height, so its standing height is taken. FALSE if it has none. */
static boolean physics_jolt_stand_in_shape(
	long object_index,
	struct object_datum const *object,
	unsigned long *shape,
	real *size,
	struct ragdoll_pose *pose)
{
	if (object->object.type == _object_type_vehicle)
	{
		real_matrix4x3 const *node = object_get_node_matrices(object_index);
		real_point3d center;
		real_vector3d half_extents;

		if (!ragdoll_hull_get(object->definition_index, &center, &half_extents))
			return FALSE;
		ragdoll_pose_from_matrix(node, pose);
		pose->position.x = node->position.x + node->scale *
			(node->forward.i * center.x + node->left.i * center.y + node->up.i * center.z);
		pose->position.y = node->position.y + node->scale *
			(node->forward.j * center.x + node->left.j * center.y + node->up.j * center.z);
		pose->position.z = node->position.z + node->scale *
			(node->forward.k * center.x + node->left.k * center.y + node->up.k * center.z);
		*shape = HOST_PHYSICS_SHAPE_BOX;
		size[0] = node->scale * half_extents.i;
		size[1] = node->scale * half_extents.j;
		size[2] = node->scale * half_extents.k;
	}
	else
	{
		struct biped_definition const *definition = biped_definition_get(object->definition_index);
		real_point3d base;
		real height;
		real width;

		biped_get_physics_pill(object_index, &base, &height, &width);
		if (height <= 0.0f && !TEST_FLAG(definition->biped.flags, _biped_spherical_bit))
			height = definition->biped.collision_height_standing - 2.0f * width;
		if (width <= 0.0f)
			return FALSE;
		pose->position = base;
		pose->rotation.v.i = pose->rotation.v.k = 0.0f;
		pose->rotation.w = 1.0f;
		pose->rotation.v.j = 0.0f;
		size[0] = width;
		size[1] = size[2] = 0.0f;
		*shape = HOST_PHYSICS_SHAPE_SPHERE;
		if (height > 0.0f)
		{
			/* (x turned up) */
			pose->position.z += 0.5f * height;
			pose->rotation.v.j = -0.70710678f;
			pose->rotation.w = 0.70710678f;
			size[1] = 0.5f * height;
			*shape = HOST_PHYSICS_SHAPE_CAPSULE;
		}
	}
	return TRUE;
}

static void physics_jolt_body_pose_words(
	struct ragdoll_pose const *pose,
	unsigned long *words)
{
	words[0] = physics_jolt_word(pose->position.x);
	words[1] = physics_jolt_word(pose->position.y);
	words[2] = physics_jolt_word(pose->position.z);
	words[3] = physics_jolt_word(pose->rotation.v.i);
	words[4] = physics_jolt_word(pose->rotation.v.j);
	words[5] = physics_jolt_word(pose->rotation.v.k);
	words[6] = physics_jolt_word(pose->rotation.w);
}

/* the vehicles and living bipeds near a ragdoll, as kinematic bodies that
the host moves to them each step: added as they come near, moved, and
removed as they go (or die, or get in a vehicle) */
static void physics_jolt_stand_ins_update(void)
{
	struct object_iterator iterator;
	struct object_datum *object;
	long id;

	ragdolls_stand_ins_begin();
	object_iterator_new(&iterator, _object_mask_biped | _object_mask_vehicle, 0);
	while ((object = (struct object_datum *)object_iterator_next(&iterator)) != NULL)
	{
		struct ragdoll_pose pose;
		unsigned long shape;
		real size[3];
		unsigned long *command;
		boolean added;

		if (object->object.parent_object_index != NONE ||
			(object->object.type == _object_type_biped && TEST_FLAG(object->object.damage_flags, _object_dead_bit)) ||
			ragdolls_of_object(iterator.index) ||
			!ragdolls_near(&object->object.bounding_sphere_center, object->object.bounding_sphere_radius) ||
			!physics_jolt_stand_in_shape(iterator.index, object, &shape, size, &pose))
		{
			continue;
		}
		id = ragdolls_stand_in(iterator.index, &added);
		if (id == NONE)
			continue;
		if (!added)
		{
			command = physics_jolt_command(HOST_PHYSICS_OP_BODY_MOVE, HOST_PHYSICS_BODY_MOVE_WORDS);
			if (command)
			{
				command[1] = id;
				physics_jolt_body_pose_words(&pose, command + 2);
			}
			continue;
		}
		command = physics_jolt_command(HOST_PHYSICS_OP_BODY_ADD, HOST_PHYSICS_BODY_ADD_WORDS);
		if (!command)
			continue;
		command[1] = id;
		command[2] = HOST_PHYSICS_MOTION_KINEMATIC;
		command[3] = shape;
		command[4] = physics_jolt_word(size[0]);
		command[5] = physics_jolt_word(size[1]);
		command[6] = physics_jolt_word(size[2]);
		command[7] = physics_jolt_word(1.0f);
		command[8] = physics_jolt_word(PHYSICS_JOLT_STAND_IN_FRICTION);
		command[9] = physics_jolt_word(PHYSICS_JOLT_STAND_IN_RESTITUTION);
		physics_jolt_body_pose_words(&pose, command + 10);
		/* (the game's velocities are a tick's) */
		command[17] = physics_jolt_word(object->object.translational_velocity.i * TICKS_PER_SECOND);
		command[18] = physics_jolt_word(object->object.translational_velocity.j * TICKS_PER_SECOND);
		command[19] = physics_jolt_word(object->object.translational_velocity.k * TICKS_PER_SECOND);
		command[20] = physics_jolt_word(object->object.angular_velocity.i * TICKS_PER_SECOND);
		command[21] = physics_jolt_word(object->object.angular_velocity.j * TICKS_PER_SECOND);
		command[22] = physics_jolt_word(object->object.angular_velocity.k * TICKS_PER_SECOND);
	}
	for (id = 1; id <= RAGDOLL_STAND_INS_MAX; id++)
	{
		if (ragdolls_stand_in_unseen(id))
		{
			unsigned long *command = physics_jolt_command(HOST_PHYSICS_OP_BODY_REMOVE, HOST_PHYSICS_BODY_REMOVE_WORDS);

			if (command)
				command[1] = id;
		}
	}
}

static real physics_jolt_result_real(
	unsigned long word)
{
	real value;

	memcpy(&value, &word, sizeof(value));
	return value;
}

/* each ragdoll's poses from the results, as far as they are whole */
static void physics_jolt_results_read(void)
{
	unsigned long const *words = physics_jolt_results;
	long length;
	long at = HOST_PHYSICS_RESULTS_HEADER_WORDS;
	unsigned long count;
	unsigned long index;

	if (words[0] != HOST_PHYSICS_RESULTS_MAGIC || words[1] != HOST_PHYSICS_VERSION)
		return;
	length = (long)(words[2] / 4);
	if (length < HOST_PHYSICS_RESULTS_HEADER_WORDS || length > PHYSICS_JOLT_RESULTS_WORDS)
		return;
	count = words[3];
	for (index = 0; index < count && at + HOST_PHYSICS_RESULT_RAGDOLL_WORDS <= length; index++)
	{
		unsigned long const *entry = words + at;
		struct ragdoll *ragdoll = ragdolls_get((long)entry[0]);
		unsigned long part_count = entry[1];
		unsigned long const *pose_words = entry + HOST_PHYSICS_RESULT_RAGDOLL_WORDS;
		unsigned long part;

		if (part_count > HOST_PHYSICS_PARTS_MAX ||
			at + HOST_PHYSICS_RESULT_RAGDOLL_WORDS + (long)part_count * HOST_PHYSICS_POSE_WORDS > length)
		{
			return;
		}
		at += HOST_PHYSICS_RESULT_RAGDOLL_WORDS + part_count * HOST_PHYSICS_POSE_WORDS;
		if (!ragdoll || (long)part_count != ragdoll->body->part_count)
			continue;
		ragdoll->awake = entry[2] != 0;
		ragdoll->broken_joints = entry[3];
		for (part = 0; part < part_count; part++)
		{
			struct ragdoll_pose *pose = &ragdoll->poses[part];
			unsigned long const *from = pose_words + part * HOST_PHYSICS_POSE_WORDS;

			pose->position.x = physics_jolt_result_real(from[0]);
			pose->position.y = physics_jolt_result_real(from[1]);
			pose->position.z = physics_jolt_result_real(from[2]);
			pose->rotation.v.i = physics_jolt_result_real(from[3]);
			pose->rotation.v.j = physics_jolt_result_real(from[4]);
			pose->rotation.v.k = physics_jolt_result_real(from[5]);
			pose->rotation.w = physics_jolt_result_real(from[6]);
		}
		ragdoll->posed = TRUE;
	}
}

void physics_jolt_update(
	real seconds)
{
	long index;
	long id;

	/* (the reconnect procs run only on a switch from another bsp: a map's
	first is sent here) */
	if (physics_jolt_enabled() && physics_jolt_level_bsp_index != global_structure_bsp_index_get() &&
		global_structure_bsp_index_get() != NONE)
	{
		physics_jolt_level_send();
	}
	if (!physics_jolt_enabled() || physics_jolt_level_bsp_index == NONE)
	{
		ragdolls_deaths_clear();
		ragdolls_impulses_clear();
		return;
	}
	physics_jolt_batch_begin();
	for (index = 0; index < ragdolls_death_count(); index++)
		physics_jolt_ragdoll_add(ragdolls_death_get(index), index);
	ragdolls_deaths_clear();
	for (index = 0; index < ragdolls_impulse_count(); index++)
		physics_jolt_ragdoll_impulse(ragdolls_impulse_get(index));
	ragdolls_impulses_clear();
	for (id = 1; id <= RAGDOLLS_MAX; id++)
	{
		switch (ragdolls_action(id))
		{
		case _ragdoll_action_drive:
			physics_jolt_ragdoll_drive(id);
			break;
		case _ragdoll_action_limp:
		{
			unsigned long *command = physics_jolt_command(HOST_PHYSICS_OP_RAGDOLL_LIMP, HOST_PHYSICS_RAGDOLL_LIMP_WORDS);

			if (command)
				command[1] = id;
			break;
		}
		case _ragdoll_action_remove:
			physics_jolt_ragdoll_remove(id);
			break;
		}
	}
	physics_jolt_stand_ins_update();
	if (physics_jolt_batch_send(seconds) < 0)
		return;
	physics_jolt_results_read();
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
	ragdoll_bodies_dispose_from_old_map();
	ragdolls_dispose_from_old_map();
	if (physics_jolt_level_bsp_index == NONE)
		return;
	physics_jolt_batch_begin();
	physics_jolt_command(HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
	physics_jolt_batch_send(0.0f);
	physics_jolt_level_bsp_index = NONE;
}

/* A saved game loaded (a revert, a checkpoint): its objects may take the
handles the ragdolls' bipeds had, so every ragdoll and stand-in goes, and the
host's world is made again with the level alone. */
void physics_jolt_game_state_loaded(void)
{
	ragdolls_dispose_from_old_map();
	if (physics_jolt_level_bsp_index != NONE)
	{
		physics_jolt_batch_begin();
		physics_jolt_command(HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
		physics_jolt_batch_send(0.0f);
		physics_jolt_level_bsp_index = NONE;
	}
	physics_jolt_structure_bsp_reconnect();
}

#else

void physics_jolt_structure_bsp_reconnect(void)
{
}

void physics_jolt_dispose_from_old_map(void)
{
	ragdoll_bodies_dispose_from_old_map();
	ragdolls_dispose_from_old_map();
}

void physics_jolt_game_state_loaded(void)
{
	ragdolls_dispose_from_old_map();
}

void physics_jolt_update(
	real seconds)
{
	(void)seconds;
	ragdolls_deaths_clear();
	ragdolls_impulses_clear();
}

#endif
