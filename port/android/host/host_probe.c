/*
HOST_PROBE.C

Reports where the game's memory window holds values that point back into
it.

The game data is linked to the addresses inside the window (a map file's
tag cache sits at 0x803a6000, and its tag directory holds a name and a
base address per tag), so a build that puts the window somewhere else can
only work if every one of those values is moved with it. This probe finds
them, so that work can be written against what is really there rather
than against a guess.

The window is the guest's memory and this is the same process, so the
probe just reads it: 32-bit words, aligned, whose value is an address
inside the window. A value that lands there by accident (a texel of
0x80808080, say) cannot be told from a pointer by looking at it, so the
report groups the values by the 4 KB page they were found in. A handful
of pages full of pointers is a handful of structures to move; pages that
are mostly texture data are not.

Two things make the reading unsafe, and both are dealt with here rather
than assumed away:

- the guest maps and unmaps as it runs (thread stacks come and go), so a
range that was readable when the probe looked may be gone by the time it
reads. host_probe_skip_fault() catches the fault and the page is
skipped, so the worst case is a page missing from the report;
- the part of the window the guest has not mapped is reserved, and
reading that faults. The probe therefore takes its ranges from
/proc/self/maps and only reads what is mapped and readable.

It starts itself a few seconds after the game does and then looks every
few seconds, and says something whenever the numbers move, which is how
one notices that a map has finished loading. Nothing else in the app
uses it.
*/

#include "host.h"

#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PROBE_INTERVAL_SECONDS 3
#define PROBE_STARTUP_SECONDS 8
#define PROBE_PAGE 4096
#define PROBE_TOP_PAGES 6
#define PROBE_VALUES 4
#define PROBE_MAX_RANGES 512

/* one readable piece of the window */
struct probe_range
{
	unsigned long start, end;
	char permissions[8];
	char name[96];
};

struct probe_page
{
	unsigned long address;
	unsigned long pointers;
	const char *range;
};

static struct probe_range probe_ranges[PROBE_MAX_RANGES];
static int probe_range_count;

/* where a fault is sent back to, per thread: the probe is the only code
that reads memory the guest may unmap under it */
static __thread sigjmp_buf probe_fault_jump;
static __thread int probe_fault_armed;

void host_probe_start(void);

/* a fault on the probe's thread, inside the window, is a page the guest
unmapped as the probe read it: back to the loop, which skips the page */
int host_probe_skip_fault(uintptr_t address)
{
	if (!probe_fault_armed)
		return 0;
	if (address < HALO_GUEST_WINDOW_BASE || address >= (uintptr_t)host_memory_window_base() + HALO_GUEST_WINDOW_SIZE)
		return 0;
	probe_fault_armed = 0;
	siglongjmp(probe_fault_jump, 1);
	return 1;
}

static int probe_collect_ranges(void)
{
	/* the window is wherever the host put it, which is not necessarily where
	the game expects it */
	const unsigned long long window = host_memory_window_base();
	const unsigned long long window_end = window + HALO_GUEST_WINDOW_SIZE;
	FILE *maps = fopen("/proc/self/maps", "r");
	char line[512];

	probe_range_count = 0;
	if (!maps)
		return 0;
	while (probe_range_count < PROBE_MAX_RANGES && fgets(line, sizeof(line), maps))
	{
		unsigned long long first, last;
		char permissions[8] = { 0 }, name[96] = { 0 };
		struct probe_range *range;

		if (sscanf(line, "%llx-%llx %7s", &first, &last, permissions) != 3)
			continue;
		if (permissions[0] != 'r')
			continue; /* the guest leaves the rest of the window reserved */
		if (last <= window || first >= window_end)
			continue;
		range = &probe_ranges[probe_range_count++];
		range->start = (unsigned long)(first < window ? window : first);
		range->end = (unsigned long)(last > window_end ? window_end : last);
		range->permissions[0] = permissions[0];
		sscanf(line, "%*s %*s %*s %*s %*s %95[^\n]", name);
		snprintf(range->name, sizeof(range->name), "%s", name[0] ? name : "(anonymous)");
	}
	fclose(maps);
	return probe_range_count;
}

static const char *probe_range_name(unsigned long address)
{
	int index;

	for (index = 0; index < probe_range_count; index++)
	{
		if (address >= probe_ranges[index].start && address < probe_ranges[index].end)
			return probe_ranges[index].name;
	}
	return "(unmapped)";
}

/* the values of one page, the first few of them; 0 if the page was
unmapped as it was read */
static unsigned long page_pointers(unsigned long address, uint32_t *values, unsigned long capacity){
	const uint32_t *words = (const uint32_t *)(uintptr_t)address;
	unsigned long found = 0, kept = 0;
	int index;

	probe_fault_armed = 1;
	if (sigsetjmp(probe_fault_jump, 1) == 0)
	{
		for (index = 0; index < PROBE_PAGE / 4; index++)
		{
			uint32_t value = words[index];

			if (value & 3)
				continue;
			if (value < host_memory_window_base() || value >= (uintptr_t)host_memory_window_base() + HALO_GUEST_WINDOW_SIZE)
				continue;
			if (kept < capacity)
				values[kept++] = value;
			found++;
		}
	}
	probe_fault_armed = 0;
	return found;
}

