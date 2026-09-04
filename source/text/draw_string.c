/*
DRAW_STRING.C

symbols in this file:
0018AB70 0010:
	_draw_string_initialize (0000)
0018AB80 0070:
	_draw_string_initialize_for_new_map (0000)
0018ABF0 0010:
	_draw_string_dispose_from_old_map (0000)
0018AC00 0010:
	_draw_string_dispose (0000)
0018AC10 0070:
	_text_bounds_draw_character (0000)
0018AC80 00e0:
	_text_pick_draw_character (0000)
0018AD60 0050:
	_draw_string_get_string (0000)
0018ADB0 0070:
	_draw_string_set_tab_stops (0000)
0018AE20 0070:
	_draw_string_set_indents (0000)
0018AE90 0150:
	_draw_string_set_color (0000)
0018AFE0 0050:
	_draw_string_get_color (0000)
0018B030 0020:
	_draw_string_set_font (0000)
0018B050 00b0:
	_draw_string_set_format (0000)
0018B100 0040:
	_draw_string_set_draw_mode (0000)
0018B140 0020:
	_draw_string_set_highlight (0000)
0018B160 03b0:
	_bitmap_draw_character (0000)
0018B510 0070:
	_styled_font_get (0000)
0018B580 0100:
	_parse_string_new (0000)
0018B680 0270:
	_parse_string (0000)
0018B8F0 0110:
	_parse_unicode_string (0000)
0018BA00 0210:
	_draw_string_partial (0000)
0018BC10 0210:
	_draw_unicode_string_partial (0000)
0018BE20 0390:
	_draw_string (0000)
0018C1B0 0390:
	_draw_unicode_string (0000)
0018C540 00c0:
	_draw_string_compute_bounds (0000)
0018C600 00c0:
	_draw_unicode_string_compute_bounds (0000)
0018C6C0 0050:
	_draw_string_pick (0000)
0018C710 0130:
	_bitmap_draw_string (0000)
002A26E4 002d:
	??_C@_0CN@NJAONNEI@internal?5string?5localization?5tag@ (0000)
002A2714 0029:
	??_C@_0CJ@LDLNPDDM@index?$DO?$DN0?5?$CG?$CG?5index?$DMNUMBER_OF_TEXT@ (0000)
002A2740 0022:
	??_C@_0CC@EBHGCODL@c?3?2halo?2SOURCE?2text?2draw_string?4@ (0000)
002A2764 002e:
	??_C@_0CO@LAOAKNHK@count?$DO?$DN0?5?$CG?$CG?5count?$DMMAXIMUM_NUMBER@ (0000)
002A2794 0014:
	??_C@_0BE@FGPECGKA@paragraph_indent?$DO?$DN0?$AA@ (0000)
002A27A8 0012:
	??_C@_0BC@DNGFHNA@initial_indent?$DO?$DN0?$AA@ (0000)
002A27BC 002d:
	??_C@_0CN@CGOGANEE@?$CIcolor?9?$DOblue?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9?$DO@ (0000)
002A27EC 002f:
	??_C@_0CP@FADKAPOG@?$CIcolor?9?$DOgreen?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9@ (0000)
002A281C 002b:
	??_C@_0CL@COFHKNKB@?$CIcolor?9?$DOred?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9?$DOr@ (0000)
002A2848 002f:
	??_C@_0CP@DFGKLIFB@?$CIcolor?9?$DOalpha?5?$DO?$DN?50?4f?$CJ?5?$CG?$CG?5?$CIcolor?9@ (0000)
002A2878 0040:
	??_C@_0EA@DLFMIHAM@justification?$DO?$DN0?5?$CG?$CG?5justificatio@ (0000)
002A28B8 0046:
	??_C@_0EG@IPKHNIBH@style?$DN?$DN_text_style_plain?5?$HM?$HM?5?$CIsty@ (0000)
002A2900 0029:
	??_C@_0CJ@CIJKMAAN@VALID_FLAGS?$CIflags?0?5NUMBER_OF_TEX@ (0000)
002A292C 003e:
	??_C@_0DO@NHIHGJAE@x0?$DO?$DN0?5?$CG?$CG?5x0?$CLdx?$DM?$DNdraw_character_s@ (0000)
002A296C 003c:
	??_C@_0DM@BMLHMCCB@y0?$DO?$DN0?5?$CG?$CG?5y0?$DM?$DNdraw_character_soft@ (0000)
002A29A8 000d:
	??_C@_0N@HNKPGPMM@result?$CB?$DNNONE?$AA@ (0000)
002A29B8 0049:
	??_C@_0EJ@CJKOFANJ@tab_stop_index?$DO?$DN0?5?$CG?$CG?5tab_stop_in@ (0000)
004C1908 00f8:
	_draw_string_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "interface/interface.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "text/international_strings.h"
#include "text/text_group.h"

#include <ctype.h>

/* ---------- constants */

enum
{
	NUMBER_OF_TEXT_STRINGS = 1,
	NUMBER_OF_TEXT_FLAGS = 4,
	MAXIMUM_NUMBER_OF_TAB_STOPS = 16,

	_text_justification_left = 0,
	_text_justification_right,
	_text_justification_center,
	NUMBER_OF_TEXT_JUSTIFICATIONS,

	_draw_text_wrap_horizontally_bit = 0,
	_draw_text_wrap_vertically_bit,

	_parsed_end_of_string = 0,
	_parsed_end_of_line = 1,
	_parsed_end_of_word = 2,
	_parsed_end_of_column = 3,
	_parsed_justification_change = 4,
	_parsed_color_change = 5,
	_parsed_character = 6,
	_parsed_style_change = 7,
	_parsed_ascii_character = 2,
	_parsed_ascii_word_break = 6,

	_string_index_can_end_words = 4,
	_string_index_cannot_end_words = 5,
	_string_index_cannot_begin_words = 6,

