/*
RASTERIZER_CINEMATICS.C

symbols in this file:
0016D140 0020:
	_code_0016d140 (0000)
0016D160 0040:
	_rasterizer_screen_effects_initialize (0000)
0016D1A0 0030:
	_rasterizer_screen_effects_initialize_for_new_map (0000)
0016D1D0 0010:
	_rasterizer_screen_effects_dispose_from_old_map (0000)
0016D1E0 0010:
	_rasterizer_screen_effects_dispose (0000)
0016D1F0 0030:
	_rasterizer_script_screen_effect_set_value (0000)
0016D220 0030:
	_rasterizer_script_screen_effect_get_value (0000)
0016D250 0040:
	_rasterizer_screen_effect_start (0000)
0016D290 0070:
	_rasterizer_screen_effect_set_convolution (0000)
0016D300 0070:
	_rasterizer_screen_effect_set_filter (0000)
0016D370 0020:
	_rasterizer_screen_effect_set_filter_desaturation_tint (0000)
0016D390 0120:
	_rasterizer_screen_effect_set_video (0000)
0016D4B0 0010:
	_rasterizer_screen_effect_stop (0000)
0016D4C0 0250:
	_rasterizer_screen_effect_get_cinematic_parameters (0000)
0016D710 0020:
	_rasterizer_set_near_clip_distance (0000)
0016D730 0030:
	_rasterizer_get_near_clip_distance (0000)
0029D844 0020:
	??_C@_0CA@HAHKAAEK@cinematic_screen_effect_globals?$AA@ (0000)
0029D864 0032:
	??_C@_0DC@FCKAGPCD@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029D898 0014:
	??_C@_0BE@COKLOHBN@screen?5effect?5filth?$AA@ (0000)
0029D8B0 004a:
	??_C@_0EK@IJBENNIM@?$CD?$CD?$CD?5ERROR?5cinematics?5failed?5to?5s@ (0000)
0029D900 008d:
	??_C@_0IN@PCKCIAIJ@?$CD?$CD?$CD?5FATAL_ERROR?5screen?5effects?5c@ (0000)
004662F4 0004:
	_bss_004662f4 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "game/game.h"
#include "real_math.h"
#include "saved games/game_state.h"

/* ---------- constants */

/* ---------- macros */

#define cinematic_screen_effect_globals bss_004662f4

/* ---------- structures */

struct rasterizer_cinematic_screen_effect_parameters
{
	short convolution_extra_passes;
	short convolution_type;
	real convolution_radius;
	struct bitmap_data *convolution_mask;
	real filter_light_enhancement_intensity;
	real filter_desaturation_intensity;
	real_rgb_color filter_desaturation_tint;
	boolean filter_desaturation_is_additive;
	boolean filter_light_enhancement_uses_convolution_mask;
	boolean filter_desaturation_uses_convolution_mask;
	boolean video_on;
	short video_overbright_mode;
	byte reserved26[2];
	struct bitmap_data *video_scanline_map;
	real video_noise_intensity;
	real video_noise_map_scale;
	struct bitmap_data *video_noise_map;
};

struct rasterizer_cinematic_screen_effect_state
{
	struct rasterizer_cinematic_screen_effect_parameters parameters;
	boolean has_control;
	boolean initialized;
	byte reserved3A[2];
	real convolution_radius[2];
	real convolution_time[2];
	real filter_light_enhancement_intensity[2];
	real filter_desaturation_intensity[2];
	real filter_time[2];
	real script_values[4];
	real near_clip_distance;
};

struct rasterizer_global_defaults_prefix
{
	real near_clip_distance;
};

