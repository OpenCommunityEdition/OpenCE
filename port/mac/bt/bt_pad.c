/* DualSense and DualShock 4 reports over Bluetooth (bt_stack.h): the input
   reports rewritten to the layout the game's decoder reads
   (mac_gamepad_report.c), and the output reports that set the light bar,
   the player lights and the motors.

   The layouts come from public descriptions: the Linux kernel's
   hid-playstation driver and SDL's PlayStation drivers were read as
   documentation, and nothing was copied.

   DualSense output report 0x31 (Bluetooth), 78 bytes:
     [0] 0x31   [1] sequence number in the high nibble (0 to 15, one more for
     every report)   [2] 0x10 (the tag)   [3..49] the 47 bytes the USB report 0x02
     has after its id, "common" below   [50..73] zero   [74..77] CRC-32,
     little-endian, over 0xA2 (the HIDP header of an output report, not
     in the packet) and bytes 0 to 73.
   common[]:
     [0] valid flags 0: 0x01 compatible vibration, 0x02 haptics select
     [1] valid flags 1: 0x01 mute LED, 0x02 power save, 0x04 light bar colour,
         0x08 release the lights to the pad, 0x10 player lights
     [2] right (small) motor   [3] left (large) motor
     [4..9] audio volumes and flags, [8] mute LED, [9] power save: zero
     [10..37] zero
     [38] valid flags 2: 0x02 light bar setup, 0x04 the second vibration mode
     [39,40] zero
     [41] light bar setup: 0x02 fades the pad's own light out (the pairing
          light that pulses blue until a host takes it over), 0x01 fades in
     [42] LED brightness   [43] player lights: bits 0 to 4, the middle one 0x04
     [44] red  [45] green  [46] blue
   A pad that has just connected keeps showing its own light until a report
   with the light bar setup flag (flags 2 and common[41] = 2) fades it
   out; the colour is then taken. The setup report goes first and alone.

   DualShock 4 output report 0x11 (Bluetooth), 78 bytes:
     [0] 0x11  [1] 0xC0 (HID and CRC flags) | poll interval  [2] 0
     [3..] valid flags 0: 0x01 motors, 0x02 light bar, 0x04 flash;
     [4] flags 1, [5] 0, [6] right (small) motor, [7] left (large) motor,
     [8] red [9] green [10] blue, [11] flash on, [12] flash off;
     [74..77] CRC-32 as above. (Not tried on a pad yet.) */
#include "bt_stack.h"

#include <string.h>

int bt_pad_kind_from_name(const char *name)
{
    if (!name) return BT_KIND_UNKNOWN;
    if (strstr(name, "DualSense")) return BT_KIND_DUALSENSE;
    if (strstr(name, "DualShock") || strcmp(name, "Wireless Controller") == 0) return BT_KIND_DUALSHOCK4;
    return BT_KIND_UNKNOWN;
}

const char *bt_pad_kind_name(int kind)
{
    return kind == BT_KIND_DUALSENSE ? "DualSense" : kind == BT_KIND_DUALSHOCK4 ? "DualShock 4" : "";
}

unsigned bt_pad_build_dualsense(unsigned char *r, unsigned sequence, const struct bt_pad_output *o)
{
    unsigned char *c = r + 3;
    unsigned crc;
    memset(r, 0, 78);
    r[0] = 0x31;
    r[1] = (unsigned char)((sequence & 0x0f) << 4);
    r[2] = 0x10;
    if (o->fade_out_setup) {
        /* the setup report: only the light bar setup, and nothing else */
        c[38] = 0x02;
        c[41] = 0x02;
    } else {
        if (o->set_rumble) {
            c[0] = 0x03;
            c[2] = o->weak;
            c[3] = o->strong;
        }
        if (o->set_light) {
            c[1] |= 0x04;
            c[44] = o->red;
            c[45] = o->green;
            c[46] = o->blue;
        }
        if (o->set_players) {
            c[1] |= 0x10;
            c[43] = (unsigned char)(o->player_leds & 0x1f);
        }
    }
    crc = bt_crc32_report(0xa2, r, 74);
    bt_put32(r + 74, crc);
    return 78;
}