	_bitmap_format_a8 = 0,
	_bitmap_format_y8 = 1,
	_bitmap_format_ay8 = 2,
	_bitmap_format_r5g6b5 = 6,
	_bitmap_format_a8r8g8b8 = 11,
};

/* ---------- macros */

#define draw_character_software_globals draw_string_globals

/* ---------- structures */

struct draw_string_globals
{
	struct bitmap_data *bitmap;
	byte encoding_shift;
	byte __pad05[3];
	point2d pick_point;
	short best_pick_string_index;
	short best_pick_distance;
	short last_string_index;
	byte __pad12[2];
	rectangle2d text_bounds;
	struct font_header *last_font_header;
	long localization_string_list_index;
	long font_index;
	unsigned long flags;
	short style;
	short justification;
	real_argb_color color;
	short tab_stop_count;
	short tab_stops[16];
	short highlight_start;
	short highlight_end;
	short initial_indent;
	short paragraph_indent;
	byte __pad6A[2];
	byte multitexture_parameters[0x8C];
};

struct font_character
{
	word character;
	short character_width;
	short bitmap_width;
	short bitmap_height;
	short bitmap_origin_x;
	short bitmap_origin_y;
	short hardware_character_index;
	word pad;
	long pixels_offset;
};

struct parse_string_state
{
	long base_font_index;
	struct font_header *font_header;
	byte *string;
	short string_index;
	short style;
	short justification;
	word character;
	short result;
	word pad;
	pixel32 color;
};

/* ---------- prototypes */

static struct font_header *styled_font_get(
	short style,
	long font_index);
static void parse_string_new(
	struct parse_string_state *state,
	char const *string,
	long font_index,
	short style,
	short justification,
	real_argb_color const *color);
static short parse_string(
	struct parse_string_state *state);
static word parse_unicode_string(
	struct parse_string_state *state);
static void draw_string_partial(
	draw_character_proc draw_character,
	point2d *cursor,
	rectangle2d const *bounds,
	rectangle2d const *clip,
	pixel32 color,
	char const *string,
	short string_index,
	short string_length);
static void draw_unicode_string_partial(
	draw_character_proc draw_character,
	point2d *cursor,
	rectangle2d const *bounds,
	rectangle2d const *clip,
	pixel32 color,
	wchar_t const *string,
	short string_index,
	short string_length);

/* ---------- globals */

struct draw_string_globals draw_string_globals = { 0 };


/* ---------- public code */

void draw_string_initialize(
	void)
{
	return;
}

void draw_string_initialize_for_new_map(
	void)
{
	long localization_string_list_index = interface_get_tag_index(_interface_string_list_localization);
	draw_string_globals.localization_string_list_index = localization_string_list_index;

	if (localization_string_list_index != NONE)
	{
		set_language_code((short)atoi(string_list_get_string(localization_string_list_index, 0)));
		draw_string_globals.tab_stop_count = 0;
		draw_string_globals.flags = 0;
		draw_string_globals.justification = _text_justification_left;
		draw_string_globals.initial_indent = 0;
		draw_string_globals.paragraph_indent = 0;
		draw_string_globals.font_index = NONE;
	}
	else
	{
		error(_error_immediate, "internal string localization tag is missing.");
	}

	return;
}

void draw_string_dispose_from_old_map(
	void)
{
	draw_string_globals.localization_string_list_index = NONE;
	return;
}

void draw_string_dispose(
	void)
{
	return;
}

static void text_bounds_draw_character(
	struct parse_string_state *state,
	struct font_header *font,
	struct font_character *character,
	pixel32 color,
	short x0,
	short y0,
	short x,
	short y,
	short dx,
	short dy)
{
	short x1 = (short)(x0 + dx);
	short y1 = (short)(y0 + dy);

	if (x0 < draw_string_globals.text_bounds.x0)
		draw_string_globals.text_bounds.x0 = x0;
	if (y0 < draw_string_globals.text_bounds.y0)
		draw_string_globals.text_bounds.y0 = y0;
	if (x1 > draw_string_globals.text_bounds.x1)
		draw_string_globals.text_bounds.x1 = x1;
	if (y1 > draw_string_globals.text_bounds.y1)
		draw_string_globals.text_bounds.y1 = y1;
	draw_string_globals.last_font_header = font;

	return;
}

static void text_pick_draw_character(
	struct parse_string_state *state,
	struct font_header *font,
	struct font_character *character,
	pixel32 color,
	short x0,
	short y0,
	short x,
	short y,
	short dx,
	short dy)
{
	short x1 = (short)(x0 + dx);
	short y1 = (short)(y0 + dy);
	short distance_x0 = (short)ABS((short)(x0 - draw_string_globals.pick_point.x));
	short distance_x1 = (short)ABS((short)(x1 - draw_string_globals.pick_point.x));
	short distance_y0 = (short)ABS((short)(y0 - draw_string_globals.pick_point.y));
	short distance_y1 = (short)ABS((short)(y1 - draw_string_globals.pick_point.y));
	short maximum_x_distance = distance_x0 > distance_x1 ? distance_x0 : distance_x1;
	short distance = maximum_x_distance <= distance_y0 ? distance_y0 : maximum_x_distance;
	distance = distance <= distance_y1 ? distance_y1 : distance;

	if (distance < draw_string_globals.best_pick_distance)
	{
		draw_string_globals.best_pick_distance = distance;
		if (draw_string_globals.pick_point.x - x0 < ((x1 - x0) >> 1))
		{
			draw_string_globals.best_pick_string_index = draw_string_globals.last_string_index;
			draw_string_globals.last_string_index = state->string_index;
			return;
		}

		draw_string_globals.best_pick_string_index = state->string_index;
		draw_string_globals.last_string_index = state->string_index;
		return;
	}

	draw_string_globals.last_string_index = state->string_index;

	return;
}

char *draw_string_get_string(
	short index)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 341, index>=0 && index<NUMBER_OF_TEXT_STRINGS);

	return string_list_get_string(draw_string_globals.localization_string_list_index, index + 7);
}

