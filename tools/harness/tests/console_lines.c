/*
CONSOLE_LINES.C (test)

A dedicated server's command lines (port/linux/src/server_console.c): the
init file's and standard input's, a line too long to run left out rather
than run cut short, and server.console_exit_on_eof's "quit" after the
standard input's last line. test_console_lines.py takes the whole file with
its headers; the settings, the standard input and the log are the test's.
*/

#include "harness.h"

#include <unistd.h>

#include "server_log.h"

static const char *init_file = "";
static int exit_on_eof;
/* standard input: what it has, how much is read, and how much a read
gives at most */
static const char *input = "";
static size_t input_read;
static int input_chunk = 4096;
static int warnings;

int config_boolean(const char *name)
{
	if (!strcmp(name, "server.console"))
		return 1;
	return !strcmp(name, "server.console_exit_on_eof") && exit_on_eof;
}

const char *config_string(const char *name)
{
	return !strcmp(name, "server.init_file") ? init_file : "";
}

const char *platform_data_root(void)
{
	return ".";
}

int halo_server_mode(void)
{
	return 1;
}

int posix_server_stdin_read(char *buffer, int size)
{
	size_t left = strlen(input) - input_read;
	int count = size < input_chunk ? size : input_chunk;

	if (!left)
		return -1;
	if ((size_t)count > left)
		count = (int)left;
	memcpy(buffer, input + input_read, (size_t)count);
	input_read += (size_t)count;
	return count;
}

void server_log(int level, const char *category, const char *format, ...)
{
	(void)category;
	(void)format;
	if (level == _server_log_warning)
		warnings++;
}

#include "under_test.inc"

static char line[SERVER_COMMAND_LINE_SIZE];

/* the next line run, NULL if none */
static const char *next_line(void)
{
	int origin;

	return server_console_next(line, sizeof(line), &origin) ? line : NULL;
}

/* a line of so many x's, after its first character */
static char *long_line(char *text, char first, int length)
{
	memset(text, 'x', (size_t)length);
	text[0] = first;
	text[length] = 0;
	return text;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	const char *taken;

	/* the standard input ended with a line of its own: it, then quit */
	CASE("eof-quit")
	{
		exit_on_eof = 1;
		input = "sv_status\nsay hi";
		input_chunk = 3;
		taken = next_line();
		CHECK(taken && !strcmp(taken, "sv_status"), "the first line was %s", taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "say hi"), "the unterminated last line was %s", taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "quit"), "after the last line came %s, not quit", taken ? taken : "none");
		CHECK(!next_line(), "a line came after the quit");
		CHECK(!next_line(), "a line came after the quit");
		return 0;
	}
	/* a standard input ending with a newline: quit after its last line */
	CASE("eof-quit-terminated")
	{
		exit_on_eof = 1;
		input = "sv_status\n";
		taken = next_line();
		CHECK(taken && !strcmp(taken, "sv_status"), "the line was %s", taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "quit"), "after the last line came %s, not quit", taken ? taken : "none");
		CHECK(!next_line(), "a line came after the quit");
		return 0;
	}
	/* without server.console_exit_on_eof: the last line, and no quit */
	CASE("eof-go-on")
	{
		input = "sv_status\nsay hi";
		taken = next_line();
		CHECK(taken && !strcmp(taken, "sv_status"), "the first line was %s", taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "say hi"), "the unterminated last line was %s", taken ? taken : "none");
		CHECK(!next_line(), "a line came after the standard input's end");
		return 0;
	}
	/* standard input's lines: SERVER_COMMAND_LINE_SIZE - 1 characters run,
	more are left out, the unterminated last one too */
	CASE("input-limits")
	{
		static char text[4 * SERVER_COMMAND_LINE_SIZE];
		char longest[SERVER_COMMAND_LINE_SIZE + 8];

		snprintf(text, sizeof(text), "%s\n", long_line(longest, 'a', SERVER_COMMAND_LINE_SIZE - 1));
		snprintf(text + strlen(text), sizeof(text) - strlen(text), "%s\n", long_line(longest, 'b',
			SERVER_COMMAND_LINE_SIZE));
		snprintf(text + strlen(text), sizeof(text) - strlen(text), "after\n%s", long_line(longest, 'c',
			SERVER_COMMAND_LINE_SIZE));
		exit_on_eof = 1;
		input = text;
		input_chunk = 100;
		taken = next_line();
		CHECK(taken && taken[0] == 'a' && strlen(taken) == SERVER_COMMAND_LINE_SIZE - 1, "the longest line came as %s",
			taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "after"), "after the longest came %.20s (%d characters), not after",
			taken ? taken : "none", taken ? (int)strlen(taken) : 0);
		taken = next_line();
		CHECK(taken && !strcmp(taken, "quit"), "after the line too long came %.20s, not quit", taken ? taken : "none");
		CHECK(warnings == 2, "%d lines were left out, not 2", warnings);
		return 0;
	}
	/* the init file's lines: as standard input's */
	CASE("init-limits")
	{
		char path[] = "/tmp/halo-harness-init-XXXXXX";
		char longest[SERVER_COMMAND_LINE_SIZE + 128];
		int descriptor = mkstemp(path);
		FILE *file = descriptor >= 0 ? fdopen(descriptor, "w") : NULL;

		CHECK(file != NULL, "no init file could be written");
		fprintf(file, "%s\n", long_line(longest, 'a', SERVER_COMMAND_LINE_SIZE - 1));
		fprintf(file, "%s\n", long_line(longest, 'b', SERVER_COMMAND_LINE_SIZE));
		fprintf(file, "%s\n", long_line(longest, 'c', SERVER_COMMAND_LINE_SIZE + 1));
		fprintf(file, "%s\r\n", long_line(longest, 'd', SERVER_COMMAND_LINE_SIZE + 100));
		fprintf(file, "# a comment\nlast");
		fclose(file);
		init_file = path;
		server_console_start();
		unlink(path);
		taken = next_line();
		CHECK(taken && taken[0] == 'a' && strlen(taken) == SERVER_COMMAND_LINE_SIZE - 1, "the longest line came as %.20s",
			taken ? taken : "none");
		taken = next_line();
		CHECK(taken && !strcmp(taken, "last"), "after the longest came %.20s (%d characters), not last",
			taken ? taken : "none", taken ? (int)strlen(taken) : 0);
		CHECK(warnings == 3, "%d lines were left out, not 3", warnings);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
