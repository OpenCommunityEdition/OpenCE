/*
BINK_DRAW_FRAME.C (test)

The real bink_draw_frame (source/bink/bink_playback.c), without its
debug_bink statistics: where a 640x480 movie is drawn on the screen. As the
Xbox, at its own size in the middle; with the credits' flag
(display.widescreen_credits), its 16:9 picture as large as the screen holds
it, the movie's black bars past the edges of a 16:9 screen, as before on a
4:3 one. test_bink_draw_frame.py takes the code and the flags from the
sources.
*/

#include "harness.h"

#include "config.inc"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef unsigned long pixel32;
typedef struct { short y0, x0, y1, x1; } rectangle2d;
struct dynamic_screen_vertex
{
	struct { real x, y; } position;
	struct { real x, y; } texture_coordinates;
	pixel32 color;
};
struct rasterizer_dynamic_screen_geometry_parameters { int unused; };

static struct { struct { rectangle2d screen_bounds; } reserved04; } rasterizer_globals;
static struct
{
	unsigned long flags;
	short width, height;
	struct rasterizer_dynamic_screen_geometry_parameters screen_geometry;
} bink_globals;

static struct dynamic_screen_vertex drawn[4];

static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *geometry,
	struct dynamic_screen_vertex *vertices)
{
	(void)geometry;
	memcpy(drawn, vertices, sizeof(drawn));
}

#include "under_test.inc"

/* draws the movie on a screen of that size: its corners' smallest and
largest x and y */
static void draw(short screen_width, boolean widescreen_picture, real *bounds)
{
	int index;

	rasterizer_globals.reserved04.screen_bounds.x0 = 0;
	rasterizer_globals.reserved04.screen_bounds.y0 = 0;
	rasterizer_globals.reserved04.screen_bounds.x1 = screen_width;
	rasterizer_globals.reserved04.screen_bounds.y1 = 480;
	bink_globals.width = 640;
	bink_globals.height = 480;
	bink_globals.flags = 0x2e | (widescreen_picture ? FLAG(_bink_playback_widescreen_picture_bit) : 0);
	bink_draw_frame();
	bounds[0] = bounds[2] = 1e9f;
	bounds[1] = bounds[3] = -1e9f;
	for (index = 0; index < 4; index++)
	{
		CHECK(drawn[index].texture_coordinates.x == 0.0f || drawn[index].texture_coordinates.x == 640.0f,
			"texture x %g", drawn[index].texture_coordinates.x);
		CHECK(drawn[index].texture_coordinates.y == 0.0f || drawn[index].texture_coordinates.y == 480.0f,
			"texture y %g", drawn[index].texture_coordinates.y);
		bounds[0] = MIN(bounds[0], drawn[index].position.x);
		bounds[1] = drawn[index].position.x > bounds[1] ? drawn[index].position.x : bounds[1];
		bounds[2] = MIN(bounds[2], drawn[index].position.y);
		bounds[3] = drawn[index].position.y > bounds[3] ? drawn[index].position.y : bounds[3];
	}
}

static void expect(short screen_width, boolean widescreen_picture, real x0, real x1, real y0, real y1)
{
	real bounds[4];

	draw(screen_width, widescreen_picture, bounds);
	CHECK(bounds[0] == x0 && bounds[1] == x1 && bounds[2] == y0 && bounds[3] == y1,
		"a %d-wide screen%s: x %g to %g, y %g to %g, not x %g to %g, y %g to %g", screen_width,
		widescreen_picture ? ", the credits' flag" : "", bounds[0], bounds[1], bounds[2], bounds[3], x0, x1, y0, y1);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("4:3")
	{
		expect(640, FALSE, 0, 640, 0, 480);
		expect(640, TRUE, 0, 640, 0, 480);
		return 0;
	}
	CASE("16:9")
	{
		/* (its 16:9 picture, rows 60 to 420, fills the screen) */
		expect(854, FALSE, 107, 747, 0, 480);
		expect(854, TRUE, 0, 853, -80, 560);
		return 0;
	}
	CASE("21:9")
	{
		/* (the picture as tall as the screen, in the middle) */
		expect(1120, FALSE, 240, 880, 0, 480);
		expect(1120, TRUE, 133, 986, -80, 560);
		return 0;
	}
	fprintf(stderr, "no case named %s\n", case_name);
	return 2;
}
