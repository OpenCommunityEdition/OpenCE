/*
WIN64_THREAD.C

Threads that run guest code, on the Windows x64 host (port/windows/host,
docs/x64_route.md "Windows x64 host"); the Linux host's host_thread.c does
the same with pthreads.

Guest (x32) code keeps stack addresses in 32-bit registers, so it runs on a
stack below 4 GB, which each such thread gets from the guest memory pools.
Unlike the Linux host's threads, which run on that stack throughout, a
Windows thread keeps its own stack for the host's code: Windows checks the
stack pointer against the stack's limits in the TEB (stack growth,
__chkstk, exception dispatch), and system DLLs and drivers expect room.
host_enter_guest (win64_switch.S) moves to the guest's stack, and every
import thunk (tools/windows_host_thunks.py) back to the host's; the TEB's
limits follow. Each thread's struct host_switch, in a TLS slot whose TEB
offset the assembly reads (host_switch_tls_offset), keeps both.

The guest's thread pointer (its musl struct pthread) is kept per thread
here too.
*/

#include "host.h"
#include "win64_host.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define GUARD_SIZE 0x10000
/* room above the guest stack's first frame (never touched) */
#define STACK_TOP_RESERVE 0x100
/* the host's own stack for a thread that runs guest code (drivers and the
system's DLLs run on it) */
#define HOST_STACK_SIZE (8 * 1024 * 1024)

/* the layout win64_switch.S and the thunks read */
struct host_switch
{
	uint64_t host_sp;
	uint64_t guest_sp;
	uint64_t host_base, host_limit;
	uint64_t guest_base, guest_limit;
	uint64_t host_deallocation, guest_deallocation;
	/* (the host's only) */
	uint32_t guest_tp;
	HANDLE timer;
	int timer_made;
	void *stack_mapping;
	size_t stack_mapping_size;
};

_Static_assert(offsetof(struct host_switch, guest_deallocation) == 56, "win64_switch.S's layout");

/* the TEB offset of the TLS slot (TlsSlots, 0x1480, for an index below 64) */
uint32_t host_switch_tls_offset;
static DWORD tls_index = TLS_OUT_OF_INDEXES;

uint32_t host_enter_guest(uint64_t function, uint64_t a, uint64_t b, uint64_t c, uint64_t d);

void host_threads_initialize(void)
{
	tls_index = TlsAlloc();
	if (tls_index == TLS_OUT_OF_INDEXES || tls_index >= 64)
		host_fatal("no TLS slot for the guest's threads (%lu)", (unsigned long)tls_index);
	host_switch_tls_offset = 0x1480 + 8 * tls_index;
}

static struct host_switch *current(void)
{
	return tls_index == TLS_OUT_OF_INDEXES ? NULL : TlsGetValue(tls_index);
}

uint32_t host_get_tp(void)
{
	struct host_switch *context = current();

	return context ? context->guest_tp : 0;
}

void host_set_tp(uint32_t thread)
{
	struct host_switch *context = current();

	if (context)
		context->guest_tp = thread;
}

HANDLE host_thread_timer(void)
{
	struct host_switch *context = current();

	if (!context)
		return NULL;
	if (!context->timer_made)
	{
		context->timer_made = 1;
		context->timer = CreateWaitableTimerExW(NULL, NULL, 0x00000002 /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */,
			TIMER_ALL_ACCESS);
		if (!context->timer)
			context->timer = CreateWaitableTimerExW(NULL, NULL, 0, TIMER_ALL_ACCESS);
	}
	return context->timer;
}

/* ---------- calling into the guest */

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	struct host_switch *context = current();

	if (!context)
		host_fatal("guest code called on a thread without a guest stack");
	if (!context->guest_tp)
		host_enter_guest(host_image.header->thread_attach, 0, 0, 0, 0);
	return host_enter_guest(function, a, b, c, d);
}

