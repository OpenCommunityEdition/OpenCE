#include "host.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#ifdef __aarch64__
#define PAGE 0x4000ULL
#else
#define PAGE 0x1000ULL
#endif
#define LOW_LIMIT 0x100000000ULL
#define RESERVE_BASE 0x40000000ULL
#define POOL_BASE ((uint64_t)HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
#define POOL_PAGES ((LOW_LIMIT - POOL_BASE) / PAGE)
#define HOST(address) HALO_GUEST_ADDRESS(address)
#define GUEST(pointer) ((uint64_t)(uintptr_t)(pointer) - HALO_GUEST_BIAS)

#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MAP_FIXED_NOREPLACE 0x100000
#define LINUX_MADV_DONTNEED 4

static uint8_t pool_used[POOL_PAGES];
static uint64_t pool_next;
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t image_base, image_end;

static uint64_t round_up(uint64_t value)
{
	return (value + PAGE - 1) & ~(PAGE - 1);
}

static int in_range(uint64_t address, uint64_t size, uint64_t base, uint64_t end)
{
	return address >= base && address + size <= end && address + size >= address;
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	void *reserved;

	if (base != HALO_GUEST_IMAGE_BASE || size > HALO_GUEST_IMAGE_RESERVE)
	{
		host_logf(HOST_LOG_ERROR, "the guest image (%08x, %u bytes) does not fit its range", base, size);
		return -1;
	}
	reserved = mmap(HOST(RESERVE_BASE), LOW_LIMIT - RESERVE_BASE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (reserved != HOST(RESERVE_BASE))
	{
		host_logf(HOST_LOG_ERROR, "the guest's memory at %p is taken (%p)", HOST(RESERVE_BASE), reserved);
		return -1;
	}
	image_base = base;
	image_end = base + round_up(size);
	return 0;
}

static int map_fresh(uint64_t address, uint64_t size, int protection)
{
	uint64_t start = round_up(address), end = (address + size) & ~(PAGE - 1), page;

	if (start < end && mmap(HOST(start), end - start, protection, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) ==
		MAP_FAILED)
	{
		return -1;
	}
	for (page = address & ~(PAGE - 1); page < address + size; page += PAGE)
	{
		uint64_t from = page > address ? page : address, to = page + PAGE < address + size ? page + PAGE : address + size;

		if (page >= start && page + PAGE <= end)
		{
			page = end - PAGE;
			continue;
		}
		mprotect(HOST(page), PAGE, PROT_READ | PROT_WRITE);
		if (protection != PROT_NONE)
			memset(HOST(from), 0, to - from);
	}
	return 0;
}

void *host_low_map(size_t size, int protection)
{
	uint64_t pages = round_up(size) / PAGE, run = 0, page, tried;
	uint64_t address = 0;

	if (!pages || pages > POOL_PAGES)
		return NULL;
	pthread_mutex_lock(&memory_lock);
	for (page = pool_next, tried = 0; tried < POOL_PAGES + pages; tried++, page++)
	{
		if (page == POOL_PAGES)
		{
			page = 0;
			run = 0;
		}
		if (pool_used[page])
		{
			run = 0;
			continue;
		}
		if (++run == pages)
		{
			uint64_t first = page + 1 - pages;

			memset(&pool_used[first], 1, pages);
			pool_next = page + 1 == POOL_PAGES ? 0 : page + 1;
			address = POOL_BASE + first * PAGE;
			break;
		}
	}
	pthread_mutex_unlock(&memory_lock);
	if (address && mmap(HOST(address), pages * PAGE, protection, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) !=
		HOST(address))
	{
		host_low_unmap(HOST(address), pages * PAGE);
		address = 0;
	}
	return address ? HOST(address) : NULL;
}

void host_low_unmap(void *pointer, size_t size)
{
	uint64_t start = GUEST(pointer) & ~(PAGE - 1);
	uint64_t length = round_up(GUEST(pointer) + size) - start;

	if (!in_range(start, length, POOL_BASE, LOW_LIMIT))
		return;
	mmap(HOST(start), length, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
	pthread_mutex_lock(&memory_lock);
	memset(&pool_used[(start - POOL_BASE) / PAGE], 0, length / PAGE);
	pthread_mutex_unlock(&memory_lock);
}

static int host_low_owns(uintptr_t pointer, size_t size)
{
	return in_range(GUEST(pointer), size, RESERVE_BASE, LOW_LIMIT);
}

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	int darwin_flags = (flags & (MAP_SHARED | MAP_PRIVATE)) | (flags & LINUX_MAP_ANONYMOUS ? MAP_ANON : 0);
	void *result;

	if (!size)
		return -EINVAL;
	if (flags & (MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE))
	{
		if (!(flags & LINUX_MAP_ANONYMOUS) || !in_range(address, size, RESERVE_BASE, LOW_LIMIT) ||
			in_range(address, size, image_base, image_end))
		{
			return -EINVAL;
		}
		return map_fresh(address, size, protection) ? -host_linux_errno(errno) : (long)address;
	}
	result = host_low_map(size, PROT_NONE);
	if (!result)
		return -ENOMEM;
	if (mmap(result, round_up(size), protection, darwin_flags | MAP_FIXED, fd, offset) != result)
	{
		int error = errno;

		host_low_unmap(result, size);
		return -host_linux_errno(error);
	}
	return (long)GUEST(result);
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	if (!in_range(address, size, RESERVE_BASE, LOW_LIMIT) || in_range(address, size, image_base, image_end))
		return -EINVAL;
	if (address >= POOL_BASE)
		host_low_unmap(HOST(address), size);
	else
		map_fresh(address, size, PROT_NONE);
	return 0;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	uint64_t start = round_up(address), end = (address + size) & ~(PAGE - 1);

	if (!in_range(address, size, RESERVE_BASE, LOW_LIMIT))
		return -EINVAL;
	if (start < end && mprotect(HOST(start), end - start, protection))
		return -host_linux_errno(errno);
	if (protection != PROT_NONE && address < start)
		mprotect(HOST(address & ~(PAGE - 1)), PAGE, PROT_READ | PROT_WRITE);
	if (protection != PROT_NONE && address + size > end && (address + size) & (PAGE - 1))
		mprotect(HOST(end), PAGE, PROT_READ | PROT_WRITE);
	return 0;
}

long host_guest_madvise(uint64_t address, uint64_t size, int advice)
{
	if (!in_range(address, size, RESERVE_BASE, LOW_LIMIT))
		return -EINVAL;
	if (advice == LINUX_MADV_DONTNEED && map_fresh(address, size, PROT_READ | PROT_WRITE))
		return -host_linux_errno(errno);
	return 0;
}

#define WATCH_PAGE_COUNT (HALO_GUEST_WINDOW_SIZE / PAGE)

static uint8_t page_protected[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static int watch_active;

static uint64_t watch_page(uint64_t address)
{
	return (address - HALO_GUEST_WINDOW_BASE) / PAGE;
}

static int watch_pages(uint64_t address, uint64_t size, uint64_t *first, uint64_t *last)
{
	uint64_t window_end = (uint64_t)HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE;

	if (!size || address + size <= HALO_GUEST_WINDOW_BASE || address >= window_end)
		return 0;
	*first = watch_page(address < HALO_GUEST_WINDOW_BASE ? HALO_GUEST_WINDOW_BASE : address);
	*last = watch_page((address + size < window_end ? address + size : window_end) - 1);
	return 1;
}

static void mark_written(uint64_t page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	page_protected[page] = 2;
	mprotect(HOST(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, PROT_READ | PROT_WRITE);
}

static struct sigaction previous_segv, previous_bus, previous_ill;

static void report_crash(int signal_number, siginfo_t *information, void *context)
{
#ifdef __aarch64__
	const struct __darwin_arm_thread_state64 *registers = &((ucontext_t *)context)->uc_mcontext->__ss;
	uint64_t pc = registers->__pc, sp = registers->__sp, frame = registers->__fp;
#else
	const struct __darwin_x86_thread_state64 *registers = &((ucontext_t *)context)->uc_mcontext->__ss;
	uint64_t pc = registers->__rip, sp = registers->__rsp, frame = registers->__rbp;
#endif
	int index;

	host_logf(HOST_LOG_ERROR, "signal %d at address %p: pc %016llx sp %016llx", signal_number, information->si_addr,
		(unsigned long long)pc, (unsigned long long)sp);
	if (GUEST(pc) >= host_image.base && GUEST(pc) < host_image.end)
	{
		host_logf(HOST_LOG_ERROR, "  in the guest image: llvm-symbolizer --obj=build/macos/halo_guest_<arch>.elf 0x%llx",
			(unsigned long long)GUEST(pc));
	}
	for (index = 0; index < 24 && host_low_owns(frame, 16) && !(frame & 7); index++)
	{
		const uint64_t *record = (const uint64_t *)frame;

		host_logf(HOST_LOG_ERROR, "  frame %2d: return %08llx", index, (unsigned long long)(uint32_t)record[1]);
		frame = HALO_GUEST_BIAS | (uint32_t)record[0];
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

static void fault_handler(int signal_number, siginfo_t *information, void *context)
{
	uint64_t page = watch_page(GUEST(information->si_addr));

	if (watch_active && page < WATCH_PAGE_COUNT)
	{
		if (page_protected[page] == 1)
		{
			mark_written(page);
			return;
		}
		if (page_protected[page] == 2)
			return;
	}
	report_crash(signal_number, information, context);
	chain(signal_number == SIGBUS ? &previous_bus : signal_number == SIGSEGV ? &previous_segv : &previous_ill,
		signal_number, information, context);
}

void host_install_signal_handlers(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = fault_handler;
	sigaction(SIGSEGV, &action, &previous_segv);
	sigaction(SIGBUS, &action, &previous_bus);
	sigaction(SIGILL, &action, &previous_ill);
}

void host_memory_watch_initialize(void)
{
	watch_active = 1;
}

void host_memory_watch_protect(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;

	if (!watch_active || !watch_pages(address, size, &first, &last))
		return;
	for (page = first; page <= last; page++)
	{
		if (page_protected[page] != 1)
		{
			page_protected[page] = 1;
			mprotect(HOST(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, PROT_READ);
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

	if (!watch_pages(address, size, &first, &last))
		return 0;
	for (page = first; page <= last; page++)
	{
		if (page_generation[page] > newest)
			newest = page_generation[page];
	}
	return newest;
}

void host_memory_watch_prepare_write(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;

	if (!watch_active || !watch_pages(address, size, &first, &last))
		return;
	for (page = first; page <= last; page++)
	{
		if (page_protected[page] == 1)
			mark_written(page);
	}
}

void host_memory_watch_forget(uint32_t address, uint32_t size)
{
	uint64_t first, last, page;

	if (!watch_pages(address, size, &first, &last))
		return;
	for (page = first; page <= last; page++)
	{
		page_protected[page] = 0;
		page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}

void host_memory_watch_begin_frame(void)
{
}
