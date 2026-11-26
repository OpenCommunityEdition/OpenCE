/*
OBJECT_BOUNDS_CACHE.C

The objects' bounding spheres packed by absolute index, so collision's object
loops (collisions.c) pass over the objects of a crowded cluster that are far
from a sphere, or that a vector misses, without reading each one. Written by
their only writer (object_compute_node_matrices); an entry counts only for its
datum index and until the game state is next replaced (game_state.c). The
same tests on the same values: what the loops find does not change. (Kept out
of the game state, whose layout the saved games share.)

And each collision model bsp's bounding sphere (its vertices'), for
collision_model_test_vector to pass over the nodes of a model a vector does
not come near: a weapon falling among a pile of dead bodies tested each of
their nodes' bsps. Counts until the next map or game state, like the rest.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "physics/collision_bsp_definitions.h"
#include "tag_files/tag_groups.h"

#include <math.h>

struct object_bounds
{
	long object_index;
	long epoch;
	real_point3d center;
	real radius;
};

static struct object_bounds object_bounds[MAXIMUM_OBJECTS_PER_MAP];
static long object_bounds_epoch = 1;

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
	if (++object_bounds_epoch <= 0)
	{
		object_bounds_epoch = 1;
	}
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
sphere (collision's own test on the copy); FALSE when it is not known */
boolean object_bounds_cache_missed_by_vector(
	long object_index,
	real_point3d const *point,
	real_vector3d const *vector)
{
	struct object_bounds const *bounds = &object_bounds[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)];

	return bounds->object_index == object_index && bounds->epoch == object_bounds_epoch &&
		!fast_vector_intersects_sphere(point, vector, &bounds->center, bounds->radius);
}

#define COLLISION_BSP_BOUNDS_COUNT 8192
#define COLLISION_BSP_BOUNDS_PROBES 16

struct collision_bsp_bounds
{
	struct collision_bsp const *bsp;
	long epoch;
	real_point3d center;
	real radius;
};

static struct collision_bsp_bounds collision_bsp_bounds[COLLISION_BSP_BOUNDS_COUNT];

/* a sphere around all of the bsp's vertices (so around each of its surfaces),
with a margin for the bsp's own tolerances; FALSE when there is none to use.
Wait-free for concurrent readers: misses compute on the stack and claim a slot
with -epoch before publishing epoch with release ordering; hits acquire-load
epoch with no writes. */
boolean collision_bsp_bounds_cache_sphere(
	struct collision_bsp const *bsp,
	real_point3d *center,
	real *radius)
{
	unsigned long hash = ((unsigned long)bsp >> 4) * 2654435761UL;
	long epoch = __atomic_load_n(&object_bounds_epoch, __ATOMIC_RELAXED);
	boolean computed = FALSE;
	real_point3d computed_center = { 0.f, 0.f, 0.f };
	real computed_radius = -1.f;
	long probe;

	for (probe = 0; probe < COLLISION_BSP_BOUNDS_PROBES; probe++)
	{
		struct collision_bsp_bounds *entry =
			&collision_bsp_bounds[(hash + probe) % COLLISION_BSP_BOUNDS_COUNT];
		long entry_epoch = __atomic_load_n(&entry->epoch, __ATOMIC_ACQUIRE);

		if (entry_epoch == epoch && entry->bsp == bsp)
		{
			if (entry->radius < 0.f)
				return FALSE;
			*center = entry->center;
			*radius = entry->radius;
			return TRUE;
		}
		if (entry_epoch != epoch && entry_epoch != -epoch)
		{
			if (!computed)
			{
				long count = bsp->vertices.count;
				long i;
				real_point3d low, high;
				real largest = 0.f;

				computed = TRUE;
				if (count > 0)
				{
					low = high = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, 0, struct collision_vertex)->point;
					for (i = 1; i < count; i++)
					{
						real_point3d const *point = &TAG_BLOCK_GET_ELEMENT(&bsp->vertices, i, struct collision_vertex)->point;

						low.x = point->x < low.x ? point->x : low.x;
						low.y = point->y < low.y ? point->y : low.y;
						low.z = point->z < low.z ? point->z : low.z;
						high.x = point->x > high.x ? point->x : high.x;
						high.y = point->y > high.y ? point->y : high.y;
						high.z = point->z > high.z ? point->z : high.z;
					}
					computed_center.x = 0.5f * (low.x + high.x);
					computed_center.y = 0.5f * (low.y + high.y);
					computed_center.z = 0.5f * (low.z + high.z);
					for (i = 0; i < count; i++)
					{
						real_point3d const *point = &TAG_BLOCK_GET_ELEMENT(&bsp->vertices, i, struct collision_vertex)->point;
						real dx = point->x - computed_center.x;
						real dy = point->y - computed_center.y;
						real dz = point->z - computed_center.z;
						real distance_squared = dx * dx + dy * dy + dz * dz;

						largest = distance_squared > largest ? distance_squared : largest;
					}
					if (largest >= 0.f && largest < 1.0e12f)
					{
						computed_radius = 1.001f * (real)sqrt(largest) + 0.01f;
					}
				}
			}
			if (__atomic_compare_exchange_n(&entry->epoch, &entry_epoch, -epoch, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
			{
				entry->bsp = bsp;
				entry->center = computed_center;
				entry->radius = computed_radius;
				__atomic_store_n(&entry->epoch, epoch, __ATOMIC_RELEASE);
				if (computed_radius < 0.f)
					return FALSE;
				*center = computed_center;
				*radius = computed_radius;
				return TRUE;
			}
		}
	}
	if (computed && computed_radius >= 0.f)
	{
		*center = computed_center;
		*radius = computed_radius;
		return TRUE;
	}
	return FALSE;
}
