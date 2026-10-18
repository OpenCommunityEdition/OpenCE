/* cc -o test_config test_config.c controller_setup_config.c controller_setup_hid.c ../mac_gamepad_report.c ../app/launcher_config.c */
#include "controller_setup_config.h"
#include "controller_setup_hid.h"
#include "../mac_gamepad_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static void write_text(const char *path, const char *text)
{
	FILE *f = fopen(path, "wb");
	fputs(text, f);
	fclose(f);
}

static void read_text(const char *path, char *buf, int size)
{
	FILE *f = fopen(path, "rb");
	int n = f ? (int)fread(buf, 1, size - 1, f) : 0;
	buf[n] = 0;
	if (f)
		fclose(f);
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/tmp/cs_test_config.toml";
	char buf[2048], val[64];
	cs_pad_state p;

	/* a missing file is created, with the keys in their [input] section */
	remove(path);
	CHECK(cs_config_save_calibration(path, 9000, 12000, 1) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(buf, "[input]\n") == buf);
	CHECK(strstr(buf, "gamepad_left_dead_zone = 9000\n") != NULL);
	CHECK(strstr(buf, "gamepad_invert_look = true\n") != NULL);
	CHECK(strstr(buf, "input.gamepad") == NULL);
	CHECK(cs_config_get(path, "input.gamepad_right_dead_zone", val, sizeof val) == 1);
	CHECK(strcmp(val, "12000") == 0);

	/* a file with an [input] table: keys are replaced in place, missing ones added
	   inside the table (never above it), other tables and lines are kept */
	write_text(path, "# my config\n[display]\nwidth = 800\n\n[input]\n  gamepad_left_dead_zone=1\r\n"
		"# gamepad_right_dead_zone = 5\nlook = 3\n\n[extra]\nnet = 1");
	CHECK(cs_config_save_calibration(path, 20000, 100, 0) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(buf, "# my config\n[display]\nwidth = 800\n") == buf);
	CHECK(strstr(buf, "gamepad_left_dead_zone = 16000\r\n") != NULL);
	CHECK(strstr(buf, "# gamepad_right_dead_zone = 5\n") != NULL);
	CHECK(strstr(buf, "gamepad_right_dead_zone = 100") > strstr(buf, "[input]"));
	CHECK(strstr(buf, "gamepad_right_dead_zone = 100") < strstr(buf, "[extra]"));
	CHECK(strstr(buf, "gamepad_setup_done = true") > strstr(buf, "[input]"));
	CHECK(strstr(buf, "gamepad_setup_done = true") < strstr(buf, "[extra]"));
	CHECK(strstr(buf, "input.gamepad") == NULL);
	CHECK(strstr(buf, "[extra]\nnet = 1") != NULL);
	CHECK(strlen(strstr(buf, "net =")) == 7); /* no newline added at the end */

	/* a second save changes values and adds no duplicates */
	CHECK(cs_config_save_calibration(path, 5, 6, 1) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(strstr(buf, "gamepad_setup_done") + 1, "gamepad_setup_done") == NULL);
	CHECK(cs_config_get(path, "input.gamepad_left_dead_zone", val, sizeof val) == 1 && strcmp(val, "5") == 0);

	/* the old helper's file: dotted keys at the top and an [input] table below
	   are moved into the table (the game refuses the file otherwise) */
	write_text(path, "input.gamepad_left_dead_zone = 9000\ninput.gamepad_right_dead_zone = 9000\n"
		"input.gamepad_invert_look = false\ninput.gamepad_setup_done = true\n\n[display]\nmode = \"fullscreen\"\n\n"
		"[input]\nlook = 2\ngamepad_left_dead_zone = 100\n");
	CHECK(cs_config_save_skip(path) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(buf, "input.") == NULL);
	CHECK(strstr(buf, "\n[display]\nmode = \"fullscreen\"\n") == buf);
	CHECK(strstr(buf, "gamepad_invert_look = false") > strstr(buf, "[input]"));
	CHECK(cs_config_get(path, "input.gamepad_left_dead_zone", val, sizeof val) == 1 && strcmp(val, "9000") == 0);
	CHECK(cs_config_get(path, "input.look", val, sizeof val) == 1 && strcmp(val, "2") == 0);

	/* keys without a section go to a new one at the end; a file with no trailing newline */
	write_text(path, "[a]\nb = 1");
	CHECK(cs_config_save_skip(path) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strcmp(buf, "[a]\nb = 1\n\n[input]\ngamepad_setup_done = true\n") == 0);
	CHECK(cs_config_get(path, "nothing", val, sizeof val) == 0);
	remove(path);

	/* Sony pads' names, and a DualShock 4's elements (square, cross, circle, triangle, hat 0..7 with 8 neutral) */
	CHECK(strcmp(cs_sony_model(0x054c, 0x05c4), "DualShock 4") == 0);
	CHECK(strcmp(cs_sony_model(0x054c, 0x09cc), "DualShock 4") == 0);
	CHECK(strcmp(cs_sony_model(0x054c, 0x0ce6), "DualSense") == 0);
	CHECK(cs_sony_model(0x045e, 0x05c4)[0] == 0 && cs_sony_model(0x054c, 0x0268)[0] == 0);
	cs_pad_clear(&p, CS_LAYOUT_PLAYSTATION);
	CHECK(cs_hid_apply(&p, 9, 2, 1, 0, 1) && (p.buttons & 1)); /* cross is the Xbox A */
	CHECK(cs_hid_apply(&p, 9, 3, 1, 0, 1) && (p.buttons & 2)); /* circle: B */
	CHECK(cs_hid_apply(&p, 9, 1, 1, 0, 1) && (p.buttons & 4)); /* square: X */
	CHECK(cs_hid_apply(&p, 9, 4, 1, 0, 1) && (p.buttons & 8)); /* triangle: Y */
	CHECK(cs_hid_apply(&p, 9, 10, 1, 0, 1) && (p.buttons & (1UL << 7))); /* options: Menu */
	CHECK(cs_hid_apply(&p, 1, 0x33, 255, 0, 255) && p.lt == 1023); /* L2 analog */
	CHECK(cs_hid_apply(&p, 1, 0x35, 255, 0, 255) && p.ry == 32767); /* Rz is the right stick's y */
	CHECK(cs_hid_apply(&p, 1, 0x39, 8, 0, 7) && p.hat == 0);
	CHECK(cs_hid_apply(&p, 1, 0x39, 2, 0, 7) && p.hat == 3);

	/* hid mapping, Xbox layout */
	cs_pad_clear(&p, 1);
	CHECK(cs_hid_apply(&p, 1, 0x30, -32768, -32768, 32767) && p.lx == -32768);
	CHECK(cs_hid_apply(&p, 1, 0x34, 32767, -32768, 32767) && p.ry == 32767);
	CHECK(cs_hid_apply(&p, 1, 0x32, 1023, 0, 1023) && p.lt == 1023);
	CHECK(cs_hid_apply(&p, 1, 0x35, 0, 0, 1023) && p.rt == 0);
	CHECK(cs_hid_apply(&p, 1, 0x39, 3, 1, 8) && p.hat == 3);
	CHECK(cs_hid_apply(&p, 1, 0x39, 0, 1, 8) && p.hat == 0);
	CHECK(cs_hid_apply(&p, 9, 1, 1, 0, 1) && (p.buttons & 1));
	CHECK(cs_hid_apply(&p, 9, 12, 1, 0, 1) && (p.buttons & (1UL << 11)));
	CHECK(cs_hid_apply(&p, 9, 1, 0, 0, 1) && !(p.buttons & 1));

	/* generic pad: 0..255 sticks, Z/Rz as right stick, 0..7 hat with 8 neutral */
	cs_pad_clear(&p, 0);
	cs_hid_apply(&p, 1, 0x30, 0, 0, 255);
	CHECK(p.lx == -32768);
	cs_hid_apply(&p, 1, 0x30, 255, 0, 255);
	CHECK(p.lx == 32767);
	cs_hid_apply(&p, 1, 0x32, 255, 0, 255);
	CHECK(p.rx == 32767);
	cs_hid_apply(&p, 1, 0x39, 0, 0, 7);
	CHECK(p.hat == 1);
	cs_hid_apply(&p, 1, 0x39, 8, 0, 7);
	CHECK(p.hat == 0);
	cs_hid_apply(&p, 1, 0x33, 0, 0, 255);
	CHECK(p.seen_rstick && p.rx == -32768);
	CHECK(!cs_hid_apply(&p, 1, 0x32, 255, 0, 255));
	CHECK(cs_hid_in_dead_zone(100, 100, 9000) && !cs_hid_in_dead_zone(9000, 9000, 9000));

	/* a DualShock 4 report: stick right-down, cross and L1 down, hat north-east, L2 half */
	{
		unsigned char r[64];
		struct mac_gamepad_frame fr;
		unsigned extra = 0;
		cs_pad_state q;
		memset(r, 0, sizeof r);
		r[0] = 1; r[1] = 255; r[2] = 255; r[3] = 128; r[4] = 0;
		r[5] = 0x20 | 1; r[6] = 0x01 | 0x20; r[7] = 0x01; r[8] = 128;
		CHECK(mac_gamepad_decode_report(MAC_PAD_REPORT_DUALSHOCK4, r, 64, &fr, &extra));
		cs_pad_clear(&q, CS_LAYOUT_PLAYSTATION);
		cs_pad_from_frame(&q, &fr, extra);
		CHECK(q.lx == 32767 && q.ly == 32767 && q.ry == -32768);
		CHECK(q.hat == 2);
		CHECK((q.buttons & 1UL) && (q.buttons & (1UL << 4)) && (q.buttons & (1UL << 7)) && (q.buttons & (1UL << 10)));
		CHECK(q.lt == 128 * 1023 / 255 && q.rt == 0);
	}

	printf(failures ? "%d failures\n" : "all tests passed\n", failures);
	return failures != 0;
}
