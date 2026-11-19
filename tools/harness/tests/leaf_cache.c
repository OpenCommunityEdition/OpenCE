/*
LEAF_CACHE.C (test)

The real scenario_leaf_index_from_point_cached (scenario.c),
bsp3d_test_point_clearance (bsp3d.c) and collision_bsp_test_vector
(collision_bsp.c) over random bsps: a point's leaf is kept with its
clearance, the least distance from it to a plane it was sorted by, and a
point nearer than that to it is given that leaf without the bsp being
walked. Particles moving a little each frame are given exactly the leaf
bsp3d_test_point gives, half of them from what was kept; their steps, tested
as vectors, give exactly what the walk of the collision bsp gives, and those
given without the walk are ones the walk takes through a single leaf, with
no surface tested; the others' walks keep their starts' leaves and
clearances as bsp3d_test_point_clearance gives them. A new bsp keeps
nothing of the old one's.
test_leaf_cache.py takes the structures, constants and code from the
sources (config.inc, under_test.inc).
*/

#include <limits.h>
#include <math.h>
#include "harness.h"

struct tag_block { long count; void *address; void const *definition; };
typedef struct { real_vector3d n; real d; } real_plane3d;
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])
#define error(priority, ...) ((void)0)
#define csmemset memset

static real plane3d_distance_to_point(real_plane3d const *plane, real_point3d const *point)
{
	return point->x * plane->n.i + point->y * plane->n.j + point->z * plane->n.k - plane->d;
}

typedef unsigned short word;
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

#include "config.inc"

/* the collision bsp: its bsp3d and its leaves; no surfaces */
struct collision_bsp { struct bsp3d bsp3d; struct tag_block leaves; struct tag_block surfaces; };

/* the current bsp */
static struct bsp3d *current_bsp;
static struct bsp3d *global_bsp3d_get(void) { return current_bsp; }
static struct collision_bsp *global_collision_bsp;
static boolean warned_about_bsp3d_nodes;
static boolean warned_about_collision_bsp_nodes;

/* the collision tests' usage and times, not kept */
#define collision_log_usage(function) ((void)0)
#define collision_log_start_time(time) ((void)0)
#define collision_log_end_time(function, time) ((void)0)
static struct { struct { long long QuadPart; } vector; } collision_bsp_usage_times;

static boolean collision_bsp_valid_leaf(struct collision_bsp const *bsp, long leaf_index)
{
	return leaf_index >= 0 && leaf_index < bsp->leaves.count;
}

/* the surfaces of a leaf the vector leaves: the tests counted, nothing hit */
static long leaf_tests;
static long collision_leaf_test_vector(struct collision_bsp const *bsp, short breakable_surface_count,
	byte const *breakable_surface_flags, real_point3d const *point, real_vector3d const *vector, long leaf_index,
	long plane_index, real t, boolean test_surface)
{
	leaf_tests++;
	return NONE;
}

static boolean collision_bsp_test_vector_recursive(struct collision_bsp_test_vector_context *data, long node_index,
	real t0, real t1, short depth);

#include "under_test.inc"

/* a deterministic random number in [0,1) */
static unsigned long random_seed;
static real random_real(void)
{
	random_seed = random_seed * 1664525UL + 1013904223UL;
	return (real)((random_seed >> 8) & 0xFFFFFF) / (real)0x1000000;
}

static real random_range(real lower, real upper)
{
	return lower + (upper - lower) * random_real();
}

/* a bsp of random planes through the world, [-50,50] each way, BSP_DEPTH
nodes deep: leaves numbered from 1 (no point is in leaf 0), one in ten solid */
enum { BSP_DEPTH = 12, BSP_NODES = (1 << BSP_DEPTH) - 1 };

struct test_bsp
{
	struct collision_bsp bsp;
	struct bsp3d_node nodes[BSP_NODES];
	real_plane3d planes[BSP_NODES];
	struct collision_leaf leaves[BSP_NODES + 2];
};

static long next_leaf;

static long make_child(struct test_bsp *test, long index, short depth)
{
	if (depth == BSP_DEPTH)
	{
		if (random_real() < 0.1f)
			return NONE;
		return LONG_MIN | next_leaf++;
	}
	return index;
}

