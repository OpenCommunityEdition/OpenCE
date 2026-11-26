/*
OBJECT_REACH_GRID.C

The objects in reach of a sphere, found from where they are rather than by
going through every object of its clusters. A moving biped's pill
(collisions.c's collision_get_features_in_sphere) went through each object
of the clusters it touched, a crowd of hundreds in one cluster in network
co-op's extra enemies, to find the two or three near it.

The objects' bounding spheres, as object_bounds_cache.c holds them, are kept
in a grid of squares by where they are; a sphere's squares give the objects
that may be in its reach, each tested as collision tests it
(object_bounds_cache_out_of_reach). They are then put in the order the
clusters' lists give them: the clusters in the order collision takes them,
and within a cluster's list (collideable objects, cluster_partitions.c),
which puts each new reference first, the newest reference first. Each
reference is noted where it is put and taken out, with when it was put; a
loaded game state's lists, or ones no longer noted, are read again from the
lists themselves. Where a list holds an object whose bounding sphere is not
held (none yet, or the game state was replaced), or anything does not fit,
collision goes through the clusters' lists as before. The same objects, in
the same order, with the same tests: what collision finds does not change.
(Kept out of the game state, whose layout the saved games share.)
*/

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "objects/reference_lists.h"
#include "structures/cluster_partitions.h"
#include "structures/structures.h"
#include "halo_port_capacity.h"

boolean object_bounds_cache_out_of_reach(long object_index, real_point3d const *point, real radius);

/* ---------- constants */

enum
{
	REACH_GRID_BUCKETS = 4096, /* a power of two */
	REACH_GRID_SQUARES_PER_OBJECT = 16, /* more, and an object is in the list of large ones */
	REACH_GRID_QUERY_SQUARES = 64, /* more, and the clusters' lists are gone through */
	REACH_GRID_MAXIMUM_REFERENCES = HALO_PORT_MAXIMUM_CLUSTER_REFERENCES,
};

#define REACH_GRID_SQUARE_SIZE 1.0f
/* how far past its sphere an object's squares, and a query's, reach: past
any rounding of collision's own test (point_in_sphere) */
#define REACH_GRID_MARGIN 0.01f
/* farther out than this, an object is in the list of large ones */
#define REACH_GRID_LIMIT 30000.0f

/* ---------- globals */

/* the partition whose lists are noted (objects.c's collideable objects) */
struct cluster_partition *object_reach_grid_partition;

/* where each object is in the grid, by absolute index: the object whose
bounding sphere is held (NONE: none), its squares, or large */
static long grid_objects[MAXIMUM_OBJECTS_PER_MAP];
static short grid_x0[MAXIMUM_OBJECTS_PER_MAP], grid_y0[MAXIMUM_OBJECTS_PER_MAP];
static short grid_x1[MAXIMUM_OBJECTS_PER_MAP], grid_y1[MAXIMUM_OBJECTS_PER_MAP];
static boolean grid_large[MAXIMUM_OBJECTS_PER_MAP];
/* the squares' lists, by bucket: an object's nth square is node
absolute index * REACH_GRID_SQUARES_PER_OBJECT + n */
static long grid_buckets[REACH_GRID_BUCKETS];
static long grid_node_next[MAXIMUM_OBJECTS_PER_MAP * REACH_GRID_SQUARES_PER_OBJECT];
static long grid_node_previous[MAXIMUM_OBJECTS_PER_MAP * REACH_GRID_SQUARES_PER_OBJECT];
static short grid_node_bucket[MAXIMUM_OBJECTS_PER_MAP * REACH_GRID_SQUARES_PER_OBJECT];
/* the large objects' list */
static long grid_large_first;
static long grid_large_next[MAXIMUM_OBJECTS_PER_MAP], grid_large_previous[MAXIMUM_OBJECTS_PER_MAP];

