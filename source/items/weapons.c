/*
WEAPONS.C

symbols in this file:
000EA4D0 0010:
	_animation_convert_frame_to_pal (0000)
000EA4E0 0010:
	_animation_key_frame_index (0000)
000EA4F0 0020:
	_animation_update (0000)
000EA510 0020:
	_animation_choose_random_permutation (0000)
000EA530 0010:
	_weapons_initialize (0000)
000EA540 0010:
	_weapons_initialize_for_new_map (0000)
000EA550 0010:
	_weapons_dispose_from_old_map (0000)
000EA560 0010:
	_weapons_dispose (0000)
000EA570 00d0:
	_weapon_place (0000)
000EA640 0050:
	_weapon_preprocess_node_orientations (0000)
000EA690 0030:
	_weapon_get_label (0000)
000EA6C0 0020:
	_weapon_set_integrated_light_power (0000)
000EA6E0 0080:
	_weapon_estimate_time_to_target (0000)
000EA760 0090:
	_weapon_can_be_fired (0000)
000EA7F0 0030:
	_weapon_useful (0000)
000EA820 0070:
	_weapon_compute_movement_penalty (0000)
000EA890 0010:
	_weapon_melee_attack (0000)
000EA8A0 0030:
	_weapon_must_be_readied (0000)
000EA8D0 0030:
	_weapon_is_flag (0000)
000EA900 0050:
	_weapon_prevents_grenade_throwing (0000)
000EA950 01b0:
	_weapon_get_first_person_animation_time (0000)
000EAB00 0030:
	_weapon_overcharged (0000)
000EAB30 0050:
	_code_000eab30 (0000)
000EAB80 0050:
	_code_000eab80 (0000)
000EABD0 0050:
	_code_000eabd0 (0000)
000EAC20 0040:
	_code_000eac20 (0000)
000EAC60 0030:
	_code_000eac60 (0000)
000EAC90 0040:
	_code_000eac90 (0000)
000EACD0 0050:
	_code_000eacd0 (0000)
000EAD20 0090:
	_code_000ead20 (0000)
000EADB0 00f0:
	_code_000eadb0 (0000)
000EAEA0 0050:
	_code_000eaea0 (0000)
000EAEF0 00f0:
	_code_000eaef0 (0000)
000EAFE0 0070:
	_code_000eafe0 (0000)
000EB050 0040:
	_code_000eb050 (0000)
000EB090 0090:
	_code_000eb090 (0000)
000EB120 0080:
	_code_000eb120 (0000)
000EB1A0 0050:
	_code_000eb1a0 (0000)
000EB1F0 0020:
	_code_000eb1f0 (0000)
000EB210 0020:
	_code_000eb210 (0000)
000EB230 01b0:
	_code_000eb230 (0000)
000EB3E0 0120:
	_weapon_set_total_rounds (0000)
000EB500 0010:
	_power (0000)
000EB510 0010:
	_random (0000)
000EB520 0190:
	_weapon_new (0000)
000EB6B0 0060:
	_weapon_delete (0000)
000EB710 0390:
	_weapon_export_function_values (0000)
000EBAA0 0220:
	_weapon_handle_potential_inventory_item (0000)
000EBCC0 00a0:
	_weapon_owner_update (0000)
000EBD60 0140:
	_weapon_build_weapon_interface_state (0000)
000EBEA0 0080:
	_weapon_reloading (0000)
000EBF20 0070:
	_weapon_rotate_zoom_level (0000)
000EBF90 0160:
	_weapon_get_zoom_magnification (0000)
000EC0F0 0050:
	_weapon_get_field_of_view (0000)
000EC140 0060:
	_weapon_prevents_melee_attack (0000)
000EC1A0 0160:
	_code_000ec1a0 (0000)
000EC300 00e0:
	_code_000ec300 (0000)
000EC3E0 00c0:
	_code_000ec3e0 (0000)
000EC4A0 0080:
	_code_000ec4a0 (0000)
000EC520 00c0:
	_code_000ec520 (0000)
000EC5E0 0090:
	_code_000ec5e0 (0000)
000EC670 0060:
	_code_000ec670 (0000)
000EC6D0 0060:
	_code_000ec6d0 (0000)
000EC730 0190:
	_code_000ec730 (0000)
000EC8C0 00a0:
	_code_000ec8c0 (0000)
000EC960 0030:
	_code_000ec960 (0000)
000EC990 0160:
	_weapon_set_current_amount (0000)
000ECAF0 0080:
	_weapon_ready (0000)
000ECB70 00a0:
	_weapon_put_away (0000)
000ECC10 0110:
	_weapon_aim (0000)
000ECD20 0010:
	_weapon_stop_reload (0000)
000ECD30 0050:
	_code_000ecd30 (0000)
000ECD80 0720:
	_code_000ecd80 (0000)
000ED4A0 07c0:
	_code_000ed4a0 (0000)
000EDC60 0270:
	_code_000edc60 (0000)
000EDED0 00d0:
	_code_000eded0 (0000)
000EDFA0 0100:
	_code_000edfa0 (0000)
000EE0A0 0080:
	_code_000ee0a0 (0000)
000EE120 0af0:
	_weapon_update (0000)
00279248 000e:
	??_C@_0O@NGIMIMAN@weapon_update?$AA@ (0000)
00279258 0010:
	??_C@_0BA@HKDAKBBH@?$HOsecondary?9blur?$AA@ (0000)
00279268 000e:
	??_C@_0O@MICLHJCM@?$HOprimary?9blur?$AA@ (0000)
00279278 001f:
	??_C@_0BP@EKCOHDKN@c?3?2halo?2SOURCE?2items?2weapons?4c?$AA@ (0000)
00279298 004b:
	??_C@_0EL@OJBCMDA@trigger_index?$DO?$DN0?5?$CG?$CG?5trigger_inde@ (0000)
002792E8 004e:
	??_C@_0EO@GOIBLAPE@magazine_index?$DO?$DN0?5?$CG?$CG?5magazine_in@ (0000)
00279338 0033:
	??_C@_0DD@MMDDCEO@new_state?$DO?$DN0?5?$CG?$CG?5new_state?$DMNUMBER@ (0000)
00279370 0048:
	??_C@_0EI@IEKIMHHO@trigger_index?$DO?$DN0?5?$CG?$CG?5trigger_inde@ (0000)
002793B8 000d:
	??_C@_0N@PABHPCND@rounds_array?$AA@ (0000)
002793C8 001e:
	??_C@_0BO@MNJMFFGN@?$CBweapon_is_flag?$CIweapon_index?$CJ?$AA@ (0000)
002793E8 001f:
	??_C@_0BP@BIBKIDBE@weapon?9?$DOweapon?4primary_trigger?$AA@ (0000)
00279408 0013:
	??_C@_0BD@CJIOBLLL@magnification?$DO0?40f?$AA@ (0000)
0027941C 000e:
	??_C@_0O@NODOJABJ@magnification?$AA@ (0000)
0027942C 0004:
	__real@40470d23 (0000)
00279430 0004:
	__real@3d00adfd (0000)
00279434 0012:
	??_C@_0BC@IMIGIGHG@secondary?5trigger?$AA@ (0000)
00279448 0004:
	__real@3d2aaaab (0000)
00307140 0600:
	_data_00307140 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "weapons.h"

#include "weapon_definitions.h"
#include "projectile_definitions.h"
#include "projectiles.h"

#include "cache/cache_files.h"
#include "cseries/profile.h"
#include "effects/effect_definitions.h"
#include "effects/effects.h"
#include "game/cheats.h"
#include "interface/first_person_weapons.h"
#include "models/model_animation_definitions.h"
#include "sound/game_sound.h"
#include "sound/sound_definitions.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

boolean game_engine_running(
	void);
real transition_function_evaluate(
	short function_type,
	real value);
void unit_handle_weapon_state_change(
	long object_index,
	short new_state);

static struct weapon_trigger *weapon_trigger_get(
	struct weapon_datum *weapon,
	short trigger_index);
static struct weapon_magazine *weapon_magazine_get(
	struct weapon_datum *weapon,
	short magazine_index);

static boolean weapon_busy(
	long weapon_index);
static boolean weapon_magazine_state_change_ok(
	long weapon_index);
static long weapon_get_effect_object_index(
	long weapon_index);
static long weapon_get_owner_object_index(
	long weapon_index);
static long weapon_effect_new(
	long weapon_index,
	long effect_index,
	real effect_scale,
	real effect_error);
static void weapon_reset(
	long weapon_index);

static boolean weapon_state_interruptable(
	short old_state,
	short new_state);
static boolean weapon_set_state(
	long weapon_index,
	short new_state,
	boolean immediate);

void weapon_magazine_finish_reload(
	long weapon_index,
	short magazine_index);
static void weapon_magazine_start_reload(
	long weapon_index,
	short magazine_index,
	boolean unknown);
static void weapon_state_next(
	long weapon_index);

/* ---------- globals */