void draw_string_set_tab_stops(
	short const *tab_stops,
	short count)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 350, count>=0 && count<MAXIMUM_NUMBER_OF_TAB_STOPS);

	draw_string_globals.tab_stop_count = MIN(count, MAXIMUM_NUMBER_OF_TAB_STOPS);

	if (draw_string_globals.tab_stop_count > 0)
	{
		csmemcpy(draw_string_globals.tab_stops, tab_stops, draw_string_globals.tab_stop_count * sizeof(short));
	}

	return;
}

void draw_string_set_indents(
	short initial_indent,
	short paragraph_indent)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 366, initial_indent>=0);
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 367, paragraph_indent>=0);

	draw_string_globals.paragraph_indent = paragraph_indent;
	draw_string_globals.initial_indent = initial_indent;

	return;
}

void draw_string_set_color(
	real_argb_color const *color)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 378, color);
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 379, (color->alpha >= 0.f) && (color->alpha <= 1.f));
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 380, (color->red >= 0.f) && (color->red <= 1.f));
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 381, (color->green >= 0.f) && (color->green <= 1.f));
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 382, (color->blue >= 0.f) && (color->blue <= 1.f));

	draw_string_globals.color = *color;

	return;
}

void draw_string_get_color(
	real_argb_color *color)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 392, color);

	*color = draw_string_globals.color;

	return;
}

void draw_string_set_font(
	long font_index)
{
	font_definition_get(font_index);
	draw_string_globals.font_index = font_index;

	return;
}

void draw_string_set_format(
	short style,
	short justification,
	unsigned long flags)
{
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 410, VALID_FLAGS(flags, NUMBER_OF_TEXT_FLAGS));
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 411, style==_text_style_plain || (style>=0 && style<NUMBER_OF_TEXT_STYLES));
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 412, justification>=0 && justification<NUMBER_OF_TEXT_JUSTIFICATIONS);

	draw_string_globals.style = style;
	draw_string_globals.justification = justification;
	draw_string_globals.flags = flags;

	return;
}

void draw_string_set_draw_mode(
	long font_index,
	short style,
	short justification,
	unsigned long flags,
	real_argb_color const *color)
{
	draw_string_set_font(font_index);
	draw_string_set_color(color);
	draw_string_set_format(style, justification, flags);

	return;
}

void draw_string_set_highlight(
	short start,
	short end)
{
	draw_string_globals.highlight_start = start;
	draw_string_globals.highlight_end = end;

	return;
}

/* ---------- private code */

static void bitmap_draw_character(
	struct parse_string_state *state,
	struct font_header *font,
	struct font_character *character,
	pixel32 color,
	short x0,
	short y0,
	short x,
	short y,
	short dx,
	short dy)
{
	unsigned long coverage_scale = color >> 24;
	byte *glyph_pixels = (byte *)font->pixels.address + character->pixels_offset;
	short format = draw_character_software_globals.bitmap->format;
	pixel32 destination_color = color;
	short row;

	if (format == _bitmap_format_r5g6b5)
	{
		destination_color =
			(8 * (((color >> 11) & 0xFF00) | ((color >> 8) & 0xFC))) |
			((byte)color >> 3);
	}

	if (dy <= 0)
		return;

	for (row = dy; row > 0; row--, y++, y0++)
	{
		short bits_per_pixel = bitmap_format_get_bits_per_pixel(
			draw_character_software_globals.bitmap->format);
		byte *source_pixel = &glyph_pixels[character->bitmap_width * y + x];
		byte *destination_pixel = (byte *)draw_character_software_globals.bitmap->base_address +
			(x0 << draw_character_software_globals.encoding_shift) +
			y0 * (bits_per_pixel * draw_character_software_globals.bitmap->width / 8);
		short column;

		match_assert(
			"c:\\halo\\SOURCE\\text\\draw_string.c",
			495,
			y0>=0 && y0<=draw_character_software_globals.bitmap->height);
		match_assert(
			"c:\\halo\\SOURCE\\text\\draw_string.c",
			496,
			x0>=0 && x0+dx<=draw_character_software_globals.bitmap->width);

		switch (format)
		{
		case _bitmap_format_a8:
		case _bitmap_format_y8:
		case _bitmap_format_ay8:
			for (column = dx; column > 0; column--, source_pixel++, destination_pixel++)
			{
				if (*source_pixel)
				{
					byte destination_value = *destination_pixel;
					short coverage = (short)(((short)coverage_scale * *source_pixel) >> 8);
					coverage = MIN(coverage, destination_value);
					*destination_pixel = (byte)coverage;
				}
			}
			break;

		case _bitmap_format_r5g6b5:
			for (column = dx; column > 0; column--, source_pixel++, destination_pixel += 2)
			{
				if (*source_pixel)
				{
					short coverage = (short)(((short)coverage_scale * *source_pixel) >> 8);
					short inverse = (short)(255 - coverage);
					word destination = *(word *)destination_pixel;
					*(word *)destination_pixel =
						(word)((((destination * inverse + (word)destination_color * coverage) >> 8) & 0xF800) |
						((((destination & 0x1F) * inverse + ((word)destination_color & 0x1F) * coverage) >> 8) & 0x1F) |
						((((destination & 0x7FF) * inverse + ((word)destination_color & 0x7FF) * coverage) >> 8) & 0x7E0));
				}
			}
			break;

		case _bitmap_format_a8r8g8b8:
			for (column = dx; column > 0; column--, source_pixel++, destination_pixel += 4)
			{
				if (*source_pixel)
				{
					pixel32 destination = *(pixel32 *)destination_pixel;
					short coverage = (short)(((short)coverage_scale * *source_pixel) >> 8);
					short inverse = (short)(255 - coverage);
					pixel32 destination_alpha = MAX((pixel32)coverage, destination >> 24);
					pixel32 red;
					pixel32 green;
					pixel32 blue;

					red = ((((destination >> 16) & 0xFF) * inverse) & 0xFFFFFF00) +
						((((color >> 16) & 0xFF) * coverage) & 0xFFFFFF00);
					green = ((((destination >> 8) & 0xFF) * inverse) & 0xFFFFFF00) +
						((((color >> 8) & 0xFF) * coverage) & 0xFFFFFF00);
					blue = (((destination & 0xFF) * inverse) >> 8) +
						(((color & 0xFF) * coverage) >> 8);
					*(pixel32 *)destination_pixel =
						(red << 8) | green | blue | (destination_alpha << 24);
				}
			}
			break;

		default:
			display_assert(
				"### ERROR unsupported bitmap format",
				"c:\\halo\\SOURCE\\text\\draw_string.c",
				565,
				TRUE);
			system_exit(-1);
			break;
		}
	}

	return;
}

