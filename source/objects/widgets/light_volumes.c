/*
LIGHT_VOLUMES.C

symbols in this file:
001243A0 0040:
	_light_volumes_initialize (0000)
001243E0 0010:
	_light_volumes_dispose (0000)
001243F0 0020:
	_light_volumes_initialize_for_new_map (0000)
00124410 0020:
	_light_volumes_dispose_from_old_map (0000)
00124430 0040:
	_light_volume_new (0000)
00124470 0020:
	_light_volume_delete (0000)
00124490 0210:
	_light_volume_interpolate_frames (0000)
001246A0 0030:
	_pow1 (0000)
001246D0 0390:
	_light_volume_render (0000)
00124A60 0110:
	_light_volume_submit (0000)
002891A0 0027:
	??_C@_0CH@EHOFEIKK@light_volume_globals?4light_volum@ (0000)
002891C8 002f:
	??_C@_0CP@PLPCECFN@c?3?2halo?2SOURCE?2objects?2widgets?2l@ (0000)
002891F8 000e:
	??_C@_0O@BAADBFJE@light?5volumes?$AA@ (0000)
00456D90 00b4:
	_light_volume_globals (0000)
*/

/* ---------- headers */

#include "objects/widgets/light_volumes.h"

#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_color_conversion.h"
#include "cseries/cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "interface/first_person_weapons.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer_widgets.h"
#include "rasterizer/rasterizer.h"

void rasterizer_set_stencil_mode(long stencil_mode);
#include "render/render.h"
#include "saved games/game_state.h"
#include "shaders/shaders.h"

#include <math.h>

/* ---------- constants */

/* ---------- macros */

#define light_volume_get(light_volume_index) \
	((struct light_volume_datum *)datum_get(light_volume_globals.light_volume_data, (light_volume_index)))

/* ---------- prototypes */

static struct light_volume_frame *light_volume_interpolate_frames(
	struct light_volume_definition *definition,
	long object_index);
static real pow1(
	real value,
	real exponent);
static void light_volume_ease_marker(
	long light_volume_index,
	long object_index,
	char const *marker_name,
	struct object_marker *marker);

/* ---------- globals */

static struct light_volume_globals light_volume_globals = {0};

/* the beam last drawn, so a marker that moves on the tick can be eased
across the frames in between. one slot per light volume the pool can hold */
struct light_volume_drawn
{
	long object_index;
	boolean valid;
	real_point3d position;
	real_vector3d forward;
};

static struct light_volume_drawn light_volume_drawn_markers[HALO_PORT_MAXIMUM_LIGHT_VOLUMES];

/* ---------- public code */

void light_volumes_initialize(
	void)
{
	light_volume_globals.light_volume_data = game_state_data_new("light volumes", HALO_PORT_MAXIMUM_LIGHT_VOLUMES, 8);
	if (!light_volume_globals.light_volume_data)
	{
		display_assert(
			"light_volume_globals.light_volume_data",
			"c:\\halo\\SOURCE\\objects\\widgets\\light_volumes.c",
			44,
			TRUE);
		system_exit(-1);
	}

	return;
}

void light_volumes_dispose(
	void)
{
	return;
}

void light_volumes_initialize_for_new_map(
	void)
{
	if (light_volume_globals.light_volume_data)
		data_make_valid(light_volume_globals.light_volume_data);

	return;
}

void light_volumes_dispose_from_old_map(
	void)
{
	if (light_volume_globals.light_volume_data)
		data_make_invalid(light_volume_globals.light_volume_data);

	return;
}

long light_volume_new(
	long definition_index)
{
	long light_volume_index = datum_new(light_volume_globals.light_volume_data);

	if (light_volume_index != NONE)
		light_volume_get(light_volume_index)->definition_index = definition_index;

	return light_volume_index;
}

void light_volume_delete(
	long light_volume_index)
{
	if (light_volume_index != NONE)
		datum_delete(light_volume_globals.light_volume_data, light_volume_index);

	return;
}

/* ---------- private code */

