/*
CREWMAN_HANDS.C

The crewmen's hands drawn on what they work, not in it. Their poses were
made against nothing: in a10's cryo bay the assistant rests his right hand
4 cm (0.013 units) into the console beside the technician, and both type
with their fingers 1 cm into the keys in the close shot of the keyboard;
on the bridge the two pilots hold their hands 5 to 6 cm into their chairs'
consoles. The crewman has no finger nodes: each hand is one node, its
fingers rigid with it.

When a crewman's node matrices are made each tick (objects.c,
object_postprocess_node_matrices, after the unit's own hand placing), one
playing a scripted animation or seated has each arm measured: a line from
the shoulder (its upper arm's node) to each vertex the forearm or hand
holds, as the model has them, tested against the level and the objects
before him (not other bipeds). A vertex past a surface the line crosses is
in it by how far it is behind that surface's plane. The deepest one is
lifted out by bending the elbow: the forearm and hand are turned about the
forearm's node, across the line from the elbow to that vertex and the
surface's facing, by as much as moves the vertex
CREWMAN_HANDS_CLEARANCE past the surface. That is measured and done again
up to CREWMAN_HANDS_PASSES times, and no arm is bent more than
CREWMAN_HANDS_MAXIMUM_ANGLE. A vertex more than CREWMAN_HANDS_MAXIMUM_DEPTH
behind the surface its line crosses is past something thin, not in it,
and is left alone.

A crewman walking, running or standing idle is not measured, and one
with nothing in his hands' way is not changed. The animations and the
model are not changed; only the arm's nodes are, for every machine alike,
as they come from the same pose and the same level.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

#include <xtl.h>

#include "crewman_hands.h"

/* ---------- constants */

#define CREWMAN_HANDS_MODEL_NAME "characters\\crewman\\crewman"

/* the most vertices an arm's forearm and hand hold (the crewman's: 125 at
most, before those in the same place are taken once) */
#define CREWMAN_HANDS_MAXIMUM_VERTICES 512
/* how far past the surface a lifted vertex is left (1.5 mm) */
#define CREWMAN_HANDS_CLEARANCE 0.0005f
/* deeper than this (9 cm) a vertex is past something thin, not in it */
#define CREWMAN_HANDS_MAXIMUM_DEPTH 0.03f
/* the most an elbow is bent (30 degrees) */
#define CREWMAN_HANDS_MAXIMUM_ANGLE 0.5235988f
/* how many times an arm is measured and bent */
#define CREWMAN_HANDS_PASSES 3
/* nearer than this (3 mm) across the elbow's turn, a vertex is not moved by it */
#define CREWMAN_HANDS_MINIMUM_REACH 0.001f

#define CREWMAN_HANDS_COLLISION_FLAGS ( \
	FLAG(_collision_test_front_facing_surfaces_bit) | \
	FLAG(_collision_test_ignore_invisible_surfaces_bit) | \
	FLAG(_collision_test_structure_bit) | \
	FLAG(_collision_test_objects_bit) | \
	FLAG(_collision_test_objects_vehicles_bit) | \
	FLAG(_collision_test_objects_scenery_bit) | \
	FLAG(_collision_test_objects_machines_bit) | \
	FLAG(_collision_test_objects_controls_bit))

/* a compressed vertex's first node, as rasterizer_geometry.c packs it */
#define CREWMAN_HANDS_VERTEX_NODE_SCALE 3

enum
{
	_crewman_hands_right_arm = 0,
	_crewman_hands_left_arm,
	NUMBER_OF_CREWMAN_HANDS_ARMS
};

enum
{
	_model_geometry_part_stripped_bit = 0,
	_model_geometry_part_local_nodes_bit,
};

/* ---------- structures */

/* this build's model geometry and part, as models.c and rasterizer.c each
define them for themselves (no header declares them) */
struct model_geometry
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct model_geometry_part
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
	struct triangle_buffer triangle_buffer;
	struct vertex_buffer vertex_buffer;
};

typedef char verify_model_geometry_part_size[sizeof(struct model_geometry_part) == 0x68 ? 1 : -1];

/* an arm's nodes */
struct crewman_hands_arm
{
	short upper_arm_node_index;
	short forearm_node_index;
	short hand_node_index;
};

/* a vertex the forearm or hand holds, in that node's frame */
struct crewman_hands_vertex
{
	short node_index;
	real_point3d position;
};

