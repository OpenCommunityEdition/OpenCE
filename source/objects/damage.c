/*
DAMAGE.C

symbols in this file:
00125DD0 0010:
	_damage_initialize (0000)
00125DE0 0010:
	_damage_dispose (0000)
00125DF0 0010:
	_damage_initialize_for_new_map (0000)
00125E00 0010:
	_damage_dispose_from_old_map (0000)
00125E10 0010:
	_damage_render_debug (0000)
00125E20 00e0:
	_object_initialize_vitality (0000)
00125F00 0050:
	_object_get_maximum_body_vitality (0000)
00125F50 0050:
	_object_get_maximum_shield_vitality (0000)
00125FA0 0040:
	_damage_data_new (0000)
00125FE0 0050:
	_object_restore_body (0000)
00126030 0060:
	_object_double_charge_shield (0000)
00126090 0050:
	_code_00126090 (0000)
001260E0 0050:
	_get_player_index_from_object_or_parents (0000)
00126130 0050:
	_object_can_take_damage (0000)
00126180 0050:
	_object_cannot_take_damage (0000)
001261D0 0030:
	_object_set_ranged_attack_inhibited (0000)
00126200 0030:
	_object_set_melee_attack_inhibited (0000)
00126230 0020:
	_damage_effect_new_on_object (0000)
00126250 0080:
	_object_permutation_shield_regions (0000)
001262D0 0060:
	_object_get_actual_body_vitality (0000)
00126330 0060:
	_object_get_actual_shield_vitality (0000)
00126390 0080:
	_object_deplete_shield (0000)
00126410 0380:
	_code_00126410 (0000)
00126790 0230:
	_code_00126790 (0000)
001269C0 0200:
	_code_001269c0 (0000)
00126BC0 01d0:
	_render_debug_object_damage (0000)
00126D90 00e0:
	_object_deplete_body (0000)
00126E70 0070:
	_object_destroy (0000)
00126EE0 0140:
	_code_00126ee0 (0000)
00127020 0550:
	_code_00127020 (0000)
00127570 07c0:
	_object_cause_damage (0000)
00127D30 0420:
	_object_damage_update (0000)
00128150 0530:
	_area_of_effect_cause_damage_to_object (0000)
00128680 0080:
	_area_of_effect_cause_damage (0000)
00289410 006f:
	??_C@_0GP@MMLJIPGP@damage_resistance?9?$DOshield_materi@ (0000)
00289480 0020:
	??_C@_0CA@CANDACNM@c?3?2halo?2SOURCE?2objects?2damage?4c?$AA@ (0000)
002894A0 0029:
	??_C@_0CJ@DEGDFOIJ@collision?4type?$DN?$DN_collision_resul@ (0000)
002894D0 0061:
	??_C@_0GB@CBCJEGGG@?$CFs?$HMnbody?5?$CF0?43f?$HMn?5?5current?5?$CF0?43f?$HM@ (0000)
00289534 002c:
	??_C@_0CM@MBIMGLAN@no?5object?5to?5debug?$HMn?$CIpoint?5and?5p@ (0000)
00289560 0009:
	??_C@_08KBLDDAKN@?$HOdamaged?$AA@ (0000)
0028956C 003b:
	??_C@_0DL@NODEPHOL@region_index?$DO?$DN0?5?$CG?$CG?5region_index?$DM@ (0000)
002895A8 004b:
	??_C@_0EL@KFMBAJDI@damage_material?9?$DOtype?$DO?$DN0?5?$CG?$CG?5dama@ (0000)
002895F4 0028:
	??_C@_0CI@HMLKKOKN@?$CFs?3?5?$CC?$CFs?$CC?5?$CC?$CFs?$CC?5k?$DN?$CF0?42f?5S?$FL?$CF3?42f?$FN?5B@ (0000)
00289620 0041:
	??_C@_0EB@BEIFMKJC@damaged_object_count?$DMsizeof?$CIdama@ (0000)
00289668 0053:
	??_C@_0FD@MEFHNJOJ@region_index?$DN?$DNNONE?5?$HM?$HM?5?$CIregion_in@ (0000)
002896BC 0004:
	__real@3a422e45 (0000)
00456E48 004c:
	_damage_globals (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "damage.h"
#include "effects/effects.h"
#include "game/cheats.h"
#include "game/game_allegiance.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "hs/object_lists.h"
#include "input/input.h"
#include "damage_effect_definitions.h"
#include "object_definitions.h"
#include "object_types.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_model_definitions.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "units/unit_definitions.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_object_region_missing_when_shield_is_zero_bit = 4,
};

enum
{
	_damage_effect_dont_scale_damage_by_distance_bit = 0,
};

enum
{
	_damage_does_not_hurt_owner_bit = 0,
	_damage_does_not_hurt_friends_bit = 3,
	_damage_does_not_hurt_infection_forms_bit = 10,
	_damage_infection_form_pop_bit = 12,
};

enum
{
	_damage_resistance_children_take_area_damage_bit = 3,
};

enum
{
	_damage_area_of_effect_collision_flags =
		FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_objects_vehicles_bit) |
		FLAG(_collision_test_objects_scenery_bit) |
		FLAG(_collision_test_objects_machines_bit),
};

/* ---------- macros */

