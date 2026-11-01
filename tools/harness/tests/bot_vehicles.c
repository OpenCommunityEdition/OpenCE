#include "harness.h"
#include <math.h>
#define BOTS_MAXIMUM 15
#define TICKS_PER_SECOND 30
#define MAXIMUM_WEAPONS_PER_UNIT 4
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define PIN(a,l,h) ((a) < (l) ? (l) : (a) > (h) ? (h) : (a))
#define csmemset memset
#define SET_FLAG(f,b,v) ((v) ? ((f) |= FLAG(b)) : ((f) &= ~FLAG(b)))
#define platform_log(...) ((void)0)
#define VEHICLE_DEFINITION_TAG 0
#define _object_mask_vehicle 1
#define _object_mask_unit 2
enum { _object_dead_bit, _unit_not_enterable_by_player_bit, _unit_seat_driver_bit, _unit_seat_gunner_bit };
enum { _unit_control_action_bit, _unit_control_jump_bit, _unit_control_throw_grenade_bit,
	_unit_control_use_equipment_bit, _unit_control_weapon_primary_trigger_bit };
enum { _unit_nearby_seat_available = 2 };
struct object_state {
	long parent_object_index;
	long first_child_object_index, next_object_index;
	short type;
	real_vector3d up, forward;
	real_point3d position;
	real_point3d bounding_sphere_center;
	real bounding_sphere_radius;
	unsigned long damage_flags;
};
struct object_datum { long definition_index; struct object_state object; };
struct unit_datum {
	long definition_index;
	struct object_state object;
	struct { long driver_object_index, gunner_object_index, weapon_object_indices[4];
		short current_weapon_index, parent_seat_index; unsigned long flags; long player_index; real_vector3d aiming_vector; } unit;
};
struct unit_seat { unsigned long flags; };
struct unit_definition { struct { struct { long count; struct unit_seat *address; } seats; } unit; };
#define TAG_BLOCK_GET_ELEMENT(block, i, type) (&(block)->address[i])
struct player_action { struct { real i, j; } throttle; struct { real yaw, pitch; } desired_facing;
	unsigned long control_flags; real primary_trigger; };
static struct unit_datum units[3];
static struct unit_seat seat;
static struct unit_definition tag;
static byte vehicle_tag[1024];
static long now = 100;
static int enabled = 1, available = 1, clock_ready;
struct player_datum { short team_index; };
static struct player_datum player;
static int game_engine_has_teams(void) { return 1; }
static struct player_datum *player_try_and_get(long index) { return index == NONE ? NULL : &player; }
struct weapon_trigger_definition { real charging_time; };
struct weapon_definition { struct { struct { long count; struct weapon_trigger_definition *address; } triggers; } weapon; };
static struct weapon_definition weapon_tag;
static struct weapon_trigger_definition trigger;
static struct object_datum *weapon_get(long index) { (void)index; return (void *)&units[0]; }
static struct weapon_definition *weapon_definition_get(long index) { (void)index; return &weapon_tag; }
static void bots_vehicle_obstacle(long index, real_point3d const *goal) { (void)index; (void)goal; }
static void vector_from_points3d(real_point3d const *a, real_point3d const *b, real_vector3d *v)
{ v->i = b->x-a->x; v->j = b->y-a->y; v->k = b->z-a->z; }
static real dot_product3d(real_vector3d const *a, real_vector3d const *b) { return a->i*b->i+a->j*b->j+a->k*b->k; }
static real magnitude3d(real_vector3d const *v) { return (real)sqrt(dot_product3d(v,v)); }
static int config_boolean(char const *name) { (void)name; return enabled; }
static long game_time_get(void) { CHECK(clock_ready, "clock read outside a running map"); return now; }
static struct object_datum *object_get(long index) { CHECK(index >= 0 && index < 3, "index"); return (void *)&units[index]; }
static struct object_datum *object_try_and_get(long index) { return index >= 0 && index < 3 ? object_get(index) : NULL; }
static struct object_datum *object_try_and_get_and_verify_type(long index, int type)
{ (void)type; return index == 1 ? object_get(index) : NULL; }
static struct unit_datum *unit_get(long index) { return &units[index]; }
static void *tag_get(int group, long index) { (void)group; (void)index; return vehicle_tag; }
static struct unit_definition *unit_definition_get(long index) { (void)index; return &tag; }
static boolean unit_can_enter_seat(long a, long b, short c, void *d)
{ (void)a; (void)b; (void)c; (void)d; return available; }
static boolean unit_has_animation_to_enter_seat(long a, long b, short c)
{ (void)a; (void)b; (void)c; return TRUE; }
static int unit_find_nearby_seat(long a, long b, short *chosen)
{ (void)a; (void)b; *chosen = 0; return available ? _unit_nearby_seat_available : 0; }
#include "under_test.inc"

