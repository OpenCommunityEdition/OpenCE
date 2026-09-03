/*
RASTERIZER_LIGHTS.C

symbols in this file:
00170840 0030:
	_screenshot_in_progress (0000)
00170870 0040:
	_lens_flare_submit_parameter_get (0000)
001708B0 00f0:
	_lens_flare_occlusion_test_results_get (0000)
001709A0 0030:
	_rasterizer_lights_reset_for_new_map (0000)
001709D0 0130:
	_rasterizer_lights_begin_for_new_frame (0000)
00170B00 0010:
	_rasterizer_lights_begin (0000)
00170B10 0150:
	_rasterizer_light_submit (0000)
00170C60 0010:
	_rasterizer_lights_end (0000)
00170C70 0250:
	_lens_flare_evaluate_corona_rotation_function (0000)
00170EC0 0290:
	_rasterizer_lens_flare_submit (0000)
00171150 0190:
	_rasterizer_lens_flare_submit_for_cluster (0000)
001712E0 0190:
	_rasterizer_lens_flares_submit_occlusion_tests (0000)
00171470 08c0:
	_rasterizer_lens_flares_draw (0000)
0029E6A4 003f:
	??_C@_0DP@MMPMMAFC@lens_flare_index?$DO?$DN0?5?$CG?$CG?5lens_flar@ (0000)
0029E6E4 002e:
	??_C@_0CO@MGBADAON@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029E718 0063:
	??_C@_0GD@JPCGFHAO@lens_flare_parameters?9?$DOlight_ind@ (0000)
0029E780 0082:
	??_C@_0IC@IBFAKJOH@structure_lens_flare_index?$DO?$DN0?5?$CG?$CG@ (0000)
0029E804 0016:
	??_C@_0BG@JFBJFIEK@lens_flare_parameters?$AA@ (0000)
0029E81C 002e:
	??_C@_0CO@FCEOCGFK@?$CD?$CD?$CD?5ERROR?5too?5many?5lights?5submit@ (0000)
0029E84C 003f:
	??_C@_0DP@CJDDDBGK@parameters?9?$DOcolor?4blue?5?$DO?$DN0?40f?5?$CG?$CG@ (0000)
0029E88C 003f:
	??_C@_0DP@IFBCMKNP@parameters?9?$DOcolor?4green?$DO?$DN0?40f?5?$CG?$CG@ (0000)
0029E8CC 003d:
	??_C@_0DN@IFBOCEFD@parameters?9?$DOcolor?4red?5?$DO?$DN0?40f?5?$CG?$CG?5@ (0000)
0029E90C 003a:
	??_C@_0DK@NODNGFKL@?$CD?$CD?$CD?5ERROR?5unsupported?5lens?5flare@ (0000)
0029E948 0032:
	??_C@_0DC@BJJPKDMH@?$CD?$CD?$CD?5ERROR?5too?5many?5lens?5flares?5s@ (0000)
0029E980 004d:
	??_C@_0EN@OMGAKMEG@parameters?9?$DOlight_index?$DO?$DN0?5?$CG?$CG?5pa@ (0000)
0029E9D0 0065:
	??_C@_0GF@CKCJNIMD@structure_lens_flare_index?$DO?$DN0?5?$CG?$CG@ (0000)
0029EA38 005b:
	??_C@_0FL@ECFOOILG@parameters?9?$DOlens_flare_index?$DO?$DN0?5@ (0000)
0029EA98 006b:
	??_C@_0GL@HCLLBKGN@?$CIparameters?9?$DOcompressed_window_i@ (0000)
0029EB04 0017:
	??_C@_0BH@FDNIDAHL@parameters?9?$DOdefinition?$AA@ (0000)
0029EB1C 003c:
	??_C@_0DM@DJHNIOGP@?$CD?$CD?$CD?5ERROR?5unsupported?5lens?5flare@ (0000)
0029EB58 003b:
	??_C@_0DL@EPCOBKHO@animation_color?4blue?5?$DO?$DN0?40f?5?$CG?$CG?5a@ (0000)
0029EB94 003b:
	??_C@_0DL@CLPEFPIB@animation_color?4green?$DO?$DN0?40f?5?$CG?$CG?5a@ (0000)
0029EBD0 0039:
	??_C@_0DJ@NIJDHHFP@animation_color?4red?5?$DO?$DN0?40f?5?$CG?$CG?5an@ (0000)
0029EC0C 003b:
	??_C@_0DL@HIJLAENC@animation_color?4alpha?$DO?$DN0?40f?5?$CG?$CG?5a@ (0000)
0029EC48 0023:
	??_C@_0CD@LGKNCBJE@reflection?9?$DOanimation_period?$CB?$DN0?4@ (0000)
0029EC6C 0004:
	__real@42652ee1 (0000)
00466B80 51725:
	_bss_00466b80 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "render.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"
#include "rasterizer.h"
#include "rasterizer_lights.h"
#include "rasterizer_geometry.h"
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"

/* ---------- constants */

