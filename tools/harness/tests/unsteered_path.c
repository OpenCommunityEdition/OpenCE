/* upstream's path of an unsteered vehicle (network_objects.c), taken from the sources against fakes: a replaced object
in a slot is not carried by the old object's path, and a teleport restarts it */
#include "harness.h"
#include <math.h>

typedef struct { real i, j; } real_vector2d;
#define VEHICLE_A 0x12340007L
#define VEHICLE_B 0x12350007L
#define MAXIMUM_TRACKED_OBJECTS 64
#define _object_mask_vehicle 1
#define _network_vehicle_write_carry 1
#define _network_vehicle_write_state 0
#include "types.inc"
struct object_datum
{
	struct
	{
		real_point3d position;
		real_vector3d forward, up, translational_velocity, angular_velocity;
		long parent_object_index;
	} object;
};
struct object_iterator { long index; int visited; };
static struct object_datum vehicle;
static long tick, iterated = VEHICLE_A;
static int sets;
static real_vector3d const zero = {0, 0, 0};
static real_vector3d const *global_zero_vector3d = &zero;
static struct object_datum *object_get(long index) { return &vehicle; }
static long game_time_get(void) { return tick; }
static boolean distributed_vehicle_unsteered(long index) { return TRUE; }
static boolean distributed_transform_valid(real_point3d const *p, real_vector3d const *f, real_vector3d const *u,
	real_vector3d const *v, real_vector3d const *a, real_vector3d *vf, real_vector3d *vu)
{
	*vf = *f; *vu = *u; return TRUE;
}
static void unsteered_blend_facing(real_vector3d const *f0, real_vector3d const *u0, real_vector3d const *f1,
	real_vector3d const *u1, real t, real_vector3d *f, real_vector3d *u)
{
	*f = *f1; *u = *u1;
}
static void object_set_position(long index, real_point3d const *p, real_vector3d const *f, real_vector3d const *u)
{
	sets++;
	vehicle.object.position = *p; vehicle.object.forward = *f; vehicle.object.up = *u;
}
static void network_objects_vehicle_written(long index, real_point3d const *p, real_vector3d const *f,
	real_vector3d const *u, short kind) { }
static void object_iterator_new(struct object_iterator *it, unsigned long mask, long flags) { it->visited = 0; it->index = iterated; }
static struct object_datum *object_iterator_next(struct object_iterator *it) { return it->visited++ ? NULL : &vehicle; }
#include "under_test.inc"

static void setup(void)
{
	memset(&vehicle, 0, sizeof(vehicle)); memset(objects_client_unsteered_path, 0, sizeof(objects_client_unsteered_path));
	vehicle.object.forward.i = 1.f; vehicle.object.up.k = 1.f; vehicle.object.parent_object_index = NONE;
	tick = 100; iterated = VEHICLE_A; sets = 0;
}
static void sample(real x, long host_time)
{
	real_point3d position = {x, 0.f, 0.f};
	real_vector3d forward = {1.f, 0.f, 0.f}, up = {0.f, 0.f, 1.f};

	distributed_client_note_unsteered_path(VEHICLE_A, &position, &forward, &up, host_time, FALSE);
}
int main(int argc, char **argv)
{
	char const *case_name = argv[1];

	CASE("path-replaced-object-kept-still")
	{
		real_point3d before;

		setup();
		sample(10.f, 100);
		tick = 102;
		sample(11.f, 102);
		/* (the slot's vehicle was replaced: the old path's salted index is not the new one's) */
		iterated = VEHICLE_B;
		before = vehicle.object.position;
		sets = 0;
		tick = 103;
		distributed_client_carry_unsteered_vehicles();
		CHECK(sets == 0 && !memcmp(&before, &vehicle.object.position, sizeof(before)),
			"the replaced vehicle was moved by the old one's path: %d sets, x %f", sets, vehicle.object.position.x);
		return 0;
	}
	CASE("path-carries-its-own-object")
	{
		setup();
		sample(10.f, 100);
		tick = 102;
		sample(11.f, 102);
		sets = 0;
		tick = 103;
		distributed_client_carry_unsteered_vehicles();
		CHECK(sets == 1 && vehicle.object.position.x > 10.f && vehicle.object.position.x <= 11.f,
			"the path's own vehicle was not carried: %d sets, x %f", sets, vehicle.object.position.x);
		return 0;
	}
	CASE("path-teleport-restarts")
	{
		setup();
		sample(10.f, 100);
		tick = 102;
		sample(11.f, 102);
		CHECK(objects_client_unsteered_path[7].velocity.i != 0.f, "no path velocity before the jump: setup");
		tick = 103;
		sample(500.f, 103);
		CHECK(vehicle.object.position.x == 500.f && vehicle.object.translational_velocity.i == 0.f,
			"a teleport was glided to, not taken whole: x %f", vehicle.object.position.x);
		CHECK(objects_client_unsteered_path[7].velocity.i == 0.f && objects_client_unsteered_path[7].previous_position.x == 500.f &&
			objects_client_unsteered_path[7].interval == 1.f, "a teleport did not restart the path: velocity %f previous %f",
			objects_client_unsteered_path[7].velocity.i, objects_client_unsteered_path[7].previous_position.x);
		return 0;
	}
	fprintf(stderr, "unknown case %s\n", case_name);
	return 2;
}
