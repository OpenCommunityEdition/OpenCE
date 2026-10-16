/*
MEMORY_WATCH.C

Write tracking for guest memory that the renderer caches.

Textures live in the Xbox contiguous window, where the game (or its
streaming threads) can rewrite them at any time. Instead of hashing their
contents every frame, the pages behind a cached texture are made read-only;
the first write faults, the handler records a new generation for the page,
makes it writable again and lets the write proceed. A cache entry is stale
when any of its pages has a generation newer than the entry.

Writes that the kernel performs on the game's behalf (read() into a
buffer) would fail with EFAULT instead of faulting, so the file layer reads
guest memory through a bounce buffer (xbox_files.c). Unprotecting ahead of
such a write (memory_watch_prepare_write) is not enough on its own: the
renderer can protect the pages again before the kernel writes them.

The same handler reports a crash (SIGSEGV, and SIGBUS, SIGILL, SIGFPE and
SIGABRT too): the signal, the registers and the calls, to the terminal and
debug.txt, and then hands the signal to the handler before it, or the
default action. It is installed before main, so that a crash at any time
is reported; a library that installs its own later would take the watch's
faults, so the renderer's device installs it again, chained to that one.
*/

#include "platform.h"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>

#define WATCH_PAGE_SIZE 0x1000UL
#define WATCH_PAGE_COUNT (PLATFORM_CONTIGUOUS_SIZE / WATCH_PAGE_SIZE)

static unsigned char page_protected[WATCH_PAGE_COUNT];
static unsigned long page_generation[WATCH_PAGE_COUNT];
static volatile unsigned long current_generation = 1;
static BOOL watch_active = FALSE;

/* the signals reported as a crash, SIGSEGV first (the write tracking's), and
what handled each before this file's handler */
static const int crash_signals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
#define CRASH_SIGNAL_COUNT (int)(sizeof(crash_signals) / sizeof(crash_signals[0]))
static struct sigaction previous_actions[CRASH_SIGNAL_COUNT];
/* a crash being reported: a handler chained to that hands the signal back
here (one installed after this file's, which took this file's as its
previous) gets the default action instead of a loop */
static volatile sig_atomic_t crash_chaining = 0;

static unsigned long page_index(unsigned long address)
{
	return (address - PLATFORM_CONTIGUOUS_BASE) / WATCH_PAGE_SIZE;
}

static void mark_written(unsigned long page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	page_protected[page] = 0;
	mprotect((void *)(PLATFORM_CONTIGUOUS_BASE + page * WATCH_PAGE_SIZE), WATCH_PAGE_SIZE, PROT_READ | PROT_WRITE);
}

/* errors.c's: debug.txt (a player sends it; the terminal's lines may be
gone) */
void write_to_error_file(char *string, unsigned char date);

/* a line of a crash report (ending in a newline) to debug.txt too, best
effort: the crash may be in the middle of writing it */
static void crash_debug_line(const char *line)
{
	char text[200];
	size_t length = strcspn(line, "\n");

	if (length > sizeof(text) - 3)
		length = sizeof(text) - 3;
	memcpy(text, line, length);
	memcpy(text + length, "\r\n", 3);
	write_to_error_file(text, 1);
}

static const char *crash_signal_name(int signal_number)
{
	switch (signal_number)
	{
	case SIGSEGV:
		return "segmentation fault";
	case SIGBUS:
		return "bus error";
	case SIGILL:
		return "illegal instruction";
	case SIGFPE:
		return "arithmetic exception";
	case SIGABRT:
		return "abort";
	}
	return "signal";
}

static int crash_signal_index(int signal_number)
{
	int index;

	for (index = 0; index < CRASH_SIGNAL_COUNT; index++)
	{
		if (crash_signals[index] == signal_number)
			return index;
	}
	return -1;
}

