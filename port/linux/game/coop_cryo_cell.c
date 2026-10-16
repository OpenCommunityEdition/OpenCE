/*
COOP_CRYO_CELL.C

The cell two cryo bay tubes stand in: Bungie's cradle under cryotube_1 and
_2, cut from the loaded a10a, as a model, a collision model and a scenery
tag added to the map's tags (coop_cryo.c places it).
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game_globals.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "objects/object_definitions.h"
#include "objects/object_types.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

#include "coop_cryo.h"

#include <math.h>
#include <string.h>

void *cache_files_tag_instances(long *count);
void cache_files_set_tag_instances(void *instances, long count);

/* ---------- definitions */

/* cache_files.c's, models.c's, collision_bsp.c's and damage.c's own */
struct cache_file_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

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

struct environment_vertex_compressed
{
	real_point3d position;
	unsigned long normal;
	unsigned long binormal;
	unsigned long tangent;
	real_point2d texcoord;
};

struct collision_leaf
{
	word flags;
	short bsp2d_reference_count;
	long first_bsp2d_reference_index;
};

struct bsp2d_reference
{
	long plane_designator;
	long root_index;
};

struct damage_region
{
	char name[TAG_STRING_LENGTH+1];
	unsigned long flags;
	long unused0;
	real damage_threshold;
	long unused1[3];
	struct tag_reference destroyed_effect;
	struct tag_block permutations;
};

typedef char cell_tag_instance_size_assert[sizeof(struct cache_file_tag_instance) == 0x20 ? 1 : -1];
typedef char cell_model_shader_reference_size_assert[sizeof(struct model_shader_reference) == 0x20 ? 1 : -1];
typedef char cell_model_geometry_size_assert[sizeof(struct model_geometry) == 0x30 ? 1 : -1];
typedef char cell_model_geometry_part_size_assert[sizeof(struct model_geometry_part) == 0x68 ? 1 : -1];
typedef char cell_environment_vertex_size_assert[sizeof(struct environment_vertex_compressed) == 0x20 ? 1 : -1];
typedef char cell_collision_leaf_size_assert[sizeof(struct collision_leaf) == 0x08 ? 1 : -1];
typedef char cell_damage_region_size_assert[sizeof(struct damage_region) == 0x54 ? 1 : -1];

/* ---------- constants */

#define CELL_TAG_NAME "levels\\a10\\devices\\cryotube\\cryotube cell"
#define CELL_CAP_SHADER "levels\\a10\\shaders\\panel scuff"
#define CELL_EDGE_SHADER "levels\\a10\\shaders\\steel"
#define CELL_STRIP_SHADER "levels\\a10\\shaders\\panel strip"

enum
{
	CELL_MAXIMUM_POINTS = 16,
	CELL_MAXIMUM_POLYGONS = 2048,
	CELL_MAXIMUM_SEGMENTS = 512,
	CELL_MAXIMUM_TRIANGLES = 1024,
	CELL_MAXIMUM_SHADERS = 16,
	CELL_MAXIMUM_LOOP_POINTS = 256,
	CELL_NEW_TAGS = 3,
	CELL_PIECES = 5,
	CELL_MAXIMUM_PIECE_FACES = 32,
	CELL_MAXIMUM_FACE_POINTS = 8,
	CELL_MAXIMUM_BLOCKS = 64,
	CELL_EXPECTED_TRIANGLES = 315,

	_cell_plane_back = 0,
	_cell_plane_top = 5,
	NUMBER_OF_CELL_PLANES = 6,
};

/* the cut: between the cradle's two tubes, in a10a */
#define CELL_ORIGIN_X -56.3216
#define CELL_SHIFT 0.035
#define CELL_X_BACK -57.41
#define CELL_X_FRONT -55.122
#define CELL_Y_HALF 0.92
#define CELL_PLATFORM_Y 0.8
#define CELL_Z_LOW -0.03
#define CELL_Z_TOP 1.25
/* the trays (under the hazard strips) and the rim, lifted onto a plate */
#define CELL_TRAY -0.025
#define CELL_RIM 0.0
#define CELL_PLATE 0.006
#define CELL_STEP 0.006

static struct
{
	short axis;
	double value;
	double sign;
} const cell_planes[NUMBER_OF_CELL_PLANES] =
{
	{ 0, CELL_X_BACK, 1.0 }, { 0, CELL_X_FRONT, -1.0 }, { 1, -CELL_Y_HALF, 1.0 }, { 1, CELL_Y_HALF, -1.0 },
	{ 2, CELL_Z_LOW, 1.0 }, { 2, CELL_Z_TOP, -1.0 },
};

/* the plinth, the console and the three columns (the cell's frame), each a
convex hull: its corners and outward triangles. A collision surface has at
most 8 corners, so the right column leaves out one in line with two others */
static real const cell_collision_corners[][3] =
{
	{ -0.7849f, 0.7991f, 0.8f }, { -0.7849f, -0.8002f, 0.8f }, { -1.1234f, 0.5878f, 0.8f },
	{ -1.1234f, -0.5887f, 0.8f }, { -0.4849f, -0.8002f, 0.3f }, { -0.4849f, 0.7991f, 0.3f },
	{ -0.7849f, 0.7991f, 0.0f }, { -0.7849f, -0.8002f, 0.0f }, { -1.1234f, -0.5887f, 0.0f },
	{ -0.4849f, -0.8002f, 0.0f }, { -0.4849f, 0.7991f, 0.0f }, { -1.1234f, 0.5878f, 0.0f },
	{ 0.7151f, -0.1371f, 0.4f }, { 0.7526f, -0.1371f, 0.4f }, { 0.7151f, 0.1371f, 0.4f },
	{ 0.7526f, 0.1371f, 0.4f }, { 1.0151f, -0.18f, 0.25f }, { 1.0151f, 0.18f, 0.25f },
	{ 0.9151f, 0.18f, 0.275f }, { 0.9151f, -0.18f, 0.275f }, { 0.7151f, 0.1371f, 0.0f },
	{ 0.7151f, -0.1371f, 0.0f }, { 1.0151f, 0.18f, 0.0f }, { 0.9151f, 0.18f, 0.0f }, { 1.0151f, -0.18f, 0.0f },
	{ 0.9151f, -0.18f, 0.0f }, { -0.7849f, 0.15f, 1.25f }, { -0.7849f, -0.15f, 1.25f },
	{ -0.8851f, 0.25f, 1.25f }, { -1.0851f, -0.2511f, 1.25f }, { -0.8851f, -0.2502f, 1.25f },
	{ -1.1234f, 0.2106f, 1.25f }, { -1.0851f, 0.2489f, 1.25f }, { -1.1234f, -0.2127f, 1.25f },
	{ -0.7849f, 0.15f, 0.805f }, { -1.0851f, 0.2489f, 0.805f }, { -0.8851f, 0.25f, 0.805f },
	{ -1.0851f, -0.2511f, 0.805f }, { -0.8851f, -0.2502f, 0.805f }, { -0.7849f, -0.15f, 0.805f },
	{ -1.1234f, 0.2106f, 0.805f }, { -1.1234f, -0.2127f, 0.805f }, { -1.0851f, 0.8989f, 1.25f },
	{ -0.8851f, 0.8999f, 1.25f }, { -1.0851f, 0.5495f, 1.25f }, { -0.7849f, 0.7991f, 1.25f },
	{ -1.1234f, 0.8606f, 1.25f }, { -0.7849f, 0.6496f, 1.25f }, { -0.8851f, 0.5495f, 1.25f },
	{ -1.1234f, 0.5879f, 1.25f }, { -1.0851f, 0.8989f, 0.805f }, { -1.0851f, 0.5495f, 0.805f },
	{ -0.8851f, 0.5495f, 0.805f }, { -0.8851f, 0.8999f, 0.805f }, { -0.7849f, 0.7991f, 0.805f },
	{ -1.1234f, 0.8606f, 0.805f }, { -0.7849f, 0.6496f, 0.805f }, { -1.1234f, 0.5879f, 0.805f },
	{ -0.8851f, -0.9f, 1.25f }, { -1.0851f, -0.8989f, 1.25f }, { -1.0851f, -0.5503f, 1.25f },
	{ -0.8851f, -0.5503f, 1.25f }, { -0.7849f, -0.8002f, 1.25f }, { -1.1234f, -0.8611f, 1.25f },
	{ -0.7849f, -0.6506f, 1.25f }, { -1.1234f, -0.5887f, 1.25f }, { -0.8851f, -0.9f, 0.805f },
	{ -1.0851f, -0.5503f, 0.805f }, { -0.8851f, -0.5503f, 0.805f }, { -1.0851f, -0.8989f, 0.805f },
	{ -1.1234f, -0.8611f, 0.805f }, { -0.7849f, -0.6506f, 0.805f }, { -0.7849f, -0.8002f, 0.805f },
	{ -1.1234f, -0.5887f, 0.805f },
};

