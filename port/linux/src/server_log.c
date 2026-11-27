/*
SERVER_LOG.C

A dedicated server's log (server_log.h).
*/

#include "server_log.h"
#include "server_mode.h"

#if HALO_SERVER_BUILD

#include "server_json.h"
#include "posix_server.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static struct
{
	pthread_mutex_t lock;
	struct server_log_line lines[SERVER_LOG_RING_SIZE];
	unsigned long latest;
	int json;
	void (*listener)(struct server_log_line const *line);
} server_log_globals = { PTHREAD_MUTEX_INITIALIZER };

const char *server_log_level_name(int level)
{
	return level == _server_log_error ? "error" : level == _server_log_warning ? "warning" : "info";
}

void server_log_set_json(int json)
{
	server_log_globals.json = json;
}

void server_log_set_listener(void (*listener)(struct server_log_line const *line))
{
	pthread_mutex_lock(&server_log_globals.lock);
	server_log_globals.listener = listener;
	pthread_mutex_unlock(&server_log_globals.lock);
}

/* a line's fields, its text to printable characters (a player's name or
chat line never breaks a line of the log) */
static void server_log_fill(struct server_log_line *line, int level, const char *category, const char *text)
{
	int index;

	line->time = posix_server_unix_milliseconds();
	line->level = level;
	snprintf(line->category, sizeof(line->category), "%s", category ? category : "server");
	for (index = 0; text[index] && index < SERVER_LOG_TEXT_SIZE - 1; index++)
	{
		unsigned char character = (unsigned char)text[index];

		line->text[index] = character < 0x20 || character == 0x7F ? ' ' : (char)character;
	}
	line->text[index] = 0;
}

/* the line kept, and given to the listener; under the lock */
static struct server_log_line const *server_log_keep(int level, const char *category, const char *text)
{
	struct server_log_line *line;

	server_log_globals.latest++;
	line = &server_log_globals.lines[server_log_globals.latest % SERVER_LOG_RING_SIZE];
	line->number = server_log_globals.latest;
	server_log_fill(line, level, category, text);
	return line;
}

void server_log_ring(int level, const char *category, const char *text)
{
	struct server_log_line copy;
	void (*listener)(struct server_log_line const *line);

	pthread_mutex_lock(&server_log_globals.lock);
	copy = *server_log_keep(level, category, text);
	listener = server_log_globals.listener;
	pthread_mutex_unlock(&server_log_globals.lock);
	if (listener)
		listener(&copy);
}

/* a line on standard output, kept in the ring and given to the listener
when kept (else the standard output's alone: a secret's, which the web's
viewers of the log must not read) */
static void server_log_write(int level, const char *category, const char *text, int kept)
{
	char when[32];
	struct server_log_line copy;
	void (*listener)(struct server_log_line const *line) = NULL;

	pthread_mutex_lock(&server_log_globals.lock);
	if (kept)
	{
		copy = *server_log_keep(level, category, text);
		listener = server_log_globals.listener;
	}
	else
	{
		/* (as kept, with no number) */
		memset(&copy, 0, sizeof(copy));
		server_log_fill(&copy, level, category, text);
	}
	posix_server_utc_text((unsigned long)(copy.time / 1000), when, sizeof(when));
	if (server_log_globals.json)
	{
		struct server_json json;

		server_json_begin(&json);
		server_json_object_begin(&json, NULL);
		server_json_string(&json, "time", when);
		server_json_string(&json, "level", server_log_level_name(level));
		server_json_string(&json, "cat", copy.category);
		server_json_string(&json, "msg", copy.text);
		server_json_object_end(&json);
		fprintf(stdout, "%s\n", server_json_text(&json));
		server_json_free(&json);
	}
	else
	{
		fprintf(stdout, "%s %s %s: %s\n", when,
			level == _server_log_error ? "ERROR" : level == _server_log_warning ? "WARN" : "INFO", copy.category,
			copy.text);
	}
	fflush(stdout);
	pthread_mutex_unlock(&server_log_globals.lock);
	if (listener)
		listener(&copy);
}

void server_log(int level, const char *category, const char *format, ...)
{
	char text[SERVER_LOG_TEXT_SIZE];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	server_log_write(level, category, text, 1);
}

void server_log_private(int level, const char *category, const char *format, ...)
{
	char text[SERVER_LOG_TEXT_SIZE];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	server_log_write(level, category, text, 0);
}

int server_log_read(unsigned long after, struct server_log_line *lines, int maximum, unsigned long *next,
	unsigned long *missed)
{
	unsigned long oldest, number;
	int count = 0;

	pthread_mutex_lock(&server_log_globals.lock);
	/* (a cursor past the latest, as after a restart: from now on) */
	if (after > server_log_globals.latest)
		after = server_log_globals.latest;
	oldest = server_log_globals.latest >= SERVER_LOG_RING_SIZE ?
		server_log_globals.latest - SERVER_LOG_RING_SIZE + 1 : 1;
	*missed = after + 1 < oldest ? oldest - (after + 1) : 0;
	number = after + 1 < oldest ? oldest : after + 1;
	for (; number <= server_log_globals.latest && count < maximum; number++)
		lines[count++] = server_log_globals.lines[number % SERVER_LOG_RING_SIZE];
	*next = number - 1;
	pthread_mutex_unlock(&server_log_globals.lock);
	return count;
}

unsigned long server_log_latest(void)
{
	unsigned long latest;

	pthread_mutex_lock(&server_log_globals.lock);
	latest = server_log_globals.latest;
	pthread_mutex_unlock(&server_log_globals.lock);
	return latest;
}

#else

void server_log(int level, const char *category, const char *format, ...)
{
	(void)level;
	(void)category;
	(void)format;
}

void server_log_private(int level, const char *category, const char *format, ...)
{
	(void)level;
	(void)category;
	(void)format;
}

void server_log_ring(int level, const char *category, const char *text)
{
	(void)level;
	(void)category;
	(void)text;
}

int server_log_read(unsigned long after, struct server_log_line *lines, int maximum, unsigned long *next,
	unsigned long *missed)
{
	(void)after;
	(void)lines;
	(void)maximum;
	*next = 0;
	*missed = 0;
	return 0;
}

unsigned long server_log_latest(void)
{
	return 0;
}

void server_log_set_listener(void (*listener)(struct server_log_line const *line))
{
	(void)listener;
}

void server_log_set_json(int json)
{
	(void)json;
}

const char *server_log_level_name(int level)
{
	(void)level;
	return "info";
}

#endif
