/*
CREWMAN_HANDS.C (test)

The real crewman_hands_turn, _node_turn, _deepest, _arm_lift and
_postprocess_node_matrices (crewman_hands.c): a crewman's arm held with a
fingertip 0.01 into a console (a plane here; the line from the shoulder to
each vertex crossing it is the collision) has its elbow bent until every
vertex is out by the clearance, the elbow staying where it is and the upper
arm not moved; one with its forearm in it, beside the elbow, is raised at
the shoulder instead, the shoulder staying where it is; an arm out of
everything, or one past something thin, is not bent; no joint bends more
than 30 degrees; and only a seated crewman is measured.
test_crewman_hands.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define square_root(x) ((real)sqrt(x))
#define sine(x) ((real)sin(x))
#define cosine(x) ((real)cos(x))

typedef struct { real_vector3d n; real d; } real_plane3d;
typedef struct real_matrix4x3
{
	real scale;
	real_vector3d forward;
	real_vector3d left;
	real_vector3d up;
	real_point3d position;
} real_matrix4x3;

enum
{
	_collision_test_front_facing_surfaces_bit = 0,
	_collision_test_ignore_invisible_surfaces_bit = 3,
	_collision_test_structure_bit = 5,
	_collision_test_objects_bit = 7,
	_collision_test_objects_vehicles_bit = 9,
	_collision_test_objects_scenery_bit = 14,
	_collision_test_objects_machines_bit = 15,
	_collision_test_objects_controls_bit = 16,
};
enum { _object_mask_unit = 3 };

struct collision_result
{
	real_plane3d plane;
	long object_index;
};

struct object_datum
{
	long definition_index;
	struct { long parent_object_index; } object;
};
struct object_definition
{
	struct { struct { long index; } model; } object;
};

#include "config.inc"

/* the console: a plane (unit facing, distance); the crewman being tested */
static real_plane3d test_console;
static long test_ignored = NONE;
static struct object_datum test_crewman;
static struct object_definition test_definition;
static boolean test_is_unit = TRUE;

static real_point3d *matrix4x3_transform_point(real_matrix4x3 const *m, real_point3d const *p, real_point3d *r)
{
	real_point3d out;

	out.x = m->position.x + m->scale * (m->forward.i * p->x + m->left.i * p->y + m->up.i * p->z);
	out.y = m->position.y + m->scale * (m->forward.j * p->x + m->left.j * p->y + m->up.j * p->z);
	out.z = m->position.z + m->scale * (m->forward.k * p->x + m->left.k * p->y + m->up.k * p->z);
	*r = out;
	return r;
}

static real test_height(real_point3d const *p)
{
	return test_console.n.i * p->x + test_console.n.j * p->y + test_console.n.k * p->z - test_console.d;
}

/* the line crosses the console's front, from before it to behind it */
static boolean collision_test_vector(unsigned long flags, real_point3d const *point, real_vector3d const *vector,
	long ignore_object_index, struct collision_result *collision)
{
	real_point3d end;

	(void)flags;
	test_ignored = ignore_object_index;
	end.x = point->x + vector->i;
	end.y = point->y + vector->j;
	end.z = point->z + vector->k;
	if (test_height(point) > 0.f && test_height(&end) < 0.f)
	{
		collision->plane = test_console;
		collision->object_index = NONE;
		return TRUE;
	}
	return FALSE;
}

static struct object_datum *object_get(long object_index) { (void)object_index; return &test_crewman; }
static struct object_definition *object_definition_get(long definition_index) { (void)definition_index; return &test_definition; }
static void *object_try_and_get_and_verify_type(long object_index, long mask)
{
	(void)object_index; (void)mask;
	return test_is_unit ? &test_crewman : NULL;
}

#include "under_test.inc"

/* nodes: 0 right upper arm, 1 right forearm, 2 right hand (no left arm's
vertices); every node square to the world, so a vertex is at its node's
place plus its offset */
static real_matrix4x3 test_nodes[3];

static void test_node(short index, real x, real y, real z)
{
	memset(&test_nodes[index], 0, sizeof(test_nodes[index]));
	test_nodes[index].scale = 1.f;
	test_nodes[index].forward.i = 1.f;
	test_nodes[index].left.j = 1.f;
	test_nodes[index].up.k = 1.f;
	test_nodes[index].position.x = x;
	test_nodes[index].position.y = y;
	test_nodes[index].position.z = z;
}

