/* The Bluetooth stack's headless checks (bt_selftest.h). */
#include "bt_selftest.h"
#include "bt_stack.h"
#include "bt_session.h"

#ifdef __APPLE__
#include "bt_usb.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) failures++;
}

static void test_log(void *ctx, const char *line)
{
    (void)ctx;
    printf("      %s\n", line);
}

static void run(struct bt_stack *st, double seconds)
{
    double end = bt_now() + seconds;
    while (bt_now() < end) {
        bt_stack_tick(st);
        bt_stack_pump(st, 0.005);
    }
}

static void test_basics(void)
{
    static const unsigned char digits[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    unsigned char r[78], addr[6];
    struct bt_pad_output o;
    char text[20];
    unsigned crc;
    unsigned char *image = NULL;
    unsigned char fw[700];
    unsigned n;

    crc = ~bt_crc32_update(0xffffffffu, digits, 9);
    check(crc == 0xcbf43926u, "CRC-32 of 123456789");

    memset(&o, 0, sizeof(o));
    o.red = 255; o.green = 0; o.blue = 128; o.player_leds = 0x04; o.strong = 90; o.weak = 40;
    o.set_light = o.set_players = o.set_rumble = 1;
    check(bt_pad_build_dualsense(r, 5, &o) == 78, "DualSense output report is 78 bytes");
    check(r[0] == 0x31 && r[1] == 0x50 && r[2] == 0x10, "DualSense output: id 0x31, sequence 5 in the high nibble, tag 0x10");
    check(r[3] == 0x03 && r[4] == 0x14, "DualSense output: valid flags 0 = 0x03, flags 1 = 0x04 | 0x10");
    check(r[5] == 40 && r[6] == 90, "DualSense output: right (weak) motor, left (strong) motor");
    check(r[3 + 44] == 255 && r[3 + 45] == 0 && r[3 + 46] == 128, "DualSense output: light bar colour at common[44..46]");
    check(r[3 + 43] == 0x04, "DualSense output: player lights at common[43]");
    check(bt_get32(r + 74) == bt_crc32_report(0xa2, r, 74), "DualSense output: CRC-32 over 0xA2 and bytes 0..73");
    o.fade_out_setup = 1;
    bt_pad_build_dualsense(r, 6, &o);
    check(r[3 + 38] == 0x02 && r[3 + 41] == 0x02 && r[3 + 44] == 0 && r[4] == 0,
        "DualSense setup report: light bar setup flag 2 and fade-out 2, no colour");
    check(bt_get32(r + 74) == bt_crc32_report(0xa2, r, 74), "DualSense setup report: CRC-32");
    bt_pad_build_dualshock4(r, 0, &o);
    check(r[0] == 0x11 && r[1] == 0xc4 && bt_get32(r + 74) == bt_crc32_report(0xa2, r, 74), "DualShock 4 output report: id, flags, CRC-32");

    check(bt_addr_parse("4c:b9:9b:fd:1d:81", addr) == 0 && addr[0] == 0x81 && addr[5] == 0x4c, "address parsing keeps wire order");
    bt_addr_text(addr, text);
    check(strcmp(text, "4c:b9:9b:fd:1d:81") == 0, "address printing");

    /* a firmware file with one patch for ROM version 1 (chip id 2) */
    memset(fw, 0, sizeof(fw));
    memcpy(fw, "Realtech", 8);
    bt_put32(fw + 8, 0xdfc6d922u);
    bt_put16(fw + 12, 1);
    bt_put16(fw + 14, 2);
    bt_put16(fw + 16, 600);
    bt_put32(fw + 18, 22);
    memset(fw + 22, 0x55, 600);
    n = bt_realtek_image(fw, 622, NULL, 0, 1, &image);
    check(n == 606 && image && bt_get32(image + 596) == 0xdfc6d922u && image[600] == 0x55 && image[601] == 0xab,
        "Realtek image: patch, version word in place of its last 4 bytes, default config");
    free(image);
    check(bt_realtek_image(fw, 622, NULL, 0, 5, &image) == 0, "Realtek image: no patch for another ROM");
    check(bt_realtek_image(fw, 100, NULL, 0, 1, &image) == 0, "Realtek image: truncated file refused");
}

static void test_keys(void)
{
    char path[64] = "/tmp/bt_keys_test.XXXXXX";
    int fd = mkstemp(path);
    unsigned char local[6], other[6];
    struct bt_paired k, out[4];
    int i;
    if (fd >= 0) close(fd);
    bt_addr_parse("20:e1:5d:8d:15:d8", local);
    bt_addr_parse("00:11:22:33:44:55", other);
    memset(&k, 0, sizeof(k));
    bt_addr_parse("4c:b9:9b:fd:1d:81", k.addr);
    for (i = 0; i < 16; i++) k.key[i] = (unsigned char)(i * 17);
    k.key_type = 4; k.kind = BT_KIND_DUALSENSE;
    strcpy(k.name, "DualSense Wireless Controller");
    check(bt_keys_store(path, local, &k) == 0, "key store writes");
    check(bt_keys_store(path, other, &k) == 0, "key store keeps another dongle's line");
    check(bt_keys_load(path, local, out, 4) == 1 && !memcmp(out[0].key, k.key, 16) && out[0].kind == BT_KIND_DUALSENSE
        && !strcmp(out[0].name, k.name), "key store round trip");
    k.key[0] = 99;
    bt_keys_store(path, local, &k);
    check(bt_keys_load(path, local, out, 4) == 1 && out[0].key[0] == 99, "key store replaces");
    bt_keys_remove(path, local, k.addr);
    check(bt_keys_load(path, local, out, 4) == 0 && bt_keys_load(path, other, out, 4) == 1, "key store removes only this dongle's pad");
    unlink(path);
}

static void test_fake(void)
{
    struct bt_stack *st = bt_stack_create();
    struct bt_fake *f;
    char fwpath[64] = "/tmp/bt_fw_test.XXXXXX";
    char keys[64] = "/tmp/bt_keys_fake.XXXXXX";
    int fd = mkstemp(fwpath), kd = mkstemp(keys);
    unsigned char fw[700];
    unsigned char out[80];
    double t;
    unsigned char pad[6];
    struct bt_found *found;
    char text[20];

    memset(fw, 0, sizeof(fw));
    memcpy(fw, "Realtech", 8);
    bt_put32(fw + 8, 0xdfc6d922u);
    bt_put16(fw + 12, 1);
    bt_put16(fw + 14, 2);
    bt_put16(fw + 16, 600);
    bt_put32(fw + 18, 22);
    memset(fw + 22, 0x55, 600);
    if (fd >= 0) { if (write(fd, fw, 622) != 622) failures++; close(fd); }
    if (kd >= 0) close(kd);
    unlink(keys);
    st->log = test_log;
    strcpy(st->firmware_path, fwpath);
    strcpy(st->keys_path, keys);
    f = bt_fake_create(st, 1);
    bt_addr_parse("4c:b9:9b:fd:1d:81", pad);

    check(bt_stack_start(st) == 0, "fake dongle: start-up with firmware download");
    check(bt_fake_commands_seen(f, 0xfc20) == 3, "fake dongle: 606 bytes went in 3 fragments");
    check(st->firmware_loaded && st->dongle == BT_DONGLE_READY, "fake dongle: firmware loaded, ready");
    check(st->reconnect, "fake dongle: page scan on");

    bt_stack_search(st, 10.0);
    t = bt_now() + 3;
    while (bt_now() < t && !(st->found_count && st->found[0].named)) { bt_stack_tick(st); bt_stack_pump(st, 0.005); }
    check(st->found_count == 1 && st->found[0].named && st->found[0].gamepad && st->found[0].supported &&
        st->found[0].kind == BT_KIND_DUALSENSE, "search finds the DualSense by name");
    found = &st->found[0];
    bt_addr_text(found->addr, text);
    check(strcmp(text, "4c:b9:9b:fd:1d:81") == 0, "found address");

    bt_stack_pair(st, pad);
    t = bt_now() + 5;
    while (bt_now() < t && !st->pad.streaming) { bt_stack_tick(st); bt_stack_pump(st, 0.005); }
    check(st->link_up && st->link_authed && st->link_encrypted, "pairing: link up, authenticated, encrypted");
    check(st->hid_ready, "HID control and interrupt channels open");
    check(st->pad.streaming, "full input reports arrive");
    check(st->paired_count == 1 && st->paired[0].key[0] == 0xa0 && st->paired[0].kind == BT_KIND_DUALSENSE, "link key stored");
    check(!st->pairing, "pairing finished");
    run(st, 0.3);
    check(st->pad.frame.analog[MAC_GAMEPAD_RIGHT_TRIGGER] == 255 && st->pad.frame.analog[MAC_GAMEPAD_LEFT_TRIGGER] == 3,
        "analog triggers decoded");
    check(st->pad.frame.analog[MAC_GAMEPAD_A] == 255 && st->pad.frame.analog[MAC_GAMEPAD_BLACK] == 255, "cross and R1 decoded");
    check(st->pad.extra & MAC_PAD_EXTRA_TOUCHPAD, "touchpad click decoded");
    check(st->pad.touch[0].active && st->pad.touch[0].x == 272 && st->pad.touch[0].y == 519 && !st->pad.touch[1].active,
        "touchpad finger decoded");
    check(st->pad.battery_percent == 75, "battery level decoded");
    check(st->pad.crc_errors == 0, "input report CRCs match");
    check(st->pad.frame.left_x < -30000 && st->pad.frame.left_y > 0 ? 1 : st->pad.frame.left_x != 0, "stick decoded");

    {
        struct bt_pad_output o;
        int n;
        memset(&o, 0, sizeof(o));
        o.red = 255; o.green = 0; o.blue = 255; o.set_light = 1;
        bt_stack_output(st, &o);
        run(st, 0.4);
        n = bt_fake_output_report(f, out, sizeof(out));
        check(n == 79 && out[0] == 0xa2 && out[1] == 0x31, "output report reached the pad (0xA2 0x31)");
        check(n == 79 && bt_get32(out + 75) == bt_crc32_report(0xa2, out + 1, 74), "output report CRC-32 verifies");
        check(n == 79 && out[5] == 0x04 && out[4 + 44] == 255 && out[4 + 45] == 0 && out[4 + 46] == 255, "output report carries the light bar colour");
        memset(&o, 0, sizeof(o));
        o.strong = 200; o.weak = 100; o.set_rumble = 1;
        bt_stack_output(st, &o);
        run(st, 0.1);
        n = bt_fake_output_report(f, out, sizeof(out));
        check(n == 79 && out[4] == 0x03 && out[4 + 2] == 100 && out[4 + 3] == 200, "rumble report sent");
        run(st, 0.6);
        n = bt_fake_output_report(f, out, sizeof(out));
        check(n == 79 && out[4 + 2] == 0 && out[4 + 3] == 0, "rumble switched off again");
    }
    bt_stack_disconnect(st);
    run(st, 0.3);
    check(bt_fake_commands_seen(f, 0x0406) == 1, "disconnect sent");
    bt_stack_forget(st, pad);
    check(st->paired_count == 0, "forget");
    bt_stack_stop(st);
    bt_fake_destroy(f);
    bt_stack_destroy(st);
    unlink(fwpath);
    unlink(keys);
}

#ifdef __APPLE__
static void live_print(struct bt_stack *st)
{
    struct bt_pad_status *p = &st->pad;
    printf("      pad: sticks %d,%d %d,%d  L2 %d R2 %d  buttons 0x%x A%d B%d X%d Y%d  extra 0x%x  touch %d(%d,%d)  %u reports (%.0f/s), crc errors %u, simple %u\n",
        p->frame.left_x, p->frame.left_y, p->frame.right_x, p->frame.right_y,
        p->frame.analog[MAC_GAMEPAD_LEFT_TRIGGER], p->frame.analog[MAC_GAMEPAD_RIGHT_TRIGGER], p->frame.buttons,
        p->frame.analog[MAC_GAMEPAD_A], p->frame.analog[MAC_GAMEPAD_B], p->frame.analog[MAC_GAMEPAD_X], p->frame.analog[MAC_GAMEPAD_Y],
        p->extra, p->touch[0].active, p->touch[0].x, p->touch[0].y, p->reports, p->rate, p->crc_errors, p->simple_reports);
}

static int live(int argc, char **argv)
{
    struct bt_stack *st = bt_stack_create();
    struct bt_firmware_info fw;
    const char *fwdir = NULL, *keys = NULL;
    double search = 0, reconnect = 0, secs = 10;
    int do_pair = 0, do_output = 0, i, dongle = 0;
    char reason[200], datadir[512], keypath[600];
    double end, next_print;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fw") && i + 1 < argc) fwdir = argv[++i];
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc) keys = argv[++i];
        else if (!strcmp(argv[i], "--search") && i + 1 < argc) search = atof(argv[++i]);
        else if (!strcmp(argv[i], "--reconnect") && i + 1 < argc) reconnect = atof(argv[++i]);
        else if (!strcmp(argv[i], "--secs") && i + 1 < argc) secs = atof(argv[++i]);
        else if (!strcmp(argv[i], "--pair")) do_pair = 1;
        else if (!strcmp(argv[i], "--output")) do_output = 1;
    }
    st->log = test_log;
    bt_data_dir(datadir, sizeof(datadir));
    sprintf(keypath, "%s/keys.txt", datadir);
    strcpy(st->keys_path, keys ? keys : keypath);
    bt_firmware_find(fwdir, &fw);
    printf("firmware: %s\n", fw.found ? fw.firmware : "NOT FOUND");
    if (fw.found) { strcpy(st->firmware_path, fw.firmware); strcpy(st->config_path, fw.config); }
    if (bt_usb_open(&st->tr, &dongle, reason, sizeof(reason)) != 0) {
        printf("FAIL: dongle: %s (state %d)\n", reason, dongle);
        return 1;
    }
    check(bt_stack_start(st) == 0, "dongle start-up");
    printf("      %s\n      %s\n", st->chip, st->message);
    if (failures) { bt_stack_stop(st); return 1; }
    if (search > 0) {
        bt_stack_search(st, search);
        end = bt_now() + search + 1;
        while (bt_now() < end) {
            bt_stack_tick(st);
            bt_stack_pump(st, 0.01);
            if (do_pair && !st->pairing && !st->link_up) {
                for (i = 0; i < st->found_count; i++)
                    if (st->found[i].named && st->found[i].supported) {
                        printf("      pairing with %s\n", st->found[i].name);
                        bt_stack_pair(st, st->found[i].addr);
                        break;
                    }
            }
            if (st->link_up) break;
        }
        for (i = 0; i < st->found_count; i++) {
            char a[20];
            bt_addr_text(st->found[i].addr, a);
            printf("      found %s \"%s\" cod 0x%06x rssi %d gamepad %d supported %d\n", a, st->found[i].name,
                st->found[i].cod, st->found[i].rssi, st->found[i].gamepad, st->found[i].supported);
        }
    }
    if (reconnect > 0 || st->link_up) {
        end = bt_now() + (reconnect > 0 ? reconnect : 60);
        printf("      waiting for a pad (press PS) for %.0f s\n", end - bt_now());
        while (bt_now() < end && !st->pad.streaming && !st->failed) {
            bt_stack_tick(st);
            bt_stack_pump(st, 0.01);
        }
    }
    if (st->pad.streaming) {
        struct bt_pad_output o;
        check(1, "full reports streaming");
        end = bt_now() + secs;
        next_print = bt_now() + 1;
        if (do_output) {
            memset(&o, 0, sizeof(o));
            o.red = 255; o.green = 0; o.blue = 255; o.set_light = 1;
            o.player_leds = 0x0a; o.set_players = 1;
            bt_stack_output(st, &o);
        }
        while (bt_now() < end && st->link_up) {
            bt_stack_tick(st);
            bt_stack_pump(st, 0.01);
            if (do_output && bt_now() > end - secs + 3 && st->output_initial_sent == 1) {
                st->output_initial_sent = 2;
                memset(&o, 0, sizeof(o));
                o.strong = 160; o.weak = 160; o.set_rumble = 1;
                bt_stack_output(st, &o);
            }
            if (bt_now() >= next_print) { next_print = bt_now() + 1; live_print(st); }
        }
        check(st->pad.crc_errors == 0, "no CRC errors on input reports");
    } else
        printf("      no pad streamed (message: %s)\n", st->message);
    bt_stack_stop(st);
    return failures ? 1 : 0;
}
#endif

int bt_selftest_main(int argc, char **argv)
{
    int i, want_live = 0;
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--live")) want_live = 1;
    if (want_live) {
#ifdef __APPLE__
        return live(argc, argv);
#else
        printf("--live needs the Mac build\n");
        return 1;
#endif
    }
    test_basics();
    test_keys();
    test_fake();
    printf("%s (%d failure%s)\n", failures ? "FAILED" : "all checks passed", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
