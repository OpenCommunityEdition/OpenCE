#include "harness.h"
#include <math.h>

#include "config.inc"
typedef struct { real i, j; } real_vector2d;
typedef struct { real x0, x1, y0, y1; } real_rectangle2d;
struct unit_animation_update_data { short state_desired; };
struct biped_definition
{
	struct { unsigned long flags; } unit;
	struct
	{
		unsigned long flags;
		real moving_turning_speed, runtime_cosine_stationary_turning_threshold;
		real flying_pitch_amount, flying_bank_amount, flying_bank_apply_time, flying_bank_decay_time;
		real flying_angular_velocity, flying_angular_acceleration;
	} biped;
};
struct biped_datum
{
	long definition_index;
	struct { unsigned long damage_flags; real_vector3d forward, up, translational_velocity, angular_velocity; } object;
	struct
	{
		long player_index;
		unsigned long flags, control_flags;
		real_vector3d desired_facing_vector, throttle;
		struct { short base_seat_index, state; } animation;
	} unit;
	struct { short state; real bank; } biped;
};
static struct biped_definition definition;
static struct biped_datum biped;
static boolean enhanced;
static const real_vector3d *global_up3d = &(real_vector3d){ 0.f, 0.f, 1.f };
static struct biped_datum *biped_get(long index) { CHECK(index == 3, "wrong biped"); return &biped; }
static struct biped_definition *biped_definition_get(long index) { return &definition; }
static short connection = _game_connection_local;
static boolean unit_animation_enhanced(long index) { return enhanced; }
static boolean unit_animation_enhanced_unit(long index) { return TRUE; }
static short game_connection(void) { return connection; }
#include "turns_with_aim.inc"
static void biped_verify_object_vectors(long index, const char *where) { }
static void biped_snap_facing(long index) { CHECK(0, "snapped as a flying biped"); }
static real dot_product3d(const real_vector3d *a, const real_vector3d *b) { return a->i*b->i + a->j*b->j + a->k*b->k; }
static real magnitude_squared3d(const real_vector3d *a) { return dot_product3d(a, a); }
static real dot_product2d(const real_vector2d *a, const real_vector2d *b) { return a->i*b->i + a->j*b->j; }
static real cross_product2d(const real_vector2d *a, const real_vector2d *b) { return a->i*b->j - a->j*b->i; }
static real normalize2d(real_vector2d *v)
{
	real length = sqrtf(v->i*v->i + v->j*v->j);

	if (length != 0.f)
	{
		v->i /= length;
		v->j /= length;
	}
	return length;
}
static real normalize3d(real_vector3d *v)
{
	real length = sqrtf(magnitude_squared3d(v));

	if (length != 0.f)
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
	}
	return length;
}
static void rotate_vector2d(const real_vector2d *v, real sine, real cosine, real_vector2d *result)
{
	real_vector2d rotated = { v->i*cosine - v->j*sine, v->i*sine + v->j*cosine };

	*result = rotated;
}
#define cross_product3d(a, b, result) CHECK(0, "climbing or flying turn")
#define rotate_vector_about_axis(vector, axis, sine, cosine) CHECK(0, "climbing turn")
static void unit_euler_aiming_update(void *matrix, real_vector3d *vector, const real_vector3d *desired,
	real_vector3d *velocity, const real_rectangle2d *bounds, real velocity_limit, real acceleration_limit)
{
	CHECK(0, "flying turn");
}
#include "under_test.inc"

/* the body's yaw, in degrees */
static real yaw(void)
{
	return atan2f(biped.object.forward.j, biped.object.forward.i) * 180.f / _pi;
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];
	struct unit_animation_update_data animation;
	int tick;

	/* a player in the air facing where the jump began, the aim turned 150 degrees left */
	definition.biped.moving_turning_speed = 2.f * _pi;
	definition.biped.runtime_cosine_stationary_turning_threshold = 0.9f;
	biped.definition_index = 1;
	biped.object.forward = (real_vector3d){ 1.f, 0.f, 0.f };
	biped.object.up = *global_up3d;
	biped.unit.player_index = 0;
	biped.unit.desired_facing_vector = (real_vector3d){ cosf(150.f * _pi / 180.f), sinf(150.f * _pi / 180.f), 0.f };
	biped.unit.animation.base_seat_index = _unit_base_seat_asleep + 1;
	biped.unit.animation.state = _unit_state_airborne;
	biped.biped.state = biped_state_unknown;
	enhanced = TRUE;
	CASE("turns")
	{
		/* the moving state's rate: 12 degrees a tick, there in 13 ticks */
		biped_update_turning(3, &animation);
		CHECK(fabsf(yaw() - 12.f) < 0.01f, "turned to %f after a tick", yaw());
		for (tick = 1; tick < 13; tick++)
			biped_update_turning(3, &animation);
		CHECK(fabsf(yaw() - 150.f) < 0.01f, "at %f, not the aim's 150", yaw());
		biped_update_turning(3, &animation);
		CHECK(fabsf(yaw() - 150.f) < 0.01f, "past the aim, at %f", yaw());
		CHECK(biped.object.forward.k == 0.f, "body pitched");
	}
	CASE("off")
	{
		/* game.enhanced_animations off: the original's body, left where the jump began */
		enhanced = FALSE;
		for (tick = 0; tick < 20; tick++)
			biped_update_turning(3, &animation);
		CHECK(yaw() == 0.f, "turned to %f", yaw());
	}
	CASE("network")
	{
		/* a network game: the turn is every machine's, whatever its own setting */
		enhanced = FALSE;
		connection = _game_connection_network_client;
		for (tick = 0; tick < 13; tick++)
			biped_update_turning(3, &animation);
		CHECK(fabsf(yaw() - 150.f) < 0.01f, "a client with the setting off at %f, not the aim's 150", yaw());
	}
	CASE("actor")
	{
		/* not a player's: unchanged */
		biped.unit.player_index = NONE;
		for (tick = 0; tick < 20; tick++)
			biped_update_turning(3, &animation);
		CHECK(yaw() == 0.f, "turned to %f", yaw());
	}
	CASE("melee")
	{
		/* an airborne melee keeps the original's facing */
		biped.unit.animation.state = _unit_state_melee_airborne;
		for (tick = 0; tick < 20; tick++)
			biped_update_turning(3, &animation);
		CHECK(yaw() == 0.f, "turned to %f", yaw());
	}
	CASE("look")
	{
		/* look without turning, as when running */
		biped.unit.control_flags = FLAG(_unit_control_look_dont_turn_bit);
		for (tick = 0; tick < 20; tick++)
			biped_update_turning(3, &animation);
		CHECK(yaw() == 0.f, "turned to %f", yaw());
	}
	return 0;
}