/* the noted references, by absolute index: the cluster, the object, when it
was put (later is larger, within a cluster's list), and each object's list
of them */
static short reference_clusters[REACH_GRID_MAXIMUM_REFERENCES];
static long reference_objects[REACH_GRID_MAXIMUM_REFERENCES];
static unsigned long reference_times[REACH_GRID_MAXIMUM_REFERENCES];
static long reference_next[REACH_GRID_MAXIMUM_REFERENCES], reference_previous[REACH_GRID_MAXIMUM_REFERENCES];
static long object_first_references[MAXIMUM_OBJECTS_PER_MAP];
static unsigned long reference_time;
/* by cluster, its list's references whose object's bounding sphere is not
held */
static long cluster_references_unheld[MAXIMUM_CLUSTERS_PER_STRUCTURE];
/* the notes' state: unknown (to be read again from the lists), reading (one
thread is reading them; concurrent queries fall back to NONE), or ready */
enum
{
	REACH_GRID_REFERENCES_UNKNOWN = 0,
	REACH_GRID_REFERENCES_READING = 1,
	REACH_GRID_REFERENCES_READY = 2,
};
static long references_state = REACH_GRID_REFERENCES_UNKNOWN;
/* the grid is empty, not yet made so */
static boolean grid_made;

/* a query's clusters' places in its order, and the objects it met (per thread
so concurrent queries do not contend on shared scratch arrays) */
static __thread unsigned short query_mark;
static __thread unsigned short query_cluster_marks[MAXIMUM_CLUSTERS_PER_STRUCTURE];
static __thread short cluster_places[MAXIMUM_CLUSTERS_PER_STRUCTURE];
static __thread unsigned short query_object_marks[MAXIMUM_OBJECTS_PER_MAP];

/* ---------- private code */

static short reach_grid_square(
	real value)
{
	real square = value * (1.0f / REACH_GRID_SQUARE_SIZE);

	return (short)(square >= 0.0f ? (long)square : (long)square - ((real)(long)square != square));
}

static long reach_grid_bucket(
	short x,
	short y)
{
	return (long)(((unsigned long)(long)x * 73856093UL) ^ ((unsigned long)(long)y * 19349663UL)) & (REACH_GRID_BUCKETS - 1);
}

static boolean reach_grid_in_limit(
	real_point3d const *center,
	real radius)
{
	return center->x > -REACH_GRID_LIMIT && center->x < REACH_GRID_LIMIT &&
		center->y > -REACH_GRID_LIMIT && center->y < REACH_GRID_LIMIT &&
		radius >= 0.0f && radius < REACH_GRID_LIMIT;
}

/* the object (absolute index) out of the grid */
static void reach_grid_remove(
	long absolute_index)
{
	if (grid_objects[absolute_index] == NONE)
		return;
	if (grid_large[absolute_index])
	{
		long next = grid_large_next[absolute_index];
		long previous = grid_large_previous[absolute_index];

		if (previous == NONE)
			grid_large_first = next;
		else
			grid_large_next[previous] = next;
		if (next != NONE)
			grid_large_previous[next] = previous;
	}
	else
	{
		long count = (long)(grid_x1[absolute_index] - grid_x0[absolute_index] + 1) *
			(grid_y1[absolute_index] - grid_y0[absolute_index] + 1);
		long node = absolute_index * REACH_GRID_SQUARES_PER_OBJECT;
		long index;

		for (index = 0; index < count; index++, node++)
		{
			long next = grid_node_next[node];
			long previous = grid_node_previous[node];

			if (previous == NONE)
				grid_buckets[grid_node_bucket[node]] = next;
			else
				grid_node_next[previous] = next;
			if (next != NONE)
				grid_node_previous[next] = previous;
		}
	}
	grid_objects[absolute_index] = NONE;
}

/* the object (absolute index) into the grid, in the squares given or large */
static void reach_grid_insert(
	long absolute_index,
	long object_index,
	boolean large,
	short x0,
	short y0,
	short x1,
	short y1)
{
	grid_objects[absolute_index] = object_index;
	grid_large[absolute_index] = large;
	if (large)
	{
		grid_large_previous[absolute_index] = NONE;
		grid_large_next[absolute_index] = grid_large_first;
		if (grid_large_first != NONE)
			grid_large_previous[grid_large_first] = absolute_index;
		grid_large_first = absolute_index;
	}
	else
	{
		long node = absolute_index * REACH_GRID_SQUARES_PER_OBJECT;
		short x, y;

		grid_x0[absolute_index] = x0;
		grid_y0[absolute_index] = y0;
		grid_x1[absolute_index] = x1;
		grid_y1[absolute_index] = y1;
		for (y = y0; y <= y1; y++)
		{
			for (x = x0; x <= x1; x++, node++)
			{
				long bucket = reach_grid_bucket(x, y);

				grid_node_bucket[node] = (short)bucket;
				grid_node_previous[node] = NONE;
				grid_node_next[node] = grid_buckets[bucket];
				if (grid_buckets[bucket] != NONE)
					grid_node_previous[grid_buckets[bucket]] = node;
				grid_buckets[bucket] = node;
			}
		}
	}
}