static struct font_header *styled_font_get(
	short style,
	long font_index)
{
	long styled_font_index;

	if (style != _text_style_plain)
	{
		struct font_header *font;

		match_assert(
			"c:\\halo\\SOURCE\\text\\draw_string.c",
			1039,
			style==_text_style_plain || (style>=0 && style<NUMBER_OF_TEXT_STYLES));
		font = font_definition_get(font_index);
		styled_font_index = font->style_fonts[style].index;
	}
	else
	{
		styled_font_index = font_index;
	}

	if (styled_font_index == NONE)
		styled_font_index = font_index;

	return font_definition_get(styled_font_index);
}

static void parse_string_new(
	struct parse_string_state *state,
	char const *string,
	long font_index,
	short style,
	short justification,
	real_argb_color const *color)
{
	pixel32 packed_color;

	match_assert(
		"c:\\halo\\SOURCE\\text\\draw_string.c",
		1054,
		style==_text_style_plain || (style>=0 && style<NUMBER_OF_TEXT_STYLES));
	match_assert(
		"c:\\halo\\SOURCE\\text\\draw_string.c",
		1055,
		justification>=0 && justification<NUMBER_OF_TEXT_JUSTIFICATIONS);

	state->base_font_index = font_index;
	state->string = (byte *)string;
	state->justification = justification;
	state->string_index = 0;
	state->style = style;

	packed_color = (long)(color->alpha * 255.f);
	packed_color = (packed_color << 8) | (long)(color->red * 255.f);
	packed_color = (packed_color << 8) | (long)(color->green * 255.f);
	packed_color = (packed_color << 8) | (long)(color->blue * 255.f);
	state->color = packed_color;
	state->font_header = styled_font_get(style, font_index);

	return;
}

static short parse_string(
	struct parse_string_state *state)
{
	short *string_index = &state->string_index;
	word next_character;
	long result;

	do
	{
		next_character = get_next_character(state->string, string_index);
		result = NONE;

		if ((next_character & 0xFF00) == 0x7C00)
		{
			switch (tolower((byte)next_character))
			{
			case 'b': result = _parsed_style_change; state->style = _text_style_bold; break;
			case 'c': result = _parsed_justification_change; state->justification = _text_justification_center; break;
			case 'i': result = _parsed_style_change; state->style = _text_style_italic; break;
			case 'k': result = _parsed_style_change; state->style = _text_style_condense; break;
			case 'l': result = _parsed_justification_change; state->justification = _text_justification_left; break;
			case 'n': result = _parsed_end_of_line; break;
			case 'p': result = _parsed_style_change; state->style = _text_style_plain; break;
			case 'r': result = _parsed_justification_change; state->justification = _text_justification_right; break;
			case 't': result = _parsed_end_of_column; break;
			case 'u': result = _parsed_style_change; state->style = _text_style_underline; break;
			}
		}

		if ((short)result == NONE)
		{
			word following_character;
			short following_index;
			char *whitespace_pattern;
			char *break_before_pattern;
			char *break_after_pattern;

			switch (next_character)
			{
			case '\0': result = _parsed_end_of_string; continue;
			case '\t': result = _parsed_end_of_column; continue;
			case '\r': result = _parsed_end_of_line; continue;
			}

			following_index = *string_index;
			following_character = get_next_character(state->string, &following_index);
			whitespace_pattern = string_list_get_string(draw_string_globals.localization_string_list_index, _string_index_can_end_words);
			break_before_pattern = string_list_get_string(draw_string_globals.localization_string_list_index, _string_index_cannot_end_words);
			break_after_pattern = string_list_get_string(draw_string_globals.localization_string_list_index, _string_index_cannot_begin_words);

			if ((next_character & 0xFF00) != 0)
				goto check_break_before;
			if (character_in_pattern(next_character, whitespace_pattern))
			{
				result = _parsed_ascii_word_break;
				continue;
			}
			if ((next_character & 0xFF00) != 0)
			{
check_break_before:
				if (character_in_pattern(next_character, break_before_pattern))
				{
					result = _parsed_ascii_word_break;
					continue;
				}
			}
			if (character_in_pattern(following_character, break_after_pattern))
			{
				result = _parsed_ascii_word_break;
				continue;
			}
			result = _parsed_ascii_character;
		}
		else if ((short)result == _parsed_style_change)
		{
			state->font_header = styled_font_get(state->style, state->base_font_index);
		}
	}
	while ((short)result == _parsed_style_change || (short)result == _parsed_color_change);

	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 1203, result!=NONE);
	state->result = result;
	state->character = next_character;
	return result;
}

static word parse_unicode_string(
	struct parse_string_state *state)
{
	wchar_t const *string = (wchar_t const *)state->string;
	short string_index = state->string_index;
	word character = string[string_index];

	state->character = character;
	state->string_index = string_index + 1;

	switch (character)
	{
	case '|':
		if (string[state->string_index++] == 'n')
		{
			state->character = '\r';
			state->result = _parsed_end_of_line;
		}
		break;
	case 0:
		state->result = _parsed_end_of_string;
		break;
	case '\t':
		state->result = _parsed_end_of_column;
		break;
	case '\r':
		state->result = _parsed_end_of_line;
		break;
	default:
		state->result = _parsed_character;
		break;
	}

