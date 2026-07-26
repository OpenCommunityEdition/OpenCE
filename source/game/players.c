/*
PLAYERS.C

symbols in this file:
000A9470 0070:
	_players_initialize (0000)
000A94E0 00b0:
	_players_initialize_for_new_map (0000)
000A9590 0020:
	_players_dispose_from_old_map (0000)
000A95B0 0030:
	_players_dispose (0000)
000A95E0 0020:
	_machine_get_player_list (0000)
000A9600 0050:
	_code_000a9600 (0000)
000A9650 0060:
	_local_player_exists (0000)
000A96B0 0050:
	_find_unused_local_player_index (0000)
000A9700 0020:
	_player_delete (0000)
000A9720 0010:
	_players_get_respawn_failure (0000)
000A9730 0050:
	_local_player_get_player_index (0000)
000A9780 00a0:
	_local_player_set_player_index (0000)
000A9820 0010:
	_local_player_count (0000)
000A9830 0040:
	_local_player_get_next (0000)
000A9870 0050:
	_player_index_from_unit_index (0000)
000A98C0 0090:
	_player_died (0000)
000A9950 0010:
	_players_are_all_dead (0000)
000A9960 00c0:
	_players_set_local_player_unit (0000)
000A9A20 0010:
	_players_get_combined_pvs_local (0000)
000A9A30 0010:
	_players_get_combined_pvs (0000)
000A9A40 0020:
	_player_input_enable (0000)
000A9A60 0020:
	_player_input_enabled (0000)
000A9A80 00f0:
	_any_player_is_in_the_air (0000)
000A9B70 0050:
	_any_player_is_dead (0000)
000A9BC0 0040:
	_code_000a9bc0 (0000)
000A9C00 00e0:
	_code_000a9c00 (0000)
000A9CE0 0120:
	_player_control_fix_for_loaded_game_state (0000)
000A9E00 0050:
	_player_get_starting_location_count (0000)
000A9E50 00c0:
	_player_get_starting_location (0000)
000A9F10 0070:
	_placement_data_set_change_color (0000)
000A9F80 0070:
	_code_000a9f80 (0000)
000A9FF0 0170:
	_code_000a9ff0 (0000)
000AA160 0020:
	_code_000aa160 (0000)
000AA180 0010:
	_code_000aa180 (0000)
000AA190 0090:
	_unit_should_autopick_weapon (0000)
000AA220 0020:
	_code_000aa220 (0000)
000AA240 00c0:
	_code_000aa240 (0000)
000AA300 00b0:
	_code_000aa300 (0000)
000AA3B0 00b0:
	_code_000aa3b0 (0000)
000AA460 0090:
	_code_000aa460 (0000)
000AA4F0 0040:
	_code_000aa4f0 (0000)
000AA530 0030:
	_code_000aa530 (0000)
000AA560 0030:
	_code_000aa560 (0000)
000AA590 0070:
	_players_handle_deleted_object (0000)
000AA600 0020:
	_random_direction3d (0000)
000AA620 0040:
	_valid_real_vector2d (0000)
000AA660 0120:
	_player_new (0000)
000AA780 0150:
	_player_add_equipment (0000)
000AA8D0 0110:
	_player_aiming_vector_from_facing (0000)
000AA9E0 0510:
	_code_000aa9e0 (0000)
000AAEF0 0060:
	_player_teleport (0000)
000AAF50 00d0:
	_find_best_starting_location_index (0000)
000AB020 0330:
	_code_000ab020 (0000)
000AB350 00f0:
	_code_000ab350 (0000)
000AB440 0250:
	_code_000ab440 (0000)
000AB690 00f0:
	_player_handle_powerup (0000)
000AB780 00a0:
	_player_handle_powerup_minor (0000)
000AB820 0070:
	_code_000ab820 (0000)
000AB890 01a0:
	_players_debug_render (0000)
000ABA30 0090:
	_debug_player_teleport (0000)
000ABAC0 01d0:
	_players_respawn_coop (0000)
000ABC90 0140:
	_code_000abc90 (0000)
000ABDD0 02e0:
	_players_reconnect_to_structure_bsp (0000)
000AC0B0 01c0:
	_code_000ac0b0 (0000)
000AC270 00b0:
	_code_000ac270 (0000)
000AC320 0150:
	_code_000ac320 (0000)
000AC470 06e0:
	_players_update_before_game (0000)
000ACB50 0320:
	_code_000acb50 (0000)
000ACE70 00f0:
	_code_000ace70 (0000)
000ACF60 0330:
	_players_update_after_game (0000)
0025CED8 006c:
	_rdata_0025ced8 (0000)
0025CF44 001a:
	??_C@_0BK@OOAKICCI@players_update_after_game?$AA@ (0000)
0025CF60 001b:
	??_C@_0BL@FKGHDGFM@players_update_before_game?$AA@ (0000)
0025CF7C 0010:
	??_C@_0BA@CEEHCBBD@players?5globals?$AA@ (0000)
0025CF8C 0006:
	??_C@_05EINLALJG@teams?$AA@ (0000)
0025CF94 0008:
	??_C@_07BCGOFIOM@players?$AA@ (0000)
0025CF9C 001a:
	??_C@_0BK@CGPNFENM@failed?5to?5create?5a?5player?$AA@ (0000)
0025CFB8 001e:
	??_C@_0BO@IAEAGICD@c?3?2halo?2SOURCE?2game?2players?4c?$AA@ (0000)
0025CFD8 004f:
	??_C@_0EP@PCIBAKOM@local_player_index?$DO?$DNNONE?5?$CG?$CG?5loca@ (0000)
0025D028 009b:
	??_C@_0JL@EEPEKCFO@tried?5to?5fix?5broken?5player?5contr@ (0000)
0025D0C8 0069:
	??_C@_0GJ@LOLKJFPO@failed?5to?5correct?5player?5control@ (0000)
0025D134 0031:
	??_C@_0DB@FKMNJOMP@corrected?5player?5control?5for?5res@ (0000)
0025D168 0070:
	??_C@_0HA@DONIDEOA@?$CI?$CIlocal_player_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIloc@ (0000)
0025D1D8 002b:
	??_C@_0CL@OKOJEGMM@Could?5not?5attach?5starting?5weapon@ (0000)
0025D204 0021:
	??_C@_0CB@EBNMJKAL@player?9?$DOlocal_player_index?$CB?$DNNONE@ (0000)
0025D228 002f:
	??_C@_0CP@BNOOKFKL@couldn?8t?5teleport?5player?5into?5a?5@ (0000)
0025D258 0019:
	??_C@_0BJ@LHNHGGK@player?9?$DOunit_index?$CB?$DNNONE?$AA@ (0000)
0025D274 0006:
	??_C@_05MHMEBBGO@scale?$AA@ (0000)
0025D27C 0025:
	??_C@_0CF@PKKNMABJ@magnitude3d?$CI?$CGbest_adjustment_vec@ (0000)
0025D2A4 0032:
	??_C@_0DC@CLGKJEDK@source_unit_index?$DN?$DNNONE?5?$HM?$HM?5local@ (0000)
0025D2D8 0008:
	__real@3fe6a09e60000000 (0000)
0025D2E0 003a:
	??_C@_0DK@GFFNPIMP@powerup_type?$DO?$DN0?5?$CG?$CG?5powerup_type?$DM@ (0000)
0025D31C 0016:
	??_C@_0BG@HMDKOAOM@no?5players?5in?5the?5bsp?$AA@ (0000)
0025D334 0013:
	??_C@_0BD@ELAHGGCE@seat_index?5?$CB?$DN?5NONE?$AA@ (0000)
0025D348 0046:
	??_C@_0EG@POGHKPOA@?$CINONE?5?$DN?$DN?5control_data?4zoom_level@ (0000)
0025D390 008c:
	??_C@_0IM@DKBPHNBM@?$CINONE?5?$DN?$DN?5control_data?4grenade_in@ (0000)
0025D420 0085:
	??_C@_0IF@OCHKLACI@?$CINONE?5?$DN?$DN?5control_data?4weapon_ind@ (0000)
0025D4A8 004c:
	??_C@_0EM@ECBIAFIL@?$CINONE?5?$DN?$DN?5action?9?$DOdesired_zoom_le@ (0000)
0025D4F8 0095:
	??_C@_0JF@HCBNBPOG@?$CINONE?5?$DN?$DN?5action?9?$DOdesired_grenade@ (0000)
0025D590 008e:
	??_C@_0IO@DMKDCDID@?$CINONE?5?$DN?$DN?5action?9?$DOdesired_weapon_@ (0000)
0025D620 0018:
	??_C@_0BI@GKAKFFBF@action?9?$DOprimary_trigger?$AA@ (0000)
0025D638 0027:
	??_C@_0CH@OILDLPCJ@?$CFs?3?5assert_valid_real_vector2d?$CI?$CF@ (0000)
0025D660 0012:
	??_C@_0BC@MPDBBHIH@?$CGaction?9?$DOthrottle?$AA@ (0000)
0025D674 001b:
	??_C@_0BL@PMBMFGJI@action?9?$DOdesired_facing?4yaw?$AA@ (0000)
0025D690 001d:
	??_C@_0BN@NJOPCJPO@action?9?$DOdesired_facing?4pitch?$AA@ (0000)
0025D6B0 0042:
	??_C@_0EC@JMPLFNIK@action_index?$DO?$DN0?5?$CG?$CG?5action_index?$DM@ (0000)
0025D6F4 003b:
	??_C@_0DL@NOPAIALJ@?$CB?$CB?$CBWARNING?$CB?$CB?$CB?5teleported?5player?5@ (0000)
0025D730 0004:
	__real@3c360b61 (0000)
002DEE08 0c18:
	_data_002dee08 (0000)
00453408 005d:
	_bss_00453408 (0000)
	_debug_render_player_teleport (005c)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai/ai_debug.h"
#include "ai/ai_scenario_definitions.h"
#include "data.h"
#include "effects/player_effects.h"
#include "game.h"
#include "game_engine.h"
#include "input.h"
#include "interface/hud.h"
#include "items/equipment.h"
#include "items/equipment_definitions.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "objects/damage.h"
#include "physics/collisions.h"
#include "real_math.h"
#include "players.h"
#include "player_control.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "text/unicode.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"

/* ---------- constants */