enum
{
	_rasterizer_target_render_primary = 0
};

enum
{
	_rasterizer_statistics_mode_none = 0,
	_rasterizer_statistics_mode_framerate,
	_rasterizer_statistics_mode_geometry
};

enum
{
	_lens_flare_first_person_weapon_flag = FLAG(7),
	_lens_flare_window_index_mask = ~_lens_flare_first_person_weapon_flag
};

enum
{
	_lens_flare_dynamic_light_flag = FLAG(15),
	_lens_flare_dynamic_light_index_mask = FLAG(15)-1
};

/* ---------- macros */

#define LENS_FLARE_LIGHT_COLOR_ALPHA_MASK 0xff000000

#define LENS_FLARE_DEFINITION_TAG 'lens'
#define lens_flare_definition_get(index) ((struct lens_flare_definition *)tag_get(LENS_FLARE_DEFINITION_TAG, (index)))

/* ---------- structures */

struct lens_flare_definition
{
	byte reserved00[0x10];
	real occlusion_radius;
	short occlusion_offset_direction;
	byte reserved16[0x6];
	real far_fade_distance;
};

struct structure_bsp
{
	byte reserved000[0x11C];
	struct tag_block lens_flares;
	struct tag_block lens_flare_markers;
	struct tag_block clusters;
};

struct structure_cluster
{
	byte reserved00[0x40];
	word first_lens_flare_marker_index;
	word lens_flare_marker_count;
	byte reserved44[0x24];
};

struct structure_lens_flare
{
	struct tag_reference lens_flare;
};

struct structure_lens_flare_marker
{
	real_point3d position;
	char i_direction;
	char j_direction;
	char k_direction;
	byte lens_flare_index;
};

struct rasterizer_lights_window_parameters
{
	short rasterizer_target;
	short window_index;
	byte reserved04[4];
	real_point3d camera_position;
	real_vector3d camera_forward;
};

struct lens_flare_occlusion_test_results
{
	short light_identifier;
	byte data[MAXIMUM_LENS_FLARES_PER_LIGHT][MAXIMUM_WINDOWS];
};

struct rasterizer_lens_flare_submit_parameters
{
	struct lens_flare_definition *definition;
	real_point3d position;
	unsigned long compressed_direction;
	unsigned long compressed_up;
	unsigned long compressed_light_color;
	short light_identifier;
	short light_index;
	short lens_flare_index;
	byte compressed_window_index;
	byte compressed_light_scale;
	long internal__occlusion_pixels;
};

struct rasterizer_light_submit_parameters
{
	struct point_light_definition *definition;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_rgb_color color;
	real radius;
};

struct rasterizer_lights_globals
{
	long light_count;
	struct rasterizer_light_submit_parameters lights[MAXIMUM_LIGHTS_PER_WINDOW];
	long fixed_function_light_count;
};

struct rasterizer_lights_debug_options_prefix
{
	byte reserved000[2];
	short statistics_mode;
	byte reserved004[0x1B];
	boolean lens_flares;
};

struct rasterizer_lights_frame_statistics_prefix
{
	byte reserved000[0x148];
	long dynamic_light_count;
	long lens_flare_count;
};

typedef char verify_structure_cluster_size[
	sizeof(struct structure_cluster) == 0x68 ? 1 : -1];
typedef char verify_structure_cluster_lens_flare_marker_count_offset[
	offsetof(
		struct structure_cluster,
		lens_flare_marker_count) == 0x42 ? 1 : -1];
typedef char verify_structure_lens_flare_marker_size[
	sizeof(struct structure_lens_flare_marker) == 0x10 ? 1 : -1];
typedef char verify_lens_flare_definition_far_fade_distance_offset[
	offsetof(
		struct lens_flare_definition,
		far_fade_distance) == 0x1C ? 1 : -1];
