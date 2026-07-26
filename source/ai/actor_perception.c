/*
ACTOR_PERCEPTION.C

symbols in this file:
0001D8C0 00d0:
	_actor_perception_acknowledge (0000)
0001D990 00f0:
	_actor_get_perception_knowledge (0000)
0001DA80 0140:
	_actor_get_vision_distances (0000)
0001DBC0 0040:
	_code_0001dbc0 (0000)
0001DC00 00f0:
	_code_0001dc00 (0000)
0001DCF0 0230:
	_actor_perception_desire_prop (0000)
0001DF20 00a0:
	_actor_perception_find_prop_pathfinding_location (0000)
0001DFC0 00c0:
	_actor_perception_find_killer_prop_index (0000)
0001E080 0010:
	_arctangent (0000)
0001E090 00f0:
	_actor_perception_find_recent_damaging_prop_index (0000)
0001E180 0050:
	_actor_perception_forget_recent_damage (0000)
0001E1D0 0060:
	_actor_perception_retreat_successful (0000)
0001E230 00f0:
	_actor_compute_prop_unopposable (0000)
0001E320 03a0:
	_actor_compute_prop_target_weight (0000)
0001E6C0 0200:
	_actor_situation_update_target_status (0000)
0001E8C0 0140:
	_actor_situation_combat_status_update (0000)
0001EA00 04f0:
	_actor_situation_update (0000)
0001EEF0 00f0:
	_actor_situation_try_new_target (0000)
0001EFE0 01b0:
	_actor_perception_friend_prop_is_attacking (0000)
0001F190 0190:
	_actor_perception_aiming_vector_test_blockage (0000)
0001F320 0150:
	_actor_emotion_flee_with_friends (0000)
0001F470 0080:
	_code_0001f470 (0000)
0001F4F0 0070:
	_code_0001f4f0 (0000)
0001F560 04f0:
	_code_0001f560 (0000)
0001FA50 00b0:
	_actor_berserk (0000)
0001FB00 0360:
	_actor_visibility_at_point (0000)
0001FE60 0240:
	_actor_audibility_at_point (0000)
000200A0 0170:
	_actor_perception_find_sense_position (0000)
00020210 01f0:
	_code_00020210 (0000)
00020400 0380:
	_prop_position_refresh (0000)
00020780 0210:
	_code_00020780 (0000)
00020990 05c0:
	_code_00020990 (0000)
00020F50 0180:
	_actor_expected_acknowledgement (0000)
000210D0 0090:
	_actor_perception_unreachable (0000)
00021160 0060:
	_actor_perception_tried_to_uncover (0000)
000211C0 0060:
	_actor_perception_tried_to_search (0000)
00021220 00a0:
	_actor_perception_abandoned_search (0000)
000212C0 0680:
	_actor_emotion_update (0000)
00021940 0110:
	_actor_perception_become_acknowledged (0000)
00021A50 0e60:
	_prop_status_refresh (0000)
000228B0 06d0:
	_code_000228b0 (0000)
00022F80 0310:
	_actor_perception_create_orphan_from_friend (0000)
00023290 0970:
	_code_00023290 (0000)
00023C00 1270:
	_actor_perception_update (0000)
00245AB8 0038:
	_global_combat_status_table (0000)
	_global_acknowledgement_speeds (0018)
00245AF0 0020:
	??_C@_0CA@OIEKNKJL@prop?9?$DOorphan_prop_index?5?$DN?$DN?5NONE?$AA@ (0000)
00245B10 0018:
	??_C@_0BI@EKGDDPPJ@prop_acknowledged?$CIprop?$CJ?$AA@ (0000)
00245B28 0027:
	??_C@_0CH@HJCPLECH@prop?9?$DOowner_actor_index?5?$DN?$DN?5actor@ (0000)
00245B50 0025:
	??_C@_0CF@BOOBPIOF@c?3?2halo?2SOURCE?2ai?2actor_percepti@ (0000)
00245B78 0004:
	__real@42100000 (0000)
00245B7C 0004:
	__real@43610000 (0000)
00245B80 0004:
	__real@44c80000 (0000)
00245B84 0022:
	??_C@_0CC@ECCIDOIG@damaging_prop_index?5?$CB?$DN?50x0000000@ (0000)
00245BA8 0014:
	??_C@_0BE@JCINMMJG@prop_orphaned?$CIprop?$CJ?$AA@ (0000)
00245BBC 0013:
	??_C@_0BD@GACNHPJP@target_prop?9?$DOenemy?$AA@ (0000)
00245BD0 005f:
	??_C@_0FP@LFCAOKBD@?$CIactor?9?$DOtarget?4target_type?5?$DO?$DN?50?$CJ@ (0000)
00245C30 003a:
	??_C@_0DK@BNBOMGMC@?$CIactor_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIactor_type@ (0000)
00245C6C 0010:
	??_C@_0BA@NODDFKHB@new_prop?9?$DOenemy?$AA@ (0000)
00245C80 004c:
	??_C@_0EM@HGNPMJBO@prop_acknowledged?$CIfriend_prop?$CJ?5?$CG@ (0000)
00245CCC 0004:
	__real@bf4ccccd (0000)
00245CD0 0004:
	__real@3f9ae148 (0000)
00245CD4 0004:
	__real@3eb851ec (0000)
00245CD8 0004:
	__real@bf490fdb (0000)
00245CDC 0004:
	__real@3f060a92 (0000)
00245CE0 0004:
	__real@3e19999a (0000)
00245CE4 0004:
	__real@3fd55555 (0000)
00245CE8 0004:
	__real@40010204 (0000)
00245CEC 0004:
	__real@3ee66666 (0000)
00245CF0 0018:
	??_C@_0BI@GICPJHMO@best_unit_index?5?$CB?$DN?5NONE?$AA@ (0000)
00245D08 0025:
	??_C@_0CF@GHOKMPCH@actor?9?$DOmeta?4swarm_unit_index?5?$CB?$DN?5@ (0000)
00245D30 0021:
	??_C@_0CB@MPKGJDOK@actor?9?$DOmeta?4swarm_unit_count?5?$DO?50@ (0000)
00245D54 003b:
	??_C@_0DL@MCFLLIBG@?$CIexisting_unit_index?5?$DN?$DN?5NONE?$CJ?5?$HM?$HM@ (0000)
00245D90 0004:
	__real@40100000 (0000)
00245D94 0018:
	??_C@_0BI@LJGDFJPJ@swarm_actor?9?$DOmeta?4swarm?$AA@ (0000)
00245DAC 0004:
	__real@3a91a2b4 (0000)
00245DB0 002f:
	??_C@_0CP@MPBEBNC@object?9?$DOobject?4type?5?$DN?$DN?5_object_t@ (0000)
00245DE0 0004:
	__real@383a69dc (0000)
00245DE8 00c6:
	??_C@_0MG@IKFAABEC@?$CIactor?9?$DOdanger_zone?4danger_type?5@ (0000)
00245EB0 0008:
	__real@3ff8000000000000 (0000)
00245EB8 0015:
	??_C@_0BF@NEEODENG@?$CBprop_orphaned?$CIprop?$CJ?$AA@ (0000)
00245ED0 0008:
	__real@bfa7a8d000000000 (0000)
00245ED8 0004:
	__real@3f680347 (0000)
00245EDC 0004:
	__real@3f7e147b (0000)
00245EE0 0004:
	__real@bb5a740e (0000)
00245EE4 0004:
	__real@bc888889 (0000)
00245EE8 0004:
	__real@bd088889 (0000)
00245EEC 0004:
	__real@3c888889 (0000)
00245EF0 0004:
	__real@3b5a740e (0000)
00245EF4 0006:
	??_C@_05KKADFBMH@?$CBdead?$AA@ (0000)
00245EFC 0038:
	??_C@_0DI@GEDMEBOM@current_orphan?9?$DOparent_prop_inde@ (0000)
00245F34 0038:
	??_C@_0DI@CJNILOBC@current_prop?9?$DOorphan_prop_index?5@ (0000)
00245F6C 0031:
	??_C@_0DB@FLDFMNOK@current_orphan?9?$DOowner_actor_inde@ (0000)
00245FA0 002f:
	??_C@_0CP@JODIIBNI@current_prop?9?$DOowner_actor_index?5@ (0000)
00245FD0 001e:
	??_C@_0BO@PHJOMEBD@prop_orphaned?$CIcurrent_orphan?$CJ?$AA@ (0000)
00245FF0 0022:
	??_C@_0CC@OGFEAFJH@prop_unacknowledged?$CIcurrent_prop@ (0000)
00246014 003c:
	??_C@_0DM@LBOGCEIF@actor_perception_refresh?5overflo@ (0000)
00246050 0008:
	??_C@_07BJLCCHPO@friends?$AA@ (0000)
00246058 0008:
	??_C@_07OMOEAPJD@enemies?$AA@ (0000)
00246060 0004:
	__real@3f31c71c (0000)
00246064 000c:
	??_C@_0M@JMHOJBDN@?$CBprop?9?$DOdead?$AA@ (0000)
00246070 0020:
	??_C@_0CA@JNPJKEOJ@prop?9?$DOparent_prop_index?5?$CB?$DN?5NONE?$AA@ (0000)
00246090 0017:
	??_C@_0BH@IAHDHPJP@new_state?$CB?$DNprop?9?$DOstate?$AA@ (0000)
002460A8 0012:
	??_C@_0BC@COANDKJL@?$CFs?3?5become?5aware?$CB?$AA@ (0000)
002460C0 0041:
	??_C@_0EB@HOGHONEE@?5?5awareness?5delta?3?5?$CF?42f?5?$CIcurrent@ (0000)
00246104 002b:
	??_C@_0CL@DNKONGGM@?$CFs?3?5knowledge?5?$CFs?5percep?5?$CFs?5?9?$DO?5aw@ (0000)
00246130 000d:
	??_C@_0N@OCHNCJBI@unmistakable?$AA@ (0000)
00246140 0005:
	??_C@_04PLMLMMEO@full?$AA@ (0000)
00246148 0008:
	??_C@_07JHIHCBKH@partial?$AA@ (0000)
00246150 0009:
	??_C@_08GMLBJMKA@definite?$AA@ (0000)
0024615C 000a:
	??_C@_09HJLAOOGF@searching?$AA@ (0000)
00246168 0008:
	??_C@_07IFJEFPGA@instant?$AA@ (0000)
00246170 0006:
	??_C@_05DAFACFLE@never?$AA@ (0000)
00246178 0051:
	??_C@_0FB@GKMFLAFD@?$CIprop?9?$DOperception?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIprop@ (0000)
002461D0 004c:
	??_C@_0EM@LBLHCEME@?$CIknowledge_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIknowle@ (0000)
0024621C 0018:
	??_C@_0BI@PALENGAG@?$CFs?3?5stop?5becoming?5aware?$AA@ (0000)
00246234 001a:
	??_C@_0BK@OHGMLFML@?$CFs?3?5start?5to?5become?5aware?$AA@ (0000)
00246250 0024:
	??_C@_0CE@DMMCLDJG@?$CBrefresh_status?5?$HM?$HM?5refresh_posit@ (0000)
00246274 0031:
	??_C@_0DB@OOOKPDMG@parent_prop?9?$DOorphan_prop_index?5?$DN@ (0000)
002462A8 002b:
	??_C@_0CL@GMGBPAA@prop?9?$DOunopposable_casualties_inf@ (0000)
002B6AE0 0004:
	_data_002b6ae0 (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "actor_definitions.h"
#include "actors.h"
#include "props.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/*
 * January actor/prop fields whose HCEX-derived shared structure positions do
 * not agree with this executable. Keep the executable-specific view local
 * until the complete January layouts are recovered.
 */
