/*
STRUCTURES.C

symbols in this file:
00187C10 0010:
	_structures_initialize (0000)
00187C20 0010:
	_structures_initialize_for_new_map (0000)
00187C30 0010:
	_structures_dispose_from_old_map (0000)
00187C40 0010:
	_structures_dispose (0000)
00187C50 0040:
	_structure_cluster_marker_begin (0000)
00187C90 0080:
	_structure_cluster_unmarked (0000)
00187D10 0080:
	_structure_cluster_mark (0000)
00187D90 0040:
	_structure_cluster_marker_end (0000)
00187DD0 01b0:
	_structure_render_surface_from_point_and_leaf (0000)
00187F80 00d0:
	_structure_get_planar_fog_definition_index (0000)
00188050 01b0:
	_sphere_intersects_cluster_portal (0000)
00188200 0120:
	_code_00188200 (0000)
00188320 01e0:
	_structure_clusters_in_cone (0000)
00188500 0260:
	_structure_test_vector (0000)
00188760 01c0:
	_structure_get_planar_fog (0000)
00188920 0160:
	_render_debug_fog_planes (0000)
00188A80 0130:
	_structure_clusters_in_sphere (0000)
002A1E54 002e:
	??_C@_0CO@IAFCMJGA@?$CBstructure_globals?4cluster_marke@ (0000)
002A1E84 0027:
	??_C@_0CH@MEPFLBCJ@c?3?2halo?2SOURCE?2structures?2struct@ (0000)
002A1EAC 002d:
	??_C@_0CN@MDKOILJF@structure_globals?4cluster_marker@ (0000)
002A1EDC 002b:
	??_C@_0CL@OOHBKJHI@stack_depth?$DMMAXIMUM_CLUSTERS_PER@ (0000)
002A1F08 0002:
	??_C@_01LKDEMHDF@s?$AA@ (0000)
002A1F0C 000e:
	??_C@_0O@GNJLHEFA@surface_index?$AA@ (0000)
002A1F1C 000f:
	??_C@_0P@MLEINILI@material_index?$AA@ (0000)
002A1F2C 0014:
	??_C@_0BE@GKJHHJJB@intersected_indices?$AA@ (0000)
004C1100 0808:
	_bss_004c1100 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "structures.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct structure_runtime_globals
{
	boolean initialized;
	boolean cluster_marker_initialized;
	byte reserved0002[2];
	long cluster_marker;
	long cluster_magic_numbers[MAXIMUM_CLUSTERS_PER_STRUCTURE];
};

typedef char verify_structure_cluster_marker_initialized_offset[
	offsetof(
		struct structure_runtime_globals,
		cluster_marker_initialized) == 0x1 ? 1 : -1];
typedef char verify_structure_cluster_marker_offset[
	offsetof(
		struct structure_runtime_globals,
		cluster_marker) == 0x4 ? 1 : -1];
typedef char verify_structure_cluster_magic_numbers_offset[
	offsetof(
		struct structure_runtime_globals,
		cluster_magic_numbers) == 0x8 ? 1 : -1];
typedef char verify_structure_runtime_globals_size[
	sizeof(struct structure_runtime_globals) == 0x808 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct structure_runtime_globals bss_004c1100;

#define structure_globals bss_004c1100

/* ---------- public code */

void structures_initialize(
	void)
{
	structure_detail_objects_initialize();
	structure_decals_initialize();
	return;
}

void structures_initialize_for_new_map(
	void)
{
	structure_detail_objects_initialize_for_new_map();
	structure_decals_initialize_for_new_map();
	return;
}

void structures_dispose_from_old_map(
	void)
{
	structure_decals_dispose_from_old_map();
	structure_detail_objects_dispose_from_old_map();
	return;
}

void structures_dispose(
	void)
{
	structure_decals_dispose();
	structure_detail_objects_dispose();
	return;
}

void structure_cluster_marker_begin(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\structures\\structures.c",
		0x103,
		!structure_globals.cluster_marker_initialized);
	structure_globals.cluster_marker++;
	structure_globals.cluster_marker_initialized = TRUE;

	return;
}

void structure_cluster_marker_end(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\structures\\structures.c",
		0x130,
		structure_globals.cluster_marker_initialized);
	structure_globals.cluster_marker_initialized = FALSE;

	return;
}

/* ---------- private code */
