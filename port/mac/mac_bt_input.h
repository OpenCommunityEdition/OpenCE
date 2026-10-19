#ifndef HALO_MAC_BT_INPUT_H
#define HALO_MAC_BT_INPUT_H

#include "mac_gamepad.h"

/* A DualSense or DualShock 4 over Bluetooth, read for the game through
   PowerSpark's own Bluetooth stack (bt/) and a USB dongle (a TP-Link UB500).
   input.bluetooth turns it on; it is off by default.

   The stack runs on a thread of its own (bt/bt_session.c): it opens the
   dongle, loads the firmware, accepts the paired pad when its PS button is
   pressed and decodes its reports. The game's thread only copies the
   thread's latest snapshot, under a lock that is held for a copy and never
   across anything slow, so a frame never waits for the dongle. Pairing is
   Controller Setup's (its Bluetooth tab); the game never searches.

   mac_gamepad.c calls these (it has weak stand-ins, so tools that link it
   without the stack see no Bluetooth pad). */

/* The pad's state now. Returns 1, with *out connected, while a pad is
   connected and sending reports; 0 otherwise. Starts the stack when the
   setting is on (and stops it when it goes off), and starts it again every
   few seconds while the dongle is unplugged or held by another program
   (Controller Setup's window, Apple's driver). Cheap enough for every frame. */
int mac_bt_input_poll(struct mac_gamepad_frame *out);
/* The motors, 0 to 255 (strong: the large one). Returns 1 when a pad is connected to take them. */
int mac_bt_input_rumble(unsigned char strong, unsigned char weak);
/* the connected pad's name ("DualSense", ...) for logs and lists, or "" */
const char *mac_bt_input_name(void);
/* Stops the stack and lets the dongle go (the pad is disconnected). Waits for it, about a second
   with a pad connected. */
void mac_bt_input_stop(void);

#endif
