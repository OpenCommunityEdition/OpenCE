/*
ERRORS.C

symbols in this file:
0007D860 0010:
	_errors_dispose (0000)
0007D870 0010:
	_errors_output_to_debug_file (0000)
0007D880 0010:
	_errors_overflow_suppression_enable (0000)
0007D890 0010:
	_error_get (0000)
0007D8A0 0130:
	_write_to_error_file (0000)
0007D9D0 0010:
	_code_0007d9d0 (0000)
0007D9E0 0020:
	_errors_initialize (0000)
0007DA00 0270:
	_error (0000)
0007DC70 0020:
	_errors_handle (0000)
0007DC90 0010:
	_errors_clear (0000)
00257C68 0015:
	??_C@_0BF@MNPBN@?$DMTIME?5UNAVAILABLE?$DO?5?5?$AA@ (0000)
00257C80 0020:
	??_C@_0CA@LIAFHJOP@?$CF02d?4?$CF02d?4?$CF02d?5?$CF02d?3?$CF02d?3?$CF02d?5?5?$AA@ (0000)
00257CA0 000d:
	??_C@_0N@MOPBLMMO@d?3?2debug?4txt?$AA@ (0000)
00257CB0 0018:
	??_C@_0BI@PFJPDCFK@reference?5address?3?5?$CFx?$AN?6?$AA@ (0000)
00257CC8 0019:
	??_C@_0BJ@KFNJAEPH@reference?5function?3?5?$CFs?$AN?6?$AA@ (0000)
00257CE4 0015:
	??_C@_0BF@OBFANCCF@_write_to_error_file?$AA@ (0000)
00257D00 0054:
	??_C@_0FE@EJMLAPFK@halobeta?5xbox?501?401?414?42342?$CICACH@ (0000)
00257D54 0005:
	??_C@_04FMMHFHOO@?$AN?6?$AN?6?$AA@ (0000)
00257D60 0047:
	??_C@_0EH@CIPINKCN@prefix_size?5?$CL?5copy_size?5?$CL?5new_si@ (0000)
00257DA8 0023:
	??_C@_0CD@BCANCLDH@?$FL?4?4?4too?5many?5errors?5to?5print?4?4?4?$FN@ (0000)
00257DCC 002c:
	??_C@_0CM@FGNANMFL@too?5many?5errors?0?5only?5printing?5t@ (0000)
00257DF8 003b:
	??_C@_0DL@DEAPBIIL@priority?$DO?$DN0?5?$CG?$CG?5priority?$DMNUMBER_O@ (0000)
00257E34 0020:
	??_C@_0CA@JEDAOIGL@c?3?2halo?2SOURCE?2cseries?2errors?4c?$AA@ (0000)
002DCD2C 0001:
	_data_002dcd2c (0000)
0031DF2C 0010:
	_bss_0031df2c (0000)
	_find_all_fucked_up_shit (0008)
	_fucked_up_shit_count (000c)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "errors.h"
#include "interface/terminal.h"

#include <stdarg.h>
#include <time.h>

/* ---------- constants */

/* port: how big debug.txt grows before it is rolled to debug.old.txt and
begun again (write_to_debug_file) */
#define DEBUG_FILE_PORT_MAXIMUM_SIZE (64L * 1024 * 1024)

/* ---------- macros */

/* ---------- structures */

struct error_suppression_globals
{
	long last_error_time;
	long error_count;
};

/* ---------- globals */

static boolean data_002dcd2c = TRUE;
static struct error_suppression_globals bss_0031df2c = { 0, 0 };
boolean find_all_fucked_up_shit = FALSE;
long fucked_up_shit_count = 0;

/* ---------- public code */

void errors_dispose(
	void)
{
	stack_walk_dispose();

	return;
}

void errors_output_to_debug_file(
	boolean output_to_debug_file)
{
	error_globals.output_to_debug_file = output_to_debug_file;

	return;
}

void errors_overflow_suppression_enable(
	boolean overflow_suppression)
{
	error_globals.overflow_suppression = overflow_suppression;

	return;
}

char *error_get(
	void)
{
	return error_globals.message_buffer;
}

/* The native builds keep the log open and flush each line: opening and
closing it per line takes milliseconds on Windows, and a host logs
thousands of lines when a hundred machines join, load or leave. Each line is
one write, under a lock, so lines from several threads do not interleave.

port: A line written again as it was (the same warning each tick) is
counted, not written, until a different one comes ("last line repeated N
times"). Past DEBUG_FILE_PORT_MAXIMUM_SIZE the file is rolled to
debug.old.txt (the one before that let go) and begun again: a game left
running for days wrote without end. */
static void debug_file_port_write(
	FILE *file,
	char const *prefix,
	char const *string);

static FILE *debug_file_port = NULL;
static long debug_file_port_size = 0;
static char debug_file_port_last_line[1024];
static unsigned long debug_file_port_repeat_count = 0;
static volatile long debug_file_port_lock = 0;
/* (write_to_error_file's header again, atop the file begun again) */
static boolean debug_file_port_rolled = FALSE;

