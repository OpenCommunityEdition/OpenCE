/*
AI.C

symbols in this file:
0002DC80 0080:
	_ai_initialize (0000)
0002DD00 0030:
	_ai_dispose (0000)
0002DD30 0040:
	_ai_dispose_from_old_map (0000)
0002DD70 0010:
	_ai_place (0000)
0002DD80 0040:
	_ai_globals_ai_active (0000)
0002DDC0 0050:
	_ai_globals_dialogue_triggers_enabled (0000)
0002DE10 0050:
	_ai_globals_grenades_enabled (0000)
0002DE60 00b0:
	_ai_get_major_upgrade_chance (0000)
0002DF10 0070:
	_ai_adjust_damage (0000)
0002DF80 00d0:
	_ai_erase (0000)
0002E050 00c0:
	_ai_release_inactive_swarms (0000)
0002E110 0040:
	_code_0002e110 (0000)
0002E150 0150:
	_ai_find_inactive_encounters (0000)
0002E2A0 0130:
	_ai_release_inactive_encounters (0000)
0002E3D0 0070:
	_ai_try_vehicle_eviction (0000)
0002E440 0080:
	_ai_get_responsible_unit (0000)
0002E4C0 0090:
	_ai_handle_death (0000)
0002E550 00d0:
	_ai_handle_killing_spree (0000)
0002E620 0140:
	_ai_handle_allegiance_status_changed (0000)
0002E760 0130:
	_ai_handle_allegiance_broken_notification (0000)
0002E890 00e0:
	_ai_update_team_status (0000)
0002E970 0100:
	_ai_handle_bump (0000)
0002EA70 0110:
	_ai_handle_damage (0000)
0002EB80 0120:
	_code_0002eb80 (0000)
0002ECA0 0070:
	_ai_create_mounted_weapons_for_unit (0000)
0002ED10 0160:
	_ai_handle_deleted_object (0000)
0002EE70 0140:
	_ai_handle_unit_effect (0000)
0002EFB0 0040:
	_ai_handle_enter_vehicle (0000)
0002EFF0 0060:
	_ai_handle_exit_vehicle (0000)
0002F050 0040:
	_code_0002f050 (0000)
0002F090 0500:
	_ai_disconnect_from_structure_bsp (0000)
0002F590 00c0:
	_ai_reconnect_to_structure_bsp (0000)
0002F650 0050:
	_ai_get_race_from_team_index (0000)
0002F6A0 00f0:
	_ai_initialize_for_new_map (0000)
0002F790 00d0:
	_ai_update (0000)
0002F860 0170:
	_ai_consider_major_upgrade (0000)
0002F9D0 0060:
	_code_0002f9d0 (0000)
0002FA30 0170:
	_code_0002fa30 (0000)
0002FBA0 0150:
	_ai_test_line_of_fire (0000)
0002FCF0 03f0:
	_ai_test_line_of_sight (0000)
000300E0 03b0:
	_ai_test_ballistic_line_of_fire (0000)
00030490 0510:
	_ai_handle_editing (0000)
000309A0 0210:
	_code_000309a0 (0000)
00030BB0 0010:
	_ai_enemies_can_see_player (0000)
00030BC0 0010:
	_ai_enemies_attacking_player (0000)
00030BD0 046a:
	_ai_handle_spatial_effect (0000)
00247120 0006:
	??_C@_05PMBAOCDH@quiet?$AA@ (0000)
00247128 0006:
	??_C@_05BAOFOMKD@shout?$AA@ (0000)
00247130 0005:
	??_C@_04FFPBHBOD@loud?$AA@ (0000)
00247138 0007:
	??_C@_06HIDOEFBI@medium?$AA@ (0000)
00247140 0007:
	??_C@_06MJMIHDAA@silent?$AA@ (0000)
00247148 000a:
	??_C@_09EPLOMCJK@ai_update?$AA@ (0000)
00247154 000b:
	??_C@_0L@MPBGDIHA@ai_globals?$AA@ (0000)
00247160 0017:
	??_C@_0BH@OFFADIKK@c?3?2halo?2SOURCE?2ai?2ai?4c?$AA@ (0000)
00247178 000b:
	??_C@_0L@OAAMDEHE@ai?5globals?$AA@ (0000)
00247184 002a:
	??_C@_0CK@KLHIAEEK@force_major?5?$CG?$CG?5is_random?5?$CG?$CG?5rand@ (0000)
002471B0 000f:
	??_C@_0P@DJKGCMDL@?$CFd?5swarm?5units?$AA@ (0000)
002471C0 0026:
	??_C@_0CG@KJABFHAE@result_description?5?$CG?$CG?5more_to_re@ (0000)
002471E8 0045:
	??_C@_0EF@INALGHA@working_memory_size?5?$DO?$DN?5sizeof?$CIst@ (0000)
00247230 0018:
	??_C@_0BI@IJNFHBDG@encounter?5?$CFs?5?$CI?$CFd?5units?$CJ?$AA@ (0000)
00247248 0017:
	??_C@_0BH@EHECFHBL@encounterless?9actor?5?$CFs?$AA@ (0000)
00247260 0015:
	??_C@_0BF@IJHLDNKE@?$CFs?5killing?5spree?3?5?$CFd?$AA@ (0000)
00247278 0009:
	??_C@_08MAAODMKK@player?$CFd?$AA@ (0000)
00247284 0028:
	??_C@_0CI@COCLKLAG@allegiance?5between?5teams?5?$CFs?5and?5@ (0000)
002472AC 0009:
	??_C@_08ELLPFBJE@reformed?$AA@ (0000)
002472B8 0007:
	??_C@_06HKHHAOPL@broken?$AA@ (0000)
002472C0 000d:
	??_C@_0N@PLDCHNDI@?5permanently?$AA@ (0000)
002472D0 0036:
	??_C@_0DG@IOHAMJKK@allegiance?5between?5teams?5?$CFs?5and?5@ (0000)
00247308 005f:
	??_C@_0FP@HJJPEFKF@WARNING?3?5cannot?5create?5mounted?5w@ (0000)
00247368 0038:
	??_C@_0DI@LLKOJPAJ@effect_type?$DO?$DN0?5?$CG?$CG?5effect_type?$DMNU@ (0000)
002473A0 002f:
	??_C@_0CP@KEBPDGDE@volume?$DO?$DN0?5?$CG?$CG?5volume?$DMNUMBER_OF_AI@ (0000)
002473D0 001a:
	??_C@_0BK@IPBEOPCE@actor?9?$DOmeta?4encounterless?$AA@ (0000)
002473EC 0029:
	??_C@_0CJ@ECGMOGMP@?$CFs?3?5disconnect?5and?5transition?5to@ (0000)
00247418 0030:
	??_C@_0DA@HNMNMDJM@?$CFs?3?5?$CFd?5of?5?$CFd?5units?5outside?5PVS?0?5@ (0000)
00247448 0010:
	??_C@_0BA@OOMJBPJL@?5?$CIdeleted?5some?$CJ?$AA@ (0000)
00247458 002b:
	??_C@_0CL@CHFDAENO@?$CFs?3?5no?5units?5inside?5PVS?0?5do?5not?5@ (0000)
00247484 002f:
	??_C@_0CP@IOHLMJDL@?$CFs?3?5all?5swarm?5inside?5PVS?0?5transi@ (0000)
002474B8 0041:
	??_C@_0EB@GAAEJBAL@components_outside_pvs_count?5?$DM?5M@ (0000)
002474FC 0027:
	??_C@_0CH@BCDOMGFK@target_prop?9?$DOparent_prop_index?5?$CB@ (0000)
00247524 0003:
	??_C@_02DKCKIIND@?$CFs?$AA@ (0000)
00247528 0056:
	??_C@_0FG@MCBCGPDK@?$CFs?1?$CFs?5major?5upgrade?5chance?5?$CF?42f?3@ (0000)
00247580 0003:
	??_C@_02NFLNFON@NO?$AA@ (0000)
00247584 0004:
	??_C@_03IPHJCBHH@YES?$AA@ (0000)
00247588 0004:
	__real@beaaaaab (0000)
00247590 005d:
	??_C@_0FN@FHDONHFP@adjust_encounter?9?$DOplatoon_base?5?$CL@ (0000)
002475F0 0055:
	??_C@_0FF@ENLKAKCD@adjust_encounter?9?$DOplatoon_base?5?$DO@ (0000)
00247648 0057:
	??_C@_0FH@PKEMGHNJ@adjust_encounter?9?$DOsquad_base?5?$CL?5a@ (0000)
002476A0 004f:
	??_C@_0EP@KKLKNEDM@adjust_encounter?9?$DOsquad_base?5?$DO?$DN?5@ (0000)
002476F0 003c:
	??_C@_0DM@GLFLBCJN@editing?5caused?5an?5overflow?5of?5MA@ (0000)
0024772C 003a:
	??_C@_0DK@JGCJOKPE@editing?5caused?5an?5overflow?5of?5MA@ (0000)
00247768 004d:
	??_C@_0EN@KGPGOCFG@?$CIdistance_to_effect?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIdi@ (0000)
002477B8 0007:
	??_C@_06MCBHDBOL@submit?$AA@ (0000)
002477C0 003b:
	??_C@_0DL@MFAGBNKG@effect_type?$DO?$DN0?5?$CG?$CG?5effect_type?$DMNU@ (0000)
002477FC 0008:
	??_C@_07NBEECHDO@count?$DO0?$AA@ (0000)
002B6CF0 0618:
	_data_002b6cf0 (0000)
	_ai_sound_volume_enum (060c)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"

#include "ai/ai.h"
#include "ai/ai_communication.h"
#include "ai/ai_debug.h"
#include "ai/ai_profile.h"
#include "ai/ai_runtime.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/ai_script.h"
#include "ai/actor_iterators.h"
#include "ai/actor_types.h"

#include "ai/actors.h"
#include "ai/encounters.h"
#include "ai/path.h"
#include "ai/props.h"
#include "units/unit_definitions.h"
#include "units/units.h"
#include "game/game.h"
#include "game/game_allegiance.h"
#include "game/game_globals.h"

#include "memory/data.h"
#include "objects/damage.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

#include <stddef.h>

/* ---------- constants */

