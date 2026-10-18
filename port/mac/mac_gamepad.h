#ifndef HALO_MAC_GAMEPAD_H
#define HALO_MAC_GAMEPAD_H

/* Game pads through IOKit's HID device interface, which Mac OS X 10.4 to
   10.6 all have, on PowerPC and Intel. Up to four HID game pads or joysticks are
   read (the first found is in slot 0), a pad published as an Xbox 360 pad (vendor 0x045E, product
   0x028E: the Xbox One S controller's driver, or a wired 360 pad's) before
   any other. The values come in XInput's shape: sticks -32768..32767 with
   up positive, triggers and the face buttons 0..255. */

enum {
    MAC_GAMEPAD_DPAD_UP = 1 << 0,
    MAC_GAMEPAD_DPAD_DOWN = 1 << 1,
    MAC_GAMEPAD_DPAD_LEFT = 1 << 2,
    MAC_GAMEPAD_DPAD_RIGHT = 1 << 3,
    MAC_GAMEPAD_START = 1 << 4,
    MAC_GAMEPAD_BACK = 1 << 5,
    MAC_GAMEPAD_LEFT_THUMB = 1 << 6,
    MAC_GAMEPAD_RIGHT_THUMB = 1 << 7
};

/* the analog buttons, in the game's order (input.h) */
enum {
    MAC_GAMEPAD_A, MAC_GAMEPAD_B, MAC_GAMEPAD_X, MAC_GAMEPAD_Y,
    MAC_GAMEPAD_BLACK, MAC_GAMEPAD_WHITE,
    MAC_GAMEPAD_LEFT_TRIGGER, MAC_GAMEPAD_RIGHT_TRIGGER,
    MAC_GAMEPAD_ANALOG_COUNT
};

struct mac_gamepad_frame {
    int connected;
    unsigned char analog[MAC_GAMEPAD_ANALOG_COUNT];
    unsigned buttons;
    short left_x, left_y, right_x, right_y;
};

/* One HID element as the device describes it. */
struct mac_gamepad_element {
    unsigned usage_page, usage;
    long minimum, maximum;
};

#define MAC_GAMEPAD_MAXIMUM_ELEMENTS 64
/* the pads read at once: the controllers 1 to 4 of local play */
#define MAC_GAMEPAD_MAXIMUM_DEVICES 4

/* The pad's state from its elements' values (pure: no IOKit). */
void mac_gamepad_map(const struct mac_gamepad_element *elements,
    const long *values, unsigned count, unsigned vendor, unsigned product,
    struct mac_gamepad_frame *out);

/* Every pad's state now, by slot (a pad keeps its slot while it stays
   connected; a free slot is not connected). Returns the pads connected.
   Looks for more pads every two seconds while a slot is free. */
int mac_gamepad_poll_all(struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES]);
const char *mac_gamepad_name_at(int slot);

/* The first pad's state now; not connected when there is none. Looks for a
   pad again every two seconds while there is none, and when one goes. */
void mac_gamepad_poll(struct mac_gamepad_frame *out);
void mac_gamepad_stop(void);
/* the pad's name for logs, or "" */
const char *mac_gamepad_name(void);

#endif
