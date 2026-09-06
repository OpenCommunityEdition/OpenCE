/*
RENDER_DEBUG.C

symbols in this file:
001780E0 00e0:
	_render_debug_triangle (0000)
001781C0 0120:
	_render_debug_quadrilateral (0000)
001782E0 0090:
	_render_debug_polygon (0000)
00178370 00d0:
	_code_00178370 (0000)
00178440 0070:
	_code_00178440 (0000)
001784B0 00a0:
	_code_001784b0 (0000)
00178550 01c0:
	_code_00178550 (0000)
00178710 0290:
	_code_00178710 (0000)
001789A0 0120:
	_render_debug_point (0000)
00178AC0 00b0:
	_render_debug_line (0000)
00178B70 00c0:
	_render_debug_vector (0000)
00178C30 0070:
	_render_debug_tick (0000)
00178CA0 0080:
	_render_debug_line_offset (0000)
00178D20 0070:
	_render_debug_matrix (0000)
00178D90 0190:
	_render_debug_sphere (0000)
00178F20 0190:
	_render_debug_cylinder (0000)
001790B0 01c0:
	_render_debug_pill (0000)
00179270 0180:
	_render_debug_box (0000)
001793F0 00a0:
	_render_debug_polygon_edges (0000)
00179490 0070:
	_render_debug_string (0000)
00179500 0130:
	_render_debug_string_at_point (0000)
00179630 0220:
	_code_00179630 (0000)
00179850 0110:
	_code_00179850 (0000)
00179960 0080:
	_code_00179960 (0000)
001799E0 01e0:
	_code_001799e0 (0000)
00179BC0 0070:
	_code_00179bc0 (0000)
00179C30 01a0:
	_code_00179c30 (0000)
00179DD0 00d0:
	_render_debug_point2d (0000)
00179EA0 0120:
	_render_debug_line2d (0000)
00179FC0 00f0:
	_render_debug_vector2d (0000)
0017A0B0 0130:
	_render_debug_circle (0000)
0017A1E0 0040:
	_render_debug_vectors (0000)
0017A220 0030:
	_render_debug_quaternion (0000)
0017A250 0130:
	_render_debug_box2d_outline (0000)
0017A380 0120:
	_render_debug_box_outline (0000)
0017A4A0 0240:
	_render_debug (0000)
0029FC9C 0007:
	??_C@_06IGAKEMNE@point2?$AA@ (0000)
0029FCA4 0007:
	??_C@_06KNCHBPBH@point1?$AA@ (0000)
0029FCAC 0007:
	??_C@_06LEDMCOFG@point0?$AA@ (0000)
0029FCB4 000a:
	??_C@_09MBGEJPJA@immediate?$AA@ (0000)
0029FCC0 0025:
	??_C@_0CF@JKFPHJJE@c?3?2halo?2SOURCE?2render?2render_deb@ (0000)
0029FCE8 0007:
	??_C@_06JPBBHNJF@point3?$AA@ (0000)
0029FCF0 0023:
	??_C@_0CD@CLODFBGE@render?5debug?5cache?5string?5overfl@ (0000)
0029FD18 0008:
	__real@3fd921fb60000000 (0000)
0029FD20 001d:
	??_C@_0BN@JJPKCCPJ@render?5debug?5cache?5overflow?4?$AA@ (0000)
0029FD40 0007:
	??_C@_06LNLHEAAG@height?$AA@ (0000)
0029FD48 0005:
	??_C@_04OEJCCLCC@?$HMn?$CFs?$AA@ (0000)
0029FD50 0041:
	??_C@_0EB@EGDCEMGJ@?$HMnground_point?$CI?$CF01?42f?0?$CF01?42f?0?$CF01@ (0000)
0029FD94 003d:
	??_C@_0DN@EHAAAEJO@point?$CI?$CF01?42f?0?$CF01?42f?0?$CF01?42f?$CJ?5leaf@ (0000)
0029FDD4 0022:
	??_C@_0CC@HBIPCGOB@speed?5?$CF5f?$HMnslide?5?$CF5f?$HMnturn?5?5?$CF5f?$CF@ (0000)
0029FDF8 0009:
	??_C@_08FHIPOKAI@?$HMnstuck?$CB?$AA@ (0000)
0029FE04 0013:
	??_C@_0BD@FCHBBNKP@riding?5an?5elevator?$AA@ (0000)
0029FE18 0004:
	??_C@_03PMGGPEJJ@?$CFd?6?$AA@ (0000)
0029FE1C 0011:
	??_C@_0BB@GECKHLGI@d?3?2debug_bsp?4txt?$AA@ (0000)
0029FE30 0006:
	??_C@_05OPIONHBI@solid?$AA@ (0000)
0029FE38 000a:
	??_C@_09PHAEBKPA@?5leaf?5?$CF5d?$AA@ (0000)
0029FE44 000d:
	??_C@_0N@IGDJDOFE@?$CF5d?5?$CF5d?5?$CFc?$HMn?$AA@ (0000)
0029FE54 0020:
	??_C@_0CA@CADNHOKJ@plane_count?$DMMAXIMUM_BSP3D_DEPTH?$AA@ (0000)
0029FE74 000e:
	??_C@_0O@LHEAABKE@?5node?5plane?$HMn?$AA@ (0000)
0029FE84 0004:
	__real@3c4aa525 (0000)
0029FE88 0004:
	__real@3ccaa525 (0000)
0029FE8C 001f:
	??_C@_0BP@NHMLPPIA@can?8t?5add?5box2d?5to?5debug?5cache?$AA@ (0000)
004B8C40 740c:
	_bss_004b8c40 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "interface/interface.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_debug.h"
#include "render/render.h"
#include "render/render_debug.h"
#include "render/render_debug_geometry.h"

/* ---------- constants */

