/* A depth-only batch for the visibility field. The simulation cannot advance
   during these synchronous render queries. Cache cluster membership and object
   metadata for this batch, but use the engine's exact BSP/model intersection
   routines for every ray. Overflow or unsupported inputs use the stock query. */
#include "cseries.h"
#include "plasma_collision.h"
#include "physics/collisions.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_models.h"
#include "physics/breakable_surfaces.h"
#include "objects/objects.h"
#include "structures/structure_bsp_definitions.h"
#include "scenario/scenario.h"
#include <string.h>
#include <math.h>

extern boolean debug_collision_skip_vectors, debug_collision_skip_objects;

enum { TRACE_OBJECTS = 512, TRACE_REFERENCES = 4096 };
struct trace_object
{
	long index, next, child;
	real_point3d center;
	real radius;
	real_point3d relative;
	real sphere_distance;
	int eligible, model_state;
	struct collision_model_instance model;
	unsigned int pending[PLASMA_COLLISION_WORDS];
};
static struct
{
	int active, failed, count, references, defer_models;
	unsigned long flags, ray;
	long ignore;
	real_point3d const *point;
	struct structure_bsp const *structure;
	struct collision_bsp const *bsp;
	real extent;
	real vector_squared;
	struct { int ready; long node; short depth; } prefixes[8];
	struct collision_bsp_vector_cache cache[8];
	struct trace_object objects[TRACE_OBJECTS];
	unsigned short lookup[MAXIMUM_OBJECTS_PER_MAP];
	unsigned long visited[MAXIMUM_OBJECTS_PER_MAP];
	struct { int ready, first, count; unsigned long visited; } clusters[MAXIMUM_CLUSTERS_PER_STRUCTURE];
	long roots[TRACE_REFERENCES];
} batch;

/* All endpoints, and the origin, must lie strictly on the same side.
   The margin leaves rounding and borderline planes to engine traversal. */
static int trace_side(real_plane3d const *plane, real distance, int octant)
{
	real bound, margin, low, high, components[3];
	int axis;
	bound = batch.extent * (fabsf(plane->n.i) + fabsf(plane->n.j) + fabsf(plane->n.k));
	margin = 0.0001f * (1 + bound + fabsf(plane->d) + fabsf(plane->n.i * batch.point->x) +
		fabsf(plane->n.j * batch.point->y) + fabsf(plane->n.k * batch.point->z));
	low = high = distance;
	components[0] = plane->n.i;
	components[1] = plane->n.j;
	components[2] = plane->n.k;
	for (axis = 0; axis < 3; axis++)
	{
		real endpoint = components[axis] * ((octant & (1 << axis)) ? batch.extent : -batch.extent);
		if (endpoint < 0) low += endpoint;
		else high += endpoint;
	}
	return low > margin ? 1 : high < -margin ? 0 : -1;
}

/* Prepare only the part of the world tree that this ray octant can reach.
   One-sided chains below branches are just as invariant as the common root
   prefix. Keep exact engine traversal at intersecting/borderline planes.
   The bounded cache may evict entries; a miss only loses the optimisation. */
static void trace_prepare(long index, short depth, int octant, int *work, long *target, short *skipped)
{
	struct bsp3d_node const *node;
	real_plane3d const *plane;
	real distance;
	long child_target, slot;
	short child_skipped;
	int side;
	*target = index;
	*skipped = 0;
	if (index & LONG_MIN) return;
	if (*work <= 0 || index >= batch.bsp->bsp3d.nodes.count ||
		depth >= MAXIMUM_BSP3D_TRAVERSAL_DEPTH || depth >= batch.bsp->bsp3d.nodes.count) return;
	--*work;
	node = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.nodes, index, struct bsp3d_node);
	if (node->plane_designator < 0 || node->plane_designator >= batch.bsp->bsp3d.planes.count) return;
	plane = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.planes, node->plane_designator, real_plane3d);
	distance = plane3d_distance_to_point(plane, batch.point);
	side = trace_side(plane, distance, octant);
	if (side >= 0)
	{
		trace_prepare(node->children[side], depth + 1, octant, work, target, skipped);
		++*skipped;
	}
	else
	{
		trace_prepare(node->children[0], depth + 1, octant, work, &child_target, &child_skipped);
		trace_prepare(node->children[1], depth + 1, octant, work, &child_target, &child_skipped);
	}
	slot = (unsigned long)index % COLLISION_VECTOR_CACHE_NODES;
	batch.cache[octant].nodes[slot].index_plus_one = index + 1;
	batch.cache[octant].nodes[slot].node = node;
	batch.cache[octant].nodes[slot].plane = plane;
	batch.cache[octant].nodes[slot].distance = distance;
	batch.cache[octant].nodes[slot].skip_node = *target;
	batch.cache[octant].nodes[slot].skip_count = *skipped;
}

