/*
OBJECT_REACH_GRID.C (test)

Collision's real sphere test (collisions.c's collision_get_features_in_sphere)
over a fake crowded world whose clusters' lists are cluster_partitions.c's real
ones, telling object_reach_grid.c where the references are: with the grid, and
with the lists gone through as before, the features gathered, in order, and the
objects marked must be the same, while objects move, rejoin their clusters
(first in each list), die and are made again in their places, the lists fill,
and the game state is replaced. test_object_reach_grid.py takes the enums
(enums.inc) and the code (under_test.inc) from the sources.
*/

#include "harness.h"
#include <limits.h>
#include "enums.inc"

#define csmemset memset
typedef unsigned short word;
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define match_vassert(file, line, condition, message) ((void)0)
#define csprintf(buffer, ...) ((void)0)
#define error(...) ((void)0)
static char temporary[1];
static void *datum_try_and_get(struct data_array *array, long datum_index)
{
	struct datum_header *header;

	if (datum_index == NONE || DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_index) >= array->maximum_count)
		return NULL;
	header = datum_get(array, datum_index);
	return header->identifier == (short)((unsigned long)datum_index >> 16) ? header : NULL;
}
static unsigned long lists_full;
#define reference_list_port_full(array) ((void)lists_full++)

/* objects: the members the loop reads; each place's salt changes when an
object there dies and another is made */
struct object_datum
{
	struct
	{
		unsigned long flags, damage_flags;
		short type;
		long parent_object_index, next_object_index, first_child_object_index;
		real_point3d bounding_sphere_center;
		real bounding_sphere_radius;
		long first_cluster_reference;
		boolean connected;
	} object;
	struct { long player_index; short parent_seat_index; } unit;
	struct { unsigned long flags; } biped;
};
#define biped_datum object_datum
enum { OBJECTS = 600, CROWD = 450 };
static struct object_datum objects[OBJECTS];
static long salts[OBJECTS];
#define OBJECT_INDEX(i) ((long)(((salts[i] * 7 + (i) + 1) & 0x7FFF) << 16) | (i)) /* a datum index, salt and all */
#define object_get(object_index) (&objects[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)])

/* the world: a 4x4 grid of 10-unit clusters, each its own BSP leaf; an object is in every cluster its sphere touches */
enum { GRID = 4, CLUSTERS = GRID * GRID };
struct location { short cluster_index; };
static short clusters_touched(real_point3d const *center, real radius, short maximum_count, short *clusters)
{
	short x, y, count = 0;

	for (y = 0; y < GRID; y++)
		for (x = 0; x < GRID; x++)
			if (count < maximum_count &&
				center->x + radius >= x * 10.0f && center->x - radius <= (x + 1) * 10.0f &&
				center->y + radius >= y * 10.0f && center->y - radius <= (y + 1) * 10.0f)
				clusters[count++] = y * GRID + x;
	return count;
}
#define structure_clusters_in_sphere(cluster_index, position, radius, maximum_count, clusters) \
	clusters_touched(position, radius, maximum_count, clusters)

/* the partition of collideable objects, whose lists collision goes through */
static struct cluster_partition partition;
static long cluster_partition_port_no_cluster_first_reference;
#define cluster_get_first_collideable_object(reference, cluster) cluster_partition_get_first_datum(&partition, reference, cluster)
#define cluster_get_next_collideable_object(reference) cluster_partition_get_next_datum(&partition, reference)

/* the leaves a sphere touches, in an order that turns with each query */
static long leaf_turn;
struct structure_bsp;
struct collision_bsp;
struct collision_bsp_test_sphere_result { long leaf_count; long leaf_indices[256]; };
static boolean leaves_touched(real_point3d const *center, real radius, struct collision_bsp_test_sphere_result *result)
{
	short clusters[CLUSTERS];
	long count = clusters_touched(center, radius, CLUSTERS, clusters), index;

	for (index = 0; index < count; index++)
		result->leaf_indices[index] = clusters[(index + leaf_turn) % count];
	result->leaf_count = count;
	leaf_turn++;
	return count > 0;
}
#define global_structure_bsp_get() ((struct structure_bsp const *)NULL)
#define global_collision_bsp_get() ((struct collision_bsp const *)NULL)
#define collision_bsp_test_sphere(bsp, maximum, breakable, center, radius, result) leaves_touched(center, radius, result)
#define collision_cluster_index_from_leaf(leaf_index) (leaf_index)

