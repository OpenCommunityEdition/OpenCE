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

#define REAL_MATH_EXTERNAL_ARCTANGENT
#define REAL_MATH_EXTERNAL_DOT_PRODUCT3D
#define REAL_MATH_EXTERNAL_PLANE3D_DISTANCE_TO_POINT
#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "ai/ai_profile.h"
#include "ai/path.h"
#include "cache/sound_cache.h"
#include "effects/decal_definitions.h"
#include "effects/decals.h"
#include "game/game.h"
#include "game/players.h"
#include "hs/hs.h"
#include "interface/interface.h"
#include "input/input.h"
#include "math/real_math.h"
#include "physics/bsp3d.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_debug.h"
#include "render/render.h"
#include "render/render_cameras_internal.h"
#include "render/render_debug.h"
#include "render/render_debug_geometry.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "structures/structure_vector_tests.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "units/bipeds.h"
#include "units/vehicles.h"

/* ---------- constants */

enum
{
	NUMBER_OF_RENDER_DEBUG_CACHE_STRING_CHARACTERS = 1024,
	MAXIMUM_RENDER_DEBUG_CACHE_ENTRIES = 512,

	NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS = 16,
	MAXIMUM_BSP3D_DEPTH = 128,
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

#define vehicle_runtime_try_and_get(index) \
	((struct vehicle_runtime_datum *)object_try_and_get_and_verify_type((index), _object_mask_vehicle))

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

struct render_debug_vehicle_data
{
	word flags;
	short unknown426;
	byte unknown428[4];
	real speed;
	real slide;
	real turn;
	byte unknown438[0x40];
	long stuck;
};

struct vehicle_runtime_datum
{
	long definition_index;
	struct _object_datum object;
	struct _unit_datum unit;
	struct render_debug_vehicle_data vehicle;
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
typedef char render_debug_vehicle_speed_offset_check[
	offsetof(struct vehicle_runtime_datum, vehicle.speed) == 0x42C ? 1 : -1];
typedef char render_debug_vehicle_slide_offset_check[
	offsetof(struct vehicle_runtime_datum, vehicle.slide) == 0x430 ? 1 : -1];
typedef char render_debug_vehicle_turn_offset_check[
	offsetof(struct vehicle_runtime_datum, vehicle.turn) == 0x434 ? 1 : -1];
typedef char render_debug_vehicle_stuck_offset_check[
	offsetof(struct vehicle_runtime_datum, vehicle.stuck) == 0x478 ? 1 : -1];

/* ---------- prototypes */

static char *render_debug_add_cache_string(
	char const *string);
static void build_circle_points(
	real radius,
	real_point2d *points);
static real build_height_matrix(
	real_point3d const *base,
	real_vector3d const *height,
	real_matrix4x3 *matrix);
static void build_pill_points(
	real_point3d const *base,
	real_vector3d const *height,
	real width,
	real_point3d *top_points,
	real_point3d *bottom_points,
	real_point3d *top_yz_points,
	real_point3d *bottom_yz_points,
	real_point3d *top_xz_points,
	real_point3d *bottom_xz_points);
static void render_debug_add_cache_entry(
	short type,
	...);
static void render_debug_bsp(
	void);
static void render_debug_camera(
	void);
static void render_debug_input(
	void);
static void render_debug_player(
	void);
static void render_debug_structure(
	void);
static void render_debug_structure_decals(
	void);

/* ---------- globals */

extern boolean debug_bsp;
extern boolean debug_camera;
extern boolean debug_input;
extern boolean debug_permanent_decals;
extern boolean debug_player;
extern boolean debug_structure;

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

void render_debug(
	void)
{
	short entry_index;

	ai_debug_render();
	ai_profile_render();
	render_debug_object_damage();
	render_debug_scripting();
	render_debug_trigger_volumes();
	texture_cache_debug_render();
	sound_cache_debug_render();
	render_debug_recording();
	render_debug_detail_objects();
	collision_debug_render();
	collision_log_render();
	render_debug_obstacle_path();
	render_debug_fog_planes();
	render_debug_decals();
	render_debug_camera();
	render_debug_player();
	render_debug_structure();
	render_debug_bsp();
	render_debug_input();
	render_debug_structure_decals();
	players_debug_render();

	for (entry_index = 0; entry_index < render_debug_globals.entry_count; entry_index++)
	{
		struct render_debug_cache_entry *entry = &render_debug_globals.entries[entry_index];

		switch (entry->type)
		{
			case _render_debug_cache_circle:
				render_debug_circle(
					TRUE,
					&entry->circle.plane,
					entry->circle.projection,
					entry->circle.sign,
					&entry->circle.center,
					entry->circle.radius,
					&entry->circle.color,
					entry->circle.offset);
				break;

			case _render_debug_cache_point:
				render_debug_point(
					TRUE,
					&entry->point.point,
					entry->point.size,
					&entry->point.color);
				break;

			case _render_debug_cache_line:
				render_debug_line(
					TRUE,
					&entry->line.point0,
					&entry->line.point1,
					&entry->line.color);
				break;

			case _render_debug_cache_sphere:
				render_debug_sphere(
					TRUE,
					&entry->point.point,
					entry->point.size,
					&entry->point.color);
				break;

			case _render_debug_cache_cylinder:
				render_debug_cylinder(
					TRUE,
					&entry->pill.base,
					&entry->pill.height,
					entry->pill.width,
					&entry->pill.color);
				break;

			case _render_debug_cache_pill:
				render_debug_pill(
					TRUE,
					&entry->pill.base,
					&entry->pill.height,
					entry->pill.width,
					&entry->pill.color);
				break;

			case _render_debug_cache_box:
				render_debug_box(
					TRUE,
					&entry->box.bounds,
					&entry->box.color);
				break;

			case _render_debug_cache_box_outline:
				render_debug_box_outline(
					TRUE,
					&entry->box.bounds,
					&entry->box.color);
				break;

			case _render_debug_cache_string:
				render_debug_string(
					TRUE,
					entry->string.string);
				break;

			case _render_debug_cache_string_at_point:
				render_debug_string_at_point(
					TRUE,
					&entry->string_at_point.point,
					entry->string_at_point.string,
					&entry->string_at_point.color);
				break;

			default:
				match_vassert(
					"c:\\halo\\SOURCE\\render\\render_debug.c",
					1269,
					FALSE,
					NULL);
				break;
		}
	}

	if (render_debug_globals.game_time != (short)game_time_get() - 1)
	{
		render_debug_globals.game_time = (short)game_time_get();
		render_debug_globals.entry_count = 0;
		render_debug_globals.string_offset = 0;
		render_debug_globals.strings[0] = 0;
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

void render_debug_sphere(
	boolean immediate,
	real_point3d const *center,
	real radius,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		518,
		center);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		519,
		color);

	if (immediate)
	{
		if (render_frustum_sphere_visible(&render.frustum, center, radius))
		{
			real_point2d points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
			real_point3d point0;
			real_point3d point1;
			short index;

			build_circle_points(radius, points);

			for (index = 1; index<NUMBEROF(points); index++)
			{
				point0.x = center->x + points[index-1].x;
				point0.y = center->y + points[index-1].y;
				point0.z = center->z;
				point1.x = center->x + points[index].x;
				point1.y = center->y + points[index].y;
				point1.z = center->z;
				rasterizer_debug_line(&point0, &point1, color);

				point0.x = center->x + points[index-1].y;
				point0.y = center->y;
				point0.z = center->z + points[index-1].x;
				point1.x = center->x + points[index].y;
				point1.y = center->y;
				point1.z = center->z + points[index].x;
				rasterizer_debug_line(&point0, &point1, color);

				point0.x = center->x;
				point0.y = center->y + points[index-1].x;
				point0.z = center->z + points[index-1].y;
				point1.x = center->x;
				point1.y = center->y + points[index].x;
				point1.z = center->z + points[index].y;
				rasterizer_debug_line(&point0, &point1, color);
			}
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_sphere,
			center,
			radius,
			color);
	}

	return;
}

void render_debug_cylinder(
	boolean immediate,
	real_point3d const *base,
	real_vector3d const *height,
	real width,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		645,
		base);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		646,
		height);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		647,
		color);

	if (immediate)
	{
		real_point3d bottom_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
		real_point3d top_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
		short index;

		build_pill_points(
			base,
			height,
			width,
			top_points,
			bottom_points,
			NULL,
			NULL,
			NULL,
			NULL);

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS; index++)
		{
			rasterizer_debug_line(&bottom_points[index], &bottom_points[index+1], color);
			rasterizer_debug_line(&top_points[index], &top_points[index+1], color);
		}

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS; index+= NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/4)
		{
			rasterizer_debug_line(&bottom_points[index], &top_points[index], color);
		}

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2; index+= NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/4)
		{
			rasterizer_debug_line(&bottom_points[index], &bottom_points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2], color);
			rasterizer_debug_line(&top_points[index], &top_points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2], color);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_cylinder,
			base,
			height,
			width,
			color);
	}

	return;
}

