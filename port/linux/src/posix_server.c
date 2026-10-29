/*
POSIX_SERVER.C

A dedicated server's signals, standard input and output, and the process's
use of memory and processor (posix_server.h), on Linux. None on Android and
the web, which have no dedicated server.
*/

#include "posix_server.h"

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* (the bits set by the handler and taken by the game's thread, each
atomically: a bit set between the game's read and its clear is kept) */
static volatile sig_atomic_t server_signals;
static volatile sig_atomic_t server_quit_signal;
static volatile sig_atomic_t server_quit_count;
static volatile sig_atomic_t server_deadline_code;

static void server_signal_handler(int number)
{
	switch (number)
	{
	case SIGTERM:
	case SIGINT:
		/* (a second one: at once) */
		if (server_quit_count++)
			_exit(128 + number);
		server_quit_signal = number;
		__atomic_fetch_or(&server_signals, _posix_server_signal_quit, __ATOMIC_SEQ_CST);
		break;
	case SIGHUP:
		__atomic_fetch_or(&server_signals, _posix_server_signal_reload, __ATOMIC_SEQ_CST);
		break;
	case SIGUSR1:
		__atomic_fetch_or(&server_signals, _posix_server_signal_status, __ATOMIC_SEQ_CST);
		break;
	case SIGALRM:
		_exit(server_quit_signal ? 128 + server_quit_signal : server_deadline_code);
		break;
	}
}

void posix_server_signals_install(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_handler = server_signal_handler;
	/* (the others held while one is handled: its counts and bits are the
	handler's alone at a time) */
	sigfillset(&action.sa_mask);
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
	sigaction(SIGUSR1, &action, NULL);
	sigaction(SIGALRM, &action, NULL);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
}

int posix_server_signals_take(int *quit_signal)
{
	int taken = __atomic_exchange_n(&server_signals, 0, __ATOMIC_SEQ_CST);

	if (quit_signal)
		*quit_signal = server_quit_signal;
	return taken;
}

void posix_server_quit_deadline(int seconds, int exit_code)
{
	server_deadline_code = exit_code;
	alarm(seconds > 0 ? (unsigned)seconds : 1);
}

void posix_server_stdout_line_buffered(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
}

int posix_server_stdin_is_terminal(void)
{
	return isatty(STDIN_FILENO);
}

int posix_server_stdin_read(char *buffer, int size)
{
	struct pollfd descriptor;
	ssize_t count;

	descriptor.fd = STDIN_FILENO;
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	if (poll(&descriptor, 1, 0) <= 0)
		return 0;
	if (descriptor.revents & POLLNVAL)
		return -1;
	if (!(descriptor.revents & (POLLIN | POLLHUP | POLLERR)))
		return 0;
	/* (read(2), not stdio: stdio's lock on stdin would hold exit()'s flush) */
	count = read(STDIN_FILENO, buffer, (size_t)size);
	if (count < 0)
		return errno == EAGAIN || errno == EINTR ? 0 : -1;
	return count == 0 ? -1 : (int)count;
}

unsigned long posix_server_resident_bytes(void)
{
	FILE *file = fopen("/proc/self/statm", "r");
	unsigned long pages = 0, resident = 0;

	if (!file)
		return 0;
	if (fscanf(file, "%lu %lu", &pages, &resident) != 2)
		resident = 0;
	fclose(file);
	return resident * (unsigned long)sysconf(_SC_PAGESIZE);
}

unsigned long posix_server_cpu_milliseconds(void)
{
	struct rusage usage;

	if (getrusage(RUSAGE_SELF, &usage) != 0)
		return 0;
	return (unsigned long)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000UL +
		(unsigned long)(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000UL;
}

int posix_server_rename(const char *from, const char *to)
{
	return rename(from, to) == 0;
}

int posix_server_write_file(const char *path, const char *text, unsigned long size)
{
	char temporary[4200];
	FILE *file;
	int written;

	if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary))
		return 0;
	file = fopen(temporary, "wb");
	if (!file)
		return 0;
	written = fwrite(text, 1, size, file) == size;
	if (fclose(file) != 0 || !written)
	{
		unlink(temporary);
		return 0;
	}
	return posix_server_rename(temporary, path);
}

int posix_server_private_file(const char *path)
{
	return chmod(path, S_IRUSR | S_IWUSR) == 0;
}

int posix_server_write_private_file(const char *path, const void *data, unsigned long size)
{
	const char *bytes = data;
	unsigned long written = 0;
	int descriptor = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR);

	if (descriptor < 0)
		return 0;
	/* (a file that was there keeps its mode: made its owner's too) */
	if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0)
	{
		close(descriptor);
		return 0;
	}
	while (written < size)
	{
		ssize_t count = write(descriptor, bytes + written, (size_t)(size - written));

		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			break;
		written += (unsigned long)count;
	}
	return close(descriptor) == 0 && written == size;
}

int posix_server_remove(const char *path)
{
	return unlink(path) == 0;
}

unsigned long posix_server_unix_seconds(void)
{
	return (unsigned long)time(NULL);
}

long long posix_server_unix_milliseconds(void)
{
	struct timespec now;

	clock_gettime(CLOCK_REALTIME, &now);
	return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

void posix_server_utc_text(unsigned long seconds, char *text, int size)
{
	time_t when = (time_t)seconds;
	struct tm utc;

	if (!gmtime_r(&when, &utc) || !strftime(text, (size_t)size, "%Y-%m-%dT%H:%M:%SZ", &utc))
		snprintf(text, (size_t)size, "%lu", seconds);
}

void posix_server_local_text(unsigned long seconds, char *text, int size)
{
	time_t when = (time_t)seconds;
	struct tm local;

	if (!localtime_r(&when, &local) || !strftime(text, (size_t)size, "%Y-%m-%d %H:%M:%S", &local))
		snprintf(text, (size_t)size, "%lu", seconds);
}

#endif
