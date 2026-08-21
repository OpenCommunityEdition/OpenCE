/*
HUD.C

symbols in this file:
000BF660 0030:
	_code_000bf660 (0000)
000BF690 0050:
	_hud_initialize (0000)
000BF6E0 0020:
	_hud_dispose (0000)
000BF700 0080:
	_hud_initialize_for_new_map (0000)
000BF780 0020:
	_hud_dispose_from_old_map (0000)
000BF7A0 0020:
	_scripted_show_hud (0000)
000BF7C0 0020:
	_scripted_show_hud_help_text (0000)
000BF7E0 0060:
	_hud_update (0000)
000BF840 0030:
	_code_000bf840 (0000)
000BF870 06d0:
	_code_000bf870 (0000)
000BFF40 0050:
	_hud_get_item_string (0000)
000BFF90 0020:
	_hud_picked_up_grenade (0000)
000BFFB0 0030:
	_hud_picked_up_ammunition (0000)
000BFFE0 0020:
	_hud_picked_up_weapon (0000)
000C0000 0020:
	_hud_picked_up_powerup (0000)
000C0020 00d0:
	_code_000c0020 (0000)
000C00F0 0060:
	_hud_load (0000)
000C0150 00e0:
	_hud_autosave (0000)
000C0230 0160:
	_code_000c0230 (0000)
000C0390 00a0:
	_code_000c0390 (0000)
000C0430 0370:
	_code_000c0430 (0000)
000C07A0 0140:
	_hud_draw_screen (0000)
0026FC1C 0015:
	??_C@_0BF@NENKDAEK@hud_scripted_globals?$AA@ (0000)
0026FC34 001f:
	??_C@_0BP@ILJLOMEO@c?3?2halo?2SOURCE?2interface?2hud?4c?$AA@ (0000)
0026FC54 0015:
	??_C@_0BF@MENHAMGO@hud?5scripted?5globals?$AA@ (0000)
0026FC6C 0036:
	??_C@_0DG@OKALAJAB@interface_get_tag_index?$CI_interfa@ (0000)
0026FCA4 0013:
	??_C@_0BD@EAHBLILM@ACTIVE?9CAMOUFLAGE?5?$AA@ (0000)
0026FCB8 0016:
	??_C@_0BG@GFCGCBJE@FULL?9SPECTRUM?5VISION?5?$AA@ (0000)
0026FCD0 0005:
	??_C@_04OPODHAPJ@?$CFs?$HMn?$AA@ (0000)
0026FCD8 0037:
	??_C@_0DH@GKJDMEOF@?$CFs?$HMntotal?5?$CFd?1?$CFd?$HMnloaded?5?$CFd?1?$CFd?$HMnh@ (0000)
0026FD10 0008:
	??_C@_07IFCAFJHP@?$CFd?5?$CFs?$HMn?$AA@ (0000)
0026FD18 001a:
	??_C@_0BK@NJIPBDGF@?$CFs?5?$CIpress?5WHITE?5to?5use?$CJ?$HMn?$AA@ (0000)
002E4C34 0004:
	_data_002e4c34 (0000)
00453AAC 0008:
	_hud_globals (0000)
	_hud_scripted_globals (0004)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "interface/hud_messaging.h"
#include "interface/hud_unit.h"
#include "interface/hud_weapon.h"
#include "interface/motion_sensor.h"
#include "memory/data.h"
#include "saved games/game_state.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct hud_scripted_globals
{
	boolean show_hud;
	boolean show_hud_help_text;
	byte reserved2[2];
};

typedef char hud_scripted_globals_size_assert[
	sizeof(struct hud_scripted_globals) == 0x4 ? 1 : -1];
typedef char hud_scripted_globals_show_hud_help_text_offset_assert[
	offsetof(struct hud_scripted_globals, show_hud_help_text) == 0x1 ? 1 : -1];

/* ---------- prototypes */

void hud_dispose_nav_points_from_old_map(
	void);
void hud_dispose_nav_points(
	void);
void hud_messaging_initialize(
	void);
void hud_initialize_unit_interface(
	void);
void hud_initialize_weapon_interface(
	void);
void hud_initialize_nav_points(
	void);
void hud_update_weapon(
	void);
void hud_update_unit(
	void);
void hud_update_nav_points(
	void);
void hud_messaging_globals_update(
	void);
void hud_play_unit_sounds(
	struct player_datum const *player,
	boolean show_hud);

/* ---------- globals */

extern struct hud_scripted_globals *hud_scripted_globals;

/* ---------- public code */

void hud_initialize(
	void)
{
	hud_scripted_globals = game_state_malloc(
		"hud scripted globals",
		NULL,
		sizeof(*hud_scripted_globals));
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud.c",
		87,
		hud_scripted_globals);
	hud_messaging_initialize();
	hud_initialize_unit_interface();
	hud_initialize_weapon_interface();
	hud_initialize_nav_points();
	motion_sensor_initialize();
	return;
}

void hud_dispose(
	void)
{
	motion_sensor_dispose();
	hud_dispose_nav_points();
	hud_dispose_weapon_interface();
	hud_dispose_unit_interface();
	hud_messaging_dispose();
	return;
}

void hud_dispose_from_old_map(
	void)
{
	motion_sensor_dispose_from_old_map();
	hud_dispose_nav_points_from_old_map();
	hud_dispose_weapon_interface_from_old_map();
	hud_dispose_unit_interface_from_old_map();
	hud_messaging_dispose_from_old_map();
	return;
}

boolean scripted_show_hud(
	boolean show)
{
	hud_scripted_globals->show_hud = show;
	return hud_scripted_globals->show_hud;
}

boolean scripted_show_hud_help_text(
	boolean show)
{
	hud_scripted_globals->show_hud_help_text = show;
	return hud_scripted_globals->show_hud_help_text;
}

void hud_update(
	void)
{
	short local_player_index;
	long player_index;

	hud_update_weapon();
	hud_update_unit();
	hud_update_nav_points();
	hud_messaging_globals_update();
	if (game_engine_force_single_screen())
	{
		for (local_player_index = 0;
			local_player_index < MAXIMUM_LOCAL_PLAYERS;
			local_player_index++)
		{
			player_index = local_player_get_player_index(local_player_index);
			if (player_index != NONE)
			{
				hud_play_unit_sounds(
					datum_get(player_data, player_index),
					FALSE);
			}
		}
	}
	return;
}

/* ---------- private code */