void render_debug_pill(
	boolean immediate,
	real_point3d const *base,
	real_vector3d const *height,
	real width,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		693,
		base);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		694,
		height);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		695,
		color);

	if (immediate)
	{
		real_point3d top_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
		real_point3d bottom_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
		real_point3d top_yz_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2+1];
		real_point3d bottom_yz_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2+1];
		real_point3d top_xz_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2+1];
		real_point3d bottom_xz_points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2+1];
		short index;

		build_pill_points(
			base,
			height,
			width,
			top_points,
			bottom_points,
			top_yz_points,
			bottom_yz_points,
			top_xz_points,
			bottom_xz_points);

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS; index++)
		{
			rasterizer_debug_line(&top_points[index], &top_points[index+1], color);
			rasterizer_debug_line(&bottom_points[index], &bottom_points[index+1], color);
		}

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS; index+= NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/4)
		{
			rasterizer_debug_line(&top_points[index], &bottom_points[index], color);
		}

		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2; index++)
		{
			rasterizer_debug_line(&top_yz_points[index], &top_yz_points[index+1], color);
			rasterizer_debug_line(&bottom_yz_points[index], &bottom_yz_points[index+1], color);
			rasterizer_debug_line(&top_xz_points[index], &top_xz_points[index+1], color);
			rasterizer_debug_line(&bottom_xz_points[index], &bottom_xz_points[index+1], color);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_pill,
			base,
			height,
			width,
			color);
	}

	return;
}

