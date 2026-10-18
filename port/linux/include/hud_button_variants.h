/*
HUD_BUTTON_VARIANTS.H

The PlayStation faces for the Xbox button icons the maps' menus draw (the
A SELECT, B BACK, X and Y icons of the menu button bar, and the smaller A and
B icons of the dialogs and the pause menu), shared by the texture caches of
the builds (port/linux/src/hud_hires.c, port/mac/mac_hud_hires.c).

Those icons are bitmaps of the maps, matched by the pixels they were drawn for
and replaced by the redraws of port/assets/buttons (tools/button_assets.py).
While input_prompt_glyphs_playstation() is true, the texture drawn in place of
one is the PlayStation face of the gameplay prompts instead: cross for A,
circle for B, square for X, triangle for Y. The faces are read from their own
art (port/assets/buttons/ps_*.png, which anyone may replace by files of the
same names and sizes) when the texture is made, so no picture is kept for
them but those PNGs. A face is drawn in the cell of the large icon (the top
left 21x26 texels of 32x32); a small icon's bitmap has its art smaller and
a little moved, so the face is scaled and moved as the redraw of the small
icon is (place: scale, then the move in texels, as buttons.json has them).
*/

#ifndef HUD_BUTTON_VARIANTS_H
#define HUD_BUTTON_VARIANTS_H

#include <string.h>
#include <stdlib.h>

struct hud_button_variant
{
	char const *tag;             /* the Xbox icon's bitmap */
	char const *playstation_tag; /* the asset drawn for it with a PlayStation pad */
	float scale, x, y;           /* where the face lies in the icon's bitmap */
};

static const struct hud_button_variant hud_button_variants[] =
{
	{ "ui\\shell\\bitmaps\\a_butn", "ui\\hud\\buttons\\ps_cross", 1.0f, 0.0f, 0.0f },
	{ "ui\\shell\\bitmaps\\b_butn", "ui\\hud\\buttons\\ps_circle", 1.0f, 0.0f, 0.0f },
	{ "ui\\shell\\bitmaps\\x_butn", "ui\\hud\\buttons\\ps_square", 1.0f, 0.0f, 0.0f },
	{ "ui\\shell\\bitmaps\\y_butn", "ui\\hud\\buttons\\ps_triangle", 1.0f, 0.0f, 0.0f },
	{ "ui\\shell\\bitmaps\\a_butn_sm", "ui\\hud\\buttons\\ps_cross", 0.7702f, -0.222f, -0.272f },
	{ "ui\\shell\\bitmaps\\b_butn_sm", "ui\\hud\\buttons\\ps_circle", 0.7677f, -0.14f, -0.225f },
};

/* the variant of the icon whose bitmap is tag, or NULL */
static __inline struct hud_button_variant const *hud_button_variant_find(char const *tag)
{
	unsigned int index;

	for (index = 0; tag && index < sizeof(hud_button_variants) / sizeof(hud_button_variants[0]); index++)
	{
		if (!strcmp(hud_button_variants[index].tag, tag))
			return &hud_button_variants[index];
	}
	return NULL;
}

/* A face's RGBA texels (width by height, top row first) scaled by scale about
the corner and moved by x and y texels of a bitmap size texels across (bilinear, over
colour weighted by alpha so that no dark fringe comes from the transparent
texels). Allocated; the caller frees it. NULL if it cannot be. */
static __inline unsigned char *hud_button_variant_place(const unsigned char *rgba, unsigned long width,
	unsigned long height, unsigned long size, float scale, float x, float y)
{
	unsigned char *out;
	unsigned long column, row, channel;
	float texel = (float)width / (float)size;

	if (!rgba || !width || !height || !size || scale <= 0.0f)
		return NULL;
	out = (unsigned char *)malloc(width * height * 4);
	if (!out)
		return NULL;
	for (row = 0; row < height; row++)
	{
		for (column = 0; column < width; column++)
		{
			/* (the source point of this texel's centre) */
			float source_x = ((column + 0.5f) / texel - x) / scale * texel - 0.5f;
			float source_y = ((row + 0.5f) / texel - y) / scale * texel - 0.5f;
			long x0 = (long)source_x, y0 = (long)source_y;
			float fx, fy, sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			int tap;

			if (source_x < 0.0f) x0--;
			if (source_y < 0.0f) y0--;
			fx = source_x - (float)x0;
			fy = source_y - (float)y0;
			for (tap = 0; tap < 4; tap++)
			{
				long tx = x0 + (tap & 1), ty = y0 + (tap >> 1);
				float weight = ((tap & 1) ? fx : 1.0f - fx) * ((tap >> 1) ? fy : 1.0f - fy);
				const unsigned char *pixel;
				float alpha;

				if (tx < 0 || ty < 0 || tx >= (long)width || ty >= (long)height)
					continue;
				pixel = rgba + ((unsigned long)ty * width + (unsigned long)tx) * 4;
				alpha = pixel[3] * weight;
				sum[0] += pixel[0] * alpha;
				sum[1] += pixel[1] * alpha;
				sum[2] += pixel[2] * alpha;
				sum[3] += alpha;
			}
			for (channel = 0; channel < 3; channel++)
			{
				float value = sum[3] > 0.0f ? sum[channel] / sum[3] : 0.0f;

				out[(row * width + column) * 4 + channel] = (unsigned char)(value + 0.5f);
			}
			out[(row * width + column) * 4 + 3] = (unsigned char)(sum[3] + 0.5f);
		}
	}
	return out;
}

#endif