struct actor_perception_actor_view
{
	byte __unknown000[4];
	short team_index;
	byte __unknown006[0x12];
	long unit_index;
	byte __unknown01C[0x4E];
	short combat_status;
	short friend_state;
	short artificial_combat_status;
	byte __unknown070[0x38];
	short friend_fighting_count;
	byte __unknown0AA[0xB7];
	boolean flying;
	byte __unknown162[0x8B];
	boolean searching;
	byte __unknown1EE[0x14];
	boolean vehicle_passenger;
	byte __unknown203[0x65];
	short target_type;
	byte __unknown26A[2];
	long target_unit_index;
	long target_prop_index;
	byte __unknown274[8];
	boolean target_outside_active_area;
	byte __unknown27D[0x8B];
	short active_threat_count;
	byte __unknown30A[0x9E];
	short unopposable_retreat_timer;
	byte __unknown3AA[2];
	long unopposable_retreat_prop_index;
};

struct actor_perception_prop_view
{
	byte __unknown000[4];
	long owner_actor_index;
	byte __unknown008[4];
	long orphan_prop_index;
	short type;
	byte __unknown012[6];
	long unit_index;
	long actor_index;
	byte __unknown020[4];
	short state;
	byte __unknown026[0x2A];
	long copied_position_data[4];
	boolean enemy;
	byte __unknown061[5];
	short perception;
	byte __unknown068[0x34];
	short unreachable_ticks;
	byte __unknown09E[2];
	long last_unreachable_time;
	boolean unopposable_enemy;
	byte __unknown0A5[1];
	short unopposable_casualties_inflicted;
	short unopposable_casualty_decay_timer;
	short unopposable_trigger_hysteresis;
	short unopposable_trigger_timer;
	short unopposable_trigger_threshold;
	byte __unknown0B0[8];
	boolean orphan_corpse_cheated;
	byte __unknown0B9[3];
	real_point3d body_position;
	real_point3d center_of_mass;
	real_vector3d velocity;
	real_vector3d actor_to_prop;
	long pathfinding_surface_index;
	real_point3d pathfinding_point;
	byte __unknown0FC[0x14];
	long vehicle_index;
	byte __unknown114[0x13];
	boolean dead;
	byte __unknown128[4];
	boolean fleeing;
	byte __unknown12D[8];
	boolean dangerous_vehicle_driver;
	boolean preferred_target;
};

