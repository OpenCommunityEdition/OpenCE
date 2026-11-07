/*
FIRST_PERSON_LEGS_EYE.C (test)

The real first_person_legs_clear_of_eye (first_person_legs.c), with the
names it reads the legs by: how far back behind the eye the first-person
legs are drawn, so that no vertex of the legs comes nearer the eye than the
near clip plane and a little. A crouched walk raised the knees to the eye,
where the near clip plane cut them open and the legs were seen from inside.
The fake mesh's vertices are in the world already (every node's matrix and
inverse the identity); the eye is at the origin, and the body is set back
along -x (back = (-1, 0, 0)). test_first_person_legs_eye.py takes the
constants and the code from the sources (config.inc, under_test.inc).
*/

#include <math.h>
#include "harness.h"

#define TAG_STRING_LENGTH 31
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))

#include "config.inc"

typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;

static void matrix4x3_multiply(real_matrix4x3 const *a, real_matrix4x3 const *b, real_matrix4x3 *out) { *out = *a; }
static void matrix4x3_transform_point(real_matrix4x3 const *matrix, real_point3d const *point, real_point3d *out)
{
	out->x = point->x + matrix->position.x;
	out->y = point->y + matrix->position.y;
	out->z = point->z + matrix->position.z;
}
static void vector_from_points3d(real_point3d const *from, real_point3d const *to, real_vector3d *out)
{
	out->i = to->x - from->x;
	out->j = to->y - from->y;
	out->k = to->z - from->z;
}
static real dot_product3d(real_vector3d const *a, real_vector3d const *b) { return a->i * b->i + a->j * b->j + a->k * b->k; }
static real square_root(real value) { return sqrtf(value); }

/* the model's nodes: their names */
struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])
struct model_node { char name[32]; };
struct model { struct tag_block nodes; };
enum { PELVIS, SPINE, LEFT_THIGH, LEFT_CALF, LEFT_FOOT, HEAD, NODE_COUNT };
static struct model_node node_names[NODE_COUNT] = {
	{ "bip01 pelvis" }, { "bip01 spine" }, { "bip01 l thigh" }, { "bip01 l calf" }, { "bip01 l foot" }, { "bip01 head" }
};
static struct model model = { { NODE_COUNT, node_names } };

/* the mesh: its vertices, bound to the nodes */
#include "mesh.inc"
static struct object_mesh_vertex vertices[64];
static struct object_mesh mesh;
static real_matrix4x3 nodes[NODE_COUNT];

/* the window: the eye and the near clip plane */
static struct { struct { real_point3d position; } camera; struct { real z_near; } frustum; } global_window_parameters =
	{ { { 0.f, 0.f, 0.f } }, { 0.0625f } };

#include "under_test.inc"

static real_vector3d back = { -1.f, 0.f, 0.f };

static void reset(void)
{
	memset(vertices, 0, sizeof(vertices));
	memset(&mesh, 0, sizeof(mesh));
	memset(nodes, 0, sizeof(nodes));
	mesh.node_count = NODE_COUNT;
	mesh.vertices = vertices;
}

/* a vertex at a point, bound to one node, or to two (weight of the first, of 32767) */
static void vertex(real x, real y, real z, short first, short second, short weight)
{
	struct object_mesh_vertex *v = &vertices[mesh.vertex_count++];

	v->position.x = x;
	v->position.y = y;
	v->position.z = z;
	v->nodes[0] = (byte)first;
	v->nodes[1] = (byte)second;
	v->weight = weight;
}

#define CLEAR (0.0625f + FIRST_PERSON_LEGS_CLEARANCE)

/* how near the eye the legs' vertices come, set back so far */
static real nearest(real set_back)
{
	real least = 1e9f;
	long index;

	for (index = 0; index < mesh.vertex_count; index++)
	{
		struct object_mesh_vertex const *v = &vertices[index];
		real dx = v->position.x - set_back;
		real d = sqrtf(dx * dx + v->position.y * v->position.y + v->position.z * v->position.z);

		if (d < least)
			least = d;
	}
	return least;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	real set_back;

	/* a knee a little ahead of the eye and just below it, as a crouched walk
	raises it: the legs set back until it is behind the eye and clear */
	CASE("knee-at-eye")
	{
		reset();
		vertex(0.02f, 0.f, -0.03f, LEFT_CALF, LEFT_CALF, 32767);
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.04f);
		CHECK(set_back > 0.04f, "the knee at the eye was not cleared (set back %f)", set_back);
		CHECK(fabsf(nearest(set_back) - CLEAR) < 1e-4f, "the knee is %f from the eye, not %f", nearest(set_back), CLEAR);
		return 0;
	}
	/* legs far below the eye, as when standing: the set back as it was */
	CASE("legs-far")
	{
		reset();
		vertex(0.05f, 0.f, -0.30f, LEFT_THIGH, LEFT_THIGH, 32767);
		vertex(0.10f, 0.02f, -0.50f, LEFT_FOOT, LEFT_CALF, 20000);
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.04f);
		CHECK(set_back == 0.04f, "legs far from the eye moved the set back to %f", set_back);
		return 0;
	}
	/* a knee ahead of the eye, nearer than the set back reaches: left as it
	is (moving it back would bring it to the eye) */
	CASE("knee-ahead")
	{
		reset();
		vertex(0.30f, 0.f, -0.05f, LEFT_CALF, LEFT_CALF, 32767);
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.04f);
		CHECK(set_back == 0.04f, "a knee well ahead of the eye moved the set back to %f", set_back);
		/* (but a set back that would bring it to the eye goes past it) */
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.30f);
		CHECK(nearest(set_back) >= CLEAR - 1e-4f, "set back %f leaves the knee %f from the eye", set_back,
			nearest(set_back));
		return 0;
	}
	/* the upper body (folded away into the waist when drawn) and the waist
	that joins it are not the legs' */
	CASE("upper-body")
	{
		reset();
		vertex(0.01f, 0.f, -0.02f, HEAD, HEAD, 32767);
		vertex(0.01f, 0.f, -0.02f, PELVIS, SPINE, 16000);
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.04f);
		CHECK(set_back == 0.04f, "the upper body moved the set back to %f", set_back);
		return 0;
	}
	/* two knees, the second where the first's clearing set back puts it: both cleared */
	CASE("chain")
	{
		reset();
		vertex(0.00f, 0.f, -0.02f, LEFT_CALF, LEFT_CALF, 32767);
		vertex(0.15f, 0.f, -0.02f, LEFT_THIGH, LEFT_CALF, 30000);
		set_back = first_person_legs_clear_of_eye(&mesh, &model, nodes, &back, 0.04f);
		CHECK(nearest(set_back) >= CLEAR - 1e-4f, "set back %f leaves a knee %f from the eye", set_back,
			nearest(set_back));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
