/*
HUD_UPSCALE_CACHE.C

The enlarged Custom Edition HUD sheets kept by their pixels (hud_hires.c): a
sheet asked for again is the same texture while any texture uses it or it is
kept, sheets of another kind or size, or whose pixels differ under the same
CRC, are not; a texture is deleted only when no texture uses it, the oldest
unused first, keeping at most UPSCALE_CACHE_UNUSED and UPSCALE_CACHE_UNUSED_BYTES.
test_hud_upscale_cache.py takes the code from the sources.
*/

#include "harness.h"

typedef unsigned int GLuint;
typedef unsigned char Bytef;
typedef unsigned int uInt;

enum { TEXTURES = 1000 };

static int force_crc;
static unsigned long made, deleted;
/* each texture made: whether it is deleted, and how many textures use it */
static int texture_deleted[TEXTURES];
static long texture_users[TEXTURES];

#define xgpu_gl_ready() TRUE
#define xgpu_gl_state_invalidate() ((void)0)

static unsigned long crc32(unsigned long crc, Bytef const *bytes, uInt count)
{
	uInt index;

	if (force_crc)
		return 0x1234;
	crc = ~crc;
	for (index = 0; index < count; index++)
		crc = (crc ^ bytes[index]) * 16777619UL;
	return crc;
}

static void glDeleteTextures(int count, GLuint const *texture)
{
	CHECK(count == 1 && *texture > 0 && *texture <= made, "deleted %u", *texture);
	CHECK(!texture_deleted[*texture], "texture %u deleted twice", *texture);
	CHECK(!texture_users[*texture], "texture %u deleted while %ld textures use it", *texture, texture_users[*texture]);
	texture_deleted[*texture] = TRUE;
	deleted++;
}

static unsigned int upscale_texture_make(unsigned char const *bgra, unsigned long width, unsigned long height,
	long kind, unsigned long *levels)
{
	(void)bgra;
	(void)kind;
	*levels = width + height;
	return (unsigned int)++made;
}

#include "under_test.inc"

static unsigned char sheets[64][32 * 32 * 4];
/* (big enough that UPSCALE_CACHE_UNUSED of them are more than the bytes kept) */
static unsigned char big_sheets[UPSCALE_CACHE_UNUSED][256 * 128 * 4];

static void make_sheets(void)
{
	int sheet, index;

	for (sheet = 0; sheet < 64; sheet++)
		for (index = 0; index < 32 * 32 * 4; index++)
			sheets[sheet][index] = (unsigned char)(sheet * 7 + index * 13);
	for (sheet = 0; sheet < UPSCALE_CACHE_UNUSED; sheet++)
		for (index = 0; index < 256 * 128 * 4; index++)
			big_sheets[sheet][index] = (unsigned char)(sheet * 5 + index * 11);
}

/* a texture of the xbox texture cache asking for the sheet, as xgpu_texture_get does */
static unsigned int take_pixels(unsigned char const *pixels, unsigned long width, unsigned long height, long kind)
{
	unsigned long levels = 0;
	unsigned int texture = hud_hires_upscale_texture(pixels, width, height, kind, &levels);

	CHECK(texture, "no texture");
	CHECK(levels == width + height, "the levels of texture %u", texture);
	texture_users[texture]++;
	return texture;
}
#define take(sheet, width, height, kind) take_pixels(sheets[sheet], width, height, kind)

static void let_go(unsigned int texture)
{
	CHECK(texture_users[texture] > 0, "let go of %u, used by none", texture);
	texture_users[texture]--;
	hud_hires_upscale_release(texture);
}

static long kept_unused(void)
{
	long index, count = 0;

	for (index = 0; index < UPSCALE_CACHE_SIZE; index++)
		count += upscale_cache[index].texture && !upscale_cache[index].users;
	return count;
}

