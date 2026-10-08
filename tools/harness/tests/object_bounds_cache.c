/*
OBJECT_BOUNDS_CACHE.C (test)

Collision's real object loop (collisions.c) over a fake crowded world, with and
without the object bounds cache: the features gathered, in order, must be the
same. test_object_bounds_cache.py takes the enums (enums.inc) and the code
(under_test.inc) from the sources.
*/

#include "harness.h"
#include "enums.inc"

/* objects: the members the loop reads */
struct object_datum
{
	struct
	{
		unsigned long flags, damage_flags;
		short type;
		long parent_object_index, next_object_index, first_child_object_index;
		real_point3d bounding_sphere_center;
		real bounding_sphere_radius;
	} object;
	struct { long player_index; short parent_seat_index; } unit;
	struct { unsigned long flags; } biped;
};
#define biped_datum object_datum
enum { OBJECTS = 600, CROWD = 450 };
static struct object_datum objects[OBJECTS];
#define OBJECT_INDEX(i) ((long)((((i) * 7 + 1) & 0x7FFF) << 16) | (i)) /* a datum index, salt and all */
#define object_get(object_index) (&objects[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)])

/* the world: a 4x4 grid of 10-unit clusters, each its own BSP leaf; an object is in every cluster its sphere touches */
enum { GRID = 4, CLUSTERS = GRID * GRID };
static long cluster_objects[CLUSTERS][OBJECTS], cluster_counts[CLUSTERS];
static int clusters_touched(real_point3d const *center, real radius, long *clusters)
{
	int x, y, count = 0;

	for (y = 0; y < GRID; y++)
		for (x = 0; x < GRID; x++)
			if (center->x + radius >= x * 10.0f && center->x - radius <= (x + 1) * 10.0f &&
				center->y + radius >= y * 10.0f && center->y - radius <= (y + 1) * 10.0f)
				clusters[count++] = y * GRID + x;
	return count;
}
static long cluster_get_first_collideable_object(long *reference, short cluster)
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
struct structure_bsp;
struct collision_bsp;
struct collision_bsp_test_sphere_result { int leaf_count; long leaf_indices[CLUSTERS]; };
#define global_structure_bsp_get() ((struct structure_bsp const *)NULL)
#define global_collision_bsp_get() ((struct collision_bsp const *)NULL)
#define collision_bsp_test_sphere(bsp, maximum, breakable, center, radius, result) \
	(((result)->leaf_count = clusters_touched(center, radius, (result)->leaf_indices)) > 0)
#define collision_cluster_index_from_leaf(leaf_index) (leaf_index)

/* each query visits a cluster and an object once */
static long cluster_marks[CLUSTERS], object_marks[OBJECTS], marker;
#define structure_cluster_marker_begin() (++marker)
#define structure_cluster_mark(cluster) (cluster_marks[cluster] != marker && (cluster_marks[cluster] = marker))
#define object_mark_function(object_index) \
	(object_marks[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)] != marker && \
		(object_marks[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)] = marker))

/* the features gathered: each object's, by type and query, hashed in order */
struct collision_feature_list { short count[NUMBER_OF_COLLISION_FEATURE_TYPES]; };
static unsigned long feature_hash, feature_total;
static void record(struct collision_feature_list *features, int type, long object_index, real a, real b)
{
	unsigned long values[4] = { (unsigned long)object_index, (unsigned long)type };
	int index;

	memcpy(&values[2], &a, sizeof(a));
	memcpy(&values[3], &b, sizeof(b));
	for (index = 0; index < 4; index++)
		feature_hash = (feature_hash ^ values[index]) * 16777619UL;
	features->count[type]++;
	feature_total++;
}
struct collision_model_instance { long object_index; };
struct physics_instance { long object_index; };
#define collision_features_new(features) memset(features, 0, sizeof(*(features)))
#define collision_features_from_point(point, height, width, object_index, a, b, c, d, features) \
	record(features, _collision_feature_sphere, object_index, (point)->x + (point)->y + (point)->z, (height) + (width))