enum
{
	MAXIMUM_NUMBER_OF_MOUNTED_WEAPON_UNITS = 8,
	NUMBER_OF_AI_SPEECH_TIMERS = 2,
	MAXIMUM_POTENTIALLY_RELEASABLE_ENTITIES = 256,
	AI_ENDANGER_PLAYER_RECENT_VISIBILITY_TICKS = 90,
	NUMBER_OF_AI_SOUND_VOLUMES = 5,
	AI_UNIT_EFFECT_TIMEOUT = 30,
	MINIMUM_ACTOR_KILLING_SPREE = 3,
	MINIMUM_PLAYER_KILLING_SPREE = 5,
	AI_DESCRIPTION_BUFFER_SIZE = 512,
};

enum
{
	_ai_unit_effect_bump = 0,
	_ai_unit_effect_shooting,
	_ai_unit_effect_death_scream,
	_ai_unit_effect_magic_sight,
	NUMBER_OF_AI_UNIT_EFFECTS,
};

enum
{
	_damage_category_none = 0,
	_damage_category_falling,
	_damage_category_bullet,
	_damage_category_grenade,
	_damage_category_highexplosive,
	_damage_category_sniper,
	_damage_category_melee,
	_damage_category_flame,
	_damage_category_mountedweapon,
	_damage_category_vehicle,
	_damage_category_plasma,
	_damage_category_needle,
	_damage_category_shotgun,
	NUMBER_OF_DAMAGE_CATEGORIES,
};

/* ---------- macros */

/* ---------- structures */

struct ai_globals_data
{
	boolean ai_active;
	boolean ai_initialized_for_map;
	boolean ai_has_control_data;
	byte reserved003[0x5];
	long first_encounterless_actor_index;
	real major_upgrade_error;
	boolean dialogue_triggers_enabled;
	byte reserved011[0x3];
	long last_chatter_time[NUMBER_OF_AI_SPEECH_TIMERS];
	long last_talk_time[NUMBER_OF_AI_SPEECH_TIMERS];
	long last_shout_time[NUMBER_OF_AI_SPEECH_TIMERS];
	byte reserved02C[0x104];
	short spatial_effect_first_index;
	short spatial_effect_last_index;
	byte spatial_effects[0x280];
	boolean grenades_enabled;
	byte reserved3B5[0x503];
	short mounted_weapon_unit_count;
	byte reserved8BA[0x2];
	long mounted_weapon_unit_indices[MAXIMUM_NUMBER_OF_MOUNTED_WEAPON_UNITS];
};