struct actor_perception_swarm_component_view
{
	short identifier;
	word flags;
	real_point3d position;
};

typedef char actor_perception_actor_view_target_prop_index_offset_assert[
	offsetof(struct actor_perception_actor_view, target_prop_index) == 0x270 ? 1 : -1];
typedef char actor_perception_prop_view_unopposable_enemy_offset_assert[
	offsetof(struct actor_perception_prop_view, unopposable_enemy) == 0xA4 ? 1 : -1];
typedef char actor_perception_actor_view_active_threat_count_offset_assert[
	offsetof(struct actor_perception_actor_view, active_threat_count) == 0x308 ? 1 : -1];
typedef char actor_perception_prop_view_fleeing_offset_assert[
	offsetof(struct actor_perception_prop_view, fleeing) == 0x12C ? 1 : -1];
typedef char actor_perception_swarm_component_position_offset_assert[
	offsetof(struct actor_perception_swarm_component_view, position) == 4 ? 1 : -1];

union actor_perception_boolean_slot
{
	boolean value;
	long storage;
};

/*
 * Runtime perception values in the January actor definition. The shared
 * HCEX-derived definition still labels two of these slots as unused.
 */
struct actor_perception_definition_view
{
	byte __unknown000[0x1C];
	real maximum_vision_angle;
	real central_vision_angle;
	byte __unknown024[4];
	real peripheral_distance;
	real maximum_peripheral_distance;
};

