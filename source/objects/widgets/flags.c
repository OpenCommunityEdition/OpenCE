/*
FLAGS.C

symbols in this file:
00121090 00b0:
	_flag_datum_get_vertex (0000)
00121140 00c0:
	_flag_datum_get_cell (0000)
00121200 0030:
	_flags_initialize (0000)
00121230 0010:
	_flags_initialize_for_new_map (0000)
00121240 0010:
	_flags_dispose_from_old_map (0000)
00121250 0020:
	_flags_dispose (0000)
00121270 0120:
	_flag_tesselate_region (0000)
00121390 0020:
	_flag_delete (0000)
001213B0 02a0:
	_flag_update_attachment (0000)
00121650 00d0:
	_flag_set_attachment_shape (0000)
00121720 00f0:
	_flag_set_trailing_shape (0000)
00121810 04a0:
	_flag_update (0000)
00121CB0 0840:
	_flag_render_proper (0000)
001224F0 0180:
	_flag_new (0000)
00122670 0080:
	_flag_render (0000)
001226F0 00a0:
	_flags_update (0000)
00288F18 001d:
	??_C@_0BN@NBECLHNE@y?$DO?$DN0?5?$CG?$CG?5y?$DMdefinition?9?$DOheight?$AA@ (0000)
00288F38 001c:
	??_C@_0BM@BBBAMOGB@x?$DO?$DN0?5?$CG?$CG?5x?$DMdefinition?9?$DOwidth?$AA@ (0000)
00288F54 0013:
	??_C@_0BD@JPECIJNC@flag?5?$CG?$CG?5definition?$AA@ (0000)
00288F68 0027:
	??_C@_0CH@MIJJCHGH@c?3?2halo?2SOURCE?2objects?2widgets?2f@ (0000)
00288F90 001f:
	??_C@_0BP@EGJJCFHH@y?$DO?$DN0?5?$CG?$CG?5y?$DMdefinition?9?$DOheight?91?$AA@ (0000)
00288FB0 001e:
	??_C@_0BO@PGPCHGEA@x?$DO?$DN0?5?$CG?$CG?5x?$DMdefinition?9?$DOwidth?91?$AA@ (0000)
00288FD0 001f:
	??_C@_0BP@CFMNJLHE@couldn?8t?5allocate?5flag?5globals?$AA@ (0000)
00288FF0 0005:
	??_C@_04DNPGJOMD@flag?$AA@ (0000)
00288FF8 0004:
	__real@3927c5ac (0000)
00288FFC 0004:
	__real@39d1b717 (0000)
00289000 0014:
	??_C@_0BE@KKEACJGN@i?$DMmaximum_triangles?$AA@ (0000)
*/

/* ---------- headers */

#include "objects/widgets/flags.h"

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	MAXIMUM_FLAG_WIDTH = 40,
	MAXIMUM_FLAG_VERTICES = 225,
	MAXIMUM_FLAG_CELLS = 196,
};

enum trailing_edge_shape
{
	_trailing_edge_shape_flat,
	_trailing_edge_shape_concave_triangular,
	_trailing_edge_shape_convex_triangular,
	_trailing_edge_shape_trapezoid_short_top,
	_trailing_edge_shape_trapezoid_short_bottom
};

enum attached_edge_shape
{
	_attached_edge_shape_flat,
	_attached_edge_shape_triangular_notch
};

enum tesselate
{
	_tesselate_both,
	_tesselate_none,
	_tesselate_top_left,
	_tesselate_bottom_left,
	_tesselate_top_right,
	_tesselate_bottom_right
};

/* ---------- macros */

#define FLAG_TAG 'flag'

#define flag_get(index) \
	((struct flag_datum_prefix *)datum_get(flag_data, (index)))
#define flag_definition_get(index) \
	((struct flag_definition *)tag_get(FLAG_TAG, (index)))

/* ---------- structures */

