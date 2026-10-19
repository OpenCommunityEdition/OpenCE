/*
STRUCTURE_CULLING.C

Six-plane rejection and conservative facing summaries for structure buckets.
*/

/* ---------- headers */

#include "structure_culling.h"
#include "cseries/cseries_windows.h"
#include "cseries/errors.h"
#include "cseries/profile.h"
#include "interface/interface.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "shaders/shader_definitions.h"
#include "rasterizer/rasterizer_geometry_environment.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "render/render_cameras_internal.h"
#include "render/render_debug.h"
#include "render/render_debug_geometry.h"
#include "structures/structure_visibility.h"
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ---------- constants */

/* the renderer discards positive (edge0 cross edge1) dot (camera minus point0) with D3DCULL_CCW */
#define STRUCTURE_CULLING_FRONT_SIGN -1.0f
#define STRUCTURE_CULLING_MAXIMUM_BUCKET_SURFACES 64
#define STRUCTURE_CULLING_LOOSE_BUCKET_SIZE 8.0f
#define STRUCTURE_CULLING_CONE_SPLIT_CUTOFF 0.5f
#define STRUCTURE_CULLING_BUILD_BUDGET (64UL * 1024 * 1024)
#define STRUCTURE_CULLING_MAXIMUM_DEBUG_TRIANGLES 6144

enum
{
	COMPRESSED_STRUCTURE_VERTEX_SIZE = 0x20,
	_shader_type_environment = 3,
	_structure_culling_frustum_plane_far = 5
};

/* ---------- structures */

/* copies of structure_visibility.c's private cluster and subcluster tag block views */
struct structure_visibility_cluster
{
	short sky_index;
	short fog_designator;
	short background_sound_palette_index;
	short sound_environment_palette_index;
	short weather_palette_index;
	short transition_structure_bsp_index;
	short first_runtime_decal_index;
	word runtime_decal_count;
	long unused[6];
	struct tag_block predicted_resources;
	struct tag_block subclusters;
	word first_lens_flare_marker_index;
	word lens_flare_marker_count;
	struct tag_block surface_indices;
	struct tag_block mirrors;
	struct tag_block portal_indices;
};

struct structure_visibility_subcluster
{
	real_rectangle3d world_bounds;
	struct tag_block surface_indices;
};

typedef char structure_visibility_cluster_size_assert[
	sizeof(struct structure_visibility_cluster) == 0x68 ? 1 : -1];
typedef char structure_visibility_subcluster_size_assert[
	sizeof(struct structure_visibility_subcluster) == 0x24 ? 1 : -1];

struct structure_culling_material
{
	byte const *vertices;
	long vertex_count;
	boolean facing_eligible;
};

struct structure_culling_bucket
{
	struct structure_culling_cone cone;
	real_rectangle3d bounds;
	real_rectangle3d const *source_bounds;
	long first_pairing;
	long pairing_count;
	boolean facing_eligible;
};

struct structure_culling_cluster
{
	real_rectangle3d bounds;
	long first_bucket;
	long bucket_count;
};

struct structure_culling_data
{
	void *allocation;
	struct structure_bsp *structure;
	void *surface_address;
	long surface_count;
	struct structure_culling_material *materials;
	struct structure_culling_cluster *clusters;
	struct structure_culling_bucket *buckets;
	struct structure_culling_pairing *pairings;
	long material_count;
	long bucket_count;
	long pairing_count;
	long bytes;
	long split_count;
	long disagree_count;
	long triangle_count;
	unsigned long build_milliseconds;
	boolean from_subclusters;
};

struct structure_culling_workspace
{
	short *owners;
	long *vertex_counts;
	long *lightmap_first;
	struct structure_culling_split_surface *surfaces;
	struct structure_culling_pairing *pairings;
	long *ends;
	real_point3d *points;
};

/* ---------- globals */

static short structure_culling_split_axis;
static struct structure_culling_data structure_culling;
static long structure_culling_tests = FLAG(_structure_culling_triangle_frustum_bit) |
	FLAG(_structure_culling_bucket_facing_bit) | FLAG(_structure_culling_triangle_facing_bit) |
	FLAG(_structure_culling_finer_buckets_bit) | FLAG(_structure_culling_near_first_bit);
static long structure_culling_submitted;
static long structure_culling_outside;
static long structure_culling_facing;
static boolean structure_culling_debug_enabled;
static unsigned long structure_culling_recorded[2][BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_SURFACES_PER_STRUCTURE)];

/* ---------- prototypes */

boolean tag_index_is_group(long tag_index, long group_tag);
long config_integer(char const *name); /* port/linux/src/port_config.c */
static void structure_culling_pairing_points(
	struct structure_bsp *structure, struct structure_culling_pairing const *pairing, real_point3d *points);

/* ---------- private code */

static short structure_culling_split_class(struct structure_culling_split_surface const *surface)
{
	real const *normal = &surface->normal.i;
	short axis = 0;
	short i;

	for (i = 1; i < 3; ++i)
	{
		if ((normal[i] < 0 ? -normal[i] : normal[i]) > (normal[axis] < 0 ? -normal[axis] : normal[axis]))
		{
			axis = i;
		}
	}
	return axis * 2 + (normal[axis] < 0.0f);
}

static int structure_culling_split_compare(void const *a, void const *b)
{
	struct structure_culling_split_surface const *left = a;
	struct structure_culling_split_surface const *right = b;
	real x = (&left->centroid.x)[structure_culling_split_axis];
	real y = (&right->centroid.x)[structure_culling_split_axis];

	return (x > y) - (x < y);
}

