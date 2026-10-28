/*
TEXT_HIRES.C

The game's text drawn with fonts at the display's resolution (text_hires.h).

Each font tag the fonts draw (port/assets/fonts/fonts.json) gets its font
sized so that its capitals are as tall as the tag's: the game's layout,
from the tag's character widths, is kept, and the fonts were chosen to fit
the Xbox maps' widths. A tag whose letters are narrower (the fonts of a
Custom Edition loc.map) gets its font narrowed until its letters are, on
the whole, as wide as the tag's, so that they do not run into each other;
the Xbox maps' fonts are narrowed by a few hundredths. A glyph is rasterized when first drawn, at the display's
pixels per unit of the 480 lines, into an 8-bit atlas; the GL texture the
game's placeholder bitmap stands for (xbox_textures.c) is the atlas, its new
rows uploaded when the placeholder is bound. A change of the display's
scale (a window made fullscreen) or a full atlas starts it over.
*/

#include "text_hires.h"
#include "platform.h"
#include "port_config.h"
#include "xgpu.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb/stb_truetype.h"

/* (the Xbox maps' tags and a Custom Edition loc.map's, which share names,
each at its oversamples) */
#define MAXIMUM_FONTS 16
#define ATLAS_SIZE 2048
/* clear texels around each glyph, so that filtering never reaches the next */
#define ATLAS_PADDING 2
#define GLYPH_CACHE_SIZE 4096

static struct
{
	char tag[64];
	/* the tag's measures it was made for */
	float cap_height;
	long letters_width;
	stbtt_fontinfo info;
	/* font units to units of the 480 lines */
	float scale;
	/* the glyphs' widths, against their heights, to fit the tag's letters */
	float narrowing;
	/* how many times the display's pixels its glyphs are rasterized with */
	float oversample;
	/* made while text_hires_force was on */
	int forced;
} fonts[MAXIMUM_FONTS];
static int forced_text;

void text_hires_force(int on)
{
	forced_text = on != 0;
}

int text_hires_forced(void)
{
	return forced_text;
}
static long font_count;

static struct
{
	long font;          /* -1: free */
	unsigned long code;
	short x, y, width, height;
	short left, top;    /* the glyph's corner, in pixels from the pen on the baseline */
	float advance;      /* in units of the 480 lines */
} glyphs[GLYPH_CACHE_SIZE];

static unsigned char *atlas;
static float atlas_scale;   /* the pixels per unit its glyphs have */
static int pack_x, pack_y, pack_row_height;
static int dirty_top = ATLAS_SIZE, dirty_bottom;
static unsigned long placeholder_data, placeholder_width, placeholder_height;
static GLuint atlas_texture;

/* tag names compared as the game does, ignoring case (and without the POSIX
strcasecmp, which the Windows build has not) */
static int names_differ(const char *a, const char *b)
{
	while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b))
	{
		a++;
		b++;
	}
	return tolower((unsigned char)*a) != tolower((unsigned char)*b);
}

int text_hires_enabled(void)
{
	static int enabled;
	static unsigned long read_at = (unsigned long)-1;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		enabled = config_boolean("display.high_res_text");
	}
	return enabled;
}

static void (*reset_hook)(void);

void text_hires_set_reset_hook(void (*hook)(void))
{
	reset_hook = hook;
}

/* the font of the characters that the fonts lack, for the subtitles'
language: Noto Sans CJK, a subset of its region's forms (fonts.json) */
static stbtt_fontinfo fallback_info;
static long fallback_language = -1;     /* the embedded font in use, -1: none */
static int fallback_stale;              /* the glyphs of another font are in the atlas */
static unsigned long fallback_read_at = (unsigned long)-1;

/* the characters that the fallback is for: CJK ideographs, symbols and
punctuation, kana, Hangul and the full-width forms; no other characters, which
a font tag without them is without in the maps' own text */
static int fallback_range(unsigned long code)
{
	return (code >= 0x2E80 && code <= 0x9FFF) || (code >= 0xAC00 && code <= 0xD7AF) ||
		(code >= 0xF900 && code <= 0xFAFF) || (code >= 0xFF00 && code <= 0xFFEF);
}

/* whether the subtitles' language has a fallback font, read again when the
settings change */
static int fallback_ready(void)
{
	if (fallback_read_at != config_changes())
	{
		char const *language = config_string("display.subtitles_language");
		long found = -1;
		unsigned int index;

		fallback_read_at = config_changes();
		for (index = 0; index < text_hires_fallbacks_count; index++)
		{
			const struct text_hires_fallback *embedded = &text_hires_fallbacks[index];

			if (language && embedded->language && !strcmp(embedded->language, language))
			{
				const unsigned char *data = (const unsigned char *)embedded->data;

				if (stbtt_InitFont(&fallback_info, data, stbtt_GetFontOffsetForIndex(data, 0)))
					found = (long)index;
				else
					platform_log("high-res text: could not read %s", embedded->file);
				break;
			}
		}
		if (found != fallback_language)
		{
			fallback_language = found;
			fallback_stale = 1;
		}
	}
	return fallback_language >= 0 && text_hires_enabled();
}