struct weapons_globals
{
	char *blurred_permutation_names[2];
	struct profile_section update_profile;
};

struct weapons_globals data_00307140 =
{
	{"~primary-blur", "~secondary-blur"},
	{"weapon_update", NONE, TRUE}
};

/* ---------- public code */

void weapons_initialize(
	void)
{
	return;
}

void weapons_initialize_for_new_map(
	void)
{
	return;
}

void weapons_dispose_from_old_map(
	void)
{
	return;
}

void weapons_dispose(
	void)
{
	return;
}

void weapon_ready(
	long weapon_index)
{
	struct weapon_datum* weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	weapon_reset(weapon_index);
	weapon_set_state(weapon_index, _weapon_state_ready, TRUE);
	first_person_weapon_message_from_weapon(weapon_index, _first_person_weapon_message_ready);
	weapon_effect_new(weapon_index, weapon_definition->weapon.ready_effect.index, 0.f, 0.f);
	weapon->weapon.state_timer = weapon_get_first_person_animation_time(weapon_index, 0, _first_person_weapon_animation_ready, NONE);

	return;
}

void weapon_update(
	long weapon_index)
{
	weapon_state_next(weapon_index);

	return;
}

boolean weapon_put_away(
	long weapon_index,
	boolean immediate)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	boolean put_away = FALSE;

	if ((immediate || !weapon_busy(weapon_index)) && weapon_set_state(weapon_index, _weapon_state_put_away, immediate))
	{
		weapon->weapon.control_flags = 0;
		weapon_reset(weapon_index);

		if (weapon->weapon.overheated_effect_index != NONE)
		{
			effect_delete(weapon->weapon.overheated_effect_index);
			weapon->weapon.overheated_effect_index = NONE;
		}
		
		first_person_weapon_message_from_weapon(weapon_index, 11);
		put_away = TRUE;
	}

	return put_away;
}

