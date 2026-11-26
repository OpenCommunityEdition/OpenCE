/*
OBJECT_BOUNDS_CACHE.C

The objects' bounding spheres and types packed by absolute index, so
collision's object loops (collisions.c) pass over the objects of a crowded
cluster that are far from a sphere, or that a vector misses or does not test
for, without reading each one. Written by
their only writer (object_compute_node_matrices); an entry counts only for its
datum index and until the game state is next replaced (game_state.c). The
same tests on the same values: what the loops find does not change. (Kept out
of the game state, whose layout the saved games share.)

And each cluster's collideable objects that are not bipeds, in the order of
the cluster's list, for the vectors that test for no bipeds (an AI's line of
sight, a dead body's limbs): with hundreds of bipeds in a cluster, such a
vector passed over each of them in the list. Only the objects of the types
the vector tests for: a sound's wall test (game_sound.c) tests for scenery
and machines, past the hundreds of weapons a crowd drops. A cluster keeps
lists for two sets of types, the ones last asked for. A cluster's list is made from
its list when first wanted, and made again once the list has changed
(cluster_port_collideable_object_changes): the vectors come many at a time
while nothing moves (each AI hearing a unit hurt, a body's limbs). The
objects the vectors would have tested, in the same order: what they find
does not change either.

*/

#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "structures/structures.h"

struct object_bounds
{
	long object_index;
	long epoch;
	real_point3d center;
	real radius;
	short type;
};

static struct object_bounds object_bounds[MAXIMUM_OBJECTS_PER_MAP];
static long object_bounds_epoch = 1;

#define CLUSTER_OBJECTS_LISTS 2
#define CLUSTER_OBJECTS_CAPACITY 256

struct cluster_objects
{
	long epoch;
	unsigned long changes;
	/* the object types listed (_collision_test_objects_all_types_flags's
	bits) */
	unsigned long types;
	/* NONE when more than CLUSTER_OBJECTS_CAPACITY objects matched */
	long count;
	long objects[CLUSTER_OBJECTS_CAPACITY];
};

struct cluster_objects_slot
{
	/* odd while a thread is rebuilding one of this cluster's lists */
	unsigned long sequence;
	unsigned long next_list;
	struct cluster_objects lists[CLUSTER_OBJECTS_LISTS];
};

static struct cluster_objects_slot cluster_objects[MAXIMUM_CLUSTERS_PER_STRUCTURE];