int text_hires_cjk_covers(unsigned long code)
{
	return placeholder_data && fallback_range(code) && fallback_ready() &&
		stbtt_FindGlyphIndex(&fallback_info, (int)code) != 0;
}

float text_hires_forced_advance(unsigned long code, float cap_height)
{
	static stbtt_fontinfo small_info;
	static int small_state;     /* 0: not looked for, 1: found, -1: none */
	int x0, y0, x1, y1, glyph, advance = 0, bearing = 0;

	if (!forced_text || !placeholder_data || cap_height <= 0.0f || !text_hires_enabled())
		return 0.0f;
	if (!small_state)
	{
		unsigned int index;

		small_state = -1;
		for (index = 0; index < text_hires_embedded_count; index++)
		{
			const unsigned char *data = (const unsigned char *)text_hires_embedded[index].data;

			if (text_hires_embedded[index].tag && !names_differ(text_hires_embedded[index].tag, "ui\\small_ui") &&
				stbtt_InitFont(&small_info, data, stbtt_GetFontOffsetForIndex(data, 0)))
			{
				small_state = 1;
				break;
			}
		}
	}
	if (small_state < 0 || !stbtt_GetCodepointBox(&small_info, 'H', &x0, &y0, &x1, &y1) || y1 <= y0)
		return 0.0f;
	glyph = stbtt_FindGlyphIndex(&small_info, (int)code);
	if (!glyph)
		return 0.0f;
	stbtt_GetGlyphHMetrics(&small_info, glyph, &advance, &bearing);
	return advance * cap_height / (float)(y1 - y0);
}

/* the em of a fallback glyph, in its cell: Noto's ideographs fill most of the
em, and the cell is the text's line */
#define FALLBACK_EM 0.92f

/* the atlas emptied: every glyph to be rasterized again */
static void atlas_reset(float scale)
{
	long index;

	/* (text batched from the glyphs there drawn first) */
	if (reset_hook && atlas)
		reset_hook();
	if (!atlas)
		atlas = malloc(ATLAS_SIZE * ATLAS_SIZE);
	if (atlas)
		memset(atlas, 0, ATLAS_SIZE * ATLAS_SIZE);
	for (index = 0; index < GLYPH_CACHE_SIZE; index++)
		glyphs[index].font = -1;
	atlas_scale = scale;
	pack_x = pack_y = pack_row_height = 0;
	dirty_top = 0;
	dirty_bottom = ATLAS_SIZE;
}

long text_hires_font(char const *tag_name, float cap_height, short const *letter_widths, float oversample)
{
	unsigned int index;
	long font;
	long letters_width = 0;
	int x0, y0, x1, y1;
	int letter;
	int use_small;

	if (!text_hires_enabled() || !tag_name || cap_height <= 0.0f || !letter_widths)
		return -1;
	/* (text is never drawn with fewer pixels than the display's) */
	if (oversample < 1.0f)
		oversample = 1.0f;
	for (letter = 0; letter < TEXT_HIRES_FIT_LETTERS; letter++)
		letters_width += letter_widths[letter] > 0 ? letter_widths[letter] : 0;
	for (font = 0; font < font_count; font++)
	{
		if (!names_differ(fonts[font].tag, tag_name) && fonts[font].cap_height == cap_height &&
			fonts[font].letters_width == letters_width && fonts[font].oversample == oversample &&
			fonts[font].forced == forced_text)
		{
			return fonts[font].scale > 0.0f ? font : -1;
		}
	}
	if (font_count >= MAXIMUM_FONTS)
		return -1;
	font = font_count++;
	snprintf(fonts[font].tag, sizeof(fonts[font].tag), "%s", tag_name);
	fonts[font].cap_height = cap_height;
	fonts[font].letters_width = letters_width;
	fonts[font].scale = 0.0f;
	fonts[font].narrowing = 1.0f;
	fonts[font].oversample = oversample;
	fonts[font].forced = forced_text;
	/* (a tag no font stands for, drawn with the small font while forced) */
	for (index = 0; index < text_hires_embedded_count; index++)
	{
		if (text_hires_embedded[index].tag && !names_differ(text_hires_embedded[index].tag, tag_name))
			break;
	}
	use_small = forced_text && index == text_hires_embedded_count;
	for (index = 0; index < text_hires_embedded_count; index++)
	{
		const struct text_hires_embedded *embedded = &text_hires_embedded[index];
		const unsigned char *data = (const unsigned char *)embedded->data;

		if (!embedded->tag ||
			(use_small ? names_differ(embedded->tag, "ui\\small_ui") : names_differ(embedded->tag, tag_name)))
		{
			continue;
		}
		if (!stbtt_InitFont(&fonts[font].info, data, stbtt_GetFontOffsetForIndex(data, 0)) ||
			!stbtt_GetCodepointBox(&fonts[font].info, 'H', &x0, &y0, &x1, &y1) || y1 <= y0)
		{
			platform_log("high-res text: could not read %s for %s", embedded->file, tag_name);
			break;
		}
		fonts[font].scale = cap_height / (float)(y1 - y0);
		{
			float tag_width = 0.0f;
			float font_width = 0.0f;

			for (letter = 0; letter < TEXT_HIRES_FIT_LETTERS; letter++)
			{
				int glyph = stbtt_FindGlyphIndex(&fonts[font].info, letter < 26 ? 'A' + letter : 'a' + letter - 26);
				int advance = 0, bearing = 0;

				if (letter_widths[letter] <= 0 || !glyph)
					continue;
				stbtt_GetGlyphHMetrics(&fonts[font].info, glyph, &advance, &bearing);
				tag_width += letter_widths[letter];
				font_width += advance * fonts[font].scale;
			}
			if (tag_width > 0.0f && font_width > tag_width)
				fonts[font].narrowing = tag_width / font_width;
		}
		platform_log("high-res text: %s (capitals %.0f units) drawn with %s, %.0f%% wide",
			tag_name, cap_height, embedded->file, fonts[font].narrowing * 100.0f);
		break;
	}
	return fonts[font].scale > 0.0f ? font : -1;
}