enum
{
	NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS = 1024,
	MAXIMUM_RENDER_DEBUG_CACHE_ENTRIES = 512,
};

enum
{
	_render_debug_cache_circle,
	_render_debug_cache_point,
	_render_debug_cache_line,
	_render_debug_cache_sphere,
	_render_debug_cache_cylinder,
	_render_debug_cache_pill,
	_render_debug_cache_box,
	_render_debug_cache_box_outline,
	_render_debug_cache_string,
	_render_debug_cache_string_at_point,
	NUMBER_OF_RENDER_DEBUG_CACHE_TYPES
};

/* ---------- macros */

/* ---------- structures */

struct render_debug_cache_entry
{
	short type;
	union
	{
		struct
		{
			real_plane3d plane;
			short projection;
			boolean sign;
			real_point2d center;
			real radius;
			real_argb_color color;
			real offset;
		} circle;
		struct
		{
			real_point3d point;
			real size;
			real_argb_color color;
		} point;
		struct
		{
			real_point3d point0;
			real_point3d point1;
			real_argb_color color;
		} line;
		struct
		{
			real_point3d base;
			real_vector3d height;
			real width;
			real_argb_color color;
		} pill;
		struct
		{
			real_rectangle3d bounds;
			real_argb_color color;
		} box;
		struct
		{
			char const *string;
		} string;
		struct
		{
			char const *string;
			real_point3d point;
			real_argb_color color;
		} string_at_point;
	};
};

struct render_debug_globals_definition
{
	char strings[NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS];
	struct render_debug_cache_entry entries[MAXIMUM_RENDER_DEBUG_CACHE_ENTRIES];
	short game_time;
	byte opaque_after_game_time[2];
	short entry_count;
	byte opaque_after_entry_count[2];
	short string_offset;
	boolean entry_overflow_reported;
	boolean string_overflow_reported;
};

typedef char render_debug_cache_entry_size_check[
	sizeof(struct render_debug_cache_entry) == 0x38 ? 1 : -1];
