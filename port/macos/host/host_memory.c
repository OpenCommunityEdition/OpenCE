/*
HOST_MEMORY.C

Guest address space for the macOS port.

The host is linked with a 64 KB __PAGEZERO (tools/macos_build.py), so the
low 4 GB is ordinary address space. Before anything else can map memory
there (a constructor, ahead of main and of SDL), the host reserves one
region, [HOST_GUEST_LOW, HOST_GUEST_HIGH), without access. Everything the
guest uses is carved from it:

- the Xbox contiguous window at 0x80000000 and the image's own range, at
  the fixed addresses the guest was built for;
- pages for the guest's other mappings (malloc arenas, thread stacks), from
  a page map over the rest of the region.

Memory the guest gives back is returned to the system but the address
space stays reserved, so nothing of the host's can land inside it.

This file also implements guest memory write tracking (the interface of
port/linux/src/memory_watch.c): the renderer write-protects the pages behind
the textures it caches, and the fault handler here records the first write
to each. On macOS a write to a read-only page arrives as SIGBUS as well as
SIGSEGV.
*/

#include "host.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define PAGE 0x1000ULL
#define LOW_LIMIT 0x100000000ULL
/* the reserved region: the image at its start (HALO_GUEST_IMAGE_BASE),
the Xbox window inside it, and the pools around them */
#define HOST_GUEST_LOW 0x20000000ULL
#define HOST_GUEST_HIGH 0xfff00000ULL
#define REGION_PAGES ((HOST_GUEST_HIGH - HOST_GUEST_LOW) / PAGE)

/* 1 for each page handed out, or part of the window or the image */
static uint8_t *page_used;
static uint64_t search_hint;
static int reserved;
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t window_base, window_end;
static uint64_t image_base, image_end;

static uint64_t round_up(uint64_t value)
{
	return (value + PAGE - 1) & ~(PAGE - 1);
}

static int in_range(uint64_t address, uint64_t size, uint64_t base, uint64_t end)
{
	return address >= base && address + size <= end && address + size >= address;
}