typedef char verify_rasterizer_lights_window_parameters_camera_forward_offset[
	offsetof(
		struct rasterizer_lights_window_parameters,
		camera_forward) == 0x14 ? 1 : -1];
typedef char verify_rasterizer_light_submit_parameters_size[
	sizeof(struct rasterizer_light_submit_parameters) == 0x38 ? 1 : -1];
typedef char verify_rasterizer_lights_debug_options_statistics_mode_offset[
	offsetof(
		struct rasterizer_lights_debug_options_prefix,
		statistics_mode) == 0x2 ? 1 : -1];
typedef char verify_rasterizer_lights_debug_options_lens_flares_offset[
	offsetof(
		struct rasterizer_lights_debug_options_prefix,
		lens_flares) == 0x1F ? 1 : -1];
typedef char verify_rasterizer_lights_frame_statistics_dynamic_light_count_offset[
	offsetof(
		struct rasterizer_lights_frame_statistics_prefix,
		dynamic_light_count) == 0x148 ? 1 : -1];
typedef char verify_rasterizer_lights_frame_statistics_lens_flare_count_offset[
	offsetof(
		struct rasterizer_lights_frame_statistics_prefix,
		lens_flare_count) == 0x14C ? 1 : -1];

/* ---------- prototypes */

static boolean screenshot_in_progress(
	void);
static struct rasterizer_lens_flare_submit_parameters *lens_flare_submit_parameter_get(
	short lens_flare_index);
static byte *lens_flare_occlusion_test_results_get(
	struct rasterizer_lens_flare_submit_parameters *lens_flare_parameters);

/* ---------- globals */

static struct lens_flare_occlusion_test_results local_lens_flare_occlusion_test_results[MAXIMUM_LIGHTS_PER_MAP];
static byte local_lens_flare_occlusion_test_results2[MAXIMUM_LENS_FLARE_MARKERS_PER_STRUCTURE+MAXIMUM_QUEUED_LENS_FLARES][MAXIMUM_WINDOWS];
static struct rasterizer_lens_flare_submit_parameters local_lens_flare_parameters[MAXIMUM_LENS_FLARES_PER_FRAME] = {0};
static long local_lens_flare_count = 0;
static boolean local_lens_flare_error_printed = FALSE;
extern struct rasterizer_lights_globals rasterizer_lights;
extern struct rasterizer_lights_debug_options_prefix rasterizer_debug_options;
extern struct rasterizer_lights_frame_statistics_prefix rasterizer_frame_statistics;
extern struct rasterizer_lights_window_parameters global_window_parameters;
extern short global_screenshot_count;
extern short global_screenshot_size;

/* ---------- private code */

static boolean screenshot_in_progress(
	void)
{
	return global_screenshot_count>1 || (global_screenshot_count==1 && global_screenshot_size>1);
}

static struct rasterizer_lens_flare_submit_parameters *lens_flare_submit_parameter_get(
	short lens_flare_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		67,
		lens_flare_index>=0 && lens_flare_index<local_lens_flare_count);

	return &local_lens_flare_parameters[lens_flare_index];
}

static byte *lens_flare_occlusion_test_results_get(
	struct rasterizer_lens_flare_submit_parameters *lens_flare_parameters)
{
	word light_index;

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		76,
		lens_flare_parameters);

	light_index= lens_flare_parameters->light_index;

	if (light_index & _lens_flare_dynamic_light_flag)
	{
		long window_index= lens_flare_parameters->compressed_window_index & _lens_flare_window_index_mask;
		long structure_lens_flare_index= ((light_index & _lens_flare_dynamic_light_index_mask)<<16) |
			lens_flare_parameters->lens_flare_index;

		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
			87,
			window_index>=0 && window_index<MAXIMUM_WINDOWS);
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
			88,
			structure_lens_flare_index>=0 && structure_lens_flare_index<(MAXIMUM_LENS_FLARE_MARKERS_PER_STRUCTURE+MAXIMUM_QUEUED_LENS_FLARES));

		return &local_lens_flare_occlusion_test_results2[structure_lens_flare_index][window_index];
	}

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		94,
		lens_flare_parameters->light_index>=0 && lens_flare_parameters->light_index<MAXIMUM_LIGHTS_PER_MAP);

	return &local_lens_flare_occlusion_test_results[lens_flare_parameters->light_index].
		data[lens_flare_parameters->lens_flare_index]
		[lens_flare_parameters->compressed_window_index & _lens_flare_window_index_mask];
}