typedef char render_debug_globals_size_check[
	sizeof(struct render_debug_globals_definition) == 0x740C ? 1 : -1];
typedef char render_debug_globals_strings_offset_check[
	offsetof(struct render_debug_globals_definition, strings) == 0x0000 ? 1 : -1];
typedef char render_debug_globals_entries_offset_check[
	offsetof(struct render_debug_globals_definition, entries) == 0x0400 ? 1 : -1];
typedef char render_debug_globals_game_time_offset_check[
	offsetof(struct render_debug_globals_definition, game_time) == 0x7400 ? 1 : -1];
typedef char render_debug_globals_entry_count_offset_check[
	offsetof(struct render_debug_globals_definition, entry_count) == 0x7404 ? 1 : -1];
typedef char render_debug_globals_string_offset_offset_check[
	offsetof(struct render_debug_globals_definition, string_offset) == 0x7408 ? 1 : -1];
typedef char render_debug_globals_entry_overflow_offset_check[
	offsetof(struct render_debug_globals_definition, entry_overflow_reported) == 0x740A ? 1 : -1];
typedef char render_debug_globals_string_overflow_offset_check[
	offsetof(struct render_debug_globals_definition, string_overflow_reported) == 0x740B ? 1 : -1];

/* ---------- prototypes */

static char *render_debug_add_cache_string(
	char const *string);
static void render_debug_add_cache_entry(
	short type,
	...);

/* ---------- globals */

static struct render_debug_globals_definition render_debug_globals;

/* ---------- public code */

void render_debug_triangle(
	boolean immediate,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		480,
		immediate);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		481,
		point0);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		482,
		point1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		483,
		point2);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		484,
		color);

	rasterizer_debug_triangle(
		point0,
		point1,
		point2,
		color);

	return;
}

void render_debug_quadrilateral(
	boolean immediate,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_point3d const *point3,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		499,
		immediate);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		500,
		point0);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		501,
		point1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		502,
		point2);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		503,
		point3);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		504,
		color);

	render_debug_triangle(
		immediate,
		point0,
		point1,
		point2,
		color);
	render_debug_triangle(
		immediate,
		point0,
		point2,
		point3,
		color);

	return;
}

void render_debug_polygon(
	real_point3d const *points,
	short point_count,
	real_argb_color const *color)
{
	short index;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		855,
		points);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		856,
		color);

	for (index = 1; index<point_count-1; index++)
	{
		render_debug_triangle(
			TRUE,
			points,
			&points[index],
			&points[index+1],
			color);
	}

	return;
}

void render_debug_box_outline(
	boolean immediate,
	real_rectangle3d const *bounds,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		812,
		bounds);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		813,
		color);

	if (immediate)
	{
		real_point3d points[8];
		short index;

		points[0].x = bounds->x0;
		points[0].y = bounds->y0;
		points[0].z = bounds->z0;
		points[1].x = bounds->x1;
		points[1].y = bounds->y0;
		points[1].z = bounds->z0;
		points[2].x = bounds->x1;
		points[2].y = bounds->y1;
		points[2].z = bounds->z0;
		points[3].x = bounds->x0;
		points[3].y = bounds->y1;
		points[3].z = bounds->z0;
		points[4].x = bounds->x0;
		points[4].y = bounds->y0;
		points[4].z = bounds->z1;
		points[5].x = bounds->x1;
		points[5].y = bounds->y0;
		points[5].z = bounds->z1;
		points[6].x = bounds->x1;
		points[6].y = bounds->y1;
		points[6].z = bounds->z1;
		points[7].x = bounds->x0;
		points[7].y = bounds->y1;
		points[7].z = bounds->z1;

		render_debug_polygon_edges(
			&points[0],
			4,
			color);
		render_debug_polygon_edges(
			&points[4],
			4,
			color);

		for (index = 0; index<4; index++)
		{
			render_debug_line(
				TRUE,
				&points[index],
				&points[index+4],
				color);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_box_outline,
			bounds,
			color);
	}

	return;
}