static short const cell_collision_triangles[][3] =
{
	{ 5, 10, 6 }, { 5, 6, 0 }, { 5, 9, 10 }, { 5, 4, 9 }, { 5, 1, 4 }, { 5, 0, 1 }, { 2, 3, 1 }, { 2, 1, 0 },
	{ 11, 8, 3 }, { 11, 3, 2 }, { 11, 0, 6 }, { 11, 2, 0 }, { 7, 3, 8 }, { 7, 1, 3 }, { 7, 6, 10 }, { 7, 8, 11 },
	{ 7, 10, 9 }, { 7, 11, 6 }, { 7, 4, 1 }, { 7, 9, 4 }, { 2, 3, 6 }, { 5, 6, 3 }, { 7, 1, 0 }, { 7, 4, 1 },
	{ 2, 8, 9 }, { 2, 9, 0 }, { 2, 1, 3 }, { 2, 0, 1 }, { 2, 11, 8 }, { 2, 6, 11 }, { 5, 12, 10 }, { 5, 4, 12 },
	{ 5, 3, 1 }, { 5, 1, 4 }, { 5, 10, 11 }, { 5, 11, 6 }, { 13, 9, 8 }, { 13, 8, 11 }, { 13, 11, 10 },
	{ 13, 10, 12 }, { 13, 0, 9 }, { 13, 7, 0 }, { 13, 12, 4 }, { 13, 4, 7 }, { 14, 7, 5 }, { 14, 15, 7 },
	{ 10, 2, 0 }, { 10, 0, 8 }, { 4, 5, 7 }, { 4, 1, 0 }, { 4, 0, 2 }, { 4, 6, 5 }, { 4, 7, 3 }, { 4, 2, 6 },
	{ 11, 3, 7 }, { 11, 7, 15 }, { 11, 4, 3 }, { 11, 12, 4 }, { 13, 0, 1 }, { 13, 8, 0 }, { 13, 1, 4 },
	{ 13, 4, 12 }, { 9, 15, 14 }, { 9, 11, 15 }, { 9, 10, 8 }, { 9, 8, 13 }, { 9, 12, 11 }, { 9, 13, 12 },
	{ 9, 14, 5 }, { 9, 5, 6 }, { 9, 6, 2 }, { 9, 2, 10 }, { 13, 4, 0 }, { 13, 0, 8 }, { 13, 7, 4 },
	{ 13, 15, 7 }, { 9, 6, 2 }, { 9, 10, 6 }, { 9, 2, 7 }, { 9, 7, 15 }, { 11, 0, 1 }, { 11, 8, 0 },
	{ 11, 1, 3 }, { 11, 3, 12 }, { 5, 4, 7 }, { 5, 0, 4 }, { 5, 3, 1 }, { 5, 2, 6 }, { 5, 1, 0 }, { 5, 7, 2 },
	{ 14, 8, 11 }, { 14, 10, 9 }, { 14, 13, 8 }, { 14, 15, 13 }, { 14, 9, 15 }, { 14, 11, 12 }, { 14, 6, 10 },
	{ 14, 5, 6 }, { 14, 12, 3 }, { 14, 3, 5 }, { 14, 4, 0 }, { 14, 0, 8 }, { 12, 5, 7 }, { 12, 7, 15 },
	{ 6, 7, 5 }, { 6, 5, 1 }, { 6, 0, 4 }, { 6, 3, 2 }, { 6, 1, 0 }, { 6, 2, 7 }, { 6, 4, 14 }, { 6, 14, 13 },
	{ 9, 7, 2 }, { 9, 15, 7 }, { 11, 8, 0 }, { 11, 0, 1 }, { 11, 1, 5 }, { 11, 5, 12 }, { 10, 14, 8 },
	{ 10, 13, 14 }, { 10, 15, 9 }, { 10, 12, 15 }, { 10, 8, 11 }, { 10, 11, 12 }, { 10, 3, 6 }, { 10, 6, 13 },
	{ 10, 2, 3 }, { 10, 9, 2 },
};

/* first corner, corners, first triangle, triangles */
static short const cell_collision_pieces[CELL_PIECES][4] =
{
	{ 0, 12, 0, 20 }, { 12, 14, 20, 24 }, { 26, 16, 44, 28 }, { 42, 16, 72, 28 }, { 58, 16, 100, 28 },
};

/* planes between the pieces: z under the columns, x between the plinth
and the console, y between the columns */
static real const cell_separators[][4] =
{
	{ 0.0f, 0.0f, 1.0f, 0.8025f }, { 1.0f, 0.0f, 0.0f, 0.115f }, { 0.0f, 1.0f, 0.0f, 0.4f },
	{ 0.0f, 1.0f, 0.0f, -0.4f },
};

static real const cell_pathfinding_spheres[][4] =
{
	{ -0.8f, -0.53f, 0.4f, 0.32f }, { -0.8f, 0.0f, 0.4f, 0.32f }, { -0.8f, 0.53f, 0.4f, 0.32f },
	{ 0.865f, 0.0f, 0.2f, 0.18f },
};

/* ---------- structures */

struct cell_point
{
	double position[3];
	double normal[3];
	double binormal[3];
	double tangent[3];
	double texcoord[2];
};

struct cell_polygon
{
	long shader_index;
	boolean made;
	short count;
	struct cell_point points[CELL_MAXIMUM_POINTS];
};

struct cell_segment
{
	double ends[2][2];
};

struct cell_triangle
{
	long shader_index;
	boolean made;
	struct cell_point points[3];
};

/* ---------- globals */

static struct
{
	long definition_index;
	boolean tried;
	struct cache_file_tag_instance *instances;
	struct cache_file_tag_instance *original_instances;
	long original_count;
	struct model_geometry_part *parts;
	short part_count;
	void *blocks[CELL_MAXIMUM_BLOCKS];
	short block_count;
} cell_globals = { NONE };

/* ---------- private code */

static void *cell_allocate(
	long size)
{
	void *block;

	if (cell_globals.block_count >= CELL_MAXIMUM_BLOCKS || !(block = malloc(size > 0 ? size : 1)))
		return NULL;
	memset(block, 0, size > 0 ? size : 1);
	cell_globals.blocks[cell_globals.block_count++] = block;
	return block;
}

static void cell_block_set(
	struct tag_block *block,
	long count,
	void *address)
{
	block->count = count;
	block->address = address;
	block->definition = NULL;
}

static void cell_reference_set(
	struct tag_reference *reference,
	unsigned long group_tag,
	long tag_index)
{
	reference->group_tag = group_tag;
	reference->name = tag_index != NONE ? tag_get_name(tag_index) : "";
	reference->name_length = (long)strlen(reference->name);
	reference->index = tag_index;
}

static void cell_point_lerp(
	struct cell_point const *a,
	struct cell_point const *b,
	double t,
	struct cell_point *result)
{
	short index;

	for (index = 0; index < 3; index++)
	{
		result->position[index] = a->position[index] + (b->position[index] - a->position[index]) * t;
		result->normal[index] = a->normal[index] + (b->normal[index] - a->normal[index]) * t;
		result->binormal[index] = a->binormal[index] + (b->binormal[index] - a->binormal[index]) * t;
		result->tangent[index] = a->tangent[index] + (b->tangent[index] - a->tangent[index]) * t;
	}
	result->texcoord[0] = a->texcoord[0] + (b->texcoord[0] - a->texcoord[0]) * t;
	result->texcoord[1] = a->texcoord[1] + (b->texcoord[1] - a->texcoord[1]) * t;
}

/* keeps what is on the plane's side; the crossings' points to `cut` */
static short cell_clip(
	struct cell_point const *points,
	short count,
	short plane,
	struct cell_point *result,
	double cut[2][3],
	short *cut_count)
{
	short axis = cell_planes[plane].axis;
	short result_count = 0;
	short index;

	*cut_count = 0;
	for (index = 0; index < count; index++)
	{
		struct cell_point const *a = &points[index];
		struct cell_point const *b = &points[(index + 1) % count];
		double da = cell_planes[plane].sign * (a->position[axis] - cell_planes[plane].value);
		double db = cell_planes[plane].sign * (b->position[axis] - cell_planes[plane].value);

		if (da >= -1e-6 && result_count < CELL_MAXIMUM_POINTS)
			result[result_count++] = *a;
		if ((da < -1e-6) != (db < -1e-6) && result_count < CELL_MAXIMUM_POINTS)
		{
			cell_point_lerp(a, b, da / (da - db), &result[result_count]);
			result[result_count].position[axis] = cell_planes[plane].value;
			if (*cut_count < 2)
				memcpy(cut[*cut_count], result[result_count].position, sizeof(cut[0]));
			(*cut_count)++;
			result_count++;
		}
	}
	return result_count;
}

