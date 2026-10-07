/*
HOST_MEMORY.C

Guest address space for the macOS port.

A native macOS process cannot map anything in its low 4 GB, so the
guest's 4 GB lie at host_guest_base instead, a multiple of 4 GB reserved at
start-up (the guest's code adds it to every address it uses,
tools/guest_asm_rebase.py). Inside them:

- the first 16 MB stay unmapped, so that null pointers fault;
- the Xbox contiguous window and the image's range are at the guest
  addresses the image was built for (halo_android_abi.h);
- the rest is handed out for the guest's own mappings (its malloc's) and
  the host's (thread stacks).

The guest's pages are 4 KB (its musl, and the Xbox memory it emulates,
port/linux/src/xbox_memory.c); Apple silicon's are 16 KB. The guest's
mappings are therefore kept here per 4 KB page, and each host page is
mapped with what its four guest pages together need: unmapped while none is
in use, otherwise with every protection any of them has. A guest page that
comes into use in a host page already mapped is cleared, as a fresh mapping
would be. Mappings the guest does not place go on whole host pages, so
that they never share one with another.

This file also implements guest memory write tracking (the interface of
port/linux/src/memory_watch.c), per host page: the renderer write-protects
the pages behind the textures it caches, and the fault handler here records
the first write to each. Other faults are reported (with guest addresses)
and passed on.
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

uint64_t host_guest_base;

#define GUEST_PAGE 0x1000ull
#define GUEST_PAGE_COUNT (HALO_MACOS_GUEST_SPAN / GUEST_PAGE)
/* nothing of the guest's below here (null pointers fault) or from the top
64 KB (where address arithmetic would wrap) */
#define LOW_START 0x01000000ull
#define HIGH_END 0xffff0000ull

/* the guest's Linux mmap flags (musl's arm64_32 bits/mman.h) */
#define LINUX_MAP_SHARED 0x01
#define LINUX_MAP_PRIVATE 0x02
#define LINUX_MAP_FIXED 0x10
#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MAP_FIXED_NOREPLACE 0x100000
#define LINUX_MADV_DONTNEED 4

/* a guest page: free, or in use with its protection (PROT_*, 0 for a
reservation), or the padding that fills the last host page of a mapping
the host placed (freed with the mapping before it) */
#define PAGE_FREE 0x00
#define PAGE_IN_USE 0x80
#define PAGE_PADDING 0x40
#define PAGE_PROTECTION 0x07

static uint8_t page_state[GUEST_PAGE_COUNT];
static uint64_t host_page;
static uint64_t pages_per_host_page;
/* each host page's protection as mapped, -1 while unmapped */
static int8_t *host_protection;
static uint64_t host_page_count;
/* where the search for free host pages starts next */
static uint64_t search_from;
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t window_base, window_end;
static uint32_t image_base, image_end;

static void watch_forget_host_page(uint64_t host_index);

static uint64_t round_up(uint64_t value, uint64_t unit)
{
	return (value + unit - 1) & ~(unit - 1);
}

static int in_range(uint64_t address, uint64_t size, uint64_t base, uint64_t end)
{
	return address >= base && address + size <= end && address + size >= address;
}

static void *host_address(uint64_t guest)
{
	return (void *)(uintptr_t)(host_guest_base + guest);
}

/* ---------- host pages */

