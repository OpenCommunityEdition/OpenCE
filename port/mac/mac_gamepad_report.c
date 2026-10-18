/* Sony pads' USB input reports (mac_gamepad_report.h). */
#include "mac_gamepad_report.h"
#include <string.h>

int mac_gamepad_report_kind(unsigned vendor, unsigned product)
{
    if (vendor == MAC_PAD_8BITDO_VENDOR)
        return product == MAC_PAD_8BITDO_DINPUT_PRODUCT ? MAC_PAD_REPORT_8BITDO_DINPUT : MAC_PAD_REPORT_NONE;
    if (vendor != MAC_PAD_SONY_VENDOR) return MAC_PAD_REPORT_NONE;
    if (product == MAC_PAD_DUALSHOCK4_PRODUCT || product == MAC_PAD_DUALSHOCK4_V2_PRODUCT)
        return MAC_PAD_REPORT_DUALSHOCK4;
    if (product == MAC_PAD_DUALSENSE_PRODUCT) return MAC_PAD_REPORT_DUALSENSE;
    return MAC_PAD_REPORT_NONE;
}

const char *mac_gamepad_report_name(int kind)
{
    return kind == MAC_PAD_REPORT_DUALSHOCK4 ? "DualShock 4" :
        kind == MAC_PAD_REPORT_DUALSENSE ? "DualSense" :
        kind == MAC_PAD_REPORT_8BITDO_DINPUT ? "8BitDo Dinput" : "";
}

/* 8BitDo's Dinput order (mac_gamepad_report.h), by button number 1 to 15 */
unsigned mac_gamepad_dinput_button(unsigned number)
{
    static const unsigned char table[15] = { 1, 2, 0, 3, 4, 0, 5, 6, 0, 0, 7, 8, 0, 9, 10 };
    return number >= 1 && number <= 15 ? table[number - 1] : 0;
}

/* The PlayStation Classic pad as SDL's controller database lists it
   (030000004c050000da0c...: a:b2 b:b1 x:b3 y:b0 leftshoulder:b6
   rightshoulder:b7 back:b8 start:b9; L2 and R2, b4 and b5, are the triggers;
   the d-pad is the X and Y axes), by button number 1 to 10 */
unsigned mac_gamepad_psclassic_button(unsigned number)
{
    static const unsigned char table[10] = { 4, 2, 1, 3, 0, 0, 5, 6, 7, 8 };
    return number >= 1 && number <= 10 ? table[number - 1] : 0;
}

static void game_button(unsigned number, struct mac_gamepad_frame *out)
{
    static const int analog[6] = {
        MAC_GAMEPAD_A, MAC_GAMEPAD_B, MAC_GAMEPAD_X, MAC_GAMEPAD_Y,
        MAC_GAMEPAD_WHITE, MAC_GAMEPAD_BLACK
    };
    static const unsigned digital[4] = {
        MAC_GAMEPAD_BACK, MAC_GAMEPAD_START, MAC_GAMEPAD_LEFT_THUMB, MAC_GAMEPAD_RIGHT_THUMB
    };
    if (number >= 1 && number <= 6) out->analog[analog[number - 1]] = 255;
    else if (number >= 7 && number <= 10) out->buttons |= digital[number - 7];
}

/* a stick byte (0 left or up) as XInput's axis, up positive when flipped:
   the same scaling as mac_gamepad.c's stick_value over 0..255 */
static short stick_byte(unsigned char value, int flip)
{
    int scaled = (int)value * 257 - 32768;
    if (flip) scaled = -scaled - 1;
    return (short)scaled;
}

static unsigned hat_nibble(unsigned char nibble)
{
    static const unsigned eight[8] = {
        MAC_GAMEPAD_DPAD_UP, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_RIGHT,
        MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_RIGHT,
        MAC_GAMEPAD_DPAD_DOWN, MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_LEFT,
        MAC_GAMEPAD_DPAD_LEFT, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_LEFT
    };
    return nibble < 8 ? eight[nibble] : 0;
}

/* Cross, circle, square, triangle as the A, B, X, Y buttons; L1 and R1 as
   the white and black buttons (the Xbox One S driver's pad does the same
   with its bumpers); share or create as back, options as start. */
static void face_buttons(unsigned char face, unsigned char shoulder,
    struct mac_gamepad_frame *out)
{
    if (face & 0x20) out->analog[MAC_GAMEPAD_A] = 255;
    if (face & 0x40) out->analog[MAC_GAMEPAD_B] = 255;
    if (face & 0x10) out->analog[MAC_GAMEPAD_X] = 255;
    if (face & 0x80) out->analog[MAC_GAMEPAD_Y] = 255;
    if (shoulder & 0x01) out->analog[MAC_GAMEPAD_WHITE] = 255;
    if (shoulder & 0x02) out->analog[MAC_GAMEPAD_BLACK] = 255;
    if (shoulder & 0x10) out->buttons |= MAC_GAMEPAD_BACK;
    if (shoulder & 0x20) out->buttons |= MAC_GAMEPAD_START;
    if (shoulder & 0x40) out->buttons |= MAC_GAMEPAD_LEFT_THUMB;
    if (shoulder & 0x80) out->buttons |= MAC_GAMEPAD_RIGHT_THUMB;
}

