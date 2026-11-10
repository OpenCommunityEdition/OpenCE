/*
DEVICE_RENDER_BOUNDS.C

A device's render sphere (the object's render_bounding_radius around its
bounding offset, which only culling reads: objects.h,
object_get_render_bounding_sphere) holds its model at rest, but its position
animation can carry the model far out of it. a30's beam emitter fires its
beam 1089 world units up into the sky out of a sphere of 50 around a point 58
above the emitter: the whole beam was culled as soon as that sphere left the
view, so looking up and down at the beam cut it off and brought it back (its
light and lens flare, drawn of their own, stayed), on the Xbox too. c20's
index platforms ride out of theirs as well.

When a map's tags load (scenario_tags_load), each device with a position
animation gets a render sphere that holds every vertex of every geometry and
part of its model, posed as device_preprocess_node_orientations (devices.c)
poses it, by the model's own nodes: from rest, its position animation at
DEVICE_RENDER_BOUNDS_STEPS + 1 points along its whole travel, each with its
power animation (if it has one) at as many points along its. A vertex is taken
at each of its two nodes and the farther counts, so its weights need not be
read. The radius only ever grows, around the same bounding offset; the
bounding radius physics, pickups and the game state use is left as it is, so
nothing played or sent between machines changes.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "cache/cache_files.h"
#include "devices/device_definitions.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "models/models.h"
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

	/* devices.c's device animations */
	_device_render_bounds_animation_position = 0,
	_device_render_bounds_animation_power,

	/* the steps along each animation */
	DEVICE_RENDER_BOUNDS_STEPS = 16,
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

/* devices.c's animation_graph_device_animations */
struct device_render_bounds_animations
{
	long unused[21];
	struct tag_block animations; /* animation_graph_animation_index */
};

typedef char verify_device_model_geometry_part_size[sizeof(struct model_geometry_part) == 0x68 ? 1 : -1];
typedef char verify_device_render_bounds_animations_size[
	sizeof(struct device_render_bounds_animations) == 0x60 ? 1 : -1];

/* ---------- prototypes */

void device_render_bounds_tags_loaded(void);

/* ---------- private code */

/* the graph's device animation (position or power) for a model of
node_count nodes; NULL: none */
static struct animation const *device_render_bounds_animation(
	struct animation_graph const *graph,
	short device_animation,
	short node_count)
{
	struct device_render_bounds_animations const *device_animations;
	struct animation const *animation;
	short animation_index;

	if (graph->device_animations.count <= 0 || !graph->device_animations.address)
		return NULL;
	device_animations = TAG_BLOCK_GET_ELEMENT(&graph->device_animations, 0, struct device_render_bounds_animations);
	if (device_animations->animations.count <= device_animation || !device_animations->animations.address)
		return NULL;
	animation_index = animation_graph_animation_index_get(&device_animations->animations)[device_animation].animation_index;
	if (!VALID_INDEX(animation_index, graph->animations.count) || !graph->animations.address)
		return NULL;
	animation = TAG_BLOCK_GET_ELEMENT(&graph->animations, animation_index, struct animation);
	if (animation->frame_count <= 0 || animation->node_count <= 0 || animation->node_count > node_count)
		return NULL;

	return animation;
}

/* the farthest from center (squared) of the model's vertices, posed with
skins (each node's matrix times its default inverse), into
*maximum_squared; FALSE if a vertex cannot be read */
static boolean device_render_bounds_pose(
	struct model const *model,
	real_matrix4x3 const *skins,
	real_point3d const *center,
	real *maximum_squared)
{
	long geometry_index;

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
			boolean valid;
			long stride;
			long vertex_index;

			if (TEST_FLAG(part->flags, _model_geometry_part_stripped_bit))
				continue;
			/* (a part with its own node map, which this build's part doesn't
			carry, or a buffer not a model's) */
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
				byte const *vertex = vertices + vertex_index * stride;
				long nodes[2];
				short node;

				if (buffer->type == _rasterizer_vertex_type_model_compressed)
				{
					nodes[0] = ((struct model_vertex_compressed const *)vertex)->nodes[0] / 3;
					nodes[1] = ((struct model_vertex_compressed const *)vertex)->nodes[1] / 3;
				}
				else
				{
					nodes[0] = ((struct model_vertex_uncompressed const *)vertex)->nodes[0];
					nodes[1] = ((struct model_vertex_uncompressed const *)vertex)->nodes[1];
				}
				/* (none: the first) */
				if (!VALID_INDEX(nodes[0], model->nodes.count))
					nodes[0] = 0;
				if (!VALID_INDEX(nodes[1], model->nodes.count))
					nodes[1] = nodes[0];
				for (node = 0; node < 2; node++)
				{
					real_point3d point;
					real distance_squared;

					matrix4x3_transform_point(&skins[nodes[node]], (real_point3d const *)vertex, &point);
					distance_squared = distance_squared3d(&point, center);
					/* (not a number, or infinite) */
					if (!(distance_squared >= 0.f && distance_squared < FLT_MAX))
						valid = FALSE;
					else if (distance_squared > *maximum_squared)
						*maximum_squared = distance_squared;
				}
			}
			if (vertices)
				IDirect3DVertexBuffer8_Unlock(buffer->hardware_format);
			if (!valid)
				return FALSE;
		}
	}

	return TRUE;
}

