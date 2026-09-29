/*
HOST_MEMORY.C

Guest address space for the Android port.

Everything the guest touches must lie below 4 GB, but an Android process
shares that range with ART, whose heap spaces use compressed 32-bit
references and so also live there. The host therefore claims only what the
guest needs, when it needs it:

- the Xbox contiguous window at 0x80000000 and the image's own range, at
  start-up (both at fixed addresses the guest was built for);
- pools of address space for the guest's other mappings (malloc arenas,
  thread stacks), reserved in free gaps below 4 GB as they fill up.

This file also implements guest memory write tracking (the interface of
port/linux/src/memory_watch.c): the renderer write-protects the pages behind
the textures it caches, and the SIGSEGV handler here records the first
write to each. Other faults are reported (with guest-relative addresses) and
passed on to the previous handler.
*/

#include "host.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#define PAGE 0x1000ULL
#define LOW_LIMIT 0x100000000ULL
#define LOW_START 0x01000000ULL
#define POOL_SIZE (256ULL * 1024 * 1024)
#define POOL_PAGES (POOL_SIZE / PAGE)
#define MAXIMUM_POOLS 12

struct pool
{
	uint64_t base;
	uint32_t free_pages;
	uint8_t used[POOL_PAGES]; /* 1 for each page handed out */
};

static struct pool *pools[MAXIMUM_POOLS];
static int pool_count;
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t window_base, window_end;
static int reserved;
static uint64_t image_base, image_end;

static uint64_t round_up(uint64_t value)
{
	return (value + PAGE - 1) & ~(PAGE - 1);
}

static int in_range(uint64_t address, uint64_t size, uint64_t base, uint64_t end)
{
	return address >= base && address + size <= end && address + size >= address;
}

/* ---------- reserving address space below 4 GB */

/* the lowest free gap of at least size bytes at or above minimum, from
/proc/self/maps, at a multiple of alignment; 0 if none */
static uint64_t find_gap(uint64_t size, uint64_t minimum, uint64_t alignment)
{
	FILE *maps = fopen("/proc/self/maps", "r");
	char line[512];
	uint64_t previous_end = minimum;
	uint64_t result = 0;

	if (!maps)
		return 0;
	while (fgets(line, sizeof(line), maps))
	{
		unsigned long long start, end;

		if (sscanf(line, "%llx-%llx", &start, &end) != 2)
			continue;
		if (end <= previous_end)
			continue;
		if (start > previous_end && start - previous_end >= size)
			break;
		previous_end = end > previous_end ? end : previous_end;
		if (previous_end >= LOW_LIMIT)
			break;
	}
	fclose(maps);
	previous_end = (round_up(previous_end) + alignment - 1) & ~(alignment - 1);
	if (previous_end + size <= LOW_LIMIT)
		result = previous_end;
	return result;
}

static void log_conflicts(uint64_t address, uint64_t size)
{
	FILE *maps = fopen("/proc/self/maps", "r");
	char line[512];

	while (maps && fgets(line, sizeof(line), maps))
	{
		unsigned long long start, end;

		if (sscanf(line, "%llx-%llx", &start, &end) != 2)
			continue;
		if (end > address && start < address + size)
			host_logf(HOST_LOG_ERROR, "  %s", line);
	}
	if (maps)
		fclose(maps);
}

int host_probe_skip_fault(uintptr_t address);

/* address space the guest will have to itself; the mappings that are in
the way are named in the log when the reservation fails */
static int reserve(uint64_t address, uint64_t size)
{
	void *result = mmap((void *)address, size, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);

	if (result == (void *)address)
		return 0;
	if (result != MAP_FAILED)
		munmap(result, size);
	return -1;
}

static struct pool *pool_new(void)
{
	/* above the image first: ART allocates its own low-4 GB memory from
	the bottom up */
	uint64_t minimum = image_end ? image_end : LOW_START;
	struct pool *pool;
	int attempt;

