/* Host test of launcher_config.c: cc -std=gnu89 -Wall -o /tmp/tlc test_launcher_config.c launcher_config.c && /tmp/tlc */
#include "launcher_config.h"
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

static char *read_all(const char *path)
{
	static char buf[8192];
	FILE *f = fopen(path, "rb");
	size_t n = f ? fread(buf, 1, sizeof(buf) - 1, f) : 0;

	buf[n] = 0;
	if (f)
		fclose(f);
	return buf;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/tmp/lc_test.toml";
	struct lc_model m;
	char v[64], *t;
	size_t len;

	/* get: sections, root dotted keys, quotes, comments */
	CHECK(lc_text_get("[display]\nmode = \"fullscreen\" # x\nvsync = false # y\n", "display.mode", v, sizeof v) && !strcmp(v, "fullscreen"));
	CHECK(lc_text_get("[display]\nvsync = false # y\n", "display.vsync", v, sizeof v) && !strcmp(v, "false"));
	CHECK(lc_text_get("display.gamma = 1.2\n[audio]\nvolume = 0.5\n", "display.gamma", v, sizeof v) && !strcmp(v, "1.2"));
	CHECK(lc_text_get("display.gamma = 1.2\n[audio]\nvolume = 0.5\n", "audio.volume", v, sizeof v) && !strcmp(v, "0.5"));
	CHECK(!lc_text_get("[audio]\nmode = 1\n", "display.mode", v, sizeof v));
	CHECK(!lc_text_get("# mode = 1\n[display]\n#mode = 2\n", "display.mode", v, sizeof v));
	CHECK(!lc_text_get("[audio]\nvolume = 1\n[display]\n", "display.volume", v, sizeof v));
	CHECK(lc_text_get("[display]\r\nmode = \"windowed\"\r\n", "display.mode", v, sizeof v) && !strcmp(v, "windowed"));

	/* defaults, no file */
	remove(path);
	lc_model_load(&m, NULL);
	CHECK(!strcmp(m.value[LC_MODE], "windowed") && !strcmp(m.value[LC_DETAIL], "auto"));
	CHECK(lc_bool(m.value[LC_VSYNC]) && !lc_bool(m.value[LC_ACHIEVEMENTS]));
	CHECK(lc_model_save(&m, path) == 0);			/* nothing changed: no file */
	CHECK(fopen(path, "rb") == NULL);

	/* a new file: sections made */
	m.hide = 0;
	strcpy(m.value[LC_MODE], "fullscreen");
	lc_set_bool(&m, LC_ACHIEVEMENTS, 1);
	lc_set_tenths(&m, LC_GAMMA, 12);
	CHECK(lc_model_save(&m, path) == 0);
	t = read_all(path);
	CHECK(strstr(t, "[display]\nmode = \"fullscreen\"\ngamma = 1.2\n") != NULL);
	CHECK(strstr(t, "[game]\nachievements = true\n") != NULL);
	lc_model_load(&m, t);
	CHECK(!strcmp(m.value[LC_MODE], "fullscreen") && lc_tenths(m.value[LC_GAMMA], 5, 20) == 12 && lc_bool(m.value[LC_ACHIEVEMENTS]));

	/* an existing file: edits in place, the rest kept byte for byte, keys added in their sections */
	write_text(path,
		"# header\ninput.gamepad_setup_done = true\n\n[display]\n# the mode\nmode = \"windowed\"   # kept?\nvsync = true\n\n"
		"[audio]\nvolume = 1.0\n\n[mac]\ndetail = \"auto\"\n");
	lc_model_load(&m, read_all(path));
	strcpy(m.value[LC_MODE], "fullscreen");
	strcpy(m.value[LC_RESOLUTION], "1024x768");
	lc_set_bool(&m, LC_VSYNC, 0);
	lc_set_tenths(&m, LC_VOLUME, 5);
	strcpy(m.value[LC_DETAIL], "low");
	lc_set_bool(&m, LC_MOVIES, 0);
	CHECK(lc_model_save(&m, path) == 0);
	t = read_all(path);
	CHECK(strstr(t, "# header\ninput.gamepad_setup_done = true\n\n[display]\n# the mode\nmode = \"fullscreen\"\nvsync = false\nresolution = \"1024x768\"\n\n[audio]\nvolume = 0.5\n\n[mac]\ndetail = \"low\"\n") != NULL);
	CHECK(strstr(t, "[game]\nmovies = false\n") != NULL);
	lc_model_load(&m, t);
	CHECK(!strcmp(m.value[LC_RESOLUTION], "1024x768") && !lc_bool(m.value[LC_VSYNC]) && !lc_bool(m.value[LC_MOVIES]));

	/* no trailing newline, CRLF, dotted keys at the top */
	t = strdup("[display]\r\nmode = \"windowed\"");
	len = strlen(t);
	CHECK(lc_text_set(&t, &len, "display.vsync", "false") == 0);
	CHECK(!strcmp(t, "[display]\r\nmode = \"windowed\"\r\nvsync = false\r\n"));
	free(t);
	t = strdup("display.mode = \"windowed\"\naudio.volume = 1.0\n");
	len = strlen(t);
	CHECK(lc_text_set(&t, &len, "display.vsync", "false") == 0);
	CHECK(lc_text_set(&t, &len, "audio.volume", "0.3") == 0);
	CHECK(lc_text_set(&t, &len, "mac.detail", "\"low\"") == 0);
	CHECK(!strcmp(t, "display.mode = \"windowed\"\ndisplay.vsync = false\naudio.volume = 0.3\n\n[mac]\ndetail = \"low\"\n"));
	free(t);
	t = NULL;
	len = 0;
	CHECK(lc_text_set(&t, &len, "a.b", "1") == 0 && !strcmp(t, "[a]\nb = 1\n"));
	free(t);

	CHECK(lc_tenths("0.04", 5, 20) == 5 && lc_tenths("9", 5, 20) == 20 && lc_tenths("1.25", 5, 20) == 13);
	remove(path);
	printf(failures ? "%d failures\n" : "launcher_config: all checks passed\n", failures);
	return failures != 0;
}
