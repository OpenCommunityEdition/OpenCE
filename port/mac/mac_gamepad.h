#ifndef HALO_MAC_GAMEPAD_H
#define HALO_MAC_GAMEPAD_H

/* Game pads through IOKit's HID device interface, which Mac OS X 10.4 to
   10.6 all have, on PowerPC and Intel. Up to four HID game pads or joysticks are
   read (the first found is in slot 0), a pad published as an Xbox 360 pad (vendor 0x045E, product
   0x028E: the Xbox One S controller's driver, or a wired 360 pad's) before
   any other. A Sony DualShock 4 or DualSense (mac_gamepad_report.h) is
   read from its USB input reports, as is 8BitDo's Wireless USB Adapter 2 in
   Dinput mode (2dc8:3105); every other pad from its HID elements, by usage
   (mac_gamepad_map). A device that is not a game pad, as the adapter is
   while it waits for a controller (2dc8:3107, a vendor-defined page), is
   never opened. The values come in XInput's shape: sticks -32768..32767 with
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
   Looks for more pads every two seconds while a slot is free, and at once
   when IOKit publishes a HID device or terminates a pad's (a pad that goes
   is closed on the next call, and its slot is held for twenty seconds for a
   pad of its kind: 8BitDo's adapter detaches and attaches again, with other
   ids, as it pairs, and keeps its player). */
int mac_gamepad_poll_all(struct mac_gamepad_frame frames[MAC_GAMEPAD_MAXIMUM_DEVICES]);
const char *mac_gamepad_name_at(int slot);
/* whether the slot's pad is open but silent: an adapter that streams all the time (Dinput mode) has sent
   no report for a second, its pad asleep or out of range. The slot is not connected (neutral input, as an
   unplugged pad) and keeps its pad: the reports resuming (PS pressed) connect it again. */
int mac_gamepad_asleep_at(int slot);
/* whether the slot's pad is a Sony pad (a DualShock 4 or DualSense, or an
   adapter presenting itself as one): for the PlayStation button names */
int mac_gamepad_is_playstation_at(int slot);

/* Sets the motors of the pad in a slot (strong: the large one, weak: the small
   one; 0 to 255). Only a Sony pad on USB takes it (mac_gamepad_report.h's
   output reports); returns 1 when the pad has them set, 0 for any other pad. */
int mac_gamepad_rumble(int slot, unsigned char strong, unsigned char weak);

/* The first pad's state now; not connected when there is none. Looks for a
   pad again every two seconds while there is none, and when one goes. */
void mac_gamepad_poll(struct mac_gamepad_frame *out);
void mac_gamepad_stop(void);
/* the pad's name for logs, or "" */
const char *mac_gamepad_name(void);

#endif
