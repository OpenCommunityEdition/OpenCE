/*
MARINE_FINGERS.C (test)

The real clearing of the armored marines' reflection mask under their
fingertips (marine_fingers.c): the texels a triangle samples (any texel it
covers, and the neighbours that filtering reads, the edges wrapping), the
DXT1 blocks of a level whose texels only the hands' triangles sample, a
shader's texture animation that leaves its maps where they are (the
marines' arms' turn a whole turn and move a whole texture), and a block's
red cleared: every texel's where no other triangle samples it, keeping its
green, blue and opacity; only the hands' texels, each to the block's least
red opaque color, where another does. test_marine_fingers.py takes the
constants, structures and code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>
#include <string.h>

typedef unsigned short word;
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define csmemset memset
struct bitmap_data;
typedef struct { real x, y; } real_point2d;

#include "under_test.inc"

/* ---------- a reference DXT1 decoder: a texel's red, green, blue and alpha */

struct texel { long red, green, blue, alpha; };

static long expand(long value, long bits)
{
	return bits == 5 ? (value << 3) | (value >> 2) : (value << 2) | (value >> 4);
}

static struct texel decoded(byte const *block, short texel)
{
	unsigned short first = (unsigned short)(block[0] | (block[1] << 8));
	unsigned short second = (unsigned short)(block[2] | (block[3] << 8));
	unsigned long indices = (unsigned long)block[4] | ((unsigned long)block[5] << 8) |
		((unsigned long)block[6] << 16) | ((unsigned long)block[7] << 24);
	unsigned long index = (indices >> (2 * texel)) & 3;
	long r0 = expand((first >> 11) & 31, 5), g0 = expand((first >> 5) & 63, 6), b0 = expand(first & 31, 5);
	long r1 = expand((second >> 11) & 31, 5), g1 = expand((second >> 5) & 63, 6), b1 = expand(second & 31, 5);
	struct texel result;

	result.alpha = 255;
	if (index == 0) { result.red = r0; result.green = g0; result.blue = b0; }
	else if (index == 1) { result.red = r1; result.green = g1; result.blue = b1; }
	else if (first > second)
	{
		long a = index == 2 ? 2 : 1;

		result.red = (a * r0 + (3 - a) * r1) / 3;
		result.green = (a * g0 + (3 - a) * g1) / 3;
		result.blue = (a * b0 + (3 - a) * b1) / 3;
	}
	else if (index == 2)
	{
		result.red = (r0 + r1) / 2; result.green = (g0 + g1) / 2; result.blue = (b0 + b1) / 2;
	}
	else
	{
		result.red = result.green = result.blue = 0; result.alpha = 0;
	}
	return result;
}

static unsigned short color(long red5, long green6, long blue5)
{
	return (unsigned short)((red5 << 11) | (green6 << 5) | blue5);
}

static void block_set(byte *block, unsigned short first, unsigned short second, unsigned long indices)
{
	block[0] = (byte)first; block[1] = (byte)(first >> 8);
	block[2] = (byte)second; block[3] = (byte)(second >> 8);
	block[4] = (byte)indices; block[5] = (byte)(indices >> 8); block[6] = (byte)(indices >> 16); block[7] = (byte)(indices >> 24);
}

/* every texel of the block, before and after, the same but for its red, which is now 0 (or, the texel not
among texels, the same) */
static boolean block_red_cleared(byte const *before, byte const *after, unsigned short texels)
{
	short texel;

	for (texel = 0; texel < 16; texel++)
	{
		struct texel was = decoded(before, texel);
		struct texel now = decoded(after, texel);

		if (!(texels & (1 << texel)))
		{
			if (was.red != now.red || was.green != now.green || was.blue != now.blue || was.alpha != now.alpha)
				return FALSE;
			continue;
		}
		if (now.red != 0 || now.green != was.green || now.blue != was.blue || now.alpha != was.alpha)
		{
			fprintf(stderr, "texel %d: was (%ld %ld %ld %ld) now (%ld %ld %ld %ld)\n", texel, was.red, was.green,
				was.blue, was.alpha, now.red, now.green, now.blue, now.alpha);
			return FALSE;
		}
	}
	return TRUE;
}

static boolean marked(unsigned long const *texels, long width, long x, long y)
{
	return texels_test(texels, width, x, y);
}

static struct shader_texture_animation still(void)
{
	struct shader_texture_animation animation;

	memset(&animation, 0, sizeof(animation));
	return animation;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	byte block[8];
	byte before[8];

