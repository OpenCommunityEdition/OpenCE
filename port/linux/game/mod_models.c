/*
MOD_MODELS.C

The models of mods (mods.c): each of a map's model tags that a mod has a
tag file of (tags\...\name.gbxmodel, Halo PC's, or tags\...\name.model, the
Xbox's as invader-extract writes it) is drawn from that file instead of the
map, and mods_reload reads them again.

A model is more than what is drawn: its nodes are what its animations move,
its markers are where projectiles leave a weapon, where a unit sits and
where things attach, and its regions and permutations are what damage and
an object's variant choose among. Those are what the machines of a network
game agree on, so a mod's model must have the map's: the same nodes (names,
hierarchy, default positions), the same markers (names, regions,
permutations, nodes, positions and rotations, whether the file keeps them
on its permutations, as the Halo Editing Kit does, or on the model, as a
map does) and the same regions and permutations (names and flags). Only what
is drawn is the file's: its geometries and their parts, which geometry each
permutation draws at each detail level, the detail levels' cutoffs, the base
map's scale, and which of the map's shaders each part is drawn with (a mod
cannot add tags: every shader the file names must be one the map has). The
model played keeps the map's nodes, markers and regions; nothing a machine
simulates reads anything else of a model.

A tag file is big-endian, as the Halo Editing Kit's tool and Invader write
it: the model, then each block's elements followed by each element's own
blocks, in order (its markers and their instances, its nodes, its regions,
their permutations and their markers, its geometries, their parts and each
part's vertices, compressed vertices and strip, and its shaders and their
names). Everything is checked against the file before it is used, which must
end where its last shader's name does. A part's strip is the triangles
block's indices, three to an element, less the empty ones that end it, as
the tool builds a map's strips. What a map keeps that a tag file has not got
is made as the tool makes it: each part's centroid nodes and weights (its
two most weighted nodes). A part's vertices are compressed as a Custom
Edition map's are, by the game's own rasterizer_geometry_compress_vertices,
into the Xbox's contiguous memory, and given buffers by the game's own
rasterizer_vertex_buffer_new and rasterizer_triangle_buffer_new; a model of
more nodes than the renderer skins at once has each part drawn its own nodes
at a time (custom_edition_part_palette asks mod_models_part_palette).

The map's model tag takes the made model's definition in place, so every
object that holds the tag keeps holding it, and its own comes back when the
file goes. A model made is kept until the map goes, its buffers with it, as
a frame may still draw from it.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "cache/cache_files.h"
#include "main/console.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "models/models.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "tag_files/tag_groups.h"
#include "mods.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <xtl.h>

/* the game's (port) */
char *tag_get_name(long tag_index);
unsigned long tag_get_group_tag(long tag_index);

/* ---------- constants */

/* a model tag's file: tags\<its name>.gbxmodel (Halo PC's), else
tags\<its name>.model (the Xbox's) */
#define GBXMODEL_FILE_EXTENSION ".gbxmodel"
#define MODEL_FILE_EXTENSION ".model"
#define GBXMODEL_GROUP_TAG 'mod2'
#define GBXMODEL_TAG_VERSION 5

/* the sizes of a tag file's model, marker, marker instance, node, region,
permutation, permutation marker, geometry, part (the Xbox's and Halo PC's),
vertex, compressed vertex, strip element and shader reference */
#define MODEL_SIZE 0xE8
#define MARKER_SIZE 0x40
#define MARKER_INSTANCE_SIZE 0x20
#define NODE_SIZE 0x9C
#define REGION_SIZE 0x4C
#define PERMUTATION_SIZE 0x58
#define PERMUTATION_MARKER_SIZE 0x50
#define GEOMETRY_SIZE 0x30
#define PART_SIZE 0x68
#define GBXMODEL_PART_SIZE 0x84
#define VERTEX_SIZE 0x44
#define COMPRESSED_VERTEX_SIZE 0x20
#define STRIP_ELEMENT_SIZE 6
#define SHADER_REFERENCE_SIZE 0x20

/* where a tag file's model, marker, node, region, permutation, permutation
marker, geometry, part and shader reference keep what is read of them */
#define MODEL_FLAGS_OFFSET 0x00
#define MODEL_CUTOFFS_OFFSET 0x08
#define MODEL_BASE_MAP_SCALE_OFFSET 0x30
#define MODEL_MARKERS_OFFSET 0xAC
#define MODEL_NODES_OFFSET 0xB8
#define MODEL_REGIONS_OFFSET 0xC4
#define MODEL_GEOMETRIES_OFFSET 0xD0
#define MODEL_SHADERS_OFFSET 0xDC
#define MARKER_INSTANCES_OFFSET 0x34
#define NODE_HIERARCHY_OFFSET 0x20
#define NODE_TRANSLATION_OFFSET 0x28
#define NODE_ROTATION_OFFSET 0x34
#define NODE_DISTANCE_OFFSET 0x44
#define REGION_PERMUTATIONS_OFFSET 0x40
#define PERMUTATION_FLAGS_OFFSET 0x20
#define PERMUTATION_GEOMETRIES_OFFSET 0x40
#define PERMUTATION_MARKERS_OFFSET 0x4C
#define PERMUTATION_MARKER_NODE_OFFSET 0x20
#define PERMUTATION_MARKER_ROTATION_OFFSET 0x24
#define PERMUTATION_MARKER_TRANSLATION_OFFSET 0x34
#define GEOMETRY_PARTS_OFFSET 0x24
#define PART_SHADER_OFFSET 0x04
#define PART_PREVIOUS_OFFSET 0x06
#define PART_NEXT_OFFSET 0x07
#define PART_CENTROID_OFFSET 0x14
#define PART_VERTICES_OFFSET 0x20
#define PART_COMPRESSED_VERTICES_OFFSET 0x2C
#define PART_TRIANGLES_OFFSET 0x38
#define GBXMODEL_PART_LOCAL_NODE_COUNT_OFFSET 0x6B
#define GBXMODEL_PART_LOCAL_NODES_OFFSET 0x6C
#define SHADER_REFERENCE_NAME_LENGTH_OFFSET 0x08
#define SHADER_REFERENCE_PERMUTATION_OFFSET 0x10

/* a tag file's model's flags: its vertices' normals blended where they
share a position (the tool does it as it builds a map), and its parts'
vertices name nodes through their parts' tables (Halo PC's: as
custom_edition_geometry.c's _gbxmodel_parts_have_local_nodes_bit) */
#define MODEL_BLEND_SHARED_NORMALS_BIT 0
#define MODEL_PARTS_HAVE_LOCAL_NODES_BIT 1
/* a part's flags: its vertices name local nodes (custom_edition_geometry.c) */
#define PART_LOCAL_NODES_BIT 1
/* a permutation's flags the machines agree on */
#define PERMUTATION_FLAGS_MASK FLAG(_model_region_permutation_cannot_be_chosen_randomly_bit)

/* an empty strip index (the tool's padding at a strip's end), and an empty
part link */
#define NULL_INDEX 0xFFFF
#define NULL_LINK 0xFF

/* how far a node's or marker's position or rotation may be from the map's
(a model exported again is not always bit for bit the map's) */
#define MAXIMUM_DIFFERENCE 0.001f

/* rasterizer_geometry_compress_vertices clamps normals and the like to
[-1, 1] and asserts the packed vector is within 0.01 of the original
(custom_edition_geometry.c's limit) */
#define MAXIMUM_COMPRESSIBLE_COMPONENT 1.005f

/* the most marker instances compared: a model's markers' (the most
markers of the most instances), and as many of its permutations' */
#define MAXIMUM_MARKER_INSTANCES (2 * MAXIMUM_MARKERS_PER_MODEL * MAXIMUM_INSTANCES_PER_MODEL_MARKER)

/* the most of the Xbox's contiguous memory the mods' models take (what is
left is the game's: its own maps' geometry, and on Android and the web a
window of it in all); past it, their geometry is drawn from the heap */
#if defined(HALO_ANDROID) || defined(HALO_WEB)
#define MAXIMUM_CONTIGUOUS_GEOMETRY_BYTES (24UL * 1024 * 1024)
#else
#define MAXIMUM_CONTIGUOUS_GEOMETRY_BYTES (128UL * 1024 * 1024)
#endif

/* ---------- structures */

/* this build's model geometry, part and shader reference, as models.c and
rasterizer.c each define them for themselves */
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

struct model_shader_reference
{
	struct tag_reference shader;
	short permutation_index;
	word pad;
	long unused[3];
};