void object_bounds_cache_update(
	long object_index,
	short type,
	real_point3d const *center,
	real radius)
{
	struct object_bounds *bounds = &object_bounds[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	bounds->object_index = object_index;
	bounds->epoch = object_bounds_epoch;
	bounds->center = *center;
	bounds->radius = radius;
	bounds->type = type;
}

void object_bounds_cache_invalidate(
	void)
{
	long epoch = __atomic_load_n(&object_bounds_epoch, __ATOMIC_RELAXED) + 1;

	if (epoch <= 0)
		epoch = 1;
	__atomic_store_n(&object_bounds_epoch, epoch, __ATOMIC_RELEASE);
}

/* whether the object's bounding sphere is known to be out of the reach of
the point (collision's own test on the copy); FALSE when it is not known */
boolean object_bounds_cache_out_of_reach(
	long object_index,
	real_point3d const *point,
	real radius)
{
	struct object_bounds const *bounds = &object_bounds[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	return bounds->object_index == object_index && bounds->epoch == object_bounds_epoch &&
		!point_in_sphere(point, &bounds->center, bounds->radius + radius);
}

/* whether the vector from the point is known to miss the object's bounding
sphere, or the object is of a type the test's flags leave out (collision's
own tests on the copy, the type first: object_test_vector tests neither it
nor what is attached to it then, and a crowd of dead bodies' limbs test only
for scenery and machines, past hundreds of bipeds); FALSE when it is not
known */
boolean object_bounds_cache_missed_by_vector(
	long object_index,
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector)
{
	struct object_bounds const *bounds = &object_bounds[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	return bounds->object_index == object_index && bounds->epoch == object_bounds_epoch &&
		(!TEST_FLAG(flags, bounds->type + _collision_test_objects_first_type_bit) ||
			!fast_vector_intersects_sphere(point, vector, &bounds->center, bounds->radius));
}

/* copies the cluster's collideable objects that are not bipeds, in its list's
order, into object_indices[0..maximum_count-1], returning how many were copied;
NONE when the vector tests for bipeds, or the list is not to be had or does not
fit (the cluster's list walked then). Readers copy out under a per-cluster
seqlock with no shared writes on a hit, and fall back to NONE if another thread
is rebuilding the cluster's list. */
long object_bounds_cache_cluster_objects(
	short cluster_index,
	unsigned long flags,
	long *object_indices,
	long maximum_count)
{
	struct cluster_objects_slot *slot;
	long epoch;
	unsigned long changes;
	unsigned long types = flags & _collision_test_objects_all_types_flags;
	int attempt;

	if (TEST_FLAG(flags, _collision_test_objects_bipeds_bit) ||
		cluster_index < 0 || cluster_index >= MAXIMUM_CLUSTERS_PER_STRUCTURE ||
		!object_indices || maximum_count <= 0)
	{
		return NONE;
	}
	slot = &cluster_objects[cluster_index];
	epoch = __atomic_load_n(&object_bounds_epoch, __ATOMIC_ACQUIRE);
	changes = cluster_port_collideable_object_changes(cluster_index);

	for (attempt = 0; attempt < 8; attempt++)
	{
		unsigned long sequence = __atomic_load_n(&slot->sequence, __ATOMIC_ACQUIRE);
		long list;

		if (sequence & 1)
			return NONE;

		for (list = 0; list < CLUSTER_OBJECTS_LISTS; list++)
		{
			struct cluster_objects const *entry = &slot->lists[list];
			long entry_epoch = __atomic_load_n(&entry->epoch, __ATOMIC_RELAXED);
			unsigned long entry_changes = __atomic_load_n(&entry->changes, __ATOMIC_RELAXED);
			unsigned long entry_types = __atomic_load_n(&entry->types, __ATOMIC_RELAXED);

			if (entry_epoch == epoch && entry_changes == changes && entry_types == types)
			{
				long count = __atomic_load_n(&entry->count, __ATOMIC_RELAXED);
				long index;

				if (count >= 0 && count <= CLUSTER_OBJECTS_CAPACITY && count <= maximum_count)
				{
					for (index = 0; index < count; index++)
						object_indices[index] = __atomic_load_n(&entry->objects[index], __ATOMIC_RELAXED);
				}
				else
				{
					count = NONE;
				}
				__atomic_thread_fence(__ATOMIC_ACQUIRE);
				if (__atomic_load_n(&slot->sequence, __ATOMIC_RELAXED) == sequence)
					return count;
				goto retry;
			}
		}

		/* miss: claim this cluster's sequence (even -> odd) to rebuild one list */
		if (__atomic_compare_exchange_n(&slot->sequence, &sequence, sequence + 1, FALSE,
			__ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		{
			struct cluster_objects *entry = NULL;
			long reference_index;
			long object_index;
			long count = 0;

			/* (prefer a list already holding these types, else a stale one, else round-robin) */
			for (list = 0; list < CLUSTER_OBJECTS_LISTS; list++)
			{
				if (slot->lists[list].types == types)
				{
					entry = &slot->lists[list];
					break;
				}
			}
			if (!entry)
			{
				for (list = 0; list < CLUSTER_OBJECTS_LISTS; list++)
				{
					if (slot->lists[list].epoch != epoch || slot->lists[list].changes != changes)
					{
						entry = &slot->lists[list];
						break;
					}
				}
			}
			if (!entry)
			{
				entry = &slot->lists[slot->next_list % CLUSTER_OBJECTS_LISTS];
				slot->next_list++;
			}

			/* order the odd sequence store before any relaxed writes to entry */
			__atomic_thread_fence(__ATOMIC_RELEASE);

			for (object_index = cluster_get_first_collideable_object(&reference_index, cluster_index);
				object_index != NONE;
				object_index = cluster_get_next_collideable_object(&reference_index))
			{
				short type = object_get(object_index)->object.type;

				/* (a type the vector does not test for: object_test_vector
				tests neither the object nor what is attached to it) */
				if (type == _object_type_biped ||
					!TEST_FLAG(types, type + _collision_test_objects_first_type_bit))
					continue;
				if (count >= CLUSTER_OBJECTS_CAPACITY)
				{
					count = NONE;
					break;
				}
				__atomic_store_n(&entry->objects[count], object_index, __ATOMIC_RELAXED);
				if (count < maximum_count)
					object_indices[count] = object_index;
				count++;
			}
			__atomic_store_n(&entry->count, count, __ATOMIC_RELAXED);
			__atomic_store_n(&entry->types, types, __ATOMIC_RELAXED);
			__atomic_store_n(&entry->changes, changes, __ATOMIC_RELAXED);
			__atomic_store_n(&entry->epoch, epoch, __ATOMIC_RELAXED);
			__atomic_store_n(&slot->sequence, sequence + 2, __ATOMIC_RELEASE);
			return count != NONE && count <= maximum_count ? count : NONE;
		}
	retry:
		;
	}
	return NONE;
}
