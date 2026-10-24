/*
MUTATOR_BIG_HEAD.C

The big head mutator (mutators.c's big_head): every biped's head, players'
and the AI's, drawn and hit at big_head.scale times its size.

The head's node is scaled as the biped's node orientations are made
(biped_preprocess_node_orientations), before its node matrices are: its
model is drawn with them, and its collision model is tested with them
(collision_models.c takes each node's matrix and its scale), so the head
is hit where it is drawn, on every machine alike. What hangs from the head
node (its markers) moves with it.

The object's bounding sphere is what the game looks for an object in
before it tests its collision model (a shot, a melee, an explosion, the
host's check of a client's hit), so it is made large enough to hold the
scaled head: past the head node's position by its collision geometry's
reach from the node, scaled. The sphere it is drawn by (its render
bounding radius, about the same center) is grown the same way, a quarter
of the head's reach more, for the model's parts outside its collision
geometry (a helmet's visor), so that a head in view is not culled.
*/

#include "cseries.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"

#include "mutators.h"

/* ---------- constants */

/* how much farther than its collision geometry a head's model is taken to
reach, for drawing */
#define BIG_HEAD_RENDER_MARGIN 1.25f

/* the collision models whose heads' reach is kept (a map has a few kinds
of biped) */
#define MAXIMUM_HEAD_REACHES 32

/* ---------- globals */

static struct
{
	long collision_models[MAXIMUM_HEAD_REACHES];
	real reaches[MAXIMUM_HEAD_REACHES];
	short count;
} big_head_globals;

/* ---------- private code */

/* a biped's head node, if its model has it, else NONE */
static short big_head_node(long object_index)
{
	struct object_datum *object = object_get(object_index);
	struct biped_definition *definition;
	short head;

	if (object->object.type != _object_type_biped)
		return NONE;
	definition = biped_definition_get(object->definition_index);
	head = definition->biped.runtime_head_node_index;
	if (definition->object.model.index == NONE ||
		!VALID_INDEX(head, MIN(model_definition_get(definition->object.model.index)->nodes.count, MAXIMUM_NODES_PER_MODEL)))
	{
		return NONE;
	}
	return head;
}

/* how far the collision geometry of a model's node reaches from the node,
at the node's own scale (0 if it has none) */
static real big_head_reach(long collision_model_index, short node_index)
{
	struct collision_model *model;
	struct collision_node *node;
	real reach_squared = 0.f;
	long bsp_index;
	short index;

	for (index = 0; index < big_head_globals.count; index++)
	{
		if (big_head_globals.collision_models[index] == collision_model_index)
			return big_head_globals.reaches[index];
	}
	model = collision_model_definition_get(collision_model_index);
	if (VALID_INDEX(node_index, model->nodes.count))
	{
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
	}
	if (big_head_globals.count < MAXIMUM_HEAD_REACHES)
	{
		big_head_globals.collision_models[big_head_globals.count] = collision_model_index;
		big_head_globals.reaches[big_head_globals.count++] = square_root(reach_squared);
	}
	return square_root(reach_squared);
}

/* the radius of a sphere about the object's bounding sphere center that
holds its head, scaled, its reach taken `margin` times; 0 if it has no head
or the effect is off */
static real big_head_radius(long object_index, real margin)
{
	struct object_datum *object;
	struct object_definition *definition;
	real_matrix4x3 const *head_matrix;
	real reach;
	short head;

	if (!mutators_effect_active(_mutator_effect_big_head))
		return 0.f;
	head = big_head_node(object_index);
	object = object_get(object_index);
	if (head == NONE || head >= object->object.node_matrices.size / (long)sizeof(real_matrix4x3))
		return 0.f;
	definition = object_definition_get(object->definition_index);
	reach = definition->object.collision_model.index == NONE ? 0.f :
		big_head_reach(definition->object.collision_model.index, head);
	head_matrix = object_get_node_matrix(object_index, head);
	return distance3d(&object->object.bounding_sphere_center, &head_matrix->position) +
		reach * head_matrix->scale * margin;
}

/* ---------- public code */

void mutator_big_head_new_game(
	void)
{
	big_head_globals.count = 0;
}

void mutator_big_head_node_orientations(
	long biped_index,
	struct real_orientation *node_orientations)
{
	short head;

	if (!mutators_effect_active(_mutator_effect_big_head))
		return;
	head = big_head_node(biped_index);
	if (head != NONE)
		node_orientations[head].scale *= mutators_parameter(_mutator_big_head_scale);
}

real mutator_big_head_bounding_radius(
	long object_index,
	real bounding_radius)
{
	return MAX(bounding_radius, big_head_radius(object_index, 1.f));
}

real mutator_big_head_render_radius(
	long object_index,
	real render_radius)
{
	return MAX(render_radius, big_head_radius(object_index, BIG_HEAD_RENDER_MARGIN));
}
