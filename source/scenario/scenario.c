/*
SCENARIO.C

symbols in this file:
0017DA90 0020:
	_code_0017da90 (0000)
0017DAB0 0020:
	_code_0017dab0 (0000)
0017DAD0 0020:
	_scenario_initialize (0000)
0017DAF0 0050:
	_scenario_initialize_for_new_map (0000)
0017DB40 0010:
	_scenario_dispose_from_old_map (0000)
0017DB50 0010:
	_scenario_frame_update (0000)
0017DB60 0070:
	_scenario_unload (0000)
0017DBD0 0030:
	_global_scenario_get (0000)
0017DC00 0010:
	_global_scenario_try_and_get (0000)
0017DC10 0030:
	_global_structure_bsp_get (0000)
0017DC40 0030:
	_global_collision_bsp_get (0000)
0017DC70 0030:
	_global_bsp3d_get (0000)
0017DCA0 0030:
	_scenario_get_game_globals (0000)
0017DCD0 0050:
	_global_structure_bsp_tag_index_get (0000)
0017DD20 0010:
	_scenario_location_award_bonus (0000)
0017DD30 0020:
	_default_material_definition_get (0000)
0017DD50 00c0:
	_scenario_material_definition_get (0000)
0017DE10 00d0:
	_scenario_location_deafening (0000)
0017DEE0 0010:
	_scenario_fog_at_point (0000)
0017DEF0 0080:
	_scenario_illumination_at_point (0000)
0017DF70 0050:
	_scenario_leaf_index_from_point (0000)
0017DFC0 0060:
	_scenario_get_sky_definition_index (0000)
0017E020 0030:
	_scenario_get_sky (0000)
0017E050 00a0:
	_scenario_test_pvs (0000)
0017E0F0 0070:
	_scenario_test_pas (0000)
0017E160 00a0:
	_scenario_location_potentially_visible_local (0000)
0017E200 00a0:
	_scenario_location_potentially_visible (0000)
0017E2A0 0060:
	_scenario_object_name_index_from_string (0000)
0017E300 0090:
	_scenario_fog_region_get_fog_index (0000)
0017E390 0190:
	_scenario_switch_structure_bsp (0000)
0017E520 0050:
	_scenario_reload_structure_bsp_if_necessary (0000)
0017E570 0070:
	_scenario_get_structure_reference_index_from_tag_index (0000)
0017E5E0 0170:
	_scenario_trigger_volume_test_point (0000)
0017E750 0030:
	_scenario_trigger_volume_test_object (0000)
0017E780 0150:
	_scenario_debug_to_file (0000)
0017E8D0 0010:
	_global_structure_bsp_index_get (0000)
0017E8E0 00f0:
	_scenario_load (0000)
0017E9D0 00b0:
	_scenario_location_from_point (0000)
0017EA80 0020:
	_scenario_location_from_line (0000)
0017EAA0 0080:
	_scenario_ensure_point_within_world (0000)
0017EB20 0110:
	_scenario_get_fog_region_index (0000)
0017EC30 0130:
	_scenario_location_underwater (0000)
0017ED60 00f0:
	_scenario_location_water_depth (0000)
0017EE50 0520:
	_scenario_get_sound_environment (0000)
0017F370 00a0:
	_code_0017f370 (0000)
0017F410 0330:
	_scenario_get_atmospheric_fog (0000)
002A053C 0011:
	??_C@_0BB@PPPFHENE@scenario?5globals?$AA@ (0000)
002A0550 0018:
	??_C@_0BI@KNDLJHDD@?$CBbink_playback_active?$CI?$CJ?$AA@ (0000)
002A0568 0023:
	??_C@_0CD@MHNEBKHK@c?3?2halo?2SOURCE?2scenario?2scenario@ (0000)
002A058C 0010:
	??_C@_0BA@HJMKGEMF@global_scenario?$AA@ (0000)
002A059C 0015:
	??_C@_0BF@JLHIBHPB@global_structure_bsp?$AA@ (0000)
002A05B4 0015:
	??_C@_0BF@KKAPCCOB@global_collision_bsp?$AA@ (0000)
002A05CC 000d:
	??_C@_0N@BGGBCKLE@global_bsp3d?$AA@ (0000)
002A05DC 0014:
	??_C@_0BE@GJAAGEGH@global_game_globals?$AA@ (0000)
002A05F0 0054:
	??_C@_0FE@CGGKIBFC@material_type?$DN?$DNNONE?5?$HM?$HM?5?$CImaterial@ (0000)
002A0648 0042:
	??_C@_0EC@HOLIKEMB@cluster_index1?$DO?$DN0?5?$CG?$CG?5cluster_ind@ (0000)
002A068C 0022:
	??_C@_0CC@NOBONILA@failed?5to?5load?5structure?5bsp?5?8?$CFs@ (0000)
002A06B0 0016:
	??_C@_0BG@HHCFBLGP@?$DMno?5scenario?5loaded?$DO?6?$AA@ (0000)
002A06C8 0007:
	??_C@_06FJDCLHHM@?5dead?6?$AA@ (0000)
002A06D0 002b:
	??_C@_0CL@BODEOCHI@?5at?5?$CI?$CF?42f?0?$CF?42f?0?$CF?42f?$CJ?5?$CIleaf?$CD?$CFd?0cl@ (0000)
002A06FC 000e:
	??_C@_0O@NJBGJAF@player?50x?$CF08x?$AA@ (0000)
002A070C 0015:
	??_C@_0BF@OHELHNCO@?$CC?$CFs?$CC?5bsp?5?$CC?$CFs?$CC?5?$CI?$CD?$CFd?$CJ?6?$AA@ (0000)
002A0724 0020:
	??_C@_0CA@IJIAGEDC@need?5to?5get?5the?5following?5tags?3?$AA@ (0000)
002A0744 0026:
	??_C@_0CG@BPOMKFJA@scenario?5doesn?8t?5have?5a?5structur@ (0000)
002A076C 0010:
	??_C@_0BA@EONNEPHC@globals?2globals?$AA@ (0000)
002A077C 000e:
	??_C@_0O@GBNBMLBO@scenario_load?$AA@ (0000)
002A078C 0004:
	__real@44160000 (0000)
002A0790 0004:
	__real@c4160000 (0000)
002A0794 0004:
	__real@3b449ba6 (0000)
002A0798 0004:
	__real@bb449ba6 (0000)
002A079C 0004:
	__real@3db851ec (0000)
002A07A0 0004:
	__real@bdb851ec (0000)
002A07A4 0004:
	__real@bcf5c28f (0000)
002A07A8 000b:
	??_C@_0L@LIOBMPBJ@?$HMn?$HMn?$HMn?$HMn?$CFs?$AA@ (0000)
002A07B4 0015:
	??_C@_0BF@LNAHLDLC@no?5sound?5environment?$AA@ (0000)
0030E790 006c:
	_global_scenario_index (0000)
	_global_structure_bsp_index (0004)
004C0520 0375:
	_bss_004c0520 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "scenario.h"

#include "cache/cache_files.h"
#include "effects/material_effect_definitions.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "objects/objects.h"
#include "physics/bsp3d.h"
#include "physics/collision_bsp_definitions.h"
#include "scenario_definitions.h"
#include "sky_definitions.h"
#include "scenario/wind.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct scenario_material_globals
{
	struct material_definition default_material;
	boolean default_material_initialized;
};

/* ---------- prototypes */

