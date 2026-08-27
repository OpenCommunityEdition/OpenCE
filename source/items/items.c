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
#include "cseries.h"

#include "game/game.h"
#include "effects/effects.h"
#include "game/players.h"
#include "items/item_definitions.h"
#include "items.h"
#include "scenario/scenario.h"
#include "units/units.h"
#undef valid_real_vector3d_axes3
#undef valid_real_matrix4x3

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

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
