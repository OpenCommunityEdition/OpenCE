/*
SERVER_CONSOLE.C

A dedicated server's commands from its owner, and the console's output
(server_console.h).
*/

#include "platform.h"
#include "port_config.h"
#include "server_console.h"
#include "server_mode.h"

#include <ctype.h>

#if HALO_SERVER_BUILD

#include "posix_server.h"
#include "server_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	/* the init file's most lines */
	MAXIMUM_INIT_LINES = 512,
	/* standard input's lines taken a frame */
	MAXIMUM_INPUT_LINES_PER_FRAME = 32,
	INPUT_BUFFER_SIZE = 4096,
	MAXIMUM_CAPTURE_DEPTH = 4,
};

static struct
{
	/* the init file's lines, and the next */
	char (*init_lines)[SERVER_COMMAND_LINE_SIZE];
	int init_count;
	int init_next;
	/* standard input: what is read and not yet a whole line, and its end */
	char input[INPUT_BUFFER_SIZE];
	int input_length;
	int input_ended;
	/* the "quit" its end asks for (server.console_exit_on_eof), not yet
	taken: its last line came first */
	int quit_owed;
	int input_overlong;
	int lines_this_frame;
	/* the captures: each one's text */
	char *captures[MAXIMUM_CAPTURE_DEPTH];
	int capture_lengths[MAXIMUM_CAPTURE_DEPTH];
	int capture_depth;
	int capture_overflow;
} server_console;

static const char *const role_names[] = { "viewer", "moderator", "admin", "owner" };

/* whether a word is the text, in either case */
static int word_is(const char *word, int length, const char *text)
{
	int index;

	for (index = 0; index < length && text[index]; index++)
	{
		if (tolower((unsigned char)word[index]) != tolower((unsigned char)text[index]))
			return 0;
	}
	return index == length && !text[index];
}

/* whether the text holds the part, in either case */
static int holds_word(const char *text, int length, const char *part)
{
	int start, size = (int)strlen(part);

	for (start = 0; start + size <= length; start++)
	{
		if (word_is(text + start, size, part))
			return 1;
	}
	return 0;
}

/* the word at the cursor, as the commands split a line (server_commands.c's
split_words: a quoted word whole, without its quotes): its start and
length, the cursor after it; 0 at the line's end */
static int next_word(const char **cursor, const char **word, int *length)
{
	const char *at = *cursor;

	while (*at == ' ' || *at == '\t')
		at++;
	if (!*at)
		return 0;
	if (*at == '"')
	{
		*word = ++at;
		while (*at && *at != '"')
			at++;
	}
	else
	{
		*word = at;
		while (*at && *at != ' ' && *at != '\t')
			at++;
	}
	*length = (int)(at - *word);
	*cursor = *at ? at + 1 : at;
	return 1;
}

int server_console_line_is_secret(const char *line)
{
	const char *cursor = line, *word, *setting;
	int length, setting_length;

	if (!next_word(&cursor, &word, &length))
		return 0;
	if (word_is(word, length, "sv_password"))
		return 1;
	if (!word_is(word, length, "sv_set"))
		return 0;
	/* (sv_set's setting: a password's, or the publisher key's) */
	if (!next_word(&cursor, &setting, &setting_length))
		return 0;
	return holds_word(setting, setting_length, "password") || holds_word(setting, setting_length, "publisher_key");
}

int server_role_named(const char *name)
{
	int index;

	for (index = 0; index < (int)(sizeof(role_names) / sizeof(role_names[0])); index++)
	{
		if (!strcmp(name, role_names[index]))
			return index;
	}
	return _server_role_none;
}

const char *server_role_name(int role)
{
	return role >= 0 && role < (int)(sizeof(role_names) / sizeof(role_names[0])) ? role_names[role] : "none";
}

/* a line kept to printable ASCII, its ends' spaces and a comment's line
dropped: empty if nothing is left to run */
static void clean_line(char *line)
{
	int read, written = 0;

	for (read = 0; line[read]; read++)
	{
		unsigned char character = (unsigned char)line[read];

		if (character == '\t')
			character = ' ';
		if (character >= 0x20 && character < 0x7F)
			line[written++] = (char)character;
	}
	line[written] = 0;
	while (written > 0 && line[written - 1] == ' ')
		line[--written] = 0;
	for (read = 0; line[read] == ' '; read++)
	{
	}
	if (read)
		memmove(line, line + read, strlen(line + read) + 1);
	if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/'))
		line[0] = 0;
}