static double cell_lift(
	double z)
{
	if (z > CELL_RIM + 0.002)
		return z;
	z = z < CELL_TRAY ? CELL_TRAY : z > CELL_RIM ? CELL_RIM : z;
	return CELL_PLATE + (z - CELL_TRAY) / (CELL_RIM - CELL_TRAY) * CELL_STEP;
}

static double cell_cross(
	double const *o,
	double const *a,
	double const *b)
{
	return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
}

static double cell_area(
	double const (*points)[2],
	short count)
{
	double area = 0.0;
	short index;

	for (index = 0; index < count; index++)
	{
		double const *a = points[index];
		double const *b = points[(index + 1) % count];

		area += a[0] * b[1] - b[0] * a[1];
	}
	return area;
}

/* closed loops of the cut's segments, without points less than 2 mm apart
or in line */
static short cell_loops(
	struct cell_segment const *segments,
	short segment_count,
	double (*loops)[CELL_MAXIMUM_LOOP_POINTS][2],
	short *loop_counts,
	short maximum_loops)
{
	static double keys[CELL_MAXIMUM_SEGMENTS * 2][2];
	static short neighbours[CELL_MAXIMUM_SEGMENTS * 2][4];
	static short neighbour_counts[CELL_MAXIMUM_SEGMENTS * 2];
	static boolean seen[CELL_MAXIMUM_SEGMENTS * 2];
	short key_count = 0;
	short loop_count = 0;
	short pass;
	short index;

	for (index = 0; index < segment_count; index++)
	{
		short ends[2];
		short end;

		for (end = 0; end < 2; end++)
		{
			double key[2];
			short other;

			key[0] = floor(segments[index].ends[end][0] * 10000.0 + 0.5) / 10000.0;
			key[1] = floor(segments[index].ends[end][1] * 10000.0 + 0.5) / 10000.0;
			for (other = 0; other < key_count && (keys[other][0] != key[0] || keys[other][1] != key[1]); other++)
				;
			if (other == key_count)
			{
				keys[key_count][0] = key[0];
				keys[key_count][1] = key[1];
				neighbour_counts[key_count] = 0;
				seen[key_count] = FALSE;
				key_count++;
			}
			ends[end] = other;
		}
		if (ends[0] != ends[1])
		{
			for (end = 0; end < 2; end++)
			{
				short from = ends[end];
				short to = ends[!end];
				short other;

				for (other = 0; other < neighbour_counts[from] && neighbours[from][other] != to; other++)
					;
				if (other == neighbour_counts[from] && neighbour_counts[from] < 4)
					neighbours[from][neighbour_counts[from]++] = to;
			}
		}
	}
	for (pass = 0; pass < 2; pass++)
	{
		short start;

		for (start = 0; start < key_count; start++)
		{
			short line[CELL_MAXIMUM_LOOP_POINTS];
			short length = 0;
			short previous = NONE;

			if (seen[start] || (pass == 0 && neighbour_counts[start] != 1))
				continue;
			line[length++] = start;
			seen[start] = TRUE;
			while (length < CELL_MAXIMUM_LOOP_POINTS)
			{
				short last = line[length - 1];
				short next = NONE;

				for (index = 0; index < neighbour_counts[last] && next == NONE; index++)
				{
					short candidate = neighbours[last][index];

					if (candidate != previous && (!seen[candidate] || (candidate == start && length > 2)))
						next = candidate;
				}
				if (next == NONE)
					break;
				previous = last;
				line[length++] = next;
				if (next == start)
					break;
				seen[next] = TRUE;
			}
			if (length >= 4 && line[length - 1] == start && loop_count < maximum_loops)
			{
				double (*loop)[2] = loops[loop_count];
				short count = 0;
				short kept = 0;

				for (index = 0; index < length - 1; index++)
				{
					double const *point = keys[line[index]];
					double const *before = keys[line[index ? index - 1 : length - 1]];
					double dx = point[0] - before[0];
					double dy = point[1] - before[1];

					if (dx * dx + dy * dy > 2e-3 * 2e-3)
					{
						loop[count][0] = point[0];
						loop[count][1] = point[1];
						count++;
					}
				}
				for (index = 0; index < count; index++)
				{
					if (fabs(cell_cross(loop[(index + count - 1) % count], loop[index], loop[(index + 1) % count])) > 1e-6)
					{
						loop[kept][0] = loop[index][0];
						loop[kept][1] = loop[index][1];
						kept++;
					}
				}
				loop_counts[loop_count++] = kept;
			}
		}
	}
	return loop_count;
}

/* triangles (index triples) of a simple counterclockwise polygon */
static short cell_ear_clip(
	double const (*points)[2],
	short count,
	short (*triangles)[3])
{
	short indices[CELL_MAXIMUM_LOOP_POINTS];
	short triangle_count = 0;
	short index;

	for (index = 0; index < count; index++)
		indices[index] = index;
	while (count > 3)
	{
		boolean clipped = FALSE;

		for (index = 0; index < count && !clipped; index++)
		{
			short a = indices[(index + count - 1) % count];
			short b = indices[index];
			short c = indices[(index + 1) % count];
			boolean inside = FALSE;
			short other;

			if (cell_cross(points[a], points[b], points[c]) <= 1e-9)
				continue;
			for (other = 0; other < count && !inside; other++)
			{
				short p = indices[other];

				inside = p != a && p != b && p != c &&
					cell_cross(points[a], points[b], points[p]) >= 0.0 &&
					cell_cross(points[b], points[c], points[p]) >= 0.0 &&
					cell_cross(points[c], points[a], points[p]) >= 0.0;
			}
			if (inside)
				continue;
			triangles[triangle_count][0] = a;
			triangles[triangle_count][1] = b;
			triangles[triangle_count][2] = c;
			triangle_count++;
			memmove(&indices[index], &indices[index + 1], (count - index - 1) * sizeof(indices[0]));
			count--;
			clipped = TRUE;
		}
		if (!clipped)
			return NONE;
	}
	triangles[triangle_count][0] = indices[0];
	triangles[triangle_count][1] = indices[1];
	triangles[triangle_count][2] = indices[2];
	return triangle_count + 1;
}

static void cell_made_point(
	double x,
	double y,
	double z,
	double const *normal,
	short u_axis,
	short v_axis,
	struct cell_point *point)
{
	memset(point, 0, sizeof(*point));
	point->position[0] = x;
	point->position[1] = y;
	point->position[2] = z;
	memcpy(point->normal, normal, sizeof(point->normal));
	point->texcoord[0] = point->position[u_axis] * 2.0;
	point->texcoord[1] = point->position[v_axis] * 2.0;
}

/* the uv of `position` by the affine map of triangle `t` */
static void cell_uv_map(
	struct cell_point const *t,
	double const *position,
	double *texcoord)
{
	double e1[3], e2[3], e3[3], d[3];
	double determinant, s, r;
	short index;

	for (index = 0; index < 3; index++)
	{
		e1[index] = t[1].position[index] - t[0].position[index];
		e2[index] = t[2].position[index] - t[0].position[index];
		d[index] = position[index] - t[0].position[index];
	}
	e3[0] = e1[1] * e2[2] - e1[2] * e2[1];
	e3[1] = e1[2] * e2[0] - e1[0] * e2[2];
	e3[2] = e1[0] * e2[1] - e1[1] * e2[0];
	determinant = e1[0] * (e2[1] * e3[2] - e2[2] * e3[1]) - e2[0] * (e1[1] * e3[2] - e1[2] * e3[1]) +
		e3[0] * (e1[1] * e2[2] - e1[2] * e2[1]);
	s = (d[0] * (e2[1] * e3[2] - e2[2] * e3[1]) - e2[0] * (d[1] * e3[2] - d[2] * e3[1]) +
		e3[0] * (d[1] * e2[2] - d[2] * e2[1])) / determinant;
	r = (e1[0] * (d[1] * e3[2] - d[2] * e3[1]) - d[0] * (e1[1] * e3[2] - e1[2] * e3[1]) +
		e3[0] * (e1[1] * d[2] - e1[2] * d[1])) / determinant;
	texcoord[0] = t[0].texcoord[0] + s * (t[1].texcoord[0] - t[0].texcoord[0]) + r * (t[2].texcoord[0] - t[0].texcoord[0]);
	texcoord[1] = t[0].texcoord[1] + s * (t[1].texcoord[1] - t[0].texcoord[1]) + r * (t[2].texcoord[1] - t[0].texcoord[1]);
}

static boolean cell_same_point(
	double const *a,
	double const *b)
{
	return floor(a[0] * 10000.0 + 0.5) == floor(b[0] * 10000.0 + 0.5) &&
		floor(a[1] * 10000.0 + 0.5) == floor(b[1] * 10000.0 + 0.5) &&
		floor(a[2] * 10000.0 + 0.5) == floor(b[2] * 10000.0 + 0.5);
}

