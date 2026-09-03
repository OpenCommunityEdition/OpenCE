/*
PLAYER_EFFECTS.C

symbols in this file:
00090D00 0070:
	_player_effect_get (0000)
00090D70 0040:
	_player_effect_initialize (0000)
00090DB0 0010:
	_player_effect_dispose (0000)
00090DC0 0040:
	_player_effect_initialize_for_new_map (0000)
00090E00 0010:
	_player_effect_dispose_from_old_map (0000)
00090E10 0140:
	_player_effect_add_continuous_effect (0000)
00090F50 0040:
	_scripted_player_effect_set_rotation (0000)
00090F90 0010:
	_scripted_player_effect_set_rumble (0000)
00090FA0 0040:
	_player_telefrag_effect_stop (0000)
00090FE0 0050:
	_player_effect_screen_fade_in (0000)
00091030 0050:
	_player_effect_screen_fade_out (0000)
00091080 0060:
	_player_effect_get_damage_indicators (0000)
000910E0 0020:
	_player_effect_clear_damage_indicators (0000)
00091100 0020:
	_effect_scale_factor (0000)
00091120 00f0:
	_player_effect_update_screen_flash (0000)
00091210 00d0:
	_player_effect_update_camera_shake (0000)
000912E0 0030:
	_effect_scale_value (0000)
00091310 0090:
	_player_effect_update (0000)
000913A0 0090:
	_player_effect_continuous_refresh (0000)
00091430 0030:
	_scripted_player_effect_set_translation (0000)
00091460 0050:
	_scripted_player_effect_start (0000)
000914B0 0040:
	_scripted_player_effect_stop (0000)
000914F0 0050:
	_player_effect_screen_flash (0000)
00091540 00f0:
	_player_telefrag_effect_start (0000)
00091630 0320:
	_player_effect_get_screen_flash (0000)
00091950 0090:
	_get_shake_matrix (0000)
000919E0 0520:
	_player_effect_get_camera_effect_matrix (0000)
00091F00 02f0:
	_player_effect_update_camera_impulse (0000)
000921F0 02e0:
	_player_effect_start (0000)
0025AA2C 0016:
	??_C@_0BG@DLCFJLMF@player_effect_globals?$AA@ (0000)
0025AA44 0028:
	??_C@_0CI@HGGDJELB@c?3?2halo?2SOURCE?2effects?2player_ef@ (0000)
0025AA6C 000f:
	??_C@_0P@DCHLBMFM@player?5effects?$AA@ (0000)
0025AA80 0008:
	__real@3f847ae147ae147b (0000)
0025AA88 0018:
	??_C@_0BI@MALCPACJ@screen_flash?9?$DOintensity?$AA@ (0000)
0025AAA0 003f:
	??_C@_0DP@NOLEHAI@screen_flash?9?$DOintensity?$DO?$DN0?40f?5?$CG?$CG@ (0000)
0025AAE0 000d:
	??_C@_0N@MFJHANDO@screen_flash?$AA@ (0000)
0025AAF0 0007:
	??_C@_06CGNOPMBC@matrix?$AA@ (0000)
0025AAF8 0004:
	__real@4016cbe4 (0000)
002DDDA0 000e:
	_render_screen_flash_type_map (0000)
0043D58C 0004:
	_player_effect_globals (0000)
*/

/* ---------- headers */

#include "effects/player_effects.h"

#include "game/game.h"
#include "game/game_globals.h"
#include "game/player_rumble.h"
#include "game/players.h"
#include "math/periodic_functions.h"
#include "main/console.h"
#include "networking/network_connection.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "objects/objects.h"
#include "render/render_cameras.h"
#include "saved games/game_state.h"
#include "tag_files/tag_groups.h"

#include <stddef.h>

/* ---------- constants */

enum
{
	NUMBER_OF_DAMAGE_INDICATORS = 4
};

