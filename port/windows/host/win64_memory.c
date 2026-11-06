/*
WIN64_MEMORY.C

Guest address space for the Windows x64 host: what the Linux host's
host_memory.c does with mmap, on VirtualAlloc.

Everything the guest touches lies below 4 GB:

- the fixed ranges, reserved first thing in the process (win64_main.c,
  which starts the process again with them reserved by the first one if
  something else holds them): the Xbox window, the image's range, and
  Custom Edition's tag cache at 0x40440000, which the guest maps itself
  when Custom Edition maps may run (port/linux/src/xbox_memory.c);
- pools of address space below 4 GB for the guest's other mappings (malloc
  arenas, thread stacks), reserved as they fill up (64 KB granularity, as
  VirtualAlloc reserves; pages are committed and decommitted inside).

A mapping is fresh, zeroed memory as the guest's mmap promises: its pages
are decommitted and committed again. File mappings (which the guest does
not make today) are read into such memory.

The write tracking (port/linux/src/memory_watch.c's interface) is the
Linux host's: the renderer write-protects the pages behind the textures it
caches, and the vectored exception handler here records the first write to
each; any other fault is reported (as win32_crash.c does for the 32-bit
build, more briefly) and left to Windows. With debug.memory_watch = false
it compares page contents instead (host_watch_hash.c).
*/

#include "win64_host.h"
#include "host_watch_hash.h"
#include "halo_guest_syscall.h"

#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE 0x1000ULL
#define GRANULE 0x10000ULL
#define LOW_LIMIT 0x100000000ULL
#define LOW_START 0x01000000ULL
#define POOL_SIZE (64ULL * 1024 * 1024)
#define POOL_PAGES (POOL_SIZE / PAGE)
#define MAXIMUM_POOLS 48

/* port/linux/game/cache_file_formats.h */
#define CUSTOM_EDITION_BASE 0x40440000ULL
#define CUSTOM_EDITION_SIZE 0x01700000ULL

struct pool
{
	uint64_t base;
	uint32_t free_pages;
	uint8_t used[POOL_PAGES];
};

/* A pool is never freed, and its base never changes: pool_count is
published after its entry, so host_low_owns and the exception handler read
both without the lock. */
static struct pool *pools[MAXIMUM_POOLS];
static volatile LONG pool_count;
static SRWLOCK memory_lock = SRWLOCK_INIT;

struct fixed_range
{
	const char *name;
	uint64_t base, size;
	int reserved;
};

static struct fixed_range fixed[] =
{
	{"the Xbox memory window", HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE, 0},
	{"the guest image's range", HALO_GUEST_IMAGE_BASE, HALO_GUEST_IMAGE_RESERVE, 0},
	{"Custom Edition's tag cache", CUSTOM_EDITION_BASE, CUSTOM_EDITION_SIZE, 0},
};

#define FIXED_COUNT ((int)(sizeof(fixed) / sizeof(fixed[0])))
enum { FIXED_WINDOW, FIXED_IMAGE, FIXED_CUSTOM_EDITION };

static uint64_t image_base, image_end;

static uint64_t round_up(uint64_t value)
{
	return (value + PAGE - 1) & ~(PAGE - 1);
}

static int in_range(uint64_t address, uint64_t size, uint64_t base, uint64_t end)
{
	return address >= base && address < end && size <= end - address;
}

static DWORD windows_protection(int protection)
{
	switch (protection & (PROT_READ | PROT_WRITE | PROT_EXEC))
	{
	case PROT_NONE: return PAGE_NOACCESS;
	case PROT_READ: return PAGE_READONLY;
	case PROT_EXEC: return PAGE_EXECUTE;
	case PROT_READ | PROT_EXEC: return PAGE_EXECUTE_READ;
	case PROT_WRITE | PROT_EXEC:
	case PROT_READ | PROT_WRITE | PROT_EXEC: return PAGE_EXECUTE_READWRITE;
	default: return PAGE_READWRITE;
	}
}

/* ---------- committing pages inside a reservation */

