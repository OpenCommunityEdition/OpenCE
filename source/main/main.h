/*
MAIN.H
*/

#ifndef __MAIN_H
#define __MAIN_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/MAIN.C */

float main_get_seconds_elapsed(
	void);

void game_connection_set(
	short connection);

void main_disallow_persistent_storage(
	void);

void main_defer_map_map_change(
	void);

char *main_get_map_name(
	void);

char *main_get_multiplayer_map_name(
	void);

short main_get_difficulty(
	void);

boolean main_saving_map(
	void);

void main_save_cancel(
	void);

void main_won_map(
	void);

void main_lost_map(
	void);

void main_revert_map(
	void);

void main_skip_cinematic(
	void);

void main_save_map_nonsafe(
	void);

void main_respawn(
	boolean in_multiplayer);

void main_save_core(
	void);

void main_load_core(
	void);

void main_load_core_at_startup(
	void);

void main_goto_main_menu(
	void);

void main_menu_unload(
	void);

void main_menu_ensure_player_queues_exist(
	void);

boolean main_menu_fade_active(
	void);

void main_menu_switch_to_single_player(
	void);

void main_set_game_connection_to_film_playback(
	void);

short main_get_current_solo_level(
	void);

void main_run_demos(
	void);

void main_reset_map(void);

void main_stop_time(void);
void main_start_time(void);

void main_vertical_blank_interrupt_handler(unsigned long);

void main_loop(void);

/* ---------- globals */

extern short player_spawn_count;

/* ---------- public code */

#endif // __MAIN_H
