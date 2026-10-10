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
passed on to the previous handler. Under ARM translation (the x86 emulator)
page faults cannot be caught, and the tracking compares page contents
instead (host_watch_hash.c).
*/

#include "host.h"
#include "host_watch_hash.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/system_properties.h>
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
/proc/self/maps; 0 if none */
static uint64_t find_gap(uint64_t size, uint64_t minimum)
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
	previous_end = round_up(previous_end);
	if (previous_end + size <= LOW_LIMIT)
		result = previous_end;
	return result;
}

/* ART reserves its spaces at addresses chosen at zygote start, and on some
devices one of them covers the fixed Xbox memory window. ART's heap
(dalvik.vm.heapsize) decides how much of the low 4 GB its spaces take: on
handhelds with a large heap (the Retroid Pocket Flip2, the AYN Thor) the
free-list large object space usually lands right above the boot image, over
0x80000000.

That space commits pages lazily from its bottom, and the game's process
(HaloActivity runs in a process of its own, ":game", and reserves the
window from JNI_OnLoad before its Java side has done anything:
host_memory_reserve_early) hardly allocates large objects, so the slice
over the window is normally idle address space: unmap exactly the
intersection (never more) and let the caller retry. Only that space, and
only a slice with no page in use (present or swapped out): ART's other
spaces (its heap, bitmaps and card tables) are live, and unmapping them
would corrupt it where failing to start is at least clear. */
#define ART_LARGE_OBJECT_SPACE "[anon:dalvik-free list large object space]"

/* what range_usage found */
enum
{
	RANGE_UNUSED,
	RANGE_IN_USE,
	RANGE_UNKNOWN
};

/* why the fixed ranges could not be had: each reason also goes to the
log, and the report (host_memory_report_low_mappings) starts with them, so
memory_map.txt alone tells which case it was */
static char findings[2048];

static void finding(int priority, const char *format, ...)
{
	char line[512];
	size_t used = strlen(findings);
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	host_logf(priority, "%s", line);
	if (used + strlen(line) + 2 < sizeof(findings))
		snprintf(findings + used, sizeof(findings) - used, "%s\n", line);
}

struct range_usage
{
	uint64_t pages_in_use;
	uint64_t lowest_in_use, highest_in_use;
	const char *method;
};

static void note_in_use(struct range_usage *usage, uint64_t address)
{
	if (!usage->pages_in_use++)
		usage->lowest_in_use = address;
	usage->highest_in_use = address;
}

/* debug.halo.art_overlap's -nopagemap (simulate_art_overlap) */
static int simulated_no_pagemap;

/* from /proc/self/pagemap: bit 63 present, bit 62 swapped out */
static int range_usage_pagemap(uint64_t from, uint64_t to, struct range_usage *usage)
{
	int descriptor = simulated_no_pagemap ? -1 : open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
	uint64_t entries[512];
	uint64_t page = from / PAGE;

	if (simulated_no_pagemap)
		errno = EACCES;
	if (descriptor < 0)
	{
		finding(HOST_LOG_WARN, "cannot read /proc/self/pagemap (%s)", strerror(errno));
		return RANGE_UNKNOWN;
	}
	usage->method = "pagemap";
	while (page < to / PAGE)
	{
		size_t wanted = (size_t)(to / PAGE - page);
		ssize_t bytes;
		size_t count, index;

		if (wanted > sizeof(entries) / sizeof(entries[0]))
			wanted = sizeof(entries) / sizeof(entries[0]);
		bytes = pread(descriptor, entries, wanted * sizeof(entries[0]), (off_t)(page * sizeof(entries[0])));
		if (bytes <= 0 || bytes % sizeof(entries[0]))
		{
			finding(HOST_LOG_WARN, "cannot read /proc/self/pagemap at %08llx (%s)",
				(unsigned long long)(page * PAGE), bytes < 0 ? strerror(errno) : "short read");
			close(descriptor);
			return RANGE_UNKNOWN;
		}
		count = (size_t)bytes / sizeof(entries[0]);
		for (index = 0; index < count; index++)
		{
			if (entries[index] & (3ULL << 62))
				note_in_use(usage, (page + index) * PAGE);
		}
		page += count;
	}
	close(descriptor);
	return usage->pages_in_use ? RANGE_IN_USE : RANGE_UNUSED;
}