int text_hires_covers(long font, unsigned long code)
{
	if (font < 0 || font >= font_count || fonts[font].scale <= 0.0f)
		return 0;
	/* (control characters are never drawn) */
	return code < 32 || code == ' ' || stbtt_FindGlyphIndex(&fonts[font].info, (int)code) != 0 ||
		text_hires_cjk_covers(code);
}

int text_hires_glyph(long font, unsigned long code, struct text_hires_glyph *glyph)
{
	return text_hires_glyph_cell(font, code, 0.0f, glyph);
}

int text_hires_glyph_cell(long font, unsigned long code, float cell, struct text_hires_glyph *glyph)
{
	float scale = halo_screen_pixel_scale();
	float pixel_scale;
	unsigned long slot;
	long probe;

	if (font < 0 || font >= font_count || fonts[font].scale <= 0.0f || !placeholder_data || scale <= 0.0f)
		return 0;
	/* (a fallback font of another language has other glyphs for the characters) */
	fallback_ready();
	if (!atlas || scale != atlas_scale || fallback_stale)
	{
		fallback_stale = 0;
		atlas_reset(scale);
	}
	if (!atlas)
		return 0;
	slot = ((unsigned long)font * 2654435761UL ^ code * 40503UL) & (GLYPH_CACHE_SIZE - 1);
	for (probe = 0; probe < GLYPH_CACHE_SIZE; probe++, slot = (slot + 1) & (GLYPH_CACHE_SIZE - 1))
	{
		if (glyphs[slot].font == -1 || (glyphs[slot].font == font && glyphs[slot].code == code))
			break;
	}
	if (probe == GLYPH_CACHE_SIZE)
	{
		atlas_reset(scale);
		return text_hires_glyph_cell(font, code, cell, glyph);
	}
	if (glyphs[slot].font == -1)
	{
		/* rasterized now */
		float unit_scale = fonts[font].scale * fonts[font].narrowing;
		float pixels = fonts[font].scale * scale * fonts[font].oversample;
		float pixels_x = pixels * fonts[font].narrowing;
		stbtt_fontinfo *info = &fonts[font].info;
		int index = stbtt_FindGlyphIndex(info, (int)code);
		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		int width, height, advance = 0, bearing = 0;

		/* (a character the font lacks, from the fallback, its em in the cell) */
		if (!index && cell > 0.0f && fallback_range(code) && fallback_ready())
		{
			index = stbtt_FindGlyphIndex(&fallback_info, (int)code);
			if (index)
			{
				info = &fallback_info;
				unit_scale = cell * FALLBACK_EM * stbtt_ScaleForMappingEmToPixels(info, 1.0f);
				pixels = unit_scale * scale * fonts[font].oversample;
				pixels_x = pixels;
			}
		}
		if (index)
		{
			stbtt_GetGlyphBitmapBox(info, index, pixels_x, pixels, &x0, &y0, &x1, &y1);
			stbtt_GetGlyphHMetrics(info, index, &advance, &bearing);
		}
		width = x1 - x0;
		height = y1 - y0;
		if (width > 0 && height > 0)
		{
			if (width + 2 * ATLAS_PADDING > ATLAS_SIZE || height + 2 * ATLAS_PADDING > ATLAS_SIZE)
				return 0;
			if (pack_x + width + 2 * ATLAS_PADDING > ATLAS_SIZE)
			{
				pack_x = 0;
				pack_y += pack_row_height;
				pack_row_height = 0;
			}
			if (pack_y + height + 2 * ATLAS_PADDING > ATLAS_SIZE)
			{
				atlas_reset(scale);
				return text_hires_glyph_cell(font, code, cell, glyph);
			}
			stbtt_MakeGlyphBitmap(info,
				atlas + (pack_y + ATLAS_PADDING) * ATLAS_SIZE + pack_x + ATLAS_PADDING,
				width, height, ATLAS_SIZE, pixels_x, pixels, index);
			if (pack_y < dirty_top)
				dirty_top = pack_y;
			if (pack_y + height + 2 * ATLAS_PADDING > dirty_bottom)
				dirty_bottom = pack_y + height + 2 * ATLAS_PADDING;
		}
		glyphs[slot].font = font;
		glyphs[slot].code = code;
		glyphs[slot].x = (short)(pack_x + ATLAS_PADDING);
		glyphs[slot].y = (short)(pack_y + ATLAS_PADDING);
		glyphs[slot].width = (short)(width > 0 && height > 0 ? width : 0);
		glyphs[slot].height = (short)(width > 0 && height > 0 ? height : 0);
		glyphs[slot].left = (short)x0;
		glyphs[slot].top = (short)y0;
		glyphs[slot].advance = advance * unit_scale;
		if (width > 0 && height > 0)
		{
			pack_x += width + 2 * ATLAS_PADDING;
			if (height + 2 * ATLAS_PADDING > pack_row_height)
				pack_row_height = height + 2 * ATLAS_PADDING;
		}
	}
	if (!glyphs[slot].width || !glyphs[slot].height)
		return 0;
	pixel_scale = scale * fonts[font].oversample;
	glyph->left = glyphs[slot].left / pixel_scale;
	glyph->top = glyphs[slot].top / pixel_scale;
	glyph->right = (glyphs[slot].left + glyphs[slot].width) / pixel_scale;
	glyph->bottom = (glyphs[slot].top + glyphs[slot].height) / pixel_scale;
	glyph->advance = glyphs[slot].advance;
	glyph->u0 = (float)glyphs[slot].x * placeholder_width / ATLAS_SIZE;
	glyph->v0 = (float)glyphs[slot].y * placeholder_height / ATLAS_SIZE;
	glyph->u1 = (float)(glyphs[slot].x + glyphs[slot].width) * placeholder_width / ATLAS_SIZE;
	glyph->v1 = (float)(glyphs[slot].y + glyphs[slot].height) * placeholder_height / ATLAS_SIZE;
	return 1;
}

