/*
SERVER_LOG.H

A dedicated server's log (server_log.c): its lines on standard output, as
text or JSON (server.log_format), each also kept in a ring of the last
SERVER_LOG_RING_SIZE that the API, the remote console and the web page read
from (each line numbered, the next asked for after the last one seen). Any
thread may write.

The categories: "server", "lobby", "game", "player", "chat", "moderation",
"net", "master", "console" (commands and their answers), "health",
"platform" (the platform layer's lines, kept in the ring alone: they go to
the standard error).
*/

#ifndef __HALO_LINUX_SERVER_LOG_H
#define __HALO_LINUX_SERVER_LOG_H

enum
{
	_server_log_info,
	_server_log_warning,
	_server_log_error,
};

enum
{
	SERVER_LOG_RING_SIZE = 2048,
	SERVER_LOG_CATEGORY_SIZE = 16,
	SERVER_LOG_TEXT_SIZE = 384,
};

struct server_log_line
{
	unsigned long number;
	long long time;
	int level;
	char category[SERVER_LOG_CATEGORY_SIZE];
	char text[SERVER_LOG_TEXT_SIZE];
};

/* a line on standard output and in the ring */
void server_log(int level, const char *category, const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 3, 4)))
#endif
	;
/* a line on standard output alone, kept out of the ring the web's viewers
read (/api/log, sv_log) and of the remote console's: a secret's */
void server_log_private(int level, const char *category, const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 3, 4)))
#endif
	;
/* a line in the ring alone */
void server_log_ring(int level, const char *category, const char *text);
/* the lines numbered after after (0: from the oldest), up to maximum; how
many. next is the number to ask after next time, missed how many left the
ring before they were read */
int server_log_read(unsigned long after, struct server_log_line *lines, int maximum, unsigned long *next,
	unsigned long *missed);
/* the number of the latest line (0: none yet) */
unsigned long server_log_latest(void);
/* each line as it is written (the remote console's log events), on the
thread that wrote it; NULL for none */
void server_log_set_listener(void (*listener)(struct server_log_line const *line));
/* standard output's lines written as JSON (server.log_format "json") */
void server_log_set_json(int json);
/* the level's name: "info", "warning", "error" */
const char *server_log_level_name(int level);

#endif