/* the object's (absolute index's) references of the object counted as
held (-1) or not held (+1) in their clusters */
static void reach_grid_count_unheld(
	long absolute_index,
	long object_index,
	long change)
{
	long reference;

	for (reference = object_first_references[absolute_index]; reference != NONE; reference = reference_next[reference])
	{
		if (reference_objects[reference] == object_index)
			cluster_references_unheld[reference_clusters[reference]] += change;
	}
}

static boolean reach_grid_note(
	long reference_index,
	short cluster_index,
	long object_index,
	unsigned long time)
{
	long reference = DATUM_INDEX_TO_ABSOLUTE_INDEX(reference_index);
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	/* (a cluster without a list of its own keeps none: cluster_partitions.c's
	cluster_partition_get_first_reference) */
	if (cluster_index < 0 || cluster_index >= MAXIMUM_CLUSTERS_PER_STRUCTURE)
		return TRUE;
	if (reference < 0 || reference >= REACH_GRID_MAXIMUM_REFERENCES ||
		absolute_index < 0 || absolute_index >= MAXIMUM_OBJECTS_PER_MAP)
	{
		return FALSE;
	}
	reference_clusters[reference] = cluster_index;
	reference_objects[reference] = object_index;
	reference_times[reference] = time;
	reference_previous[reference] = NONE;
	reference_next[reference] = object_first_references[absolute_index];
	if (object_first_references[absolute_index] != NONE)
		reference_previous[object_first_references[absolute_index]] = reference;
	object_first_references[absolute_index] = reference;
	if (grid_objects[absolute_index] != object_index)
		cluster_references_unheld[cluster_index]++;
	return TRUE;
}

/* the notes, read again from the lists: within each cluster's list, the
first the latest */
static boolean reach_grid_read_references(
	void)
{
	struct cluster_partition *partition = object_reach_grid_partition;
	short cluster_index;

	if (!partition || !partition->data_reference_data || !partition->data_reference_data->valid ||
		!partition->cluster_first_data_references)
	{
		return FALSE;
	}
	csmemset(object_first_references, 0xFF, sizeof(object_first_references));
	csmemset(cluster_references_unheld, 0, sizeof(cluster_references_unheld));
	reference_time = 0;
	for (cluster_index = 0; cluster_index < MAXIMUM_CLUSTERS_PER_STRUCTURE; cluster_index++)
	{
		long count = 0;
		long reference_index;

		for (reference_index = partition->cluster_first_data_references[cluster_index];
			reference_index != NONE && count <= REACH_GRID_MAXIMUM_REFERENCES;
			count++)
		{
			reference_index = ((struct data_reference *)datum_get(partition->data_reference_data, reference_index))->next_reference_index;
		}
		if (count > REACH_GRID_MAXIMUM_REFERENCES)
		{
			return FALSE;
		}
		for (reference_index = partition->cluster_first_data_references[cluster_index];
			reference_index != NONE;
			count--)
		{
			struct data_reference const *reference =
				(struct data_reference const *)datum_get(partition->data_reference_data, reference_index);

			if (!reach_grid_note(reference_index, cluster_index, reference->datum_index, reference_time + (unsigned long)count))
				return FALSE;
			reference_index = reference->next_reference_index;
		}
		reference_time += REACH_GRID_MAXIMUM_REFERENCES;
	}
	return TRUE;
}

/* ---------- public code */

/* the game state is replaced: no bounding sphere is held (as
object_bounds_cache_invalidate), and the lists are read again */
void object_reach_grid_invalidate(
	void)
{
	csmemset(grid_objects, 0xFF, sizeof(grid_objects));
	csmemset(grid_buckets, 0xFF, sizeof(grid_buckets));
	grid_large_first = NONE;
	__atomic_store_n(&grid_made, TRUE, __ATOMIC_RELEASE);
	__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
}