static void write_to_debug_file(
	char const *string,
	boolean date)
{
	FILE *file;
	char prefix[32];
	long spin;

	/* (a moment's wait for another thread's line; a crash report written
	while this thread held the lock goes on without it) */
	for (spin = 0; spin < 50000000 && __sync_lock_test_and_set(&debug_file_port_lock, 1); spin++)
	{
	}

	/* port: a dedicated server's debug.txt also on the standard error
	(server.log_debug_echo), and there alone when the file cannot be written
	(a read-only data root) */
	if (halo_server_mode() && halo_server_debug_echo())
		fprintf(stderr, "debug: %s%s", string, string[0] && string[csstrlen(string) - 1] == '\n' ? "" : "\n");
	file = debug_file_port;
	if (!file)
	{
		file = fopen("d:\\debug.txt", "a+b");
		if (!file)
		{
			if (halo_server_mode() && !halo_server_debug_echo())
				fprintf(stderr, "debug: %s%s", string, string[0] && string[csstrlen(string) - 1] == '\n' ? "" : "\n");
			__sync_lock_release(&debug_file_port_lock);
			return;
		}
		fseek(file, 0, SEEK_END);
		debug_file_port_size = ftell(file);
		if (debug_file_port_size < 0)
		{
			/* (too big to say, as the 32-bit builds find one past 2 GB:
			full) */
			debug_file_port_size = DEBUG_FILE_PORT_MAXIMUM_SIZE;
		}
		debug_file_port = file;
	}

	if (!csstrcmp(string, debug_file_port_last_line))
	{
		debug_file_port_repeat_count++;
		__sync_lock_release(&debug_file_port_lock);
		return;
	}

	prefix[0] = 0;
	if (date || debug_file_port_repeat_count)
	{
		long timeptr;
		struct tm *_time;

		time(&timeptr);
		_time = localtime(&timeptr);
		if (_time)
		{
			sprintf(
				prefix,
				"%02d.%02d.%02d %02d:%02d:%02d  ",
				_time->tm_mon + 1,
				_time->tm_mday,
				_time->tm_year % 100,
				_time->tm_hour,
				_time->tm_min,
				_time->tm_sec);
		}
		else
		{
			csstrcpy(prefix, "<TIME UNAVAILABLE>  ");
		}
	}

	if (debug_file_port_repeat_count)
	{
		char repeated[64];

		sprintf(repeated, "last line repeated %lu times\r\n", debug_file_port_repeat_count);
		debug_file_port_write(file, prefix, repeated);
		debug_file_port_repeat_count = 0;
		file = debug_file_port;
	}
	if (file)
	{
		long index;

		for (index = 0; index < (long)sizeof(debug_file_port_last_line) - 1 && string[index]; index++)
		{
			debug_file_port_last_line[index] = string[index];
		}
		debug_file_port_last_line[index] = 0;
		debug_file_port_write(file, date ? prefix : "", string);
	}

	__sync_lock_release(&debug_file_port_lock);
}

/* port: a line to debug.txt (under write_to_debug_file's lock), rolling the
file to debug.old.txt first when it is full */
static void debug_file_port_write(
	FILE *file,
	char const *prefix,
	char const *string)
{
	static char const rolled[] = "debug.txt was full: its earlier lines are in debug.old.txt\r\n";

	if (debug_file_port_size >= DEBUG_FILE_PORT_MAXIMUM_SIZE)
	{
		fclose(file);
		debug_file_port = NULL;
		remove("d:\\debug.old.txt");
		/* (or, where it cannot be renamed, begun again in place) */
		file = rename("d:\\debug.txt", "d:\\debug.old.txt") ?
			fopen("d:\\debug.txt", "w+b") :
			fopen("d:\\debug.txt", "a+b");
		if (!file)
		{
			return;
		}
		debug_file_port = file;
		debug_file_port_size = 0;
		debug_file_port_rolled = TRUE;
		fprintf(file, "%s%s", prefix, rolled);
		debug_file_port_size += (long)(csstrlen(prefix) + csstrlen(rolled));
	}
	fprintf(file, "%s%s", prefix, string);
	fflush(file);
	debug_file_port_size += (long)(csstrlen(prefix) + csstrlen(string));
#ifdef HALO_WEB
	/* port: and to the page's log (port/web/README.md) */
	fputs(string, stderr);
#endif
}

void write_to_error_file(
	char *string,
	boolean date)
{
	char line[1024];
	long time_value;

	/* port: the header again in a debug.txt begun again (its reference
	address places a crash report's addresses) */
	if (debug_file_port_rolled)
	{
		debug_file_port_rolled = FALSE;
		data_002dcd2c = TRUE;
	}
	if (data_002dcd2c)
	{
		data_002dcd2c = FALSE;
		write_to_error_file("\r\n\r\n", FALSE);
		write_to_error_file("halobeta xbox 01.01.14.2342(CACHE) ----------------------------------------------\r\n", TRUE);
		sprintf(line, "reference function: %s\r\n", "_write_to_error_file");
		write_to_error_file(line, TRUE);
		sprintf(line, "reference address: %x\r\n", write_to_error_file);
		write_to_error_file(line, TRUE);
	}

	if (error_globals.output_to_debug_file)
	{
		write_to_debug_file(string, date);
	}

	return;
}