/* keeps the busiest pages seen so far */
static void page_remember(struct probe_page *top, int *count, unsigned long address,
	unsigned long pointers, const char *range)
{
	int index, worst;

	if (*count < PROBE_TOP_PAGES)
	{
		top[*count].address = address;
		top[*count].pointers = pointers;
		top[*count].range = range;
		(*count)++;
		return;
	}
	worst = 0;
	for (index = 1; index < PROBE_TOP_PAGES; index++)
	{
		if (top[index].pointers < top[worst].pointers)
			worst = index;
	}
	if (top[worst].pointers < pointers)
	{
		top[worst].address = address;
		top[worst].pointers = pointers;
		top[worst].range = range;
	}
}

/* The guest's tag directory, read here rather than asked for: the guest's
own printf crosses to the host with 32-bit arguments, so what it prints is
not what it means, while this reads the same memory directly. The two
globals are named rather than numbered, because a rebuild moves them. */
static void probe_tag_directory(void)
{
	const unsigned long image = host_image.base;
	/* the guest is 32-bit, so its longs and pointers are four bytes even
	though this is a 64-bit process: everything read here is a 32-bit word */
	const uint32_t *globals;
	uint32_t instances, header, count, index, bsp;

	if (host_image.end == 0 || !host_image.cache_file_globals || !host_image.global_tag_instances)
		return;
	globals = (const uint32_t *)(image + (host_image.cache_file_globals - HALO_GUEST_IMAGE_BASE));
	/* cache_file_globals is { boolean tags_loaded; byte pad[3];
	struct cache_file_header header; struct cache_file_tag_header *tag_header;
	struct cache_file_structure_bsp_header *structure_bsp_header; } and the
	header is 0x800 bytes, so the pointers are at 0x804 and 0x808 */
	header = globals[0x804 / sizeof(uint32_t)];
	host_logf(HOST_LOG_INFO, "tag directory: header %08x, loaded %d, instances %08x",
		header, *(const int *)globals,
		*(const uint32_t *)(image + (host_image.global_tag_instances - HALO_GUEST_IMAGE_BASE)));
	/* where the level's own data is, so a page of pointers that were not
	moved can be placed against it */
	bsp = globals[0x808 / sizeof(uint32_t)];
	if (bsp >= host_memory_window_base() && bsp < host_memory_window_base() + HALO_GUEST_WINDOW_SIZE)
		host_logf(HOST_LOG_INFO, "  structure bsp header at %08x, window offset %08lx",
			bsp, (unsigned long)(bsp - host_memory_window_base()));
	/* the window is not necessarily at its preferred address, so the range it
	can hold a pointer into starts where the host put it */
	if (!header || header < host_memory_window_base())
		return;
	count = ((const uint32_t *)header)[3];
	instances = ((const uint32_t *)header)[0];
	host_logf(HOST_LOG_INFO, "  %u tags, instances at %08x, vertex buffers %08x",
		count, instances, ((const uint32_t *)header)[5]);
	for (index = 0; index < count && index < 3; index++)
	{
		/* a tag instance is 0x20 bytes, or eight words */
		const uint32_t *instance = (const uint32_t *)instances + index * 8;

		host_logf(HOST_LOG_INFO, "    tag %u: group %08x, index %d, name %08x, base %08x",
			index, instance[0], (int)instance[3], instance[4], instance[5]);
	}
}

/* whether the guest has that page of the window mapped, out of the ranges
already read; a pointer moved to where the window is should land on one */
static int probe_mapped(unsigned long address)
{
	int index;

	for (index = 0; index < probe_range_count; index++)
	{
		if (address >= probe_ranges[index].start && address < probe_ranges[index].end)
			return 1;
	}
	return 0;
}

