/*
HOST_THREAD.C

Threads that run guest code.

Guest (ILP32) code keeps stack addresses in 32-bit registers, so every
thread that runs it needs its stack in guest memory: the guest's own threads
are created here with such a stack, and host threads that call into the
guest (SDL's audio thread) switch to one of their own first. The guest's
thread pointer (its musl struct pthread) is kept per thread in host TLS.

Guest thread stacks are freed by a reaper thread once the thread has fully
exited.
*/

#include "host.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define FOREIGN_STACK_SIZE (512 * 1024)
#define MAIN_STACK_SIZE (16 * 1024 * 1024)
#define GUARD_SIZE 0x4000

static __thread uint32_t guest_tp;
static __thread uint64_t foreign_stack_top;
/* where this thread's own stack continues while it runs guest code: ART
checks the stack pointer against the thread's stack on every call into
Java, so SDL (which calls Java) must run there (host_run_native) */
static __thread uint64_t native_stack_top;

/* the stack below the caller's frame is unused while the guest runs */
#define NATIVE_STACK_TOP() (((uint64_t)__builtin_frame_address(0) - 512) & ~15ULL)

uint32_t host_get_tp(void)
{
	return guest_tp;
}

void host_set_tp(uint32_t thread)
{
	guest_tp = thread;
}

/* ---------- stacks */

static void *stack_allocate(size_t size, void **mapping, size_t *mapping_size)
{
	size_t total = size + GUARD_SIZE;
	void *base = host_low_map(total, PROT_READ | PROT_WRITE);

	if (!base)
		return NULL;
	/* guard page at the bottom */
	mprotect(base, GUARD_SIZE, PROT_NONE);
	*mapping = base;
	*mapping_size = total;
	return (char *)base + GUARD_SIZE;
}

static int on_guest_stack(void)
{
	uint64_t sp = (uint64_t)__builtin_frame_address(0);

	return sp < 0x100000000ULL;
}

/* ---------- calling into the guest */

typedef uint32_t (*guest_function)(uint32_t, uint32_t, uint32_t, uint32_t);

static uint32_t call_guest_here(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	if (on_guest_stack())
		return ((guest_function)(uintptr_t)function)(a, b, c, d);
	if (!foreign_stack_top)
	{
		void *mapping;
		size_t mapping_size;
		void *stack = stack_allocate(FOREIGN_STACK_SIZE, &mapping, &mapping_size);

		if (!stack)
			host_fatal("cannot allocate a guest stack for a host thread");
		foreign_stack_top = (uint64_t)stack + FOREIGN_STACK_SIZE;
	}
	{
		uint64_t saved = native_stack_top;
		uint32_t result;

		native_stack_top = NATIVE_STACK_TOP();
		result = (uint32_t)host_call_on_stack(function, a, b, c, d, foreign_stack_top);
		native_stack_top = saved;
		return result;
	}
}

uint64_t host_run_native(uint64_t function, uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	/* guest threads created by host_thread_create have only the guest
	stack, which is also the one ART knows */
	if (native_stack_top && on_guest_stack())
		return host_call_on_stack(function, a, b, c, d, native_stack_top);
	return ((uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t))(uintptr_t)function)(a, b, c, d);
}

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	if (!guest_tp)
		call_guest_here(host_image.header->thread_attach, 0, 0, 0, 0);
	return call_guest_here(function, a, b, c, d);
}

void host_run_guest_main(uint32_t boot)
{
	void *mapping;
	size_t mapping_size;
	void *stack = stack_allocate(MAIN_STACK_SIZE, &mapping, &mapping_size);

	if (!stack)
		host_fatal("cannot allocate the guest's main stack");
	host_debug_thread_started();
	native_stack_top = NATIVE_STACK_TOP();
	host_call_on_stack(host_image.header->start, boot, 0, 0, 0, (uint64_t)stack + MAIN_STACK_SIZE);
	host_fatal("the guest returned from __guest_start");
}

/* ---------- guest threads */

struct thread_start
{
	uint32_t guest_thread;
	void *mapping;
	size_t mapping_size;
};

struct finished_thread
{
	struct finished_thread *next;
	pthread_t thread;
	void *mapping;
	size_t mapping_size;
};

static pthread_mutex_t reaper_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reaper_condition = PTHREAD_COND_INITIALIZER;
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
	host_debug_thread_started();
	host_call_guest(host_image.header->thread_start, start.guest_thread, 0, 0, 0);
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

int host_thread_create(uint32_t guest_thread, uint32_t stack_size)
{
	struct thread_start *start = calloc(1, sizeof(*start));
	pthread_attr_t attributes;
	pthread_t thread;
	void *stack;
	int error;

	if (!start)
		return ENOMEM;
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

	stack_size = (stack_size + 0xffff) & ~0xffffu;
	stack = stack_allocate(stack_size, &start->mapping, &start->mapping_size);
	if (!stack)
	{
		free(start);
		return EAGAIN;
	}
	start->guest_thread = guest_thread;
	pthread_attr_init(&attributes);
	pthread_attr_setstack(&attributes, stack, stack_size);
	error = pthread_create(&thread, &attributes, thread_main, start);
	pthread_attr_destroy(&attributes);
	if (error)
	{
		host_low_unmap(start->mapping, start->mapping_size);
		free(start);
	}
	return error;
}