static void test_vertex(short node_index, real x, real y, real z)
{
	short count = crewman_hands_vertex_counts[0];

	crewman_hands_vertices[0][count].node_index = node_index;
	crewman_hands_vertices[0][count].position.x = x;
	crewman_hands_vertices[0][count].position.y = y;
	crewman_hands_vertices[0][count].position.z = z;
	crewman_hands_vertex_counts[0] = (short)(count + 1);
}

/* the shoulder above the console, the elbow at its height, the wrist 0.07
on and a fingertip 0.01 into it (the console z = 0, facing up) */
static void test_arm(boolean fingertip_only)
{
	test_node(0, 0.f, 0.f, 0.25f);
	test_node(1, 0.05f, 0.f, 0.f);
	test_node(2, 0.12f, 0.f, 0.f);
	crewman_hands_arms[0].upper_arm_node_index = 0;
	crewman_hands_arms[0].forearm_node_index = 1;
	crewman_hands_arms[0].hand_node_index = 2;
	crewman_hands_arms[1] = crewman_hands_arms[0];
	crewman_hands_vertex_counts[0] = 0;
	crewman_hands_vertex_counts[1] = 0;
	test_vertex(2, 0.06f, 0.f, -0.01f);
	if (!fingertip_only)
	{
		test_vertex(2, 0.02f, 0.f, 0.01f);
		test_vertex(1, 0.03f, 0.f, 0.02f);
	}
	test_console.n.i = 0.f;
	test_console.n.j = 0.f;
	test_console.n.k = 1.f;
	test_console.d = 0.f;
}

/* how far out of the console the arm's lowest vertex is */
static real test_lowest(void)
{
	real lowest = 1000.f;
	short index;

	for (index = 0; index < crewman_hands_vertex_counts[0]; index++)
	{
		real_point3d point;

		matrix4x3_transform_point(&test_nodes[crewman_hands_vertices[0][index].node_index],
			&crewman_hands_vertices[0][index].position, &point);
		lowest = MIN(lowest, test_height(&point));
	}
	return lowest;
}

