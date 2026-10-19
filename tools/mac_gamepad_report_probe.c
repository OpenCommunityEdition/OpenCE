/* Checks the Sony pads' report decoding (port/mac/mac_gamepad_report.c) with
   reports built from the documented layouts, and against the HID-element
   mapping (mac_gamepad_map) of the same states. Pure: no pad is needed. */
#include "mac_gamepad_report.h"
#include <stdio.h>
#include <string.h>

static int checks, failures;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static void ds4(unsigned char *r, unsigned char lx, unsigned char ly, unsigned char rx, unsigned char ry,
    unsigned char face_hat, unsigned char shoulder, unsigned char system, unsigned char l2, unsigned char r2)
{
    memset(r, 0, 64);
    r[0] = 1; r[1] = lx; r[2] = ly; r[3] = rx; r[4] = ry; r[5] = face_hat; r[6] = shoulder; r[7] = system;
    r[8] = l2; r[9] = r2;
    r[10] = 0xff; r[11] = 0xff; /* (a timestamp and the sensors follow: never read) */
    r[12] = 0x80; r[30] = 0x1b;
}

static void sense(unsigned char *r, unsigned char lx, unsigned char ly, unsigned char rx, unsigned char ry,
    unsigned char l2, unsigned char r2, unsigned char face_hat, unsigned char shoulder, unsigned char system)
{
    memset(r, 0, 64);
    r[0] = 1; r[1] = lx; r[2] = ly; r[3] = rx; r[4] = ry; r[5] = l2; r[6] = r2; r[7] = 0x42;
    r[8] = face_hat; r[9] = shoulder; r[10] = system; r[11] = 0;
    r[12] = 0x9a; r[13] = 0xbc; r[40] = 0xff;
}

static short stick_ref(unsigned char value, int flip)
{
    int scaled = (int)value * 257 - 32768;
    if (flip) scaled = -scaled - 1;
    return (short)scaled;
}

