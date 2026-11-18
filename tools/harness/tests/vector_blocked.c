/*
VECTOR_BLOCKED.C

collision_port_test_vector_blocked (collisions.c), the test that ends at its
first hit, against collision_test_vector's: over a fake level of walls, water
and objects, every vector is blocked by one exactly when it is by the other,
for each set of what the vector tests for. test_vector_blocked.py takes the
enums (enums.inc) and the code (under_test.inc) from the sources.
*/

#include "harness.h"
#include <limits.h>
#include <math.h>
#include "enums.inc"

typedef unsigned short word;
typedef struct { real_vector3d n; real d; } real_plane3d;
typedef struct { long long QuadPart; } LARGE_INTEGER;
#define REAL_MAX 3.4028235e38f
#define _real_epsilon 0.0001f
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define MAXIMUM_BREAKABLE_SURFACES_PER_MAP 256

struct location { long leaf_index; short cluster_index; word bonus; };
#include "structures.inc"

/* the level: clusters 10 units wide along x, from 0 to 40, each its own leaf */
enum { CLUSTERS = 4, OBJECTS = 40, WALLS = 3 };

static real wall_x[WALLS];
static boolean cluster_has_water[CLUSTERS];
static struct structure_fog_plane cluster_fog_planes[CLUSTERS];
static struct fog_definition cluster_fogs[CLUSTERS];
static real_point3d object_centers[OBJECTS];
static real object_radii[OBJECTS];
static long cluster_objects[CLUSTERS][OBJECTS], cluster_counts[CLUSTERS];
static real_plane3d const wall_plane = { { 1.0f, 0.0f, 0.0f }, 0.0f };

static long cluster_of_x(real x)
{
	long cluster = (long)floorf(x / 10.0f);

	return cluster < 0 || cluster >= CLUSTERS ? NONE : cluster;
}

struct structure_bsp;
struct collision_bsp;
#define global_structure_bsp_get() ((struct structure_bsp *)NULL)
#define global_collision_bsp_get() ((struct collision_bsp const *)NULL)
#define breakable_surface_flags_get() ((unsigned long *)NULL)
#define collision_cluster_index_from_leaf(leaf_index) (leaf_index)
#define collision_structure_material_type(structure_bsp, material_index) ((long)(material_index))
#define collision_log_usage(function) ((void)0)
#define collision_log_start_time(time) ((void)0)
#define collision_log_end_time(function, time) ((void)0)
static struct collision_usage_times collision_usage_times;
static boolean debug_collision_skip_vectors, debug_collision_skip_objects;

/* the nearest wall the vector crosses, and the clusters it passes through to it */
static boolean collision_bsp_test_vector(unsigned long bsp_flags, struct collision_bsp const *bsp, long maximum,
	unsigned long *breakable, real_point3d const *point, real_vector3d const *vector, real maximum_t,
	struct collision_bsp_test_vector_result *result)
{
	real end_t = 1.0f;
	boolean hit = FALSE;
	long wall;
	real t;
	long first, last, cluster;

	for (wall = 0; wall < WALLS; wall++)
	{
		if (fabsf(vector->i) < _real_epsilon)
			continue;
		t = (wall_x[wall] - point->x) / vector->i;
		if (t >= 0.0f && t < end_t)
		{
			end_t = t;
			hit = TRUE;
		}
	}
	result->t = end_t;
	result->plane = &wall_plane;
	result->surface_index = 1;
	result->plane_designator = vector->i > 0.0f ? LONG_MIN : 0;
	result->flags = 0;
	result->breakable_surface_index = 0;
	result->material_index = 3;
	result->leaf_count = 0;
	first = cluster_of_x(point->x);
	last = cluster_of_x(point->x + vector->i * end_t);
	if (first != NONE && last != NONE)
	{
		for (cluster = first; ; cluster += first <= last ? 1 : -1)
		{
			result->leaf_indices[result->leaf_count++] = cluster;
			if (cluster == last)
				break;
		}
	}
	(void)bsp_flags;
	(void)bsp;
	(void)maximum;
	(void)breakable;
	(void)maximum_t;
	return hit;
}