struct potentially_releasable_entity
{
	boolean is_actor;
	byte pad[3];
	long entity_index;
	long last_active_time;
};

struct potentially_releasable_storage
{
	short count;
	short cursor;
	struct potentially_releasable_entity entities[MAXIMUM_POTENTIALLY_RELEASABLE_ENTITIES];
};

struct actor_iterator
{
	struct data_iterator encounter_iterator;
	boolean iterated_encounterless_list;
	boolean active_only;
	byte pad[2];
	long index;
	long next_index;
};

struct encounter_actor_iterator
{
	long encounter_index;
	long index;
	long next_index;
};

struct encounter_iterator
{
	struct data_iterator data;
	long index;
	boolean active_only;
};

typedef char ai_globals_active_offset_assert[
	offsetof(struct ai_globals_data, ai_active) == 0x0 ? 1 : -1];
typedef char ai_globals_initialized_offset_assert[
	offsetof(struct ai_globals_data, ai_initialized_for_map) == 0x1 ? 1 : -1];
typedef char ai_globals_first_encounterless_actor_offset_assert[
	offsetof(struct ai_globals_data, first_encounterless_actor_index) == 0x8 ? 1 : -1];
typedef char ai_globals_dialogue_offset_assert[
	offsetof(struct ai_globals_data, dialogue_triggers_enabled) == 0x10 ? 1 : -1];
typedef char ai_globals_last_chatter_time_offset_assert[
	offsetof(struct ai_globals_data, last_chatter_time) == 0x14 ? 1 : -1];
typedef char ai_globals_spatial_effects_offset_assert[
	offsetof(struct ai_globals_data, spatial_effects) == 0x134 ? 1 : -1];
typedef char ai_globals_grenades_offset_assert[
	offsetof(struct ai_globals_data, grenades_enabled) == 0x3B4 ? 1 : -1];
typedef char ai_globals_mounted_weapon_count_offset_assert[
	offsetof(struct ai_globals_data, mounted_weapon_unit_count) == 0x8B8 ? 1 : -1];
typedef char ai_globals_size_assert[
	sizeof(struct ai_globals_data) == 0x8DC ? 1 : -1];
typedef char ai_potentially_releasable_storage_size_assert[
	sizeof(struct potentially_releasable_storage) == 0xC04 ? 1 : -1];
typedef char ai_unit_actor_index_offset_assert[
	offsetof(struct unit_datum, unit.actor_index) == 0x1A4 ? 1 : -1];
typedef char ai_actor_last_vehicle_exit_forced_offset_assert[
	offsetof(struct actor_datum, emotions.last_vehicle_exit_forced) == 0x38C ? 1 : -1];
typedef char ai_actor_iterator_size_assert[
	sizeof(struct actor_iterator) == 0x1C ? 1 : -1];
typedef char ai_actor_iterator_index_offset_assert[
	offsetof(struct actor_iterator, index) == 0x14 ? 1 : -1];
typedef char ai_encounter_iterator_size_assert[
	sizeof(struct encounter_iterator) == 0x18 ? 1 : -1];
typedef char ai_encounter_iterator_index_offset_assert[
	offsetof(struct encounter_iterator, index) == 0x10 ? 1 : -1];
typedef char ai_encounter_actor_iterator_size_assert[
	sizeof(struct encounter_actor_iterator) == 0xC ? 1 : -1];
typedef char ai_encounter_actor_iterator_index_offset_assert[
	offsetof(struct encounter_actor_iterator, index) == 0x4 ? 1 : -1];
typedef char ai_actor_squad_index_offset_assert[
	offsetof(struct actor_datum, meta.squad_index) == 0x3A ? 1 : -1];
typedef char ai_actor_platoon_index_offset_assert[
	offsetof(struct actor_datum, meta.platoon_index) == 0x3C ? 1 : -1];
typedef char ai_actor_team_index_offset_assert[
	offsetof(struct actor_datum, meta.team_index) == 0x3E ? 1 : -1];
typedef char ai_unit_player_index_offset_assert[
	offsetof(struct unit_datum, unit.player_index) == 0x1C8 ? 1 : -1];
typedef char ai_unit_team_index_offset_assert[
	offsetof(struct unit_datum, object.owner_team_index) == 0x68 ? 1 : -1];
typedef char ai_prop_iterator_size_assert[
	sizeof(struct prop_iterator) == 0x8 ? 1 : -1];
typedef char ai_prop_iterator_index_offset_assert[
	offsetof(struct prop_iterator, index) == 0x0 ? 1 : -1];
typedef char ai_prop_team_index_offset_assert[
	offsetof(struct prop_datum, team_index) == 0x12 ? 1 : -1];
typedef char ai_prop_unit_index_offset_assert[
	offsetof(struct prop_datum, unit_index) == 0x18 ? 1 : -1];
typedef char ai_prop_target_weight_offset_assert[
	offsetof(struct prop_datum, target_weight) == 0x50 ? 1 : -1];
typedef char ai_prop_enemy_offset_assert[
	offsetof(struct prop_datum, enemy) == 0x60 ? 1 : -1];
typedef char ai_prop_ally_offset_assert[
	offsetof(struct prop_datum, ally) == 0x61 ? 1 : -1];
typedef char ai_prop_unopposable_enemy_offset_assert[
	offsetof(struct prop_datum, unopposable_enemy) == 0xA4 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern struct ai_globals_data *ai_globals;

/* ---------- public code */

void ai_initialize(
	void)
{
	ai_globals = game_state_malloc("ai globals", NULL, sizeof(struct ai_globals_data));
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x8C, ai_globals);

	csmemset(ai_globals, 0, sizeof(struct ai_globals_data));

	ai_debug_initialize();
	ai_profile_initialize();
	paths_initialize();
	actors_initialize();
	props_initialize();
	encounters_initialize();
	ai_script_initialize();
	ai_communication_initialize();
	actor_move_initialize();

	return;
}

void ai_dispose(
	void)
{
	ai_communication_dispose();
	ai_script_dispose();
	encounters_dispose();
	props_dispose();
	actors_dispose();
	paths_dispose();
	ai_profile_dispose();
	ai_debug_dispose();

	return;
}