#define collision_model_instance_new(instance, index) ((instance)->object_index = (index), TRUE)
#define collision_model_get_features_in_sphere(instance, center, radius, height, width, features) \
	record(features, _collision_feature_cylinder, (instance)->object_index, radius, (height) + (width))
#define physics_instance_new(instance, index) ((instance)->object_index = (index), TRUE)
#define physics_get_features_in_sphere(instance, center, radius, height, width, features) \
	record(features, _collision_feature_prism, (instance)->object_index, radius, (height) + (width))
#define biped_get_physics_pill(object_index, base, height, width) \
	(*(base) = object_get(object_index)->object.bounding_sphere_center, *(height) = 0.4f, *(width) = 0.2f)

/* not under test: the structure's own features, collision's statistics */
#define collision_bsp_get_features_in_sphere(...) ((void)0)
#define structure_cluster_marker_end() ((void)0)
#define object_marker_begin() ((void)0)
#define object_marker_end() ((void)0)
#define collision_log_usage(...) ((void)0)
#define collision_log_start_time(...) ((void)0)
#define collision_log_end_time(...) ((void)0)
#define debug_collision_skip_objects FALSE

#include "under_test.inc"

static unsigned long seed = 12345;
static real random_real(real low, real high)
{
	seed = seed * 1103515245UL + 12345UL;
	return low + (high - low) * (real)((seed >> 8) & 0xFFFF) / 65535.0f;
}

/* the clusters' lists from where the objects are (a child is reached through its parent) */
static void place_objects(void)
{
	long index, clusters[CLUSTERS];
	int count;

	memset(cluster_counts, 0, sizeof(cluster_counts));
	for (index = 0; index < OBJECTS; index++)
	{
		struct object_datum *object = &objects[index];

		if (object->object.parent_object_index != NONE)
			continue;
		for (count = clusters_touched(&object->object.bounding_sphere_center, object->object.bounding_sphere_radius, clusters);
			count--;)
			cluster_objects[clusters[count]][cluster_counts[clusters[count]]++] = OBJECT_INDEX(index);
	}
}

/* the crowd: three objects in four packed into one cluster, as a carrier swarm is; bipeds, scenery and vehicles, some
invisible, without collisions, dead, passing through bipeds or a player's; every 13th carries a child (a weapon) */
static void build_world(void)
{
	long index;

	for (index = 0; index < OBJECTS; index++)
	{
		struct object_datum *object = &objects[index];
		real extent = index < CROWD ? 3.0f : 20.0f;

		object->object.type = index % 10 < 7 ? _object_type_biped : index % 10 < 9 ? _object_type_scenery : _object_type_vehicle;
		object->object.flags = (index % 31 ? 0 : FLAG(_object_invisible_bit)) | (index % 37 ? 0 : FLAG(_object_no_collisions_bit));
		object->object.damage_flags = index % 41 ? 0 : FLAG(_object_dead_bit);
		object->biped.flags = index % 23 ? 0 : FLAG(_biped_movement_passes_through_bipeds_bit);
		object->unit.player_index = index % 50 ? NONE : 1;
		object->unit.parent_seat_index = NONE;
		object->object.parent_object_index = object->object.next_object_index = object->object.first_child_object_index = NONE;
		object->object.bounding_sphere_radius = random_real(0.2f, 1.5f);
		object->object.bounding_sphere_center.x = random_real(15.0f - extent, 15.0f + extent);
		object->object.bounding_sphere_center.y = random_real(15.0f - extent, 15.0f + extent);
		object->object.bounding_sphere_center.z = random_real(0.0f, 2.0f);
	}
	for (index = 0; index < OBJECTS; index += 13)
	{
		struct object_datum *parent = &objects[index], *child = &objects[(index + 300) % OBJECTS];

		if (parent->object.first_child_object_index == NONE && child->object.parent_object_index == NONE &&
			child->object.first_child_object_index == NONE)
		{
			parent->object.first_child_object_index = OBJECT_INDEX((index + 300) % OBJECTS);
			child->object.parent_object_index = OBJECT_INDEX(index);
		}
	}
	place_objects();
}

