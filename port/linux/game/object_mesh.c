/*
OBJECT_MESH.C

An object's drawn surface (object_mesh.h), for the port's effects that work on
what is drawn (dynamic blood paints it, stencil shadows cast it, first-person
legs close it): the triangles of its model's most detailed geometry, in the permutation of each region the
object shows, read from the vertex and index buffers the game draws them
from (as powerup_render_bounds.c reads them), with each vertex's nodes and
weight. Opaque parts only (no glass, no effects), and none the game doesn't
draw (stripped parts). A few meshes are kept, the least used going first.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "shaders/shader_definitions.h"
#include "shaders/shaders.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"

#include "object_mesh.h"

#include <xtl.h>
#include <string.h>
#include <stdio.h>

/* ---------- constants */

enum
{
	OBJECT_MESH_CACHE_SIZE = 12, /* (a vehicle, a few kinds of tree and rock, crates: all at hand) */
	OBJECT_MESH_DETAIL_LEVELS = 5, /* (models.c's NUMBER_OF_DETAIL_LEVELS_PER_MODEL) */
};

enum
{
	_object_mesh_part_stripped_bit = 0,
	_object_mesh_part_local_nodes_bit,
};

/* ---------- structures */

/* this build's model geometry and part (models.c's; no header declares
them) */
struct object_mesh_model_geometry
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct object_mesh_model_geometry_part
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

typedef char verify_object_mesh_model_geometry_part_size[sizeof(struct object_mesh_model_geometry_part) == 0x68 ? 1 : -1];

struct object_mesh_model_shader_reference
{
	struct tag_reference shader;
	short permutation_index;
	word pad;
	long unused[3];
};

struct object_mesh_slot
{
	struct object_mesh mesh;
	unsigned long last_used;
	char report[128];
};

/* ---------- globals */

static struct object_mesh_slot object_mesh_slots[OBJECT_MESH_CACHE_SIZE];
static struct object_mesh_vertex object_mesh_vertex_memory[OBJECT_MESH_CACHE_SIZE][OBJECT_MESH_MAXIMUM_VERTICES];
static unsigned short object_mesh_triangle_memory[OBJECT_MESH_CACHE_SIZE][OBJECT_MESH_MAXIMUM_TRIANGLES][3];
static byte object_mesh_triangle_flag_memory[OBJECT_MESH_CACHE_SIZE][OBJECT_MESH_MAXIMUM_TRIANGLES];
static unsigned long object_mesh_clock = 0;
static boolean object_mesh_initialized = FALSE;

/* ---------- private code */

static void object_mesh_initialize(
	void)
{
	short index;

	if (object_mesh_initialized)
		return;
	for (index = 0; index < OBJECT_MESH_CACHE_SIZE; index++)
	{
		object_mesh_slots[index].mesh.key = NONE;
		object_mesh_slots[index].mesh.vertices = object_mesh_vertex_memory[index];
		object_mesh_slots[index].mesh.triangles = object_mesh_triangle_memory[index];
		object_mesh_slots[index].mesh.triangle_flags = object_mesh_triangle_flag_memory[index];
		object_mesh_slots[index].last_used = 0;
	}
	object_mesh_initialized = TRUE;
}

/* (why the last model built has what it has: for object_mesh_report) */
static struct
{
	short parts;
	short stripped;
	short shaders_skipped;
	short no_buffers;
	short formats;
	short unreadable;
	short full;
	short added;
} object_mesh_why;

/* (a key for the model and the permutations it shows) */
static long object_mesh_key(
	long model_index,
	byte const *permutations)
{
	unsigned long key = (unsigned long)model_index * 2654435761ul;
	short index;

	for (index = 0; index < MAXIMUM_REGIONS_PER_OBJECT; index++)
		key = (key ^ permutations[index]) * 16777619ul;
	key &= 0x7fffffff;
	return key == (unsigned long)NONE ? 0 : (long)key;
}