/* ---------- globals */

struct structure_bsp *global_structure_bsp;
struct scenario *global_scenario;
struct collision_bsp *global_collision_bsp;
struct scenario_material_globals bss_004c0520;
struct bsp3d *global_bsp3d;
struct game_globals *global_game_globals;

/* ---------- public code */

void scenario_dispose_from_old_map(
	void)
{
	wind_dispose_from_old_map();

	return;
}

void scenario_frame_update(
	real delta_time)
{
	wind_update();

	return;
}

struct scenario *global_scenario_try_and_get(
	void)
{
	return global_scenario;
}

struct scenario *global_scenario_get(
	void)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 183, global_scenario);

	return global_scenario;
}

struct structure_bsp *global_structure_bsp_get(
	void)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 197, global_structure_bsp);

	return global_structure_bsp;
}

struct collision_bsp *global_collision_bsp_get(
	void)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 205, global_collision_bsp);

	return global_collision_bsp;
}

struct bsp3d *global_bsp3d_get(
	void)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 213, global_bsp3d);

	return global_bsp3d;
}

struct game_globals *scenario_get_game_globals(
	void)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 221, global_game_globals);

	return global_game_globals;
}

long global_structure_bsp_tag_index_get(
	void)
{
	struct scenario_structure_bsp_reference *reference = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->structure_bsp_references,
		global_structure_bsp_index,
		struct scenario_structure_bsp_reference);

	return reference->structure_bsp.index;
}