	/* a block no other triangle samples, all red (the fingertips' mask):
	every texel's red cleared, opaque */
	CASE("whole red")
	{
		block_set(block, color(31, 0, 0), color(20, 0, 0), 0xE4E4E4E4UL);
		memcpy(before, block, 8);
		dxt1_block_clear(block, 0x0001, FALSE);
		CHECK(block_red_cleared(before, block, 0xFFFF), "the block's red is not cleared");
		return 0;
	}
	/* its colors, once without red, in the other order: swapped, each texel
	keeping its green and blue */
	CASE("swapped")
	{
		block_set(block, color(31, 3, 1), color(2, 40, 20), 0x1B1B1B1BUL);
		memcpy(before, block, 8);
		dxt1_block_clear(block, 0x0010, FALSE);
		CHECK(block_red_cleared(before, block, 0xFFFF), "a swapped block's texels changed");
		CHECK(block[0] | (block[1] << 8), "no colors");
		CHECK((block[0] | (block[1] << 8)) > (block[2] | (block[3] << 8)), "the block left four colors");
		return 0;
	}
	/* a block of three colors and transparent texels: kept three, the
	transparent ones transparent */
	CASE("three colors")
	{
		block_set(block, color(10, 50, 3), color(31, 2, 30), 0xE4E4E4E4UL);
		memcpy(before, block, 8);
		dxt1_block_clear(block, 0x8000, FALSE);
		CHECK(block_red_cleared(before, block, 0xFFFF), "a three-color block's texels changed");
		return 0;
	}
	/* a block another triangle samples: only the hands' texels take its
	color of least red, the others as they were */
	CASE("shared")
	{
		block_set(block, color(31, 0, 0), color(0, 0, 0), 0x00000000UL);
		memcpy(before, block, 8);
		dxt1_block_clear(block, 0x00F0, TRUE);
		CHECK(block_red_cleared(before, block, 0x00F0), "the hands' texels not cleared, or another changed");
		return 0;
	}
	/* in a shared block of three colors, never the transparent one */
	CASE("shared three colors")
	{
		block_set(block, color(20, 0, 0), color(31, 0, 0), 0x00000000UL);
		memcpy(before, block, 8);
		dxt1_block_clear(block, 0xFFFF, TRUE);
		CHECK(decoded(block, 0).alpha == 255 && decoded(block, 15).alpha == 255, "a texel was made transparent");
		CHECK(decoded(block, 0).red == decoded(before, 0).red, "the least red opaque color is the texel's own");
		return 0;
	}
	/* the texels a triangle covers, one it only touches at an edge, and not
	one it doesn't reach */
	CASE("covered")
	{
		real u[3] = { 2.f, 6.f, 2.f };
		real v[3] = { 2.f, 2.f, 6.f };

		CHECK(triangle_covers_texel(u, v, 3, 3), "a texel inside");
		CHECK(triangle_covers_texel(u, v, 1, 3), "a texel touching its edge");
		CHECK(triangle_covers_texel(u, v, 4, 3), "a texel the triangle crosses");
		CHECK(!triangle_covers_texel(u, v, 5, 5), "a texel beyond its long edge");
		CHECK(!triangle_covers_texel(u, v, 0, 3), "a texel beside it");
		return 0;
	}
	/* a triangle's texels and their neighbours marked, the edges wrapping */
	CASE("marked")
	{
		static unsigned long texels[MARINE_FINGERS_LEVEL_LONGS];
		real u[3] = { -0.5f, 0.4f, -0.5f };
		real v[3] = { 10.2f, 10.2f, 10.8f };

		memset(texels, 0, sizeof(texels));
		CHECK(triangle_mark(texels, 32, 64, u, v), "not marked");
		CHECK(marked(texels, 32, 31, 10) && marked(texels, 32, 0, 10), "the triangle's texels on both edges");
		CHECK(marked(texels, 32, 1, 11) && marked(texels, 32, 30, 9), "their neighbours");
		CHECK(!marked(texels, 32, 2, 10) && !marked(texels, 32, 0, 12) && !marked(texels, 32, 29, 10), "beyond the neighbours");
		u[1] = 100.f;
		CHECK(!triangle_mark(texels, 32, 64, u, v), "a triangle over twice the level marked");
		return 0;
	}
	/* a level's blocks: those with the hands' texels only, and those another
	triangle samples too, at their places in the pixels */
	CASE("blocks")
	{
		long x;
		long y;

		memset(marine_fingers_hand_texels, 0, sizeof(marine_fingers_hand_texels));
		memset(marine_fingers_other_texels, 0, sizeof(marine_fingers_other_texels));
		marine_fingers_globals.block_count = 0;
		/* (the hands at x 4 to 11 of rows 8 to 11; another triangle at x 10) */
		for (y = 8; y < 12; y++)
			for (x = 4; x < 12; x++)
				marine_fingers_hand_texels[(y * 16 + x) / LONG_BITS] |= 1UL << ((y * 16 + x) % LONG_BITS);
		marine_fingers_other_texels[(9 * 16 + 10) / LONG_BITS] |= 1UL << ((9 * 16 + 10) % LONG_BITS);
		CHECK(level_blocks_add(16, 16, 1000), "not added");
		CHECK(marine_fingers_globals.block_count == 2, "%d blocks", marine_fingers_globals.block_count);
		CHECK(marine_fingers_globals.blocks[0].offset == 1000 + (2 * 4 + 1) * 8 && marine_fingers_globals.blocks[0].texels == 0xFFFF &&
			!marine_fingers_globals.blocks[0].shared, "the first block");
		CHECK(marine_fingers_globals.blocks[1].offset == 1000 + (2 * 4 + 2) * 8 && marine_fingers_globals.blocks[1].shared &&
			marine_fingers_globals.blocks[1].texels == (0xFFFF & ~(1 << (1 * 4 + 2))), "the second block (%04x)",
			marine_fingers_globals.blocks[1].texels);
		return 0;
	}
	/* a texture animation that leaves the maps where they are, as the
	marines' arms': none, a whole texture's move, a whole turn */
	CASE("still")
	{
		struct shader_texture_animation animation = still();

		CHECK(texture_animation_still(&animation), "none");
		animation.u_scale = 1.f;
		animation.v_scale = 1.f;
		animation.r_scale = 360.f;
		CHECK(texture_animation_still(&animation), "the marines' arms'");
		animation.r_scale = 90.f;
		CHECK(!texture_animation_still(&animation), "a quarter turn");
		animation = still();
		animation.u_scale = 0.5f;
		CHECK(!texture_animation_still(&animation), "half a texture");
		animation = still();
		animation.v_scale = 1.f;
		animation.v_function = 2;
		CHECK(!texture_animation_still(&animation), "a moving one");
		animation = still();
		animation.u_scale = 1.f;
		animation.u_source = 1;
		CHECK(!texture_animation_still(&animation), "one an object's function moves");
		return 0;
	}

	return 2;
}