/* ---------- prototypes */

void actor_situation_update_target_status(
	long actor_index);

void actor_situation_combat_status_update(
	long actor_index);

real actor_compute_prop_target_weight(
	long actor_index,
	long prop_index);

boolean actor_compute_prop_unopposable(
	long actor_index,
	long prop_index);

void actor_stimulus_prop_acknowledged(
	long actor_index,
	long prop_index,
	long stimulus,
	boolean initial_acknowledgement);

long ai_get_responsible_unit(
	long object_index,
	boolean include_self);

long vehicle_find_pathfinding_surface_index(
	long vehicle_index,
	real_point3d *position);

long biped_find_pathfinding_surface_index(
	long biped_index,
	real_point3d *position);

boolean actor_expected_acknowledgement(
	long actor_index,
	long prop_index);

void actor_switch_props(
	long actor_index,
	long old_prop_index,
	long new_prop_index);

void actor_input_sample_position(
	long actor_index,
	long unit_index,
	struct actor_position_data *position);

void prop_delete(
	long actor_index,
	long prop_index);

static __inline boolean prop_acknowledged(
	struct prop_datum const *prop)
{
	return prop->state >= _prop_state_becoming_unacknowledged &&
		prop->state <= _prop_state_acknowledged;
}

/* ---------- globals */

/* ---------- public code */

void actor_perception_acknowledge(
	long actor_index,
	long prop_index,
	long stimulus,
	boolean initial_acknowledgement)
{
	struct prop_datum *prop = prop_get(prop_index);

#line 1037 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	assert(prop->owner_actor_index == actor_index);
	assert(prop_acknowledged(prop));
#line 1039 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	vassert(prop->orphan_prop_index == NONE, "prop->orphan_prop_index == NONE");
#line 300 "source\\ai\\actor_perception.c"

	prop->tried_to_search = FALSE;
	prop->tried_to_uncover = FALSE;
	prop->abandoned_search = FALSE;
	prop->refresh_stimuli = TRUE;

	actor_stimulus_prop_acknowledged(
		actor_index,
		prop_index,
		stimulus,
		initial_acknowledgement);

	return;
}

long actor_get_perception_knowledge(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);

	if (prop_index != NONE)
	{
		struct actor_perception_prop_view *prop =
			(struct actor_perception_prop_view *)prop_get(prop_index);

#line 1394 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(prop->owner_actor_index == actor_index);
#line 390 "source\\ai\\actor_perception.c"

		if (prop->state >= 2 && prop->state <= 3)
			return 3;

		if (prop->perception == 1 || prop->perception == 2)
			return 3;

		if (!prop->enemy &&
			(!prop->dead || actor->combat_status >= 3))
		{
			return 3;
		}

		if (prop->orphan_prop_index != NONE)
		{
			struct actor_perception_prop_view *orphan =
				(struct actor_perception_prop_view *)prop_get(
					prop->orphan_prop_index);
			long result = (orphan->orphan_corpse_cheated != FALSE) + 2;

			if ((short)result != NONE)
				return result;
		}
	}

	if (actor->artificial_combat_status >= 2)
		return 2;

	return actor->combat_status >= 3;
}

