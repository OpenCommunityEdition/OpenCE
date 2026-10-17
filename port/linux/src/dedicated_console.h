/*
DEDICATED_CONSOLE.H

A dedicated server's console (dedicated_console.c): the commands of its
command file (server.init_file), read once at the start, then the lines typed
on its standard input (Linux), handed to the game
(port/linux/game/dedicated_server.c) one at a time; and its output, a line at a
time on the standard output. Nothing here runs a command: the game's side
knows only its own sv_* commands. The Linux and Windows builds; elsewhere none
of it does anything.
*/

#ifndef __HALO_LINUX_DEDICATED_CONSOLE_H
#define __HALO_LINUX_DEDICATED_CONSOLE_H

enum
{
	/* a command's longest line, with its end (the rest of a longer one is
	dropped) */
	DEDICATED_CONSOLE_LINE_SIZE = 256,
};

/* a line on the standard output (and in the log) */
void dedicated_console_print(const char *format, ...);
/* the command file's path (server.init_file, in the data folder unless a
full path), and whether it was there: read on the first question */
const char *dedicated_console_file(int *found);
/* the next command: the file's, all of them, then the console's; 1 with
the line in text (from_file says which), 0 if none waits */
int dedicated_console_next(char *text, int size, int *from_file);
/* the console starts reading the standard input (once: a standard input
that is not there, or closed, gives no commands), and Ctrl+C, a terminal
closed and a service stopped ask the server to stop */
void dedicated_console_start(void);
/* whether a signal asked the server to stop: it stops at its next frame,
as `quit` does, so its listing in the server browser is closed */
int dedicated_console_quit_requested(void);

#endif