static void structure_culling_split_range(struct structure_culling_split_surface *surfaces, long first, long count,
	long *ends, long *bucket_count, boolean allow_facing)
{
	long i;
	long j;
	long eligible_count = 0;
	real_point3d low;
	real_point3d high;
	real_vector3d sum = {0, 0, 0};
	real length;
	real cutoff = 1.0f;
	short axis = 0;
	short facing_class;
	struct structure_culling_split_surface swap;

	for (i = first; i < first + count; ++i)
	{
		if (surfaces[i].facing_eligible)
		{
			++eligible_count;
		}
	}
	if (eligible_count && eligible_count < count)
	{
		j = first;
		for (i = first; i < first + count; ++i)
		{
			if (surfaces[i].facing_eligible)
			{
				swap = surfaces[j];
				surfaces[j++] = surfaces[i];
				surfaces[i] = swap;
			}
		}
		structure_culling_split_range(surfaces, first, eligible_count, ends, bucket_count, allow_facing);
		structure_culling_split_range(surfaces, j, count - eligible_count, ends, bucket_count, allow_facing);
		return;
	}
	low = high = surfaces[first].centroid;
	for (i = first; i < first + count; ++i)
	{
		for (j = 0; j < 3; ++j)
		{
			(&low.x)[j] = MIN((&low.x)[j], (&surfaces[i].centroid.x)[j]);
			(&high.x)[j] = MAX((&high.x)[j], (&surfaces[i].centroid.x)[j]);
			(&sum.i)[j] += (&surfaces[i].normal.i)[j];
		}
	}
	length = square_root(magnitude_squared3d(&sum));
	if (length > 1.0e-10f)
	{
		scale_vector3d(&sum, 1.0f / length, &sum);
		for (i = first; i < first + count; ++i)
		{
			cutoff = MIN(cutoff, dot_product3d(&sum, &surfaces[i].normal));
		}
	}
	else
	{
		cutoff = -1.0f;
	}
	if (allow_facing && cutoff < STRUCTURE_CULLING_CONE_SPLIT_CUTOFF)
	{
		j = first;
		for (facing_class = 0; facing_class < 6; ++facing_class)
		{
			long start = j;

			for (i = j; i < first + count; ++i)
			{
				if (structure_culling_split_class(&surfaces[i]) == facing_class)
				{
					swap = surfaces[j];
					surfaces[j++] = surfaces[i];
					surfaces[i] = swap;
				}
			}
			if (j > start)
			{
				structure_culling_split_range(surfaces, start, j - start, ends, bucket_count, FALSE);
			}
		}
		return;
	}
	for (i = 1; i < 3; ++i)
	{
		if ((&high.x)[i] - (&low.x)[i] > (&high.x)[axis] - (&low.x)[axis])
		{
			axis = i;
		}
	}
	if (count > STRUCTURE_CULLING_MAXIMUM_BUCKET_SURFACES ||
		(count > 8 && (&high.x)[axis] - (&low.x)[axis] > STRUCTURE_CULLING_LOOSE_BUCKET_SIZE))
	{
		structure_culling_split_axis = axis;
		qsort(surfaces + first, count, sizeof(*surfaces), structure_culling_split_compare);
		structure_culling_split_range(surfaces, first, count / 2, ends, bucket_count, FALSE);
		structure_culling_split_range(surfaces, first + count / 2, count - count / 2, ends, bucket_count, FALSE);
		return;
	}
	ends[(*bucket_count)++] = first + count;
}

static boolean structure_culling_data_matches(struct structure_bsp const *structure)
{
	return structure_culling.allocation && structure && structure_culling.structure == structure &&
		structure_culling.surface_address == structure->surfaces.address &&
		structure_culling.surface_count == structure->surfaces.count;
}

/* a surface drawn through another cluster wins over either rejection reason */
static void structure_culling_record(long surface_index, short reason)
{
	short i;
	boolean counted;

	if (!structure_culling_debug_enabled)
	{
		return;
	}
	counted = BIT_VECTOR_TEST_FLAG(structure_culling_recorded[0], surface_index) ||
		BIT_VECTOR_TEST_FLAG(structure_culling_recorded[1], surface_index);
	for (i = 0; i < 2; ++i)
	{
		long *count = i ? &structure_culling_facing : &structure_culling_outside;
		boolean old = BIT_VECTOR_TEST_FLAG(structure_culling_recorded[i], surface_index);
		boolean keep = reason && (old || (!counted && reason == i + 1));

		*count += keep - old;
		BIT_VECTOR_SET_FLAG(structure_culling_recorded[i], surface_index, keep);
	}
}

static void structure_culling_pairing_points(
	struct structure_bsp *structure, struct structure_culling_pairing const *pairing, real_point3d *points)
{
	struct structure_surface const *surface =
		TAG_BLOCK_GET_ELEMENT(&structure->surfaces, pairing->surface_index, struct structure_surface);
	struct structure_culling_material const *material = &structure_culling.materials[pairing->material_index];
	short i;

	for (i = 0; i < 3; ++i)
	{
		points[i] =
			*(real_point3d const *)(material->vertices + surface->vertex_indices[i] * COMPRESSED_STRUCTURE_VERTEX_SIZE);
	}
}

static void structure_culling_bounds_add(real_rectangle3d *bounds, real_point3d const *point)
{
	bounds->x0 = MIN(bounds->x0, point->x);
	bounds->x1 = MAX(bounds->x1, point->x);
	bounds->y0 = MIN(bounds->y0, point->y);
	bounds->y1 = MAX(bounds->y1, point->y);
	bounds->z0 = MIN(bounds->z0, point->z);
	bounds->z1 = MAX(bounds->z1, point->z);
}

