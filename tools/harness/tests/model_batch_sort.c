#include "harness.h"

/* The queued solid parts' order (rasterizer_xbox_models.c, model_batch_sort):
each kind (shader, permutation, vertex buffer, triangle buffer) together, the
kinds in the order their first parts were queued, a kind's parts in the
order they were queued; every part once. */

struct shader;
struct triangle_buffer;
struct vertex_buffer;

#include "config.inc"

static struct model_batch_draw *model_batch_draws;
static long model_batch_draw_count;
static struct model_batch_kind *model_batch_kinds;
static long model_batch_kind_capacity;
static long model_batch_kind_stamp;
static long *model_batch_draw_kinds;
static long *model_batch_kind_starts;
static long *model_batch_order;

#include "under_test.inc"

#define DRAWS 4096

static unsigned long seed = 12345;

static unsigned long next(unsigned long range)
{
	seed = seed * 1103515245UL + 12345UL;
	return (seed >> 8) % range;
}

/* room for DRAWS parts, the kinds' table twice as large (as model_batch_draw_room) */
static void queue_new(long kind_capacity)
{
	model_batch_draws = calloc(DRAWS, sizeof(*model_batch_draws));
	model_batch_draw_kinds = calloc(DRAWS, sizeof(long));
	model_batch_kind_starts = calloc(DRAWS, sizeof(long));
	model_batch_order = calloc(DRAWS, sizeof(long));
	model_batch_kinds = calloc((size_t)kind_capacity, sizeof(*model_batch_kinds));
	model_batch_kind_capacity = kind_capacity;
	model_batch_draw_count = 0;
}

static void queue(long shader, short permutation, long vertices, long triangles)
{
	struct model_batch_draw *draw = &model_batch_draws[model_batch_draw_count++];

	memset(draw, 0, sizeof(*draw));
	draw->shader = (struct shader *)(0x01000000UL + 0x40UL * (unsigned long)shader);
	draw->shader_permutation_index = permutation;
	draw->vertex_buffer = (struct vertex_buffer const *)(0x02000000UL + 0x20UL * (unsigned long)vertices);
	draw->triangle_buffer = (struct triangle_buffer const *)(0x03000000UL + 0x20UL * (unsigned long)triangles);
	draw->model_index = model_batch_draw_count;
}

static boolean same_kind(long a, long b)
{
	struct model_batch_draw const *x = &model_batch_draws[a], *y = &model_batch_draws[b];

	return x->shader == y->shader && x->shader_permutation_index == y->shader_permutation_index &&
		x->vertex_buffer == y->vertex_buffer && x->triangle_buffer == y->triangle_buffer;
}

static void check_order(const char *what)
{
	long count = model_batch_draw_count, at, earlier, previous_first = -1;
	char *placed = calloc((size_t)count + 1, 1);

	model_batch_sort();
	for (at = 0; at < count; at++)
	{
		long draw = model_batch_order[at];

		CHECK(draw >= 0 && draw < count, "%s: place %ld holds %ld, not a queued part", what, at, draw);
		CHECK(!placed[draw], "%s: part %ld placed twice", what, draw);
		placed[draw] = TRUE;
		if (at > 0 && same_kind(model_batch_order[at - 1], draw))
		{
			CHECK(model_batch_order[at - 1] < draw, "%s: part %ld after part %ld of its kind, queued later", what,
				draw, model_batch_order[at - 1]);
			continue;
		}
		/* a kind's first place: its first part, its kind first queued after
		the kind before's */
		for (earlier = 0; earlier < draw; earlier++)
			CHECK(!same_kind(earlier, draw), "%s: part %ld starts a run of its kind, part %ld was queued before it",
				what, draw, earlier);
		CHECK(draw > previous_first, "%s: the kind first queued at %ld drawn before the one first queued at %ld",
			what, previous_first, draw);
		previous_first = draw;
	}
	free(placed);
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	CASE("kinds")
	{
		/* a crowd's parts: few shaders, buffers shared by many models; kinds
		differing in one of the four only. (The tables crowded, so that kinds
		alike but for one key probe into each other's slots: 180 kinds in
		256, 9 in 16.) */
		long index;

		queue_new(256);
		for (index = 0; index < DRAWS; index++)
			queue((long)next(6), (short)next(3), (long)next(5), (long)next(2));
		check_order("a crowd");
		queue_new(16);
		for (index = 0; index < 64; index++)
			queue(1, (short)(index & 1), 1 + (index >> 1 & 1), 1 + (index >> 2 & 1));
		queue(2, 0, 1, 1);
		check_order("one key differing");
	}
	CASE("full")
	{
		/* every part its own kind, the table half full (as large as it gets:
		twice the parts' room), slots taken by others' probing */
		long index;

		queue_new(2 * DRAWS);
		for (index = 0; index < DRAWS; index++)
			queue(index, (short)(index & 3), index * 7, index * 13);
		check_order("all different");
	}
	CASE("flushes")
	{
		/* flush after flush with the table not cleared: the last flush's
		kinds are not this one's */
		long flush, index;

		queue_new(2 * DRAWS);
		for (flush = 0; flush < 20; flush++)
		{
			model_batch_draw_count = 0;
			for (index = 0; index < 50 + (long)next(500); index++)
				queue((long)next(4), (short)next(2), (long)next(3), (long)next(3));
			check_order("flush after flush");
		}
	}
	return 0;
}