/* a part's triangles and vertices onto the mesh; FALSE when it can't be read
(the mesh is then not used) */
static boolean object_mesh_add_part(
	struct object_mesh *mesh,
	struct model const *model,
	struct object_mesh_model_geometry_part const *part,
	boolean transparent_too)
{
	struct vertex_buffer const *vertex_buffer = &part->vertex_buffer;
	struct triangle_buffer const *triangle_buffer = &part->triangle_buffer;
	byte *vertices = NULL;
	BYTE *indices = NULL;
	long stride;
	long first_vertex = mesh->vertex_count;
	long index_count;
	long index;
	byte triangle_flags = 0;

	/* (only what the game draws, opaque) */
	object_mesh_why.parts++;
	if (TEST_FLAG(part->flags, _object_mesh_part_stripped_bit))
	{
		object_mesh_why.stripped++;
		return TRUE;
	}
	if (VALID_INDEX(part->shader_index, model->shaders.count))
	{
		struct object_mesh_model_shader_reference const *reference = TAG_BLOCK_GET_ELEMENT(
			&model->shaders, part->shader_index, struct object_mesh_model_shader_reference);
		struct shader *shader = reference->shader.index != NONE && reference->shader.index != 0 ?
			shader_definition_get(reference->shader.index) : NULL;
		char const *shader_name = shader ? tag_get_name(reference->shader.index) : NULL;

		/* (a visor, glass: no marks there) */
		if (shader_name && (strstr(shader_name, "visor") || strstr(shader_name, "glass")))
			triangle_flags |= OBJECT_MESH_TRIANGLE_NO_MARKS;

		/* (any shader at all, on the see-through pass: a tree drawn in
		whatever shaders it is drawn in) */
		if (!shader || (!transparent_too && (!shader_type_is_valid_for_model(shader->base.type) ||
			shader_type_is_transparent(shader->base.type))))
		{
			object_mesh_why.shaders_skipped++;
			return TRUE;
		}
	}
	else if (!transparent_too)
	{
		object_mesh_why.shaders_skipped++;
		return TRUE;
	}
	if (!vertex_buffer->hardware_format ||
		vertex_buffer->count <= 0 || !triangle_buffer->hardware_format ||
		triangle_buffer->count <= 0)
	{
		object_mesh_why.no_buffers++;
		return TRUE;
	}
	if (vertex_buffer->type == _rasterizer_vertex_type_model_compressed)
		stride = sizeof(struct model_vertex_compressed);
	else if (vertex_buffer->type == _rasterizer_vertex_type_model_uncompressed)
		stride = sizeof(struct model_vertex_uncompressed);
	else
	{
		object_mesh_why.formats++;
		return TRUE;
	}
	if (triangle_buffer->type == 1)
		index_count = triangle_buffer->count + 2; /* a strip */
	else if (triangle_buffer->type == 0)
		index_count = triangle_buffer->count * 3; /* a list */
	else
	{
		object_mesh_why.formats++;
		return TRUE;
	}
	if (mesh->vertex_count + vertex_buffer->count > OBJECT_MESH_MAXIMUM_VERTICES)
	{
		object_mesh_why.full++;
		return FALSE;
	}

	IDirect3DVertexBuffer8_Lock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);
	IDirect3DIndexBuffer8_Lock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format, 0, 0, &indices, D3DLOCK_READONLY);
	if (!vertices || !indices)
	{
		if (vertices)
			IDirect3DVertexBuffer8_Unlock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format);
		if (indices)
			IDirect3DIndexBuffer8_Unlock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format);
		object_mesh_why.unreadable++;
		return TRUE;
	}
	object_mesh_why.added++;

	/* its vertices, with their nodes */
	for (index = 0; index < vertex_buffer->count; index++)
	{
		struct object_mesh_vertex *out = &mesh->vertices[mesh->vertex_count + index];
		long node0;
		long node1;
		real weight;

		if (vertex_buffer->type == _rasterizer_vertex_type_model_compressed)
		{
			struct model_vertex_compressed const *in = (struct model_vertex_compressed const *)(vertices + index * stride);

			out->position = in->position;
			node0 = in->nodes[0] / 3;
			node1 = in->nodes[1] / 3;
			weight = (real)in->node_weight * (1.f / 32767.f);
		}
		else
		{
			struct model_vertex_uncompressed const *in = (struct model_vertex_uncompressed const *)(vertices + index * stride);

			out->position = in->position;
			node0 = in->nodes[0];
			node1 = in->nodes[1];
			weight = in->node_weights[0];
		}
		/* (a part with its own node map, which this build's part doesn't
		carry: held to the node its middle is on; a tree's trunk, a rock) */
		if (TEST_FLAG(part->flags, _object_mesh_part_local_nodes_bit))
		{
			node0 = node1 = VALID_INDEX(part->centroid_primary_node_index, mesh->node_count) ?
				part->centroid_primary_node_index : 0;
			weight = 1.f;
		}
		if (!VALID_INDEX(node0, mesh->node_count))
			node0 = 0;
		if (!VALID_INDEX(node1, mesh->node_count))
		{
			node1 = node0;
			weight = 1.f;
		}
		if (!valid_real(out->position.x) || !valid_real(out->position.y) || !valid_real(out->position.z))
			out->position.x = out->position.y = out->position.z = 0.f;
		out->nodes[0] = (byte)node0;
		out->nodes[1] = (byte)node1;
		out->weight = (short)(PIN(valid_real(weight) ? weight : 1.f, 0.f, 1.f) * 32767.f);
	}
	mesh->vertex_count += vertex_buffer->count;

	/* its triangles: a strip's every one (turning, its degenerate ones left
	out), or a list's */
	{
		unsigned short const *list = (unsigned short const *)indices;

		for (index = 0; index + 2 < index_count && mesh->triangle_count < OBJECT_MESH_MAXIMUM_TRIANGLES;
			index += (triangle_buffer->type == 1) ? 1 : 3)
		{
			unsigned short a = list[index];
			unsigned short b = list[index + 1];
			unsigned short c = list[index + 2];
			unsigned short *out = mesh->triangles[mesh->triangle_count];

			if (a == b || b == c || a == c ||
				a >= vertex_buffer->count || b >= vertex_buffer->count || c >= vertex_buffer->count)
			{
				continue;
			}
			/* (a strip's odd triangles turn the other way) */
			if (triangle_buffer->type == 1 && (index & 1))
			{
				unsigned short swap = b;

				b = c;
				c = swap;
			}
			out[0] = (unsigned short)(first_vertex + a);
			out[1] = (unsigned short)(first_vertex + b);
			out[2] = (unsigned short)(first_vertex + c);
			mesh->triangle_flags[mesh->triangle_count] = triangle_flags;
			mesh->triangle_count++;
		}
	}

	IDirect3DVertexBuffer8_Unlock((IDirect3DVertexBuffer8 *)vertex_buffer->hardware_format);
	IDirect3DIndexBuffer8_Unlock((IDirect3DIndexBuffer8 *)triangle_buffer->hardware_format);
	return TRUE;
}