/* the strip's mitred front corners take the texture of their neighbour */
static void cell_square_corners(
	struct cell_triangle *triangles,
	short count,
	long strip_shader_index)
{
	boolean corner[CELL_MAXIMUM_TRIANGLES];
	short index;
	short side;

	for (index = 0; index < count; index++)
	{
		struct cell_triangle const *t = &triangles[index];
		short point;

		corner[index] = t->shader_index == strip_shader_index;
		for (point = 0; point < 3; point++)
		{
			corner[index] = corner[index] && fabs(t->points[point].position[2] - (CELL_PLATE + CELL_STEP)) < 1e-3 &&
				t->points[point].position[0] > 1.0;
		}
	}
	for (side = -1; side <= 1; side += 2)
	{
		boolean mitred[CELL_MAXIMUM_TRIANGLES];

		for (index = 0; index < count; index++)
		{
			short point;

			mitred[index] = corner[index];
			for (point = 0; point < 3; point++)
				mitred[index] = mitred[index] && side * triangles[index].points[point].position[1] > 0.64;
		}
		for (index = 0; index < count; index++)
		{
			short other;

			if (!mitred[index])
				continue;
			for (other = 0; other < count; other++)
			{
				short shared = 0;
				short a, b;

				if (other == index || mitred[other] || !corner[other])
					continue;
				for (a = 0; a < 3; a++)
				{
					for (b = 0; b < 3; b++)
						shared += cell_same_point(triangles[index].points[a].position, triangles[other].points[b].position);
				}
				if (shared == 2)
				{
					for (a = 0; a < 3; a++)
						cell_uv_map(triangles[other].points, triangles[index].points[a].position, triangles[index].points[a].texcoord);
					break;
				}
			}
		}
	}
}

static void cell_normalize(
	double *v)
{
	double length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	if (length > 0.0)
	{
		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	}
}

/* tangent along u, binormal along v (as the level's) */
static void cell_tangents(
	struct cell_triangle *t)
{
	double dp1[3], dp2[3], tangent[3], binormal[3];
	double du1 = t->points[1].texcoord[0] - t->points[0].texcoord[0];
	double dv1 = t->points[1].texcoord[1] - t->points[0].texcoord[1];
	double du2 = t->points[2].texcoord[0] - t->points[0].texcoord[0];
	double dv2 = t->points[2].texcoord[1] - t->points[0].texcoord[1];
	double determinant = du1 * dv2 - du2 * dv1;
	short index;

	if (fabs(determinant) < 1e-12)
		return;
	for (index = 0; index < 3; index++)
	{
		dp1[index] = t->points[1].position[index] - t->points[0].position[index];
		dp2[index] = t->points[2].position[index] - t->points[0].position[index];
		tangent[index] = (dp1[index] * dv2 - dp2[index] * dv1) / determinant;
		binormal[index] = (dp2[index] * du1 - dp1[index] * du2) / determinant;
	}
	cell_normalize(tangent);
	cell_normalize(binormal);
	for (index = 0; index < 3; index++)
	{
		memcpy(t->points[index].tangent, tangent, sizeof(tangent));
		memcpy(t->points[index].binormal, binormal, sizeof(binormal));
	}
}

/* the cut of a10a's rendered triangles in the box, the plate lifted,
capped, in the cell's frame */
static short cell_cut(
	struct structure_bsp const *bsp,
	struct cell_triangle *triangles)
{
	static struct cell_polygon polygons[CELL_MAXIMUM_POLYGONS];
	static struct cell_segment segments[2][CELL_MAXIMUM_SEGMENTS];
	static double loops[16][CELL_MAXIMUM_LOOP_POINTS][2];
	short segment_counts[2] = { 0, 0 };
	short loop_counts[16];
	short polygon_count = 0;
	short triangle_count = 0;
	long cap_shader_index = NONE;
	long edge_shader_index = NONE;
	long strip_shader_index = NONE;
	double low[3], high[3];
	short lightmap_index;
	short index;

