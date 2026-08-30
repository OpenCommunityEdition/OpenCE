/*
ACTORS.C

symbols in this file:
00028FA0 00c0:
	_actors_initialize (0000)
00029060 0010:
	_actors_dispose (0000)
00029070 0030:
	_actors_initialize_for_new_map (0000)
000290A0 0030:
	_actors_dispose_from_old_map (0000)
000290D0 00c0:
	_actor_set_team (0000)
00029190 00a0:
	_actor_activation_debug_color (0000)
00029230 0080:
	_code_00029230 (0000)
000292B0 00e0:
	_actor_verify_activation (0000)
00029390 00e0:
	_actor_detach_from_unit (0000)
00029470 01d0:
	_actor_swarm_detach_from_unit (0000)
00029640 0080:
	_actor_swarm_cache_delete (0000)
000296C0 0050:
	_code_000296c0 (0000)
00029710 0020:
	_actor_is_noncombat (0000)
00029730 0030:
	_actor_in_combat (0000)
00029760 0040:
	_actor_is_fighting (0000)
000297A0 00b0:
	_actor_attacking_target (0000)
00029850 0030:
	_actor_is_leaping (0000)
00029880 00b0:
	_actor_get_weapon (0000)
00029930 0060:
	_actor_has_ranged_weapon (0000)
00029990 0040:
	_actor_target_unit_index (0000)
000299D0 0050:
	_actor_derive_target_information (0000)
00029A20 01d0:
	_actor_switch_props (0000)
00029BF0 0050:
	_actor_flush_position_indices (0000)
00029C40 00c0:
	_actor_flush_structure_indices (0000)
00029D00 0010:
	_actor_has_unlimited_grenades (0000)
00029D10 00e0:
	_actor_handle_damage (0000)
00029DF0 0080:
	_code_00029df0 (0000)
00029E70 00a0:
	_code_00029e70 (0000)
00029F10 0040:
	_actors_freeze (0000)
00029F50 00c0:
	_code_00029f50 (0000)
0002A010 00a0:
	_actors_move_randomly (0000)
0002A0B0 0070:
	_actor_change_encounter (0000)
0002A120 0030:
	_code_0002a120 (0000)
0002A150 0010:
	_code_0002a150 (0000)
0002A160 00a0:
	_code_0002a160 (0000)
0002A200 00a0:
	_code_0002a200 (0000)
0002A2A0 00c0:
	_actor_find_pathfinding_location (0000)
0002A360 0090:
	_actor_destination_tolerance (0000)
0002A3F0 0070:
	_actor_input_sample_position (0000)
0002A460 0040:
	_code_0002a460 (0000)
0002A4A0 0230:
	_code_0002a4a0 (0000)
0002A6D0 0100:
	_actors_handle_spatial_effect (0000)
0002A7D0 00a0:
	_actor_handle_communication (0000)
0002A870 0040:
	_actor_unit_control_crouch (0000)
0002A8B0 0030:
	_actor_unit_control_jump (0000)
0002A8E0 0060:
	_actor_unit_control_primary_trigger (0000)
0002A940 0040:
	_actor_unit_control_secondary_trigger (0000)
0002A980 0030:
	_actor_unit_control_throw_grenade (0000)
0002A9B0 0040:
	_actor_unit_control_exact_facing (0000)
0002A9F0 0030:
	_actor_unit_control_stop_animation_impulse (0000)
0002AA20 03b0:
	_actor_new (0000)
0002ADD0 0280:
	_actor_customize_unit (0000)
0002B050 0110:
	_actor_set_dormant (0000)
0002B160 0070:
	_code_0002b160 (0000)
0002B1D0 0050:
	_actor_delete_props (0000)
0002B220 0120:
	_actor_delete (0000)
0002B340 0110:
	_actor_swarm_cache_new (0000)
0002B450 00d0:
	_actor_get_running_blind_vector (0000)
0002B520 00e0:
	_actor_kill (0000)
0002B600 0340:
	_actor_died (0000)
0002B940 00a0:
	_actor_swarm_unit_died (0000)
0002B9E0 0060:
	_actor_braindead (0000)
0002BA40 01c0:
	_actor_handle_unit_effect (0000)
0002BC00 00d0:
	_actor_set_active (0000)
0002BCD0 0290:
	_actor_swarm_attach_unit (0000)
0002BF60 00a0:
	_actor_erase (0000)
0002C000 0230:
	_code_0002c000 (0000)
0002C230 0950:
	_code_0002c230 (0000)
0002CB80 0230:
	_actors_handle_unit_effect (0000)
0002CDB0 0310:
	_code_0002cdb0 (0000)
0002D0C0 01d0:
	_actor_attach_unit (0000)
0002D290 0140:
	_code_0002d290 (0000)
0002D3D0 0270:
	_actor_create_for_unit (0000)
0002D640 0320:
	_actor_place (0000)
0002D960 02a0:
	_actors_spawn_from_unit (0000)
0002DC00 0080:
	_actors_update (0000)
00246834 000a:
	_global_movement_animation_states (0000)
00246840 0015:
	??_C@_0BF@IKFFBLEB@swarm_component_data?$AA@ (0000)
00246858 0010:
	??_C@_0BA@EHGCJOFL@swarm?5component?$AA@ (0000)
00246868 000b:
	??_C@_0L@JFMPKLIM@swarm_data?$AA@ (0000)
00246874 0006:
	??_C@_05IMMFCEFO@swarm?$AA@ (0000)
0024687C 000b:
	??_C@_0L@NNGPDIPH@actor_data?$AA@ (0000)
00246888 001b:
	??_C@_0BL@IFEJIDPD@c?3?2halo?2SOURCE?2ai?2actors?4c?$AA@ (0000)
002468A4 0006:
	??_C@_05OJNICPJM@actor?$AA@ (0000)
002468AC 001f:
	??_C@_0BP@DJKNNBPO@?$CFs?5unit?5activation?5logic?5error?$AA@ (0000)
002468CC 000b:
	??_C@_0L@KOKJAGEJ@individual?$AA@ (0000)
002468D8 000f:
	??_C@_0P@KCHBJAML@inactive?5swarm?$AA@ (0000)
002468E8 000d:
	??_C@_0N@JNMFNIL@active?5swarm?$AA@ (0000)
002468F8 001b:
	??_C@_0BL@FHOFLAAE@actor?5dormancy?5logic?5error?$AA@ (0000)
00246914 0023:
	??_C@_0CD@KACFCMHE@encounter?9?$DOunique_leader_count?5?$DO@ (0000)
00246938 0026:
	??_C@_0CG@JABJGCJL@unit?9?$DOunit?4actor_index?5?$DN?$DN?5actor_@ (0000)
00246960 0006:
	??_C@_05JEMLEKKA@found?$AA@ (0000)
00246968 0032:
	??_C@_0DC@FACDKDBJ@swarm?9?$DOunit_count?5?$DN?$DN?5actor?9?$DOmeta@ (0000)
0024699C 0022:
	??_C@_0CC@KIKJBOEI@swarm?9?$DOactor_index?5?$DN?$DN?5actor_inde@ (0000)
002469C0 000e:
	??_C@_0O@CIOBJKKB@attack_vector?$AA@ (0000)
002469D0 004c:
	??_C@_0EM@LLEBOIJP@AI?5error?5condition?5detected?0?5att@ (0000)
00246A1C 002b:
	??_C@_0CL@CDNDAKNI@actor?9type?5?$CFs?5?$CFs?5internal?5logic?5@ (0000)
00246A48 001d:
	??_C@_0BN@EGBIINPF@?5infinite?5decision?5loop?5?$CI?$CFs?$CJ?$AA@ (0000)
00246A68 0002:
	??_C@_01KMDKNFGN@?1?$AA@ (0000)
00246A6C 000f:
	??_C@_0P@CACMJKNL@actor?9type?5?$CFs?5?$AA@ (0000)
00246A80 0072:
	??_C@_0HC@JKKAFOMD@cannot?5add?5grenades?5or?5non?9power@ (0000)
00246AF4 0036:
	??_C@_0DG@HDAOIBGG@swarm?9?$DOunit_count?5?$DM?5MAXIMUM_NUMB@ (0000)
00246B2C 002b:
	??_C@_0CL@CPOHAIKA@actor_index?5?$CB?$DN?5global_updating_a@ (0000)
00246B58 0034:
	??_C@_0DE@EGNJPMD@unable?5to?5create?5any?5more?5swarm?5@ (0000)
00246B90 0042:
	??_C@_0EC@PMEGKNAL@actor?9?$DOmeta?4swarm_unit_count?5?$DM?$DN?5@ (0000)
00246BD4 002e:
	??_C@_0CO@DAKJODHF@exceeded?5MAXIMUM_NUMBER_OF_ACTIV@ (0000)
00246C04 000b:
	??_C@_0L@IJDNIGGG@run_vector?$AA@ (0000)
00246C10 0025:
	??_C@_0CF@EMPOOLED@actor?9?$DOmeta?4swarm_unit_index?5?$DN?$DN?5@ (0000)
00246C38 0012:
	??_C@_0BC@OPFCJMCE@actor?9?$DOmeta?4swarm?$AA@ (0000)
00246C4C 003c:
	??_C@_0DM@ICAEGPLB@swarm?5tried?5to?5become?5active?5but@ (0000)
00246C88 0035:
	??_C@_0DF@HONDOMG@swarm_first_unit?9?$DOunit?4swarm_pre@ (0000)
00246CC0 0041:
	??_C@_0EB@KMMOKFOD@actor?9?$DOmeta?4swarm_unit_count?5?$DM?5M@ (0000)
00246D04 0025:
	??_C@_0CF@OBGEKIIO@unit?9?$DOunit?4swarm_actor_index?5?$DN?$DN?5@ (0000)
00246D2C 001f:
	??_C@_0BP@PKBDPEJ@unit?9?$DOunit?4actor_index?5?$DN?$DN?5NONE?$AA@ (0000)
00246D4C 001f:
	??_C@_0BP@NOLNBLLP@actor?9?$DOmeta?4unit_index?5?$DN?$DN?5NONE?$AA@ (0000)
00246D70 0046:
	??_C@_0EG@IJNEHKGD@tried?5to?5update?5a?5swarm?5actor?5wi@ (0000)
00246DB8 002c:
	??_C@_0CM@CIODPINB@realcmp?$CIactor?9?$DOinput?4facing_vect@ (0000)
00246DE4 002e:
	??_C@_0CO@NNCPOMJ@?$CIreal_vector2d?5?$CK?$CJ?5?$CGactor?9?$DOinput?4@ (0000)
00246E14 001d:
	??_C@_0BN@LPPNLGMB@?$CGactor?9?$DOinput?4looking_vector?$AA@ (0000)
00246E34 001c:
	??_C@_0BM@FKDFBMGA@?$CGactor?9?$DOinput?4aiming_vector?$AA@ (0000)
00246E50 004d:
	??_C@_0EN@IKDAGGPK@actor?5?$CFs?5is?5not?5flying?5but?5was?5p@ (0000)
00246EA0 004d:
	??_C@_0EN@LFGKGCAN@actor?5?$CFs?5is?5flying?5but?5was?5place@ (0000)
00246EF0 0004:
	__real@42200000 (0000)
00246EF8 007e:
	??_C@_0HO@FHPPIAO@?$CIfabs?$CIcontrol_data?4throttle?4i?$CJ?5?$DM@ (0000)
00246F78 0008:
	__real@3ff0000000000000 (0000)
00246F80 001d:
	??_C@_0BN@KECJEOEE@?$CGcontrol_data?4looking_vector?$AA@ (0000)
00246FA0 001c:
	??_C@_0BM@IPGIOCDF@?$CGcontrol_data?4aiming_vector?$AA@ (0000)
00246FBC 001c:
	??_C@_0BM@FJJHEIIG@?$CGcontrol_data?4facing_vector?$AA@ (0000)
00246FD8 002e:
	??_C@_0CO@NIIPAAJM@?$CIreal_vector2d?5?$CK?$CJ?5?$CGcontrol_data?4@ (0000)
00247008 0044:
	??_C@_0EE@LFBDCPIL@?$CFs?5actor?5variant?5?$CFs?5cannot?5have?5@ (0000)
0024704C 0023:
	??_C@_0CD@BMFCNAIM@WARNING?3?5cannot?5create?5actor?5?$CFs?5@ (0000)
00247070 002c:
	??_C@_0CM@DONKLMPF@WARNING?3?5cannot?5create?5unit?5for?5@ (0000)
0024709C 0010:
	??_C@_0BA@NEIKIEMF@?$DMencounterless?$DO?$AA@ (0000)
002470AC 0027:
	??_C@_0CH@HGDOJFAF@actor_variant_definition_index?5?$CB@ (0000)
002470D4 0012:
	??_C@_0BC@CHJAJEBN@starting_location?$AA@ (0000)
002470E8 0035:
	??_C@_0DF@LOENPPFC@WARNING?3?5cannot?5create?5actor?5to?5@ (0000)
002B6CE8 0004:
	_global_updating_actor_index (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "actors.h"
#include "actor_definitions.h"
#include "actor_iterators.h"
#include "actor_types.h"
#include "props.h"

#include "saved games/game_state.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_actor_mode_braindead = 0,
	_actor_mode_alert = 2,
};

/* ---------- macros */

