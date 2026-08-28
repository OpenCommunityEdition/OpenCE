/*
ITEMS.C

symbols in this file:
000E60A0 0020:
	_object_get_type (0000)
000E60C0 0010:
	_items_initialize (0000)
000E60D0 0010:
	_items_initialize_for_new_map (0000)
000E60E0 0010:
	_items_dispose_from_old_map (0000)
000E60F0 0010:
	_items_dispose (0000)
000E6100 0040:
	_item_new (0000)
000E6140 0010:
	_item_delete (0000)
000E6150 0010:
	_code_000e6150 (0000)
000E6160 0050:
	_dangerous_items_near_player (0000)
000E61B0 00a0:
	_item_in_unit_inventory (0000)
000E6250 0090:
	_item_get_position_even_if_in_inventory (0000)
000E62E0 0090:
	_item_detonate (0000)
000E6370 00c0:
	_code_000e6370 (0000)
000E6430 00c0:
	_valid_real_vector3d_axes3 (0000)
000E64F0 0060:
	_valid_real_matrix4x3 (0000)
000E6550 03b0:
	_item_accelerate (0000)
000E6900 0230:
	_code_000e6900 (0000)
000E6B30 0930:
	_item_update (0000)
00278FB0 0004:
	_rdata_00278fb0 (0000)
00278FB4 000c:
	??_C@_0M@HIOGOKBN@item_update?$AA@ (0000)
00278FC0 000d:
	??_C@_0N@MIPPJENF@ground?5point?$AA@ (0000)
00278FD0 001d:
	??_C@_0BN@FFDGPJIN@c?3?2halo?2SOURCE?2items?2items?4c?$AA@ (0000)
00278FF0 002b:
	??_C@_0CL@EEILLIKO@valid_real_matrix4x3?$CI?$CGground_poi@ (0000)
0027901C 0008:
	??_C@_07GAHNFPHG@scale?$DO0?$AA@ (0000)
00279024 0004:
	__real@3f350481 (0000)
00306498 05f8:
	_data_00306498 (0000)
*/

/* ---------- headers */

#define valid_real_matrix4x3 items_valid_real_matrix4x3_inline
#define valid_real_vector3d_axes3 items_valid_real_vector3d_axes3_inline
#define object_get_type items_object_get_type_inline
#include "cseries.h"

#include "game/game.h"
#include "effects/effects.h"
#include "game/players.h"
#include "game/game_engine.h"
#include "items/item_definitions.h"
#include "items.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_usage.h"
#include "scenario/scenario.h"
#include "units/units.h"
#undef object_get_type
#undef valid_real_vector3d_axes3
#undef valid_real_matrix4x3

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

short object_get_type(
	long object_index)
{
	return object_header_get(object_index)->type;
}

void items_initialize(
	void)
{
	return;
}

void items_initialize_for_new_map(
	void)
{
	return;
}

void items_dispose_from_old_map(
	void)
{
	return;
}

void items_dispose(
	void)
{
	return;
}

void item_delete(
	long item_index)
{
	return;
}

void code_000e6150(
	void)
{
	return;
}

boolean item_new(
	long item_index)
{
	struct item_datum *item = item_get(item_index);

	item->object.flags |= FLAG(_object_dynamic_lighting_recompute_bit) |
		FLAG(_object_static_lighting_recompute_bit);
	item->item.last_owned_time = game_time_get();
	item->item.ignore_object_index = NONE;

	return TRUE;
}

boolean dangerous_items_near_player(
	void)
{
	struct object_iterator iterator;
	struct item_datum *item;

	object_iterator_new(&iterator, _object_mask_item, 1);
	while (item = (struct item_datum *)object_iterator_next(&iterator))
	{
		if (item->item.detonation_ticks > 0)
			return TRUE;
	}

	return FALSE;
}

