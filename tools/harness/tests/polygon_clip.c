/*
POLYGON_CLIP.C (test)

The real convex_hull2d_intersect and convex_polygon2d_clip_to_plane
(source/math/geometry.c), which clip each portal's outline on the screen to
the region seen so far as the camera walks the structure's clusters
(structure_visibility_traverse_cluster). The outlines share corners: a corner
on a clip line, with the corner before it a rounding error behind the line,
made the crossing 0/0 and a NaN corner that every later clip spread, so the
clusters past that portal went undrawn for that frame (the level
"flashing" to the fog colour). test_polygon_clip.py takes the code (under_test.inc).
*/

#include <math.h>
#include "harness.h"

typedef union { real n[2]; struct { real x; real y; }; } real_point2d;
typedef union { real n[2]; struct { real i; real j; }; } real_vector2d;
typedef struct { real_vector2d n; real d; } real_plane2d;
#define csmemcpy memcpy
#define SET_FLAG(f, b, v) ((v) ? ((f)|=(unsigned)FLAG(b)) : ((f)&=(unsigned)~FLAG(b)))
#define CEILING(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
#define PIN(n,floor,ceiling) ((n)<(floor) ? (floor) : CEILING((n),(ceiling)))
#define _real_epsilon 0.0001f

#include "under_test.inc"

/* the region seen so far and a portal's outline from Sali Creek's portal walk (h2d_salicreek, the camera at
-11.2 5.4 0.42 and 9.7 -0.7 0.42 looking down; dumped as hexadecimal floats when the result had a NaN) */
static const float region_a[] = { -0x1.6a897ep-1f, 0x1.2c47d8p-1f, -0x1.55253ap-1f, 0x1.2dfd0ep-1f, -0x1.597254p-1f,
	0x1.437af8p-1f, -0x1.ced1a2p-1f, 0x1.62c958p-1f, -0x1.2fbff6p+0f, 0x1.89606ap-1f, -0x1.292cp+0f, 0x1.627b7ap-1f,
	-0x1.9a09f8p-1f, 0x1.3690b2p-1f, -0x1.934c6ap-1f, 0x1.35198p-1f, -0x1.78621cp-1f, 0x1.2f3f56p-1f, -0x1.6a9f96p-1f,
	0x1.2c461p-1f };
static const float outline_a[] = { -0x1.6a9f96p-1f, 0x1.2c461p-1f, -0x1.6a897ep-1f, 0x1.2c47d8p-1f, -0x1.67e3e8p-1f,
	0x1.2c7dfp-1f, -0x1.1a70f6p-1f, 0x1.32acdcp-1f, -0x1.8cfa9cp+0f, 0x1.a94a44p+2f, -0x1.b4036p+0f, 0x1.82d124p+2f,
	-0x1.c16a16p+0f, 0x1.74a054p+2f, -0x1.e981a8p+0f, 0x1.4a2c0ep+2f, -0x1.2daf58p+0f, 0x1.0c935cp+1f, -0x1.fb47ecp-1f,
	0x1.52faeap+0f, -0x1.faa896p-1f, 0x1.51b248p+0f };
static const float region_b[] = { 0x1.78140cp-6f, 0x1.1a3902p-1f, 0x1.b9aa04p-5f, 0x1.1d71a8p-1f, 0x1.be6c74p-5f,
	0x1.2fa286p-1f, -0x1.75761ep-4f, 0x1.3da414p-1f, -0x1.e8b8dep-3f, 0x1.4bd18cp-1f, -0x1.e13ed6p-3f, 0x1.3148bcp-1f,
	-0x1.35ed52p-5f, 0x1.1ef6b6p-1f, -0x1.e324c2p-6f, 0x1.1e4ec2p-1f, 0x1.36bcbp-8f, 0x1.1b9dcap-1f, 0x1.76117ap-6f,
	0x1.1a35bap-1f };
