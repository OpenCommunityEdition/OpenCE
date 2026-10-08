/*
HOST_THREAD.C

Threads that run guest code.

x32 code keeps stack addresses in 32-bit registers (it moves the stack
pointer with 32-bit arithmetic, which clears the upper half), so every
thread that runs it needs its stack below 4 GB. Every such thread is
created here, with a stack in guest memory given to pthread_create; the
main thread is moved onto one by host_main.c (Cocoa needs the process's
main thread). SDL's audio thread hands its work to a thread made here
(host_sdl.c). The guest's thread pointer (its musl struct pthread) is kept
per thread in host TLS, as on Android: macOS lets no program set the %fs
base an x86 thread pointer would live in.

Guest thread ids are small numbers handed out here: musl keeps them in 30
bits of its mutexes, and Darwin's own thread ids are 64-bit.

Thread stacks are freed by a reaper thread once the thread has fully exited.
*/

#include "host.h"
#include "host_linux.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/qos.h>
#include <unistd.h>

#define GUARD_SIZE 0x4000

static __thread uint32_t guest_tp;
static __thread int thread_id;
static int last_thread_id;

uint32_t host_get_tp(void)
{
	return guest_tp;
}

void host_set_tp(uint32_t thread)
{
	guest_tp = thread;
}

int host_thread_id(void)
{
	if (!thread_id)
		thread_id = __sync_add_and_fetch(&last_thread_id, 1);
	return thread_id;
}

/* ---------- stacks */

static void *stack_allocate(size_t size, void **mapping, size_t *mapping_size)
{
	size_t total = size + GUARD_SIZE;
	void *base = host_low_map(total, PROT_READ | PROT_WRITE);

	if (!base)
		return NULL;
	/* guard pages at the bottom */
	mprotect(base, GUARD_SIZE, PROT_NONE);
	*mapping = base;
	*mapping_size = total;
	return (char *)base + GUARD_SIZE;
}

static int on_guest_stack(void)
{
	uint64_t sp = (uint64_t)__builtin_frame_address(0);

	return sp < 0x100000000ull;
}

/* ---------- calling into the guest */

/* (the guest takes its pointer arguments zero-extended, and x86-64 leaves
a 32-bit argument's upper half undefined: they are passed as 64-bit values) */
typedef uint32_t (*guest_function)(uint64_t, uint64_t, uint64_t, uint64_t);

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	if (!on_guest_stack())
		host_fatal("guest code called on a thread without a guest stack");
	if (!guest_tp)
		((guest_function)(uintptr_t)host_image.header->thread_attach)(0, 0, 0, 0);
	return ((guest_function)(uintptr_t)function)(a, b, c, d);
}

void host_run_guest_main(uint32_t boot)
{
	((void (*)(uint64_t))(uintptr_t)host_image.header->start)(boot);
	host_fatal("the guest returned from __guest_start");
}

/* ---------- guest threads */

struct thread_start
{
	void *(*function)(void *);
	void *argument;
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
	host_signal_stack_install();
	pthread_setname_np("halo reaper");
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
	host_signal_stack_install();
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

	stack_size = (stack_size + 0xffff) & ~(size_t)0xffff;
	stack = stack_allocate(stack_size, &start->mapping, &start->mapping_size);
	if (!stack)
	{
		free(start);
		return EAGAIN;
	}
	start->function = function;
	start->argument = argument;
	pthread_attr_init(&attributes);
	pthread_attr_setstack(&attributes, stack, stack_size);
	/* the game's threads serve the frame being drawn (and the audio
	callback's hand-off is waited on by Core Audio's real-time thread), so
	the scheduler is told they are interactive work */
	pthread_attr_set_qos_class_np(&attributes, QOS_CLASS_USER_INTERACTIVE, 0);
	error = pthread_create(&thread, &attributes, thread_main, start);
	pthread_attr_destroy(&attributes);
	if (error)
	{
		host_low_unmap(start->mapping, start->mapping_size);
		free(start);
	}
	return error;
}

static void *guest_thread_main(void *guest_thread)
{
	host_call_guest(host_image.header->thread_start, (uint32_t)(uintptr_t)guest_thread, 0, 0, 0);
	return NULL;
}

/* the guest's pthread_create: 0, or a Linux errno value */
int host_thread_create(uint32_t guest_thread, uint32_t stack_size)
{
	return host_linux_errno(host_native_thread_create(guest_thread_main, (void *)(uintptr_t)guest_thread,
		stack_size));
}