/* fresh zeroed pages with this protection (none: reserved only) */
static int commit_fresh(uint64_t address, uint64_t length, int protection)
{
	VirtualFree((void *)(uintptr_t)address, length, MEM_DECOMMIT);
	if (!(protection & (PROT_READ | PROT_WRITE | PROT_EXEC)))
		return 0;
	return VirtualAlloc((void *)(uintptr_t)address, length, MEM_COMMIT, windows_protection(protection)) ? 0 : -1;
}

/* the protection of committed pages changed, and pages that were reserved
only committed (zeroed) with it */
static int protect_range(uint64_t address, uint64_t length, int protection)
{
	uint64_t end = address + length;

	while (address < end)
	{
		MEMORY_BASIC_INFORMATION information;
		uint64_t region_end, piece;

		if (!VirtualQuery((void *)(uintptr_t)address, &information, sizeof(information)))
			return -1;
		region_end = (uint64_t)(uintptr_t)information.BaseAddress + information.RegionSize;
		piece = (region_end < end ? region_end : end) - address;
		if (information.State == MEM_COMMIT)
		{
			DWORD old;

			if (!VirtualProtect((void *)(uintptr_t)address, piece, windows_protection(protection), &old))
				return -1;
		}
		else if (information.State == MEM_RESERVE)
		{
			if ((protection & (PROT_READ | PROT_WRITE | PROT_EXEC)) &&
				!VirtualAlloc((void *)(uintptr_t)address, piece, MEM_COMMIT, windows_protection(protection)))
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
		address += piece;
	}
	return 0;
}

/* ---------- the fixed ranges */

int host_memory_fixed_count(void)
{
	return FIXED_COUNT;
}

void host_memory_fixed_range(int index, uint64_t *base, uint64_t *size)
{
	*base = fixed[index].base;
	*size = fixed[index].size;
}

static int reserved_for_us(const struct fixed_range *range)
{
	MEMORY_BASIC_INFORMATION information;

	return VirtualQuery((void *)(uintptr_t)range->base, &information, sizeof(information)) &&
		information.State == MEM_RESERVE && (uint64_t)(uintptr_t)information.AllocationBase == range->base &&
		information.RegionSize >= range->size;
}

int host_memory_reserve_fixed(int adopt)
{
	int index, failed = 0;

	for (index = 0; index < FIXED_COUNT; index++)
	{
		struct fixed_range *range = &fixed[index];

		if (range->reserved)
			continue;
		if (VirtualAlloc((void *)(uintptr_t)range->base, range->size, MEM_RESERVE, PAGE_NOACCESS) ==
			(void *)(uintptr_t)range->base)
		{
			range->reserved = 1;
		}
		else if (adopt && reserved_for_us(range))
		{
			range->reserved = 1;
		}
		else
		{
			failed = 1;
		}
	}
	return failed ? -1 : 0;
}

/* lists what holds the low 4 GB, for a fixed range that cannot be had */
static void report_low_mappings(void)
{
	uint64_t address = 0;

	while (address < LOW_LIMIT)
	{
		MEMORY_BASIC_INFORMATION information;

		if (!VirtualQuery((void *)(uintptr_t)address, &information, sizeof(information)))
			break;
		if (information.State != MEM_FREE)
		{
			host_logf(HOST_LOG_INFO, "  %08llx-%08llx %s (allocation %08llx)", (unsigned long long)address,
				(unsigned long long)(address + information.RegionSize),
				information.State == MEM_COMMIT ? "committed" : "reserved",
				(unsigned long long)(uintptr_t)information.AllocationBase);
		}
		address += information.RegionSize;
	}
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	int index;

	if (base != HALO_GUEST_IMAGE_BASE || size > HALO_GUEST_IMAGE_RESERVE)
	{
		host_logf(HOST_LOG_ERROR, "the guest image (%08x, %u bytes) does not fit its range", base, size);
		return -1;
	}
	host_memory_reserve_fixed(1);
	for (index = 0; index < FIXED_COUNT; index++)
	{
		if (!fixed[index].reserved && index != FIXED_CUSTOM_EDITION)
		{
			host_logf(HOST_LOG_ERROR, "cannot reserve %s at %08llx; the low 4 GB:", fixed[index].name,
				(unsigned long long)fixed[index].base);
			report_low_mappings();
			return -1;
		}
	}
	image_base = base;
	image_end = base + round_up(size);
	return 0;
}

int host_memory_fixed_unavailable(void)
{
	return !fixed[FIXED_WINDOW].reserved || !fixed[FIXED_IMAGE].reserved;
}

/* host_loader.c's mapping of the image's range (sys/mman.h) */
void *host_win_mmap(void *address, size_t size, int protection, int flags, int fd, long long offset)
{
	uint64_t start = (uint64_t)(uintptr_t)address;

	(void)offset;
	if (fd != -1 || !(flags & MAP_FIXED) || !in_range(start, size, image_base, image_end))
		return MAP_FAILED;
	return commit_fresh(start, round_up(size), protection) == 0 ? address : MAP_FAILED;
}

int host_win_mprotect(void *address, size_t size, int protection)
{
	return protect_range((uint64_t)(uintptr_t)address, round_up(size), protection);
}

/* ---------- pools */

static struct pool *pool_new(void)
{
	uint64_t address = LOW_START;
	struct pool *pool;
	int count = pool_count;

	if (count == MAXIMUM_POOLS)
	{
		host_logf(HOST_LOG_ERROR, "guest memory: all %d pools are in use", MAXIMUM_POOLS);
		return NULL;
	}
	while (address + POOL_SIZE <= LOW_LIMIT)
	{
		MEMORY_BASIC_INFORMATION information;
		uint64_t start, end;

		if (!VirtualQuery((void *)(uintptr_t)address, &information, sizeof(information)))
			break;
		start = ((uint64_t)(uintptr_t)information.BaseAddress + GRANULE - 1) & ~(GRANULE - 1);
		end = (uint64_t)(uintptr_t)information.BaseAddress + information.RegionSize;
		if (information.State == MEM_FREE && end > start && end - start >= POOL_SIZE && start + POOL_SIZE <= LOW_LIMIT &&
			VirtualAlloc((void *)(uintptr_t)start, POOL_SIZE, MEM_RESERVE, PAGE_NOACCESS) == (void *)(uintptr_t)start)
		{
			pool = calloc(1, sizeof(*pool));
			if (!pool)
			{
				VirtualFree((void *)(uintptr_t)start, 0, MEM_RELEASE);
				return NULL;
			}
			pool->base = start;
			pool->free_pages = POOL_PAGES;
			pools[count] = pool;
			InterlockedExchange(&pool_count, count + 1);
			host_logf(HOST_LOG_INFO, "guest memory pool %d at %08llx", count, (unsigned long long)start);
			return pool;
		}
		address = end > address ? end : address + GRANULE;
	}
	host_logf(HOST_LOG_ERROR, "guest memory: no room below 4 GB for another pool");
	return NULL;
}

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
			pool->free_pages -= (uint32_t)pages;
			return (void *)(uintptr_t)(pool->base + first * PAGE);
		}
	}
	return NULL;
}

