#include "harness.h"
#include <math.h>

#include "config.inc"

/* One local player's first-person weapon of two nodes, drawn from a camera at
the origin looking along x (render_interpolation.c). Each tick the game poses
the nodes in the world from the camera; each frame between ticks the pose is
blended from the last tick's. */
#define WEAPON 0x10002L
#define OTHER_WEAPON 0x10003L
#define GRAPH 0x20004L
#define OTHER_GRAPH 0x20005L
#define NODES 2
#define PI 3.14159265f

struct render_camera { real_point3d position; real_vector3d forward, up; };
static void cross_product3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	result->i = a->j * b->k - a->k * b->j;
	result->j = a->k * b->i - a->i * b->k;
	result->k = a->i * b->j - a->j * b->i;
}
static void set_real_point3d(real_point3d *point, real x, real y, real z) { point->x = x; point->y = y; point->z = z; }
static long interpolation_tick;
static boolean interpolation_rendering = TRUE;
static real interpolation_fraction;

#include "under_test.inc"

static struct render_camera camera = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
static real_matrix4x3 nodes[NODES];

/* a node so far along x, turned so far about z */
static void pose(short node, real x, real angle)
{
	real_matrix4x3 *matrix = &nodes[node];

	matrix->scale = 1.0f;
	matrix->forward.i = (real)cos(angle);
	matrix->forward.j = (real)sin(angle);
	matrix->forward.k = 0.0f;
	matrix->left.i = -matrix->forward.j;
	matrix->left.j = matrix->forward.i;
	matrix->left.k = 0.0f;
	matrix->up.i = 0.0f;
	matrix->up.j = 0.0f;
	matrix->up.k = 1.0f;
	set_real_point3d(&matrix->position, x, 0.0f, 0.0f);
}

/* a frame drawn a fraction into a tick, both nodes so far along x */
static void frame(long tick, real fraction, long weapon, long graph, real x)
{
	interpolation_tick = tick;
	interpolation_fraction = fraction;
	pose(0, x, 0.0f);
	pose(1, x, 0.0f);
	render_interpolation_first_person(0, weapon, graph, nodes, NODES, &camera, NONE, NULL, NULL);
}

/* a frame drawn with the ammunition's overlay showing a count, by its frame:
node 1 is the one it moves (a shell), held by node 0, one it does not (the
hand), so far along x; the shell so much further along x than the hand */
static short const held_shell[NODES] = { NONE, 0 };
static short const loose_shell[NODES] = { NONE, NONE };
static void counted(long tick, real fraction, short count, real hand, real shell_on_hand, short const *parents)
{
	static unsigned long const count_nodes[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_INTERPOLATED_NODES)] = { 1UL << 1 };

	interpolation_tick = tick;
	interpolation_fraction = fraction;
	pose(0, hand, 0.0f);
	pose(1, hand + shell_on_hand, 0.0f);
	render_interpolation_first_person(0, WEAPON, GRAPH, nodes, NODES, &camera, count, count_nodes, parents);
}

static void expect_each(real hand, real shell)
{
	CHECK(fabs(nodes[0].position.x - hand) < 1e-4f && fabs(nodes[1].position.x - shell) < 1e-4f,
		"drawn at %f and %f, not %f and %f", nodes[0].position.x, nodes[1].position.x, hand, shell);
}

