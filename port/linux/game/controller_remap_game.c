/* Existing profile mappings remain authoritative for stock and menu input.
 * Custom layouts translate physical buttons before keyboard/touch merging. */
#include "cseries.h"
#include "controller_remap.h"
#include "input/input_abstraction.h"
#include "game/game.h"
#include "game/players.h"
#include "interface/ui_widget.h"
unsigned char network_chat_keyboard_open(void);

void controller_remap_profile(short port, unsigned char *buttons)
{
    struct game_input_preferences preferences;
    static unsigned char const standard[CONTROLLER_REMAP_ACTIONS] = {0,4,2,3,1,5,6,7,14,15};
    unsigned int seen = 0;
    int i;
    input_abstraction_get_local_player_preferences(port, &preferences);
    for (i = 0; i < CONTROLLER_REMAP_ACTIONS; i++)
    {
        int button = preferences.game_control_to_xbox_buttons[controller_remap_controls[i]];
        if ((button > 7 && button != 10 && button != 14 && button != 15) || (seen & (1u << button))) break;
        seen |= 1u << button;
        buttons[i] = (unsigned char)button;
    }
    /* Profiles can be queried before input initialization from the main menu. */
    if (i < CONTROLLER_REMAP_ACTIONS) csmemcpy(buttons, standard, sizeof(standard));
}

int controller_remap_playing(short port)
{
    return game_in_progress() && !ui_widgets_active_for_local_player(port) &&
        !(port == 0 && network_chat_keyboard_open());
}

void controller_remap_prompt(short port, struct game_input_preferences *preferences)
{
    unsigned char buttons[CONTROLLER_REMAP_ACTIONS];
    long player;
    int i;
    if (port < 0 || port >= 4) return;
    player = local_player_get_player_index(port);
    if (player != NONE) port = player_get(player)->local_player_index;
    if (!controller_remap_read(port, buttons)) return;
    for (i = 0; i < CONTROLLER_REMAP_ACTIONS; i++)
        preferences->game_control_to_xbox_buttons[controller_remap_controls[i]] = buttons[i];
}
