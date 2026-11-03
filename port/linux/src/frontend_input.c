/* Input-source selection for the optional frontend. The caller reports raw
 * physical edges, before keyboard, touch and controller emulation are merged. */
#include "frontend_input.h"
#include <string.h>

static int frontend_input_enabled;
static int frontend_device = -1;
static int frontend_nonpad_device = -1;
static unsigned long frontend_pad_buttons[4];
static unsigned char frontend_keys[512];

void frontend_input_enable(int enabled)
{
    frontend_input_enabled = enabled;
#ifdef HALO_ANDROID
    frontend_device = -2;
#else
    frontend_device = -1;
#endif
    frontend_nonpad_device = frontend_device;
    memset(frontend_pad_buttons, 0, sizeof(frontend_pad_buttons));
    memset(frontend_keys, 0, sizeof(frontend_keys));
}

int frontend_input_device(void)
{
    return frontend_device;
}

void frontend_input_pad(short port, unsigned long buttons, int connected)
{
    if (!frontend_input_enabled || port < 0 || port >= 4) return;
    if (!connected)
    {
        frontend_pad_buttons[port] = 0;
        if (frontend_device == port) frontend_device = frontend_nonpad_device;
        return;
    }
    if (buttons & ~frontend_pad_buttons[port]) frontend_device = port;
    frontend_pad_buttons[port] = buttons;
}

void frontend_input_keys(unsigned char const *keys, unsigned int count)
{
    unsigned int i;
    if (!frontend_input_enabled) return;
    if (count > sizeof(frontend_keys)) count = sizeof(frontend_keys);
    for (i = 0; i < count; i++)
    {
        if (keys[i] && !frontend_keys[i]) frontend_device = frontend_nonpad_device = -1;
        frontend_keys[i] = keys[i];
    }
}

void frontend_input_pointer(int touch, int activity)
{
    if (frontend_input_enabled && activity) frontend_device = frontend_nonpad_device = touch ? -2 : -1;
}