/* ---------- globals */

static char const *const crewman_hands_node_names[NUMBER_OF_CREWMAN_HANDS_ARMS][3] =
{
	{"bip01 r upperarm", "bip01 r forearm", "bip01 r hand"},
	{"bip01 l upperarm", "bip01 l forearm", "bip01 l hand"},
};

/* the crewman's model in the map loaded, or NONE; its arms and their
vertices */
static long crewman_hands_model_index = NONE;
static struct crewman_hands_arm crewman_hands_arms[NUMBER_OF_CREWMAN_HANDS_ARMS];
static struct crewman_hands_vertex crewman_hands_vertices[NUMBER_OF_CREWMAN_HANDS_ARMS][CREWMAN_HANDS_MAXIMUM_VERTICES];
static short crewman_hands_vertex_counts[NUMBER_OF_CREWMAN_HANDS_ARMS];

/* ---------- private code */

/* the model's node of that name, or NONE */
static short crewman_hands_node_find(
	struct model const *model,
	char const *name)
{
	short node_index;

	for (node_index = 0; node_index < model->nodes.count && node_index < MAXIMUM_NODES_PER_MODEL; node_index++)
	{
		if (!csstrcmp(TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node)->name, name))
		{
			return node_index;
		}
	}

	return NONE;
}

/* the vertex, in its node's frame, added to the arm's unless it has it
already or is full; FALSE when full */
static boolean crewman_hands_vertex_add(
	short arm_index,
	short node_index,
	real_point3d const *position)
{
	struct crewman_hands_vertex *vertices = crewman_hands_vertices[arm_index];
	short count = crewman_hands_vertex_counts[arm_index];
	short vertex_index;

	for (vertex_index = 0; vertex_index < count; vertex_index++)
	{
		if (vertices[vertex_index].node_index == node_index &&
			vertices[vertex_index].position.x == position->x &&
			vertices[vertex_index].position.y == position->y &&
			vertices[vertex_index].position.z == position->z)
		{
			return TRUE;
		}
	}
	if (count >= CREWMAN_HANDS_MAXIMUM_VERTICES)
	{
		return FALSE;
	}

	vertices[count].node_index = node_index;
	vertices[count].position = *position;
	crewman_hands_vertex_counts[arm_index] = (short)(count + 1);

	return TRUE;
}

/* the part's vertices whose first node is an arm's forearm or hand, added to
that arm's in that node's frame; FALSE when an arm is full */
static boolean crewman_hands_part_add(
	struct model const *model,
	struct model_geometry_part const *part)
{
	struct vertex_buffer const *buffer = &part->vertex_buffer;
	byte *vertices = NULL;
	boolean fits = TRUE;
	long stride;
	long vertex_index;

	if (TEST_FLAG(part->flags, _model_geometry_part_stripped_bit) ||
		TEST_FLAG(part->flags, _model_geometry_part_local_nodes_bit) ||
		buffer->type != _rasterizer_vertex_type_model_compressed || !buffer->hardware_format || buffer->offset ||
		buffer->count <= 0 || buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART)
	{
		return TRUE;
	}

	stride = rasterizer_geometry_get_vertex_size(buffer->type);
	IDirect3DVertexBuffer8_Lock(buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);
	if (!vertices)
	{
		return TRUE;
	}
	for (vertex_index = 0; fits && vertex_index < buffer->count; vertex_index++)
	{
		struct model_vertex_compressed const *vertex =
			(struct model_vertex_compressed const *)(vertices + vertex_index * stride);
		short node_index = (short)(vertex->nodes[0] / CREWMAN_HANDS_VERTEX_NODE_SCALE);
		short arm_index;

		for (arm_index = 0; arm_index < NUMBER_OF_CREWMAN_HANDS_ARMS; arm_index++)
		{
			struct crewman_hands_arm const *arm = &crewman_hands_arms[arm_index];

			if (node_index == arm->forearm_node_index || node_index == arm->hand_node_index)
			{
				real_point3d position;

				matrix4x3_transform_point(
					&TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node)->runtime_default_inverse_matrix,
					&vertex->position,
					&position);
				fits = crewman_hands_vertex_add(arm_index, node_index, &position);
			}
		}
	}
	IDirect3DVertexBuffer8_Unlock(buffer->hardware_format);

	return fits;
}

