/*
BITMAP_DRAWING.C

symbols in this file:
00060610 00c0:
	_bitmap_initialize_line (0000)
000606D0 0140:
	_bitmap_step_line (0000)
00060810 02f0:
	_bitmap_fill_rectangle (0000)
00060B00 0ae0:
	_bitmap_copy (0000)
000615E0 0420:
	_bitmap_tile_and_bevel_rectangle (0000)
00061A00 03e0:
	_bitmap_draw_line (0000)
00061DE0 00c0:
	_bitmap_frame_rectangle (0000)
00251114 0012:
	_bitmap_bevel_sequence_indices (0000)
00251128 0035:
	??_C@_0DF@JBBFIHJO@bitmap?5?$EA?$CFp?5has?5bad?5encoding?5?$CD?$CFd?5@ (0000)
00251160 0028:
	??_C@_0CI@IJDPNIHH@c?3?2halo?2SOURCE?2bitmaps?2bitmap_dr@ (0000)
00251188 003f:
	??_C@_0DP@BDCJPGKD@no?5copy?5bitmap?5translation?5exist@ (0000)
002511C8 0016:
	??_C@_0BG@KLCGHOAK@?$CD?$CFd?$CL?$CD?$CFd?$CI?$CD?$CFd?0?$CD?$CFd?$CJ?$DM?$DN?$CD?$CFd?$AA@ (0000)
002511E0 0020:
	??_C@_0CA@IKDBNOCM@adjusted_destination_point?4y?$DO?$DN0?$AA@ (0000)
00251200 0020:
	??_C@_0CA@LHFBPHJM@adjusted_destination_point?4x?$DO?$DN0?$AA@ (0000)
00251220 0016:
	??_C@_0BG@OAHFNKOM@destination?5?$CG?$CG?5source?$AA@ (0000)
00251238 0039:
	??_C@_0DJ@ODLKIEJD@bitmap?5?$EA?$CFp?5has?5bad?5encoding?5?$CD?$CFd?5@ (0000)
00251274 001a:
	??_C@_0BK@LAJPHKOM@destination?9?$DObase_address?$AA@ (0000)
002DB678 0a54:
	_bitmap_copy_translation_table_data (0000)
	_translation_table (0010)
*/

/* ---------- headers */

#include "bitmaps/bitmap_drawing.h"

#include "bitmaps/bitmaps_internal.h"
#include "bitmaps/bitmap_group.h"
#include "cseries/errors.h"

/* ---------- constants */

enum
{
	_bitmap_format_a8 = 0,
	_bitmap_format_y8,
	_bitmap_format_ay8,
	_bitmap_format_r5g6b5 = 6,
	_bitmap_format_a8r8g8b8 = 11,
};

enum
{
	_bitmap_line_step_pixel = 0,
	_bitmap_line_step_vertical,
	_bitmap_line_step_horizontal,
};

enum
{
	_bitmap_fill_write_16bit = 0,
	_bitmap_fill_blend_a1r5g5b5,
	_bitmap_fill_blend_r5g6b5,
};

enum
{
	_bitmap_pixel_write_a8 = 0,
	_bitmap_pixel_write_16bit,
	_bitmap_pixel_write_32bit,
	_bitmap_pixel_blend_a1r5g5b5,
	_bitmap_pixel_blend_r5g6b5,
	_bitmap_pixel_blend_a8r8g8b8,
};

/* ---------- macros */

/* ---------- structures */

struct bitmap_line_state
{
	short two_dx;
	short two_dy;
	short x_step;
	short y_step;
	short dx;
	short dy;
	short error;
	point2d point;
	point2d end_point;
};

/* ---------- prototypes */

void bitmap_initialize_line(
	struct bitmap_line_state *state,
	point2d const *p0,
	point2d const *p1);
boolean bitmap_step_line(
	struct bitmap_line_state *state,
	short step_mode);

/* ---------- globals */

/* ---------- public code */

void bitmap_initialize_line(
	struct bitmap_line_state *state,
	point2d const *p0,
	point2d const *p1)
{
	state->dx = p1->x - p0->x;
	state->dy = p1->y - p0->y;
	state->two_dx = 2 * ABS(state->dx);
	state->two_dy = 2 * ABS(state->dy);
	state->x_step = (state->dx != 0) ? ((state->dx >= 0) ? 1 : -1) : 0;
	state->y_step = (state->dy != 0) ? ((state->dy >= 0) ? 1 : -1) : 0;
	state->point = *p0;
	state->end_point = *p1;
	state->error = (state->two_dx > state->two_dy) ?
		(state->two_dy - (state->two_dx >> 1)) :
		(state->two_dx - (state->two_dy >> 1));

	return;
}