void host_run_guest_main(uint32_t boot)
{
	if (!current())
		host_fatal("the guest's main called on a thread without a guest stack");
	host_enter_guest(host_image.header->start, boot, 0, 0, 0);
	host_fatal("the guest returned from __guest_start");
}

/* ---------- threads */

struct thread_start
{
	void *(*function)(void *);
	void *argument;
	size_t stack_size;
};

/* gives the calling thread a guest stack and its struct host_switch; 0 on
success */
static int attach(size_t stack_size)
{
	struct host_switch *context = calloc(1, sizeof(*context));
	ULONG_PTR low = 0, high = 0;
	char *mapping;

	if (!context)
		return -1;
	stack_size = (stack_size + 0xffff) & ~(size_t)0xffff;
	mapping = host_low_map(stack_size + GUARD_SIZE, PROT_READ | PROT_WRITE);
	if (!mapping)
	{
		free(context);
		return -1;
	}
	/* a guard region at the bottom */
	host_low_protect(mapping, GUARD_SIZE, PROT_NONE);
	context->stack_mapping = mapping;
	context->stack_mapping_size = stack_size + GUARD_SIZE;
	context->guest_base = (uint64_t)(uintptr_t)(mapping + GUARD_SIZE + stack_size);
	context->guest_limit = (uint64_t)(uintptr_t)(mapping + GUARD_SIZE);
	context->guest_deallocation = (uint64_t)(uintptr_t)mapping;
	context->guest_sp = context->guest_base - STACK_TOP_RESERVE;
	GetCurrentThreadStackLimits(&low, &high);
	context->host_base = high;
	context->host_limit = (uint64_t)(uintptr_t)((NT_TIB *)NtCurrentTeb())->StackLimit;
	context->host_deallocation = low;
	TlsSetValue(tls_index, context);
	return 0;
}

static void detach(void)
{
	struct host_switch *context = current();

	if (!context)
		return;
	/* (a thread the guest made has let its struct pthread go by now; one
	of the host's that ran guest code, the audio thread, has it still) */
	if (context->guest_tp)
		host_enter_guest(host_image.header->thread_detach, 0, 0, 0, 0);
	TlsSetValue(tls_index, NULL);
	if (context->timer)
		CloseHandle(context->timer);
	host_low_unmap(context->stack_mapping, context->stack_mapping_size);
	free(context);
}

static DWORD WINAPI thread_main(void *parameter)
{
	struct thread_start start = *(struct thread_start *)parameter;

	free(parameter);
	if (attach(start.stack_size) != 0)
	{
		host_logf(HOST_LOG_ERROR, "no guest stack (%zu bytes) for a new thread", start.stack_size);
		return 1;
	}
	host_debug_thread_started();
	start.function(start.argument);
	host_debug_thread_exited();
	detach();
	return 0;
}

int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size)
{
	struct thread_start *start = malloc(sizeof(*start));
	HANDLE thread;

	if (!start)
		return ENOMEM;
	start->function = function;
	start->argument = argument;
	start->stack_size = stack_size;
	thread = CreateThread(NULL, HOST_STACK_SIZE, thread_main, start, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
	if (!thread)
	{
		free(start);
		return EAGAIN;
	}
	CloseHandle(thread);
	return 0;
}

static void *guest_thread_main(void *guest_thread)
{
	host_call_guest(host_image.header->thread_start, (uint32_t)(uintptr_t)guest_thread, 0, 0, 0);
	return NULL;
}

int host_thread_create(uint32_t guest_thread, uint32_t stack_size)
{
	return host_native_thread_create(guest_thread_main, (void *)(uintptr_t)guest_thread, stack_size);
}

/* ---------- debugging (host_debug.c's sampler is the Linux host's) */

void host_debug_thread_started(void)
{
}

void host_debug_thread_exited(void)
{
}

void host_debug_start_sampler(const char *setting)
{
	host_logf(HOST_LOG_WARN, "debug.sample_seconds = %s: the sampler is not in the Windows host", setting);
}

int host_signal_stack_install(void)
{
	return 0;
}

void host_signal_stack_remove(void)
{
}
