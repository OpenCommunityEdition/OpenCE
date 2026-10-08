/*
HOST_MEMORY.C

Guest address space for the macOS port, and the host's fatal signals.

Everything the guest touches must lie below 4 GB. The host executable is
linked with a 64 KB page zero instead of the usual 4 GB, which makes that
range mappable at all; but then the system maps into it too, from the
bottom up and anywhere it likes (under Rosetta: the translation runtime,
the translated code's cache, the frameworks' and the GL driver's memory).
So the very first thing the host does (host_main.c) is claim every page
below 4 GB that is still free, as one inaccessible reservation
(host_memory_claim), and from then on it hands that space out itself:

- the Xbox window at 0x80000000 and the image's own range, at fixed
  addresses the guest was built for (host_memory_initialize);
- the guest's other mappings and thread stacks, wherever there is room
  (host_low_map, and the guest's mmap with Linux's flags);

with one byte per 4 KB page recording what each page is. Nothing the host
does afterwards can land below 4 GB, as the claim leaves no free page there.

This file also implements guest memory write tracking (the interface of
port/linux/src/memory_watch.c): the renderer write-protects the pages behind
the textures it caches, and the fault handler here records the first write
to each. macOS reports such a write fault as SIGBUS (with the exact address),
where Linux reports SIGSEGV; both are handled. Every other fault is a crash:
it is logged with the registers and the guest's frame chain, and the process
ends with _exit, never as a crash the system would report (Rosetta processes
that crash have been seen to hang the system's crash reporting).
*/

#include "host.h"
#include "host_linux.h"

#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define PAGE 0x1000ull
#define LOW_LIMIT 0x100000000ull
#define PAGE_COUNT (LOW_LIMIT / PAGE)
/* (shown as "Memory Tag 240" by vmmap) */
#define CLAIM_TAG 240

enum
{
	/* not the guest's: the page zero, the host's own image and whatever the
	system had mapped before the claim */
	_page_foreign,
	/* claimed, not handed out (an inaccessible reservation) */
	_page_free,
	/* handed out: anonymous memory, or a file's */
	_page_used,
	_page_file,
	/* the Xbox window and the image: the guest maps inside them itself */
	_page_fixed,
};

static uint8_t page_states[PAGE_COUNT];
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;
/* where the search for free pages starts (above the image, once there is
one, as Android places its pools) */
static uint64_t search_start;
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

static void set_states(uint64_t address, uint64_t length, int state)
{
	memset(&page_states[address / PAGE], state, length / PAGE);
}

/* whether every page of the range is in one of the states in the mask
(1 << state) */
static int all_pages(uint64_t address, uint64_t length, unsigned mask)
{
	uint64_t page;

	if (address + length > LOW_LIMIT || address + length < address)
		return 0;
	for (page = address / PAGE; page < (address + length) / PAGE; page++)
	{
		if (!(mask & (1u << page_states[page])))
			return 0;
	}
	return 1;
}

/* gives pages back as an inaccessible reservation (fresh, so their memory
is released), keeping the address space */
static void reserve_again(uint64_t address, uint64_t length)
{
	mmap((void *)address, length, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE | MAP_FIXED, -1, 0);
}

/* ---------- the claim */

int host_memory_claim(void)
{
	mach_port_t task = mach_task_self();
	uint64_t claimed = 0;
	int pass;

	/* (a second pass for anything a gap's neighbour grew into meanwhile:
	the system's own threads may map while this runs) */
	for (pass = 0; pass < 4; pass++)
	{
		mach_vm_address_t cursor = 0;
		int progress = 0;

		while (cursor < LOW_LIMIT)
		{
			mach_vm_address_t region = cursor;
			mach_vm_size_t size = 0;
			vm_region_basic_info_data_64_t information;
			mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
			mach_port_t object = MACH_PORT_NULL;
			kern_return_t result = mach_vm_region(task, &region, &size, VM_REGION_BASIC_INFO_64,
				(vm_region_info_t)&information, &count, &object);
			uint64_t gap_end = result == KERN_SUCCESS ? region : LOW_LIMIT;

			if (gap_end > LOW_LIMIT)
				gap_end = LOW_LIMIT;
			if (gap_end > cursor)
			{
				mach_vm_address_t address = cursor;

				/* VM_FLAGS_FIXED fails rather than replace a mapping
				that appeared meanwhile, as mmap's MAP_FIXED would */
				if (mach_vm_allocate(task, &address, gap_end - cursor, VM_FLAGS_FIXED | VM_MAKE_TAG(CLAIM_TAG))
					== KERN_SUCCESS)
				{
					mach_vm_protect(task, address, gap_end - cursor, FALSE, VM_PROT_NONE);
					set_states(cursor, gap_end - cursor, _page_free);
					claimed += gap_end - cursor;
					progress = 1;
				}
			}
			if (result != KERN_SUCCESS)
				break;
			cursor = region + size;
		}
		if (!progress)
			break;
	}
	host_logf(HOST_LOG_INFO, "claimed %llu MB of the address space below 4 GB",
		(unsigned long long)(claimed >> 20));
	return claimed ? 0 : -1;
}