static void structure_culling_build_group(struct structure_bsp *structure, struct structure_culling_workspace *work,
	long const *list, long list_count, boolean from_surface_list, short group_material,
	real_rectangle3d const *parent_bounds)
{
	long count;
	long i;
	long j;
	long bucket_count;
	long first;
	long eligible_count = 0;

	count = structure_culling_gather_pairings(list, list_count, from_surface_list, group_material,
		structure->surfaces.count, work->owners, work->vertex_counts, (word const *)structure->surfaces.address,
		work->pairings);
	if (!count)
	{
		return;
	}
	for (i = 0; i < count; ++i)
	{
		struct structure_culling_pairing const *pairing = &work->pairings[i];
		struct structure_culling_split_surface *surface = &work->surfaces[i];
		real_vector3d edge0;
		real_vector3d edge1;
		real squared;

		memset(surface, 0, sizeof(*surface));
		/* an ordinal preserves distinct pairings of the same surface while splitting */
		surface->surface_index = i;
		surface->centroid.x = (parent_bounds->x0 + parent_bounds->x1) * 0.5f;
		surface->centroid.y = (parent_bounds->y0 + parent_bounds->y1) * 0.5f;
		surface->centroid.z = (parent_bounds->z0 + parent_bounds->z1) * 0.5f;
		if (!pairing->testable)
		{
			continue;
		}
		structure_culling_pairing_points(structure, pairing, work->points);
		for (j = 0; j < 3; ++j)
		{
			(&surface->centroid.x)[j] =
				((&work->points[0].x)[j] + (&work->points[1].x)[j] + (&work->points[2].x)[j]) / 3.0f;
		}
		vector_from_points3d(work->points, work->points + 1, &edge0);
		vector_from_points3d(work->points, work->points + 2, &edge1);
		cross_product3d(&edge0, &edge1, &surface->normal);
		squared = magnitude_squared3d(&surface->normal);
		if (squared > 1.0e-20f && squared <= FLT_MAX)
		{
			scale_vector3d(&surface->normal, STRUCTURE_CULLING_FRONT_SIGN / square_root(squared), &surface->normal);
		}
		else
		{
			memset(&surface->normal, 0, sizeof(surface->normal));
		}
		surface->facing_eligible = structure_culling.materials[pairing->material_index].facing_eligible;
		if (surface->facing_eligible)
		{
			real_vector3d mean = {0, 0, 0};
			real_vector3d normal;
			struct structure_surface const *triangle =
				TAG_BLOCK_GET_ELEMENT(&structure->surfaces, pairing->surface_index, struct structure_surface);
			byte const *vertices = structure_culling.materials[pairing->material_index].vertices;

			for (j = 0; j < 3; ++j)
			{
				environment_vertex_compressed_get_normal(
					(struct environment_vertex_compressed const *)(vertices +
						triangle->vertex_indices[j] * COMPRESSED_STRUCTURE_VERTEX_SIZE),
					&normal);
				mean.i += normal.i;
				mean.j += normal.j;
				mean.k += normal.k;
			}
			++structure_culling.triangle_count;
			if (dot_product3d(&surface->normal, &mean) < 0.0f)
			{
				++structure_culling.disagree_count;
			}
		}
	}
	if (TEST_FLAG(structure_culling_tests, _structure_culling_finer_buckets_bit))
	{
		bucket_count = structure_culling_split(work->surfaces, count, work->ends, count);
	}
	else
	{
		/* even unsplit buckets must not apply an opaque material's verdict to another shader */
		for (i = 0; i < count; ++i)
		{
			if (work->surfaces[i].facing_eligible)
			{
				struct structure_culling_split_surface swap = work->surfaces[eligible_count];
				work->surfaces[eligible_count++] = work->surfaces[i];
				work->surfaces[i] = swap;
			}
		}
		bucket_count = 1;
		work->ends[0] = count;
		if (eligible_count && eligible_count < count)
		{
			work->ends[0] = eligible_count;
			work->ends[1] = count;
			bucket_count = 2;
		}
	}
	structure_culling.split_count += bucket_count - 1;
	first = 0;
	for (i = 0; i < bucket_count; ++i)
	{
		struct structure_culling_bucket *bucket = &structure_culling.buckets[structure_culling.bucket_count++];
		boolean invalid = FALSE;
		boolean have_bounds = FALSE;
		long point_count = 0;

		bucket->first_pairing = structure_culling.pairing_count;
		bucket->pairing_count = work->ends[i] - first;
		bucket->facing_eligible = work->surfaces[first].facing_eligible;
		bucket->source_bounds = from_surface_list ? NULL : parent_bounds;
		for (j = first; j < work->ends[i]; ++j)
		{
			struct structure_culling_pairing pairing = work->pairings[work->surfaces[j].surface_index];
			short v;

			structure_culling.pairings[structure_culling.pairing_count++] = pairing;
			if (!pairing.testable)
			{
				invalid = TRUE;
				continue;
			}
			structure_culling_pairing_points(structure, &pairing, work->points + point_count);
			for (v = 0; v < 3; ++v)
			{
				real_point3d const *point = &work->points[point_count++];
				if (!have_bounds)
				{
					bucket->bounds.x0 = bucket->bounds.x1 = point->x;
					bucket->bounds.y0 = bucket->bounds.y1 = point->y;
					bucket->bounds.z0 = bucket->bounds.z1 = point->z;
					have_bounds = TRUE;
				}
				structure_culling_bounds_add(&bucket->bounds, point);
			}
		}
		structure_culling_cone_build(&bucket->cone, point_count / 3, work->points);
		if (invalid)
		{
			/* an untestable subcluster pairing keeps the original subcluster's visibility */
			bucket->bounds = *parent_bounds;
			bucket->cone.cutoff = -1.0f;
		}
		first = work->ends[i];
	}
}

static long structure_culling_build_lists(
	struct structure_bsp *structure, struct structure_culling_workspace *work, long *maximum_group_count)
{
	long c;
	long s;
	long total = 0;
	boolean subclusters =
		TAG_BLOCK_GET_ELEMENT(&structure->clusters, 0, struct structure_visibility_cluster)->subclusters.count != 0;