unsigned bt_pad_build_dualshock4(unsigned char *r, unsigned sequence, const struct bt_pad_output *o)
{
    unsigned char *c = r + 3;
    unsigned crc;
    (void)sequence;
    memset(r, 0, 78);
    r[0] = 0x11;
    r[1] = 0xc4;
    if (o->set_rumble) {
        c[0] |= 0x01;
        c[3] = o->weak;
        c[4] = o->strong;
    }
    if (o->set_light) {
        c[0] |= 0x02;
        c[5] = o->red;
        c[6] = o->green;
        c[7] = o->blue;
    }
    crc = bt_crc32_report(0xa2, r, 74);
    bt_put32(r + 74, crc);
    return 78;
}

int bt_pad_input_crc_ok(const unsigned char *r, unsigned size)
{
    if (size != 78) return 1; /* (nothing to check) */
    return bt_get32(r + 74) == bt_crc32_report(0xa1, r, 74);
}

/* one touch point: contact (bit 7 set when there is no finger), x low,
   x high in the low nibble with y low above it, y high */
static void touch_point(const unsigned char *p, struct bt_touch *t)
{
    t->active = (p[0] & 0x80) == 0;
    t->x = (int)p[1] | (((int)p[2] & 0x0f) << 8);
    t->y = ((int)p[3] << 4) | ((int)p[2] >> 4);
}

int bt_pad_decode(int kind, const unsigned char *r, unsigned size, struct mac_gamepad_frame *frame,
    unsigned *extra, struct bt_touch touch[2], int *touch_width, int *touch_height, int *battery_percent)
{
    unsigned char usb[64];
    unsigned i;
    int decoder = kind == BT_KIND_DUALSENSE ? MAC_PAD_REPORT_DUALSENSE : MAC_PAD_REPORT_DUALSHOCK4;

    touch[0].active = touch[1].active = 0;
    *battery_percent = -1;
    if (size < 10 || !r) return 0;
    memset(usb, 0, sizeof(usb));
    usb[0] = 0x01;
    if (r[0] == 0x31 && kind == BT_KIND_DUALSENSE && size >= 12) {
        /* every field one byte later than the USB report's */
        for (i = 1; i < 64 && i + 1 < size; i++) usb[i] = r[i + 1];
        *touch_width = 1920;
        *touch_height = 1080;
        if (size >= 42) {
            touch_point(r + 34, &touch[0]);
            touch_point(r + 38, &touch[1]);
        }
        if (size >= 55) {
            unsigned level = r[54] & 0x0f;
            *battery_percent = level >= 10 ? 100 : (int)(level * 10 + 5);
        }
    } else if (r[0] == 0x11 && size >= 12) {
        /* two bytes later than the USB report's */
        decoder = MAC_PAD_REPORT_DUALSHOCK4;
        for (i = 1; i < 64 && i + 2 < size; i++) usb[i] = r[i + 2];
        *touch_width = 1920;
        *touch_height = 942;
        if (size >= 45) {
            touch_point(r + 37, &touch[0]);
            touch_point(r + 41, &touch[1]);
        }
        if (size >= 33) {
            unsigned level = r[32] & 0x0f;
            *battery_percent = level >= 8 ? 100 : (int)(level * 100 / 8);
        }
    } else if (r[0] == 0x01) {
        /* the simple report: the DualShock 4's USB layout, from either pad */
        decoder = MAC_PAD_REPORT_DUALSHOCK4;
        for (i = 0; i < 64 && i < size; i++) usb[i] = r[i];
    } else
        return 0;
    return mac_gamepad_decode_report(decoder, usb, 64, frame, extra);
}
