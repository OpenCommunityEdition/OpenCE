/*
CAPTAIN_MOUTH.C

Captain Keyes's model (characters\captain\captain, the one that talks, in
a10, a50, c10 and d40) has the right half of his lower lip, the vertices its
"frame lip lower right" node moves, 2 mm lower and 0.8 mm further back than
the left half: they were left where that node had been pulled to when the
model was made (the middle of the lip, half on each node, is halfway). So his
lips never meet on his right, and when he talks his mouth opens further on
that side: a lopsided grin. His models without a face to animate
(captain_ingame, fatigues) have the lip level.

When an Xbox map's tags load (scenario_tags_load), the right half is moved up
and forward to the left half's level: by how far the vertices only the left
half's node moves sit, on average, above and in front of those only the right
half's node moves. Each vertex moves as far as its weight on the right half's
node (the middle, half). The two halves must have as many vertices each and be
between half a millimetre and 5 mm apart, so a model already level is left as
it is. Only what is drawn moves: the lip's node, its animations, and anything
played or sent between machines are as they were.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "tag_files/tag_groups.h"

#include <xtl.h>

/* ---------- constants */

#define CAPTAIN_MODEL_NAME "characters\\captain\\captain"
#define LOWER_LIP_LEFT_NODE_NAME "frame lip lower left"
#define LOWER_LIP_RIGHT_NODE_NAME "frame lip lower right"

/* how far apart the halves may be to be levelled (the captain's are 2 mm) */
#define LOWER_LIP_MINIMUM_OFFSET 0.0005f
#define LOWER_LIP_MAXIMUM_OFFSET 0.005f

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

/* ---------- private code */

/* a compressed vertex's weight on a node (its nodes are three times their
indices; its second node weighs what its first does not) */
static real model_vertex_node_weight(
	struct model_vertex_compressed const *vertex,
	short node_index)
{
	real first_weight = (real)vertex->node_weight / MODEL_VERTEX_WHOLE_WEIGHT;
	real weight = 0.f;

	if (vertex->nodes[0] == node_index * 3)
	{
		weight += first_weight;
	}
	if (vertex->nodes[1] == node_index * 3)
	{
		weight += 1.f - first_weight;
	}

	return weight;
}

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

/* how far forward and up the right half is moved to the left's level (not
across: they are mirror images); FALSE when the halves are not ones to level */
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

/* moves each vertex by the offset, as far as its weight on the right half's
node */
static void lower_lip_right_half_move(
	struct model_vertex_compressed *vertices,
	long count,
	short node_index,
	real_vector3d const *offset)
{
	long vertex_index;

	for (vertex_index = 0; vertex_index < count; vertex_index++)
	{
		struct model_vertex_compressed *vertex = &vertices[vertex_index];
		real weight = model_vertex_node_weight(vertex, node_index);

		if (weight > 0.f)
		{
			vertex->position.x += offset->i * weight;
			vertex->position.y += offset->j * weight;
			vertex->position.z += offset->k * weight;
		}
	}

	return;
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

/* the part's compressed vertices, locked, or NULL if it has none this reads */
static struct model_vertex_compressed *model_part_vertices_lock(
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

	IDirect3DVertexBuffer8_Lock(buffer->hardware_format, 0,
		(UINT)(buffer->count * sizeof(struct model_vertex_compressed)), &vertices, 0);

	return (struct model_vertex_compressed *)vertices;
}

/* each part of each of the model's geometries whose vertices this reads, the
right half moved (or, without, the halves measured) */
static void model_lower_lip_visit(
	struct model const *model,
	short left_node_index,
	short right_node_index,
	struct lower_lip_half *left,
	struct lower_lip_half *right,
	real_vector3d const *offset)
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
			struct model_vertex_compressed *vertices = model_part_vertices_lock(part);

			if (!vertices)
			{
				continue;
			}
			if (offset)
			{
				lower_lip_right_half_move(vertices, part->vertex_buffer.count, right_node_index, offset);
			}
			else
			{
				lower_lip_half_add(left, vertices, part->vertex_buffer.count, left_node_index);
				lower_lip_half_add(right, vertices, part->vertex_buffer.count, right_node_index);
			}
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
	short left_node_index;
	short right_node_index;
	struct lower_lip_half left;
	struct lower_lip_half right;
	real_vector3d offset;

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

	csmemset(&left, 0, sizeof(left));
	csmemset(&right, 0, sizeof(right));
	model_lower_lip_visit(model, left_node_index, right_node_index, &left, &right, NULL);
	if (lower_lip_offset(&left, &right, &offset))
	{
		model_lower_lip_visit(model, left_node_index, right_node_index, NULL, NULL, &offset);
	}

	return;
}