void scenario_location_award_bonus(
	struct location *location)
{
	location->bonus = NONE;

	return;
}

struct material_definition *default_material_definition_get(
	void)
{
	if (!bss_004c0520.default_material_initialized)
	{
		bss_004c0520.default_material.melee_hit_sound.index = NONE;
		bss_004c0520.default_material_initialized = TRUE;
	}

	return &bss_004c0520.default_material;
}

long scenario_get_sky_definition_index(
	short sky_index)
{
	struct scenario *scenario = global_scenario_get();
	long definition_index = NONE;

	if (sky_index >= 0 && sky_index < scenario->sky_references.count)
	{
		struct tag_reference *sky_reference = TAG_BLOCK_GET_ELEMENT(
			&scenario->sky_references,
			sky_index,
			struct tag_reference);

		definition_index = sky_reference->index;
	}

	return definition_index;
}

struct sky *scenario_get_sky(
	short sky_index)
{
	long definition_index = scenario_get_sky_definition_index(sky_index);
	struct sky *sky = NULL;

	if (definition_index != NONE)
		sky = sky_definition_get(definition_index);

	return sky;
}

boolean scenario_illumination_at_point(
	const real_point3d *point,
	real_vector3d *surface_normal,
	real_vector3d *radiosity_vector,
	real_rgb_color *radiosity_color,
	real_rgb_color *diffuse_color)
{
	if (surface_normal)
		*surface_normal = *global_up3d;
	if (radiosity_vector)
		*radiosity_vector = *global_left3d;
	if (radiosity_color)
		*radiosity_color = *global_real_rgb_white;
	if (diffuse_color)
		*diffuse_color = *global_real_rgb_white;

	return TRUE;
}

short global_structure_bsp_index_get(
	void)
{
	return global_structure_bsp_index;
}

long scenario_leaf_index_from_point(
	const union real_point3d *point)
{
	return bsp3d_test_point(global_bsp3d_get(), 0, point);
}

boolean scenario_ensure_point_within_world(
	real_point3d *point)
{
	short iteration_count = 0;

	while (bsp3d_test_point(global_bsp3d_get(), 0, point) == NONE)
	{
		if (iteration_count++ >= 150)
			break;
		point->z += 0.05f;
	}

	return iteration_count == 0;
}

real scenario_fog_at_point(
	const struct location *viewer_location,
	const real_point3d *viewer_point,
	const real_point3d *point)
{
	return 0.0f;
}

short scenario_object_name_index_from_string(
	struct scenario *scenario,
	const char *name)
{
	short object_name_index;

	for (object_name_index = 0; object_name_index < scenario->object_names.count; object_name_index++)
	{
		struct scenario_object_name *object_name = TAG_BLOCK_GET_ELEMENT(
			&scenario->object_names,
			object_name_index,
			struct scenario_object_name);

		if (!csstrcmp(object_name->name, name))
			return object_name_index;
	}

	return NONE;
}

short scenario_get_structure_reference_index_from_tag_index(
	struct scenario *scenario,
	long structure_bsp_index)
{
	const char *structure_bsp_name = tag_get_name(structure_bsp_index);
	short structure_bsp_reference_index;
	short result = NONE;

	for (structure_bsp_reference_index = 0;
		structure_bsp_reference_index < scenario->structure_bsp_references.count;
		structure_bsp_reference_index++)
	{
		struct scenario_structure_bsp_reference *reference = TAG_BLOCK_GET_ELEMENT(
			&scenario->structure_bsp_references,
			structure_bsp_reference_index,
			struct scenario_structure_bsp_reference);

		if (!csstrcmp(structure_bsp_name, reference->structure_bsp.name))
		{
			result = structure_bsp_reference_index;
			break;
		}
	}

	return result;
}

void scenario_location_from_line(
	struct location *location,
	const struct location *start_location,
	const real_point3d *start_point,
	const real_point3d *end_point)
{
	scenario_location_from_point(location, end_point);

	return;
}

boolean scenario_test_pvs(
	short cluster_index0,
	short cluster_index1)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();
	unsigned long *pvs = structure_bsp_get_cluster_pvs(structure_bsp, cluster_index0);

	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 468,
		cluster_index1>=0 && cluster_index1<structure_bsp->clusters.count);

	return BIT_VECTOR_TEST_FLAG(pvs, cluster_index1);
}