struct flag_datum_prefix
{
	struct datum_header header;
	boolean noop;
	boolean initialized;
	boolean update_state;
	byte pad5;
	short updates_since_last_render;
	long object_index;
	long definition_index;
	real_point3d first_attachment;
	struct flag_vertex_datum
	{
		real_point3d position;
		real_vector3d velocity;
	} vertices[MAXIMUM_FLAG_VERTICES];
	struct flag_cell_datum
	{
		short tesselation;
	} cells[MAXIMUM_FLAG_CELLS];
};

struct flag_attachment_point
{
	short height_to_next_attachment;
	short pad2;
	long unused[4];
	char marker_name[32];
};

struct flag_definition
{
	unsigned long flags;
	short trailing_edge_shape;
	short trailing_edge_offset;
	short attached_edge_shape;
	short padA;
	short width;
	short height;
	real cell_width_scale;
	real cell_height_scale;
	struct tag_reference shader_red;
	struct tag_reference physics;
	real wind_noise;
	long unused3C[2];
	struct tag_reference shader_blue;
	struct tag_block attachment_points;
};

/* ---------- prototypes */

void flag_update(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	real delta);
void flag_render_proper(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	struct render_lighting const *lighting,
	struct render_animation const *animation);
struct flag_vertex_datum *flag_datum_get_vertex(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	short x,
	short y);
struct flag_cell_datum *flag_datum_get_cell(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	short x,
	short y);
void flag_tesselate_region(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag,
	short x,
	short y,
	short size,
	short tesselation);
void flag_set_attachment_shape(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag);
void flag_set_trailing_shape(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag);

/* ---------- globals */

struct data_array *flag_data;

/* ---------- public code */

void flags_initialize(
	void)
{
	flag_data = game_state_data_new("flag", 2, 0x16BC);
	if (!flag_data)
		error(_error_immediate, "couldn't allocate flag globals");

	return;
}

void flags_initialize_for_new_map(
	void)
{
	data_make_valid(flag_data);

	return;
}

void flags_dispose_from_old_map(
	void)
{
	data_make_invalid(flag_data);

	return;
}

void flags_dispose(
	void)
{
	if (flag_data)
		flag_data = NULL;

	return;
}

void flag_delete(
	long flag_index)
{
	datum_delete(flag_data, flag_index);

	return;
}

void flags_update(
	real delta)
{
	long flag_index;

	for (flag_index = data_next_index(flag_data, NONE);
		flag_index != NONE;
		flag_index = data_next_index(flag_data, flag_index))
	{
		struct flag_datum_prefix *flag = flag_get(flag_index);
		struct flag_definition *definition =
			flag_definition_get(flag->definition_index);

		flag->updates_since_last_render++;
		if (flag->object_index != NONE &&
			flag->updates_since_last_render < 5 &&
			delta != 0.0f)
		{
			flag_update(flag, definition, delta);
		}
	}

	return;
}

void flag_render(
	long object_index,
	long flag_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation)
{
	struct flag_datum_prefix *flag;
	struct flag_definition *definition;

	object_get(object_index);
	flag = flag_get(flag_index);
	definition = flag_definition_get(flag->definition_index);
	flag->object_index = object_index;

	if (flag->updates_since_last_render > 5 || !flag->initialized)
	{
		flag_update(flag, definition, 5.0f);
		flag->initialized = TRUE;
	}

	flag->updates_since_last_render = 0;
	if (!flag->noop)
		flag_render_proper(flag, definition, lighting, animation);

	return;
}