enum screen_flash_type
{
	_screen_flash_type_none = 0,
	_screen_flash_type_lighten,
	_screen_flash_type_darken,
	_screen_flash_type_max,
	_screen_flash_type_min,
	_screen_flash_type_invert,
	_screen_flash_type_tint,

	NUMBER_OF_SCREEN_FLASH_TYPES
};

enum screen_flash_priority
{
	_screen_flash_low_priority = 0,
	_screen_flash_medium_priority,
	_screen_flash_high_priority,

	NUMBER_OF_SCREEN_FLASH_PRIORITIES
};

enum render_screen_flash_type
{
	_render_screen_flash_type_none = 0,
	_render_screen_flash_type_lighten,
	_render_screen_flash_type_darken,
	_render_screen_flash_type_max,
	_render_screen_flash_type_min,
	_render_screen_flash_type_invert,
	_render_screen_flash_type_tint,

	NUMBER_OF_RENDER_SCREEN_FLASH_TYPES
};

enum
{
	_scripted_player_effect_active_bit,
	_scripted_player_effect_stopping_bit,

	NUMBER_OF_SCRIPTED_PLAYER_EFFECT_FLAGS
};

enum
{
	_player_effect_screen_flash_just_started_bit,
	_player_effect_camera_impulse_just_started_bit,
	_player_effect_camera_shake_just_started_bit,

	NUMBER_OF_PLAYER_EFFECT_FLAGS
};

/* ---------- macros */

/* ---------- structures */

struct continuous_player_effect_datum
{
	real vibrate_frequencies[2];
	real translational_shake;
	real rotational_shake;
};

struct player_effect_datum
{
	real_vector3d direction;
	real_vector3d jitter;
	struct screen_flash_definition screen_flash;
	struct camera_impulse_definition camera_impulse;
	struct camera_shake_definition camera_shake;
	struct continuous_player_effect_datum continuous_effect;
	short continuous_effect_timer;
	short screen_flash_time_left;
	short camera_impulse_time_left;
	short camera_shake_time_left;
	byte damage_indicator_ticks[NUMBER_OF_DAMAGE_INDICATORS];
	byte flags;
	byte pad[3];
};

struct screen_fade_definition
{
	real_rgb_color color;
	long start_time;
	short ticks;
	boolean fading_out;
	byte pad;
};

struct scripted_player_effect_definition
{
	real_vector3d max_translation;
	real_euler_angles3d max_rotation;
	real max_intensity;
	short timer;
	short total_time;
};