void render_debug_box(
	boolean immediate,
	real_rectangle3d const *bounds,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		776,
		bounds);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		777,
		color);

	if (immediate)
	{
		real_point3d points[8];

		points[0].x = bounds->x0;
		points[0].y = bounds->y0;
		points[0].z = bounds->z0;
		points[1].x = bounds->x1;
		points[1].y = bounds->y0;
		points[1].z = bounds->z0;
		points[2].x = bounds->x0;
		points[2].y = bounds->y0;
		points[2].z = bounds->z1;
		points[3].x = bounds->x1;
		points[3].y = bounds->y0;
		points[3].z = bounds->z1;
		points[4].x = bounds->x0;
		points[4].y = bounds->y1;
		points[4].z = bounds->z1;
		points[5].x = bounds->x1;
		points[5].y = bounds->y1;
		points[5].z = bounds->z1;
		points[6].x = bounds->x0;
		points[6].y = bounds->y1;
		points[6].z = bounds->z0;
		points[7].x = bounds->x1;
		points[7].y = bounds->y1;
		points[7].z = bounds->z0;

		render_debug_quadrilateral(
			TRUE, &points[0], &points[2], &points[4], &points[6], color);
		render_debug_quadrilateral(
			TRUE, &points[1], &points[3], &points[5], &points[7], color);
		render_debug_quadrilateral(
			TRUE, &points[0], &points[1], &points[3], &points[2], color);
		render_debug_quadrilateral(
			TRUE, &points[4], &points[5], &points[7], &points[6], color);
		render_debug_quadrilateral(
			TRUE, &points[0], &points[1], &points[7], &points[6], color);
		render_debug_quadrilateral(
			TRUE, &points[2], &points[3], &points[5], &points[4], color);
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_box,
			bounds,
			color);
	}

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