enum equipment_powerup_type
{
	_equipment_powerup_double_speed = 1,
	_equipment_powerup_overshield,
	_equipment_powerup_active_camouflage,
	_equipment_powerup_full_spectrum_vision,
	_equipment_powerup_health
};

/* ---------- macros */

/* ---------- structures */

struct _players_vehicle_datum
{
	byte unknown_state;
	byte pad[3];
};

struct players_vehicle_datum
{
	long definition_index;
	struct _object_datum object;
	struct _unit_datum unit;
	long unknown424;
	struct _players_vehicle_datum vehicle;
};

struct scenario_player_starting_location
{
	real_point3d position;
	real facing;
	short team_index;
	short structure_bsp_reference_index;
	byte unused[0x20];
};

struct scenario_bsp_switch_trigger_volume
{
	short trigger_volume_index;
	short source_structure_bsp_index;
	short destination_structure_bsp_index;
	short cutscene_flag_index;
};

struct scenario_cutscene_flag
{
	long runtime_unused;
	char name[TAG_STRING_LENGTH];
	real_point3d position;
	real_euler_angles2d facing;
	byte unused[0x24];
};

struct player_screen_flash_parameters
{
	short type;
	word pad;
	real maximum_intensity;
	real red;
	real green;
	real duration;
};