void ai_dispose_from_old_map(
	void)
{
	ai_communication_dispose_from_old_map();
	ai_script_dispose_from_old_map();
	encounters_dispose_from_old_map();
	props_dispose_from_old_map();
	actors_dispose_from_old_map();
	paths_dispose_from_old_map();
	ai_profile_dispose_from_old_map();
	ai_debug_dispose_from_old_map();

	ai_globals->ai_initialized_for_map = FALSE;

	return;
}

void ai_place(
	void)
{
	encounters_create_for_new_map();

	return;
}

void ai_globals_ai_active(
	boolean enabled)
{
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x13A, ai_globals);

	ai_globals->ai_active = enabled;

	return;
}

void ai_globals_dialogue_triggers_enabled(
	boolean enabled)
{
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x143, ai_globals);

	ai_globals->dialogue_triggers_enabled = enabled;

	return;
}

void ai_globals_grenades_enabled(
	boolean enabled)
{
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x14C, ai_globals);

	ai_globals->grenades_enabled = enabled;

	return;
}

void ai_get_major_upgrade_chance(
	short major_upgrade,
	boolean *force_major,
	boolean *is_random,
	real *random_chance)
{
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x158, force_major && is_random && random_chance);

	switch (major_upgrade)
	{
	case _actor_major_upgrade_none:
		*is_random = FALSE;
		*force_major = FALSE;
		break;

	case _actor_major_upgrade_all:
		*is_random = FALSE;
		*force_major = TRUE;
		break;

	case _actor_major_upgrade_few:
		*is_random = TRUE;
		*random_chance = game_difficulty_get_value(_game_difficulty_value_major_upgrade_1);
		break;

	case _actor_major_upgrade_many:
		*is_random = TRUE;
		*random_chance = game_difficulty_get_value(_game_difficulty_value_major_upgrade_2);
		break;

	default:
		*is_random = TRUE;
		*random_chance = game_difficulty_get_value(_game_difficulty_value_major_upgrade);
		break;
	}

	return;
}

boolean ai_adjust_damage(
	long actor_index,
	struct damage_data *damage_data,
	real *damage)
{
	boolean adjusted = FALSE;

	if (actor_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);

		if (TEST_FLAG(damage_data->flags, _damage_from_weapon_bit) &&
			actor->control.burst_damage_modifier > 0.0f)
		{
			adjusted = TRUE;
			*damage = actor->control.burst_damage_modifier * *damage;
		}

		if (actor->external_orders.playfighting)
		{
			*damage = *damage * 0.3f;
			adjusted = TRUE;
		}
	}

	return adjusted;
}

void ai_erase(
	long encounter_index,
	long platoon_index,
	long squad_index,
	boolean immediate)
{
	if (!ai_globals->ai_initialized_for_map)
		return;

	if (encounter_index == NONE)
	{
		struct actor_iterator iterator;

		actor_iterator_new(&iterator, FALSE);
		while (actor_iterator_next(&iterator))
			actor_erase(iterator.index, immediate);
	}
	else
	{
		struct encounter_actor_iterator iterator;
		struct actor_datum *actor;

		encounter_actor_iterator_new(&iterator, encounter_index);
		actor = encounter_actor_iterator_next(&iterator);

		while (actor)
		{
			if ((platoon_index == NONE ||
				actor->meta.platoon_index == platoon_index) &&
				(squad_index == NONE ||
				actor->meta.squad_index == squad_index))
			{
				actor_erase(iterator.index, immediate);
			}

			actor = encounter_actor_iterator_next(&iterator);
		}
	}

	return;
}

boolean ai_release_inactive_swarms(
	char *result_description,
	boolean *more_to_release,
	byte *working_memory,
	short working_memory_size)
{
	struct actor_iterator iterator;
	short released_unit_count = 0;
	struct actor_datum *actor;

	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x1F7, result_description && more_to_release);

	actor_iterator_new(&iterator, FALSE);
	actor = actor_iterator_next(&iterator);

	while (actor)
	{
		if (actor->meta.swarm &&
			!actor->meta.active &&
			actor->meta.last_active_time != NONE)
		{
			released_unit_count += actor->meta.swarm_unit_count;
			actor_erase(iterator.index, TRUE);
		}

		actor = actor_iterator_next(&iterator);
	}

	sprintf(result_description, "%d swarm units", released_unit_count);
	*more_to_release = FALSE;

	return released_unit_count > 0;
}

int compare_potentially_releasable_entities(
	void const *element0,
	void const *element1)
{
	struct potentially_releasable_entity const *entity0 =
		(struct potentially_releasable_entity const *)element0;
	struct potentially_releasable_entity const *entity1 =
		(struct potentially_releasable_entity const *)element1;
	long last_active_time0 = entity0->last_active_time;
	long last_active_time1 = entity1->last_active_time;

	if (last_active_time1 < last_active_time0)
		return 1;
	if (last_active_time1 > last_active_time0)
		return -1;
	if (entity0->is_actor > entity1->is_actor)
		return -1;

	return entity0->is_actor < entity1->is_actor;
}

void ai_find_inactive_encounters(
	byte *working_memory,
	short working_memory_size)
{
	struct potentially_releasable_storage *storage;

	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x22E, working_memory_size >= sizeof(struct potentially_releasable_storage));

	storage = (struct potentially_releasable_storage *)working_memory;
	storage->count = 0;
	storage->cursor = 0;

	{
		struct encounter_actor_iterator actor_iterator;
		struct actor_datum *actor;

		encounter_actor_iterator_new(&actor_iterator, NONE);
		actor = encounter_actor_iterator_next(&actor_iterator);

		while (actor)
		{
			if (storage->count >= MAXIMUM_POTENTIALLY_RELEASABLE_ENTITIES)
				break;

			if (!actor->meta.active &&
				actor->meta.last_active_time != NONE)
			{
				storage->entities[storage->count].is_actor = TRUE;
				storage->entities[storage->count].entity_index = actor_iterator.index;
				storage->entities[storage->count].last_active_time = actor->meta.last_active_time;
				storage->count++;
			}

			actor = encounter_actor_iterator_next(&actor_iterator);
		}
	}

	{
		struct encounter_iterator encounter_iterator;
		struct encounter_datum *encounter;

		encounter_iterator_new(&encounter_iterator, FALSE);
		encounter = encounter_iterator_next(&encounter_iterator);

		while (encounter)
		{
			if (storage->count >= MAXIMUM_POTENTIALLY_RELEASABLE_ENTITIES)
				break;

			if (!encounter->active &&
				encounter->current_count > 0 &&
				encounter->last_active_time != NONE)
			{
				storage->entities[storage->count].is_actor = FALSE;
				storage->entities[storage->count].entity_index = encounter_iterator.index;
				storage->entities[storage->count].last_active_time = encounter->last_active_time;
				storage->count++;
			}

			encounter = encounter_iterator_next(&encounter_iterator);
		}
	}

	if (storage->count > 0)
	{
		qsort(
			storage->entities,
			storage->count,
			sizeof(struct potentially_releasable_entity),
			compare_potentially_releasable_entities);
	}

	return;
}

