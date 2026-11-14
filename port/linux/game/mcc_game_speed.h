#ifndef MCC_GAME_SPEED_H
#define MCC_GAME_SPEED_H
#include "cseries.h"

void mcc_game_speed_reset(void);
boolean mcc_game_speed_global(word designator);
void mcc_game_speed_set(real value);
real mcc_game_speed_client(real original);
unsigned long mcc_game_speed_clock(unsigned long now);
void mcc_game_speed_frame(void);
void mcc_game_speed_receive(void const *entries, short count);
/* MCC's game_paused: the scenario global its scripts and console set (NONE
 * without it), bound at the map's load (mcc_scripts.c). */
void mcc_game_paused_bind(short scenario_global);
#endif
