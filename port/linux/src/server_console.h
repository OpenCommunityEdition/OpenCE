/*
SERVER_CONSOLE.H

A dedicated server's commands from its owner (server_console.c): the lines
of server.init_file, run once at the start, and of the standard input, read
without waiting each frame; and the console's output captured while a
command runs, for its reply. The commands themselves are the game's
(port/linux/game/server_commands.c), whatever the line came from: the HTTP
API and the remote console (server_http.c) run them too.
*/

#ifndef __HALO_LINUX_SERVER_CONSOLE_H
#define __HALO_LINUX_SERVER_CONSOLE_H

enum
{
	SERVER_COMMAND_LINE_SIZE = 512,
	/* the most of a command's output kept for its reply */
	SERVER_COMMAND_OUTPUT_SIZE = 64 * 1024,
};

/* who may run a command: each role may run its own and those below it */
enum
{
	_server_role_none = -1,
	_server_role_viewer = 0,
	_server_role_moderator,
	_server_role_admin,
	/* the standard input, the init file, the telnet console: whoever holds
	the process */
	_server_role_owner,
};

/* where a command came from */
enum
{
	_server_origin_init_file,
	_server_origin_standard_input,
	_server_origin_web,
	_server_origin_telnet,
};

/* reads the init file's lines (the first to be taken) */
void server_console_start(void);
/* a frame begins: so many lines of standard input are taken a frame */
void server_console_frame(void);
/* the next line to run (the init file's, then standard input's): 1 with it
in line, 0 if none now. A standard input that ends asks to quit as "quit"
when server.console_exit_on_eof says so */
int server_console_next(char *line, int size, int *origin);

/* the console's output (each line the terminal prints: terminal.c) */
void server_console_output(const char *text);
/* the output captured from now (nested captures nest: the outer one sees
nothing of the inner's) until server_console_capture_end, which copies it,
lines separated by newlines, into text (the output's to free: NULL if
none); 1 if it began */
int server_console_capture_begin(void);
char *server_console_capture_end(void);
/* frees what server_console_capture_end (and server_commands_run) gave: the
game's sources, whose free is the game's allocator's, call this */
void server_console_free(char *text);

/* the role a role's name ("viewer"...) is, _server_role_none if none */
int server_role_named(const char *name);
/* whether a command line holds a secret, so that its text is not logged:
sv_password, or sv_set of a password's setting or the publisher key's, in
any case */
int server_console_line_is_secret(const char *line);
const char *server_role_name(int role);

#endif