static struct pool *pool_of(uint64_t address, uint64_t size)
{
	int count = pool_count;
	int index;

	for (index = 0; index < count; index++)
	{
		if (in_range(address, size, pools[index]->base, pools[index]->base + POOL_SIZE))
			return pools[index];
	}
	return NULL;
}

/* with the memory lock held: the memory goes, the address space stays */
static void pool_release(struct pool *pool, uint64_t start, uint64_t length)
{
	uint64_t first = (start - pool->base) / PAGE, count = length / PAGE, page;

	VirtualFree((void *)(uintptr_t)start, length, MEM_DECOMMIT);
	for (page = first; page < first + count; page++)
	{
		if (pool->used[page])
		{
			pool->used[page] = 0;
			pool->free_pages++;
		}
	}
}

void *host_low_map(size_t size, int protection)
{
	uint64_t pages = round_up(size) / PAGE;
	void *address = NULL;
	int index;

	if (!pages || pages > POOL_PAGES)
		return NULL;
	AcquireSRWLockExclusive(&memory_lock);
	for (index = 0; index < pool_count && !address; index++)
		address = pool_take(pools[index], pages);
	if (!address)
	{
		struct pool *pool = pool_new();

		if (pool)
			address = pool_take(pool, pages);
	}
	ReleaseSRWLockExclusive(&memory_lock);
	if (!address)
		return NULL;
	if (commit_fresh((uint64_t)(uintptr_t)address, pages * PAGE, protection) != 0)
	{
		host_low_unmap(address, pages * PAGE);
		return NULL;
	}
	return address;
}