long flag_new(
	long definition_index)
{
	long flag_index;

	global_scenario_get();

	flag_index = NONE;
	if (definition_index != NONE)
	{
		struct flag_definition *definition = flag_definition_get(definition_index);

		flag_index = datum_new(flag_data);
		if (flag_index != NONE)
		{
			struct flag_datum_prefix *flag = flag_get(flag_index);

			if (definition->height * definition->width >= MAXIMUM_FLAG_VERTICES ||
				definition->width >= MAXIMUM_FLAG_WIDTH ||
				definition->shader_blue.index == NONE)
			{
				flag->noop = TRUE;
			}
			else
			{
				short x;
				short y;

				flag->noop = FALSE;
				flag->initialized = FALSE;
				flag->object_index = NONE;
				flag->definition_index = definition_index;
				flag->first_attachment.x = flag->first_attachment.y = flag->first_attachment.z = 0.0f;

				for (x = 0; x < definition->width; x++)
				{
					for (y = 0; y < definition->height; y++)
					{
						struct flag_vertex_datum *vertex = flag_datum_get_vertex(flag, definition, x, y);

						vertex->position = *global_origin3d;
						vertex->velocity = *global_zero_vector3d;
						if (x < definition->width - 1 && y < definition->height - 1)
							flag_datum_get_cell(flag, definition, x, y)->tesselation = _tesselate_both;
					}
				}

				flag_set_attachment_shape(definition, flag);
				flag_set_trailing_shape(definition, flag);
			}
		}
	}

	return flag_index;
}

void flag_update_attachment(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	struct location *location,
	real_point3d *attachment_points,
	real_point3d *attachment_force_points,
	short *attachment_y,
	short *y_attachments)
{
	struct object_marker marker;
	real_vector3d delta;
	short i;
	short y;

	for (i = 0; i < definition->attachment_points.count; i++)
	{
		struct flag_attachment_point *point = TAG_BLOCK_GET_ELEMENT(
			&definition->attachment_points,
			i,
			struct flag_attachment_point);

		object_get_marker_by_name(flag->object_index, point->marker_name, &marker, 1);
		attachment_points[i] = marker.matrix.position;
	}

	scenario_location_from_point(location, attachment_points);

	if (flag->noop)
		return;

	for (y = 0; y < definition->height; y++)
		y_attachments[y] = NONE;

	y = 0;
	for (i = 0; i < definition->attachment_points.count; i++)
	{
		struct flag_attachment_point *point;
		short span;
		short y_end;
		short row_start;

		if (y >= definition->height)
			break;

		point = TAG_BLOCK_GET_ELEMENT(
			&definition->attachment_points,
			i,
			struct flag_attachment_point);
		span = PIN(point->height_to_next_attachment, 0, definition->height - y) & ~1;
		y_end = y + span;

		attachment_y[i] = y;
		y_attachments[y] = i;
		row_start = y;

		for (; y <= y_end; y++)
		{
			real t = ((real)y - row_start) / ((real)y_end - row_start);
			real_point3d *from = &attachment_points[i];
			real_point3d *to = &attachment_points[i + 1];
			real_point3d *force_point = &attachment_force_points[y];

			force_point->x = from->x * (1.0f - t) + to->x * t;
			force_point->y = from->y * (1.0f - t) + to->y * t;
			force_point->z = from->z * (1.0f - t) + to->z * t;
		}

		y--;
	}

	delta.i = attachment_points->x - flag->first_attachment.x;
	delta.j = attachment_points->y - flag->first_attachment.y;
	delta.k = attachment_points->z - flag->first_attachment.z;

	/*
	 * BUG (original): integer truncation makes this an effective two-unit
	 * threshold, as in antenna_update_attachment. A non-matching correctness
	 * fix would compare fabs(delta.i), fabs(delta.j), and fabs(delta.k)
	 * directly against 1.0f.
	 */
	if ((real)abs((long)delta.i) > 1.0f ||
		(real)abs((long)delta.j) > 1.0f ||
		(real)abs((long)delta.k) > 1.0f)
	{
		short x;

		for (x = 0; x < definition->width; x++)
		{
			for (y = 0; y < definition->height; y++)
			{
				struct flag_vertex_datum *vertex = flag_datum_get_vertex(flag, definition, x, y);

				vertex->position.x += delta.i;
				vertex->position.y += delta.j;
				vertex->position.z += delta.k;
			}
		}
	}

	flag->first_attachment = *attachment_points;

	return;
}

/* ---------- private code */

struct flag_vertex_datum *flag_datum_get_vertex(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	short x,
	short y)
{
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		96,
		flag && definition);
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		97,
		x>=0 && x<definition->width);
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		98,
		y>=0 && y<definition->height);

	return &flag->vertices[x * definition->height + y];
}