/* each query visits a cluster and an object once; the objects' marks are kept, to compare */
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
struct collision_model_instance { long object_index; void const *model; int matrices[1]; };
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

/* the grid answers (the lists are gone through when it says NONE, or when going_through says to) */
static boolean going_through;
static unsigned long grid_answers;

#include "under_test.inc"

static unsigned long seed = 12345;
static unsigned long random_next(void)
{
	seed = seed * 1103515245UL + 12345UL;
	return (seed >> 8) & 0xFFFFFF;
}
static real random_real(real low, real high)
{
	return low + (high - low) * (real)(random_next() & 0xFFFF) / 65535.0f;
}

/* an object's place in its clusters' lists, as objects.c's object_reconnect_to_map gives it: out of them, and in
again, first in each */
static void reconnect(long index)
{
	struct object_datum *object = &objects[index];
	struct location location = { 0 };

	if (object->object.parent_object_index != NONE)
		return;
	if (object->object.connected)
		cluster_partition_disconnect(&partition, OBJECT_INDEX(index), &object->object.first_cluster_reference);
	cluster_partition_reconnect(&partition, OBJECT_INDEX(index), &object->object.first_cluster_reference,
		&object->object.bounding_sphere_center, object->object.bounding_sphere_radius, &location);
	object->object.connected = TRUE;
}

/* its bounding sphere, as object_compute_node_matrices gives it */
static void tell_cache(long index)
{
	object_bounds_cache_update(OBJECT_INDEX(index), &objects[index].object.bounding_sphere_center,
		objects[index].object.bounding_sphere_radius);
}

static void make_object(long index)
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
	/* (now and then a large one: a vehicle or a piece of scenery the size of a building) */
	object->object.bounding_sphere_radius = index % 97 ? random_real(0.2f, 1.5f) : random_real(4.0f, 12.0f);
	object->object.bounding_sphere_center.x = random_real(15.0f - extent, 15.0f + extent);
	object->object.bounding_sphere_center.y = random_real(15.0f - extent, 15.0f + extent);
	object->object.bounding_sphere_center.z = random_real(0.0f, 2.0f);
	object->object.first_cluster_reference = NONE;
	object->object.connected = FALSE;
}

/* the world, its objects in their clusters and (told) the cache; every 13th carries a child (a weapon) */
static void build_world(boolean told, long references)
{
	long index;

	memset(&partition, 0, sizeof(partition));
	partition.cluster_first_data_references = malloc(MAXIMUM_CLUSTERS_PER_STRUCTURE * sizeof(long));
	memset(partition.cluster_first_data_references, 0xFF, MAXIMUM_CLUSTERS_PER_STRUCTURE * sizeof(long));
	partition.data_reference_data = game_state_data_new("cluster references", references, sizeof(struct data_reference));
	partition.cluster_reference_data = game_state_data_new("references clusters", references, sizeof(struct data_reference));
	partition.port_previous_references = malloc(HALO_PORT_MAXIMUM_CLUSTER_REFERENCES * sizeof(long));
	partition.port_cluster_data_references = malloc(HALO_PORT_MAXIMUM_CLUSTER_REFERENCES * sizeof(long));
	object_reach_grid_partition = &partition;
	cluster_partition_port_forget(&partition);
	for (index = 0; index < OBJECTS; index++)
		make_object(index);
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
	for (index = 0; index < OBJECTS; index++)
	{
		if (told)
			tell_cache(index);
		reconnect(index);
	}
}