struct screen_flash_definition
{
	short type;
	short priority;
	unsigned long unused1[3];
	real duration;
	short fade_function;
	word pad;
	unsigned long unused2[2];
	real max_intensity;
	real zero_scale_factor;
	real_argb_color screen_flash_color;
};

/* ---------- prototypes */

void hud_fix_unit_data(
	short old_local_player_index,
	short new_local_player_index);
void hud_fix_weapon_data(
	short old_local_player_index,
	short new_local_player_index);
short player_ui_get_single_player_local_player_controller(
	short local_player_index);
real game_engine_get_starting_location_rating(
	long player_index,
	struct scenario_player_starting_location *starting_location);
boolean code_000aa9e0(
	long player_index,
	long source_unit_index,
	real_point3d const *position);
void unit_exit_seat_end(
	long unit_index);
void hud_picked_up_powerup(
	short local_player_index,
	long powerup_definition_index);
void player_effect_screen_flash(
	long player_index,
	struct screen_flash_definition const *screen_flash,
	real scale);
boolean object_double_charge_shield(
	long object_index);

static long code_000a9bc0(
	short bsp_switch_trigger_volume_index,
	long object_index);
static void code_000aa300(
	long player_index);
static void code_000aa3b0(
	long player_index);
static void code_000aa460(
	long player_index);
static void code_000abc90(
	long player_index,
	long source_unit_index,
	real_point3d const *position);

/* ---------- globals */

struct data_array *player_data;
extern struct data_array *team_data;
extern long bss_00453408[][MAXIMUM_LOCAL_PLAYERS];
extern short player_spawn_count;
extern byte data_002dee08[];

#define PLAYER_SCREEN_FLASH_PARAMETERS(offset) \
	((struct player_screen_flash_parameters *)(data_002dee08 + (offset)))
#define PLAYER_SCREEN_FLASH_FADE_FUNCTION(offset) \
	(*(short *)((byte *)bss_00453408 + (offset)))
#define PLAYER_SCREEN_FLASH_COLOR(offset) \
	(*(real *)((byte *)bss_00453408 + (offset)))

/* ---------- public code */

void players_initialize(
	void)
{
	player_data = game_state_data_new(
		"players",
		16,
		sizeof(struct player_datum));
	team_data = game_state_data_new(
		"teams",
		16,
		0x40);
	players_globals = (struct players_globals *)game_state_malloc(
		"players globals",
		NULL,
		sizeof(struct players_globals));

	csmemset(
		players_globals->local_players,
		NONE,
		sizeof(players_globals->local_players));
	players_globals->unknown0 = NONE;
	players_globals->local_player_count = 0;

	player_control_initialize();

	return;
}

void players_initialize_for_new_map(
	void)
{
	player_control_dispose();
	csmemset(
		players_globals,
		0,
		sizeof(struct players_globals));
	csmemset(
		players_globals->local_players,
		NONE,
		sizeof(players_globals->local_players));
	csmemset(
		players_globals->dead_units,
		NONE,
		sizeof(players_globals->dead_units));
	players_globals->unknown0 = NONE;
	players_globals->input_disabled = FALSE;
	players_globals->double_speed_ticks = 0;
	players_globals->all_dead = FALSE;
	players_globals->pending_teleport_starting_location_index = NONE;
	players_globals->respawn_failure = 0;
	data_make_valid(player_data);
	data_make_valid(team_data);
	csmemset(
		bss_00453408,
		NONE,
		0x40);

	return;
}

void players_dispose_from_old_map(
	void)
{
	data_make_invalid(player_data);
	data_make_invalid(team_data);

	return;
}

void players_dispose(
	void)
{
	if (player_data)
		player_data = NULL;
	if (team_data)
		team_data = NULL;
	if (players_globals)
		players_globals = NULL;

	return;
}

long *machine_get_player_list(
	long machine_index)
{
	machine_index &= 0xFFFF;

	return bss_00453408[machine_index];
}

boolean local_player_exists(
	long local_player_index)
{
	boolean result;
	struct player_datum *player;
	struct data_iterator iterator;

	result = FALSE;
	data_iterator_new(
		&iterator,
		player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->local_player_index == local_player_index)
		{
			result = TRUE;
			break;
		}
	}

	return result;
}

long find_unused_local_player_index(
	void)
{
	long index;
	long result;

	result = NONE;
	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		if (input_has_gamepad((short)index) && !local_player_exists(index))
		{
			result = index;
			break;
		}
	}

	if (result == NONE)
	{
		for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
		{
			if (!local_player_exists(index))
			{
				result = index;
				break;
			}
		}
	}

	return result;
}

void player_delete(
	long player_index)
{
	datum_delete(
		player_data,
		player_index);

	return;
}

short players_get_respawn_failure(
	void)
{
	return players_globals->respawn_failure;
}

long local_player_get_player_index(
	short local_player_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\players.c",
		943,
		local_player_index>=NONE && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

	if (local_player_index == NONE)
		return NONE;

	return players_globals->local_players[local_player_index];
}

void local_player_set_player_index(
	short local_player_index,
	long player_index)
{
	long previous_player_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\players.c",
		952,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

	previous_player_index = players_globals->local_players[local_player_index];
	if (previous_player_index != NONE)
		player_get(previous_player_index)->local_player_index = NONE;

	players_globals->local_players[local_player_index] = player_index;

	if (player_index != NONE)
	{
		player_get(players_globals->local_players[local_player_index])->local_player_index =
			local_player_index;
	}

	return;
}

short local_player_count(
	void)
{
	return players_globals->local_player_count;
}

short local_player_get_next(
	short local_player_index)
{
	short result;
	short index;

	result = NONE;
	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		if (players_globals->local_players[index] != NONE &&
			index > local_player_index &&
			(index < result || result == NONE))
		{
			result = index;
		}
	}

	return result;
}

long player_index_from_unit_index(
	long unit_index)
{
	long player_index;
	struct player_datum *player;
	struct data_iterator iterator;

	player_index = NONE;
	data_iterator_new(
		&iterator,
		player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index == unit_index)
			player_index = iterator.datum_index;
	}

	return player_index;
}