static void make_bsp(struct test_bsp *test, unsigned long seed)
{
	long index;

	random_seed = seed;
	next_leaf = 1;
	for (index = 0; index < BSP_NODES; index++)
	{
		real_plane3d *plane = &test->planes[index];
		real length;
		short depth = 1;
		long i;

		for (i = index + 1; i > 1; i >>= 1)
			depth++;
		do
		{
			plane->n.i = random_range(-1.0f, 1.0f);
			plane->n.j = random_range(-1.0f, 1.0f);
			plane->n.k = random_range(-1.0f, 1.0f);
			length = (real)sqrt(plane->n.i * plane->n.i + plane->n.j * plane->n.j + plane->n.k * plane->n.k);
		}
		while (length < 0.1f);
		plane->n.i /= length;
		plane->n.j /= length;
		plane->n.k /= length;
		plane->d = random_range(-40.0f, 40.0f);
		test->nodes[index].plane_designator = index;
		test->nodes[index].children[0] = make_child(test, 2 * index + 1, depth);
		test->nodes[index].children[1] = make_child(test, 2 * index + 2, depth);
	}
	/* one leaf in five with two-sided surfaces */
	for (index = 0; index < next_leaf; index++)
	{
		test->leaves[index].flags = random_real() < 0.2f ? FLAG(_collision_leaf_contains_two_sided_bit) : 0;
		test->leaves[index].bsp2d_reference_count = 0;
		test->leaves[index].first_bsp2d_reference_index = NONE;
	}
	test->bsp.bsp3d.nodes.count = BSP_NODES;
	test->bsp.bsp3d.nodes.address = test->nodes;
	test->bsp.bsp3d.planes.count = BSP_NODES;
	test->bsp.bsp3d.planes.address = test->planes;
	test->bsp.leaves.count = next_leaf;
	test->bsp.leaves.address = test->leaves;
}

static struct test_bsp first_bsp;
static struct test_bsp second_bsp;

/* particles moving a little each frame, now and then far: each leaf the one
the bsp gives; the share given from what was kept */
static real moving_particles(void)
{
	enum { PARTICLES = 200, FRAMES = 300 };
	static real_point3d positions[PARTICLES];
	static real_vector3d velocities[PARTICLES];
	long queries = 0;
	long walks = 0;
	short frame;
	short particle;

	make_bsp(&first_bsp, 1);
	current_bsp = &first_bsp.bsp.bsp3d;
	scenario_leaf_cache_clear();
	random_seed = 99;
	for (particle = 0; particle < PARTICLES; particle++)
	{
		positions[particle].x = random_range(-50.0f, 50.0f);
		positions[particle].y = random_range(-50.0f, 50.0f);
		positions[particle].z = random_range(-50.0f, 50.0f);
		velocities[particle].i = random_range(-0.05f, 0.05f);
		velocities[particle].j = random_range(-0.05f, 0.05f);
		velocities[particle].k = random_range(-0.05f, 0.05f);
	}
	for (frame = 0; frame < FRAMES; frame++)
	{
		for (particle = 0; particle < PARTICLES; particle++)
		{
			real_point3d *position = &positions[particle];
			long expected;
			long leaf;
			real clearance;

			if (random_real() < 0.01f)
			{
				position->x = random_range(-50.0f, 50.0f);
				position->y = random_range(-50.0f, 50.0f);
				position->z = random_range(-50.0f, 50.0f);
			}
			position->x += velocities[particle].i;
			position->y += velocities[particle].j;
			position->z += velocities[particle].k;

			expected = bsp3d_test_point(current_bsp, 0, position);
			{
				struct scenario_leaf_cache_entry before[SCENARIO_LEAF_CACHE_SIZE];

				memcpy(before, scenario_leaf_cache, sizeof(before));
				leaf = scenario_leaf_index_from_point_cached(position);
				walks += memcmp(before, scenario_leaf_cache, sizeof(before)) != 0;
			}
			queries++;
			CHECK(leaf == expected, "frame %d particle %d at (%f,%f,%f): leaf %ld, the bsp gives %ld",
				frame, particle, position->x, position->y, position->z, leaf, expected);
			CHECK(bsp3d_test_point_clearance(current_bsp, 0, position, &clearance) == expected,
				"the clearance walk gives leaf %ld", expected);
		}
	}

	return 1.0f - (real)walks / (real)queries;
}