/* ---------- public code */

void rasterizer_lights_reset_for_new_map(
	void)
{
	memset(
		local_lens_flare_occlusion_test_results,
		0,
		sizeof(local_lens_flare_occlusion_test_results));
	memset(local_lens_flare_occlusion_test_results2, 0, sizeof(local_lens_flare_occlusion_test_results2));
	local_lens_flare_count = 0;

	return;
}

void rasterizer_lens_flare_submit(
	struct rasterizer_lens_flare_submit_parameters const *parameters)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		266,
		parameters);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		267,
		parameters->definition);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		268,
		(parameters->compressed_window_index&_lens_flare_window_index_mask)==global_window_parameters.window_index);

	if (rasterizer_debug_options.lens_flares && !screenshot_in_progress() &&
		global_window_parameters.rasterizer_target==_rasterizer_target_render_primary)
	{
		if (local_lens_flare_count<MAXIMUM_LENS_FLARES_PER_FRAME)
		{
			real_vector3d camera_offset;
			real camera_distance;

			vector_from_points3d(
				&global_window_parameters.camera_position,
				&parameters->position,
				&camera_offset);
			camera_distance= dot_product3d(&global_window_parameters.camera_forward, &camera_offset);

			if ((parameters->definition->far_fade_distance==0.0f ||
				camera_distance<parameters->definition->far_fade_distance) &&
				(parameters->compressed_light_color&LENS_FLARE_LIGHT_COLOR_ALPHA_MASK)>0)
			{
				struct rasterizer_lens_flare_submit_parameters *lens_flare_parameters=
					lens_flare_submit_parameter_get((short)local_lens_flare_count++);

				memcpy(lens_flare_parameters, parameters, sizeof(*lens_flare_parameters));

				if (parameters->light_identifier==NONE)
				{
					if (parameters->light_index==NONE)
					{
						lens_flare_parameters->light_index= _lens_flare_dynamic_light_flag;

						match_assert(
							"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
							310,
							parameters->lens_flare_index>=0 && parameters->lens_flare_index<MAXIMUM_QUEUED_LENS_FLARES);
					}
					else
					{
						long structure_lens_flare_index= (parameters->light_index<<16)|parameters->lens_flare_index;

						match_assert(
							"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
							321,
							structure_lens_flare_index>=0 && structure_lens_flare_index<MAXIMUM_LENS_FLARE_MARKERS_PER_STRUCTURE);

						lens_flare_parameters->lens_flare_index= (short)(structure_lens_flare_index+MAXIMUM_QUEUED_LENS_FLARES);
						lens_flare_parameters->light_index= (short)((structure_lens_flare_index>>16)|(short)_lens_flare_dynamic_light_flag);
					}
				}
				else
				{
					struct lens_flare_occlusion_test_results *occlusion_test_results=
						&local_lens_flare_occlusion_test_results[lens_flare_parameters->light_index];

					match_assert(
						"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
						338,
						parameters->light_index>=0 && parameters->light_index<MAXIMUM_LIGHTS_PER_MAP);

					if (parameters->light_identifier!=occlusion_test_results->light_identifier)
					{
						memset(occlusion_test_results->data, 0, sizeof(occlusion_test_results->data));
						occlusion_test_results->light_identifier= lens_flare_parameters->light_identifier;
					}
				}

				if (rasterizer_debug_options.statistics_mode==_rasterizer_statistics_mode_geometry)
				{
					rasterizer_frame_statistics.lens_flare_count++;
				}
			}
		}
		else
		{
			if (!local_lens_flare_error_printed)
			{
				error(2, "### ERROR too many lens flares submitted to frame");
				local_lens_flare_error_printed= TRUE;
			}
		}
	}

	return;
}

