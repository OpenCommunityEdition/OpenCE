/*
OBJECT_LIGHTS.C

symbols in this file:
00128730 0030:
	_texture_cache_bitmap_load (0000)
00128760 0020:
	_code_00128760 (0000)
00128780 0040:
	_code_00128780 (0000)
001287C0 0060:
	_code_001287c0 (0000)
00128820 0100:
	_sample_lightmap (0000)
00128920 00f0:
	_sample_diffuse_texture (0000)
00128A10 0020:
	_code_00128a10 (0000)
00128A30 00c0:
	_lights_initialize (0000)
00128AF0 0010:
	_lights_dispose (0000)
00128B00 0030:
	_lights_initialize_for_new_map (0000)
00128B30 0020:
	_lights_dispose_from_old_map (0000)
00128B50 0010:
	_lights_enable (0000)
00128B60 0040:
	_light_delete (0000)
00128BA0 0060:
	_code_00128ba0 (0000)
00128C00 00d0:
	_object_get_self_illumination (0000)
00128CD0 0260:
	_light_particle (0000)
00128F30 0060:
	_light_disconnect_from_map (0000)
00128F90 00b0:
	_lights_disconnect_from_structure_bsp (0000)
00129040 0020:
	_code_00129040 (0000)
00129060 00a0:
	_code_00129060 (0000)
00129100 0020:
	_code_00129100 (0000)
00129120 0020:
	_code_00129120 (0000)
00129140 0040:
	_code_00129140 (0000)
00129180 0060:
	_code_00129180 (0000)
001291E0 0060:
	_code_001291e0 (0000)
00129240 0040:
	_code_00129240 (0000)
00129280 0110:
	_code_00129280 (0000)
00129390 00e0:
	_lights_queue_lens_flare (0000)
00129470 0230:
	_code_00129470 (0000)
001296A0 0400:
	_code_001296a0 (0000)
00129AA0 00f0:
	_code_00129aa0 (0000)
00129B90 00e0:
	_code_00129b90 (0000)
00129C70 01d0:
	_lights_render_diffuse (0000)
00129E40 0150:
	_lights_render_specular (0000)
00129F90 02d0:
	_lights_illumination_at_point (0000)
0012A260 0110:
	_lights_prepare_for_object_dynamic (0000)
0012A370 03b0:
	_lights_distant_lighting_at_point (0000)
0012A720 0280:
	_light_reconnect_to_map (0000)
0012A9A0 0060:
	_lights_reconnect_to_structure_bsp (0000)
0012AA00 00e0:
	_light_new (0000)
0012AAE0 00f0:
	_light_new_unattached (0000)
0012ABD0 0960:
	_lights_preprocess_scene (0000)
0012B530 0340:
	_lights_prepare_for_object_static (0000)
002896C0 00b0:
	_default_object_lighting (0000)
00289770 0021:
	??_C@_0CB@NDAECIIM@c?3?2halo?2source?2objects?2objects?4h@ (0000)
00289798 00b9:
	??_C@_0LJ@HECDDFBP@material?9?$DOlightmap_vertices?4type@ (0000)
00289854 0027:
	??_C@_0CH@GHJLLMCD@c?3?2halo?2SOURCE?2objects?2object_li@ (0000)
00289880 0095:
	??_C@_0JF@BFHNCHBA@material?9?$DOvertices?4type?$DN?$DN_raster@ (0000)
00289918 002c:
	??_C@_0CM@CCFOPHPG@couldn?8t?5allocate?5memory?5for?5obj@ (0000)
00289944 0014:
	??_C@_0BE@MMBBLMKK@lights_game_globals?$AA@ (0000)
00289958 000b:
	??_C@_0L@PNIBKHKO@light_data?$AA@ (0000)
00289964 000f:
	??_C@_0P@MLOLKFEJ@lights?5globals?$AA@ (0000)
00289974 0007:
	??_C@_06IDBEMGIK@lights?$AA@ (0000)
0028997C 003b:
	??_C@_0DL@OAOFOGFN@TEST_FLAG?$CIlight?9?$DOflags?0?5_point_l@ (0000)
002899B8 0023:
	??_C@_0CD@CEANBCDI@?$CBlights_globals?4marker_initializ@ (0000)
002899DC 0022:
	??_C@_0CC@DMOOLGHL@lights_globals?4marker_initialize@ (0000)
00289A00 0004:
	__real@3f34fdf4 (0000)
00289A04 003c:
	??_C@_0DM@NOKGCMAM@?$CBTEST_FLAG?$CIlight?9?$DOflags?0?5_point_@ (0000)
00289A40 002a:
	??_C@_0CK@JMHDKADE@lens_flare_parameters?4light_iden@ (0000)
00289A6C 0035:
	??_C@_0DF@MMLEIBDB@light?9?$DOcolor?4blue?5?$DO?$DN0?40f?5?$CG?$CG?5ligh@ (0000)
00289AA4 0035:
	??_C@_0DF@KFPEGOCE@light?9?$DOcolor?4green?$DO?$DN0?40f?5?$CG?$CG?5ligh@ (0000)
00289ADC 0033:
	??_C@_0DD@NIPJINKE@light?9?$DOcolor?4red?5?$DO?$DN0?40f?5?$CG?$CG?5light@ (0000)
00289B10 0004:
	__real@bf3504f3 (0000)
0030B388 0605:
	_data_0030b388 (0000)
	_object_light_ambient_base (05f8)
	_object_light_ambient_scale (05fc)
	_object_light_secondary_scale (0600)
	_object_light_interpolate (0604)
00456E94 0004:
	_lights_game_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/object_lights.h"
#include "structures/cluster_partitions.h"

/* ---------- constants */

