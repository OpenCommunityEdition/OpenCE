/*
TEXT_HIRES.H

The game's text drawn with fonts at the display's resolution, in place of
the bitmap fonts of the maps (port/assets/fonts, embedded by
tools/embed_assets.py). The game lays its text out as before, from its font
tags' character widths; each character is drawn from a glyph of the font
that stands for its tag (fonts.json), rasterized with stb_truetype at as
many pixels as the display gives the 480 lines, into an atlas that the game
binds as a placeholder bitmap (source/rasterizer/rasterizer_text.c).
*/

#ifndef TEXT_HIRES_H
#define TEXT_HIRES_H

/* an embedded font, and the font tag it draws (by name) */
struct text_hires_embedded
{
	const char *tag;
	const char *file;
	const unsigned int *data;
	unsigned int size;
};

extern const struct text_hires_embedded text_hires_embedded[];
extern const unsigned int text_hires_embedded_count;

/* an embedded font of the characters that the fonts above lack (Chinese,
Japanese and Korean), and the subtitles' language (display.subtitles_language)
that it draws them for (fonts.json's fallbacks) */
struct text_hires_fallback
{
	const char *language;
	const char *file;
	const unsigned int *data;
	unsigned int size;
};

extern const struct text_hires_fallback text_hires_fallbacks[];
extern const unsigned int text_hires_fallbacks_count;

/* a glyph: its quad, in units of the 480 lines from the pen on the baseline,
its texels in the atlas, in texels of the atlas's placeholder bitmap, and
the font's advance for it, in units */
struct text_hires_glyph
{
	float left, top, right, bottom;
	float u0, v0, u1, v1;
	float advance;
};

/* the letters a font is fitted to its tag by: A to Z, then a to z */
#define TEXT_HIRES_FIT_LETTERS 52

/* The committed setting; checked before looking up cached font choices. */
int text_hires_enabled(void);

/* the font standing for a font tag, sized so that its capitals are
cap_height units tall and narrowed where its letters are wider than the
tag's letter_widths (TEXT_HIRES_FIT_LETTERS of them, 0 for a letter the tag
has not), its glyphs rasterized with oversample times the display's pixels
(for text drawn scaled up); -1 if none, or display.high_res_text is off */
long text_hires_font(char const *tag_name, float cap_height, short const *letter_widths, float oversample);
/* while on, a font tag that no font stands for (fonts.json) is drawn with the
HUD's small font, as the subtitles are, in a font tag of their own (the text
drawn meanwhile has its fonts apart from the rest of the text's) */
void text_hires_force(int on);
int text_hires_forced(void);
/* while forced, the small font's advance for a character, in units of the 480
lines, with its capitals cap_height units tall (as a font tag's text is sized);
0 if it has none or the high-res text is not on (the layout of the subtitles
is the glyphs' own, not the font tag's character widths) */
float text_hires_forced_advance(unsigned long code, float cap_height);
/* whether the font has a glyph for the character (it draws a string only
if it has all of its characters) */
int text_hires_covers(long font, unsigned long code);
/* the glyph of a character (rasterized into the atlas if it is not yet);
0 if it has none */
int text_hires_glyph(long font, unsigned long code, struct text_hires_glyph *glyph);
/* the same, for a character the font lacks, that the language's fallback font
draws in a cell of cell units (its width and height; 0 for a character
that the font has) */
int text_hires_glyph_cell(long font, unsigned long code, float cell, struct text_hires_glyph *glyph);
/* whether the fallback font of the subtitles' language has a glyph for the
character, which the font tags, without one, are given a cell for
(font_group.c); 0 while high-res text is off or its atlas is not there */
int text_hires_cjk_covers(unsigned long code);
/* the atlas's placeholder: the D3D texture of the bitmap the game binds,
and its size; NULL forgets it */
void text_hires_register_atlas(const unsigned long *texture, unsigned long width, unsigned long height);

/* the GL texture of the atlas when data (a D3D texture's Data) is its
placeholder's, its rasterized glyphs uploaded; 0 otherwise */
unsigned int text_hires_atlas_texture(unsigned long data);

/* a function called before the atlas is emptied, which draws what has been
batched from it (rasterizer_xbox_text.c's rasterizer_text_flush) */
void text_hires_set_reset_hook(void (*hook)(void));
/* d3d8_gl.c: the display's pixels for each of the 480 lines */
float halo_screen_pixel_scale(void);

#endif