/* ---------- structures */

struct actor_iterator
{
	struct data_iterator encounter_iterator;
	boolean iterated_encounterless_list;
	boolean active_only;
	byte pad[2];
	long index;
	long next_index;
};

typedef char actor_iterator_size_assert[
	sizeof(struct actor_iterator) == 0x1C ? 1 : -1];
typedef char actor_iterator_index_offset_assert[
	offsetof(struct actor_iterator, index) == 0x14 ? 1 : -1];

typedef char actor_datum_output_control_flags_offset_assert[
	offsetof(struct actor_datum, output.control_flags) == 0x6D0 ? 1 : -1];
typedef char actor_datum_output_animation_impulse_offset_assert[
	offsetof(struct actor_datum, output.animation.impulse) == 0x6EC ? 1 : -1];
typedef char actor_datum_meta_encounterless_offset_assert[
	offsetof(struct actor_datum, meta.encounterless) == 0x09 ? 1 : -1];
typedef char actor_datum_meta_encounter_index_offset_assert[
	offsetof(struct actor_datum, meta.encounter_index) == 0x34 ? 1 : -1];
typedef char actor_datum_meta_first_prop_index_offset_assert[
	offsetof(struct actor_datum, meta.first_prop_index) == 0x50 ? 1 : -1];

/* ---------- prototypes */