static boolean collision_cluster_fog_plane(struct structure_bsp const *structure_bsp, long cluster_index,
	struct structure_fog_plane const **fog_plane, struct fog_definition const **fog)
{
	(void)structure_bsp;
	if (!cluster_has_water[cluster_index])
		return FALSE;
	*fog_plane = &cluster_fog_planes[cluster_index];
	*fog = &cluster_fogs[cluster_index];
	return TRUE;
}

static real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static real plane3d_distance_to_point(real_plane3d const *plane, real_point3d const *point)
{
	return plane->n.i * point->x + plane->n.j * point->y + plane->n.k * point->z - plane->d;
}

static real_plane3d *plane3d_negate(real_plane3d const *plane, real_plane3d *result)
{
	result->n.i = -plane->n.i;
	result->n.j = -plane->n.j;
	result->n.k = -plane->n.k;
	result->d = -plane->d;
	return result;
}

static real_point3d *point_from_line3d(real_point3d const *point, real_vector3d const *vector, real t,
	real_point3d *result)
{
	result->x = point->x + vector->i * t;
	result->y = point->y + vector->j * t;
	result->z = point->z + vector->k * t;
	return result;
}

static void add_vectors3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	result->i = a->i + b->i;
	result->j = a->j + b->j;
	result->k = a->k + b->k;
}

static long scenario_leaf_index_from_point(real_point3d const *point)
{
	return cluster_of_x(point->x);
}

static void scenario_location_from_point(struct location *location, real_point3d const *point)
{
	location->leaf_index = cluster_of_x(point->x);
	location->cluster_index = (short)location->leaf_index;
}

/* each cluster and object visited once a vector */
static long cluster_marks[CLUSTERS], object_marks[OBJECTS], marker;
#define structure_cluster_marker_begin() (++marker)
#define structure_cluster_marker_end() ((void)0)
#define object_marker_begin() ((void)0)
#define object_marker_end() ((void)0)
#define structure_cluster_mark(cluster) (cluster_marks[cluster] != marker && (cluster_marks[cluster] = marker))
#define object_mark_function(object_index) (object_marks[object_index] != marker && (object_marks[object_index] = marker))
#define object_bounds_cache_missed_by_vector(object_index, point, vector) FALSE

static long cluster_get_first_collideable_object(long *reference, long cluster)
{
	*reference = cluster * OBJECTS;
	return cluster_counts[cluster] ? cluster_objects[cluster][0] : NONE;
}

static long cluster_get_next_collideable_object(long *reference)
{
	long cluster = *reference / OBJECTS, place = *reference % OBJECTS + 1;

	*reference = cluster * OBJECTS + place;
	return place < cluster_counts[cluster] ? cluster_objects[cluster][place] : NONE;
}

/* an object is its sphere, hit where the vector enters it, if nearer than the
collision so far (as object_test_vector's tests are) */
static boolean object_test_vector(long object_index, unsigned long flags, unsigned long bsp_flags,
	real_point3d const *point, real_vector3d const *vector, long ignore_object_index,
	struct collision_result *collision)
{
	real_vector3d to_point = {
		point->x - object_centers[object_index].x,
		point->y - object_centers[object_index].y,
		point->z - object_centers[object_index].z };
	real a = dot_product3d(vector, vector);
	real b = 2.0f * dot_product3d(&to_point, vector);
	real c = dot_product3d(&to_point, &to_point) - object_radii[object_index] * object_radii[object_index];
	real discriminant = b * b - 4.0f * a * c;
	real t;

	(void)flags;
	(void)bsp_flags;
	if (object_index == ignore_object_index || discriminant < 0.0f || a <= 0.0f)
		return FALSE;
	t = (-b - sqrtf(discriminant)) / (2.0f * a);
	if (t < 0.0f || t >= collision->t)
		return FALSE;
	collision->t = t;
	collision->type = _collision_result_object;
	collision->object_index = object_index;
	return TRUE;
}

#include "under_test.inc"

static unsigned long random_state = 7;

static real random_real(real low, real high)
{
	random_state = random_state * 1103515245 + 12345;
	return low + (high - low) * (real)((random_state >> 8) & 0xFFFF) / 65535.0f;
}

