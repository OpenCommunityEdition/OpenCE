/*
FIRST_PERSON_WEAPONS.C

symbols in this file:
000CBC60 0020:
	_animation_update_render_only (0000)
000CBC80 0040:
	_first_person_weapons_initialize (0000)
000CBCC0 0010:
	_first_person_weapons_dispose (0000)
000CBCD0 0050:
	_first_person_weapons_initialize_for_new_map (0000)
000CBD20 0010:
	_first_person_weapons_dispose_from_old_map (0000)
000CBD30 00c0:
	_code_000cbd30 (0000)
000CBDF0 0110:
	_code_000cbdf0 (0000)
000CBF00 0120:
	_code_000cbf00 (0000)
000CC020 0040:
	_code_000cc020 (0000)
000CC060 00a0:
	_code_000cc060 (0000)
000CC100 00b0:
	_code_000cc100 (0000)
000CC1B0 00e0:
	_code_000cc1b0 (0000)
000CC290 0060:
	_code_000cc290 (0000)
000CC2F0 0040:
	_code_000cc2f0 (0000)
000CC330 0080:
	_code_000cc330 (0000)
000CC3B0 0290:
	_first_person_weapon_draw (0000)
000CC640 0080:
	_first_person_weapon_get_local_index (0000)
000CC6C0 00d0:
	_first_person_weapon_get_marker_by_name (0000)
000CC790 00e0:
	_first_person_weapon_center_flashlight (0000)
000CC870 00d0:
	_first_person_weapon_adjust_light (0000)
000CC940 00c0:
	_first_person_weapon_get_node_matrix (0000)
000CCA00 00b0:
	_code_000cca00 (0000)
000CCAB0 0560:
	_code_000ccab0 (0000)
000CD010 00b0:
	_first_person_weapon_render_update (0000)
000CD0C0 0040:
	_first_person_weapon_get_marker_by_name_render (0000)
000CD100 02b0:
	_code_000cd100 (0000)
000CD3B0 0260:
	_code_000cd3b0 (0000)
000CD610 0060:
	_code_000cd610 (0000)
000CD670 0220:
	_code_000cd670 (0000)
000CD890 0050:
	_first_person_weapon_message_from_unit (0000)
000CD8E0 0040:
	_first_person_weapon_message_from_weapon (0000)
000CD920 0170:
	_code_000cd920 (0000)
000CDA90 0600:
	_code_000cda90 (0000)
000CE090 00f0:
	_first_person_weapons_update (0000)
0027076C 0015:
	??_C@_0BF@MEAEHHHF@first_person_weapons?$AA@ (0000)
00270784 0030:
	??_C@_0DA@LGOECHKB@c?3?2halo?2SOURCE?2interface?2first_p@ (0000)
002707B4 0015:
	??_C@_0BF@LKJCKPFC@first?5person?5weapons?$AA@ (0000)
002707CC 0028:
	??_C@_0CI@FFNKKBPO@first_person_weapon?9?$DOweapon_inde@ (0000)
002707F8 0059:
	??_C@_0FJ@DFINBDBK@animation_graph_node_index?$DO?$DN0?5?$CG?$CG@ (0000)
00270854 000b:
	??_C@_0L@HGCOMEEN@flashlight?$AA@ (0000)
00270860 0039:
	??_C@_0DJ@KDPHKICA@node_index?$DO?$DN0?5?$CG?$CG?5node_index?$DManim@ (0000)
0027089C 0035:
	??_C@_0DF@COECBIGL@local?5player?5?$CFd?0?5weapon?5?$CI0x?$CFx?$CJ?0?5@ (0000)
002708D8 00a5:
	??_C@_0KF@OCJNMLNK@first_person_weapon?9?$DOshotgun_rel@ (0000)
00270980 0004:
	__real@c1f00000 (0000)
00453CC8 0004:
	_bss_00453cc8 (0000)
*/

/* ---------- headers */

#include "interface/first_person_weapons.h"

#include "cseries/cseries.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "models/model_animation_definitions.h"
#include "networking/network_connection.h"
#include "saved games/game_state.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct first_person_weapon
{
	boolean visible;
	byte reserved0001[3];
	long unit_index;
	long weapon_index;
	byte reserved000c[0x1080];
	struct real_matrix4x3 node_matrices[MAXIMUM_NODES_PER_ANIMATION];
	byte reserved1d8c[0x10C];
	long current_sound_index;
	short current_sound_state;
	byte reserved1e9e[2];
};

typedef char verify_first_person_weapon_size[
	sizeof(struct first_person_weapon) == 0x1EA0 ? 1 : -1];
typedef char verify_first_person_weapon_node_matrices_offset[
	offsetof(struct first_person_weapon, node_matrices) == 0x108C ? 1 : -1];
typedef char verify_first_person_weapon_current_sound_index_offset[
	offsetof(struct first_person_weapon, current_sound_index) == 0x1E98 ? 1 : -1];
typedef char verify_first_person_weapon_current_sound_state_offset[
	offsetof(struct first_person_weapon, current_sound_state) == 0x1E9C ? 1 : -1];
typedef char verify_weapon_first_person_animations_index_offset[
	offsetof(
		struct weapon_definition,
		weapon.interface_definition.first_person_animations.index) == 0x478 ? 1 : -1];
typedef char verify_animation_graph_nodes_offset[
	offsetof(struct animation_graph, nodes) == 0x68 ? 1 : -1];

/* ---------- globals */

extern struct first_person_weapon *bss_00453cc8;

#define first_person_weapons bss_00453cc8

/* ---------- public code */

short animation_update_render_only(
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index)
{
	return animation_update_internal(
		0,
		animation_graph_index,
		state,
		sound_index);
}

void first_person_weapons_initialize(
	void)
{
	first_person_weapons = (struct first_person_weapon *)game_state_malloc(
		"first person weapons",
		NULL,
		sizeof(*first_person_weapons) * MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\first_person_weapons.c",
		240,
		first_person_weapons);

	return;
}

void first_person_weapons_dispose(
	void)
{
	return;
}

void first_person_weapons_dispose_from_old_map(
	void)
{
	return;
}

struct real_matrix4x3 *first_person_weapon_get_node_matrix(
	short local_player_index,
	short node_index)
{
	struct first_person_weapon *first_person_weapon;
	struct weapon_datum *weapon;
	struct weapon_definition *weapon_definition;
	struct animation_graph *animation_graph;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\first_person_weapons.c",
		1433,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
	first_person_weapon = &first_person_weapons[local_player_index];
	weapon = weapon_get(first_person_weapon->weapon_index);
	weapon_definition = weapon_definition_get(weapon->definition_index);
	animation_graph = animation_graph_definition_get(
		weapon_definition->weapon.interface_definition.first_person_animations.index);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\first_person_weapons.c",
		718,
		node_index>=0 && node_index<animation_graph->nodes.count);

	return &first_person_weapon->node_matrices[node_index];
}

/* ---------- private code */