typedef char rasterizer_cinematic_screen_effect_parameters_size_assert[
	sizeof(struct rasterizer_cinematic_screen_effect_parameters) == 0x38 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_parameters_tint_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_parameters, filter_desaturation_tint) == 0x14 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_parameters_video_on_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_parameters, video_on) == 0x23 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_parameters_video_scanline_map_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_parameters, video_scanline_map) == 0x28 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_parameters_video_noise_map_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_parameters, video_noise_map) == 0x34 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_size_assert[
	sizeof(struct rasterizer_cinematic_screen_effect_state) == 0x78 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_has_control_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, has_control) == 0x38 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_initialized_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, initialized) == 0x39 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_convolution_radius_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, convolution_radius) == 0x3C ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_filter_time_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, filter_time) == 0x5C ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_script_values_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, script_values) == 0x64 ? 1 : -1];
typedef char rasterizer_cinematic_screen_effect_state_near_clip_distance_offset_assert[
	offsetof(struct rasterizer_cinematic_screen_effect_state, near_clip_distance) == 0x74 ? 1 : -1];

/* ---------- prototypes */

short main_get_window_count(
	void);

/* ---------- globals */

extern struct rasterizer_cinematic_screen_effect_state *bss_004662f4;
extern const struct rasterizer_global_defaults_prefix rasterizer_global_defaults;

/* ---------- public code */

real code_0016d140(
	void)
{
	return (real)game_time_get() * (1.0f / TICKS_PER_SECOND);
}

void rasterizer_screen_effects_initialize(
	void)
{
	cinematic_screen_effect_globals = game_state_malloc(
		"screen effect filth",
		NULL,
		sizeof(*cinematic_screen_effect_globals));
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\rasterizer_cinematics.c",
		54,
		cinematic_screen_effect_globals);

	return;
}

void rasterizer_screen_effects_dispose_from_old_map(
	void)
{
	return;
}

void rasterizer_screen_effects_dispose(
	void)
{
	return;
}

void rasterizer_script_screen_effect_set_value(
	word effect_index,
	real value)
{
	short signed_effect_index = (short)effect_index;

	if (bss_004662f4 &&
		signed_effect_index >= 0 &&
		signed_effect_index < 4)
	{
		bss_004662f4->script_values[signed_effect_index] = value;
	}

	return;
}

real rasterizer_script_screen_effect_get_value(
	short effect_index)
{
	real value = 0.0f;

	if (bss_004662f4 && effect_index >= 0 && effect_index < 4)
	{
		value = bss_004662f4->script_values[effect_index];
	}

	return value;
}

void rasterizer_screen_effect_start(
	boolean clear)
{
	struct rasterizer_cinematic_screen_effect_state *globals =
		cinematic_screen_effect_globals;

	if (globals)
	{
		if (clear || !globals->initialized)
		{
			csmemset(
				&globals->parameters,
				0,
				sizeof(globals->parameters));
			globals = cinematic_screen_effect_globals;
			globals->initialized = TRUE;
		}

		globals->has_control = TRUE;
	}

	return;
}

struct rasterizer_cinematic_screen_effect_state *rasterizer_screen_effect_get_cinematic_parameters(
	struct rasterizer_cinematic_screen_effect_state *parameters)
{
	struct rasterizer_cinematic_screen_effect_state *result;