/* the radius around the bounding offset that holds every vertex of the
device's model wherever its position and power animations pose it; FALSE if
it has no position animation, or its model or animations cannot be read */
static boolean device_render_bounds_radius(
	struct device_definition const *device,
	real *radius)
{
	struct model const *model = model_definition_get(device->object.model.index);
	struct animation_graph const *graph = animation_graph_definition_get(device->object.animation_graph.index);
	struct animation const *position;
	struct animation const *power;
	real_orientation orientations[MAXIMUM_NODES_PER_MODEL];
	real_matrix4x3 matrices[MAXIMUM_NODES_PER_MODEL];
	real_matrix4x3 skins[MAXIMUM_NODES_PER_MODEL];
	real maximum_squared = 0.f;
	short node_count;
	short position_step;

	if (!model || !graph || model->nodes.count <= 0 || model->nodes.count > MAXIMUM_NODES_PER_MODEL ||
		!model->nodes.address ||
		model->geometries.count <= 0 || model->geometries.count > MAXIMUM_GEOMETRIES_PER_MODEL || !model->geometries.address)
	{
		return FALSE;
	}
	node_count = (short)model->nodes.count;
	position = device_render_bounds_animation(graph, _device_render_bounds_animation_position, node_count);
	if (!position)
		return FALSE;
	power = device_render_bounds_animation(graph, _device_render_bounds_animation_power, node_count);

	for (position_step = 0; position_step <= DEVICE_RENDER_BOUNDS_STEPS; position_step++)
	{
		real position_fraction = (real)position_step / DEVICE_RENDER_BOUNDS_STEPS;
		real frame_index = position_fraction * (TEST_FLAG(device->device.flags, _device_position_loops_bit) ?
			(real)position->frame_count : (real)(position->frame_count - 1));
		short power_step;

		for (power_step = 0; power_step <= (power ? DEVICE_RENDER_BOUNDS_STEPS : 0); power_step++)
		{
			real_point3d center;
			short node_index;

			/* (as devices.c's device_preprocess_node_orientations poses it,
			from rest) */
			model_get_node_orientations(model, orientations);
			if (TEST_FLAG(device->device.flags, _device_position_animation_not_interpolated_bit))
				overlay_animation_apply(position, (short)frame_index, orientations);
			else
				overlay_animation_apply_continuous(position, frame_index, orientations);
			if (power)
			{
				overlay_animation_apply_continuous(power,
					power->frame_count * ((real)power_step / DEVICE_RENDER_BOUNDS_STEPS), orientations);
			}
			/* (by the model's own nodes, as object_compute_node_matrices
			poses an object: a30's beam emitter's graph names none) */
			model_node_matrices_from_orientations(model, matrices, orientations,
				global_origin3d, global_forward3d, global_up3d);
			for (node_index = 0; node_index < node_count; node_index++)
			{
				struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);

				matrix4x3_multiply(&matrices[node_index], &node->runtime_default_inverse_matrix, &skins[node_index]);
			}
			/* (around the bounding offset as the root carries it:
			object_compute_node_matrices) */
			matrix4x3_transform_point(&matrices[0], &device->object.bounding_offset, &center);
			if (!device_render_bounds_pose(model, skins, &center, &maximum_squared))
				return FALSE;
		}
	}

	if (!(maximum_squared > 0.f))
		return FALSE;
	/* a little past the farthest vertex, so its rounding leaves it inside */
	*radius = square_root(maximum_squared) * 1.0001f;

	return TRUE;
}

/* ---------- public code */

void device_render_bounds_tags_loaded(
	void)
{
	struct tag_iterator iterator;
	long device_index;

	tag_iterator_new(&iterator, DEVICE_DEFINITION_TAG);
	while ((device_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct device_definition *device = device_definition_get(device_index);
		real radius;

		if (device->object.model.index != NONE && device->object.animation_graph.index != NONE &&
			device_render_bounds_radius(device, &radius) && radius > device->object.render_bounding_radius)
		{
			device->object.render_bounding_radius = radius;
		}
	}

	return;
}