	low[0] = CELL_X_BACK - 0.01;
	low[1] = -CELL_Y_HALF - 0.01;
	low[2] = CELL_Z_LOW - 0.01;
	high[0] = CELL_X_FRONT + 0.01;
	high[1] = CELL_Y_HALF + 0.01;
	high[2] = CELL_Z_TOP + 0.01;
	for (lightmap_index = 0; lightmap_index < bsp->lightmaps.count; lightmap_index++)
	{
		struct structure_lightmap const *lightmap = TAG_BLOCK_GET_ELEMENT(&bsp->lightmaps, lightmap_index,
			struct structure_lightmap);
		short material_index;

		for (material_index = 0; material_index < lightmap->materials.count; material_index++)
		{
			struct structure_material const *material = TAG_BLOCK_GET_ELEMENT(&lightmap->materials, material_index,
				struct structure_material);
			struct environment_vertex_compressed const *vertices = material->compressed_vertex_data.address;
			char const *name = material->shader.index != NONE ? tag_get_name(material->shader.index) : "";
			long surface_index;

			if (!csstrcasecmp(name, CELL_CAP_SHADER))
				cap_shader_index = material->shader.index;
			else if (!csstrcasecmp(name, CELL_EDGE_SHADER))
				edge_shader_index = material->shader.index;
			if (!vertices || material->vertices.type != _rasterizer_vertex_type_environment_compressed ||
				material->compressed_vertex_data.size < material->vertices.count * (long)sizeof(*vertices))
			{
				continue;
			}
			for (surface_index = material->first_surface_index;
				surface_index < material->first_surface_index + material->surface_count &&
				surface_index < bsp->surfaces.count;
				surface_index++)
			{
				struct structure_surface const *surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index,
					struct structure_surface);
				struct cell_point points[CELL_MAXIMUM_POINTS];
				struct cell_point clipped[CELL_MAXIMUM_POINTS];
				short count = 3;
				short plane;
				short corner;
				boolean outside = FALSE;

				for (corner = 0; corner < 3; corner++)
				{
					/* (BSP triangles wind against their normals) */
					word vertex_index = surface->vertex_indices[2 - corner];
					struct environment_vertex_compressed const *vertex = &vertices[vertex_index];
					real_vector3d vector;

					if (vertex_index >= material->vertices.count)
					{
						outside = TRUE;
						break;
					}
					points[corner].position[0] = vertex->position.x;
					points[corner].position[1] = vertex->position.y;
					points[corner].position[2] = vertex->position.z;
					vector = uncompress_int32_to_real_vector3d(vertex->normal);
					points[corner].normal[0] = vector.i;
					points[corner].normal[1] = vector.j;
					points[corner].normal[2] = vector.k;
					vector = uncompress_int32_to_real_vector3d(vertex->binormal);
					points[corner].binormal[0] = vector.i;
					points[corner].binormal[1] = vector.j;
					points[corner].binormal[2] = vector.k;
					vector = uncompress_int32_to_real_vector3d(vertex->tangent);
					points[corner].tangent[0] = vector.i;
					points[corner].tangent[1] = vector.j;
					points[corner].tangent[2] = vector.k;
					points[corner].texcoord[0] = vertex->texcoord.x;
					points[corner].texcoord[1] = vertex->texcoord.y;
				}
				if (outside)
					continue;
				for (plane = 0; plane < 3 && !outside; plane++)
				{
					double minimum = points[0].position[plane];
					double maximum = points[0].position[plane];

					for (corner = 1; corner < 3; corner++)
					{
						minimum = MIN(minimum, points[corner].position[plane]);
						maximum = MAX(maximum, points[corner].position[plane]);
					}
					outside = maximum < low[plane] || minimum > high[plane];
				}
				if (outside)
					continue;
				for (plane = 0; plane < NUMBER_OF_CELL_PLANES && count >= 3; plane++)
				{
					double cut[2][3];
					short cut_count;
					short record = plane == _cell_plane_back ? 0 : plane == _cell_plane_top ? 1 : NONE;

					count = cell_clip(points, count, plane, clipped, cut, &cut_count);
					memcpy(points, clipped, count * sizeof(points[0]));
					if (cut_count == 2 && record != NONE && segment_counts[record] < CELL_MAXIMUM_SEGMENTS)
					{
						struct cell_segment *segment = &segments[record][segment_counts[record]++];

						segment->ends[0][0] = record ? cut[0][0] : cut[0][1];
						segment->ends[0][1] = record ? cut[0][1] : cut[0][2];
						segment->ends[1][0] = record ? cut[1][0] : cut[1][1];
						segment->ends[1][1] = record ? cut[1][1] : cut[1][2];
					}
				}
				if (count < 3 || polygon_count >= CELL_MAXIMUM_POLYGONS)
					continue;
				{
					double minimum = points[0].position[2], maximum = points[0].position[2];
					double mean_y = 0.0, mean_z = 0.0;

					for (corner = 0; corner < count; corner++)
					{
						minimum = MIN(minimum, points[corner].position[2]);
						maximum = MAX(maximum, points[corner].position[2]);
						mean_y += points[corner].position[1] / count;
						mean_z += points[corner].position[2] / count;
					}
					/* (the bay floor beside the platform) */
					if (maximum - minimum < 2e-3 && mean_z < 2e-3 && fabs(mean_y) > CELL_PLATFORM_Y)
						continue;
				}
				for (corner = 0; corner < count; corner++)
					points[corner].position[2] = cell_lift(points[corner].position[2]);
				polygons[polygon_count].shader_index = material->shader.index;
				polygons[polygon_count].made = FALSE;
				polygons[polygon_count].count = count;
				memcpy(polygons[polygon_count].points, points, count * sizeof(points[0]));
				polygon_count++;
				if (!csstrcasecmp(name, CELL_STRIP_SHADER))
					strip_shader_index = material->shader.index;
			}
		}
	}
	if (cap_shader_index == NONE || edge_shader_index == NONE)
		return 0;

	/* the plinth's section at the back, seen from behind */
	{
		short loop_count = cell_loops(segments[0], segment_counts[0], loops, loop_counts, NUMBEROF(loop_counts));
		short loop_index;
		static double const back_normal[3] = { -1.0, 0.0, 0.0 };

		for (loop_index = 0; loop_index < loop_count; loop_index++)
		{
			double view[CELL_MAXIMUM_LOOP_POINTS][2];
			short clipped[CELL_MAXIMUM_LOOP_POINTS][3];
			short count = loop_counts[loop_index];
			short clipped_count;

			for (index = 0; index < count; index++)
			{
				view[index][0] = -loops[loop_index][index][0];
				view[index][1] = loops[loop_index][index][1];
			}
			if (cell_area(view, count) <= 0.0)
			{
				for (index = 0; index < count / 2; index++)
				{
					double swap[2];

					memcpy(swap, loops[loop_index][index], sizeof(swap));
					memcpy(loops[loop_index][index], loops[loop_index][count - 1 - index], sizeof(swap));
					memcpy(loops[loop_index][count - 1 - index], swap, sizeof(swap));
					memcpy(swap, view[index], sizeof(swap));
					memcpy(view[index], view[count - 1 - index], sizeof(swap));
					memcpy(view[count - 1 - index], swap, sizeof(swap));
				}
			}
			clipped_count = cell_ear_clip(view, count, clipped);
			for (index = 0; index < clipped_count && polygon_count < CELL_MAXIMUM_POLYGONS; index++)
			{
				struct cell_polygon *polygon = &polygons[polygon_count++];
				short corner;

				polygon->shader_index = cap_shader_index;
				polygon->made = TRUE;
				polygon->count = 3;
				for (corner = 0; corner < 3; corner++)
				{
					double const *point = loops[loop_index][clipped[index][corner]];

					cell_made_point(CELL_X_BACK, point[0], point[1], back_normal, 1, 2, &polygon->points[corner]);
				}
			}
		}
	}
	/* the columns' sections on top */
	{
		short loop_count = cell_loops(segments[1], segment_counts[1], loops, loop_counts, NUMBEROF(loop_counts));
		short loop_index;
		static double const top_normal[3] = { 0.0, 0.0, 1.0 };

		for (loop_index = 0; loop_index < loop_count && polygon_count < CELL_MAXIMUM_POLYGONS; loop_index++)
		{
			struct cell_polygon *polygon = &polygons[polygon_count++];
			short count = MIN(loop_counts[loop_index], CELL_MAXIMUM_POINTS);
			boolean reverse = cell_area(loops[loop_index], loop_counts[loop_index]) <= 0.0;

			polygon->shader_index = cap_shader_index;
			polygon->made = TRUE;
			polygon->count = count;
			for (index = 0; index < count; index++)
			{
				double const *point = loops[loop_index][reverse ? count - 1 - index : index];

				cell_made_point(point[0], point[1], CELL_Z_TOP, top_normal, 0, 1, &polygon->points[index]);
			}
		}
	}
	/* the plate's edge: its front and both sides */
	if (polygon_count + 3 <= CELL_MAXIMUM_POLYGONS)
	{
		static double const front_normal[3] = { 1.0, 0.0, 0.0 };
		static double const left_normal[3] = { 0.0, 1.0, 0.0 };
		static double const right_normal[3] = { 0.0, -1.0, 0.0 };
		double z0 = 0.0, z1 = CELL_PLATE + CELL_STEP, y = CELL_PLATFORM_Y;
		struct cell_polygon *front = &polygons[polygon_count++];
		struct cell_polygon *left = &polygons[polygon_count++];
		struct cell_polygon *right = &polygons[polygon_count++];

		front->count = left->count = right->count = 4;
		front->shader_index = left->shader_index = right->shader_index = edge_shader_index;
		front->made = left->made = right->made = TRUE;
		cell_made_point(CELL_X_FRONT, -y, z0, front_normal, 1, 2, &front->points[0]);
		cell_made_point(CELL_X_FRONT, y, z0, front_normal, 1, 2, &front->points[1]);
		cell_made_point(CELL_X_FRONT, y, z1, front_normal, 1, 2, &front->points[2]);
		cell_made_point(CELL_X_FRONT, -y, z1, front_normal, 1, 2, &front->points[3]);
		cell_made_point(CELL_X_BACK, y, z0, left_normal, 0, 2, &left->points[0]);
		cell_made_point(CELL_X_BACK, y, z1, left_normal, 0, 2, &left->points[1]);
		cell_made_point(CELL_X_FRONT, y, z1, left_normal, 0, 2, &left->points[2]);
		cell_made_point(CELL_X_FRONT, y, z0, left_normal, 0, 2, &left->points[3]);
		cell_made_point(CELL_X_BACK, -y, z0, right_normal, 0, 2, &right->points[0]);
		cell_made_point(CELL_X_FRONT, -y, z0, right_normal, 0, 2, &right->points[1]);
		cell_made_point(CELL_X_FRONT, -y, z1, right_normal, 0, 2, &right->points[2]);
		cell_made_point(CELL_X_BACK, -y, z1, right_normal, 0, 2, &right->points[3]);
	}

	/* fans, in the cell's frame */
	for (index = 0; index < polygon_count; index++)
	{
		struct cell_polygon const *polygon = &polygons[index];
		short fan;

		for (fan = 1; fan < polygon->count - 1 && triangle_count < CELL_MAXIMUM_TRIANGLES; fan++)
		{
			struct cell_triangle *t = &triangles[triangle_count];
			double e1[3], e2[3], c[3];
			short corner;

			t->shader_index = polygon->shader_index;
			t->made = polygon->made;
			t->points[0] = polygon->points[0];
			t->points[1] = polygon->points[fan];
			t->points[2] = polygon->points[fan + 1];
			for (corner = 0; corner < 3; corner++)
				t->points[corner].position[0] -= CELL_ORIGIN_X + CELL_SHIFT;
			for (corner = 0; corner < 3; corner++)
			{
				e1[corner] = t->points[1].position[corner] - t->points[0].position[corner];
				e2[corner] = t->points[2].position[corner] - t->points[0].position[corner];
			}
			c[0] = e1[1] * e2[2] - e1[2] * e2[1];
			c[1] = e1[2] * e2[0] - e1[0] * e2[2];
			c[2] = e1[0] * e2[1] - e1[1] * e2[0];
			if (sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]) > 1e-7)
				triangle_count++;
		}
	}
	cell_square_corners(triangles, triangle_count, strip_shader_index);
	for (index = 0; index < triangle_count; index++)
	{
		if (triangles[index].made)
			cell_tangents(&triangles[index]);
	}
	return triangle_count;
}

static void cell_vertex(
	struct cell_point const *point,
	real_vector2d const *scale,
	struct model_vertex_uncompressed *vertex)
{
	double normal[3], binormal[3], tangent[3];

	memcpy(normal, point->normal, sizeof(normal));
	memcpy(binormal, point->binormal, sizeof(binormal));
	memcpy(tangent, point->tangent, sizeof(tangent));
	cell_normalize(normal);
	cell_normalize(binormal);
	cell_normalize(tangent);
	vertex->position.x = (real)point->position[0];
	vertex->position.y = (real)point->position[1];
	vertex->position.z = (real)point->position[2];
	vertex->normal.i = (real)normal[0];
	vertex->normal.j = (real)normal[1];
	vertex->normal.k = (real)normal[2];
	vertex->binormal.i = (real)binormal[0];
	vertex->binormal.j = (real)binormal[1];
	vertex->binormal.k = (real)binormal[2];
	vertex->tangent.i = (real)tangent[0];
	vertex->tangent.j = (real)tangent[1];
	vertex->tangent.k = (real)tangent[2];
	vertex->texcoord.x = (real)(point->texcoord[0] / scale->i);
	vertex->texcoord.y = (real)(point->texcoord[1] / scale->j);
	vertex->nodes[0] = 0;
	vertex->nodes[1] = 0;
	vertex->node_weights[0] = 1.0f;
	vertex->node_weights[1] = 0.0f;
}

