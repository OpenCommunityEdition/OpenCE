/*
FONT_GROUP.C

symbols in this file:
0018C840 0070:
	_font_get_character_by_ascii_code (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "font_group.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* port: the font tags' characters (draw_string.c's and rasterizer_text.c's
struct font_character), of which this file makes some */
struct synthetic_font_character
{
	word character;
	short character_width;
	short bitmap_width;
	short bitmap_height;
	short bitmap_origin_x;
	short bitmap_origin_y;
	short hardware_character_index;
	short pad;
	long pixels_offset;
};

/* ---------- prototypes */

static struct font_character *font_get_tag_character(
	struct font_header *font,
	word character_code);

/* port/linux/src/text_hires.c */
extern int text_hires_cjk_covers(unsigned long code);
extern int text_hires_forced(void);
extern float text_hires_forced_advance(unsigned long code, float cap_height);

static short font_capital_height(
	struct font_header *font);

/* ---------- globals */

/* ---------- public code */

/* port: a font tag has no Chinese, Japanese or Korean characters, which the
high-res text's fallback font (Noto Sans CJK, for the subtitles' language) can
draw. Such a character is a cell of the font's line, a square of its ascent
and descent, drawn only by the high-res text: its pixels_offset is -1, which
the draws of the font tag's pixels leave out. The cells are made as they are
asked for, in a table that a character of another font, or one that shares
its place, overwrites; the draws take them one by one. */
#define SYNTHETIC_CHARACTER_COUNT 512

struct font_character *font_get_character_by_ascii_code(
	struct font_header *font,
	word character_code)
{
	static struct synthetic_font_character synthetic_characters[SYNTHETIC_CHARACTER_COUNT];
	struct synthetic_font_character *synthetic;
	struct font_character *character = font_get_tag_character(font, character_code);
	short cell;

	/* port: while the subtitles are drawn, a character is as wide as the
	high-res font's glyph (the table's copy of it, marked by -2 as its
	hardware character index, which only the high-res text draws) */
	if (character && text_hires_forced())
	{
		short capital = font_capital_height(font);
		float advance = capital > 0 ? text_hires_forced_advance(character_code, (float)capital) : 0.0f;

		if (advance > 0.0f)
		{
			synthetic = &synthetic_characters[((unsigned long)character_code * 2654435761UL ^ (unsigned long)font >> 4 ^ 0x5A5AUL) &
				(SYNTHETIC_CHARACTER_COUNT - 1)];
			*synthetic = *(struct synthetic_font_character *)character;
			synthetic->character_width = (short)(advance + 0.5f);
			/* (its box the glyph's cell, which the line breaks and clips by) */
			synthetic->bitmap_width = synthetic->character_width;
			synthetic->hardware_character_index = -2;

			return (struct font_character *)synthetic;
		}
	}
	if (character || character_code < 0x2E80)
		return character;
	cell = (short)(font->ascending_height + font->descending_height);
	if (cell <= 0 || !text_hires_cjk_covers(character_code))
		return NULL;
	synthetic = &synthetic_characters[((unsigned long)character_code * 2654435761UL ^ (unsigned long)font >> 4) &
		(SYNTHETIC_CHARACTER_COUNT - 1)];
	synthetic->character = character_code;
	synthetic->character_width = cell;
	synthetic->bitmap_width = cell;
	synthetic->bitmap_height = cell;
	synthetic->bitmap_origin_x = 0;
	synthetic->bitmap_origin_y = font->ascending_height;
	synthetic->hardware_character_index = NONE;
	synthetic->pad = 0;
	synthetic->pixels_offset = -1;

	return (struct font_character *)synthetic;
}

struct font_character *font_get_tag_character_unforced(
	struct font_header *font,
	word character_code)
{
	return font_get_tag_character(font, character_code);
}

/* ---------- private code */

/* port: the height of the font tag's capital H, its rows with ink (as
rasterizer_text.c sizes the high-res font to), 0 if it has none; the last
font's is kept */
static short font_capital_height(
	struct font_header *font)
{
	static struct font_header *last_font = NULL;
	static short last_height = 0;
	struct synthetic_font_character *capital;
	byte const *pixels;
	short top = NONE;
	short bottom = NONE;
	short row, column;

	if (font == last_font)
		return last_height;
	capital = (struct synthetic_font_character *)font_get_tag_character(font, 'H');
	if (capital && capital->pixels_offset >= 0)
	{
		pixels = (byte const *)font->pixels.address + capital->pixels_offset;
		for (row = 0; row < capital->bitmap_height; row++)
		{
			for (column = 0; column < capital->bitmap_width; column++)
			{
				if (pixels[row * capital->bitmap_width + column])
				{
					if (top == NONE)
						top = row;
					bottom = row;
					break;
				}
			}
		}
	}
	last_font = font;
	last_height = top == NONE ? 0 : (short)(bottom - top + 1);

	return last_height;
}

static struct font_character *font_get_tag_character(
	struct font_header *font,
	word character_code)
{
	struct font_character_table *character_table;
	struct font_character *character = NULL;

	/* port: no glyph for a character past the font's tables, a table that
	isn't a whole 256 entries, or an entry past the font's characters (the
	map's font, and any string's characters, e.g. a player's name) */
	if (!VALID_INDEX(character_code >> 8, font->character_tables.count))
		return NULL;

	character_table = TAG_BLOCK_GET_ELEMENT(
		&font->character_tables,
		character_code >> 8,
		struct font_character_table);

	if (character_table->character_indices.count == 256)
	{
		short *character_index = TAG_BLOCK_GET_ELEMENT(&character_table->character_indices, character_code & 0xFF, short);

		if (VALID_INDEX(*character_index, font->characters.count))
		{
			character = tag_block_get_element_with_size(
				&font->characters,
				*character_index,
				FONT_CHARACTER_SIZE);
		}
	}

	return character;
}