static boolean object_mesh_build(
	struct object_mesh *mesh,
	long model_index,
	byte const *permutations)
{
	struct model *model = model_definition_get(model_index);
	short region_index;
	short node_index;
	short pass;

	memset(&object_mesh_why, 0, sizeof(object_mesh_why));
	mesh->model_index = model_index;
	mesh->outward = 1;
	mesh->vertex_count = 0;
	mesh->triangle_count = 0;
	if (!model || !model->nodes.address || model->nodes.count <= 0 || !model->regions.address ||
		model->geometries.count <= 0 || !model->geometries.address)
	{
		return FALSE;
	}
	mesh->node_count = (short)MIN(model->nodes.count, OBJECT_MESH_MAXIMUM_NODES);
	for (node_index = 0; node_index < mesh->node_count; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);

		mesh->inverse[node_index] = node->runtime_default_inverse_matrix;
	}

	/* (what it draws opaque; or, a model drawn all in see-through shaders, a
	tree's bark and leaves both, all of it) */
	for (pass = 0; pass < 2 && mesh->triangle_count == 0; pass++)
	{
	mesh->vertex_count = 0;
	for (region_index = 0; region_index < MIN(model->regions.count, MAXIMUM_REGIONS_PER_OBJECT); region_index++)
	{
		struct model_region const *region = TAG_BLOCK_GET_ELEMENT(&model->regions, region_index, struct model_region);
		short permutation_index = permutations[region_index];
		struct model_region_permutation const *permutation;
		short geometry_index;
		short level;

		if (!VALID_INDEX(permutation_index, region->permutations.count) || !region->permutations.address)
			continue;
		permutation = TAG_BLOCK_GET_ELEMENT(&region->permutations, permutation_index, struct model_region_permutation);
		/* (its most detailed geometry) */
		geometry_index = NONE;
		for (level = OBJECT_MESH_DETAIL_LEVELS - 1; level >= 0 && !VALID_INDEX(geometry_index, model->geometries.count); level--)
			geometry_index = permutation->geometry_indices[level];
		if (!VALID_INDEX(geometry_index, model->geometries.count))
			continue;
		{
			struct object_mesh_model_geometry const *geometry = TAG_BLOCK_GET_ELEMENT(
				&model->geometries, geometry_index, struct object_mesh_model_geometry);
			long part_index;

			if (geometry->parts.count <= 0 || geometry->parts.count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY || !geometry->parts.address)
				continue;
			for (part_index = 0; part_index < geometry->parts.count; part_index++)
			{
				struct object_mesh_model_geometry_part const *part = TAG_BLOCK_GET_ELEMENT(
					&geometry->parts, part_index, struct object_mesh_model_geometry_part);

				if (!object_mesh_add_part(mesh, model, part, pass == 1))
				{
					/* (full: what it has) */
					region_index = MAXIMUM_REGIONS_PER_OBJECT;
					break;
				}
			}
		}
	}
	}
	/* (which way the triangles turn: out of the model, by the volume they
	enclose) */
	{
		double volume = 0.;
		long triangle;

		for (triangle = 0; triangle < mesh->triangle_count; triangle++)
		{
			real_point3d const *a = &mesh->vertices[mesh->triangles[triangle][0]].position;
			real_point3d const *b = &mesh->vertices[mesh->triangles[triangle][1]].position;
			real_point3d const *c = &mesh->vertices[mesh->triangles[triangle][2]].position;

			volume += a->x * (b->y * c->z - b->z * c->y) - a->y * (b->x * c->z - b->z * c->x) +
				a->z * (b->x * c->y - b->y * c->x);
		}
		mesh->outward = volume >= 0. ? 1 : -1;
	}
	return mesh->triangle_count > 0;
}