/* one node, region and geometry, a part for each shader */
static struct model *cell_model_new(
	struct cell_triangle const *triangles,
	short triangle_count)
{
	struct model *model = cell_allocate(sizeof(struct model));
	struct model_node *node = cell_allocate(sizeof(struct model_node));
	struct model_region *region = cell_allocate(sizeof(struct model_region));
	struct model_region_permutation *permutation = cell_allocate(sizeof(struct model_region_permutation));
	struct model_geometry *geometry = cell_allocate(sizeof(struct model_geometry));
	struct model_shader_reference *shaders = cell_allocate(CELL_MAXIMUM_SHADERS * sizeof(struct model_shader_reference));
	struct model_geometry_part *parts = cell_allocate(CELL_MAXIMUM_SHADERS * sizeof(struct model_geometry_part));
	struct model_vertex_uncompressed *vertices = cell_allocate(3 * triangle_count * sizeof(struct model_vertex_uncompressed));
	struct model_vertex_compressed *compressed = cell_allocate(3 * triangle_count * sizeof(struct model_vertex_compressed));
	word *indices = cell_allocate(3 * triangle_count * sizeof(word));
	real_vector2d scale;
	short shader_count = 0;
	short index;

	if (!model || !node || !region || !permutation || !geometry || !shaders || !parts || !vertices || !compressed ||
		!indices)
	{
		return NULL;
	}
	scale.i = scale.j = 1.0f;
	for (index = 0; index < triangle_count; index++)
	{
		short corner;

		for (corner = 0; corner < 3; corner++)
		{
			scale.i = MAX(scale.i, (real)ceil(fabs(triangles[index].points[corner].texcoord[0])));
			scale.j = MAX(scale.j, (real)ceil(fabs(triangles[index].points[corner].texcoord[1])));
		}
	}

	model->base_map_scale = scale;
	strcpy(node->name, "frame");
	node->next_sibling_node_index = NONE;
	node->first_child_node_index = NONE;
	node->parent_node_index = NONE;
	node->default_rotation.w = 1.0f;
	node->runtime_default_inverse_matrix.scale = 1.0f;
	node->runtime_default_inverse_matrix.forward.i = 1.0f;
	node->runtime_default_inverse_matrix.left.j = 1.0f;
	node->runtime_default_inverse_matrix.up.k = 1.0f;
	strcpy(region->name, "__base");
	strcpy(permutation->name, "__base");
	cell_block_set(&region->permutations, 1, permutation);
	cell_block_set(&model->nodes, 1, node);
	cell_block_set(&model->regions, 1, region);
	cell_block_set(&model->geometries, 1, geometry);

	for (index = 0; index < triangle_count; index++)
	{
		short shader;

		for (shader = 0; shader < shader_count && shaders[shader].shader.index != triangles[index].shader_index; shader++)
			;
		if (shader == shader_count && shader_count < CELL_MAXIMUM_SHADERS)
		{
			cell_reference_set(&shaders[shader_count].shader, 'shdr', triangles[index].shader_index);
			shader_count++;
		}
	}
	{
		long vertex_count = 0;
		short shader;

		for (shader = 0; shader < shader_count; shader++)
		{
			struct model_geometry_part *part = &parts[shader];
			long first = vertex_count;
			real_point3d centroid = { 0.0f, 0.0f, 0.0f };

			for (index = 0; index < triangle_count; index++)
			{
				short corner;

				if (triangles[index].shader_index != shaders[shader].shader.index)
					continue;
				/* (back to the game's winding, against the normals) */
				for (corner = 0; corner < 3; corner++)
				{
					cell_vertex(&triangles[index].points[2 - corner], &scale, &vertices[vertex_count]);
					centroid.x += vertices[vertex_count].position.x;
					centroid.y += vertices[vertex_count].position.y;
					centroid.z += vertices[vertex_count].position.z;
					indices[vertex_count] = (word)(vertex_count - first);
					vertex_count++;
				}
			}
			part->shader_index = shader;
			part->previous_part_index = NONE;
			part->next_part_index = NONE;
			part->centroid_primary_node_index = 0;
			part->centroid_secondary_node_index = 0;
			part->centroid_primary_node_weight = 1.0f;
			part->centroid.x = centroid.x / (vertex_count - first);
			part->centroid.y = centroid.y / (vertex_count - first);
			part->centroid.z = centroid.z / (vertex_count - first);
			rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_model_uncompressed, vertex_count - first,
				&compressed[first], (vertex_count - first) * sizeof(*compressed), &vertices[first],
				(vertex_count - first) * sizeof(*vertices));
			if (!rasterizer_vertex_buffer_new(&part->vertex_buffer, _rasterizer_vertex_type_model_compressed,
					vertex_count - first, &compressed[first], (vertex_count - first) * sizeof(*compressed)) ||
				!rasterizer_triangle_buffer_new(&part->triangle_buffer, _triangle_buffer_type_triangles,
					(vertex_count - first) / 3, &indices[first]))
			{
				cell_globals.parts = parts;
				cell_globals.part_count = shader + 1;
				return NULL;
			}
		}
		cell_globals.parts = parts;
		cell_globals.part_count = shader_count;
	}
	cell_block_set(&geometry->parts, shader_count, parts);
	cell_block_set(&model->shaders, shader_count, shaders);
	return model;
}

/* the pieces' faces: the coplanar triangles of each, as one polygon wound
counterclockwise round its outward normal */
struct cell_face
{
	short piece;
	short count;
	short corners[CELL_MAXIMUM_FACE_POINTS];
	real_plane3d plane;
};

static short cell_faces(
	struct cell_face *faces,
	short maximum_faces)
{
	short face_count = 0;
	short piece;

	for (piece = 0; piece < CELL_PIECES; piece++)
	{
		short first_corner = cell_collision_pieces[piece][0];
		short first_triangle = cell_collision_pieces[piece][2];
		short triangle;
		short first_face = face_count;

		for (triangle = first_triangle; triangle < first_triangle + cell_collision_pieces[piece][3]; triangle++)
		{
			real_point3d points[3];
			real_plane3d plane;
			short corner;
			short face;

			for (corner = 0; corner < 3; corner++)
			{
				real const *c = cell_collision_corners[first_corner + cell_collision_triangles[triangle][corner]];

				points[corner].x = c[0];
				points[corner].y = c[1];
				points[corner].z = c[2];
			}
			if (!plane3d_from_points(&plane, &points[0], &points[1], &points[2]))
				continue;
			for (face = first_face; face < face_count; face++)
			{
				if (dot_product3d(&faces[face].plane.n, &plane.n) > 0.9999f && fabs(faces[face].plane.d - plane.d) < 1e-3f)
					break;
			}
			if (face == face_count)
			{
				if (face_count >= maximum_faces)
					return NONE;
				faces[face_count].piece = piece;
				faces[face_count].count = 0;
				faces[face_count].plane = plane;
				face_count++;
			}
			for (corner = 0; corner < 3; corner++)
			{
				short corner_index = first_corner + cell_collision_triangles[triangle][corner];
				short other;

				for (other = 0; other < faces[face].count && faces[face].corners[other] != corner_index; other++)
					;
				if (other == faces[face].count && faces[face].count < CELL_MAXIMUM_FACE_POINTS)
					faces[face].corners[faces[face].count++] = corner_index;
			}
		}
		/* (each face's corners round its centre, counterclockwise seen from outside) */
		for (triangle = first_face; triangle < face_count; triangle++)
		{
			struct cell_face *face = &faces[triangle];
			real_point3d centre = { 0.0f, 0.0f, 0.0f };
			real_vector3d u, v;
			real angles[CELL_MAXIMUM_FACE_POINTS];
			short a, b;

			for (a = 0; a < face->count; a++)
			{
				centre.x += cell_collision_corners[face->corners[a]][0] / face->count;
				centre.y += cell_collision_corners[face->corners[a]][1] / face->count;
				centre.z += cell_collision_corners[face->corners[a]][2] / face->count;
			}
			if (fabs(face->plane.n.k) < 0.9f)
				set_real_vector3d(&u, -face->plane.n.j, face->plane.n.i, 0.0f);
			else
				set_real_vector3d(&u, 1.0f, 0.0f, 0.0f);
			normalize3d(&u);
			cross_product3d(&face->plane.n, &u, &v);
			for (a = 0; a < face->count; a++)
			{
				real const *c = cell_collision_corners[face->corners[a]];
				real x = (c[0] - centre.x) * u.i + (c[1] - centre.y) * u.j + (c[2] - centre.z) * u.k;
				real y = (c[0] - centre.x) * v.i + (c[1] - centre.y) * v.j + (c[2] - centre.z) * v.k;
				real d = (real)(fabs(x) + fabs(y));

				/* (a pseudo-angle, rising counterclockwise) */
				angles[a] = d == 0.0f ? 0.0f : y >= 0.0f ? (x >= 0.0f ? y / d : 2.0f - y / d) : (x < 0.0f ? 2.0f - y / d : 4.0f + y / d);
			}
			for (a = 1; a < face->count; a++)
			{
				for (b = a; b > 0 && angles[b - 1] > angles[b]; b--)
				{
					real angle = angles[b];
					short corner = face->corners[b];

					angles[b] = angles[b - 1];
					face->corners[b] = face->corners[b - 1];
					angles[b - 1] = angle;
					face->corners[b - 1] = corner;
				}
			}
		}
	}
	return face_count;
}