void player_died(
	long player_index)
{
	struct player_datum *player;
	struct player_datum *other_player;
	struct data_iterator iterator;

	player = player_get(player_index);
	player->dead_unit_index = player->unit_index;
	player->unit_index = NONE;
	if (player->local_player_index != NONE)
	{
		player_control_new_unit(
			player->local_player_index,
			NONE);
	}

	players_globals->all_dead = TRUE;
	data_iterator_new(
		&iterator,
		player_data);
	while ((other_player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (other_player->unit_index != NONE)
			players_globals->all_dead = FALSE;
	}

	return;
}

void players_set_local_player_unit(
	short local_player_index,
	long unit_index)
{
	long old_unit_index;
	long player_index;
	struct player_datum *player;
	struct unit_datum *unit;

	old_unit_index = player_control_get_unit_index(local_player_index);
	match_assert(
		"c:\\halo\\SOURCE\\game\\players.c",
		1060,
		game_connection()==_game_connection_local);

	if (old_unit_index != NONE)
	{
		unit = unit_get(old_unit_index);
		unit->unit.player_index = NONE;
		unit_set_actively_controlled(old_unit_index, FALSE);
	}

	if (unit_index != NONE)
	{
		unit = unit_get(unit_index);
		unit_set_actively_controlled(unit_index, TRUE);
		unit->unit.player_index = local_player_get_player_index(local_player_index);
	}

	player_index = local_player_get_player_index(local_player_index);
	player = player_get(player_index);
	player->unit_index = unit_index;
	player->dead_unit_index = NONE;
	player_control_new_unit(local_player_index, unit_index);

	return;
}

boolean players_are_all_dead(
	void)
{
	return players_globals->all_dead;
}

unsigned long const *players_get_combined_pvs_local(
	void)
{
	return players_globals->combined_pvs_local;
}

unsigned long const *players_get_combined_pvs(
	void)
{
	return players_globals->combined_pvs;
}

void player_input_enable(
	boolean enable)
{
	players_globals->input_disabled = !enable;

	return;
}

boolean player_input_enabled(
	void)
{
	return !players_globals->input_disabled;
}

boolean any_player_is_in_the_air(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct unit_datum *unit;
	struct object_datum *root_object;
	struct players_vehicle_datum *vehicle;
	struct unit_definition *vehicle_definition;
	long root_object_index;

	data_iterator_new(&iterator, player_data);
	while (player = data_iterator_next(&iterator))
	{
		if (player->unit_index == NONE)
			continue;

		unit = unit_get(player->unit_index);
		root_object_index = object_get_ultimate_parent(player->unit_index);
		root_object = object_get(root_object_index);
		if (TEST_FLAG(root_object->object.flags, _object_outside_of_map_bit))
			return TRUE;

		if (unit->object.parent_object_index == NONE)
		{
			if (unit->object.type == _object_type_biped)
			{
				if (biped_flying_through_air(player->unit_index))
					return TRUE;
				continue;
			}
			if (unit->object.type != _object_type_vehicle)
				continue;
			vehicle = (struct players_vehicle_datum *)unit;
		}
		else
		{
			vehicle = (struct players_vehicle_datum *)object_try_and_get_and_verify_type(
				unit->object.parent_object_index,
				_object_mask_vehicle);
			if (!vehicle)
				continue;
			vehicle_definition = vehicle_definition_get(vehicle->definition_index);
			if (!TEST_FLAG(
					vehicle_definition->unit.flags,
					_unit_causes_passenger_dialogue_bit))
			{
				continue;
			}
		}

		if (vehicle->vehicle.unknown_state > 2)
			return TRUE;
	}

	return FALSE;
}

boolean any_player_is_dead(
	void)
{
	struct player_datum *player;
	struct data_iterator iterator;

	data_iterator_new(
		&iterator,
		player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index == NONE)
			return TRUE;
	}

	return FALSE;
}

void player_control_fix_for_loaded_game_state(
	void)
{
	struct player_datum *player;
	long player_index;
	short controller_local_player_index;
	short local_player_index;

	local_player_index = 0;
	controller_local_player_index =
		player_ui_get_single_player_local_player_controller(0);
	if (controller_local_player_index == NONE)
		controller_local_player_index = 0;

	if (local_player_get_player_index(controller_local_player_index) == NONE)
	{
		if (player_spawn_count == 1)
		{
			for (local_player_index = 0;
				local_player_index < MAXIMUM_LOCAL_PLAYERS;
				local_player_index++)
			{
				player_index = local_player_get_player_index(local_player_index);
				if (player_index != NONE)
				{
					player = player_get(player_index);
					local_player_set_player_index(local_player_index, NONE);
					player_control_new_unit(local_player_index, NONE);
					local_player_set_player_index(
						controller_local_player_index,
						player_index);
					player_control_new_unit(
						controller_local_player_index,
						player->unit_index);
					hud_fix_weapon_data(
						local_player_index,
						controller_local_player_index);
					hud_fix_unit_data(
						local_player_index,
						controller_local_player_index);
					error(2, "corrected player control for restored saved game");
					break;
				}
			}

			if (local_player_index == MAXIMUM_LOCAL_PLAYERS)
			{
				error(2,
					"failed to correct player control for restored saved game... probably won't be able to control the player");
			}
		}
		else
		{
			error(2,
				"tried to fix broken player control for a game w/ player_spawn_count= %d... but we don't allow restored games for anything other than player_spawn_count= 1",
				player_spawn_count);
		}
	}

	return;
}

short player_get_starting_location_count(
	void)
{
	struct scenario *scenario;
	struct encounter_definition *encounter;
	short starting_location_count;

	scenario = global_scenario_get();
	starting_location_count = scenario->players.count;
	if (ai_debug.selected_squad_index != NONE)
	{
		encounter = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_debug.selected_squad_index),
			struct encounter_definition);
		if (encounter->player_starting_locations.count > 0)
			starting_location_count = encounter->player_starting_locations.count;
	}

	return starting_location_count;
}

