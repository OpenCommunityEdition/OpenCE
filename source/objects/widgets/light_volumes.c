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
#include "cseries/cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "saved games/game_state.h"
#include "shaders/shaders.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- prototypes */

static struct light_volume_frame *light_volume_interpolate_frames(
	struct light_volume_definition *definition,
	long object_index);
static real pow1(
	real value,
	real exponent);

/* ---------- globals */

struct light_volume_globals light_volume_globals = {0};

/* ---------- public code */

void light_volumes_initialize(
	void)
{
	light_volume_globals.light_volume_data = game_state_data_new("light volumes", 256, 8);
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
	if (exponent != 1.f)
		value = power(value, exponent);

	return value;
}

void light_volume_render(
	long object_index,
	long light_volume_index)
{
	struct light_volume_datum *light_volume;
	struct light_volume_definition *definition;
	struct light_volume_frame *frame;
	struct object_marker marker;
	real_point3d delta;
	real parallel_factor;
	real angle_brightness;
	real intensity;

	if (object_index != NONE && light_volume_index != NONE)
	{
		light_volume = light_volume_get(light_volume_index);
		definition = light_volume_definition_get(light_volume->definition_index);
		if (definition->count > 0 && definition->frames.count > 0)
		{
			real distance_fade = 1.f;

			frame = light_volume_interpolate_frames(definition, object_index);
			object_get_marker_by_name(object_index, definition->attachment_marker, &marker, 1);
			delta.x = marker.matrix.position.x - render.camera.position.x;
			delta.y = marker.matrix.position.y - render.camera.position.y;
			delta.z = marker.matrix.position.z - render.camera.position.z;

			parallel_factor =
				marker.matrix.forward.i * render.camera.forward.i +
				(render.camera.forward.j * marker.matrix.forward.j +
				render.camera.forward.k * marker.matrix.forward.k);
			parallel_factor = ABS(parallel_factor);

			if (definition->far_fade_distance > 0.f)
			{
				real depth =
					delta.x * render.camera.forward.i +
					(render.camera.forward.j * delta.y +
					render.camera.forward.k * delta.z);
				real fade =
					(depth - definition->far_fade_distance) /
					(definition->near_fade_distance - definition->far_fade_distance);

				distance_fade = PIN(fade, 0.f, 1.f);
			}

			angle_brightness =
				definition->perpendicular_brightness_scale * (1.f - parallel_factor) +
				definition->parallel_brightness_scale * parallel_factor;
			angle_brightness = PIN(angle_brightness, 0.f, 1.f);

			intensity = angle_brightness * distance_fade;
			{
				real function_value;

				if (object_get_function_value(object_index, definition->brightness_scale_source - 1, &function_value))
					intensity *= function_value;
			}

			if (
				intensity > 0.f &&
				(frame->color_hither.alpha > 0.f || frame->color_yon.alpha > 0.f) &&
				(frame->radius_hither > 0.f || frame->radius_yon > 0.f))
			{
				short count;
				long sprite_index;

				rasterizer_widget_begin(
					_widget_type_internal_sprite,
					FLAG(_rasterizer_widget_z_enable_bit));
				rasterizer_widget_set_texture(
					0,
					definition->map.index,
					definition->sequence_index);
				count = definition->count;
				if (count > 0)
				{
					sprite_index = 0;
					do
					{
						real offset_fraction;
						real radius_fraction;
						real color_fraction;
						real brightness_fraction;
						real radius;
						real offset;
						real_argb_color color;
						real_point3d position;

						offset_fraction = (real)sprite_index / (real)(definition->count - 1);
						offset_fraction = pow1(
							offset_fraction,
							frame->offset_exponent);
						radius_fraction = pow1(
							offset_fraction,
							frame->radius_exponent);
						radius =
							(1.f - radius_fraction) * frame->radius_hither +
							frame->radius_yon * radius_fraction;
						color_fraction = pow1(
							offset_fraction,
							frame->color_exponent);
						brightness_fraction = pow1(
							offset_fraction,
							frame->brightness_exponent);

						offset = frame->length * offset_fraction + frame->offset_from_marker;
						position.x = marker.matrix.forward.i * offset + marker.matrix.position.x;
						position.y = marker.matrix.forward.j * offset + marker.matrix.position.y;
						position.z = marker.matrix.forward.k * offset + marker.matrix.position.z;

						rgb_colors_interpolate(
							&color.rgb,
							definition->flags &
								(FLAG(_light_volume_color_interpolate_in_hsv_bit) |
								 FLAG(_light_volume_color_interpolate_along_farthest_hue_path_bit)),
							&frame->color_hither.rgb,
							&frame->color_yon.rgb,
							color_fraction);
						color.alpha =
							((1.f - brightness_fraction) * frame->color_hither.alpha +
							frame->color_yon.alpha * brightness_fraction) * intensity;
						rasterizer_widget_draw_sprite3d(
							&position,
							radius,
							NULL,
							0.0f,
							real_argb_color_to_pixel32(&color));
						sprite_index++;
					}
					while (--count);
				}
				rasterizer_widget_end();
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
			if (!source || !animation || animation->values[source - 1] > 0.f)
			{
				struct object_marker marker;
				real_point3d delta;
				real distance;

				object_get_marker_by_name(object_index, definition->attachment_marker, &marker, 1);
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
