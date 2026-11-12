/*
CAPTAIN_MOUTH.C

Captain Keyes's model (characters\captain\captain, the one that talks, in
a10, a50, c10 and d40) holds its "frame lip lower right" node 2 mm lower and
0.8 mm further back than its left's mirror image, and with it the right half
of his lower lip, the vertices that node moves: the pose the model was made
in. Each frame of his emotions (the "stand emotions" overlay, which the
cutscenes' scripts lay over his pose with unit_set_emotion) moves that node
2 mm further than the left's, back to its level, and the lip is level. With
no emotion (before a cutscene sets one, when it sets one he has no frame for,
and wherever a script sets none) the right half sags: his lips do not meet on
his right, and he talks with a lopsided grin.

So a pose that no emotion's frame is laid over, and that holds the node where
the model has it, has the node moved as the emotions move it: in its parent's
(the jaw's) frame, by how far the vertices only the left half's node moves
sit, on average, above and in front of those only the right half's node
moves. The halves are measured as an Xbox map's tags load
(scenario_tags_load); they must have as many vertices each and be between
half a millimetre and 5 mm apart, so a model already level is left as it is.
A pose that moves the node itself (think_1, think_2, x10's
well_thats_it_then) is left as it was made, and so is every emotion's.

The model's vertices are not changed. Moved to the left half's level, they
stood 2 mm too high under every emotion, which is in the cutscenes: the
emotions' frames level the lip already.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

#include <xtl.h>

#include "captain_mouth.h"

/* ---------- constants */

#define CAPTAIN_MODEL_NAME "characters\\captain\\captain"
#define LOWER_LIP_LEFT_NODE_NAME "frame lip lower left"
#define LOWER_LIP_RIGHT_NODE_NAME "frame lip lower right"

/* how far apart the halves may be to be levelled (the captain's are 2 mm) */
#define LOWER_LIP_MINIMUM_OFFSET 0.0005f
#define LOWER_LIP_MAXIMUM_OFFSET 0.005f

/* how near where the model has it a pose's node is held there (the poses
that move it move it 0.7 mm and more) */
#define LOWER_LIP_HELD_DISTANCE 0.0001f

/* a compressed vertex's weight on its first node, 1 (rasterizer_geometry.c,
compress_real_to_int16_clamp) */
#define MODEL_VERTEX_WHOLE_WEIGHT 32767.f

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

/* the vertices only one half of the lower lip moves: how many, and the sum of
where they are */
struct lower_lip_half
{
	long count;
	real_point3d sum;
};

/* ---------- globals */

/* the captain's model in the map loaded (NONE: none, or its lip not one to
level), its right lower lip's node, where the model holds that node in its
parent's frame, and how far the node is moved there to level the lip */
static long captain_mouth_model_index = NONE;
static short captain_mouth_node_index = NONE;
static real_point3d captain_mouth_node_translation;
static real_vector3d captain_mouth_node_offset;

/* ---------- private code */

/* adds the vertices only the half's node moves */
static void lower_lip_half_add(
	struct lower_lip_half *half,
	struct model_vertex_compressed const *vertices,
	long count,
	short node_index)
{
	long vertex_index;

	for (vertex_index = 0; vertex_index < count; vertex_index++)
	{
		struct model_vertex_compressed const *vertex = &vertices[vertex_index];

		if (vertex->nodes[0] == node_index * 3 && vertex->node_weight == (short)MODEL_VERTEX_WHOLE_WEIGHT)
		{
			half->count++;
			half->sum.x += vertex->position.x;
			half->sum.y += vertex->position.y;
			half->sum.z += vertex->position.z;
		}
	}

	return;
}

/* how far forward and up of the right half the left half is, in the model's
frame (not across: they are mirror images); FALSE when the halves are not
ones to level */
static boolean lower_lip_offset(
	struct lower_lip_half const *left,
	struct lower_lip_half const *right,
	real_vector3d *offset)
{
	real length;

	if (left->count <= 0 || left->count != right->count)
	{
		return FALSE;
	}

	offset->i = left->sum.x / (real)left->count - right->sum.x / (real)right->count;
	offset->j = 0.f;
	offset->k = left->sum.z / (real)left->count - right->sum.z / (real)right->count;
	length = square_root(offset->i * offset->i + offset->k * offset->k);

	return length >= LOWER_LIP_MINIMUM_OFFSET && length <= LOWER_LIP_MAXIMUM_OFFSET;
}

/* a pose's node, if the pose holds it where the model has it, moved by the
offset (both in its parent's frame); FALSE: the pose moves the node itself,
and is left as it was made */
static boolean lower_lip_node_level(
	struct real_orientation *orientation,
	real_point3d const *model_translation,
	real_vector3d const *node_offset)
{
	real_vector3d moved;

	moved.i = orientation->translation.x - model_translation->x;
	moved.j = orientation->translation.y - model_translation->y;
	moved.k = orientation->translation.z - model_translation->z;
	if (moved.i * moved.i + moved.j * moved.j + moved.k * moved.k > LOWER_LIP_HELD_DISTANCE * LOWER_LIP_HELD_DISTANCE)
	{
		return FALSE;
	}

	orientation->translation.x += node_offset->i;
	orientation->translation.y += node_offset->j;
	orientation->translation.z += node_offset->k;

	return TRUE;
}