struct scenario_player_starting_location *player_get_starting_location(
	short starting_location_index)
{
	struct scenario *scenario;
	struct encounter_definition *encounter;
	struct scenario_player_starting_location *starting_location;
	short structure_bsp_reference_index;

	scenario = global_scenario_get();
	starting_location = NULL;
	if (starting_location_index >= 0 && starting_location_index < scenario->players.count)
	{
		starting_location = TAG_BLOCK_GET_ELEMENT(
			&scenario->players,
			starting_location_index,
			struct scenario_player_starting_location);
	}

	if (ai_debug.selected_squad_index != NONE)
	{
		encounter = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_debug.selected_squad_index),
			struct encounter_definition);

		if (starting_location_index >= 0 &&
			starting_location_index < encounter->player_starting_locations.count)
		{
			starting_location = TAG_BLOCK_GET_ELEMENT(
				&encounter->player_starting_locations,
				starting_location_index,
				struct scenario_player_starting_location);
			structure_bsp_reference_index = encounter->runtime_structure_bsp_reference_index;
			if (structure_bsp_reference_index >= 0 &&
				structure_bsp_reference_index < scenario->structure_bsp_references.count)
			{
				starting_location->structure_bsp_reference_index = structure_bsp_reference_index;
			}
		}
	}

	return starting_location;
}

static long code_000a9bc0(
	short bsp_switch_trigger_volume_index,
	long object_index)
{
	struct scenario_bsp_switch_trigger_volume *bsp_switch_trigger_volume;

	if (bsp_switch_trigger_volume_index != NONE)
	{
		bsp_switch_trigger_volume = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->bsp_switch_trigger_volumes,
			bsp_switch_trigger_volume_index,
			struct scenario_bsp_switch_trigger_volume);
		if (scenario_trigger_volume_test_object(
			bsp_switch_trigger_volume->trigger_volume_index,
			object_index))
		{
			return TRUE;
		}
	}

	return FALSE;
}

void placement_data_set_change_color(
	struct object_placement_data *placement_data,
	real_rgb_color const *change_color)
{
	placement_data->change_colors[_object_change_color_a] = *change_color;
	placement_data->change_colors[_object_change_color_b] = *change_color;
	placement_data->change_colors[_object_change_color_c] = *change_color;
	placement_data->change_colors[_object_change_color_d] = *change_color;

	return;
}

boolean valid_real_vector2d(
	real_vector2d const *vector)
{
	return valid_real(vector->i) && valid_real(vector->j);
}

static void code_000a9600(
	long machine_index,
	long player_index)
{
	long *machine_player_list;
	long machine_player_index;

	machine_index &= 0xFFFF;
	machine_index <<= 4;
	machine_player_list = (long *)((byte *)bss_00453408 + machine_index);
	for (machine_player_index = 0;
		machine_player_index < MAXIMUM_LOCAL_PLAYERS;
		machine_player_index++)
	{
		if (machine_player_list[machine_player_index] == NONE)
		{
			machine_player_list[machine_player_index] = player_index;
			break;
		}
	}

	if (machine_player_index == MAXIMUM_LOCAL_PLAYERS)
	{
		display_assert(
			"failed to create a player",
			"c:\\halo\\SOURCE\\game\\players.c",
			240,
			TRUE);
		system_exit(-1);
	}

	return;
}

long player_new(
	long machine_index,
	long player_index,
	short local_player_index,
	struct network_player const *network_player)
{
	struct player_datum *player;
	struct player_datum *player2;
	wchar_t const *player_name;

	if (player_index == NONE)
		player_index = datum_new(player_data);
	else
		player_index = datum_new_at_index(player_data, player_index);

	match_assert("c:\\halo\\SOURCE\\game\\players.c", 309,
		((local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)) || (local_player_index==NONE));

	if (player_index != NONE)
	{
		player = player_get(player_index);
		player_name = L"";
		if (network_player)
			player_name = network_player->name;
		ustrncpy(player->name, player_name, 11);
		player->name[11] = 0;
		player->local_player_index = local_player_index;
		player->unit_index = NONE;
		player->dead_unit_index = NONE;
		player->squad_index = NONE;
		player->cluster_index = NONE;
		player->aim_assist_unit_index = NONE;
		player->speed_multiplier = 1.0f;
		player->team_index = 1;

		player2 = player_get(player_index);
		player2->action_result = 0;
		player2->action_object_index = NONE;
		player->unknown_cc = NONE;
		player->unknown_d1 = FALSE;

		if (network_player)
			csmemcpy(&player->network_player_data, network_player, sizeof(struct network_player));
	}

	code_000a9600(machine_index, player_index);

	return player_index;
}

long find_best_starting_location_index(
	long player_index)
{
	struct scenario *scenario;
	struct encounter_definition *encounter;
	struct scenario_player_starting_location *starting_location;
	short starting_location_count;
	short starting_location_index;
	short best_starting_location_index;
	real starting_location_rating;
	real best_starting_location_rating;

	scenario = global_scenario_get();
	starting_location_count = scenario->players.count;
	if (ai_debug.selected_squad_index != NONE)
	{
		encounter = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_debug.selected_squad_index),
			struct encounter_definition);
		if (encounter->player_starting_locations.count > 0)
			starting_location_count = encounter->player_starting_locations.count;
	}

	best_starting_location_index = NONE;
	best_starting_location_rating = 0.0f;
	for (starting_location_index = 0;
		starting_location_index < starting_location_count;
		starting_location_index++)
	{
		starting_location = player_get_starting_location(starting_location_index);
		starting_location_rating = game_engine_get_starting_location_rating(
			player_index,
			starting_location);
		starting_location_rating *= (real)pow(
			(double)real_random_range(0.0f, 1.0f),
			0.5);
		if (starting_location_rating > best_starting_location_rating)
		{
			best_starting_location_rating = starting_location_rating;
			best_starting_location_index = starting_location_index;
		}
	}

	return best_starting_location_index;
}

