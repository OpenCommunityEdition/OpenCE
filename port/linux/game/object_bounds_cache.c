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

#include <stdlib.h>

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

struct cluster_objects
{
	long epoch;
	unsigned long changes;
	/* the object types listed (_collision_test_objects_all_types_flags's
	bits) */
	unsigned long types;
	/* when last asked for (cluster_objects_asked) */
	unsigned long asked;
	long count;
	long capacity;
	long *objects;
};

#define CLUSTER_OBJECTS_LISTS 2

static struct cluster_objects cluster_objects[MAXIMUM_CLUSTERS_PER_STRUCTURE][CLUSTER_OBJECTS_LISTS];
static unsigned long cluster_objects_asked;

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
	object_bounds_epoch++;
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

/* the cluster's collideable objects that are not bipeds, in its list's order,
for a vector that tests for no bipeds (count set); NULL when the vector tests
for bipeds, or the list is not to be had (the cluster's list walked then) */
long const *object_bounds_cache_cluster_objects(
	short cluster_index,
	unsigned long flags,
	long *count)
{
	struct cluster_objects *entry;
	unsigned long changes;
	unsigned long types = flags & _collision_test_objects_all_types_flags;
	long list;

	if (TEST_FLAG(flags, _collision_test_objects_bipeds_bit) ||
		cluster_index < 0 || cluster_index >= MAXIMUM_CLUSTERS_PER_STRUCTURE)
	{
		return NULL;
	}
	/* (the list of these types, or the one asked for longest ago) */
	entry = &cluster_objects[cluster_index][0];
	for (list = 0; list < CLUSTER_OBJECTS_LISTS; list++)
	{
		if (cluster_objects[cluster_index][list].types == types)
		{
			entry = &cluster_objects[cluster_index][list];
			break;
		}
		if (cluster_objects[cluster_index][list].asked < entry->asked)
			entry = &cluster_objects[cluster_index][list];
	}
	entry->asked = ++cluster_objects_asked;
	changes = cluster_port_collideable_object_changes(cluster_index);
	if (entry->epoch != object_bounds_epoch || entry->changes != changes || entry->types != types || !entry->objects)
	{
		long reference_index;
		long object_index;

		entry->count = 0;
		entry->types = types;
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
			if (entry->count >= entry->capacity)
			{
				long capacity = entry->capacity ? 2 * entry->capacity : 64;
				long *objects = (long *)realloc(entry->objects, capacity * sizeof(long));

				if (!objects)
				{
					entry->epoch = 0;
					return NULL;
				}
				entry->objects = objects;
				entry->capacity = capacity;
			}
			entry->objects[entry->count++] = object_index;
		}
		if (!entry->objects)
		{
			entry->objects = (long *)malloc(64 * sizeof(long));
			if (!entry->objects)
				return NULL;
			entry->capacity = 64;
		}
		entry->epoch = object_bounds_epoch;
		entry->changes = changes;
	}
	*count = entry->count;
	return entry->objects;
}