static struct light_volume_frame *light_volume_interpolate_frames(
	struct light_volume_definition *definition,
	long object_index)
{
	real function_value;
	struct light_volume_frame *result;

	match_assert(
		"c:\\halo\\SOURCE\\objects\\widgets\\light_volumes.c",
		110,
		definition);

	result = TAG_BLOCK_GET_ELEMENT(
		&definition->frames,
		0,
		struct light_volume_frame);

	if (definition->frames.count > 1)
	{
		struct light_volume_frame *frame0;
		struct light_volume_frame *frame1;
		real inverse_function_value;

		frame0 = TAG_BLOCK_GET_ELEMENT(
			&definition->frames,
			0,
			struct light_volume_frame);
		frame1 = TAG_BLOCK_GET_ELEMENT(
			&definition->frames,
			0,
			struct light_volume_frame);

		if (object_get_function_value(
			object_index,
			definition->frame_animation_source - 1,
			&function_value))
		{
			result = &light_volume_globals.frame_storage;
			inverse_function_value = 1.0f - function_value;

			light_volume_globals.frame_storage.offset_from_marker =
				frame0->offset_from_marker * inverse_function_value + frame1->offset_from_marker * function_value;
			light_volume_globals.frame_storage.offset_exponent =
				frame0->offset_exponent * inverse_function_value + frame1->offset_exponent * function_value;
			light_volume_globals.frame_storage.length =
				frame0->length * inverse_function_value + frame1->length * function_value;
			light_volume_globals.frame_storage.radius_hither =
				frame0->radius_hither * inverse_function_value + frame1->radius_hither * function_value;
			light_volume_globals.frame_storage.radius_yon =
				frame0->radius_yon * inverse_function_value + frame1->radius_yon * function_value;
			light_volume_globals.frame_storage.radius_exponent =
				frame0->radius_exponent * inverse_function_value + frame1->radius_exponent * function_value;
			light_volume_globals.frame_storage.color_hither.alpha =
				frame0->color_hither.alpha * inverse_function_value + frame1->color_hither.alpha * function_value;
			light_volume_globals.frame_storage.color_hither.red =
				frame0->color_hither.red * inverse_function_value + frame1->color_hither.red * function_value;
			light_volume_globals.frame_storage.color_hither.green =
				frame0->color_hither.green * inverse_function_value + frame1->color_hither.green * function_value;
			light_volume_globals.frame_storage.color_hither.blue =
				frame0->color_hither.blue * inverse_function_value + frame1->color_hither.blue * function_value;
			light_volume_globals.frame_storage.color_yon.alpha =
				frame0->color_yon.alpha * inverse_function_value + frame1->color_yon.alpha * function_value;
			light_volume_globals.frame_storage.color_yon.red =
				frame0->color_yon.red * inverse_function_value + frame1->color_yon.red * function_value;
			light_volume_globals.frame_storage.color_yon.green =
				frame0->color_yon.green * inverse_function_value + frame1->color_yon.green * function_value;
			light_volume_globals.frame_storage.color_yon.blue =
				frame0->color_yon.blue * inverse_function_value + frame1->color_yon.blue * function_value;
			light_volume_globals.frame_storage.color_exponent =
				frame0->color_exponent * inverse_function_value + frame1->color_exponent * function_value;
			light_volume_globals.frame_storage.brightness_exponent =
				frame0->brightness_exponent * inverse_function_value + frame1->brightness_exponent * function_value;
		}
	}
	else
	{
		result = TAG_BLOCK_GET_ELEMENT(
			&definition->frames,
			0,
			struct light_volume_frame);
	}

	return result;
}

static real pow1(
	real value,
	real exponent)
{
	return exponent != 1.f ? pow(value, exponent) : value;
}

/* port: how far this frame eases a beam toward the pose of the tick. A
remote player's aim arrives every couple of ticks, and the picture is drawn
faster than the ticks, so the beam would step. About 40 ms to settle. */
static real light_volume_ease_alpha(
	void)
{
	static long eased_tick = NONE;
	static real eased_fraction = 0.0f;
	static real eased_alpha = 0.5f;
	long tick = game_time_get();
	real fraction = game_time_get_tick_fraction();

	if (tick != eased_tick || fraction != eased_fraction)
	{
		real dt;

		if (eased_tick == NONE || tick < eased_tick)
			dt = 1.0f / 60.0f;
		else
			dt = ((real)(tick - eased_tick) + (fraction - eased_fraction)) / (real)TICKS_PER_SECOND;
		if (!(dt > 0.0f && dt < 0.1f))
			dt = 1.0f / 60.0f;
		eased_alpha = 1.0f - (real)exp((double)(-dt / 0.04f));
		eased_tick = tick;
		eased_fraction = fraction;
	}
	return eased_alpha;
}

/* port: the local player's own beam, in a match, from the viewmodel, which
follows the view every frame. Anyone else's eases toward the tick's marker. */
static void light_volume_ease_marker(
	long light_volume_index,
	long object_index,
	char const *marker_name,
	struct object_marker *marker)
{
	struct object_marker viewmodel;
	long absolute_index;
	struct light_volume_drawn *drawn;
	real alpha;
	real_vector3d step;
	real length_squared;

