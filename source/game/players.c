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
#include "camera/observer.h"
#include "data.h"
#include "devices/devices.h"
#include "editor/editor_stubs.h"
#include "effects/effects.h"
#include "effects/player_effects.h"
#include "game.h"
#include "game/game_globals.h"
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
#include "render/render_debug.h"
#include "players.h"
#include "player_control.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "text/unicode.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"

/* ---------- constants */

enum equipment_powerup_type
{
	_equipment_powerup_double_speed = 1,
	_equipment_powerup_overshield,
	_equipment_powerup_active_camouflage,
	_equipment_powerup_full_spectrum_vision,
	_equipment_powerup_health
};

enum
{
	_collision_test_for_player_teleport_flags =
		FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_ignore_invisible_surfaces_bit) |
		FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_objects_scenery_bit),
	_object_mask_player_interaction =
		_object_mask_unit |
		_object_mask_item |
		_object_mask_control
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

struct _vehicle_datum
{
	byte flags;
	byte unused425[3];
	byte unknown_state;
	char approach_type;
	byte unknown42A;
	byte unused42B;
};

struct vehicle_datum
{
	long definition_index;
	struct _object_datum object;
	struct _unit_datum unit;
	struct _vehicle_datum vehicle;
};

struct player_vehicle_difficulty_information
{
	byte unused[0x70];
	real vehicle_ram_angle;
	byte unused74[0xC];
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

void game_statistics_record_kill(
	long dead_unit_index,
	long killing_player_index,
	long damage_type,
	short killing_team_index);
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
void hud_picked_up_weapon(
	short local_player_index,
	long weapon_definition_index);
void hud_picked_up_grenade(
	short local_player_index,
	long grenade_definition_index);
void hud_picked_up_ammunition(
	short local_player_index,
	long weapon_definition_index,
	short ammunition_count);
void player_effect_screen_flash(
	long player_index,
	struct screen_flash_definition const *screen_flash,
	real scale);
boolean object_double_charge_shield(
	long object_index);
void device_touched(
	long device_index,
	long unit_index);
boolean unit_add_equipment_to_inventory(
	long unit_index,
	long equipment_index,
	boolean replace);
boolean unit_add_grenade_to_inventory(
	long unit_index,
	long equipment_index);
boolean unit_approve_weapon_swap(
	long unit_index,
	long weapon_index);
boolean unit_can_enter_seat(
	long unit_index,
	long target_unit_index,
	short seat_index,
	long *occupant_unit_index);
void unit_enter_seat(
	long unit_index,
	long target_unit_index,
	short seat_index);
void ai_try_vehicle_eviction(
	long actor_index,
	long entering_unit_index,
	boolean immediate);
boolean weapon_handle_potential_inventory_item(
	long inventory_item_index,
	long weapon_index,
	short local_player_index,
	short *ammunition_count);
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
void observer_obsolete_position(
	short local_player_index);
boolean biped_fix_position(
	long unit_index,
	long seat_index,
	real_point3d *initial_position,
	real_point3d *final_position,
	real scale,
	boolean keep_basis,
	boolean dont_teleport,
	boolean scale_by_height);
short unit_find_nearby_seat(
	long unit_index,
	long target_unit_index,
	short *seat_index);

static long code_000a9bc0(
	short bsp_switch_trigger_volume_index,
	long object_index);
static void code_000aa220(
	long player_index);
static boolean code_000aa240(
	long player_index);
static void code_000aa300(
	long player_index);
static void code_000aa3b0(
	long player_index);
static void code_000aa460(
	long player_index);
static void code_000aa4f0(
	long player_index,
	short powerup_type);
static void code_000aa530(
	long player_index,
	short powerup_type);
static void code_000aa560(
	long player_index,
	long powerup_type);
static __declspec(noinline) void code_000ab350(
	long player_index,
	short action_result,
	long object_index,
	short seat_index);
static boolean code_000ab440(
	long player_index);
static void code_000ab820(
	long player_index);
static void code_000ac0b0(
	long player_index,
	long vehicle_index);
static void code_000acb50(
	long player_index,
	long item_index);
static void code_000ace70(
	long player_index);
void code_000ac320(
	long player_index,
	long equipment_index);
static void code_000abc90(
	long player_index,
	long source_unit_index,
	real_point3d const *position);

/* ---------- globals */

struct data_array *player_data;
extern struct data_array *team_data;
byte bss_00453408[0x5C] = { 0 };
extern short player_spawn_count;
extern byte data_002dee08[];
boolean debug_render_player_teleport = FALSE;
/* Exact: January owns this external 9-point co-op teleport search table in
   Players.obj .rdata (0x6C bytes, no relocations). */
real_point3d const rdata_0025ced8[9] =
{
	{ { 1.f, 0.f, 0.f } },
	{ { 0.f, 1.f, 0.f } },
	{ { 0.f, -1.f, 0.f } },
	{ { 0.70710677f, -0.70710677f, 0.f } },
	{ { 0.70710677f, 0.70710677f, 0.f } },
	{ { 0.57735026f, 0.57735026f, 0.57735026f } },
	{ { 0.57735026f, 0.57735026f, -0.57735026f } },
	{ { 0.57735026f, -0.57735026f, 0.57735026f } },
	{ { 0.57735026f, -0.57735026f, -0.57735026f } },
};

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
	return (long *)(bss_00453408 + ((machine_index & 0xFFFF) << 4));
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

__declspec(noinline) long local_player_get_player_index(
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

__declspec(noinline) short local_player_get_next(
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

void code_000ab020(
	long player_index)
{
	struct player_datum *player;
	struct game_globals *volatile game_globals;
	struct player_datum *player2;
	struct unit_datum *unit;
	struct scenario *scenario;
	struct game_globals_player_information *player_information;
	struct game_globals_multiplayer_information *multiplayer_information;
	struct scenario_player_starting_location *starting_location;
	struct object_placement_data placement_data;
	real_rgb_color change_color;
	real_rgb_color change_color_storage;
	long saved_unit_index;
	long weapon_index;
	long unit_definition_index;
	long unit_index;
	long starting_equipment_count;
	short starting_location_index;

	player = player_get(player_index);
	saved_unit_index = NONE;
	if (!game_engine_running() && player->local_player_index != NONE)
	{
		saved_unit_index =
			players_globals->dead_units[player->local_player_index];
		players_globals->dead_units[player->local_player_index] = NONE;
		if (saved_unit_index != NONE)
		{
			unit = unit_get(saved_unit_index);
			if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			{
				object_delete(saved_unit_index);
				saved_unit_index = NONE;
			}
		}
	}

	if (!game_engine_running() && saved_unit_index != NONE)
	{
		unit = unit_get(saved_unit_index);
		weapon_index = unit_inventory_get_weapon(
			saved_unit_index,
			unit->unit.current_weapon_index);
		match_assert(
			"c:\\halo\\SOURCE\\game\\players.c",
			0x73A,
			player->local_player_index!=NONE);
		object_activate(saved_unit_index);
		object_set_visibility(saved_unit_index, TRUE);
		players_set_local_player_unit(
			player->local_player_index,
			saved_unit_index);
		if (weapon_index != NONE)
			object_set_visibility(weapon_index, TRUE);
	}
	else
	{
		scenario = global_scenario_get();
		if (ai_debug.selected_squad_index != NONE)
		{
			TAG_BLOCK_GET_ELEMENT(
				&scenario->ai_encounters,
				DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_debug.selected_squad_index),
				struct encounter_definition);
		}

		starting_location_index =
			(short)find_best_starting_location_index(player_index);
		if (starting_location_index != NONE)
		{
			game_globals = scenario_get_game_globals();
			player_information = TAG_BLOCK_GET_ELEMENT(
				&scenario_get_game_globals()->player_information,
				0,
				struct game_globals_player_information);
			if (player_information->player_unit.index != NONE)
			{
				starting_location =
					player_get_starting_location(starting_location_index);
				if (game_engine_running())
				{
					multiplayer_information = TAG_BLOCK_GET_ELEMENT(
						&game_globals->multiplayer_information,
						0,
						struct game_globals_multiplayer_information);
					unit_definition_index = multiplayer_information->unit.index;
				}
				else
				{
					unit_definition_index = player_information->player_unit.index;
				}

				object_placement_data_new(
					&placement_data,
					unit_definition_index,
					NONE);
				placement_data.position = starting_location->position;
				vector3d_from_angle(
					&placement_data.forward,
					starting_location->facing);
				placement_data.up = *global_up3d;
				change_color = *game_engine_player_get_change_color(
					&change_color_storage,
					player_index);
				placement_data_set_change_color(
					&placement_data,
					&change_color);
				unit_index = object_new(&placement_data);
				if (unit_index != NONE)
				{
					unit = unit_try_and_get(unit_index);
					if (unit != NULL)
					{
						player2 = player_get(player_index);
						unit->object.owner_player_index = player_index;
						unit->object.owner_team_index = (short)player2->team_index;
						unit->unit.player_index = player_index;
						player2->unit_index = unit_index;
						unit_set_actively_controlled(unit_index, TRUE);
						if (player2->local_player_index != NONE)
						{
							player_control_new_unit(
								player2->local_player_index,
								unit_index);
						}

						if (!game_engine_running())
						{
							scenario = global_scenario_get();
							starting_equipment_count =
								*(long *)((byte *)scenario + 0x348);
							if (starting_equipment_count > 1 &&
								*(short *)((byte *)player2 + 0xAA) > 0)
							{
								player_add_equipment(
									player2->unit_index,
									1,
									TRUE);
							}
							else if (starting_equipment_count != 0)
							{
								player_add_equipment(
									player2->unit_index,
									0,
									TRUE);
							}
						}
					}
				}
			}
		}
	}

	csmemset(
		player->powerup_durations,
		0,
		sizeof(player->powerup_durations));
	player2 = player_get(player_index);
	player2->action_result = 0;
	player2->action_object_index = NONE;
	if (player->local_player_index != NONE)
		observer_obsolete_position(player->local_player_index);

	return;
}

/* Exact: January emits this private dead-unit replacement helper from the
   reconstructed code_000aa9e0 caller below. */
static void code_000a9c00(
	long player_index,
	long replacement_dead_unit_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	long dead_unit_index;
	long weapon_index;

	player = player_get(player_index);
	if (player->unit_index != NONE)
	{
		if (game_engine_can_score())
		{
			game_statistics_record_kill(
				player->unit_index,
				NONE,
				NONE,
				NONE);
		}

		players_globals->dead_units[player->local_player_index] =
			player->unit_index;
		player_died(player_index);

		dead_unit_index =
			players_globals->dead_units[player->local_player_index];
		unit = unit_get(dead_unit_index);
		weapon_index = unit_inventory_get_weapon(
			dead_unit_index,
			unit_get(dead_unit_index)->unit.current_weapon_index);
		unit->unit.player_index = NONE;
		object_deactivate(dead_unit_index);
		object_set_visibility(dead_unit_index, FALSE);
		if (weapon_index != NONE)
			object_set_visibility(weapon_index, FALSE);

		if (replacement_dead_unit_index != NONE)
			player->dead_unit_index = replacement_dead_unit_index;
		players_globals->all_dead = FALSE;
	}

	return;
}

/* NonMatching: exact 0x170 padded size and all 22 relocation identities.
   The residual is a single instruction-scheduling window after
   objects_get_activating_cluster_index: January hoists &iterator before
   storing EAX and loading player_data. This body remains dormant from the
   exact-credit ledger. */
static void code_000a9ff0(
	unsigned long *combined_pvs,
	boolean local_player_only)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct object_datum *root_object;
	struct structure_bsp *structure_bsp;
	struct structure_leaf *leaf;
	long leaf_index;
	long activating_cluster_index;
	long root_object_index;
	short cluster_index;

	structure_bsp = global_structure_bsp_get();
	csmemset(combined_pvs, 0, 0x40);
	if (!game_in_editor())
	{
		activating_cluster_index =
			((long (*)(void))objects_get_activating_cluster_index)();
		data_iterator_new(&iterator, player_data);
		while (player = data_iterator_next(&iterator))
		{
			if (local_player_only && player->local_player_index == NONE)
				continue;

			if (player->unit_index != NONE)
			{
				root_object_index =
					object_get_ultimate_parent(player->unit_index);
				root_object = object_get(root_object_index);
				if (root_object->object.location.cluster_index != NONE)
				{
					player->cluster_index =
						root_object->object.location.cluster_index;
				}
			}

			cluster_index = player->cluster_index;
			if (cluster_index != NONE)
			{
				bit_vector_or(
					(short)structure_bsp->clusters.count,
					combined_pvs,
					structure_bsp_get_cluster_pvs(
						structure_bsp,
						cluster_index),
					combined_pvs);
			}
		}

		if ((short)activating_cluster_index != NONE)
		{
			bit_vector_or(
				(short)structure_bsp->clusters.count,
				combined_pvs,
				structure_bsp_get_cluster_pvs(
					structure_bsp,
					(short)activating_cluster_index),
				combined_pvs);
		}
	}
	else
	{
		leaf_index = scenario_leaf_index_from_point(
			&observer_get_camera(0)->position);
		if (leaf_index != NONE)
		{
			leaf = TAG_BLOCK_GET_ELEMENT(
				&global_structure_bsp_get()->leaves,
				scenario_leaf_index_from_point(
					&observer_get_camera(0)->position) & LONG_MAX,
				struct structure_leaf);
			cluster_index = leaf->cluster_index;
			if (cluster_index != NONE)
			{
				bit_vector_or(
					(short)structure_bsp->clusters.count,
					combined_pvs,
					structure_bsp_get_cluster_pvs(
						structure_bsp,
						cluster_index),
					combined_pvs);
			}
		}
	}

	return;
}

static boolean code_000ab440(
	long player_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct unit_datum *nearby_unit;
	struct item_datum *equipment;
	struct vehicle_datum *target_vehicle;
	real_matrix4x3 player_matrix;
	real_matrix4x3 target_matrix;
	real_point3d *player_position;
	real_point3d *target_position;
	real_vector3d delta;
	real dot;
	char approach_type;
	boolean result;

	player = player_get(player_index);
	unit_get(player->unit_index);
	result = FALSE;
	switch (player->action_result)
	{
	case 10:
		device_touched(player->action_object_index, player->unit_index);
		result = TRUE;
		break;

	case 5:
		unit_drop_current_equipment(player->unit_index);
		if (unit_add_equipment_to_inventory(
			player->unit_index,
			player->action_object_index,
			FALSE))
		{
			equipment = equipment_get(player->action_object_index);
			hud_picked_up_powerup(
				player->local_player_index,
				equipment->definition_index);
		}
		result = TRUE;
		break;

	case 8:
	case 9:
	{
		long nearby_unit_index;

		if (unit_can_enter_seat(
			player->unit_index,
			player->action_object_index,
			player->action_seat_index,
			(nearby_unit_index = NONE, &nearby_unit_index)))
		{
			unit_enter_seat(
				player->unit_index,
				player->action_object_index,
				player->action_seat_index);
		}
		else if (nearby_unit_index != NONE)
		{
			nearby_unit = unit_get(nearby_unit_index);
			if (nearby_unit->unit.actor_index != NONE)
			{
				ai_try_vehicle_eviction(
					nearby_unit->unit.actor_index,
					player->unit_index,
					TRUE);
			}
		}
		break;
	}

	case 11:
		unit = unit_get(player->unit_index);
		target_vehicle = (struct vehicle_datum *)vehicle_get(
			player->action_object_index);
		unit->unit.last_vehicle_index = player->action_object_index;
		unit->unit.game_time_at_last_vehicle_exit = game_time_get();

		if (fabs(target_vehicle->object.forward.k) > 0.70710677f)
		{
			approach_type = (target_vehicle->object.forward.k < 0.f) + 3;
		}
		else
		{
			target_position = &object_get_world_matrix(
				player->action_object_index,
				&target_matrix)->position;
			player_position = &object_get_world_matrix(
				player->unit_index,
				&player_matrix)->position;
			delta.i = target_position->x - player_position->x;
			delta.j = target_position->y - player_position->y;
			delta.k = target_position->z - player_position->z;
			cross_product3d(global_up3d, &delta, &delta);
			dot = dot_product3d(&delta, &target_vehicle->object.forward);
			approach_type = (dot > 0.f) + 1;
		}

		SET_FLAG(target_vehicle->vehicle.flags, 4, TRUE);
		target_vehicle->vehicle.approach_type = approach_type;
		target_vehicle->vehicle.unknown42A = 0;
		result = TRUE;
		break;
	}

	return result;
}

/* NonMatching foundation: exact 0x510 padded size and all 61 relocation
   identities, with residual register allocation and block-layout differences.
   This semantic caller is retained because it authentically emits the exact
   private code_000a9c00 helper, the exact header-inline random_direction3d
   leaf, and references the exact source-owned rdata_0025ced8 table. The caller
   body remains dormant from the exact-credit ledger. */
boolean code_000aa9e0(
	long player_index,
	long source_unit_index,
	real_point3d const *position)
{
	struct player_datum *player;
	struct biped_datum *source_biped;
	struct
	{
		struct object_datum *source_root_object;
		struct biped_datum *biped;
	} pointers;
	struct scenario *scenario;
	struct scenario_bsp_switch_trigger_volume *bsp_switch_trigger_volume;
	struct game_globals_player_information *player_information;
	real_vector3d const *adjustment_vector;
	real_matrix4x3 placement_matrix;
	real_vector3d best_adjustment_vector;
	real_vector3d random_adjustment_vector;
	real_point3d adjusted_position;
	real_point3d random_adjusted_position;
	long player_unit_index;
	long source_root_object_index;
	long respawn_effect_index;
	short adjustment_index;
	short random_adjustment_index;
	boolean result;

	player = player_get(player_index);
	player_unit_index = player->unit_index;
	pointers.biped = biped_get(player_unit_index);
	result = FALSE;

	if (*(volatile long *)&source_unit_index != NONE)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\game\\players.c",
			0x4FB,
			local_player_count()>1,
			"source_unit_index==NONE || local_player_count()>1");
		if (object_get_ultimate_parent(
			*(volatile long *)&source_unit_index) !=
			*(volatile long *)&source_unit_index)
		{
			real scale;
			real collision_height;

			source_root_object_index =
				object_get_ultimate_parent(
					*(volatile long *)&source_unit_index);
			unit_get(*(volatile long *)&source_unit_index);
			pointers.source_root_object = object_get(source_root_object_index);

			best_adjustment_vector =
				pointers.source_root_object->object.translational_velocity;
			best_adjustment_vector.k = 0.f;
			source_unit_index = source_root_object_index;
			if (!(magnitude_squared3d(&best_adjustment_vector) > 0.f))
			{
				adjustment_vector =
					pointers.source_root_object->object.forward.k < 0.70710677f
						? &pointers.source_root_object->object.forward
						: &pointers.source_root_object->object.up;
				best_adjustment_vector = *adjustment_vector;
				best_adjustment_vector.k = 0.f;
			}

			collision_height = biped_definition_get(
				pointers.biped->definition_index)->biped.collision_height_standing;
			scale = collision_height * 3.f +
				pointers.source_root_object->object.bounding_sphere_radius;
		match_assert(
			"c:\\halo\\SOURCE\\game\\players.c",
			0x525,
			magnitude3d(&best_adjustment_vector));
		match_assert(
			"c:\\halo\\SOURCE\\game\\players.c",
			0x526,
			scale);

		negate_vector3d(
			&best_adjustment_vector,
			&best_adjustment_vector);
		normalize3d(&best_adjustment_vector);
			matrix4x3_from_point_and_vectors(
				&placement_matrix,
				&pointers.source_root_object->object.bounding_sphere_center,
				&best_adjustment_vector,
			global_up3d);
		placement_matrix.scale = scale;

		adjustment_index = 0;
		do
		{
			if (result)
				break;

			matrix4x3_transform_point(
				&placement_matrix,
				&rdata_0025ced8[adjustment_index],
				&adjusted_position);
			result = biped_fix_position(
				player_unit_index,
				source_unit_index,
				&adjusted_position,
				NULL,
				2.f,
				FALSE,
				FALSE,
				TRUE);

			if (!result)
			{
				random_adjustment_index = 0;
				do
				{
					if (result)
						break;

					random_adjustment_vector = *global_zero_vector3d;
					random_direction3d(&random_adjustment_vector);
					random_adjusted_position.x = adjusted_position.x +
						random_adjustment_vector.i *
						collision_height;
					random_adjusted_position.y = adjusted_position.y +
						random_adjustment_vector.j *
						collision_height;
					random_adjusted_position.z = adjusted_position.z +
						random_adjustment_vector.k *
						collision_height;
					result = biped_fix_position(
						player_unit_index,
						source_unit_index,
						&random_adjusted_position,
						NULL,
						2.f,
						FALSE,
						FALSE,
						TRUE);
					random_adjustment_index++;
				}
				while (random_adjustment_index < 8);
			}

			adjustment_index++;
		}
		while (adjustment_index < NUMBEROF(rdata_0025ced8));
			goto placement_complete;
		}
		else
		{
			result = biped_fix_position(
				player_unit_index,
				source_unit_index,
				(real_point3d *)position,
				NULL,
				2.f,
				FALSE,
				FALSE,
				TRUE);
		}
	}
	else
	{
		result = biped_fix_position(
			player_unit_index,
			source_unit_index,
			(real_point3d *)position,
			NULL,
			2.f,
			FALSE,
			FALSE,
			TRUE);
	}

