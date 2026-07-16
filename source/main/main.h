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

real main_get_seconds_elapsed(
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

void main_reset_map(void);

void main_stop_time(void);
void main_start_time(void);

void main_vertical_blank_interrupt_handler(unsigned long);

void main_loop(void);

/* ---------- globals */

extern short player_spawn_count;

/* ---------- public code */

#endif // __MAIN_H