typedef char verify_model_size[sizeof(struct model) == MODEL_SIZE ? 1 : -1];
typedef char verify_model_node_size[sizeof(struct model_node) == NODE_SIZE ? 1 : -1];
typedef char verify_model_region_size[sizeof(struct model_region) == REGION_SIZE ? 1 : -1];
typedef char verify_model_region_permutation_size[
	sizeof(struct model_region_permutation) == PERMUTATION_SIZE ? 1 : -1];
typedef char verify_model_marker_size[sizeof(struct model_marker) == MARKER_SIZE ? 1 : -1];
typedef char verify_model_marker_instance_size[sizeof(struct model_marker_instance) == MARKER_INSTANCE_SIZE ? 1 : -1];
typedef char verify_model_geometry_size[sizeof(struct model_geometry) == GEOMETRY_SIZE ? 1 : -1];
typedef char verify_model_geometry_part_size[sizeof(struct model_geometry_part) == PART_SIZE ? 1 : -1];
typedef char verify_model_shader_reference_size[
	sizeof(struct model_shader_reference) == SHADER_REFERENCE_SIZE ? 1 : -1];
typedef char verify_model_vertex_size[sizeof(struct model_vertex_uncompressed) == VERTEX_SIZE ? 1 : -1];

/* a marker's instance, of a file or of the map: what is compared */
struct marker_instance
{
	char name[TAG_STRING_LENGTH + 1];
	short region_index;
	short permutation_index;
	short node_index;
	real_point3d translation;
	real_quaternion rotation;
	boolean matched;
};

/* where a file's part is, and its vertices and strip (offsets in the file) */
struct file_part
{
	unsigned long offset;
	unsigned long vertices;
	long vertex_count;
	unsigned long strip;
	long strip_element_count;
	/* the indices its strip has, less the empty ones that end it */
	long strip_index_count;
};

/* what a file has, and where (offsets in the file), as it is walked */
struct model_file
{
	byte const *bytes;
	unsigned long size;
	boolean gbxmodel;
	unsigned long part_size;
	unsigned long model_flags;
	unsigned long nodes;
	long node_count;
	unsigned long regions;
	long region_count;
	/* each region's permutations, and each permutation's markers */
	unsigned long permutations[MAXIMUM_REGIONS_PER_MODEL];
	long permutation_counts[MAXIMUM_REGIONS_PER_MODEL];
	unsigned long permutation_markers[MAXIMUM_REGIONS_PER_MODEL][MAXIMUM_PERMUTATIONS_PER_MODEL_REGION];
	long permutation_marker_counts[MAXIMUM_REGIONS_PER_MODEL][MAXIMUM_PERMUTATIONS_PER_MODEL_REGION];
	/* the model's markers (a map's way: the tool's tag files have none),
	and each one's instances */
	unsigned long markers;
	long marker_count;
	unsigned long marker_instances[MAXIMUM_MARKERS_PER_MODEL];
	long marker_instance_counts[MAXIMUM_MARKERS_PER_MODEL];
	long geometry_count;
	long geometry_part_counts[MAXIMUM_GEOMETRIES_PER_MODEL];
	struct file_part *parts;
	long part_count;
	unsigned long shaders;
	long shader_count;
	unsigned long shader_names[MAXIMUM_SHADERS_PER_MODEL];
};

/* the nodes a part of a model of many nodes is drawn with, by its vertex
buffer (custom_edition_geometry.c's part_palette) */
struct mod_model_palette
{
	struct vertex_buffer const *vertex_buffer;
	byte node_count;
	byte nodes[MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART];
};

/* a model tag file read, and what it makes */
struct mod_model
{
	struct mod_model *next;
	long tag_index;
	/* the file (its index in the mods' files of the last time they were
	found), and its path, size and modification time, to tell whether it
	changed */
	long file;
	char *path;
	unsigned long file_size;
	unsigned long stamp[2];
	/* the definition it makes, and the blocks it has of its own */
	struct model model;
	struct model_region *regions;
	struct model_region_permutation *permutations;
	struct model_geometry *geometries;
	struct model_geometry_part *parts;
	long part_count;
	struct model_shader_reference *shaders;
	struct mod_model_palette *palettes;
	long palette_count;
	/* the compressed vertices and strips the parts' buffers are made from,
	their size, and whether they are in contiguous memory */
	byte *geometry;
	unsigned long geometry_size;
	boolean geometry_contiguous;
	unsigned long kept_bytes;
};

/* a model of the map drawn from a file: the map's own definition, given
back when it goes */
struct mod_model_override
{
	struct mod_model *model;
	struct model map_model;
};

static struct
{
	/* by the map's tag, its override (none: the map's own) */
	struct mod_model_override *overrides;
	long override_count;
	/* every model made while the map is loaded, and the contiguous memory
	their geometry takes */
	struct mod_model *models;
	unsigned long contiguous_bytes;
} mod_models_globals;

/* ---------- private code */

static boolean close_enough(
	real first,
	real second)
{
	return fabs(first - second) <= MAXIMUM_DIFFERENCE;
}

static boolean points_close(
	real_point3d const *first,
	real_point3d const *second)
{
	return close_enough(first->x, second->x) && close_enough(first->y, second->y) &&
		close_enough(first->z, second->z);
}

/* (a rotation and its negation are the same) */
static boolean rotations_close(
	real_quaternion const *first,
	real_quaternion const *second)
{
	return (close_enough(first->v.i, second->v.i) && close_enough(first->v.j, second->v.j) &&
			close_enough(first->v.k, second->v.k) && close_enough(first->w, second->w)) ||
		(close_enough(first->v.i, -second->v.i) && close_enough(first->v.j, -second->v.j) &&
			close_enough(first->v.k, -second->v.k) && close_enough(first->w, -second->w));
}

static boolean vector_compressible(
	real_vector3d const *vector)
{
	/* a NaN fails every comparison */
	return fabs(vector->i) <= MAXIMUM_COMPRESSIBLE_COMPONENT &&
		fabs(vector->j) <= MAXIMUM_COMPRESSIBLE_COMPONENT &&
		fabs(vector->k) <= MAXIMUM_COMPRESSIBLE_COMPONENT;
}

static void read_point(
	byte const *bytes,
	real_point3d *point)
{
	point->x = mods_read_real(bytes);
	point->y = mods_read_real(bytes + 4);
	point->z = mods_read_real(bytes + 8);
}

static void read_vector(
	byte const *bytes,
	real_vector3d *vector)
{
	vector->i = mods_read_real(bytes);
	vector->j = mods_read_real(bytes + 4);
	vector->k = mods_read_real(bytes + 8);
}

static void read_quaternion(
	byte const *bytes,
	real_quaternion *quaternion)
{
	read_vector(bytes, &quaternion->v);
	quaternion->w = mods_read_real(bytes + 12);
}

static boolean point_finite(
	real_point3d const *point)
{
	return mods_finite_real(point->x) && mods_finite_real(point->y) && mods_finite_real(point->z);
}

static boolean quaternion_finite(
	real_quaternion const *quaternion)
{
	return mods_finite_real(quaternion->v.i) && mods_finite_real(quaternion->v.j) &&
		mods_finite_real(quaternion->v.k) && mods_finite_real(quaternion->w);
}

/* a tag file's name (TAG_STRING_LENGTH characters at most), terminated */
static void read_name(
	byte const *bytes,
	char name[TAG_STRING_LENGTH + 1])
{
	memcpy(name, bytes, TAG_STRING_LENGTH);
	name[TAG_STRING_LENGTH] = 0;
}

/* `count` elements of `size` bytes at *cursor, which passes them: FALSE
when they are more than `maximum` or not all in the file */
static boolean block_take(
	struct model_file const *file,
	unsigned long *cursor,
	long count,
	long maximum,
	unsigned long size,
	unsigned long *offset)
{
	if (count < 0 || count > maximum || !mods_in_file(*cursor, (unsigned long)count * size, file->size))
	{
		return FALSE;
	}
	*offset = *cursor;
	*cursor += (unsigned long)count * size;

	return TRUE;
}

/* a block's count, in a tag file's element */
static long block_count(
	byte const *element,
	unsigned long offset)
{
	return (long)mods_read_be32(element + offset);
}