	if (game_connection() == _game_connection_local)
		return;
	if (first_person_weapon_get_marker_by_name_render(object_index, marker_name, &viewmodel, 1) > 0)
	{
		marker->matrix.position = viewmodel.matrix.position;
		marker->matrix.forward = viewmodel.matrix.forward;
		marker->matrix.up = viewmodel.matrix.up;
		return;
	}
	/* a muzzle flash is a short volume. Easing it leaves the cone behind the
	barrel and it ends on a hard edge. Only the flashlight beam is eased. */
	{
		char const *expected = "flashlight";

		if (!marker_name)
			return;
		while (*expected != 0 && *marker_name == *expected)
		{
			marker_name++;
			expected++;
		}
		if (*expected != 0 || *marker_name != 0)
			return;
	}
	absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(light_volume_index);
	if (absolute_index < 0 || absolute_index >= HALO_PORT_MAXIMUM_LIGHT_VOLUMES)
		return;
	drawn = &light_volume_drawn_markers[absolute_index];
	step.i = marker->matrix.position.x - drawn->position.x;
	step.j = marker->matrix.position.y - drawn->position.y;
	step.k = marker->matrix.position.z - drawn->position.z;
	length_squared = step.i * step.i + step.j * step.j + step.k * step.k;
	if (!drawn->valid || drawn->object_index != object_index || !(length_squared <= 4.0f))
	{
		drawn->valid = TRUE;
		drawn->object_index = object_index;
		drawn->position = marker->matrix.position;
		drawn->forward = marker->matrix.forward;
		return;
	}
	alpha = light_volume_ease_alpha();
	drawn->position.x += step.i * alpha;
	drawn->position.y += step.j * alpha;
	drawn->position.z += step.k * alpha;
	drawn->forward.i += (marker->matrix.forward.i - drawn->forward.i) * alpha;
	drawn->forward.j += (marker->matrix.forward.j - drawn->forward.j) * alpha;
	drawn->forward.k += (marker->matrix.forward.k - drawn->forward.k) * alpha;
	length_squared = drawn->forward.i * drawn->forward.i + drawn->forward.j * drawn->forward.j +
		drawn->forward.k * drawn->forward.k;
	if (length_squared > 0.0f)
	{
		real inverse = 1.0f / (real)sqrt((double)length_squared);

		drawn->forward.i *= inverse;
		drawn->forward.j *= inverse;
		drawn->forward.k *= inverse;
	}
	marker->matrix.position = drawn->position;
	marker->matrix.forward = drawn->forward;
}