void rasterizer_lights_begin_for_new_frame(
	void)
{
	rasterizer_profile_begin(_rasterizer_profile_lens_flare_occlusion_query);

	if (rasterizer_debug_options.lens_flares && !screenshot_in_progress())
	{
		short lens_flare_index;

		for (lens_flare_index= 0; lens_flare_index<local_lens_flare_count; lens_flare_index++)
		{
			struct rasterizer_lens_flare_submit_parameters *lens_flare_parameters= lens_flare_submit_parameter_get(lens_flare_index);
			byte *occlusion_test_result= lens_flare_occlusion_test_results_get(lens_flare_parameters);
			byte latest_visibility;

			if (lens_flare_parameters->internal__occlusion_pixels>0)
			{
				long new_visibility= (255*rasterizer_widget_get_occlusion_test_result(lens_flare_index) +
					(lens_flare_parameters->internal__occlusion_pixels>>1)) /
					lens_flare_parameters->internal__occlusion_pixels;

				latest_visibility= (byte)MIN(255, new_visibility);
			}
			else
			{
				latest_visibility= 0;
			}

			if (!latest_visibility)
			{
				*occlusion_test_result= 0;
			}
			else
			{
				byte previous_visibility= *occlusion_test_result;

				if (latest_visibility>previous_visibility)
				{
					*occlusion_test_result= (byte)((3*previous_visibility + latest_visibility)/4);
				}
				else if (latest_visibility<previous_visibility)
				{
					*occlusion_test_result= (byte)((previous_visibility + latest_visibility)/2);
				}
			}
		}

		local_lens_flare_count= 0;
	}

	rasterizer_profile_end(_rasterizer_profile_lens_flare_occlusion_query);

	return;
}

void rasterizer_lights_begin(
	void)
{
	rasterizer_lights.light_count = 0;

	return;
}

long rasterizer_light_submit(
	struct rasterizer_light_submit_parameters const *parameters)
{
	long light_index= NONE;

	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		240,
		parameters);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		241,
		parameters->color.red >=0.0f && parameters->color.red <=1.0f);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		242,
		parameters->color.green>=0.0f && parameters->color.green<=1.0f);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_lights.c",
		243,
		parameters->color.blue >=0.0f && parameters->color.blue <=1.0f);

	if (rasterizer_lights.light_count<MAXIMUM_LIGHTS_PER_WINDOW)
	{
		light_index= rasterizer_lights.light_count++;
		rasterizer_lights.lights[light_index]= *parameters;

		if (rasterizer_debug_options.statistics_mode==_rasterizer_statistics_mode_geometry)
		{
			rasterizer_frame_statistics.dynamic_light_count++;
		}
	}
	else
	{
		error(2, "### ERROR too many lights submitted to window");
	}

	return light_index;
}

void rasterizer_lights_end(
	void)
{
	return;
}

void rasterizer_lens_flare_submit_for_cluster(
	short cluster_index)
{
	if (rasterizer_debug_options.lens_flares && !screenshot_in_progress())
	{
		struct structure_bsp *structure_bsp= global_structure_bsp_get();
		struct structure_cluster *cluster= TAG_BLOCK_GET_ELEMENT(&structure_bsp->clusters, cluster_index, struct structure_cluster);
		long lens_flare_marker_index;

		for (lens_flare_marker_index= 0; lens_flare_marker_index<cluster->lens_flare_marker_count; lens_flare_marker_index++)
		{
			long structure_lens_flare_marker_index= cluster->first_lens_flare_marker_index+lens_flare_marker_index;
			struct structure_lens_flare_marker *marker= TAG_BLOCK_GET_ELEMENT(&structure_bsp->lens_flare_markers, structure_lens_flare_marker_index, struct structure_lens_flare_marker);
			struct structure_lens_flare *structure_lens_flare= TAG_BLOCK_GET_ELEMENT(&structure_bsp->lens_flares, marker->lens_flare_index, struct structure_lens_flare);
			struct rasterizer_lens_flare_submit_parameters parameters;
			real_vector3d direction;
			real_vector3d up;

			set_real_vector3d(
				&direction,
				marker->i_direction*(1.0f/127.0f),
				marker->j_direction*(1.0f/127.0f),
				marker->k_direction*(1.0f/127.0f));

			perpendicular3d(&direction, &up);
			normalize3d(&direction);
			normalize3d(&up);

			parameters.compressed_direction= compress_real_vector3d_to_int32_clamp(&direction);
			parameters.compressed_up= compress_real_vector3d_to_int32_clamp(&up);
			parameters.definition= lens_flare_definition_get(structure_lens_flare->lens_flare.index);
			parameters.position= marker->position;
			parameters.compressed_light_color= NONE;
			parameters.light_identifier= NONE;
			parameters.compressed_light_scale= 0;
			parameters.light_index= (short)(structure_lens_flare_marker_index>>16);
			parameters.lens_flare_index= (short)structure_lens_flare_marker_index;
			parameters.compressed_window_index= (byte)render.window_index;

			rasterizer_lens_flare_submit(&parameters);
		}
	}

	return;
}