/* Walks the file, finding where everything is: FALSE (with *reason) when a
count is past what the tool allows, something is not in the file, or the
file goes on past its last shader's name. */
static boolean model_file_walk(
	struct model_file *file,
	char const **reason)
{
	byte const *root = file->bytes + MOD_TAG_FILE_HEADER_SIZE;
	unsigned long cursor = MOD_TAG_FILE_HEADER_SIZE + MODEL_SIZE;
	long index;
	long part_index = 0;

	*reason = "a count of its blocks is past what the tool allows, or the blocks are not all in the file";
	file->model_flags = mods_read_be32(root + MODEL_FLAGS_OFFSET);
	file->marker_count = block_count(root, MODEL_MARKERS_OFFSET);
	file->node_count = block_count(root, MODEL_NODES_OFFSET);
	file->region_count = block_count(root, MODEL_REGIONS_OFFSET);
	file->geometry_count = block_count(root, MODEL_GEOMETRIES_OFFSET);
	file->shader_count = block_count(root, MODEL_SHADERS_OFFSET);

	if (!block_take(file, &cursor, file->marker_count, MAXIMUM_MARKERS_PER_MODEL, MARKER_SIZE, &file->markers))
	{
		return FALSE;
	}
	for (index = 0; index < file->marker_count; index++)
	{
		byte const *marker = file->bytes + file->markers + (unsigned long)index * MARKER_SIZE;

		file->marker_instance_counts[index] = block_count(marker, MARKER_INSTANCES_OFFSET);
		if (!block_take(file, &cursor, file->marker_instance_counts[index], MAXIMUM_INSTANCES_PER_MODEL_MARKER,
			MARKER_INSTANCE_SIZE, &file->marker_instances[index]))
		{
			return FALSE;
		}
	}
	if (!block_take(file, &cursor, file->node_count, MAXIMUM_NODES_PER_MODEL, NODE_SIZE, &file->nodes) ||
		!block_take(file, &cursor, file->region_count, MAXIMUM_REGIONS_PER_MODEL, REGION_SIZE, &file->regions))
	{
		return FALSE;
	}
	for (index = 0; index < file->region_count; index++)
	{
		byte const *region = file->bytes + file->regions + (unsigned long)index * REGION_SIZE;
		long permutation;

		file->permutation_counts[index] = block_count(region, REGION_PERMUTATIONS_OFFSET);
		if (!block_take(file, &cursor, file->permutation_counts[index], MAXIMUM_PERMUTATIONS_PER_MODEL_REGION,
			PERMUTATION_SIZE, &file->permutations[index]))
		{
			return FALSE;
		}
		for (permutation = 0; permutation < file->permutation_counts[index]; permutation++)
		{
			byte const *element = file->bytes + file->permutations[index] + (unsigned long)permutation * PERMUTATION_SIZE;

			file->permutation_marker_counts[index][permutation] = block_count(element, PERMUTATION_MARKERS_OFFSET);
			if (!block_take(file, &cursor, file->permutation_marker_counts[index][permutation],
				MAXIMUM_MARKERS_PER_MODEL_REGION_PERMUTATION, PERMUTATION_MARKER_SIZE,
				&file->permutation_markers[index][permutation]))
			{
				return FALSE;
			}
		}
	}
	{
		unsigned long geometries;

		if (!block_take(file, &cursor, file->geometry_count, MAXIMUM_GEOMETRIES_PER_MODEL, GEOMETRY_SIZE, &geometries))
		{
			return FALSE;
		}
		for (index = 0; index < file->geometry_count; index++)
		{
			byte const *geometry = file->bytes + geometries + (unsigned long)index * GEOMETRY_SIZE;
			unsigned long parts;
			long part;

			file->geometry_part_counts[index] = block_count(geometry, GEOMETRY_PARTS_OFFSET);
			if (!block_take(file, &cursor, file->geometry_part_counts[index], MAXIMUM_PARTS_PER_MODEL_GEOMETRY,
				file->part_size, &parts))
			{
				return FALSE;
			}
			for (part = 0; part < file->geometry_part_counts[index]; part++)
			{
				struct file_part *found = &file->parts[part_index++];
				byte const *element = file->bytes + parts + (unsigned long)part * file->part_size;
				unsigned long compressed;

				found->offset = parts + (unsigned long)part * file->part_size;
				found->vertex_count = block_count(element, PART_VERTICES_OFFSET);
				found->strip_element_count = block_count(element, PART_TRIANGLES_OFFSET);
				if (!block_take(file, &cursor, found->vertex_count, MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART, VERTEX_SIZE,
						&found->vertices) ||
					!block_take(file, &cursor, block_count(element, PART_COMPRESSED_VERTICES_OFFSET),
						MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART, COMPRESSED_VERTEX_SIZE, &compressed) ||
					!block_take(file, &cursor, found->strip_element_count, MAXIMUM_TRIANGLES_PER_MODEL_GEOMETRY_PART,
						STRIP_ELEMENT_SIZE, &found->strip))
				{
					return FALSE;
				}
			}
		}
	}
	file->part_count = part_index;
	if (!block_take(file, &cursor, file->shader_count, MAXIMUM_SHADERS_PER_MODEL, SHADER_REFERENCE_SIZE, &file->shaders))
	{
		return FALSE;
	}
	*reason = "a shader's name is not in the file";
	for (index = 0; index < file->shader_count; index++)
	{
		byte const *reference = file->bytes + file->shaders + (unsigned long)index * SHADER_REFERENCE_SIZE;
		unsigned long length = mods_read_be32(reference + SHADER_REFERENCE_NAME_LENGTH_OFFSET);

		file->shader_names[index] = cursor;
		if (!length || length > 255 || !mods_in_file(cursor, length + 1, file->size) ||
			file->bytes[cursor + length] != 0 || memchr(file->bytes + cursor, 0, length))
		{
			return FALSE;
		}
		cursor += length + 1;
	}
	*reason = "the file goes on past its last shader's name";

	return cursor == file->size;
}

/* the most parts a file walked can find (MAXIMUM_PARTS_PER_MODEL_GEOMETRY
for each of its geometries), for the room to note them in; NONE when its
geometries are more than the tool allows */
static long model_file_part_count(
	struct model_file const *file)
{
	byte const *root = file->bytes + MOD_TAG_FILE_HEADER_SIZE;
	long geometry_count = block_count(root, MODEL_GEOMETRIES_OFFSET);

	/* (model_file_walk finds the parts; this is only their most) */
	if (geometry_count < 0 || geometry_count > MAXIMUM_GEOMETRIES_PER_MODEL)
		return NONE;

	return geometry_count * MAXIMUM_PARTS_PER_MODEL_GEOMETRY;
}

/* Whether the file's nodes are the map's: their names, hierarchy and
default positions. */
static boolean nodes_match(
	struct model_file const *file,
	struct model const *map_model)
{
	long index;

	if (file->node_count != map_model->nodes.count)
	{
		return FALSE;
	}
	for (index = 0; index < file->node_count; index++)
	{
		byte const *element = file->bytes + file->nodes + (unsigned long)index * NODE_SIZE;
		struct model_node const *map_node = TAG_BLOCK_GET_ELEMENT(&map_model->nodes, index, struct model_node);
		char name[TAG_STRING_LENGTH + 1];
		real_point3d translation;
		real_quaternion rotation;

		if (!map_node)
			return FALSE;
		read_name(element, name);
		read_point(element + NODE_TRANSLATION_OFFSET, &translation);
		read_quaternion(element + NODE_ROTATION_OFFSET, &rotation);
		if (strncmp(name, map_node->name, TAG_STRING_LENGTH) ||
			mods_read_short(element + NODE_HIERARCHY_OFFSET) != map_node->next_sibling_node_index ||
			mods_read_short(element + NODE_HIERARCHY_OFFSET + 2) != map_node->first_child_node_index ||
			mods_read_short(element + NODE_HIERARCHY_OFFSET + 4) != map_node->parent_node_index ||
			!point_finite(&translation) || !quaternion_finite(&rotation) ||
			!points_close(&translation, &map_node->default_translation) ||
			!rotations_close(&rotation, &map_node->default_rotation) ||
			!close_enough(mods_read_real(element + NODE_DISTANCE_OFFSET), map_node->node_distance_from_parent))
		{
			return FALSE;
		}
	}

	return TRUE;
}

/* Whether the file's regions and permutations are the map's (names and the
flags the machines agree on), each permutation's geometries ones the file
has. */
static boolean regions_match(
	struct model_file const *file,
	struct model const *map_model)
{
	long region_index;