	if (pool_count == MAXIMUM_POOLS)
		return NULL;
	for (attempt = 0; attempt < 64; attempt++)
	{
		uint64_t address = find_gap(POOL_SIZE, minimum, PAGE);

		if (!address && minimum != LOW_START)
		{
			minimum = LOW_START;
			address = find_gap(POOL_SIZE, minimum, PAGE);
		}
		if (!address)
			return NULL;
		if (reserve(address, POOL_SIZE) == 0)
		{
			pool = calloc(1, sizeof(*pool));
			pool->base = address;
			pool->free_pages = POOL_PAGES;
			pools[pool_count++] = pool;
			host_logf(HOST_LOG_INFO, "guest memory pool %d at %08llx", pool_count - 1, (unsigned long long)address);
			return pool;
		}
		/* raced with another mapping; look further up */
		minimum = address + PAGE;
	}
	return NULL;
}

/* The guest needs the Xbox memory window, 128 MB whose address both the
game and the game data name, and the range its image is linked at. Android's
Java runtime maps a large part of the low 4 GB for its own heaps, so both
are claimed here, before the first pool and before SDL maps anything. */
int host_memory_reserve_guest(void)
{
	/* The game's memory window and the guest's own code both sit at fixed
	addresses: the game data is written for 0x80000000, and the image is
	linked where it is. Android's Java runtime has mapped its large object
	space across 0x80000000 by the time the app runs, and that space cannot
	be taken from it, so the window goes to the first free 256 MB-aligned
	range below 4 GB and the guest is told where (halo_guest_boot).
	contiguous_base); the port moves the data with it
	(source/cache/cache_files.c). The image is claimed first, so the window
	search sees it and does not land on top of it. */
	uint64_t minimum = LOW_START, window = 0;
	int attempt;

	image_base = HALO_GUEST_IMAGE_BASE;
	image_end = image_base + HALO_GUEST_IMAGE_RESERVE;
	if (reserve(image_base, HALO_GUEST_IMAGE_RESERVE) != 0)
	{
		host_logf(HOST_LOG_ERROR, "cannot reserve the guest image at %08llx-%08llx:",
			(unsigned long long)image_base, (unsigned long long)image_end);
		log_conflicts(image_base, HALO_GUEST_IMAGE_RESERVE);
		return HOST_MEMORY_NO_IMAGE;
	}
	if (reserve(HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE) == 0)
	{
		window = HALO_GUEST_WINDOW_BASE;
	}
	else
	{
		host_logf(HOST_LOG_INFO, "0x80000000 is taken; the window goes elsewhere");
		for (attempt = 0; attempt < 128 && !window; attempt++)
		{
			window = find_gap(HALO_GUEST_WINDOW_SIZE, minimum, HALO_GUEST_WINDOW_ALIGNMENT);
			if (!window)
				break;
			if (reserve(window, HALO_GUEST_WINDOW_SIZE) == 0)
				break;
			/* taken between the search and the reservation; look further up */
			host_logf(HOST_LOG_INFO, "  %08llx is taken, looking on",
				(unsigned long long)window);
			minimum = window + PAGE;
			window = 0;
		}
	}
	if (!window)
	{
		host_logf(HOST_LOG_ERROR, "no free range of %u MB for the Xbox memory window;"
			" what holds the low 4 GB:", (unsigned int)(HALO_GUEST_WINDOW_SIZE / (1024 * 1024)));
		log_conflicts(LOW_START, LOW_LIMIT - LOW_START);
		return HOST_MEMORY_NO_WINDOW;
	}
	window_base = window;
	window_end = window_base + HALO_GUEST_WINDOW_SIZE;
	host_logf(HOST_LOG_INFO, "Xbox memory window at %08llx-%08llx, guest image at %08llx",
		(unsigned long long)window_base, (unsigned long long)window_end,
		(unsigned long long)image_base);
	reserved = 1;
	return 0;
}

/* where the window ended up, for the guest's boot structure */
uint32_t host_memory_window_base(void)
{
	return (uint32_t)window_base;
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	uint64_t end = (uint64_t)base + round_up(size);


	/* the range was reserved at start-up, for the pools to keep out of */
	if (base != image_base || end > image_end)
	{
		host_logf(HOST_LOG_ERROR, "the guest image at %08x is not where it was reserved",
			base);
		return -1;
	}
	if (end < image_end)
	{
		munmap((void *)(uintptr_t)end, image_end - end);
		image_end = end;
	}
	return 0;
}