void actor_get_vision_distances(
	long actor_index,
	real maximum_vision_distance,
	real perception_factor,
	real horizontal_angle,
	real *full_distance_reference,
	real *partial_distance_reference)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_perception_definition_view const *definition =
		(struct actor_perception_definition_view const *)
			actor_definition_get(actor->meta.definition_index);
	real full_distance;
	real partial_distance;

	if (horizontal_angle > definition->peripheral_distance)
	{
		partial_distance = 0.0f;
		full_distance = 0.0f;
	}
	else
	{
		real maximum_distance =
			maximum_vision_distance * perception_factor;
		real peripheral_distance =
			perception_factor * definition->maximum_peripheral_distance;
		real minimum_full_distance = 0.7f * maximum_distance;
		real minimum_partial_distance = 0.7f * peripheral_distance;

		if (minimum_partial_distance > 3.5f)
			minimum_partial_distance = 3.5f;

		if (horizontal_angle > definition->central_vision_angle)
		{
			partial_distance = peripheral_distance;
			full_distance = minimum_partial_distance;
		}
		else
		{
			real maximum_vision_angle =
				definition->maximum_vision_angle;
			real full_vision_angle = 0.8f * maximum_vision_angle;

			if (horizontal_angle < maximum_vision_angle)
			{
				partial_distance = maximum_distance;
			}
			else
			{
				real interpolation =
					(horizontal_angle - maximum_vision_angle) /
					(definition->central_vision_angle -
						maximum_vision_angle);

				partial_distance =
					(1.0f - interpolation) * maximum_distance +
					peripheral_distance * interpolation;
			}

			if (horizontal_angle < full_vision_angle)
			{
				full_distance = minimum_full_distance;
			}
			else
			{
				real interpolation =
					(horizontal_angle - full_vision_angle) /
					(definition->central_vision_angle -
						full_vision_angle);

				full_distance =
					(1.0f - interpolation) *
						minimum_full_distance +
					minimum_partial_distance * interpolation;
			}
		}
	}

	*partial_distance_reference = partial_distance;
	*full_distance_reference = full_distance;

	return;
}

void actor_situation_update_target_status(
	long actor_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);

	if (actor->target_prop_index == NONE)
	{
		actor->target_type = 0;
		actor->target_unit_index = NONE;
		actor->target_outside_active_area = FALSE;
	}
	else
	{
		struct actor_perception_prop_view *target_prop =
			(struct actor_perception_prop_view *)prop_get(
				actor->target_prop_index);
		struct unit_datum *target_unit =
			unit_get(target_prop->unit_index);
		short target_type;

#line 4291 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(target_prop->enemy);
#line 510 "source\\ai\\actor_perception.c"

		switch (target_prop->state)
		{
		case _prop_state_unacknowledged:
			target_type = 0;
			actor->target_prop_index = NONE;
			actor->target_unit_index = NONE;
			break;

		case _prop_state_becoming_acknowledged:
			target_type = 1;
			break;

		case _prop_state_becoming_unacknowledged:
		case _prop_state_acknowledged:
			if (*((boolean *)target_prop + 0x127))
				target_type = 2;
			else if (*((boolean *)target_prop + 0x74))
				target_type = 11;
			else if (*(short *)((byte *)target_prop + 0x32) >= 2)
				target_type = 10;
			else if (*(short *)((byte *)target_prop + 0x38) != 0 &&
				*(short *)((byte *)target_prop + 0x38) != 1)
			{
				target_type = 7;
			}
			else if (*((char *)target_prop + 0x122) <= 2 &&
				*(real *)((byte *)target_prop + 0x11C) < 6.f)
			{
				target_type = 9;
			}
			else
			{
				target_type = 8;
			}
			break;

		case _prop_state_inspected_orphan:
			if (*((boolean *)target_prop + 0x127))
				target_type = 2;
			else
				target_type =
					*((boolean *)target_prop + 0xBB) ? 3 : 4;
			break;

		case _prop_state_uninspected_orphan:
			target_type =
				(*((boolean *)target_prop + 0xB8) != FALSE) + 5;
			break;

		default:
			display_assert(
				NULL,
				"c:\\halo\\SOURCE\\ai\\actor_perception.c",
				4362,
				TRUE);
			system_exit(-1);
			break;
		}

		actor->target_type = target_type;

		if (target_prop->state >=
				_prop_state_becoming_unacknowledged &&
			target_prop->state <= _prop_state_acknowledged)
		{
			actor->target_outside_active_area =
				!*((boolean *)target_prop + 0x127);

			if (*(short *)((byte *)target_prop + 0x32) > 0)
				actor->target_unit_index =
					*(long *)((byte *)target_prop + 0x8C);
		}
		else
		{
			actor->target_outside_active_area =
				!TEST_FLAG(
					*((byte *)target_unit + 0xB6),
					2);
		}
	}

	return;
}

void actor_berserk(
	long actor_index,
	boolean berserk)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (berserk != actor->emotions.berserk)
	{
		actor->emotions.berserk = berserk;
		actor->emotions.played_berserk_sound = FALSE;

		if (actor->meta.swarm)
		{
			long object_index;

			for (object_index = actor->meta.swarm_unit_index;
				object_index != NONE;)
			{
				struct unit_datum *unit = unit_get(object_index);

				SET_FLAG(
					unit->object.damage_flags,
					_object_melee_attack_inhibited_bit,
					TRUE);
				object_index = unit->unit.swarm_next_unit_index;
			}
		}
		else
		{
			struct unit_datum *unit = unit_get(actor->meta.unit_index);

			SET_FLAG(
				unit->unit.flags,
				7,
				berserk);
		}

		if (berserk)
			actor->emotions.forced_to_charge = TRUE;
	}

	return;
}

