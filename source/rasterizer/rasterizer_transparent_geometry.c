/*
RASTERIZER_TRANSPARENT_GEOMETRY.C

symbols in this file:
00173AB0 00a0:
	_rasterizer_transparent_geometry_initialize (0000)
00173B50 0030:
	_rasterizer_transparent_geometry_begin (0000)
00173B80 0030:
	_rasterizer_transparent_geometry_new_group (0000)
00173BB0 0030:
	_rasterizer_transparent_geometry_new_group2 (0000)
00173BE0 0020:
	_rasterizer_transparent_geometry_get_groups2 (0000)
00173C00 00a0:
	_rasterizer_transparent_geometry_next_group (0000)
00173CA0 0010:
	_rasterizer_transparent_geometry_get_groups (0000)
00173CB0 0050:
	_rasterizer_transparent_geometry_get_group_from_presorted_index (0000)
00173D00 00c0:
	_rasterizer_transparent_geometry_get_group_presorted_index (0000)
00173DC0 0040:
	_rasterizer_transparent_geometry_get_group_pending_status (0000)
00173E00 0060:
	_rasterizer_transparent_geometry_set_group_pending_status (0000)
00173E60 0070:
	_rasterizer_transparent_geometry_get_primary_vertex_type (0000)
00173ED0 0010:
	_rasterizer_transparent_geometry_end (0000)
00173EE0 0080:
	_rasterizer_transparent_geometry_dispose (0000)
00173F60 0010:
	_rasterizer_transparent_geometry_stop (0000)
00173F70 0030:
	_code_00173f70 (0000)
00173FA0 0180:
	_code_00173fa0 (0000)
00174120 00b0:
	_code_00174120 (0000)
001741D0 01e0:
	_rasterizer_transparent_geometry_draw (0000)
0029F19C 0039:
	??_C@_0DJ@FPHMGBGH@?$CD?$CD?$CD?5ERROR?5failed?5to?5allocate?5tra@ (0000)
0029F1D8 003c:
	??_C@_0DM@PHFAKDJD@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029F214 001b:
	??_C@_0BL@MKAECLFF@next_group_sorted_index?$DO?$DN0?$AA@ (0000)
0029F230 004f:
	??_C@_0EP@FBJFLNLL@group?9?$DOsorted_index?$DO?$DN0?5?$CG?$CG?5group?9@ (0000)
0029F280 0053:
	??_C@_0FD@BILDLJMA@group_presorted_index?$DO?$DN0?5?$CG?$CG?5grou@ (0000)
0029F2D8 006f:
	??_C@_0GP@EGPBNBIE@?$CI?$CIunsigned?5long?$CJgroup?9?$CIunsigned?5@ (0000)
0029F348 0035:
	??_C@_0DF@OBMEADDC@?$CD?$CD?$CD?5ERROR?5transparent?5geometry?5g@ (0000)
0029F380 0057:
	??_C@_0FH@DJGEIAGF@group_index2?5?$CG?$CG?5?$CI?$CKgroup_index2?$CJ?$DO@ (0000)
0029F3D8 0057:
	??_C@_0FH@MLHLDOCN@group_index1?5?$CG?$CG?5?$CI?$CKgroup_index1?$CJ?$DO@ (0000)
0029F430 0013:
	??_C@_0BD@KIEBCHPI@?$CBfirst_person_flag?$AA@ (0000)
0029F444 0007:
	??_C@_06OGEFEFJ@?$CBwater?$AA@ (0000)
004B8AD8 004a:
	_bss_004b8ad8 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "rasterizer.h"
#include "rasterizer_geometry.h"

/* ---------- constants */

enum
{
	MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS = 384,
	MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS2 = 32,
};

/* ---------- macros */

/* ---------- structures */

/* January's assert strings name this type and its sorted_index field, and pin
the stride: the group array is walked with a 0xA0 element size. */
struct transparent_geometry_group
{
	byte opaque00[0x54];
	long dynamic_vertex_buffer_index;
	struct vertex_buffer const *vertex_buffer;
	byte opaque5c[0x34];
	long sorted_index;
	byte opaque94[0xC];
};

typedef char transparent_geometry_group_size_assert[
	sizeof(struct transparent_geometry_group) == 0xA0 ? 1 : -1];

#pragma pack(push, 1)
struct rasterizer_transparent_geometry_globals
{
	byte opaque00[0x4];
	unsigned long group_pending_flags[12];
	struct transparent_geometry_group *groups;
	struct transparent_geometry_group *groups2;
	long group_count;
	long group_count2;
	short *group_sorted_indices;
	short next_group_sorted_index;
};
#pragma pack(pop)

typedef char rasterizer_transparent_geometry_globals_size_assert[
	sizeof(struct rasterizer_transparent_geometry_globals) == 0x4A ? 1 : -1];

/* ---------- prototypes */

short rasterizer_transparent_geometry_get_group_presorted_index(
	struct transparent_geometry_group *group);
void rasterizer_set_stencil_mode(
	long stencil_mode);
void rasterizer_transparent_geometry_dispose_aux_buffer(
	void);

/* ---------- globals */

struct rasterizer_transparent_geometry_globals bss_004b8ad8;