/* the collision bsp: separator planes between the pieces, then a node for
each of a piece's faces, its outside an empty leaf whose one reference is
that face, its inside the next face (solid past the last) */
static boolean cell_collision_bsp(
	struct collision_bsp *bsp)
{
	static struct cell_face faces[CELL_PIECES * CELL_MAXIMUM_PIECE_FACES];
	short face_count = cell_faces(faces, NUMBEROF(faces));
	short edge_capacity = face_count * CELL_MAXIMUM_FACE_POINTS;
	struct bsp3d_node *nodes;
	real_plane3d *planes;
	struct collision_leaf *leaves;
	struct bsp2d_reference *references;
	struct bsp2d_node *nodes2d;
	struct collision_surface *surfaces;
	struct collision_edge *edges;
	struct collision_vertex *vertices;
	short separator_count = NUMBEROF(cell_separators);
	short node2d_count = 0;
	short edge_count = 0;
	short face;
	short piece;
	long piece_roots[CELL_PIECES];

	if (face_count <= 0)
		return FALSE;
	nodes = cell_allocate((separator_count + face_count) * sizeof(*nodes));
	planes = cell_allocate((separator_count + face_count) * sizeof(*planes));
	leaves = cell_allocate(face_count * sizeof(*leaves));
	references = cell_allocate(face_count * sizeof(*references));
	nodes2d = cell_allocate(edge_capacity * sizeof(*nodes2d));
	surfaces = cell_allocate(face_count * sizeof(*surfaces));
	edges = cell_allocate(edge_capacity * sizeof(*edges));
	vertices = cell_allocate(NUMBEROF(cell_collision_corners) * sizeof(*vertices));
	if (!nodes || !planes || !leaves || !references || !nodes2d || !surfaces || !edges || !vertices)
		return FALSE;

	for (face = 0; face < (short)NUMBEROF(cell_collision_corners); face++)
	{
		set_real_point3d(&vertices[face].point, cell_collision_corners[face][0], cell_collision_corners[face][1],
			cell_collision_corners[face][2]);
		vertices[face].first_edge_index = NONE;
	}
	/* faces: their planes, surfaces, edges and bsp2ds */
	for (face = 0; face < face_count; face++)
	{
		struct cell_face const *f = &faces[face];
		long plane_index = separator_count + face;
		short ring[CELL_MAXIMUM_FACE_POINTS];
		real_point2d points[CELL_MAXIMUM_FACE_POINTS];
		real absolute_i = (real)fabs(f->plane.n.i), absolute_j = (real)fabs(f->plane.n.j), absolute_k = (real)fabs(f->plane.n.k);
		short projection = absolute_k >= absolute_j && absolute_k >= absolute_i ? _z : absolute_j >= absolute_i ? _y : _x;
		boolean projection_sign = projection_sign_from_vector3d(&f->plane.n, projection);
		real area = 0.0f;
		short k;

		planes[plane_index] = f->plane;
		surfaces[face].plane_designator = plane_index;
		surfaces[face].flags = 0;
		surfaces[face].breakable_surface_index = (byte)NONE;
		surfaces[face].material_index = 0;
		for (k = 0; k < f->count; k++)
		{
			short a = f->corners[k];
			short b = f->corners[(k + 1) % f->count];
			short e;

			for (e = 0; e < edge_count && !(edges[e].vertex_indices[0] == b && edges[e].vertex_indices[1] == a); e++)
				;
			if (e == edge_count)
			{
				if (edge_count >= edge_capacity)
					return FALSE;
				edges[e].vertex_indices[0] = a;
				edges[e].vertex_indices[1] = b;
				edges[e].surface_indices[0] = face;
				edges[e].surface_indices[1] = NONE;
				edges[e].edge_indices[0] = NONE;
				edges[e].edge_indices[1] = NONE;
				edge_count++;
			}
			else
			{
				edges[e].surface_indices[1] = face;
			}
			ring[k] = e;
			if (vertices[a].first_edge_index == NONE)
				vertices[a].first_edge_index = e;
		}
		for (k = 0; k < f->count; k++)
			edges[ring[k]].edge_indices[edges[ring[k]].surface_indices[1] == face] = ring[(k + 1) % f->count];
		surfaces[face].first_edge_index = ring[0];

		for (k = 0; k < f->count; k++)
			project_point3d(&vertices[f->corners[k]].point, projection, projection_sign, &points[k]);
		for (k = 0; k < f->count; k++)
		{
			real_point2d const *a = &points[k];
			real_point2d const *b = &points[(k + 1) % f->count];

			area += a->x * b->y - b->x * a->y;
		}
		references[face].plane_designator = plane_index;
		references[face].root_index = node2d_count;
		for (k = 0; k < f->count; k++)
		{
			struct bsp2d_node *node2d = &nodes2d[node2d_count];
			real_point2d const *a = &points[k];
			real_point2d const *b = &points[(k + 1) % f->count];
			real length;

			/* (inside: left of each edge of a counterclockwise outline) */
			node2d->plane.n.i = area > 0.0f ? -(b->y - a->y) : b->y - a->y;
			node2d->plane.n.j = area > 0.0f ? b->x - a->x : -(b->x - a->x);
			length = square_root(node2d->plane.n.i * node2d->plane.n.i + node2d->plane.n.j * node2d->plane.n.j);
			if (length > 0.0f)
			{
				node2d->plane.n.i /= length;
				node2d->plane.n.j /= length;
			}
			node2d->plane.d = node2d->plane.n.i * a->x + node2d->plane.n.j * a->y;
			node2d->child_indices[0] = NONE;
			node2d->child_indices[1] = k + 1 < f->count ? node2d_count + 1 : (long)(LONG_MIN | face);
			node2d_count++;
		}
		leaves[face].flags = 0;
		leaves[face].bsp2d_reference_count = 1;
		leaves[face].first_bsp2d_reference_index = face;
	}
	for (face = 0; face < edge_count; face++)
	{
		if (edges[face].surface_indices[1] == NONE)
			return FALSE;
	}

	/* each piece's chain */
	for (piece = 0; piece < CELL_PIECES; piece++)
	{
		long next = NONE;

		piece_roots[piece] = NONE;
		for (face = face_count - 1; face >= 0; face--)
		{
			struct bsp3d_node *node = &nodes[separator_count + face];

			if (faces[face].piece != piece)
				continue;
			node->plane_designator = separator_count + face;
			node->children[1] = (long)(LONG_MIN | face);
			node->children[0] = next;
			next = separator_count + face;
		}
		piece_roots[piece] = next;
	}
	for (face = 0; face < separator_count; face++)
	{
		set_real_vector3d(&planes[face].n, cell_separators[face][0], cell_separators[face][1], cell_separators[face][2]);
		planes[face].d = cell_separators[face][3];
		nodes[face].plane_designator = face;
	}
	/* z: columns above, the rest below; x: the console in front of the
	plinth; y: the columns left to right */
	nodes[0].children[1] = 2;
	nodes[0].children[0] = 1;
	nodes[1].children[1] = piece_roots[1];
	nodes[1].children[0] = piece_roots[0];
	nodes[2].children[1] = piece_roots[3];
	nodes[2].children[0] = 3;
	nodes[3].children[1] = piece_roots[2];
	nodes[3].children[0] = piece_roots[4];

	cell_block_set(&bsp->bsp3d.nodes, separator_count + face_count, nodes);
	cell_block_set(&bsp->bsp3d.planes, separator_count + face_count, planes);
	cell_block_set(&bsp->leaves, face_count, leaves);
	cell_block_set(&bsp->bsp2d_references, face_count, references);
	cell_block_set(&bsp->bsp2d.nodes, node2d_count, nodes2d);
	cell_block_set(&bsp->surfaces, face_count, surfaces);
	cell_block_set(&bsp->edges, edge_count, edges);
	cell_block_set(&bsp->vertices, NUMBEROF(cell_collision_corners), vertices);
	return TRUE;
}

