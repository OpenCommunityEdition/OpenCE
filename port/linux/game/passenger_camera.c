/*
PASSENGER_CAMERA.C

Optional third person for local Warthog and Scorpion passengers. Seat tags
and the deterministic camera used by networking keep their stock values.
The observer looks along the passenger's aiming vector; firing projects
the origin onto that rendered camera's line without changing the direction.
Weapon zoom is disabled while the passenger view is enabled.
*/

#include "passenger_camera.h"
#include "camera/director.h"
#include "camera/observer.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "units/vehicles.h"

#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

static boolean passenger_camera_enabled(void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;
	unsigned long current_changes = config_changes();

	if (changes != current_changes)
	{
		changes = current_changes;
		enabled = config_boolean("display.passenger_third_person") != 0;
	}
	return enabled;
}

/* Shared seat eligibility; the caller supplies or finds the local player. */
static struct unit_datum *passenger_camera_unit(long unit_index, boolean *warthog)
{
	struct unit_datum *unit, *vehicle;
	struct unit_definition *definition;
	struct unit_seat *seat;
	const char *name;
	boolean is_warthog;

	if (!passenger_camera_enabled() || unit_index == NONE)
		return NULL;
	unit = unit_try_and_get(unit_index);
	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit) ||
		unit->object.parent_object_index == NONE ||
		unit->unit.animation.state == _unit_state_exiting_seat)
		return NULL;
	vehicle = vehicle_try_and_get(unit->object.parent_object_index);
	if (!vehicle)
		return NULL;
	definition = unit_definition_get(vehicle->definition_index);
	if (unit->unit.parent_seat_index < 0 || unit->unit.parent_seat_index >= definition->unit.seats.count)
		return NULL;
	seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, unit->unit.parent_seat_index, struct unit_seat);
	if (seat->flags & (FLAG(_unit_seat_driver_bit) | FLAG(_unit_seat_gunner_bit)))
		return NULL;
	name = tag_get_name(vehicle->definition_index);
	if (!name)
		return NULL;
	is_warthog = !strncmp(name, "vehicles\\warthog\\", sizeof("vehicles\\warthog\\") - 1) ||
		!strncmp(name, "vehicles\\rwarthog\\", sizeof("vehicles\\rwarthog\\") - 1);
	if (!is_warthog && strncmp(name, "vehicles\\scorpion\\", sizeof("vehicles\\scorpion\\") - 1))
		return NULL;
	if (warthog)
		*warthog = is_warthog;
	return unit;
}

static struct unit_datum *passenger_camera_player_unit(short local_player_index, boolean *warthog)
{
	if (local_player_index < 0 || local_player_index >= MAXIMUM_NUMBER_OF_LOCAL_PLAYERS ||
		local_player_get_player_index(local_player_index) == NONE)
		return NULL;
	return passenger_camera_unit(player_control_get_unit_index(local_player_index), warthog);
}

/* Entry already uses third person, but aim alignment starts once seated. */
static boolean passenger_camera_aiming_allowed(short local_player_index, struct unit_datum const *unit)
{
	return unit && unit->unit.animation.state != _unit_state_entering_seat &&
		!cinematic_in_progress() && !(director_camera_scripted && *director_camera_scripted) &&
		!director_inhibited_facing(local_player_index) &&
		director_get_perspective(local_player_index) == _director_perspective_third_person;
}

short passenger_camera_local_player(long unit_index)
{
	short local_player_index;

	if (!passenger_camera_unit(unit_index, NULL))
		return NONE;

	for (local_player_index = local_player_get_next(NONE); local_player_index != NONE;
		local_player_index = local_player_get_next(local_player_index))
	{
		if (player_control_get_unit_index(local_player_index) == unit_index)
			return local_player_index;
	}
	return NONE;
}

boolean passenger_camera_aim(short local_player_index, real_vector3d *forward)
{
	struct unit_datum *unit = passenger_camera_player_unit(local_player_index, NULL);

	if (!passenger_camera_aiming_allowed(local_player_index, unit))
		return FALSE;
	*forward = unit->unit.aiming_vector;
	return TRUE;
}

boolean passenger_camera_position(long unit_index, real_point3d *position)
{
	short local_player_index = passenger_camera_local_player(unit_index);

	if (local_player_index == NONE || !passenger_camera_aiming_allowed(local_player_index, unit_get(unit_index)))
		return FALSE;
	*position = observer_get_camera(local_player_index)->position;
	return TRUE;
}

boolean passenger_camera_blocks_zoom(short local_player_index)
{
	return passenger_camera_player_unit(local_player_index, NULL) != NULL;
}

/* Keep the Warthog passenger's input at the pitch its aiming animation can
reach. Otherwise the input keeps moving past the visible camera's stop and
has to come back through that excess before reversing the view. The bounds
are relative to the passenger's body, including the vehicle's tilt. */
void passenger_camera_clip_pitch(short local_player_index, real_euler_angles2d *angles)
{
	struct unit_datum *unit;
	real_matrix4x3 matrix;
	real_vector3d forward, relative;
	real_euler_angles2d relative_angles;
	real pitch;
	boolean warthog = FALSE;

	unit = passenger_camera_player_unit(local_player_index, &warthog);
	if (!passenger_camera_aiming_allowed(local_player_index, unit) || !warthog ||
		!unit->unit.animation.aiming_with_euler_screen)
		return;

	matrix.scale = 1.f;
	object_get_orientation(player_control_get_unit_index(local_player_index), &matrix.forward, &matrix.up);
	cross_product3d(&matrix.up, &matrix.forward, &matrix.left);
	matrix.position = *global_origin3d;
	vector3d_from_euler_angles2d(&forward, angles);
	matrix4x3_inverse_transform_normal(&matrix, &forward, &relative);
	euler_angles2d_from_vector3d(&relative_angles, &relative);
	pitch = PIN(relative_angles.pitch,
		unit->unit.animation.aiming_screen_bounds.y0,
		unit->unit.animation.aiming_screen_bounds.y1);
	if (pitch == relative_angles.pitch)
		return;
	relative_angles.pitch = pitch;
	vector3d_from_euler_angles2d(&relative, &relative_angles);
	matrix4x3_transform_normal(&matrix, &relative, &forward);
	normalize3d(&forward);
	euler_angles2d_from_vector3d(angles, &forward);
	if (angles->yaw < 0.f)
		angles->yaw += _pi * 2.f;
}
