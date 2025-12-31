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

void main_reset_map(void);

void main_stop_time(void);
void main_start_time(void);

void main_vertical_blank_interrupt_handler(unsigned long);

void main_loop(void);

/* ---------- globals */

extern short player_spawn_count;

/* ---------- public code */

#endif // __MAIN_H