/* an object's bounding sphere, as object_bounds_cache_update holds it */
void object_reach_grid_update(
	long object_index,
	real_point3d const *center,
	real radius)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
	long held;
	boolean large = !reach_grid_in_limit(center, radius);
	short x0 = 0, y0 = 0, x1 = 0, y1 = 0;

	if (!__atomic_load_n(&grid_made, __ATOMIC_ACQUIRE))
		object_reach_grid_invalidate();
	held = grid_objects[absolute_index];
	if (!large)
	{
		real reach = radius + REACH_GRID_MARGIN;

		x0 = reach_grid_square(center->x - reach);
		y0 = reach_grid_square(center->y - reach);
		x1 = reach_grid_square(center->x + reach);
		y1 = reach_grid_square(center->y + reach);
		large = (long)(x1 - x0 + 1) * (y1 - y0 + 1) > REACH_GRID_SQUARES_PER_OBJECT;
	}
	if (held == object_index && grid_large[absolute_index] == large &&
		(large || (grid_x0[absolute_index] == x0 && grid_y0[absolute_index] == y0 &&
			grid_x1[absolute_index] == x1 && grid_y1[absolute_index] == y1)))
	{
		return;
	}
	if (held != object_index && __atomic_load_n(&references_state, __ATOMIC_ACQUIRE) == REACH_GRID_REFERENCES_READY)
	{
		/* (its references' objects held and no longer held) */
		reach_grid_count_unheld(absolute_index, object_index, -1);
		if (held != NONE)
			reach_grid_count_unheld(absolute_index, held, 1);
	}
	reach_grid_remove(absolute_index);
	reach_grid_insert(absolute_index, object_index, large, x0, y0, x1, y1);
}

/* a reference put first in a cluster's list of the partition */
void object_reach_grid_reference_added(
	struct cluster_partition const *partition,
	long reference_index,
	short cluster_index,
	long object_index)
{
	if (partition != object_reach_grid_partition ||
		__atomic_load_n(&references_state, __ATOMIC_ACQUIRE) != REACH_GRID_REFERENCES_READY)
	{
		return;
	}
	if (reference_time >= 0x7FFFFFF0UL)
	{
		__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
		return;
	}
	if (!reach_grid_note(reference_index, cluster_index, object_index, ++reference_time))
		__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
}

/* a reference taken out of a cluster's list of the partition */
void object_reach_grid_reference_removed(
	struct cluster_partition const *partition,
	long reference_index)
{
	long reference = DATUM_INDEX_TO_ABSOLUTE_INDEX(reference_index);
	long absolute_index;
	long next, previous;

	if (partition != object_reach_grid_partition ||
		__atomic_load_n(&references_state, __ATOMIC_ACQUIRE) != REACH_GRID_REFERENCES_READY)
	{
		return;
	}
	if (reference < 0 || reference >= REACH_GRID_MAXIMUM_REFERENCES)
	{
		__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
		return;
	}
	absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(reference_objects[reference]);
	next = reference_next[reference];
	previous = reference_previous[reference];
	if (previous == NONE)
		object_first_references[absolute_index] = next;
	else
		reference_next[previous] = next;
	if (next != NONE)
		reference_previous[next] = previous;
	if (grid_objects[absolute_index] != reference_objects[reference])
		cluster_references_unheld[reference_clusters[reference]]--;
}

/* the partition's lists are no longer as noted (replaced, made anew) */
void object_reach_grid_forget(
	struct cluster_partition const *partition)
{
	if (partition == object_reach_grid_partition)
		__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
}

