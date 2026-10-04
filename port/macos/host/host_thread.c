/*
HOST_THREAD.C

Threads that run guest code (the interface of the Android port's
host_thread.c, on Hypervisor.framework vCPUs).

Guest (ILP32) code keeps stack addresses in 32-bit registers, so every
thread that runs it needs its stack in guest memory (the pools). Each
thread owns one vCPU, created on the thread (the hypervisor requires
that), with the thread's guest stack planted before its first call.

The main thread and SDL's audio callback hand their work to threads made
by host_native_thread_create; guest threads (pthread_create) run
__guest_thread_start on their own stack.
*/

#include "host.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define GUARD_SIZE 0x10000

__thread unsigned int guest_tp;

unsigned int host_get_tp(void)
{
	return guest_tp;
}

void host_set_tp(unsigned int thread)
{
	guest_tp = thread;
}

/* ---------- stacks */

static int stack_allocate(size_t size, void **mapping, size_t *mapping_size, uint32_t *guest_top)
{
	size_t total = size + GUARD_SIZE;
	uint8_t *base = host_low_map(total);

	if (!base)
		return -1;
	/* (the guard is ordinary memory here: a runaway stack overwrites
	it, and the next doorbell exit or a crash log says where; the guest
	never runs long enough without a doorbell to smash anything) */
	memset(base, 0, GUARD_SIZE);
	*mapping = base;
	*mapping_size = total;
	/* the guest's stacks grow down from the top of the range */
	*guest_top = (uint32_t)(uintptr_t)base + total - 64;
	return 0;
}

/* ---------- calling into the guest */

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

/* ---------- host threads (stacks in guest memory) */

struct thread_start
{
	void *(*function)(void *);
	void *argument;
	void *mapping;
	size_t mapping_size;
	uint32_t guest_top;
};

struct finished_thread
{
	struct finished_thread *next;
	pthread_t thread;
	void *mapping;
	size_t mapping_size;
};

static pthread_mutex_t reaper_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reaper_condition = PTHREAD_MUTEX_INITIALIZER;
static struct finished_thread *finished_threads;
static int reaper_started;

static void *reaper(void *unused)
{
	(void)unused;
	for (;;)
	{
		struct finished_thread *finished;

		pthread_mutex_lock(&reaper_lock);
		while (!finished_threads)
			pthread_cond_wait(&reaper_condition, &reaper_lock);
		finished = finished_threads;
		finished_threads = finished->next;
		pthread_mutex_unlock(&reaper_lock);
		pthread_join(finished->thread, NULL);
		host_low_unmap(finished->mapping, finished->mapping_size);
		free(finished);
	}
	return NULL;
}

static void *thread_main(void *context)
{
	struct thread_start start = *(struct thread_start *)context;
	struct finished_thread *finished;

	free(context);
	if (host_vcpu_attach() != 0)
		host_fatal("cannot attach a vCPU to a guest thread");
	host_vcpu_set_stack(start.guest_top);
	host_debug_thread_started();
	start.function(start.argument);
	host_debug_thread_exited();
	guest_tp = 0;

	finished = calloc(1, sizeof(*finished));
	finished->thread = pthread_self();
	finished->mapping = start.mapping;
	finished->mapping_size = start.mapping_size;
	pthread_mutex_lock(&reaper_lock);
	finished->next = finished_threads;
	finished_threads = finished;
	pthread_cond_signal(&reaper_condition);
	pthread_mutex_unlock(&reaper_lock);
	return NULL;
}

int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size)
{
	struct thread_start *start = calloc(1, sizeof(*start));
	pthread_attr_t attributes;
	pthread_t thread;
	int error;

	if (!start)
		return 12 /* ENOMEM */;
	pthread_mutex_lock(&reaper_lock);
	if (!reaper_started)
	{
		pthread_t reaper_thread;

		if (pthread_create(&reaper_thread, NULL, reaper, NULL) == 0)
		{
			pthread_detach(reaper_thread);
			reaper_started = 1;
		}
	}
	pthread_mutex_unlock(&reaper_lock);

	if (!stack_size)
		stack_size = 1024 * 1024;
	stack_size = (stack_size + 0xffff) & ~(size_t)0xffff;
	if (stack_allocate(stack_size, &start->mapping, &start->mapping_size, &start->guest_top) != 0)
	{
		free(start);
		return 35 /* EAGAIN */;
	}
	start->function = function;
	start->argument = argument;
	pthread_attr_init(&attributes);
	error = pthread_create(&thread, &attributes, thread_main, start);
	pthread_attr_destroy(&attributes);
	if (error)
	{
		host_low_unmap(start->mapping, start->mapping_size);
		free(start);
	}
	return error;
}

/* ---------- guest threads (pthread_create) */

static void *guest_thread_main(void *context)
{
	host_call_guest(host_image.header->thread_start, (uint32_t)(uintptr_t)context, 0, 0, 0);
	return NULL;
}

int host_thread_create(unsigned int thread, unsigned int stack_size)
{
	return host_native_thread_create(guest_thread_main, (void *)(uintptr_t)thread,
		stack_size ? stack_size : 1024 * 1024);
}