/* a tick: a third of the objects move and rejoin their clusters, the cache told before or after; a few die and are
made again in their places, the cache told before they join their clusters or after; the still ones rejoin too, and
the cache is told of every object, but for one in untold (0: none) */
static void tick(long untold)
{
	static boolean told[OBJECTS];
	long index;

	memset(told, 0, sizeof(told));
	for (index = 0; index < OBJECTS; index++)
	{
		unsigned long choice = random_next() % 100;
		struct object_datum *object = &objects[index];
		boolean before = random_next() % 2;

		if (object->object.parent_object_index != NONE || object->object.first_child_object_index != NONE)
			continue;
		if (choice < 2)
		{
			if (object->object.connected)
				cluster_partition_disconnect(&partition, OBJECT_INDEX(index), &object->object.first_cluster_reference);
			salts[index]++;
			make_object(index);
		}
		else if (choice < 35)
		{
			object->object.bounding_sphere_center.x += random_real(-1.5f, 1.5f);
			object->object.bounding_sphere_center.y += random_real(-1.5f, 1.5f);
		}
		else if (choice >= 60)
		{
			continue;
		}
		if (before && (!untold || random_next() % untold))
		{
			tell_cache(index);
			told[index] = TRUE;
		}
		reconnect(index);
	}
	for (index = 0; index < OBJECTS; index++)
		if (!told[index] && (!untold || random_next() % untold))
			tell_cache(index);
}

/* a query with the grid and with the lists gone through: the same features in the same order, the same objects
marked */
static unsigned long queries_compared;
static void compare_query(long query)
{
	struct collision_feature_list features;
	real_point3d point;
	real extent = query % 3 ? 4.0f : 20.0f;
	real radius = query % 50 ? random_real(0.3f, 2.5f) : random_real(4.0f, 12.0f);
	unsigned long flags = FLAG(_collision_test_objects_bit) |
		(query % 2 ? _collision_test_objects_all_types_flags : 0) |
		(query % 5 ? 0 : FLAG(_collision_test_skip_passthrough_bipeds_bit)) |
		(query % 7 ? 0 : FLAG(_collision_test_skip_player_bipeds_bit)) |
		(query % 11 ? 0 : FLAG(_collision_test_use_vehicle_physics_bit));
	long ignore = OBJECT_INDEX(query % OBJECTS);
	unsigned long hash_grid, hash_lists, total_grid;
	static long marks_grid[OBJECTS];
	long index, turn;

	point.x = random_real(15.0f - extent, 15.0f + extent);
	point.y = random_real(15.0f - extent, 15.0f + extent);
	point.z = random_real(0.0f, 2.0f);
	turn = leaf_turn;

	feature_hash = 2166136261UL;
	feature_total = 0;
	going_through = FALSE;
	collision_get_features_in_sphere(flags, &point, radius, 0.4f, 0.2f, ignore, &features);
	hash_grid = feature_hash;
	total_grid = feature_total;
	for (index = 0; index < OBJECTS; index++)
		marks_grid[index] = object_marks[index] == marker;

	leaf_turn = turn;
	feature_hash = 2166136261UL;
	feature_total = 0;
	going_through = TRUE;
	collision_get_features_in_sphere(flags, &point, radius, 0.4f, 0.2f, ignore, &features);
	hash_lists = feature_hash;
	going_through = FALSE;

	CHECK(hash_grid == hash_lists && total_grid == feature_total,
		"query %ld: %lu features with the grid, %lu going through the lists, or in another order",
		query, total_grid, feature_total);
	for (index = 0; index < OBJECTS; index++)
		CHECK(marks_grid[index] == (object_marks[index] == marker), "query %ld: object %ld marked by one and not the other",
			query, index);
	queries_compared++;
}

static void compare_queries(long count)
{
	long query;

	for (query = 0; query < count; query++)
		compare_query(query);
}

/* a saved game state: the clusters' lists and the objects */
static struct
{
	long first_references[MAXIMUM_CLUSTERS_PER_STRUCTURE];
	struct data_array data_references, cluster_references;
	unsigned char *data_reference_data, *cluster_reference_data;
	struct object_datum objects[OBJECTS];
	long salts[OBJECTS];
} saved;