/* ---------- structures */

struct damage_globals
{
	byte unknown[0x48];
	long debug_object_index;
};

struct damage_region
{
	char name[TAG_STRING_LENGTH+1];
	unsigned long flags;
	long unused0;
	real damage_threshold;
	long unused1[3];
	struct tag_reference destroyed_effect;
	struct tag_block permutations;
};

typedef char damage_region_size_assert[
	sizeof(struct damage_region) == 0x54 ? 1 : -1];

typedef char object_deplete_body_definition_index_offset_assert[
	offsetof(struct object_datum, definition_index) == 0x00 ? 1 : -1];
typedef char object_deplete_body_type_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, type) == 0x64 ? 1 : -1];
typedef char object_deplete_body_damage_flags_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, damage_flags) == 0xB6 ? 1 : -1];
typedef char object_deplete_body_next_object_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, next_object_index) == 0xC4 ? 1 : -1];
typedef char object_deplete_body_first_child_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, first_child_object_index) == 0xC8 ? 1 : -1];
typedef char object_deplete_body_player_index_offset_assert[
	offsetof(struct unit_datum, unit) + offsetof(struct _unit_datum, player_index) == 0x1C8 ? 1 : -1];
typedef char object_deplete_body_parent_seat_offset_assert[
	offsetof(struct unit_datum, unit) + offsetof(struct _unit_datum, parent_seat_index) == 0x2A0 ? 1 : -1];
typedef char object_deplete_body_collision_model_offset_assert[
	offsetof(struct object_definition, object) + offsetof(struct _object_definition, collision_model) + offsetof(struct tag_reference, index) == 0x7C ? 1 : -1];
typedef char object_deplete_body_effect_offset_assert[
	offsetof(struct collision_model, resistance) + offsetof(struct damage_resistance, body_depleted_effect) + offsetof(struct tag_reference, index) == 0xB4 ? 1 : -1];
typedef char object_destroy_effect_offset_assert[
	offsetof(struct collision_model, resistance) + offsetof(struct damage_resistance, body_destroyed_effect) + offsetof(struct tag_reference, index) == 0xC8 ? 1 : -1];

/* ---------- prototypes */

void render_debug_object_damage(
	void);

static long get_player_index_from_object_or_parents(
	long object_index);

static void object_permutation_shield_regions(
	long object_index,
	boolean active);
void area_of_effect_cause_damage_to_object(
	struct damage_data *damage,
	long object_index,
	boolean damage_next_object);

static void damage_effect_new_on_object(
	long effect_definition_index,
	long object_index);

/* ---------- globals */

extern boolean debug_damage;

struct damage_globals damage_globals;

/* ---------- public code */

void damage_initialize(void)
{
	return;
}

void damage_dispose(void)
{
	return;
}

void damage_initialize_for_new_map(void)
{
	damage_globals.debug_object_index = NONE;
	return;
}

void damage_dispose_from_old_map(void)
{
	return;
}

void damage_render_debug(void)
{
	return;
}

void object_initialize_vitality(
	long object_index,
	real *custom_body_vitality,
	real *custom_shield_vitality)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	real maximum_body_vitality = 0.f;
	real maximum_shield_vitality = 0.f;

	if (definition->object.collision_model.index != NONE)
	{
		struct collision_model *collision_model =
			collision_model_definition_get(definition->object.collision_model.index);

		if (collision_model)
		{
			maximum_body_vitality = collision_model->resistance.maximum_body_vitality;
			maximum_shield_vitality = collision_model->resistance.maximum_shield_vitality;
		}
	}

	if (custom_body_vitality)
		maximum_body_vitality = *custom_body_vitality;

	if (custom_shield_vitality)
		maximum_shield_vitality = *custom_shield_vitality;

	object->object.maximum_body_vitality = maximum_body_vitality;
	object->object.maximum_shield_vitality = maximum_shield_vitality;
	object->object.body_vitality = maximum_body_vitality > 0.f ? 1.f : 0.f;
	object->object.shield_vitality = maximum_shield_vitality > 0.f ? 1.f : 0.f;
	return;
}