void render_debug_string_at_point(
	boolean immediate,
	real_point3d const *point,
	char const *string,
	real_argb_color const *color)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		914,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		915,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		916,
		color);

	if (immediate)
	{
		real_point2d screen_point;

		if (render_camera_world_to_screen(
			&render.camera,
			&render.frustum,
			point,
			&screen_point))
		{
			rectangle2d bounds;

			bounds.x0 = (short)(screen_point.x - render.camera.viewport_bounds.x0);
			bounds.y0 = (short)(screen_point.y - render.camera.viewport_bounds.y0);
			bounds.x1 = SHORT_MAX;
			bounds.y1 = SHORT_MAX;

			interface_set_bitmap_text_draw_mode(
				_interface_font_terminal,
				NONE,
				0,
				0,
				_interface_color_table_dialog,
				0);
			draw_string_set_color(color);
			rasterizer_draw_string(
				&bounds,
				NULL,
				NULL,
				0,
				string);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_string_at_point,
			string,
			point,
			color);
	}

	return;
}

static void render_debug_camera(
	void)
{
	if (debug_camera)
	{
		char text[2048];
		struct location location;
		struct collision_result collision;
		real_vector3d down;
		real_vector3d forward;
		real_point3d collision_point;
		short material_index;
		short lightmap_index;
		long surface_index;
		real s;
		real t;

		scenario_location_from_point(
			&location,
			&render.camera.position);
		_snprintf(
			text,
			sizeof(text),
			"point(%01.2f,%01.2f,%01.2f) leaf(#%d [%d]) cluster(#%d [%d])",
			render.camera.position.x,
			render.camera.position.y,
			render.camera.position.z,
			location.leaf_index,
			render.leaf_index,
			location.cluster_index,
			render.cluster_index);

		scale_vector3d(
			global_down3d,
			1000.0f,
			&down);
		if (collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_structure_bit),
			&render.camera.position,
			&down,
			NONE,
			&collision))
		{
			_snprintf(
				text + csstrlen(text),
				sizeof(text) - csstrlen(text),
				"|nground_point(%01.2f,%01.2f,%01.2f) facing(%01.2f) surface(#%d)",
				collision.point.x,
				collision.point.y,
				collision.point.z,
				(real)atan2(render.camera.forward.j, render.camera.forward.i) * 57.29578f,
				collision.surface_index);
		}

		scale_vector3d(
			&render.camera.forward,
			50.0f,
			&forward);
		if (structure_test_vector(
			&render.camera.position,
			&forward,
			&collision_point,
			&lightmap_index,
			&material_index,
			&surface_index,
			&s,
			&t))
		{
			struct structure_material *material = TAG_BLOCK_GET_ELEMENT(
				&TAG_BLOCK_GET_ELEMENT(
					&global_structure_bsp_get()->lightmaps,
					lightmap_index,
					struct structure_lightmap)->materials,
				material_index,
				struct structure_material);

			_snprintf(
				text + csstrlen(text),
				sizeof(text) - csstrlen(text),
				"|n%s",
				material->shader.name);
		}

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
			text);
	}

	return;
}

static void render_debug_player(
	void)
{
	if (debug_player &&
		render.local_player_index != NONE &&
		local_player_get_player_index(render.local_player_index) != NONE)
	{
		struct player_datum *player = player_get(
			local_player_get_player_index(render.local_player_index));

		if (player->unit_index != NONE)
		{
			struct biped_datum *biped = biped_try_and_get(player->unit_index);

			if (biped && biped->biped.elevator_object_index != NONE)
			{
				render_debug_string(
					TRUE,
					"riding an elevator");
			}

			if (biped->object.parent_object_index != NONE && biped->unit.parent_seat_index != NONE)
			{
				struct vehicle_runtime_datum *vehicle = vehicle_runtime_try_and_get(
					biped->object.parent_object_index);
				char text[1024];

				sprintf(
					text,
					"speed %5f|nslide %5f|nturn  %5f%s",
					vehicle->vehicle.speed,
					vehicle->vehicle.slide,
					vehicle->vehicle.turn,
					vehicle->vehicle.stuck ? "|nstuck!" : "");
				render_debug_string(
					TRUE,
					text);
			}
		}
	}

	return;
}