void text_hires_register_atlas(const unsigned long *texture, unsigned long width, unsigned long height)
{
	placeholder_data = texture ? texture[1] : 0;
	placeholder_width = width;
	placeholder_height = height;
}

unsigned int text_hires_atlas_texture(unsigned long data)
{
	if (!data || data != placeholder_data || !atlas || !xgpu_gl_ready())
		return 0;
	if (!atlas_texture)
	{
		glGenTextures(1, &atlas_texture);
		glBindTexture(GL_TEXTURE_2D, atlas_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		xgpu_gl_state_invalidate();
		dirty_top = 0;
		dirty_bottom = ATLAS_SIZE;
	}
	if (dirty_top < dirty_bottom)
	{
		/* (white, the glyph's coverage its alpha, as the maps' fonts are) */
		long rows = dirty_bottom - dirty_top;
		unsigned char *texels = malloc((size_t)rows * ATLAS_SIZE * 4);

		if (texels)
		{
			long index;
			const unsigned char *coverage = atlas + (long)dirty_top * ATLAS_SIZE;

			for (index = 0; index < rows * ATLAS_SIZE; index++)
			{
				texels[index * 4 + 0] = texels[index * 4 + 1] = texels[index * 4 + 2] = 255;
				texels[index * 4 + 3] = coverage[index];
			}
			glBindTexture(GL_TEXTURE_2D, atlas_texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, dirty_top, ATLAS_SIZE, (GLsizei)rows, GL_RGBA, GL_UNSIGNED_BYTE, texels);
			xgpu_gl_state_invalidate();
			free(texels);
			dirty_top = ATLAS_SIZE;
			dirty_bottom = 0;
		}
	}
	return atlas_texture;
}
