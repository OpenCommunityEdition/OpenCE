/*
SERVER_MODE.H

A dedicated server (halo --server: server_mode.c; docs/dedicated_server.md):
whether this run is one, its start before the game's, its signals, its
frames' pace and its exit codes. The Linux and Windows builds have it; the
Android and web builds compile the server's files to nothing
(HALO_SERVER_BUILD 0), and halo_server_mode() is always 0 there.
*/

#ifndef __HALO_LINUX_SERVER_MODE_H
#define __HALO_LINUX_SERVER_MODE_H

#if defined(HALO_ANDROID) || defined(HALO_WEB)
#define HALO_SERVER_BUILD 0
#else
#define HALO_SERVER_BUILD 1
#endif

/* the exit codes (docs/dedicated_server.md) */
enum
{
	SERVER_EXIT_STOPPED = 0,
	SERVER_EXIT_FATAL = 1,
	SERVER_EXIT_NO_GAME_DATA = 2,
	SERVER_EXIT_CONFIGURATION = 3,
	SERVER_EXIT_PORT_IN_USE = 4,
	SERVER_EXIT_RESTART = 5,
};

/* whether this run is a dedicated server */
int halo_server_mode(void);

/* main()'s first (source/shell/shell_xbox.c): --version, --help, --keygen
and --sign answered (and the process ended), and a dedicated server's
start: its signals, its game data and settings checked (ending it with
exit code 2 or 3 if they will not do), its web port bound (4 if taken) */
void server_mode_start(void);

/* the signals that came since the last call (posix_server.h's bits); the
quitting one's number in quit_signal */
int server_mode_signals(int *quit_signal);

/* waits out the rest of the frame (server.frame_rate a second, or 10 a
second while idle): before the frame's time is measured (main.c) */
void server_mode_pace_frame(void);
/* whether the server is idle (no other machine connected): its frames
slower */
void server_mode_set_idle(int idle);
/* the frames' time (milliseconds): the longest of the last minute's and the
mean, for the health line and the metrics */
void server_mode_frame_times(double *longest, double *mean);

/* the game's fatal error (halt_and_catch_fire, main_loop_of_death): its
last lines logged, and the process ended with exit code 1 */
void server_mode_fatal(const char *text);
/* ends the process with the code, the logs flushed; the exit hooks
(server_mode_at_exit) run first */
void server_mode_exit(int code);
/* a function run as the process ends by server_mode_exit (the listings'
withdrawal), the last added first */
void server_mode_at_exit(void (*hook)(void));

/* the build, "build-<n>" ("build-0" for one not built by CI) */
const char *server_mode_build(void);
/* the seconds since the server started */
double server_mode_uptime(void);
/* when the server started (seconds since 1970; 0 for none), and the
process's id */
unsigned long server_mode_started(void);
unsigned long server_mode_process_id(void);

#endif