/* the model's node of that name, or NONE */
static short model_node_find(
	struct model const *model,
	char const *name)
{
	short node_index;

	for (node_index = 0; node_index < model->nodes.count && node_index < MAXIMUM_NODES_PER_MODEL; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);

		if (!csstrcmp(node->name, name))
		{
			return node_index;
		}
	}

	return NONE;
}

/* the part's compressed vertices, locked to be read, or NULL if it has none
this reads */
static struct model_vertex_compressed const *model_part_vertices_lock(
	struct model_geometry_part const *part)
{
	struct vertex_buffer const *buffer = &part->vertex_buffer;
	byte *vertices = NULL;

	if (TEST_FLAG(part->flags, _model_geometry_part_stripped_bit) ||
		TEST_FLAG(part->flags, _model_geometry_part_local_nodes_bit) ||
		buffer->type != _rasterizer_vertex_type_model_compressed || !buffer->hardware_format || buffer->offset ||
		buffer->count <= 0 || buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART)
	{
		return NULL;
	}

	IDirect3DVertexBuffer8_Lock(buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);

	return (struct model_vertex_compressed const *)vertices;
}

/* the halves of the lower lip, measured over each part of each of the
model's geometries whose vertices this reads */
static void model_lower_lip_measure(
	struct model const *model,
	short left_node_index,
	short right_node_index,
	struct lower_lip_half *left,
	struct lower_lip_half *right)
{
	long geometry_index;

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
			struct model_geometry_part const *part = TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part);
			struct model_vertex_compressed const *vertices = model_part_vertices_lock(part);

			if (!vertices)
			{
				continue;
			}
			lower_lip_half_add(left, vertices, part->vertex_buffer.count, left_node_index);
			lower_lip_half_add(right, vertices, part->vertex_buffer.count, right_node_index);
			IDirect3DVertexBuffer8_Unlock(part->vertex_buffer.hardware_format);
		}
	}

	return;
}

/* ---------- public code */

void captain_mouth_tags_loaded(
	void)
{
	long model_index = tag_loaded(MODELS_GROUP_TAG, CAPTAIN_MODEL_NAME);
	struct model const *model;
	struct model_node const *right_node;
	short left_node_index;
	short right_node_index;
	struct lower_lip_half left;
	struct lower_lip_half right;
	real_vector3d offset;

	captain_mouth_model_index = NONE;
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
	left_node_index = model_node_find(model, LOWER_LIP_LEFT_NODE_NAME);
	right_node_index = model_node_find(model, LOWER_LIP_RIGHT_NODE_NAME);
	if (left_node_index == NONE || right_node_index == NONE)
	{
		return;
	}
	right_node = TAG_BLOCK_GET_ELEMENT(&model->nodes, right_node_index, struct model_node);
	if (right_node->parent_node_index < 0 || right_node->parent_node_index >= model->nodes.count)
	{
		return;
	}

	csmemset(&left, 0, sizeof(left));
	csmemset(&right, 0, sizeof(right));
	model_lower_lip_measure(model, left_node_index, right_node_index, &left, &right);
	if (lower_lip_offset(&left, &right, &offset))
	{
		/* (from the model's frame to the node's parent's, as the model
		stands: the parent's inverse matrix at rest) */
		matrix4x3_transform_vector(
			&TAG_BLOCK_GET_ELEMENT(&model->nodes, right_node->parent_node_index, struct model_node)->runtime_default_inverse_matrix,
			&offset,
			&captain_mouth_node_offset);
		captain_mouth_node_translation = right_node->default_translation;
		captain_mouth_node_index = right_node_index;
		captain_mouth_model_index = model_index;
	}

	return;
}

/* a unit's pose, once its emotion's frame has been laid over it or it has
none (units.c, unit_preprocess_node_orientations): the captain's, with no
emotion, has his lower lip levelled */
void captain_mouth_node_orientations(
	long unit_index,
	boolean emotion_applied,
	struct real_orientation *node_orientations)
{
	if (captain_mouth_model_index != NONE && !emotion_applied)
	{
		struct object_datum *object = object_get(unit_index);
		struct object_definition *definition = object_definition_get(object->definition_index);

		/* (and the model of the map loaded now: a map whose tags this
		did not look at leaves the last one's index) */
		if (definition->object.model.index == captain_mouth_model_index &&
			!csstrcmp(tag_get_name(captain_mouth_model_index), CAPTAIN_MODEL_NAME))
		{
			lower_lip_node_level(
				&node_orientations[captain_mouth_node_index],
				&captain_mouth_node_translation,
				&captain_mouth_node_offset);
		}
	}

	return;
}
