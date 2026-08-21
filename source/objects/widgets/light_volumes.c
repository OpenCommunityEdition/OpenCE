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
	_code_00124490 (0000)
001246A0 0030:
	_code_001246a0 (0000)
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
	_bss_00456d90 (0000)
*/

/* ---------- headers */

#include "objects/widgets/light_volumes.h"

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "render/render.h"
#include "saved games/game_state.h"
#include "shaders/shaders.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

typedef void (*light_volume_render_proc)(
	long object_index,
	long light_volume_index);

/* ---------- prototypes */

real code_001246a0(
	real value,
	real exponent);
void light_volume_render(
	long object_index,
	long light_volume_index);
void rasterizer_widget_submit(
	long object_index,
	long light_volume_index,
	real_point3d const *centroid,
	light_volume_render_proc render_proc);

/* ---------- globals */

struct light_volume_globals bss_00456d90 = {0};

/* ---------- public code */

void light_volumes_initialize(
	void)
{
	bss_00456d90.light_volumes = game_state_data_new("light volumes", 256, 8);
	if (!bss_00456d90.light_volumes)
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
	if (bss_00456d90.light_volumes)
		data_make_valid(bss_00456d90.light_volumes);

	return;
}

void light_volumes_dispose_from_old_map(
	void)
{
	if (bss_00456d90.light_volumes)
		data_make_invalid(bss_00456d90.light_volumes);

	return;
}

long light_volume_new(
	long definition_index)
{
	long light_volume_index = datum_new(bss_00456d90.light_volumes);

	if (light_volume_index != NONE)
		light_volume_get(light_volume_index)->definition_index = definition_index;

	return light_volume_index;
}

void light_volume_delete(
	long light_volume_index)
{
	if (light_volume_index != NONE)
		datum_delete(bss_00456d90.light_volumes, light_volume_index);

	return;
}

/* ---------- private code */

real code_001246a0(
	real value,
	real exponent)
{
	if (exponent != 1.f)
		value = power(value, exponent);

	return value;
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