/* January reached these as individual file-scope variables; its assert strings
name them. We pin the .bss layout with a struct because MSVC's allocation order
for separate statics does not reproduce it, so alias the attested spellings. */
#define transparent_geometry_groups bss_004b8ad8.groups
#define transparent_geometry_group_count bss_004b8ad8.group_count
#define transparent_geometry_group_sorted_indices bss_004b8ad8.group_sorted_indices
#define transparent_geometry_groups2 bss_004b8ad8.groups2
#define transparent_geometry_group_count2 bss_004b8ad8.group_count2

/* ---------- public code */

void rasterizer_transparent_geometry_begin(
	void)
{
	bss_004b8ad8.group_count = 0;
	bss_004b8ad8.next_group_sorted_index = 0;
	memset(bss_004b8ad8.group_pending_flags, 0, sizeof(bss_004b8ad8.group_pending_flags));
	bss_004b8ad8.group_count2 = 0;

	return;
}

void rasterizer_transparent_geometry_end(
	void)
{
	return;
}

short rasterizer_transparent_geometry_get_primary_vertex_type(
	struct transparent_geometry_group *group)
{
	short vertex_type = NONE;

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0xF4, group);

	if (group->vertex_buffer)
	{
		vertex_type = group->vertex_buffer->type;
	}
	else if (group->dynamic_vertex_buffer_index!=NONE)
	{
		vertex_type = rasterizer_dynamic_vertices_get_type(group->dynamic_vertex_buffer_index);
	}
	else
	{
		error(_error_silent, "### ERROR transparent geometry group has no vertices");
	}

	return vertex_type;
}

void rasterizer_transparent_geometry_set_group_pending_status(
	struct transparent_geometry_group *group,
	boolean pending)
{
	short group_presorted_index = rasterizer_transparent_geometry_get_group_presorted_index(group);

	if (group_presorted_index!=NONE)
		BIT_VECTOR_SET_FLAG(bss_004b8ad8.group_pending_flags, group_presorted_index, !pending);

	return;
}

boolean rasterizer_transparent_geometry_get_group_pending_status(
	struct transparent_geometry_group *group)
{
	short group_presorted_index = rasterizer_transparent_geometry_get_group_presorted_index(group);
	boolean pending = TRUE;

	if (group_presorted_index!=NONE)
		pending = !BIT_VECTOR_TEST_FLAG(bss_004b8ad8.group_pending_flags, group_presorted_index);

	return pending;
}

short rasterizer_transparent_geometry_get_group_presorted_index(
	struct transparent_geometry_group *group)
{
	short group_presorted_index = NONE;

	if (group>=transparent_geometry_groups &&
		group<transparent_geometry_groups+transparent_geometry_group_count)
	{
		group_presorted_index = (short)(group-transparent_geometry_groups);

		match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0xCB,
			group_presorted_index>=0 && group_presorted_index<transparent_geometry_group_count);
		match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0xCC,
			((unsigned long)group-(unsigned long)transparent_geometry_groups)%sizeof(struct transparent_geometry_group)==0);
	}

	return group_presorted_index;
}

void *rasterizer_transparent_geometry_get_group_from_presorted_index(
	short group_presorted_index)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0xBC,
		group_presorted_index>=0 && group_presorted_index<transparent_geometry_group_count);

	return (byte *)bss_004b8ad8.groups + group_presorted_index*0xA0;
}

void *rasterizer_transparent_geometry_get_groups2(
	short *group_count)
{
	if (group_count)
		*group_count = (short)bss_004b8ad8.group_count2;

	return bss_004b8ad8.groups2;
}

void *rasterizer_transparent_geometry_get_groups(
	void)
{
	return transparent_geometry_groups;
}

struct transparent_geometry_group *rasterizer_transparent_geometry_new_group(
	void)
{
	struct transparent_geometry_group *group = NULL;
	long group_index = transparent_geometry_group_count;

	if (group_index<MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS)
	{
		group = transparent_geometry_groups + group_index;
		group->sorted_index = group_index;
		transparent_geometry_group_count = group_index + 1;
	}

	return group;
}

struct transparent_geometry_group *rasterizer_transparent_geometry_new_group2(
	void)
{
	struct transparent_geometry_group *group = NULL;
	long group_index = transparent_geometry_group_count2;

	if (group_index<MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS2)
	{
		group = transparent_geometry_groups2 + group_index;
		group->sorted_index = group_index;
		transparent_geometry_group_count2 = group_index + 1;
	}

	return group;
}

void rasterizer_transparent_geometry_stop(
	void)
{
	rasterizer_set_stencil_mode(0);

	return;
}

void rasterizer_transparent_geometry_dispose(
	void)
{
	rasterizer_transparent_geometry_dispose_aux_buffer();

	if (transparent_geometry_groups)
		debug_free(transparent_geometry_groups,
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0x111);
	transparent_geometry_groups = NULL;

	if (transparent_geometry_group_sorted_indices)
		debug_free(transparent_geometry_group_sorted_indices,
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0x114);
	transparent_geometry_group_sorted_indices = NULL;

	if (transparent_geometry_groups2)
		debug_free(transparent_geometry_groups2,
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer_transparent_geometry.c", 0x118);
	transparent_geometry_groups2 = NULL;

	transparent_geometry_group_count2 = 0;
	transparent_geometry_group_count = 0;

	return;
}

/* ---------- private code */