/* ---------- page pools */

static void *pool_take(struct pool *pool, uint64_t pages)
{
	uint64_t run = 0, page;

	if (pool->free_pages < pages)
		return NULL;
	for (page = 0; page < POOL_PAGES; page++)
	{
		if (pool->used[page])
		{
			run = 0;
			continue;
		}
		if (++run == pages)
		{
			uint64_t first = page + 1 - pages;

			memset(&pool->used[first], 1, pages);
			pool->free_pages -= pages;
			return (void *)(pool->base + first * PAGE);
		}
	}
	return NULL;
}

void *host_low_map(size_t size, int protection)
{
	uint64_t pages = round_up(size) / PAGE;
	void *address = NULL;
	int index;

	if (!pages || pages > POOL_PAGES)
		return NULL;
	pthread_mutex_lock(&memory_lock);
	for (index = 0; index < pool_count && !address; index++)
		address = pool_take(pools[index], pages);
	if (!address)
	{
		struct pool *pool = pool_new();

		if (pool)
			address = pool_take(pool, pages);
	}
	pthread_mutex_unlock(&memory_lock);
	if (!address)
		return NULL;
	if (mmap(address, pages * PAGE, protection, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != address)
	{
		host_low_unmap(address, pages * PAGE);
		return NULL;
	}
	return address;
}

static struct pool *pool_of(uint64_t address, uint64_t size)
{
	int index;

	for (index = 0; index < pool_count; index++)
	{
		if (in_range(address, size, pools[index]->base, pools[index]->base + POOL_SIZE))
			return pools[index];
	}
	return NULL;
}

void host_low_unmap(void *address, size_t size)
{
	uint64_t start = (uint64_t)address & ~(PAGE - 1);
	uint64_t length = round_up((uint64_t)address + size) - start;
	struct pool *pool;

	pthread_mutex_lock(&memory_lock);
	pool = pool_of(start, length);
	if (pool)
	{
		uint64_t first = (start - pool->base) / PAGE, count = length / PAGE, page;

		/* give the memory back but keep the address space */
		mmap((void *)start, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0);
		for (page = first; page < first + count; page++)
		{
			if (pool->used[page])
			{
				pool->used[page] = 0;
				pool->free_pages++;
			}
		}
	}
	pthread_mutex_unlock(&memory_lock);
}

int host_low_owns(uintptr_t address, size_t size)
{
	int result;

	if (in_range(address, size, window_base, window_end) || in_range(address, size, image_base, image_end))
		return 1;
	pthread_mutex_lock(&memory_lock);
	result = pool_of(address, size) != NULL;
	pthread_mutex_unlock(&memory_lock);
	return result;
}

/* ---------- the guest's memory system calls */

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	uint64_t length = round_up(size);
	void *result;

	if (!length)
		return -EINVAL;
	if (flags & (MAP_FIXED | MAP_FIXED_NOREPLACE))
	{
		int fixed_flags = (flags & ~MAP_FIXED_NOREPLACE) | MAP_FIXED;

		if (address + length > LOW_LIMIT)
			return -ENOMEM;
		/* inside a range the host reserved for the guest, a "no replace"
		request replaces the reservation */
		if (!host_low_owns(address, length))
		{
			if (!(flags & MAP_FIXED_NOREPLACE))
				return -EINVAL;
			fixed_flags = flags;
		}
		result = mmap((void *)address, length, protection, fixed_flags, fd, offset);
		if (result == MAP_FAILED)
			return -errno;
		return (long)(uintptr_t)result;
	}
	result = host_low_map(length, PROT_NONE);
	if (!result)
		return -ENOMEM;
	if (mmap(result, length, protection, flags | MAP_FIXED, fd, offset) != result)
	{
		int error = errno;

		host_low_unmap(result, length);
		return -error;
	}
	return (long)(uintptr_t)result;
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	uint64_t length = round_up(size);

	if (address + length > LOW_LIMIT)
		return -EINVAL;
	if (in_range(address, length, window_base, window_end))
	{
		mmap((void *)address, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0);
		return 0;
	}
	if (in_range(address, length, image_base, image_end))
		return -EINVAL;
	if (host_low_owns(address, length))
	{
		host_low_unmap((void *)address, length);
		return 0;
	}
	return munmap((void *)address, length) ? -errno : 0;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	if (address + size > LOW_LIMIT)
		return -EINVAL;
	return mprotect((void *)address, size, protection) ? -errno : 0;
}