/* maps host page index as its guest pages need it; the guest pages in
fresh (those coming into use) are cleared */
static void host_page_apply(uint64_t index, const uint8_t *fresh)
{
	uint64_t first = index * pages_per_host_page, page;
	int protection = -1;
	int clear = 0;

	for (page = first; page < first + pages_per_host_page; page++)
	{
		if (page_state[page] & PAGE_IN_USE)
			protection = (protection < 0 ? 0 : protection) | (page_state[page] & PAGE_PROTECTION);
		else if (page_state[page] & PAGE_PADDING)
			protection = protection < 0 ? 0 : protection;
		if (fresh && fresh[page - first])
			clear = 1;
	}
	if (protection == host_protection[index] && !clear)
		return;
	if (protection >= 0 && (protection & (PROT_WRITE | PROT_EXEC)) == (PROT_WRITE | PROT_EXEC))
	{
		/* (which macOS refuses: the image's code and data never share a
		host page, host_loader.c) */
		host_logf(HOST_LOG_ERROR, "guest page %08llx cannot be both writable and executable",
			(unsigned long long)(index * host_page));
	}
	watch_forget_host_page(index);
	if (protection < 0)
	{
		/* (a fresh mapping: the old pages' memory goes back to the system) */
		mmap(host_address(index * host_page), host_page, PROT_NONE,
			MAP_PRIVATE | MAP_ANON | MAP_FIXED | MAP_NORESERVE, -1, 0);
		host_protection[index] = -1;
		return;
	}
	if (host_protection[index] < 0)
	{
		/* unmapped until now, so already clear */
		mprotect(host_address(index * host_page), host_page, protection);
		host_protection[index] = (int8_t)protection;
		return;
	}
	if (clear)
	{
		mprotect(host_address(index * host_page), host_page, PROT_READ | PROT_WRITE);
		for (page = first; page < first + pages_per_host_page; page++)
		{
			if (fresh[page - first])
				memset(host_address(page * GUEST_PAGE), 0, GUEST_PAGE);
		}
		host_protection[index] = PROT_READ | PROT_WRITE;
	}
	if (protection != host_protection[index])
	{
		mprotect(host_address(index * host_page), host_page, protection);
		host_protection[index] = (int8_t)protection;
	}
}

/* sets the guest pages of [address, address + size) to state, and maps
their host pages to match; new_mapping is nonzero for a new mapping, whose
pages are cleared */
static void pages_set(uint64_t address, uint64_t size, uint8_t state, int new_mapping)
{
	uint64_t first = address / GUEST_PAGE, last = round_up(address + size, GUEST_PAGE) / GUEST_PAGE;
	uint64_t host_first = first / pages_per_host_page;
	uint64_t host_last = (last + pages_per_host_page - 1) / pages_per_host_page;
	uint64_t index, page;

	for (index = host_first; index < host_last; index++)
	{
		uint64_t host_start = index * pages_per_host_page;
		uint8_t fresh[16] = { 0 };
		int whole = 1;

		for (page = host_start; page < host_start + pages_per_host_page; page++)
		{
			if (page < first || page >= last)
			{
				whole = 0;
				continue;
			}
			if (new_mapping && (state & PAGE_IN_USE))
				fresh[page - host_start] = 1;
			page_state[page] = state;
		}
		if (whole && new_mapping && host_protection[index] >= 0)
		{
			/* every guest page of it new: fresh host memory at once */
			watch_forget_host_page(index);
			mmap(host_address(index * host_page), host_page, PROT_NONE,
				MAP_PRIVATE | MAP_ANON | MAP_FIXED | MAP_NORESERVE, -1, 0);
			host_protection[index] = -1;
			host_page_apply(index, NULL);
			continue;
		}
		host_page_apply(index, new_mapping ? fresh : NULL);
	}
}

/* frees [address, address + size) and the padding after it */
static void pages_free(uint64_t address, uint64_t size)
{
	uint64_t end = round_up(address + size, GUEST_PAGE);

	while (end < HALO_MACOS_GUEST_SPAN && page_state[end / GUEST_PAGE] == PAGE_PADDING)
		end += GUEST_PAGE;
	pages_set(address, end - address, PAGE_FREE, 0);
}

/* the first of count free host pages in a row, or 0 */
static uint64_t host_pages_find(uint64_t count)
{
	uint64_t low = LOW_START / host_page, high = HIGH_END / host_page;
	int pass;

	for (pass = 0; pass < 2; pass++)
	{
		uint64_t index = pass ? low : (search_from > low ? search_from : low);
		uint64_t run = 0;

		for (; index < high; index++)
		{
			uint64_t page = index * pages_per_host_page, sub;
			int free_page = 1;

			for (sub = 0; sub < pages_per_host_page; sub++)
			{
				if (page_state[page + sub] != PAGE_FREE)
				{
					free_page = 0;
					break;
				}
			}
			if (!free_page)
			{
				run = 0;
				continue;
			}
			if (++run == count)
			{
				search_from = index + 1;
				return (index + 1 - count) * host_page;
			}
		}
	}
	return 0;
}