real object_get_maximum_body_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	struct object_datum *object = object_get(object_index);
	real result = object->object.maximum_body_vitality;

	if (!ignore_difficulty)
		result *= game_difficulty_get_team_value(1, object->object.owner_team_index);

	return result;
}

real object_get_maximum_shield_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	struct object_datum *object = object_get(object_index);
	real result = object->object.maximum_shield_vitality;

	if (!ignore_difficulty)
		result *= game_difficulty_get_team_value(2, object->object.owner_team_index);

	return result;
}

real object_get_actual_body_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	real body_vitality = object_get(object_index)->object.body_vitality;
	real result = object_get_maximum_body_vitality(object_index, ignore_difficulty);

	result *= body_vitality;

	return result;
}

real object_get_actual_shield_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	real shield_vitality = object_get(object_index)->object.shield_vitality;
	real result = object_get_maximum_shield_vitality(object_index, ignore_difficulty);

	result *= shield_vitality;

	return result;
}

void damage_data_new(
	struct damage_data *damage_data,
	long definition_index)
{
	csmemset(damage_data, 0, sizeof(*damage_data));
	damage_data->definition_index = definition_index;
	damage_data->material_type = NONE;
	damage_data->owner_player_index = NONE;
	damage_data->owner_object_index = NONE;
	damage_data->owner_team_index = NONE;
	damage_data->location.cluster_index = NONE;
	damage_data->scale = 1.f;
	damage_data->multiplier = 1.f;
	return;
}

boolean object_restore_body(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	boolean restored = FALSE;

	if (!TEST_FLAG(object->object.damage_flags, _object_dead_bit) &&
		object->object.body_vitality < 1.f)
	{
		object->object.body_vitality = 1.f;
		restored = TRUE;
	}

	return restored;
}

boolean object_double_charge_shield(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	boolean charged = object->object.shield_vitality <= 1.f;

	if (charged)
	{
		SET_FLAG(object->object.damage_flags, _object_shield_over_charging_bit, TRUE);
		if (object->object.shield_vitality == 0.f)
			object->object.shield_vitality = 0.01f;

		object->object.shield_stun_ticks = 0;
	}

	return charged;
}

void object_deplete_body(
	long object_index)
{
	struct object_datum *object;
	word damage_flags;

	object = object_get(object_index);
	damage_flags = object->object.damage_flags;
	if (!TEST_FLAG(damage_flags, _object_dead_bit))
	{
		struct object_definition *definition;
		long collision_model_index;

		SET_FLAG(damage_flags, _object_dead_bit, TRUE);
		object->object.damage_flags = damage_flags;

		definition = object_definition_get(object->definition_index);
		collision_model_index = definition->object.collision_model.index;
		if (collision_model_index != NONE)
		{
			struct collision_model *collision_model;

			collision_model = collision_model_definition_get(collision_model_index);
			damage_effect_new_on_object(
				collision_model->resistance.body_depleted_effect.index,
				object_index);
		}

		if (object->object.type == _object_type_vehicle)
		{
			long unit_index;

			unit_index = object->object.first_child_object_index;
			while (unit_index != NONE)
			{
				struct unit_datum *unit;

				unit = (struct unit_datum *)object_get(unit_index);
				if (unit->object.type == _object_type_biped &&
					(unit->unit.player_index == NONE || !cheat.deathless_player) &&
					unit->unit.parent_seat_index != NONE)
				{
					unit_kill(unit_index);
				}

				unit_index = unit->object.next_object_index;
			}
		}

		object_deplete_shield(object_index);
	}

	return;
}

void code_00126090(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	long child_object_index = object->object.first_child_object_index;

	while (child_object_index != NONE)
	{
		long next_object_index = object_get(child_object_index)->object.next_object_index;

		if (!object_type_handle_parent_destroyed(child_object_index))
			code_00126090(child_object_index);

		child_object_index = next_object_index;
	}

	return;
}

