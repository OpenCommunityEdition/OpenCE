/*
CINEMATIC_FRAMING.C

A cinematic shot drawn no wider than the Xbox drew it, where what a wide
window adds at its sides is not meant to be seen. The port keeps a
camera's vertical angle and widens the view to the window's shape
(main.c, set_window_camera_values), cinematics too. a10's close shot of
the cryo bay's keyboard (the camera point keyboard_med, in the script
cryo) is seen through the assistant's eyes: the camera sits in his chest,
6 cm (0.06 units) under his head, looking down at his hands. The Xbox's
4:3 frame shows his forearms and hands; the strips a 16:9 window adds at
its sides show his upper arm and shoulder from inside him (the left one
from the shot's start, 150 of 1920 pixels across, the right one at its
end).

While the scripted camera looks from such a point (scripted_camera_point_
position: a network co-op client's camera is set to the point's position,
not its index), on a window wider than 4:3 the vertical angle is narrowed
by the ratio of a 4:3 window's width to the window's, so that the sides
are the Xbox's and the top and bottom are cropped. Narrower windows, every
other shot and play are drawn as they were. Only the drawing changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "camera/camera_scripting.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

#include "cinematic_framing.h"

/* ---------- constants */

/* how near the point the camera must be set to (a millimetre and a half) */
#define CINEMATIC_FRAMING_POINT_DISTANCE 0.0005f

/* ---------- structures */

/* a camera point whose shot keeps the Xbox's width */
struct cinematic_framing_shot
{
	char const *scenario_name;
	char const *camera_point_name;
};

/* ---------- globals */

static struct cinematic_framing_shot const cinematic_framing_shots[] =
{
	{"levels\\a10\\a10", "keyboard_med"},
};

/* ---------- private code */

/* whether the camera looks from one of the shots' points in this scenario */
static boolean cinematic_framing_camera_at_shot(
	char const *scenario_name,
	struct scenario *scenario,
	real_point3d const *camera)
{
	short shot_index;

	if (!scenario_name || !scenario || !scenario->cutscene_camera_points.address)
	{
		return FALSE;
	}
	for (shot_index = 0; shot_index < NUMBEROF(cinematic_framing_shots); shot_index++)
	{
		long point_index;

		if (csstrcmp(scenario_name, cinematic_framing_shots[shot_index].scenario_name))
		{
			continue;
		}
		for (point_index = 0; point_index < scenario->cutscene_camera_points.count; point_index++)
		{
			struct scenario_cutscene_camera_point const *point = TAG_BLOCK_GET_ELEMENT(
				&scenario->cutscene_camera_points, point_index, struct scenario_cutscene_camera_point);
			real dx = camera->x - point->position.x;
			real dy = camera->y - point->position.y;
			real dz = camera->z - point->position.z;

			if (!csstrcmp(point->name, cinematic_framing_shots[shot_index].camera_point_name) &&
				dx * dx + dy * dy + dz * dz <= CINEMATIC_FRAMING_POINT_DISTANCE * CINEMATIC_FRAMING_POINT_DISTANCE)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

/* the vertical angle on a window of that width and height that shows a 4:3
window's width */
static real cinematic_framing_xbox_width(
	real vertical_field_of_view,
	real width,
	real height)
{
	if (!(height > 0.f) || width * 3.f <= height * 4.f ||
		!(vertical_field_of_view > 0.f && vertical_field_of_view < _pi))
	{
		return vertical_field_of_view;
	}

	return 2.f * arctangent(tangent(0.5f * vertical_field_of_view) * (4.f * height) / (3.f * width), 1.f);
}

/* ---------- public code */

real cinematic_framing_vertical_field_of_view(
	rectangle2d const *viewport_bounds,
	real vertical_field_of_view)
{
	real_point3d camera;

	if (!viewport_bounds || !scripted_camera_point_position(&camera) || global_scenario_index == NONE ||
		!cinematic_framing_camera_at_shot(tag_get_name(global_scenario_index), global_scenario_get(), &camera))
	{
		return vertical_field_of_view;
	}

	return cinematic_framing_xbox_width(
		vertical_field_of_view,
		(real)(viewport_bounds->x1 - viewport_bounds->x0),
		(real)(viewport_bounds->y1 - viewport_bounds->y0));
}