void host_low_unmap(void *address, size_t size)
{
	uint64_t start = (uint64_t)(uintptr_t)address & ~(PAGE - 1);
	uint64_t length = round_up((uint64_t)(uintptr_t)address + size) - start;
	struct pool *pool;

	AcquireSRWLockExclusive(&memory_lock);
	pool = pool_of(start, length);
	if (pool)
		pool_release(pool, start, length);
	ReleaseSRWLockExclusive(&memory_lock);
}

int host_low_protect(void *address, size_t size, int protection)
{
	return protect_range((uint64_t)(uintptr_t)address, round_up(size), protection);
}

int host_low_owns(uintptr_t address, size_t size)
{
	int index;

	for (index = 0; index < FIXED_COUNT; index++)
	{
		if (fixed[index].reserved && in_range(address, size, fixed[index].base, fixed[index].base + fixed[index].size))
			return 1;
	}
	return pool_of(address, size) != NULL;
}

void host_memory_report_low_mappings(FILE *file)
{
	(void)file;
	report_low_mappings();
}

void host_memory_reserve_early(void)
{
	host_memory_reserve_fixed(1);
}

/* ---------- the guest's memory system calls

As the Linux host's: the guest maps, unmaps, reprotects and advises only
memory that is its own (the pools' pages, the fixed ranges, and ranges it
reserved itself at addresses of its choosing); what each page is in use for
is kept so that a "no replace" mapping fails where something is mapped. */

/* a bit for each page below 4 GB outside the pools and the image that the
guest has mapped: in the window and Custom Edition's range, and its own
ranges */
static uint8_t guest_pages[LOW_LIMIT / PAGE / 8];

static int guest_page(uint64_t address)
{
	uint64_t page = address / PAGE;

	return (guest_pages[page >> 3] >> (page & 7)) & 1;
}

static void set_guest_pages(uint64_t address, uint64_t length, int value)
{
	uint64_t page;

	for (page = address / PAGE; page < (address + length) / PAGE; page++)
	{
		if (value)
			guest_pages[page >> 3] |= (uint8_t)(1 << (page & 7));
		else
			guest_pages[page >> 3] &= (uint8_t)~(1 << (page & 7));
	}
}

enum
{
	PAGE_HOST, /* the host's, or free address space */
	PAGE_POOL,
	PAGE_FIXED, /* the window, or Custom Edition's range */
	PAGE_IMAGE,
	PAGE_GUEST, /* a range the guest reserved itself */
};

static int page_kind(uint64_t address, struct pool **pool)
{
	int index;

	*pool = NULL;
	if (in_range(address, PAGE, image_base, image_end))
		return PAGE_IMAGE;
	for (index = 0; index < FIXED_COUNT; index++)
	{
		if (index != FIXED_IMAGE && fixed[index].reserved &&
			in_range(address, PAGE, fixed[index].base, fixed[index].base + fixed[index].size))
		{
			return PAGE_FIXED;
		}
	}
	*pool = pool_of(address, PAGE);
	if (*pool)
		return PAGE_POOL;
	return guest_page(address) ? PAGE_GUEST : PAGE_HOST;
}

static uint64_t run_end(uint64_t address, uint64_t end, int *kind, struct pool **pool)
{
	struct pool *next_pool;

	*kind = page_kind(address, pool);
	for (address += PAGE; address < end; address += PAGE)
	{
		if (page_kind(address, &next_pool) != *kind || next_pool != *pool)
			break;
	}
	return address;
}

static int run_in_use(uint64_t address, uint64_t end, int kind, const struct pool *pool)
{
	for (; address < end; address += PAGE)
	{
		if (kind == PAGE_IMAGE || kind == PAGE_GUEST)
			return 1;
		if (kind == PAGE_POOL && pool->used[(address - pool->base) / PAGE])
			return 1;
		if (kind == PAGE_FIXED && guest_page(address))
			return 1;
	}
	return 0;
}