boolean player_teleport(
	long player_index,
	long source_unit_index,
	real_point3d const *position)
{
	struct player_datum *player;
	struct biped_datum *biped;
	long unit_index;
	boolean result;

	player = player_get(player_index);
	unit_index = player->unit_index;
	biped = biped_try_and_get(unit_index);
	result = FALSE;
	if (biped)
	{
		if (biped->object.parent_object_index != NONE)
			unit_exit_seat_end(unit_index);
		result = code_000aa9e0(player_index, source_unit_index, position);
	}

	return result;
}

void debug_player_teleport(
	short local_player_index,
	short target_local_player_index)
{
	long player_index;
	long unit_index;
	long target_player_index;
	long target_unit_index;
	struct player_datum *player;
	struct unit_datum *target_unit;

	player_index = local_player_get_player_index(local_player_index);
	if (player_index == NONE)
	{
		unit_index = NONE;
	}
	else
	{
		player = player_get(local_player_get_player_index(local_player_index));
		unit_index = player->unit_index;
	}

	target_player_index = local_player_get_player_index(target_local_player_index);
	if (target_player_index == NONE)
	{
		target_unit_index = NONE;
	}
	else
	{
		player = player_get(local_player_get_player_index(target_local_player_index));
		target_unit_index = player->unit_index;
	}

	if (unit_index != NONE && target_unit_index != NONE)
	{
		target_unit = unit_get(target_unit_index);
		code_000aa9e0(
			player_index_from_unit_index(unit_index),
			target_unit_index,
			&target_unit->object.bounding_sphere_center);
	}

	return;
}

static void code_000abc90(
	long player_index,
	long source_unit_index,
	real_point3d const *position)
{
	struct player_datum *player;
	struct biped_datum *biped;
	struct biped_datum *source_biped;
	struct scenario_bsp_switch_trigger_volume *bsp_switch;
	long unit_index;
	volatile boolean outside_switch_trigger;
	boolean inside_switch_trigger;
	boolean teleport_succeeded;

	player = player_get(player_index);
	unit_index = player->unit_index;
	biped = biped_try_and_get(unit_index);
	match_assert("c:\\halo\\SOURCE\\game\\players.c", 0x4CB,
		player_index!=NONE);
	match_assert("c:\\halo\\SOURCE\\game\\players.c", 0x4CC,
		position);
	if (biped)
	{
		if (players_globals->pending_teleport_starting_location_index != NONE)
		{
			bsp_switch = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->bsp_switch_trigger_volumes,
				players_globals->pending_teleport_starting_location_index,
				struct scenario_bsp_switch_trigger_volume);
			inside_switch_trigger = scenario_trigger_volume_test_object(
				bsp_switch->trigger_volume_index,
				unit_index);
			if (inside_switch_trigger)
			{
				outside_switch_trigger = TRUE;
				outside_switch_trigger = FALSE;
			}
			else
			{
				outside_switch_trigger = TRUE;
			}
		}
		else
		{
			outside_switch_trigger = FALSE;
		}

		if (scenario_leaf_index_from_point(
			&biped->object.bounding_sphere_center) == NONE ||
			outside_switch_trigger)
		{
			if (biped->object.parent_object_index != NONE)
			{
				source_biped = biped_get(source_unit_index);
				if (biped->object.parent_object_index !=
					source_biped->object.parent_object_index)
				{
					unit_exit_seat_end(unit_index);
				}
			}

			if (biped->object.parent_object_index == NONE)
			{
				teleport_succeeded = code_000aa9e0(
					player_index,
					source_unit_index,
					position);
			}
			else
			{
				teleport_succeeded = TRUE;
			}
			players_globals->respawn_failed = !teleport_succeeded;
		}
	}

	return;
}