/* whether a value read out of the game data is a pointer the port has not
moved. Plenty of ordinary data looks like an address in the old window (a
float of -0.0f is 0x80000000, and there are thousands of those), so looking
at the value is not enough: moved to where the window is, it has to land on
a page this process has mapped. Noise moves to nothing. */
static int probe_value_is_missed_pointer(uint32_t value)
{
	int32_t shift;
	unsigned long moved;

	if (value & 3)
		return 0;
	if (value < HALO_GUEST_WINDOW_BASE || value >= HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
		return 0;
	/* the guest is 32-bit, so the shift wraps in 32 bits; computed in 64 it
	lands above the window and nothing is ever found */
	shift = (int32_t)((int32_t)host_memory_window_base() - (int32_t)HALO_GUEST_WINDOW_BASE);
	moved = (unsigned long)(uint32_t)(value + shift);
	return probe_mapped(moved);
}

/* the missed pointers in one page */
static unsigned long page_stale(unsigned long address)
{
	const uint32_t *words = (const uint32_t *)(uintptr_t)address;
	unsigned long found = 0;
	int index;

	probe_fault_armed = 1;
	if (sigsetjmp(probe_fault_jump, 1) == 0)
	{
		for (index = 0; index < PROBE_PAGE / 4; index++)
		{
			if (probe_value_is_missed_pointer(words[index]))
				found++;
		}
	}
	probe_fault_armed = 0;
	return found;
}

/* the first few of one page's stale values, and how many it has */
static unsigned long page_stale_values(unsigned long address, uint32_t *values, unsigned long capacity)
{
	const uint32_t *words = (const uint32_t *)(uintptr_t)address;
	unsigned long found = 0, kept = 0;
	int index;

	probe_fault_armed = 1;
	if (sigsetjmp(probe_fault_jump, 1) == 0)
	{
		for (index = 0; index < PROBE_PAGE / 4; index++)
		{
			uint32_t value = words[index];

			if (!probe_value_is_missed_pointer(value))
				continue;
			if (kept < capacity)
				values[kept++] = value;
			found++;
		}
	}
	probe_fault_armed = 0;
	return found;
}

static void probe_once(void)
{
	struct probe_page top[PROBE_TOP_PAGES];
	struct probe_page stale_top[PROBE_TOP_PAGES];
	unsigned long total = 0, scanned = 0, total_stale = 0;
	int count = 0, index, range, window_first = 1;
	int stale_count = 0;

	probe_tag_directory();
	total_stale = 0;
	if (!probe_collect_ranges())
		return;
	for (range = 0; range < probe_range_count; range++)
	{
		unsigned long address;
		unsigned long range_total = 0;
		unsigned long range_stale = 0;

		for (address = probe_ranges[range].start; address + PROBE_PAGE <= probe_ranges[range].end;
			address += PROBE_PAGE)
		{
			unsigned long pointers = page_pointers(address, NULL, 0);

			scanned += PROBE_PAGE;
			{
				unsigned long stale = page_stale(address);

				range_stale += stale;
				total_stale += stale;
				if (stale)
					page_remember(stale_top, &stale_count, address, stale, probe_ranges[range].name);
			}
			if (!pointers)
				continue;
			range_total += pointers;
			total += pointers;
			page_remember(top, &count, address, pointers, probe_ranges[range].name);
		}
		if (window_first)
		{
			/* what the guest has mapped inside the window: the arena it
			reserved, and the parts of it it has written to */
			host_logf(HOST_LOG_INFO, "    %08lx-%08lx %c%s", probe_ranges[range].start,
				probe_ranges[range].end, probe_ranges[range].permissions[0],
				probe_ranges[range].name);
		}
		window_first = 0;
		if (range_total || range_stale)
		{
			host_logf(HOST_LOG_INFO, "  %s: %lu values point into the window, %lu still name the old one (%lu KB mapped here)",
				probe_ranges[range].name, range_total, range_stale,
				(probe_ranges[range].end - probe_ranges[range].start) / 1024);
		}
	}
	host_logf(HOST_LOG_INFO, "window: %lu values point into it, %lu still name the old one, in %d busiest pages of %lu KB readable",
		total, total_stale, count, scanned / 1024);
	for (index = 0; index < stale_count; index++)
	{
		uint32_t values[PROBE_VALUES];
		unsigned long kept = page_stale_values(stale_top[index].address, values, PROBE_VALUES);

		host_logf(HOST_LOG_INFO, "    stale %08lx (+%08lx in the window) [%s]: %lu values  %08lx %08lx %08lx %08lx",
			stale_top[index].address,
			(unsigned long)(stale_top[index].address - host_memory_window_base()),
			stale_top[index].range, stale_top[index].pointers,
			(unsigned long)(kept > 0 ? values[0] : 0), (unsigned long)(kept > 1 ? values[1] : 0),
			(unsigned long)(kept > 2 ? values[2] : 0), (unsigned long)(kept > 3 ? values[3] : 0));
	}
	for (index = 0; index < count; index++)
	{
		uint32_t values[PROBE_VALUES];
		unsigned long kept = page_pointers(top[index].address, values, PROBE_VALUES);

		host_logf(HOST_LOG_INFO, "    %08lx [%s]: %lu values  %08lx %08lx %08lx %08lx",
			top[index].address, top[index].range, top[index].pointers,
			(unsigned long)(kept > 0 ? values[0] : 0), (unsigned long)(kept > 1 ? values[1] : 0),
			(unsigned long)(kept > 2 ? values[2] : 0), (unsigned long)(kept > 3 ? values[3] : 0));
	}
}

static void *probe_thread(void *unused)
{
	int seconds = 0;

	(void)unused;
	for (;;)
	{
		sleep(PROBE_INTERVAL_SECONDS);
		/* the game is still loading in the first passes */
		if (++seconds * PROBE_INTERVAL_SECONDS > PROBE_STARTUP_SECONDS)
			probe_once();
	}
	return NULL;
}

void host_probe_start(void)
{
	pthread_t thread;

	pthread_create(&thread, NULL, probe_thread, NULL);
	pthread_detach(thread);
}