boolean bitmap_step_line(
	struct bitmap_line_state *state,
	short step_mode)
{
	boolean done = FALSE;

	if (state->two_dx > state->two_dy)
	{
		if (state->point.x == state->end_point.x)
		{
			done = TRUE;
		}
		else
		{
			switch (step_mode)
			{
				case _bitmap_line_step_pixel:
					if (state->error >= 0)
					{
						state->point.y += state->y_step;
						state->error -= state->two_dx;
					}
					state->point.x += state->x_step;
					state->error += state->two_dy;
					break;

				case _bitmap_line_step_horizontal:
					while (state->error < 0 && state->point.x != state->end_point.x)
					{
						state->error += state->two_dy;
						state->point.x += state->x_step;
					}
					break;
			}
		}
	}
	else
	{
		if (state->point.y == state->end_point.y)
		{
			done = TRUE;
		}
		else
		{
			switch (step_mode)
			{
				case _bitmap_line_step_pixel:
					if (state->error >= 0)
					{
						state->point.x += state->x_step;
						state->error -= state->two_dy;
					}
					state->point.y += state->y_step;
					state->error += state->two_dx;
					break;

				case _bitmap_line_step_vertical:
					while (state->error < 0 && state->point.y != state->end_point.y)
					{
						state->error += state->two_dx;
						state->point.y += state->y_step;
					}
					break;
			}
		}
	}

	return done;
}

void bitmap_fill_rectangle(
	struct bitmap_data *destination,
	pixel32 color,
	rectangle2d const *rectangle,
	rectangle2d const *clip_rectangle)
{
	rectangle2d bounds = *rectangle;
	short alpha, inverse_alpha;
	short x_minimum, x_maximum, y_minimum, y_maximum;
	short width, y;
	short mode;
	long pixel;

	match_assert(
		"c:\\halo\\SOURCE\\bitmaps\\bitmap_drawing.c",
		275,
		destination);

	if (clip_rectangle && !intersect_rectangles2d(rectangle, clip_rectangle, &bounds))
	{
		return;
	}

	alpha = color >> 24;
	inverse_alpha = 255 - alpha;

	switch (destination->format)
	{
		case _bitmap_format_r5g6b5:
			pixel = ((((color >> 16) & 0xf8) << 5) | ((color >> 8) & 0xfc)) << 3 | ((color >> 3) & 0x1f);
			mode = (alpha != 255) ? _bitmap_fill_blend_r5g6b5 : _bitmap_fill_write_16bit;
			break;

		default:
			match_vassert(
				"c:\\halo\\SOURCE\\bitmaps\\bitmap_drawing.c",
				301,
				FALSE,
				csprintf(
					temporary,
					"bitmap @%p has bad encoding #%d for fill_rectangle()",
					destination,
					destination->format));
			break;
	}

	x_minimum = PIN(bounds.x0, 0, destination->width);
	x_maximum = PIN(bounds.x1, 0, destination->width);
	y_minimum = PIN(bounds.y0, 0, destination->height);
	y_maximum = PIN(bounds.y1, 0, destination->height);
	width = x_maximum - x_minimum;

	for (y = y_minimum; y < y_maximum; y++)
	{
		word *address = bitmap_2d_address(destination, x_minimum, y, 0);
		short x;

		switch (mode)
		{
			case _bitmap_fill_write_16bit:
				for (x = 0; x < width; x++)
				{
					*address++ = (word)pixel;
				}
				break;

			case _bitmap_fill_blend_a1r5g5b5:
				for (x = 0; x < width; x++)
				{
					word destination_pixel = *address;

					*address = (word)
						(((((destination_pixel & 0x1f) * inverse_alpha + (pixel & 0x1f) * alpha) >> 8) & 0x1f) |
						((((destination_pixel & 0x3ff) * inverse_alpha + (pixel & 0x3ff) * alpha) >> 8) & 0x3e0) |
						(((destination_pixel * inverse_alpha + pixel * alpha) >> 8) & 0x7c00));
				}
				break;

			case _bitmap_fill_blend_r5g6b5:
				for (x = 0; x < width; x++)
				{
					word destination_pixel = *address;

					*address = (word)
						(((((destination_pixel & 0x1f) * inverse_alpha + (pixel & 0x1f) * alpha) >> 8) & 0x1f) |
						((((destination_pixel & 0x7ff) * inverse_alpha + (pixel & 0x7ff) * alpha) >> 8) & 0x7e0) |
						(((destination_pixel * inverse_alpha + pixel * alpha) >> 8) & 0xf800));
				}
				break;
		}
	}

	return;
}

