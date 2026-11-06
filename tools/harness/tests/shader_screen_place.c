/*
SHADER_SCREEN_PLACE.C (test)

The real nv2a_pixel_shader_to_glsl (port/linux/src/nv2a_psh.c), the pixel
shader translator, with the port's xgpu_text_append. The active camouflage's
distortion (a cube map stage, a dot product stage, then a DOT_ST stage that
reads the copy of the screen) gets its texture coordinates 1 and 2, places on
the screen its vertex program divided by w, interpolated linearly across the
screen (noperspective), so that a triangle reaching toward the camera reads
each pixel's own place, not the near vertex's far off the copy; every other
shader's coordinates are interpolated with the perspective, as are all on
OpenGL ES, which has no such interpolation. test_shader_screen_place.py takes
the constants, the structures and the code from the sources (config.inc,
under_test.inc), as the desktop build compiles them or with HALO_GLES.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"

typedef unsigned long DWORD;
typedef int BOOL;

#include "config.inc"

/* the port's settings: none of the debug ones on */
static const char *config_string(const char *name) { return ""; }
static int config_boolean(const char *name) { return 0; }
BOOL xgpu_reversed_depth = TRUE;
#ifdef HALO_GLES
struct { const char *shading_language; } xgpu_capabilities = { "300 es" };
#endif

#include "under_test.inc"

/* the stages' texture modes, packed as the key holds them */
enum
{
	MODE_NONE = 0x00,
	MODE_PROJECT2D = 0x01,
	MODE_CUBEMAP = 0x03,
	MODE_DOT_ST = 0x09,
	MODE_DOT_STR_CUBE = 0x0e,
	MODE_DOT_PRODUCT = 0x11,
};

static char *translated(DWORD mode0, DWORD mode1, DWORD mode2, DWORD mode3)
{
	static struct nv2a_pixel_shader_key key;
	char *glsl;

	memset(&key, 0, sizeof(key));
	key.texture_modes = mode0 | mode1 << 5 | mode2 << 10 | mode3 << 15;
	key.combiner_state[D3DRS_PSCOMBINERCOUNT] = 1;
	glsl = nv2a_pixel_shader_to_glsl(&key);
	CHECK(glsl != NULL, "no shader translated");
	return glsl;
}

/* how the shader declares texture coordinate n: "noperspective " or "" */
static void declares(const char *glsl, int coordinate, const char *qualifier, const char *what)
{
	char declaration[64];
	char const *found;

	snprintf(declaration, sizeof(declaration), "in vec4 xT%d;\n", coordinate);
	found = strstr(glsl, declaration);
	CHECK(found != NULL, "%s: texture coordinate %d is not declared", what, coordinate);
	if (*qualifier)
	{
		CHECK(found - glsl >= (long)strlen(qualifier) &&
			!strncmp(found - strlen(qualifier), qualifier, strlen(qualifier)),
			"%s: texture coordinate %d is not declared %s", what, coordinate, qualifier);
	}
	else
	{
		CHECK(found == glsl || found[-1] == '\n',
			"%s: texture coordinate %d is declared with an interpolation of its own", what, coordinate);
	}
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* the cloak's distortion: coordinates 1 and 2 linear across the screen
	(on OpenGL ES, with the perspective) */
	CASE("cloak")
	{
		char *glsl = translated(MODE_CUBEMAP, MODE_DOT_PRODUCT, MODE_DOT_ST, MODE_NONE);
#ifdef HALO_GLES
		const char *screen = "";
#else
		const char *screen = "noperspective ";
#endif

		declares(glsl, 0, "", "the cloak");
		declares(glsl, 1, screen, "the cloak");
		declares(glsl, 2, screen, "the cloak");
		declares(glsl, 3, "", "the cloak");
		CHECK(strstr(glsl, "noperspective") == NULL || *screen, "the cloak on OpenGL ES uses noperspective");
		return 0;
	}
	/* other shaders: a texture, and an environment's bumped reflection (dot
	products into a cube map): coordinates with the perspective */
	CASE("others")
	{
		char *glsl = translated(MODE_PROJECT2D, MODE_NONE, MODE_NONE, MODE_NONE);
		int coordinate;

		for (coordinate = 0; coordinate < 4; coordinate++)
			declares(glsl, coordinate, "", "a texture");
		glsl = translated(MODE_PROJECT2D, MODE_DOT_PRODUCT, MODE_DOT_PRODUCT, MODE_DOT_STR_CUBE);
		for (coordinate = 0; coordinate < 4; coordinate++)
			declares(glsl, coordinate, "", "a bumped reflection");
		CHECK(strstr(glsl, "noperspective") == NULL, "a bumped reflection uses noperspective");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