/* the vector turned about the unit axis */
static void crewman_hands_turn(
	real_vector3d const *axis,
	real sine,
	real cosine,
	real_vector3d *vector)
{
	real along = axis->i * vector->i + axis->j * vector->j + axis->k * vector->k;
	real_vector3d across;
	real_vector3d turned;

	across.i = axis->j * vector->k - axis->k * vector->j;
	across.j = axis->k * vector->i - axis->i * vector->k;
	across.k = axis->i * vector->j - axis->j * vector->i;
	turned.i = vector->i * cosine + across.i * sine + axis->i * along * (1.f - cosine);
	turned.j = vector->j * cosine + across.j * sine + axis->j * along * (1.f - cosine);
	turned.k = vector->k * cosine + across.k * sine + axis->k * along * (1.f - cosine);
	*vector = turned;

	return;
}

/* the node's matrix turned about the unit axis through the pivot */
static void crewman_hands_node_turn(
	real_matrix4x3 *matrix,
	real_point3d const *pivot,
	real_vector3d const *axis,
	real sine,
	real cosine)
{
	real_vector3d offset;

	crewman_hands_turn(axis, sine, cosine, &matrix->forward);
	crewman_hands_turn(axis, sine, cosine, &matrix->left);
	crewman_hands_turn(axis, sine, cosine, &matrix->up);
	offset.i = matrix->position.x - pivot->x;
	offset.j = matrix->position.y - pivot->y;
	offset.k = matrix->position.z - pivot->z;
	crewman_hands_turn(axis, sine, cosine, &offset);
	matrix->position.x = pivot->x + offset.i;
	matrix->position.y = pivot->y + offset.j;
	matrix->position.z = pivot->z + offset.k;

	return;
}

/* how deep the arm's deepest vertex is in what is before it, and where it
is and that surface's facing; 0 when none is in anything */
static real crewman_hands_deepest(
	long object_index,
	real_matrix4x3 const *node_matrices,
	short arm_index,
	real_point3d *deepest_point,
	real_vector3d *deepest_normal)
{
	struct crewman_hands_arm const *arm = &crewman_hands_arms[arm_index];
	real_point3d const *shoulder = &node_matrices[arm->upper_arm_node_index].position;
	real deepest = 0.f;
	short vertex_index;

	for (vertex_index = 0; vertex_index < crewman_hands_vertex_counts[arm_index]; vertex_index++)
	{
		struct crewman_hands_vertex const *vertex = &crewman_hands_vertices[arm_index][vertex_index];
		struct collision_result collision;
		real_point3d point;
		real_vector3d line;

		matrix4x3_transform_point(&node_matrices[vertex->node_index], &vertex->position, &point);
		line.i = point.x - shoulder->x;
		line.j = point.y - shoulder->y;
		line.k = point.z - shoulder->z;
		if (collision_test_vector(CREWMAN_HANDS_COLLISION_FLAGS, shoulder, &line, object_index, &collision))
		{
			real depth = collision.plane.d -
				(collision.plane.n.i * point.x + collision.plane.n.j * point.y + collision.plane.n.k * point.z);

			if (depth > deepest && depth < CREWMAN_HANDS_MAXIMUM_DEPTH)
			{
				deepest = depth;
				*deepest_point = point;
				*deepest_normal = collision.plane.n;
			}
		}
	}

	return deepest;
}