static void reset_error_state(
	void)
{
	error_globals.delayed = FALSE;
	error_globals.message_buffer_size = 0;

	return;
}

void errors_initialize(
	void)
{
	error_globals.output_to_debug_file = TRUE;
	error_globals.overflow_suppression = TRUE;
	reset_error_state();
	stack_walk_initialize();

	return;
}

void error(
	short priority,
	const char *format,
	...)
{
	char string[1024];
	char *newline;
	long new_size;

	match_assert(
		"c:\\halo\\SOURCE\\cseries\\errors.c",
		0x61,
		priority>=0 && priority<NUMBER_OF_ERROR_MESSAGE_PRIORITIES);

	if (error_globals.overflow_suppression && priority == _error_silent)
	{
		long time = system_milliseconds();

		if ((unsigned long)(time - bss_0031df2c.last_error_time) > 900)
		{
			bss_0031df2c.error_count = 0;
		}
		bss_0031df2c.last_error_time = time;
		if (bss_0031df2c.error_count == 10 && terminal_shows(_terminal_message_chatter))
		{
			terminal_printf(
				global_real_argb_white,
				"too many errors, only printing to debug.txt");
		}
		bss_0031df2c.error_count++;
		if (bss_0031df2c.error_count >= 10)
		{
			priority = _error_log;
		}
	}

	if (!error_globals.recursion_lock)
	{
		error_globals.recursion_lock = TRUE;
		if (priority == _error_delayed)
		{
			error_globals.delayed = TRUE;
		}

		if (format)
		{
			va_list argument_list;

			va_start(argument_list, format);
			/* port: no longer than the buffer, with room for the "\r\n" */
			vsnprintf(string, NUMBEROF(string)-2, format, argument_list);
			va_end(argument_list);
			csstrcat(string, "\r\n");

			/* (port: the file first: printing used to halt on a long line
			while this call still held the lock, and the reason never reached
			debug.txt) */
			write_to_error_file(string, TRUE);

			/* (port: on screen as config.toml's game.console_log says: an
			assert that stops the game, and what a command someone typed
			logs (its answer), always; the rest, the game's chatter.
			debug.txt has it whatever that is) */
			if (priority != _error_log &&
				terminal_shows(terminal_command_running || !strncmp(string, "EXCEPTION halt", 14) ||
					!strncmp(string, "EXCEPTION assert", 16) ? _terminal_message_serious : _terminal_message_chatter))
			{
				terminal_printf(global_real_argb_white, "%s", string);
			}

			new_size = csstrlen(string);
			if (error_globals.message_buffer_size+new_size >= ERROR_MESSAGE_BUFFER_MAXIMUM_SIZE)
			{
				long old_size;
				long copy_size;
				long prefix_size = csstrlen("[...too many errors to print...]\r\n");
				long offset = prefix_size+1024+new_size;

				if (offset < 0)
				{
					offset = 0;
				}
				else if (offset > error_globals.message_buffer_size-1)
				{
					offset = error_globals.message_buffer_size-1;
				}

				newline = strchr(error_globals.message_buffer+offset, '\n');
				if (!newline)
				{
					old_size = error_globals.message_buffer_size;
				}
				else
				{
					old_size = newline-error_globals.message_buffer+1;
				}
				copy_size = error_globals.message_buffer_size-old_size;
				match_assert(
					"c:\\halo\\SOURCE\\cseries\\errors.c",
					0xBF,
					prefix_size + copy_size + new_size < ERROR_MESSAGE_BUFFER_MAXIMUM_SIZE);
				csstrncpy(
					error_globals.message_buffer,
					"[...too many errors to print...]\r\n",
					prefix_size);
				if (copy_size > 0)
				{
					csmemmove(
						error_globals.message_buffer+prefix_size,
						newline,
						copy_size);
				}

				error_globals.message_buffer[prefix_size+copy_size] = 0;
				error_globals.message_buffer_size = (short)(prefix_size+copy_size);
			}

			if (error_globals.message_buffer_size+new_size < ERROR_MESSAGE_BUFFER_MAXIMUM_SIZE)
			{
				csstrcpy(
					error_globals.message_buffer+error_globals.message_buffer_size,
					string);
				error_globals.message_buffer_size += (short)new_size;
			}
		}

		if (priority == _error_immediate)
		{
			system_exit(-4998);
		}
		error_globals.recursion_lock = FALSE;
	}

	return;
}

boolean errors_handle(
	void)
{
	boolean delayed = error_globals.delayed;

	reset_error_state();

	return delayed;
}

void errors_clear(
	void)
{
	reset_error_state();

	return;
}

/* ---------- private code */