	return state->result;
}

static void draw_string_partial(
	draw_character_proc draw_character,
	point2d *cursor,
	rectangle2d const *bounds,
	rectangle2d const *clip,
	pixel32 color,
	char const *string,
	short string_index,
	short string_length)
{
	long clip_left = SHORT_MIN;
	long clip_top = SHORT_MIN;
	short clip_right = SHORT_MAX;
	short clip_bottom = SHORT_MAX;
	long region_right;
	long region_bottom;
	struct parse_string_state state;

	if (bounds)
	{
		if (bounds->x0 != SHORT_MIN)
			clip_left = (word)bounds->x0;
		if (bounds->x1 != SHORT_MAX)
			clip_right = bounds->x1;
		if (bounds->y0 != SHORT_MIN)
			clip_top = (word)bounds->y0;
		if (bounds->y1 != SHORT_MAX)
			clip_bottom = bounds->y1;
	}

	if (clip)
	{
		if (clip->x0 > (short)clip_left)
			clip_left = (word)clip->x0;
		if (clip->x1 < clip_right)
			clip_right = clip->x1;
		if (clip->y0 > (short)clip_top)
			clip_top = (word)clip->y0;
		if (clip->y1 < clip_bottom)
			clip_bottom = clip->y1;
	}

	region_right = clip_right;
	if ((short)clip_left >= clip_right)
		return;
	region_bottom = clip_bottom;
	if ((short)clip_top >= clip_bottom)
		return;

	parse_string_new(
		&state,
		string,
		draw_string_globals.font_index,
		draw_string_globals.style,
		draw_string_globals.justification,
		&draw_string_globals.color);

	for (state.string_index = string_index; state.string_index < string_length; )
	{
		long character_index = state.string_index;
		pixel32 character_color =
			character_index >= draw_string_globals.highlight_start &&
			character_index < draw_string_globals.highlight_end
				? color ^ 0xFFFFFF
				: color;
		struct font_character *character;
		long source_x = 0;
		long source_y = 0;
		short bitmap_width;
		short bitmap_height;
		short cursor_y;
		long destination_x;
		long destination_x_unclipped;
		long destination_y;

		parse_string(&state);
		character = font_get_character_by_ascii_code(state.font_header, state.character);
		if (!character)
			continue;

		bitmap_width = character->bitmap_width;
		bitmap_height = character->bitmap_height;
		cursor_y = cursor->y;
		destination_x = (short)(cursor->x - character->bitmap_origin_x);
		destination_x_unclipped = destination_x;
		destination_y = (short)(cursor_y - character->bitmap_origin_y);

		cursor->x = (short)(character->character_width + cursor->x);

		if (bitmap_width + destination_x > region_right)
			bitmap_width = (short)(region_right - destination_x);
		if (destination_x < (short)clip_left)
		{
			destination_x = clip_left;
			source_x = (short)(clip_left - destination_x_unclipped);
			bitmap_width = (short)(bitmap_width - source_x);
		}

		if (bitmap_height + (short)destination_y > region_bottom)
			bitmap_height = (short)(region_bottom - destination_y);
		if ((short)destination_y < (short)clip_top)
		{
			destination_y = clip_top;
			source_y = (short)(clip_top - (cursor_y - character->bitmap_origin_y));
			bitmap_height = (short)(bitmap_height - source_y);
		}

		if (bitmap_width > 0 && bitmap_height > 0)
		{
			draw_character(
				&state,
				state.font_header,
				character,
				character_color,
				(short)destination_x,
				(short)destination_y,
				(short)source_x,
				(short)source_y,
				bitmap_width,
				bitmap_height);
		}
	}

	return;
}

static void draw_unicode_string_partial(
	draw_character_proc draw_character,
	point2d *cursor,
	rectangle2d const *bounds,
	rectangle2d const *clip,
	pixel32 color,
	wchar_t const *string,
	short string_index,
	short string_length)
{
	long clip_left = SHORT_MIN;
	long clip_top = SHORT_MIN;
	short clip_right = SHORT_MAX;
	short clip_bottom = SHORT_MAX;
	long region_right;
	long region_bottom;
	struct parse_string_state state;

	if (bounds)
	{
		if (bounds->x0 != SHORT_MIN)
			clip_left = (word)bounds->x0;
		if (bounds->x1 != SHORT_MAX)
			clip_right = bounds->x1;
		if (bounds->y0 != SHORT_MIN)
			clip_top = (word)bounds->y0;
		if (bounds->y1 != SHORT_MAX)
			clip_bottom = bounds->y1;
	}

	if (clip)
	{
		if (clip->x0 > (short)clip_left)
			clip_left = (word)clip->x0;
		if (clip->x1 < clip_right)
			clip_right = clip->x1;
		if (clip->y0 > (short)clip_top)
			clip_top = (word)clip->y0;
		if (clip->y1 < clip_bottom)
			clip_bottom = clip->y1;
	}

	region_right = clip_right;
	if ((short)clip_left >= clip_right)
		return;
	region_bottom = clip_bottom;
	if ((short)clip_top >= clip_bottom)
		return;

	parse_string_new(
		&state,
		(char const *)string,
		draw_string_globals.font_index,
		draw_string_globals.style,
		draw_string_globals.justification,
		&draw_string_globals.color);

	for (state.string_index = string_index; state.string_index < string_length; )
	{
		long character_index = state.string_index;
		pixel32 character_color =
			character_index >= draw_string_globals.highlight_start &&
			character_index < draw_string_globals.highlight_end
				? color ^ 0xFFFFFF
				: color;
		struct font_character *character;
		long source_x = 0;
		long source_y = 0;
		short bitmap_width;
		short bitmap_height;
		short cursor_y;
		long destination_x;
		long destination_x_unclipped;
		long destination_y;

		parse_unicode_string(&state);
		character = font_get_character_by_ascii_code(state.font_header, state.character);
		if (!character)
			continue;

		bitmap_width = character->bitmap_width;
		bitmap_height = character->bitmap_height;
		cursor_y = cursor->y;
		destination_x = (short)(cursor->x - character->bitmap_origin_x);
		destination_x_unclipped = destination_x;
		destination_y = (short)(cursor_y - character->bitmap_origin_y);

		cursor->x = (short)(character->character_width + cursor->x);

		if (bitmap_width + destination_x > region_right)
			bitmap_width = (short)(region_right - destination_x);
		if (destination_x < (short)clip_left)
		{
			destination_x = clip_left;
			source_x = (short)(clip_left - destination_x_unclipped);
			bitmap_width = (short)(bitmap_width - source_x);
		}

		if (bitmap_height + (short)destination_y > region_bottom)
			bitmap_height = (short)(region_bottom - destination_y);
		if ((short)destination_y < (short)clip_top)
		{
			destination_y = clip_top;
			source_y = (short)(clip_top - (cursor_y - character->bitmap_origin_y));
			bitmap_height = (short)(bitmap_height - source_y);
		}

		if (bitmap_width > 0 && bitmap_height > 0)
		{
			draw_character(
				&state,
				state.font_header,
				character,
				character_color,
				(short)destination_x,
				(short)destination_y,
				(short)source_x,
				(short)source_y,
				bitmap_width,
				bitmap_height);
		}
	}

	return;
}