/* a trigger: its analog byte, or a full pull when only its button is down */
static unsigned char trigger_byte(unsigned char analog, int button)
{
    return analog == 0 && button ? 255 : analog;
}

int mac_gamepad_decode_report(int kind, const unsigned char *data, unsigned size,
    struct mac_gamepad_frame *out, unsigned *extra)
{
    unsigned char left_trigger, right_trigger, face_hat, shoulder, system;
    if (extra) *extra = 0;
    if (!data) return 0;
    if (kind == MAC_PAD_REPORT_8BITDO_DINPUT) {
        unsigned bits, number;
        if (size < 10 || data[0] != 0x01) return 0;
        memset(out, 0, sizeof(*out));
        out->connected = 1;
        bits = (unsigned)data[1] | ((unsigned)data[2] << 8);
        for (number = 1; number <= 15; ++number)
            if (bits & (1u << (number - 1))) game_button(mac_gamepad_dinput_button(number), out);
        out->buttons |= hat_nibble((unsigned char)(data[3] & 0x0f));
        out->left_x = stick_byte(data[4], 0);
        out->left_y = stick_byte(data[5], 1);
        out->right_x = stick_byte(data[6], 0);
        out->right_y = stick_byte(data[7], 1);
        /* (as the receiver sent them with a DualSense: L2 is byte 8 with button 10, R2 byte 9 with button 9) */
        out->analog[MAC_GAMEPAD_LEFT_TRIGGER] = trigger_byte(data[8], (bits & 0x200) != 0);
        out->analog[MAC_GAMEPAD_RIGHT_TRIGGER] = trigger_byte(data[9], (bits & 0x100) != 0);
        if (extra && (data[3] & 0x10)) *extra |= MAC_PAD_EXTRA_TOUCHPAD;
        return 1;
    }
    if (kind == MAC_PAD_REPORT_DUALSHOCK4) {
        /* (Bluetooth's full report 0x11 has two more header bytes) */
        if (size >= 3 && data[0] == 0x11) { data += 2; size -= 2; }
        if (size < 10 || data[0] != 0x01) return 0;
        memset(out, 0, sizeof(*out));
        out->connected = 1;
        out->left_x = stick_byte(data[1], 0);
        out->left_y = stick_byte(data[2], 1);
        out->right_x = stick_byte(data[3], 0);
        out->right_y = stick_byte(data[4], 1);
        face_hat = data[5];
        shoulder = data[6];
        system = data[7];
        left_trigger = trigger_byte(data[8], (shoulder & 0x04) != 0);
        right_trigger = trigger_byte(data[9], (shoulder & 0x08) != 0);
    } else if (kind == MAC_PAD_REPORT_DUALSENSE) {
        if (size < 11 || data[0] != 0x01) return 0;
        memset(out, 0, sizeof(*out));
        out->connected = 1;
        out->left_x = stick_byte(data[1], 0);
        out->left_y = stick_byte(data[2], 1);
        out->right_x = stick_byte(data[3], 0);
        out->right_y = stick_byte(data[4], 1);
        face_hat = data[8];
        shoulder = data[9];
        system = data[10];
        left_trigger = trigger_byte(data[5], (shoulder & 0x04) != 0);
        right_trigger = trigger_byte(data[6], (shoulder & 0x08) != 0);
    } else
        return 0;
    out->buttons |= hat_nibble((unsigned char)(face_hat & 0x0f));
    face_buttons((unsigned char)(face_hat & 0xf0), shoulder, out);
    out->analog[MAC_GAMEPAD_LEFT_TRIGGER] = left_trigger;
    out->analog[MAC_GAMEPAD_RIGHT_TRIGGER] = right_trigger;
    if (extra) {
        if (system & 0x01) *extra |= MAC_PAD_EXTRA_PS;
        if (system & 0x02) *extra |= MAC_PAD_EXTRA_TOUCHPAD;
        if (kind == MAC_PAD_REPORT_DUALSENSE && (system & 0x04)) *extra |= MAC_PAD_EXTRA_MUTE;
    }
    return 1;
}

unsigned mac_gamepad_rumble_report(int kind, unsigned char strong, unsigned char weak,
    unsigned char *out, unsigned size)
{
    unsigned length;
    if (kind == MAC_PAD_REPORT_DUALSHOCK4) length = 32;
    else if (kind == MAC_PAD_REPORT_DUALSENSE) length = 63;
    else return 0;
    if (!out || size < length) return 0;
    memset(out, 0, length);
    if (kind == MAC_PAD_REPORT_DUALSHOCK4) {
        out[0] = 0x05;
        out[1] = 0x01;
        out[4] = weak;
        out[5] = strong;
    } else {
        out[0] = 0x02;
        out[1] = 0x03;
        out[3] = weak;
        out[4] = strong;
    }
    return length;
}