placement_complete:

	(*(struct player_datum *volatile *)&player)->cluster_index = NONE;
	if (!result)
		goto failure;

	scenario = global_scenario_get();
	match_assert(
		"c:\\halo\\SOURCE\\game\\players.c",
		0x56A,
		player->unit_index!=NONE);
	for (adjustment_index = 0;
		adjustment_index < scenario->bsp_switch_trigger_volumes.count;
		adjustment_index++)
	{
		bsp_switch_trigger_volume = TAG_BLOCK_GET_ELEMENT(
			&scenario->bsp_switch_trigger_volumes,
			adjustment_index,
			struct scenario_bsp_switch_trigger_volume);
		if (bsp_switch_trigger_volume->source_structure_bsp_index ==
			global_structure_bsp_index &&
			scenario_trigger_volume_test_object(
				bsp_switch_trigger_volume->trigger_volume_index,
				player->unit_index))
		{
			result = FALSE;
			goto failure;
		}
	}

	pointers.biped->object.translational_velocity = *global_zero_vector3d;
	if (source_unit_index != NONE)
	{
		source_biped = biped_get(source_unit_index);
		best_adjustment_vector = source_biped->object.forward;
		source_biped = biped_try_and_get(source_unit_index);
		if (source_biped && source_biped->biped.elevator_object_index != NONE)
		{
			pointers.biped->biped.elevator_object_index =
				source_biped->biped.elevator_object_index;
			pointers.biped->biped.elevator_ticks = source_biped->biped.elevator_ticks;
		}

		pointers.biped->unit.desired_facing_vector = best_adjustment_vector;
		pointers.biped->unit.desired_aiming_vector = best_adjustment_vector;
		pointers.biped->unit.desired_looking_vector = best_adjustment_vector;
		if (player->local_player_index != NONE)
		{
			player_control_set_facing(
				player->local_player_index,
				&best_adjustment_vector);
		}

		player_information = TAG_BLOCK_GET_ELEMENT(
			&scenario_get_game_globals()->player_information,
			0,
			struct game_globals_player_information);
		respawn_effect_index = player_information->coop_respawn_effect.index;
		if (respawn_effect_index != NONE)
		{
			code_000a9ff0(players_globals->combined_pvs, FALSE);
			effect_new_from_object(
				respawn_effect_index,
				player_unit_index,
				player_unit_index,
				NONE,
				0.f,
				0.f,
				NULL,
				NULL);
		}
	}

	return result;