static void make_level(void)
{
	long index, cluster;

	for (index = 0; index < WALLS; index++)
		wall_x[index] = random_real(0.0f, 40.0f);
	for (cluster = 0; cluster < CLUSTERS; cluster++)
	{
		cluster_counts[cluster] = 0;
		cluster_has_water[cluster] = random_real(0.0f, 1.0f) < 0.6f;
		cluster_fog_planes[cluster].plane.n.i = 0.0f;
		cluster_fog_planes[cluster].plane.n.j = 0.0f;
		cluster_fog_planes[cluster].plane.n.k = 1.0f;
		cluster_fog_planes[cluster].plane.d = random_real(-2.0f, 2.0f);
		cluster_fog_planes[cluster].runtime_material_type = (short)random_real(-2.0f, 40.0f);
		cluster_fogs[cluster].plane_distance = random_real(-1.0f, 1.0f);
	}
	for (index = 0; index < OBJECTS; index++)
	{
		object_centers[index].x = random_real(0.0f, 40.0f);
		object_centers[index].y = random_real(-5.0f, 5.0f);
		object_centers[index].z = random_real(-5.0f, 5.0f);
		object_radii[index] = random_real(0.2f, 1.5f);
		for (cluster = 0; cluster < CLUSTERS; cluster++)
		{
			if (object_centers[index].x + object_radii[index] >= cluster * 10.0f &&
				object_centers[index].x - object_radii[index] <= (cluster + 1) * 10.0f)
				cluster_objects[cluster][cluster_counts[cluster]++] = index;
		}
	}
}

/* every vector blocked by the first-hit test exactly when by the full one,
and some blocked by each of the structure, water and objects alone */
static void case_equivalent(void)
{
	#define OBJECTS_FLAGS (FLAG(_collision_test_objects_bit) | _collision_test_objects_all_types_flags)
	static unsigned long const tested[] = {
		FLAG(_collision_test_structure_bit),
		FLAG(_collision_test_media_bit),
		OBJECTS_FLAGS,
		FLAG(_collision_test_structure_bit) | FLAG(_collision_test_media_bit),
		FLAG(_collision_test_structure_bit) | OBJECTS_FLAGS,
		FLAG(_collision_test_media_bit) | OBJECTS_FLAGS,
		FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit) |
			FLAG(_collision_test_media_bit) | OBJECTS_FLAGS,
	};
	long blocked_by[3] = { 0, 0, 0 };
	long levels, ray, set, total_blocked = 0, total = 0;

	for (levels = 0; levels < 50; levels++)
	{
		make_level();
		for (ray = 0; ray < 400; ray++)
		{
			real_point3d point = { random_real(0.5f, 39.5f), random_real(-5.0f, 5.0f), random_real(-4.0f, 4.0f) };
			real_vector3d vector = { random_real(-20.0f, 20.0f), random_real(-6.0f, 6.0f), random_real(-6.0f, 6.0f) };
			long ignore = random_real(0.0f, 1.0f) < 0.3f ? (long)random_real(0.0f, OBJECTS - 1) : NONE;

			for (set = 0; set < (long)(sizeof(tested) / sizeof(tested[0])); set++)
			{
				struct collision_result nearest, first;
				boolean full = collision_test_vector(tested[set], &point, &vector, ignore, &nearest);
				boolean blocked = collision_port_test_vector_blocked(tested[set], &point, &vector, ignore, &first);

				CHECK(full == blocked, "level %ld ray %ld flags %lx: full test %d, first-hit test %d",
					levels, ray, tested[set], full, blocked);
				if (full && set < 3)
					blocked_by[set]++;
				total_blocked += full;
				total++;
			}
		}
	}
	CHECK(blocked_by[0] > 1000 && blocked_by[1] > 1000 && blocked_by[2] > 1000,
		"blocked by the structure %ld, water %ld, objects %ld times: too few to judge",
		blocked_by[0], blocked_by[1], blocked_by[2]);
	CHECK(total_blocked < total - total / 10, "%ld of %ld blocked: too few clear", total_blocked, total);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("equivalent") case_equivalent();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