static void render_debug_structure(
	void)
{
	if (debug_structure)
	{
		struct collision_bsp *collision_bsp = global_collision_bsp_get();
		long edge_index;

		for (edge_index = 0; edge_index < collision_bsp->edges.count; edge_index++)
		{
			struct collision_edge *edge = TAG_BLOCK_GET_ELEMENT(
				&collision_bsp->edges,
				edge_index,
				struct collision_edge);
			struct collision_vertex *vertex0 = TAG_BLOCK_GET_ELEMENT(
				&collision_bsp->vertices,
				edge->vertex_indices[0],
				struct collision_vertex);
			struct collision_vertex *vertex1 = TAG_BLOCK_GET_ELEMENT(
				&collision_bsp->vertices,
				edge->vertex_indices[1],
				struct collision_vertex);

			render_debug_line(
				TRUE,
				&vertex0->point,
				&vertex1->point,
				global_real_argb_green);
		}
	}

	return;
}

static void render_debug_bsp(
	void)
{
	if (debug_bsp)
	{
		char text[2048];
		long plane_designators[MAXIMUM_BSP3D_DEPTH];
		short plane_count = 0;
		struct bsp3d *bsp = global_bsp3d_get();
		char *current = text + sprintf(text, " node plane|n");
		long node_index = 0;

		while (!(node_index & LONG_MIN))
		{
			struct bsp3d_node *node = TAG_BLOCK_GET_ELEMENT(
				&bsp->nodes,
				node_index,
				struct bsp3d_node);
			real_plane3d *plane = TAG_BLOCK_GET_ELEMENT(
				&bsp->planes,
				node->plane_designator,
				real_plane3d);
			real_point3d const *point = &render.camera.position;
			boolean side =
				point->x*plane->n.i +
				point->y*plane->n.j +
				point->z*plane->n.k -
				plane->d >= 0.0f;

			match_assert(
				"c:\\halo\\SOURCE\\render\\render_debug.c",
				1618,
				plane_count<MAXIMUM_BSP3D_DEPTH);
			plane_designators[plane_count++] = node->plane_designator;
			current += sprintf(
				current,
				"%5d %5d %c|n",
				node_index,
				node->plane_designator,
				side ? '+' : '-');
			node_index = node->children[side];
		}

		if (node_index != NONE)
		{
			sprintf(
				current,
				" leaf %5d",
				node_index & LONG_MAX);
		}
		else
		{
			sprintf(
				current,
				"solid");
		}

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
			text);

		if (input_key_is_down(_key_b))
		{
			FILE *file = fopen("d:\\debug_bsp.txt", "w");

			if (file)
			{
				short plane_index;

				fprintf(file, "%d\n", plane_count);
				for (plane_index = 0; plane_index < plane_count; plane_index++)
				{
					fprintf(file, "%d\n", plane_designators[plane_index]);
				}
				fclose(file);
			}
		}
	}

	return;
}

static void render_debug_input(
	void)
{
	if (debug_input)
	{
		short tab_stops[3];
		char buffer[512];

		tab_stops[0] = 200;
		tab_stops[1] = 400;
		tab_stops[2] = 550;
		draw_string_set_tab_stops(
			tab_stops,
			NUMBEROF(tab_stops));
		input_get_raw_data_string(
			buffer,
			sizeof(buffer) - 1);
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
			buffer);
	}

	return;
}