/* Each octant shares the origin and one eighth of the batch's endpoint cube.
   Prove its common tree prefix once, then retain the original traversal below
   it. Both the origin and the entire endpoint box must be strictly on one
   side; convexity then covers every segment. Margins leave borderline planes
   to the stock arithmetic. Malformed trees keep the original root entry. */
static void trace_start(int octant)
{
	long node_index = 0;
	short depth = 0;
	real extent = batch.extent;
	if (!(extent > 0 && extent < 1000)) return;
	while (!(node_index & LONG_MIN))
	{
		struct bsp3d_node const *node;
		real_plane3d const *plane;
		real distance;
		int side;
		if (node_index >= batch.bsp->bsp3d.nodes.count ||
			depth >= MAXIMUM_BSP3D_TRAVERSAL_DEPTH || depth >= batch.bsp->bsp3d.nodes.count)
			return;
		node = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.nodes, node_index, struct bsp3d_node);
		if (node->plane_designator < 0 || node->plane_designator >= batch.bsp->bsp3d.planes.count)
			return;
		plane = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.planes, node->plane_designator, real_plane3d);
		distance = plane3d_distance_to_point(plane, batch.point);
		side = trace_side(plane, distance, octant);
		if (side < 0) break;
		node_index = node->children[side];
		depth++;
	}
	batch.prefixes[octant].node = node_index;
	batch.prefixes[octant].depth = depth;
}

static struct trace_object *trace_object(long index)
{
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	struct trace_object *cached;
	struct object_datum const *object;
	real bound, margin;
	if (slot >= MAXIMUM_OBJECTS_PER_MAP) { batch.failed = 1; return NULL; }
	if (batch.lookup[slot])
	{
		cached = &batch.objects[batch.lookup[slot]-1];
		if (cached->index == index) return cached;
		batch.failed = 1;
		return NULL;
	}
	if (batch.count == TRACE_OBJECTS) { batch.failed = 1; return NULL; }
	object = object_get(index);
	cached = &batch.objects[batch.count++];
	batch.lookup[slot] = (unsigned short)batch.count;
	cached->index = index;
	cached->next = object->object.next_object_index;
	cached->child = object->object.first_child_object_index;
	cached->center = object->object.bounding_sphere_center;
	cached->radius = object->object.bounding_sphere_radius;
	cached->relative.x = batch.point->x - cached->center.x;
	cached->relative.y = batch.point->y - cached->center.y;
	cached->relative.z = batch.point->z - cached->center.z;
	cached->sphere_distance = cached->relative.x * cached->relative.x +
		cached->relative.y * cached->relative.y + cached->relative.z * cached->relative.z - cached->radius * cached->radius;
	cached->eligible = index != batch.ignore && !TEST_FLAG(object->object.flags, _object_invisible_bit) &&
		TEST_FLAG(batch.flags, object->object.type + _collision_test_objects_first_type_bit);
	/* Every ray endpoint stays inside the batch cube. Reject a disjoint
	   sphere once per batch, rather than rechecking it for every ray. Leave
	   borderline and invalid bounds to the original sphere arithmetic. */
	bound = batch.extent + cached->radius;
	margin = 0.0001f * (1 + fabsf(batch.point->x) + fabsf(batch.point->y) +
		fabsf(batch.point->z) + fabsf(bound));
	if (cached->radius >= 0 && batch.extent > 0 &&
		(fabsf(cached->center.x - batch.point->x) > bound + margin ||
		 fabsf(cached->center.y - batch.point->y) > bound + margin ||
		 fabsf(cached->center.z - batch.point->z) > bound + margin))
		cached->eligible = FALSE;
	return cached;
}

