/*
TAG_SCHEMA_MODELS.C

The schemas (tag_schema.h) of models and animations (mode, antr).

Besides the fields, the checks: a model's and an animation graph's nodes
are a tree from node 0 (models.c's model_get_node_matrices walks it, with
a queue of MAXIMUM_NODES_PER_MODEL), a model's geometry parts draw only
vertices and indices that lie in the tags (their vertex and index buffers
are the map header's), an animation's frames are all inside its data (the
node flags say how many bytes a frame and the defaults take, compressed
animations' keyframes are in order), and the animations' permutation
lists (next_animation_index) end.
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "items/weapons.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/object_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "units/units.h"

#include <string.h>

/* ---------- constants */

enum
{
	/* model_animations.c */
	COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS = 12,
	NUMBER_OF_ANIMATION_DAMAGE_TYPES = 4,
	NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS = 4,
	NUMBER_OF_DAMAGE_PARTS = 11,
	/* bipeds.c: none, dx dy, dx dy dyaw, dx dy dz dyaw */
	NUMBER_OF_ANIMATION_FRAME_INFO_TYPES = 4,
	/* the animation lists' lengths (model_animation_definitions.c) */
	NUMBER_OF_WEAPON_TYPE_ANIMATIONS = 10,
	NUMBER_OF_WEAPON_ANIMATIONS = 11,
	NUMBER_OF_VEHICLE_ANIMATIONS = 8,
	NUMBER_OF_DEVICE_ANIMATIONS = 2,
	NUMBER_OF_UNIT_DAMAGE_ANIMATIONS_PER_GRAPH =
		NUMBER_OF_ANIMATION_DAMAGE_TYPES * NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS * NUMBER_OF_DAMAGE_PARTS,
	/* objects.c: an overlay is played by an outgoing function (A to D),
	by frame or by scale */
	NUMBER_OF_OBJECT_OVERLAY_FUNCTIONS = 4,
	/* vehicle_datum.h's suspension states */
	MAXIMUM_SUSPENSIONS_PER_VEHICLE = 8,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_OBJECT_OVERLAYS_PER_GRAPH = 4,
	MAXIMUM_UNIT_SEATS_PER_GRAPH = 32,
	MAXIMUM_WEAPON_CLASSES_PER_UNIT_SEAT = 16,
	MAXIMUM_WEAPON_TYPES_PER_WEAPON_CLASS = 16,
	MAXIMUM_IK_POINTS = 4,
	MAXIMUM_ANIMATION_DATA_SIZE = MAXIMUM_FRAMES_PER_ANIMATION * MAXIMUM_NODES_PER_ANIMATION * 24,
	MAXIMUM_ANIMATION_DEFAULT_DATA_SIZE = MAXIMUM_NODES_PER_ANIMATION * 24,
	MAXIMUM_ANIMATION_FRAME_INFO_SIZE = MAXIMUM_FRAMES_PER_ANIMATION * 16,

	/* a compressed animation's header, up to its rotation node headers */
	COMPRESSED_ANIMATION_HEADER_SIZE = 0x2C,
};

/* ---------- structures */

/* models.c's */

struct model_shader_reference
{
	struct tag_reference shader;
	short permutation_index;
	word pad;
	long unused[3];
};

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

/* a region permutation's marker (model_definitions.h's markers block, which
the game never reads) */
struct model_region_permutation_marker
{
	char name[32];
	short node_index;
	word pad;
	real_quaternion rotation;
	real_point3d translation;
	byte unused[16];
};

/* a model's vertices and triangles as the tags keep them (the parts' blocks,
read only by rasterizer.c's debug display) */
struct model_vertex_uncompressed_element
{
	byte bytes[68];
};

struct model_vertex_compressed_element
{
	byte bytes[32];
};

struct model_triangle_element
{
	short vertex_indices[3];
};

typedef char verify_model_shader_reference_size[sizeof(struct model_shader_reference) == 0x20 ? 1 : -1];
typedef char verify_model_geometry_size[sizeof(struct model_geometry) == 0x30 ? 1 : -1];
typedef char verify_model_geometry_part_size[sizeof(struct model_geometry_part) == 0x68 ? 1 : -1];
typedef char verify_model_region_permutation_marker_size[
	sizeof(struct model_region_permutation_marker) == 0x50 ? 1 : -1];
typedef char verify_model_marker_instance_size[sizeof(struct model_marker_instance) == 0x20 ? 1 : -1];
typedef char verify_model_marker_size[sizeof(struct model_marker) == 0x40 ? 1 : -1];
typedef char verify_model_node_size[sizeof(struct model_node) == 0x9C ? 1 : -1];
typedef char verify_model_region_permutation_size[sizeof(struct model_region_permutation) == 0x58 ? 1 : -1];
typedef char verify_model_region_size[sizeof(struct model_region) == 0x4C ? 1 : -1];
typedef char verify_model_size[sizeof(struct model) == 0xE8 ? 1 : -1];

/* model_animations.c's */