/* the Swap: line of the /proc/self/smaps entry starting at start, in kB; -1
if it cannot be read (or no entry starts there); with end, the entry must
also end there. dont_dump, if not NULL, tells whether its VmFlags have dd
(MADV_DONTDUMP). */
static long mapping_swap_kb_ending(uint64_t start, uint64_t end, int *dont_dump)
{
	FILE *smaps = fopen("/proc/self/smaps", "r");
	char line[512];
	int inside = 0;
	long result = -1;

	if (dont_dump)
		*dont_dump = 0;
	if (!smaps)
		return -1;
	while (fgets(line, sizeof(line), smaps))
	{
		unsigned long long lo, hi;
		long kb;

		if (sscanf(line, "%llx-%llx ", &lo, &hi) == 2 && strchr(line, '-') < strchr(line, ' '))
		{
			if (inside)
				break;
			inside = lo == start && (!end || hi == end);
			continue;
		}
		if (!inside)
			continue;
		if (sscanf(line, "Swap: %ld kB", &kb) == 1)
			result = kb;
		else if (dont_dump && !strncmp(line, "VmFlags:", 8))
			*dont_dump = strstr(line, " dd") != NULL;
	}
	fclose(smaps);
	return result;
}

static long mapping_swap_kb(uint64_t start)
{
	return mapping_swap_kb_ending(start, 0, NULL);
}

/* whether any of from..to (in the mapping starting at mapping_start) is
swapped out, when that mapping has swap somewhere: smaps counts swap per
mapping, so the range is made a mapping of its own for a moment
(MADV_DONTDUMP splits it off and changes nothing about its pages: neither
ART nor the game ever dumps core), read, and merged back (MADV_DODUMP). Not
for a mapping already marked so (the restore would then change it). -1 if
that cannot be done */
static long range_swap_kb(uint64_t mapping_start, uint64_t from, uint64_t to)
{
	int dont_dump;
	long kb;

	if (mapping_swap_kb_ending(mapping_start, 0, &dont_dump) < 0)
	{
		finding(HOST_LOG_WARN, "cannot read the smaps entry of ART's large object space at %08llx",
			(unsigned long long)mapping_start);
		return -1;
	}
	if (dont_dump)
	{
		finding(HOST_LOG_WARN, "ART's large object space at %08llx is marked not to be dumped; "
			"the swap of %08llx-%08llx cannot be read on its own", (unsigned long long)mapping_start,
			(unsigned long long)from, (unsigned long long)to);
		return -1;
	}
	if (madvise((void *)from, (size_t)(to - from), MADV_DONTDUMP) != 0)
	{
		finding(HOST_LOG_WARN, "cannot split %08llx-%08llx off ART's large object space (%s)",
			(unsigned long long)from, (unsigned long long)to, strerror(errno));
		return -1;
	}
	kb = mapping_swap_kb_ending(from, to, NULL);
	madvise((void *)from, (size_t)(to - from), MADV_DODUMP);
	if (kb < 0)
		finding(HOST_LOG_WARN, "no smaps entry for %08llx-%08llx once split off", (unsigned long long)from,
			(unsigned long long)to);
	return kb;
}

/* without pagemap (some kernels or policies refuse it): mincore() tells
which pages are resident, and smaps whether any of the range is swapped
out, which mincore cannot see */
static int range_usage_mincore(uint64_t mapping_start, uint64_t from, uint64_t to, struct range_usage *usage)
{
	unsigned char residency[4096];
	uint64_t address = from;
	long swapped = mapping_swap_kb(mapping_start);

	usage->method = "mincore";
	if (swapped < 0)
	{
		finding(HOST_LOG_WARN, "cannot read the smaps entry of ART's large object space at %08llx",
			(unsigned long long)mapping_start);
		return RANGE_UNKNOWN;
	}
	if (swapped > 0)
	{
		/* (swap somewhere in the space: only the range's own matters) */
		long range_swapped = range_swap_kb(mapping_start, from, to);

		if (range_swapped < 0)
			return RANGE_UNKNOWN;
		if (range_swapped > 0)
		{
			/* (objects there, swapped out: which pages smaps does not say) */
			usage->method = "the range's swap in smaps";
			usage->pages_in_use = (uint64_t)range_swapped * 1024 / PAGE;
			usage->lowest_in_use = from;
			usage->highest_in_use = to - PAGE;
			return RANGE_IN_USE;
		}
		finding(HOST_LOG_INFO, "ART's large object space at %08llx: %ld kB of it swapped out, none of %08llx-%08llx",
			(unsigned long long)mapping_start, swapped, (unsigned long long)from, (unsigned long long)to);
	}
	while (address < to)
	{
		uint64_t length = to - address;
		size_t index;

		if (length > sizeof(residency) * PAGE)
			length = sizeof(residency) * PAGE;
		if (mincore((void *)address, length, residency) != 0)
		{
			finding(HOST_LOG_WARN, "mincore at %08llx failed (%s)", (unsigned long long)address, strerror(errno));
			return RANGE_UNKNOWN;
		}
		for (index = 0; index < length / PAGE; index++)
		{
			if (residency[index] & 1)
				note_in_use(usage, address + index * PAGE);
		}
		address += length;
	}
	return usage->pages_in_use ? RANGE_IN_USE : RANGE_UNUSED;
}