/* particles' steps tested as vectors on the collision bsp, now and then as
long as a shot: what is given without the walk the same as the walk gives,
and only for a vector the walk takes through a single leaf, with no surface
tested; the share given without the walk */
static real moving_vectors(void)
{
	enum { PARTICLES = 200, FRAMES = 300 };
	static real_point3d positions[PARTICLES];
	static real_vector3d velocities[PARTICLES];
	unsigned long flags = FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit);
	long vectors = 0;
	long kept = 0;
	short frame;
	short particle;

	make_bsp(&first_bsp, 3);
	current_bsp = &first_bsp.bsp.bsp3d;
	scenario_leaf_cache_clear();
	random_seed = 5;
	for (particle = 0; particle < PARTICLES; particle++)
	{
		positions[particle].x = random_range(-50.0f, 50.0f);
		positions[particle].y = random_range(-50.0f, 50.0f);
		positions[particle].z = random_range(-50.0f, 50.0f);
		velocities[particle].i = random_range(-0.05f, 0.05f);
		velocities[particle].j = random_range(-0.05f, 0.05f);
		velocities[particle].k = random_range(-0.05f, 0.05f);
	}
	for (frame = 0; frame < FRAMES; frame++)
	{
		for (particle = 0; particle < PARTICLES; particle++)
		{
			static struct collision_bsp_test_vector_result walked;
			static struct collision_bsp_test_vector_result given;
			static struct collision_bsp_test_vector_result scratch;
			static struct scenario_leaf_cache_entry before[SCENARIO_LEAF_CACHE_SIZE];
			real_point3d *position = &positions[particle];
			real_vector3d vector = velocities[particle];
			real maximum_t = random_real() < 0.5f ? 1.0f : random_range(-0.2f, 1.5f);
			boolean without_walk;
			boolean walked_hit;
			boolean given_hit;
			long tests;
			long index;

			/* now and then a shot */
			if (random_real() < 0.05f)
			{
				vector.i *= 100.0f;
				vector.j *= 100.0f;
				vector.k *= 100.0f;
			}
			without_walk = collision_bsp_test_vector_short(&vector, PIN(maximum_t, 0.f, 1.f)) &&
				collision_bsp_test_vector_in_one_leaf(&first_bsp.bsp, position, &vector, PIN(maximum_t, 0.f, 1.f),
					&scratch);

			global_collision_bsp = NULL;
			leaf_tests = 0;
			walked_hit = collision_bsp_test_vector(flags, &first_bsp.bsp, 0, NULL, position, &vector, maximum_t, &walked);
			tests = leaf_tests;
			global_collision_bsp = &first_bsp.bsp;
			memcpy(before, scenario_leaf_cache, sizeof(before));
			given_hit = collision_bsp_test_vector(flags, &first_bsp.bsp, 0, NULL, position, &vector, maximum_t, &given);
			/* what the walk kept: a point's leaf and clearance as the clearance walk gives them */
			for (index = 0; index < SCENARIO_LEAF_CACHE_SIZE; index++)
			{
				struct scenario_leaf_cache_entry const *entry = &scenario_leaf_cache[index];
				real clearance;
				long leaf;

				if (!memcmp(&before[index], entry, sizeof(*entry)))
					continue;
				CHECK(!without_walk, "frame %d particle %d: given without the walk, and kept", frame, particle);
				leaf = bsp3d_test_point_clearance(current_bsp, 0, &entry->point, &clearance);
				CHECK(entry->leaf_index == leaf && entry->clearance == clearance,
					"frame %d particle %d: kept leaf %ld clearance %f, the clearance walk gives leaf %ld clearance %f",
					frame, particle, entry->leaf_index, entry->clearance, leaf, clearance);
			}

			vectors++;
			kept += without_walk;
			CHECK(given_hit == walked_hit && given.t == walked.t && given.leaf_count == walked.leaf_count,
				"frame %d particle %d: hit %d t %f %ld leaves, the walk gives hit %d t %f %ld leaves",
				frame, particle, given_hit, given.t, given.leaf_count, walked_hit, walked.t, walked.leaf_count);
			for (index = 0; index < given.leaf_count && index < 256; index++)
				CHECK(given.leaf_indices[index] == walked.leaf_indices[index],
					"frame %d particle %d: leaf #%ld %ld, the walk gives %ld",
					frame, particle, index, given.leaf_indices[index], walked.leaf_indices[index]);
			if (without_walk)
				CHECK(walked.leaf_count <= 1 && tests == 0,
					"frame %d particle %d: given without the walk, which goes through %ld leaves, testing %ld",
					frame, particle, walked.leaf_count, tests);

			position->x += velocities[particle].i;
			position->y += velocities[particle].j;
			position->z += velocities[particle].k;
			if (random_real() < 0.01f)
			{
				position->x = random_range(-50.0f, 50.0f);
				position->y = random_range(-50.0f, 50.0f);
				position->z = random_range(-50.0f, 50.0f);
			}
		}
	}
	global_collision_bsp = NULL;

	return (real)kept / (real)vectors;
}

