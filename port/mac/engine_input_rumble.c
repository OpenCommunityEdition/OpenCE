/* Engine rumble requests share the native input backend's explicit no-device
   result. Carbon keyboard/mouse input has no gamepad haptics to report. */
#include "cseries.h"
#include "input/input.h"
#include "interface/player_ui.h"
#define MAC_INPUT_NO_CARBON
#include "mac_input.h"

void input_set_gamepad_rumbler_state(
    short gamepad_index,
    word left_speed,
    word right_speed)
{
    match_assert("port/mac/engine_input_rumble.c", 0, gamepad_index>=0 && gamepad_index<MAXIMUM_GAMEPADS);
    if(!player_ui_rumble_disabled(gamepad_index))
        (void)mac_input_rumble((unsigned int)gamepad_index,left_speed,right_speed);
    return;
}