struct player_effect_globals_definition
{
	struct player_effect_datum local_player_effect_data[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	struct screen_fade_definition screen_fade;
	struct scripted_player_effect_definition scripted_effect;
	unsigned long global_flags;
	long reference_time;
};

typedef char player_effect_datum_size_assert[
	sizeof(struct player_effect_datum) == 0xEC ? 1 : -1];
typedef char player_effect_datum_screen_flash_offset_assert[
	offsetof(struct player_effect_datum, screen_flash) == 0x18 ? 1 : -1];
typedef char player_effect_datum_camera_impulse_offset_assert[
	offsetof(struct player_effect_datum, camera_impulse) == 0x50 ? 1 : -1];
typedef char player_effect_datum_camera_shake_offset_assert[
	offsetof(struct player_effect_datum, camera_shake) == 0x84 ? 1 : -1];
typedef char player_effect_datum_continuous_offset_assert[
	offsetof(struct player_effect_datum, continuous_effect) == 0xCC ? 1 : -1];
typedef char player_effect_damage_indicator_ticks_offset_assert[
	offsetof(struct player_effect_datum, damage_indicator_ticks) == 0xE4 ? 1 : -1];
typedef char player_effect_globals_size_assert[
	sizeof(struct player_effect_globals_definition) == 0x3EC ? 1 : -1];
typedef char player_effect_globals_screen_fade_offset_assert[
	offsetof(struct player_effect_globals_definition, screen_fade) == 0x3B0 ? 1 : -1];
typedef char player_effect_globals_scripted_effect_offset_assert[
	offsetof(struct player_effect_globals_definition, scripted_effect) == 0x3C4 ? 1 : -1];
typedef char scripted_player_effect_definition_size_assert[
	sizeof(struct scripted_player_effect_definition) == 0x20 ? 1 : -1];
typedef char player_effect_globals_global_flags_offset_assert[
	offsetof(struct player_effect_globals_definition, global_flags) == 0x3E4 ? 1 : -1];
typedef char player_effect_globals_reference_time_offset_assert[
	offsetof(struct player_effect_globals_definition, reference_time) == 0x3E8 ? 1 : -1];

/* ---------- prototypes */

struct player_effect_datum *player_effect_get(
	short local_player_index);

static real effect_scale_factor(
	real zero_scale_factor,
	real scale);

static void player_effect_update_screen_flash(
	short local_player_index,
	struct player_effect_datum *effect,
	struct screen_flash_definition const *screen_flash,
	real scale,
	real time_scale);

static void player_effect_update_camera_shake(
	short local_player_index,
	struct player_effect_datum *effect,
	struct camera_shake_definition const *camera_shake,
	real scale,
	real time_scale);

/* ---------- globals */

static struct player_effect_globals_definition *player_effect_globals;

static short render_screen_flash_type_map[NUMBER_OF_SCREEN_FLASH_TYPES] =
{
	_render_screen_flash_type_none,
	_render_screen_flash_type_lighten,
	_render_screen_flash_type_darken,
	_render_screen_flash_type_max,
	_render_screen_flash_type_min,
	_render_screen_flash_type_invert,
	_render_screen_flash_type_tint
};

/* ---------- public code */

struct player_effect_datum *player_effect_get(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\effects\\player_effects.c", 115, local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	match_vassert("c:\\halo\\SOURCE\\effects\\player_effects.c", 116, player_effect_globals, "player_effect_globals");

	return &player_effect_globals->local_player_effect_data[local_player_index];
}

void player_effect_initialize(
	void)
{
	player_effect_globals = (struct player_effect_globals_definition *)game_state_malloc(
		"player effects",
		NULL,
		sizeof(struct player_effect_globals_definition));
	match_vassert("c:\\halo\\SOURCE\\effects\\player_effects.c", 127, player_effect_globals, "player_effect_globals");

	return;
}

void player_effect_dispose(
	void)
{
	return;
}

void player_effect_initialize_for_new_map(
	void)
{
	csmemset(player_effect_globals, 0, sizeof(struct player_effect_globals_definition));
	player_effect_globals->screen_fade.ticks = NONE;
	player_effect_globals->reference_time = game_time_get();

	return;
}

void player_effect_dispose_from_old_map(
	void)
{
	return;
}

void player_effect_add_continuous_effect(
	short local_player_index,
	long definition_index,
	real distance)
{
	struct continuous_damage_effect_definition *definition =
		continuous_damage_effect_definition_get(definition_index);

	if (distance < definition->cutoff_radius)
	{
		struct player_effect_datum *effect = player_effect_get(local_player_index);
		struct continuous_player_effect_datum *continuous = &effect->continuous_effect;
		real scale = PIN(1.0f - (distance - definition->falloff_radius) / (definition->falloff_radius - definition->cutoff_radius), 0.0f, 1.0f);
		real magnitude;

		magnitude = ((1.0f - definition->camera_shake.periodic_weight) +
			periodic_function_evaluate(definition->camera_shake.periodic_function, game_time_get() / definition->camera_shake.periodic_period) *
			definition->camera_shake.periodic_weight) * scale;

		if (effect->continuous_effect_timer > 0)
		{
			effect->continuous_effect_timer = 0;
			csmemset(continuous, 0, sizeof(*continuous));
		}

		continuous->translational_shake += MAX(
			magnitude * definition->camera_shake.random_translation_magnitude,
			0.0f);
		continuous->rotational_shake += MAX(
			magnitude * definition->camera_shake.random_rotation_magnitude,
			0.0f);
		continuous->vibrate_frequencies[0] += scale * definition->vibrate.frequencies[0];
		continuous->vibrate_frequencies[1] += scale * definition->vibrate.frequencies[1];
	}

	return;
}

void scripted_player_effect_set_rotation(
	real yaw,
	real pitch,
	real roll)
{
	player_effect_globals->scripted_effect.max_rotation.yaw = DEGREES_TO_RADIANS(yaw);
	player_effect_globals->scripted_effect.max_rotation.pitch = DEGREES_TO_RADIANS(pitch);
	player_effect_globals->scripted_effect.max_rotation.roll = DEGREES_TO_RADIANS(roll);

	return;
}

void scripted_player_effect_set_rumble(
	real left_motor,
	real right_motor)
{
	rumble_player_set_scripted_values(left_motor, right_motor);
	return;
}

void player_telefrag_effect_stop(
	long player_index)
{
	long local_player_index = player_get(player_index)->local_player_index;

	if (local_player_index != NONE)
	{
		player_effect_get((short)local_player_index);
		rumble_player_continuous((short)local_player_index, 0.0f, 0.0f);
	}

	return;
}

void player_effect_screen_fade_in(
	real red,
	real green,
	real blue,
	short ticks)
{
	player_effect_globals->screen_fade.color.red = red;
	player_effect_globals->screen_fade.color.green = green;
	player_effect_globals->screen_fade.color.blue = blue;
	player_effect_globals->screen_fade.ticks = ticks;
	player_effect_globals->screen_fade.fading_out = FALSE;
	player_effect_globals->screen_fade.start_time = game_time_get();

	return;
}

void player_effect_screen_fade_out(
	real red,
	real green,
	real blue,
	short ticks)
{
	player_effect_globals->screen_fade.color.red = red;
	player_effect_globals->screen_fade.color.green = green;
	player_effect_globals->screen_fade.color.blue = blue;
	player_effect_globals->screen_fade.ticks = ticks;
	player_effect_globals->screen_fade.fading_out = TRUE;
	player_effect_globals->screen_fade.start_time = game_time_get();

	return;
}

void player_effect_get_damage_indicators(
	short local_player_index,
	byte *damage_indicators)
{
	struct player_effect_datum *effect = player_effect_get(local_player_index);
	short index;

	csmemcpy(damage_indicators, effect->damage_indicator_ticks, sizeof(effect->damage_indicator_ticks));

	for (index = 0; index < NUMBER_OF_DAMAGE_INDICATORS; index++)
	{
		if (effect->damage_indicator_ticks[index])
		{
			effect->damage_indicator_ticks[index] = (byte)((game_time_get_elapsed() + effect->damage_indicator_ticks[index] < 255)
				? game_time_get_elapsed() + effect->damage_indicator_ticks[index]
				: 255);
		}
	}

	return;
}

void player_effect_clear_damage_indicators(
	short local_player_index)
{
	struct player_effect_datum *effect = player_effect_get(local_player_index);

	csmemset(
		effect->damage_indicator_ticks,
		0,
		sizeof(effect->damage_indicator_ticks));
	return;
}

static real effect_scale_factor(
	real zero_scale_factor,
	real scale)
{
	return zero_scale_factor + (1.0f - zero_scale_factor) * scale;
}

static void player_effect_update_screen_flash(
	short local_player_index,
	struct player_effect_datum *effect,
	struct screen_flash_definition const *screen_flash,
	real scale,
	real time_scale)
{
	real time_factor = time_scale * TICKS_PER_SECOND;

	if (!(effect->screen_flash.priority > screen_flash->priority &&
		effect->screen_flash_time_left > screen_flash->duration * time_factor) &&
		render_screen_flash_type_map[screen_flash->type])
	{
		effect->screen_flash = *screen_flash;
		effect->screen_flash.duration = time_factor * effect->screen_flash.duration;
		effect->screen_flash_time_left = (short)effect->screen_flash.duration;
		effect->screen_flash.zero_scale_factor = PIN(
			effect_scale_factor(screen_flash->zero_scale_factor, scale),
			0.0f,
			screen_flash->max_intensity);
		SET_FLAG(effect->flags, _player_effect_screen_flash_just_started_bit, TRUE);
	}

	return;
}

static void player_effect_update_camera_shake(
	short local_player_index,
	struct player_effect_datum *effect,
	struct camera_shake_definition const *camera_shake,
	real scale,
	real time_scale)
{
	real time_factor;
	real intensity;

	game_time_get();

	time_factor = time_scale * TICKS_PER_SECOND;
	intensity = effect_scale_factor(camera_shake->zero_scale_factor, scale);

	if (effect->camera_shake_time_left < time_factor * camera_shake->duration ||
		intensity > effect->camera_shake.zero_scale_factor ||
		(intensity >= effect->camera_shake.zero_scale_factor &&
			effect->camera_shake_time_left < time_factor * camera_shake->duration))
	{
		effect->camera_shake = *camera_shake;
		effect->camera_shake.zero_scale_factor = intensity;
		effect->camera_shake.duration = time_factor * effect->camera_shake.duration;
		effect->camera_shake_time_left = (short)effect->camera_shake.duration;
		effect->camera_shake.periodic_period = time_factor * effect->camera_shake.periodic_period;
		SET_FLAG(effect->flags, _player_effect_camera_shake_just_started_bit, TRUE);
	}

	return;
}

void player_effect_update(
	void)
{
	short local_player_index;

	for (local_player_index = local_player_get_next(NONE);
		local_player_index != NONE;
		local_player_index = local_player_get_next(local_player_index))
	{
		if (local_player_get_player_index(local_player_index) == NONE
			|| player_get(local_player_get_player_index(local_player_index))->unit_index == NONE)
		{
			player_effect_clear_damage_indicators(local_player_index);
			csmemset(player_effect_get(local_player_index), 0, sizeof(struct player_effect_datum));
			rumble_player_clear(local_player_index);
		}
	}

	return;
}

void player_effect_continuous_refresh(
	long definition_index,
	real_point3d const *position)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
	{
		if (local_player_get_player_index(local_player_index) != NONE)
		{
			long unit_index = player_get(local_player_get_player_index(local_player_index))->unit_index;

			if (unit_index != NONE)
			{
				real_point3d origin;

				object_get_origin(unit_index, &origin);
				player_effect_add_continuous_effect(local_player_index, definition_index, distance3d(&origin, position));
			}
		}
	}