/* ---------- public code */

void object_mesh_reset(
	void)
{
	short index;

	object_mesh_initialize();
	for (index = 0; index < OBJECT_MESH_CACHE_SIZE; index++)
		object_mesh_slots[index].mesh.key = NONE;
}

struct object_mesh const *object_mesh_get(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition;
	struct object_mesh_slot *slot = NULL;
	long model_index;
	long key;
	short index;

	object_mesh_initialize();
	if (!object)
		return NULL;
	definition = object_definition_get(object->definition_index);
	model_index = definition ? definition->object.model.index : NONE;
	if (model_index == NONE)
		return NULL;
	key = object_mesh_key(model_index, object->object.region_permutations);
	object_mesh_clock++;

	for (index = 0; index < OBJECT_MESH_CACHE_SIZE; index++)
	{
		struct object_mesh_slot *candidate = &object_mesh_slots[index];

		if (candidate->mesh.key == key && candidate->mesh.model_index == model_index)
		{
			candidate->last_used = object_mesh_clock;
			return candidate->mesh.triangle_count > 0 ? &candidate->mesh : NULL;
		}
		if (!slot || candidate->mesh.key == NONE ||
			(slot->mesh.key != NONE && candidate->last_used < slot->last_used))
		{
			slot = candidate;
		}
	}

	slot->mesh.key = key;
	slot->last_used = object_mesh_clock;
	if (!object_mesh_build(&slot->mesh, model_index, object->object.region_permutations))
		slot->mesh.triangle_count = 0; /* (remembered: none to paint) */
	snprintf(slot->report, sizeof(slot->report),
		"%s: %ld triangles; parts %d (added %d, stripped %d, shaders %d, no buffers %d, formats %d, unreadable %d, full %d)",
		tag_get_name(model_index), slot->mesh.triangle_count, object_mesh_why.parts, object_mesh_why.added,
		object_mesh_why.stripped, object_mesh_why.shaders_skipped, object_mesh_why.no_buffers, object_mesh_why.formats,
		object_mesh_why.unreadable, object_mesh_why.full);
	return slot->mesh.triangle_count > 0 ? &slot->mesh : NULL;
}


char const *object_mesh_report(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition = object ? object_definition_get(object->definition_index) : NULL;
	long model_index = definition ? definition->object.model.index : NONE;
	short index;

	if (model_index == NONE)
		return "no model";
	object_mesh_get(object_index);
	for (index = 0; index < OBJECT_MESH_CACHE_SIZE; index++)
	{
		if (object_mesh_slots[index].mesh.key == object_mesh_key(model_index, object->object.region_permutations) &&
			object_mesh_slots[index].mesh.model_index == model_index)
		{
			return object_mesh_slots[index].report;
		}
	}
	return "?";
}
