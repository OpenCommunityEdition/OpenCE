/*
RASTERIZER_XBOX_ENVIRONMENT.C

symbols in this file:
0014FCC0 01b0:
	_code_0014fcc0 (0000)
0014FE70 0050:
	_code_0014fe70 (0000)
0014FEC0 0220:
	_code_0014fec0 (0000)
001500E0 0010:
	_code_001500e0 (0000)
001500F0 0060:
	_code_001500f0 (0000)
00150150 0010:
	_code_00150150 (0000)
00150160 0010:
	__rasterizer_environment_lightmap_end (0000)
00150170 0010:
	__rasterizer_environment_lightmaps_end (0000)
00150180 0010:
	__rasterizer_environment_diffuse_light_end (0000)
00150190 0010:
	__rasterizer_environment_diffuse_lights_end (0000)
001501A0 0020:
	__rasterizer_environment_diffuse_textures_end (0000)
001501C0 0010:
	__rasterizer_environment_specular_light_end (0000)
001501D0 0010:
	__rasterizer_environment_specular_lights_end (0000)
001501E0 0010:
	__rasterizer_environment_specular_lightmap_end (0000)
001501F0 0010:
	__rasterizer_environment_specular_lightmaps_end (0000)
00150200 0200:
	__rasterizer_environment_reflection_lightmap_mask_draw (0000)
00150400 0010:
	__rasterizer_environment_reflection_lightmap_mask_end (0000)
00150410 0010:
	__rasterizer_environment_reflection_lightmap_masks_end (0000)
00150420 0010:
	__rasterizer_environment_reflection_mirrors_begin (0000)
00150430 0010:
	__rasterizer_environment_reflection_mirrors_end (0000)
00150440 0010:
	__rasterizer_environment_reflections_begin (0000)
00150450 0010:
	__rasterizer_environment_reflections_end (0000)
00150460 0010:
	__rasterizer_environment_transparent_geometry_begin (0000)
00150470 0010:
	__rasterizer_environment_transparent_geometry_end (0000)
00150480 0190:
	__rasterizer_environment_lightmaps_begin (0000)
00150610 0190:
	__rasterizer_environment_lightmap_begin (0000)
001507A0 0fb0:
	__rasterizer_environment_lightmap_draw (0000)
00151750 02c0:
	__rasterizer_environment_diffuse_lights_begin (0000)
00151A10 03a0:
	__rasterizer_environment_diffuse_light_begin (0000)
00151DB0 0230:
	__rasterizer_environment_diffuse_light_draw (0000)
00151FE0 0190:
	__rasterizer_environment_diffuse_textures_begin (0000)
00152170 0670:
	__rasterizer_environment_diffuse_texture_draw (0000)
001527E0 0240:
	__rasterizer_environment_specular_lights_begin (0000)
00152A20 03c0:
	_code_00152a20 (0000)
00152DE0 0380:
	__rasterizer_environment_specular_light_begin (0000)
00153160 0330:
	__rasterizer_environment_specular_light_draw (0000)
00153490 03a0:
	__rasterizer_environment_specular_lightmaps_begin (0000)
00153830 00f0:
	__rasterizer_environment_specular_lightmap_begin (0000)
00153920 0310:
	__rasterizer_environment_specular_lightmap_draw (0000)
00153C30 01b0:
	__rasterizer_environment_reflection_lightmap_masks_begin (0000)
00153DE0 0100:
	__rasterizer_environment_reflection_lightmap_mask_begin (0000)
00153EE0 0660:
	__rasterizer_environment_reflection_mirror_draw (0000)
00154540 0730:
	__rasterizer_environment_reflection_draw (0000)
00154C70 0360:
	__rasterizer_environment_transparent_geometry_submit (0000)
0028FDE8 0007:
	??_C@_06BILHELBG@shader?$AA@ (0000)
0028FDF0 003d:
	??_C@_0DN@KFFMBFDK@c?3?2halo?2SOURCE?2rasterizer?2xbox?2r@ (0000)
0028FE30 0040:
	??_C@_0EA@HODFKFKF@?$CD?$CD?$CD?5ERROR?5unsupported?5drawing?5mo@ (0000)
0028FE70 002c:
	??_C@_0CM@DBKIGDIP@illumination?9?$DOplasma_animation_p@ (0000)
0028FE9C 002f:
	??_C@_0CP@OOCPMCFJ@illumination?9?$DOsecondary_animatio@ (0000)
0028FECC 002d:
	??_C@_0CN@JNMPABNC@illumination?9?$DOprimary_animation_@ (0000)
0028FEFC 000e:
	??_C@_0O@EFOAJPDM@vertex_buffer?$AA@ (0000)
0028FF0C 0028:
	??_C@_0CI@FGDOPIAK@light?9?$DOdefinition?9?$DOgel?4roll_peri@ (0000)
0028FF34 0029:
	??_C@_0CJ@LCKNANBH@light?9?$DOdefinition?9?$DOgel?4pitch_per@ (0000)
0028FF60 0027:
	??_C@_0CH@MPENKOEE@light?9?$DOdefinition?9?$DOgel?4yaw_perio@ (0000)
0028FF88 003c:
	??_C@_0DM@PEENBPOM@light_index?$DO?$DN0?5?$CG?$CG?5light_index?$DMra@ (0000)
0028FFC4 003f:
	??_C@_0DP@CDHCMCNJ@?$CD?$CD?$CD?5ERROR?5unsupported?5drawing?5mo@ (0000)
00290004 0039:
	??_C@_0DJ@PDBPKOHB@?$CD?$CD?$CD?5ERROR?5unsupported?5environmen@ (0000)
00290040 002e:
	??_C@_0CO@HGCEMJKB@?$CD?$CD?$CD?5ERROR?5unsupported?5environmen@ (0000)
00290070 000e:
	??_C@_0O@GPGHEPFO@light?9?$DOradius?$AA@ (0000)
00290080 0026:
	??_C@_0CG@IFHNBCPC@?$CD?$CD?$CD?5ERROR?5unsupported?5reflection@ (0000)
002900A8 0054:
	??_C@_0FE@BFENDLHD@reflection_type?$DO?$DN0?5?$CG?$CG?5reflection@ (0000)
002900FC 003e:
	??_C@_0DO@MCLDAE@?$CBTEST_FLAG?$CIgeometry_flags?0?5_rast@ (0000)
0029013C 0038:
	??_C@_0DI@EJBHOIIP@shader_type_is_valid_for_environ@ (0000)
00290174 002c:
	??_C@_0CM@HAIODBIC@shader?9?$DObase?4type?$CB?$DN_shader_type_@ (0000)
0030CEF8 0002:
	_data_0030cef8 (0000)
00465A18 00b5:
	_bss_00465a18 (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

enum
{
	_d3d_texture_address_clamp = 3,
	_d3d_texture_state_address_u = 10,
	_d3d_texture_state_address_v = 11,
	_d3d_texture_state_mag_filter = 13,
	_d3d_texture_state_min_filter = 14,
	_d3d_texture_state_mip_filter = 15,
};

/* ---------- macros */

