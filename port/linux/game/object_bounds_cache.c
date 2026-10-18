/*
OBJECT_BOUNDS_CACHE.C

The objects' bounding spheres packed by absolute index, so collision's object
loop (collisions.c) passes over the far objects of a crowded cluster without
reading each one. Written by their only writer (object_compute_node_matrices);
an entry counts only for its datum index and until the game state is next
replaced (game_state.c). The same test on the same values: the features
gathered do not change. (Kept out of the game state, whose layout the saved
games share.)
*/

#include <stdio.h>
#include <string.h>
#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"

struct object_bounds
{
	long object_index;
	long epoch;
	real_point3d center;
	real radius;
};

static struct object_bounds object_bounds[MAXIMUM_OBJECTS_PER_MAP];
static long object_bounds_epoch = 1;

/* The inputs of an object's node matrices (objects.c: object_compute_node_matrices) as they were when it last
computed them, for an object that nothing moves or animates, and the bounding sphere that came of them. Counts for
its datum index and until the game state is next replaced, like the bounds. */

struct object_node_stamp
{
	long object_index;
	long epoch;
	struct object_node_stamp_key key;
	real_point3d center;
	real radius;
};

static struct object_node_stamp object_node_stamps[MAXIMUM_OBJECTS_PER_MAP];
static long object_node_stamps_on = 0;
static unsigned long object_node_stamps_hits = 0;
static unsigned long object_node_stamps_mismatches = 0;

/* 0: off, 1: on, 2: on, and each hit checked against the matrices computed anyway */
long object_node_stamps_mode(
	void)
{
	return object_node_stamps_on;
}

void object_node_stamps_set(
	long mode)
{
	object_node_stamps_on = mode;
	object_bounds_epoch++;
}

void object_node_stamp_checked(
	boolean same,
	long object_index)
{
	object_node_stamps_hits++;
	if (!same)
	{
		object_node_stamps_mismatches++;
		if (object_node_stamps_mismatches <= 10)
		{
			fprintf(stderr, "node stamps: object %08lx's matrices are not what the stamp says\n", (unsigned long)object_index);
		}
	}
}

void object_node_stamps_report(
	void)
{
	if (object_node_stamps_on > 1)
	{
		fprintf(stderr, "node stamps: %lu hits checked, %lu differ\n", object_node_stamps_hits, object_node_stamps_mismatches);
	}
}

boolean object_node_stamp_unchanged(
	long object_index,
	struct object_node_stamp_key const *key,
	real_point3d *center,
	real *radius)
{
	struct object_node_stamp const *stamp = &object_node_stamps[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	if (stamp->object_index == object_index && stamp->epoch == object_bounds_epoch &&
		memcmp(&stamp->key, key, sizeof(*key)) == 0)
	{
		*center = stamp->center;
		*radius = stamp->radius;
		return TRUE;
	}

	return FALSE;
}

void object_node_stamp_store(
	long object_index,
	struct object_node_stamp_key const *key,
	real_point3d const *center,
	real radius)
{
	struct object_node_stamp *stamp = &object_node_stamps[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	stamp->object_index = object_index;
	stamp->epoch = object_bounds_epoch;
	stamp->key = *key;
	stamp->center = *center;
	stamp->radius = radius;
}

void object_node_stamp_forget(
	long object_index)
{
	object_node_stamps[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)].object_index = NONE;
}

void object_bounds_cache_update(
	long object_index,
	real_point3d const *center,
	real radius)
{
	struct object_bounds *bounds = &object_bounds[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	bounds->object_index = object_index;
	bounds->epoch = object_bounds_epoch;
	bounds->center = *center;
	bounds->radius = radius;
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