struct flag_cell_datum *flag_datum_get_cell(
	struct flag_datum_prefix *flag,
	struct flag_definition *definition,
	short x,
	short y)
{
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		109,
		flag && definition);
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		110,
		x>=0 && x<definition->width-1);
	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
		111,
		y>=0 && y<definition->height-1);

	return &flag->cells[x * (definition->height - 1) + y];
}

void flag_tesselate_region(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag,
	short x,
	short y,
	short size,
	short tesselation)
{
	short i;
	short j;

	for (i = x; i < x + size; i++)
	{
		for (j = y; j < y + size; j++)
		{
			struct flag_cell_datum *cell;
			short horizontal;
			short vertical;

			if (i < 0 || j < 0 ||
				i >= definition->width - 1 ||
				j >= definition->height - 1)
			{
				continue;
			}

			horizontal = (tesselation == _tesselate_top_right || tesselation == _tesselate_bottom_right) ?
				i - x : size - i + x - 1;
			vertical = (tesselation == _tesselate_top_right || tesselation == _tesselate_top_left) ?
				j - y : size - j + y - 1;

			cell = flag_datum_get_cell(flag, definition, i, j);
			if (horizontal == vertical)
				cell->tesselation = tesselation;
			else
				cell->tesselation = horizontal <= vertical ? _tesselate_none : _tesselate_both;
		}
	}

	return;
}

void flag_set_attachment_shape(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag)
{
	short point_index;
	short y = 0;

	if (definition->attached_edge_shape == _attached_edge_shape_flat)
		return;

	for (point_index = 0;
		point_index < definition->attachment_points.count;
		point_index++)
	{
		struct flag_attachment_point *point;
		short span;
		short half_span;

		if (y >= definition->height)
			break;

		point = TAG_BLOCK_GET_ELEMENT(
			&definition->attachment_points,
			point_index,
			struct flag_attachment_point);
		span = PIN(point->height_to_next_attachment, 0, definition->height - y) & ~1;
		half_span = span >> 1;

		flag_tesselate_region(
			definition, flag, 0, y, half_span, _tesselate_top_right);
		flag_tesselate_region(
			definition, flag, 0, y + half_span, half_span, _tesselate_bottom_right);

		y += span;
	}

	return;
}

void flag_set_trailing_shape(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag)
{
	short trailing_edge_shape;
	short split;
	short x;

	trailing_edge_shape = definition->trailing_edge_shape;
	if (trailing_edge_shape == _trailing_edge_shape_flat)
		return;

	if (trailing_edge_shape == _trailing_edge_shape_trapezoid_short_top ||
		trailing_edge_shape == _trailing_edge_shape_trapezoid_short_bottom)
	{
		split = definition->height - 1;
	}
	else
	{
		split = definition->height >> 1;
	}

	x = MAX(
		0,
		definition->width + definition->trailing_edge_offset - split - 1);

	if (trailing_edge_shape == _trailing_edge_shape_trapezoid_short_top)
	{
		flag_tesselate_region(
			definition, flag, x, 0, split, _tesselate_bottom_left);
		return;
	}

	if (trailing_edge_shape == _trailing_edge_shape_trapezoid_short_bottom)
	{
		flag_tesselate_region(
			definition, flag, x, 0, split, _tesselate_top_left);
		return;
	}

	if (trailing_edge_shape == _trailing_edge_shape_concave_triangular)
	{
		flag_tesselate_region(
			definition, flag, x, 0, split, _tesselate_top_left);
		flag_tesselate_region(
			definition, flag, x, split, split, _tesselate_bottom_left);
		return;
	}

	if (trailing_edge_shape == _trailing_edge_shape_convex_triangular)
	{
		flag_tesselate_region(
			definition, flag, x, 0, split, _tesselate_bottom_left);
		flag_tesselate_region(
			definition, flag, x, split, split, _tesselate_top_left);
	}

	return;
}