long actor_perception_find_killer_prop_index(
	long actor_index,
	long prop_index,
	boolean enemies_only)
{
	struct prop_datum *prop = prop_get(prop_index);
	struct unit_datum *unit = unit_get(prop->unit_index);
	long killer_prop_index = NONE;
	long most_recent_damage_time = 0;
	short attacker_index;

	for (attacker_index = 0;
		attacker_index < MAXIMUM_ATTACKERS_PER_UNIT;
		attacker_index++)
	{
		long *attacker_object_index =
			&unit->unit.attackers[attacker_index].object_index;
		long damage_time = attacker_object_index[-2];
		long unit_index =
			ai_get_responsible_unit(
				*attacker_object_index,
				TRUE);

		if (unit_index != NONE)
		{
			long current_prop_index =
				prop_get_active_by_unit_index(
					actor_index,
					unit_index);

			if (current_prop_index != NONE)
			{
				struct prop_datum *current_prop =
					prop_get(current_prop_index);

				if (current_prop->state >=
						_prop_state_becoming_unacknowledged &&
					current_prop->state <=
						_prop_state_acknowledged &&
					(current_prop->enemy || !enemies_only) &&
					damage_time > most_recent_damage_time)
				{
					killer_prop_index = current_prop_index;
					most_recent_damage_time = damage_time;
				}
			}
		}
	}

	return killer_prop_index;
}

long actor_perception_find_recent_damaging_prop_index(
	long actor_index,
	boolean enemies_only)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	long damaging_prop_index = NONE;

	if (actor->unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(actor->unit_index);
		unsigned long most_recent_damage_time = 0;
		short attacker_index;

		for (attacker_index = 0;
			attacker_index < MAXIMUM_ATTACKERS_PER_UNIT;
			attacker_index++)
		{
			struct unit_attacker *attacker =
				&unit->unit.attackers[attacker_index];
			long unit_index =
				ai_get_responsible_unit(
					attacker->object_index,
					TRUE);

			if (unit_index != NONE)
			{
				long current_prop_index =
					prop_get_active_by_unit_index(
						actor_index,
						unit_index);

				if (current_prop_index != NONE)
				{
					struct prop_datum *current_prop =
						prop_get(current_prop_index);

					if (current_prop->state >=
							_prop_state_becoming_unacknowledged &&
						current_prop->state <=
							_prop_state_acknowledged &&
						(current_prop->enemy || !enemies_only) &&
						attacker->game_time_stamp >
							most_recent_damage_time)
					{
						damaging_prop_index =
							current_prop_index;
						most_recent_damage_time =
							attacker->game_time_stamp;
					}
				}
			}

		}

#line 3726 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(damaging_prop_index != 0x00000000);
#line 500 "source\\ai\\actor_perception.c"
	}

	return damaging_prop_index;
}

void actor_perception_forget_recent_damage(
	long actor_index)
{
	struct prop_iterator iterator;
	struct prop_datum *prop;

	prop_iterator_new(&iterator, actor_index);
	prop = prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		prop->currently_damaging_me = FALSE;
		prop->ticks_since_damage = NONE;
		prop = prop_iterator_next(&iterator);
	}

	return;
}

void actor_perception_retreat_successful(
	long actor_index)
{
	struct prop_iterator iterator;
	struct prop_datum *prop;

	actor_get(actor_index);
	prop_iterator_new(&iterator, actor_index);
	prop = prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		prop->unopposable_trigger_hysteresis = 0;
		prop->unopposable_trigger_threshold = 0;
		prop->unopposable_trigger_timer = 0;
		prop = prop_iterator_next(&iterator);
	}

	return;
}

boolean actor_compute_prop_unopposable(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	short type = prop->state;
	boolean result = FALSE;

	if (type >= 2 && type <= 3 &&
		prop->enemy &&
		!prop->dead)
	{
		if (prop->unreachable_ticks != 0 &&
			(actor->target_prop_index == prop_index ||
				!actor->searching))
		{
			result = TRUE;
		}
		else if ((prop->dangerous_vehicle_driver ||
			prop->preferred_target) &&
			!actor->flying &&
			!actor->vehicle_passenger)
		{
			result = TRUE;
		}
		else if (prop->type == 15)
		{
			result = TRUE;
		}
	}

	if (prop->unopposable_enemy && !result)
	{
		prop->unopposable_trigger_hysteresis = 0;
		prop->unopposable_trigger_threshold = 0;
		prop->unopposable_trigger_timer = 0;
	}

	if (type >= 2 && type <= 3 &&
		!result &&
		actor->unopposable_retreat_timer > 0 &&
		actor->unopposable_retreat_prop_index == prop_index)
	{
		actor->unopposable_retreat_timer = 0;
		actor->unopposable_retreat_prop_index = NONE;
	}

	prop->unopposable_enemy = result;

	return result;
}

void actor_perception_find_prop_pathfinding_location(
	long actor_index,
	long prop_index)
{
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);