enum
{
	_point_light_connects_to_map_bit = 1,
	_point_light_connected_to_map_bit = 2,
};

/* ---------- macros */

/* ---------- structures */

struct lights_game_globals
{
	boolean render_lights;
	byte reserved01[3];
};

struct light_datum_prefix
{
	struct datum_header header;
	unsigned short flags;
	long definition_index;
	long rasterizer_light_index;
	long marker;
	long cluster_reference;
};

typedef char verify_lights_game_globals_size[
	sizeof(struct lights_game_globals) == 0x4 ? 1 : -1];
typedef char verify_light_datum_prefix_flags_offset[
	offsetof(struct light_datum_prefix, flags) == 0x2 ? 1 : -1];
typedef char verify_light_datum_prefix_cluster_reference_offset[
	offsetof(struct light_datum_prefix, cluster_reference) == 0x10 ? 1 : -1];

/* ---------- prototypes */

struct bitmap_data;

void profile_texture_start(
	void);
void profile_texture_end(
	void);
void *_texture_cache_bitmap_get_hardware_format(
	struct bitmap_data *bitmap,
	boolean block,
	boolean load);

/* ---------- globals */

extern struct data_array *light_data;
extern struct cluster_partition light_cluster_partition;
extern struct lights_game_globals *lights_game_globals;

/* ---------- public code */

void *texture_cache_bitmap_load(
	struct bitmap_data *bitmap)
{
	void *hardware_format;

	profile_texture_start();
	hardware_format = _texture_cache_bitmap_get_hardware_format(
		bitmap,
		TRUE,
		TRUE);
	profile_texture_end();

	return hardware_format;
}

void lights_dispose(
	void)
{
	cluster_partition_delete(&light_cluster_partition);

	return;
}

void lights_initialize_for_new_map(
	void)
{
	data_make_valid(light_data);
	lights_game_globals->render_lights = TRUE;
	cluster_partition_make_valid(&light_cluster_partition);

	return;
}

void lights_dispose_from_old_map(
	void)
{
	data_make_invalid(light_data);
	cluster_partition_make_invalid(&light_cluster_partition);

	return;
}

boolean lights_enable(
	boolean enable)
{
	lights_game_globals->render_lights = enable;

	return enable;
}

void light_delete(
	long light_index)
{
	struct light_datum_prefix *light = datum_get(light_data, light_index);

	cluster_partition_disconnect(
		&light_cluster_partition,
		light_index,
		&light->cluster_reference);
	datum_delete(light_data, light_index);

	return;
}

void light_disconnect_from_map(
	long light_index)
{
	struct light_datum_prefix *light = datum_get(light_data, light_index);

	if (TEST_FLAG(light->flags, _point_light_connects_to_map_bit))
	{
		match_assert(
			"c:\\halo\\SOURCE\\objects\\object_lights.c",
			0x4D0,
			TEST_FLAG(light->flags, _point_light_connected_to_map_bit));
		cluster_partition_disconnect(
			&light_cluster_partition,
			light_index,
			&light->cluster_reference);
		SET_FLAG(light->flags, _point_light_connected_to_map_bit, FALSE);
	}

	return;
}

void lights_disconnect_from_structure_bsp(
	void)
{
	long light_index;

	for (light_index = data_next_index(light_data, NONE);
		light_index != NONE;
		light_index = data_next_index(light_data, light_index))
	{
		struct light_datum_prefix *light = datum_get(light_data, light_index);

		if (TEST_FLAG(light->flags, _point_light_connected_to_map_bit))
		{
			light_disconnect_from_map(light_index);
			SET_FLAG(light->flags, _point_light_connected_to_map_bit, TRUE);
		}
	}

	return;
}

/* ---------- private code */

long code_00129100(
	long *reference_index,
	short cluster_index)
{
	return cluster_partition_get_first_datum(
		&light_cluster_partition,
		reference_index,
		cluster_index);
}

long code_00129120(
	long *reference_index)
{
	return cluster_partition_get_next_datum(
		&light_cluster_partition,
		reference_index);
}
