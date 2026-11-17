/*
COLLISION_MODEL_SPHERES.C

collision_model_test_vector (collision_models.c), which passes over a node
whose bsp's bounding sphere (object_bounds_cache.c) the vector does not come
near, against itself without that (each node's bsp tested): the same hit, t,
node, region, bsp, plane and surface, for rotated and scaled nodes, rays that
graze the bsps' vertices, zero-length rays, and bsps whose vertices change
after the cache is invalidated (a new map's tags where the last map's were).
A bsp's test here is each of its triangles, tested whole.
test_collision_model_spheres.py takes the code from the sources.
*/

#include "harness.h"
#include <float.h>
#include <math.h>

typedef struct { long long QuadPart; } LARGE_INTEGER;
typedef struct { real_vector3d n; real d; } real_plane3d;
#include "structures.inc"

struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (((type *)(block)->address) + (index))
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))
#define collision_log_usage(function) ((void)0)
#define collision_log_start_time(time) ((void)0)
#define collision_log_end_time(function, time) ((void)0)

enum { NODES = 8, PERMUTATIONS = 2, TRIANGLES = 6, VERTICES = TRIANGLES * 3 };

struct collision_vertex { real_point3d point; };
struct collision_bsp
{
	struct { struct tag_block nodes; } bsp3d;
	struct tag_block vertices;
	struct collision_vertex vertex_storage[VERTICES];
};
struct collision_node { short region_index; struct tag_block bsps; };
struct collision_model { struct tag_block nodes; };
struct collision_model_instance
{
	long object_index;
	struct collision_model const *model;
	byte const *region_permutation_indices;
	real_matrix4x3 const *matrices;
	short node_count;
};

static struct collision_bsp bsps[NODES][PERMUTATIONS];
static struct collision_node nodes[NODES];
static struct collision_model model;
static real_matrix4x3 matrices[NODES];
static byte permutations[NODES];
static long bsp_tests;

#define collision_node_region_valid(node) TRUE

/* a bsp's test: its triangles, both faces, nearest first, up to maximum_t
(at most 1), and the result's t the maximum when nothing is hit, as
collision_bsp_test_vector's are */
static boolean collision_bsp_test_vector(unsigned long flags, struct collision_bsp const *bsp, short breakable_count,
	byte const *breakable_flags, real_point3d const *point, real_vector3d const *vector, real maximum_t,
	struct collision_bsp_test_vector_result *result)
{
	static real_plane3d planes[TRIANGLES];
	real best = PIN(maximum_t, 0.0f, 1.0f);
	boolean hit = FALSE;
	long triangle;

	(void)flags;
	(void)breakable_count;
	(void)breakable_flags;
	bsp_tests++;
	result->t = maximum_t < 0.0f ? 0.0f : maximum_t;
	result->leaf_count = 0;
	for (triangle = 0; triangle < bsp->vertices.count / 3; triangle++)
	{
		real_point3d const *a = &bsp->vertex_storage[triangle * 3].point;
		real_point3d const *b = &bsp->vertex_storage[triangle * 3 + 1].point;
		real_point3d const *c = &bsp->vertex_storage[triangle * 3 + 2].point;
		real_vector3d e1 = { b->x - a->x, b->y - a->y, b->z - a->z };
		real_vector3d e2 = { c->x - a->x, c->y - a->y, c->z - a->z };
		real_vector3d p = { vector->j * e2.k - vector->k * e2.j, vector->k * e2.i - vector->i * e2.k,
			vector->i * e2.j - vector->j * e2.i };
		real determinant = e1.i * p.i + e1.j * p.j + e1.k * p.k;
		real_vector3d s, q;
		real u, v, t;

		if (fabsf(determinant) < 1e-9f)
			continue;
		s.i = point->x - a->x;
		s.j = point->y - a->y;
		s.k = point->z - a->z;
		u = (s.i * p.i + s.j * p.j + s.k * p.k) / determinant;
		if (u < 0.0f || u > 1.0f)
			continue;
		q.i = s.j * e1.k - s.k * e1.j;
		q.j = s.k * e1.i - s.i * e1.k;
		q.k = s.i * e1.j - s.j * e1.i;
		v = (vector->i * q.i + vector->j * q.j + vector->k * q.k) / determinant;
		if (v < 0.0f || u + v > 1.0f)
			continue;
		t = (e2.i * q.i + e2.j * q.j + e2.k * q.k) / determinant;
		if (t < 0.0f || t >= best)
			continue;
		best = t;
		hit = TRUE;
		result->t = t;
		planes[triangle].n.i = e1.j * e2.k - e1.k * e2.j;
		planes[triangle].n.j = e1.k * e2.i - e1.i * e2.k;
		planes[triangle].n.k = e1.i * e2.j - e1.j * e2.i;
		planes[triangle].d = planes[triangle].n.i * a->x + planes[triangle].n.j * a->y + planes[triangle].n.k * a->z;
		result->plane = &planes[triangle];
		result->surface_index = triangle;
	}
	return hit;
}

