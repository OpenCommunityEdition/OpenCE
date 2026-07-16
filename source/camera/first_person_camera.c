/*
FIRST_PERSON_CAMERA.C

symbols in this file:
000772B0 0040:
	_first_person_camera_new (0000)
000772F0 00d0:
	_first_person_camera_deterministic (0000)
000773C0 04f0:
	_code_000773c0 (0000)
000778B0 0030:
	_first_person_camera_fake (0000)
000778E0 00c0:
	_first_person_camera_update (0000)
00256DBC 002c:
	??_C@_0CM@BADBGAHA@c?3?2halo?2SOURCE?2camera?2first_pers@ (0000)
00256DE8 0010:
	??_C@_0BA@DGFIDHMB@primary?5trigger?$AA@ (0000)
00256DF8 0039:
	??_C@_0DJ@GGFKGPIO@valid_real_vector3d_axes2?$CI?$CGresul@ (0000)
*/

/* ---------- headers */

#include "first_person_camera.h"

#include "objects/objects.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void first_person_camera_new(
	struct first_person_camera *camera)
{
	match_assert("c:\\halo\\SOURCE\\camera\\first_person_camera.c", 24, camera);
	camera->field_of_view = 0.f;
	return;
}

void first_person_camera_deterministic(
	long unit_index,
	real_point3d *position,
	real_vector3d *forward)
{
	struct unit_datum *unit;

	unit = unit_get(unit_index);
	unit_get_camera_position(unit_index, position);
	*forward = unit->unit.aiming_vector;

	if (unit->object.parent_object_index != NONE)
	{
		struct unit_datum *vehicle;

		vehicle = vehicle_try_and_get(unit->object.parent_object_index);
		if (vehicle)
		{
			struct unit_definition *definition;
			struct unit_seat *seat;

			definition = vehicle_definition_get(vehicle->definition_index);
			seat = TAG_BLOCK_GET_ELEMENT(
				&definition->unit.seats,
				unit->unit.parent_seat_index,
				struct unit_seat);

			if (TEST_FLAG(seat->flags, _unit_seat_first_person_camera_bit))
			{
				struct object_marker marker;

				if (object_get_marker_by_name(
					unit->object.parent_object_index,
					"primary trigger",
					&marker,
					1))
				{
					*position = marker.matrix.position;
					*forward = marker.matrix.forward;
				}
			}
		}
	}

	return;
}

/* ---------- private code */