/* the queries: two in three in and around the crowd, with a mix of the flags the loop tests */
enum { QUERIES = 4000 };
static struct { real_point3d point; real radius; unsigned long flags; } queries[QUERIES];
static void build_queries(void)
{
	int index;

	for (index = 0; index < QUERIES; index++)
	{
		real extent = index % 3 ? 4.0f : 20.0f;

		queries[index].point.x = random_real(15.0f - extent, 15.0f + extent);
		queries[index].point.y = random_real(15.0f - extent, 15.0f + extent);
		queries[index].point.z = random_real(0.0f, 2.0f);
		queries[index].radius = random_real(0.3f, 2.5f);
		queries[index].flags = FLAG(_collision_test_objects_bit) |
			(index % 2 ? _collision_test_objects_all_types_flags : 0) |
			(index % 5 ? 0 : FLAG(_collision_test_skip_passthrough_bipeds_bit)) |
			(index % 7 ? 0 : FLAG(_collision_test_skip_player_bipeds_bit)) |
			(index % 11 ? 0 : FLAG(_collision_test_use_vehicle_physics_bit));
	}
}

/* every query's features, hashed in order */
static unsigned long run_queries(void)
{
	struct collision_feature_list features;
	int index;

	feature_hash = 2166136261UL;
	feature_total = 0;
	for (index = 0; index < QUERIES; index++)
		collision_get_features_in_sphere(queries[index].flags, &queries[index].point, queries[index].radius, 0.4f, 0.2f,
			OBJECT_INDEX(index % OBJECTS), &features);
	return feature_hash;
}

/* the same from the objects themselves: the cache emptied directly, not through its own code (the code under test) */
static unsigned long run_queries_without_cache(void)
{
	memset(object_bounds, 0xFF, sizeof(object_bounds));
	return run_queries();
}

/* every object's bounding sphere to the cache, as object_compute_node_matrices gives it */
static void tell_cache(void)
{
	long index;

	for (index = 0; index < OBJECTS; index++)
		object_bounds_cache_update(OBJECT_INDEX(index), &objects[index].object.bounding_sphere_center,
			objects[index].object.bounding_sphere_radius);
}

/* every third object moves; told: the cache with it */
static void move_some(boolean told)
{
	long index;

	for (index = 0; index < OBJECTS; index += 3)
	{
		objects[index].object.bounding_sphere_center.x += random_real(-1.5f, 1.5f);
		objects[index].object.bounding_sphere_center.y += random_real(-1.5f, 1.5f);
	}
	place_objects();
	if (told)
		tell_cache();
}

/* whether the cache, as the case left it, gathers what the objects themselves give */
static boolean same_features(void)
{
	unsigned long with = run_queries();

	return with == run_queries_without_cache();
}

static double seconds(unsigned long (*queries)(void))
{
	struct timespec start, end;
	int round;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (round = 0; round < 10; round++)
		queries();
	clock_gettime(CLOCK_MONOTONIC, &end);
	return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	build_world();
	build_queries();
	CASE("identical")
	{
		tell_cache();
		CHECK(same_features(), "the features differ with the cache");
		CHECK(feature_total > 1000, "only %lu features: the fake world is too sparse to compare", feature_total);
		return 0;
	}
	CASE("moving")
	{
		tell_cache();
		move_some(TRUE);
		move_some(TRUE);
		CHECK(same_features(), "the features differ after objects moved");
		return 0;
	}
	/* the comparison can fail: objects moved behind the cache's back change the features */
	CASE("detects-stale")
	{
		tell_cache();
		move_some(FALSE);
		CHECK(!same_features(), "a stale cache gathered the same features: the comparison sees nothing");
		return 0;
	}
	/* a replaced game state invalidates the cache, so the objects are read again */
	CASE("invalidated")
	{
		tell_cache();
		move_some(FALSE);
		object_bounds_cache_invalidate();
		CHECK(same_features(), "the features differ after the cache was invalidated");
		return 0;
	}
	/* not a check: how long the queries take without and with the cache */
	CASE("benchmark")
	{
		double without = seconds(run_queries_without_cache), with = (tell_cache(), seconds(run_queries));

		printf("%.1f ms without, %.1f ms with (%.2fx)\n", 1000.0 * without, 1000.0 * with, without / with);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