#include "under_test.inc"

static unsigned long random_state = 4242;

static real random_real(real low, real high)
{
	random_state = random_state * 1103515245 + 12345;
	return low + (high - low) * (real)((random_state >> 8) & 0xFFFF) / 65535.0f;
}

static void normalize(real_vector3d *v)
{
	real length = sqrtf(v->i * v->i + v->j * v->j + v->k * v->k);

	v->i /= length;
	v->j /= length;
	v->k /= length;
}

static void make_bsp(struct collision_bsp *bsp, real_point3d const *offset)
{
	long vertex;
	real size = random_real(0.2f, 1.0f);
	long count = random_real(0.0f, 1.0f) < 0.1f ? 0 : VERTICES;

	bsp->bsp3d.nodes.count = random_real(0.0f, 1.0f) < 0.1f ? 0 : 1;
	bsp->vertices.count = count;
	bsp->vertices.address = bsp->vertex_storage;
	for (vertex = 0; vertex < VERTICES; vertex++)
	{
		bsp->vertex_storage[vertex].point.x = offset->x + random_real(-size, size);
		bsp->vertex_storage[vertex].point.y = offset->y + random_real(-size, size);
		bsp->vertex_storage[vertex].point.z = offset->z + random_real(-size, size);
	}
}

/* nodes rotated anywhere about the model, some scaled (one by 0) */
static void make_model(void)
{
	long node, permutation;
	real_point3d origin = { 0.0f, 0.0f, 0.0f };

	model.nodes.count = NODES;
	model.nodes.address = nodes;
	for (node = 0; node < NODES; node++)
	{
		real_matrix4x3 *matrix = &matrices[node];
		real_vector3d forward = { random_real(-1, 1), random_real(-1, 1), random_real(-1, 1) };
		real_vector3d up = { random_real(-1, 1), random_real(-1, 1), random_real(-1, 1) };
		real along;

		nodes[node].region_index = (short)node;
		nodes[node].bsps.count = PERMUTATIONS;
		nodes[node].bsps.address = bsps[node];
		for (permutation = 0; permutation < PERMUTATIONS; permutation++)
			make_bsp(&bsps[node][permutation], &origin);
		permutations[node] = (byte)(node % 3 == 2 ? NONE : node % PERMUTATIONS);
		normalize(&forward);
		along = up.i * forward.i + up.j * forward.j + up.k * forward.k;
		up.i -= along * forward.i;
		up.j -= along * forward.j;
		up.k -= along * forward.k;
		normalize(&up);
		matrix->forward = forward;
		matrix->up = up;
		matrix->left.i = up.j * forward.k - up.k * forward.j;
		matrix->left.j = up.k * forward.i - up.i * forward.k;
		matrix->left.k = up.i * forward.j - up.j * forward.i;
		matrix->scale = node == 0 ? 0.0f : node % 4 == 1 ? 1.0f : random_real(0.4f, 3.0f);
		matrix->position.x = random_real(-3.0f, 3.0f);
		matrix->position.y = random_real(-3.0f, 3.0f);
		matrix->position.z = random_real(-3.0f, 3.0f);
	}
}

static struct collision_model_instance instance(void)
{
	struct collision_model_instance made;

	made.object_index = 1;
	made.model = &model;
	made.region_permutation_indices = permutations;
	made.matrices = matrices;
	made.node_count = NODES;
	return made;
}

static long compared, skipped_tests, hits;

/* one vector through both: the same answer, field by field (all but the
leaves, which a model's result does not give its callers) */
static void compare(real_point3d const *point, real_vector3d const *vector)
{
	struct collision_model_instance made = instance();
	struct collision_model_test_vector_result fast, full;
	long tests_before = bsp_tests;
	boolean fast_hit, full_hit;

	memset(&fast, 0x5A, sizeof(fast));
	memset(&full, 0x5A, sizeof(full));
	fast_hit = collision_model_test_vector(&made, 0, point, vector, &fast);
	skipped_tests -= bsp_tests - tests_before;
	tests_before = bsp_tests;
	full_hit = collision_model_test_vector_without_spheres(&made, 0, point, vector, &full);
	skipped_tests += bsp_tests - tests_before;
	compared++;
	CHECK(fast_hit == full_hit, "vector %ld: hit %d, without the spheres %d", compared, fast_hit, full_hit);
	CHECK(fast.bsp_result.t == full.bsp_result.t, "vector %ld: t %g, without the spheres %g", compared,
		fast.bsp_result.t, full.bsp_result.t);
	if (full_hit)
	{
		hits++;
		CHECK(fast.node_index == full.node_index && fast.region_index == full.region_index &&
			fast.bsp_index == full.bsp_index, "vector %ld: node %d, without the spheres %d", compared,
			fast.node_index, full.node_index);
		CHECK(fast.bsp_result.plane == full.bsp_result.plane && fast.bsp_result.surface_index == full.bsp_result.surface_index,
			"vector %ld: another surface", compared);
	}
}