boolean weapon_is_flag(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	return (weapon_definition->weapon.flags>>3)&1;
}

boolean weapon_must_be_readied(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	return (weapon_definition->weapon.flags>>3)&1;
}

boolean weapon_overcharged(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);

	return weapon->weapon.triggers[0].state==_trigger_charging || weapon->weapon.triggers[0].state==_trigger_charged;
}

void weapon_stop_reload(
	long weapon_index)
{
	weapon_reset(weapon_index);

	return;
}

char const *weapon_get_label(
	long weapon_index)
{
	char const *label = "";

	if (weapon_index!=NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

		label = weapon_definition->weapon.label;
	}

	return label;
}

boolean weapon_useful(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);

	if (weapon->weapon.age>=1.0f)
		return FALSE;

	return TRUE;
}

void weapon_set_integrated_light_power(
	long weapon_index,
	real light_power)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);

	weapon->weapon.integrated_light_power = light_power;

	return;
}

boolean weapon_prevents_grenade_throwing(
	long weapon_index)
{
	boolean result = TRUE;

	if (weapon_index!=NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

		result = (weapon_definition->weapon.flags>>6)&1;
		if (weapon->weapon.state>=_weapon_state_primary_reload && weapon->weapon.state<=_weapon_state_put_away)
			result = TRUE;
	}

	return result;
}