/* whether any page from..to (page aligned) of the mapping starting at
mapping_start holds data */
static int range_usage(uint64_t mapping_start, uint64_t from, uint64_t to, struct range_usage *usage)
{
	int result;

	memset(usage, 0, sizeof(*usage));
	result = range_usage_pagemap(from, to, usage);
	if (result == RANGE_UNKNOWN)
	{
		memset(usage, 0, sizeof(*usage));
		result = range_usage_mincore(mapping_start, from, to, usage);
	}
	return result;
}

static int reclaim_art_overlap(uint64_t address, uint64_t size)
{
	FILE *maps = fopen("/proc/self/maps", "r");
	char line[512];
	int reclaimed = 0;

	if (!maps)
		return 0;
	while (fgets(line, sizeof(line), maps))
	{
		unsigned long long lo, hi;
		uint64_t from, to;
		struct range_usage usage;
		char *name;
		int name_offset = 0;

		if (sscanf(line, "%llx-%llx %*s %*s %*s %*s %n", &lo, &hi, &name_offset) < 2 || !name_offset)
			continue;
		if (hi <= address || lo >= address + size)
			continue;
		name = line + name_offset;
		name[strcspn(name, "\n")] = '\0';
		from = lo > address ? lo : address;
		to = hi < address + size ? hi : address + size;
		if (strcmp(name, ART_LARGE_OBJECT_SPACE))
		{
			finding(HOST_LOG_ERROR, "a fixed guest range is overlapped by %08llx-%08llx (%s), which is not ART's large object space; left alone",
				lo, hi, name[0] ? name : "unnamed");
			continue;
		}
		switch (range_usage(lo, from, to, &usage))
		{
		case RANGE_IN_USE:
			finding(HOST_LOG_ERROR,
				"ART's large object space %08llx-%08llx holds objects over %08llx-%08llx "
				"(%llu pages in use, %08llx-%08llx, by %s); left alone",
				lo, hi, (unsigned long long)from, (unsigned long long)to,
				(unsigned long long)usage.pages_in_use, (unsigned long long)usage.lowest_in_use,
				(unsigned long long)usage.highest_in_use + PAGE, usage.method);
			continue;
		case RANGE_UNKNOWN:
			finding(HOST_LOG_ERROR,
				"ART's large object space %08llx-%08llx covers %08llx-%08llx, and whether that part is in use "
				"cannot be told; left alone", lo, hi, (unsigned long long)from, (unsigned long long)to);
			continue;
		}
		if (munmap((void *)from, to - from) == 0)
		{
			host_logf(HOST_LOG_INFO,
				"reclaimed idle ART range %08llx-%08llx of %08llx-%08llx (%s, checked by %s)",
				(unsigned long long)from, (unsigned long long)to, lo, hi, name, usage.method);
			reclaimed = 1;
		}
	}
	fclose(maps);
	return reclaimed;
}