boolean ai_release_inactive_encounters(
	char *result_description,
	boolean *more_to_release,
	byte *working_memory,
	short working_memory_size)
{
	struct potentially_releasable_storage *storage;
	boolean released = FALSE;

	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x270, result_description && more_to_release);
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x271, working_memory_size >= sizeof(struct potentially_releasable_storage));

	storage = (struct potentially_releasable_storage *)working_memory;

	if (storage->cursor < storage->count)
	{
		struct potentially_releasable_entity *entity = &storage->entities[storage->cursor];

		if (entity->is_actor)
		{
			sprintf(
				result_description,
				"encounterless-actor %s",
				tag_name_strip_path(tag_get_name(
					actor_get(entity->entity_index)->meta.variant_definition_index)));
			actor_erase(entity->entity_index, TRUE);
		}
		else
		{
			struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->ai_encounters,
				DATUM_INDEX_TO_ABSOLUTE_INDEX(entity->entity_index),
				struct encounter_definition);

			sprintf(
				result_description,
				"encounter %s (%d units)",
				encounter_definition->name,
				encounter_get(entity->entity_index)->current_count);
			ai_erase(entity->entity_index, NONE, NONE, TRUE);
		}

		storage->cursor++;
		released = TRUE;
	}

	*more_to_release = storage->cursor < storage->count;

	return released;
}

boolean ai_try_vehicle_eviction(
	long actor_index,
	long entering_unit_index,
	boolean immediate)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	if (entering_unit_index != NONE)
	{
		struct unit_datum *entering_unit = unit_get(entering_unit_index);

		if (entering_unit->unit.player_index != NONE &&
			!game_team_is_enemy(
				entering_unit->object.owner_team_index,
				actor->meta.team_index))
		{
			result = TRUE;

			if (immediate)
				actor_stimulus_vehicle_eviction(actor_index);
		}
	}

	return result;
}

long ai_get_responsible_unit(
	long object_index,
	boolean responsible_for_weapon_fire)
{
	long responsible_unit_index = NONE;

	if (object_index != NONE)
	{
		struct unit_datum *unit = unit_try_and_get(object_index);

		if (unit)
		{
			responsible_unit_index = object_index;

			if (responsible_for_weapon_fire &&
				unit->unit.gunner_object_index != NONE)
			{
				responsible_unit_index = unit->unit.gunner_object_index;
			}
			else if (unit->unit.driver_object_index != NONE)
			{
				responsible_unit_index = unit->unit.driver_object_index;
			}

			if (responsible_unit_index != NONE &&
				!game_connection() &&
				ai_debug.ignore_player &&
				unit_get(responsible_unit_index)->unit.player_index != NONE)
			{
				responsible_unit_index = NONE;
			}
		}
	}

	return responsible_unit_index;
}

void ai_handle_death(
	long unit_index,
	long killer_object_index,
	short damage_category)
{
	long responsible_unit_index = ai_get_responsible_unit(
		killer_object_index,
		damage_category != _damage_category_vehicle);
	short hostility = _comm_hostility_none;

	if (unit_index == responsible_unit_index)
	{
		hostility = _comm_hostility_self;
	}
	else if (responsible_unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(unit_index);
		struct unit_datum *responsible_unit = unit_get(responsible_unit_index);

		hostility = game_team_is_enemy(
			unit->object.owner_team_index,
			responsible_unit->object.owner_team_index)
			? _comm_hostility_enemy
			: _comm_hostility_friend;
	}

	ai_communication_event(
		_ai_communication_death,
		unit_index,
		responsible_unit_index,
		hostility,
		damage_category,
		NONE,
		NULL);
	ai_conversation_unit_died(unit_index, FALSE);
	encounters_unit_died(unit_index);

	return;
}

boolean ai_handle_killing_spree(
	long unit_index,
	short killing_spree_count)
{
	char description[AI_DESCRIPTION_BUFFER_SIZE];
	struct unit_datum *unit = unit_get(unit_index);
	long player_index = unit->unit.player_index;
	short minimum_killing_spree = (player_index != NONE)
		? MINIMUM_PLAYER_KILLING_SPREE
		: MINIMUM_ACTOR_KILLING_SPREE;
	boolean killing_spree = FALSE;

	if (ai_debug.print_killing_sprees)
	{
		if (player_index == NONE)
		{
			ai_debug_describe_actor(
				unit->unit.actor_index,
				unit_index,
				TRUE,
				description,
				sizeof(description));
		}
		else
		{
			sprintf(description, "player%d", DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index));
		}

		console_printf(FALSE, "%s killing spree: %d", description, killing_spree_count);
	}

	if (killing_spree_count >= minimum_killing_spree)
	{
		ai_communication_event(
			_ai_communication_killing_spree,
			unit_index,
			NONE,
			NONE,
			NONE,
			NONE,
			NULL);

		killing_spree = TRUE;
	}

	return killing_spree;
}

void ai_handle_allegiance_status_changed(
	short team1_index,
	short team2_index,
	boolean currently_broken,
	boolean permanently_broken)
{
	struct actor_iterator actor_iterator;
	struct actor_datum *actor;

	if (ai_debug.print_allegiance)
	{
		console_printf(
			FALSE,
			"allegiance between teams %s and %s %s%s",
			global_game_team_names[team1_index],
			global_game_team_names[team2_index],
			currently_broken ? "broken" : "reformed",
			permanently_broken ? " permanently" : "");
	}

	actor_iterator_new(&actor_iterator, TRUE);
	actor = actor_iterator_next(&actor_iterator);

	while (actor)
	{
		short other_team_index = NONE;

		if (actor->meta.team_index == team1_index)
			other_team_index = team2_index;
		else if (actor->meta.team_index == team2_index)
			other_team_index = team1_index;

		if (other_team_index != NONE)
		{
			struct prop_iterator prop_iterator;
			struct prop_datum *prop;

			prop_iterator_new(&prop_iterator, actor_iterator.index);
			prop = prop_iterator_next(&prop_iterator);

			while (prop)
			{
				if (prop->team_index == other_team_index)
				{
					if (!permanently_broken)
					{
						prop->ally = TRUE;
						prop->ally_status_changed = TRUE;
					}

					if (!currently_broken || permanently_broken)
					{
						prop->enemy = currently_broken;
						prop->unopposable_enemy = actor_compute_prop_unopposable(
							actor_iterator.index,
							prop_iterator.index);
						prop->target_weight = actor_compute_prop_target_weight(
							actor_iterator.index,
							prop_iterator.index);
					}
				}

				prop = prop_iterator_next(&prop_iterator);
			}
		}

		actor = actor_iterator_next(&actor_iterator);
	}

	return;
}

