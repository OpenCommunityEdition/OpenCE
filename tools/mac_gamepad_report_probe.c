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
    CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSENSE, r, 10, &f, NULL) == 0);

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

    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
