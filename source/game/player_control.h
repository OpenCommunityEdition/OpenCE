#ifndef __PLAYER_CONTROL_H
#define __PLAYER_CONTROL_H
#pragma once

#include "networking/network_connection.h"

/* ---------- constants */

enum
{
	_player_control_action_bit,
	_player_control_jump_bit,
	_player_control_accept_bit,
	_player_control_back_bit,
	_player_control_primary_trigger_bit,
	_player_control_grenade_trigger_bit,
	_player_control_zoom_bit,
	_player_control_look_relative_up_bit,
	_player_control_look_relative_down_bit,
	_player_control_look_relative_left_bit,
	_player_control_look_relative_right_bit,
};

enum
{
	_player_control_camera_control_disabled_bit,
	_player_control_look_relative_all_directions_flags = 0x780,
	_player_control_move_relative_all_directions_flags = 0x7800,
};

/* ---------- structures */

struct player_control
{
	long unit_index;
	long unknown04;
	short inhibited_action_flags;
	short persistent_inhibited_action_flags;
	float desired_yaw;
	float desired_pitch;
	byte unknown14[0xC];
	short desired_weapon_index;
	short desired_grenade_index;
	short zoom_level;
	boolean unknown26;
	byte unknown27;
	long target_object_index;
	float autoaim_level;
	byte unknown30[0x10];
};

struct player_control_globals_data
{
	unsigned long action_flags;
	unsigned long action_test_flags;
	unsigned long suppressed_action_flags;
	unsigned long flags;
	struct player_control players[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
};

typedef char player_control_size_assert[
	sizeof(struct player_control) == 0x40 ? 1 : -1];
typedef char player_control_unit_index_offset_assert[
	offsetof(struct player_control, unit_index) == 0x0 ? 1 : -1];
typedef char player_control_inhibited_action_flags_offset_assert[
	offsetof(struct player_control, inhibited_action_flags) == 0x8 ? 1 : -1];
typedef char player_control_desired_yaw_offset_assert[
	offsetof(struct player_control, desired_yaw) == 0xC ? 1 : -1];
typedef char player_control_desired_pitch_offset_assert[
	offsetof(struct player_control, desired_pitch) == 0x10 ? 1 : -1];
typedef char player_control_desired_weapon_index_offset_assert[
	offsetof(struct player_control, desired_weapon_index) == 0x20 ? 1 : -1];
typedef char player_control_desired_grenade_index_offset_assert[
	offsetof(struct player_control, desired_grenade_index) == 0x22 ? 1 : -1];
typedef char player_control_zoom_level_offset_assert[
	offsetof(struct player_control, zoom_level) == 0x24 ? 1 : -1];
typedef char player_control_target_object_index_offset_assert[
	offsetof(struct player_control, target_object_index) == 0x28 ? 1 : -1];
typedef char player_control_autoaim_level_offset_assert[
	offsetof(struct player_control, autoaim_level) == 0x2C ? 1 : -1];
typedef char player_control_globals_size_assert[
	sizeof(struct player_control_globals_data) == 0x110 ? 1 : -1];
typedef char player_control_globals_players_offset_assert[
	offsetof(struct player_control_globals_data, players) == 0x10 ? 1 : -1];
typedef char player_control_globals_action_test_flags_offset_assert[
	offsetof(struct player_control_globals_data, action_test_flags) == 0x4 ? 1 : -1];
typedef char player_control_globals_suppressed_action_flags_offset_assert[
	offsetof(struct player_control_globals_data, suppressed_action_flags) == 0x8 ? 1 : -1];
typedef char player_control_globals_flags_offset_assert[
	offsetof(struct player_control_globals_data, flags) == 0xC ? 1 : -1];

/* ---------- prototypes */

void player_control_dispose(
	void);
void player_control_dispose_from_old_map(
	void);
struct player_control *player_control_get(
	short local_player_index);
void player_control_inhibit_buttons(
	short local_player_index,
	word action_flags,
	boolean persistent);
long player_control_get_target_object_index(
	short local_player_index);
long player_control_get_unit_index(
	short local_player_index);
short player_control_get_zoom_level(
	short local_player_index);
float player_control_get_autoaim_level(
	short local_player_index);
void players_unzoom_all(
	void);
void player_control_unzoom(
	long unit_index);
boolean scripted_player_control_set_camera_control(
	boolean camera_control);
void player_control_action_test_reset(
	void);
boolean player_control_action_test_accept(
	void);
boolean player_control_action_test_back(
	void);
boolean player_control_action_test_action(
	void);
boolean player_control_action_test_jump(
	void);
boolean player_control_action_test_primary_trigger(
	void);
boolean player_control_action_test_grenade_trigger(
	void);
boolean player_control_action_test_zoom(
	void);
boolean player_control_action_test_look_relative_left(
	void);
boolean player_control_action_test_look_relative_right(
	void);
boolean player_control_action_test_look_relative_up(
	void);
boolean player_control_action_test_look_relative_down(
	void);
boolean player_control_action_test_move_relative_all_directions(
	void);
boolean player_control_action_test_look_relative_all_directions(
	void);

/* ---------- globals */

extern struct player_control_globals_data *bss_0043ee30;

#define player_control_globals bss_0043ee30

#endif // __PLAYER_CONTROL_H
