/*
HOST_DEBUG.C

A sampler for finding where the guest spends its time (or hangs) without a
debugger (lldb knows nothing of the ELF guest under Rosetta): with
sample_seconds in config.toml's [debug], every guest thread is interrupted
that often and its program counter and frame chain are logged. The
addresses symbolize against build/macos/halo_guest.elf (llvm-symbolizer
--obj=build/macos/halo_guest.elf 0x...).
*/

#include "host.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define MAXIMUM_THREADS 64
#define SAMPLE_SIGNAL SIGURG

static pthread_t guest_threads[MAXIMUM_THREADS];
static int guest_thread_used[MAXIMUM_THREADS];
static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;

void host_debug_thread_started(void)
{
	int index;

	pthread_mutex_lock(&threads_lock);
	for (index = 0; index < MAXIMUM_THREADS; index++)
	{
		if (!guest_thread_used[index])
		{
			guest_threads[index] = pthread_self();
			guest_thread_used[index] = 1;
			break;
		}
	}
	pthread_mutex_unlock(&threads_lock);
}

void host_debug_thread_exited(void)
{
	pthread_t self = pthread_self();
	int index;

	pthread_mutex_lock(&threads_lock);
	for (index = 0; index < MAXIMUM_THREADS; index++)
	{
		if (guest_thread_used[index] && pthread_equal(guest_threads[index], self))
			guest_thread_used[index] = 0;
	}
	pthread_mutex_unlock(&threads_lock);
}

static void sample_handler(int signal_number, siginfo_t *information, void *context)
{
	const ucontext_t *ucontext = context;
	const _STRUCT_X86_THREAD_STATE64 *registers = &ucontext->uc_mcontext->__ss;
	char line[512];
	int length, depth;
	uint64_t fp = registers->__rbp;

	(void)signal_number;
	(void)information;
	length = snprintf(line, sizeof(line), "sample thread %d: rip %llx", host_thread_id(),
		(unsigned long long)registers->__rip);
	/* (x32 frame records: [rbp] the caller's rbp, [rbp + 8] the return
	address) */
	for (depth = 0; depth < 12 && fp && fp < 0x100000000ull && !(fp & 7) && host_low_owns(fp, 16); depth++)
	{
		const uint64_t *frame = (const uint64_t *)fp;

		if (length >= (int)sizeof(line) - 24)
			break;
		length += snprintf(line + length, sizeof(line) - length, " %llx", (unsigned long long)frame[1]);
		if (frame[0] <= fp)
			break;
		fp = frame[0];
	}
	host_logf(HOST_LOG_INFO, "%s", line);
}

static void *sampler(void *context)
{
	useconds_t interval = (useconds_t)(uintptr_t)context;

	host_signal_stack_install();
	pthread_setname_np("halo sampler");
	for (;;)
	{
		pthread_t threads[MAXIMUM_THREADS];
		int used[MAXIMUM_THREADS];
		int index;

		usleep(interval);
		pthread_mutex_lock(&threads_lock);
		memcpy(threads, guest_threads, sizeof(threads));
		memcpy(used, guest_thread_used, sizeof(used));
		pthread_mutex_unlock(&threads_lock);
		for (index = 0; index < MAXIMUM_THREADS; index++)
		{
			if (used[index])
				pthread_kill(threads[index], SAMPLE_SIGNAL);
		}
	}
	return NULL;
}

void host_debug_start_sampler(const char *setting)
{
	struct sigaction action;
	pthread_t thread;
	/* (fractions of a second too: "0.5") */
	double seconds = setting ? strtod(setting, NULL) : 0.0;
	useconds_t interval;

	if (!(seconds > 0.0))
		return;
	interval = seconds > 3600.0 ? 3600u * 1000000u : (useconds_t)(seconds * 1000000.0);
	if (interval < 1000)
		interval = 1000;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = sample_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
	sigemptyset(&action.sa_mask);
	sigaction(SAMPLE_SIGNAL, &action, NULL);
	if (pthread_create(&thread, NULL, sampler, (void *)(uintptr_t)interval) == 0)
		pthread_detach(thread);
	host_logf(HOST_LOG_INFO, "sampling guest threads every %g s", (double)interval / 1000000.0);
}