static struct collision_model *cell_collision_model_new(
	real_point3d const *low,
	real_point3d const *high)
{
	struct collision_model *model = cell_allocate(sizeof(struct collision_model));
	struct damage_resistance_material *material = cell_allocate(sizeof(struct damage_resistance_material));
	struct damage_region *region = cell_allocate(sizeof(struct damage_region));
	struct pathfinding_sphere *spheres = cell_allocate(NUMBEROF(cell_pathfinding_spheres) * sizeof(*spheres));
	struct collision_node *node = cell_allocate(sizeof(struct collision_node));
	struct collision_bsp *bsp = cell_allocate(sizeof(struct collision_bsp));
	short index;

	if (!model || !material || !region || !spheres || !node || !bsp || !cell_collision_bsp(bsp))
		return NULL;
	strcpy(material->name, "steel");
	material->material_type = _material_thick_metal;
	material->shield_damage_multiplier = 1.0f;
	material->body_damage_multiplier = 1.0f;
	strcpy(region->name, "default");
	cell_reference_set(&region->destroyed_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.localized_damage_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.area_damage_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.body_damaged_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.body_depleted_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.body_destroyed_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.shield_damaged_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.shield_depleted_effect, 'effe', NONE);
	cell_reference_set(&model->resistance.shield_recharging_effect, 'effe', NONE);
	cell_block_set(&model->resistance.materials, 1, material);
	cell_block_set(&model->resistance.regions, 1, region);
	model->pathfinding_box.x0 = low->x;
	model->pathfinding_box.x1 = high->x;
	model->pathfinding_box.y0 = low->y;
	model->pathfinding_box.y1 = high->y;
	model->pathfinding_box.z0 = low->z;
	model->pathfinding_box.z1 = high->z;
	for (index = 0; index < (short)NUMBEROF(cell_pathfinding_spheres); index++)
	{
		spheres[index].node_index = 0;
		set_real_point3d(&spheres[index].center, cell_pathfinding_spheres[index][0], cell_pathfinding_spheres[index][1],
			cell_pathfinding_spheres[index][2]);
		spheres[index].radius = cell_pathfinding_spheres[index][3];
	}
	cell_block_set(&model->pathfinding_spheres, NUMBEROF(cell_pathfinding_spheres), spheres);
	strcpy(node->name, "frame");
	node->region_index = 0;
	node->parent_node_index = NONE;
	node->next_sibling_node_index = NONE;
	node->first_child_node_index = NONE;
	node->runtime_damage_part = 0;
	cell_block_set(&node->bsps, 1, bsp);
	cell_block_set(&model->nodes, 1, node);
	return model;
}

/* the tag table with the cell's model, collision model and scenery added */
static long cell_tags_add(
	struct model *model,
	struct collision_model *collision_model,
	struct object_definition *scenery)
{
	long existing_count;
	struct cache_file_tag_instance *existing = cache_files_tag_instances(&existing_count);
	struct cache_file_tag_instance *instances;
	static long const groups[CELL_NEW_TAGS][3] =
	{
		{ MODELS_GROUP_TAG, NONE, NONE }, { 'coll', NONE, NONE }, { 'scen', OBJECT_DEFINITION_TAG, NONE },
	};
	void *definitions[CELL_NEW_TAGS];
	long tag_indices[CELL_NEW_TAGS];
	long salt = 0;
	long index;

	if (!existing || existing_count < 0 || existing_count > UNSIGNED_SHORT_MAX - CELL_NEW_TAGS ||
		!(instances = cell_allocate((existing_count + CELL_NEW_TAGS) * sizeof(*instances))))
	{
		return NONE;
	}
	memcpy(instances, existing, existing_count * sizeof(*instances));
	for (index = 0; index < existing_count; index++)
		salt = MAX(salt, (long)((unsigned long)existing[index].tag_index >> 16));
	definitions[0] = model;
	definitions[1] = collision_model;
	definitions[2] = scenery;
	for (index = 0; index < CELL_NEW_TAGS; index++)
	{
		struct cache_file_tag_instance *instance = &instances[existing_count + index];

		tag_indices[index] = ((salt + 1 + index) << 16) | (existing_count + index);
		instance->group_tag = groups[index][0];
		instance->parent_group_tags[0] = groups[index][1];
		instance->parent_group_tags[1] = groups[index][2];
		instance->tag_index = tag_indices[index];
		instance->name = (char *)CELL_TAG_NAME;
		instance->base_address = definitions[index];
	}
	cell_globals.instances = instances;
	cell_globals.original_instances = existing;
	cell_globals.original_count = existing_count;
	cache_files_set_tag_instances(instances, existing_count + CELL_NEW_TAGS);

	cell_reference_set(&scenery->object.model, MODELS_GROUP_TAG, tag_indices[0]);
	cell_reference_set(&scenery->object.collision_model, 'coll', tag_indices[1]);
	return tag_indices[2];
}

static long cell_build(
	void)
{
	static struct cell_triangle triangles[CELL_MAXIMUM_TRIANGLES];
	struct structure_bsp *bsp = global_structure_bsp_get();
	short triangle_count;
	struct model *model;
	struct collision_model *collision_model;
	struct object_definition *scenery;
	real_point3d low, high, centre;
	real radius = 0.0f;
	short index;

	if (!bsp || global_structure_bsp_index_get() != 0)
		return NONE;
	triangle_count = cell_cut(bsp, triangles);
	if (triangle_count <= 0)
		return NONE;
	set_real_point3d(&low, 1e9f, 1e9f, 1e9f);
	set_real_point3d(&high, -1e9f, -1e9f, -1e9f);
	for (index = 0; index < triangle_count; index++)
	{
		short corner;

		for (corner = 0; corner < 3; corner++)
		{
			double const *p = triangles[index].points[corner].position;

			low.x = MIN(low.x, (real)p[0]);
			low.y = MIN(low.y, (real)p[1]);
			low.z = MIN(low.z, (real)p[2]);
			high.x = MAX(high.x, (real)p[0]);
			high.y = MAX(high.y, (real)p[1]);
			high.z = MAX(high.z, (real)p[2]);
		}
	}
	set_real_point3d(&centre, (low.x + high.x) / 2.0f, (low.y + high.y) / 2.0f, (low.z + high.z) / 2.0f);
	for (index = 0; index < triangle_count; index++)
	{
		short corner;

		for (corner = 0; corner < 3; corner++)
		{
			real_point3d p;

			set_real_point3d(&p, (real)triangles[index].points[corner].position[0],
				(real)triangles[index].points[corner].position[1], (real)triangles[index].points[corner].position[2]);
			radius = MAX(radius, distance3d(&p, &centre));
		}
	}
	error(_error_silent, "co-op: the cryo bay's cell is %d triangles (%d expected), %.4f %.4f %.4f to %.4f %.4f %.4f",
		triangle_count, CELL_EXPECTED_TRIANGLES, low.x, low.y, low.z, high.x, high.y, high.z);

	model = cell_model_new(triangles, triangle_count);
	collision_model = cell_collision_model_new(&low, &high);
	scenery = cell_allocate(sizeof(struct object_definition));
	if (!model || !collision_model || !scenery)
		return NONE;
	scenery->object.type = _object_type_scenery;
	scenery->object.flags = FLAG(_object_does_not_cast_shadow_bit);
	scenery->object.bounding_radius = radius + 0.01f;
	scenery->object.bounding_offset = centre;
	scenery->object.render_bounding_radius = radius + 0.01f;
	cell_reference_set(&scenery->object.animation_graph, 'antr', NONE);
	cell_reference_set(&scenery->object.physics, 'phys', NONE);
	cell_reference_set(&scenery->object.modifier_shader, 'shdr', NONE);
	cell_reference_set(&scenery->object.creation_effect, 'effe', NONE);
	return cell_tags_add(model, collision_model, scenery);
}

/* ---------- public code */

long coop_cryo_cell_definition(
	void)
{
	if (!cell_globals.tried)
	{
		cell_globals.tried = TRUE;
		cell_globals.definition_index = cell_build();
		if (cell_globals.definition_index == NONE)
			error(_error_silent, "co-op: the cryo bay's cells were not made");
	}
	return cell_globals.definition_index;
}

void coop_cryo_cell_release(
	void)
{
	long count;
	short index;

	if (cell_globals.instances && cache_files_tag_instances(&count) == cell_globals.instances)
		cache_files_set_tag_instances(cell_globals.original_instances, cell_globals.original_count);
	for (index = 0; index < cell_globals.part_count; index++)
	{
		rasterizer_vertex_buffer_delete(&cell_globals.parts[index].vertex_buffer);
		rasterizer_triangle_buffer_delete(&cell_globals.parts[index].triangle_buffer);
	}
	for (index = 0; index < cell_globals.block_count; index++)
		free(cell_globals.blocks[index]);
	memset(&cell_globals, 0, sizeof(cell_globals));
	cell_globals.definition_index = NONE;
}
