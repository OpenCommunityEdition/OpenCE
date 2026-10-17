/*
DEDICATED_CONSOLE.C

A dedicated server's console (dedicated_console.h; the server is
port/linux/game/dedicated_server.c, server.dedicated):

- its command file (server.init_file: init.txt in the data folder, as Halo
  PC's dedicated server reads it), read once at the start, MAXIMUM_FILE_LINES
  lines at most;
- the lines typed on its standard input (Linux), read on a thread of their
  own and queued (MAXIMUM_QUEUED_LINES; more are dropped until the game takes
  them). The thread reads the descriptor, not stdio's stdin, whose lock it
  would hold while it waits: exit() flushes every stream, and would wait for
  it for ever;
- Ctrl+C (SIGINT), a service stopped (SIGTERM) and a terminal closed (SIGHUP),
  which ask the server to stop at its next frame instead of ending it at
  once, so that its listing in the server browser is closed as it goes.

Nothing here runs a command, a script or a program: the game's side takes
only its own sv_* commands. Lines are cut to DEDICATED_CONSOLE_LINE_SIZE.
*/

#include "platform.h"
#include "port_config.h"
#include "dedicated_console.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) && !defined(HALO_ANDROID) && !defined(HALO_WEB)
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#endif

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)

enum
{
	/* the command file's lines (a longer file is cut, and the operator told) */
	MAXIMUM_FILE_LINES = 512,
	/* the console's lines waiting for the game */
	MAXIMUM_QUEUED_LINES = 64,
	/* a line as read, before it is cut */
	READ_LINE_SIZE = 1024,
};

static struct
{
	char path[1024];
	int read;
	int found;
	char (*lines)[DEDICATED_CONSOLE_LINE_SIZE];
	int line_count;
	int next_line;
} command_file;

static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static char queued[MAXIMUM_QUEUED_LINES][DEDICATED_CONSOLE_LINE_SIZE];
static int queue_first, queue_count;

#ifndef _WIN32
static volatile sig_atomic_t quit_signal;
#endif

void dedicated_console_print(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vfprintf(stdout, format, arguments);
	va_end(arguments);
	fputc('\n', stdout);
	fflush(stdout);
}

/* a line of a file or the standard input, cut to size: 0 at its end. The
rest of a line longer than the buffer is read and dropped */
static int read_line(FILE *file, char *line, int size)
{
	char buffer[READ_LINE_SIZE];
	size_t length;

	if (!fgets(buffer, sizeof(buffer), file))
		return 0;
	length = strlen(buffer);
	if (length && buffer[length - 1] != '\n' && !feof(file))
	{
		int character;

		while ((character = fgetc(file)) != EOF && character != '\n')
			;
	}
	buffer[strcspn(buffer, "\r\n")] = 0;
	snprintf(line, (size_t)size, "%s", buffer);
	return 1;
}

static int full_path(const char *path)
{
#ifdef _WIN32
	return path[0] == '\\' || path[0] == '/' || (path[0] && path[1] == ':');
#else
	return path[0] == '/';
#endif
}

static void read_command_file(void)
{
	const char *name = config_string("server.init_file");
	FILE *file;

	command_file.read = 1;
	if (!name[0])
		return;
	if (full_path(name))
		snprintf(command_file.path, sizeof(command_file.path), "%s", name);
	else
		snprintf(command_file.path, sizeof(command_file.path), "%s/%s", platform_data_root(), name);
	file = fopen(command_file.path, "r");
	if (!file)
		return;
	command_file.found = 1;
	command_file.lines = calloc(MAXIMUM_FILE_LINES, sizeof(*command_file.lines));
	if (command_file.lines)
	{
		char line[DEDICATED_CONSOLE_LINE_SIZE];

		while (read_line(file, line, sizeof(line)))
		{
			if (command_file.line_count == MAXIMUM_FILE_LINES)
			{
				dedicated_console_print("server: %s has more than %d lines; the rest are left out", command_file.path,
					MAXIMUM_FILE_LINES);
				break;
			}
			memcpy(command_file.lines[command_file.line_count++], line, sizeof(line));
		}
	}
	fclose(file);
}