static void *map_none(uint64_t address, uint64_t size)
{
	return mmap((void *)address, size, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
}

/* ---------- the reservation */

int host_memory_reserve(void)
{
	void *result;

	if (reserved)
		return 0;
	/* not MAP_FIXED: that would replace whatever is already there */
	result = mmap((void *)HOST_GUEST_LOW, HOST_GUEST_HIGH - HOST_GUEST_LOW, PROT_NONE,
		MAP_PRIVATE | MAP_ANON, -1, 0);
	if (result != (void *)HOST_GUEST_LOW)
	{
		if (result != MAP_FAILED)
			munmap(result, HOST_GUEST_HIGH - HOST_GUEST_LOW);
		return -1;
	}
	page_used = calloc(REGION_PAGES, 1);
	if (!page_used)
		return -1;
	reserved = 1;
	return 0;
}

/* as early as possible: before SDL, the dynamic loader's later work or the
allocator can take any of the range */
__attribute__((constructor(101))) static void reserve_early(void)
{
	host_memory_reserve();
}

static void mark(uint64_t address, uint64_t size, uint8_t value)
{
	uint64_t first = (address - HOST_GUEST_LOW) / PAGE;
	uint64_t count = round_up(size) / PAGE;

	memset(page_used + first, value, count);
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	if (host_memory_reserve() != 0)
	{
		host_logf(HOST_LOG_ERROR, "cannot reserve guest memory at %08llx-%08llx (%s)",
			HOST_GUEST_LOW, HOST_GUEST_HIGH, strerror(errno));
		return -1;
	}
	window_base = HALO_GUEST_WINDOW_BASE;
	window_end = window_base + HALO_GUEST_WINDOW_SIZE;
	image_base = base;
	image_end = base + round_up(size);
	if (!in_range(window_base, HALO_GUEST_WINDOW_SIZE, HOST_GUEST_LOW, HOST_GUEST_HIGH) ||
		!in_range(image_base, image_end - image_base, HOST_GUEST_LOW, HOST_GUEST_HIGH) ||
		(image_base < window_end && image_end > window_base))
	{
		host_logf(HOST_LOG_ERROR, "the guest image %08llx-%08llx does not fit the reserved region",
			image_base, image_end);
		return -1;
	}
	pthread_mutex_lock(&memory_lock);
	mark(window_base, HALO_GUEST_WINDOW_SIZE, 1);
	mark(image_base, image_end - image_base, 1);
	search_hint = image_end;
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

/* ---------- pages */

/* the first run of free pages at or after start; 0 if none */
static uint64_t find_run(uint64_t pages, uint64_t start)
{
	uint64_t first = (start - HOST_GUEST_LOW) / PAGE, page, run = 0;

	for (page = first; page < REGION_PAGES; page++)
	{
		if (page_used[page])
		{
			run = 0;
			continue;
		}
		if (++run == pages)
			return HOST_GUEST_LOW + (page + 1 - pages) * PAGE;
	}
	return 0;
}

void *host_low_map(size_t size, int protection)
{
	uint64_t length = round_up(size);
	uint64_t address;

	if (!length || !reserved)
		return NULL;
	pthread_mutex_lock(&memory_lock);
	address = find_run(length / PAGE, search_hint ? search_hint : HOST_GUEST_LOW);
	if (!address)
		address = find_run(length / PAGE, HOST_GUEST_LOW);
	if (address)
	{
		mark(address, length, 1);
		search_hint = address + length;
	}
	pthread_mutex_unlock(&memory_lock);
	if (!address)
		return NULL;
	if (mmap((void *)address, length, protection, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) != (void *)address)
	{
		host_low_unmap((void *)address, length);
		return NULL;
	}
	return (void *)address;
}

static int in_pool(uint64_t address, uint64_t size)
{
	return in_range(address, size, HOST_GUEST_LOW, HOST_GUEST_HIGH) &&
		!in_range(address, size, window_base, window_end) && !in_range(address, size, image_base, image_end);
}

void host_low_unmap(void *address, size_t size)
{
	uint64_t start = (uint64_t)address & ~(PAGE - 1);
	uint64_t length = round_up((uint64_t)address + size) - start;

	if (!in_pool(start, length))
		return;
	/* give the memory back but keep the address space */
	map_none(start, length);
	pthread_mutex_lock(&memory_lock);
	mark(start, length, 0);
	if (start < search_hint)
		search_hint = start;
	pthread_mutex_unlock(&memory_lock);
}

int host_low_owns(uintptr_t address, size_t size)
{
	uint64_t first, last, page;
	int result = 1;

	if (in_range(address, size, window_base, window_end) || in_range(address, size, image_base, image_end))
		return 1;
	if (!size || !in_range(address, size, HOST_GUEST_LOW, HOST_GUEST_HIGH))
		return 0;
	first = (address - HOST_GUEST_LOW) / PAGE;
	last = (address + size - 1 - HOST_GUEST_LOW) / PAGE;
	pthread_mutex_lock(&memory_lock);
	for (page = first; page <= last; page++)
	{
		if (!page_used[page])
		{
			result = 0;
			break;
		}
	}
	pthread_mutex_unlock(&memory_lock);
	return result;
}

/* ---------- the guest's memory system calls (Linux flag values) */

#define LINUX_MAP_SHARED 0x01
#define LINUX_MAP_PRIVATE 0x02
#define LINUX_MAP_FIXED 0x10
#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MAP_FIXED_NOREPLACE 0x100000

static int host_map_flags(int flags)
{
	int result = 0;

	if (flags & LINUX_MAP_SHARED)
		result |= MAP_SHARED;
	if (flags & LINUX_MAP_PRIVATE)
		result |= MAP_PRIVATE;
	if (flags & LINUX_MAP_ANONYMOUS)
		result |= MAP_ANON;
	return result;
}

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	uint64_t length = round_up(size);
	int host_flags = host_map_flags(flags) | MAP_FIXED;
	void *result;

	if (!length)
		return -22; /* EINVAL */
	if (flags & LINUX_MAP_ANONYMOUS)
		fd = -1;
	if (flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE))
	{
		if (address + length > LOW_LIMIT || !host_low_owns(address, length))
			return -12; /* ENOMEM */
		result = mmap((void *)address, length, protection, host_flags, fd, offset);
		if (result == MAP_FAILED)
			return -host_linux_errno(errno);
		return (long)(uintptr_t)result;
	}
	result = host_low_map(length, PROT_NONE);
	if (!result)
		return -12;
	if (mmap(result, length, protection, host_flags, fd, offset) != result)
	{
		int error = errno;

		host_low_unmap(result, length);
		return -host_linux_errno(error);
	}
	return (long)(uintptr_t)result;
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	uint64_t length = round_up(size);

	if (address + length > LOW_LIMIT)
		return -22;
	if (in_range(address, length, window_base, window_end))
	{
		map_none(address, length);
		return 0;
	}
	if (in_range(address, length, image_base, image_end))
		return -22;
	if (host_low_owns(address, length))
	{
		host_low_unmap((void *)address, length);
		return 0;
	}
	return -22;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	if (address + size > LOW_LIMIT)
		return -22;
	return mprotect((void *)address, size, protection) ? -host_linux_errno(errno) : 0;
}

