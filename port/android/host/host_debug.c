/*
HOST_DEBUG.C

A sampler for finding where the guest spends its time (or hangs) on a
device without root, where debuggerd cannot attach: with sample_seconds in
config.toml's [debug], every guest thread is interrupted that often and its program
counter, link register (none on x86-64) and frame chain are written to the
log (host_arch.c reads them). The addresses symbolize against the build's
halo_guest.elf (llvm-symbolizer --obj=build/android/halo_guest.elf 0x...).
*/

#include "host.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAXIMUM_THREADS 64
#define SAMPLE_SIGNAL SIGURG

static pid_t guest_threads[MAXIMUM_THREADS];
static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;

void host_debug_thread_started(void)
{
	static int overflow_logged;
	int index;

	pthread_mutex_lock(&threads_lock);
	for (index = 0; index < MAXIMUM_THREADS; index++)
	{
		if (!guest_threads[index])
		{
			guest_threads[index] = gettid();
			break;
		}
	}
	if (index == MAXIMUM_THREADS && !overflow_logged)
	{
		overflow_logged = 1;
		host_logf(HOST_LOG_WARN, "more than %d guest threads: the sampler leaves the others out", MAXIMUM_THREADS);
	}
	pthread_mutex_unlock(&threads_lock);
}

void host_debug_thread_exited(void)
{
	pid_t self = gettid();
	int index;

	pthread_mutex_lock(&threads_lock);
	for (index = 0; index < MAXIMUM_THREADS; index++)
	{
		if (guest_threads[index] == self)
			guest_threads[index] = 0;
	}
	pthread_mutex_unlock(&threads_lock);
}

/* (a signal handler: it formats into its own buffer, the log's sink
neither locks nor uses stdio, and the frame walk copies each record without
faulting, whatever the frame pointer register held: host_arch.c) */
static void sample_handler(int signal_number, siginfo_t *information, void *context)
{
	char line[512];
	uint64_t pc, sp, fp, lr;
	uint64_t returns[12];
	int length, count, index;
	int saved_errno = errno;

	(void)signal_number;
	(void)information;
	host_arch_context(context, &pc, &sp, &fp, &lr);
	length = snprintf(line, sizeof(line), "sample tid %d: pc %llx lr %llx", (int)gettid(),
		(unsigned long long)pc, (unsigned long long)lr);
	count = host_arch_walk_frames(fp, host_arch_read_memory, returns, (int)(sizeof(returns) / sizeof(returns[0])));
	for (index = 0; index < count && length > 0 && (size_t)length < sizeof(line); index++)
		length += snprintf(line + length, sizeof(line) - length, " %llx", (unsigned long long)returns[index]);
	host_log(HOST_LOG_INFO, line);
	/* (the interrupted code may be between a call and its errno check) */
	errno = saved_errno;
}

/* the time between samples */
static struct timespec sample_interval;

static void *sampler(void *context)
{
	(void)context;
	for (;;)
	{
		pid_t threads[MAXIMUM_THREADS];
		struct timespec remaining = sample_interval;
		int index;

		while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR)
			;
		pthread_mutex_lock(&threads_lock);
		memcpy(threads, guest_threads, sizeof(threads));
		pthread_mutex_unlock(&threads_lock);
		for (index = 0; index < MAXIMUM_THREADS; index++)
		{
			if (threads[index])
				syscall(SYS_tgkill, getpid(), threads[index], SAMPLE_SIGNAL);
		}
	}
	return NULL;
}

void host_debug_start_sampler(const char *setting)
{
	struct sigaction action;
	pthread_t thread;
	/* (a real number of seconds, as the setting is: 0.5 is twice a second) */
	double seconds = setting ? strtod(setting, NULL) : 0.0;

	if (!(seconds > 0.0))
		return;
	/* (at most a hundred samples a second, and at least one a day) */
	if (seconds < 0.01)
		seconds = 0.01;
	if (seconds > 86400.0)
		seconds = 86400.0;
	sample_interval.tv_sec = (time_t)seconds;
	sample_interval.tv_nsec = (long)((seconds - (double)sample_interval.tv_sec) * 1e9);
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = sample_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&action.sa_mask);
	sigaction(SAMPLE_SIGNAL, &action, NULL);
	if (pthread_create(&thread, NULL, sampler, NULL) == 0)
		pthread_detach(thread);
	host_logf(HOST_LOG_INFO, "sampling guest threads every %g s", seconds);
}