boolean scenario_location_potentially_visible_local(
	const struct location *location)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 487,
		location->cluster_index>=0 && location->cluster_index<global_structure_bsp_get()->clusters.count);

	return BIT_VECTOR_TEST_FLAG(players_get_combined_pvs_local(), location->cluster_index);
}

boolean scenario_location_potentially_visible(
	const struct location *location)
{
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 495,
		location->cluster_index>=0 && location->cluster_index<global_structure_bsp_get()->clusters.count);

	return BIT_VECTOR_TEST_FLAG(players_get_combined_pvs(), location->cluster_index);
}

boolean scenario_trigger_volume_test_object(
	short trigger_volume_index,
	long object_index)
{
	boolean result = FALSE;

	if (object_index != NONE)
		result = scenario_trigger_volume_test_point(trigger_volume_index, &object_get(object_index)->object.bounding_sphere_center);

	return result;
}

boolean scenario_trigger_volume_test_point(
	short trigger_volume_index,
	const real_point3d *position)
{
	struct scenario_trigger_volume *trigger_volume = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->trigger_volumes,
		trigger_volume_index,
		struct scenario_trigger_volume);

	switch (trigger_volume->type)
	{
	case _scenario_trigger_volume_type_axis_aligned:
		return position->x > trigger_volume->bounds.x0 &&
			position->y > trigger_volume->bounds.y0 &&
			position->z > trigger_volume->bounds.z0 &&
			position->x < trigger_volume->bounds.x1 &&
			position->y < trigger_volume->bounds.y1 &&
			position->z < trigger_volume->bounds.z1;

	case _scenario_trigger_volume_type_oriented:
	{
		real_matrix4x3 matrix;
		real_point3d transformed_position;

		matrix4x3_from_point_and_vectors(
			&matrix,
			&trigger_volume->position,
			&trigger_volume->forward,
			&trigger_volume->up);
		matrix4x3_inverse_transform_point(&matrix, position, &transformed_position);

		return transformed_position.x > 0.0f &&
			transformed_position.y > 0.0f &&
			transformed_position.z > 0.0f &&
			transformed_position.x < trigger_volume->extents.i &&
			transformed_position.y < trigger_volume->extents.j &&
			transformed_position.z < trigger_volume->extents.k;
	}

	default:
		match_vassert("c:\\halo\\SOURCE\\scenario\\scenario.c", 817, FALSE, NULL);
		return FALSE;
	}
}

long scenario_fog_region_get_fog_index(
	short fog_region_index)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();

	if (fog_region_index != NONE)
	{
		struct structure_fog_region *fog_region = TAG_BLOCK_GET_ELEMENT(
			&structure_bsp->fog_regions,
			fog_region_index,
			struct structure_fog_region);

		if (fog_region->fog_palette_index != NONE)
		{
			struct structure_fog_palette_entry *fog_palette_entry = TAG_BLOCK_GET_ELEMENT(
				&structure_bsp->fog_palette,
				fog_region->fog_palette_index,
				struct structure_fog_palette_entry);

			if (fog_palette_entry->fog.index != NONE)
				return fog_palette_entry->fog.index;
		}
	}

	return NONE;
}

boolean scenario_test_pas(
	short cluster_index0,
	short cluster_index1)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();
	unsigned long *pvs0 = structure_bsp_get_cluster_pvs(structure_bsp, cluster_index0);
	unsigned long *pvs1 = structure_bsp_get_cluster_pvs(structure_bsp, cluster_index1);

	return bit_vector_and((short)structure_bsp->clusters.count, pvs0, pvs1, NULL);
}

void scenario_reload_structure_bsp_if_necessary(
	void)
{
	if (scenario_globals->structure_bsp_index != global_structure_bsp_index)
	{
		struct scenario_structure_bsp_reference *reference = TAG_BLOCK_GET_ELEMENT(
			&global_scenario->structure_bsp_references,
			global_structure_bsp_index,
			struct scenario_structure_bsp_reference);

		scenario_structure_bsp_unload(reference);
		global_structure_bsp_index = NONE;
		scenario_switch_structure_bsp(scenario_globals->structure_bsp_index);
	}

	return;
}

/* ---------- private code */