void object_destroy(
	long object_index)
{
	struct object_datum *object;
	struct object_definition *definition;
	long collision_model_index;

	object = object_get(object_index);
	definition = object_definition_get(object->definition_index);

	object_deplete_body(object_index);

	collision_model_index = definition->object.collision_model.index;
	if (collision_model_index != NONE)
	{
		struct collision_model *collision_model;

		collision_model = collision_model_definition_get(collision_model_index);
		damage_effect_new_on_object(
			collision_model->resistance.body_destroyed_effect.index,
			object_index);
	}

	code_00126090(object_index);
	object_delete(object_index);

	return;
}

void object_deplete_shield(
	long object_index)
{
	struct object_datum *object = object_get(object_index);

	if (!TEST_FLAG(object->object.damage_flags, _object_shield_depleted_bit))
	{
		struct object_definition *definition = object_definition_get(object->definition_index);
		long collision_model_index = definition->object.collision_model.index;

		if (collision_model_index != NONE)
		{
			struct collision_model *collision_model =
				collision_model_definition_get(collision_model_index);

			damage_effect_new_on_object(
				collision_model->resistance.shield_depleted_effect.index,
				object_index);
		}

		object->object.current_shield_damage = 0.f;
		SET_FLAG(object->object.damage_flags, _object_shield_depleted_bit, TRUE);
		object_permutation_shield_regions(object_index, FALSE);
	}

	return;
}

void render_debug_object_damage(
	void)
{
	if (debug_damage)
	{
		char buffer[2048];
		struct collision_result collision;
		real_vector3d direction;
		rectangle2d bounds = render.camera.window_bounds;

		bounds.x0 += 320;
		if (damage_globals.debug_object_index == NONE)
		{
			_snprintf(
				buffer,
				sizeof(buffer),
				"no object to debug|n(point and press space)");
		}
		else
		{
			struct object_datum *object =
				object_try_and_get(damage_globals.debug_object_index);

			if (object)
			{
				_snprintf(
					buffer,
					sizeof(buffer),
					"%s|nbody %0.3f|n  current %0.3f|n  recent %0.3f|nshield %0.3f|n  current %0.3f|n  recent %0.3f|n",
					strrchr(tag_get_name(object->definition_index), '\\'),
					(double)object->object.body_vitality,
					(double)object->object.current_body_damage,
					(double)object->object.recent_body_damage,
					(double)object->object.shield_vitality,
					(double)object->object.current_shield_damage,
					(double)object->object.recent_shield_damage);
			}
			else
			{
				damage_globals.debug_object_index = NONE;
			}
		}

		draw_string_set_format(NONE, 0, 0);
		draw_string_set_color(global_real_argb_white);
		rasterizer_draw_string(&bounds, NULL, NULL, 0, buffer);

		if (input_key_is_down(_key_space))
		{
			long unit_index = NONE;

			if (render.local_player_index != NONE)
			{
				unit_index = player_get(
					local_player_get_player_index(render.local_player_index))->unit_index;
			}

			scale_vector3d(&render.camera.forward, 50.f, &direction);
			if (collision_test_vector(
				FLAG(_collision_test_front_facing_surfaces_bit) |
					FLAG(_collision_test_objects_bit),
				&render.camera.position,
				&direction,
				unit_index,
				&collision))
			{
				match_assert(
					"c:\\halo\\SOURCE\\objects\\damage.c",
					0x794,
					collision.type==_collision_result_object);
				damage_globals.debug_object_index = collision.object_index;
			}
		}
	}

	return;
}

void object_can_take_damage(
	long object_list_index)
{
	long reference_index;
	long object_index = object_list_get_first(object_list_index, &reference_index);

	while (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_cannot_take_damage_bit, FALSE);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void object_cannot_take_damage(
	long object_list_index)
{
	long reference_index;
	long object_index = object_list_get_first(object_list_index, &reference_index);

	while (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_cannot_take_damage_bit, TRUE);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void object_set_ranged_attack_inhibited(
	long object_index,
	boolean inhibited)
{
	if (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_ranged_attack_inhibited_bit, inhibited);
	}

	return;
}

void object_set_melee_attack_inhibited(
	long object_index,
	boolean inhibited)
{
	if (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_melee_attack_inhibited_bit, inhibited);
	}

	return;
}