/* ---------- write tracking (port/linux/src/memory_watch.c) */

#define WATCH_PAGE_COUNT (HALO_GUEST_WINDOW_SIZE / PAGE)

static uint8_t page_protected[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static int watch_active;

static int in_window(uint64_t address)
{
	return address >= HALO_GUEST_WINDOW_BASE && address - HALO_GUEST_WINDOW_BASE < HALO_GUEST_WINDOW_SIZE;
}

static uint64_t watch_page(uint64_t address)
{
	return (address - HALO_GUEST_WINDOW_BASE) / PAGE;
}

static void mark_written(uint64_t page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	page_protected[page] = 0;
	mprotect((void *)(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, PROT_READ | PROT_WRITE);
}

static struct sigaction previous_segv, previous_bus, previous_ill;

static void report_crash(int signal_number, siginfo_t *information, void *context)
{
	ucontext_t *ucontext = context;
	const _STRUCT_X86_THREAD_STATE64 *registers = &ucontext->uc_mcontext->__ss;
	uint64_t pc = registers->__rip;
	int index;

	host_logf(HOST_LOG_ERROR, "signal %d at address %p: rip %016llx rsp %016llx rbp %016llx",
		signal_number, information->si_addr, (unsigned long long)pc, (unsigned long long)registers->__rsp,
		(unsigned long long)registers->__rbp);
	if (pc >= host_image.base && pc < host_image.end)
		host_logf(HOST_LOG_ERROR, "  in the guest image: llvm-symbolizer --obj=build/macos/halo_guest.elf 0x%llx",
			(unsigned long long)pc);
	host_logf(HOST_LOG_ERROR, "  rax %016llx rbx %016llx rcx %016llx rdx %016llx", registers->__rax, registers->__rbx,
		registers->__rcx, registers->__rdx);
	host_logf(HOST_LOG_ERROR, "  rsi %016llx rdi %016llx r8  %016llx r9  %016llx", registers->__rsi, registers->__rdi,
		registers->__r8, registers->__r9);
	/* the guest's frame records: saved rbp and return address, 8 bytes each */
	{
		uint64_t fp = registers->__rbp;

		for (index = 0; index < 24 && fp && fp < LOW_LIMIT && (fp & 7) == 0; index++)
		{
			const uint64_t *frame = (const uint64_t *)fp;

			if (!host_low_owns(fp, 16))
				break;
			host_logf(HOST_LOG_ERROR, "  frame %2d: return %016llx", index, (unsigned long long)frame[1]);
			fp = frame[0];
		}
	}
}

static void chain(struct sigaction *previous, int signal_number, siginfo_t *information, void *context)
{
	sigaction(signal_number, previous, NULL);
	if (previous->sa_flags & SA_SIGINFO)
	{
		if (previous->sa_sigaction)
			previous->sa_sigaction(signal_number, information, context);
	}
	else if (previous->sa_handler != SIG_DFL && previous->sa_handler != SIG_IGN)
	{
		previous->sa_handler(signal_number);
	}
}

static int watched_write(siginfo_t *information)
{
	uint64_t address = (uint64_t)information->si_addr;

	if (watch_active && in_window(address))
	{
		uint64_t page = watch_page(address);

		if (page_protected[page])
		{
			mark_written(page);
			return 1;
		}
	}
	return 0;
}

static void segv_handler(int signal_number, siginfo_t *information, void *context)
{
	if (watched_write(information))
		return;
	report_crash(signal_number, information, context);
	chain(&previous_segv, signal_number, information, context);
}

static void bus_handler(int signal_number, siginfo_t *information, void *context)
{
	if (watched_write(information))
		return;
	report_crash(signal_number, information, context);
	chain(&previous_bus, signal_number, information, context);
}

static void ill_handler(int signal_number, siginfo_t *information, void *context)
{
	report_crash(signal_number, information, context);
	chain(&previous_ill, signal_number, information, context);
}

void host_install_signal_handlers(void)
{
	struct sigaction action;
	static char alternate_stack[256 * 1024];
	stack_t stack;

	stack.ss_sp = alternate_stack;
	stack.ss_size = sizeof(alternate_stack);
	stack.ss_flags = 0;
	sigaltstack(&stack, NULL);
	memset(&action, 0, sizeof(action));
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = segv_handler;
	sigaction(SIGSEGV, &action, &previous_segv);
	action.sa_sigaction = bus_handler;
	sigaction(SIGBUS, &action, &previous_bus);
	action.sa_sigaction = ill_handler;
	sigaction(SIGILL, &action, &previous_ill);
	/* a closed network connection must not end the game */
	signal(SIGPIPE, SIG_IGN);
}

void host_memory_watch_initialize(void)
{
	watch_active = 1;
}

void host_memory_watch_protect(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;

	if (!watch_active || !size || !in_window(address))
		return;
	first = watch_page(address);
	last = watch_page((uint64_t)address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (!page_protected[page])
		{
			page_protected[page] = 1;
			mprotect((void *)(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, PROT_READ);
		}
	}
}

uint32_t host_memory_watch_serial(void)
{
	return current_generation;
}

uint32_t host_memory_watch_generation(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;
	uint32_t newest = 0;

	if (!size || !in_window(address))
		return 0;
	first = watch_page(address);
	last = watch_page((uint64_t)address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (page_generation[page] > newest)
			newest = page_generation[page];
	}
	return newest;
}

void host_memory_watch_prepare_write(uint32_t address, uint32_t size)
{
	uint64_t start = address, first, last, page;

	if (!watch_active || !size)
		return;
	if (start + size <= HALO_GUEST_WINDOW_BASE || start >= (uint64_t)HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
		return;
	if (start < HALO_GUEST_WINDOW_BASE)
		start = HALO_GUEST_WINDOW_BASE;
	first = watch_page(start);
	last = watch_page((uint64_t)address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		if (page_protected[page])
			mark_written(page);
	}
}

void host_memory_watch_forget(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;

	if (!size || !in_window(address))
		return;
	first = watch_page(address);
	last = watch_page((uint64_t)address + size - 1);
	if (last >= WATCH_PAGE_COUNT)
		last = WATCH_PAGE_COUNT - 1;
	for (page = first; page <= last; page++)
	{
		page_protected[page] = 0;
		page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}