/* a vector from near the model to near it, across it */
static void random_vectors(long count)
{
	long index;

	for (index = 0; index < count; index++)
	{
		real_point3d point = { random_real(-6, 6), random_real(-6, 6), random_real(-6, 6) };
		real_vector3d vector = { random_real(-12, 12), random_real(-12, 12), random_real(-12, 12) };

		compare(&point, &vector);
	}
}

/* vectors that pass just inside a triangle's corner: through the vertices
farthest from the bsp's middle, where its sphere is tightest */
static void grazing_vectors(void)
{
	long node, permutation, triangle, corner;

	for (node = 0; node < NODES; node++)
	{
		for (permutation = 0; permutation < PERMUTATIONS; permutation++)
		{
			struct collision_bsp const *bsp = &bsps[node][permutation];

			for (triangle = 0; triangle < bsp->vertices.count / 3; triangle++)
			{
				for (corner = 0; corner < 3; corner++)
				{
					real_point3d local, through, from;
					real_vector3d vector, side = { random_real(-1, 1), random_real(-1, 1), random_real(-1, 1) };
					long other;

					local.x = local.y = local.z = 0.0f;
					for (other = 0; other < 3; other++)
					{
						real weight = other == corner ? 0.98f : 0.01f;

						local.x += weight * bsp->vertex_storage[triangle * 3 + other].point.x;
						local.y += weight * bsp->vertex_storage[triangle * 3 + other].point.y;
						local.z += weight * bsp->vertex_storage[triangle * 3 + other].point.z;
					}
					matrix4x3_transform_point(&matrices[node], &local, &through);
					normalize(&side);
					from.x = through.x - 4.0f * side.i;
					from.y = through.y - 4.0f * side.j;
					from.z = through.z - 4.0f * side.k;
					vector.i = 8.0f * side.i;
					vector.j = 8.0f * side.j;
					vector.k = 8.0f * side.k;
					compare(&from, &vector);
				}
			}
		}
	}
}

static void case_random(void)
{
	long model_index;

	for (model_index = 0; model_index < 200; model_index++)
	{
		make_model();
		object_bounds_cache_invalidate();
		random_vectors(200);
	}
	CHECK(hits > 1000 && skipped_tests > 50000, "%ld hits, %ld bsp tests passed over: too few to judge", hits,
		skipped_tests);
}

static void case_grazing(void)
{
	long model_index;

	for (model_index = 0; model_index < 200; model_index++)
	{
		make_model();
		object_bounds_cache_invalidate();
		grazing_vectors();
	}
	CHECK(hits > 10000, "only %ld hits", hits);
}

/* zero-length vectors, and vectors from inside a node's sphere */
static void case_short(void)
{
	long model_index, index;

	for (model_index = 0; model_index < 100; model_index++)
	{
		make_model();
		object_bounds_cache_invalidate();
		for (index = 0; index < 100; index++)
		{
			real_point3d point = { random_real(-4, 4), random_real(-4, 4), random_real(-4, 4) };
			real_vector3d zero = { 0.0f, 0.0f, 0.0f };
			real_vector3d tiny = { random_real(-0.01f, 0.01f), random_real(-0.01f, 0.01f), random_real(-0.01f, 0.01f) };

			compare(&point, &zero);
			compare(&point, &tiny);
		}
	}
}

/* the bsps' vertices change where they are (a new map's tags), the cache
invalidated as a new map does */
static void case_invalidated(void)
{
	long round, node, permutation;

	make_model();
	object_bounds_cache_invalidate();
	grazing_vectors();
	for (round = 0; round < 20; round++)
	{
		for (node = 0; node < NODES; node++)
		{
			for (permutation = 0; permutation < PERMUTATIONS; permutation++)
			{
				real_point3d offset = { random_real(-2, 2), random_real(-2, 2), random_real(-2, 2) };
				long count = bsps[node][permutation].vertices.count;

				make_bsp(&bsps[node][permutation], &offset);
				bsps[node][permutation].vertices.count = count;
				bsps[node][permutation].bsp3d.nodes.count = 1;
			}
		}
		object_bounds_cache_invalidate();
		grazing_vectors();
		random_vectors(100);
	}
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("random") case_random();
	else CASE("grazing") case_grazing();
	else CASE("short") case_short();
	else CASE("invalidated") case_invalidated();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