static void copy_array(struct data_array *to, unsigned char **to_data, struct data_array const *from)
{
	if (!*to_data)
		*to_data = malloc((size_t)(from->maximum_count * from->size));
	memcpy(*to_data, from->data, (size_t)(from->maximum_count * from->size));
	*to = *from;
	to->data = *to_data;
}

static void save_game_state(void)
{
	memcpy(saved.first_references, partition.cluster_first_data_references, sizeof(saved.first_references));
	copy_array(&saved.data_references, &saved.data_reference_data, partition.data_reference_data);
	copy_array(&saved.cluster_references, &saved.cluster_reference_data, partition.cluster_reference_data);
	memcpy(saved.objects, objects, sizeof(objects));
	memcpy(saved.salts, salts, sizeof(salts));
}

static void load_game_state(void)
{
	unsigned char *data;

	memcpy(partition.cluster_first_data_references, saved.first_references, sizeof(saved.first_references));
	data = partition.data_reference_data->data;
	copy_array(partition.data_reference_data, &data, &saved.data_references);
	data = partition.cluster_reference_data->data;
	copy_array(partition.cluster_reference_data, &data, &saved.cluster_references);
	memcpy(objects, saved.objects, sizeof(objects));
	memcpy(salts, saved.salts, sizeof(salts));
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long round, index;

	/* objects moving and rejoining their clusters, dying and made again */
	CASE("moving")
	{
		build_world(TRUE, HALO_PORT_MAXIMUM_CLUSTER_REFERENCES);
		for (round = 0; round < 60; round++)
		{
			tick(0);
			compare_queries(300);
		}
		printf("%lu queries, %lu answered by the grid\n", queries_compared, grid_answers);
		CHECK(grid_answers * 10 > queries_compared * 8, "the grid answered %lu of %lu queries: too few to compare",
			grid_answers, queries_compared);
		return 0;
	}
	/* objects whose bounding spheres the cache does not hold: their clusters' lists are gone through */
	CASE("unheld")
	{
		build_world(TRUE, HALO_PORT_MAXIMUM_CLUSTER_REFERENCES);
		for (round = 0; round < 40; round++)
		{
			tick(2);
			compare_queries(300);
		}
		printf("%lu queries, %lu answered by the grid\n", queries_compared, grid_answers);
		CHECK(grid_answers * 10 > queries_compared && grid_answers * 2 < queries_compared,
			"the grid answered %lu of %lu queries: too few or too many to compare", grid_answers, queries_compared);
		return 0;
	}
	/* the game state replaced by one saved earlier: the lists and the objects as they were, the cache emptied and the
	lists to be read again (game_state.c's before-load procs), then the objects told again; queried at once */
	CASE("replaced")
	{
		build_world(TRUE, HALO_PORT_MAXIMUM_CLUSTER_REFERENCES);
		for (round = 0; round < 40; round++)
		{
			if (round % 5 == 1)
				save_game_state();
			if (round % 5 == 3)
			{
				load_game_state();
				cluster_partition_port_forget(&partition);
				object_bounds_cache_invalidate();
				for (index = 0; index < OBJECTS; index++)
					tell_cache(index);
				compare_queries(300);
			}
			tick(0);
			compare_queries(300);
		}
		printf("%lu queries, %lu answered by the grid\n", queries_compared, grid_answers);
		CHECK(grid_answers * 10 > queries_compared * 5, "the grid answered %lu of %lu queries: too few to compare",
			grid_answers, queries_compared);
		return 0;
	}
	/* full lists: the references that did not fit are not in them, nor found by the grid */
	CASE("full")
	{
		build_world(TRUE, 500);
		for (round = 0; round < 40; round++)
		{
			tick(0);
			compare_queries(300);
		}
		printf("%lu queries, %lu answered by the grid, %lu references did not fit\n", queries_compared, grid_answers,
			lists_full);
		CHECK(lists_full > 1000, "only %lu references did not fit: the lists were not full", lists_full);
		CHECK(grid_answers * 10 > queries_compared * 8, "the grid answered %lu of %lu queries: too few to compare",
			grid_answers, queries_compared);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