/* reclaim_art: the fixed ranges the guest was built for may take ART's
idle large object space (reclaim_art_overlap); the pools, placed in free
gaps, never do: a mapping in the way there is one ART just made, maybe live */
static int reserve(uint64_t address, uint64_t size, int reclaim_art)
{
	void *result = mmap((void *)address, size, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);

	if (result == (void *)address)
		return 0;
	if (result != MAP_FAILED)
	{
		/* (a kernel older than 4.17 takes MAP_FIXED_NOREPLACE as a hint,
		and maps elsewhere when something is in the way) */
		munmap(result, size);
		errno = EEXIST;
	}
	if (reclaim_art && errno == EEXIST)
	{
		int error = errno;

		/* (the caller reports the first failure if nothing was reclaimed) */
		if (!reclaim_art_overlap(address, size))
		{
			errno = error;
			return -1;
		}
		result = mmap((void *)address, size, PROT_NONE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE,
			-1, 0);
		if (result == (void *)address)
			return 0;
		if (result != MAP_FAILED)
		{
			munmap(result, size);
			errno = EEXIST;
		}
	}
	return -1;
}

/* the mappings below 4 GB, for a report of why the fixed ranges could not
be had (logcat, and memory_map.txt in the data folder: host_main.c) */
void host_memory_report_low_mappings(FILE *file)
{
	FILE *maps = fopen("/proc/self/maps", "r");
	char line[512];
	char model[PROP_VALUE_MAX] = "", heap[PROP_VALUE_MAX] = "", release[PROP_VALUE_MAX] = "";

	__system_property_get("ro.product.model", model);
	__system_property_get("dalvik.vm.heapsize", heap);
	__system_property_get("ro.build.version.release", release);
	if (file)
	{
		fprintf(file, "device: %s, Android %s, Java heap %s\n", model[0] ? model : "unknown",
			release[0] ? release : "unknown", heap[0] ? heap : "unknown");
		fprintf(file, "why:\n%s\nmappings below 4 GB:\n", findings[0] ? findings : "(no reason recorded)\n");
	}
	if (!maps)
		return;
	while (fgets(line, sizeof(line), maps))
	{
		unsigned long long lo, hi;

		if (sscanf(line, "%llx-%llx", &lo, &hi) != 2 || lo >= LOW_LIMIT)
			continue;
		line[strcspn(line, "\n")] = '\0';
		host_logf(HOST_LOG_INFO, "low mapping: %s", line);
		if (file)
			fprintf(file, "%s\n", line);
	}
	fclose(maps);
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
		uint64_t address = find_gap(POOL_SIZE, minimum);

		if (!address && minimum != LOW_START)
		{
			minimum = LOW_START;
			address = find_gap(POOL_SIZE, minimum);
		}
		if (!address)
			return NULL;
		if (reserve(address, POOL_SIZE, 0) == 0)
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

/* The fixed ranges: the Xbox window and, right above it, room for the guest
image (HALO_GUEST_IMAGE_RESERVE), reserved as early as the process allows
(host_memory_reserve_early, from JNI_OnLoad) or else when the image loads. */
static int fixed_reserved;
static int fixed_error;

/* when: the attempt, as the report names it (the reasons below it are
those of that attempt) */
static int reserve_fixed(const char *when)
{
	size_t used = strlen(findings);

	if (fixed_reserved)
		return 0;
	snprintf(findings + used, sizeof(findings) - used, "%s:\n", when);
	if (reserve(HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE, 1) != 0)
	{
		fixed_error = errno;
		finding(HOST_LOG_ERROR, "cannot reserve the Xbox memory window at %08llx (%s)",
			(unsigned long long)HALO_GUEST_WINDOW_BASE, strerror(errno));
		return -1;
	}
	if (reserve(HALO_GUEST_IMAGE_BASE, HALO_GUEST_IMAGE_RESERVE, 1) != 0)
	{
		fixed_error = errno;
		finding(HOST_LOG_ERROR, "cannot reserve the guest image range at %08llx (%s)",
			(unsigned long long)HALO_GUEST_IMAGE_BASE, strerror(errno));
		munmap((void *)(uintptr_t)HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE);
		return -1;
	}
	fixed_reserved = 1;
	return 0;
}

int host_memory_fixed_unavailable(void)
{
	return !fixed_reserved && fixed_error != 0;
}

/* For testing the reclaim on any device: with the system property
debug.halo.art_overlap set (adb shell setprop debug.halo.art_overlap idle,
or busy), a mapping named as ART's large object space is put over the fixed
ranges first, idle, or with a page in use inside the window, the way ART's
is on the devices that need the reclaim. Players never set it.

Either may be followed by -nopagemap, for devices that refuse
/proc/self/pagemap (the check then falls back to mincore and smaps), and
by -swapped, which pushes the stand-in's objects (its bottom pages, and the
window's page if busy) out to swap, as they can be on those devices. */
#ifndef MADV_PAGEOUT
#define MADV_PAGEOUT 21
#endif
#define PR_SET_VMA_ 0x53564d41
#define PR_SET_VMA_ANON_NAME_ 0
#define SIMULATED_SPACE_BASE 0x7c000000ULL
#define SIMULATED_SPACE_SIZE 0x14000000ULL

static void simulate_art_overlap(void)
{
	char value[PROP_VALUE_MAX] = "";
	char *window_page = (char *)(uintptr_t)HALO_GUEST_WINDOW_BASE + 0x100000;
	int busy;
	void *space;

	if (__system_property_get("debug.halo.art_overlap", value) <= 0 || (strncmp(value, "idle", 4) && strncmp(value, "busy", 4)))
		return;
	busy = !strncmp(value, "busy", 4);
	space = mmap((void *)SIMULATED_SPACE_BASE, SIMULATED_SPACE_SIZE, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
	if (space != (void *)SIMULATED_SPACE_BASE)
	{
		host_logf(HOST_LOG_WARN, "debug.halo.art_overlap: %08llx-%08llx is not free",
			SIMULATED_SPACE_BASE, SIMULATED_SPACE_BASE + SIMULATED_SPACE_SIZE);
		if (space != MAP_FAILED)
			munmap(space, SIMULATED_SPACE_SIZE);
		return;
	}
	prctl(PR_SET_VMA_, PR_SET_VMA_ANON_NAME_, space, SIMULATED_SPACE_SIZE, "dalvik-free list large object space");
	/* objects at its bottom, as ART allocates them */
	memset(space, 0x5a, 4 * PAGE);
	if (busy)
		memset(window_page, 0x5a, PAGE);
	if (strstr(value, "-swapped"))
	{
		if (madvise(space, 4 * PAGE, MADV_PAGEOUT) != 0 || (busy && madvise(window_page, PAGE, MADV_PAGEOUT) != 0))
			host_logf(HOST_LOG_WARN, "debug.halo.art_overlap: cannot page out the stand-in (%s)", strerror(errno));
	}
	simulated_no_pagemap = strstr(value, "-nopagemap") != NULL;
	host_logf(HOST_LOG_WARN, "debug.halo.art_overlap=%s: a stand-in for ART's large object space at %08llx-%08llx "
		"(%ld kB of it swapped out)", value, SIMULATED_SPACE_BASE, SIMULATED_SPACE_BASE + SIMULATED_SPACE_SIZE,
		mapping_swap_kb(SIMULATED_SPACE_BASE));
}

void host_memory_reserve_early(void)
{
	simulate_art_overlap();
	if (reserve_fixed("at start-up (JNI_OnLoad)") == 0)
		host_logf(HOST_LOG_INFO, "reserved the Xbox memory window and the image range at start-up");
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	if (base != HALO_GUEST_IMAGE_BASE || size > HALO_GUEST_IMAGE_RESERVE)
	{
		host_logf(HOST_LOG_ERROR, "the guest image (%08x, %u bytes) does not fit its range", base, size);
		return -1;
	}
	if (reserve_fixed("when the image loaded") != 0)
	{
		errno = fixed_error;
		return -1;
	}
	window_base = HALO_GUEST_WINDOW_BASE;
	window_end = window_base + HALO_GUEST_WINDOW_SIZE;
	image_base = base;
	image_end = base + round_up(size);
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

void host_memory_watch_use_hashes(void)
{
	watch_hashing = 1;
}

/* memory_watch_protect: write-protects the pages of a range, or keeps their
hash under host_memory_watch_use_hashes; a size outside the guest window
is ignored */
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

/* memory_watch_generation: the newest generation of the pages of a range.
Under hashing this hashes the pages again, once a frame. */
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

/* memory_watch_prepare_write: unprotects the pages the host itself is about
to write into. Nothing under hashing: the write shows in the next hash and
there is no protection to fault on. */
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
		if (page_protected[page])
			mark_written(page);
	}
}

/* memory_watch_forget: the pages of a range were remapped or reprotected, so
treat them as written and unwatched */
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
		page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	}
}

/* memory_watch_begin_frame: pages may be hashed again; nothing under page
protection */
void host_memory_watch_begin_frame(void)
{
	if (watch_hashing)
		watch_hash_begin_frame(&page_hashes);
}