	if (file->region_count != map_model->regions.count)
	{
		return FALSE;
	}
	for (region_index = 0; region_index < file->region_count; region_index++)
	{
		byte const *region = file->bytes + file->regions + (unsigned long)region_index * REGION_SIZE;
		struct model_region const *map_region = TAG_BLOCK_GET_ELEMENT(&map_model->regions, region_index,
			struct model_region);
		char name[TAG_STRING_LENGTH + 1];
		long permutation_index;

		read_name(region, name);
		if (!map_region || strncmp(name, map_region->name, TAG_STRING_LENGTH) ||
			file->permutation_counts[region_index] != map_region->permutations.count)
		{
			return FALSE;
		}
		for (permutation_index = 0; permutation_index < file->permutation_counts[region_index]; permutation_index++)
		{
			byte const *permutation = file->bytes + file->permutations[region_index] +
				(unsigned long)permutation_index * PERMUTATION_SIZE;
			struct model_region_permutation const *map_permutation = TAG_BLOCK_GET_ELEMENT(&map_region->permutations,
				permutation_index, struct model_region_permutation);
			long detail;

			read_name(permutation, name);
			if (!map_permutation || strncmp(name, map_permutation->name, TAG_STRING_LENGTH) ||
				(mods_read_be32(permutation + PERMUTATION_FLAGS_OFFSET) & PERMUTATION_FLAGS_MASK) !=
					((unsigned long)map_permutation->flags & PERMUTATION_FLAGS_MASK))
			{
				return FALSE;
			}
			for (detail = 0; detail < 5; detail++)
			{
				short geometry = mods_read_short(permutation + PERMUTATION_GEOMETRIES_OFFSET + 2 * detail);

				/* (a detail level the map draws nothing at stays so) */
				if (map_permutation->geometry_indices[detail] == NONE ? geometry != NONE :
					geometry < 0 || geometry >= file->geometry_count)
				{
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

/* the marker instances of the map's model (its markers', and its
permutations' should it have any), into `instances`: their count, or NONE
when they are more than it holds */
static long map_marker_instances(
	struct model const *map_model,
	struct marker_instance *instances)
{
	long count = 0;
	long marker_index;
	long region_index;

	for (marker_index = 0; marker_index < map_model->markers.count; marker_index++)
	{
		struct model_marker const *marker = TAG_BLOCK_GET_ELEMENT(&map_model->markers, marker_index, struct model_marker);
		long instance_index;

		for (instance_index = 0; marker && instance_index < marker->instances.count; instance_index++)
		{
			struct model_marker_instance const *instance = TAG_BLOCK_GET_ELEMENT(&marker->instances, instance_index,
				struct model_marker_instance);
			struct marker_instance *found = &instances[count];

			if (!instance)
				continue;
			if (count == MAXIMUM_MARKER_INSTANCES)
				return NONE;
			memcpy(found->name, marker->name, TAG_STRING_LENGTH);
			found->name[TAG_STRING_LENGTH] = 0;
			found->region_index = instance->region_index;
			found->permutation_index = instance->permutation_index;
			found->node_index = instance->node_index;
			found->translation = instance->translation;
			found->rotation = instance->rotation;
			found->matched = FALSE;
			count++;
		}
	}
	for (region_index = 0; region_index < map_model->regions.count; region_index++)
	{
		struct model_region const *region = TAG_BLOCK_GET_ELEMENT(&map_model->regions, region_index, struct model_region);
		long permutation_index;

		for (permutation_index = 0; region && permutation_index < region->permutations.count; permutation_index++)
		{
			struct model_region_permutation const *permutation = TAG_BLOCK_GET_ELEMENT(&region->permutations,
				permutation_index, struct model_region_permutation);
			long marker_index_in_permutation;

			/* (object_marker: a name, a node, a rotation and a translation,
			as a tag file's permutation markers are, PERMUTATION_MARKER_SIZE) */
			for (marker_index_in_permutation = 0; permutation &&
				marker_index_in_permutation < permutation->markers.count; marker_index_in_permutation++)
			{
				byte const *marker = (byte const *)permutation->markers.address +
					(unsigned long)marker_index_in_permutation * PERMUTATION_MARKER_SIZE;
				struct marker_instance *found = &instances[count];

				if (!permutation->markers.address)
					break;
				if (count == MAXIMUM_MARKER_INSTANCES)
					return NONE;
				memcpy(found->name, marker, TAG_STRING_LENGTH);
				found->name[TAG_STRING_LENGTH] = 0;
				found->region_index = (short)region_index;
				found->permutation_index = (short)permutation_index;
				csmemcpy(&found->node_index, marker + PERMUTATION_MARKER_NODE_OFFSET, sizeof(short));
				csmemcpy(&found->rotation, marker + PERMUTATION_MARKER_ROTATION_OFFSET, sizeof(found->rotation));
				csmemcpy(&found->translation, marker + PERMUTATION_MARKER_TRANSLATION_OFFSET, sizeof(found->translation));
				found->matched = FALSE;
				count++;
			}
		}
	}

	return count;
}

/* Whether a marker instance of the file is one of the map's not yet
matched, and matches it. */
static boolean marker_instance_match(
	struct marker_instance *instances,
	long count,
	char const *name,
	short region_index,
	short permutation_index,
	short node_index,
	real_point3d const *translation,
	real_quaternion const *rotation)
{
	long index;

	if (!point_finite(translation) || !quaternion_finite(rotation))
	{
		return FALSE;
	}
	for (index = 0; index < count; index++)
	{
		struct marker_instance *instance = &instances[index];

		if (!instance->matched && !strncmp(instance->name, name, TAG_STRING_LENGTH) &&
			instance->region_index == region_index && instance->permutation_index == permutation_index &&
			instance->node_index == node_index && points_close(&instance->translation, translation) &&
			rotations_close(&instance->rotation, rotation))
		{
			instance->matched = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}

/* Whether the file's markers, on its model or on its permutations, are the
map's, every one of them (*reason says why not). */
static boolean markers_match(
	struct model_file const *file,
	struct model const *map_model,
	char const **reason)
{
	struct marker_instance *instances = malloc(MAXIMUM_MARKER_INSTANCES * sizeof(*instances));
	long count;
	long matched = 0;
	long index;
	boolean match = TRUE;

	*reason = "out of memory";
	if (!instances)
	{
		return FALSE;
	}
	*reason = "the map's model has more marker instances than are compared";
	count = map_marker_instances(map_model, instances);
	if (count == NONE)
	{
		free(instances);
		return FALSE;
	}
	*reason = "its markers are not the map's (their names, nodes, positions and rotations must be)";
	/* (as many as the map's first: each is then looked for among them) */
	for (index = 0; index < file->marker_count; index++)
		matched += file->marker_instance_counts[index];
	for (index = 0; index < file->region_count; index++)
	{
		long permutation_index;

		for (permutation_index = 0; permutation_index < file->permutation_counts[index]; permutation_index++)
			matched += file->permutation_marker_counts[index][permutation_index];
	}
	if (matched != count)
	{
		free(instances);
		return FALSE;
	}
	for (index = 0; match && index < file->marker_count; index++)
	{
		byte const *marker = file->bytes + file->markers + (unsigned long)index * MARKER_SIZE;
		char name[TAG_STRING_LENGTH + 1];
		long instance_index;

		read_name(marker, name);
		for (instance_index = 0; match && instance_index < file->marker_instance_counts[index]; instance_index++)
		{
			byte const *instance = file->bytes + file->marker_instances[index] +
				(unsigned long)instance_index * MARKER_INSTANCE_SIZE;
			real_point3d translation;
			real_quaternion rotation;

			read_point(instance + 4, &translation);
			read_quaternion(instance + 16, &rotation);
			match = marker_instance_match(instances, count, name, instance[0], instance[1], instance[2], &translation,
				&rotation);
		}
	}
	for (index = 0; match && index < file->region_count; index++)
	{
		long permutation_index;

		for (permutation_index = 0; match && permutation_index < file->permutation_counts[index]; permutation_index++)
		{
			long marker_index;

			for (marker_index = 0; match && marker_index < file->permutation_marker_counts[index][permutation_index];
				marker_index++)
			{
				byte const *marker = file->bytes + file->permutation_markers[index][permutation_index] +
					(unsigned long)marker_index * PERMUTATION_MARKER_SIZE;
				char name[TAG_STRING_LENGTH + 1];
				real_point3d translation;
				real_quaternion rotation;

				read_name(marker, name);
				read_quaternion(marker + PERMUTATION_MARKER_ROTATION_OFFSET, &rotation);
				read_point(marker + PERMUTATION_MARKER_TRANSLATION_OFFSET, &translation);
				match = marker_instance_match(instances, count, name, (short)index, (short)permutation_index,
					mods_read_short(marker + PERMUTATION_MARKER_NODE_OFFSET), &translation, &rotation);
			}
		}
	}
	free(instances);

	/* (each matched one of the map's, as many as it has: none is missing) */
	return match;
}

/* the map's shader the file's shader reference `index` names (a tag of one
of the shader groups of that name, its own group's first), or NONE */
static long shader_find(
	struct model_file const *file,
	long index)
{
	static unsigned long const shader_groups[] =
	{
		'soso', 'senv', 'schi', 'scex', 'sotr', 'sgla', 'smet', 'spla', 'swat',
	};
	byte const *reference = file->bytes + file->shaders + (unsigned long)index * SHADER_REFERENCE_SIZE;
	char const *name = (char const *)file->bytes + file->shader_names[index];
	unsigned long file_group = mods_read_be32(reference);
	long tag_index = NONE;
	long group;

	for (group = 0; tag_index == NONE && group < (long)NUMBEROF(shader_groups); group++)
	{
		if (shader_groups[group] == file_group)
			tag_index = tag_loaded((long)file_group, name);
	}
	for (group = 0; tag_index == NONE && group < (long)NUMBEROF(shader_groups); group++)
	{
		tag_index = tag_loaded((long)shader_groups[group], name);
	}

	return tag_index;
}

/* the model node a vertex's node index names, through its part's table
when the file's parts have local nodes; NONE for none, and for one past what
is named */
static short vertex_model_node(
	struct model_file const *file,
	byte const *part,
	short node_index)
{
	boolean local = file->gbxmodel && TEST_FLAG(file->model_flags, MODEL_PARTS_HAVE_LOCAL_NODES_BIT);

	if (node_index < 0)
	{
		return NONE;
	}
	if (local)
	{
		byte local_count = part[GBXMODEL_PART_LOCAL_NODE_COUNT_OFFSET];

		if (node_index >= local_count || local_count > MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART)
			return NONE;
		node_index = part[GBXMODEL_PART_LOCAL_NODES_OFFSET + node_index];
	}

	return node_index < file->node_count ? node_index : NONE;
}

/* A part's vertices, as this build keeps them uncompressed, into
`vertices`, their nodes the model's: FALSE when a vertex names a node the
model has not got, or has a value this build cannot draw. */
static boolean part_vertices_read(
	struct model_file const *file,
	struct file_part const *part,
	struct model_vertex_uncompressed *vertices)
{
	byte const *element = file->bytes + part->offset;
	long index;

	for (index = 0; index < part->vertex_count; index++)
	{
		byte const *bytes = file->bytes + part->vertices + (unsigned long)index * VERTEX_SIZE;
		struct model_vertex_uncompressed *vertex = &vertices[index];
		short second;

		read_point(bytes, &vertex->position);
		read_vector(bytes + 0x0C, &vertex->normal);
		read_vector(bytes + 0x18, &vertex->binormal);
		read_vector(bytes + 0x24, &vertex->tangent);
		vertex->texcoord.x = mods_read_real(bytes + 0x30);
		vertex->texcoord.y = mods_read_real(bytes + 0x34);
		vertex->nodes[0] = vertex_model_node(file, element, mods_read_short(bytes + 0x38));
		second = mods_read_short(bytes + 0x3A);
		vertex->node_weights[0] = mods_read_real(bytes + 0x3C);
		vertex->node_weights[1] = mods_read_real(bytes + 0x40);
		/* (none: the first node's alone; it is the first node again, of no
		weight, so that it names no node more) */
		if (second == NONE)
		{
			vertex->nodes[1] = vertex->nodes[0];
			vertex->node_weights[1] = 0.f;
		}
		else
		{
			vertex->nodes[1] = vertex_model_node(file, element, second);
		}
		if (vertex->nodes[0] == NONE || vertex->nodes[1] == NONE ||
			!point_finite(&vertex->position) || !mods_finite_real(vertex->texcoord.x) ||
			!mods_finite_real(vertex->texcoord.y) || !mods_finite_real(vertex->node_weights[0]) ||
			!mods_finite_real(vertex->node_weights[1]) || !vector_compressible(&vertex->normal) ||
			!vector_compressible(&vertex->binormal) || !vector_compressible(&vertex->tangent))
		{
			return FALSE;
		}
	}

	return TRUE;
}

/* The part's strip's indices (the triangles block's, less the empty ones
that end it) counted into part->strip_index_count: FALSE when it has not
got a triangle's, or names a vertex the part has not got. */
static boolean part_strip_check(
	struct model_file const *file,
	struct file_part *part)
{
	byte const *strip = file->bytes + part->strip;
	long count = part->strip_element_count * 3;
	long index;

	while (count > 0 && mods_read_be16(strip + 2 * (count - 1)) == NULL_INDEX)
	{
		count--;
	}
	if (count < 3)
	{
		return FALSE;
	}
	for (index = 0; index < count; index++)
	{
		if (mods_read_be16(strip + 2 * index) >= (unsigned long)part->vertex_count)
			return FALSE;
	}
	part->strip_index_count = count;

	return TRUE;
}

/* a part's link to the part drawn before or after it (prev_filthy_part_index,
next_filthy_part_index), NULL_LINK for none: a tag's two links of 0 are
none, as the tool takes them */
static byte part_link(
	byte const *element,
	unsigned long offset)
{
	return element[PART_PREVIOUS_OFFSET] == 0 && element[PART_NEXT_OFFSET] == 0 ? NULL_LINK : element[offset];
}

/* Whether the parts of each geometry are what this build can draw: their
shaders the file's, their links within the geometry and with no ring, their
strips and vertices sound. */
static boolean parts_check(
	struct model_file *file,
	struct model_vertex_uncompressed *scratch,
	char const **reason)
{
	long geometry_index;
	long first = 0;

	for (geometry_index = 0; geometry_index < file->geometry_count; geometry_index++)
	{
		long count = file->geometry_part_counts[geometry_index];
		long part_index;

		for (part_index = 0; part_index < count; part_index++)
		{
			struct file_part *part = &file->parts[first + part_index];
			byte const *element = file->bytes + part->offset;
			short shader = mods_read_short(element + PART_SHADER_OFFSET);
			byte previous = part_link(element, PART_PREVIOUS_OFFSET);
			byte next = part_link(element, PART_NEXT_OFFSET);
			long steps;
			long link;
			real_point3d centroid;

			*reason = "a part names a shader or part the model has not got, or its parts' links go round";
			if (shader < 0 || shader >= file->shader_count ||
				(previous != NULL_LINK && previous >= count) || (next != NULL_LINK && next >= count))
			{
				return FALSE;
			}
			/* (the transparent sort draws a part after those it links
			before it: the links must end) */
			for (link = part_index, steps = 0; link != NULL_LINK && steps <= count; steps++)
			{
				link = part_link(file->bytes + file->parts[first + link].offset, PART_NEXT_OFFSET);
				if (link != NULL_LINK && link >= count)
					return FALSE;
			}
			if (steps > count)
			{
				return FALSE;
			}
			*reason = "a part has no uncompressed vertices (the tool's tag files have them), or its strip has no triangle or names a vertex it has not got";
			if (part->vertex_count < 1 || !part_strip_check(file, part))
			{
				return FALSE;
			}
			*reason = "a part's vertex names a node the model has not got, or has a value this build cannot draw";
			read_point(element + PART_CENTROID_OFFSET, &centroid);
			if (!point_finite(&centroid) || !part_vertices_read(file, part, scratch))
			{
				return FALSE;
			}
		}
		first += count;
	}

	return TRUE;
}

/* the part's centroid's nodes and weights, as the tool makes them: its two
nodes most weighted by its vertices, their weights shared between them */
static void part_centroid_nodes(
	struct model_geometry_part *part,
	struct model_vertex_uncompressed const *vertices,
	long vertex_count,
	long node_count)
{
	real weights[MAXIMUM_NODES_PER_MODEL];
	short first = NONE;
	short second = NONE;
	long index;

	csmemset(weights, 0, sizeof(weights));
	for (index = 0; index < vertex_count; index++)
	{
		weights[vertices[index].nodes[0]] += vertices[index].node_weights[0];
		weights[vertices[index].nodes[1]] += vertices[index].node_weights[1];
	}
	for (index = 0; index < node_count; index++)
	{
		if (first == NONE || weights[index] > weights[first])
		{
			second = first;
			first = (short)index;
		}
		else if (second == NONE || weights[index] > weights[second])
		{
			second = (short)index;
		}
	}
	part->centroid_primary_node_index = 0;
	part->centroid_secondary_node_index = 0;
	part->centroid_primary_node_weight = 1.f;
	part->centroid_secondary_node_weight = 0.f;
	if (node_count > 1 && first != NONE && weights[first] > 0.f)
	{
		part->centroid_primary_node_index = first;
		if (second != NONE && weights[second] > 0.f)
		{
			real total = weights[first] + weights[second];

			part->centroid_secondary_node_index = second;
			part->centroid_primary_node_weight = weights[first] / total;
			part->centroid_secondary_node_weight = weights[second] / total;
		}
	}
}

/* The nodes a part's vertices name, at most
MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART of them, into the palette, and the
vertices made to name them by their place there: FALSE when there are more. */
static boolean part_palette_make(
	struct mod_model_palette *palette,
	struct model_vertex_uncompressed *vertices,
	long vertex_count)
{
	long index;

	palette->node_count = 0;
	for (index = 0; index < vertex_count; index++)
	{
		long slot;

		for (slot = 0; slot < 2; slot++)
		{
			short node = vertices[index].nodes[slot];
			short local;

			for (local = 0; local < palette->node_count && palette->nodes[local] != node; local++)
			{
			}
			if (local == palette->node_count)
			{
				if (palette->node_count == MAXIMUM_NODES_PER_MODEL_GEOMETRY_PART)
					return FALSE;
				palette->nodes[palette->node_count++] = (byte)node;
			}
			vertices[index].nodes[slot] = local;
		}
	}

	return TRUE;
}

/* geometry the renderer draws from: in the Xbox's contiguous memory, as a
map's is (custom_edition_geometry.c's geometry_allocate), while the mods'
models take no more of it than MAXIMUM_CONTIGUOUS_GEOMETRY_BYTES; else the
heap */
static void *geometry_allocate(
	unsigned long size,
	boolean *contiguous)
{
	void *geometry = NULL;

	if (size <= MAXIMUM_CONTIGUOUS_GEOMETRY_BYTES - mod_models_globals.contiguous_bytes)
		geometry = XPhysicalAlloc(size, (unsigned long)-1, 0, PAGE_READWRITE);
	*contiguous = geometry != NULL;
	if (geometry)
		mod_models_globals.contiguous_bytes += size;
	else
		geometry = system_malloc(size);

	return geometry;
}

static void geometry_free(
	void *geometry,
	unsigned long size,
	boolean contiguous)
{
	if (contiguous)
	{
		XPhysicalFree(geometry);
		mod_models_globals.contiguous_bytes -= size;
	}
	else
	{
		system_free(geometry);
	}
}

static void mod_model_free(
	struct mod_model *made)
{
	long index;

	for (index = 0; made->parts && index < made->part_count; index++)
	{
		rasterizer_triangle_buffer_delete(&made->parts[index].triangle_buffer);
		rasterizer_vertex_buffer_delete(&made->parts[index].vertex_buffer);
	}
	/* (the game's free, debug_free, does not take NULL) */
	if (made->geometry)
		geometry_free(made->geometry, made->geometry_size, made->geometry_contiguous);
	if (made->path)
		free(made->path);
	if (made->regions)
		free(made->regions);
	if (made->permutations)
		free(made->permutations);
	if (made->geometries)
		free(made->geometries);
	if (made->parts)
		free(made->parts);
	if (made->shaders)
		free(made->shaders);
	if (made->palettes)
		free(made->palettes);
	free(made);

	return;
}

/* The map's regions and permutations, as the map has them (their markers
the map's), each permutation drawing the file's geometries. */
static boolean regions_make(
	struct mod_model *made,
	struct model_file const *file,
	struct model const *map_model)
{
	long permutation_count = 0;
	long region_index;
	long next = 0;

	for (region_index = 0; region_index < map_model->regions.count; region_index++)
	{
		permutation_count += file->permutation_counts[region_index];
	}
	made->regions = malloc((size_t)map_model->regions.count * sizeof(*made->regions) + 1);
	made->permutations = malloc((size_t)permutation_count * sizeof(*made->permutations) + 1);
	if (!made->regions || !made->permutations)
	{
		return FALSE;
	}
	made->kept_bytes += (unsigned long)map_model->regions.count * sizeof(*made->regions) +
		(unsigned long)permutation_count * sizeof(*made->permutations);
	for (region_index = 0; region_index < map_model->regions.count; region_index++)
	{
		struct model_region const *map_region = TAG_BLOCK_GET_ELEMENT(&map_model->regions, region_index,
			struct model_region);
		long permutation_index;

		made->regions[region_index] = *map_region;
		made->regions[region_index].permutations.address = made->permutations + next;
		made->regions[region_index].permutations.definition = NULL;
		for (permutation_index = 0; permutation_index < map_region->permutations.count; permutation_index++)
		{
			struct model_region_permutation *permutation = &made->permutations[next++];
			byte const *element = file->bytes + file->permutations[region_index] +
				(unsigned long)permutation_index * PERMUTATION_SIZE;
			long detail;

			*permutation = *TAG_BLOCK_GET_ELEMENT(&map_region->permutations, permutation_index,
				struct model_region_permutation);
			for (detail = 0; detail < 5; detail++)
			{
				permutation->geometry_indices[detail] = mods_read_short(element + PERMUTATION_GEOMETRIES_OFFSET + 2 * detail);
			}
		}
	}
	made->model.regions.address = made->regions;
	made->model.regions.definition = NULL;

	return TRUE;
}

/* The file's shaders, each the map's of its name. */
static boolean shaders_make(
	struct mod_model *made,
	struct model_file const *file,
	char const **reason)
{
	long index;

	made->shaders = malloc((size_t)file->shader_count * sizeof(*made->shaders) + 1);
	if (!made->shaders)
	{
		*reason = "out of memory";
		return FALSE;
	}
	made->kept_bytes += (unsigned long)file->shader_count * sizeof(*made->shaders);
	for (index = 0; index < file->shader_count; index++)
	{
		struct model_shader_reference *shader = &made->shaders[index];
		long tag_index = shader_find(file, index);

		if (tag_index == NONE)
		{
			*reason = "it names a shader the map has not got (a mod cannot add tags)";
			return FALSE;
		}
		csmemset(shader, 0, sizeof(*shader));
		shader->shader.group_tag = tag_get_group_tag(tag_index);
		shader->shader.index = tag_index;
		shader->shader.name = tag_get_name(tag_index);
		shader->shader.name_length = (long)strlen(shader->shader.name);
		/* (a negative one, the tool's none, is the shader's first: the
		renderer takes the permutation modulo the bitmap's count) */
		shader->permutation_index = MAX(0, mods_read_short(file->bytes + file->shaders +
			(unsigned long)index * SHADER_REFERENCE_SIZE + SHADER_REFERENCE_PERMUTATION_OFFSET));
	}
	made->model.shaders.count = file->shader_count;
	made->model.shaders.address = made->shaders;
	made->model.shaders.definition = NULL;

	return TRUE;
}

/* the position's hash, by its bits */
static unsigned long position_hash(
	real_point3d const *position)
{
	unsigned long words[3];
	unsigned long hash = 2166136261UL;
	long index;

	memcpy(words, position, sizeof(words));
	for (index = 0; index < 3; index++)
		hash = (hash ^ words[index]) * 16777619UL;

	return hash;
}

/* Blends the normals of the model's vertices that have the same position,
every part's, as the tool does for a model whose tag asks for it: each
their sum made a unit vector (one that sums to none, the x axis). FALSE
when there is no memory for it. */
static boolean shared_normals_blend(
	struct model_vertex_uncompressed *vertices,
	long count)
{
	unsigned long size = 1;
	long *table;
	long *firsts;
	real_vector3d *sums;
	long index;

	while (size < (unsigned long)count * 2)
		size <<= 1;
	table = malloc(size * sizeof(*table));
	firsts = malloc((size_t)count * sizeof(*firsts) + 1);
	sums = malloc((size_t)count * sizeof(*sums) + 1);
	if (!table || !firsts || !sums)
	{
		if (table)
			free(table);
		if (firsts)
			free(firsts);
		if (sums)
			free(sums);
		return FALSE;
	}
	for (index = 0; index < (long)size; index++)
		table[index] = NONE;
	/* (each vertex's first of its position, and the sum of their normals) */
	for (index = 0; index < count; index++)
	{
		unsigned long slot = position_hash(&vertices[index].position) & (size - 1);

		while (table[slot] != NONE &&
			memcmp(&vertices[table[slot]].position, &vertices[index].position, sizeof(real_point3d)))
		{
			slot = (slot + 1) & (size - 1);
		}
		if (table[slot] == NONE)
		{
			table[slot] = index;
			sums[index].i = sums[index].j = sums[index].k = 0.f;
		}
		firsts[index] = table[slot];
		add_vectors3d(&sums[firsts[index]], &vertices[index].normal, &sums[firsts[index]]);
	}
	for (index = 0; index < count; index++)
	{
		if (firsts[index] == index && normalize3d(&sums[index]) == 0.f)
		{
			sums[index].i = 1.f;
			sums[index].j = sums[index].k = 0.f;
		}
	}
	for (index = 0; index < count; index++)
		vertices[index].normal = sums[firsts[index]];
	free(table);
	free(firsts);
	free(sums);

	return TRUE;
}

/* The file's geometries and parts, their vertices compressed and given
buffers, as a Custom Edition map's are. */
static boolean geometries_make(
	struct mod_model *made,
	struct model_file const *file,
	char const **reason)
{
	long vertex_size = rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_model_compressed);
	long uncompressed_vertex_size = rasterizer_geometry_get_vertex_size(_rasterizer_vertex_type_model_uncompressed);
	boolean palettes = file->node_count >= RASTERIZER_MAXIMUM_NODES_PER_MODEL;
	unsigned long geometry_bytes = 0;
	struct model_vertex_uncompressed *uncompressed;
	struct model_vertex_uncompressed *scratch;
	struct model_vertex_compressed *vertices;
	word *strips;
	long vertex_total = 0;
	long geometry_index;
	long part_index;
	long first = 0;

	*reason = "out of memory";
	for (part_index = 0; part_index < file->part_count; part_index++)
	{
		vertex_total += file->parts[part_index].vertex_count;
		geometry_bytes += (unsigned long)file->parts[part_index].vertex_count * (unsigned long)vertex_size +
			(unsigned long)file->parts[part_index].strip_index_count * sizeof(word);
	}
	/* (every part's vertices, as this build keeps them uncompressed, at once:
	their normals may be blended across parts. parts_check read them) */
	uncompressed = malloc((size_t)vertex_total * sizeof(*uncompressed) + 1);
	if (!uncompressed)
	{
		return FALSE;
	}
	for (part_index = 0, scratch = uncompressed; part_index < file->part_count; part_index++)
	{
		part_vertices_read(file, &file->parts[part_index], scratch);
		scratch += file->parts[part_index].vertex_count;
	}
	if (TEST_FLAG(file->model_flags, MODEL_BLEND_SHARED_NORMALS_BIT) && !shared_normals_blend(uncompressed, vertex_total))
	{
		free(uncompressed);
		return FALSE;
	}
	made->geometries = malloc((size_t)file->geometry_count * sizeof(*made->geometries) + 1);
	made->parts = malloc((size_t)file->part_count * sizeof(*made->parts) + 1);
	made->palettes = palettes ? malloc((size_t)file->part_count * sizeof(*made->palettes) + 1) : NULL;
	made->geometry_size = geometry_bytes + 1;
	made->geometry = geometry_allocate(made->geometry_size, &made->geometry_contiguous);
	if (!made->geometries || !made->parts || (palettes && !made->palettes) || !made->geometry)
	{
		free(uncompressed);
		return FALSE;
	}
	csmemset(made->parts, 0, (size_t)file->part_count * sizeof(*made->parts));
	made->kept_bytes += geometry_bytes + (unsigned long)file->geometry_count * sizeof(*made->geometries) +
		(unsigned long)file->part_count * (sizeof(*made->parts) + (palettes ? sizeof(*made->palettes) : 0));
	vertices = (struct model_vertex_compressed *)made->geometry;
	strips = (word *)(vertices + vertex_total);
	scratch = uncompressed;

	for (geometry_index = 0; geometry_index < file->geometry_count; geometry_index++)
	{
		struct model_geometry *geometry = &made->geometries[geometry_index];
		long count = file->geometry_part_counts[geometry_index];

		csmemset(geometry, 0, sizeof(*geometry));
		geometry->parts.count = count;
		geometry->parts.address = made->parts + first;
		for (part_index = 0; part_index < count; part_index++)
		{
			struct file_part const *source = &file->parts[first + part_index];
			struct model_geometry_part *part = &made->parts[first + part_index];
			byte const *element = file->bytes + source->offset;
			long strip_index;

			part->flags = mods_read_be32(element) & ~FLAG(PART_LOCAL_NODES_BIT);
			part->shader_index = mods_read_short(element + PART_SHADER_OFFSET);
			part->previous_part_index = part_link(element, PART_PREVIOUS_OFFSET) == NULL_LINK ? NONE :
				(char)part_link(element, PART_PREVIOUS_OFFSET);
			part->next_part_index = part_link(element, PART_NEXT_OFFSET) == NULL_LINK ? NONE :
				(char)part_link(element, PART_NEXT_OFFSET);
			read_point(element + PART_CENTROID_OFFSET, &part->centroid);
			part_centroid_nodes(part, scratch, source->vertex_count, file->node_count);
			if (palettes)
			{
				struct mod_model_palette *palette = &made->palettes[made->palette_count];

				if (!part_palette_make(palette, scratch, source->vertex_count))
				{
					*reason = "a part of this model of many nodes has vertices that name more nodes than a part is drawn with";
					free(uncompressed);
					return FALSE;
				}
				palette->vertex_buffer = &part->vertex_buffer;
				made->palette_count++;
			}
			rasterizer_geometry_compress_vertices(
				_rasterizer_vertex_type_model_uncompressed,
				source->vertex_count,
				vertices,
				source->vertex_count * vertex_size,
				scratch,
				source->vertex_count * uncompressed_vertex_size);
			for (strip_index = 0; strip_index < source->strip_index_count; strip_index++)
			{
				strips[strip_index] = (word)mods_read_be16(file->bytes + source->strip + 2 * strip_index);
			}
			made->part_count = first + part_index + 1;
			*reason = "the renderer cannot make its buffers";
			if (!rasterizer_vertex_buffer_new(&part->vertex_buffer, _rasterizer_vertex_type_model_compressed,
					source->vertex_count, vertices, source->vertex_count * vertex_size) ||
				!rasterizer_triangle_buffer_new(&part->triangle_buffer, _triangle_buffer_type_precompiled_strip,
					source->strip_index_count - 2, strips))
			{
				free(uncompressed);
				return FALSE;
			}
			vertices += source->vertex_count;
			strips += source->strip_index_count;
			scratch += source->vertex_count;
		}
		first += count;
	}
	made->model.geometries.count = file->geometry_count;
	made->model.geometries.address = made->geometries;
	made->model.geometries.definition = NULL;
	free(uncompressed);

	return TRUE;
}

static struct mod_model *mod_model_new(
	long tag_index,
	long file_index,
	struct model const *map_model,
	char const **reason)
{
	struct mod_model *made;
	struct model_file file;
	struct model_vertex_uncompressed *scratch = NULL;
	byte *bytes;
	unsigned long size = 0;
	long part_room;
	long index;

	*reason = "out of memory";
	bytes = mods_tag_file_read(file_index, &size, reason);
	if (!bytes)
	{
		return NULL;
	}
	csmemset(&file, 0, sizeof(file));
	file.bytes = bytes;
	file.size = size;
	file.gbxmodel = mods_read_be32(bytes + MOD_TAG_FILE_GROUP_OFFSET) == GBXMODEL_GROUP_TAG;
	file.part_size = file.gbxmodel ? GBXMODEL_PART_SIZE : PART_SIZE;
	made = malloc(sizeof(*made));
	if (made)
	{
		csmemset(made, 0, sizeof(*made));
		made->tag_index = tag_index;
		made->file = file_index;
		made->path = malloc(strlen(port_mods_file_path(file_index)) + 1);
	}
	*reason = "out of memory";
	if (!made || !made->path || !port_mods_file_stamp(file_index, &made->file_size, made->stamp))
	{
		goto refused;
	}
	strcpy(made->path, port_mods_file_path(file_index));
	made->kept_bytes = sizeof(*made) + strlen(made->path) + 1;

	*reason = "it is not a model tag file of the Halo Editing Kit's version";
	if (size < MOD_TAG_FILE_HEADER_SIZE + MODEL_SIZE ||
		!(file.gbxmodel ? mods_tag_file_header_valid(bytes, GBXMODEL_GROUP_TAG, GBXMODEL_TAG_VERSION) :
			mods_tag_file_header_valid(bytes, MODELS_GROUP_TAG, MODELS_GROUP_VERSION)))
	{
		goto refused;
	}
	part_room = model_file_part_count(&file);
	*reason = "a count of its blocks is past what the tool allows";
	if (part_room == NONE)
	{
		goto refused;
	}
	*reason = "out of memory";
	file.parts = malloc((size_t)part_room * sizeof(*file.parts) + 1);
	scratch = malloc((size_t)MAXIMUM_VERTICES_PER_MODEL_GEOMETRY_PART * sizeof(*scratch));
	if (!file.parts || !scratch || !model_file_walk(&file, reason))
	{
		goto refused;
	}

	/* what the machines agree on: the map's */
	*reason = "its nodes are not the map's (their names, hierarchy and default positions must be)";
	if (!nodes_match(&file, map_model))
	{
		goto refused;
	}
	*reason = "its regions and permutations are not the map's, or a permutation draws a geometry it has not got";
	if (!regions_match(&file, map_model))
	{
		goto refused;
	}
	if (!markers_match(&file, map_model, reason))
	{
		goto refused;
	}
	if (!parts_check(&file, scratch, reason))
	{
		goto refused;
	}

	/* what is drawn: the file's */
	made->model = *map_model;
	made->model.flags &= ~FLAG(MODEL_PARTS_HAVE_LOCAL_NODES_BIT);
	/* (a tag file has its detail levels' cutoffs from super high to super
	low, a map from super low to super high: the tool turns them round) */
	for (index = 0; index < 5; index++)
	{
		made->model.detail_cutoff_pixels[index] = mods_read_real(bytes + MOD_TAG_FILE_HEADER_SIZE +
			MODEL_CUTOFFS_OFFSET + 4 * (4 - index));
	}
	/* (a scale of 0 is the tool's default, 1) */
	made->model.base_map_scale.i = mods_read_real(bytes + MOD_TAG_FILE_HEADER_SIZE + MODEL_BASE_MAP_SCALE_OFFSET);
	made->model.base_map_scale.j = mods_read_real(bytes + MOD_TAG_FILE_HEADER_SIZE + MODEL_BASE_MAP_SCALE_OFFSET + 4);
	if (made->model.base_map_scale.i == 0.f)
		made->model.base_map_scale.i = 1.f;
	if (made->model.base_map_scale.j == 0.f)
		made->model.base_map_scale.j = 1.f;
	*reason = "a detail level's cutoff or the base map's scale is not a number";
	if (!mods_finite_real(made->model.detail_cutoff_pixels[0]) || !mods_finite_real(made->model.detail_cutoff_pixels[1]) ||
		!mods_finite_real(made->model.detail_cutoff_pixels[2]) || !mods_finite_real(made->model.detail_cutoff_pixels[3]) ||
		!mods_finite_real(made->model.detail_cutoff_pixels[4]) || !mods_finite_real(made->model.base_map_scale.i) ||
		!mods_finite_real(made->model.base_map_scale.j))
	{
		goto refused;
	}
	*reason = "out of memory";
	if (!regions_make(made, &file, map_model) || !shaders_make(made, &file, reason) ||
		!geometries_make(made, &file, reason))
	{
		goto refused;
	}
	free(scratch);
	free(file.parts);
	free(bytes);

	return made;

refused:
	if (scratch)
		free(scratch);
	if (file.parts)
		free(file.parts);
	free(bytes);
	if (made)
		mod_model_free(made);

	return NULL;
}

/* the override of the model tag `tag_index` */
static struct mod_model_override *override_get(
	long tag_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);

	return absolute_index >= 0 && absolute_index < mod_models_globals.override_count ?
		&mod_models_globals.overrides[absolute_index] : NULL;
}

/* Makes the table of overrides, one for each of the map's tags up to its
last model tag; FALSE when it cannot be. */
static boolean overrides_new(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long count = 0;

	if (mod_models_globals.overrides)
	{
		return TRUE;
	}
	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		count = MAX(count, DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index) + 1);
	}
	if (count <= 0 || !(mod_models_globals.overrides = malloc((size_t)count * sizeof(struct mod_model_override))))
	{
		return FALSE;
	}
	memset(mod_models_globals.overrides, 0, (size_t)count * sizeof(struct mod_model_override));
	mod_models_globals.override_count = count;

	return TRUE;
}

/* Gives the map's model tag `tag_index` its own definition back. */
static void model_unplug(
	long tag_index,
	struct mod_model_override *override)
{
	memcpy(model_definition_get(tag_index), &override->map_model, sizeof(override->map_model));
	override->model = NULL;

	return;
}

/* Draws the model made `made` for the map's model tag `tag_index`. */
static void model_plug(
	long tag_index,
	struct mod_model_override *override,
	struct mod_model *made)
{
	struct model *definition = model_definition_get(tag_index);

	memcpy(&override->map_model, definition, sizeof(*definition));
	memcpy(definition, &made->model, sizeof(*definition));
	override->model = made;

	return;
}

/* a model made before for the tag `tag_index` from the file at `path` as
it is now (`size` bytes, modified at `stamp`), played or not, or NULL */
static struct mod_model *model_kept(
	long tag_index,
	char const *path,
	unsigned long size,
	unsigned long const stamp[2])
{
	struct mod_model *made;

	for (made = mod_models_globals.models; made; made = made->next)
	{
		if (made->tag_index == tag_index && made->file_size == size && made->stamp[0] == stamp[0] &&
			made->stamp[1] == stamp[1] && !strcmp(made->path, path))
		{
			return made;
		}
	}

	return NULL;
}

/* ---------- public code */

long mod_models_update(
	boolean remove_all,
	boolean at_load)
{
	struct tag_iterator iterator;
	long tag_index;
	long loaded_count = 0;
	long removed_count = 0;
	long refused_count = 0;
	long played_count = 0;
	boolean changed = FALSE;
	long index;

	if (!mods_map_loaded() || !overrides_new())
	{
		return 0;
	}

	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct mod_model_override *override = override_get(tag_index);
		struct model *definition = model_definition_get(tag_index);
		struct mod_model *made = NULL;
		long file = NONE;
		char const *reason = NULL;

		if (!override || !definition)
		{
			continue;
		}
		/* (of a .gbxmodel and a .model, the file of the mod loaded later;
		of one mod's two, the .gbxmodel) */
		if (!remove_all)
		{
			long xbox_file = mods_tag_file_find(tag_index, MODEL_FILE_EXTENSION);

			file = mods_tag_file_find(tag_index, GBXMODEL_FILE_EXTENSION);
			if (file == NONE || (xbox_file != NONE && port_mods_file_order(xbox_file) > port_mods_file_order(file)))
				file = xbox_file;
		}
		if (file != NONE)
		{
			unsigned long size;
			unsigned long stamp[2];

			/* (one made before from the file as it is: only where it is in the
			mods' files changes) */
			if (port_mods_file_stamp(file, &size, stamp))
				made = model_kept(tag_index, port_mods_file_path(file), size, stamp);
			if (made)
				made->file = file;
			if (made && made == override->model)
				continue;
		}
		if (file != NONE && !made)
		{
			made = mod_model_new(tag_index, file, override->model ? &override->map_model : definition, &reason);
			if (made && !mods_memory_take(made->kept_bytes))
			{
				reason = "the mods' tags already hold too much memory";
				mod_model_free(made);
				made = NULL;
			}
			/* (one that changed and now fails a check leaves what was played
			before, as the other groups' files do) */
			if (!made)
			{
				error(_error_silent, "mods: %s left out: %s", port_mods_file_path(file), reason);
				console_warning("mods: the model %s of %s left out: %s", tag_get_name(tag_index), port_mods_file_mod(file),
					reason);
				refused_count++;
				continue;
			}
			else
			{
				made->next = mod_models_globals.models;
				mod_models_globals.models = made;
				loaded_count++;
			}
		}
		if (!made && !override->model)
		{
			continue;
		}
		if (override->model)
		{
			model_unplug(tag_index, override);
			if (!made)
				removed_count++;
		}
		if (made)
		{
			model_plug(tag_index, override, made);
		}
		changed = TRUE;
	}
	for (index = 0; index < mod_models_globals.override_count; index++)
	{
		if (mod_models_globals.overrides[index].model)
			played_count++;
	}
	/* (a map's are linked as its render starts: render_initialize_for_new_map;
	a model whose parts are already linked is left as it is) */
	if (changed && !at_load)
	{
		models_fix_transparent_part_links();
	}
	if (!at_load || loaded_count || refused_count)
	{
		error(_error_silent, "mods: %ld models from their files (%ld new), %ld given back, %ld left out",
			played_count, loaded_count, removed_count, refused_count);
		if (!at_load && mods_say())
		{
			console_printf(FALSE, "mods: %ld models from their files (%ld new), %ld given back, %ld left out",
				played_count, loaded_count, removed_count, refused_count);
		}
	}

	return played_count;
}

/* (before the tags go: the map's models get their own definitions back,
unless they are gone already, and every model made lets go of its
buffers) */
void mod_models_tags_unloaded(
	boolean give_back)
{
	struct mod_model *made = mod_models_globals.models;
	long index;

	for (index = 0; give_back && index < mod_models_globals.override_count; index++)
	{
		struct mod_model_override *override = &mod_models_globals.overrides[index];

		if (override->model)
			model_unplug(override->model->tag_index, override);
	}
	while (made)
	{
		struct mod_model *next = made->next;

		mod_model_free(made);
		made = next;
	}
	if (mod_models_globals.overrides)
		free(mod_models_globals.overrides);
	memset(&mod_models_globals, 0, sizeof(mod_models_globals));

	return;
}

short mod_models_part_palette(
	struct vertex_buffer const *vertex_buffer,
	byte const **nodes)
{
	struct mod_model *made;

	for (made = mod_models_globals.models; made; made = made->next)
	{
		long index;

		for (index = 0; index < made->palette_count; index++)
		{
			if (made->palettes[index].vertex_buffer == vertex_buffer)
			{
				*nodes = made->palettes[index].nodes;
				return made->palettes[index].node_count;
			}
		}
	}

	return 0;
}
