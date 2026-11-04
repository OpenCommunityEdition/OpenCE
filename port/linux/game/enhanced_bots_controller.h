#ifndef ENHANCED_BOTS_CONTROLLER_H
#define ENHANCED_BOTS_CONTROLLER_H
struct player_action;
void enhanced_controller_initialize(void);
void enhanced_controller_dispose(void);
void enhanced_controller_update(struct player_action *actions);
#endif