/* a new mapping placed by the host: whole host pages, the last one's
pages beyond size padding; its guest address, or 0 */
static uint64_t place(uint64_t size, int protection)
{
	uint64_t length = round_up(size, GUEST_PAGE);
	uint64_t whole = round_up(length, host_page);
	uint64_t address = host_pages_find(whole / host_page);

	if (!address)
		return 0;
	pages_set(address, length, (uint8_t)(PAGE_IN_USE | protection), 1);
	if (whole > length)
		pages_set(address + length, whole - length, PAGE_PADDING, 0);
	return address;
}

/* ---------- start-up */

int host_memory_initialize(uint32_t base, uint32_t size)
{
	uint64_t span = HALO_MACOS_GUEST_SPAN;
	uint8_t *reservation;
	uint64_t start, end;

	host_page = (uint64_t)getpagesize();
	if (host_page < GUEST_PAGE || host_page > 16 * GUEST_PAGE || host_page % GUEST_PAGE)
	{
		host_logf(HOST_LOG_ERROR, "pages of %llu bytes are not supported", (unsigned long long)host_page);
		return -1;
	}
	pages_per_host_page = host_page / GUEST_PAGE;
	host_page_count = span / host_page;
	host_protection = malloc(host_page_count);
	if (!host_protection)
		return -1;
	memset(host_protection, -1, host_page_count);

	/* 8 GB, of which the 4 GB-aligned 4 GB inside are kept */
	reservation = mmap(NULL, span * 2, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
	if (reservation == MAP_FAILED)
	{
		host_logf(HOST_LOG_ERROR, "cannot reserve the guest's 4 GB (%s)", strerror(errno));
		return -1;
	}
	start = round_up((uint64_t)(uintptr_t)reservation, span);
	end = start + span;
	if (start > (uint64_t)(uintptr_t)reservation)
		munmap(reservation, start - (uint64_t)(uintptr_t)reservation);
	if ((uint64_t)(uintptr_t)reservation + span * 2 > end)
		munmap((void *)(uintptr_t)end, (uint64_t)(uintptr_t)reservation + span * 2 - end);
	host_guest_base = start;

	/* the guest may never have these */
	memset(page_state, PAGE_IN_USE, LOW_START / GUEST_PAGE);
	memset(page_state + HIGH_END / GUEST_PAGE, PAGE_IN_USE, (span - HIGH_END) / GUEST_PAGE);
	/* the Xbox window, reserved until the guest maps it
	(port/linux/src/xbox_memory.c) */
	window_base = HALO_GUEST_WINDOW_BASE;
	window_end = HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE;
	memset(page_state + window_base / GUEST_PAGE, PAGE_IN_USE, HALO_GUEST_WINDOW_SIZE / GUEST_PAGE);
	/* the image's range, which the loader maps */
	image_base = base;
	image_end = (uint32_t)round_up((uint64_t)base + size, host_page);
	memset(page_state + image_base / GUEST_PAGE, PAGE_IN_USE, (image_end - image_base) / GUEST_PAGE);
	search_from = image_end / host_page;
	host_logf(HOST_LOG_INFO, "guest memory at %llx, %llu-byte pages", (unsigned long long)host_guest_base,
		(unsigned long long)host_page);
	return 0;
}

int host_image_map(uint32_t address, uint32_t size, int protection)
{
	if (!in_range(address, size, image_base, image_end))
		return -1;
	pthread_mutex_lock(&memory_lock);
	pages_set(address, size, (uint8_t)(PAGE_IN_USE | protection), 1);
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

int host_image_protect(uint32_t address, uint32_t size, int protection)
{
	if (!in_range(address, size, image_base, image_end))
		return -1;
	pthread_mutex_lock(&memory_lock);
	pages_set(address, size, (uint8_t)(PAGE_IN_USE | protection), 0);
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

/* ---------- the host's own guest memory */

uint32_t host_low_map(size_t size)
{
	uint64_t address;

	pthread_mutex_lock(&memory_lock);
	address = place(size, PROT_READ | PROT_WRITE);
	pthread_mutex_unlock(&memory_lock);
	return (uint32_t)address;
}

void host_low_unmap(uint32_t address, size_t size)
{
	pthread_mutex_lock(&memory_lock);
	pages_free(address, size);
	pthread_mutex_unlock(&memory_lock);
}

/* ---------- the guest's memory system calls */

static int protection_from_linux(int protection)
{
	/* (the same bits; the guest has no business making code) */
	return protection & (PROT_READ | PROT_WRITE);
}

long host_guest_mmap(uint32_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	uint64_t length = round_up(size, GUEST_PAGE);
	uint64_t result;
	uint8_t state = (uint8_t)(PAGE_IN_USE | protection_from_linux(protection));

	if (!length || length > HIGH_END)
		return -EINVAL;
	if (fd >= 0 && (flags & LINUX_MAP_SHARED))
		return -ENODEV;
	if (fd < 0 && !(flags & LINUX_MAP_ANONYMOUS))
		return -EBADF;
	pthread_mutex_lock(&memory_lock);
	if (flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE))
	{
		uint64_t page;

		if (address % GUEST_PAGE || (uint64_t)address + length > HIGH_END || address < LOW_START)
		{
			pthread_mutex_unlock(&memory_lock);
			return -EINVAL;
		}
		/* a "no replace" request fails over another mapping, but replaces
		the window's reservation, which is the guest's (Android's host does
		the same) */
		if (flags & LINUX_MAP_FIXED_NOREPLACE)
		{
			for (page = address / GUEST_PAGE; page < (address + length) / GUEST_PAGE; page++)
			{
				uint64_t at = page * GUEST_PAGE;

				if (page_state[page] != PAGE_FREE && !(at >= window_base && at < window_end))
				{
					pthread_mutex_unlock(&memory_lock);
					return -EEXIST;
				}
			}
		}
		/* (never over the image) */
		if ((uint64_t)address < image_end && (uint64_t)address + length > image_base)
		{
			pthread_mutex_unlock(&memory_lock);
			return -EINVAL;
		}
		pages_set(address, length, state, 1);
		result = address;
	}
	else
	{
		result = place(length, protection_from_linux(protection));
		if (!result)
		{
			pthread_mutex_unlock(&memory_lock);
			return -ENOMEM;
		}
	}
	pthread_mutex_unlock(&memory_lock);
	if (fd >= 0)
	{
		/* a private file mapping: the file's bytes, read once */
		uint64_t done = 0;

		if (!(protection & PROT_WRITE))
			host_guest_mprotect((uint32_t)result, length, PROT_READ | PROT_WRITE);
		while (done < size)
		{
			ssize_t got = pread(fd, host_address(result + done), (size_t)(size - done), (off_t)(offset + done));

			if (got <= 0)
				break;
			done += (uint64_t)got;
		}
		if (!(protection & PROT_WRITE))
			host_guest_mprotect((uint32_t)result, length, protection);
	}
	return (long)result;
}

long host_guest_munmap(uint32_t address, uint64_t size)
{
	uint64_t length = round_up(size, GUEST_PAGE);

	if (address % GUEST_PAGE || !length || (uint64_t)address + length > HALO_MACOS_GUEST_SPAN)
		return -EINVAL;
	if ((uint64_t)address < image_end && (uint64_t)address + length > image_base)
		return -EINVAL;
	pthread_mutex_lock(&memory_lock);
	if (in_range(address, length, window_base, window_end))
	{
		/* the window stays the guest's, reserved */
		pages_set(address, length, PAGE_IN_USE, 0);
	}
	else if (address >= LOW_START && (uint64_t)address + length <= HIGH_END)
	{
		pages_free(address, length);
	}
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

long host_guest_mprotect(uint32_t address, uint64_t size, int protection)
{
	uint64_t length = round_up(size, GUEST_PAGE);
	uint64_t page;

	if (address % GUEST_PAGE || (uint64_t)address + length > HALO_MACOS_GUEST_SPAN)
		return -EINVAL;
	pthread_mutex_lock(&memory_lock);
	for (page = address / GUEST_PAGE; page < (address + length) / GUEST_PAGE; page++)
	{
		if (!(page_state[page] & PAGE_IN_USE))
		{
			pthread_mutex_unlock(&memory_lock);
			return -ENOMEM;
		}
	}
	pages_set(address, length, (uint8_t)(PAGE_IN_USE | protection_from_linux(protection)), 0);
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

long host_guest_madvise(uint32_t address, uint64_t size, int advice)
{
	uint64_t length = round_up(size, GUEST_PAGE);
	uint64_t page;

	if ((uint64_t)address + length > HALO_MACOS_GUEST_SPAN)
		return -EINVAL;
	/* only "don't need" changes what the guest sees: its pages read as
	zero again */
	if (advice != LINUX_MADV_DONTNEED)
		return 0;
	pthread_mutex_lock(&memory_lock);
	for (page = address / GUEST_PAGE; page < (address + length) / GUEST_PAGE; page++)
	{
		if (page_state[page] & PAGE_IN_USE)
			pages_set(page * GUEST_PAGE, GUEST_PAGE, page_state[page], 1);
	}
	pthread_mutex_unlock(&memory_lock);
	return 0;
}

int host_guest_writable(uint32_t address, uint64_t size)
{
	uint64_t page, last;

	if (!size)
		return 1;
	if ((uint64_t)address + size > HALO_MACOS_GUEST_SPAN)
		return 0;
	last = ((uint64_t)address + size - 1) / GUEST_PAGE;
	for (page = address / GUEST_PAGE; page <= last; page++)
	{
		if ((page_state[page] & (PAGE_IN_USE | PROT_WRITE)) != (PAGE_IN_USE | PROT_WRITE))
			return 0;
	}
	return 1;
}

/* ---------- write tracking (port/linux/src/memory_watch.c), per host page */

#define WATCH_PAGE_MAXIMUM (HALO_GUEST_WINDOW_SIZE / GUEST_PAGE)

static uint8_t watch_protected[WATCH_PAGE_MAXIMUM];
static uint32_t watch_generation[WATCH_PAGE_MAXIMUM];
static volatile uint32_t current_generation = 1;
static int watch_active;

static int in_window(uint64_t address)
{
	return address >= HALO_GUEST_WINDOW_BASE && address - HALO_GUEST_WINDOW_BASE < HALO_GUEST_WINDOW_SIZE;
}

/* the window's host page that guest address is in */
static uint64_t watch_page(uint64_t address)
{
	return (address - HALO_GUEST_WINDOW_BASE) / host_page;
}

static uint64_t watch_page_count(void)
{
	return HALO_GUEST_WINDOW_SIZE / host_page;
}

static void mark_written(uint64_t page)
{
	uint64_t index = (HALO_GUEST_WINDOW_BASE / host_page) + page;

	watch_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	watch_protected[page] = 0;
	if (host_protection[index] >= 0)
		mprotect(host_address(index * host_page), host_page, host_protection[index]);
}

/* a host page's mapping changed: whatever it held was rewritten */
static void watch_forget_host_page(uint64_t index)
{
	uint64_t address = index * host_page;
	uint64_t page;

	if (!in_window(address))
		return;
	page = watch_page(address);
	watch_protected[page] = 0;
	watch_generation[page] = __sync_add_and_fetch(&current_generation, 1);
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
	if (last >= watch_page_count())
		last = watch_page_count() - 1;
	for (page = first; page <= last; page++)
	{
		uint64_t index = (HALO_GUEST_WINDOW_BASE / host_page) + page;

		if (!watch_protected[page] && host_protection[index] >= 0 && (host_protection[index] & PROT_WRITE))
		{
			watch_protected[page] = 1;
			mprotect(host_address(index * host_page), host_page, PROT_READ);
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
	if (last >= watch_page_count())
		last = watch_page_count() - 1;
	for (page = first; page <= last; page++)
	{
		if (watch_generation[page] > newest)
			newest = watch_generation[page];
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
	if (last >= watch_page_count())
		last = watch_page_count() - 1;
	for (page = first; page <= last; page++)
	{
		if (watch_protected[page])
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
	if (last >= watch_page_count())
		last = watch_page_count() - 1;
	for (page = first; page <= last; page++)
	{
		if (watch_protected[page])
			mark_written(page);
		watch_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}

/* ---------- faults */

static struct sigaction previous_segv, previous_bus, previous_ill;

static void report_crash(int signal_number, siginfo_t *information, void *context)
{
	ucontext_t *ucontext = context;
	const struct __darwin_arm_thread_state64 *registers = &ucontext->uc_mcontext->__ss;
	uint64_t pc = __darwin_arm_thread_state64_get_pc(*registers);
	uint64_t lr = __darwin_arm_thread_state64_get_lr(*registers);
	uint64_t fp = __darwin_arm_thread_state64_get_fp(*registers);
	uint64_t sp = __darwin_arm_thread_state64_get_sp(*registers);
	uint64_t address = (uint64_t)(uintptr_t)information->si_addr;
	int index;

	host_logf(HOST_LOG_ERROR, "signal %d at address %llx (guest %08llx): pc %016llx lr %016llx sp %016llx",
		signal_number, (unsigned long long)address,
		(unsigned long long)(address - host_guest_base), (unsigned long long)pc, (unsigned long long)lr,
		(unsigned long long)sp);
	if (pc - host_guest_base >= host_image.base && pc - host_guest_base < host_image.end)
	{
		host_logf(HOST_LOG_ERROR, "  in the guest image: llvm-symbolizer --obj=build/macos/halo_guest.elf 0x%llx 0x%llx",
			(unsigned long long)(pc - host_guest_base), (unsigned long long)(lr - host_guest_base));
	}
	for (index = 0; index < 29; index += 4)
	{
		host_logf(HOST_LOG_ERROR, "  x%-2d %016llx %016llx %016llx %016llx", index,
			(unsigned long long)registers->__x[index],
			(unsigned long long)(index + 1 < 29 ? registers->__x[index + 1] : fp),
			(unsigned long long)(index + 2 < 29 ? registers->__x[index + 2] : lr),
			(unsigned long long)(index + 3 < 29 ? registers->__x[index + 3] : 0));
	}
	/* the frame records: fp and lr, 8 bytes each */
	for (index = 0; index < 32 && fp - host_guest_base < HALO_MACOS_GUEST_SPAN && (fp & 7) == 0; index++)
	{
		const uint64_t *frame = (const uint64_t *)(uintptr_t)fp;

		if (!host_guest_writable((uint32_t)(fp - host_guest_base), 16))
			break;
		host_logf(HOST_LOG_ERROR, "  frame %2d: return %08llx", index,
			(unsigned long long)(uint32_t)frame[1]);
		fp = frame[0];
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

/* a write to a watched page (macOS reports a write to a read-only page as
SIGBUS, an unmapped one as SIGSEGV) */
static int watch_fault(siginfo_t *information)
{
	uint64_t address = (uint64_t)(uintptr_t)information->si_addr - host_guest_base;

	if (watch_active && address < HALO_MACOS_GUEST_SPAN && in_window(address))
	{
		uint64_t page = watch_page(address);

		if (watch_protected[page])
		{
			mark_written(page);
			return 1;
		}
	}
	return 0;
}

static void segv_handler(int signal_number, siginfo_t *information, void *context)
{
	if (watch_fault(information))
		return;
	report_crash(signal_number, information, context);
	chain(&previous_segv, signal_number, information, context);
}

static void bus_handler(int signal_number, siginfo_t *information, void *context)
{
	if (watch_fault(information))
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

	memset(&action, 0, sizeof(action));
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = segv_handler;
	sigaction(SIGSEGV, &action, &previous_segv);
	action.sa_sigaction = bus_handler;
	sigaction(SIGBUS, &action, &previous_bus);
	action.sa_sigaction = ill_handler;
	sigaction(SIGILL, &action, &previous_ill);
}