void render_debug_point(
	boolean immediate,
	real_point3d const *point,
	real size,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		327,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		328,
		color);

	if (immediate)
	{
		real_point3d points[6];
		short index;

		points[0].x = point->x-size*0.5f;
		points[0].y = point->y;
		points[0].z = point->z;
		points[1].x = point->x+size*0.5f;
		points[1].y = point->y;
		points[1].z = point->z;
		points[2].x = point->x;
		points[2].y = point->y-size*0.5f;
		points[2].z = point->z;
		points[3].x = point->x;
		points[3].y = point->y+size*0.5f;
		points[3].z = point->z;
		points[4].x = point->x;
		points[4].y = point->y;
		points[4].z = point->z-size*0.5f;
		points[5].x = point->x;
		points[5].y = point->y;
		points[5].z = point->z+size*0.5f;

		for (index = 0; index<NUMBEROF(points)/2; index++)
		{
			rasterizer_debug_line(
				&points[index*2],
				&points[index*2+1],
				color);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_point,
			point,
			size,
			color);
	}

	return;
}

void render_debug_line(
	boolean immediate,
	real_point3d const *point0,
	real_point3d const *point1,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		363,
		point0);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		364,
		point1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		365,
		color);

	if (immediate)
	{
		rasterizer_debug_line(
			point0,
			point1,
			color);
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_line,
			point0,
			point1,
			color);
	}

	return;
}

void render_debug_vector(
	boolean immediate,
	real_point3d const *point,
	real_vector3d const *vector,
	real size,
	real_argb_color const *color)
{
	real_point3d end_point;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		388,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		389,
		vector);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		390,
		color);

	end_point.x = point->x + size*vector->i;
	end_point.y = point->y + size*vector->j;
	end_point.z = point->z + size*vector->k;

	render_debug_line(
		immediate,
		point,
		&end_point,
		color);

	return;
}

void render_debug_tick(
	boolean immediate,
	real_point3d const *point,
	real_vector3d const *tick_vector,
	real tick_size,
	real_argb_color const *color)
{
	real_point3d point0;
	real_point3d point1;
	real negative_tick_size;

	point0.x = point->x + tick_size*tick_vector->i;
	point0.y = point->y + tick_size*tick_vector->j;
	point0.z = point->z + tick_size*tick_vector->k;
	negative_tick_size = -tick_size;
	point1.x = point->x + negative_tick_size*tick_vector->i;
	point1.y = point->y + negative_tick_size*tick_vector->j;
	point1.z = point->z + negative_tick_size*tick_vector->k;

	render_debug_line(
		immediate,
		&point0,
		&point1,
		color);

	return;
}

void render_debug_line_offset(
	boolean immediate,
	real_point3d const *p0,
	real_point3d const *p1,
	real_argb_color const *color,
	real offset)
{
	real_point3d point0;
	real_point3d point1;

	point0.x = p0->x + offset*global_up3d->i;
	point0.y = p0->y + offset*global_up3d->j;
	point0.z = p0->z + offset*global_up3d->k;
	point1.x = p1->x + offset*global_up3d->i;
	point1.y = p1->y + offset*global_up3d->j;
	point1.z = p1->z + offset*global_up3d->k;

	render_debug_line(
		immediate,
		&point0,
		&point1,
		color);

	return;
}

void render_debug_matrix(
	boolean immediate,
	struct real_matrix4x3 const *matrix,
	real size)
{
	render_debug_vector(
		immediate,
		&matrix->position,
		&matrix->forward,
		size*matrix->scale,
		global_real_argb_red);
	render_debug_vector(
		immediate,
		&matrix->position,
		&matrix->left,
		size*matrix->scale,
		global_real_argb_green);
	render_debug_vector(
		immediate,
		&matrix->position,
		&matrix->up,
		size*matrix->scale,
		global_real_argb_blue);

	return;
}