	return;
}

void scripted_player_effect_set_translation(
	real horizontal,
	real vertical,
	real depth)
{
	real_vector3d *translation = &player_effect_globals->scripted_effect.max_translation;

	translation->i = horizontal;
	translation->j = vertical;
	translation->k = depth;

	return;
}

void scripted_player_effect_start(
	real maximum_intensity,
	real attack_time)
{
	short ticks;

	player_effect_globals->scripted_effect.max_intensity = maximum_intensity;

	ticks = (short)fast_ftol(attack_time * TICKS_PER_SECOND);
	player_effect_globals->scripted_effect.timer = ticks;
	player_effect_globals->scripted_effect.total_time = ticks;
	SET_FLAG(
		player_effect_globals->global_flags,
		_scripted_player_effect_stopping_bit,
		FALSE);
	SET_FLAG(
		player_effect_globals->global_flags,
		_scripted_player_effect_active_bit,
		TRUE);

	return;
}

void scripted_player_effect_stop(
	real duration)
{
	short ticks = (short)fast_ftol(duration * TICKS_PER_SECOND);

	player_effect_globals->scripted_effect.timer = ticks;
	player_effect_globals->scripted_effect.total_time = ticks;
	SET_FLAG(
		player_effect_globals->global_flags,
		_scripted_player_effect_stopping_bit,
		TRUE);

	return;
}