/* the same sheet, taken twice and let go: one texture, kept, and the same again */
static void case_reuse(void)
{
	unsigned int first = take(1, 32, 32, 1), second = take(1, 32, 32, 1), again;

	CHECK(first == second && made == 1, "the same sheet made twice");
	let_go(first);
	let_go(second);
	CHECK(!deleted, "a kept texture deleted");
	again = take(1, 32, 32, 1);
	CHECK(again == first && made == 1, "a kept sheet made again");
}

/* the same pixels as another kind or size, and other pixels under the same CRC */
static void case_distinct(int forced)
{
	unsigned int a, b, c, d, e;

	force_crc = forced;
	a = take(2, 32, 32, 1);
	b = take(2, 32, 32, 2);
	c = take(2, 16, 64, 1);
	d = take(3, 32, 32, 1);
	e = take(2, 32, 32, 1);
	CHECK(a != b && a != c && b != c && a != d, "different sheets share a texture: %u %u %u %u", a, b, c, d);
	CHECK(e == a, "the same sheet not shared");
	CHECK(made == 4, "%lu made, expected 4", made);
}

/* more sheets held than the cache keeps; then all let go, in turn: never a
texture in use deleted, the ones past the cache's room deleted at once, the
oldest unused deleted first, UPSCALE_CACHE_UNUSED kept */
static void case_eviction(void)
{
	unsigned int held[40];
	int sheet;

	for (sheet = 0; sheet < 40; sheet++)
		held[sheet] = take(sheet, 8, 8, 1);
	CHECK(made == 40, "%lu made", made);
	for (sheet = 0; sheet < 40; sheet++)
		let_go(held[sheet]);
	CHECK(kept_unused() == UPSCALE_CACHE_UNUSED, "%ld kept unused", kept_unused());
	for (sheet = 0; sheet < 40; sheet++)
	{
		/* (the ones past the cache's room, and all but the last let go of the rest) */
		boolean expected = sheet >= UPSCALE_CACHE_SIZE || sheet < UPSCALE_CACHE_SIZE - UPSCALE_CACHE_UNUSED;

		CHECK(texture_deleted[held[sheet]] == expected, "sheet %d %s, expected %s", sheet,
			texture_deleted[held[sheet]] ? "deleted" : "kept", expected ? "deleted" : "kept");
	}
	/* (a full cache of used ones: a new sheet is made, used and let go) */
	for (sheet = 0; sheet < UPSCALE_CACHE_SIZE; sheet++)
		held[sheet] = take(sheet, 16, 16, 3);
	held[39] = take(39, 16, 16, 3);
	let_go(held[39]);
	CHECK(texture_deleted[held[39]], "a sheet with no room kept");
}

/* big sheets let go: the unused ones kept hold no more than the bytes allowed */
static void case_bytes(void)
{
	unsigned int held[UPSCALE_CACHE_UNUSED];
	unsigned long bytes = 0;
	int sheet, index;

	for (sheet = 0; sheet < UPSCALE_CACHE_UNUSED; sheet++)
		held[sheet] = take_pixels(big_sheets[sheet], 256, 128, 1);
	for (sheet = 0; sheet < UPSCALE_CACHE_UNUSED; sheet++)
		let_go(held[sheet]);
	for (index = 0; index < UPSCALE_CACHE_SIZE; index++)
		if (upscale_cache[index].texture && !upscale_cache[index].users)
			bytes += upscale_cache[index].bytes;
	CHECK(bytes <= UPSCALE_CACHE_UNUSED_BYTES, "%lu bytes kept unused", bytes);
	CHECK(kept_unused() > 0 && kept_unused() < UPSCALE_CACHE_UNUSED, "%ld kept: the bytes did not limit them",
		kept_unused());
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	make_sheets();
	CASE("reuse") case_reuse();
	else CASE("distinct") case_distinct(FALSE);
	else CASE("distinct-same-crc") case_distinct(TRUE);
	else CASE("eviction") case_eviction();
	else CASE("bytes") case_bytes();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