void players_reconnect_to_structure_bsp(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct scenario *scenario;
	struct scenario_bsp_switch_trigger_volume *bsp_switch;
	struct scenario_cutscene_flag *cutscene_flag;
	struct structure_leaf *leaf;
	real_point3d biped_base;
	real_point3d teleport_position;
	real biped_height;
	long source_unit_index;
	real adjustment_or_width;
	boolean teleport_position_valid;
	boolean found_player;
	short local_player_index;
	short cutscene_flag_index;
	long player_index;
	long player_unit_index;
	long cluster_index;
	long (*local_player_index_getter)(short local_player_index);

	if (players_globals->pending_teleport_starting_location_index != NONE &&
		players_globals->local_player_count > 1)
	{
		scenario = global_scenario_get();
		bsp_switch = TAG_BLOCK_GET_ELEMENT(
			&scenario->bsp_switch_trigger_volumes,
			players_globals->pending_teleport_starting_location_index,
			struct scenario_bsp_switch_trigger_volume);
		source_unit_index = NONE;
		found_player = FALSE;
		teleport_position_valid = FALSE;
		cutscene_flag_index = bsp_switch->cutscene_flag_index;
		if (cutscene_flag_index != NONE)
		{
			adjustment_or_width = 0.f;
			cutscene_flag = TAG_BLOCK_GET_ELEMENT(
				&scenario->cutscene_flags,
				cutscene_flag_index,
				struct scenario_cutscene_flag);
			teleport_position = cutscene_flag->position;
		collision_test:
			if (collision_test_point(
				FLAG(_collision_test_front_facing_surfaces_bit) |
					FLAG(_collision_test_ignore_invisible_surfaces_bit) |
					FLAG(_collision_test_structure_bit) |
					FLAG(_collision_test_objects_scenery_bit),
				&teleport_position,
				NONE))
			{
				teleport_position.z += 0.05f;
				adjustment_or_width += 0.05f;
				if (adjustment_or_width < 0.3f)
					goto collision_test;
			}
			teleport_position_valid = adjustment_or_width < 0.3f;
		}

		data_iterator_new(&iterator, player_data);
		while ((player = data_iterator_next(&iterator)) && !found_player)
		{
			player_unit_index = player->unit_index;
			if (player_unit_index != NONE &&
				players_globals->pending_teleport_starting_location_index != NONE)
			{
				if (code_000a9bc0(
					players_globals->pending_teleport_starting_location_index,
					player_unit_index))
				{
					biped_get_physics_pill(
						player->unit_index,
						&biped_base,
						&biped_height,
						&adjustment_or_width);
					if (scenario_leaf_index_from_point(&biped_base) != NONE)
					{
						leaf = TAG_BLOCK_GET_ELEMENT(
							&global_structure_bsp_get()->leaves,
							scenario_leaf_index_from_point(&biped_base) & LONG_MAX,
							struct structure_leaf);
						cluster_index =
							*(volatile short *)&leaf->cluster_index;
						if (cluster_index != NONE)
						{
							if (!teleport_position_valid)
								teleport_position = biped_base;
							else
								teleport_position.z =
									*(volatile real *)&adjustment_or_width +
									teleport_position.z;
							source_unit_index = player->unit_index;
							found_player = TRUE;
						}
					}
				}
			}
		}

		match_vassert(
			"c:\\halo\\SOURCE\\game\\players.c",
			0x63E,
			found_player,
			"no players in the bsp");
		if (found_player)
		{
			local_player_index_getter = local_player_get_player_index;
			local_player_index = local_player_get_next(NONE);
			while (local_player_index != NONE)
			{
				player_index = local_player_index_getter(local_player_index);
				player = player_get(player_index);
				if (player->unit_index != NONE &&
					player->unit_index != source_unit_index)
				{
					code_000abc90(
						player_index,
						source_unit_index,
						&teleport_position);
					player_get(player_index)->cluster_index = NONE;
				}
				local_player_index = local_player_get_next(local_player_index);
			}
		}
		players_globals->pending_teleport_starting_location_index = NONE;
	}

	data_iterator_new(&iterator, player_data);
	while (player = data_iterator_next(&iterator))
		player->cluster_index = NONE;

	return;
}

boolean unit_should_autopick_weapon(
	long unit_index,
	long weapon_index)
{
	struct weapon_datum *weapon;
	struct weapon_definition *volatile weapon_definition;
	long weapon_count;

	weapon = weapon_try_and_get(weapon_index);
	weapon_definition = weapon_definition_get(weapon->definition_index);
	weapon_count = unit_get_weapon_count(unit_index);

	if (!(unit_approve_weapon_pickup(unit_index, weapon_index) &&
		TEST_FLAG(weapon_definition->weapon.flags, _weapon_doesnt_count_toward_maximum_bit)) &&
		weapon_count != 0 &&
		(game_engine_running() ||
			!unit_approve_weapon_pickup(unit_index, weapon_index) ||
			weapon_count >= 2) &&
		!game_engine_force_autopickup(unit_index, weapon_index))
	{
		return FALSE;
	}

	return TRUE;
}

long code_000aa160(
	void)
{
	long local_player_count = 0;
	short local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (players_globals->local_players[local_player_index] != NONE)
			local_player_count++;
	}

	return local_player_count;
}

void code_000aa180(
	void)
{
	return;
}

boolean player_handle_powerup(
	long player_index,
	short powerup_type,
	short duration)
{
	struct player_datum *player;
	long powerup_index;
	struct unit_datum *unit;

	player = player_get(player_index);
	match_assert("c:\\halo\\SOURCE\\game\\players.c", 0xAEE,
		powerup_type>=0 && powerup_type<NUMBER_OF_PLAYER_POWERUPS);

	if (powerup_type == 0)
	{
		long unit_index = player->unit_index;

		unit = unit_get(unit_index);
		if (TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
			return FALSE;
	}

	powerup_index = powerup_type;
	if (player->powerup_durations[powerup_index] == 0)
	{
		struct player_datum *powerup_player;

		powerup_player = player_get(player_index);
		unit = unit_get(powerup_player->unit_index);
		if (powerup_index == 0)
		{
			SET_FLAG(unit->unit.flags, _unit_active_camouflaged_bit, TRUE);
			unit->unit.cause_for_camo_regrowth = (short)powerup_index;
		}
	}
	else if (!game_engine_running())
	{
		struct player_datum *powerup_player;

		powerup_player = player_get(player_index);
		unit = unit_get(powerup_player->unit_index);
		if (powerup_index == 0)
			SET_FLAG(unit->unit.flags, _unit_super_camouflaged_bit, TRUE);
	}

	player->powerup_durations[powerup_index] += duration;

	return TRUE;
}

static void code_000aa300(
	long player_index)
{
	struct player_datum *player;

	if (player_index != NONE)
	{
		player = player_get(player_index);
		if (player->local_player_index != NONE)
		{
			struct screen_flash_definition screen_flash = { 0 };

			screen_flash.fade_function =
				PLAYER_SCREEN_FLASH_FADE_FUNCTION(0x44);
			screen_flash.type =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xBF0)->type;
			screen_flash.priority = 2;
			screen_flash.duration =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xBF0)->duration;
			screen_flash.max_intensity =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xBF0)->maximum_intensity;
			screen_flash.zero_scale_factor = 0.f;
			screen_flash.screen_flash_color.alpha =
				PLAYER_SCREEN_FLASH_COLOR(0x48);
			screen_flash.screen_flash_color.red =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xBF0)->red;
			screen_flash.screen_flash_color.green =
				PLAYER_SCREEN_FLASH_COLOR(0x4C);
			screen_flash.screen_flash_color.blue =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xBF0)->green;
			player_effect_screen_flash(player_index, &screen_flash, 1.f);
		}
	}

	return;
}