void server_console_start(void)
{
	const char *setting = config_string("server.init_file");
	char path[1024];
	FILE *file;
	char line[SERVER_COMMAND_LINE_SIZE + 2];

	if (!setting[0])
		return;
	/* (a full path, else the data root's) */
	if (setting[0] == '/' || (setting[0] && setting[1] == ':'))
		snprintf(path, sizeof(path), "%s", setting);
	else
		snprintf(path, sizeof(path), "%s/%s", platform_data_root(), setting);
	file = fopen(path, "r");
	if (!file)
	{
		server_log(_server_log_info, "server", "no init file (%s): the settings alone", path);
		return;
	}
	server_console.init_lines = calloc(MAXIMUM_INIT_LINES, SERVER_COMMAND_LINE_SIZE);
	while (server_console.init_lines && fgets(line, sizeof(line), file))
	{
		int whole = strchr(line, '\n') != NULL || feof(file);

		line[strcspn(line, "\r\n")] = 0;
		if (!whole)
		{
			int character;

			/* (the rest of a line too long to run, left out) */
			while ((character = fgetc(file)) != EOF && character != '\n')
			{
			}
			server_log(_server_log_warning, "server", "%s: a line longer than %d characters left out", setting,
				SERVER_COMMAND_LINE_SIZE - 1);
			continue;
		}
		clean_line(line);
		if (!line[0])
			continue;
		/* (a line as long as the buffer: left out, not run cut short) */
		if (strlen(line) >= SERVER_COMMAND_LINE_SIZE)
		{
			server_log(_server_log_warning, "server", "%s: a line longer than %d characters left out", setting,
				SERVER_COMMAND_LINE_SIZE - 1);
			continue;
		}
		if (server_console.init_count >= MAXIMUM_INIT_LINES)
		{
			server_log(_server_log_warning, "server", "%s: lines past %d left out", setting, MAXIMUM_INIT_LINES);
			break;
		}
		snprintf(server_console.init_lines[server_console.init_count++], SERVER_COMMAND_LINE_SIZE, "%s", line);
	}
	fclose(file);
	server_log(_server_log_info, "server", "%d command%s from %s", server_console.init_count,
		server_console.init_count == 1 ? "" : "s", path);
}

/* a whole line of standard input's, taken out of the buffer: 1 */
static int take_input_line(char *line, int size)
{
	char *end = memchr(server_console.input, '\n', (size_t)server_console.input_length);
	int length;

	if (!end)
		return 0;
	length = (int)(end - server_console.input);
	if (server_console.input_overlong)
	{
		/* (the end of a line too long, dropped with its start) */
		server_console.input_overlong = 0;
		line[0] = 0;
	}
	else if (length >= size)
	{
		/* (a line longer than a command's: left out, not run cut short) */
		server_log(_server_log_warning, "console", "a line longer than %d characters left out", size - 1);
		line[0] = 0;
	}
	else
	{
		snprintf(line, (size_t)size, "%.*s", length, server_console.input);
	}
	memmove(server_console.input, end + 1, (size_t)(server_console.input_length - length - 1));
	server_console.input_length -= length + 1;
	return 1;
}

