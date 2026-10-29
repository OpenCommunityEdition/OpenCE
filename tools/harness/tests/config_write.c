/*
CONFIG_WRITE.C (test)

config_write (port/linux/src/port_config.c): a setting set for now and its
line in config.toml changed, a string as long as a server's rotation
(escaped as a TOML basic string) included, and nothing written by a run
that only answers its command line. test_config_write.py takes the code
and its types from the sources; the settings and the file are the test's.
*/

#include "harness.h"

#include <ctype.h>
#include <pthread.h>

#include "types.inc"

static const struct config_setting config_settings[] =
{
	{ "display.vsync", _config_boolean, "true", NULL, _environment_value, _platform_all, "" },
	{ "server.rotation", _config_string, "\"\"", NULL, _environment_value, _platform_all, "" },
};
#define NUMBER_OF_CONFIG_SETTINGS (sizeof(config_settings) / sizeof(config_settings[0]))

static struct config_value config_values[NUMBER_OF_CONFIG_SETTINGS];
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile unsigned long config_change_count;
static int config_answer_only;

/* config.toml: what it holds, how many times it was written, and how many
of those by a run that only answers its command line */
static char *file_text;
static int file_writes;
static int answering_writes;

static long config_setting_index(const char *name);

static const struct config_value *config_value(const char *name, enum config_type type)
{
	(void)type;
	return &config_values[config_setting_index(name)];
}

static void config_path(char *path, size_t size)
{
	snprintf(path, size, "config.toml");
}

static char *config_read_file(const char *path, size_t *size)
{
	(void)path;
	*size = file_text ? strlen(file_text) : 0;
	return file_text ? strdup(file_text) : NULL;
}

static int config_write_file(const char *path, const char *text)
{
	(void)path;
	free(file_text);
	file_text = strdup(text);
	file_writes++;
	answering_writes += config_answer_only != 0;
	return 1;
}

#include "under_test.inc"

/* the file's line for the key, NULL if none */
static const char *file_line(const char *key)
{
	const char *line;

	for (line = file_text; line && *line; line = strchr(line, '\n') ? strchr(line, '\n') + 1 : line + strlen(line))
	{
		if (!strncmp(line, key, strlen(key)) && line[strlen(key)] == ' ')
			return line;
	}
	return NULL;
}

static char value[8192];
static char expected[8 * 8192];

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	const char *line;
	size_t index;

	file_text = strdup("[display]\nvsync = true\n\n[server]\nrotation = \"bloodgulch\"\n");
	config_values[1].string = strdup("bloodgulch");
	/* a rotation as long as one may be, with quotes, backslashes and control
	characters escaped */
	CASE("long-string")
	{
		char *end;

		for (index = 0; index < CONFIG_MAXIMUM_WRITTEN_STRING; index++)
			value[index] = "ab\"\\\x01 "[index % 6];
		value[CONFIG_MAXIMUM_WRITTEN_STRING] = 0;
		CHECK(config_write("server.rotation", value), "a rotation of %d characters was refused",
			CONFIG_MAXIMUM_WRITTEN_STRING);
		CHECK(!strcmp(config_values[1].string, value), "the rotation set was not the one written");
		end = expected + sprintf(expected, "rotation = \"");
		for (index = 0; value[index]; index++)
		{
			if (value[index] == '\x01')
				end += sprintf(end, "\\u0001");
			else if (value[index] == '"' || value[index] == '\\')
				end += sprintf(end, "\\%c", value[index]);
			else
				*end++ = value[index];
		}
		strcpy(end, "\"\n");
		line = file_line("rotation");
		CHECK(line && !strncmp(line, expected, strlen(expected)), "the file's line was %.60s", line ? line : "none");
		CHECK(!strstr(file_text, "bloodgulch") && strstr(file_text, "vsync = true\n"),
			"the file's other lines were changed:\n%.200s", file_text);
		return 0;
	}
	/* each character escaped as \u0001: the longest line */
	CASE("all-escaped")
	{
		memset(value, '\x01', CONFIG_MAXIMUM_WRITTEN_STRING);
		value[CONFIG_MAXIMUM_WRITTEN_STRING] = 0;
		CHECK(config_write("server.rotation", value), "a rotation of control characters was refused");
		line = file_line("rotation");
		CHECK(line && strlen(line) == strlen("rotation = \"\"\n") + 6 * CONFIG_MAXIMUM_WRITTEN_STRING,
			"the file's line was %d characters", line ? (int)strlen(line) : 0);
		return 0;
	}
	/* longer than a rotation may be: refused, the setting and the file as
	they were */
	CASE("too-long")
	{
		memset(value, 'a', CONFIG_MAXIMUM_WRITTEN_STRING + 1);
		value[CONFIG_MAXIMUM_WRITTEN_STRING + 1] = 0;
		CHECK(!config_write("server.rotation", value), "a rotation of %d characters was taken",
			CONFIG_MAXIMUM_WRITTEN_STRING + 1);
		CHECK(!strcmp(config_values[1].string, "bloodgulch") && file_writes == 0, "a refused rotation was set");
		/* (a boolean's text stays short) */
		memset(value, 'a', 300);
		value[300] = 0;
		CHECK(!config_write("display.vsync", value), "a boolean's text of 300 characters was taken");
		CHECK(config_write("display.vsync", "false") && file_line("vsync") &&
			!strncmp(file_line("vsync"), "vsync = false\n", 14), "a boolean was not written");
		return 0;
	}
	/* a run that only answers its command line (a probe's) writes nothing:
	the value is its own */
	CASE("answer-only")
	{
		config_answer_only = 1;
		CHECK(config_write("server.rotation", "prisoner"), "the write was refused");
		CHECK(!strcmp(config_values[1].string, "prisoner"), "the value was not set");
		CHECK(file_writes == 0 && answering_writes == 0 && strstr(file_text, "bloodgulch"), "config.toml was written");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
