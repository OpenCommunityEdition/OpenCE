/* Host test of the config.toml repair and check (launcher_config.c, launcher_toml_check.c):
   cc -std=gnu99 -Wall -I../../third_party/tomlc17 -o /tmp/tlt test_launcher_toml.c launcher_config.c \
      launcher_toml_check.c ../../third_party/tomlc17/tomlc17.c && /tmp/tlt */
#include "launcher_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static char *copy(const char *s)
{
	char *p = (char *)malloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}

int main(void)
{
	/* the file the old Controller Setup left (the user's), with the launcher's choices below */
	const char *broken =
		"input.gamepad_left_dead_zone = 9000\n"
		"input.gamepad_right_dead_zone = 9000\n"
		"input.gamepad_invert_look = false\n"
		"input.gamepad_setup_done = true\n"
		"# OpenCE settings\n"
		"[display]\n"
		"mode = \"fullscreen\"\n"
		"resolution = \"1280x800\"\n"
		"\n"
		"[input]\n"
		"look_sensitivity = 2.0\n";
	char *text = copy(broken), message[200], value[64];
	size_t length = strlen(text);

	/* 1 and 3: the check names the line the game's parser stops at */
	CHECK(!lc_text_check(text, length, message, sizeof(message)));
	CHECK(strstr(message, "table defined before") != NULL);
	printf("check says: %s\n", message);
	CHECK(lc_text_check("[a]\nb = 1\n", 10, message, sizeof(message)) == 1 && message[0] == 0);
	CHECK(!lc_text_check("[a\nb = 1\n", 9, message, sizeof(message)) && message[0]);
	CHECK(!lc_text_check("[a]\nb = 1\nb = 2\n", 16, message, sizeof(message)));

	/* 2: the repair moves the four keys into [input] */
	CHECK(lc_text_repair(&text, &length) == 4);
	CHECK(length == strlen(text));
	CHECK(lc_text_check(text, length, message, sizeof(message)) == 1);
	CHECK(strstr(text, "input.") == NULL);
	CHECK(strstr(text, "mode = \"fullscreen\"") != NULL && strstr(text, "resolution = \"1280x800\"") != NULL);
	CHECK(strstr(text, "# OpenCE settings\n[display]") != NULL);
	CHECK(strstr(text, "gamepad_setup_done = true") > strstr(text, "[input]"));
	CHECK(lc_text_get(text, "input.gamepad_left_dead_zone", value, sizeof(value)) && !strcmp(value, "9000"));
	CHECK(lc_text_get(text, "input.look_sensitivity", value, sizeof(value)) && !strcmp(value, "2.0"));
	/* and again: nothing is left to move */
	CHECK(lc_text_repair(&text, &length) == 0);
	free(text);

	/* the dotted line wins over the table's own value; CRLF files keep their line ends */
	text = copy("input.gamepad_invert_look = true\r\n[input]\r\ngamepad_invert_look = false\r\nx = 1\r\n");
	length = strlen(text);
	CHECK(lc_text_repair(&text, &length) == 1);
	CHECK(lc_text_check(text, length, message, sizeof(message)) == 1);
	CHECK(lc_text_get(text, "input.gamepad_invert_look", value, sizeof(value)) && !strcmp(value, "true"));
	CHECK(strstr(text, "\r\n") != NULL && strchr(text, '\n') == strstr(text, "\r\n") + 1);
	free(text);

	/* valid files are not touched: dotted keys with no such table, a clean file */
	text = copy("input.gamepad_setup_done = true\n[display]\nmode = \"windowed\"\n");
	length = strlen(text);
	CHECK(lc_text_check(text, length, message, sizeof(message)) == 1);
	CHECK(lc_text_repair(&text, &length) == 0);
	CHECK(!strcmp(text, "input.gamepad_setup_done = true\n[display]\nmode = \"windowed\"\n"));
	free(text);
	text = copy("");
	length = 0;
	CHECK(lc_text_repair(&text, &length) == 0 && lc_text_check(text, 0, message, sizeof(message)) == 1);
	free(text);

	printf(failures ? "%d failures\n" : "all tests passed\n", failures);
	return failures != 0;
}