const char *dedicated_console_file(int *found)
{
	if (!command_file.read)
		read_command_file();
	if (found)
		*found = command_file.found;
	return command_file.path[0] ? command_file.path : "(none: server.init_file is empty)";
}

int dedicated_console_next(char *text, int size, int *from_file)
{
	int result = 0;

	if (!command_file.read)
		read_command_file();
	if (command_file.next_line < command_file.line_count)
	{
		snprintf(text, (size_t)size, "%s", command_file.lines[command_file.next_line++]);
		*from_file = 1;
		if (command_file.next_line == command_file.line_count)
		{
			free(command_file.lines);
			command_file.lines = NULL;
			command_file.line_count = command_file.next_line = 0;
		}
		return 1;
	}
	pthread_mutex_lock(&queue_lock);
	if (queue_count)
	{
		snprintf(text, (size_t)size, "%s", queued[queue_first]);
		queue_first = (queue_first + 1) % MAXIMUM_QUEUED_LINES;
		queue_count--;
		*from_file = 0;
		result = 1;
	}
	pthread_mutex_unlock(&queue_lock);
	return result;
}

#ifndef _WIN32
/* a line typed, queued for the game */
static void queue_line(const char *line)
{
	pthread_mutex_lock(&queue_lock);
	if (queue_count < MAXIMUM_QUEUED_LINES)
	{
		snprintf(queued[(queue_first + queue_count) % MAXIMUM_QUEUED_LINES], DEDICATED_CONSOLE_LINE_SIZE, "%s", line);
		queue_count++;
	}
	pthread_mutex_unlock(&queue_lock);
}

/* (the console's thread) the standard input, a line at a time (a longer
one cut), queued */
static void *console_thread(void *unused)
{
	char buffer[READ_LINE_SIZE];
	char line[DEDICATED_CONSOLE_LINE_SIZE];
	int length = 0;

	(void)unused;
	for (;;)
	{
		ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
		ssize_t index;

		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			break;
		for (index = 0; index < count; index++)
		{
			if (buffer[index] == '\n')
			{
				line[length] = 0;
				queue_line(line);
				length = 0;
			}
			else if (buffer[index] != '\r' && length < (int)sizeof(line) - 1)
				line[length++] = buffer[index];
		}
	}
	if (length)
	{
		line[length] = 0;
		queue_line(line);
	}
	return NULL;
}

static void quit_handler(int number)
{
	(void)number;
	quit_signal = 1;
}
#endif

void dedicated_console_start(void)
{
	static int started;
#ifndef _WIN32
	pthread_t thread;
#endif

	if (started)
		return;
	started = 1;
#ifndef _WIN32
	{
		struct sigaction action;

		memset(&action, 0, sizeof(action));
		action.sa_handler = quit_handler;
		sigemptyset(&action.sa_mask);
		sigaction(SIGTERM, &action, NULL);
		sigaction(SIGINT, &action, NULL);
		sigaction(SIGHUP, &action, NULL);
		/* (started in a shell's background, reading the terminal would stop
		the whole server: ignored, the read fails and there is no console) */
		signal(SIGTTIN, SIG_IGN);
	}
	if (pthread_create(&thread, NULL, console_thread, NULL) == 0)
		pthread_detach(thread);
#endif
}

int dedicated_console_quit_requested(void)
{
#ifdef _WIN32
	return 0;
#else
	return quit_signal != 0;
#endif
}

#else

/* (Android and the web have no dedicated server) */

void dedicated_console_print(const char *format, ...)
{
	(void)format;
}

const char *dedicated_console_file(int *found)
{
	if (found)
		*found = 0;
	return "";
}

int dedicated_console_next(char *text, int size, int *from_file)
{
	(void)text;
	(void)size;
	(void)from_file;
	return 0;
}

void dedicated_console_start(void)
{
}

int dedicated_console_quit_requested(void)
{
	return 0;
}

#endif