/* The same finite-segment predicate as fast_vector_intersects_sphere. Its
   origin/sphere terms are constant for the batch and its vector length is
   constant for the ray. Keep the original arithmetic and strict comparisons,
   including tangencies; only move invariant work out of the object loop. */
static int trace_sphere(struct trace_object const *object, real_vector3d const *vector)
{
	real b, disc, neg_a_minus_b;
	if (object->sphere_distance < 0.f) return TRUE;
	b = vector->i * object->relative.x + vector->j * object->relative.y + vector->k * object->relative.z;
	if (b >= 0.f) return FALSE;
	disc = b * b - batch.vector_squared * object->sphere_distance;
	if (disc <= 0.f) return FALSE;
	neg_a_minus_b = -batch.vector_squared - b;
	return neg_a_minus_b < 0.f || neg_a_minus_b * neg_a_minus_b < disc;
}

static void trace_objects(long index, real_vector3d const *vector, real *depth, int nesting)
{
	int siblings = 0;
	if (nesting >= TRACE_OBJECTS) { batch.failed = 1; return; }
	while (index != NONE && !batch.failed)
	{
		struct trace_object *object = trace_object(index);
		if (!object) return;
		if (++siblings > TRACE_OBJECTS) { batch.failed = 1; return; }
		if (object->eligible && trace_sphere(object, vector))
		{
			if (batch.defer_models)
				object->pending[(batch.ray - 1) / 32] |= 1U << ((batch.ray - 1) % 32);
			else
			{
				struct collision_model_test_vector_result result;
				if (!object->model_state)
					object->model_state = collision_model_instance_new(&object->model, index) ? 1 : -1;
				if (object->model_state > 0 && collision_model_test_vector(&object->model,
					FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit),
					batch.point, vector, &result) && *depth > result.bsp_result.t)
					*depth = result.bsp_result.t;
			}
			/* Children are tested only after the parent's eligibility and sphere,
			   exactly as in object_test_vector; siblings are always considered. */
			if (object->child != NONE) trace_objects(object->child, vector, depth, nesting+1);
		}
		index = object->next;
	}
}

static void trace_cluster(long cluster, real_vector3d const *vector, real *depth)
{
	int i;
	if (batch.clusters[cluster].visited == batch.ray) return;
	batch.clusters[cluster].visited = batch.ray;
	if (!batch.clusters[cluster].ready)
	{
		long reference, index;
		batch.clusters[cluster].first = batch.references;
		for (index = cluster_get_first_collideable_object(&reference, (short)cluster); index != NONE;
			index = cluster_get_next_collideable_object(&reference))
		{
			struct trace_object *object = trace_object(index);
			if (!object) return;
			if (!object->eligible && object->next == NONE) continue;
			if (batch.references == TRACE_REFERENCES) { batch.failed = 1; return; }
			batch.roots[batch.references++] = index;
		}
		batch.clusters[cluster].count = batch.references - batch.clusters[cluster].first;
		batch.clusters[cluster].ready = 1;
	}
	for (i = 0; i < batch.clusters[cluster].count && !batch.failed; i++)
	{
		long index = batch.roots[batch.clusters[cluster].first+i];
		unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
		if (batch.visited[slot] != batch.ray)
		{
			batch.visited[slot] = batch.ray;
			trace_objects(index, vector, depth, 0);
		}
	}
}

void plasma_collision_begin(unsigned long flags, real_point3d const *point, long ignore)
{
	unsigned long supported = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) |
		FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit);
	memset(&batch, 0, sizeof(batch));
	batch.active = flags == supported || flags == (supported | FLAG(_collision_test_objects_vehicles_bit));
	batch.flags = flags;
	batch.point = point;
	batch.ignore = ignore;
	if (batch.active)
	{
		batch.structure = global_structure_bsp_get();
		batch.bsp = global_collision_bsp_get();
	}
}

