/*
HUD_LINE_ART.C (test)

The real upscale_pixels, upscale_blur_pass and upscale_sharpen_alpha
(hud_hires.c): the message icons' sheets (the weapons the pickup prompt
shows, the buttons and sticks) have no redraw, and were drawn from the map's
texels by interpolation alone, 4 times their size: their outlines a texel
or two wide were soft at a screen of 1080 lines and more. They are drawn 8
times their size, their coverage sharpened by an unsharp mask: an edge
steeper, a flat part as it was.
test_hud_line_art.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include "config.inc"

#include "under_test.inc"

#define SIDE 16

/* a BGRA sheet: white, its alpha an outline a texel wide (column 5), a
flat tinted fill (columns 9 to 14, alpha 90) and nothing else */
static unsigned char sheet[SIDE * SIDE * 4];

static void sheet_reset(void)
{
	long x, y;

	for (y = 0; y < SIDE; y++)
	{
		for (x = 0; x < SIDE; x++)
		{
			unsigned char *texel = sheet + (y * SIDE + x) * 4;

			texel[0] = texel[1] = texel[2] = 255;
			texel[3] = x == 5 ? 255 : x >= 9 && x <= 14 ? 90 : 0;
		}
	}
}

/* across the outline, the upscaled texels' width where its alpha is over
a quarter and under three quarters: its soft edge, in texels */
static long soft_width(unsigned char const *pixels, long factor)
{
	long width = SIDE * factor, y = SIDE * factor / 2, x, soft = 0;

	for (x = 2 * factor; x < 8 * factor; x++)
	{
		unsigned char alpha = pixels[(y * width + x) * 4 + 3];

		soft += alpha > 64 && alpha < 192;
	}
	return soft;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	sheet_reset();

	/* 8 times the sheet's size, its alignment kept: the outline's texel
	centred where it was */
	CASE("size")
	{
		unsigned char *pixels = upscale_pixels(sheet, SIDE, SIDE, 256, UPSCALE_LINE_ART_FACTOR);
		long width = SIDE * UPSCALE_LINE_ART_FACTOR, y = width / 2, peak = 0, x;

		CHECK(pixels != NULL, "not upscaled");
		for (x = 1; x < width; x++)
			if (pixels[(y * width + x) * 4 + 3] > pixels[(y * width + peak) * 4 + 3])
				peak = x;
		CHECK(peak == 5 * UPSCALE_LINE_ART_FACTOR + UPSCALE_LINE_ART_FACTOR / 2 ||
			peak == 5 * UPSCALE_LINE_ART_FACTOR + UPSCALE_LINE_ART_FACTOR / 2 - 1, "the outline at %ld", peak);
		free(pixels);
		return 0;
	}
	/* sharpened: the outline's edges steeper than interpolation leaves
	them */
	CASE("steeper")
	{
		unsigned char *soft = upscale_pixels(sheet, SIDE, SIDE, 256, UPSCALE_LINE_ART_FACTOR);
		unsigned char *sharp = upscale_pixels(sheet, SIDE, SIDE, 256, UPSCALE_LINE_ART_FACTOR);
		long before, after;

		CHECK(soft && sharp, "not upscaled");
		upscale_sharpen_alpha(sharp, SIDE * UPSCALE_LINE_ART_FACTOR, SIDE * UPSCALE_LINE_ART_FACTOR);
		before = soft_width(soft, UPSCALE_LINE_ART_FACTOR);
		after = soft_width(sharp, UPSCALE_LINE_ART_FACTOR);
		CHECK(after * 3 <= before * 2, "edges %ld texels soft, %ld before", after, before);
		free(soft);
		free(sharp);
		return 0;
	}
	/* the flat fill's alpha as it was, inside it, and nothing where the
	sheet had nothing */
	CASE("flat kept")
	{
		unsigned char *sharp = upscale_pixels(sheet, SIDE, SIDE, 256, UPSCALE_LINE_ART_FACTOR);
		long width = SIDE * UPSCALE_LINE_ART_FACTOR, y = width / 2;
		unsigned char inside, outside;

		CHECK(sharp != NULL, "not upscaled");
		upscale_sharpen_alpha(sharp, width, width);
		inside = sharp[(y * width + 12 * UPSCALE_LINE_ART_FACTOR) * 4 + 3];
		outside = sharp[(y * width + 1 * UPSCALE_LINE_ART_FACTOR) * 4 + 3];
		CHECK(inside >= 88 && inside <= 92, "the fill's alpha %d", inside);
		CHECK(outside == 0, "alpha %d where there was none", outside);
		CHECK(sharp[(y * width + 12 * UPSCALE_LINE_ART_FACTOR) * 4] == 255, "colour changed");
		free(sharp);
		return 0;
	}

	return 2;
}