	result = parameters;
	if (cinematic_screen_effect_globals && cinematic_screen_effect_globals->has_control)
	{
		real convolution_fraction;
		real filter_fraction;

		if (cinematic_screen_effect_globals->convolution_time[1] !=
			cinematic_screen_effect_globals->convolution_time[0])
		{
			convolution_fraction = PIN(
				(code_0016d140() - cinematic_screen_effect_globals->convolution_time[0]) /
					(cinematic_screen_effect_globals->convolution_time[1] -
						cinematic_screen_effect_globals->convolution_time[0]),
				0.0f,
				1.0f);
		}
		else
		{
			convolution_fraction = 1.0f;
		}

		if (cinematic_screen_effect_globals->filter_time[1] !=
			cinematic_screen_effect_globals->filter_time[0])
		{
			filter_fraction = PIN(
				(code_0016d140() - cinematic_screen_effect_globals->filter_time[0]) /
					(cinematic_screen_effect_globals->filter_time[1] -
						cinematic_screen_effect_globals->filter_time[0]),
				0.0f,
				1.0f);
		}
		else
		{
			filter_fraction = 1.0f;
		}

		scalars_interpolate(
			cinematic_screen_effect_globals->convolution_radius[0],
			cinematic_screen_effect_globals->convolution_radius[1],
			convolution_fraction,
			&cinematic_screen_effect_globals->parameters.convolution_radius);
		scalars_interpolate_and_clamp_0_to_1(
			cinematic_screen_effect_globals->filter_light_enhancement_intensity[0],
			cinematic_screen_effect_globals->filter_light_enhancement_intensity[1],
			filter_fraction,
			&cinematic_screen_effect_globals->parameters.filter_light_enhancement_intensity);
		scalars_interpolate_and_clamp_0_to_1(
			cinematic_screen_effect_globals->filter_desaturation_intensity[0],
			cinematic_screen_effect_globals->filter_desaturation_intensity[1],
			filter_fraction,
			&cinematic_screen_effect_globals->parameters.filter_desaturation_intensity);

		if (csmemcmp(
			&cinematic_screen_effect_globals->parameters.filter_desaturation_tint,
			global_real_rgb_black,
			sizeof(cinematic_screen_effect_globals->parameters.filter_desaturation_tint)) == 0)
		{
			parameters = cinematic_screen_effect_globals;
			parameters->parameters.filter_desaturation_tint = *global_real_rgb_green;
		}
		else
		{
			parameters = cinematic_screen_effect_globals;
		}

		if (parameters->parameters.convolution_radius <= _real_epsilon)
		{
			parameters->parameters.convolution_radius = 0.0f;
			parameters->parameters.convolution_type = 0;
			parameters->parameters.convolution_extra_passes = 0;
		}
		else
		{
			if (main_get_window_count() > 1)
			{
				display_assert(
					"### FATAL_ERROR screen effects can't use convolution when main_get_window_count>1\r\nmaybe you forgot to turn off the cinematic screen effect?",
					"c:\\halo\\SOURCE\\rasterizer\\rasterizer_cinematics.c",
					336,
					TRUE);
				system_exit(-1);
			}

			parameters = cinematic_screen_effect_globals;
		}

		if (parameters->parameters.filter_light_enhancement_intensity <= _real_epsilon &&
			parameters->parameters.filter_desaturation_intensity <= _real_epsilon &&
			filter_fraction >= 1.0f)
		{
			parameters->parameters.filter_light_enhancement_intensity = 0.0f;
			parameters->parameters.filter_desaturation_intensity = 0.0f;
		}

		return parameters;
	}

	return result;
}

void rasterizer_screen_effect_set_filter_desaturation_tint(
	real red,
	real green,
	real blue)
{
	struct rasterizer_cinematic_screen_effect_state *globals = bss_004662f4;

	if (globals)
	{
		globals->parameters.filter_desaturation_tint.red = red;
		globals->parameters.filter_desaturation_tint.green = green;
		globals->parameters.filter_desaturation_tint.blue = blue;
	}

	return;
}

void rasterizer_screen_effect_stop(
	void)
{
	if (bss_004662f4)
	{
		bss_004662f4->has_control = FALSE;
	}

	return;
}

void rasterizer_set_near_clip_distance(
	real near_clip_distance)
{
	if (bss_004662f4)
	{
		bss_004662f4->near_clip_distance = near_clip_distance;
	}

	return;
}

real rasterizer_get_near_clip_distance(
	void)
{
	real near_clip_distance = rasterizer_global_defaults.near_clip_distance;
	struct rasterizer_cinematic_screen_effect_state *globals = bss_004662f4;

	if (globals && globals->near_clip_distance > 0.0f)
	{
		near_clip_distance = globals->near_clip_distance;
	}

	return near_clip_distance;
}

/* ---------- private code */
