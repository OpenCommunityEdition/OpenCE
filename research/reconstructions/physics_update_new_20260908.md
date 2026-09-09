# `physics_update_new` research reconstruction

Date: 2026-09-08

Status: **non-production research artifact; not compiled; zero exact credit**.

This is the complete best-current typed reconstruction preserved verbatim after
the active-source version was withdrawn. January disassembly and independent
HCEA behavior support its control flow, but the authenticated final
stuck-mass-point bitmask write belongs to a vehicle datum field that the current
vehicle owner does not expose. No vehicle file was changed and no raw-offset
overlay was accepted.

The candidate also required publishing `matrix3x3_multiply` through
`math/real_math.h`. The 473-unit full-tree sweep showed that declaration
regressed the already accepted exact `_rasterizer_dynamic_geometry_initialize`,
while also producing one accidental bitmap exact and three stale parks. The
zero-regression policy therefore withdrew that declaration, the update-only
vehicle include, the debug global, the active function body, and its park entry.

Historical measurement of the preserved candidate:

| Side | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January | 1,976 | 1,984 | 66 | `8cc42cd03258ede247eca9dbe551ae2981bfb1f8573502d9932d747c6445b049` |
| Candidate | 1,921 | 1,936 | 65 | `6f62f8642db6d076287389ac4b87e285466a2b5d6a480482f22d87859ed979a7` |

Historical objdiff: **82.58637%**. These figures are research evidence only and
must not be included in canonical exact or parked totals.