/* ---------- write tracking (port/linux/src/memory_watch.c) */

#define WATCH_PAGE_COUNT (HALO_GUEST_WINDOW_SIZE / PAGE)

static uint8_t page_protected[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static int watch_active;

static int in_window(uint64_t address)
{
	return address >= window_base && address - window_base < HALO_GUEST_WINDOW_SIZE;
}

static uint64_t watch_page(uint64_t address)
{
	return (address - window_base) / PAGE;
}

static void mark_written(uint64_t page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	page_protected[page] = 0;
	mprotect((void *)(window_base + page * PAGE), PAGE, PROT_READ | PROT_WRITE);
}

static struct sigaction previous_segv, previous_bus, previous_ill;

static void report_crash(int signal_number, siginfo_t *information, void *context)
{
	ucontext_t *ucontext = context;
	const struct sigcontext *registers = (const struct sigcontext *)&ucontext->uc_mcontext;
	uint64_t pc = registers->pc, lr = registers->regs[30];
	int index;

	host_logf(HOST_LOG_ERROR, "signal %d at address %p: pc %016llx lr %016llx sp %016llx",
		signal_number, information->si_addr, (unsigned long long)pc, (unsigned long long)lr,
		(unsigned long long)registers->sp);
	if (pc >= host_image.base && pc < host_image.end)
		host_logf(HOST_LOG_ERROR, "  in the guest image: addr2line -e halo_guest.elf 0x%llx 0x%llx",
			(unsigned long long)pc, (unsigned long long)lr);
	for (index = 0; index < 31; index += 4)
	{
		host_logf(HOST_LOG_ERROR, "  x%-2d %016llx %016llx %016llx %016llx", index,
			(unsigned long long)registers->regs[index],
			(unsigned long long)(index + 1 < 31 ? registers->regs[index + 1] : 0),
			(unsigned long long)(index + 2 < 31 ? registers->regs[index + 2] : 0),
			(unsigned long long)(index + 3 < 31 ? registers->regs[index + 3] : 0));
	}
	/* the guest's frame records: fp and lr, 8 bytes each */
	{
		uint64_t fp = registers->regs[29];

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
	/* returning re-executes the faulting instruction under the previous
	(or default) handler */
}

static void segv_handler(int signal_number, siginfo_t *information, void *context)
{
	uint64_t address = (uint64_t)information->si_addr;

	/* the probe reads memory the guest may unmap under it (host_probe.c);
	it handles its own faults and skips the page */
	if (host_probe_skip_fault((uintptr_t)address))
		return;
	if (watch_active && in_window(address))
	{
		uint64_t page = watch_page(address);

		if (page_protected[page])
		{
			mark_written(page);
			return;
		}
	}
	report_crash(signal_number, information, context);
	chain(&previous_segv, signal_number, information, context);
}

static void bus_handler(int signal_number, siginfo_t *information, void *context)
{
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

	memset(&action, 0, sizeof(action));
	action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = segv_handler;
	sigaction(SIGSEGV, &action, &previous_segv);
	action.sa_sigaction = bus_handler;
	sigaction(SIGBUS, &action, &previous_bus);
	action.sa_sigaction = ill_handler;
	sigaction(SIGILL, &action, &previous_ill);
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
			mprotect((void *)(window_base + page * PAGE), PAGE, PROT_READ);
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
	if (start + size <= window_base || start >= (uint64_t)window_base + HALO_GUEST_WINDOW_SIZE)
		return;
	if (start < window_base)
		start = window_base;
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