static void render_debug_structure_decals(
	void)
{
	if (debug_permanent_decals)
	{
		struct structure_bsp *structure_bsp = global_structure_bsp_get();
		long decal_index;

		for (decal_index = 0; decal_index < structure_bsp->runtime_decals.count; decal_index++)
		{
			struct structure_runtime_decal *decal = TAG_BLOCK_GET_ELEMENT(
				&structure_bsp->runtime_decals,
				decal_index,
				struct structure_runtime_decal);
			long decal_definition_index = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->decal_palette,
				decal->palette_index,
				struct scenario_decal_palette_entry)->reference.index;
			long leaf_index;
			long cluster_index;
			struct structure_cluster_runtime_decals *cluster;
			struct collision_result collision;
			real_vector3d vector;

			decal_definition_get(decal_definition_index);
			leaf_index = scenario_leaf_index_from_point(&decal->position);
			cluster_index = leaf_index == NONE
				? NONE
				: TAG_BLOCK_GET_ELEMENT(
					&global_structure_bsp_get()->leaves,
					scenario_leaf_index_from_point(&decal->position) & LONG_MAX,
					struct structure_leaf)->cluster_index;
			cluster = TAG_BLOCK_GET_ELEMENT(
				&structure_bsp->clusters,
				cluster_index,
				struct structure_cluster_runtime_decals);

			{
				real_euler_angles2d angles =
				{
					decal->yaw * (_pi / 127.0f),
					decal->pitch * (_pi / 254.0f),
				};

				vector3d_from_euler_angles2d(&vector, &angles);

				if (collision_test_vector(
					FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_structure_bit) |
						FLAG(_collision_test_media_bit),
					&decal->position,
					&vector,
					NONE,
					&collision))
				{
					if (cluster->first_decal_index != NONE &&
						decal_index - cluster->first_decal_index < cluster->decal_count)
					{
						render_debug_sphere(
							TRUE,
							&collision.point,
							0.1f,
							global_real_argb_yellow);
					}
					else
					{
						render_debug_sphere(
							TRUE,
							&collision.point,
							0.1f,
							global_real_argb_orange);
					}
				}
				else
				{
					render_debug_sphere(
						TRUE,
						&decal->position,
						0.1f,
						global_real_argb_red);
				}

				render_debug_string_at_point(
					FALSE,
					&decal->position,
					tag_name_strip_path(tag_get_name(decal_definition_index)),
					global_real_argb_green);
			}
		}
	}

	return;
}

void render_debug_circle(
	boolean immediate,
	real_plane3d const *plane,
	short projection,
	boolean sign,
	real_point2d const *center,
	real radius,
	real_argb_color const *color,
	real offset)
{
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		290,
		plane);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		291,
		center);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		292,
		color);

	if (immediate)
	{
		real_point2d points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
		real_point2d point0;
		real_point2d point1;
		short index;

		build_circle_points(radius, points);

		for (index = 1; index<NUMBEROF(points); index++)
		{
			point0.x = center->x + points[index-1].x;
			point0.y = center->y + points[index-1].y;
			point1.x = center->x + points[index].x;
			point1.y = center->y + points[index].y;
			render_debug_line2d(
				TRUE,
				plane,
				projection,
				sign,
				&point0,
				&point1,
				color,
				offset);
		}
	}
	else
	{
		render_debug_add_cache_entry(
			_render_debug_cache_circle,
			plane,
			projection,
			sign,
			center,
			radius,
			color);
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

void render_debug_point2d(
	boolean immediate,
	real_plane3d const *plane,
	short projection,
	boolean sign,
	real_point2d const *point,
	real size,
	real_argb_color const *color,
	real offset)
{
	real_point3d point3d;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		219,
		plane);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		220,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		221,
		color);

	project_point2d(
		point,
		plane,
		projection,
		sign,
		&point3d);
	point3d.n[projection] += sign ? offset : -offset;

	render_debug_point(
		immediate,
		&point3d,
		size,
		color);

	return;
}

void render_debug_line2d(
	boolean immediate,
	real_plane3d const *plane,
	short projection,
	boolean sign,
	real_point2d const *p0,
	real_point2d const *p1,
	real_argb_color const *color,
	real offset)
{
	real_point3d point0;
	real_point3d point1;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		242,
		plane);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		243,
		p0);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		244,
		p1);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		245,
		color);

	project_point2d(
		p0,
		plane,
		projection,
		sign,
		&point0);
	project_point2d(
		p1,
		plane,
		projection,
		sign,
		&point1);
	point0.n[projection] += sign ? offset : -offset;
	point1.n[projection] += sign ? offset : -offset;

	render_debug_line(
		immediate,
		&point0,
		&point1,
		color);

	return;
}