static void expect(real x)
{
	CHECK(fabs(nodes[0].position.x - x) < 1e-4f && fabs(nodes[1].position.x - x) < 1e-4f,
		"drawn at %f and %f, not %f", nodes[0].position.x, nodes[1].position.x, x);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	frame(10, 1.0f, WEAPON, GRAPH, 0.0f);
	/* a melee swings the weapon further in a tick than any limit that would
	tell it from another weapon: it is blended all the same */
	CASE("melee")
	{
		frame(11, 0.25f, WEAPON, GRAPH, 0.8f);
		expect(0.2f);
		frame(11, 0.75f, WEAPON, GRAPH, 0.8f);
		expect(0.6f);
		/* (from the tick's pose, not the last frame drawn) */
		frame(12, 0.5f, WEAPON, GRAPH, 1.6f);
		expect(1.2f);
	}
	/* another weapon of as many nodes: drawn as it is, then blended */
	CASE("weapon")
	{
		frame(11, 0.5f, OTHER_WEAPON, GRAPH, 0.1f);
		expect(0.1f);
		frame(12, 0.5f, OTHER_WEAPON, GRAPH, 0.2f);
		expect(0.15f);
	}
	CASE("graph")
	{
		frame(11, 0.5f, WEAPON, OTHER_GRAPH, 0.1f);
		expect(0.1f);
	}
	/* a weapon changed between two frames of a tick */
	CASE("within a tick")
	{
		frame(11, 0.25f, WEAPON, GRAPH, 0.4f);
		expect(0.1f);
		frame(11, 0.75f, OTHER_WEAPON, GRAPH, 0.2f);
		expect(0.2f);
		frame(12, 0.5f, OTHER_WEAPON, GRAPH, 0.4f);
		expect(0.3f);
	}
	/* the view away from first person for a while */
	CASE("gap")
	{
		frame(14, 0.5f, WEAPON, GRAPH, 0.4f);
		expect(0.4f);
	}
	CASE("not a number")
	{
		interpolation_tick = 11;
		interpolation_fraction = 0.5f;
		pose(0, 0.4f, 0.0f);
		pose(1, 0.4f, 0.0f);
		nodes[1].position.y = NAN;
		render_interpolation_first_person(0, WEAPON, GRAPH, nodes, NODES, &camera, NONE, NULL, NULL);
		CHECK(nodes[0].position.x == 0.4f, "a pose with a node not a number blended");
	}
	/* a node set back by half a turn snaps; the other still blends */
	CASE("half turn")
	{
		interpolation_tick = 11;
		interpolation_fraction = 0.5f;
		pose(0, 0.4f, 0.0f);
		pose(1, 0.0f, PI);
		render_interpolation_first_person(0, WEAPON, GRAPH, nodes, NODES, &camera, NONE, NULL, NULL);
		CHECK(fabs(nodes[0].position.x - 0.2f) < 1e-4f, "the other node not blended");
		CHECK(nodes[1].forward.i < -0.999f, "half a turn blended: forward (%f, %f)",
			nodes[1].forward.i, nodes[1].forward.j);
	}
	/* a large turn short of half a turn is blended */
	CASE("large turn")
	{
		interpolation_tick = 11;
		interpolation_fraction = 0.5f;
		pose(0, 0.0f, 0.0f);
		pose(1, 0.0f, 170.0f * PI / 180.0f);
		render_interpolation_first_person(0, WEAPON, GRAPH, nodes, NODES, &camera, NONE, NULL, NULL);
		CHECK(fabs(atan2(nodes[1].forward.j, nodes[1].forward.i) - 85.0f * PI / 180.0f) < 1e-3f,
			"170 degrees not blended halfway: forward (%f, %f)", nodes[1].forward.i, nodes[1].forward.j);
	}
	/* a shell loaded: the count's frame changes and the shell moves from 0.1
	along the hand to 0.3 in the tick. Through the tick it stays at 0.1 on the
	hand as the hand blends (not through the gun, nor at 0.3 ahead of the
	hand), and takes its new place as the blend reaches the tick that loaded
	it; the next tick, the count unchanged, it blends again */
	CASE("count")
	{
		counted(10, 1.0f, 3, 0.0f, 0.1f, held_shell);
		counted(11, 0.5f, 4, 0.8f, 0.3f, held_shell);
		expect_each(0.4f, 0.5f);
		counted(11, 0.75f, 4, 0.8f, 0.3f, held_shell);
		expect_each(0.6f, 0.7f);
		counted(12, 0.5f, 4, 1.6f, 0.4f, held_shell);
		expect_each(1.2f, 1.55f);
	}
	/* the count unchanged: its nodes blend as the others do */
	CASE("count kept")
	{
		counted(10, 1.0f, 3, 0.0f, 0.1f, held_shell);
		counted(11, 0.5f, 3, 0.8f, 0.2f, held_shell);
		expect_each(0.4f, 0.55f);
	}
	/* a node the count moves that has no parent: where the tick before had it */
	CASE("count loose")
	{
		counted(10, 1.0f, 3, 0.0f, 0.1f, loose_shell);
		counted(11, 0.5f, 4, 0.8f, 0.3f, loose_shell);
		expect_each(0.4f, 0.1f);
	}
	/* no count shown (a needler's needles settling as its reload ends): nothing snaps */
	CASE("no count")
	{
		counted(10, 1.0f, 3, 0.0f, 0.0f, held_shell);
		frame(11, 0.5f, WEAPON, GRAPH, 0.8f);
		expect(0.4f);
	}
	return 0;
}
