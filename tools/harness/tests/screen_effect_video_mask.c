/*
SCREEN_EFFECT_VIDEO_MASK.C (test)

The real rasterizer_screen_effect_set_texture_transforms
(rasterizer_xbox_screen_effect.c): the video effect's mask (rasterizer\video
mask, 1024x512, its picture in the middle 640 columns and rows 16 to 495,
black past them) is laid with its middle 640 columns across the whole width
of the screen, as the Xbox's 640 columns showed it, its rows one a line; and
the other effects' textures are laid as before. Laid a texel to a column on a
wider screen, the recording (343 Guilty Spark's) covered only the middle 640
columns, black past them. test_screen_effect_video_mask.py takes the
structures, the enums and the code (config.inc, under_test.inc).
*/

#include <math.h>
#include "harness.h"

typedef struct { real i, j; } real_vector2d;
typedef struct { short y0, x0, y1, x1; } rectangle2d;
typedef union { struct { real red, green, blue; }; real n[3]; } real_rgb_color;
union point2d { struct { short x, y; }; short n[2]; };
#define BITMAP_GROUP_TAG 0x6269746DUL
#define MILLISECONDS_PER_SECOND 1000
#define TICKS_PER_SECOND 30

#include "config.inc"

static void *global_d3d_device;
static struct { struct { rectangle2d viewport_bounds; rectangle2d window_bounds; } camera; } global_window_parameters;
static short main_get_window_count(void) { return 1; }
static unsigned long system_milliseconds(void) { return 0; }
static real real_seed_random(unsigned long *seed) { return 0.5f; }

/* the constants the vertex shader is given: two rows a stage, each a
texture coordinate's dot product with the vertex's (x, y, 0, 1) */
static real given[8][4];
static long IDirect3DDevice8_SetVertexShaderConstant(void *device, long reg, void const *constants, long count)
{
	memcpy(given, constants, sizeof(given));
	return 0;
}

#include "under_test.inc"

/* the mask's texel (from its left or top edge) at a point of the screen */
static real mask_column(real x, real y, real width) { return (given[2][0] * x + given[2][1] * y + given[2][3]) * width; }
static real mask_row(real x, real y, real height) { return (given[3][0] * x + given[3][1] * y + given[3][3]) * height; }

static boolean near(real a, real b) { return fabsf(a - b) < 0.01f; }

static void screen(short width)
{
	global_window_parameters.camera.viewport_bounds.x0 = 0;
	global_window_parameters.camera.viewport_bounds.y0 = 0;
	global_window_parameters.camera.viewport_bounds.x1 = width;
	global_window_parameters.camera.viewport_bounds.y1 = 480;
	global_window_parameters.camera.window_bounds = global_window_parameters.camera.viewport_bounds;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	static struct bitmap_data mask, noise;
	struct rasterizer_cinematic_screen_effect_parameters video;

	mask.width = 1024;
	mask.height = 512;
	noise.width = 256;
	noise.height = 256;
	memset(&video, 0, sizeof(video));
	video.video_on = TRUE;
	video.video_scanline_map = &mask;
	video.video_noise_map = &noise;
	video.video_noise_intensity = 0.5f;

	/* 16:9 (854 columns): the mask's picture, columns 192 to 831, from the
	screen's left edge to its right, the rows as on the Xbox */
	CASE("fills-a-wide-screen")
	{
		screen(854);
		rasterizer_screen_effect_set_texture_transforms(&video, 1, 2);
		CHECK(near(mask_column(0.0f, 0.0f, 1024.0f), 192.5f),
			"the screen's left edge is the mask's column %g, not its picture's first (192.5)",
			mask_column(0.0f, 0.0f, 1024.0f));
		CHECK(near(mask_column(854.0f, 0.0f, 1024.0f), 832.5f),
			"the screen's right edge is the mask's column %g, not its picture's last (832.5)",
			mask_column(854.0f, 0.0f, 1024.0f));
		CHECK(near(mask_row(0.0f, 0.0f, 512.0f), 16.5f) && near(mask_row(0.0f, 480.0f, 512.0f), 496.5f),
			"the screen's rows are the mask's %g to %g, not 16.5 to 496.5, a texel a line",
			mask_row(0.0f, 0.0f, 512.0f), mask_row(0.0f, 480.0f, 512.0f));
		return 0;
	}
	/* the Xbox's 640 columns: as the Xbox laid it, a texel a column */
	CASE("as-the-xbox-at-640")
	{
		screen(640);
		rasterizer_screen_effect_set_texture_transforms(&video, 1, 2);
		CHECK(near(mask_column(0.0f, 0.0f, 1024.0f), 192.5f) && near(mask_column(640.0f, 0.0f, 1024.0f), 832.5f),
			"640 columns are the mask's %g to %g, not 192.5 to 832.5", mask_column(0.0f, 0.0f, 1024.0f),
			mask_column(640.0f, 0.0f, 1024.0f));
		return 0;
	}
	/* not the video: the stage's texture is the screen, laid a pixel a
	column, as before */
	CASE("other-effects-as-before")
	{
		screen(854);
		video.video_on = FALSE;
		rasterizer_screen_effect_set_texture_transforms(&video, 1, 2);
		CHECK(near(given[2][0], 1.0f) && near(given[2][3], 0.5f),
			"another effect's second stage is laid %g a column from %g, not 1 from 0.5", given[2][0], given[2][3]);
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