void draw_string(
	draw_character_proc draw_character,
	rectangle2d const *bounds,
	point2d *cursor_reference,
	rectangle2d const *clip,
	short height_adjust,
	char const *string)
{
	struct parse_string_state state;
	point2d cursor;
	long tab_stop_index = 0;
	long paragraph_line_offset = 0;
	long wrapped_line_index = 0;
	long maximum_wrapped_line_index = 0;
	short string_index;
	struct font_header *font;

	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 648, bounds);
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 649, string);

	parse_string_new(
		&state,
		string,
		draw_string_globals.font_index,
		draw_string_globals.style,
		draw_string_globals.justification,
		&draw_string_globals.color);
	string_index = state.string_index;
	font = state.font_header;

	do
	{
		short tab_stop_count = draw_string_globals.tab_stop_count;
		short justification = state.justification;
		long line_width = 0;
		long break_string_index = 0;
		long break_line_width = 0;
		short previous_result = NONE;
		long segment_start_index = string_index;
		rectangle2d line_bounds = *bounds;
		boolean segment_done = FALSE;
		long segment_end_index = 0;
		short line_left;
		short ascending_height;
		long cursor_x_start;
		long line_top;
		long baseline_y;

		if (tab_stop_count > 0)
		{
			match_assert(
				"c:\\halo\\SOURCE\\text\\draw_string.c",
				681,
				tab_stop_index>=0 && tab_stop_index<=draw_string_globals.tab_stop_count);

			if (tab_stop_index != 0)
				line_left = draw_string_globals.tab_stops[tab_stop_index - 1];
			else if (paragraph_line_offset != 0)
				line_left = (short)(line_bounds.x0 + draw_string_globals.paragraph_indent);
			else
				line_left = (short)(line_bounds.x0 + draw_string_globals.initial_indent);

			line_bounds.x0 = line_left;
			if (tab_stop_index < tab_stop_count)
				line_bounds.x1 = draw_string_globals.tab_stops[tab_stop_index];
		}
		else
		{
			if (paragraph_line_offset != 0)
				line_left = (short)(line_bounds.x0 + draw_string_globals.paragraph_indent);
			else
				line_left = (short)(line_bounds.x0 + draw_string_globals.initial_indent);
			line_bounds.x0 = line_left;
		}

		ascending_height = font->ascending_height;
		cursor_x_start = font->leading_width + line_left;
		line_top =
			(font->leading_height + font->descending_height + height_adjust + ascending_height) *
			(wrapped_line_index + paragraph_line_offset) +
			line_bounds.y0;
		cursor.x = (short)cursor_x_start;
		baseline_y = line_top + ascending_height;
		cursor.y = (short)baseline_y;

		do
		{
			boolean wrapped = FALSE;
			struct font_character *character;

			parse_string(&state);
			if (state.result != _parsed_ascii_character && state.result != _parsed_ascii_word_break)
			{
				segment_done = TRUE;
			}
			else
			{
				character = font_get_character_by_ascii_code(state.font_header, state.character);
				if (character)
				{
					if (state.result != _parsed_ascii_character && previous_result == _parsed_ascii_character)
					{
						break_string_index = segment_end_index;
						break_line_width = line_width;
					}

					if (character->bitmap_width + cursor_x_start + (short)line_width < line_bounds.x1)
					{
						line_width = (short)(character->character_width + line_width);
					}
					else if (TEST_FLAG(draw_string_globals.flags, _draw_text_wrap_horizontally_bit))
					{
						if ((short)break_string_index > 0)
						{
							segment_end_index = break_string_index;
							line_width = break_line_width;
							wrapped = TRUE;
						}
						segment_done = TRUE;
					}
				}
			}

			if (!wrapped)
				segment_end_index = (word)state.string_index;
			previous_result = state.result;
		}
		while (!segment_done);

		if (justification == _text_justification_right)
		{
			cursor.x = (short)(
				rectangle2d_width(&line_bounds) +
				line_bounds.x0 -
				state.font_header->leading_width -
				line_width);
			font = state.font_header;
		}
		else
		{
			if (justification == _text_justification_center)
			{
				cursor.x = (short)(
					((rectangle2d_width(&line_bounds) - (short)line_width) >> 1) +
					line_bounds.x0);
			}
			font = state.font_header;
		}

		if (TEST_FLAG(draw_string_globals.flags, _draw_text_wrap_vertically_bit) || baseline_y < line_bounds.y1)
		{
			draw_string_partial(
				draw_character,
				&cursor,
				&line_bounds,
				clip,
				state.color,
				string,
				(short)segment_start_index,
				(short)segment_end_index);
			font = state.font_header;
		}

		string_index = (short)segment_end_index;
		state.string_index = (short)segment_end_index;

		switch (state.result)
		{
		case _parsed_end_of_line:
			tab_stop_index = 0;
			paragraph_line_offset += maximum_wrapped_line_index + 1;
			wrapped_line_index = 0;
			break;

		case _parsed_ascii_character:
		case _parsed_ascii_word_break:
			wrapped_line_index++;
			if (wrapped_line_index > maximum_wrapped_line_index)
				maximum_wrapped_line_index = wrapped_line_index;
			break;

		case _parsed_end_of_column:
			if (tab_stop_index < draw_string_globals.tab_stop_count)
			{
				tab_stop_index++;
				wrapped_line_index = 0;
			}
			break;

		case _parsed_justification_change:
			wrapped_line_index = 0;
			break;

		case _parsed_color_change:
		case _parsed_end_of_string:
			break;

		default:
			display_assert(NULL, "c:\\halo\\SOURCE\\text\\draw_string.c", 817, TRUE);
			system_exit(-1);
			break;
		}
	}
	while (state.result != _parsed_end_of_string);

	draw_string_globals.highlight_end = 0;
	draw_string_globals.highlight_start = 0;
	if (cursor_reference)
		*cursor_reference = cursor;

	return;
}