void render_debug_polygon_edges(
	real_point3d const *points,
	short point_count,
	real_argb_color const *color)
{
	short index;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		873,
		points);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		874,
		color);

	if (point_count>2)
	{
		render_debug_line(
			TRUE,
			&points[point_count-1],
			points,
			color);

		for (index = 1; index<point_count; index++)
		{
			render_debug_line(
				TRUE,
				&points[index-1],
				&points[index],
				color);
		}
	}

	return;
}

void render_debug_string(
	boolean immediate,
	char const *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		893,
		string);

	if (immediate)
	{
		interface_set_bitmap_text_draw_mode(
			_interface_font_terminal,
			NONE,
			0,
			0,
			_interface_color_table_dialog,
			0);
		rasterizer_draw_string(
			NULL,
			NULL,
			NULL,
			0,
			string);
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_string,
			string);
	}

	return;
}

void render_debug_vectors(
	boolean immediate,
	const real_point3d *point,
	const real_vector3d *forward,
	const real_vector3d *up,
	real size)
{
	real_matrix4x3 matrix;

	matrix4x3_from_point_and_vectors(&matrix, point, forward, up);
	render_debug_matrix(immediate, &matrix, size);
	return;
}

void render_debug_quaternion(
	boolean immediate,
	const real_point3d *point,
	const real_quaternion *quaternion,
	real size)
{
	real_matrix4x3 matrix;

	matrix4x3_from_point_and_quaternion(&matrix, point, quaternion);
	render_debug_matrix(immediate, &matrix, size);
	return;
}

void render_debug_box2d_outline(
	boolean immediate,
	real_rectangle2d const *bounds,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		745,
		bounds);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		746,
		color);

	if (immediate)
	{
		real_point3d points[4];

		points[0].x = bounds->x0;
		points[0].y = bounds->y0;
		points[0].z = -1.0f;
		points[1].x = bounds->x1;
		points[1].y = bounds->y0;
		points[1].z = -1.0f;
		points[2].x = bounds->x1;
		points[2].y = bounds->y1;
		points[2].z = -1.0f;
		points[3].x = bounds->x0;
		points[3].y = bounds->y1;
		points[3].z = -1.0f;

		matrix4x3_transform_point(
			&render.frustum.view_to_world,
			&points[0],
			&points[0]);
		matrix4x3_transform_point(
			&render.frustum.view_to_world,
			&points[1],
			&points[1]);
		matrix4x3_transform_point(
			&render.frustum.view_to_world,
			&points[2],
			&points[2]);
		matrix4x3_transform_point(
			&render.frustum.view_to_world,
			&points[3],
			&points[3]);

		render_debug_polygon_edges(
			points,
			NUMBEROF(points),
			color);
	}
	else
	{
		match_vassert(
			"c:\\halo\\SOURCE\\render\\render_debug.c",
			765,
			FALSE,
			"can't add box2d to debug cache");
	}

	return;
}

/* ---------- private code */

static char *render_debug_add_cache_string(
	char const *string)
{
	char *result = NULL;
	short offset;

	for (offset = 0; offset < render_debug_globals.string_offset; offset++)
	{
		if (!csstrcmp(string, &render_debug_globals.strings[offset]))
		{
			result = &render_debug_globals.strings[offset];
			break;
		}
	}

	if (!result)
	{
		if (render_debug_globals.string_offset < NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS - 1)
		{
			result = &render_debug_globals.strings[render_debug_globals.string_offset];

			csstrncpy(
				result,
				string,
				NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS - 1 - render_debug_globals.string_offset);
			render_debug_globals.strings[NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS - 1] = 0;

			render_debug_globals.string_offset += (short)csstrlen(string) + 1;
			render_debug_globals.string_offset = MIN(
				render_debug_globals.string_offset,
				NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS - 1);
		}
		else if (!render_debug_globals.string_overflow_reported)
		{
			error(_error_silent, "render debug cache string overflow");
			render_debug_globals.string_overflow_reported = TRUE;
		}
	}

	return result;
}