static int same(const struct mac_gamepad_frame *a, const struct mac_gamepad_frame *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

int main(void)
{
    unsigned char r[78];
    struct mac_gamepad_frame f, g;
    unsigned extra, hat;

    CHECK(mac_gamepad_report_kind(0x054c, 0x05c4) == MAC_PAD_REPORT_DUALSHOCK4);
    CHECK(mac_gamepad_report_kind(0x054c, 0x09cc) == MAC_PAD_REPORT_DUALSHOCK4);
    CHECK(mac_gamepad_report_kind(0x054c, 0x0ce6) == MAC_PAD_REPORT_DUALSENSE);
    CHECK(mac_gamepad_report_kind(0x054c, 0x0268) == MAC_PAD_REPORT_NONE);
    CHECK(mac_gamepad_report_kind(0x045e, 0x028e) == MAC_PAD_REPORT_NONE);
    CHECK(!strcmp(mac_gamepad_report_name(MAC_PAD_REPORT_DUALSENSE), "DualSense"));

    /* DualShock 4 idle: sticks near the middle, no hat, nothing pressed */
    ds4(r, 128, 128, 128, 128, 0x08, 0, 0, 0, 0);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, &extra) == 1);
    CHECK(f.connected && f.buttons == 0 && extra == 0);
    CHECK(f.left_x == 128 && f.left_y == -129 && f.right_x == 128 && f.right_y == -129);
    CHECK(f.analog[MAC_GAMEPAD_A] == 0 && f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 0);

    /* sticks at the ends: left and up are 0; the game's y is up positive */
    ds4(r, 0, 0, 255, 255, 0x08, 0, 0, 0, 0);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL) == 1);
    CHECK(f.left_x == -32768 && f.left_y == 32767 && f.right_x == 32767 && f.right_y == -32768);

    /* the hat: eight positions clockwise from north, 8 and up none */
    for (hat = 0; hat < 16; ++hat) {
        static const unsigned want[16] = {
            MAC_GAMEPAD_DPAD_UP, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_RIGHT,
            MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_DOWN,
            MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_LEFT, MAC_GAMEPAD_DPAD_LEFT,
            MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_LEFT, 0, 0, 0, 0, 0, 0, 0, 0 };
        ds4(r, 128, 128, 128, 128, (unsigned char)hat, 0, 0, 0, 0);
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL) == 1);
        CHECK(f.buttons == want[hat]);
    }

    /* face buttons: cross A, circle B, square X, triangle Y */
    ds4(r, 128, 128, 128, 128, 0x28, 0, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_A] == 255 && !f.analog[MAC_GAMEPAD_B] && !f.analog[MAC_GAMEPAD_X] && !f.analog[MAC_GAMEPAD_Y]);
    ds4(r, 128, 128, 128, 128, 0x48, 0, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_B] == 255 && !f.analog[MAC_GAMEPAD_A]);
    ds4(r, 128, 128, 128, 128, 0x18, 0, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_X] == 255 && !f.analog[MAC_GAMEPAD_A]);
    ds4(r, 128, 128, 128, 128, 0x88, 0, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_Y] == 255 && !f.analog[MAC_GAMEPAD_A]);

    /* L1/R1 as white/black, share back, options start, L3, R3 */
    ds4(r, 128, 128, 128, 128, 0x08, 0x01, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_WHITE] == 255 && !f.analog[MAC_GAMEPAD_BLACK] && f.buttons == 0);
    ds4(r, 128, 128, 128, 128, 0x08, 0x02, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_BLACK] == 255 && !f.analog[MAC_GAMEPAD_WHITE]);
    ds4(r, 128, 128, 128, 128, 0x08, 0x10, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.buttons == MAC_GAMEPAD_BACK);
    ds4(r, 128, 128, 128, 128, 0x08, 0x20, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.buttons == MAC_GAMEPAD_START);
    ds4(r, 128, 128, 128, 128, 0x08, 0xc0, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.buttons == (MAC_GAMEPAD_LEFT_THUMB | MAC_GAMEPAD_RIGHT_THUMB));

    /* triggers: the analog bytes; a button alone is a full pull */
    ds4(r, 128, 128, 128, 128, 0x08, 0x04, 0, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 255 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 0);
    ds4(r, 128, 128, 128, 128, 0x08, 0x0c, 0, 100, 200);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
    CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 100 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 200);

    /* PS and the touchpad click are beside the frame */
    ds4(r, 128, 128, 128, 128, 0x08, 0, 0xfd, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, &extra);
    CHECK(extra == MAC_PAD_EXTRA_PS && f.buttons == 0);
    ds4(r, 128, 128, 128, 128, 0x08, 0, 0x02, 0, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, &extra);
    CHECK(extra == MAC_PAD_EXTRA_TOUCHPAD);

    /* Bluetooth's short report 0x01 (10 bytes) and full report 0x11 (two more header bytes) */
    ds4(r, 10, 20, 30, 40, 0x24, 0x10, 0, 7, 9);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 10, &f, NULL) == 1);
    CHECK(f.left_x == 10 * 257 - 32768 && f.analog[MAC_GAMEPAD_A] == 255 && f.buttons == (MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_BACK));
    memset(r, 0, sizeof(r));
    r[0] = 0x11; r[1] = 0xc0; r[2] = 0x00;
    ds4(r + 2, 10, 20, 30, 40, 0x24, 0x10, 0, 7, 9);
    r[2] = 0x01;
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 78, &f, NULL) == 1);
    CHECK(f.left_x == 10 * 257 - 32768 && f.analog[MAC_GAMEPAD_A] == 255 && (f.buttons & MAC_GAMEPAD_BACK));
    CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 7 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 9);

    /* not the state: another report id, too short, no data, a wrong kind */
    ds4(r, 128, 128, 128, 128, 0x08, 0, 0, 0, 0);
    r[0] = 0x05;
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL) == 0);
    r[0] = 0x01;
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 9, &f, NULL) == 0);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, NULL, 64, &f, NULL) == 0);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_NONE, r, 64, &f, NULL) == 0);
    /* (10 bytes is Bluetooth's simple report; 9 is too short) */
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 9, &f, NULL) == 0);

    /* DualSense: triggers at 5 and 6, buttons from 8 */
    sense(r, 128, 128, 128, 128, 0, 0, 0x08, 0, 0);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 64, &f, &extra) == 1);
    CHECK(f.connected && f.buttons == 0 && extra == 0 && f.left_x == 128 && f.left_y == -129);
    sense(r, 0, 255, 64, 192, 77, 255, 0x62, 0x33, 0x07);
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 64, &f, &extra) == 1);
    CHECK(f.left_x == -32768 && f.left_y == -32768 && f.right_x == 64 * 257 - 32768 && f.right_y == -(192 * 257 - 32768) - 1);
    CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 77 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 255);
    CHECK(f.analog[MAC_GAMEPAD_A] == 255 && f.analog[MAC_GAMEPAD_B] == 255 && !f.analog[MAC_GAMEPAD_X] && !f.analog[MAC_GAMEPAD_Y]);
    CHECK(f.buttons == (MAC_GAMEPAD_DPAD_RIGHT | MAC_GAMEPAD_BACK | MAC_GAMEPAD_START));
    CHECK(f.analog[MAC_GAMEPAD_WHITE] == 255 && f.analog[MAC_GAMEPAD_BLACK] == 255);
    CHECK(extra == (MAC_PAD_EXTRA_PS | MAC_PAD_EXTRA_TOUCHPAD | MAC_PAD_EXTRA_MUTE));
    sense(r, 128, 128, 128, 128, 0, 0, 0x08, 0xc8, 0);
    mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 64, &f, NULL);
    CHECK(f.buttons == (MAC_GAMEPAD_LEFT_THUMB | MAC_GAMEPAD_RIGHT_THUMB) && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 255);

    /* the element mapping (what the game does for a pad it reads by HID usage,
       and for a Sony pad whose reports it cannot use) agrees with the reports */
    {
        /* the DualShock 4's elements as the 8BitDo receiver in that mode lists them (c2d, 10.6):
           buttons 1 to 14, X Y Z Rz, the hat 0..7, Rx Ry */
        struct mac_gamepad_element e[21];
        long v[21];
        unsigned i;
        for (i = 0; i < 14; ++i) { e[i].usage_page = 9; e[i].usage = i + 1; e[i].minimum = 0; e[i].maximum = 1; }
        e[14].usage_page = 1; e[14].usage = 0x30; e[15].usage_page = 1; e[15].usage = 0x31;
        e[16].usage_page = 1; e[16].usage = 0x32; e[17].usage_page = 1; e[17].usage = 0x35;
        e[18].usage_page = 1; e[18].usage = 0x39;
        e[19].usage_page = 1; e[19].usage = 0x33; e[20].usage_page = 1; e[20].usage = 0x34;
        for (i = 14; i < 21; ++i) { e[i].minimum = 0; e[i].maximum = 255; }
        e[18].maximum = 7;
        memset(v, 0, sizeof(v));
        /* the idle values read from that receiver: sticks 127, hat 8, nothing pressed */
        v[14] = 127; v[15] = 127; v[16] = 127; v[17] = 127; v[18] = 8;
        mac_gamepad_map(e, v, 21, 0x054c, 0x05c4, &g);
        CHECK(g.connected && g.buttons == 0 && g.analog[0] == 0 && g.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 0);
        CHECK(g.left_x == 127 * 257 - 32768 && g.right_y == -(127 * 257 - 32768) - 1);
        /* a mixed state, by report and by elements */
        ds4(r, 20, 220, 90, 160, 0xd5, 0xf3, 0x01, 33, 244);
        mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &f, NULL);
        /* face 0xd: square, circle, triangle; shoulder 0xf3: L1 R1 share options L3 R3; hat 5 */
        v[0] = 1; v[1] = 0; v[2] = 1; v[3] = 1;
        v[4] = 1; v[5] = 1; v[6] = 0; v[7] = 0; v[8] = 1; v[9] = 1; v[10] = 1; v[11] = 1; v[12] = 0; v[13] = 0;
        v[14] = 20; v[15] = 220; v[16] = 90; v[17] = 160; v[18] = 5; v[19] = 33; v[20] = 244;
        mac_gamepad_map(e, v, 21, 0x054c, 0x05c4, &g);
        CHECK(same(&f, &g));
        if (!same(&f, &g)) printf("report: %d %d %d %d buttons %x  elements: %d %d %d %d buttons %x\n", f.left_x, f.left_y, f.right_x, f.right_y, f.buttons, g.left_x, g.left_y, g.right_x, g.right_y, g.buttons);
    }

    /* the rumble output reports (bytes as the pads take them over USB) */
    {
        unsigned char o[MAC_PAD_RUMBLE_REPORT_MAXIMUM];
        unsigned i, n, others;
        n = mac_gamepad_rumble_report(MAC_PAD_REPORT_DUALSHOCK4, 200, 50, o, sizeof(o));
        CHECK(n == 32 && o[0] == 0x05 && o[1] == 0x01 && o[4] == 50 && o[5] == 200);
        for (others = 0, i = 0; i < n; ++i) if (i != 0 && i != 1 && i != 4 && i != 5 && o[i]) ++others;
        CHECK(others == 0);
        n = mac_gamepad_rumble_report(MAC_PAD_REPORT_DUALSENSE, 255, 7, o, sizeof(o));
        CHECK(n == 63 && o[0] == 0x02 && o[1] == 0x03 && o[2] == 0 && o[3] == 7 && o[4] == 255);
        for (others = 0, i = 5; i < n; ++i) if (o[i]) ++others;
        CHECK(others == 0);
        CHECK(mac_gamepad_rumble_report(MAC_PAD_REPORT_NONE, 1, 1, o, sizeof(o)) == 0);
        CHECK(mac_gamepad_rumble_report(MAC_PAD_REPORT_DUALSENSE, 1, 1, o, 62) == 0);
        CHECK(mac_gamepad_rumble_report(MAC_PAD_REPORT_DUALSHOCK4, 0, 0, o, 32) == 32 && o[4] == 0 && o[5] == 0);
    }

    /* 8BitDo's Wireless USB Adapter 2 in Dinput mode (2dc8:3105), the reports as it sent them with a
       DualSense paired (MacBook6,1, 10.6.8: ds4watch's log), and the same states by its HID elements */
    {
        static const struct { const char *text; unsigned char r[10]; } real[] = {
            { "idle", { 0x01, 0x00, 0x00, 0x0f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00 } },
            { "button 7 with the left stick", { 0x01, 0x40, 0x00, 0x0f, 0x07, 0xdc, 0x7f, 0x7f, 0x00, 0x00 } },
            { "button 4 and hat east", { 0x01, 0x08, 0x00, 0x02, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00 } },
            { "button 10 with L2 analog", { 0x01, 0x00, 0x02, 0x0f, 0x7f, 0x7f, 0x7f, 0x7f, 0xff, 0x00 } },
            { "L3 and R3 with sticks", { 0x01, 0x00, 0x60, 0x0f, 0xb7, 0xa5, 0x7f, 0x7f, 0x00, 0x00 } },
            { "touchpad click (byte 3 bit 4)", { 0x01, 0x00, 0x00, 0x1f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00 } }
        };
        unsigned char b[10];
        struct mac_gamepad_element e[22];
        long v[22];
        unsigned i, n;
        CHECK(mac_gamepad_report_kind(0x2dc8, 0x3105) == MAC_PAD_REPORT_8BITDO_DINPUT);
        CHECK(mac_gamepad_report_kind(0x2dc8, 0x3107) == MAC_PAD_REPORT_NONE);
        CHECK(!strcmp(mac_gamepad_report_name(MAC_PAD_REPORT_8BITDO_DINPUT), "8BitDo Dinput"));
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[0].r, 10, &f, NULL) == 1);
        CHECK(f.connected && f.buttons == 0 && f.left_x == 127 * 257 - 32768 && f.left_y == -(127 * 257 - 32768) - 1);
        CHECK(f.right_x == f.left_x && f.right_y == f.left_y && f.analog[MAC_GAMEPAD_A] == 0);
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[1].r, 10, &f, NULL);
        CHECK(f.analog[MAC_GAMEPAD_WHITE] == 255 && !f.analog[MAC_GAMEPAD_BLACK] && f.buttons == 0);
        CHECK(f.left_x == 7 * 257 - 32768 && f.left_y == -(0xdc * 257 - 32768) - 1);
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[2].r, 10, &f, NULL);
        CHECK(f.analog[MAC_GAMEPAD_X] == 255 && f.buttons == MAC_GAMEPAD_DPAD_RIGHT);
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[3].r, 10, &f, NULL);
        CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 255 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 0 && f.buttons == 0);
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[4].r, 10, &f, NULL);
        CHECK(f.buttons == (MAC_GAMEPAD_LEFT_THUMB | MAC_GAMEPAD_RIGHT_THUMB));
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[5].r, 10, &f, &n);
        CHECK(f.buttons == 0 && n == MAC_PAD_EXTRA_TOUCHPAD);
        /* each button alone, in the DualSense's terms; the digital L2 and R2 are full pulls */
        for (i = 1; i <= 15; ++i) {
            static const int analog_of[16] = { -1, MAC_GAMEPAD_A, MAC_GAMEPAD_B, -1, MAC_GAMEPAD_X, MAC_GAMEPAD_Y, -1,
                MAC_GAMEPAD_WHITE, MAC_GAMEPAD_BLACK, MAC_GAMEPAD_RIGHT_TRIGGER, MAC_GAMEPAD_LEFT_TRIGGER, -1, -1, -1, -1, -1 };
            static const unsigned bits_of[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, MAC_GAMEPAD_BACK, MAC_GAMEPAD_START, 0,
                MAC_GAMEPAD_LEFT_THUMB, MAC_GAMEPAD_RIGHT_THUMB };
            memcpy(b, real[0].r, 10);
            b[1 + (i - 1) / 8] = (unsigned char)(1 << ((i - 1) % 8));
            mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, b, 10, &f, NULL);
            n = 0;
            if (analog_of[i] >= 0) { CHECK(f.analog[analog_of[i]] == 255); ++n; }
            CHECK(f.buttons == bits_of[i]);
            for (hat = 0; hat < MAC_GAMEPAD_ANALOG_COUNT; ++hat) if (f.analog[hat]) --n;
            CHECK(n == 0);
        }
        /* the hat, and a short or another report (the receiver's report 2 is vendor-defined) */
        for (hat = 0; hat < 16; ++hat) {
            memcpy(b, real[0].r, 10);
            b[3] = (unsigned char)hat;
            mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, b, 10, &f, NULL);
            CHECK(f.buttons == (hat < 8 ? ((unsigned []){ MAC_GAMEPAD_DPAD_UP, MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_RIGHT,
                MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_RIGHT, MAC_GAMEPAD_DPAD_DOWN,
                MAC_GAMEPAD_DPAD_DOWN | MAC_GAMEPAD_DPAD_LEFT, MAC_GAMEPAD_DPAD_LEFT,
                MAC_GAMEPAD_DPAD_UP | MAC_GAMEPAD_DPAD_LEFT })[hat] : 0));
        }
        memcpy(b, real[0].r, 10);
        b[0] = 2;
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, b, 10, &f, NULL) == 0);
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, real[0].r, 9, &f, NULL) == 0);
        /* the analog triggers: R2 is byte 8 (Accelerator), L2 byte 9 (Brake) */
        memcpy(b, real[0].r, 10);
        b[8] = 90; b[9] = 200;
        mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, b, 10, &f, NULL);
        CHECK(f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 90 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 200);
        /* the elements the receiver lists (buttons 1 to 15, the hat, X Y Z Rz, Accelerator and Brake) give
           the same states as its reports */
        for (i = 0; i < 15; ++i) { e[i].usage_page = 9; e[i].usage = i + 1; e[i].minimum = 0; e[i].maximum = 1; }
        e[15].usage_page = 1; e[15].usage = 0x39; e[15].minimum = 0; e[15].maximum = 7;
        e[16].usage_page = 1; e[16].usage = 0x30; e[17].usage_page = 1; e[17].usage = 0x31;
        e[18].usage_page = 1; e[18].usage = 0x32; e[19].usage_page = 1; e[19].usage = 0x35;
        e[20].usage_page = 2; e[20].usage = 0xc4; e[21].usage_page = 2; e[21].usage = 0xc5;
        for (i = 16; i < 22; ++i) { e[i].minimum = 0; e[i].maximum = 255; }
        for (n = 0; n < sizeof(real) / sizeof(real[0]) + 2; ++n) {
            const unsigned char *rr = n < sizeof(real) / sizeof(real[0]) ? real[n].r : b;
            unsigned char mixed[10];
            if (n == sizeof(real) / sizeof(real[0])) { memcpy(mixed, real[0].r, 10); mixed[1] = 0xff; mixed[2] = 0x7f; mixed[3] = 0x05;
                mixed[4] = 3; mixed[5] = 250; mixed[6] = 99; mixed[7] = 180; mixed[8] = 60; mixed[9] = 30; rr = mixed; }
            for (i = 0; i < 15; ++i) v[i] = (rr[1 + i / 8] >> (i % 8)) & 1;
            v[15] = rr[3] & 0x0f;
            for (i = 0; i < 4; ++i) v[16 + i] = rr[4 + i];
            v[20] = rr[8]; v[21] = rr[9];
            mac_gamepad_decode_report(MAC_PAD_REPORT_8BITDO_DINPUT, rr, 10, &f, NULL);
            mac_gamepad_map(e, v, 22, 0x2dc8, 0x3105, &g);
            CHECK(same(&f, &g));
            if (!same(&f, &g)) printf("8BitDo case %u differs: buttons %x %x, A %d %d\n", n, f.buttons, g.buttons, f.analog[0], g.analog[0]);
        }
        /* the PlayStation Classic pad by elements (SDL's database): cross A, circle B, square X, triangle Y,
           L1 and R1, select and start, L2 and R2 full pulls, the d-pad on X and Y */
        {
            struct mac_gamepad_element pe[12];
            long pv[12];
            for (i = 0; i < 10; ++i) { pe[i].usage_page = 9; pe[i].usage = i + 1; pe[i].minimum = 0; pe[i].maximum = 1; pv[i] = 0; }
            pe[10].usage_page = 1; pe[10].usage = 0x30; pe[11].usage_page = 1; pe[11].usage = 0x31;
            pe[10].minimum = pe[11].minimum = 0; pe[10].maximum = pe[11].maximum = 255;
            pv[10] = 127; pv[11] = 127;
            mac_gamepad_map(pe, pv, 12, 0x054c, 0x0cda, &g);
            CHECK(g.connected && g.buttons == 0 && g.left_x == 0 && g.left_y == 0 && !g.analog[0]);
            pv[2] = 1; pv[1] = 1; pv[3] = 1; pv[0] = 1; pv[4] = 1; pv[5] = 1; pv[6] = 1; pv[7] = 1; pv[8] = 1; pv[9] = 1;
            pv[10] = 255; pv[11] = 0;
            mac_gamepad_map(pe, pv, 12, 0x054c, 0x0cda, &g);
            CHECK(g.analog[MAC_GAMEPAD_A] == 255 && g.analog[MAC_GAMEPAD_B] == 255 && g.analog[MAC_GAMEPAD_X] == 255 && g.analog[MAC_GAMEPAD_Y] == 255);
            CHECK(g.analog[MAC_GAMEPAD_WHITE] == 255 && g.analog[MAC_GAMEPAD_BLACK] == 255);
            CHECK(g.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 255 && g.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 255);
            CHECK(g.buttons == (MAC_GAMEPAD_BACK | MAC_GAMEPAD_START | MAC_GAMEPAD_DPAD_RIGHT | MAC_GAMEPAD_DPAD_UP));
        }
    }

    /* Bluetooth (the pad paired in the system's menu, read through IOHID) */
    {
        /* a DualSense's first full report, as the own stack logged it (pair2.log), without the 0xA1 header */
        static const unsigned char real[78] = {
            0x31,0x31,0x72,0x76,0x7f,0x75,0x00,0x00,0x01,0x08,0x00,0x00,0x00,0xea,0x45,0xed,
            0x83,0x13,0x00,0xe9,0xff,0x02,0x00,0xa4,0x02,0x21,0x1e,0xc9,0x06,0x14,0xee,0x07,
            0x0b,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x09,0x09,0x00,0x00,0x00,
            0x00,0x00,0x0d,0xfd,0x07,0x0b,0x07,0x00,0x00,0xb6,0xb1,0x54,0x3c,0xdf,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        unsigned char b[78];
        struct mac_pad_output o;
        unsigned char out[78];
        unsigned crc;
        CHECK(mac_pad_crc32_report(0xa1, real, 4) != 0);
        /* the full report 0x31: sticks at 2..5, nothing pressed (hat 8) */
        memcpy(b, real, sizeof(b));
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 78, &f, &extra) == 1);
        CHECK(f.connected && f.buttons == 0 && f.left_x == stick_ref(0x72, 0) && f.left_y == stick_ref(0x76, 1));
        CHECK(f.right_x == stick_ref(0x7f, 0) && f.right_y == stick_ref(0x75, 1) && extra == 0);
        /* the same state as the USB report 0x01 decodes */
        sense(r, 0x72, 0x76, 0x7f, 0x75, 0, 0, 0x08, 0, 0);
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 64, &g, NULL) == 1 && same(&f, &g));
        /* buttons: cross and R1 and L2 (analog 200), d-pad right, options, touchpad click and PS */
        b[6] = 200; b[7] = 0;
        b[9] = 0x22; /* hat 2 (east), cross */
        b[10] = 0x02 | 0x20; /* R1, options */
        b[11] = 0x03; /* PS, touchpad click */
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 78, &f, &extra) == 1);
        CHECK((f.buttons & MAC_GAMEPAD_DPAD_RIGHT) && f.analog[MAC_GAMEPAD_A] == 255 && f.analog[MAC_GAMEPAD_BLACK] == 255);
        CHECK((f.buttons & MAC_GAMEPAD_START) && f.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 200 && extra == (MAC_PAD_EXTRA_PS | MAC_PAD_EXTRA_TOUCHPAD));
        /* the full report that is too short or of another id is not state */
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 11, &f, NULL) == 0);
        b[0] = 0x32;
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 78, &f, NULL) == 0);
        /* the simple report (10 bytes): DualShock 4 layout, from a DualSense too */
        memset(b, 0, sizeof(b));
        b[0] = 1; b[1] = 0xff; b[2] = 0x00; b[3] = 0x80; b[4] = 0x80; b[5] = 0x08 | 0x40; b[6] = 0x01; b[7] = 0x01; b[8] = 0; b[9] = 77;
        CHECK(mac_gamepad_report_is_simple(MAC_PAD_REPORT_DUALSENSE, b, 10));
        CHECK(!mac_gamepad_report_is_simple(MAC_PAD_REPORT_DUALSENSE, real, 78));
        CHECK(!mac_gamepad_report_is_simple(MAC_PAD_REPORT_DUALSENSE, r, 64));
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 10, &f, &extra) == 1);
        CHECK(f.left_x == stick_ref(0xff, 0) && f.left_y == stick_ref(0, 1) && f.analog[MAC_GAMEPAD_B] == 255);
        CHECK(f.analog[MAC_GAMEPAD_WHITE] == 255 && f.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 77 && extra == MAC_PAD_EXTRA_PS);
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, b, 9, &f, NULL) == 0);
        /* battery: the real report's byte 54 is 0x07 (level 7: 75 percent); other reports have none */
        CHECK(mac_gamepad_report_battery(MAC_PAD_REPORT_DUALSENSE, real, 78) == 75);
        CHECK(mac_gamepad_report_battery(MAC_PAD_REPORT_DUALSENSE, r, 64) == -1);
        CHECK(mac_gamepad_report_battery(MAC_PAD_REPORT_DUALSENSE, b, 10) == -1);
        /* a DualShock 4's simple report decodes as it does on USB */
        CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, b, 10, &f, NULL) == 1 && f.analog[MAC_GAMEPAD_B] == 255);
        /* output: the CRC-32 check value, the rumble report and the setup report */
        CHECK(~mac_pad_crc32_update(0xffffffffu, (const unsigned char *)"123456789", 9) == 0xcbf43926u);
        memset(&o, 0, sizeof(o));
        o.set_rumble = 1; o.strong = 90; o.weak = 40; o.set_light = 1; o.red = 255; o.blue = 128; o.set_players = 1; o.player_leds = 4;
        CHECK(mac_pad_build_bluetooth_output(MAC_PAD_REPORT_DUALSENSE, 5, &o, out) == 78);
        CHECK(out[0] == 0x31 && out[1] == 0x50 && out[2] == 0x10 && out[3] == 0x03 && out[4] == 0x14);
        CHECK(out[5] == 40 && out[6] == 90 && out[46] == 4 && out[47] == 255 && out[48] == 0 && out[49] == 128);
        crc = (unsigned)out[74] | ((unsigned)out[75] << 8) | ((unsigned)out[76] << 16) | ((unsigned)out[77] << 24);
        CHECK(crc == mac_pad_crc32_report(0xa2, out, 74));
        o.fade_out_setup = 1;
        CHECK(mac_pad_build_bluetooth_output(MAC_PAD_REPORT_DUALSENSE, 6, &o, out) == 78);
        CHECK(out[3 + 38] == 0x02 && out[3 + 41] == 0x02 && out[4] == 0 && out[0 + 1] == 0x60);
        o.fade_out_setup = 0;
        CHECK(mac_pad_build_bluetooth_output(MAC_PAD_REPORT_DUALSHOCK4, 0, &o, out) == 78);
        CHECK(out[0] == 0x11 && out[1] == 0xc4 && out[3] == 0x03 && out[6] == 40 && out[7] == 90 && out[8] == 255);
        CHECK(mac_pad_build_bluetooth_output(MAC_PAD_REPORT_8BITDO_DINPUT, 0, &o, out) == 0);
        /* a vector worked out by hand elsewhere: the DualSense report of the own stack's self test (bt_selftest.c) agrees
           because bt_pad.c builds it with this function */
    }

    /* the silence timeout: only the streaming kind, only past a second */
    CHECK(!mac_gamepad_stream_silent(MAC_PAD_REPORT_8BITDO_DINPUT, 0.0));
    CHECK(!mac_gamepad_stream_silent(MAC_PAD_REPORT_8BITDO_DINPUT, 1.0));
    CHECK(mac_gamepad_stream_silent(MAC_PAD_REPORT_8BITDO_DINPUT, 1.01));
    CHECK(mac_gamepad_stream_silent(MAC_PAD_REPORT_8BITDO_DINPUT, 600.0));
    CHECK(!mac_gamepad_stream_silent(MAC_PAD_REPORT_DUALSHOCK4, 600.0));
    CHECK(!mac_gamepad_stream_silent(MAC_PAD_REPORT_DUALSENSE, 600.0));
    CHECK(!mac_gamepad_stream_silent(MAC_PAD_REPORT_NONE, 600.0));
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