void draw_unicode_string(
	draw_character_proc draw_character,
	rectangle2d const *bounds,
	point2d *cursor_reference,
	rectangle2d const *clip,
	short height_adjust,
	wchar_t const *string)
{
	struct parse_string_state state;
	point2d cursor;
	long tab_stop_index = 0;
	long paragraph_line_offset = 0;
	long wrapped_line_index = 0;
	long maximum_wrapped_line_index = 0;
	short string_index;
	struct font_header *font;

	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 848, bounds);
	match_assert("c:\\halo\\SOURCE\\text\\draw_string.c", 849, string);

	parse_string_new(
		&state,
		(char const *)string,
		draw_string_globals.font_index,
		draw_string_globals.style,
		draw_string_globals.justification,
		&draw_string_globals.color);
	string_index = state.string_index;
	font = state.font_header;

	do
	{
		short tab_stop_count = draw_string_globals.tab_stop_count;
		short justification = state.justification;
		long line_width = 0;
		long break_string_index = 0;
		long break_line_width = 0;
		short previous_result = NONE;
		long segment_start_index = string_index;
		rectangle2d line_bounds = *bounds;
		boolean segment_done = FALSE;
		long segment_end_index = 0;
		short line_left;
		short ascending_height;
		long cursor_x_start;
		long line_top;
		long baseline_y;

		if (tab_stop_count > 0)
		{
			match_assert(
				"c:\\halo\\SOURCE\\text\\draw_string.c",
				871,
				tab_stop_index>=0 && tab_stop_index<=draw_string_globals.tab_stop_count);

			if (tab_stop_index != 0)
				line_left = draw_string_globals.tab_stops[tab_stop_index - 1];
			else if (paragraph_line_offset != 0)
				line_left = (short)(line_bounds.x0 + draw_string_globals.paragraph_indent);
			else
				line_left = (short)(line_bounds.x0 + draw_string_globals.initial_indent);

			line_bounds.x0 = line_left;
			if (tab_stop_index < tab_stop_count)
				line_bounds.x1 = draw_string_globals.tab_stops[tab_stop_index];
		}
		else
		{
			if (paragraph_line_offset != 0)
				line_left = (short)(line_bounds.x0 + draw_string_globals.paragraph_indent);
			else
				line_left = (short)(line_bounds.x0 + draw_string_globals.initial_indent);
			line_bounds.x0 = line_left;
		}

		ascending_height = font->ascending_height;
		cursor_x_start = font->leading_width + line_left;
		line_top =
			(font->leading_height + font->descending_height + height_adjust + ascending_height) *
			(wrapped_line_index + paragraph_line_offset) +
			line_bounds.y0;
		cursor.x = (short)cursor_x_start;
		baseline_y = line_top + ascending_height;
		cursor.y = (short)baseline_y;

		do
		{
			boolean wrapped = FALSE;
			struct font_character *character;

			parse_unicode_string(&state);
			font = state.font_header;
			if (state.result != _parsed_end_of_word && state.result != _parsed_character)
			{
				segment_done = TRUE;
			}
			else
			{
				character = font_get_character_by_ascii_code(state.font_header, state.character);
				if (character)
				{
					if (state.result != _parsed_end_of_word && previous_result == _parsed_end_of_word)
					{
						break_string_index = segment_end_index;
						break_line_width = line_width;
					}

					if (character->bitmap_width + cursor_x_start + (short)line_width < line_bounds.x1)
					{
						line_width = (short)(character->character_width + line_width);
					}
					else if (TEST_FLAG(draw_string_globals.flags, _draw_text_wrap_horizontally_bit))
					{
						if ((short)break_string_index > 0)
						{
							line_width = break_line_width;
							segment_end_index = break_string_index;
							wrapped = TRUE;
						}
						segment_done = TRUE;
					}
				}
			}

			if (!wrapped)
				segment_end_index = (word)state.string_index;
			previous_result = state.result;
		}
		while (!segment_done);

		if (justification == _text_justification_right)
		{
			cursor.x = (short)(
				rectangle2d_width(&line_bounds) +
				line_bounds.x0 -
				font->leading_width -
				line_width);
		}
		else if (justification == _text_justification_center)
		{
			cursor.x = (short)(
				((rectangle2d_width(&line_bounds) - (short)line_width) >> 1) +
				line_bounds.x0);
		}

		if (TEST_FLAG(draw_string_globals.flags, _draw_text_wrap_vertically_bit) || baseline_y < line_bounds.y1)
		{
			draw_unicode_string_partial(
				draw_character,
				&cursor,
				&line_bounds,
				clip,
				state.color,
				string,
				(short)segment_start_index,
				(short)segment_end_index);
		}

		string_index = (short)segment_end_index;
		state.string_index = (short)segment_end_index;

		switch (state.result)
		{
		case _parsed_end_of_line:
			wrapped_line_index = 0;
			tab_stop_index = 0;
			paragraph_line_offset += maximum_wrapped_line_index + 1;
			break;

		case _parsed_end_of_word:
		case _parsed_character:
			wrapped_line_index++;
			if (wrapped_line_index > maximum_wrapped_line_index)
				maximum_wrapped_line_index = wrapped_line_index;
			break;

		case _parsed_end_of_column:
			if (tab_stop_index < draw_string_globals.tab_stop_count)
			{
				wrapped_line_index = 0;
				tab_stop_index++;
			}
			break;

		case _parsed_justification_change:
			wrapped_line_index = 0;
			break;

		case _parsed_color_change:
		case _parsed_end_of_string:
			break;

		default:
			display_assert(NULL, "c:\\halo\\SOURCE\\text\\draw_string.c", 1017, TRUE);
			system_exit(-1);
			break;
		}
	}
	while (state.result != _parsed_end_of_string);

	draw_string_globals.highlight_end = 0;
	draw_string_globals.highlight_start = 0;
	if (cursor_reference)
		*cursor_reference = cursor;

	return;
}

