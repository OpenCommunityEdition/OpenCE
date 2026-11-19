/*
OBJECT_POSE.C

Poses: a mutator's [[pose]] (docs/mutators.md) scales a node of the objects
of a group (or of one tag of it) and moves it in its parent's frame, as the
objects' node orientations are made (object_type_preprocess_node_orientations),
after their animations' and before their node matrices are. Their models are
drawn with those matrices, and their collision models are tested with them
(collision_models.c takes each node's matrix and its scale), so a node is hit
where it is drawn, on every machine alike: the host sends its poses with its
tag edits (mutator_tags.c). What hangs from a node, its children and its
markers, moves and scales with it. A big head is a pose of every biped's head;
long arms are a pose of their upper arms, with their hands scaled back down.

The node is named as the object's model names it ("bip01 l upperarm"), or
"head": a biped's head node, whatever its model names it. A pose is resolved
to each object definition and node index as the game starts (object_pose_add),
so that making an object's nodes looks up nothing by name; an object of no
pose costs a test of whether there are any.

An object's bounding sphere is what the game looks for an object in before it
tests its collision model (a shot, a melee, an explosion, the host's check of
a client's hit), so it is made large enough to hold each node a pose moves, and
the nodes under it: past the node's position by its collision geometry's reach
from the node, at the node's scale. The sphere it is drawn by (its render
bounding radius, about the same center) is grown the same way, a quarter of
each node's reach more, for the model's parts outside its collision geometry
(a helmet's visor), so that a node in view is not culled.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "tag_files/tag_files.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"

#include "object_pose.h"

#include <stdio.h>
#include <string.h>

/* ---------- constants */

/* how much farther than its collision geometry a node's model is taken to
reach, for drawing */
#define OBJECT_POSE_RENDER_MARGIN 1.25f

enum
{
	/* the object definitions posed (a map has a few kinds of biped), and the
	nodes posed in each */
	MAXIMUM_POSED_DEFINITIONS = 64,
	MAXIMUM_POSED_NODES = 16,
	POSE_NODE_LONGS = (MAXIMUM_NODES_PER_MODEL + 31) / 32,
};

/* ---------- structures */

struct object_pose_node
{
	short node_index;
	real scale;
	real_vector3d offset;
};

struct object_pose
{
	long definition_index;
	short node_count;
	struct object_pose_node nodes[MAXIMUM_POSED_NODES];
	/* the nodes posed and those under them, which the spheres hold */
	unsigned long moved[POSE_NODE_LONGS];
	/* how far each node's collision geometry reaches from it, at its own
	scale (0 for none) */
	real reaches[MAXIMUM_NODES_PER_MODEL];
};

struct object_pose_group
{
	char const *name;
	long group_tag;
};

/* ---------- globals */

/* each group a pose names, by the name a mutator gives it */
static struct object_pose_group const object_pose_groups[] =
{
	{ "object", 'obje' },
	{ "unit", 'unit' },
	{ "biped", 'bipd' },
	{ "vehicle", 'vehi' },
	{ "item", 'item' },
	{ "weapon", 'weap' },
	{ "equipment", 'eqip' },
	{ "garbage", 'garb' },
	{ "projectile", 'proj' },
	{ "scenery", 'scen' },
	{ "device", 'devi' },
	{ "machine", 'mach' },
	{ "control", 'ctrl' },
	{ "light_fixture", 'lifi' },
	{ "sound_scenery", 'ssce' },
	{ "placeholder", 'plac' },
};

static struct
{
	short count;
	struct object_pose poses[MAXIMUM_POSED_DEFINITIONS];
} object_pose_globals;

/* ---------- private code */

static struct object_pose *object_pose_find(long definition_index)
{
	short index;

	for (index = 0; index < object_pose_globals.count; index++)
	{
		if (object_pose_globals.poses[index].definition_index == definition_index)
			return &object_pose_globals.poses[index];
	}
	return NULL;
}

static boolean object_pose_moved(struct object_pose const *pose, short node_index)
{
	return (pose->moved[node_index / 32] >> (node_index % 32)) & 1;
}