	if (maximum_group_count)
	{
		*maximum_group_count = 0;
	}
	for (c = 0; c < structure->clusters.count; ++c)
	{
		struct structure_visibility_cluster *cluster =
			TAG_BLOCK_GET_ELEMENT(&structure->clusters, c, struct structure_visibility_cluster);
		long position = 0;
		if (work)
		{
			structure_culling.clusters[c].first_bucket = structure_culling.bucket_count;
		}
		for (s = 0; subclusters ? s < cluster->subclusters.count : position < cluster->surface_indices.count; ++s)
		{
			long const *list;
			long count;
			short material_index = NONE;
			real_rectangle3d const *bounds = &structure->world_bounds;
			if (subclusters)
			{
				struct structure_visibility_subcluster *subcluster =
					TAG_BLOCK_GET_ELEMENT(&cluster->subclusters, s, struct structure_visibility_subcluster);
				list = subcluster->surface_indices.address;
				count = subcluster->surface_indices.count;
				bounds = &subcluster->world_bounds;
			}
			else
			{
				struct tag_block *materials;
				long const *header = (long const *)cluster->surface_indices.address + position;
				/* malformed headers end this cluster, just as in the original walk */
				if (position > cluster->surface_indices.count - 3 || header[0] < 0 ||
					header[0] >= structure->lightmaps.count)
				{
					break;
				}
				materials =
					&TAG_BLOCK_GET_ELEMENT(&structure->lightmaps, header[0], struct structure_lightmap)->materials;
				if (header[1] < 0 || header[1] >= materials->count)
				{
					break;
				}
				position += 3;
				count = MIN(MAX(header[2], 0), cluster->surface_indices.count - position);
				list = header + 3;
				if (work)
				{
					material_index = (short)(work->lightmap_first[header[0]] + header[1]);
				}
				position += count;
			}
			if (count < 0 || total > LONG_MAX - count)
			{
				return NONE;
			}
			total += count;
			if (maximum_group_count)
			{
				*maximum_group_count = MAX(*maximum_group_count, count);
			}
			if (work)
			{
				structure_culling_build_group(structure, work, list, count, !subclusters, material_index, bounds);
			}
		}
		if (work)
		{
			struct structure_culling_cluster *built = &structure_culling.clusters[c];
			long b;
			built->bucket_count = structure_culling.bucket_count - built->first_bucket;
			for (b = built->first_bucket; b < structure_culling.bucket_count; ++b)
			{
				real_rectangle3d const *box = &structure_culling.buckets[b].bounds;
				real_point3d low;
				real_point3d high;
				low.x = box->x0;
				low.y = box->y0;
				low.z = box->z0;
				high.x = box->x1;
				high.y = box->y1;
				high.z = box->z1;
				if (b == built->first_bucket)
				{
					built->bounds = *box;
				}
				structure_culling_bounds_add(&built->bounds, &low);
				structure_culling_bounds_add(&built->bounds, &high);
			}
		}
	}
	return total;
}

/* ---------- public code */