/* the report: the signal, the registers, the top of the stack and the
calls, to the terminal and debug.txt */
static void crash_report(int signal_number, siginfo_t *information, void *context)
{
	ucontext_t *ucontext = context;
	char line[200];
	void *frames[48];
	unsigned long pc, frame, stack;
	int count, length;

#if defined(__i386__)
	pc = (unsigned long)ucontext->uc_mcontext.gregs[REG_EIP];
	frame = (unsigned long)ucontext->uc_mcontext.gregs[REG_EBP];
	stack = (unsigned long)ucontext->uc_mcontext.gregs[REG_ESP];
#elif defined(__x86_64__)
	pc = (unsigned long)ucontext->uc_mcontext.gregs[REG_RIP];
	frame = (unsigned long)ucontext->uc_mcontext.gregs[REG_RBP];
	stack = (unsigned long)ucontext->uc_mcontext.gregs[REG_RSP];
#else
	pc = frame = stack = 0;
	(void)ucontext;
#endif
	length = snprintf(line, sizeof(line), "halo-linux: %s (signal %d, code %d) at %p, eip %08lx ebp %08lx esp %08lx\n",
		crash_signal_name(signal_number), signal_number, information ? information->si_code : 0,
		information ? information->si_addr : NULL, pc, frame, stack);
	write(STDERR_FILENO, line, (size_t)length);
	crash_debug_line(line);
	if (stack)
	{
		/* the return address a call through a bad pointer left behind */
		const unsigned *words = (const unsigned *)stack;

		length = snprintf(line, sizeof(line), "halo-linux: stack %08x %08x %08x %08x %08x %08x\n",
			words[0], words[1], words[2], words[3], words[4], words[5]);
		write(STDERR_FILENO, line, (size_t)length);
		crash_debug_line(line);
	}
	count = backtrace(frames, 48);
	backtrace_symbols_fd(frames, count, STDERR_FILENO);
	{
		int index;

		for (index = 0; index < count; index++)
		{
			snprintf(line, sizeof(line), "halo-linux: called from %p\n", frames[index]);
			crash_debug_line(line);
		}
	}
}

/* hands the signal to what handled it before this file's handler, which
is put back: a fault returned from runs its instruction again under it, and
a signal sent (kill, abort's raise) is raised again for it, once this
handler returns */
static void crash_chain(int index, int signal_number, siginfo_t *information, void *context)
{
	struct sigaction previous = previous_actions[index];

	if (crash_chaining)
	{
		/* (back here from the handler chained to: its default ends it) */
		signal(signal_number, SIG_DFL);
		if (!information || information->si_code <= 0)
			raise(signal_number);
		return;
	}
	crash_chaining = 1;
	sigaction(signal_number, &previous, NULL);
	if (previous.sa_flags & SA_SIGINFO)
	{
		if (previous.sa_sigaction)
			previous.sa_sigaction(signal_number, information, context);
	}
	else if (previous.sa_handler == SIG_DFL)
	{
		if (!information || information->si_code <= 0)
			raise(signal_number);
	}
	else if (previous.sa_handler != SIG_IGN)
	{
		previous.sa_handler(signal_number);
	}
	crash_chaining = 0;
}

static void crash_handler(int signal_number, siginfo_t *information, void *context)
{
	int index = crash_signal_index(signal_number);

	if (signal_number == SIGSEGV && information && platform_is_contiguous(information->si_addr))
	{
		unsigned long page = page_index((unsigned long)information->si_addr);

		/* a write to a page the renderer watches: noted, and let through */
		if (page < WATCH_PAGE_COUNT && page_protected[page])
		{
			mark_written(page);
			return;
		}
	}
	if (index < 0)
		return;
	/* (a signal sent, not a fault, that was ignored before: still ignored) */
	if (!crash_chaining && previous_actions[index].sa_handler == SIG_IGN &&
		!(previous_actions[index].sa_flags & SA_SIGINFO) && information && information->si_code <= 0)
	{
		return;
	}
	/* a genuine crash: report it (once), then hand it on */
	if (!crash_chaining)
		crash_report(signal_number, information, context);
	crash_chain(index, signal_number, information, context);
}