/* the model's nodes, at most as many as an object has */
static short object_pose_model_node_count(struct model const *model)
{
	return (short)MIN(model->nodes.count, MAXIMUM_NODES_PER_MODEL);
}

/* a node of the definition's model by its name, or a biped's head; NONE if
it has none */
static short object_pose_node_find(long definition_index, char const *node_name)
{
	struct object_definition *definition = object_definition_get(definition_index);
	struct model *model;
	short index;

	if (definition->object.model.index == NONE)
		return NONE;
	model = model_definition_get(definition->object.model.index);
	if (!strcmp(node_name, "head"))
	{
		if (definition->object.type != _object_type_biped)
			return NONE;
		index = biped_definition_get(definition_index)->biped.runtime_head_node_index;
		return VALID_INDEX(index, object_pose_model_node_count(model)) ? index : NONE;
	}
	for (index = 0; index < object_pose_model_node_count(model); index++)
	{
		if (!_stricmp(TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node)->name, node_name))
			return index;
	}
	return NONE;
}

/* how far the collision geometry of a model's node reaches from the node,
at the node's own scale (0 if it has none) */
static real object_pose_reach(long collision_model_index, short node_index)
{
	struct collision_model *model = collision_model_definition_get(collision_model_index);
	struct collision_node *node;
	real reach_squared = 0.f;
	long bsp_index;

	if (!VALID_INDEX(node_index, model->nodes.count))
		return 0.f;
	node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct collision_node);
	for (bsp_index = 0; bsp_index < node->bsps.count; bsp_index++)
	{
		struct collision_bsp *bsp = TAG_BLOCK_GET_ELEMENT(&node->bsps, bsp_index, struct collision_bsp);
		long vertex_index;

		for (vertex_index = 0; vertex_index < bsp->vertices.count; vertex_index++)
		{
			struct collision_vertex *vertex = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, vertex_index,
				struct collision_vertex);
			real distance_squared = vertex->point.x * vertex->point.x + vertex->point.y * vertex->point.y +
				vertex->point.z * vertex->point.z;

			if (distance_squared > reach_squared)
				reach_squared = distance_squared;
		}
	}
	return square_root(reach_squared);
}

/* the node and those under it marked as moved, and their reaches found */
static void object_pose_mark(struct object_pose *pose, short node_index)
{
	struct object_definition *definition = object_definition_get(pose->definition_index);
	struct model *model = model_definition_get(definition->object.model.index);
	short node_count = object_pose_model_node_count(model);
	short stack[MAXIMUM_NODES_PER_MODEL];
	short depth = 0;
	short visits = 0;

	stack[depth++] = node_index;
	/* (each node once: a map's tree that loops ends) */
	while (depth > 0 && visits++ < node_count)
	{
		short index = stack[--depth];
		struct model_node *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node);

		pose->moved[index / 32] |= 1UL << (index % 32);
		pose->reaches[index] = definition->object.collision_model.index == NONE ? 0.f :
			object_pose_reach(definition->object.collision_model.index, index);
		if (index != node_index && VALID_INDEX(node->next_sibling_node_index, node_count) && depth < node_count)
			stack[depth++] = node->next_sibling_node_index;
		if (VALID_INDEX(node->first_child_node_index, node_count) && depth < node_count)
			stack[depth++] = node->first_child_node_index;
	}
}

/* the radius of a sphere about the object's bounding sphere center that
holds its posed nodes, their reaches taken `margin` times; 0 if it has no
pose */
static real object_pose_radius(long object_index, real margin)
{
	struct object_datum *object = object_get(object_index);
	struct object_pose const *pose = object_pose_find(object->definition_index);
	short node_count;
	short index;
	real radius = 0.f;

	if (!pose)
		return 0.f;
	node_count = (short)MIN(object->object.node_matrices.size / (long)sizeof(real_matrix4x3), MAXIMUM_NODES_PER_MODEL);
	for (index = 0; index < node_count; index++)
	{
		real_matrix4x3 const *matrix;
		real reach;

		if (!object_pose_moved(pose, index))
			continue;
		matrix = object_get_node_matrix(object_index, index);
		reach = distance3d(&object->object.bounding_sphere_center, &matrix->position) +
			pose->reaches[index] * matrix->scale * margin;
		radius = MAX(radius, reach);
	}
	return radius;
}