int host_memory_claimed(uint64_t address, uint64_t size)
{
	return all_pages(address & ~(PAGE - 1), round_up(address + size) - (address & ~(PAGE - 1)), 1u << _page_free);
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	int result = 0;

	pthread_mutex_lock(&memory_lock);
	window_base = HALO_GUEST_WINDOW_BASE;
	window_end = window_base + HALO_GUEST_WINDOW_SIZE;
	image_base = base;
	image_end = base + round_up(size);
	if (!all_pages(window_base, window_end - window_base, 1u << _page_free))
	{
		host_logf(HOST_LOG_ERROR, "the Xbox memory window at %08llx is not free", (unsigned long long)window_base);
		result = -1;
	}
	else if (!all_pages(image_base, image_end - image_base, 1u << _page_free))
	{
		host_logf(HOST_LOG_ERROR, "the guest image's range %08llx-%08llx is not free (is something mapped there "
			"before main?)", (unsigned long long)image_base, (unsigned long long)image_end);
		result = -1;
	}
	else
	{
		set_states(window_base, window_end - window_base, _page_fixed);
		set_states(image_base, image_end - image_base, _page_fixed);
		search_start = image_end;
	}
	pthread_mutex_unlock(&memory_lock);
	return result;
}

/* ---------- handing out pages */

/* the first run of pages free pages from search_start on (wrapping round),
marked as state; 0 if there is none. Called with the lock held. */
static uint64_t take_pages(uint64_t pages, int state)
{
	uint64_t start = search_start / PAGE, page, run = 0;
	int lap;

	for (lap = 0; lap < 2; lap++)
	{
		uint64_t from = lap ? 0 : start, to = lap ? start + pages : PAGE_COUNT;

		if (to > PAGE_COUNT)
			to = PAGE_COUNT;
		run = 0;
		for (page = from; page < to; page++)
		{
			if (page_states[page] != _page_free)
			{
				run = 0;
				continue;
			}
			if (++run == pages)
			{
				uint64_t first = page + 1 - pages;

				memset(&page_states[first], state, pages);
				return first * PAGE;
			}
		}
	}
	return 0;
}

void *host_low_map(size_t size, int protection)
{
	uint64_t length = round_up(size);
	uint64_t address;

	if (!length || length >= LOW_LIMIT)
		return NULL;
	pthread_mutex_lock(&memory_lock);
	address = take_pages(length / PAGE, _page_used);
	if (address &&
		mmap((void *)address, length, protection, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) != (void *)address)
	{
		reserve_again(address, length);
		set_states(address, length, _page_free);
		address = 0;
	}
	pthread_mutex_unlock(&memory_lock);
	return (void *)address;
}

/* frees the handed-out pages of a range (the fixed ranges keep their
state); called with the lock held */
static void release_pages(uint64_t start, uint64_t length)
{
	uint64_t page;

	reserve_again(start, length);
	for (page = start / PAGE; page < (start + length) / PAGE; page++)
	{
		if (page_states[page] == _page_used || page_states[page] == _page_file)
			page_states[page] = _page_free;
	}
}

void host_low_unmap(void *address, size_t size)
{
	uint64_t start = (uint64_t)address & ~(PAGE - 1);
	uint64_t length = round_up((uint64_t)address + size) - start;

	pthread_mutex_lock(&memory_lock);
	if (all_pages(start, length, 1u << _page_used | 1u << _page_file | 1u << _page_free))
		release_pages(start, length);
	pthread_mutex_unlock(&memory_lock);
}