/* the arm's elbow bent until its forearm and hand are out of what is
before them, or as far as it bends; how far it was bent */
static real crewman_hands_arm_lift(
	long object_index,
	real_matrix4x3 *node_matrices,
	short arm_index)
{
	struct crewman_hands_arm const *arm = &crewman_hands_arms[arm_index];
	real bent = 0.f;
	short pass;

	for (pass = 0; pass < CREWMAN_HANDS_PASSES && bent < CREWMAN_HANDS_MAXIMUM_ANGLE; pass++)
	{
		real_point3d point;
		real_point3d elbow;
		real_vector3d normal;
		real_vector3d from_elbow;
		real_vector3d axis;
		real depth = crewman_hands_deepest(object_index, node_matrices, arm_index, &point, &normal);
		real reach;
		real angle;

		if (depth <= 0.f)
		{
			break;
		}

		/* across the line from the elbow and the surface's facing, the
		vertex moves out along the facing by the angle times its reach */
		elbow = node_matrices[arm->forearm_node_index].position;
		from_elbow.i = point.x - elbow.x;
		from_elbow.j = point.y - elbow.y;
		from_elbow.k = point.z - elbow.z;
		axis.i = from_elbow.j * normal.k - from_elbow.k * normal.j;
		axis.j = from_elbow.k * normal.i - from_elbow.i * normal.k;
		axis.k = from_elbow.i * normal.j - from_elbow.j * normal.i;
		reach = square_root(axis.i * axis.i + axis.j * axis.j + axis.k * axis.k);
		if (reach < CREWMAN_HANDS_MINIMUM_REACH)
		{
			break;
		}
		axis.i /= reach;
		axis.j /= reach;
		axis.k /= reach;
		angle = MIN((depth + CREWMAN_HANDS_CLEARANCE) / reach, CREWMAN_HANDS_MAXIMUM_ANGLE - bent);

		crewman_hands_node_turn(&node_matrices[arm->forearm_node_index], &elbow, &axis, (real)sin(angle), (real)cos(angle));
		crewman_hands_node_turn(&node_matrices[arm->hand_node_index], &elbow, &axis, (real)sin(angle), (real)cos(angle));
		bent += angle;
	}

	return bent;
}

/* ---------- public code */

void crewman_hands_tags_loaded(
	void)
{
	long model_index = tag_loaded(MODELS_GROUP_TAG, CREWMAN_HANDS_MODEL_NAME);
	struct model const *model;
	long geometry_index;
	short arm_index;
	short name_index;

	crewman_hands_model_index = NONE;
	crewman_hands_vertex_counts[_crewman_hands_right_arm] = 0;
	crewman_hands_vertex_counts[_crewman_hands_left_arm] = 0;
	if (model_index == NONE)
	{
		return;
	}
	model = model_definition_get(model_index);
	if (!model->nodes.address || !model->geometries.address ||
		model->geometries.count <= 0 || model->geometries.count > MAXIMUM_GEOMETRIES_PER_MODEL)
	{
		return;
	}

	for (arm_index = 0; arm_index < NUMBER_OF_CREWMAN_HANDS_ARMS; arm_index++)
	{
		short nodes[3];

		for (name_index = 0; name_index < 3; name_index++)
		{
			nodes[name_index] = crewman_hands_node_find(model, crewman_hands_node_names[arm_index][name_index]);
			if (nodes[name_index] == NONE)
			{
				return;
			}
		}
		crewman_hands_arms[arm_index].upper_arm_node_index = nodes[0];
		crewman_hands_arms[arm_index].forearm_node_index = nodes[1];
		crewman_hands_arms[arm_index].hand_node_index = nodes[2];
	}

	for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
	{
		struct model_geometry const *geometry = TAG_BLOCK_GET_ELEMENT(&model->geometries, geometry_index, struct model_geometry);
		long part_index;

		if (geometry->parts.count < 0 || geometry->parts.count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY ||
			(geometry->parts.count && !geometry->parts.address))
		{
			continue;
		}
		for (part_index = 0; part_index < geometry->parts.count; part_index++)
		{
			if (!crewman_hands_part_add(model, TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part)))
			{
				return;
			}
		}
	}

	if (crewman_hands_vertex_counts[_crewman_hands_right_arm] > 0 &&
		crewman_hands_vertex_counts[_crewman_hands_left_arm] > 0)
	{
		crewman_hands_model_index = model_index;
	}

	return;
}

void crewman_hands_tags_unloaded(
	void)
{
	crewman_hands_model_index = NONE;

	return;
}

short crewman_hands_postprocess_node_matrices(
	long object_index,
	real_matrix4x3 *node_matrices)
{
	struct object_datum *object = object_get(object_index);
	short arms_bent = 0;
	short arm_index;

	if (crewman_hands_model_index == NONE ||
		object_definition_get(object->definition_index)->object.model.index != crewman_hands_model_index ||
		!object_try_and_get_and_verify_type(object_index, _object_mask_unit) ||
		(object->object.parent_object_index == NONE && !unit_is_playing_custom_animation(object_index)))
	{
		return 0;
	}

	for (arm_index = 0; arm_index < NUMBER_OF_CREWMAN_HANDS_ARMS; arm_index++)
	{
		if (crewman_hands_arm_lift(object_index, node_matrices, arm_index) > 0.f)
		{
			arms_bent++;
		}
	}

	return arms_bent;
}
