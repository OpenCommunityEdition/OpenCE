#include "harness.h"
#include <math.h>

#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#include "config.inc"

/* a vehicle, its pilot and passenger, and the pilot's weapon: an object is drawn at its position plus its parent's (objects.c) */
struct _object_datum
{
	real_point3d position;
	real_vector3d forward, up, translational_velocity, angular_velocity;
	unsigned long flags;
	long parent_object_index, first_child_object_index, next_object_index;
};
struct object_datum { struct _object_datum object; real_point3d node; };
static struct object_datum objects[4] =
{
	{{{10, 0, 0}, {0}, {0}, {0}, {0}, 0, NONE, 1, NONE}},
	{{{0.5f, 0, 1}, {0}, {0}, {0}, {0}, 0, 0, 3, 2}},
	{{{-0.5f, 0, 1}, {0}, {0}, {0}, {0}, 0, 0, NONE, NONE}},
	{{{0.3f, -0.2f, 0.1f}, {0}, {0}, {0}, {0}, 0, 1, NONE, NONE}},
};

static struct object_datum *object_get(long index) { return &objects[index]; }
static void object_compute_node_matrices(long index)
{
	struct object_datum *object = &objects[index];
	struct object_datum *parent = object->object.parent_object_index == NONE ? NULL : &objects[object->object.parent_object_index];

	object->node = object->object.position;
	if (parent)
	{
		object->node.x += parent->node.x;
		object->node.y += parent->node.y;
		object->node.z += parent->node.z;
	}
}
/* (objects.c: only its own nodes are computed) */
static void object_set_position(long index, real_point3d const *position, real_vector3d const *forward, real_vector3d const *up)
{
	objects[index].object.position = *position;
	object_compute_node_matrices(index);
}
static void distributed_body_unfix_pose(long index) { }
static void render_interpolation_correct_object(long index, real_vector3d const *offset) { }
static boolean distributed_vehicle_unsteered(long index) { return FALSE; }
static boolean distributed_object_follows_host(long index) { return FALSE; }
static void network_objects_vehicle_writing(long index, short kind) { }
static void network_objects_vehicle_written(long index, real_point3d const *before,
	real_vector3d const *before_forward, real_vector3d const *before_up, short kind) { }
static boolean distributed_transform_valid(real_point3d const *position, real_vector3d const *forward, real_vector3d const *up,
	real_vector3d const *velocity, real_vector3d const *angular_velocity, real_vector3d *valid_forward, real_vector3d *valid_up)
{
	*valid_forward = *forward;
	*valid_up = *up;
	return TRUE;
}
#include "under_test.inc"

static void family_follows(char const *what)
{
	long index;

	for (index = 1; index < 4; index++)
	{
		struct object_datum *object = &objects[index];
		struct object_datum *parent = &objects[object->object.parent_object_index];

		CHECK(fabs(object->node.x - parent->node.x - object->object.position.x) < 1e-4 &&
			fabs(object->node.y - parent->node.y - object->object.position.y) < 1e-4 &&
			fabs(object->node.z - parent->node.z - object->object.position.z) < 1e-4,
			"%s: object %ld is drawn at (%.2f %.2f %.2f), not on its parent (%.2f %.2f %.2f)", what, index,
			object->node.x, object->node.y, object->node.z, parent->node.x, parent->node.y, parent->node.z);
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];

	/* a vehicle the netcode moves, with a seated pilot holding a weapon and a passenger: drawn with it at once, not where it was
	until their next update (objects.c) */
	CASE("moved")
	{
		real_point3d far = {14.0f, 3.0f, 0.0f}, near = {14.4f, 3.0f, 0.0f};
		real_vector3d forward = {1.0f, 0.0f, 0.0f}, up = {0.0f, 0.0f, 1.0f};

		object_compute_node_matrices_recursive(0);
		family_follows("the fake world");
		CHECK(network_objects_reconcile(0, &far, &forward, &up, NULL, NULL, 1.0f, 0), "the fake world: put there");
		family_follows("put");
		CHECK(!network_objects_reconcile(0, &near, &forward, &up, NULL, NULL, 1.0f, 0) && objects[0].object.position.x == 14.2f,
			"the fake world: blended halfway");
		family_follows("blended");
	}
	return 0;
}