void item_in_unit_inventory(
	long item_index,
	long owner_unit_index)
{
	struct item_datum *item = item_get(item_index);

	if (owner_unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(owner_unit_index);

		item->item.flags |= FLAG(_item_attached_to_unit_bit);
		if (unit->unit.player_index != NONE)
		{
			item->item.flags |= FLAG(_item_belongs_to_player_bit);
		}
		else
		{
			item->item.flags &= ~FLAG(_item_belongs_to_player_bit);
		}

		item->object.owner_player_index = unit->unit.player_index;
		object_set_garbage(item_index, FALSE);
		item->item.flags &=
			~(FLAG(_item_on_structure_bit) | FLAG(_item_does_not_accelerate_bit));
		item->object.location.leaf_index = NONE;
		item->object.location.cluster_index = NONE;
		scenario_location_award_bonus(&item->object.location);

		return;
	}

	item->item.flags &=
		~(FLAG(_item_attached_to_unit_bit) | FLAG(_item_belongs_to_player_bit));

	return;
}

void item_get_position_even_if_in_inventory(
	long item_index,
	real_point3d *position)
{
	struct item_datum *item = item_try_and_get(item_index);

	position->x = 0.f;
	position->y = 0.f;
	position->z = 0.f;

	if (!item)
	{
		return;
	}

	if (TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
	{
		long player_index = item->object.owner_player_index;
		struct player_datum *player;
		struct unit_datum *unit;

		if (player_index == NONE)
		{
			return;
		}

		player = player_get(player_index);
		if (player->unit_index == NONE)
		{
			return;
		}

		unit = unit_get(player->unit_index);
		*position = unit->object.bounding_sphere_center;

		return;
	}

	*position = item->object.bounding_sphere_center;

	return;
}

void item_detonate(
	long item_index)
{
	struct item_datum *item = item_get(item_index);
	struct item_definition *definition = item_definition_get(item->definition_index);

	if (!item->item.detonation_ticks)
	{
		effect_new_from_object(
			definition->item.detonating_effect.index,
			item_index,
			item_index,
			NONE,
			0.f,
			0.f,
			NULL,
			NULL);
		item->item.detonation_ticks =
			(short)(real_random_range(
				definition->item.detonation_delay_timer_lower_bound,
				definition->item.detonation_delay_timer_upper_bound) * TICKS_PER_SECOND);
	}

	return;
}

/* ---------- private code */

static void code_000e6370(
	long item_index)
{
	struct item_datum *item = item_get(item_index);
	real angular_velocity_magnitude = magnitude3d(&item->object.angular_velocity);

	if (angular_velocity_magnitude != 0.f)
	{
		item->item.flags |= FLAG(_item_has_nonzero_angular_velocity_bit);
		if (!TEST_FLAG(item->object.flags, _object_at_rest_bit))
		{
			scale_vector3d(
				&item->object.angular_velocity,
				1.f / angular_velocity_magnitude,
				&item->item.rotation_axis);
		}

		item->item.rotation_sine = sine(angular_velocity_magnitude);
		item->item.rotation_cosine = cosine(angular_velocity_magnitude);
	}
	else
	{
		item->item.flags &= ~FLAG(_item_has_nonzero_angular_velocity_bit);
		item->item.rotation_sine = 0.f;
		item->item.rotation_cosine = 1.f;
	}

	return;
}

boolean valid_real_vector3d_axes3(
	real_vector3d const *forward,
	real_vector3d const *left,
	real_vector3d const *up)
{
	return
		valid_real_normal3d(forward) &&
		valid_real_normal3d(left) &&
		valid_real_normal3d(up) &&
		valid_realcmp(dot_product3d(forward, left), 0.f) &&
		valid_realcmp(dot_product3d(left, up), 0.f) &&
		valid_realcmp(dot_product3d(up, forward), 0.f);
}

boolean valid_real_matrix4x3(
	real_matrix4x3 const *matrix)
{
	return
		valid_real(matrix->scale) &&
		valid_real_vector3d_axes3(&matrix->forward, &matrix->left, &matrix->up) &&
		valid_real_point3d(&matrix->position);
}

void item_accelerate(
	long item_index,
	real_vector3d const *acceleration,
	boolean detonates_explosives)
{
	struct item_datum *item = item_get(item_index);
	struct item_definition *definition = item_definition_get(item->definition_index);

	if (TEST_FLAG(item->item.flags, _item_does_not_accelerate_bit))
	{
		return;
	}

	match_assert(
		"c:\\halo\\SOURCE\\items\\items.c",
		536,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_items;

	if (item->object.parent_object_index == NONE)
	{
	if (detonates_explosives &&
		!game_engine_running() &&
		TEST_FLAG(definition->item.flags, 1))
	{
		item_detonate(item_index);
	}

	if (TEST_FLAG(item->item.flags, _item_on_structure_bit))
	{
		if (!(magnitude_squared3d(acceleration) < 0.0001f))
		{
			struct object_marker marker;

			if (object_get_marker_by_name(item_index, "ground point", &marker, 1))
			{
				struct collision_bsp *collision_bsp;
				struct collision_surface const *surface;
				real_point3d new_position;
				real_plane3d plane;
				real distance_above;

				collision_bsp = global_collision_bsp_get();
				surface = TAG_BLOCK_GET_ELEMENT(
					&collision_bsp->surfaces,
					item->item.rested_surface_index,
					struct collision_surface);
				bsp3d_get_plane_from_designator(
					&collision_bsp->bsp3d,
					surface->plane_designator,
					&plane);

				distance_above =
					0.05f - (plane3d_distance_to_point(&plane, &marker.matrix.position));
				point_from_line3d(
					&marker.matrix.position,
					&plane.n,
					distance_above,
					&new_position);
				object_translate(item_index, &new_position, NULL);
			}

			item->object.flags &= ~FLAG(_object_at_rest_bit);
			item->item.flags &= ~FLAG(_item_on_structure_bit);
		}
	}
	else
	{
		item->object.flags &= ~FLAG(_object_at_rest_bit);
	}

	add_vectors3d(
		&item->object.translational_velocity,
		acceleration,
		&item->object.translational_velocity);

	if (item->item.ignore_object_index == NONE &&
		TEST_FLAG(item->item.flags, _item_on_structure_bit) &&
		magnitude_squared3d(acceleration) < 0.0001f)
	{
		struct object_marker marker;
		real_vector3d rotation_axis;
		real_vector3d scaled_rotation_axis;
		real rotation_magnitude;

		if (object_get_marker_by_name(item_index, "ground point", &marker, 1))
		{
			rotation_axis = marker.matrix.up;
		}
		else
		{
			rotation_axis = *global_up3d;
		}

		rotation_magnitude = real_random_range(-1.5707964f, 1.5707964f);
		scale_vector3d(
			&rotation_axis,
			rotation_magnitude,
			&scaled_rotation_axis);
		add_vectors3d(
			&item->object.angular_velocity,
			&scaled_rotation_axis,
			&item->object.angular_velocity);
	}
	else
	{
		real acceleration_magnitude = magnitude3d(acceleration);
		real_vector3d rotation_axis;
		real rotation_magnitude;

		if (acceleration_magnitude < 0.0001f)
		{
			acceleration_magnitude =
				real_seed_random(get_global_random_seed_address());
		}

		cross_product3d(global_up3d, acceleration, &rotation_axis);
		if (!(normalize3d(&rotation_axis) > 0.f))
		{
			seed_random_direction3d(
				get_global_random_seed_address(),
				&rotation_axis);
		}

		rotation_magnitude =
			real_seed_random(get_global_random_seed_address()) *
			acceleration_magnitude * 1.5707964f;
		scale_vector3d(&rotation_axis, rotation_magnitude, &rotation_axis);
		add_vectors3d(
			&item->object.angular_velocity,
			&rotation_axis,
			&item->object.angular_velocity);
	}

	code_000e6370(item_index);
	object_set_garbage(item_index, FALSE);
	}

	match_assert(
		"c:\\halo\\SOURCE\\items\\items.c",
		651,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return;
}