#line 3585 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	assert(prop->owner_actor_index == actor_index);
#line 510 "source\\ai\\actor_perception.c"

	if (prop->pathfinding_surface_index == NONE)
	{
		if (prop->vehicle_index != NONE)
		{
			prop->pathfinding_surface_index =
				vehicle_find_pathfinding_surface_index(
					prop->vehicle_index,
					&prop->pathfinding_point);
			return;
		}

		if (object_try_and_get_and_verify_type(
			prop->unit_index,
			_object_mask_biped))
		{
			prop->pathfinding_surface_index =
				biped_find_pathfinding_surface_index(
					prop->unit_index,
					&prop->pathfinding_point);
		}
	}

	return;
}

void actor_perception_tried_to_uncover(
	long actor_index,
	long prop_index)
{
	if (prop_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		prop->tried_to_uncover = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}

void actor_perception_tried_to_search(
	long actor_index,
	long prop_index)
{
	if (prop_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		prop->tried_to_search = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}

void actor_perception_abandoned_search(
	long actor_index,
	long prop_index)
{
	if (prop_index == NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);

		actor->firing_positions.pursuit_positions_count = 0;
		actor->firing_positions.pursuit_fired_at_orphan = FALSE;
		actor->firing_positions.pursuit_communicated_lost_contact = FALSE;
		actor->state.artificial_combat_status = 0;
		actor->state.suspicion_combat_status = 0;
		actor_situation_combat_status_update(actor_index);
	}
	else
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		if (prop->state == _prop_state_uninspected_orphan)
			prop->state = _prop_state_inspected_orphan;

		prop->abandoned_search = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}

void actor_perception_unreachable(
	long actor_index,
	long prop_index,
	boolean unreachable)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);

	if (unreachable)
	{
		if (prop->unreachable_ticks == 0)
			prop->unreachable_ticks = 1;

		prop->last_unreachable_time = game_time_get();
	}
	else
	{
		prop->unreachable_ticks = 0;
		prop->last_unreachable_time = NONE;
	}

	prop->unopposable_enemy =
		actor_compute_prop_unopposable(actor_index, prop_index);
	prop->target_weight =
		actor_compute_prop_target_weight(actor_index, prop_index);

	return;
}

boolean actor_perception_become_acknowledged(
	long actor_index,
	long prop_index,
	boolean *expected_acknowledgement_out)
{
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	boolean result = FALSE;
	boolean expected_acknowledgement = FALSE;

	if (prop->state < 2 || prop->state > 3)
	{
		union actor_perception_boolean_slot orphaned;
		union actor_perception_boolean_slot expected;

		orphaned.value = prop->orphan_prop_index != NONE;
		expected.value =
			actor_expected_acknowledgement(actor_index, prop_index);

		if (orphaned.value)
		{
			struct actor_perception_prop_view *orphan =
				(struct actor_perception_prop_view *)prop_get(
					prop->orphan_prop_index);

			prop->copied_position_data[0] = orphan->copied_position_data[0];
			prop->copied_position_data[1] = orphan->copied_position_data[1];
			prop->copied_position_data[2] = orphan->copied_position_data[2];
			prop->copied_position_data[3] = orphan->copied_position_data[3];
			prop->unreachable_ticks = orphan->unreachable_ticks;
			prop->last_unreachable_time = orphan->last_unreachable_time;
			prop->unopposable_enemy = orphan->unopposable_enemy;
			prop->unopposable_casualties_inflicted =
				orphan->unopposable_casualties_inflicted;
			prop->unopposable_casualty_decay_timer =
				orphan->unopposable_casualty_decay_timer;

			actor_switch_props(
				actor_index,
				prop->orphan_prop_index,
				prop_index);
			prop_delete(actor_index, prop->orphan_prop_index);
			prop->orphan_prop_index = NONE;
		}

		prop->state = 3;
			actor_perception_acknowledge(
			actor_index,
			prop_index,
			orphaned.storage,
			expected.storage);
		expected_acknowledgement = expected.value;
		result = TRUE;
	}

	if (expected_acknowledgement_out)
		*expected_acknowledgement_out = expected_acknowledgement;

	return result;
}

boolean actor_expected_acknowledgement(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	struct prop_iterator iterator;
	struct actor_perception_prop_view *current_prop;
	boolean result = FALSE;

#line 3613 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	match_vassert(
		__FILE__,
		__LINE__,
		!(prop->state >= _prop_state_uninspected_orphan &&
			prop->state <= _prop_state_inspected_orphan),
		"!prop_orphaned(prop)");