void ai_handle_allegiance_broken_notification(
	short team1_index,
	short team2_index,
	boolean broken)
{
	struct actor_iterator actor_iterator;
	struct actor_datum *actor;

	if (ai_debug.print_allegiance)
	{
		console_printf(
			FALSE,
			"allegiance between teams %s and %s communicated as %s",
			global_game_team_names[team1_index],
			global_game_team_names[team2_index],
			broken ? "broken" : "reformed");
	}

	actor_iterator_new(&actor_iterator, TRUE);
	actor = actor_iterator_next(&actor_iterator);

	while (actor)
	{
		short other_team_index = NONE;

		if (actor->meta.team_index == team1_index)
			other_team_index = team2_index;
		else if (actor->meta.team_index == team2_index)
			other_team_index = team1_index;

		if (other_team_index != NONE)
		{
			struct prop_iterator prop_iterator;
			struct prop_datum *prop;

			prop_iterator_new(&prop_iterator, actor_iterator.index);
			prop = prop_iterator_next(&prop_iterator);

			while (prop)
			{
				if (prop->team_index == other_team_index)
				{
					prop->ally = TRUE;
					prop->ally_status_changed = FALSE;
					prop->enemy = broken;
					prop->unopposable_enemy = actor_compute_prop_unopposable(
						actor_iterator.index,
						prop_iterator.index);
					prop->target_weight = actor_compute_prop_target_weight(
						actor_iterator.index,
						prop_iterator.index);
				}

				prop = prop_iterator_next(&prop_iterator);
			}
		}

		actor = actor_iterator_next(&actor_iterator);
	}

	game_allegiance_notify_change(team1_index, team2_index);

	return;
}

void ai_update_team_status(
	void)
{
	struct actor_iterator actor_iterator;
	struct actor_datum *actor;

	actor_iterator_new(&actor_iterator, TRUE);
	actor = actor_iterator_next(&actor_iterator);

	while (actor)
	{
		struct prop_iterator prop_iterator;
		struct prop_datum *prop;

		prop_iterator_new(&prop_iterator, actor_iterator.index);
		prop = prop_iterator_next(&prop_iterator);

		while (prop)
		{
			struct unit_datum *unit = unit_get(prop->unit_index);

			prop->team_index = unit->object.owner_team_index;
			prop->enemy = game_team_is_enemy(
				actor->meta.team_index,
				prop->team_index);
			prop->ally = game_team_is_ally(
				actor->meta.team_index,
				prop->team_index);
			prop->unopposable_enemy = actor_compute_prop_unopposable(
				actor_iterator.index,
				prop_iterator.index);
			prop->target_weight = actor_compute_prop_target_weight(
				actor_iterator.index,
				prop_iterator.index);

			prop = prop_iterator_next(&prop_iterator);
		}

		actor = actor_iterator_next(&actor_iterator);
	}

	return;
}

void ai_handle_bump(
	long unit_index,
	long object_index,
	union real_vector3d const *old_velocity)
{
	long responsible_unit_index = ai_get_responsible_unit(object_index, FALSE);

	(void)old_velocity;

	if (responsible_unit_index != NONE)
	{
		struct unit_datum *responsible_unit = unit_get(responsible_unit_index);

		if (responsible_unit->object.type == _object_type_biped)
		{
			struct unit_datum *unit = unit_get(unit_index);

			if (unit->unit.actor_index != NONE)
			{
				long prop_index = prop_get_base_by_unit_index(
					unit->unit.actor_index,
					responsible_unit_index,
					TRUE,
					FALSE);

				if (prop_index != NONE)
				{
					actor_handle_unit_effect(
						unit->unit.actor_index,
						prop_index,
						_ai_unit_effect_bump);
				}
			}

			if (responsible_unit->unit.actor_index != NONE)
			{
				long prop_index = prop_get_base_by_unit_index(
					responsible_unit->unit.actor_index,
					unit_index,
					TRUE,
					FALSE);

				if (prop_index != NONE)
				{
					actor_handle_unit_effect(
						responsible_unit->unit.actor_index,
						prop_index,
						_ai_unit_effect_bump);
				}
			}
		}
	}

	return;
}

void ai_create_mounted_weapons_for_unit(
	long unit_index)
{
	if (ai_globals->ai_initialized_for_map)
	{
		if (ai_globals->mounted_weapon_unit_count < MAXIMUM_NUMBER_OF_MOUNTED_WEAPON_UNITS)
		{
			ai_globals->mounted_weapon_unit_indices[ai_globals->mounted_weapon_unit_count] = unit_index;
			ai_globals->mounted_weapon_unit_count++;
		}
		else
		{
			error(
				_error_silent,
				"WARNING: cannot create mounted weapons for %s, exceeded MAXIMUM_NUMBER_OF_MOUNTED_WEAPON_UNITS",
				tag_name_strip_path(tag_get_name(unit_get(unit_index)->definition_index)));
		}
	}

	return;
}