int server_console_next(char *line, int size, int *origin)
{
	if (server_console.init_lines && server_console.init_next < server_console.init_count)
	{
		const char *taken = server_console.init_lines[server_console.init_next++];

		/* (a line longer than a command's: left out, not run cut short) */
		if (strlen(taken) >= (size_t)size)
		{
			server_log(_server_log_warning, "console", "a line of the init file longer than %d characters left out",
				size - 1);
			taken = "";
		}
		snprintf(line, (size_t)size, "%s", taken);
		*origin = _server_origin_init_file;
		if (server_console.init_next == server_console.init_count)
		{
			free(server_console.init_lines);
			server_console.init_lines = NULL;
		}
		return 1;
	}
	*origin = _server_origin_standard_input;
	if (!config_boolean("server.console"))
		return 0;
	/* (so many a frame: a pasted file runs over a few frames, and the game
	goes on) */
	if (server_console.lines_this_frame >= MAXIMUM_INPUT_LINES_PER_FRAME)
		return 0;
	for (;;)
	{
		int count;

		if (take_input_line(line, size))
		{
			clean_line(line);
			if (!line[0])
				continue;
			server_console.lines_this_frame++;
			return 1;
		}
		if (server_console.input_ended)
		{
			if (!server_console.quit_owed)
				return 0;
			server_console.quit_owed = 0;
			snprintf(line, (size_t)size, "quit");
			return 1;
		}
		if (server_console.input_length >= INPUT_BUFFER_SIZE - 1)
		{
			/* a line longer than the buffer: dropped, to its end */
			server_console.input_length = 0;
			server_console.input_overlong = 1;
			server_log(_server_log_warning, "console", "a line longer than %d characters left out", INPUT_BUFFER_SIZE - 1);
		}
		count = posix_server_stdin_read(server_console.input + server_console.input_length,
			INPUT_BUFFER_SIZE - 1 - server_console.input_length);
		if (count == 0)
			return 0;
		if (count < 0)
		{
			int last = server_console.input_length > 0 && !server_console.input_overlong;

			server_console.input_ended = 1;
			if (config_boolean("server.console_exit_on_eof"))
			{
				server_log(_server_log_info, "console", "the standard input ended: stopping "
					"(server.console_exit_on_eof)");
				server_console.quit_owed = 1;
			}
			/* (its last line, with no newline, before the quit) */
			if (last && server_console.input_length >= size)
			{
				server_log(_server_log_warning, "console", "a line longer than %d characters left out", size - 1);
				last = 0;
			}
			if (last)
			{
				snprintf(line, (size_t)size, "%.*s", server_console.input_length, server_console.input);
				clean_line(line);
			}
			server_console.input_length = 0;
			if (last && line[0])
			{
				server_console.lines_this_frame++;
				return 1;
			}
			continue;
		}
		server_console.input_length += count;
	}
}

void server_console_frame(void)
{
	server_console.lines_this_frame = 0;
}

/* ---------- output */

void server_console_output(const char *text)
{
	int depth = server_console.capture_depth;

	if (!halo_server_mode() || !text || !text[0])
		return;
	if (depth > 0)
	{
		int index = depth - 1;
		int length = (int)strlen(text);

		if (!server_console.captures[index] ||
			server_console.capture_lengths[index] + length + 2 > SERVER_COMMAND_OUTPUT_SIZE)
		{
			server_console.capture_overflow = 1;
			return;
		}
		memcpy(server_console.captures[index] + server_console.capture_lengths[index], text, (size_t)length);
		server_console.capture_lengths[index] += length;
		server_console.captures[index][server_console.capture_lengths[index]++] = '\n';
		server_console.captures[index][server_console.capture_lengths[index]] = 0;
		return;
	}
	/* (the game's own lines: its warnings, the host's notices) */
	server_log(_server_log_info, "console", "%s", text);
}

int server_console_capture_begin(void)
{
	int index = server_console.capture_depth;

	if (index >= MAXIMUM_CAPTURE_DEPTH)
		return 0;
	server_console.captures[index] = malloc(SERVER_COMMAND_OUTPUT_SIZE);
	if (server_console.captures[index])
		server_console.captures[index][0] = 0;
	server_console.capture_lengths[index] = 0;
	server_console.capture_depth++;
	return 1;
}

void server_console_free(char *text)
{
	free(text);
}

char *server_console_capture_end(void)
{
	char *text;

	if (server_console.capture_depth <= 0)
		return NULL;
	server_console.capture_depth--;
	text = server_console.captures[server_console.capture_depth];
	server_console.captures[server_console.capture_depth] = NULL;
	return text;
}

#else

void server_console_start(void)
{
}

void server_console_frame(void)
{
}

int server_console_next(char *line, int size, int *origin)
{
	(void)line;
	(void)size;
	*origin = 0;
	return 0;
}

void server_console_output(const char *text)
{
	(void)text;
}

int server_console_capture_begin(void)
{
	return 0;
}

char *server_console_capture_end(void)
{
	return NULL;
}

void server_console_free(char *text)
{
	(void)text;
}

int server_console_line_is_secret(const char *line)
{
	(void)line;
	return 0;
}

int server_role_named(const char *name)
{
	(void)name;
	return _server_role_none;
}

const char *server_role_name(int role)
{
	(void)role;
	return "none";
}

#endif