static int guest_owns(uint64_t address, uint64_t end, int image)
{
	while (address < end)
	{
		struct pool *pool;
		int kind;

		address = run_end(address, end, &kind, &pool);
		if (kind == PAGE_HOST || (kind == PAGE_IMAGE && !image))
			return 0;
	}
	return 1;
}

static void forget_window_range(uint64_t address, uint64_t length)
{
	uint64_t start = address, end = address + length;
	uint64_t window_base = HALO_GUEST_WINDOW_BASE, window_end = window_base + HALO_GUEST_WINDOW_SIZE;

	if (start < window_base)
		start = window_base;
	if (end > window_end)
		end = window_end;
	if (start < end)
		host_memory_watch_forget((uint32_t)start, (uint32_t)(end - start));
}

/* a file's bytes into a fresh mapping (MAP_PRIVATE's copy) */
static long read_file_into(uint64_t address, uint64_t length, int protection, int fd, int64_t offset)
{
	intptr_t handle = _get_osfhandle(fd);
	uint64_t done = 0;

	if (handle == -1)
		return -GUEST_EBADF;
	if (protect_range(address, length, PROT_READ | PROT_WRITE) != 0)
		return -GUEST_ENOMEM;
	while (done < length)
	{
		OVERLAPPED overlapped;
		DWORD chunk = length - done > 0x10000000 ? 0x10000000 : (DWORD)(length - done), got = 0;
		uint64_t position = (uint64_t)offset + done;

		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.Offset = (DWORD)position;
		overlapped.OffsetHigh = (DWORD)(position >> 32);
		if (!ReadFile((HANDLE)handle, (void *)(uintptr_t)(address + done), chunk, &got, &overlapped) || !got)
			break;
		done += got;
	}
	return protect_range(address, length, protection) == 0 ? 0 : -GUEST_ENOMEM;
}