boolean weapon_prevents_melee_attack(
	long weapon_index)
{
	boolean result = TRUE;

	if (weapon_index!=NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

		result = (weapon_definition->weapon.flags>>9)&1;
		if (weapon_overcharged(weapon_index))
			result = TRUE;
	}

	return result;
}

void weapon_melee_attack(
	long weapon_index)
{
	return;
}

void weapon_set_total_rounds(
	long weapon_index,
	short *rounds_array)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	short magazine_index;

	match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 3082, rounds_array);

	for (magazine_index = 0; magazine_index<weapon_definition->weapon.magazines.count; ++magazine_index)
	{
		struct weapon_magazine *magazine = weapon_magazine_get(weapon, magazine_index);
		struct weapon_magazine_definition *magazine_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.magazines, magazine_index, struct weapon_magazine_definition);

		magazine->rounds_total = MIN(magazine_definition->rounds_total_maximum, rounds_array[magazine_index]);
		magazine->rounds_loaded = MIN(magazine->rounds_loaded, magazine->rounds_total);
	}

	return;
}

void weapon_delete(
	long weapon_index)
{
	if (game_engine_running())
	{
		match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 234, !weapon_is_flag(weapon_index));
	}

	return;
}

short animation_choose_random_permutation(
	long animation_graph_index,
	short animation_index)
{
	return animation_choose_random_permutation_internal(TRUE, animation_graph_index, animation_index);
}

short animation_key_frame_index(
	struct animation const *animation)
{
	return animation->private_key_frame_index;
}

short animation_convert_frame_to_pal(
	struct animation const *animation,
	short frame_index)
{
	return frame_index;
}

boolean weapon_reloading(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	boolean result = FALSE;

	if (weapon_definition->weapon.magazines.count>0)
	{
		if (weapon_magazine_get(weapon, 0)->state==_magazine_reloading)
			result = TRUE;
	}

	return result;
}

short weapon_rotate_zoom_level(
	long weapon_index,
	short zoom_level)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	if (!weapon_reloading(weapon_index))
	{
		if (zoom_level>=0 && zoom_level<weapon_definition->weapon.zoom_level_count-1)
			zoom_level++;
		else
			zoom_level = zoom_level==weapon_definition->weapon.zoom_level_count-1 ? NONE : 0;
	}

	return zoom_level;
}

real weapon_get_zoom_magnification(
	long weapon_index,
	short zoom_level)
{
	real magnification = 1.0f;
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	if (zoom_level>=0 && zoom_level<weapon_definition->weapon.zoom_level_count)
	{
		real zoom_fraction = weapon_definition->weapon.zoom_level_count>1 ? (real)zoom_level/(weapon_definition->weapon.zoom_level_count-1) : 0.0f;
		real minimum_magnification = weapon_definition->weapon.zoom_magnification_minimum>0.0f ? weapon_definition->weapon.zoom_magnification_minimum : 1.0f;
		real maximum_magnification = weapon_definition->weapon.zoom_magnification_maximum>0.0f ? weapon_definition->weapon.zoom_magnification_maximum : 1.0f;

		magnification = power(maximum_magnification/minimum_magnification, zoom_fraction)*minimum_magnification;

		match_assert_valid_real("c:\\halo\\SOURCE\\items\\weapons.c", 1442, magnification);
		match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 1443, magnification>0.0f);
	}

	return magnification;
}

real weapon_get_field_of_view(
	long weapon_index,
	real field_of_view,
	short zoom_level)
{
	real result = field_of_view;
	real magnification = weapon_get_zoom_magnification(weapon_index, zoom_level);

	if (magnification!=1.0f)
	{
		real zoom_field_of_view = field_of_view/magnification;

		if (zoom_field_of_view>_pi/100.f && zoom_field_of_view<_pi-_pi/100.f)
			result = zoom_field_of_view;
	}

	return result;
}