void draw_string_compute_bounds(
	rectangle2d const *bounds,
	char const *string,
	rectangle2d *text_bounds,
	rectangle2d *cursor_bounds)
{
	point2d cursor;

	draw_string_globals.text_bounds.y0 = SHORT_MAX;
	draw_string_globals.text_bounds.x0 = SHORT_MAX;
	draw_string_globals.text_bounds.y1 = SHORT_MIN;
	draw_string_globals.text_bounds.x1 = SHORT_MIN;

	draw_string_globals.last_font_header = styled_font_get(
		draw_string_globals.style,
		draw_string_globals.font_index);
	draw_string(text_bounds_draw_character, bounds, &cursor, NULL, 0, string);

	cursor_bounds->x0 = cursor.x;
	cursor_bounds->x1 = (short)(cursor.x + 1);
	cursor_bounds->y0 = (short)(
		cursor.y - draw_string_globals.last_font_header->ascending_height);
	cursor_bounds->y1 = (short)(
		draw_string_globals.last_font_header->descending_height + cursor.y);

	text_bounds->x0 = draw_string_globals.text_bounds.x0;
	text_bounds->y0 = bounds->y0;
	text_bounds->x1 = draw_string_globals.text_bounds.x1;
	text_bounds->y1 = cursor_bounds->y1;

	return;
}

void draw_unicode_string_compute_bounds(
	rectangle2d const *bounds,
	wchar_t const *string,
	rectangle2d *text_bounds,
	rectangle2d *cursor_bounds)
{
	point2d cursor;

	draw_string_globals.text_bounds.y0 = SHORT_MAX;
	draw_string_globals.text_bounds.x0 = SHORT_MAX;
	draw_string_globals.text_bounds.y1 = SHORT_MIN;
	draw_string_globals.text_bounds.x1 = SHORT_MIN;

	draw_string_globals.last_font_header = styled_font_get(
		draw_string_globals.style,
		draw_string_globals.font_index);
	draw_unicode_string(text_bounds_draw_character, bounds, &cursor, NULL, 0, string);

	cursor_bounds->x0 = cursor.x;
	cursor_bounds->x1 = (short)(cursor.x + 1);
	cursor_bounds->y0 = (short)(
		cursor.y - draw_string_globals.last_font_header->ascending_height);
	cursor_bounds->y1 = (short)(
		draw_string_globals.last_font_header->descending_height + cursor.y);

	text_bounds->x0 = draw_string_globals.text_bounds.x0;
	text_bounds->y0 = bounds->y0;
	text_bounds->x1 = draw_string_globals.text_bounds.x1;
	text_bounds->y1 = cursor_bounds->y1;

	return;
}

short draw_string_pick(
	rectangle2d const *bounds,
	char const *string,
	point2d const *point)
{
	draw_string_globals.pick_point = *point;
	draw_string_globals.best_pick_distance = SHORT_MAX;
	draw_string_globals.best_pick_string_index = 0;
	draw_string_globals.last_string_index = 0;

	draw_string(text_pick_draw_character, bounds, NULL, NULL, 0, string);
	return (word)draw_string_globals.best_pick_string_index;
}

void bitmap_draw_string(
	struct bitmap_data *bitmap,
	rectangle2d const *bounds,
	rectangle2d const *clip,
	char const *string)
{
	rectangle2d bitmap_bounds;
	rectangle2d bitmap_clip;
	rectangle2d const *effective_bounds = bounds;
	short format = bitmap->format;

	draw_character_software_globals.bitmap = bitmap;

	switch (format)
	{
	case _bitmap_format_a8:
	case _bitmap_format_y8:
	case _bitmap_format_ay8:
	case _bitmap_format_r5g6b5:
	case _bitmap_format_a8r8g8b8:
		break;

	default:
		return;
	}

	if (!effective_bounds)
	{
		set_rectangle2d(
			&bitmap_bounds,
			0,
			0,
			bitmap->width,
			bitmap->height);
		effective_bounds = &bitmap_bounds;
	}

	if (clip)
	{
		set_rectangle2d(
			&bitmap_clip,
			MAX(clip->x0, 0),
			MAX(clip->y0, 0),
			MIN(clip->x1, bitmap->width),
			MIN(clip->y1, bitmap->height));
		clip = &bitmap_clip;
	}

	draw_string(
		bitmap_draw_character,
		effective_bounds,
		NULL,
		clip,
		0,
		string);

	return;
}