#line 790 "source\\ai\\actor_perception.c"

	prop_iterator_new(&iterator, actor_index);
	current_prop =
		(struct actor_perception_prop_view *)prop_iterator_next(&iterator);
	while (current_prop != NULL)
	{
		if (iterator.index != prop_index &&
			(current_prop->unit_index == prop->unit_index ||
				current_prop->actor_index == prop->actor_index ||
				(prop->enemy &&
					current_prop->enemy &&
					((current_prop->state >=
							_prop_state_uninspected_orphan &&
							current_prop->state <=
							_prop_state_inspected_orphan) ||
						(current_prop->state >=
							_prop_state_becoming_unacknowledged &&
							current_prop->state <=
							_prop_state_acknowledged)))) &&
			distance_squared2d(
				(real_point2d *)&current_prop->body_position,
				(real_point2d *)&prop->body_position) < 6.25f &&
			fabs(current_prop->body_position.z - prop->body_position.z) <
				1.5f &&
			dot_product3d(
				&prop->actor_to_prop,
				&current_prop->actor_to_prop) > 0.5f)
		{
			result = TRUE;
		}

		current_prop =
			(struct actor_perception_prop_view *)prop_iterator_next(
				&iterator);
	}

	return result;
}

void actor_perception_find_sense_position(
	long actor_index,
	real_point3d const *position,
	long unused,
	struct actor_position_data *sense_position)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->meta.swarm)
	{
		struct swarm_datum *swarm =
			swarm_get(actor->meta.swarm_cache_index);
		real best_distance_squared = FLT_MAX;
		long best_unit_index = NONE;
		short unit_index;

#line 1637 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(actor->meta.swarm_unit_count > 0);
#line 1634 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			actor->meta.swarm_unit_index != NONE,
			"actor->meta.swarm_unit_index != NONE");
#line 970 "source\\ai\\actor_perception.c"

		for (unit_index = 0; unit_index < swarm->unit_count; unit_index++)
		{
			struct actor_perception_swarm_component_view *component =
				(struct actor_perception_swarm_component_view *)datum_get(
					swarm_component_data,
					swarm->component_indices[unit_index]);
			real distance_squared =
				distance_squared3d(&component->position, position);

			if (distance_squared < best_distance_squared)
			{
				best_distance_squared = distance_squared;
				best_unit_index = swarm->unit_indices[unit_index];
			}
		}

#line 1651 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			best_unit_index != NONE,
			"best_unit_index != NONE");
#line 990 "source\\ai\\actor_perception.c"

		actor_input_sample_position(
			actor_index,
			best_unit_index,
			sense_position);
	}
	else
	{
		*sense_position = actor->input.position;
	}

	return;
}

boolean actor_emotion_flee_with_friends(
	long actor_index,
	real *desire_to_flee)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct prop_iterator iterator;
	struct actor_perception_prop_view *prop;
	short fighting_friend_count = 0;
	short fleeing_friend_count = 0;
	real modifier;

	prop_iterator_new(&iterator, actor_index);
	prop =
		(struct actor_perception_prop_view *)prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		if (prop->state >= _prop_state_becoming_unacknowledged &&
			prop->state <= _prop_state_acknowledged &&
			!prop->enemy &&
			prop->type == actor->team_index &&
			prop->actor_index != NONE)
		{
			struct actor_perception_actor_view *friend_actor =
				(struct actor_perception_actor_view *)actor_get(
					prop->actor_index);

			if (friend_actor->active_threat_count > 0 ||
				(friend_actor->friend_state == 4 &&
					friend_actor->friend_fighting_count > 0))
			{
				fighting_friend_count++;
			}
			else if (prop->fleeing)
			{
				fleeing_friend_count++;
			}
		}

		prop =
			(struct actor_perception_prop_view *)prop_iterator_next(
				&iterator);
	}

	if (fighting_friend_count > 1)
		return TRUE;

	if (fleeing_friend_count > 1)
		modifier = 1.0f - (fleeing_friend_count - 1) * 0.25f;
	else
		modifier = 1.0f + (1 - fleeing_friend_count) * 0.5f;

	*desire_to_flee *= PIN(modifier, 0.0f, 2.0f);

	return FALSE;
}

boolean actor_situation_try_new_target(
	long actor_index,
	long new_prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *new_prop =
		(struct actor_perception_prop_view *)prop_get(new_prop_index);
	struct actor_perception_prop_view *target_prop;
	boolean result = FALSE;

	if (actor->target_prop_index == NONE)
		target_prop = NULL;
	else
		target_prop =
			(struct actor_perception_prop_view *)prop_get(
				actor->target_prop_index);

	*(real *)&new_prop->copied_position_data[0] =
		actor_compute_prop_target_weight(actor_index, new_prop_index);

	if (*(real *)&new_prop->copied_position_data[0] > 0.0f)
	{
#line 4685 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(new_prop->enemy);
#line 700 "source\\ai\\actor_perception.c"

		if (!target_prop ||
			*(real *)&new_prop->copied_position_data[0] >=
				*(real *)&target_prop->copied_position_data[0])
		{
			actor->target_type = 0;
			actor->target_prop_index = new_prop_index;
			actor->target_unit_index = NONE;
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
			result = TRUE;
		}
	}

	return result;
}

/* ---------- private code */

long code_0001dbc0(
	void const *a,
	void const *b)
{
	real a_value = ((real const *)a)[2];
	real b_value = ((real const *)b)[2];

	if (a_value < b_value)
		return -1;

	if (a_value > b_value)
		return 1;

	return 0;
}
