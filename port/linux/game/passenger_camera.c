/*
PASSENGER_CAMERA.C

An optional local view for the Warthog's passenger and the Scorpion's four
passengers. The reference seat supplies the camera track and pivot only;
the passenger keeps their own aim and its limits. No seat tags are changed,
and the deterministic camera used for shooting keeps its original view.
*/

#include "cseries.h"
#include "passenger_camera.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "tag_files/tag_groups.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "units/vehicles.h"

#include <string.h>

int config_boolean(const char *name);

static boolean reticle_active;
static real_point2d reticle_offset;

/* Match stock vehicle tag paths, including the PC multiplayer Warthog.
Seat roles identify the passengers without relying on their array order. */
static struct unit_seat const *reference_seat(short local_player_index, long *vehicle_index)
{
	long unit_index, player_index;
	struct unit_datum *unit, *vehicle;
	struct unit_definition const *definition;
	struct unit_seat const *seat;
	char const *name;
	short reference_role, index;

	/* (not in multiplayer, where seeing around the vehicle is an advantage the
	other players do not have) */
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		!config_boolean("display.passenger_third_person") || game_engine_running() || cinematic_in_progress() ||
		(director_camera_scripted && *director_camera_scripted))
		return NULL;
	player_index = local_player_get_player_index(local_player_index);
	if (player_index == NONE)
		return NULL;
	unit_index = player_control_get_unit_index(local_player_index);
	if (unit_index == NONE || player_get(player_index)->unit_index != unit_index)
		return NULL;
	unit = unit_try_and_get(unit_index);
	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit) ||
		unit->object.parent_object_index == NONE || unit->unit.parent_seat_index < 0)
		return NULL;
	vehicle = vehicle_try_and_get(unit->object.parent_object_index);
	if (!vehicle || TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit))
		return NULL;
	name = tag_get_name(vehicle->definition_index);
	if (!name)
		return NULL;
	if (!strcmp(name, "vehicles\\warthog\\warthog") ||
		!strcmp(name, "vehicles\\warthog\\mp_warthog"))
		reference_role = _unit_seat_gunner_bit;
	else if (!strcmp(name, "vehicles\\scorpion\\scorpion") ||
		!strcmp(name, "vehicles\\scorpion\\scorpion_mp"))
		reference_role = _unit_seat_driver_bit;
	else
		return NULL;
	definition = unit_definition_get(vehicle->definition_index);
	if (unit->unit.parent_seat_index >= definition->unit.seats.count)
		return NULL;
	seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, unit->unit.parent_seat_index, struct unit_seat);
	if (seat->flags & (FLAG(_unit_seat_driver_bit) | FLAG(_unit_seat_gunner_bit) |
		FLAG(_unit_seat_invisible_bit)))
		return NULL;
	for (index = 0; index < definition->unit.seats.count; index++)
	{
		seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, index, struct unit_seat);
		if (TEST_FLAG(seat->flags, reference_role))
		{
			*vehicle_index = unit->object.parent_object_index;
			return seat;
		}
	}
	return NULL;
}

boolean passenger_camera_enabled(short local_player_index)
{
	long vehicle_index;

	return reference_seat(local_player_index, &vehicle_index) != NULL;
}

boolean passenger_camera_info(short local_player_index, struct player_control_unit_camera_info *info)
{
	long vehicle_index;
	struct unit_seat const *seat = reference_seat(local_player_index, &vehicle_index);

	if (!seat)
		return FALSE;
	info->camera = &seat->camera;
	info->position = unit_get(vehicle_index)->object.position;
	if (seat->camera.marker_name[0])
	{
		struct object_marker marker;

		object_get_marker_by_name(vehicle_index, seat->camera.marker_name, &marker, 1);
		info->position = marker.matrix.position;
	}
	return TRUE;
}

void passenger_camera_clamp_facing(short local_player_index, real_euler_angles2d *facing)
{
	real_vector3d direction;

	if (!passenger_camera_enabled(local_player_index))
		return;
	/* The ordinary input path already applies the passenger seat's yaw and
	pitch limits. Apply its weapon animation's narrower aiming bounds too. */
	vector3d_from_euler_angles2d(&direction, facing);
	if (unit_clip_to_aiming_bounds(player_control_get_unit_index(local_player_index), &direction, TRUE))
	{
		euler_angles2d_from_vector3d(facing, &direction);
		if (facing->yaw < 0.0f)
			facing->yaw += 2.0f * _pi;
	}
}

boolean passenger_camera_reticle_begin(short local_player_index, long weapon_index)
{
	long unit_index;
	real_point3d origin, target;
	real_vector3d direction, ray;
	real_point2d screen;
	struct collision_result collision;

	reticle_active = FALSE;
	if (!passenger_camera_enabled(local_player_index) ||
		director_get_perspective(local_player_index) != _director_perspective_third_person)
		return TRUE;
	unit_index = player_control_get_unit_index(local_player_index);
	if (weapon_index == NONE || !weapon_get_projectile_ray(weapon_index, 0, &origin, &direction))
		return FALSE;
	/* The same finite ray as the shot's aiming reference, before random
	spread or a projectile's flight. Its origin remains at the weapon. */
	scale_vector3d(&direction, 128.0f, &ray);
	if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &ray, unit_index, &collision))
		target = collision.point;
	else
		point_from_line3d(&origin, &direction, 128.0f, &target);
	if (!render_camera_world_to_screen(&render.camera, &render.frustum, &target, &screen))
		return FALSE;
	reticle_offset.x = screen.x - (render.camera.window_bounds.x0 + render.camera.window_bounds.x1) * 0.5f;
	reticle_offset.y = screen.y - (render.camera.window_bounds.y0 + render.camera.window_bounds.y1) * 0.5f;
	reticle_active = TRUE;
	return TRUE;
}

void passenger_camera_reticle_offset(point2d *point)
{
	if (reticle_active)
	{
		point->x = (short)(point->x + reticle_offset.x);
		point->y = (short)(point->y + reticle_offset.y);
	}
}

void passenger_camera_reticle_end(void)
{
	reticle_active = FALSE;
}
