/*
HUD_HIRES_SPRITES.C (test)

The real hud_hires_sprites_drawable (hud_hires.c) for a bitmap whose settings and
pixels would let its high-res sprite texture be drawn: it answers 0 while the GL
functions are not loaded (debug.null_renderer, or a context that failed), as it
would otherwise reach the texture's glGenTextures through
hud_hires_override_texture, and goes on to the pixels' check once they are.
test_hud_hires_sprites.py takes the embedded texture's structure (hud_hires_embedded)
and the code (under_test.inc). hud_hires.c is not compiled whole: its other
functions decode PNGs on a thread and upload through GL.
*/

#include "harness.h"
#include "config.inc"

typedef unsigned char Bytef;
typedef unsigned int uInt;

static unsigned long changes = 1;
static unsigned long config_changes(void) { return changes; }
static int config_boolean(const char *name) { return 1; }

#define PIXELS_CRC 0xC0FFEEUL
const struct hud_hires_embedded hud_hires_embedded[2] =
{
	{ 0 },
	{ .tag = "hud_test", .bitmap = 3, .sprites = 1, .crc = PIXELS_CRC },
};
static long hud_hires_asset_count(void) { return 2; }
static struct { int other_pixels_logged; } textures[2];

/* the pixels' CRC: the first byte of the bitmap, so a case chooses it */
static long crc_calls;
static unsigned long crc32(unsigned long crc, const Bytef *pixels, uInt size)
{
	crc_calls++;
	return pixels[0] == 1 ? PIXELS_CRC : 0;
}

/* glGenTextures as the port loads it (a pointer, NULL until loaded), and the
texture's decode and upload, which calls it */
static void *halo_glGenTextures_loaded(void) { return NULL; }
static void *(*halo_glGenTextures)(void);
#define glGenTextures halo_glGenTextures
static long texture_calls;
static unsigned int hud_hires_override_texture(long asset, unsigned long *levels)
{
	texture_calls++;
	CHECK(glGenTextures, "the texture was made with glGenTextures NULL");
	return 7;
}

static long logs;
#define platform_log(...) (logs++)

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	static unsigned char pixels[16] = { 1 };
	static unsigned char other_pixels[16] = { 2 };
	unsigned long address = (unsigned long)pixels;

	/* GL loaded, the pixels the texture was drawn for: drawn, the texture made */
	CASE("drawn-with-gl")
	{
		halo_glGenTextures = halo_glGenTextures_loaded;
		CHECK(hud_hires_sprites_drawable(1, address, 16) == 1, "the sprites' texture was not drawn with GL loaded");
		CHECK(texture_calls == 1, "the texture was made %ld times", texture_calls);
		return 0;
	}
	/* GL not loaded, the same bitmap: not drawn, and the texture is not made (glGenTextures is NULL) */
	CASE("not-drawn-without-gl")
	{
		halo_glGenTextures = NULL;
		CHECK(hud_hires_sprites_drawable(1, address, 16) == 0, "the sprites' texture was drawn without GL");
		CHECK(texture_calls == 0, "the texture was made %ld times without GL", texture_calls);
		CHECK(crc_calls == 0, "the pixels were checked %ld times without GL", crc_calls);
		return 0;
	}
	/* GL loaded, other pixels: the guard lets the call reach the pixels' check, which refuses */
	CASE("reaches-pixels-check-with-gl")
	{
		halo_glGenTextures = halo_glGenTextures_loaded;
		CHECK(hud_hires_sprites_drawable(1, (unsigned long)other_pixels, 16) == 0,
			"other pixels were drawn the texture");
		CHECK(crc_calls == 1, "the pixels were checked %ld times with GL", crc_calls);
		CHECK(texture_calls == 0, "the texture was made for other pixels");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