void ai_handle_damage(
	long unit_index,
	long shooter_object_index,
	short damage_category,
	real fraction,
	union real_vector3d *damage_velocity,
	boolean delayed)
{
	struct unit_datum *unit = unit_get(unit_index);
	long responsible_unit_index = ai_get_responsible_unit(
		shooter_object_index,
		damage_category != _damage_category_vehicle);
	struct unit_datum *responsible_unit = (responsible_unit_index == NONE)
		? NULL
		: unit_get(responsible_unit_index);
	short hostility;

	if (!delayed &&
		damage_category != _damage_category_falling &&
		unit->unit.actor_index != NONE)
	{
		actor_handle_damage(
			unit->unit.actor_index,
			responsible_unit_index,
			fraction,
			damage_velocity);
	}

	hostility = _comm_hostility_none;

	if (unit_index == responsible_unit_index)
	{
		hostility = _comm_hostility_self;
	}
	else if (responsible_unit)
	{
		hostility = game_team_is_enemy(
			unit->object.owner_team_index,
			responsible_unit->object.owner_team_index)
			? _comm_hostility_enemy
			: _comm_hostility_friend;
	}

	if (!delayed && hostility == _comm_hostility_friend)
	{
		ai_communication_event(
			_ai_communication_damage,
			unit_index,
			responsible_unit_index,
			_comm_hostility_friend,
			damage_category,
			NONE,
			NULL);
	}
	else if (fraction >= 0.3f)
	{
		ai_communication_event(
			_ai_communication_hurt,
			unit_index,
			responsible_unit_index,
			hostility,
			damage_category,
			NONE,
			NULL);
	}

	if (responsible_unit)
	{
		game_allegiance_provoke(
			responsible_unit->object.owner_team_index,
			unit->object.owner_team_index);
	}

	return;
}

void ai_handle_deleted_object(
	long object_index)
{
	struct object_datum *object;
	struct unit_datum *unit;
	struct data_iterator iterator;
	struct prop_datum *prop;
	short mounted_weapon_index;

	if (!ai_globals->ai_initialized_for_map)
		return;

	object = object_get(object_index);
	if (!TEST_FLAG(_object_mask_unit, object->object.type))
		return;

	unit = unit_get(object_index);
	if (unit->unit.actor_index != NONE)
	{
		actor_delete(unit->unit.actor_index, FALSE);
	}
	else if (unit->unit.swarm_actor_index != NONE)
	{
		actor_swarm_unit_died(unit->unit.swarm_actor_index, object_index);
	}

	data_iterator_new(&iterator, prop_data);
	prop = data_iterator_next(&iterator);

	while (prop)
	{
		if (prop->unit_index == object_index)
		{
			actor_switch_props(
				prop->owner_actor_index,
				iterator.datum_index,
				NONE);
			prop_delete(prop->owner_actor_index, iterator.datum_index);
		}
		else if (prop->vehicle_index == object_index)
		{
			prop->vehicle_index = NONE;
			prop->dangerous_vehicle_driver = FALSE;
			prop->vehicle_gunner = FALSE;
		}

		prop = data_iterator_next(&iterator);
	}

	ai_conversation_unit_died(object_index, TRUE);

	for (mounted_weapon_index = 0;
		mounted_weapon_index < ai_globals->mounted_weapon_unit_count;
		mounted_weapon_index++)
	{
		if (ai_globals->mounted_weapon_unit_indices[mounted_weapon_index] == object_index)
		{
			ai_globals->mounted_weapon_unit_count--;

			if (ai_globals->mounted_weapon_unit_count > 0)
			{
				ai_globals->mounted_weapon_unit_indices[mounted_weapon_index] =
					ai_globals->mounted_weapon_unit_indices[ai_globals->mounted_weapon_unit_count];
			}
		}
	}

	return;
}

void ai_handle_unit_effect(
	long unit_index,
	short effect_type,
	short volume)
{
	if (!ai_globals->ai_initialized_for_map)
		return;

	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x729, volume>=0 && volume<NUMBER_OF_AI_SOUND_VOLUMES);
	match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x72A, effect_type>=0 && effect_type<NUMBER_OF_AI_UNIT_EFFECTS);

	if (unit_index != NONE && volume > 0)
	{
		struct unit_datum *unit = unit_get(unit_index);
		long game_time = game_time_get();

		if (!game_connection() &&
			ai_debug.ignore_player &&
			unit->unit.player_index != NONE)
		{
			return;
		}

		if (effect_type > unit->unit.last_unit_effect_type ||
			game_time > unit->unit.game_time_at_last_unit_effect + AI_UNIT_EFFECT_TIMEOUT)
		{
			short object_type = unit->object.type;

			unit->unit.last_unit_effect_type = effect_type;
			unit->unit.game_time_at_last_unit_effect = game_time;

			if (object_type == _object_type_vehicle)
			{
				long child_object_index = unit->object.first_child_object_index;

				while (child_object_index != NONE)
				{
					struct object_datum *child = object_get(child_object_index);

					if (child->object.type == _object_type_biped)
					{
						actors_handle_unit_effect(
							child_object_index,
							effect_type,
							volume);
					}

					child_object_index = child->object.next_object_index;
				}
			}
			else if (object_type == _object_type_biped)
			{
				actors_handle_unit_effect(unit_index, effect_type, volume);
			}
		}
	}

	return;
}

void ai_handle_enter_vehicle(
	long unit_index,
	long vehicle_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	(void)vehicle_index;

	if (unit->unit.actor_index != NONE)
	{
		ai_communication_event(
			_ai_communication_vehicle_entry,
			unit_index,
			NONE,
			NONE,
			NONE,
			NONE,
			NULL);
	}

	return;
}

void ai_handle_exit_vehicle(
	long unit_index,
	long vehicle_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	long actor_index = unit->unit.actor_index;
	struct actor_datum *actor;

	(void)vehicle_index;

	if (actor_index == NONE)
	{
		return;
	}

	actor = actor_get(actor_index);
	if (!actor->emotions.last_vehicle_exit_forced)
	{
		ai_communication_event(
			_ai_communication_vehicle_exit,
			unit_index,
			NONE,
			NONE,
			NONE,
			NONE,
			NULL);
	}

	actor->emotions.last_vehicle_exit_forced = FALSE;

	return;
}

void ai_flush_spatial_effects(
	void)
{
	ai_globals->spatial_effect_last_index = 0;
	ai_globals->spatial_effect_first_index = 0;
	csmemset(
		ai_globals->spatial_effects,
		0,
		sizeof(ai_globals->spatial_effects));

	return;
}