static const float outline_b[] = { 0x1.76117ap-6f, 0x1.1a35bap-1f, 0x1.78140cp-6f, 0x1.1a3902p-1f, 0x1.b5d80ap-6f,
	0x1.1a9d6ep-1f, 0x1.2914fp-3f, 0x1.26ed62p-1f, 0x1.629324p-2f, 0x1.4b30dep+2f, 0x1.32ce2cp-3f, 0x1.183b74p+2f,
	0x1.64fb4ep-4f, 0x1.07b25cp+2f, -0x1.4fe5c6p-4f, 0x1.b61aaap+1f, -0x1.012dc4p-4f, 0x1.a5bf14p+0f, -0x1.c76704p-5f,
	0x1.1af0d4p+0f, -0x1.c701p-5f, 0x1.1a00aap+0f };

static short points_from(const float *values, short count, real_point2d *points)
{
	short index;

	for (index = 0; index < count; index++)
	{
		points[index].x = values[index * 2];
		points[index].y = values[index * 2 + 1];
	}
	return count;
}

static void bounds_of(const real_point2d *points, short count, real bounds[4])
{
	short index;

	bounds[0] = bounds[2] = 1e30f;
	bounds[1] = bounds[3] = -1e30f;
	for (index = 0; index < count; index++)
	{
		bounds[0] = fminf(bounds[0], points[index].x);
		bounds[1] = fmaxf(bounds[1], points[index].x);
		bounds[2] = fminf(bounds[2], points[index].y);
		bounds[3] = fmaxf(bounds[3], points[index].y);
	}
}

/* the region and the outline intersect to a hull of three or more corners, every one a number and inside both */
static void check_intersection(const float *region_values, short region_count, const float *outline_values,
	short outline_count)
{
	real_point2d region[64], outline[64], result[CLIP_BUFFER_SIZE];
	real region_bounds[4], outline_bounds[4];
	short count, index;

	points_from(region_values, region_count, region);
	points_from(outline_values, outline_count, outline);
	bounds_of(region, region_count, region_bounds);
	bounds_of(outline, outline_count, outline_bounds);
	count = convex_hull2d_intersect(region_count, region, outline_count, outline, 256, result, 0.0001f);
	CHECK(count >= 3, "%d corners", count);
	for (index = 0; index < count; index++)
	{
		CHECK(isfinite(result[index].x) && isfinite(result[index].y), "corner %d of %d is (%g %g)", index, count,
			result[index].x, result[index].y);
		CHECK(result[index].x >= region_bounds[0] - 0.001f && result[index].x <= region_bounds[1] + 0.001f &&
			result[index].y >= region_bounds[2] - 0.001f && result[index].y <= region_bounds[3] + 0.001f &&
			result[index].x >= outline_bounds[0] - 0.001f && result[index].x <= outline_bounds[1] + 0.001f &&
			result[index].y >= outline_bounds[2] - 0.001f && result[index].y <= outline_bounds[3] + 0.001f,
			"corner %d (%g %g) is outside the region or the outline", index, result[index].x, result[index].y);
	}
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* outlines sharing corners with the region: no NaN corner */
	CASE("shared-corners-a")
	{
		check_intersection(region_a, 10, outline_a, 11);
		return 0;
	}
	CASE("shared-corners-b")
	{
		check_intersection(region_b, 10, outline_b, 11);
		return 0;
	}
	/* an ordinary clip still clips: the square 0..2 by the line x = 1 keeps its half x >= 1 */
	CASE("ordinary-clip")
	{
		real_point2d square[4], result[CLIP_BUFFER_SIZE];
		real_plane2d plane;
		short count, index;

		square[0].x = 0.0f; square[0].y = 0.0f;
		square[1].x = 2.0f; square[1].y = 0.0f;
		square[2].x = 2.0f; square[2].y = 2.0f;
		square[3].x = 0.0f; square[3].y = 2.0f;
		plane.n.i = 1.0f;
		plane.n.j = 0.0f;
		plane.d = 1.0f;
		count = convex_polygon2d_clip_to_plane(4, square, &plane, 16, result, NULL, NULL, 0.0001f);
		CHECK(count == 4, "%d corners", count);
		for (index = 0; index < count; index++)
			CHECK(result[index].x >= 1.0f && result[index].x <= 2.0f, "corner %d at x %g", index, result[index].x);
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