failure:
	error(2, "couldn't teleport player into a valid location");
	match_assert(
		"c:\\halo\\SOURCE\\game\\players.c",
		0x5AB,
		player->local_player_index!=NONE);
	code_000a9c00(player_index, source_unit_index);

	return result;
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

void players_debug_render(
	void)
{
	struct collision_result collision;
	real_point3d pill_position;
	real_vector3d pill_vector;
	real initial_radius;
	real_point3d fixed_position;
	short local_player_count;
	real pill_height;
	real pill_width;
	short local_player_index;
	long player_index;
	long biped_index;
	struct player_datum *player;
	struct unit_datum *unit;
	struct biped_definition *biped_definition;

	if (debug_render_player_teleport)
	{
		for (local_player_count = 0, local_player_index = local_player_get_next(NONE);
			local_player_count < 2 && local_player_index != NONE;
			local_player_count++, local_player_index = local_player_get_next(local_player_index))
		{
			match_assert("c:\\halo\\SOURCE\\game\\players.c", 0x3AF,
				local_player_index>=NONE && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

			if (players_globals->local_players[local_player_index] != NONE)
			{
				player_index = local_player_get_player_index(local_player_index);
				player = player_get(player_index);
				biped_index = player->unit_index;
				if (biped_index != NONE)
				{
					unit = unit_get(biped_index);
					biped_get_physics_pill(
						biped_index,
						&fixed_position,
						&initial_radius,
						&initial_radius);
					if (biped_fix_position(
						NONE,
						biped_index,
						&fixed_position,
						&fixed_position,
						2.0f,
						FALSE,
						TRUE,
						TRUE))
					{
						biped_definition = biped_definition_get(unit->definition_index);
						biped_get_physics_pill(
							biped_index,
							&pill_position,
							&pill_height,
							&pill_width);
						fixed_position.z +=
							biped_definition->biped.collision_height_standing;
						scale_vector3d(global_up3d, pill_height, &pill_vector);
						if (collision_test_pill(
							_collision_test_for_player_teleport_flags,
							&fixed_position,
							&pill_vector,
							pill_width,
							biped_index,
							&collision))
						{
							render_debug_pill(
								FALSE,
								&fixed_position,
								&pill_vector,
								pill_width,
								global_real_argb_red);
						}
						else
						{
							render_debug_pill(
								FALSE,
								&fixed_position,
								&pill_vector,
								pill_width,
								global_real_argb_green);
						}
					}
				}
			}
		}
	}

	return;
}

static void code_000ab820(
	long player_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	short *powerup_duration;
	long powerup_index;
	long remaining_powerups;

	player = player_get(player_index);
	_ReadWriteBarrier();
	powerup_index = 0;
	powerup_duration = player->powerup_durations;
	remaining_powerups = NUMBER_OF_PLAYER_POWERUPS;
	do
	{
		if (*powerup_duration > 0)
		{
			(*powerup_duration)--;
			if (*powerup_duration == 0)
			{
				player = player_get(player_index);
				unit = unit_get(player->unit_index);
				if (powerup_index == _player_powerup_active_camouflage)
				{
					SET_FLAG(
						unit->unit.flags,
						_unit_active_camouflaged_bit,
						FALSE);
				}
			}
		}

		powerup_index++;
		powerup_duration++;
	}
	while (--remaining_powerups);

	return;
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

	/* These dead calls intentionally keep VC7's private-ABI helper bodies
	   emitted; their real call sites are manually inlined for exact codegen. */
	if (0)
	{
		code_000aa220(0);
		code_000aa240(0);
		code_000aa4f0(0, 0);
		code_000aa530(0, 0);
		code_000aa560(0, 0);
		code_000ab440(0);
		code_000ab820(0);
		code_000ac0b0(0, 0);
		code_000acb50(0, 0);
		code_000ace70(0);
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

/* Exact-emission foundation: the players_update_after_game call is inlined,
   while VC7 also emits this private helper with January's register ABI. */
static void code_000aa220(
	long player_index)
{
	struct player_datum *player;

	player = player_get(player_index);
	player->action_result = 0;
	player->action_object_index = NONE;

	return;
}

static boolean code_000aa240(
	long player_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct weapon_datum *weapon;
	boolean result;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	result = FALSE;
	switch (player->action_result)
	{
	case 6:
		if (unit_drop_current_weapon(player->unit_index, TRUE) &&
			unit_add_weapon_to_inventory(
				player->unit_index,
				player->action_object_index,
				TRUE))
		{
			weapon = weapon_get(player->action_object_index);
			hud_picked_up_weapon(
				player->local_player_index,
				weapon->definition_index);
			player_control_unzoom(player->unit_index);
		}
		result = TRUE;
		break;

	case 7:
		if (unit_add_weapon_to_inventory(
			player->unit_index,
			player->action_object_index,
			TRUE))
		{
			weapon = weapon_get(player->action_object_index);
			hud_picked_up_weapon(
				player->local_player_index,
				weapon->definition_index);
		}
		break;
	}

	return result;
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

void player_handle_powerup_minor(
	long player_index,
	short powerup_type,
	short duration)
{
	struct player_datum *player;
	struct unit_datum *unit;
	long powerup_index;

	player = player_get(player_index);
	match_assert("c:\\halo\\SOURCE\\game\\players.c", 0xB19,
		powerup_type>=0 && powerup_type<NUMBER_OF_PLAYER_POWERUPS);

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

	player->powerup_durations[powerup_index] =
		MAX(player->powerup_durations[powerup_index], duration);

	return;
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

/* VC7 inlines this helper into player_handle_powerup_minor while retaining
   the January-exact private body and register ABI. */
static void code_000aa4f0(
	long player_index,
	short powerup_type)
{
	struct player_datum *player;
	struct unit_datum *unit;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	if (powerup_type == 0)
	{
		SET_FLAG(unit->unit.flags, _unit_active_camouflaged_bit, TRUE);
		unit->unit.cause_for_camo_regrowth = powerup_type;
	}

	return;
}

/* Exact-emission foundations: these private helpers match January when their
   powerup-update callers are present. Unreferenced helpers are currently
   elided by VC7. */
static void code_000aa530(
	long player_index,
	short powerup_type)
{
	struct player_datum *player;
	struct unit_datum *unit;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	if (powerup_type == 0)
		SET_FLAG(unit->unit.flags, _unit_super_camouflaged_bit, TRUE);

	return;
}

static void code_000aa560(
	long player_index,
	long powerup_type)
{
	struct player_datum *player;
	struct unit_datum *unit;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	if ((short)powerup_type == 0)
		SET_FLAG(unit->unit.flags, _unit_active_camouflaged_bit, FALSE);

	return;
}

static __inline real player_action_distance3d(
	real_point3d const *a,
	real_point3d const *b)
{
	real_vector3d v;

	vector_from_points3d(a, b, &v);
	return square_root(v.j*v.j + v.i*v.i + v.k*v.k);
}

static __inline real player_action_new_distance3d(
	real_point3d const *a,
	real_point3d const *b)
{
	real_vector3d v;
	real magnitude_squared;

	vector_from_points3d(a, b, &v);
	magnitude_squared = v.k*v.k;
	magnitude_squared += v.j*v.j;
	magnitude_squared += v.i*v.i;
	return square_root(magnitude_squared);
}

static __declspec(noinline) void code_000ab350(
	long player_index,
	short action_result,
	long object_index,
	short seat_index)
{
	struct player_datum *player;
	real_point3d const *unit_position;
	real_point3d const *current_position;
	real_point3d const *new_position;
	real current_distance;
	real new_distance;
	boolean set_action;

	player = player_get(player_index);
	set_action = TRUE;
	if (action_result != 11)
	{
		if (action_result == player->action_result)
		{
			unit_position = &object_get(player->unit_index)->object.position;
			current_position =
				&object_get(player->action_object_index)->object.position;
			new_position = &object_get(object_index)->object.position;
			current_distance = player_action_distance3d(
				unit_position,
				current_position);
			new_distance = player_action_new_distance3d(
				unit_position,
				new_position);
			if (!(current_distance > new_distance))
				set_action = FALSE;
		}
		else if (action_result <= player->action_result)
			set_action = FALSE;
	}

	if (set_action)
	{
		player->action_result = action_result;
		player->action_object_index = object_index;
		player->action_seat_index = seat_index;
	}

	return;
}

static void code_000ac0b0(
	long player_index,
	long vehicle_index)
{
	struct player_datum *player;
	struct vehicle_datum *vehicle;
	struct vehicle_datum *test_vehicle;
	struct unit_datum *unit;
	struct player_vehicle_difficulty_information *difficulty_information;
	real vehicle_ram_angle;
	short seat_state;

	player = player_get(player_index);
	vehicle = (struct vehicle_datum *)vehicle_get(vehicle_index);
	if (!TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit))
	{
		difficulty_information = TAG_BLOCK_GET_ELEMENT(
			(struct tag_block *)((byte *)scenario_get_game_globals() + 0x110),
			0,
			struct player_vehicle_difficulty_information);
		vehicle_ram_angle =
			(_pi * 0.5f) - difficulty_information->vehicle_ram_angle;
		test_vehicle = (struct vehicle_datum *)vehicle_get(vehicle_index);
		if (*(real *)((byte *)test_vehicle + 0x38) >
			cosine(vehicle_ram_angle))
		{
			if (!unit_overcharged(player->unit_index))
			{
				unit = unit_get(player->unit_index);
				if (magnitude_squared3d(&unit->object.translational_velocity) < 0.01f)
				{
					if (magnitude_squared3d(
						&((struct vehicle_datum *)vehicle_get(
							vehicle_index))->object.angular_velocity) <
						0.01f)
					{
						short seat_index;

						seat_index = NONE;
						seat_state = unit_find_nearby_seat(
							player->unit_index,
							vehicle_index,
							&seat_index);
						switch (seat_state)
						{
						case 2:
							if (seat_index == NONE)
							{
								display_assert(
									"seat_index != NONE",
									"c:\\halo\\SOURCE\\game\\players.c",
									0x83C,
									TRUE);
								system_exit(-1);
							}
							code_000ab350(
								player_index,
								8,
								vehicle_index,
								seat_index);
							break;

						case 1:
							if (seat_index == NONE)
							{
								display_assert(
									"seat_index != NONE",
									"c:\\halo\\SOURCE\\game\\players.c",
									0x841,
									TRUE);
								system_exit(-1);
							}
							code_000ab350(
								player_index,
								9,
								vehicle_index,
								seat_index);
							break;
						}
					}
				}
			}
		}
		else if (!TEST_FLAG(vehicle->vehicle.flags, 4) &&
			vehicle->unit.driver_object_index == NONE)
		{
			code_000ab350(player_index, 11, vehicle_index, NONE);
		}
	}

	return;
}

static void code_000acb50(
	long player_index,
	long item_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct item_datum *item;
	struct item_datum *equipment;
	struct equipment_definition *equipment_definition;
	struct equipment_definition *current_equipment_definition;
	long current_equipment_index;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	item = item_get(item_index);
	if (item->object.parent_object_index != NONE ||
		item->item.ignore_object_index == player->unit_index)
	{
		return;
	}

	{
		short inventory_index;
		short ammunition_count;
		long inventory_item_index;

		for (inventory_index = 0;
			inventory_index < MAXIMUM_WEAPONS_PER_UNIT;
			inventory_index++)
		{
			inventory_item_index =
				unit->unit.weapon_object_indices[inventory_index];
			if (inventory_item_index != NONE &&
				weapon_handle_potential_inventory_item(
					inventory_item_index,
					item_index,
					player->local_player_index,
					&ammunition_count))
			{
				if (ammunition_count > 0)
				{
					hud_picked_up_ammunition(
						player->local_player_index,
						weapon_get(inventory_item_index)->definition_index,
						ammunition_count);
				}
				break;
			}
		}
	}

	equipment = equipment_try_and_get(item_index);
	if (equipment)
	{
		equipment_definition = equipment_definition_get(equipment->definition_index);
		if (equipment_definition->equipment.powerup_type == 6)
		{
			if (unit_add_grenade_to_inventory(player->unit_index, item_index))
			{
				hud_picked_up_grenade(
					player->local_player_index,
					equipment->definition_index);
			}
		}
		else if (equipment_definition->equipment.powerup_type != 0)
		{
			current_equipment_index = unit_get_current_equipment(player->unit_index);
			if (current_equipment_index == NONE)
			{
				code_000ac320(player_index, item_index);
			}
			else
			{
				/* Original bug: the current equipment is verified, but its
				 * definition is not used. To fix this, pass
				 * equipment_get(current_equipment_index)->definition_index to
				 * equipment_definition_get below. */
				equipment_get(current_equipment_index);
				current_equipment_definition =
					equipment_definition_get(equipment->definition_index);
				if (equipment_definition->equipment.powerup_type !=
					current_equipment_definition->equipment.powerup_type)
				{
					code_000ab350(player_index, 5, item_index, NONE);
				}
			}
		}
	}

	{
		struct weapon_datum *weapon;
		struct weapon_datum *current_weapon;
		struct weapon_definition *weapon_definition;
		struct weapon_definition *current_weapon_definition;
		long current_weapon_index;
		long weapon_count;
		long weapon_item_index;
		boolean weapon_trigger_active;
		boolean current_weapon_does_not_count;

		weapon_item_index = item_index;
		weapon = weapon_try_and_get(weapon_item_index);
		if (!weapon || !unit_can_use_weapon(player->unit_index, weapon_item_index))
		{
			return;
		}

		weapon_definition = weapon_definition_get(weapon->definition_index);
		weapon_trigger_active =
			TEST_FLAG(unit->unit.control_flags, _unit_control_weapon_primary_trigger_bit) ||
			TEST_FLAG(unit->unit.control_flags, _unit_control_weapon_secondary_trigger_bit);
		unit = unit_get(player->unit_index);
		current_weapon_index = unit_inventory_get_weapon(
			player->unit_index,
			unit->unit.current_weapon_index);
		weapon_count = unit_get_weapon_count(player->unit_index);
		current_weapon_does_not_count = FALSE;
		if (weapon_count >= 2 &&
			current_weapon_index != NONE &&
			!TEST_FLAG(
				weapon_definition->weapon.flags,
				_weapon_doesnt_count_toward_maximum_bit))
		{
			current_weapon = weapon_get(current_weapon_index);
			current_weapon_definition =
				weapon_definition_get(current_weapon->definition_index);
			if (TEST_FLAG(
				current_weapon_definition->weapon.flags,
				_weapon_doesnt_count_toward_maximum_bit))
			{
				current_weapon_does_not_count = TRUE;
			}
		}

		if (weapon_trigger_active &&
			TEST_FLAG(weapon_definition->weapon.flags, _weapon_must_be_readied_bit))
		{
			return;
		}

		if (unit_should_autopick_weapon(player->unit_index, weapon_item_index))
		{
			if (unit_add_weapon_to_inventory(
				player->unit_index,
				weapon_item_index,
				TRUE))
			{
				hud_picked_up_weapon(
					player->local_player_index,
					weapon_get(weapon_item_index)->definition_index);
				player_control_unzoom(player->unit_index);
			}
		}
		else if (!current_weapon_does_not_count &&
			unit_approve_weapon_swap(player->unit_index, weapon_item_index))
		{
			current_weapon = weapon_try_and_get(current_weapon_index);
			if (*(volatile long *)&weapon_count == 1 &&
				current_weapon &&
				current_weapon->definition_index != weapon->definition_index)
			{
				code_000ab350(player_index, 7, weapon_item_index, NONE);
			}
			else
			{
				code_000ab350(player_index, 6, weapon_item_index, NONE);
			}
		}
	}

	return;
}

void code_000ac270(
	long player_index,
	long device_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct device_datum *device;
	real_point3d camera_position;

	player = player_get(player_index);
	unit = unit_get(player->unit_index);
	device = device_get(device_index);
	unit_get_camera_position(player->unit_index, &camera_position);
	if (fast_vector_intersects_sphere(
		&camera_position,
		&unit->unit.aiming_vector,
		&device->object.bounding_sphere_center,
		*(volatile real *)&device->object.bounding_sphere_radius) &&
		device_frontfacing(device_index, &camera_position, &unit->unit.aiming_vector) &&
		device_can_change_position(device_index))
	{
		code_000ab350(player_index, 10, device_index, NONE);
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

static void code_000ace70(
	long player_index)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct object_datum *object;
	long object_indices[16];
	short object_count;
	short object_number;

	player = player_get(player_index);
	if (player->unit_index != NONE)
	{
		unit = unit_get(player->unit_index);
		if (unit->object.parent_object_index == NONE)
		{
			object_count = objects_in_sphere(
				0,
				_object_mask_player_interaction,
				&unit->object.location,
				&unit->object.bounding_sphere_center,
				unit->object.bounding_sphere_radius,
				object_indices,
				NUMBEROF(object_indices));
			for (object_number = 0; object_number < object_count; object_number++)
			{
				object = object_get(object_indices[object_number]);
				switch (object->object.type)
				{
				case _object_type_biped:
					continue;

				case _object_type_vehicle:
					code_000ac0b0(player_index, object_indices[object_number]);
					break;

				case _object_type_weapon:
				case _object_type_equipment:
					code_000acb50(player_index, object_indices[object_number]);
					break;

				case _object_type_control:
					code_000ac270(player_index, object_indices[object_number]);
					break;
				}
			}
		}
	}

	return;
}

static long code_000a9f80(
	struct scenario_starting_profile_weapon const *starting_weapon,
	long unit_index)
{
	struct object_placement_data placement_data;
	struct weapon_datum *weapon;
	long weapon_index;

	weapon_index = NONE;
	if (starting_weapon->weapon.index != NONE)
	{
		object_placement_data_new(
			&placement_data,
			starting_weapon->weapon.index,
			unit_index);
		weapon_index = object_new(&placement_data);
		if (weapon_index != NONE)
		{
			weapon = weapon_get(weapon_index);
			weapon->weapon.magazines[0].rounds_total =
				starting_weapon->rounds_total;
			weapon->weapon.magazines[0].rounds_loaded =
				starting_weapon->rounds_loaded;
		}
	}

	return weapon_index;
}

void player_add_equipment(
	long unit_index,
	short starting_profile_index,
	boolean reset_equipment)
{
	struct unit_datum *unit;
	struct scenario_starting_profile *starting_profile;
	struct scenario *scenario;
	long weapon_index;
	short grenade_index;

	if (unit_index != NONE && starting_profile_index != NONE)
	{
		unit = unit_try_and_get(unit_index);
		if (unit->unit.player_index != NONE)
		{
			scenario = global_scenario_get();
			starting_profile = TAG_BLOCK_GET_ELEMENT(
				&scenario->starting_profiles,
				starting_profile_index,
				struct scenario_starting_profile);

			if (reset_equipment)
			{
				unit_delete_all_weapons(unit_index);
				unit->object.shield_vitality = 0.f;
				unit->object.body_vitality = 0.f;
				*(short *)unit->unit.grenade_counts = 0;
			}

			if (starting_profile->primary_weapon.weapon.index != NONE)
			{
				weapon_index = code_000a9f80(
					&starting_profile->primary_weapon,
					unit_index);
				if (weapon_index != NONE &&
					!unit_add_weapon_to_inventory(
						unit_index,
						weapon_index,
						reset_equipment != FALSE))
				{
					error(2, "Could not attach starting weapon to player");
					object_delete(weapon_index);
				}
			}

			if (starting_profile->secondary_weapon.weapon.index != NONE)
			{
				weapon_index = code_000a9f80(
					&starting_profile->secondary_weapon,
					unit_index);
				if (weapon_index != NONE &&
					!unit_add_weapon_to_inventory(
						unit_index,
						weapon_index,
						FALSE))
				{
					error(2, "Could not attach starting weapon to player");
					object_delete(weapon_index);
				}
			}

			unit->object.shield_vitality +=
				starting_profile->starting_shield_modifier;
			unit->object.body_vitality +=
				starting_profile->starting_health_modifier;
			unit = (struct unit_datum *)unit->unit.grenade_counts;
			starting_profile =
				(struct scenario_starting_profile *)starting_profile->grenade_counts;
			grenade_index = NUMBER_OF_UNIT_GRENADE_TYPES;
			do
			{
				*(byte *)unit += *(byte *)starting_profile;
				starting_profile = (struct scenario_starting_profile *)(
					(byte *)starting_profile + 1);
				unit = (struct unit_datum *)((byte *)unit + 1);
			}
			while (--grenade_index);
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