static BOOL crash_handler_installed(int signal_number, struct sigaction *current)
{
	return sigaction(signal_number, NULL, current) == 0 && (current->sa_flags & SA_SIGINFO) &&
		current->sa_sigaction == crash_handler;
}

/* the handler, for each signal where it is not the one installed (first, or
a library put its own in since: a GL driver's, a sound library's), chained
to the one that is; TRUE if SIGSEGV's is this file's */
static BOOL crash_handlers_install(void)
{
	struct sigaction action, current;
	int index;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = crash_handler;
	/* (SA_NODEFER: a fault in the handler, or a write fault of the watch
	in a nested one, comes back here) */
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigemptyset(&action.sa_mask);
	for (index = 0; index < CRASH_SIGNAL_COUNT; index++)
	{
		if (crash_handler_installed(crash_signals[index], &current))
			continue;
		previous_actions[index] = current;
		sigaction(crash_signals[index], &action, NULL);
		/* (a handler chained to that jumped out of the signal, instead of
		returning, left the chaining unfinished) */
		crash_chaining = 0;
	}
	return crash_handler_installed(SIGSEGV, &current);
}

/* at the renderer's device (d3d8_gl.c), before any page is protected; and
from the start (below). A library that installed a SIGSEGV handler in
between (the GL driver's, as the window opened) would get the protected
pages' faults, and crash or hang at the first write to a texture: the
handler goes in again, chained to that one */
void memory_watch_initialize(void)
{
	watch_active = crash_handlers_install();
}

/* the handler from the start, for its crash report: the renderer's device
(d3d8_gl.c) installs it too, but a game with debug.null_renderer (the
network tests, net_compare) creates none, and a crash before the device
went unreported */
__attribute__((constructor))
static void memory_watch_install(void)
{
	memory_watch_initialize();
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	unsigned long first, last, page;

	if (!watch_active || !size || !platform_is_contiguous((void *)address))
		return;
	first = page_index(address);
	last = page_index(address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (!page_protected[page])
		{
			page_protected[page] = 1;
			mprotect((void *)(PLATFORM_CONTIGUOUS_BASE + page * WATCH_PAGE_SIZE), WATCH_PAGE_SIZE, PROT_READ);
		}
	}
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	unsigned long first, last, page, newest = 0;

	if (!size || !platform_is_contiguous((void *)address))
		return 0;
	first = page_index(address);
	last = page_index(address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (page_generation[page] > newest)
			newest = page_generation[page];
	}
	return newest;
}

unsigned long memory_watch_serial(void)
{
	return current_generation;
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	unsigned long start = (unsigned long)address;
	unsigned long first, last, page;

	if (!watch_active || !size)
		return;
	if (start + size <= PLATFORM_CONTIGUOUS_BASE || start >= PLATFORM_CONTIGUOUS_BASE + PLATFORM_CONTIGUOUS_SIZE)
		return;
	if (start < PLATFORM_CONTIGUOUS_BASE)
		start = PLATFORM_CONTIGUOUS_BASE;
	first = page_index(start);
	last = page_index((unsigned long)address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (page_protected[page])
			mark_written(page);
	}
}

void memory_watch_forget(void *address, unsigned long size)
{
	unsigned long start = (unsigned long)address;
	unsigned long first, last, page;

	if (!size || !platform_is_contiguous(address))
		return;
	first = page_index(start);
	last = page_index(start + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		page_protected[page] = 0;
		page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}

/* page protection sees each write at once; once a frame, the handler is
checked to be the one installed (a library may put its own in at any time:
a sound device opened, a gamepad's) */
void memory_watch_begin_frame(void)
{
	if (watch_active)
		crash_handlers_install();
}