int host_low_owns(uintptr_t address, size_t size)
{
	uint64_t start = address & ~(PAGE - 1);
	uint64_t length = round_up((uint64_t)address + size) - start;

	/* (without the lock: the crash reporter and the sampler call this from
	signal handlers; a page's state is one byte) */
	return size && all_pages(start, length, 1u << _page_used | 1u << _page_file | 1u << _page_fixed);
}

/* ---------- the guest's memory system calls (Linux flags and errors) */

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	uint64_t length = round_up(size);
	int darwin_protection = host_linux_prot_to_darwin(protection);
	int anonymous = (flags & LINUX_MAP_ANONYMOUS) != 0;
	int darwin_flags, no_replace;
	void *result;
	long error = 0;

	if (!length || length >= LOW_LIMIT || darwin_protection < 0 || (offset & (PAGE - 1)) ||
		host_linux_mmap_flags_to_darwin(flags, &darwin_flags, &no_replace) != 0)
	{
		return -LINUX_EINVAL;
	}
	if (anonymous)
	{
		fd = -1;
		offset = 0;
	}
	pthread_mutex_lock(&memory_lock);
	if (darwin_flags & MAP_FIXED)
	{
		if (address & (PAGE - 1))
		{
			error = -LINUX_EINVAL;
		}
		else if (address + length > LOW_LIMIT)
		{
			error = -LINUX_ENOMEM;
		}
		else if (no_replace)
		{
			/* Linux's MAP_FIXED_NOREPLACE: fails if anything is mapped
			there already; a reservation of the guest's own (the window,
			the image) is replaced, as on Android */
			if (!all_pages(address, length, 1u << _page_free | 1u << _page_fixed))
				error = -LINUX_EEXIST;
		}
		else if (!all_pages(address, length, 1u << _page_free | 1u << _page_used | 1u << _page_file |
			1u << _page_fixed))
		{
			/* (never over the host's own memory) */
			error = -LINUX_EINVAL;
		}
		if (!error)
		{
			result = mmap((void *)address, length, darwin_protection, darwin_flags, fd, offset);
			if (result == MAP_FAILED)
			{
				error = -host_linux_errno(errno);
			}
			else
			{
				uint64_t page;

				for (page = address / PAGE; page < (address + length) / PAGE; page++)
				{
					if (page_states[page] != _page_fixed)
						page_states[page] = anonymous ? _page_used : _page_file;
				}
			}
		}
		pthread_mutex_unlock(&memory_lock);
		return error ? error : (long)address;
	}
	/* anywhere (a hint address means nothing here) */
	address = take_pages(length / PAGE, anonymous ? _page_used : _page_file);
	if (!address)
	{
		pthread_mutex_unlock(&memory_lock);
		return -LINUX_ENOMEM;
	}
	result = mmap((void *)address, length, darwin_protection, darwin_flags | MAP_FIXED, fd, offset);
	if (result == MAP_FAILED)
	{
		error = -host_linux_errno(errno);
		reserve_again(address, length);
		set_states(address, length, _page_free);
	}
	pthread_mutex_unlock(&memory_lock);
	return error ? error : (long)address;
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	uint64_t length = round_up(size);
	long error = 0;

	if ((address & (PAGE - 1)) || !length)
		return -LINUX_EINVAL;
	if (address + length > LOW_LIMIT)
		return -LINUX_EINVAL;
	pthread_mutex_lock(&memory_lock);
	if (in_range(address, length, image_base, image_end) && image_end > image_base)
	{
		error = -LINUX_EINVAL;
	}
	else if (!all_pages(address, length, 1u << _page_free | 1u << _page_used | 1u << _page_file |
		1u << _page_fixed))
	{
		error = -LINUX_EINVAL;
	}
	else
	{
		/* (inside the window the reservation stays the guest's) */
		release_pages(address, length);
	}
	pthread_mutex_unlock(&memory_lock);
	return error;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	uint64_t length = round_up(size);
	int darwin_protection = host_linux_prot_to_darwin(protection);

	if ((address & (PAGE - 1)) || darwin_protection < 0)
		return -LINUX_EINVAL;
	if (!length)
		return 0;
	if (!all_pages(address, length, 1u << _page_used | 1u << _page_file | 1u << _page_fixed))
		return -LINUX_ENOMEM;
	return mprotect((void *)address, length, darwin_protection) ? -host_linux_errno(errno) : 0;
}

