/* FOV in horizontal degrees at 16:9. The observer, its transitions and the
network's camera values remain authored; only the local render view changes.
Default follows the original projection exactly. */
#include "cseries.h"
#include "render_fov.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "items/weapons.h"
#include "render/render_cameras.h"

#include <math.h>

double config_real(const char *name);
unsigned long config_changes(void);

float render_fov_vertical(short local_player_index, float native_vertical_field_of_view)
{
	static boolean initialized;
	static unsigned long read_at;
	static real requested_tangent;
	unsigned long changes = config_changes();
	long unit_index, weapon_index;
	struct unit_datum *unit;
	real unzoomed_angle, unzoomed_tangent, native_tangent, adjusted_tangent;

	if (!initialized || read_at != changes)
	{
		double degrees = config_real("display.fov");

		initialized = TRUE;
		read_at = changes;
		requested_tangent = isfinite(degrees) && degrees >= 20.0 && degrees <= 150.0 ?
			(real)(tan(degrees * 3.14159265358979323846 / 360.0) * (9.0 / 16.0)) : 0.0f;
	}
	if (!requested_tangent || local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		cinematic_in_progress() || (director_camera_scripted && *director_camera_scripted) ||
		director_get_perspective(local_player_index) != _director_perspective_first_person ||
		director_inhibited_facing(local_player_index))
	{
		return native_vertical_field_of_view;
	}
	unit_index = player_control_get_unit_index(local_player_index);
	if (unit_index == NONE)
		return native_vertical_field_of_view;
	unit = unit_try_and_get(unit_index);
	if (!unit || unit->object.parent_object_index != NONE || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return native_vertical_field_of_view;

	unzoomed_angle = unit_definition_get(unit->definition_index)->unit.camera_field_of_view;
	if (!(unzoomed_angle >= 0.001f && unzoomed_angle <= _pi / 2.0f) ||
		!(native_vertical_field_of_view > 0.0f && native_vertical_field_of_view < _pi))
	{
		return native_vertical_field_of_view;
	}
	unzoomed_tangent = 0.75f * render_camera_get_adjusted_field_of_view_tangent(unzoomed_angle);
	native_tangent = tanf(native_vertical_field_of_view * 0.5f);
	if (requested_tangent < unzoomed_tangent)
	{
		/* A narrow base view must not become wider when zoom is pressed. */
		adjusted_tangent = native_tangent * (requested_tangent / unzoomed_tangent);
	}
	else
	{
		real blend = 1.0f;

		weapon_index = unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index);
		if (weapon_index != NONE)
		{
			real scoped_angle = weapon_get_field_of_view(weapon_index, unzoomed_angle, 0);

			if (scoped_angle > 0.0f && scoped_angle < unzoomed_angle)
			{
				real scoped_tangent = 0.75f * render_camera_get_adjusted_field_of_view_tangent(scoped_angle);
				/* The observer's atan/tan round trip can land just above
				the endpoint. Keep a completed scope's original angle exactly. */
				if (native_tangent <= scoped_tangent * 1.000001f)
					return native_vertical_field_of_view;

				/* Fade the extra width along the observer's existing scope
				transition. Every completed scope level keeps its stock view. */
				blend = PIN((native_tangent - scoped_tangent) /
					(unzoomed_tangent - scoped_tangent), 0.0f, 1.0f);
			}
		}
		if (blend == 0.0f)
			return native_vertical_field_of_view;
		adjusted_tangent = native_tangent + (requested_tangent - unzoomed_tangent) * blend;
	}
	return 2.0f * atanf(adjusted_tangent);
}