void render_debug_vector2d(
	boolean immediate,
	real_plane3d const *plane,
	short projection,
	boolean sign,
	real_point2d const *point,
	real_vector2d const *vector,
	real size,
	real_argb_color const *color,
	real offset)
{
	real_point2d end_point;

	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		269,
		plane);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		270,
		point);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		271,
		vector);
	match_assert(
		"c:\\halo\\SOURCE\\render\\render_debug.c",
		272,
		color);

	end_point.x = point->x + size*vector->i;
	end_point.y = point->y + size*vector->j;

	render_debug_line2d(
		immediate,
		plane,
		projection,
		sign,
		point,
		&end_point,
		color,
		offset);

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

static void build_circle_points(
	real radius,
	real_point2d *points)
{
	real sine_of_angle = sine(2*_pi/NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS);
	real cosine_of_angle = cosine(2*_pi/NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS);
	short index;

	set_real_point2d(&points[0], radius, 0.f);

	for (index = 0; index+1<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS; index++)
	{
		set_real_point2d(
			&points[index+1],
			cosine_of_angle*points[index].x - sine_of_angle*points[index].y,
			sine_of_angle*points[index].x + cosine_of_angle*points[index].y);
	}

	points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS] = points[0];

	return;
}

static real build_height_matrix(
	real_point3d const *base,
	real_vector3d const *height,
	real_matrix4x3 *matrix)
{
	real height_magnitude;

	matrix->scale = 1.f;
	matrix->up = *height;
	perpendicular3d(&matrix->up, &matrix->left);
	height_magnitude = normalize3d(&matrix->up);
	normalize3d(&matrix->left);
	cross_product3d(&matrix->left, &matrix->up, &matrix->forward);
	matrix->position = *base;

	return height_magnitude;
}

static void build_pill_points(
	real_point3d const *base,
	real_vector3d const *height,
	real width,
	real_point3d *top_points,
	real_point3d *bottom_points,
	real_point3d *top_yz_points,
	real_point3d *bottom_yz_points,
	real_point3d *top_xz_points,
	real_point3d *bottom_xz_points)
{
	real_matrix4x3 matrix;
	real_point2d points[NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS+1];
	real height_magnitude = build_height_matrix(base, height, &matrix);
	short index;

	build_circle_points(width, points);

	if (top_points && bottom_points)
	{
		for (index = 0; index<NUMBEROF(points); index++)
		{
			top_points[index].x = points[index].x;
			top_points[index].y = points[index].y;
			top_points[index].z = height_magnitude;
			bottom_points[index].x = points[index].x;
			bottom_points[index].y = points[index].y;
			bottom_points[index].z = 0.f;
			matrix4x3_transform_point(&matrix, &top_points[index], &top_points[index]);
			matrix4x3_transform_point(&matrix, &bottom_points[index], &bottom_points[index]);
		}
	}

	if (top_yz_points && bottom_yz_points && top_xz_points && bottom_xz_points)
	{
		for (index = 0; index<NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2+1; index++)
		{
			top_yz_points[index].x = 0.f;
			top_yz_points[index].y = points[index].x;
			top_yz_points[index].z = height_magnitude + points[index].y;
			bottom_yz_points[index].x = 0.f;
			bottom_yz_points[index].y = points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2].x;
			bottom_yz_points[index].z = points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2].y;
			top_xz_points[index].x = points[index].x;
			top_xz_points[index].y = 0.f;
			top_xz_points[index].z = height_magnitude + points[index].y;
			bottom_xz_points[index].x = points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2].x;
			bottom_xz_points[index].y = 0.f;
			bottom_xz_points[index].z = points[index+NUMBER_OF_RENDER_DEBUG_CIRCLE_POINTS/2].y;
			matrix4x3_transform_point(&matrix, &top_yz_points[index], &top_yz_points[index]);
			matrix4x3_transform_point(&matrix, &bottom_yz_points[index], &bottom_yz_points[index]);
			matrix4x3_transform_point(&matrix, &top_xz_points[index], &top_xz_points[index]);
			matrix4x3_transform_point(&matrix, &bottom_xz_points[index], &bottom_xz_points[index]);
		}
	}

	return;
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