/* ---------- public code */

long object_pose_group_tag(
	char const *name)
{
	short index;

	for (index = 0; index < NUMBEROF(object_pose_groups); index++)
	{
		if (!strcmp(object_pose_groups[index].name, name))
			return object_pose_groups[index].group_tag;
	}
	return NONE;
}

void object_pose_clear(
	void)
{
	object_pose_globals.count = 0;
}

boolean object_pose_add(
	long group_tag,
	char const *tag_name,
	char const *node_name,
	real scale,
	real_vector3d const *offset,
	char *reason,
	unsigned long size)
{
	struct tag_iterator iterator;
	long definition_index;
	short posed = 0;
	boolean tag_found = FALSE;

	tag_iterator_new(&iterator, group_tag);
	while ((definition_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct object_pose *pose;
		short node_index;
		short index;

		if (tag_name[0] && strcmp(tag_get_name(definition_index), tag_name))
			continue;
		tag_found = TRUE;
		node_index = object_pose_node_find(definition_index, node_name);
		if (node_index == NONE)
			continue;
		pose = object_pose_find(definition_index);
		if (!pose)
		{
			if (object_pose_globals.count == MAXIMUM_POSED_DEFINITIONS)
			{
				snprintf(reason, size, "more than %d kinds of object posed", MAXIMUM_POSED_DEFINITIONS);
				return posed > 0;
			}
			pose = &object_pose_globals.poses[object_pose_globals.count++];
			csmemset(pose, 0, sizeof(*pose));
			pose->definition_index = definition_index;
		}
		/* (a node posed again, by another mutator: both poses made) */
		for (index = 0; index < pose->node_count && pose->nodes[index].node_index != node_index; index++);
		if (index == pose->node_count)
		{
			if (pose->node_count == MAXIMUM_POSED_NODES)
			{
				snprintf(reason, size, "more than %d nodes of %.64s posed", MAXIMUM_POSED_NODES,
					tag_get_name(definition_index));
				return posed > 0;
			}
			pose->nodes[pose->node_count].node_index = node_index;
			pose->nodes[pose->node_count].scale = 1.f;
			pose->node_count++;
		}
		pose->nodes[index].scale *= scale;
		pose->nodes[index].offset.i += offset->i;
		pose->nodes[index].offset.j += offset->j;
		pose->nodes[index].offset.k += offset->k;
		object_pose_mark(pose, node_index);
		posed++;
	}
	if (!posed)
	{
		if (tag_found)
			snprintf(reason, size, "no model of %.64s has a node %.32s", tag_name[0] ? tag_name : "the group", node_name);
		else
			snprintf(reason, size, "the map has no such tag");
		return FALSE;
	}
	error(_error_log, "mutators: %.32s posed in %d kinds of object", node_name, posed);
	return TRUE;
}

void object_pose_node_orientations(
	long object_index,
	struct real_orientation *node_orientations)
{
	struct object_pose const *pose;
	short index;

	if (!object_pose_globals.count)
		return;
	pose = object_pose_find(object_get(object_index)->definition_index);
	if (!pose)
		return;
	for (index = 0; index < pose->node_count; index++)
	{
		struct object_pose_node const *node = &pose->nodes[index];
		struct real_orientation *orientation = &node_orientations[node->node_index];

		orientation->scale *= node->scale;
		orientation->translation.x += node->offset.i;
		orientation->translation.y += node->offset.j;
		orientation->translation.z += node->offset.k;
	}
}

real object_pose_bounding_radius(
	long object_index,
	real bounding_radius)
{
	real posed;

	if (!object_pose_globals.count)
		return bounding_radius;
	posed = object_pose_radius(object_index, 1.f);
	return MAX(bounding_radius, posed);
}

real object_pose_render_radius(
	long object_index,
	real render_radius)
{
	real posed;

	if (!object_pose_globals.count)
		return render_radius;
	posed = object_pose_radius(object_index, OBJECT_POSE_RENDER_MARGIN);
	return MAX(render_radius, posed);
}
