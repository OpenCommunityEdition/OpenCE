/*
SUN_GLOW_TARGET.C (test)

The real surface_is_sun_glow (port/linux/src/d3d8_gl.c): which render targets
are drawn at twice the screen's scale. The sun glow's two targets are 64x64
A8R8G8B8 textures of their own; the water's ripples (a 128x128 A8R8G8B8
texture of four levels) draw their second level, 64x64 too, into its place in
their texture, where the water reads it as it lies farther off.
test_sun_glow_target.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

typedef unsigned long DWORD;
typedef int BOOL;

#define D3DFMT_A8R8G8B8 0x06
#define D3DFMT_X8R8G8B8 0x07

struct D3DBaseTexture { unsigned long Common, Data, Lock, Format, Size; };
typedef struct D3DSurface
{
	unsigned long Common, Data, Lock, Format, Size;
	struct D3DBaseTexture *Parent;
} D3DSurface;
struct xgpu_texture_description { DWORD format; unsigned long width, height, depth, levels; };

/* (the stub's words: Format is the format, Size the width above the height) */
static void xgpu_texture_describe(DWORD format_word, DWORD size_word, struct xgpu_texture_description *description)
{
	memset(description, 0, sizeof(*description));
	description->format = format_word;
	description->width = size_word >> 16;
	description->height = size_word & 0xFFFF;
	description->depth = 1;
	description->levels = 1;
}

#include "under_test.inc"

/* a texture's level as GetSurfaceLevel makes it (d3d8_resources.c): its data
past the levels before it, its parent the texture */
static D3DSurface level_surface(struct D3DBaseTexture *texture, unsigned long offset, DWORD format,
	unsigned long width, unsigned long height)
{
	D3DSurface surface;

	memset(&surface, 0, sizeof(surface));
	surface.Data = texture ? texture->Data + offset : 0x80000000UL + offset;
	surface.Format = format;
	surface.Size = width << 16 | height;
	surface.Parent = texture;
	return surface;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct D3DBaseTexture sun_glow = {0, 0x01000000UL, 0, D3DFMT_A8R8G8B8, 64UL << 16 | 64};
	struct D3DBaseTexture water = {0, 0x02000000UL, 0, D3DFMT_A8R8G8B8, 128UL << 16 | 128};

	/* the sun glow's targets: their texture's only level, or a surface of
	its own: drawn at twice the screen's scale, as before */
	CASE("sun glow")
	{
		D3DSurface top = level_surface(&sun_glow, 0, D3DFMT_A8R8G8B8, 64, 64);
		D3DSurface own = level_surface(NULL, 0, D3DFMT_A8R8G8B8, 64, 64);

		CHECK(surface_is_sun_glow(&top), "the sun glow's target not taken");
		CHECK(surface_is_sun_glow(&own), "a 64x64 target of no texture not taken");
		return 0;
	}
	/* the water's ripples: no level of theirs, the 64x64 second least */
	CASE("water")
	{
		D3DSurface level0 = level_surface(&water, 0, D3DFMT_A8R8G8B8, 128, 128);
		D3DSurface level1 = level_surface(&water, 128 * 128 * 4, D3DFMT_A8R8G8B8, 64, 64);
		D3DSurface level2 = level_surface(&water, 128 * 128 * 4 + 64 * 64 * 4, D3DFMT_A8R8G8B8, 32, 32);

		CHECK(!surface_is_sun_glow(&level0), "the water's first level taken");
		CHECK(!surface_is_sun_glow(&level1), "the water's second level (64x64) taken for the sun glow's");
		CHECK(!surface_is_sun_glow(&level2), "the water's third level taken");
		return 0;
	}
	/* other shapes and formats: left alone */
	CASE("others")
	{
		D3DSurface opaque = level_surface(NULL, 0, D3DFMT_X8R8G8B8, 64, 64);
		D3DSurface wider = level_surface(NULL, 0, D3DFMT_A8R8G8B8, 128, 64);

		CHECK(!surface_is_sun_glow(&opaque), "an X8R8G8B8 target taken");
		CHECK(!surface_is_sun_glow(&wider), "a 128x64 target taken");
		return 0;
	}

	return 2;
}
