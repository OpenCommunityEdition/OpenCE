#ifndef __PLAYER_CONTROL_H
#define __PLAYER_CONTROL_H
#pragma once

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

struct player_control_globals_header
{
	unsigned long action_flags;
	unsigned long action_test_flags;
	unsigned long suppressed_action_flags;
	unsigned long flags;
};

typedef char player_control_globals_header_size_assert[
	sizeof(struct player_control_globals_header) == 0x10 ? 1 : -1];
typedef char player_control_globals_action_test_flags_offset_assert[
	offsetof(struct player_control_globals_header, action_test_flags) == 0x4 ? 1 : -1];
typedef char player_control_globals_suppressed_action_flags_offset_assert[
	offsetof(struct player_control_globals_header, suppressed_action_flags) == 0x8 ? 1 : -1];
typedef char player_control_globals_flags_offset_assert[
	offsetof(struct player_control_globals_header, flags) == 0xC ? 1 : -1];

/* ---------- prototypes */

void player_control_dispose(
	void);
void player_control_dispose_from_old_map(
	void);
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

extern struct player_control_globals_header *bss_0043ee30;

#define player_control_globals bss_0043ee30

#endif // __PLAYER_CONTROL_H
