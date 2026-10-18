/* cc -o test_config test_config.c controller_setup_config.c controller_setup_hid.c */
#include "controller_setup_config.h"
#include "controller_setup_hid.h"

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

	/* missing file is created */
	remove(path);
	CHECK(cs_config_save_calibration(path, 9000, 12000, 1) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(buf, "input.gamepad_left_dead_zone = 9000\n") != NULL);
	CHECK(strstr(buf, "input.gamepad_invert_look = true\n") != NULL);
	CHECK(cs_config_get(path, "input.gamepad_right_dead_zone", val, sizeof val) == 1);
	CHECK(strcmp(val, "12000") == 0);

	/* other lines are kept; keys are replaced, missing keys go before a section */
	write_text(path, "# my config\nvideo.width = 800\n  input.gamepad_left_dead_zone=1\r\n"
		"# input.gamepad_right_dead_zone = 5\n\n[extra]\nnet.x = 1");
	CHECK(cs_config_save_calibration(path, 20000, 100, 0) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(buf, "# my config\nvideo.width = 800\n") == buf);
	CHECK(strstr(buf, "input.gamepad_left_dead_zone = 16000\r\n") != NULL);
	CHECK(strstr(buf, "# input.gamepad_right_dead_zone = 5\n") != NULL);
	CHECK(strstr(buf, "input.gamepad_right_dead_zone = 100\n") < strstr(buf, "[extra]"));
	CHECK(strstr(buf, "input.gamepad_setup_done = true\n") < strstr(buf, "[extra]"));
	CHECK(strstr(buf, "[extra]\nnet.x = 1") != NULL);
	CHECK(strlen(strstr(buf, "net.x")) == 9); /* no newline added at the end */

	/* second save changes values and adds no duplicates */
	CHECK(cs_config_save_calibration(path, 5, 6, 1) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strstr(strstr(buf, "gamepad_setup_done") + 1, "gamepad_setup_done") == NULL);
	CHECK(cs_config_get(path, "input.gamepad_left_dead_zone", val, sizeof val) == 1 && strcmp(val, "5") == 0);

	/* skip on a file with no trailing newline */
	write_text(path, "a.b = 1");
	CHECK(cs_config_save_skip(path) == 0);
	read_text(path, buf, sizeof buf);
	CHECK(strcmp(buf, "a.b = 1\ninput.gamepad_setup_done = true\n") == 0);
	CHECK(cs_config_get(path, "nothing", val, sizeof val) == 0);
	remove(path);

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

	printf(failures ? "%d failures\n" : "all tests passed\n", failures);
	return failures != 0;
}
