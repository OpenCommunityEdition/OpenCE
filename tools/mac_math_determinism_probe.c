/* Floating-point determinism probe: runs the maths the simulation uses over
   a fixed pseudo-random stream and prints one FNV-1a digest per function,
   so builds on different CPUs and compilers can be compared line by line
   (Mac i386, Mac ppc, Linux x86 clang). The halo_ functions are the vendored
   musl ones (port/include/halo_math.h); the others are the C library's
   functions the game's sources still call, which IEEE 754 makes exact.
   Build: see tools/mac_math_determinism_checks.sh. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include "halo_math.h"

static uint32_t hash_state;
static void hash_u32(uint32_t v)
{
	int i;
	for (i = 0; i < 4; ++i) { hash_state ^= (v >> (8 * i)) & 255u; hash_state *= 16777619u; }
}
static void hash_double(double d)
{
	uint32_t w[2];
	memcpy(w, &d, 8);
	/* both halves, in a fixed order whatever the byte order */
	if (*(const unsigned char *)&(uint32_t){1}) { hash_u32(w[1]); hash_u32(w[0]); }
	else { hash_u32(w[0]); hash_u32(w[1]); }
}
static void hash_float(float f)
{
	uint32_t w;
	memcpy(&w, &f, 4);
	hash_u32(w);
}
static uint32_t rng_state = 12345u;
static uint32_t rng(void)
{
	rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5;
	return rng_state;
}
/* a float in [-range, range) */
static float rnd(float range)
{
	return ((float)(rng() >> 8) / 8388608.0f - 1.0f) * range;
}
static void report(const char *name, long n)
{
	printf("%-12s n=%ld digest=%08lx\n", name, n, (unsigned long)hash_state);
	hash_state = 2166136261u;
}

int main(void)
{
	long i, n = 200000;
	volatile float vf;
	volatile double vd;
	hash_state = 2166136261u;
	printf("FLT_EVAL_METHOD=%d\n", (int)__FLT_EVAL_METHOD__);
	for (i = 0; i < n; ++i) { float x = rnd(100.0f); hash_double(halo_sin(x)); } report("sin", n);
	for (i = 0; i < n; ++i) { float x = rnd(100.0f); hash_double(halo_cos(x)); } report("cos", n);
	for (i = 0; i < n; ++i) { float x = rnd(1.5f); hash_double(halo_tan(x)); } report("tan", n);
	for (i = 0; i < n; ++i) { float x = rnd(1.0f); hash_double(halo_asin(x)); hash_double(halo_acos(x)); } report("asin/acos", n);
	for (i = 0; i < n; ++i) { float x = rnd(50.0f); hash_double(halo_atan(x)); } report("atan", n);
	for (i = 0; i < n; ++i) { float y = rnd(50.0f), x = rnd(50.0f); hash_double(halo_atan2(y, x)); } report("atan2", n);
	for (i = 0; i < n; ++i) { float x = rnd(20.0f); hash_double(halo_exp(x)); } report("exp", n);
	for (i = 0; i < n; ++i) { float x = rnd(1000.0f) + 1000.0f; hash_double(halo_log(x)); hash_double(halo_log10(x)); hash_double(halo_log2(x)); } report("log*", n);
	for (i = 0; i < n; ++i) { float x = rnd(10.0f) + 10.0f, y = rnd(4.0f); hash_double(halo_pow(x, y)); } report("pow", n);
	/* exact in IEEE arithmetic */
	for (i = 0; i < n; ++i) { float x = rnd(1000.0f) + 1000.0f; hash_double(sqrt((double)x)); hash_float((float)sqrt((double)x)); } report("sqrt", n);
	for (i = 0; i < n; ++i) { float x = rnd(1000.0f) + 1000.0f; hash_float(sqrtf(x)); } report("sqrtf", n);
	for (i = 0; i < n; ++i) { float x = rnd(1000.0f); hash_float((float)floor(x)); hash_float((float)ceil(x)); hash_float((float)fmod(x, 3.7)); } report("floor/ceil/fmod", n);
	/* float arithmetic: single rounding of each operation, no FMA, no extra precision */
	{
		float a = 1.0f, b = 0.5f, c = 0.25f;
		for (i = 0; i < n * 5; ++i) {
			float x = rnd(10.0f), y = rnd(10.0f), z = rnd(10.0f);
			a = a * 0.999f + x * y - z * 0.001f;
			b = (a + x) * (b - y) / (1.0f + z * z);
			c = c * x + y * z + a * b * 1e-6f;
			if (a > 1e6f || a < -1e6f) a = 1.0f;
			if (b > 1e6f || b < -1e6f) b = 0.5f;
			if (c > 1e6f || c < -1e6f) c = 0.25f;
			hash_float(a); hash_float(b); hash_float(c);
		}
		report("float-arith", n * 5);
	}
	/* mixed float/double arithmetic and conversions */
	for (i = 0; i < n; ++i) {
		float x = rnd(1000.0f), y = rnd(1000.0f);
		double d = (double)x * y + x / (y + 1.0001);
		hash_double(d); hash_float((float)d);
		hash_u32((uint32_t)(int32_t)x); hash_u32((uint32_t)(int32_t)(float)d);
	}
	report("convert", n);
	/* denormals: x * tiny, tiny / 2 */
	{
		float t = 1.17549435e-38f;
		for (i = 0; i < 2000; ++i) {
			vf = t * (0.25f + (float)(rng() & 255) / 1024.0f); hash_float(vf);
			vf = vf / 3.0f; hash_float(vf);
			vd = 2.2250738585072014e-308 * (0.25 + (double)(rng() & 255) / 1024.0); hash_double(vd);
			vd = vd / 3.0; hash_double(vd);
		}
		report("denormal", 2000);
	}
	/* out-of-range float -> int (differs between x86 and PowerPC; not relied on) */
	{
		volatile float big[4];
		big[0] = 3e9f; big[1] = -3e9f; big[2] = 2147483648.0f; big[3] = 1e30f;
		for (i = 0; i < 4; ++i) printf("float->int %g = %ld (informational)\n", big[i], (long)(int32_t)big[i]);
	}
	return 0;
}