int main(int argc, char **argv)
{
	struct player_action action;
	real_point3d goal = { 100, 0, 0 };
	real radius = 0.1f;
	int i;
	CHECK(argc == 2, "case");
	bots_vehicles_reset();
	clock_ready = 1;
	memset(&action, 0, sizeof(action));
	units[0].object.parent_object_index = 1;
	units[1].unit.driver_object_index = 0;
	units[1].unit.gunner_object_index = NONE;
	units[1].object.up.k = units[1].object.forward.i = 1;
	units[1].object.first_child_object_index = NONE;
	units[1].unit.aiming_vector.i = 1;
	tag.unit.seats.count = 1;
	tag.unit.seats.address = &seat;
	seat.flags = FLAG(_unit_seat_driver_bit);
	((struct bots_vehicle_definition *)vehicle_tag)->type = 1;
	((struct bots_vehicle_definition *)vehicle_tag)->radius = 1.5f;
	if (!strcmp(argv[1], "seat"))
	{
		CHECK(!seat_available(0, 0, 1, -1) && !seat_available(0, 0, 1, 1), "seat bounds");
		CHECK(seat_available(0, 0, 1, 0), "valid seat refused");
		vehicles[1].sought = 1; vehicles[1].seat = 0;
		CHECK(!seat_available(0, 0, 1, 0), "reservation ignored");
		bots_vehicles_reset_player(1);
		available = 0;
		CHECK(!seat_available(0, 0, 1, 0), "engine rejection ignored");
	}
	else if (!strcmp(argv[1], "driver"))
	{
		CHECK(bots_vehicle_navigation(0, &radius) == 1 && radius == 1.5f, "driver used biped navigation");
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0.7f, NONE, FALSE, &action);
		CHECK(action.throttle.i == 0, "driver left before entry grace elapsed");
		now += TICKS_PER_SECOND;
		action.primary_trigger = 1; action.control_flags = FLAG(_unit_control_weapon_primary_trigger_bit);
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0.7f, NONE, FALSE, &action);
		CHECK(action.throttle.i > 0 && action.desired_facing.yaw == 0.7f, "driver did not steer");
		CHECK(!action.primary_trigger && !TEST_FLAG(action.control_flags, _unit_control_weapon_primary_trigger_bit), "steering fired an aimed shot");
	}
	else if (!strcmp(argv[1], "disable"))
	{
		enabled = 0;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, NONE, FALSE, &action);
		CHECK(TEST_FLAG(action.control_flags, _unit_control_action_bit), "disable left seated bot stranded");
		units[0].object.parent_object_index = NONE;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, NONE, FALSE, &action);
		CHECK(vehicles[0].ridden == NONE && vehicles[0].sought == NONE, "exit retained reservation");
	}
	else if (!strcmp(argv[1], "gunner"))
	{
		units[1].unit.driver_object_index = NONE; units[1].unit.gunner_object_index = 0;
		units[1].unit.current_weapon_index = -1;
		CHECK(bots_vehicle_weapon(0, 90) == 90, "invalid weapon slot");
		units[1].unit.current_weapon_index = 2; units[1].unit.weapon_object_indices[2] = 91;
		CHECK(bots_vehicle_weapon(0, 90) == 91, "gunner did not use turret weapon");
		action.primary_trigger = 1;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, NONE, FALSE, &action);
		CHECK(action.primary_trigger == 1, "gunner trigger cleared");
	}
	else if (!strcmp(argv[1], "stuck"))
	{
		for (i = 0; i <= 6; i++) {
			now += TICKS_PER_SECOND;
			action.control_flags = 0;
			bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0, NONE, FALSE, &action);
		}
		CHECK(TEST_FLAG(action.control_flags, _unit_control_action_bit), "stuck driver never exited");
	}
	else if (!strcmp(argv[1], "ghost"))
	{
		((struct bots_vehicle_definition *)vehicle_tag)->type = 3;
		CHECK(!drivable(1), "human plane treated as scout");
		((struct bots_vehicle_definition *)vehicle_tag)->type = 4;
		CHECK(drivable(1), "alien scout refused");
		units[2].object.position.x = 50;
		bots_vehicle_action(0, 0, &goal, TRUE, TRUE, 1.0f, 2, FALSE, &action);
		now += 30; action.primary_trigger = 1;
		bots_vehicle_action(0, 0, &goal, TRUE, TRUE, 1.0f, 2, FALSE, &action);
		CHECK(action.throttle.j > 0.8f && action.desired_facing.yaw == 0 && action.primary_trigger == 1, "scout lost independent aim");
	}
	else if (!strcmp(argv[1], "claim"))
	{
		vehicles[1].sought = 1; vehicles[1].seat = 0; vehicles[1].unit_index = 2;
		units[2].object.position.x = 20;
		CHECK(seat_available(0, 0, 1, 0), "closer teammate could not take distant claim");
		units[2].object.position.x = 10;
		CHECK(!seat_available(0, 0, 1, 0), "nearby teammate claim stolen");
	}
	else if (!strcmp(argv[1], "boarding"))
	{
		vehicles[1].sought = 1; vehicles[1].unit_index = 2; vehicles[1].sought_tick = now;
		units[2].object.position.x = 2;
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0, NONE, FALSE, &action);
		now += 30;
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0, NONE, FALSE, &action);
		CHECK(action.throttle.i == 0, "driver abandoned boarding teammate");
		now += 360;
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0, NONE, FALSE, &action);
		CHECK(action.throttle.i > 0, "boarding wait never expired");
	}
	else if (!strcmp(argv[1], "turret"))
	{
		((struct bots_vehicle_definition *)vehicle_tag)->type = 6;
		units[1].unit.gunner_object_index = 0; units[1].unit.current_weapon_index = 0;
		units[2].object.bounding_sphere_center.x = 10;
		weapon_tag.weapon.triggers.count = 1; weapon_tag.weapon.triggers.address = &trigger; trigger.charging_time = 1;
		action.primary_trigger = 1;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, 2, FALSE, &action);
		now += 15; action.primary_trigger = 0;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, 2, FALSE, &action);
		CHECK(action.primary_trigger == 1, "charged turret released early");
		now += 25;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, 2, FALSE, &action);
		CHECK(action.primary_trigger == 0, "charged turret never released");
		units[2].object.bounding_sphere_center.x = -10;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, 2, FALSE, &action);
		now += 61;
		bots_vehicle_action(0, 0, &goal, FALSE, FALSE, 0, 2, FALSE, &action);
		CHECK(TEST_FLAG(action.control_flags, _unit_control_action_bit), "turret retained unreachable firing arc");
	}
	else if (!strcmp(argv[1], "flip"))
	{
		units[0].object.parent_object_index = NONE;
		units[1].object.up.k = -1; units[1].object.bounding_sphere_radius = 2;
		vehicles[0].sought = 1; vehicles[0].flipping = TRUE;
		bots_vehicle_action(0, 0, &goal, FALSE, TRUE, 0, NONE, FALSE, &action);
		CHECK(TEST_FLAG(action.control_flags, _unit_control_action_bit), "flip did not use ordinary action");
	}
	else CHECK(0, "unknown case");
	return 0;
}