boolean structure_culling_triangle_outside(
	real_plane3d const planes[6], real_point3d const *point0, real_point3d const *point1, real_point3d const *point2)
{
	short plane_index;

	for (plane_index = 0; plane_index < 6; plane_index++)
	{
		if (plane3d_distance_to_point(&planes[plane_index], point0) > 0.0f &&
			plane3d_distance_to_point(&planes[plane_index], point1) > 0.0f &&
			plane3d_distance_to_point(&planes[plane_index], point2) > 0.0f)
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean structure_culling_triangle_faces_away(
	real_point3d const *camera, real_point3d const *point0, real_point3d const *point1, real_point3d const *point2)
{
	real_vector3d edge0;
	real_vector3d edge1;
	real_vector3d normal;
	real_vector3d view;
	real squared;

	vector_from_points3d(point0, point1, &edge0);
	vector_from_points3d(point0, point2, &edge1);
	cross_product3d(&edge0, &edge1, &normal);
	squared = magnitude_squared3d(&normal);
	if (!(squared > 1.0e-20f && squared <= FLT_MAX))
	{
		return FALSE;
	}
	vector_from_points3d(point0, camera, &view);
	return STRUCTURE_CULLING_FRONT_SIGN * dot_product3d(&normal, &view) < 0.0f;
}

void structure_culling_cone_build(struct structure_culling_cone *cone, long triangle_count, real_point3d const *points)
{
	real_point3d low;
	real_point3d high;
	real_vector3d sum = {0, 0, 0};
	real squared;
	real maximum_squared = 0.0f;
	long point_index;
	long triangle_index;

	cone->center.x = cone->center.y = cone->center.z = 0.0f;
	cone->axis = sum;
	cone->radius = 0.0f;
	cone->cutoff = -1.0f;
	if (triangle_count <= 0)
	{
		return;
	}
	low = high = points[0];
	for (point_index = 0; point_index < triangle_count * 3; ++point_index)
	{
		real_point3d const *point = &points[point_index];

		low.x = MIN(low.x, point->x);
		low.y = MIN(low.y, point->y);
		low.z = MIN(low.z, point->z);
		high.x = MAX(high.x, point->x);
		high.y = MAX(high.y, point->y);
		high.z = MAX(high.z, point->z);
	}
	cone->center.x = (low.x + high.x) * 0.5f;
	cone->center.y = (low.y + high.y) * 0.5f;
	cone->center.z = (low.z + high.z) * 0.5f;
	for (point_index = 0; point_index < triangle_count * 3; ++point_index)
	{
		real_vector3d offset;

		vector_from_points3d(&cone->center, &points[point_index], &offset);
		squared = magnitude_squared3d(&offset);
		maximum_squared = MAX(maximum_squared, squared);
	}
	cone->radius = square_root(maximum_squared);
	/* unit normals keep large triangles from dominating the mean direction */
	for (triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
	{
		real_point3d const *point = &points[triangle_index * 3];
		real_vector3d edge0;
		real_vector3d edge1;
		real_vector3d normal;

		vector_from_points3d(point, point + 1, &edge0);
		vector_from_points3d(point, point + 2, &edge1);
		cross_product3d(&edge0, &edge1, &normal);
		squared = magnitude_squared3d(&normal);
		if (!(squared > 1.0e-20f && squared <= FLT_MAX))
		{
			return;
		}
		scale_vector3d(&normal, STRUCTURE_CULLING_FRONT_SIGN / square_root(squared), &normal);
		sum.i += normal.i;
		sum.j += normal.j;
		sum.k += normal.k;
	}
	squared = magnitude_squared3d(&sum);
	if (!(squared > 1.0e-20f && squared <= FLT_MAX))
	{
		return;
	}
	scale_vector3d(&sum, 1.0f / square_root(squared), &cone->axis);
	cone->cutoff = 1.0f;
	for (triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
	{
		real_point3d const *point = &points[triangle_index * 3];
		real_vector3d edge0;
		real_vector3d edge1;
		real_vector3d normal;
		real cutoff;

		vector_from_points3d(point, point + 1, &edge0);
		vector_from_points3d(point, point + 2, &edge1);
		cross_product3d(&edge0, &edge1, &normal);
		scale_vector3d(&normal, STRUCTURE_CULLING_FRONT_SIGN / square_root(magnitude_squared3d(&normal)), &normal);
		cutoff = dot_product3d(&cone->axis, &normal);
		cone->cutoff = MIN(cone->cutoff, cutoff);
	}
}

short structure_culling_cone_facing(struct structure_culling_cone const *cone, real_point3d const *camera)
{
	real_vector3d view;
	real length;
	real sin_theta;
	real sin_alpha;
	real cos_alpha;
	real bound;
	real dot;

	vector_from_points3d(&cone->center, camera, &view);
	length = square_root(magnitude_squared3d(&view));
	if (cone->cutoff <= 0.0f || length <= cone->radius)
	{
		return _structure_culling_facing_mixed;
	}
	sin_theta = square_root(1.0f - cone->cutoff * cone->cutoff);
	sin_alpha = cone->radius / length;
	cos_alpha = square_root(1.0f - sin_alpha * sin_alpha);
	/* a cone plus the sphere's angular radius must fit within a hemisphere */
	if (cone->cutoff * cos_alpha - sin_theta * sin_alpha <= 0.0f)
	{
		return _structure_culling_facing_mixed;
	}
	bound = length * (sin_theta * cos_alpha + cone->cutoff * sin_alpha);
	dot = dot_product3d(&cone->axis, &view);
	if (dot < -bound)
	{
		return _structure_culling_facing_away;
	}
	if (dot > bound)
	{
		return _structure_culling_facing_toward;
	}
	return _structure_culling_facing_mixed;
}

short structure_culling_box_test(real_plane3d const planes[6], real_rectangle3d const *box)
{
	short plane_index;
	short corner_index;
	boolean partly = FALSE;

	for (plane_index = 0; plane_index < 6; ++plane_index)
	{
		short outside = 0;

		for (corner_index = 0; corner_index < 8; ++corner_index)
		{
			real_point3d corner;

			corner.x = (corner_index & 1) ? box->x1 : box->x0;
			corner.y = (corner_index & 2) ? box->y1 : box->y0;
			corner.z = (corner_index & 4) ? box->z1 : box->z0;
			outside += plane3d_distance_to_point(&planes[plane_index], &corner) > 0.0f;
		}
		if (outside == 8)
		{
			return 0;
		}
		partly |= outside != 0;
	}
	return partly ? 1 : 2;
}

long structure_culling_split(
	struct structure_culling_split_surface *surfaces, long surface_count, long *bucket_ends, long maximum_buckets)
{
	long bucket_count = 0;

	/* one slot per surface bounds the output even for a completely scattered bucket */
	if (surface_count <= 0 || maximum_buckets < surface_count)
	{
		return 0;
	}
	structure_culling_split_range(surfaces, 0, surface_count, bucket_ends, &bucket_count, TRUE);
	return bucket_count;
}

long structure_culling_gather_pairings(long const *list, long list_count, boolean from_surface_list,
	short group_material, long surface_count, short const *surface_materials, long const *material_vertex_counts,
	word const *surface_vertex_indices, struct structure_culling_pairing *pairings)
{
	long i;
	long count = 0;

	for (i = 0; i < list_count; ++i)
	{
		long surface_index = list[i];
		short material_index;
		boolean testable;
		word const *vertices;

		if ((unsigned long)surface_index >= (unsigned long)surface_count)
		{
			continue;
		}
		material_index = from_surface_list ? group_material : surface_materials[surface_index];
		vertices = surface_vertex_indices + 3 * surface_index;
		testable = material_index >= 0 && vertices[0] < material_vertex_counts[material_index] &&
			vertices[1] < material_vertex_counts[material_index] &&
			vertices[2] < material_vertex_counts[material_index];
		if (from_surface_list && !testable)
		{
			continue;
		}
		pairings[count].surface_index = surface_index;
		pairings[count].material_index = material_index;
		pairings[count++].testable = testable;
	}
	return count;
}

void structure_culling_dispose(void)
{
	/* debug_free rejects NULL, including before the first bsp load */
	if (structure_culling.allocation)
	{
		free(structure_culling.allocation);
	}
	memset(&structure_culling, 0, sizeof(structure_culling));
}

void structure_culling_order_clusters(short *order, real const *distances, short count)
{
	short i;
	short j;
	short entry;

	for (i = 1; i < count; ++i)
	{
		entry = order[i];
		for (j = i; j > 0 && distances[order[j - 1]] > distances[entry]; --j)
		{
			order[j] = order[j - 1];
		}
		order[j] = entry;
	}
}

void structure_culling_debug(boolean enable)
{
	structure_culling_debug_enabled = enable;
}

void structure_culling_render_debug(void)
{
	static real_argb_color const colors[2] = {{0.5, 1, 0, 0}, {0.5, 0, 0, 1}};
	static long frame = NONE;
	static long count;
	static unsigned long emitted[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_SURFACES_PER_STRUCTURE)];
	struct structure_bsp *structure;
	long p;
	long truncated = 0;
	char text[256];
	rectangle2d bounds;

	if (!structure_culling_debug_enabled)
	{
		return;
	}
	if (frame != render.frame_index)
	{
		frame = render.frame_index;
		count = 0;
	}
	memset(emitted, 0, sizeof(emitted));
	structure = global_structure_bsp_get();
	if (!structure_culling_data_matches(structure))
	{
		return;
	}
	for (p = 0; p < structure_culling.pairing_count; ++p)
	{
		struct structure_culling_pairing const *pairing = &structure_culling.pairings[p];
		long s = pairing->surface_index;
		short reason = NONE;

		if (BIT_VECTOR_TEST_FLAG(structure_culling_recorded[0], s))
		{
			reason = 0;
		}
		else if (BIT_VECTOR_TEST_FLAG(structure_culling_recorded[1], s))
		{
			reason = 1;
		}
		if (reason != NONE && pairing->testable && !BIT_VECTOR_TEST_FLAG(render.environment_surface_flags, s) &&
			!BIT_VECTOR_TEST_FLAG(emitted, s))
		{
			real_point3d points[3];
			struct structure_surface const *surface =
				TAG_BLOCK_GET_ELEMENT(&structure->surfaces, s, struct structure_surface);
			long vertices = structure_culling.materials[pairing->material_index].vertex_count;

			if (surface->vertex_indices[0] >= vertices || surface->vertex_indices[1] >= vertices ||
				surface->vertex_indices[2] >= vertices)
			{
				continue;
			}
			BIT_VECTOR_SET_FLAG(emitted, s, TRUE);
			if (count >= STRUCTURE_CULLING_MAXIMUM_DEBUG_TRIANGLES)
			{
				++truncated;
				continue;
			}
			structure_culling_pairing_points(structure, pairing, points);
			render_debug_triangle(TRUE, points, points + 1, points + 2, &colors[reason]);
			++count;
		}
	}
	snprintf(text, sizeof(text),
		"structure culling: %ld drawn, %ld outside, %ld facing away; %ld buckets (%s); built in %lu ms",
		structure_culling_submitted, structure_culling_outside, structure_culling_facing,
		structure_culling.bucket_count, structure_culling.from_subclusters ? "subclusters" : "surface lists",
		structure_culling.build_milliseconds);
	if (truncated)
	{
		snprintf(text + strlen(text), sizeof(text) - strlen(text), " (%ld more not drawn)", truncated);
	}
	/* keep the counters below the cinematic letterbox at its largest height */
	bounds.y0 = (render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0) / 8 + 2;
	bounds.x0 = 0;
	bounds.x1 = SHORT_MAX;
	bounds.y1 = SHORT_MAX;
	interface_set_bitmap_text_draw_mode(_interface_font_terminal, NONE, 0, 0, _interface_color_table_dialog, 0);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
}

void structure_culling_initialize(void)
{
	/* display.structure_culling; the console's structure_culling changes it until the game quits */
	structure_culling_tests = config_integer("display.structure_culling") &
		(FLAG(NUMBER_OF_STRUCTURE_CULLING_TESTS) - 1);
}

void structure_culling_set_tests(long tests)
{
	long changed = structure_culling_tests ^ tests;

	structure_culling_tests = tests;
	/* (with no bsp loaded the next load builds with these tests) */
	if (TEST_FLAG(changed, _structure_culling_finer_buckets_bit) && global_structure_bsp_index != NONE)
	{
		structure_culling_structure_bsp_loaded();
	}
}

boolean structure_culling_traverse(struct structure_bsp *structure)
{
	short order[MAXIMUM_RENDERED_CLUSTERS];
	short c;
	short axis;
	real distances[MAXIMUM_RENDERED_CLUSTERS];
	real_plane3d planes[6];
	struct profile_section *section;

	/* split screen deliberately keeps only the last window's call */
	structure_culling_submitted = structure_culling_outside = structure_culling_facing = 0;
	if (structure_culling_debug_enabled)
	{
		memset(structure_culling_recorded, 0, sizeof(structure_culling_recorded));
	}
	if (!structure_culling_tests || !structure_culling_data_matches(structure))
	{
		return FALSE;
	}
	section = structure_culling.from_subclusters ? &render_structure_visibility_subcluster_traversal :
		&render_structure_visibility_surface_traversal;
	profile_enter((*section));
	/* the GPU far plane, not the fog-clamped frustum bounds; positive is outside */
	scale_vector3d(&render.camera.forward, 1.0f / square_root(magnitude_squared3d(&render.camera.forward)),
		&planes[_structure_culling_frustum_plane_far].n);
	planes[_structure_culling_frustum_plane_far].d =
		planes[_structure_culling_frustum_plane_far].n.i * render.camera.position.x +
		planes[_structure_culling_frustum_plane_far].n.j * render.camera.position.y +
		planes[_structure_culling_frustum_plane_far].n.k * render.camera.position.z +
		rasterizer_globals.far_clip_distance;
	/* mirror projections are oblique: this perpendicular plane is not their GPU far clip */
	if (render.camera.z_near == 0.0f)
	{
		memset(&planes[_structure_culling_frustum_plane_far], 0, sizeof(planes[_structure_culling_frustum_plane_far]));
	}
	for (c = 0; c < render.rendered_cluster_count; ++c)
	{
		struct structure_culling_cluster const *cluster =
			&structure_culling.clusters[rendered_cluster_get(c)->cluster_index];
		order[c] = c;
		distances[c] = 0.0f;
		/* empty clusters have no bounds */
		if (cluster->bucket_count && TEST_FLAG(structure_culling_tests, _structure_culling_near_first_bit))
		{
			for (axis = 0; axis < 3; ++axis)
			{
				real const *bounds = &cluster->bounds.x0 + axis * 2;
				real position = (&render.camera.position.x)[axis];
				real offset = MAX(bounds[0] - position, MAX(0.0f, position - bounds[1]));
				distances[c] += offset * offset;
			}
		}
	}
	if (TEST_FLAG(structure_culling_tests, _structure_culling_near_first_bit))
	{
		structure_culling_order_clusters(order, distances, render.rendered_cluster_count);
	}
	for (c = 0; c < render.rendered_cluster_count; ++c)
	{
		struct rendered_cluster const *rendered_cluster = rendered_cluster_get(order[c]);
		struct structure_culling_cluster const *cluster = &structure_culling.clusters[rendered_cluster->cluster_index];
		struct render_frustum const *frustum =
			structures_use_pvs_for_vs || render.cluster_index == NONE ? &render.frustum : &rendered_cluster->frustum;
		long b;
		if (render.environment_surface_count >= MAXIMUM_RENDERED_ENVIRONMENT_SURFACES)
		{
			break;
		}
		memcpy(planes, frustum->world_planes, _structure_culling_frustum_plane_far * sizeof(*planes));
		for (b = cluster->first_bucket; b < cluster->first_bucket + cluster->bucket_count; ++b)
		{
			struct structure_culling_bucket const *bucket = &structure_culling.buckets[b];
			short box;
			short facing = _structure_culling_facing_mixed;
			short bucket_reason = 0;
			boolean triangle_facing;
			long p;
			if (render.environment_surface_count >= MAXIMUM_RENDERED_ENVIRONMENT_SURFACES)
			{
				break;
			}
			/* retain the original fog-clamped test on the source, not its finer buckets */
			if (bucket->source_bounds && !render_frustum_cube_visible(frustum, bucket->source_bounds, FALSE))
			{
				box = 0;
			}
			else
			{
				box = structure_culling_box_test(planes, &bucket->bounds);
			}
			if (!box)
			{
				bucket_reason = 1;
			}
			else if (bucket->facing_eligible &&
				(TEST_FLAG(structure_culling_tests, _structure_culling_bucket_facing_bit) ||
					TEST_FLAG(structure_culling_tests, _structure_culling_triangle_facing_bit)))
			{
				facing = structure_culling_cone_facing(&bucket->cone, &render.camera.position);
				if (facing == _structure_culling_facing_away &&
					TEST_FLAG(structure_culling_tests, _structure_culling_bucket_facing_bit))
				{
					bucket_reason = 2;
				}
			}
			if (bucket_reason && !structure_culling_debug_enabled)
			{
				continue;
			}
			triangle_facing = bucket->facing_eligible && facing == _structure_culling_facing_mixed &&
				TEST_FLAG(structure_culling_tests, _structure_culling_triangle_facing_bit);
			for (p = bucket->first_pairing; p < bucket->first_pairing + bucket->pairing_count; ++p)
			{
				struct structure_culling_pairing const *pairing = &structure_culling.pairings[p];
				long surface_index = pairing->surface_index;
				short reason = bucket_reason;
				if ((unsigned long)surface_index >= (unsigned long)structure->surfaces.count ||
					BIT_VECTOR_TEST_FLAG(render.environment_surface_flags, surface_index))
				{
					continue;
				}
				if (render.environment_surface_count >= MAXIMUM_RENDERED_ENVIRONMENT_SURFACES)
				{
					break;
				}
				if (!reason && pairing->testable &&
					(triangle_facing ||
						(box != 2 && TEST_FLAG(structure_culling_tests, _structure_culling_triangle_frustum_bit))))
				{
					struct structure_surface const *surface =
						TAG_BLOCK_GET_ELEMENT(&structure->surfaces, surface_index, struct structure_surface);
					struct structure_culling_material const *material =
						&structure_culling.materials[pairing->material_index];
					if (surface->vertex_indices[0] < material->vertex_count &&
						surface->vertex_indices[1] < material->vertex_count &&
						surface->vertex_indices[2] < material->vertex_count)
					{
						real_point3d points[3];
						structure_culling_pairing_points(structure, pairing, points);
						if (box != 2 && TEST_FLAG(structure_culling_tests, _structure_culling_triangle_frustum_bit) &&
							structure_culling_triangle_outside(planes, points, points + 1, points + 2))
						{
							reason = 1;
						}
						else if (triangle_facing &&
							structure_culling_triangle_faces_away(
								&render.camera.position, points, points + 1, points + 2))
						{
							reason = 2;
						}
					}
				}
				structure_culling_record(surface_index, reason);
				if (!reason)
				{
					BIT_VECTOR_SET_FLAG(render.environment_surface_flags, surface_index, TRUE);
					++render.environment_surface_count;
					++structure_culling_submitted;
				}
			}
		}
	}
	profile_exit((*section));
	return TRUE;
}

void structure_culling_structure_bsp_loaded(void)
{
	unsigned long started = system_milliseconds();
	struct structure_bsp *structure;
	struct structure_culling_workspace work;
	long l;
	long m;
	long i;
	long material_count = 0;
	long material_index = 0;
	long capacity;
	long largest_group_count;
	size_t base_size;
	size_t unit_size;
	size_t scratch_unit_size;
	size_t size;
	size_t final_size;
	byte *cursor;
	byte *allocation;

	structure_culling_dispose();
	structure = global_structure_bsp_get();
	if (!structure || !structure_bsp_port_verify(structure))
	{
		return;
	}
	for (l = 0; l < structure->lightmaps.count; ++l)
	{
		long count = TAG_BLOCK_GET_ELEMENT(&structure->lightmaps, l, struct structure_lightmap)->materials.count;
		/* short material indices reserve NONE and must never wrap */
		if (count < 0 || count > SHRT_MAX - material_count)
		{
			return;
		}
		material_count += count;
	}
	capacity = structure_culling_build_lists(structure, NULL, &largest_group_count);
	if (capacity < 0)
	{
		return;
	}
	base_size = material_count * (sizeof(struct structure_culling_material) + sizeof(long)) +
		structure->clusters.count * sizeof(struct structure_culling_cluster) +
		structure->lightmaps.count * sizeof(long) + ((structure->surfaces.count + 1) & ~1) * sizeof(short);
	unit_size = sizeof(struct structure_culling_bucket) + sizeof(struct structure_culling_pairing);
	scratch_unit_size = sizeof(struct structure_culling_split_surface) + sizeof(struct structure_culling_pairing) +
		sizeof(long) + 3 * sizeof(real_point3d);
	if (base_size >= LONG_MAX || (size_t)capacity > (LONG_MAX - base_size) / unit_size)
	{
		return;
	}
	size = base_size + capacity * unit_size;
	if ((size_t)largest_group_count > (LONG_MAX - size) / scratch_unit_size)
	{
		return;
	}
	size += largest_group_count * scratch_unit_size;
	if (size >= STRUCTURE_CULLING_BUILD_BUDGET)
	{
		return;
	}
	allocation = malloc(size ? size : 1);
	if (!allocation)
	{
		return;
	}
	structure_culling.allocation = allocation;
	cursor = allocation;
	structure_culling.materials = (struct structure_culling_material *)cursor;
	cursor += material_count * sizeof(*structure_culling.materials);
	structure_culling.clusters = (struct structure_culling_cluster *)cursor;
	cursor += structure->clusters.count * sizeof(*structure_culling.clusters);
	structure_culling.buckets = (struct structure_culling_bucket *)cursor;
	cursor += capacity * sizeof(*structure_culling.buckets);
	structure_culling.pairings = (struct structure_culling_pairing *)cursor;
	cursor += capacity * sizeof(*structure_culling.pairings);
	work.vertex_counts = (long *)cursor;
	cursor += material_count * sizeof(long);
	work.lightmap_first = (long *)cursor;
	cursor += structure->lightmaps.count * sizeof(long);
	work.owners = (short *)cursor;
	cursor += ((structure->surfaces.count + 1) & ~1) * sizeof(short);
	/* the same parser bounds every build_group call by the counted maximum */
	work.surfaces = (struct structure_culling_split_surface *)cursor;
	cursor += largest_group_count * sizeof(*work.surfaces);
	work.pairings = (struct structure_culling_pairing *)cursor;
	cursor += largest_group_count * sizeof(*work.pairings);
	work.ends = (long *)cursor;
	cursor += largest_group_count * sizeof(long);
	work.points = (real_point3d *)cursor;
	for (i = 0; i < structure->surfaces.count; ++i)
	{
		work.owners[i] = NONE;
	}
	for (l = 0; l < structure->lightmaps.count; ++l)
	{
		struct tag_block *materials =
			&TAG_BLOCK_GET_ELEMENT(&structure->lightmaps, l, struct structure_lightmap)->materials;
		work.lightmap_first[l] = material_index;
		for (m = 0; m < materials->count; ++m, ++material_index)
		{
			struct structure_material *material = TAG_BLOCK_GET_ELEMENT(materials, m, struct structure_material);
			struct structure_culling_material *built = &structure_culling.materials[material_index];
			built->vertices = material->compressed_vertex_data.address;
			built->vertex_count =
				built->vertices ? MAX(material->compressed_vertex_data.size / COMPRESSED_STRUCTURE_VERTEX_SIZE, 0) : 0;
			work.vertex_counts[material_index] = built->vertex_count;
			built->facing_eligible = tag_index_is_group(material->shader.index, SHADER_DEFINITION_TAG) &&
				shader_definition_get(material->shader.index)->base.type == _shader_type_environment;
			for (i = MAX(material->first_surface_index, 0); i < structure->surfaces.count; ++i)
			{
				if (material->surface_count <= 0 ||
					(unsigned long)i - (unsigned long)material->first_surface_index >=
						(unsigned long)material->surface_count)
				{
					break;
				}
				if (work.owners[i] == NONE)
				{
					work.owners[i] = (short)material_index;
				}
			}
		}
	}
	structure_culling.material_count = material_count;
	structure_culling_build_lists(structure, &work, NULL);
	/* scratch and worst-case bucket slots share one allocation, then leave only the exact tables */
	final_size = material_count * sizeof(*structure_culling.materials) +
		structure->clusters.count * sizeof(*structure_culling.clusters) +
		structure_culling.bucket_count * sizeof(*structure_culling.buckets) +
		structure_culling.pairing_count * sizeof(*structure_culling.pairings);
	cursor = (byte *)structure_culling.buckets + structure_culling.bucket_count * sizeof(*structure_culling.buckets);
	memmove(cursor, structure_culling.pairings, structure_culling.pairing_count * sizeof(*structure_culling.pairings));
	/* debug_realloc invalidates the old header even on failure; keep both blocks valid */
	allocation = malloc(final_size ? final_size : 1);
	if (allocation)
	{
		memcpy(allocation, structure_culling.allocation, final_size);
		free(structure_culling.allocation);
	}
	else
	{
		allocation = structure_culling.allocation;
		final_size = size;
	}
	structure_culling.allocation = allocation;
	structure_culling.materials = (struct structure_culling_material *)allocation;
	structure_culling.clusters =
		(struct structure_culling_cluster *)(allocation + material_count * sizeof(*structure_culling.materials));
	structure_culling.buckets =
		(struct structure_culling_bucket *)(structure_culling.clusters + structure->clusters.count);
	structure_culling.pairings =
		(struct structure_culling_pairing *)(structure_culling.buckets + structure_culling.bucket_count);
	structure_culling.structure = structure;
	structure_culling.surface_address = structure->surfaces.address;
	structure_culling.surface_count = structure->surfaces.count;
	structure_culling.bytes = (long)final_size;
	structure_culling.from_subclusters =
		TAG_BLOCK_GET_ELEMENT(&structure->clusters, 0, struct structure_visibility_cluster)->subclusters.count != 0;
	structure_culling.build_milliseconds = system_milliseconds() - started;
	error(_error_silent,
		"structure culling: %ld buckets (%s, %ld split) for %ld surfaces, %ld bytes, %lu ms, %ld of %ld triangles "
		"disagree with their vertex normals",
		structure_culling.bucket_count, structure_culling.from_subclusters ? "subclusters" : "surface lists",
		structure_culling.split_count, structure_culling.surface_count, structure_culling.bytes,
		structure_culling.build_milliseconds, structure_culling.disagree_count, structure_culling.triangle_count);
}