void actor_switch_props(
	long actor_index,
	long old_prop_index,
	long new_prop_index);

void prop_delete(
	long actor_index,
	long prop_index);

void encounter_detach_actor(
	long actor_index,
	boolean died);

void encounterless_attach_actor(
	long actor_index);

void encounterless_detach_actor(
	long actor_index);

void encounter_attach_actor(
	long actor_index,
	long encounter_index,
	short squad_index,
	boolean has_previous_team);

void code_00029e70(
	long actor_index);

void actor_set_dormant(
	long actor_index,
	boolean dormant);

/* ---------- globals */

struct data_array *swarm_data = NULL;
struct data_array *swarm_component_data = NULL;
struct data_array *actor_data = NULL;
long global_updating_actor_index = NONE;

/* ---------- public code */

void actors_initialize(
	void)
{
	actor_types_initialize();
	actor_data = game_state_data_new("actor", MAXIMUM_ACTORS, sizeof(struct actor_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\actors.c", 121, actor_data);
	swarm_data = game_state_data_new("swarm", MAXIMUM_SWARMS, sizeof(struct swarm_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\actors.c", 124, swarm_data);
	swarm_component_data = game_state_data_new(
		"swarm component",
		MAXIMUM_SWARM_COMPONENTS,
		SWARM_COMPONENT_DATUM_SIZE);
	match_assert("c:\\halo\\SOURCE\\ai\\actors.c", 127, swarm_component_data);

	return;
}

void actors_dispose(
	void)
{
	return;
}

void actors_initialize_for_new_map(
	void)
{
	data_make_valid(actor_data);
	data_make_valid(swarm_data);
	data_make_valid(swarm_component_data);

	return;
}

void actors_dispose_from_old_map(
	void)
{
	data_make_invalid(actor_data);
	data_make_invalid(swarm_data);
	data_make_invalid(swarm_component_data);

	return;
}

boolean actor_has_unlimited_grenades(
	long actor_index)
{
	return TRUE;
}

boolean actor_is_noncombat(
	long actor_index)
{
	boolean result = actor_get(actor_index)->state.mode < 3;

	return result;
}

boolean actor_in_combat(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->state.mode == 3 && actor->state.combat_status > actor->state.artificial_combat_status)
	{
		return TRUE;
	}

	return FALSE;
}

boolean actor_is_fighting(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = actor->state.combat_status >= 7;

	if (result &&
		actor->state.action == _actor_action_flee &&
		actor->state.action_data.flee.panic_type > 0)
	{
		result = FALSE;
	}

	return result;
}

boolean actor_is_leaping(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	if (actor->state.action == 10)
	{
		result = action_charge_is_leaping(actor_index);
	}

	return result;
}

long actor_get_weapon(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	long result = NONE;

	if (actor->input.vehicle_gunner && actor->input.vehicle_index != NONE)
	{
		struct unit_datum *unit = unit_get(actor->input.vehicle_index);

		result = unit_inventory_get_weapon(
			actor->input.vehicle_index,
			(word)unit->unit.current_weapon_index);
	}

	if (result == NONE && actor->meta.unit_index != NONE)
	{
		struct actor_variant_definition *actor_variant_definition =
			actor_variant_definition_get(actor->meta.variant_definition_index);

		if (!TEST_FLAG(
			actor_variant_definition->flags,
			_actor_variant_definition_cannot_use_ranged_weapons_bit))
		{
			struct unit_datum *unit = unit_get(actor->meta.unit_index);

			result = unit_inventory_get_weapon(
				actor->meta.unit_index,
				unit->unit.current_weapon_index);
		}
	}

	return result;
}

boolean actor_has_ranged_weapon(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = actor_get_weapon(actor_index) != NONE;

	if (result && actor->meta.unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(actor->meta.unit_index);

		if (TEST_FLAG(unit->object.damage_flags, _object_ranged_attack_inhibited_bit))
		{
			result = FALSE;
		}
	}

	return result;
}

long actor_target_unit_index(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	long result = NONE;

	if (actor->target.target_prop_index != NONE)
	{
		result = prop_get(actor->target.target_prop_index)->unit_index;
	}

	return result;
}

void actor_derive_target_information(
	long actor_index,
	long target_actor_index)
{
	struct actor_datum *target_actor = actor_get(target_actor_index);
	long target_prop_index = target_actor->target.target_prop_index;

	if (target_prop_index != NONE)
	{
		struct prop_datum *target_prop = prop_get(target_prop_index);

		actor_perception_create_orphan_from_friend(
			actor_index,
			target_prop->unit_index,
			target_actor_index,
			target_prop_index);
	}

	return;
}

void actor_flush_position_indices(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->firing_positions.current_position_index = NONE;

	if (actor->control.path.destination_orders.destination_type == _actor_destination_firing_position ||
		actor->control.path.destination_orders.destination_type == _actor_destination_move_position)
	{
		actor->control.path.destination_orders.destination_type = _actor_destination_none;
		actor->control.path.destination_orders.ignore_target_object_index = NONE;
	}

	actor_action_flush_position_indices(actor_index);

	return;
}

void actor_change_encounter(
	long actor_index,
	long encounter_index,
	short squad_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor_flush_position_indices(actor_index);

	if (actor->meta.encounterless)
	{
		encounterless_detach_actor(actor_index);
	}
	else if (actor->meta.encounter_index != NONE)
	{
		encounter_detach_actor(actor_index, FALSE);
	}

	if (encounter_index == NONE)
	{
		encounterless_attach_actor(actor_index);
	}
	else
	{
		encounter_attach_actor(actor_index, encounter_index, squad_index, TRUE);
	}

	return;
}

void code_0002a150(
	void)
{
	global_updating_actor_index = NONE;

	return;
}

void actor_unit_control_crouch(
	long actor_index,
	boolean crouch)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (crouch)
		actor->output.control_flags |= FLAG(_unit_control_crouch_modifier_bit);
	else
		actor->output.control_flags &= ~FLAG(_unit_control_crouch_modifier_bit);

	return;
}

void actor_unit_control_jump(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->output.control_flags |= FLAG(_unit_control_jump_bit);

	return;
}

void actor_unit_control_primary_trigger(
	long actor_index,
	boolean firing,
	real analog_primary_trigger)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (firing)
		actor->output.control_flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
	else
		actor->output.control_flags &= ~FLAG(_unit_control_weapon_primary_trigger_bit);
	actor->output.analog_primary_trigger = analog_primary_trigger;

	return;
}

void actor_unit_control_secondary_trigger(
	long actor_index,
	boolean firing)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (firing)
		actor->output.control_flags |= FLAG(_unit_control_weapon_secondary_trigger_bit);
	else
		actor->output.control_flags &= ~FLAG(_unit_control_weapon_secondary_trigger_bit);

	return;
}