real weapon_estimate_time_to_target(
	long weapon_index,
	short trigger_index,
	real distance)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	real result = 0.0f;

	if (trigger_index>=0 && trigger_index<weapon_definition->weapon.triggers.count)
	{
		struct weapon_trigger_definition *trigger_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);

		result = projectile_estimate_time_to_target(projectile_definition_get(trigger_definition->projectile.index), distance);
	}

	return result;
}

real weapon_compute_movement_penalty(
	long weapon_index,
	boolean forward,
	boolean airborne)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	real penalty;

	if (forward)
		penalty = weapon_definition->weapon.forward_movement_penalty;
	else
		penalty = weapon_definition->weapon.sideways_movement_penalty;

	switch (weapon_definition->weapon.movement_penalty_mode)
	{
	case 1:
		if (!airborne)
			penalty = 0.0f;
		break;

	case 2:
		if ((weapon->weapon.magazines[0].state==_magazine_reloading || weapon->weapon.magazines[1].state==_magazine_reloading) && !airborne)
			penalty = 0.0f;
		break;
	}

	return penalty;
}

void weapon_owner_update(
	long weapon_index,
	word control_flags,
	real primary_trigger)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	weapon->weapon.control_flags = control_flags;
	weapon->weapon.primary_trigger = transition_function_evaluate(4, primary_trigger);
	match_assert_valid_real("c:\\halo\\SOURCE\\items\\weapons.c", 1199, weapon->weapon.primary_trigger);

	return;
}

void weapon_magazine_finish_reload(
	long weapon_index,
	short magazine_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_magazine *magazine = weapon_magazine_get(weapon, magazine_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	struct weapon_magazine_definition *magazine_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.magazines, magazine_index, struct weapon_magazine_definition);
	long rounds_to_load;
	short rounds_loaded;

	if (TEST_FLAG(magazine_definition->flags, 0))
		magazine->rounds_loaded = 0;

	rounds_to_load = magazine_definition->rounds_reloaded>magazine->rounds_total ? magazine->rounds_total : magazine_definition->rounds_reloaded;
	rounds_loaded = magazine->rounds_loaded+rounds_to_load;
	if (rounds_loaded>magazine_definition->rounds_loaded_maximum)
		rounds_loaded = magazine_definition->rounds_loaded_maximum;

	if (!cheat.infinite_ammo && TEST_FLAG(weapon->item.flags, 1))
		magazine->rounds_total = magazine->rounds_total-rounds_loaded+magazine->rounds_loaded;

	magazine->rounds_loaded = rounds_loaded;
	magazine->state = _magazine_unchambered;
	magazine->state_timer = 0;

	if (magazine->rounds_total>0 &&
		rounds_loaded<magazine_definition->rounds_loaded_maximum &&
		!TEST_FLAG(magazine_definition->flags, 0) &&
		!(weapon->weapon.control_flags & 0x26))
	{
		weapon_magazine_start_reload(weapon_index, magazine_index, FALSE);
	}

	return;
}

static void weapon_magazine_start_reload(
	long weapon_index,
	short magazine_index,
	boolean unknown)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_magazine *magazine = weapon_magazine_get(weapon, magazine_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	struct weapon_magazine_definition *magazine_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.magazines, magazine_index, struct weapon_magazine_definition);

	switch (magazine->state)
	{
	case _magazine_idle:
	case _magazine_unchambered:
		if (weapon_magazine_state_change_ok(weapon_index))
		{
			if (magazine->rounds_total>0 && magazine->rounds_loaded<magazine_definition->rounds_loaded_maximum)
			{
				short reload_type = NONE;

				weapon_set_state(weapon_index, (short)(_weapon_state_primary_reload+magazine_index), FALSE);
				weapon_effect_new(weapon_index, magazine_definition->reloading_effect.index, 0.0f, 0.0f);
				first_person_weapon_message_from_weapon(weapon_index, (short)(9+(magazine->rounds_loaded!=0)));

				if (weapon_definition->weapon.weapon_type==1)
				{
					if (unknown)
						reload_type = magazine_definition->rounds_loaded_maximum-magazine->rounds_loaded==1 ? 2 : 0;
					else
						reload_type = magazine_definition->rounds_loaded_maximum-magazine->rounds_loaded==1 ? 1 : NONE;
				}

				magazine->state = _magazine_reloading;
				magazine->original_time = magazine->state_timer = weapon_get_first_person_animation_time(weapon_index, 0, _first_person_weapon_animation_reload_while_empty, reload_type);
			}

			weapon->weapon.flags &= ~FLAG(3);
		}
		break;
	}

	return;
}

