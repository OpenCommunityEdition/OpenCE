/* Engine rumble requests go to the pad at the port (port/mac/engine_input.c): a DualShock 4 or DualSense on USB
   rumbles, any other pad or none answers no-device. */
#include "cseries.h"
#include "input/input.h"
#include "interface/player_ui.h"
#include "engine_input.h"

void input_set_gamepad_rumbler_state(
    short gamepad_index,
    word left_speed,
    word right_speed)
{
    match_assert("port/mac/engine_input_rumble.c", 0, gamepad_index>=0 && gamepad_index<MAXIMUM_GAMEPADS);
    if(!player_ui_rumble_disabled(gamepad_index))
        (void)mac_engine_input_rumble((unsigned)gamepad_index,left_speed,right_speed);
    return;
}