void ai_reconnect_to_structure_bsp(
	void)
{
	short structure_bsp_index = global_structure_bsp_index_get();
	long actor_index = ai_globals->first_encounterless_actor_index;

	while (actor_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		long next_actor_index = actor->meta.next_actor_index;

		match_assert("c:\\halo\\SOURCE\\ai\\ai.c", 0x96F, actor->meta.encounterless);

		if (actor->meta.disconnected_encounter_index != NONE)
		{
			struct encounter_definition *encounter = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->ai_encounters,
				DATUM_INDEX_TO_ABSOLUTE_INDEX(actor->meta.disconnected_encounter_index),
				struct encounter_definition);

			if (encounter->runtime_structure_bsp_reference_index == structure_bsp_index)
			{
				encounterless_detach_actor(actor_index);
				encounter_attach_actor(
					actor_index,
					actor->meta.disconnected_encounter_index,
					actor->meta.disconnected_squad_index,
					TRUE);
			}
		}

		actor_index = next_actor_index;
	}

	return;
}

boolean ai_consider_major_upgrade(
	long encounter_index,
	short squad_index,
	real chance)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct squad_datum *squad = encounter_get_squad(encounter, squad_index);
	real global_error = ai_globals->major_upgrade_error * -(1.0f / 3.0f);
	real squad_error = -squad->major_upgrade_error;
	real chance_error = (fabs(global_error) > fabs(squad_error))
		? global_error
		: squad_error;
	real random_chance = real_seed_random(get_global_random_seed_address());
	real adjusted_chance = chance_error + chance;
	boolean upgrade_major = random_chance < adjusted_chance;
	real error_delta = upgrade_major - chance;

	squad->major_upgrade_error += error_delta;
	ai_globals->major_upgrade_error += error_delta;

	if (ai_debug.print_major_upgrade)
	{
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index),
			struct encounter_definition);

		console_printf(
			FALSE,
			csprintf(
				temporary,
				"%s/%s major upgrade chance %.2f: %s (g %.2f s %.2f -> chance %.2f rand %.2f err %.2f)",
				encounter_definition->name,
				TAG_BLOCK_GET_ELEMENT(
					&encounter_definition->squads,
					squad_index,
					struct squad_definition)->name,
				chance,
				upgrade_major ? "YES" : "NO",
				global_error,
				squad_error,
				adjusted_chance,
				random_chance,
				error_delta));

		error(_error_silent, "%s", temporary);
	}

	return upgrade_major;
}

void ai_initialize_for_new_map(
	void)
{
	csmemset(ai_globals, 0, sizeof(struct ai_globals_data));

	ai_globals->ai_active = TRUE;
	ai_globals->ai_has_control_data = TRUE;
	ai_globals->first_encounterless_actor_index = NONE;
	ai_globals->grenades_enabled = TRUE;
	ai_globals->dialogue_triggers_enabled = TRUE;

	csmemset(
		ai_globals->last_chatter_time,
		NONE,
		sizeof(ai_globals->last_chatter_time));
	csmemset(
		ai_globals->last_talk_time,
		NONE,
		sizeof(ai_globals->last_talk_time));
	csmemset(
		ai_globals->last_shout_time,
		NONE,
		sizeof(ai_globals->last_shout_time));

	ai_debug_initialize_for_new_map();
	ai_profile_initialize_for_new_map();
	paths_initialize_for_new_map();
	actors_initialize_for_new_map();
	props_initialize_for_new_map();
	encounters_initialize_for_new_map();
	ai_script_initialize_for_new_map();
	ai_communication_initialize_for_new_map();

	ai_flush_spatial_effects();

	ai_globals->ai_initialized_for_map = TRUE;

	return;
}

boolean ai_enemies_endanger_player(
	boolean must_be_attacking)
{
	long current_time = game_time_get();
	struct data_iterator iterator;
	struct prop_datum *prop;

	data_iterator_new(&iterator, prop_data);
	prop = data_iterator_next(&iterator);

	while (prop)
	{
		if (prop->player &&
			prop->enemy &&
			unit_get(prop->unit_index)->unit.player_index != NONE)
		{
			struct actor_datum *actor = actor_get(prop->owner_actor_index);
			long object_index = actor->meta.swarm
				? actor->meta.swarm_unit_index
				: actor->meta.unit_index;
			struct unit_definition *definition = unit_definition_get(
				unit_get(object_index)->definition_index);
			boolean does_not_endanger_player = FALSE;

			if (TEST_FLAG(definition->unit.flags, _unit_is_inconsequential_bit) &&
				prop->distance > 4.0f)
			{
				does_not_endanger_player = TRUE;
			}

			if (must_be_attacking &&
				!actor->control.fire_state &&
				actor->state.action != _actor_action_charge &&
				prop->distance > 15.0f)
			{
				does_not_endanger_player = TRUE;
			}

			if (!does_not_endanger_player)
			{
				short state = prop->state;

				if ((state < _prop_state_uninspected_orphan ||
					state > _prop_state_inspected_orphan) &&
					prop->last_visible_time != NONE &&
					prop->last_visible_time + AI_ENDANGER_PLAYER_RECENT_VISIBILITY_TICKS >= current_time)
				{
					return TRUE;
				}

				if ((state < _prop_state_uninspected_orphan ||
					state > _prop_state_inspected_orphan) &&
					prop->distance < 4.0f)
				{
					return TRUE;
				}

				if (actor->target.target_prop_index == iterator.datum_index)
				{
					if (state >= _prop_state_becoming_unacknowledged &&
						state <= _prop_state_acknowledged)
					{
						return TRUE;
					}

					if (state >= _prop_state_uninspected_orphan &&
						state <= _prop_state_inspected_orphan)
					{
						struct prop_datum *parent_prop;

						if (prop->definitely_located)
							return TRUE;

						parent_prop = prop_get(prop->parent_prop_index);

						if (prop->state == _prop_state_uninspected_orphan &&
							prop->distance < 12.0f &&
							distance_squared3d(
								&parent_prop->body_position,
								&prop->body_position) < 16.0f)
						{
							return TRUE;
						}
					}
				}
			}
		}

		prop = data_iterator_next(&iterator);
	}

	return FALSE;
}

boolean ai_enemies_can_see_player(
	void)
{
	return ai_enemies_endanger_player(FALSE);
}

boolean ai_enemies_attacking_player(
	void)
{
	return ai_enemies_endanger_player(TRUE);
}

long ai_get_race_from_team_index(
	short team_index)
{
	long race = _race_none;

	if (team_index == _game_team_player)
		race = _race_player;
	else if (team_index == _game_team_human)
		race = _race_human;
	else if (team_index == _game_team_covenant)
		race = _race_covenant;
	else if (team_index == _game_team_flood)
		race = _race_flood;
	else if (team_index == _game_team_sentinel)
		race = _race_sentinel;

	return race;
}

/* ---------- private code */
