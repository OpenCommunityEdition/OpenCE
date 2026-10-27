/* Device identity and visual preferences. Neither changes input mappings. */
#include "controller_buttons.h"
#include "port_config.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

/* Updated by the game's input polling, read by menus/HUD on the same thread. */
static struct controller_device controller_devices[4];

void controller_device_update(short port, void *gamepad)
{
    if (port >= 0 && port < 4) controller_device_read(gamepad, &controller_devices[port]);
}

void controller_device_get(short port, struct controller_device *device)
{
    memset(device, 0, sizeof(*device));
    if (port >= 0 && port < 4) *device = controller_devices[port];
}

static int controller_family_from_type(SDL_GamepadType type)
{
    switch (type)
    {
    case SDL_GAMEPAD_TYPE_XBOX360:
    case SDL_GAMEPAD_TYPE_XBOXONE: return CONTROLLER_XBOX;
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5: return CONTROLLER_PLAYSTATION;
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: return CONTROLLER_SWITCH;
    default: return CONTROLLER_UNKNOWN;
    }
}

void controller_device_read(void *handle, struct controller_device *device)
{
    SDL_Gamepad *gamepad = handle;
    memset(device, 0, sizeof(*device));
    if (!gamepad) return;
    device->type = SDL_GetGamepadType(gamepad);
    device->family = controller_family_from_type((SDL_GamepadType)device->type);
#ifndef HALO_ARM64_GUEST
    {
        char const *name = SDL_GetGamepadName(gamepad);
        device->instance = SDL_GetGamepadID(gamepad);
        device->vendor = SDL_GetGamepadVendor(gamepad);
        device->product = SDL_GetGamepadProduct(gamepad);
        snprintf(device->name, sizeof(device->name), "%s", name ? name : "Unknown controller");
    }
#else
    /* The guest's SDL bridge exposes the family but not a device name. */
    snprintf(device->name, sizeof(device->name), "%s",
        device->family == CONTROLLER_XBOX ? "Xbox controller" :
        device->family == CONTROLLER_PLAYSTATION ? "PlayStation controller" :
        device->family == CONTROLLER_SWITCH ? "Switch controller" : "Unknown controller");
#endif
}

int controller_button_style_index(char const *name)
{
    static char const *names[] = {"classic", "gems", "menu_gems", "xbox_modern",
        "ps_classic", "ps_gems", "ps_dark_gems", "ps_modern",
        "switch_classic", "switch_gems", "switch_modern"};
    int i;
    if (!strcmp(name, "dark_gems")) return 2;
    for (i = 0; i < 11; i++) if (!strcmp(name, names[i])) return i;
    return 0;
}

int controller_button_style_family(int style)
{
    return style < 0 || style >= 11 ? CONTROLLER_UNKNOWN :
        style < 4 ? CONTROLLER_XBOX : style < 8 ? CONTROLLER_PLAYSTATION : CONTROLLER_SWITCH;
}

char const *controller_button_family_setting(int family)
{
    switch (family)
    {
    case CONTROLLER_XBOX: return "display.hud_button_style_xbox";
    case CONTROLLER_PLAYSTATION: return "display.hud_button_style_ps";
    case CONTROLLER_SWITCH: return "display.hud_button_style_switch";
    default: return "display.hud_button_style";
    }
}

int controller_button_style_for_port(short port)
{
    struct controller_device device;
    int style;
    /* Always identify devices, even when automatic glyph selection is off. */
    controller_device_get(port, &device);
    if (!config_boolean("input.auto_button_detection") || !device.family)
        return controller_button_style_index(config_string("display.hud_button_style"));
    style = controller_button_style_index(config_string(controller_button_family_setting(device.family)));
    if (controller_button_style_family(style) == device.family) return style;
    return device.family == CONTROLLER_PLAYSTATION ? 4 : device.family == CONTROLLER_SWITCH ? 8 : 0;
}
