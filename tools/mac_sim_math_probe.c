/* Simulation maths determinism probe: runs the game's own math units
   (source/math: real_math.c, matrix_math.c, random_math.c,
   periodic_functions.c) over a fixed pseudo-random stream and prints one
   FNV-1a digest of the result bits per group, so that the Mac i386, the Mac
   ppc (under Rosetta) and the Linux x86 builds can be compared line by line.
   See tools/mac_math_determinism_checks.sh. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "cseries.h"
#include "real_math.h"
#include "periodic_functions.h"

static uint32_t hash_state = 2166136261u;
/* a digest per source line of a HASH(), to find which call differs */
static uint32_t line_hash[1024];
/* the bit patterns of 32-bit words, fed least significant byte first, so
   that big- and little-endian builds hash the same values the same way */
static void hash_words_line(const void *data, unsigned long size, int line)
{
	const uint32_t *p = (const uint32_t *)data;
	unsigned long i;
	int shift;
	uint32_t *h = &line_hash[line & 1023];
	if (!*h) *h = 2166136261u;
	for (i = 0; i < size / 4; ++i)
		for (shift = 0; shift < 32; shift += 8) { *h ^= (p[i] >> shift) & 255u; *h *= 16777619u; }
}
static void hash_words(const void *data, unsigned long size)
{
	const uint32_t *p = (const uint32_t *)data;
	unsigned long i;
	int shift;
	for (i = 0; i < size / 4; ++i)
		for (shift = 0; shift < 32; shift += 8) { hash_state ^= (p[i] >> shift) & 255u; hash_state *= 16777619u; }
}
#define HASH(x) (hash_words(&(x), sizeof(x)), hash_words_line(&(x), sizeof(x), __LINE__))
static void hash_value(unsigned long value)
{
	uint32_t word = (uint32_t)value;
	hash_words(&word, 4);
}
static uint32_t rng_state = 987654321u;
static uint32_t rng(void)
{
	rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5;
	return rng_state;
}
static float rnd(float range)
{
	return ((float)(rng() >> 8) / 8388608.0f - 1.0f) * range;
}
static void report(const char *name, long n)
{
	printf("%-24s n=%ld digest=%08lx\n", name, n, (unsigned long)hash_state);
	hash_state = 2166136261u;
}
static void random_vector(real_vector3d *v, float range)
{
	v->i = rnd(range); v->j = rnd(range); v->k = rnd(range);
}
static void random_unit(real_vector3d *v)
{
	do { random_vector(v, 1.0f); } while (dot_product3d(v, v) < 0.01f);
	normalize3d(v);
}

