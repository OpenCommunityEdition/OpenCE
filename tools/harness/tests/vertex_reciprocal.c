/*
VERTEX_RECIPROCAL.C (test)

The NV2A vertex programs' rcp and rsq as the port writes them in GLSL
(nv2a_vsh.c's nv2a_rcp and nv2a_rsq), compiled here as C: zero times either
of them is zero, as on the NV2A, whose multiply takes zero times its
infinite rcp and rsq of zero as zero, where GLSL's infinity made it not a
number. The bumped lightmaps' program scales the incident radiosity by the
rsq of its own length squared, and a10 has 24628 lightmap vertices of none:
each was not a number, and so were the triangles it is in.
test_vertex_reciprocal.py takes the GLSL from the sources (helpers.inc).
*/

#include "harness.h"

#include <math.h>

/* GLSL as C: each helper's vec4 is its one value (it returns all four the
same) */
typedef float vec4;
#undef abs
#undef min
#undef clamp
#define vec4(x) ((float)(x))
#define abs(x) fabsf(x)
#define min(a, b) fminf((a), (b))
#define clamp(x, low, high) fminf(fmaxf((x), (low)), (high))
#define inversesqrt(x) (1.0f / sqrtf(x))

#include "helpers.inc"

#define LARGEST 1.884467e+19f

static int near(float a, float b)
{
	return fabsf(a - b) <= 1e-6f * fmaxf(1.0f, fabsf(b));
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* zero: finite, and zero times it is zero */
	CASE("zero")
	{
		CHECK(isfinite(nv2a_rsq(0.0f)) && isfinite(nv2a_rcp(0.0f)), "rsq(0) %g, rcp(0) %g", nv2a_rsq(0.0f), nv2a_rcp(0.0f));
		CHECK(nv2a_rsq(0.0f) * 0.0f == 0.0f, "rsq(0) times 0 is %g", nv2a_rsq(0.0f) * 0.0f);
		CHECK(nv2a_rcp(0.0f) * 0.0f == 0.0f, "rcp(0) times 0 is %g", nv2a_rcp(0.0f) * 0.0f);
		CHECK(nv2a_rsq(-0.0f) * 0.0f == 0.0f && nv2a_rcp(-0.0f) * 0.0f == 0.0f, "negative zero gave not zero");
		return 0;
	}
	/* the bumped lightmaps' program (vertex shader 16): a vertex's incident
	radiosity, scaled by the rsq of its length squared */
	CASE("incident radiosity")
	{
		float incident[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.5f }, { 0.6f, 0.0f, 0.8f } };
		int vertex, axis;

		for (vertex = 0; vertex < 3; vertex++)
		{
			float *v = incident[vertex];
			float length_squared = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
			float scale = nv2a_rsq(length_squared);

			for (axis = 0; axis < 3; axis++)
			{
				float out = scale * v[axis];

				CHECK(!isnan(out), "vertex %d axis %d is not a number", vertex, axis);
				CHECK(length_squared > 0.0f ? near(out, v[axis] / sqrtf(length_squared)) : out == 0.0f,
					"vertex %d axis %d is %g", vertex, axis, out);
			}
		}
		return 0;
	}
	/* elsewhere: the reciprocal (of the root), one exactly, and no more than
	rcc's largest */
	CASE("values")
	{
		CHECK(near(nv2a_rsq(4.0f), 0.5f) && near(nv2a_rsq(-4.0f), 0.5f), "rsq(4) %g, rsq(-4) %g", nv2a_rsq(4.0f), nv2a_rsq(-4.0f));
		CHECK(near(nv2a_rcp(2.0f), 0.5f) && near(nv2a_rcp(-4.0f), -0.25f), "rcp(2) %g, rcp(-4) %g", nv2a_rcp(2.0f), nv2a_rcp(-4.0f));
		CHECK(nv2a_rsq(1.0f) == 1.0f && nv2a_rcp(1.0f) == 1.0f && nv2a_rsq(-1.0f) == 1.0f, "one gave other than one");
		CHECK(nv2a_rcp(1e-30f) == LARGEST && nv2a_rcp(-1e-30f) == -LARGEST, "rcp(1e-30) %g", nv2a_rcp(1e-30f));
		CHECK(nv2a_rsq(1e-40f) == LARGEST, "rsq(1e-40) %g", nv2a_rsq(1e-40f));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