void actor_unit_control_throw_grenade(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->output.control_flags |= FLAG(_unit_control_throw_grenade_bit);

	return;
}

void actor_unit_control_exact_facing(
	long actor_index,
	boolean exact_facing)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (exact_facing)
		actor->output.control_flags |= FLAG(_unit_control_exact_facing_bit);
	else
		actor->output.control_flags &= ~FLAG(_unit_control_exact_facing_bit);

	return;
}

void actor_unit_control_stop_animation_impulse(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->output.animation.impulse = NONE;

	return;
}

void actor_delete_props(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	while (actor->meta.first_prop_index != NONE)
	{
		actor_switch_props(actor_index, actor->meta.first_prop_index, NONE);
		prop_delete(actor_index, actor->meta.first_prop_index);
	}

	return;
}

void actor_braindead(
	long actor_index,
	boolean braindead)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (braindead)
	{
		actor->state.mode = _actor_mode_braindead;
		actor->state.action = _actor_action_none;
		actor_delete_props(actor_index);
		code_00029e70(actor_index);
		actor_set_dormant(actor_index, FALSE);
	}
	else if (actor->state.mode == _actor_mode_braindead)
	{
		actor->state.mode = _actor_mode_alert;
	}

	return;
}

void actors_freeze(
	void)
{
	struct actor_iterator iterator;

	actor_iterator_new(&iterator, TRUE);
	while (actor_iterator_next(&iterator))
	{
		code_00029e70(iterator.index);
	}

	return;
}

/* ---------- private code */