static long guest_mmap_fixed(uint64_t address, uint64_t length, int protection, int flags, int fd, int64_t offset)
{
	uint64_t end = address + length, at;
	int replace = !(flags & MAP_FIXED_NOREPLACE);
	int host = 0, owned = 0, in_use = 0;
	long status;

	if (address & (PAGE - 1))
		return -GUEST_EINVAL;
	if (end > LOW_LIMIT || end < address)
		return -GUEST_ENOMEM;
	AcquireSRWLockExclusive(&memory_lock);
	for (at = address; at < end;)
	{
		struct pool *pool;
		int kind;
		uint64_t next = run_end(at, end, &kind, &pool);

		if (kind == PAGE_HOST)
			host = 1;
		else
		{
			owned = 1;
			in_use |= run_in_use(at, next, kind, pool);
		}
		at = next;
	}
	if (!owned)
	{
		/* free address space: a range of the guest's own, which Windows
		reserves at 64 KB boundaries */
		void *result;

		if (replace)
		{
			status = -GUEST_EINVAL;
			goto done;
		}
		if (address & (GRANULE - 1))
		{
			status = -GUEST_EEXIST;
			goto done;
		}
		result = VirtualAlloc((void *)(uintptr_t)address, length,
			MEM_RESERVE | ((protection & (PROT_READ | PROT_WRITE | PROT_EXEC)) ? MEM_COMMIT : 0),
			windows_protection(protection));
		if (result != (void *)(uintptr_t)address)
		{
			status = -GUEST_EEXIST;
			goto done;
		}
		set_guest_pages(address, length, 1);
		status = (long)address;
		if (fd != -1)
		{
			long read = read_file_into(address, length, protection, fd, offset);

			if (read)
				status = read;
		}
		goto done;
	}
	if (host)
	{
		status = replace ? -GUEST_EINVAL : -GUEST_EEXIST;
		goto done;
	}
	if (!replace && in_use)
	{
		status = -GUEST_EEXIST;
		goto done;
	}
	if (commit_fresh(address, length, protection) != 0)
	{
		status = -GUEST_ENOMEM;
		goto done;
	}
	for (at = address; at < end;)
	{
		struct pool *pool;
		int kind;
		uint64_t next = run_end(at, end, &kind, &pool);
		uint64_t page;

		if (kind == PAGE_POOL)
		{
			for (page = (at - pool->base) / PAGE; page < (next - pool->base) / PAGE; page++)
			{
				if (!pool->used[page])
				{
					pool->used[page] = 1;
					pool->free_pages--;
				}
			}
		}
		else if (kind == PAGE_FIXED)
			set_guest_pages(at, next - at, 1);
		at = next;
	}
	forget_window_range(address, length);
	status = (long)address;
	if (fd != -1)
	{
		long read = read_file_into(address, length, protection, fd, offset);

		if (read)
			status = read;
	}
done:
	ReleaseSRWLockExclusive(&memory_lock);
	return status;
}

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	uint64_t length = round_up(size);
	void *result;

	if (!length || length < size)
		return -GUEST_EINVAL;
	if (!(flags & MAP_ANONYMOUS))
	{
		if (flags & MAP_SHARED)
			return -GUEST_EOPNOTSUPP;
	}
	else
	{
		fd = -1;
	}
	if (flags & (MAP_FIXED | MAP_FIXED_NOREPLACE))
		return guest_mmap_fixed(address, length, protection, flags, fd, offset);
	result = host_low_map(length, protection);
	if (!result)
		return -GUEST_ENOMEM;
	if (fd != -1)
	{
		long read = read_file_into((uint64_t)(uintptr_t)result, length, protection, fd, offset);

		if (read)
		{
			host_low_unmap(result, length);
			return read;
		}
	}
	return (long)(uintptr_t)result;
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	uint64_t length = round_up(size), end = address + length, at;
	long status = 0;

	if ((address & (PAGE - 1)) || !length || length < size)
		return -GUEST_EINVAL;
	if (end > LOW_LIMIT || end < address)
		return -GUEST_EINVAL;
	AcquireSRWLockExclusive(&memory_lock);
	if (!guest_owns(address, end, 0))
	{
		status = -GUEST_EINVAL;
		goto done;
	}
	for (at = address; at < end;)
	{
		struct pool *pool;
		int kind;
		uint64_t next = run_end(at, end, &kind, &pool);

		if (kind == PAGE_POOL)
			pool_release(pool, at, next - at);
		else if (kind == PAGE_FIXED)
			VirtualFree((void *)(uintptr_t)at, next - at, MEM_DECOMMIT);
		else if (kind == PAGE_GUEST)
			/* (the address space stays reserved: Windows cannot release
			part of a reservation) */
			VirtualFree((void *)(uintptr_t)at, next - at, MEM_DECOMMIT);
		if (kind == PAGE_FIXED || kind == PAGE_GUEST)
			set_guest_pages(at, next - at, 0);
		at = next;
	}
	forget_window_range(address, length);
done:
	ReleaseSRWLockExclusive(&memory_lock);
	return status;
}

static long guest_range(uint64_t address, uint64_t size, uint64_t *length)
{
	int owned;

	if (address & (PAGE - 1))
		return -GUEST_EINVAL;
	*length = round_up(size);
	if (*length < size)
		return -GUEST_ENOMEM;
	if (!*length)
		return 0;
	if (address + *length > LOW_LIMIT || address + *length < address)
		return -GUEST_ENOMEM;
	AcquireSRWLockShared(&memory_lock);
	owned = guest_owns(address, address + *length, 1);
	ReleaseSRWLockShared(&memory_lock);
	return owned ? 1 : -GUEST_ENOMEM;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	uint64_t length;
	long check = guest_range(address, size, &length);

	if (check <= 0)
		return check;
	if (protect_range(address, length, protection) != 0)
		return -GUEST_ENOMEM;
	forget_window_range(address, length);
	return 0;
}

