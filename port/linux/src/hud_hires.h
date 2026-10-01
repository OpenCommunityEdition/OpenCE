/*
HUD_HIRES.H

The high-res HUD: textures drawn from the hand-made SVG redraws of the Halo PC
HUD sheets (port/assets/hud, made by tools/hud_assets.py), each 8x the size
of the bitmap of the Xbox maps it stands for and in that bitmap's layout.
tools/embed_assets.py makes them C data (hud_hires_embedded); hud_hires.c
decodes them, and the texture cache (xbox_textures.c) draws one in place of
its bitmap whenever that bitmap's pixels are uploaded: the game still sizes
and places the bitmap by its tag, so nothing else changes.
*/

#ifndef HUD_HIRES_H
#define HUD_HIRES_H

/* an embedded texture: an 8-bit RGBA PNG, and the bitmap it stands for (its
bitmap group tag's name, and its index there) */
struct hud_hires_embedded
{
	const char *tag;
	int bitmap;
	unsigned int width, height;
	const unsigned int *png;
	unsigned int png_size;
};

extern const struct hud_hires_embedded hud_hires_embedded[];
extern const unsigned int hud_hires_embedded_count;

/* the texture standing for the bitmap whose pixels are uploaded from address
(guest virtual) with this size, or -1: none, or display.hires_hud off */
long hud_hires_override_find(unsigned long address, unsigned long width, unsigned long height);
/* its GL texture (decoded and uploaded, mipmapped, on first use; 0 if it
could not be), and the number of its mip levels */
unsigned int hud_hires_override_texture(long asset, unsigned long *levels);

#endif