void player_effect_screen_flash(
	long player_index,
	struct screen_flash_definition const *screen_flash,
	real scale)
{
	if (player_index != NONE)
	{
		long local_player_index = player_get(player_index)->local_player_index;

		if (local_player_index != NONE)
		{
			struct player_effect_datum *effect = player_effect_get((short)local_player_index);

			player_effect_update_screen_flash((short)local_player_index, effect, screen_flash, scale, 1.0f);
		}
	}

	return;
}

void player_effect_get_screen_flash(
	short local_player_index,
	struct render_screen_flash *screen_flash)
{
	match_assert("c:\\halo\\SOURCE\\effects\\player_effects.c", 484, screen_flash);

	if (!console_is_active())
	{
		if (player_effect_globals->screen_fade.ticks != NONE &&
			(player_effect_globals->screen_fade.fading_out ||
				game_time_get() - player_effect_globals->screen_fade.start_time <= player_effect_globals->screen_fade.ticks))
		{
			screen_flash->type = _render_screen_flash_type_lighten;
			screen_flash->color.rgb = player_effect_globals->screen_fade.color;
			screen_flash->color.alpha = 1.0f;
			screen_flash->intensity = player_effect_globals->screen_fade.ticks > 0
				? transition_function_evaluate(
					_transition_function_cosine,
					PIN((real)(game_time_get() - player_effect_globals->screen_fade.start_time) / player_effect_globals->screen_fade.ticks, 0.0f, 1.0f))
				: 1.0f;

			if (!player_effect_globals->screen_fade.fading_out)
				screen_flash->intensity = 1.0f - screen_flash->intensity;

			screen_flash->intensity = PIN(screen_flash->intensity, 0.0f, 1.0f);
		}
		else if (local_player_index != NONE)
		{
			struct player_effect_datum *effect = player_effect_get(local_player_index);

			player_effect_globals->screen_fade.ticks = NONE;

			if (effect->screen_flash_time_left > 0 ||
				TEST_FLAG(effect->flags, _player_effect_screen_flash_just_started_bit))
			{
				SET_FLAG(
					effect->flags,
					_player_effect_screen_flash_just_started_bit,
					FALSE);

				screen_flash->type = render_screen_flash_type_map[effect->screen_flash.type];
				screen_flash->color = effect->screen_flash.screen_flash_color;
				if (effect->screen_flash.duration > 0.0f)
					screen_flash->intensity = transition_function_evaluate(
						effect->screen_flash.fade_function,
						effect->screen_flash.zero_scale_factor * ((real)effect->screen_flash_time_left / effect->screen_flash.duration));
				else
					screen_flash->intensity = effect->screen_flash.zero_scale_factor;

				effect->screen_flash_time_left -= game_time_get_elapsed();

				match_assert("c:\\halo\\SOURCE\\effects\\player_effects.c", 530, screen_flash->intensity>=0.0f && screen_flash->intensity<=1.0f);
			}
		}
	}

	match_assert_valid_real("c:\\halo\\SOURCE\\effects\\player_effects.c", 535, screen_flash->intensity);

	return;
}

void player_telefrag_effect_start(
	long player_index,
	real intensity)
{
	struct screen_flash_definition screen_flash = {0};
	struct camera_shake_definition camera_shake = {0};
	long local_player_index = player_get(player_index)->local_player_index;

	if (local_player_index != NONE)
	{
		struct player_effect_datum *effect = player_effect_get((short)local_player_index);

		camera_shake.random_translation_magnitude = intensity * 0.01;
		screen_flash.max_intensity = intensity;
		camera_shake.duration = 1.0f;
		screen_flash.type = _screen_flash_type_lighten;
		screen_flash.priority = _screen_flash_high_priority;
		screen_flash.duration = 1.0f;
		screen_flash.zero_scale_factor = 0.0f;
		screen_flash.screen_flash_color = *global_real_argb_white;

		rumble_player_continuous((short)local_player_index, intensity, intensity);
		player_effect_update_screen_flash((short)local_player_index, effect, &screen_flash, intensity, 1.0f);
		player_effect_update_camera_shake((short)local_player_index, effect, &camera_shake, intensity, 1.0f);
	}

	return;
}

/* ---------- private code */