/* ---------- structures */

struct bitmap_data;

struct rasterizer_environment_debug_options
{
	byte reserved0[0x4];
	short disable_environment;
	byte reserved6[0x11];
	boolean specular_lightmaps;
	boolean reflection_mirrors;
	boolean reflections;
	boolean reflection_lightmaps;
	byte reserved1B[0x1A];
	boolean point_filtering;
};

struct rasterizer_environment_rasterizer_globals
{
	byte reserved0[0x60];
	short environment_mode;
};

struct rasterizer_environment_globals
{
	byte reserved0[0xAC];
	boolean lightmap_missing;
	byte reservedAD[0x8];
};

/* ---------- prototypes */

void rasterizer_profile_begin(
	short profile);

void rasterizer_profile_end(
	short profile);

void rasterizer_set_stencil_mode(
	short mode);

void rasterizer_set_texture_bitmap_data(
	short stage,
	struct bitmap_data const *bitmap);

void __fastcall D3DDevice_SetTextureState_Deferred(
	unsigned long stage,
	unsigned long type,
	unsigned long value);

void rasterizer_transparent_geometry_groups_begin(
	void);

void rasterizer_transparent_geometry_groups_end(
	void);

/* ---------- globals */

extern void *global_d3d_device;
extern struct rasterizer_environment_debug_options rasterizer_debug_options;
extern struct rasterizer_environment_rasterizer_globals rasterizer_globals;
extern struct rasterizer_environment_globals bss_00465a18;

/* ---------- public code */

void _rasterizer_environment_lightmap_end(
	void)
{
	return;
}