static void weapon_state_next(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);

	if (weapon->weapon.state<_weapon_state_primary_charged ||
		(weapon->weapon.state>_weapon_state_secondary_charged && weapon->weapon.state!=_weapon_state_put_away))
		weapon_set_state(weapon_index, _weapon_state_idle, TRUE);

	return;
}

/* ---------- private code */

static struct weapon_trigger *weapon_trigger_get(
	struct weapon_datum *weapon,
	short trigger_index)
{
	struct weapon_definition const *weapon_definition = weapon_definition_get(weapon->definition_index);

	match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 1639, trigger_index>=0 && trigger_index<weapon_definition->weapon.triggers.count);

	return &weapon->weapon.triggers[trigger_index];
}

static struct weapon_magazine *weapon_magazine_get(
	struct weapon_datum *weapon,
	short magazine_index)
{
	struct weapon_definition const *weapon_definition = weapon_definition_get(weapon->definition_index);

	match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 1650, magazine_index>=0 && magazine_index<weapon_definition->weapon.magazines.count);

	return &weapon->weapon.magazines[magazine_index];
}

static boolean weapon_busy(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	return
		weapon->weapon.triggers[0].state != _trigger_idle ||
		weapon->weapon.triggers[1].state != _trigger_idle ||
		weapon->weapon.magazines[0].state != _magazine_idle ||
		weapon->weapon.magazines[1].state != _magazine_idle ||
		weapon->weapon.state != _weapon_state_idle;
}

static boolean weapon_magazine_state_change_ok(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);

	return 
		weapon->weapon.triggers[0].state==_trigger_idle &&
		weapon->weapon.triggers[1].state==_trigger_idle &&
		weapon->weapon.state == _weapon_state_idle;
}

static long weapon_get_effect_object_index(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	long result = weapon_index;

	if (TEST_FLAG(weapon->object.flags, _object_invisible_bit) && weapon->object.parent_object_index!=NONE)
	{
		result = weapon->object.parent_object_index;
	}

	return result;
}

static long weapon_get_owner_object_index(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	long result = NONE;

	if (weapon->object.parent_object_index!=NONE && unit_try_and_get(weapon->object.parent_object_index))
	{
		result = weapon->object.parent_object_index;
	}

	return result;
}

static long weapon_effect_new(
	long weapon_index,
	long effect_index,
	real effect_scale,
	real effect_error)
{
	long result = NONE;

	if (effect_index!=NONE)
	{
		long effect_object_index = weapon_get_effect_object_index(weapon_index);
		long object_index = weapon_get_owner_object_index(weapon_index);
		long group_tag = tag_get_group_tag(effect_index);

		if (group_tag!=EFFECT_DEFINITION_TAG)
		{
			match_vassert("c:\\halo\\SOURCE\\items\\weapons.c", 2514, group_tag==SOUND_DEFINITION_TAG, NULL);
			
			if (group_tag==SOUND_DEFINITION_TAG)
			{
				object_impulse_sound_new(object_index, effect_index, NONE, global_origin3d, global_forward3d, effect_scale);
				result = NONE;
			}
		}
		else
		{
			result = effect_new_from_object(effect_index, object_index, effect_object_index, NONE, effect_scale, effect_error, NULL, NULL);
		}
	}

	return result;
}