int main(int argc, char **argv)
{
	char const *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "same-leaf"))
	{
		moving_particles();
	}
	else if (!strcmp(name, "reuses"))
	{
		real kept = moving_particles();

		CHECK(kept > 0.65f, "%.0f%% of the leaves were given from what was kept", kept * 100.0f);
		printf("%.0f%% kept\n", kept * 100.0f);
	}
	else if (!strcmp(name, "vectors"))
	{
		moving_vectors();
	}
	else if (!strcmp(name, "vectors-without-walk"))
	{
		real kept = moving_vectors();

		CHECK(kept > 0.65f, "%.0f%% of the vectors were given without the walk", kept * 100.0f);
		printf("%.0f%% without the walk\n", kept * 100.0f);
	}
	else if (!strcmp(name, "new-bsp"))
	{
		real_point3d points[500];
		short index;

		make_bsp(&first_bsp, 1);
		make_bsp(&second_bsp, 2);
		current_bsp = &first_bsp.bsp.bsp3d;
		scenario_leaf_cache_clear();
		random_seed = 7;
		for (index = 0; index < 500; index++)
		{
			points[index].x = random_range(-50.0f, 50.0f);
			points[index].y = random_range(-50.0f, 50.0f);
			points[index].z = random_range(-50.0f, 50.0f);
			scenario_leaf_index_from_point_cached(&points[index]);
		}
		current_bsp = &second_bsp.bsp.bsp3d;
		scenario_leaf_cache_clear();
		for (index = 0; index < 500; index++)
		{
			long expected = bsp3d_test_point(current_bsp, 0, &points[index]);
			long leaf = scenario_leaf_index_from_point_cached(&points[index]);

			CHECK(leaf == expected, "point %d in the new bsp: leaf %ld, the bsp gives %ld", index, leaf, expected);
		}
	}
	else if (!strcmp(name, "malformed-bsp"))
	{
		/* a node with two parents: the walk counts each node it enters
		against the bsp's three, and the second entry of node 2 ends it */
		static struct bsp3d_node nodes[3];
		static real_plane3d planes[3];
		static struct collision_leaf leaves[3];
		static struct collision_bsp_test_vector_result walked;
		static struct collision_bsp_test_vector_result given;
		struct collision_bsp bsp;
		real_point3d start = { -0.1f, 0.1f, 0.0f };
		real_vector3d vector = { 0.2f, 0.0f, 0.0f };
		unsigned long flags = FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit);
		long index;

		/* node 0 (x = 100) sends all to node 1 (x = 0), which sends both sides
		to node 2 (y = 0): leaves 1 and 2 */
		planes[0].n.i = 1.0f; planes[0].d = 100.0f;
		planes[1].n.i = 1.0f;
		planes[2].n.j = 1.0f;
		for (index = 0; index < 3; index++)
			nodes[index].plane_designator = index;
		nodes[0].children[0] = 1; nodes[0].children[1] = 1;
		nodes[1].children[0] = 2; nodes[1].children[1] = 2;
		nodes[2].children[0] = LONG_MIN | 1; nodes[2].children[1] = LONG_MIN | 2;
		bsp.bsp3d.nodes.count = 3;
		bsp.bsp3d.nodes.address = nodes;
		bsp.bsp3d.planes.count = 3;
		bsp.bsp3d.planes.address = planes;
		bsp.leaves.count = 3;
		bsp.leaves.address = leaves;
		current_bsp = &bsp.bsp3d;
		scenario_leaf_cache_clear();
		global_collision_bsp = NULL;
		collision_bsp_test_vector(flags, &bsp, 0, NULL, &start, &vector, 1.0f, &walked);
		global_collision_bsp = &bsp;
		collision_bsp_test_vector(flags, &bsp, 0, NULL, &start, &vector, 1.0f, &given);
		global_collision_bsp = NULL;
		CHECK(walked.leaf_count == 1, "the walk gives %ld leaves", walked.leaf_count);
		CHECK(given.leaf_count == walked.leaf_count && given.t == walked.t,
			"%ld leaves t %f, the walk gives %ld t %f", given.leaf_count, given.t, walked.leaf_count, walked.t);
	}
	else if (!strcmp(name, "empty-entry"))
	{
		/* nothing kept: the world's origin is in no leaf 0 */
		real_point3d origin = { 0.0f, 0.0f, 0.0f };

		make_bsp(&first_bsp, 1);
		current_bsp = &first_bsp.bsp.bsp3d;
		scenario_leaf_cache_clear();
		CHECK(scenario_leaf_index_from_point_cached(&origin) == bsp3d_test_point(current_bsp, 0, &origin),
			"the origin: leaf %ld, the bsp gives %ld",
			scenario_leaf_index_from_point_cached(&origin), bsp3d_test_point(current_bsp, 0, &origin));
	}
	else
	{
		fprintf(stderr, "unknown case '%s'\n", name);
		return 2;
	}

	return 0;
}