struct animation_graph_node
{
	char name[TAG_STRING_LENGTH+1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	word pad;
	unsigned long flags;
	real_vector3d base_vector;
	real range;
	long pad1;
};

struct animation_graph_sound_reference
{
	struct tag_reference sound;
	long unused;
};

struct compressed_animation_header
{
	long rotation_keyframe_frame_indices_offset;
	long default_rotations_offset;
	long rotation_keyframes_offset;
	long translation_node_headers_offset;
	long translation_keyframe_frame_indices_offset;
	long default_translations_offset;
	long translation_keyframes_offset;
	long scale_node_headers_offset;
	long scale_keyframe_frame_indices_offset;
	long default_scales_offset;
	long scale_keyframes_offset;
	unsigned long rotation_node_headers[1];
};

/* weapons.c's, first_person_weapons.c's */
struct animation_graph_weapon_animations
{
	long unused1[4];
	struct tag_block animations;
};

/* vehicles.c's */
struct vehicle_animation
{
	struct animation_aiming_screen_bounds steering_screen_bounds;
	long unused[0x11];
	struct tag_block animations;
	struct tag_block suspensions;
};

struct vehicle_suspension
{
	short mass_point_index;
	short animation_index;
	real unknown4;
	real unknown8;
	byte unknownc[8];
};

/* devices.c's */
struct animation_graph_device_animations
{
	long unused[21];
	struct tag_block animations;
};

typedef char verify_animation_graph_node_size[sizeof(struct animation_graph_node) == 0x40 ? 1 : -1];
typedef char verify_animation_graph_sound_reference_size[
	sizeof(struct animation_graph_sound_reference) == 0x14 ? 1 : -1];
typedef char verify_compressed_animation_header_size[
	offsetof(struct compressed_animation_header, rotation_node_headers) == COMPRESSED_ANIMATION_HEADER_SIZE ? 1 : -1];
typedef char verify_animation_graph_weapon_animations_size[
	sizeof(struct animation_graph_weapon_animations) == 0x1C ? 1 : -1];
typedef char verify_vehicle_animation_size[sizeof(struct vehicle_animation) == 0x74 ? 1 : -1];
typedef char verify_vehicle_suspension_size[sizeof(struct vehicle_suspension) == 0x14 ? 1 : -1];
typedef char verify_animation_graph_device_animations_size[
	sizeof(struct animation_graph_device_animations) == 0x60 ? 1 : -1];
typedef char verify_animation_graph_object_overlay_size[sizeof(struct animation_graph_object_overlay) == 0x14 ? 1 : -1];
typedef char verify_animation_graph_unit_seat_size[sizeof(struct animation_graph_unit_seat) == 0x64 ? 1 : -1];
typedef char verify_animation_graph_weapon_class_size[sizeof(struct animation_graph_weapon_class) == 0xBC ? 1 : -1];
typedef char verify_animation_graph_weapon_type_size[sizeof(struct animation_graph_weapon_type) == 0x3C ? 1 : -1];
typedef char verify_animation_graph_ik_point_size[sizeof(struct animation_graph_ik_point) == 0x40 ? 1 : -1];
typedef char verify_animation_size[sizeof(struct animation) == 0xB4 ? 1 : -1];
typedef char verify_animation_graph_size[sizeof(struct animation_graph) == 0x80 ? 1 : -1];

/* ---------- globals */

/* rasterizer_geometry.c's rasterizer_vertex_type_sizes */
static short const model_vertex_type_sizes[NUMBER_OF_RASTERIZER_VERTEX_TYPES] =
{
	56, 32, 20, 8, 68, 32, 24, 36, 20, 16, 16, 8
};

/* ---------- private code */

/* node graphs (a model's, an animation graph's) */

static short *node_link(
	byte *nodes,
	long node_size,
	short node_index,
	long link_offset)
{
	return (short *)(nodes + node_index * node_size + link_offset);
}

/* the children of parent_index along the links from *link (its first
child's, then each child's next sibling): each reached once, each naming
its parent. A link to a node already reached (a loop, or a node in two
places) is cut. Returns the last link of the chain */
static short *node_chain_walk(
	struct tag_validation *validation,
	byte *nodes,
	long node_count,
	long node_size,
	long sibling_offset,
	long parent_offset,
	short parent_index,
	short *link,
	boolean *reached,
	short *queue,
	short *write_index)
{
	while (*link != NONE)
	{
		short node_index = *link;
		short *parent;

		if (node_index < 0 || node_index >= node_count || reached[node_index])
		{
			tag_validate_correct(validation, "has node %d's links reach node %d twice: cut", parent_index, node_index);
			*link = NONE;
			break;
		}
		reached[node_index] = TRUE;
		queue[(*write_index)++] = node_index;
		parent = node_link(nodes, node_size, node_index, parent_offset);
		if (*parent != parent_index)
		{
			tag_validate_correct(validation, "has node %d's parent %d, not %d: %d", node_index, *parent,
				parent_index, parent_index);
			*parent = parent_index;
		}
		link = node_link(nodes, node_size, node_index, sibling_offset);
	}

	return link;
}

/* the nodes (node_count of node_size bytes, no more than
MAXIMUM_NODES_PER_MODEL) are a tree from node 0: the root has no sibling,
every other node is reached once from it. A node the root does
not reach is made its last child */
static boolean node_tree_check(
	struct tag_validation *validation,
	byte *nodes,
	long node_count,
	long node_size,
	long sibling_offset,
	long child_offset,
	long parent_offset)
{
	boolean reached[MAXIMUM_NODES_PER_MODEL];
	short queue[MAXIMUM_NODES_PER_MODEL];
	short read_index = 0;
	short write_index = 0;
	short *root_tail = NULL;
	short *link;
	short node_index;

	if (node_count <= 0)
		return TRUE;
	if (node_count > MAXIMUM_NODES_PER_MODEL ||
		!tag_validate_contains(validation, nodes, (unsigned long)(node_count * node_size)))
	{
		tag_validate_refuse(validation, "has %ld nodes outside the tags", node_count);
		return FALSE;
	}
	memset(reached, 0, sizeof(reached));

	/* (the root's parent is never read: an animation graph's is often 0) */
	link = node_link(nodes, node_size, 0, sibling_offset);
	if (*link != NONE)
	{
		tag_validate_correct(validation, "has the root node's sibling %d: none", *link);
		*link = NONE;
	}
	reached[0] = TRUE;
	queue[write_index++] = 0;

	while (TRUE)
	{
		while (read_index < write_index)
		{
			short parent_index = queue[read_index++];

			link = node_chain_walk(validation, nodes, node_count, node_size, sibling_offset, parent_offset,
				parent_index, node_link(nodes, node_size, parent_index, child_offset), reached, queue, &write_index);
			if (parent_index == 0)
				root_tail = link;
		}

		/* (a node the root does not reach) */
		for (node_index = 1; node_index < node_count && reached[node_index]; node_index++);
		if (node_index >= node_count)
			break;
		tag_validate_correct(validation, "has node %d unreached from the root: made the root's child", node_index);
		*node_link(nodes, node_size, node_index, sibling_offset) = NONE;
		*node_link(nodes, node_size, node_index, parent_offset) = 0;
		*root_tail = node_index;
		root_tail = node_link(nodes, node_size, node_index, sibling_offset);
		reached[node_index] = TRUE;
		queue[write_index++] = node_index;
	}

	return TRUE;
}

/* models */

/* the vertices and indices a part draws (rasterizer_xbox_draw_primitives.c,
rasterizer_draw_static_triangles_static_vertices) lie in the tags: its
buffers are the map header's (the game draws from their data), its vertex
count of its type's size fits from its vertex buffer's data, its triangles'
indices fit from its index buffer's data and each names one of its
vertices. A part whose buffers cannot be drawn gets none, and is not drawn
(the draw takes none) */
static boolean model_geometry_part_check(
	struct tag_validation *validation,
	void *base)
{
	struct model_geometry_part *part = base;
	struct vertex_buffer *vertex_buffer = &part->vertex_buffer;
	struct triangle_buffer *triangle_buffer = &part->triangle_buffer;
	byte *vertices;
	word *indices;
	long vertex_size;
	long index_count;
	long bad_index_count = 0;
	long index;

	/* (a cache's parts keep no vertices or triangles of their own: the game
	draws from the buffers, and what reads those blocks, the debug vertex
	display, does so unchecked) */
	if (part->uncompressed_vertices.count || part->compressed_vertices.count || part->triangles.count)
	{
		tag_validate_correct(validation, "has %ld uncompressed vertices, %ld compressed vertices and %ld triangles"
			" of its own: none", part->uncompressed_vertices.count, part->compressed_vertices.count,
			part->triangles.count);
		part->uncompressed_vertices.count = 0;
		part->uncompressed_vertices.address = NULL;
		part->compressed_vertices.count = 0;
		part->compressed_vertices.address = NULL;
		part->triangles.count = 0;
		part->triangles.address = NULL;
	}
	if (!vertex_buffer->hardware_format || !triangle_buffer->hardware_format)
		return TRUE;
	vertices = tag_validate_vertex_buffer_data(validation, vertex_buffer->hardware_format);
	indices = tag_validate_index_buffer_data(validation, triangle_buffer->hardware_format);
	vertex_size = VALID_INDEX(vertex_buffer->type, NUMBER_OF_RASTERIZER_VERTEX_TYPES) ?
		model_vertex_type_sizes[vertex_buffer->type] :
		0;
	switch (triangle_buffer->type)
	{
	case _triangle_buffer_type_triangles:
		index_count = 3 * triangle_buffer->count;
		break;
	case _triangle_buffer_type_precompiled_strip:
		index_count = triangle_buffer->count > 0 ? triangle_buffer->count + 2 : 0;
		break;
	default:
		index_count = 0;
		break;
	}

	if (!vertices || !indices)
	{
		tag_validate_correct(validation, "has a vertex or index buffer not of the map's: none");
	}
	else if (vertex_buffer->count < 0 || vertex_buffer->count > MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART ||
		!tag_validate_contains(validation, vertices, (unsigned long)(vertex_buffer->count * vertex_size)))
	{
		tag_validate_correct(validation, "has %ld vertices of %ld bytes outside the tags: none",
			vertex_buffer->count, vertex_size);
	}
	else if (triangle_buffer->count < 0 || triangle_buffer->count > MAXIMUM_TRIANGLES_PER_MODEL_GEOMETRY_PART ||
		!tag_validate_contains(validation, indices, (unsigned long)(index_count * sizeof(word))))
	{
		tag_validate_correct(validation, "has %ld triangles, %ld indices outside the tags: none",
			triangle_buffer->count, index_count);
	}
	else if (index_count && tag_validate_any_claimed(indices, (unsigned long)(index_count * sizeof(word))))
	{
		/* (the indices say how many vertices a draw reads: ones in bytes
		of a tag, which the game writes to as it runs, could change after
		this check) */
		tag_validate_correct(validation, "has %ld indices in a tag's bytes: none", index_count);
	}
	else
	{
		/* (the vertices are only read, by index, so they may lie where
		another tag is) */
		for (index = 0; index < index_count; index++)
		{
			if (indices[index] >= vertex_buffer->count)
				bad_index_count++;
		}
		if (bad_index_count)
		{
			tag_validate_correct(validation, "has %ld indices past its %ld vertices: none", bad_index_count,
				vertex_buffer->count);
		}
		else if (!vertex_buffer->count && index_count)
		{
			tag_validate_correct(validation, "has %ld indices and no vertices: none", index_count);
		}
		else
		{
			return TRUE;
		}
	}
	vertex_buffer->hardware_format = NULL;
	triangle_buffer->hardware_format = NULL;

	return TRUE;
}

/* the model's nodes are a tree (a marker instance's permutation is only
compared: one past its region's is never matched) */
static boolean model_check(
	struct tag_validation *validation,
	void *base)
{
	struct model *model = base;

	if (!node_tree_check(validation, model->nodes.address, model->nodes.count, sizeof(struct model_node),
		offsetof(struct model_node, next_sibling_node_index), offsetof(struct model_node, first_child_node_index),
		offsetof(struct model_node, parent_node_index)))
	{
		return FALSE;
	}

	return TRUE;
}

static struct tag_schema_field const model_marker_instance_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct model_marker_instance, region_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, regions), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct model_marker_instance, node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_marker_instance_schema =
	TAG_SCHEMA_DEFINITION(model_marker_instance, struct model_marker_instance, model_marker_instance_fields);