```c
/*
Best-current reconstruction: the vehicle owner does not yet expose the
stuck-mass-point flags written by the January function.  This body therefore
receives no exact-match credit until that owner field can be represented.
*/
void physics_update_new(
	struct physics_instance const *instance,
	struct powered_mass_point_datum const *powered_mass_points,
	struct mass_point_datum const *mass_points,
	real_vector3d const *total_force,
	real_vector3d const *total_torque)
{
	struct unit_datum *vehicle = vehicle_get(instance->object_index);
	struct physics_definition const *physics = instance->physics;
	real inverse_mass;
	real_vector3d linear_acceleration;
	real_vector3d linear_velocity;
	real_point3d position;
	real_matrix3x3 frame;
	real_matrix3x3 world_inverse_inertia;
	real_vector3d angular_acceleration;
	real_vector3d angular_velocity;
	real_vector3d forward;
	real_vector3d up;
	short mass_point_index;

	match_assert(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		986,
		instance->physics->mass>0.0f);

	inverse_mass = 1.0f/physics->mass;
	linear_acceleration.i = total_force->i*inverse_mass;
	linear_acceleration.j = total_force->j*inverse_mass;
	linear_acceleration.k = total_force->k*inverse_mass;
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		990,
		&linear_acceleration);

	linear_velocity.i = vehicle->object.translational_velocity.i + linear_acceleration.i;
	linear_velocity.j = vehicle->object.translational_velocity.j + linear_acceleration.j;
	linear_velocity.k = vehicle->object.translational_velocity.k + linear_acceleration.k;
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		994,
		&linear_velocity);

	position.x = vehicle->object.position.x + linear_velocity.i;
	position.y = vehicle->object.position.y + linear_velocity.j;
	position.z = vehicle->object.position.z + linear_velocity.k;

	matrix3x3_from_forward_and_up(
		&frame,
		&vehicle->object.forward,
		&vehicle->object.up);
	matrix3x3_multiply(
		&frame,
		TAG_BLOCK_GET_ELEMENT(
			&physics->inertial_matrix,
			1,
			real_matrix3x3),
		&world_inverse_inertia);
	matrix3x3_multiply(
		&world_inverse_inertia,
		matrix3x3_transpose(&frame, &frame),
		&world_inverse_inertia);
	matrix3x3_transform_vector(
		&world_inverse_inertia,
		total_torque,
		&angular_acceleration);
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		1008,
		&angular_acceleration);

	angular_velocity.i = vehicle->object.angular_velocity.i + angular_acceleration.i;
	angular_velocity.j = vehicle->object.angular_velocity.j + angular_acceleration.j;
	angular_velocity.k = vehicle->object.angular_velocity.k + angular_acceleration.k;
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		1012,
		&angular_velocity);

	rotate_vectors3d_by_angular_velocity(
		&vehicle->object.forward,
		&vehicle->object.up,
		&angular_velocity,
		&forward,
		&up);

	vehicle->object.translational_velocity = linear_velocity;
	vehicle->object.angular_velocity = angular_velocity;

	if (debug_physics_disable_penetration_freeze)
	{
		object_set_position(instance->object_index, &position, &forward, &up);
	}
	else
	{
		short passes_remaining = 4;

		while (passes_remaining-- > 0)
		{
			boolean found_collision = FALSE;
			real_vector3d worst_delta;
			struct collision_result worst_collision;
			real_matrix4x3 world_matrix;
			real_point3d center_of_mass;

			matrix4x3_from_point_and_vectors(&world_matrix, &position, &forward, &up);
			center_of_mass.x = -physics->center_of_mass.x;
			center_of_mass.y = -physics->center_of_mass.y;
			center_of_mass.z = -physics->center_of_mass.z;
			matrix4x3_transform_point(&world_matrix, &center_of_mass, &center_of_mass);
			world_matrix.position = center_of_mass;

			for (mass_point_index = 0;
				mass_point_index < physics->mass_points.count;
				mass_point_index++)
			{
				struct mass_point_definition const *mass_point_definition = TAG_BLOCK_GET_ELEMENT(
					&physics->mass_points,
					mass_point_index,
					struct mass_point_definition);
				struct mass_point_datum const *mass_point = mass_points + mass_point_index;
				real_point3d swept_position;
				real_vector3d delta;
				struct collision_result collision;

				matrix4x3_transform_point(
					&world_matrix,
					&mass_point_definition->position,
					&swept_position);
				delta.i = swept_position.x - mass_point->position.x;
				delta.j = swept_position.y - mass_point->position.y;
				delta.k = swept_position.z - mass_point->position.z;

				if (collision_test_vector(
						_collision_test_for_vehicles_flags |
							FLAG(_collision_test_front_facing_surfaces_bit),
						&mass_point->position,
						&delta,
						instance->object_index,
						&collision))
				{
					if (!found_collision || collision.t < worst_collision.t)
					{
						found_collision = TRUE;
						worst_delta = delta;
						worst_collision = collision;
					}
				}
			}

			if (!found_collision)
			{
				object_set_position(instance->object_index, &position, &forward, &up);
				break;
			}
			else
			{
				real normal_dot_delta = dot_product3d(&worst_collision.plane.n, &worst_delta);
				real epsilon = normal_dot_delta == 0.0f ?
					0.03125f : (real)(0.0078125/fabs(normal_dot_delta));
				real t = MAX(worst_collision.t - epsilon, 0.0f);
				real normal_dot_velocity = dot_product3d(&worst_collision.plane.n, &linear_velocity);

				if (normal_dot_velocity < 0.0f)
				{
					real scale = (t - 1.0f)*normal_dot_velocity;

					linear_velocity.i += worst_collision.plane.n.i*scale;
					linear_velocity.j += worst_collision.plane.n.j*scale;
					linear_velocity.k += worst_collision.plane.n.k*scale;
					vehicle->object.translational_velocity = linear_velocity;
					position.x = vehicle->object.position.x + linear_velocity.i;
					position.y = vehicle->object.position.y + linear_velocity.j;
					position.z = vehicle->object.position.z + linear_velocity.k;
				}

				angular_velocity.i *= t;
				angular_velocity.j *= t;
				angular_velocity.k *= t;
				vehicle->object.angular_velocity = angular_velocity;
				rotate_vectors3d_by_angular_velocity(
					&vehicle->object.forward,
					&vehicle->object.up,
					&angular_velocity,
					&forward,
					&up);
			}
		}

		/* The vehicle-owned stuck-mass-point field is not exposed by vehicles.h. */
	}

	{
		short at_rest_count = 0;
		short on_ground_count = 0;
		short on_volatile_surface_count = 0;
		short in_water_count = 0;

		for (mass_point_index = 0;
			mass_point_index < physics->mass_points.count;
			mass_point_index++)
		{
			unsigned long flags = mass_points[mass_point_index].flags;

			at_rest_count += TEST_FLAG(flags, _point_at_rest_bit);
			on_ground_count += TEST_FLAG(flags, _point_on_ground_bit);
			on_volatile_surface_count += TEST_FLAG(flags, _point_on_volatile_surface_bit);
			in_water_count += TEST_FLAG(flags, _point_in_water_bit);
		}

		SET_FLAG(
			vehicle->object.flags,
			_object_at_rest_bit,
			at_rest_count == physics->mass_points.count &&
			on_ground_count >= 3 &&
			on_volatile_surface_count == 0 &&
			magnitude_squared3d(&linear_velocity) <= 0.0011111111f &&
			magnitude_squared3d(&angular_velocity) <= 0.0027415568f &&
			magnitude_squared3d(&linear_acceleration) <= 0.00000030864197f &&
			magnitude_squared3d(&angular_acceleration) <= 0.0000030461742f);
		SET_FLAG(vehicle->object.flags, _object_on_ground_bit, on_ground_count > 0);
		SET_FLAG(vehicle->object.flags, _object_on_media_bit, in_water_count > 0);
		SET_FLAG(vehicle->object.flags, _object_partially_under_media_bit, in_water_count > 0);
		SET_FLAG(
			vehicle->object.flags,
			_object_wholly_under_media_bit,
			in_water_count == physics->mass_points.count);
	}

	return;
}
```