/* MADV_DONTNEED: the pages read as zeroes again; others are only advice */
long host_guest_madvise(uint64_t address, uint64_t size, int advice)
{
	uint64_t length, at, end;
	long check = guest_range(address, size, &length);

	if (check <= 0)
		return check;
	if (advice != GUEST_MADV_DONTNEED)
		return 0;
	end = address + length;
	for (at = address; at < end;)
	{
		MEMORY_BASIC_INFORMATION information;
		uint64_t region_end, piece;

		if (!VirtualQuery((void *)(uintptr_t)at, &information, sizeof(information)))
			return -GUEST_ENOMEM;
		region_end = (uint64_t)(uintptr_t)information.BaseAddress + information.RegionSize;
		piece = (region_end < end ? region_end : end) - at;
		if (information.State == MEM_COMMIT)
		{
			DWORD protection = information.Protect;

			VirtualFree((void *)(uintptr_t)at, piece, MEM_DECOMMIT);
			VirtualAlloc((void *)(uintptr_t)at, piece, MEM_COMMIT, protection);
		}
		at += piece;
	}
	forget_window_range(address, length);
	return 0;
}

/* ---------- write tracking (port/linux/src/memory_watch.c) */

#define WATCH_PAGE_COUNT (HALO_GUEST_WINDOW_SIZE / PAGE)

/* (each page: 0 not watched, 1 watched and read-only, 2 made writable by a
watched write) */
static uint8_t page_protected[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static int watch_active;
static int watch_hashing;
static uint8_t page_hash_watched[WATCH_PAGE_COUNT];
static uint64_t page_hash[WATCH_PAGE_COUNT];
static uint32_t page_hashed_frame[WATCH_PAGE_COUNT];
static struct watch_hash page_hashes =
{
	.base = (const uint8_t *)(uintptr_t)HALO_GUEST_WINDOW_BASE,
	.page_count = WATCH_PAGE_COUNT,
	.watched = page_hash_watched,
	.hash = page_hash,
	.hashed_frame = page_hashed_frame,
	.generation = page_generation,
	.current_generation = &current_generation,
	.frame = 1,
};

static int in_window(uint64_t address)
{
	return address >= HALO_GUEST_WINDOW_BASE && address - HALO_GUEST_WINDOW_BASE < HALO_GUEST_WINDOW_SIZE;
}

static uint64_t watch_page(uint64_t address)
{
	return (address - HALO_GUEST_WINDOW_BASE) / PAGE;
}

static void set_page_protection(uint64_t page, DWORD protection)
{
	DWORD old;

	VirtualProtect((void *)(uintptr_t)(HALO_GUEST_WINDOW_BASE + page * PAGE), PAGE, protection, &old);
}

static void mark_written(uint64_t page)
{
	page_generation[page] = (uint32_t)InterlockedIncrement((volatile LONG *)&current_generation);
	page_protected[page] = 2;
	set_page_protection(page, PAGE_READWRITE);
}

/* ---------- the exception handler */

static const char *exception_name(DWORD code)
{
	switch (code)
	{
	case EXCEPTION_ACCESS_VIOLATION: return "access violation";
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
	case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
	case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer division by zero";
	case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
	case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
	case EXCEPTION_DATATYPE_MISALIGNMENT: return "misalignment";
	}
	return NULL;
}

/* follows the frame records from rbp (the guest and the host keep the
caller's frame pointer, then the return address) where they can be read */
static void report_frames(uint64_t frame)
{
	int index;

	for (index = 0; index < 24 && frame; index++)
	{
		uint64_t record[2];

		if (frame & 7 || IsBadReadPtr((const void *)(uintptr_t)frame, sizeof(record)))
			break;
		memcpy(record, (const void *)(uintptr_t)frame, sizeof(record));
		host_logf(HOST_LOG_ERROR, "  frame %2d: return %016llx", index, (unsigned long long)record[1]);
		if (record[0] <= frame)
			break;
		frame = record[0];
	}
}

static volatile LONG reporting;

static void report_crash(EXCEPTION_POINTERS *exception)
{
	const EXCEPTION_RECORD *record = exception->ExceptionRecord;
	const CONTEXT *context = exception->ContextRecord;
	const char *name = exception_name(record->ExceptionCode);
	uint64_t pc = context->Rip;

	if (InterlockedExchange(&reporting, 1))
		return;
	if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
		host_logf(HOST_LOG_ERROR, "%s (%s %016llx) at %016llx", name,
			record->ExceptionInformation[0] == 1 ? "writing" : record->ExceptionInformation[0] == 8 ? "executing" :
			"reading", (unsigned long long)record->ExceptionInformation[1], (unsigned long long)pc);
	else
		host_logf(HOST_LOG_ERROR, "%s (exception %08lx) at %016llx", name ? name : "exception",
			(unsigned long)record->ExceptionCode, (unsigned long long)pc);
	if (pc >= host_image.base && pc < host_image.end)
		host_logf(HOST_LOG_ERROR, "  in the guest image: addr2line -e halo_guest.elf 0x%llx", (unsigned long long)pc);
	else
	{
		HMODULE module = NULL;
		char path[MAX_PATH] = "";

		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(const char *)(uintptr_t)pc, &module) && GetModuleFileNameA(module, path, sizeof(path)))
		{
			host_logf(HOST_LOG_ERROR, "  in %s at +%llx", path, (unsigned long long)(pc - (uint64_t)(uintptr_t)module));
		}
	}
	host_logf(HOST_LOG_ERROR, "  rax %016llx rbx %016llx rcx %016llx rdx %016llx", (unsigned long long)context->Rax,
		(unsigned long long)context->Rbx, (unsigned long long)context->Rcx, (unsigned long long)context->Rdx);
	host_logf(HOST_LOG_ERROR, "  rsi %016llx rdi %016llx rbp %016llx rsp %016llx", (unsigned long long)context->Rsi,
		(unsigned long long)context->Rdi, (unsigned long long)context->Rbp, (unsigned long long)context->Rsp);
	host_logf(HOST_LOG_ERROR, "  r8  %016llx r9  %016llx r10 %016llx r11 %016llx", (unsigned long long)context->R8,
		(unsigned long long)context->R9, (unsigned long long)context->R10, (unsigned long long)context->R11);
	host_logf(HOST_LOG_ERROR, "  r12 %016llx r13 %016llx r14 %016llx r15 %016llx", (unsigned long long)context->R12,
		(unsigned long long)context->R13, (unsigned long long)context->R14, (unsigned long long)context->R15);
	report_frames(context->Rbp);
}