static boolean test_same(real_matrix4x3 const *a, real_matrix4x3 const *b)
{
	return !memcmp(a, b, sizeof(*a));
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the fingertip lifted out by the clearance, by the elbow alone */
	CASE("lifted")
	{
		real_matrix4x3 shoulder;
		real_point3d elbow;
		real bent;
		real length;
		real across;

		test_arm(FALSE);
		shoulder = test_nodes[0];
		elbow = test_nodes[1].position;
		bent = crewman_hands_arm_lift(5, test_nodes, 0);
		CHECK(bent > 0.f && bent < 0.2f, "bent %f", bent);
		CHECK(test_lowest() >= 0.75f * CREWMAN_HANDS_CLEARANCE && test_lowest() < 0.01f, "lowest %f", test_lowest());
		CHECK(test_ignored == 5, "the crewman himself tested against (%ld)", test_ignored);
		CHECK(test_same(&test_nodes[0], &shoulder), "the upper arm moved");
		CHECK(test_nodes[1].position.x == elbow.x && test_nodes[1].position.y == elbow.y &&
			test_nodes[1].position.z == elbow.z, "the elbow moved");
		length = (real)sqrt(
			(test_nodes[2].position.x - elbow.x) * (test_nodes[2].position.x - elbow.x) +
			(test_nodes[2].position.y - elbow.y) * (test_nodes[2].position.y - elbow.y) +
			(test_nodes[2].position.z - elbow.z) * (test_nodes[2].position.z - elbow.z));
		CHECK(fabs(length - 0.07f) < 1e-5f, "the forearm's length %f", length);
		CHECK(test_nodes[2].position.z > 0.f, "the wrist not raised (%f)", test_nodes[2].position.z);
		across = test_nodes[2].forward.i * test_nodes[2].up.i + test_nodes[2].forward.j * test_nodes[2].up.j +
			test_nodes[2].forward.k * test_nodes[2].up.k;
		CHECK(fabs(across) < 1e-5f, "the hand's frame skewed (%f)", across);
		CHECK(crewman_hands_arm_lift(5, test_nodes, 0) == 0.f, "bent again");
		return 0;
	}
	/* out of the console, or past something thin: not bent */
	CASE("untouched")
	{
		real_matrix4x3 before[3];

		test_arm(FALSE);
		test_console.d = -0.02f;
		memcpy(before, test_nodes, sizeof(before));
		CHECK(crewman_hands_arm_lift(5, test_nodes, 0) == 0.f, "bent out of the console");
		CHECK(!memcmp(before, test_nodes, sizeof(before)), "moved out of the console");
		test_arm(TRUE);
		test_console.d = 0.04f;
		memcpy(before, test_nodes, sizeof(before));
		CHECK(crewman_hands_arm_lift(5, test_nodes, 0) == 0.f, "bent for a vertex 0.05 past the surface");
		CHECK(!memcmp(before, test_nodes, sizeof(before)), "moved for a vertex 0.05 past the surface");
		return 0;
	}
	/* the forearm in the console beside the elbow: raised at the shoulder */
	CASE("shoulder")
	{
		real_point3d shoulder;
		real_point3d elbow;
		real bent;

		test_arm(TRUE);
		crewman_hands_vertex_counts[0] = 0;
		test_vertex(1, 0.01f, 0.f, -0.01f);
		shoulder = test_nodes[0].position;
		elbow = test_nodes[1].position;
		bent = crewman_hands_arm_lift(5, test_nodes, 0);
		CHECK(bent > 0.f && bent < 0.2f, "bent %f", bent);
		CHECK(test_lowest() >= 0.75f * CREWMAN_HANDS_CLEARANCE, "lowest %f", test_lowest());
		CHECK(test_nodes[0].position.x == shoulder.x && test_nodes[0].position.y == shoulder.y &&
			test_nodes[0].position.z == shoulder.z, "the shoulder moved");
		CHECK(test_nodes[0].forward.i != 1.f || test_nodes[0].forward.k != 0.f, "the upper arm not turned");
		CHECK(test_nodes[1].position.z > elbow.z, "the elbow not raised (%f)", test_nodes[1].position.z);
		return 0;
	}
	/* a hand's vertex nearly under the elbow, 0.029 in: no more than 30 degrees */
	CASE("capped")
	{
		real_vector3d forward;
		real bent;
		real angle;

		test_arm(TRUE);
		crewman_hands_vertex_counts[0] = 0;
		test_vertex(2, -0.05f, 0.f, -0.01f);
		test_nodes[1].position.z = 0.12f;
		test_console.d = 0.019f;
		forward = test_nodes[1].forward;
		bent = crewman_hands_arm_lift(5, test_nodes, 0);
		angle = (real)acos(MIN(1.f, forward.i * test_nodes[1].forward.i + forward.j * test_nodes[1].forward.j +
			forward.k * test_nodes[1].forward.k));
		CHECK(bent > 0.5f && bent <= CREWMAN_HANDS_MAXIMUM_ANGLE + 1e-5f, "bent %f", bent);
		CHECK(angle <= CREWMAN_HANDS_MAXIMUM_ANGLE + 1e-4f, "the forearm turned %f", angle);
		return 0;
	}
	/* a seated crewman, nothing else (not one standing, scripted or not) */
	CASE("who")
	{
		real_matrix4x3 before[3];

		test_arm(FALSE);
		memcpy(before, test_nodes, sizeof(before));
		crewman_hands_model_index = 7;
		test_definition.object.model.index = 7;
		test_crewman.object.parent_object_index = NONE;
		CHECK(crewman_hands_postprocess_node_matrices(5, test_nodes) == 0, "a standing crewman bent");
		CHECK(!memcmp(before, test_nodes, sizeof(before)), "a standing crewman moved");
		test_crewman.object.parent_object_index = 9;
		CHECK(crewman_hands_postprocess_node_matrices(5, test_nodes) == 1, "a seated crewman not bent");
		memcpy(test_nodes, before, sizeof(before));
		test_definition.object.model.index = 8;
		CHECK(crewman_hands_postprocess_node_matrices(5, test_nodes) == 0, "another model bent");
		test_definition.object.model.index = 7;
		test_is_unit = FALSE;
		CHECK(crewman_hands_postprocess_node_matrices(5, test_nodes) == 0, "not a unit, bent");
		test_is_unit = TRUE;
		crewman_hands_model_index = NONE;
		CHECK(crewman_hands_postprocess_node_matrices(5, test_nodes) == 0, "bent with no crewman loaded");
		CHECK(!memcmp(before, test_nodes, sizeof(before)), "moved by the others");
		return 0;
	}

	return 2;
}
