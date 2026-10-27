#ifndef CONTROLLER_BUTTONS_H
#define CONTROLLER_BUTTONS_H

enum controller_family
{
    CONTROLLER_UNKNOWN,
    CONTROLLER_XBOX,
    CONTROLLER_PLAYSTATION,
    CONTROLLER_SWITCH
};

struct controller_device
{
    int family;
    int type;
    unsigned int instance;
    unsigned short vendor, product;
    char name[128];
};

/* Uses the same port assignment as XInput, including keyboard/pad split screen.
 * A disconnected port returns a cleared identity; no saved mapping is changed. */
void controller_device_get(short port, struct controller_device *device);
void controller_device_read(void *gamepad, struct controller_device *device);
void controller_device_update(short port, void *gamepad);
int controller_button_style_index(char const *name);
int controller_button_style_family(int style);
char const *controller_button_family_setting(int family);
int controller_button_style_for_port(short port);

#endif
