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
	int eligible, model_state;
	struct collision_model_instance model;
};
static struct
{
	int active, failed, count, references;
	unsigned long flags, ray;
	long ignore;
	real_point3d const *point;
	struct structure_bsp const *structure;
	struct collision_bsp const *bsp;
	real extent;
	long start_node;
	short start_depth;
	struct collision_bsp_vector_cache cache;
	struct trace_object objects[TRACE_OBJECTS];
	unsigned short lookup[MAXIMUM_OBJECTS_PER_MAP];
	unsigned long visited[MAXIMUM_OBJECTS_PER_MAP];
	struct { int ready, first, count; unsigned long visited; } clusters[MAXIMUM_CLUSTERS_PER_STRUCTURE];
	long roots[TRACE_REFERENCES];
} batch;

/* All visibility rays fit in the first ray's axis-aligned cube. Walk its
common prefix once, retaining the stock traversal below that node. The
outward margin leaves borderline planes to the original per-ray arithmetic.
Unsupported trees and later rays outside the cube simply start at the root. */
static void trace_start(real_vector3d const *vector)
{
	long node_index = 0;
	short depth = 0;
	real extent = MAX(fabsf(vector->i), MAX(fabsf(vector->j), fabsf(vector->k)));
	batch.extent = extent;
	if (!(extent > 0 && extent < 1000)) return;
	while (!(node_index & LONG_MIN))
	{
		struct bsp3d_node const *node;
		real_plane3d const *plane;
		real distance, bound, margin;
		if (node_index >= batch.bsp->bsp3d.nodes.count ||
			depth >= MAXIMUM_BSP3D_TRAVERSAL_DEPTH || depth >= batch.bsp->bsp3d.nodes.count)
			return;
		node = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.nodes, node_index, struct bsp3d_node);
		if (node->plane_designator < 0 || node->plane_designator >= batch.bsp->bsp3d.planes.count)
			return;
		plane = TAG_BLOCK_GET_ELEMENT(&batch.bsp->bsp3d.planes, node->plane_designator, real_plane3d);
		distance = plane3d_distance_to_point(plane, batch.point);
		bound = extent * (fabsf(plane->n.i) + fabsf(plane->n.j) + fabsf(plane->n.k));
		margin = 0.0001f * (1 + bound + fabsf(plane->d) +
			fabsf(plane->n.i * batch.point->x) + fabsf(plane->n.j * batch.point->y) +
			fabsf(plane->n.k * batch.point->z));
		if (distance > bound + margin) node_index = node->children[1];
		else if (distance < -bound - margin) node_index = node->children[0];
		else break;
		depth++;
	}
	batch.start_node = node_index;
	batch.start_depth = depth;
}

static struct trace_object *trace_object(long index)
{
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	struct trace_object *cached;
	struct object_datum const *object;
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
	cached->eligible = index != batch.ignore && !TEST_FLAG(object->object.flags, _object_invisible_bit) &&
		TEST_FLAG(batch.flags, object->object.type + _collision_test_objects_first_type_bit);
	return cached;
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
		if (object->eligible && fast_vector_intersects_sphere(batch.point, vector, &object->center, object->radius))
		{
			struct collision_model_test_vector_result result;
			if (!object->model_state)
				object->model_state = collision_model_instance_new(&object->model, index) ? 1 : -1;
			if (object->model_state > 0 && collision_model_test_vector(&object->model,
				FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit),
				batch.point, vector, &result) && *depth > result.bsp_result.t)
				*depth = result.bsp_result.t;
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
	int i, hit, use_start;
	if (!batch.active || batch.failed || flags != batch.flags || point != batch.point || ignore != batch.ignore)
		goto stock;
	if (debug_collision_skip_vectors) return 2.0f;
	if (!batch.ray) trace_start(vector);
	use_start = fabsf(vector->i) <= batch.extent && fabsf(vector->j) <= batch.extent &&
		fabsf(vector->k) <= batch.extent;
	batch.ray++;
	hit = collision_bsp_test_vector_from_node(FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit), batch.bsp,
		MAXIMUM_BREAKABLE_SURFACES_PER_MAP, breakable_surface_flags_get(), point, vector, REAL_MAX, &result,
		use_start ? batch.start_node : 0, use_start ? batch.start_depth : 0, &batch.cache);
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
	return collision_test_vector(flags, point, vector, ignore, &fallback) ? fallback.t : 2.0f;
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