void area_of_effect_cause_damage(
	struct damage_data *damage,
	long unused_object_index)
{
	struct damage_effect_definition *definition =
		damage_effect_definition_get(damage->definition_index);
	long object_indices[64];
	short object_count;

	object_count = objects_in_sphere(
		0,
		0,
		&damage->location,
		&damage->origin,
		definition->cutoff_radius,
		object_indices,
		NUMBEROF(object_indices));
	if (object_count > 0)
	{
		short object_number;

		for (object_number = 0; object_number < object_count; object_number++)
		{
			area_of_effect_cause_damage_to_object(
				damage,
				object_indices[object_number],
				FALSE);
		}
	}

	breakable_surface_damage_area_of_effect(damage);
	return;
}

/* ---------- private code */

static long get_player_index_from_object_or_parents(
	long object_index)
{
	long player_index = NONE;

	while (object_index != NONE)
	{
		if (unit_try_and_get(object_index))
		{
			player_index = player_index_from_unit_index(object_index);
			break;
		}

		object_index = object_get(object_index)->object.parent_object_index;
	}

	return player_index;
}

static void damage_effect_new_on_object(
	long effect_definition_index,
	long object_index)
{
	effect_new_from_object(
		effect_definition_index,
		object_index,
		object_index,
		NONE,
		0.f,
		0.f,
		NULL,
		NULL);
	return;
}