void bitmap_draw_line(
	struct bitmap_data *destination,
	pixel32 color,
	rectangle2d const *clip_rectangle,
	real_point2d const *p0,
	real_point2d const *p1)
{
	short x_minimum = 0;
	short x_maximum = destination->width;
	short y_minimum = 0;
	short y_maximum = destination->height;
	short alpha = color >> 24;
	short inverse_alpha = 255 - alpha;
	short mode;
	long pixel;
	point2d point0, point1;
	struct bitmap_line_state state;

	match_assert(
		"c:\\halo\\SOURCE\\bitmaps\\bitmap_drawing.c",
		56,
		destination->base_address);

	switch (destination->format)
	{
		case _bitmap_format_a8:
		case _bitmap_format_y8:
		case _bitmap_format_ay8:
			pixel = color >> 24;
			mode = _bitmap_pixel_write_a8;
			break;

		case _bitmap_format_r5g6b5:
			pixel = ((((color >> 16) & 0xf8) << 5) | ((color >> 8) & 0xfc)) << 3 | ((color >> 3) & 0x1f);
			mode = (alpha != 255) ? _bitmap_pixel_blend_r5g6b5 : _bitmap_pixel_write_16bit;
			break;

		case _bitmap_format_a8r8g8b8:
			pixel = color;
			mode = (alpha != 255) ? _bitmap_pixel_blend_a8r8g8b8 : _bitmap_pixel_write_32bit;
			break;

		default:
			match_vassert(
				"c:\\halo\\SOURCE\\bitmaps\\bitmap_drawing.c",
				82,
				FALSE,
				csprintf(
					temporary,
					"bitmap @%p has bad encoding #%d for draw_line_software()",
					destination,
					destination->format));
			break;
	}

	if (clip_rectangle)
	{
		if (clip_rectangle->x0 > x_minimum)
		{
			x_minimum = clip_rectangle->x0;
		}
		if (clip_rectangle->x1 < x_maximum)
		{
			x_maximum = clip_rectangle->x1;
		}
		if (clip_rectangle->y0 > y_minimum)
		{
			y_minimum = clip_rectangle->y0;
		}
		if (clip_rectangle->y1 < y_maximum)
		{
			y_maximum = clip_rectangle->y1;
		}
	}

	if (p0->y > p1->y)
	{
		real_point2d const *swap = p0;

		p0 = p1;
		p1 = swap;
	}

	point0.x = (short)p0->x;
	point0.y = (short)p0->y;
	point1.x = (short)p1->x;
	point1.y = (short)p1->y;

	bitmap_initialize_line(&state, &point0, &point1);

	do
	{
		if (state.point.x >= x_minimum && state.point.x < x_maximum &&
			state.point.y >= y_minimum && state.point.y < y_maximum)
		{
			void *address = bitmap_2d_address(destination, state.point.x, state.point.y, 0);

			switch (mode)
			{
				case _bitmap_pixel_write_a8:
					*(byte *)address = (byte)pixel;
					break;

				case _bitmap_pixel_write_16bit:
					*(word *)address = (word)pixel;
					break;

				case _bitmap_pixel_write_32bit:
					*(pixel32 *)address = (pixel32)pixel;
					break;

				case _bitmap_pixel_blend_a1r5g5b5:
				{
					word destination_pixel = *(word *)address;

					*(word *)address = (word)
						(((((destination_pixel & 0x1f) * inverse_alpha + (pixel & 0x1f) * alpha) >> 8) & 0x1f) |
						((((destination_pixel & 0x3ff) * inverse_alpha + (pixel & 0x3ff) * alpha) >> 8) & 0x3e0) |
						(((destination_pixel * inverse_alpha + pixel * alpha) >> 8) & 0x7c00));
					break;
				}

				case _bitmap_pixel_blend_r5g6b5:
				{
					word destination_pixel = *(word *)address;

					*(word *)address = (word)
						(((((destination_pixel & 0x1f) * inverse_alpha + (pixel & 0x1f) * alpha) >> 8) & 0x1f) |
						((((destination_pixel & 0x7ff) * inverse_alpha + (pixel & 0x7ff) * alpha) >> 8) & 0x7e0) |
						(((destination_pixel * inverse_alpha + pixel * alpha) >> 8) & 0xf800));
					break;
				}

				case _bitmap_pixel_blend_a8r8g8b8:
				{
					pixel32 destination_pixel = *(pixel32 *)address;

					*(pixel32 *)address =
						(((((pixel >> 16) & 0xff) * alpha) >> 8) + ((((destination_pixel >> 16) & 0xff) * inverse_alpha) >> 8)) << 16 |
						(((((pixel >> 8) & 0xff) * alpha) >> 8) + ((((destination_pixel >> 8) & 0xff) * inverse_alpha) >> 8)) << 8 |
						((((pixel & 0xff) * alpha) >> 8) + (((destination_pixel & 0xff) * inverse_alpha) >> 8)) |
						MAX(alpha, destination_pixel >> 24) << 24;
					break;
				}
			}
		}
	}
	while (!bitmap_step_line(&state, _bitmap_line_step_pixel));

	return;
}

void bitmap_frame_rectangle(
	struct bitmap_data *destination,
	pixel32 color,
	real_rectangle2d const *bounds,
	rectangle2d const *clip_rectangle)
{
	real_point2d p0, p1;

	p0.x = bounds->x0;
	p0.y = bounds->y0;
	p1.x = bounds->x1 - 1.0f;
	p1.y = bounds->y0;
	bitmap_draw_line(destination, color, clip_rectangle, &p0, &p1);

	p0.x = bounds->x1 - 1.0f;
	p0.y = bounds->y1 - 1.0f;
	bitmap_draw_line(destination, color, clip_rectangle, &p1, &p0);

	p1.x = bounds->x0;
	p1.y = bounds->y1 - 1.0f;
	bitmap_draw_line(destination, color, clip_rectangle, &p0, &p1);

	p0.x = bounds->x0;
	p0.y = bounds->y0;
	bitmap_draw_line(destination, color, clip_rectangle, &p1, &p0);

	return;
}

/* ---------- private code */