float plasma_collision_depth(unsigned long flags, real_point3d const *point, real_vector3d const *vector, long ignore)
{
	struct collision_result fallback;
	struct collision_bsp_test_vector_result result;
	real depth = 1.0f;
	int i, hit, use_start, octant;
	if (!batch.active || batch.failed || flags != batch.flags || point != batch.point || ignore != batch.ignore)
		goto stock;
	if (debug_collision_skip_vectors) return 2.0f;
	if (!batch.ray) batch.extent = MAX(fabsf(vector->i), MAX(fabsf(vector->j), fabsf(vector->k)));
	use_start = fabsf(vector->i) <= batch.extent && fabsf(vector->j) <= batch.extent &&
		fabsf(vector->k) <= batch.extent;
	/* Cached object rejection applies only inside the original cube. */
	if (!use_start) goto stock;
	octant = (vector->i >= 0) | ((vector->j >= 0) << 1) | ((vector->k >= 0) << 2);
	if (!batch.prefixes[octant].ready)
	{
		long target;
		short skipped;
		int work = COLLISION_VECTOR_CACHE_NODES;
		batch.prefixes[octant].ready = 1;
		trace_start(octant);
		if (batch.extent > 0 && batch.extent < 1000)
			trace_prepare(batch.prefixes[octant].node, batch.prefixes[octant].depth, octant, &work, &target, &skipped);
	}
	batch.ray++;
	batch.vector_squared = vector->i * vector->i + vector->j * vector->j + vector->k * vector->k;
	hit = collision_bsp_test_vector_from_node(FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit), batch.bsp,
		MAXIMUM_BREAKABLE_SURFACES_PER_MAP, breakable_surface_flags_get(), point, vector, REAL_MAX, &result,
		batch.prefixes[octant].node, batch.prefixes[octant].depth, &batch.cache[octant]);
	if (hit) depth = result.t;
	if (!debug_collision_skip_objects)
		for (i = 0; i < result.leaf_count && !batch.failed; i++)
		{
			long leaf = result.leaf_indices[i], cluster;
			if (leaf == NONE) continue;
			if ((leaf & LONG_MAX) >= batch.structure->leaves.count) goto stock;
			cluster = TAG_BLOCK_GET_ELEMENT(&batch.structure->leaves, leaf & LONG_MAX, struct structure_leaf)->cluster_index;
			if (cluster < 0 || cluster >= MAXIMUM_CLUSTERS_PER_STRUCTURE) continue;
			trace_cluster(cluster, vector, &depth);
		}
	if (batch.failed) goto stock;
	return hit || depth < 1.0f ? depth : 2.0f;
stock:
	if (batch.defer_models) batch.failed = 1;
	return collision_test_vector(flags, point, vector, ignore, &fallback) ? fallback.t : 2.0f;
}

void plasma_collision_depths(unsigned long flags, real_point3d const *point,
	real_vector3d const *vectors, int count, long ignore, float *depths)
{
	int ray, object_index, word;
	plasma_collision_begin(flags, point, ignore);
	batch.defer_models = count > 0 && count <= PLASMA_COLLISION_RAYS;
	if (batch.active && batch.defer_models)
	{
		/* Preserve cluster, sphere and parent/child gates for every ray. Only
		   defer the expensive model queries; a bitset deduplicates identical
		   queries reached through more than one object/cluster reference. */
		for (ray = 0; ray < count && !batch.failed; ray++)
			depths[ray] = plasma_collision_depth(flags, point, &vectors[ray], ignore);
		if (!batch.failed)
			for (object_index = 0; object_index < batch.count; object_index++)
			{
				struct trace_object *object = &batch.objects[object_index];
				for (word = 0; word < (count + 31) / 32; word++)
					if (object->pending[word]) break;
				if (word == (count + 31) / 32) continue;
				if (collision_model_instance_new(&object->model, object->index))
					plasma_collision_model_depths(&object->model, point, vectors, count, object->pending, depths);
			}
	}
	else batch.failed = 1;
	if (batch.failed)
		for (ray = 0; ray < count; ray++)
		{
			struct collision_result result;
			depths[ray] = collision_test_vector(flags, point, &vectors[ray], ignore, &result) ? result.t : 2.0f;
		}
	plasma_collision_end();
}

void plasma_collision_end(void)
{
	/* Model instances borrow tag and object matrices only during the batch. */
	memset(batch.objects, 0, (size_t)batch.count * sizeof(batch.objects[0]));
	batch.active = 0;
	batch.point = NULL;
	batch.structure = NULL;
	batch.bsp = NULL;
	memset(&batch.cache, 0, sizeof(batch.cache));
}
