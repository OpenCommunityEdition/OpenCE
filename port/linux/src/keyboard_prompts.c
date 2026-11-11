/* Keyboard ownership follows XInput's port assignment, independently of ui.map.
 * Only port zero receives keyboard input; another player's pad cannot steal it. */
#include "keyboard_prompts.h"
#include "port_config.h"
#include <string.h>

static unsigned char previous_keys[512];
static unsigned long previous_buttons[4];
static int previous_analog[4], connected_ports[4];
#ifdef HALO_ANDROID
static int keyboard_active, nonpad_keyboard;
#else
static int keyboard_active = 1, nonpad_keyboard = 1;
#endif
static int previous_mouse;

void keyboard_prompt_pad(short port, unsigned long buttons, int analog, int connected)
{
    if (port < 0 || port >= 4) return;
    if (port == 0)
    {
        if (!connected) keyboard_active = nonpad_keyboard;
        else if (!connected_ports[port] || (buttons & ~previous_buttons[port]) ||
            (analog && !previous_analog[port])) keyboard_active = 0;
    }
    connected_ports[port] = connected;
    previous_buttons[port] = connected ? buttons : 0;
    previous_analog[port] = connected ? analog : 0;
}

void keyboard_prompt_keys(unsigned char const *keys, unsigned int count, int mouse)
{
    unsigned int i;
    if ((mouse & 0x100) || (mouse & ~previous_mouse & 0xff)) keyboard_active = nonpad_keyboard = 1;
    previous_mouse = mouse;
    if (count > sizeof(previous_keys)) count = sizeof(previous_keys);
    for (i = 0; i < count; i++)
    {
        if (keys[i] && !previous_keys[i]) keyboard_active = nonpad_keyboard = 1;
        previous_keys[i] = keys[i];
    }
}

void keyboard_prompt_touch(void)
{
    keyboard_active = nonpad_keyboard = 0;
}

int keyboard_prompt_active(short port)
{
    return port == 0 && keyboard_active;
}

int keyboard_prompt_enabled(short port)
{
    if (port < 0 || port >= 4) return 0;
    if (!config_boolean("input.auto_button_detection"))
        return !strcmp(config_string("display.hud_button_style"), "keyboard");
    return config_boolean("display.keyboard_glyphs") && keyboard_prompt_active(port);
}