static LONG CALLBACK exception_handler(EXCEPTION_POINTERS *exception)
{
	const EXCEPTION_RECORD *record = exception->ExceptionRecord;
	DWORD code = record->ExceptionCode;

	if (code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2 && record->ExceptionInformation[0] == 1)
	{
		uint64_t address = record->ExceptionInformation[1];

		if (watch_active && in_window(address))
		{
			uint64_t page = watch_page(address);

			if (page_protected[page] == 1)
			{
				mark_written(page);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			if (page_protected[page] == 2)
				return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	/* (the faults worth a report: a C++ exception or a debugger's breakpoint
	in a driver is someone else's to handle) */
	if (exception_name(code))
		report_crash(exception);
	return EXCEPTION_CONTINUE_SEARCH;
}

void host_install_exception_handler(void)
{
	AddVectoredExceptionHandler(1, exception_handler);
}

void host_install_signal_handlers(void)
{
	host_install_exception_handler();
}

void host_memory_watch_initialize(void)
{
	watch_active = 1;
}

void host_memory_watch_use_hashes(void)
{
	watch_hashing = 1;
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
	if (watch_hashing)
	{
		watch_hash_protect(&page_hashes, (uint32_t)first, (uint32_t)last);
		return;
	}
	for (page = first; page <= last; page++)
	{
		if (page_protected[page] != 1)
		{
			page_protected[page] = 1;
			set_page_protection(page, PAGE_READONLY);
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
	if (watch_hashing)
		return watch_hash_generation(&page_hashes, (uint32_t)first, (uint32_t)last);
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

	if (!watch_active || watch_hashing || !size)
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
		if (page_protected[page] == 1)
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
	if (watch_hashing)
	{
		watch_hash_forget(&page_hashes, (uint32_t)first, (uint32_t)last);
		return;
	}
	for (page = first; page <= last; page++)
	{
		page_protected[page] = 0;
		page_generation[page] = (uint32_t)InterlockedIncrement((volatile LONG *)&current_generation);
	}
}

void host_memory_watch_begin_frame(void)
{
	if (watch_hashing)
		watch_hash_begin_frame(&page_hashes);
}