/* the protection the pages at address have now (VM_PROT_ values), and how
far that holds; 0 if it cannot tell */
static int current_protection(uint64_t address, uint64_t *end)
{
	mach_vm_address_t region = address;
	mach_vm_size_t size = 0;
	vm_region_basic_info_data_64_t information;
	mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
	mach_port_t object = MACH_PORT_NULL;

	if (mach_vm_region(mach_task_self(), &region, &size, VM_REGION_BASIC_INFO_64, (vm_region_info_t)&information,
		&count, &object) != KERN_SUCCESS || region > address)
	{
		return -1;
	}
	*end = region + size;
	return information.protection;
}

long host_guest_madvise(uint64_t address, uint64_t size, int advice)
{
	uint64_t length = round_up(size);
	int darwin_advice = host_linux_madvise_to_darwin(advice);

	if ((address & (PAGE - 1)) || darwin_advice == HOST_LINUX_MADVISE_INVALID)
		return -LINUX_EINVAL;
	if (!length || darwin_advice == HOST_LINUX_MADVISE_IGNORE)
		return 0;
	if (!all_pages(address, length, 1u << _page_used | 1u << _page_file | 1u << _page_fixed))
		return -LINUX_ENOMEM;
	if (darwin_advice == HOST_LINUX_MADVISE_ZERO)
	{
		uint64_t at = address, end = address + length;

		/* fresh anonymous pages with the protection each part has now
		(a file's pages: only the hint, as their contents cannot be had
		back here) */
		pthread_mutex_lock(&memory_lock);
		while (at < end)
		{
			uint64_t region_end = end;
			int protection = current_protection(at, &region_end);

			if (protection < 0)
				break;
			if (region_end > end)
				region_end = end;
			if (page_states[at / PAGE] == _page_file)
				madvise((void *)at, region_end - at, MADV_DONTNEED);
			else
				mmap((void *)at, region_end - at, protection & (PROT_READ | PROT_WRITE | PROT_EXEC),
					MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
			at = region_end;
		}
		pthread_mutex_unlock(&memory_lock);
		return 0;
	}
	return madvise((void *)address, length, darwin_advice) ? -host_linux_errno(errno) : 0;
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

/* page_protected is read by the fault handler on any thread, so it is
accessed atomically: cleared only after the page is writable, set before it
is made read-only */
static int is_protected(uint64_t page)
{
	return __atomic_load_n(&page_protected[page], __ATOMIC_ACQUIRE);
}

static void set_protected(uint64_t page, int value)
{
	__atomic_store_n(&page_protected[page], (uint8_t)value, __ATOMIC_RELEASE);
}

static void mark_written(uint64_t page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	mprotect((void *)(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, PROT_READ | PROT_WRITE);
	/* (after the page is writable: a thread that faulted on it meanwhile
	finds it either still marked, or writable) */
	set_protected(page, 0);
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
		if (!is_protected(page))
		{
			set_protected(page, 1);
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
		if (is_protected(page))
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
		set_protected(page, 0);
		page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}

/* ---------- fatal signals */

static const char *signal_name(int signal_number)
{
	switch (signal_number)
	{
	case SIGSEGV: return "SIGSEGV";
	case SIGBUS: return "SIGBUS";
	case SIGILL: return "SIGILL";
	case SIGFPE: return "SIGFPE";
	case SIGTRAP: return "SIGTRAP";
	case SIGABRT: return "SIGABRT";
	case SIGSYS: return "SIGSYS";
	default: return "signal";
	}
}

void host_log_crash_backtrace(void);

static void report_crash(int signal_number, siginfo_t *information, void *context)
{
	const ucontext_t *ucontext = context;
	const _STRUCT_X86_THREAD_STATE64 *registers = &ucontext->uc_mcontext->__ss;
	const _STRUCT_X86_EXCEPTION_STATE64 *exception = &ucontext->uc_mcontext->__es;
	uint64_t rip = registers->__rip;

	host_logf(HOST_LOG_FATAL, "%s (%d) at address %p: rip %016llx rsp %016llx rbp %016llx (fault address %llx, "
		"error %x, trap %u)", signal_name(signal_number), signal_number, information->si_addr,
		(unsigned long long)rip, (unsigned long long)registers->__rsp, (unsigned long long)registers->__rbp,
		(unsigned long long)exception->__faultvaddr, exception->__err, exception->__trapno);
	if (rip >= host_image.base && rip < host_image.end)
	{
		host_logf(HOST_LOG_FATAL, "  in the guest image: llvm-symbolizer --obj=halo_guest.elf 0x%llx",
			(unsigned long long)rip);
	}
	else
	{
		Dl_info symbol;

		if (dladdr((void *)rip, &symbol) && symbol.dli_sname)
		{
			host_logf(HOST_LOG_FATAL, "  in the host: %s+0x%llx (%s)", symbol.dli_sname,
				(unsigned long long)(rip - (uint64_t)symbol.dli_saddr), symbol.dli_fname);
		}
	}
	host_logf(HOST_LOG_FATAL, "  rax %016llx rbx %016llx rcx %016llx rdx %016llx",
		(unsigned long long)registers->__rax, (unsigned long long)registers->__rbx,
		(unsigned long long)registers->__rcx, (unsigned long long)registers->__rdx);
	host_logf(HOST_LOG_FATAL, "  rsi %016llx rdi %016llx r8  %016llx r9  %016llx",
		(unsigned long long)registers->__rsi, (unsigned long long)registers->__rdi,
		(unsigned long long)registers->__r8, (unsigned long long)registers->__r9);
	host_logf(HOST_LOG_FATAL, "  r10 %016llx r11 %016llx r12 %016llx r13 %016llx",
		(unsigned long long)registers->__r10, (unsigned long long)registers->__r11,
		(unsigned long long)registers->__r12, (unsigned long long)registers->__r13);
	host_logf(HOST_LOG_FATAL, "  r14 %016llx r15 %016llx rflags %08llx",
		(unsigned long long)registers->__r14, (unsigned long long)registers->__r15,
		(unsigned long long)registers->__rflags);
	/* the guest's frame records: x32 code pushes rbp and the call its
	return address, 8 bytes each, as AArch64's frame records are laid out */
	{
		uint64_t fp = registers->__rbp;
		int index;

		for (index = 0; index < 24 && fp && fp < LOW_LIMIT && (fp & 7) == 0 && host_low_owns(fp, 16); index++)
		{
			const uint64_t *frame = (const uint64_t *)fp;

			host_logf(HOST_LOG_FATAL, "  frame %2d: return %016llx", index, (unsigned long long)frame[1]);
			if (frame[0] <= fp)
				break;
			fp = frame[0];
		}
	}
	/* and the host's own, on this stack */
	host_log_crash_backtrace();
}

static volatile int crash_thread_entered;
static pthread_t crash_thread;

static void fatal(int signal_number, siginfo_t *information, void *context)
{
	if (__sync_lock_test_and_set(&crash_thread_entered, 1))
	{
		/* a fault while reporting one: end now; another thread's crash
		meanwhile: let the first report finish */
		if (pthread_equal(crash_thread, pthread_self()))
			_exit(128 + signal_number);
		for (;;)
			pause();
	}
	crash_thread = pthread_self();
	report_crash(signal_number, information, context);
	_exit(128 + signal_number);
}

/* each thread's record of the write faults it let run again, kept at the
bottom of its alternate signal stack (host_signal_stack_install): the same
instruction faulting on the same address again and again, with the page
writable, is not a write the watch can serve, and ends as a crash rather than
spinning */
struct fault_retries
{
	uint64_t rip;
	uint64_t address;
	uint32_t count;
};

#define FAULT_RETRY_LIMIT 1000
#define FAULT_RETRIES_SIZE 64
_Static_assert(sizeof(struct fault_retries) <= FAULT_RETRIES_SIZE, "struct fault_retries");

static struct fault_retries shared_fault_retries; /* (a thread without one) */
static pthread_key_t signal_stack_key;
static volatile int signal_stack_key_ready; /* (key 0 is pthread's own slot) */

static int retry_allowed(uint64_t rip, uint64_t address)
{
	char *mapping = signal_stack_key_ready ? pthread_getspecific(signal_stack_key) : NULL;
	struct fault_retries *retries = mapping ? (struct fault_retries *)(mapping + PAGE) : &shared_fault_retries;

	if (retries->rip != rip || retries->address != address)
	{
		retries->rip = rip;
		retries->address = address;
		retries->count = 0;
	}
	return ++retries->count <= FAULT_RETRY_LIMIT;
}

static void fault_handler(int signal_number, siginfo_t *information, void *context)
{
	const ucontext_t *ucontext = context;
	uint64_t address = (uint64_t)information->si_addr;
	uint64_t rip = ucontext->uc_mcontext->__ss.__rip;
	/* an instruction fetch (a jump through a bad pointer into the window,
	which is never executable) is a crash, whatever the page's state */
	int fetch = rip == address || (ucontext->uc_mcontext->__es.__err & 0x10);

	if ((signal_number == SIGBUS || signal_number == SIGSEGV) && watch_active && in_window(address) && !fetch)
	{
		uint64_t page = watch_page(address);
		uint64_t end;
		int attempt;

		/* the flag and the page's protection are read at different times:
		another thread may clear the flag (its own fault, prepare_write) and
		the renderer protect the page again in between, so look again a few
		times before calling it a crash */
		for (attempt = 0; attempt < 4; attempt++)
		{
			int protection;

			if (is_protected(page))
			{
				mark_written(page);
				if (retry_allowed(rip, address))
					return;
				break;
			}
			/* another thread made the page writable after this one faulted
			on it: running the write again succeeds */
			protection = current_protection(address & ~(PAGE - 1), &end);
			if (protection >= 0 && (protection & VM_PROT_WRITE))
			{
				if (retry_allowed(rip, address))
					return;
				break;
			}
		}
	}
	fatal(signal_number, information, context);
}

void host_install_signal_handlers(void)
{
	static const int fatal_signals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGTRAP, SIGABRT, SIGSYS };
	struct sigaction action;
	size_t index;

	/* The handlers below keep crashes away from the system's ReportCrash
	by ending the process with _exit (under Rosetta, a crash report of a
	translated process once left every later x86_64 launch hung). Clearing
	the task's exception ports would not: the kernel then delivers to the
	host-level port, where ReportCrash listens. What still reaches it: a
	stack overflow on a thread without an alternate signal stack, and kills
	that send no signal */
	host_signal_stack_install();
	memset(&action, 0, sizeof(action));
	/* (SA_NODEFER: a fault inside the handler re-enters it, which ends the
	process, rather than being blocked, which the kernel turns into a
	crash) */
	action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = fault_handler;
	for (index = 0; index < sizeof(fatal_signals) / sizeof(fatal_signals[0]); index++)
		sigaction(fatal_signals[index], &action, NULL);
}

/* ---------- alternate signal stacks */

#define SIGNAL_STACK_SIZE (256 * 1024)

static pthread_once_t signal_stack_once = PTHREAD_ONCE_INIT;

static void signal_stack_free(void *mapping)
{
	stack_t disable;

	memset(&disable, 0, sizeof(disable));
	disable.ss_flags = SS_DISABLE;
	sigaltstack(&disable, NULL);
	munmap(mapping, SIGNAL_STACK_SIZE + PAGE);
}

static void signal_stack_key_create(void)
{
	if (pthread_key_create(&signal_stack_key, signal_stack_free) == 0)
		signal_stack_key_ready = 1;
}

void host_signal_stack_install(void)
{
	stack_t stack;
	void *mapping;

	pthread_once(&signal_stack_once, signal_stack_key_create);
	if (!signal_stack_key_ready || pthread_getspecific(signal_stack_key))
		return;
	/* (anywhere: the handlers are host code; above 4 GB once the claim is
	made) with a guard page below */
	mapping = mmap(NULL, SIGNAL_STACK_SIZE + PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (mapping == MAP_FAILED)
		return;
	mprotect(mapping, PAGE, PROT_NONE);
	memset(&stack, 0, sizeof(stack));
	/* (the stack grows down from its top; its bottom bytes hold the
	thread's struct fault_retries) */
	memset((char *)mapping + PAGE, 0, FAULT_RETRIES_SIZE);
	stack.ss_sp = (char *)mapping + PAGE + FAULT_RETRIES_SIZE;
	stack.ss_size = SIGNAL_STACK_SIZE - FAULT_RETRIES_SIZE;
	if (sigaltstack(&stack, NULL) != 0)
	{
		munmap(mapping, SIGNAL_STACK_SIZE + PAGE);
		return;
	}
	pthread_setspecific(signal_stack_key, mapping);
}