void _rasterizer_environment_lightmaps_end(
	void)
{
	rasterizer_profile_end(3);
	return;
}

void _rasterizer_environment_diffuse_light_end(
	void)
{
	return;
}

void _rasterizer_environment_diffuse_lights_end(
	void)
{
	rasterizer_profile_end(5);
	return;
}

void _rasterizer_environment_diffuse_textures_end(
	void)
{
	rasterizer_set_stencil_mode(2);
	rasterizer_profile_end(8);
	return;
}

void _rasterizer_environment_specular_light_end(
	void)
{
	return;
}

void _rasterizer_environment_specular_lights_end(
	void)
{
	rasterizer_profile_end(11);
	return;
}

void _rasterizer_environment_specular_lightmap_end(
	void)
{
	return;
}

void _rasterizer_environment_specular_lightmaps_end(
	void)
{
	rasterizer_profile_end(12);
	return;
}

void _rasterizer_environment_specular_lightmap_begin(
	struct bitmap_data const *lightmap_bitmap)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_environment.c",
		1874,
		global_d3d_device);

	if (!rasterizer_debug_options.disable_environment && rasterizer_debug_options.specular_lightmaps)
	{
		if (lightmap_bitmap)
		{
			rasterizer_set_texture_bitmap_data(1, lightmap_bitmap);
			D3DDevice_SetTextureState_Deferred(1, _d3d_texture_state_address_u, _d3d_texture_address_clamp);
			D3DDevice_SetTextureState_Deferred(1, _d3d_texture_state_address_v, _d3d_texture_address_clamp);
			D3DDevice_SetTextureState_Deferred(1, _d3d_texture_state_mag_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			D3DDevice_SetTextureState_Deferred(1, _d3d_texture_state_min_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			D3DDevice_SetTextureState_Deferred(1, _d3d_texture_state_mip_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			bss_00465a18.lightmap_missing = FALSE;
		}
		else
		{
			bss_00465a18.lightmap_missing = TRUE;
		}
	}
	return;
}

void _rasterizer_environment_reflection_lightmap_mask_end(
	void)
{
	return;
}

void _rasterizer_environment_reflection_lightmap_masks_end(
	void)
{
	rasterizer_profile_end(13);
	return;
}

void _rasterizer_environment_reflection_lightmap_mask_begin(
	struct bitmap_data const *lightmap_bitmap)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_environment.c",
		2120,
		global_d3d_device);

	if (!rasterizer_debug_options.disable_environment &&
		rasterizer_debug_options.reflection_mirrors &&
		rasterizer_debug_options.reflection_lightmaps &&
		!rasterizer_globals.environment_mode)
	{
		if (lightmap_bitmap)
		{
			rasterizer_set_texture_bitmap_data(0, lightmap_bitmap);
			D3DDevice_SetTextureState_Deferred(0, _d3d_texture_state_address_u, _d3d_texture_address_clamp);
			D3DDevice_SetTextureState_Deferred(0, _d3d_texture_state_address_v, _d3d_texture_address_clamp);
			D3DDevice_SetTextureState_Deferred(0, _d3d_texture_state_mag_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			D3DDevice_SetTextureState_Deferred(0, _d3d_texture_state_min_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			D3DDevice_SetTextureState_Deferred(0, _d3d_texture_state_mip_filter, (rasterizer_debug_options.point_filtering != FALSE) + 1);
			bss_00465a18.lightmap_missing = FALSE;
		}
		else
		{
			bss_00465a18.lightmap_missing = TRUE;
		}
	}
	return;
}

void _rasterizer_environment_reflection_mirrors_begin(
	void)
{
	rasterizer_profile_begin(14);
	return;
}

void _rasterizer_environment_reflection_mirrors_end(
	void)
{
	rasterizer_profile_end(14);
	return;
}

void _rasterizer_environment_reflections_begin(
	void)
{
	rasterizer_profile_begin(15);
	return;
}

void _rasterizer_environment_reflections_end(
	void)
{
	rasterizer_profile_end(15);
	return;
}

void _rasterizer_environment_transparent_geometry_begin(
	void)
{
	rasterizer_profile_begin(16);
	rasterizer_transparent_geometry_groups_begin();
	return;
}

void _rasterizer_environment_transparent_geometry_end(
	void)
{
	rasterizer_transparent_geometry_groups_end();
	rasterizer_profile_end(16);
	return;
}

/* ---------- private code */