/* The objects of the clusters' lists (the partition's, in that order) in
reach of the sphere (object_bounds_cache_out_of_reach), each once, in the
order going through the lists meets them: at most maximum_count of them.
NONE: they are to be gone through. */
long object_reach_grid_objects_in_reach(
	short const *cluster_indices,
	long cluster_count,
	real_point3d const *center,
	real radius,
	long *object_indices,
	long maximum_count)
{
	long places[256];
	unsigned long times[256];
	long count = 0;
	long index;
	long state;
	short x0, y0, x1, y1, x, y;
	real reach = radius + REACH_GRID_MARGIN;

	if (!__atomic_load_n(&grid_made, __ATOMIC_ACQUIRE))
		return NONE;
	state = __atomic_load_n(&references_state, __ATOMIC_ACQUIRE);
	if (state != REACH_GRID_REFERENCES_READY)
	{
		long expected = REACH_GRID_REFERENCES_UNKNOWN;

		if (state == REACH_GRID_REFERENCES_READING ||
			!__atomic_compare_exchange_n(&references_state, &expected, REACH_GRID_REFERENCES_READING, FALSE,
				__ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		{
			return NONE;
		}
		if (!reach_grid_read_references())
		{
			__atomic_store_n(&references_state, REACH_GRID_REFERENCES_UNKNOWN, __ATOMIC_RELEASE);
			return NONE;
		}
		__atomic_store_n(&references_state, REACH_GRID_REFERENCES_READY, __ATOMIC_RELEASE);
	}
	if (!reach_grid_in_limit(center, radius) || maximum_count > NUMBEROF(places))
		return NONE;
	x0 = reach_grid_square(center->x - reach);
	y0 = reach_grid_square(center->y - reach);
	x1 = reach_grid_square(center->x + reach);
	y1 = reach_grid_square(center->y + reach);
	if ((long)(x1 - x0 + 1) * (y1 - y0 + 1) > REACH_GRID_QUERY_SQUARES)
		return NONE;

	if (++query_mark == 0)
	{
		csmemset(query_cluster_marks, 0, sizeof(query_cluster_marks));
		csmemset(query_object_marks, 0, sizeof(query_object_marks));
		query_mark = 1;
	}
	for (index = 0; index < cluster_count; index++)
	{
		short cluster_index = cluster_indices[index];

		/* (a cluster without a list of its own has none of the objects) */
		if (cluster_index >= 0 && cluster_index < MAXIMUM_CLUSTERS_PER_STRUCTURE &&
			query_cluster_marks[cluster_index] != query_mark)
		{
			/* (an object in its list whose bounding sphere is not held is
			not in the grid) */
			if (cluster_references_unheld[cluster_index] != 0)
				return NONE;
			query_cluster_marks[cluster_index] = query_mark;
			cluster_places[cluster_index] = (short)index;
		}
	}

	for (y = y0; y <= y1 + 1; y++)
	{
		for (x = x0; x <= x1; x++)
		{
			long node;

			/* (the row past the last: the large objects) */
			if (y > y1)
			{
				if (x > x0)
					break;
				node = grid_large_first;
			}
			else
			{
				node = grid_buckets[reach_grid_bucket(x, y)];
			}
			while (node != NONE)
			{
				long absolute_index = y > y1 ? node : node / REACH_GRID_SQUARES_PER_OBJECT;
				long object_index = grid_objects[absolute_index];

				node = y > y1 ? grid_large_next[node] : grid_node_next[node];
				if (query_object_marks[absolute_index] == query_mark)
					continue;
				query_object_marks[absolute_index] = query_mark;
				if (!object_bounds_cache_out_of_reach(object_index, center, radius))
				{
					/* where going through the lists meets it first: the
					earliest cluster, the latest reference */
					long reference = object_first_references[absolute_index];
					long place = NONE;
					unsigned long time = 0;

					for (; reference != NONE; reference = reference_next[reference])
					{
						short cluster_index = reference_clusters[reference];

						if (reference_objects[reference] == object_index && query_cluster_marks[cluster_index] == query_mark &&
							(place == NONE || cluster_places[cluster_index] < place ||
								(cluster_places[cluster_index] == place && reference_times[reference] > time)))
						{
							place = cluster_places[cluster_index];
							time = reference_times[reference];
						}
					}
					if (place != NONE)
					{
						long at;

						if (count >= maximum_count)
							return NONE;
						for (at = count; at > 0 &&
							(places[at - 1] > place || (places[at - 1] == place && times[at - 1] < time));
							at--)
						{
							places[at] = places[at - 1];
							times[at] = times[at - 1];
							object_indices[at] = object_indices[at - 1];
						}
						places[at] = place;
						times[at] = time;
						object_indices[at] = object_index;
						count++;
					}
				}
			}
		}
	}

	return count;
}
