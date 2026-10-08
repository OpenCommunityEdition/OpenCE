/*
POWERUP_RENDER_BOUNDS.C

The overshield's and active camouflage's authored render spheres (the
object's render_bounding_radius, which only culling reads: objects.h,
object_get_render_bounding_sphere) are smaller than their meshes, so near the
edges of the screen they were culled while still in view. When a map's tags
load (scenario_tags_load), each powerup of those two kinds with a rigid
one-node model and no animations gets a radius that holds every vertex of
every geometry and part of its model, around its bounding offset. The radius
only ever grows; the bounding radius physics, pickups and the game state use
is left as it is, so nothing played or sent between machines changes.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "cache/cache_files.h"
#include "items/equipment_definitions.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "tag_files/tag_groups.h"

#include <float.h>
#include <xtl.h>

/* ---------- constants */

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

/* ---------- private code */

/* the radius around center that holds every vertex of a rigid model's
geometries; FALSE if the model is not one, or a vertex cannot be read */
static boolean model_rigid_render_radius(
	struct model const *model,
	real_point3d const *center,
	real *radius)
{
	struct model_node const *root;
	real maximum_squared = 0.f;
	long vertex_count = 0;
	long geometry_index;

	if (model->nodes.count != 1 || !model->nodes.address ||
		model->geometries.count <= 0 || model->geometries.count > MAXIMUM_GEOMETRIES_PER_MODEL ||
		!model->geometries.address)
	{
		return FALSE;
	}

	/* every level of detail and permutation, in the root node's space */
	root = TAG_BLOCK_GET_ELEMENT(&model->nodes, 0, struct model_node);
	for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
	{
		struct model_geometry const *geometry = TAG_BLOCK_GET_ELEMENT(&model->geometries, geometry_index, struct model_geometry);
		long part_index;

		if (geometry->parts.count < 0 || geometry->parts.count > MAXIMUM_PARTS_PER_MODEL_GEOMETRY ||
			(geometry->parts.count && !geometry->parts.address))
		{
			return FALSE;
		}

		for (part_index = 0; part_index < geometry->parts.count; part_index++)
		{
			struct model_geometry_part const *part = TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part);
			struct vertex_buffer const *buffer = &part->vertex_buffer;
			byte *vertices = NULL;
			boolean valid = TRUE;
			long stride;
			long vertex_index;

			if (TEST_FLAG(part->flags, _model_geometry_part_stripped_bit))
			{
				continue;
			}
			if (TEST_FLAG(part->flags, _model_geometry_part_local_nodes_bit) || !buffer->hardware_format || buffer->offset ||
				buffer->count <= 0 || buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART ||
				(buffer->type != _rasterizer_vertex_type_model_compressed && buffer->type != _rasterizer_vertex_type_model_uncompressed))
			{
				return FALSE;
			}

			stride = rasterizer_geometry_get_vertex_size(buffer->type);
			IDirect3DVertexBuffer8_Lock(buffer->hardware_format, 0, 0, &vertices, D3DLOCK_READONLY);
			valid = vertices != NULL;
			for (vertex_index = 0; valid && vertex_index < buffer->count; vertex_index++)
			{
				real_point3d point;
				real distance_squared;

				matrix4x3_transform_point(&root->runtime_default_inverse_matrix,
					(real_point3d const *)(vertices + vertex_index * stride), &point);
				distance_squared = distance_squared3d(&point, center);
				/* (not a number, or infinite) */
				if (!(distance_squared >= 0.f && distance_squared < FLT_MAX))
				{
					valid = FALSE;
				}
				else
				{
					maximum_squared = MAX(maximum_squared, distance_squared);
				}
			}
			IDirect3DVertexBuffer8_Unlock(buffer->hardware_format);
			if (!valid)
			{
				return FALSE;
			}
			vertex_count += buffer->count;
		}
	}

	if (!vertex_count)
	{
		return FALSE;
	}

	/* a little past the farthest vertex, so its rounding leaves it inside */
	*radius = square_root(maximum_squared) * 1.0001f;

	return TRUE;
}

/* ---------- public code */

void powerup_render_bounds_tags_loaded(
	void)
{
	struct tag_iterator iterator;
	long equipment_index;

	tag_iterator_new(&iterator, EQUIPMENT_DEFINITION_TAG);
	while ((equipment_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct equipment_definition *equipment = equipment_definition_get(equipment_index);
		real radius;

		if ((equipment->equipment.powerup_type == _equipment_powerup_overshield ||
				equipment->equipment.powerup_type == _equipment_powerup_active_camouflage) &&
			equipment->object.model.index != NONE && equipment->object.animation_graph.index == NONE &&
			model_rigid_render_radius(model_definition_get(equipment->object.model.index), &equipment->object.bounding_offset, &radius) &&
			radius > equipment->object.render_bounding_radius)
		{
			equipment->object.render_bounding_radius = radius;
		}
	}

	return;
}
