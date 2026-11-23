#include "harness.h"
#include <math.h>

/* One object of two nodes drawn between ticks (render_interpolation.c):
whether its two snapshots are one pose moving (blended) or different poses
(snapped to the latest), found once a tick, agrees with the snapshots as
they are in every frame: after a tick, and after a correction moves them. */
#define OBJECT 0x12340003L
#define NODES 2
#define MAXIMUM_OBJECTS_PER_MAP 8
#define MAXIMUM_LOCAL_PLAYERS 4

struct observer_result { real_point3d position; };

#include "config.inc"

static struct interpolated_object *interpolated_objects;
static struct interpolated_camera interpolated_cameras[MAXIMUM_LOCAL_PLAYERS];
static long interpolation_tick;
static long interpolation_frame;
static boolean interpolation_rendering;
static real interpolation_fraction = 1.0f;

/* the world: the one object, its nodes as the tick left them */
#define _object_mask_all 0xFFFF
struct object_iterator { long index; int returned; };
struct object_block_reference { short size; short offset; };
struct object_datum
{
	struct
	{
		struct object_block_reference node_matrices;
		long first_child_object_index;
		long next_object_index;
	} object;
};
static struct object_datum test_object = { { { NODES * sizeof(real_matrix4x3), 0 }, NONE, NONE } };
static real_matrix4x3 nodes[NODES];
static real_vector3d const zero_vector = { 0.0f, 0.0f, 0.0f };
static real_vector3d const *global_zero_vector3d = &zero_vector;

static void object_iterator_new(struct object_iterator *iterator, long mask, long flags)
{
	iterator->index = NONE;
	iterator->returned = FALSE;
}
static void *object_iterator_next(struct object_iterator *iterator)
{
	if (iterator->returned)
		return NULL;
	iterator->returned = TRUE;
	iterator->index = OBJECT;
	return &test_object;
}
#define object_get_node_matrices(index) nodes
#define object_get(index) (&test_object)
#define halo_interpolation_enabled() TRUE
#define player_control_get_unit_index(local_player_index) NONE

#include "under_test.inc"

/* the root so far along x, the second node so far from it along y */
static void pose(real x, real y)
{
	short node;

	memset(nodes, 0, sizeof(nodes));
	for (node = 0; node < NODES; node++)
	{
		nodes[node].scale = 1.0f;
		nodes[node].forward.i = 1.0f;
		nodes[node].left.j = 1.0f;
		nodes[node].up.k = 1.0f;
		nodes[node].position.x = x;
	}
	nodes[1].position.y = y;
}

static struct interpolated_object *record(void)
{
	return &interpolated_objects[DATUM_INDEX_TO_ABSOLUTE_INDEX(OBJECT)];
}

/* the snap as every frame found it before: the snapshots' as they are now */
static boolean snapshots_snap(void)
{
	struct interpolated_object *r = record();
	real_matrix4x3 const *previous = r->nodes + (r->latest ^ 1) * r->node_capacity;
	real_matrix4x3 const *latest = r->nodes + r->latest * r->node_capacity;
	boolean snap = !(distance_squared(&previous[0].position, &latest[0].position) <=
		OBJECT_SNAP_DISTANCE * OBJECT_SNAP_DISTANCE);
	short node_index;

	for (node_index = 1; !snap && node_index < r->node_count; node_index++)
	{
		real_point3d previous_local, latest_local;

		matrix4x3_inverse_transform_point(&previous[0], &previous[node_index].position, &previous_local);
		matrix4x3_inverse_transform_point(&latest[0], &latest[node_index].position, &latest_local);
		snap = !(distance_squared(&previous_local, &latest_local) <= NODE_SNAP_DISTANCE * NODE_SNAP_DISTANCE);
	}
	return snap;
}

/* a frame halfway into the tick: drawn snapped or not, as expected and as
the snapshots are */
static void frame(boolean expected, const char *what)
{
	real_matrix4x3 const *drawn;
	real_matrix4x3 const *latest;

	interpolation_rendering = TRUE;
	interpolation_frame++;
	interpolation_fraction = 0.5f;
	drawn = render_interpolation_object_node_matrices(OBJECT);
	interpolation_rendering = FALSE;
	CHECK(drawn != NULL, "%s: not blended at all", what);
	CHECK(snapshots_snap() == expected, "%s: the test's snapshots %s", what, expected ? "do not snap" : "snap");
	CHECK(record()->snap == expected, "%s: drawn %s", what, expected ? "blended, not snapped" : "snapped, not blended");
	latest = record()->nodes + record()->latest * record()->node_capacity;
	if (expected)
		CHECK(drawn[0].position.x == latest[0].position.x, "%s: root at %.9g, not the latest's %.9g", what,
			drawn[0].position.x, latest[0].position.x);
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	CASE("tick")
	{
		pose(0.0f, 0.1f);
		render_interpolation_tick();
		pose(1.0f, 0.1f);
		render_interpolation_tick();
		frame(FALSE, "a step");
		frame(FALSE, "the step's next frame");
		pose(30.0f, 0.1f);
		render_interpolation_tick();
		frame(TRUE, "a teleport");
		pose(31.0f, 0.1f);
		render_interpolation_tick();
		frame(FALSE, "a step after it");
		pose(32.0f, 1.0f);
		render_interpolation_tick();
		frame(TRUE, "a node jumping");
	}
	CASE("correction")
	{
		/* roots exactly the snap distance apart, below and above 2^20 (a
		float's steps there 1/16 and 1/8): moved by 1/16, the latest
		rounds a step further, and the snapshots snap */
		real_vector3d offset = { -0.0625f, 0.0f, 0.0f };

		pose(1048575.875f, 0.1f);
		render_interpolation_tick();
		pose(1048585.875f, 0.1f);
		render_interpolation_tick();
		frame(FALSE, "the snap distance");
		render_interpolation_correct_object(OBJECT, &offset);
		frame(TRUE, "corrected past it");
	}
	return 0;
}
