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
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum trailing_edge_shape
{
	_trailing_edge_shape_flat,
	_trailing_edge_shape_concave_triangular,
	_trailing_edge_shape_convex_triangular,
	_trailing_edge_shape_trapezoid_short_top,
	_trailing_edge_shape_trapezoid_short_bottom
};

enum tesselate
{
	_tesselate_top_left = 2,
	_tesselate_bottom_left = 3
};

/* ---------- macros */

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
	} vertices[225];
	struct flag_cell_datum
	{
		short tesselation;
	} cells[196];
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
		struct flag_datum_prefix *flag = datum_get(flag_data, flag_index);
		struct flag_definition *definition = tag_get('flag', flag->definition_index);

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
	flag = datum_get(flag_data, flag_index);
	definition = tag_get('flag', flag->definition_index);
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