int main(void)
{
	long i, n = 20000;
	real_math_initialize();
	random_math_initialize();
	real_math_reset_precision();

	for (i = 0; i < n; ++i) {
		real_vector3d a, b, r, c;
		real m, s, co;
		random_vector(&a, 100.0f); random_vector(&b, 100.0f);
		m = normalize3d(&a); HASH(m); HASH(a);
		cross_product3d(&a, &b, &r); HASH(r);
		m = dot_product3d(&a, &b); HASH(m);
		m = magnitude3d(&b); HASH(m);
		random_unit(&c); random_unit(&r);
		normals_interpolate(&c, &r, rnd(1.0f) * 0.5f + 0.5f, &a); HASH(a);
		fast_normals_interpolate(&c, &r, 0.3f, &a); HASH(a);
		m = angle_between_vectors3d(&c, &r); HASH(m);
		m = angle_between_normals3d(&c, &r); HASH(m);
		s = rnd(10.0f); co = rnd(10.0f); s = sine(s); co = cosine(co); HASH(s); HASH(co);
		rotate_vector_about_axis(&b, &c, sine(0.7f), cosine(0.7f)); HASH(b);
		perpendicular3d(&c, &r); HASH(r);
		s = rnd(10.0f); co = rnd(10.0f); m = arctangent(s, co); HASH(m);
		s = rnd(6.0f); co = rnd(6.0f); m = signed_angular_difference(s, co); HASH(m);
	}
	report("vector3d", n);

	for (i = 0; i < n; ++i) {
		real_quaternion q0, q1, q2;
		real_matrix4x3 m, inv, mul;
		real_point3d p, pr;
		real_euler_angles3d angles;
		real_vector3d axis;
		random_unit(&axis);
		quaternion_from_angle_and_vector(&q0, rnd(3.0f), &axis);
		random_unit(&axis);
		quaternion_from_angle_and_vector(&q1, rnd(3.0f), &axis);
		quaternions_multiply(&q0, &q1, &q2); HASH(q2);
		quaternion_normalize(&q2); HASH(q2);
		quaternions_interpolate_and_normalize(&q0, &q1, 0.37f, &q2); HASH(q2);
		p.x = rnd(500.0f); p.y = rnd(500.0f); p.z = rnd(50.0f);
		matrix4x3_from_point_and_quaternion(&m, &p, &q0); HASH(m);
		matrix4x3_inverse(&m, &inv); HASH(inv);
		matrix4x3_multiply(&m, &inv, &mul); HASH(mul);
		matrix4x3_transform_point(&m, &p, &pr); HASH(pr);
		matrix4x3_inverse_transform_point(&m, &p, &pr); HASH(pr);
		matrix4x3_rotation_to_quaternion(&m, &q2); HASH(q2);
		matrix4x3_rotation_to_angles(&m, &angles); HASH(angles);
		angles.yaw = rnd(3.0f); angles.pitch = rnd(1.5f); angles.roll = rnd(3.0f);
		matrix4x3_rotation_from_angles(&m, angles.yaw, angles.pitch, angles.roll); HASH(m);
		random_unit(&axis);
		matrix4x3_rotation_from_axis_and_angle(&m, &axis, sine(1.1f), cosine(1.1f)); HASH(m);
	}
	report("quaternion/matrix", n);

	for (i = 0; i < n; ++i) {
		real position = rnd(10.0f), velocity = rnd(5.0f), result;
		real_vector3d v, t, orient_position, orient_target, angular;
		real_point3d pos, target;
		accelerate_to_position(&position, &velocity, rnd(10.0f), 3.0f, 0.5f, -10.0f, 10.0f, 0); HASH(position); HASH(velocity);
		accelerate_to_velocity(&position, &velocity, 0.25f, rnd(4.0f), -10.0f, 10.0f, 1); HASH(position); HASH(velocity);
		random_vector(&v, 5.0f); random_vector(&t, 5.0f);
		accelerate_to_velocity3d(&v, &t, 0.3f); HASH(v);
		pos.x = rnd(5.0f); pos.y = rnd(5.0f); pos.z = rnd(5.0f);
		target.x = rnd(5.0f); target.y = rnd(5.0f); target.z = rnd(5.0f);
		accerate_to_position3d(&pos, &v, &target, 0, 2.0f); HASH(pos); HASH(v);
		random_unit(&orient_position); random_unit(&orient_target); random_vector(&angular, 1.0f);
		angular_accelerate_to_position(&orient_position, &orient_target, &angular, 1.5f, 0.2f); HASH(orient_position); HASH(angular);
		result = point_to_line_distance_squared3d(&pos, &target, &t); HASH(result);
	}
	report("accelerate", n);

	{
		unsigned long seed = 4242u;
		for (i = 0; i < n * 5; ++i) {
			real_vector3d d, axis, cone;
			unsigned short u = seed_random(&seed); short r = seed_random_range(&seed, -50, 50);
			real f = real_seed_random(&seed), g = real_seed_random_range(&seed, -3.0f, 7.0f);
			hash_value(u); hash_value((unsigned short)r); HASH(f); HASH(g);
			seed_random_direction3d(&seed, &d); HASH(d);
			random_unit(&axis);
			seed_random_vector_in_cone3d(&seed, &axis, 0.1f, 0.4f, &cone); HASH(cone);
		}
		hash_value(seed);
		report("random", n * 5);
	}

	for (i = 0; i < n; ++i) {
		real x = (real)(rng() & 1023) / 1023.0f, v;
		short function;
		for (function = 0; function < 12 /* (periodic_functions.c: _periodic_function_one to _spark) */; ++function) {
			v = periodic_function_evaluate(function, x); HASH(v);
		}
		for (function = 0; function < NUMBER_OF_TRANSITION_FUNCTIONS; ++function) {
			v = transition_function_evaluate(function, x); HASH(v);
		}
	}
	report("periodic", n * 20);
	if (getenv("MAC_SIM_MATH_LINES")) {
		for (i = 0; i < 1024; ++i) if (line_hash[i]) printf("line %ld %08lx\n", i, (unsigned long)line_hash[i]);
	}
	return 0;
}
