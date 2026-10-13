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
#endif