void light_volume_render(
	long object_index,
	long light_volume_index)
{
	if (object_index != NONE && light_volume_index != NONE)
	{
		struct light_volume_datum *light_volume = light_volume_get(light_volume_index);
		struct light_volume_definition *definition = light_volume_definition_get(light_volume->definition_index);

		if (definition->count > 0 && definition->frames.count > 0)
		{
			struct light_volume_frame *frame = light_volume_interpolate_frames(definition, object_index);
			struct object_marker marker;
			real_vector3d eye_to_marker_vector;
			real brightness;
			real parallel_factor;

			object_get_marker_by_name(object_index, definition->attachment_marker, &marker, 1);
			light_volume_ease_marker(light_volume_index, object_index, definition->attachment_marker, &marker);
			brightness = 1.f;
			vector_from_points3d(&render.camera.position, &marker.matrix.position, &eye_to_marker_vector);
			parallel_factor = ABS(dot_product3d(&render.camera.forward, &marker.matrix.forward));
			{
				real external_scale = 1.f;

				if (definition->far_fade_distance > 0.f)
				{
					brightness *= PIN(
						(dot_product3d(&render.camera.forward, &eye_to_marker_vector) - definition->far_fade_distance) /
						(definition->near_fade_distance - definition->far_fade_distance),
						0.f,
						1.f);
				}
				brightness *= PIN(
					(1.f - parallel_factor) * definition->perpendicular_brightness_scale +
					parallel_factor * definition->parallel_brightness_scale,
					0.f,
					1.f);
				if (object_get_function_value(object_index, definition->brightness_scale_source - 1, &external_scale))
					brightness *= external_scale;
			}

			if (
				brightness > 0.f &&
				(frame->color_hither.alpha > 0.f || frame->color_yon.alpha > 0.f) &&
				(frame->radius_hither > 0.f || frame->radius_yon > 0.f))
			{
				short count;
				short sprite_index;
				short pass;
				short pass_count;
				long local_index = first_person_weapon_get_local_index(object_index);
				char const *marker_name = definition->attachment_marker;
				char const *flashlight = "flashlight";
				boolean on_the_gun = local_index != NONE &&
					local_index == render.local_player_index;

				/* the flashlight beam still stops on a wall */
				if (on_the_gun && marker_name)
				{
					while (*flashlight != 0 && *marker_name == *flashlight)
					{
						marker_name++;
						flashlight++;
					}
					if (*flashlight == 0 && *marker_name == 0)
						on_the_gun = FALSE;
				}
				/* port: one pass on the gun, where its depth hides the charge,
				and one around the body so the world does not */
				pass_count = on_the_gun ? 2 : 1;
				for (pass = 0; pass < pass_count; pass++)
				{
				if (on_the_gun)
					rasterizer_set_stencil_mode(pass == 0 ? 3 : 2);
				rasterizer_widget_begin(
					_widget_type_internal_sprite,
					!on_the_gun || pass == 0 ?
						FLAG(_rasterizer_widget_z_enable_bit) : 0);
				rasterizer_widget_set_texture(
					0,
					definition->map.index,
					definition->sequence_index);
				rasterizer_widget_batch_sprites(TRUE); /* port: one draw */
				count = definition->count;
				for (sprite_index = 0; sprite_index < count; sprite_index++)
				{
					real offset_fraction = pow1((real)sprite_index / (real)(count - 1), frame->offset_exponent);
					real_point3d position;
					real radius_fraction = pow1(offset_fraction, frame->radius_exponent);
					real radius = (1.f - radius_fraction) * frame->radius_hither + frame->radius_yon * radius_fraction;
					real color_fraction = pow1(offset_fraction, frame->color_exponent);
					real brightness_fraction = pow1(offset_fraction, frame->brightness_exponent);
					real_argb_color color;

					point_from_line3d(
						&marker.matrix.position,
						&marker.matrix.forward,
						offset_fraction * frame->length + frame->offset_from_marker,
						&position);
					rgb_colors_interpolate(
						&color.rgb,
						definition->flags &
							(FLAG(_light_volume_color_interpolate_in_hsv_bit) |
							 FLAG(_light_volume_color_interpolate_along_farthest_hue_path_bit)),
						&frame->color_hither.rgb,
						&frame->color_yon.rgb,
						color_fraction);
					color.alpha =
						(1.f - brightness_fraction) * frame->color_hither.alpha +
						frame->color_yon.alpha * brightness_fraction;
					color.alpha *= brightness;
					rasterizer_widget_draw_sprite3d(
						&position,
						radius,
						NULL,
						0.0f,
						real_argb_color_to_pixel32(&color));
				}
				rasterizer_widget_batch_sprites(FALSE); /* port: (drawn) */
				rasterizer_widget_end();
				}
				if (on_the_gun)
					rasterizer_set_stencil_mode(0);
			}
		}
	}

	return;
}

void light_volume_submit(
	long object_index,
	long light_volume_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation)
{
	struct light_volume_datum *light_volume;
	struct light_volume_definition *definition;

	(void)lighting;
	if (object_index != NONE && light_volume_index != NONE)
	{
		light_volume = light_volume_get(light_volume_index);
		definition = light_volume_definition_get(light_volume->definition_index);
		if (definition->count > 0 && definition->frames.count > 0)
		{
			short source = definition->brightness_scale_source;
			/* port: a source past a to d is none (a map's; it read past the
			object's four values) */
			if (!(source >= _object_function_reference_a && source <= _object_function_reference_d))
			{
				source = 0;
			}
			if (!source || !animation || animation->values[source - 1] > 0.f)
			{
				struct object_marker marker;
				real_point3d delta;
				real distance;

				/* port: the local weapon's volume is on the viewmodel */
				if (first_person_weapon_get_marker_by_name_render(
					object_index,
					definition->attachment_marker,
					&marker,
					1) <= 0)
				{
					object_get_marker_by_name(object_index, definition->attachment_marker, &marker, 1);
				}
				delta.x = marker.matrix.position.x - render.camera.position.x;
				delta.y = marker.matrix.position.y - render.camera.position.y;
				delta.z = marker.matrix.position.z - render.camera.position.z;
				distance =
					render.camera.forward.i * delta.x +
					(render.camera.forward.j * delta.y +
					render.camera.forward.k * delta.z);

				if (definition->far_fade_distance == 0.f || distance < definition->far_fade_distance)
					rasterizer_widget_submit(
						object_index,
						light_volume_index,
						&marker.matrix.position,
						light_volume_render);
			}
		}
	}

	return;
}
