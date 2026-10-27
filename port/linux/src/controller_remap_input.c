/* Physical controller remapping and capture, on the game's input thread. */
#include "platform.h"
#include "sdl_platform.h"
#include "controller_remap.h"
#include "controller_buttons.h"
#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>

static WORD const digital_masks[8] = {XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB};

static struct
{
    short port;
    int phase, result, button;
    unsigned int instance;
    Uint64 polled;
} capture = {-1, 0, 0, 0, 0, 0};

static struct
{
    int custom;
    unsigned int blocked;
    unsigned char layout[CONTROLLER_REMAP_ACTIONS], targets[CONTROLLER_REMAP_ACTIONS];
} remap_ports[4];

static void physical_values(XINPUT_GAMEPAD const *pad, unsigned char *values)
{
    int i;
    memcpy(values, pad->bAnalogButtons, 8);
    for (i = 0; i < 8; i++) values[8+i] = pad->wButtons & digital_masks[i] ? 255 : 0;
}

static int capture_device_present(void)
{
    struct controller_device device;
    controller_device_get(capture.port, &device);
    return device.name[0] && device.instance == capture.instance;
}

void controller_remap_capture_begin(short port)
{
    struct controller_device device;
    controller_remap_capture_end();
    if (port < 0 || port >= 4) return;
    controller_device_get(port, &device);
    capture.port = port;
    capture.instance = device.instance;
    capture.phase = 1; /* Opening press must be released first. */
    capture.polled = SDL_GetTicks();
    platform_binding_capture_begin();
}

void controller_remap_capture_end(void)
{
    if (!capture.phase) return;
    /* A disconnected port may no longer receive XInputGetState calls. */
    capture.phase = capture_device_present() ? 4 : 0;
    if (!capture.phase) capture.port = -1;
    capture.result = 0;
    platform_binding_capture_cancel();
}

int controller_remap_capture_poll(int *button)
{
    int key, result;
    if (!capture.phase || capture.phase == 4) return 3;
    if (!capture_device_present())
    {
        controller_remap_capture_end();
        return 3;
    }
    capture.polled = SDL_GetTicks();
    result = platform_binding_capture_poll(&key);
    if (result)
    {
        if (result == 3) { capture.result = 3; capture.phase = 3; }
        else platform_binding_capture_begin(); /* Only Escape cancels; keys cannot bind a pad. */
    }
    if (capture.phase != 3) return 0;
    result = capture.result;
    *button = capture.button;
    controller_remap_capture_end();
    return result;
}

int controller_remap_input(short port, XINPUT_GAMEPAD *pad, int connected)
{
    unsigned char values[16], layout[CONTROLLER_REMAP_ACTIONS], targets[CONTROLLER_REMAP_ACTIONS];
    unsigned int held = 0;
    struct controller_device device;
    int i, active;
    if (port < 0 || port >= 4) return 0;
    /* Check from any polled port, including while the selected one is absent. */
    if (capture.phase && (!capture_device_present() ||
        (capture.phase < 4 && SDL_GetTicks() - capture.polled > 1000)))
        controller_remap_capture_end();
    /* Another controller cannot navigate the editor while it waits for the
     * selected port. Players still in gameplay retain their own input. */
    if (capture.phase && capture.port != port && !controller_remap_playing(port))
    {
        memset(pad, 0, sizeof(*pad));
        return 1;
    }
    physical_values(pad, values);
    for (i = 0; i < 16; i++) if (values[i] > 30) held |= 1u << i;
    if (capture.phase && capture.port == port)
    {
        controller_device_get(port, &device);
        if (capture.phase < 3 && (!connected || device.instance != capture.instance))
        { capture.result = 3; capture.phase = 3; }
        if (capture.phase == 1 && !held) capture.phase = 2;
        else if (capture.phase == 2 && held)
        {
            if (held & ((1u << 12) | (1u << 13)))
            { capture.result = 3; capture.phase = 3; }
            else
                for (i = 0; i < 16; i++)
                    if ((i < 8 || i == 10 || i >= 14) && (held & (1u << i)))
                    { capture.button = i; capture.result = 1; capture.phase = 3; break; }
        }
        if (capture.phase == 4 && !held) { capture.phase = 0; capture.port = -1; }
        memset(pad, 0, sizeof(*pad));
        remap_ports[port].custom = 0;
        return 1;
    }
    active = connected && controller_remap_read(port, layout) && controller_remap_playing(port);
    if (!active) { memset(&remap_ports[port], 0, sizeof(remap_ports[port])); return 0; }
    controller_remap_profile(port, targets);
    if (!remap_ports[port].custom || memcmp(layout, remap_ports[port].layout, sizeof(layout)) ||
        memcmp(targets, remap_ports[port].targets, sizeof(targets)))
    {
        remap_ports[port].blocked = held;
        memcpy(remap_ports[port].layout, layout, sizeof(layout));
        memcpy(remap_ports[port].targets, targets, sizeof(targets));
    }
    remap_ports[port].custom = 1;
    remap_ports[port].blocked &= held;
    memset(pad->bAnalogButtons, 0, sizeof(pad->bAnalogButtons));
    pad->wButtons &= ~(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB);
    /* D-pad Left can be an action button. Consume it only when the
     * custom layout or the profile translation uses it; otherwise retain movement. */
    for (i = 0; i < CONTROLLER_REMAP_ACTIONS; i++)
        if (layout[i] == 10 || targets[i] == 10) pad->wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
    for (i = 0; i < CONTROLLER_REMAP_ACTIONS; i++)
    {
        unsigned char pressure = remap_ports[port].blocked & (1u << layout[i]) ? 0 : values[layout[i]];
        if (targets[i] < 8) pad->bAnalogButtons[targets[i]] = pressure;
        else if (pressure > 30) pad->wButtons |= digital_masks[targets[i]-8];
    }
    return 0;
}

/* The existing SDL merge's maximum pressure / greatest stick deflection rules.
 * Run only after remapping the physical pad, so keys and touch keep their own bindings. */
void controller_remap_merge(XINPUT_GAMEPAD const *source, XINPUT_GAMEPAD *target)
{
    int i;
    target->wButtons |= source->wButtons;
    for (i = 0; i < 8; i++)
        if (source->bAnalogButtons[i] > target->bAnalogButtons[i]) target->bAnalogButtons[i] = source->bAnalogButtons[i];
    if (abs(source->sThumbLX) > abs(target->sThumbLX)) target->sThumbLX = source->sThumbLX;
    if (abs(source->sThumbLY) > abs(target->sThumbLY)) target->sThumbLY = source->sThumbLY;
    if (abs(source->sThumbRX) > abs(target->sThumbRX)) target->sThumbRX = source->sThumbRX;
    if (abs(source->sThumbRY) > abs(target->sThumbRY)) target->sThumbRY = source->sThumbRY;
}