void area_of_effect_cause_damage_to_object(
	struct damage_data *damage,
	long object_index,
	boolean damage_next_object)
{
	boolean did_damage;
	boolean collision_blocked;
	boolean infection_form;
	struct object_datum *object;
	struct object_definition *object_definition;
	struct damage_effect_definition *damage_effect;
	real_point3d const *epicenter;
	real_vector3d to_object;
	real_vector3d perpendicular;
	real_vector3d cross;
	real_vector3d collision_vector;
	real_vector3d offset_vector;
	real_vector3d direct_vector;
	real_point3d collision_point;
	struct collision_result spread_collision;
	struct collision_result direct_collision;
	boolean can_damage;

	object = object_get(object_index);
	object_definition = object_definition_get(object->definition_index);
	damage_effect = damage_effect_definition_get(damage->definition_index);
	can_damage = !TEST_FLAG(object->object.flags, _object_invisible_bit);
	did_damage = FALSE;
	infection_form = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\objects\\damage.c",
		601,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_area_damage;

	if (can_damage &&
		TEST_FLAG(_object_mask_unit, object->object.type) &&
		damage_effect->damage.area_of_effect_core_radius > _real_epsilon)
	{
		long ray_index;
		long rays_remaining;

		epicenter = &damage->epicenter;
		collision_blocked = TRUE;
		vector_from_points3d(
			epicenter,
			&object->object.bounding_sphere_center,
			&to_object);
		normalize3d(perpendicular3d(&to_object, &perpendicular));
		normalize3d(cross_product3d(&to_object, &perpendicular, &cross));

		for (ray_index = 0, rays_remaining = 4;
			rays_remaining;
			ray_index++, rays_remaining--)
		{
			long ultimate_parent_index;

			switch (ray_index)
			{
			case 0:
				scale_vector3d(
					&perpendicular,
					damage_effect->damage.area_of_effect_core_radius,
					&collision_vector);
				break;

			case 1:
				scale_vector3d(
					&perpendicular,
					-damage_effect->damage.area_of_effect_core_radius,
					&collision_vector);
				break;

			case 2:
				scale_vector3d(
					&cross,
					damage_effect->damage.area_of_effect_core_radius,
					&collision_vector);
				break;

			case 3:
				scale_vector3d(
					&cross,
					-damage_effect->damage.area_of_effect_core_radius,
					&collision_vector);
				break;
			}

			collision_test_vector(
				_damage_area_of_effect_collision_flags,
				epicenter,
				&collision_vector,
				object_get_ultimate_parent(object_index),
				&spread_collision);
			collision_point = spread_collision.point;
			ultimate_parent_index = object_get_ultimate_parent(object_index);
			if (!collision_test_vector(
				_damage_area_of_effect_collision_flags,
				&collision_point,
				vector_from_points3d(
					&collision_point,
					&object->object.bounding_sphere_center,
					&offset_vector),
				ultimate_parent_index,
				&spread_collision))
			{
				collision_blocked = FALSE;
			}
		}
	}
	else
	{
		long ultimate_parent_index;

		ultimate_parent_index = object_get_ultimate_parent(object_index);
		epicenter = &damage->epicenter;
		collision_blocked = collision_test_vector(
			_damage_area_of_effect_collision_flags,
			epicenter,
			vector_from_points3d(
				epicenter,
				&object->object.bounding_sphere_center,
				&direct_vector),
			ultimate_parent_index,
			&direct_collision);
	}

	if (collision_blocked)
		can_damage = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\objects\\damage.c",
		660,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	if (TEST_FLAG(damage_effect->damage.flags, _damage_does_not_hurt_owner_bit) &&
		object_index == damage->owner_object_index)
	{
		can_damage = FALSE;
	}

	if (TEST_FLAG(damage_effect->damage.flags, _damage_does_not_hurt_friends_bit) &&
		!game_team_is_enemy(object->object.owner_team_index, damage->owner_team_index))
	{
		can_damage = FALSE;
	}

	if (can_damage &&
		TEST_FLAG(damage_effect->damage.flags, _damage_infection_form_pop_bit))
	{
		can_damage = FALSE;
		if (TEST_FLAG(_object_mask_unit, object->object.type) &&
			TEST_FLAG(
				unit_definition_get(object->definition_index)->unit.flags,
				_unit_is_inconsequential_bit) &&
			object_index != damage->owner_object_index)
		{
			real infection_form_toughness = game_difficulty_get_value(
				_game_difficulty_value_infection_forms);

			can_damage = TRUE;
			if (infection_form_toughness > 0.f ||
				TEST_FLAG(
					damage_effect->damage.flags,
					_damage_does_not_hurt_infection_forms_bit))
			{
				if (TEST_FLAG(damage->flags, _damage_damaged_one_object_bit))
					can_damage = FALSE;
			}

			if (infection_form_toughness > 0.f &&
				real_seed_random(get_global_random_seed_address()) <
					infection_form_toughness * 0.25f)
			{
				can_damage = FALSE;
			}

			infection_form = TRUE;
		}
	}

	SET_FLAG(damage->flags, _damage_area_of_effect_bit, TRUE);
	if (can_damage)
	{
		real distance;
		real radius_delta;
		real scale;

		vector_from_points3d(
			epicenter,
			&object->object.bounding_sphere_center,
			&damage->direction);
		distance = normalize3d(&damage->direction);
		radius_delta = damage_effect->cutoff_radius - damage_effect->falloff_radius;
		if (radius_delta > 0.f)
		{
			scale = PIN(
				1.f -
					(distance - damage_effect->falloff_radius) /
					radius_delta,
				0.f,
				1.f);
		}
		else
		{
			scale = 1.f;
		}

		if (!TEST_FLAG(
			damage_effect->flags,
			_damage_effect_dont_scale_damage_by_distance_bit))
		{
			damage->scale = scale;
		}

		if (scale > 0.f)
		{
			object_cause_damage(
				damage,
				object_index,
				NONE,
				NONE,
				NONE,
				NULL);
				did_damage = TRUE;
		}

		if (object_definition->object.collision_model.index != NONE)
		{
			struct collision_model *collision_model =
				collision_model_definition_get(
					object_definition->object.collision_model.index);

			if (TEST_FLAG(
				collision_model->resistance.flags,
				_damage_resistance_children_take_area_damage_bit))
			{
				long child_object_index =
					object->object.first_child_object_index;

				if (child_object_index != NONE)
				{
					area_of_effect_cause_damage_to_object(
						damage,
						child_object_index,
						TRUE);
				}
			}
		}
	}

	if (infection_form && (!can_damage || did_damage))
	{
		SET_FLAG(damage->flags, _damage_damaged_one_object_bit, TRUE);
	}

	if (damage_next_object)
	{
		long next_object_index = object->object.next_object_index;

		if (next_object_index != NONE)
		{
			area_of_effect_cause_damage_to_object(
				damage,
				next_object_index,
				TRUE);
		}
	}

	return;
}

static void object_permutation_shield_regions(
	long object_index,
	boolean active)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	struct collision_model *collision_model =
		collision_model_definition_get(definition->object.collision_model.index);
	short region_index;

	for (region_index = 0;
		region_index < collision_model->resistance.regions.count;
		region_index++)
	{
		struct damage_region *region = TAG_BLOCK_GET_ELEMENT(
			&collision_model->resistance.regions,
			region_index,
			struct damage_region);

		if (TEST_FLAG(region->flags, _object_region_missing_when_shield_is_zero_bit) &&
			region->permutations.count > 1)
		{
			object->object.region_permutations[region_index] = !active;
		}
	}

	return;
}
