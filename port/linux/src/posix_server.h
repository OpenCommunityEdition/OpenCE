/*
POSIX_SERVER.H

What a dedicated server (server_mode.c) needs of the system beyond posix.h:
its signals, its standard input and output, and what the process uses.
posix_server.c does it on Linux, port/windows/src/win32_server.c on Windows
(Ctrl+C and the console's close for the signals). The Android and web builds
have no dedicated server, and none of these.

Only integers and strings cross here (the two sides are compiled with
different structure layouts), as in posix.h.
*/

#ifndef __HALO_LINUX_POSIX_SERVER_H
#define __HALO_LINUX_POSIX_SERVER_H

enum
{
	/* posix_server_signals_take's bits */
	_posix_server_signal_quit = 1 << 0,
	_posix_server_signal_reload = 1 << 1,
	_posix_server_signal_status = 1 << 2,
};

/* SIGTERM and SIGINT ask to quit (a second one ends the process at once,
with 128 + its number), SIGHUP to reload, SIGUSR1 for the status; SIGPIPE
and SIGTTIN are ignored */
void posix_server_signals_install(void);
/* the signals that came since the last call (the bits above), and the
number of the last quitting one (0 if none) */
int posix_server_signals_take(int *quit_signal);
/* ends the process (128 + the quitting signal, else exit_code) unless it
has ended within seconds: a shutdown that hangs (a map loading) */
void posix_server_quit_deadline(int seconds, int exit_code);

/* standard output written a line at a time (a panel's pipe shows each at
once) */
void posix_server_stdout_line_buffered(void);
/* whether standard input is a terminal */
int posix_server_stdin_is_terminal(void);
/* what standard input has without waiting: the bytes read (up to size), 0
for none yet, -1 at its end (closed, or there is none) */
int posix_server_stdin_read(char *buffer, int size);

/* the process's resident memory, in bytes, and its processor time (user
and system), in milliseconds */
unsigned long posix_server_resident_bytes(void);
unsigned long posix_server_cpu_milliseconds(void);

/* renames from to to, replacing to (a file written whole, then put in
place); 1 on success */
int posix_server_rename(const char *from, const char *to);
/* the file at path (the host's, not the game's Xbox path) written whole:
size bytes of text into path.tmp, put in place of path; 1 if it was */
int posix_server_write_file(const char *path, const char *text, unsigned long size);
/* makes the file readable and writable by its owner alone (a key); 1 on
success */
int posix_server_private_file(const char *path);
/* the file at path written whole (size bytes of data), readable and
writable by its owner alone from its creation (a key): created so, or made
so if it was there; 1 if it was */
int posix_server_write_private_file(const char *path, const void *data, unsigned long size);
/* removes a file; 1 on success */
int posix_server_remove(const char *path);
/* the seconds since the Unix epoch, and the milliseconds */
unsigned long posix_server_unix_seconds(void);
long long posix_server_unix_milliseconds(void);
/* the time as UTC text: "2026-10-19T18:03:11Z" */
void posix_server_utc_text(unsigned long seconds, char *text, int size);
/* local time as "2026-10-19 18:03:11" (bans.txt's) */
void posix_server_local_text(unsigned long seconds, char *text, int size);

#endif
