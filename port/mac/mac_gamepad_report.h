#ifndef HALO_MAC_GAMEPAD_REPORT_H
#define HALO_MAC_GAMEPAD_REPORT_H

#include "mac_gamepad.h"

/* The USB input reports of Sony's pads, decoded to the game's pad model
   (mac_gamepad.h). Pure C: no IOKit, byte-wise reads only (every field used
   is one byte; the multi-byte ones, the timestamp and the motion sensors,
   are never read), so PowerPC and Intel decode alike.

   DualShock 4 (report 0x01, 64 bytes over USB; the first 10 bytes of the
   Bluetooth report 0x01 are the same; report 0x11 of Bluetooth carries it
   two bytes later):
     [0] report id, [1] left x, [2] left y, [3] right x, [4] right y
         (0 is left or up, 255 right or down)
     [5] low nibble: hat (0 north, clockwise, 8 none);
         high nibble: 0x10 square, 0x20 cross, 0x40 circle, 0x80 triangle
     [6] 0x01 L1, 0x02 R1, 0x04 L2, 0x08 R2, 0x10 share, 0x20 options,
         0x40 L3, 0x80 R3
     [7] 0x01 PS, 0x02 touchpad click (the rest is a counter)
     [8] L2 analog, [9] R2 analog
   DualSense (report 0x01, 64 bytes over USB):
     [0] report id, [1] left x, [2] left y, [3] right x, [4] right y,
     [5] L2 analog, [6] R2 analog, [7] counter,
     [8] hat and face buttons as the DualShock 4's [5],
     [9] 0x01 L1, 0x02 R1, 0x04 L2, 0x08 R2, 0x10 create, 0x20 options,
         0x40 L3, 0x80 R3
     [10] 0x01 PS, 0x02 touchpad click, 0x04 mute
   8BitDo Wireless USB Adapter 2 in Dinput mode (2dc8:3105, "8BitDo Receiver";
   Create + D-pad Left for 3 seconds), whatever pad is paired to it: report 1,
   10 bytes. The layout is the receiver's own report descriptor (read on a
   MacBook6,1 on 10.6.8: 15 one-bit buttons and a padding bit, a four-bit hat,
   X Y Z Rz, then the simulation controls Accelerator and Brake) and a
   recording of it with a DualSense paired:
     [0] report id 1, [1] buttons 1 to 8 (bit 0 is button 1), [2] buttons 9 to 15,
     [3] low nibble: hat (0 north, clockwise, 8 and up none; the high nibble is
         padding; bit 0x10, the touchpad click, is set with the hat at rest), [4] left x, [5] left y, [6] right x, [7] right y
         (0 left or up, 255 right or down; 127 at rest),
     [8] L2 analog (Accelerator), [9] R2 analog (Brake): captured with a DualSense,
         L2 held sets button 10 and byte 8, R2 held sets button 9 and byte 9.
   The button numbers are 8BitDo's Dinput order, which SDL's controller
   database lists for this receiver (SDL_GameControllerDB, "8BitDo Adapter 2",
   03000000c82d00000531...: a:b0 b:b1 x:b3 y:b4 leftshoulder:b6
   rightshoulder:b7 lefttrigger:b8 righttrigger:b9 back:b10 start:b11
   leftstick:b13 rightstick:b14, its b being the button number less one);
   the recordings agree for L3 and R3 (buttons 14 and 15) but put L2 and R2
   on buttons 10 and 9, the other way round from that entry. With a DualSense: 1 cross, 2 circle, 4 square,
   5 triangle, 7 L1, 8 R1, 9 R2, 10 L2, 11 create, 12 options, 14 L3, 15 R3;
   3, 6 and 13 (the PS button, perhaps 3) are not used by the game. */

#define MAC_PAD_SONY_VENDOR 0x054c
#define MAC_PAD_DUALSHOCK4_PRODUCT 0x05c4
#define MAC_PAD_DUALSHOCK4_V2_PRODUCT 0x09cc
#define MAC_PAD_DUALSENSE_PRODUCT 0x0ce6
#define MAC_PAD_8BITDO_VENDOR 0x2dc8
/* the Wireless USB Adapter 2 in Dinput mode; its other identities are 3107
   "IDLE" (waiting for a pad: a vendor-defined report only, not a game pad),
   and in its automatic mode a DualShock 4 (054c:05c4) or a PlayStation
   Classic pad (054c:0cda) */
#define MAC_PAD_8BITDO_DINPUT_PRODUCT 0x3105
#define MAC_PAD_8BITDO_IDLE_PRODUCT 0x3107
#define MAC_PAD_PSCLASSIC_PRODUCT 0x0cda

enum {
    MAC_PAD_REPORT_NONE,
    MAC_PAD_REPORT_DUALSHOCK4,
    MAC_PAD_REPORT_DUALSENSE,
    MAC_PAD_REPORT_8BITDO_DINPUT
};

/* buttons the game has no use for, beside the frame (mac_gamepad_decode_report) */
enum {
    MAC_PAD_EXTRA_PS = 1 << 0,
    MAC_PAD_EXTRA_TOUCHPAD = 1 << 1,
    MAC_PAD_EXTRA_MUTE = 1 << 2
};

/* The game's button number (1 to 6 the analog buttons A B X Y white black,
   7 to 10 back, start, left and right thumb; 0 for none) of an 8BitDo
   Dinput button (1 to 15), and of a PlayStation Classic pad's button (1 to 10).
   The element mapping (mac_gamepad_map) and the report decoder use them. */
unsigned mac_gamepad_dinput_button(unsigned number);
unsigned mac_gamepad_psclassic_button(unsigned number);

/* the report layout of a Sony id, or MAC_PAD_REPORT_NONE */
int mac_gamepad_report_kind(unsigned vendor, unsigned product);
/* the pad's name for lists, or "" */
const char *mac_gamepad_report_name(int kind);

/* The output report that sets the rumble motors, as the pad takes it over USB (the report id first, as
   IOHIDDeviceInterface::setReport is given it): strong is the large motor, weak the small one, each 0 to 255.
     DualShock 4 (report 0x05, 32 bytes): [1] 0x01 (rumble; the light bar is left as it is),
         [4] the small motor, [5] the large motor.
     DualSense (report 0x02, 63 bytes): [1] 0x03 (compatible vibration, haptics select), [2] 0,
         [3] the right (small) motor, [4] the left (large) motor.
   Returns the report's length, or 0 when the kind has none or size is too small. */
#define MAC_PAD_RUMBLE_REPORT_MAXIMUM 63
unsigned mac_gamepad_rumble_report(int kind, unsigned char strong, unsigned char weak,
    unsigned char *out, unsigned size);

/* Decodes one input report as the interrupt pipe delivers it (the report id
   first). Returns 1, with the frame connected, or 0 when the report is not
   the pad's state (another report id, too short). extra may be NULL. */
int mac_gamepad_decode_report(int kind, const unsigned char *data, unsigned size,
    struct mac_gamepad_frame *out, unsigned *extra);

#endif