static void render_debug_add_cache_entry(
	short type,
	...)
{
	struct render_debug_cache_entry *entry;
	char const *cached_string;
	va_list list;

	if (render_debug_globals.game_time != (short)game_time_get())
	{
		render_debug_globals.game_time = (short)game_time_get();
		render_debug_globals.entry_count = 0;
		render_debug_globals.string_offset = 0;
		render_debug_globals.strings[0] = 0;
	}
	else if (render_debug_globals.entry_count >= MAXIMUM_RENDER_DEBUG_CACHE_ENTRIES)
	{
		if (!render_debug_globals.entry_overflow_reported)
		{
			error(_error_silent, "render debug cache overflow.");
			render_debug_globals.entry_overflow_reported = TRUE;
		}

		return;
	}

	entry = &render_debug_globals.entries[render_debug_globals.entry_count++];
	entry->type = type;

	/*
	 * MSVC i386 gives the named short a four-byte cdecl argument slot;
	 * va_start therefore begins at the following slot.  Arguments after the
	 * ellipsis use the C default promotions: short/boolean -> int, real -> double.
	 */
	va_start(list, type);

	switch (type)
	{
		case _render_debug_cache_circle:
			entry->circle.plane = *va_arg(list, real_plane3d const *);
			entry->circle.projection = (short)va_arg(list, int);
			entry->circle.sign = (boolean)va_arg(list, int);
			entry->circle.center = *va_arg(list, real_point2d const *);
			entry->circle.radius = (real)va_arg(list, double);
			entry->circle.color = *va_arg(list, real_argb_color const *);
			entry->circle.offset = (real)va_arg(list, double);
			break;

		case _render_debug_cache_point:
		case _render_debug_cache_sphere:
			entry->point.point = *va_arg(list, real_point3d const *);
			entry->point.size = (real)va_arg(list, double);
			entry->point.color = *va_arg(list, real_argb_color const *);
			break;

		case _render_debug_cache_line:
			entry->line.point0 = *va_arg(list, real_point3d const *);
			entry->line.point1 = *va_arg(list, real_point3d const *);
			entry->line.color = *va_arg(list, real_argb_color const *);
			break;

		case _render_debug_cache_cylinder:
			entry->pill.base = *va_arg(list, real_point3d const *);
			entry->pill.height = *va_arg(list, real_vector3d const *);
			entry->pill.width = (real)va_arg(list, double);
			entry->pill.color = *va_arg(list, real_argb_color const *);
			break;

		case _render_debug_cache_pill:
			entry->pill.base = *va_arg(list, real_point3d const *);
			entry->pill.height = *va_arg(list, real_vector3d const *);
			entry->pill.width = (real)va_arg(list, double);
			entry->pill.color = *va_arg(list, real_argb_color const *);
			break;

		case _render_debug_cache_box:
		case _render_debug_cache_box_outline:
			entry->box.bounds = *va_arg(list, real_rectangle3d const *);
			entry->box.color = *va_arg(list, real_argb_color const *);
			break;

		case _render_debug_cache_string:
			cached_string = render_debug_add_cache_string(va_arg(list, char const *));
			if (cached_string)
			{
				entry->string.string = cached_string;
			}
			else
			{
				render_debug_globals.entry_count--;
			}
			break;

		case _render_debug_cache_string_at_point:
			cached_string = render_debug_add_cache_string(va_arg(list, char const *));
			if (!cached_string)
			{
				render_debug_globals.entry_count--;
				break;
			}

			entry->string_at_point.string = cached_string;
			entry->string_at_point.point = *va_arg(list, real_point3d const *);
			entry->string_at_point.color = *va_arg(list, real_argb_color const *);
			break;
	}

	va_end(list);

	return;
}