/* (found by name: model_find_marker) */
static struct tag_schema_field const model_marker_fields[] =
{
	TAG_SCHEMA_STRING(struct model_marker, name),
	TAG_SCHEMA_BLOCK(struct model_marker, instances, model_marker_instance_schema,
		MAXIMUM_INSTANCES_PER_MODEL_MARKER),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_marker_schema =
	TAG_SCHEMA_DEFINITION(model_marker, struct model_marker, model_marker_fields);

/* (found by name: model_find_node; the links are checked as a tree by
model_check) */
static struct tag_schema_field const model_node_fields[] =
{
	TAG_SCHEMA_STRING(struct model_node, name),
	TAG_SCHEMA_BLOCK_INDEX(struct model_node, next_sibling_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct model_node, first_child_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct model_node, parent_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_node_schema =
	TAG_SCHEMA_DEFINITION(model_node, struct model_node, model_node_fields);

static struct tag_schema_field const model_region_permutation_marker_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_region_permutation_marker_schema =
	TAG_SCHEMA_DEFINITION(model_region_permutation_marker, struct model_region_permutation_marker,
		model_region_permutation_marker_fields);

static struct tag_schema_field const model_region_permutation_fields[] =
{
	TAG_SCHEMA_STRING(struct model_region_permutation, name),
	TAG_SCHEMA_BLOCK_INDEX_ARRAY(struct model_region_permutation, geometry_indices, TAG_SCHEMA_ROOT,
		offsetof(struct model, geometries), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK(struct model_region_permutation, markers, model_region_permutation_marker_schema,
		MAXIMUM_MARKERS_PER_MODEL_REGION_PERMUTATION),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_region_permutation_schema =
	TAG_SCHEMA_DEFINITION(model_region_permutation, struct model_region_permutation, model_region_permutation_fields);

static struct tag_schema_field const model_region_fields[] =
{
	TAG_SCHEMA_STRING(struct model_region, name),
	TAG_SCHEMA_BLOCK(struct model_region, permutations, model_region_permutation_schema,
		MAXIMUM_PERMUTATIONS_PER_MODEL_REGION),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_region_schema =
	TAG_SCHEMA_DEFINITION(model_region, struct model_region, model_region_fields);

static struct tag_schema_field const model_vertex_uncompressed_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_vertex_uncompressed_schema =
	TAG_SCHEMA_DEFINITION(model_vertex_uncompressed, struct model_vertex_uncompressed_element,
		model_vertex_uncompressed_fields);

static struct tag_schema_field const model_vertex_compressed_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_vertex_compressed_schema =
	TAG_SCHEMA_DEFINITION(model_vertex_compressed, struct model_vertex_compressed_element,
		model_vertex_compressed_fields);

static struct tag_schema_field const model_triangle_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_triangle_schema =
	TAG_SCHEMA_DEFINITION(model_triangle, struct model_triangle_element, model_triangle_fields);

/* (the part's vertex and triangle blocks are the tags' copy; the game draws
its buffers, model_geometry_part_check) */
static struct tag_schema_field const model_geometry_part_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct model_geometry_part, shader_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, shaders), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct model_geometry_part, previous_part_index, 1,
		offsetof(struct model_geometry, parts), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct model_geometry_part, next_part_index, 1,
		offsetof(struct model_geometry, parts), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct model_geometry_part, centroid_primary_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct model_geometry_part, centroid_secondary_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), 0),
	TAG_SCHEMA_BLOCK(struct model_geometry_part, uncompressed_vertices, model_vertex_uncompressed_schema,
		MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_BLOCK(struct model_geometry_part, compressed_vertices, model_vertex_compressed_schema,
		MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_BLOCK(struct model_geometry_part, triangles, model_triangle_schema,
		MAXIMUM_TRIANGLES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_ENUM(struct model_geometry_part, triangle_buffer.type, NUMBER_OF_TRIANGLE_BUFFER_TYPES, 0),
	TAG_SCHEMA_ENUM(struct model_geometry_part, vertex_buffer.type, NUMBER_OF_RASTERIZER_VERTEX_TYPES, 0),
	TAG_SCHEMA_CHECK(model_geometry_part_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_geometry_part_schema =
	TAG_SCHEMA_DEFINITION(model_geometry_part, struct model_geometry_part, model_geometry_part_fields);

static struct tag_schema_field const model_geometry_fields[] =
{
	TAG_SCHEMA_BLOCK(struct model_geometry, parts, model_geometry_part_schema, MAXIMUM_PARTS_PER_MODEL_GEOMETRY),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_geometry_schema =
	TAG_SCHEMA_DEFINITION(model_geometry, struct model_geometry, model_geometry_fields);

static struct tag_schema_field const model_shader_reference_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct model_shader_reference, shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const model_shader_reference_schema =
	TAG_SCHEMA_DEFINITION(model_shader_reference, struct model_shader_reference, model_shader_reference_fields);

/* (the maxima: nodes, the game's node arrays and queues (models.c);
parts, render_model_parts' sort array; the rest the tool's,
model_definitions.h. An object keeps the permutations of no more than
MAXIMUM_REGIONS_PER_OBJECT regions, and objects.c and models.c go no
further than those) */
static struct tag_schema_field const model_fields[] =
{
	TAG_SCHEMA_BLOCK(struct model, markers, model_marker_schema, MAXIMUM_MARKERS_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, nodes, model_node_schema, MAXIMUM_NODES_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, regions, model_region_schema, MAXIMUM_REGIONS_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, geometries, model_geometry_schema, MAXIMUM_GEOMETRIES_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, shaders, model_shader_reference_schema, MAXIMUM_SHADERS_PER_MODEL),
	TAG_SCHEMA_CHECK(model_check),
	TAG_SCHEMA_END
};

/* Custom Edition's gbxmodels ('mod2': cache_file_formats.c), which the game
takes as models once port/linux/game/custom_edition_geometry.c has made
their parts this build's: a model, but for its parts. Where this build's
part has its buffers, a gbxmodel part has where its strip and vertices are
in the map's model data (the loader checked that they lie in it), and after
them the model's nodes its vertices name by their place in its table, when
the model's parts have local nodes. */

struct gbxmodel_geometry_part
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
	short strip_type;
	word pad1;
	long strip_triangle_count;
	unsigned long strip_offset;
	unsigned long unused1;
	short vertex_type;
	word pad2;
	long vertex_count;
	unsigned long unused2[2];
	unsigned long vertex_offset;
	byte pad3[3];
	byte local_node_count;
	byte local_node_indices[MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART];
	word pad4;
};

typedef char verify_gbxmodel_geometry_part_size[sizeof(struct gbxmodel_geometry_part) == 0x84 ? 1 : -1];

/* its local nodes are the model's (the game's skinning names them through
the table, custom_edition_geometry.c) */
static boolean gbxmodel_geometry_part_check(
	struct tag_validation *validation,
	void *base)
{
	struct gbxmodel_geometry_part *part = base;
	struct model const *model = (struct model const *)tag_validate_root(validation);
	short node_index;

	if (part->local_node_count > MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART)
	{
		tag_validate_correct(validation, "has %d local nodes, more than %d: %d", part->local_node_count,
			MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART, MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART);
		part->local_node_count = MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART;
	}
	for (node_index = 0; node_index < part->local_node_count; node_index++)
	{
		if (part->local_node_indices[node_index] >= model->nodes.count)
		{
			tag_validate_correct(validation, "has local node %d naming node %d of its model's %ld: 0", node_index,
				part->local_node_indices[node_index], model->nodes.count);
			part->local_node_indices[node_index] = 0;
		}
	}

	return TRUE;
}

static struct tag_schema_field const gbxmodel_geometry_part_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct gbxmodel_geometry_part, shader_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, shaders), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct gbxmodel_geometry_part, previous_part_index, 1,
		offsetof(struct model_geometry, parts), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct gbxmodel_geometry_part, next_part_index, 1,
		offsetof(struct model_geometry, parts), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct gbxmodel_geometry_part, centroid_primary_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), 0),
	TAG_SCHEMA_BLOCK_INDEX(struct gbxmodel_geometry_part, centroid_secondary_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct model, nodes), 0),
	TAG_SCHEMA_BLOCK(struct gbxmodel_geometry_part, uncompressed_vertices, model_vertex_uncompressed_schema,
		MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_BLOCK(struct gbxmodel_geometry_part, compressed_vertices, model_vertex_compressed_schema,
		MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_BLOCK(struct gbxmodel_geometry_part, triangles, model_triangle_schema,
		MAXIMUM_TRIANGLES_PER_MODEL_GEOMETRY_PART),
	TAG_SCHEMA_CHECK(gbxmodel_geometry_part_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const gbxmodel_geometry_part_schema =
	TAG_SCHEMA_DEFINITION(gbxmodel_geometry_part, struct gbxmodel_geometry_part, gbxmodel_geometry_part_fields);

static struct tag_schema_field const gbxmodel_geometry_fields[] =
{
	TAG_SCHEMA_BLOCK(struct model_geometry, parts, gbxmodel_geometry_part_schema, MAXIMUM_PARTS_PER_MODEL_GEOMETRY),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const gbxmodel_geometry_schema =
	TAG_SCHEMA_DEFINITION(gbxmodel_geometry, struct model_geometry, gbxmodel_geometry_fields);

static struct tag_schema_field const gbxmodel_fields[] =
{
	TAG_SCHEMA_BLOCK(struct model, markers, model_marker_schema, MAXIMUM_MARKERS_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, nodes, model_node_schema, MAXIMUM_NODES_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, regions, model_region_schema, MAXIMUM_REGIONS_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, geometries, gbxmodel_geometry_schema, MAXIMUM_GEOMETRIES_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct model, shaders, model_shader_reference_schema, MAXIMUM_SHADERS_PER_MODEL),
	TAG_SCHEMA_CHECK(model_check),
	TAG_SCHEMA_END
};

/* animations */

/* how many of the first node_count nodes have their flag set
(model_animations.c's animation_node_flag_count) */
static long animation_node_flag_count(
	unsigned long const *flags,
	short node_count)
{
	long count = 0;
	short node_index;

	for (node_index = 0; node_index < node_count; node_index++)
	{
		if (BIT_VECTOR_TEST_FLAG(flags, node_index))
			count++;
	}

	return count;
}

/* whether element_count elements of element_size bytes, from
first_element_index on, relative_offset bytes into the size bytes there
are, are inside them (model_animations.c's animation_data_contains) */
static boolean animation_data_contains(
	long size,
	long relative_offset,
	long first_element_index,
	long element_count,
	long element_size)
{
	if (relative_offset < 0 || relative_offset > size)
		return FALSE;
	size -= relative_offset;
	if (first_element_index < 0 || element_count < 0 || first_element_index > SHORT_MAX ||
		element_count > UNSIGNED_SHORT_MAX)
	{
		return FALSE;
	}

	return first_element_index + element_count <= size / element_size;
}

/* a compressed animation's nodes of one kind (rotation, translation,
scale): node_count headers at headers_offset, each a first keyframe and a
count, whose keyframes (of keyframe_size bytes) and frame indices are in
the data, the frame indices in order and the last the animation's last
frame (model_animations.c's animation_get_keyframe_rotation and the rest) */
static char const *compressed_animation_keyframes_problem(
	struct animation const *animation,
	byte const *data,
	long size,
	long headers_offset,
	long node_count,
	long keyframes_offset,
	long frame_indices_offset,
	long keyframe_size)
{
	long node_index;

	if (!animation_data_contains(size, headers_offset, 0, node_count, sizeof(unsigned long)))
		return "compressed node headers";
	for (node_index = 0; node_index < node_count; node_index++)
	{
		unsigned long node_header = ((unsigned long const *)(data + headers_offset))[node_index];
		short first_keyframe_index = (short)(node_header >> COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS);
		short keyframe_count = (short)(node_header & (FLAG(COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS) - 1));
		word const *frame_indices;
		short keyframe_index;

		if (!keyframe_count)
			continue;
		if (!animation_data_contains(size, keyframes_offset, first_keyframe_index, keyframe_count, keyframe_size) ||
			!animation_data_contains(size, frame_indices_offset, first_keyframe_index, keyframe_count, sizeof(word)))
		{
			return "compressed keyframes";
		}
		frame_indices = (word const *)(data + frame_indices_offset) + first_keyframe_index;
		for (keyframe_index = 1; keyframe_index < keyframe_count; keyframe_index++)
		{
			if (frame_indices[keyframe_index] <= frame_indices[keyframe_index - 1])
				return "compressed keyframes' order";
		}
		if (frame_indices[keyframe_count - 1] != animation->frame_count - 1)
			return "compressed keyframes' last frame";
	}

	return NULL;
}

/* what is wrong with the frames of an animation (none: NULL): every frame
(model_animations.c's animation_frame_valid), and the defaults a base
animation reads */
static char const *animation_data_problem(
	struct animation const *animation)
{
	short node_count = animation->node_count;
	long rotation_count;
	long translation_count;
	long scale_count;

	if (node_count < 0 || node_count > MAXIMUM_NODES_PER_ANIMATION)
		return "node count";
	rotation_count = animation_node_flag_count(animation->nodes_with_rotation_flags, node_count);
	translation_count = animation_node_flag_count(animation->nodes_with_translation_flags, node_count);
	scale_count = animation_node_flag_count((unsigned long const *)animation->nodes_with_scale_flags, node_count);

	if (TEST_FLAG(animation->flags, _animation_compressed_bit))
	{
		struct compressed_animation_header const *header;
		byte const *data;
		long size;
		char const *problem;

		if (animation->compressed_data_offset < 0 || animation->compressed_data_offset > animation->data.size ||
			animation->data.size - animation->compressed_data_offset < COMPRESSED_ANIMATION_HEADER_SIZE)
		{
			return "compressed data offset";
		}
		data = (byte const *)animation->data.address + animation->compressed_data_offset;
		size = animation->data.size - animation->compressed_data_offset;
		header = (struct compressed_animation_header const *)data;
		if (!animation_data_contains(size, header->default_rotations_offset, 0, node_count, 6) ||
			!animation_data_contains(size, header->default_translations_offset, 0, node_count, sizeof(real_point3d)) ||
			!animation_data_contains(size, header->default_scales_offset, 0, scale_count, sizeof(real)))
		{
			return "compressed defaults";
		}
		problem = compressed_animation_keyframes_problem(animation, data, size, COMPRESSED_ANIMATION_HEADER_SIZE,
			rotation_count, header->rotation_keyframes_offset, header->rotation_keyframe_frame_indices_offset, 6);
		if (!problem)
		{
			problem = compressed_animation_keyframes_problem(animation, data, size,
				header->translation_node_headers_offset, translation_count, header->translation_keyframes_offset,
				header->translation_keyframe_frame_indices_offset, sizeof(real_point3d));
		}
		if (!problem)
		{
			problem = compressed_animation_keyframes_problem(animation, data, size, header->scale_node_headers_offset,
				scale_count, header->scale_keyframes_offset, header->scale_keyframe_frame_indices_offset, sizeof(real));
		}

		return problem;
	}
	else
	{
		long frame_size = rotation_count * 8 + translation_count * sizeof(real_point3d) + scale_count * sizeof(real);
		long default_size = (node_count - rotation_count) * 8 + (node_count - translation_count) * sizeof(real_point3d) +
			(node_count - scale_count) * sizeof(real);

		if (animation->frame_size != frame_size)
			return "frame size";
		if (frame_size && animation->frame_count > animation->data.size / frame_size)
			return "frame data";
		if (animation->type == _animation_base && default_size > animation->default_data.size)
			return "default data";
	}

	return NULL;
}

/* an animation's frame count, frames and frame info fit what it has: an
animation whose frames do not moves no node (none of its nodes, no
flags) */
static boolean animation_check(
	struct tag_validation *validation,
	void *base)
{
	struct animation *animation = base;
	char const *problem;
	long frame_info_size;

	if (animation->frame_count < 1)
	{
		tag_validate_correct(validation, "has animation '%.31s''s %d frames: 1", animation->name,
			animation->frame_count);
		animation->frame_count = 1;
	}
	problem = animation_data_problem(animation);
	if (problem)
	{
		tag_validate_correct(validation, "has animation '%.31s''s bad %s: no nodes", animation->name, problem);
		animation->node_count = 0;
		animation->frame_size = 0;
		animation->flags &= ~FLAG(_animation_compressed_bit);
		memset(animation->nodes_with_translation_flags, 0, sizeof(animation->nodes_with_translation_flags));
		memset(animation->nodes_with_rotation_flags, 0, sizeof(animation->nodes_with_rotation_flags));
		memset(animation->nodes_with_scale_flags, 0, sizeof(animation->nodes_with_scale_flags));
	}

	switch (animation->frame_info_type)
	{
	case 1: frame_info_size = 8; break;
	case 2: frame_info_size = 12; break;
	case 3: frame_info_size = 16; break;
	default: frame_info_size = 0; break;
	}
	if (frame_info_size && animation->frame_count > animation->frame_info.size / frame_info_size)
	{
		tag_validate_correct(validation, "has animation '%.31s''s frame info of %ld bytes for %d frames: none",
			animation->name, animation->frame_info.size, animation->frame_count);
		animation->frame_info_type = 0;
	}

	return TRUE;
}

/* the animation graph's nodes are a tree; each animation's permutation list
(its next animations) ends */
static boolean animation_graph_check(
	struct tag_validation *validation,
	void *base)
{
	struct animation_graph *graph = base;
	struct animation *animations = graph->animations.address;
	/* (0 not walked, 1 on the list being walked, 2 walked; as many as a
	short indexes, a Custom Edition map's tools' most) */
	static byte states[SHORT_MAX];
	long maximum_count = tag_validate_custom_edition(validation) ? SHORT_MAX : MAXIMUM_ANIMATIONS_PER_GRAPH;
	long first_index;

	if (!node_tree_check(validation, graph->nodes.address, graph->nodes.count, sizeof(struct animation_graph_node),
		offsetof(struct animation_graph_node, next_sibling_node_index),
		offsetof(struct animation_graph_node, first_child_node_index),
		offsetof(struct animation_graph_node, parent_node_index)))
	{
		return FALSE;
	}

	if (graph->animations.count > maximum_count)
	{
		tag_validate_refuse(validation, "has %ld animations", graph->animations.count);
		return FALSE;
	}
	memset(states, 0, (size_t)graph->animations.count);
	for (first_index = 0; first_index < graph->animations.count; first_index++)
	{
		short animation_index = (short)first_index;
		short last_index = NONE;

		/* (along the list until its end, or an animation walked before:
		one on this list is a loop, cut there) */
		while (VALID_INDEX(animation_index, graph->animations.count) && !states[animation_index])
		{
			states[animation_index] = 1;
			last_index = animation_index;
			animation_index = animations[animation_index].next_animation_index;
		}
		if (VALID_INDEX(animation_index, graph->animations.count) && states[animation_index] == 1)
		{
			tag_validate_correct(validation, "has animation '%.31s''s next animation %d loop: none",
				animations[last_index].name, animation_index);
			animations[last_index].next_animation_index = NONE;
		}
		for (animation_index = (short)first_index;
			VALID_INDEX(animation_index, graph->animations.count) && states[animation_index] == 1;
			animation_index = animations[animation_index].next_animation_index)
		{
			states[animation_index] = 2;
		}
	}

	return TRUE;
}

/* (an index into the graph's animations) */
static struct tag_schema_field const animation_index_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct animation_graph_animation_index, animation_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, animations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_index_schema =
	TAG_SCHEMA_DEFINITION(animation_index, struct animation_graph_animation_index, animation_index_fields);

static struct tag_schema_field const animation_graph_object_overlay_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct animation_graph_object_overlay, animation_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, animations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct animation_graph_object_overlay, function_index, NUMBER_OF_OBJECT_OVERLAY_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct animation_graph_object_overlay, mode, NUMBER_OF_OBJECT_OVERLAY_MODES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_object_overlay_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_object_overlay, struct animation_graph_object_overlay,
		animation_graph_object_overlay_fields);

/* (marker names, found by name: units.c) */
static struct tag_schema_field const animation_graph_ik_point_fields[] =
{
	TAG_SCHEMA_STRING(struct animation_graph_ik_point, marker_name),
	TAG_SCHEMA_STRING(struct animation_graph_ik_point, attached_to_marker_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_ik_point_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_ik_point, struct animation_graph_ik_point, animation_graph_ik_point_fields);

static struct tag_schema_field const animation_graph_weapon_type_fields[] =
{
	TAG_SCHEMA_STRING(struct animation_graph_weapon_type, label),
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_type, animations, animation_index_schema,
		NUMBER_OF_WEAPON_TYPE_ANIMATIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_weapon_type_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_weapon_type, struct animation_graph_weapon_type,
		animation_graph_weapon_type_fields);

static struct tag_schema_field const animation_graph_weapon_class_fields[] =
{
	TAG_SCHEMA_STRING(struct animation_graph_weapon_class, label),
	TAG_SCHEMA_STRING(struct animation_graph_weapon_class, grip_marker_name),
	TAG_SCHEMA_STRING(struct animation_graph_weapon_class, hand_marker_name),
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_class, animations, animation_index_schema,
		NUMBER_OF_UNIT_WEAPON_CLASS_ANIMATIONS),
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_class, ik_points, animation_graph_ik_point_schema,
		MAXIMUM_IK_POINTS),
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_class, weapon_types, animation_graph_weapon_type_schema,
		MAXIMUM_WEAPON_TYPES_PER_WEAPON_CLASS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_weapon_class_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_weapon_class, struct animation_graph_weapon_class,
		animation_graph_weapon_class_fields);

static struct tag_schema_field const animation_graph_unit_seat_fields[] =
{
	TAG_SCHEMA_STRING(struct animation_graph_unit_seat, label),
	TAG_SCHEMA_BLOCK(struct animation_graph_unit_seat, animations, animation_index_schema,
		NUMBER_OF_UNIT_SEAT_ANIMATIONS),
	TAG_SCHEMA_BLOCK(struct animation_graph_unit_seat, ik_points, animation_graph_ik_point_schema, MAXIMUM_IK_POINTS),
	TAG_SCHEMA_BLOCK(struct animation_graph_unit_seat, weapon_classes, animation_graph_weapon_class_schema,
		MAXIMUM_WEAPON_CLASSES_PER_UNIT_SEAT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_unit_seat_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_unit_seat, struct animation_graph_unit_seat, animation_graph_unit_seat_fields);

static struct tag_schema_field const animation_graph_weapon_animations_fields[] =
{
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_animations, animations, animation_index_schema,
		NUMBER_OF_WEAPON_ANIMATIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_weapon_animations_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_weapon_animations, struct animation_graph_weapon_animations,
		animation_graph_weapon_animations_fields);

/* (vehicles.c checks the mass point against the vehicle's physics) */
static struct tag_schema_field const vehicle_suspension_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct vehicle_suspension, animation_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, animations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const vehicle_suspension_schema =
	TAG_SCHEMA_DEFINITION(vehicle_suspension, struct vehicle_suspension, vehicle_suspension_fields);

static struct tag_schema_field const vehicle_animation_fields[] =
{
	TAG_SCHEMA_BLOCK(struct vehicle_animation, animations, animation_index_schema, NUMBER_OF_VEHICLE_ANIMATIONS),
	TAG_SCHEMA_BLOCK(struct vehicle_animation, suspensions, vehicle_suspension_schema,
		MAXIMUM_SUSPENSIONS_PER_VEHICLE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const vehicle_animation_schema =
	TAG_SCHEMA_DEFINITION(vehicle_animation, struct vehicle_animation, vehicle_animation_fields);

static struct tag_schema_field const animation_graph_device_animations_fields[] =
{
	TAG_SCHEMA_BLOCK(struct animation_graph_device_animations, animations, animation_index_schema,
		NUMBER_OF_DEVICE_ANIMATIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_device_animations_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_device_animations, struct animation_graph_device_animations,
		animation_graph_device_animations_fields);

static struct tag_schema_field const animation_graph_first_person_weapon_animations_fields[] =
{
	TAG_SCHEMA_BLOCK(struct animation_graph_weapon_animations, animations, animation_index_schema,
		NUMBER_OF_FIRST_PERSON_WEAPON_ANIMATIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_first_person_weapon_animations_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_first_person_weapon_animations, struct animation_graph_weapon_animations,
		animation_graph_first_person_weapon_animations_fields);

static struct tag_schema_field const animation_graph_sound_reference_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct animation_graph_sound_reference, sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_sound_reference_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_sound_reference, struct animation_graph_sound_reference,
		animation_graph_sound_reference_fields);

/* (found by name: first_person_weapons.c; the links are checked as a tree
by animation_graph_check) */
static struct tag_schema_field const animation_graph_node_fields[] =
{
	TAG_SCHEMA_STRING(struct animation_graph_node, name),
	TAG_SCHEMA_BLOCK_INDEX(struct animation_graph_node, next_sibling_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct animation_graph_node, first_child_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct animation_graph_node, parent_node_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, nodes), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_graph_node_schema =
	TAG_SCHEMA_DEFINITION(animation_graph_node, struct animation_graph_node, animation_graph_node_fields);

/* (the runtime parent and weight are the tool's, which the game does not
set: model_animations.c's animation_choose_random_permutation_internal) */
static struct tag_schema_field const animation_fields[] =
{
	TAG_SCHEMA_STRING(struct animation, name),
	TAG_SCHEMA_ENUM(struct animation, type, NUMBER_OF_ANIMATION_TYPES, 0),
	TAG_SCHEMA_ENUM(struct animation, frame_info_type, NUMBER_OF_ANIMATION_FRAME_INFO_TYPES, 0),
	TAG_SCHEMA_BLOCK_INDEX(struct animation, next_animation_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, animations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct animation, sound_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, sound_references), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct animation, runtime_parent_animation_index, TAG_SCHEMA_ROOT,
		offsetof(struct animation_graph, animations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_DATA(struct animation, frame_info, MAXIMUM_ANIMATION_FRAME_INFO_SIZE),
	TAG_SCHEMA_DATA(struct animation, default_data, MAXIMUM_ANIMATION_DEFAULT_DATA_SIZE),
	TAG_SCHEMA_DATA(struct animation, data, MAXIMUM_ANIMATION_DATA_SIZE),
	TAG_SCHEMA_CHECK(animation_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const animation_schema =
	TAG_SCHEMA_DEFINITION(animation, struct animation, animation_fields);

/* (the maxima: nodes, the game's node arrays (MAXIMUM_NODES_PER_ANIMATION);
the animation index lists, the game's animation lists (units.h, weapons.h,
model_animation_definitions.c), which it indexes them by; the weapon,
vehicle, device and first person blocks, the one element the game reads;
a vehicle's suspensions, its suspension states (vehicle_datum.h); the rest
the tool's) */
static struct tag_schema_field const animation_graph_fields[] =
{
	TAG_SCHEMA_BLOCK(struct animation_graph, object_overlays, animation_graph_object_overlay_schema,
		MAXIMUM_OBJECT_OVERLAYS_PER_GRAPH),
	TAG_SCHEMA_TOOL_BLOCK(struct animation_graph, unit_seats, animation_graph_unit_seat_schema,
		MAXIMUM_UNIT_SEATS_PER_GRAPH),
	TAG_SCHEMA_BLOCK(struct animation_graph, weapon_animations, animation_graph_weapon_animations_schema, 1),
	TAG_SCHEMA_BLOCK(struct animation_graph, vehicle_animations, vehicle_animation_schema, 1),
	TAG_SCHEMA_BLOCK(struct animation_graph, device_animations, animation_graph_device_animations_schema, 1),
	TAG_SCHEMA_BLOCK(struct animation_graph, unit_damage_animations, animation_index_schema,
		NUMBER_OF_UNIT_DAMAGE_ANIMATIONS_PER_GRAPH),
	TAG_SCHEMA_BLOCK(struct animation_graph, first_person_weapon_animations,
		animation_graph_first_person_weapon_animations_schema, 1),
	TAG_SCHEMA_BLOCK(struct animation_graph, sound_references, animation_graph_sound_reference_schema,
		MAXIMUM_SOUND_REFERENCES_PER_ANIMATION_GRAPH),
	TAG_SCHEMA_BLOCK(struct animation_graph, nodes, animation_graph_node_schema, MAXIMUM_NODES_PER_ANIMATION),
	TAG_SCHEMA_TOOL_BLOCK(struct animation_graph, animations, animation_schema, MAXIMUM_ANIMATIONS_PER_GRAPH),
	TAG_SCHEMA_CHECK(animation_graph_check),
	TAG_SCHEMA_END
};

/* the groups' roots */

static struct tag_schema_definition const model_schema =
	TAG_SCHEMA_DEFINITION(model, struct model, model_fields);
static struct tag_schema_definition const gbxmodel_schema =
	TAG_SCHEMA_DEFINITION(gbxmodel, struct model, gbxmodel_fields);
static struct tag_schema_definition const animation_graph_schema =
	TAG_SCHEMA_DEFINITION(animation_graph, struct animation_graph, animation_graph_fields);

struct tag_schema_group const tag_schema_model_groups[] =
{
	{ 'mode', { NONE, NONE }, &model_schema },
	{ 'antr', { NONE, NONE }, &animation_graph_schema },
	{ 0 }
};

struct tag_schema_group const tag_schema_custom_edition_groups[] =
{
	{ 'mod2', { NONE, NONE }, &gbxmodel_schema },
	{ 0 }
};