// TODO: finish
static void weapon_reset(
	long weapon_index)
{
	short magazine_index;

	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

	for (magazine_index = 0; magazine_index<weapon_definition->weapon.triggers.count; ++magazine_index)
	{
		struct weapon_trigger* trigger = weapon_trigger_get(weapon, magazine_index);
		struct weapon_trigger_definition *trigger_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.triggers, magazine_index, struct weapon_trigger_definition);

		trigger->state = _trigger_uninitialized;
		trigger->state_timer = 0;
	}

	for (magazine_index = 0; magazine_index<weapon_definition->weapon.magazines.count; ++magazine_index)
	{
		struct weapon_magazine *magazine = weapon_magazine_get(weapon, magazine_index);
		struct weapon_magazine_definition *magazine_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.magazines, magazine_index, struct weapon_magazine_definition);

		if (magazine->state==_magazine_reloading)
		{
			if (2*magazine->state_timer<weapon_get_first_person_animation_time(weapon_index, 0, _first_person_weapon_animation_reload_while_empty, NONE))
			{
				weapon_magazine_finish_reload(weapon_index, magazine_index);
			}
		}

		magazine->state = _magazine_idle;
		magazine->state_timer = 0;
	}

	return;
}

static boolean weapon_state_interruptable(
	short old_state,
	short new_state)
{
	boolean interruptable = FALSE;
	long state = old_state;

	if (state!=_weapon_state_idle)
	{
		if (state>_weapon_state_idle && state<=_weapon_state_secondary_recoil)
			interruptable = new_state >= old_state;
	}
	else
	{
		interruptable = TRUE;
	}

	return interruptable;
}

struct animation_graph_weapon_animation
{
	long unused[4];
	struct tag_block animations;
};

void weapon_preprocess_node_orientations(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(weapon_definition->object.animation_graph.index);

	if (animation_graph->weapon_animations.count)
		TAG_BLOCK_GET_ELEMENT(&animation_graph->weapon_animations, 0, struct animation_graph_weapon_animation);

	return;
}

static boolean weapon_set_state(
	long weapon_index,
	short new_state,
	boolean immediate)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	boolean busy = FALSE;

	if (immediate || weapon_state_interruptable(weapon->weapon.state, new_state))
	{
		long owner_object_index;

		if (weapon_definition->object.animation_graph.index!=NONE)
		{
			struct animation_graph *animation_graph = animation_graph_definition_get(weapon_definition->object.animation_graph.index);

			if (animation_graph->weapon_animations.count)
			{
				struct animation_graph_weapon_animation *weapon_animation = TAG_BLOCK_GET_ELEMENT(&animation_graph->weapon_animations, 0, struct animation_graph_weapon_animation);

				if (weapon_animation)
				{
					short animation_index;

					switch (new_state)
					{
					case _weapon_state_idle:             animation_index = 0; break;
					case _weapon_state_primary_recoil:   animation_index = 9; break;
					case _weapon_state_secondary_recoil: animation_index = 10; break;
					case _weapon_state_primary_chamber:  animation_index = 5; break;
					case _weapon_state_secondary_chamber: animation_index = 6; break;
					case _weapon_state_primary_reload:
					case _weapon_state_secondary_reload: animation_index = 3; break;
					case _weapon_state_primary_charged:
					case _weapon_state_secondary_charged: animation_index = 8; break;
					case _weapon_state_ready:            animation_index = 1; break;
					case _weapon_state_put_away:         animation_index = 2; break;
					default: goto skip_animation;
					}

					if (animation_index<weapon_animation->animations.count)
						animation_index = animation_graph_animation_index_get(&weapon_animation->animations)[animation_index].animation_index;
					else
						animation_index = NONE;

					if (animation_index!=NONE || new_state==_weapon_state_idle)
					{
						long graph_index = weapon_definition->object.animation_graph.index;

						weapon->object.animation.state.index = animation_choose_random_permutation_internal(TRUE, graph_index, animation_index);
						weapon->object.animation.state.frame_index = 0;
						weapon->weapon.state = (char)new_state;
					}

skip_animation:;
				}
			}
		}

		owner_object_index = weapon_get_owner_object_index(weapon_index);
		if (unit_try_and_get(owner_object_index))
		{
			unit_handle_weapon_state_change(owner_object_index, new_state);
		}

		busy = TRUE;
	}

	return busy;
}