static void code_000aa3b0(
	long player_index)
{
	struct player_datum *player;

	if (player_index != NONE)
	{
		player = player_get(player_index);
		if (player->local_player_index != NONE)
		{
			struct screen_flash_definition screen_flash = { 0 };

			screen_flash.fade_function =
				PLAYER_SCREEN_FLASH_FADE_FUNCTION(0x50);
			screen_flash.type =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xC04)->type;
			screen_flash.priority = 2;
			screen_flash.duration =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xC04)->duration;
			screen_flash.max_intensity =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xC04)->maximum_intensity;
			screen_flash.zero_scale_factor = 0.f;
			screen_flash.screen_flash_color.alpha =
				PLAYER_SCREEN_FLASH_COLOR(0x54);
			screen_flash.screen_flash_color.red =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xC04)->red;
			screen_flash.screen_flash_color.green =
				PLAYER_SCREEN_FLASH_PARAMETERS(0xC04)->green;
			screen_flash.screen_flash_color.blue =
				PLAYER_SCREEN_FLASH_COLOR(0x58);
			player_effect_screen_flash(player_index, &screen_flash, 1.f);
		}
	}

	return;
}

static void code_000aa460(
	long player_index)
{
	struct player_datum *player;

	if (player_index != NONE)
	{
		player = player_get(player_index);
		if (player->local_player_index != NONE)
		{
			struct screen_flash_definition screen_flash = { 0 };

			screen_flash.fade_function = 1;
			screen_flash.type = 6;
			screen_flash.priority = 2;
			screen_flash.duration = 2.f;
			screen_flash.max_intensity = 0.5f;
			screen_flash.zero_scale_factor = 0.f;
			screen_flash.screen_flash_color.alpha = 1.f;
			screen_flash.screen_flash_color.red = 0.917647f;
			screen_flash.screen_flash_color.green = 0.917647f;
			screen_flash.screen_flash_color.blue = 0.917647f;
			player_effect_screen_flash(player_index, &screen_flash, 1.f);
		}
	}

	return;
}

void code_000ac320(
	long player_index,
	long equipment_index)
{
	struct player_datum *player;
	struct item_datum *equipment;
	struct equipment_definition *equipment_definition;
	short powerup_type;
	short duration;
	long powerup_index;

	player = player_get(player_index);
	equipment = equipment_get(equipment_index);
	equipment_definition = equipment_definition_get(equipment->definition_index);
	duration = (short)(equipment_definition->equipment.powerup_time * 30.f);
	if (duration <= 0)
		return;

	powerup_type = equipment_definition->equipment.powerup_type;
	if (powerup_type == _equipment_powerup_double_speed)
	{
		players_globals->double_speed_ticks += duration;
		game_set_players_are_double_speed(TRUE);
	}
	else if (powerup_type == _equipment_powerup_overshield)
	{
		if (!object_double_charge_shield(player->unit_index))
			return;
		code_000aa300(player_index);
	}
	else if (powerup_type == _equipment_powerup_health)
	{
		if (!object_restore_body(player->unit_index))
			return;
		code_000aa460(player_index);
	}
	else
	{
		switch (powerup_type)
		{
		case _equipment_powerup_active_camouflage:
			powerup_index = 0;
			break;

		case _equipment_powerup_full_spectrum_vision:
			powerup_index = 1;
			break;

		default:
			display_assert(
				NULL,
				"c:\\halo\\SOURCE\\game\\players.c",
				0xACB,
				TRUE);
			system_exit(-1);
		}

		if (!player_handle_powerup(player_index, (short)powerup_index, duration))
			return;
		if ((short)powerup_index == 0)
			code_000aa3b0(player_index);
	}

	equipment = equipment_get(equipment_index);
	hud_picked_up_powerup(
		(unsigned short)player->local_player_index,
		equipment->definition_index);
	if (player->local_player_index != NONE)
		equipment_handle_pickup(equipment_index);
	object_delete(equipment_index);

	return;
}

void players_handle_deleted_object(
	long deleted_object_index)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct object_datum *object;

	object = object_get(deleted_object_index);
	if (TEST_FLAG(_object_mask_unit, object->object.type))
	{
		data_iterator_new(&iterator, player_data);
		while (player = data_iterator_next(&iterator))
		{
			if (player->unit_index == deleted_object_index)
				player_died(iterator.datum_index);
		}
	}

	return;
}

void player_aiming_vector_from_facing(
	long player_index,
	real_vector3d *aiming_vector,
	real_euler_angles2d const *facing)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct players_vehicle_datum *vehicle;
	struct unit_definition *vehicle_definition;
	struct unit_seat *seat;
	real_matrix4x3 rotation;

	player = player_get(player_index);
	vector3d_from_euler_angles2d(aiming_vector, facing);
	if (player->unit_index != NONE)
	{
		unit = unit_get(player->unit_index);
		if (unit->object.parent_object_index != NONE)
		{
			vehicle = (struct players_vehicle_datum *)object_try_and_get_and_verify_type(
				unit->object.parent_object_index,
				_object_mask_vehicle);
			if (vehicle)
			{
				vehicle_definition = vehicle_definition_get(
					vehicle->definition_index);
				seat = TAG_BLOCK_GET_ELEMENT(
					&vehicle_definition->unit.seats,
					unit->unit.parent_seat_index,
					struct unit_seat);
				if (!TEST_FLAG(seat->flags, _unit_seat_third_person_camera_bit))
				{
					cross_product3d(
						&vehicle->object.up,
						global_right3d,
						&rotation.forward);
					if (normalize3d(&rotation.forward) == 0.f)
					{
						cross_product3d(
							&vehicle->object.up,
							global_down3d,
							&rotation.forward);
						normalize3d(&rotation.forward);
					}
					matrix4x3_rotation_from_vectors(
						&rotation,
						&rotation.forward,
						&vehicle->object.up);
					matrix4x3_transform_normal(
						&rotation,
						aiming_vector,
						aiming_vector);
				}
			}
		}
	}

	return;
}

/* ---------- private code */
