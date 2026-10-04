/*
HOST_MEMORY_WATCH.C

Write tracking for the Xbox window's cached textures (the interface of
port/linux/src/memory_watch.c, which the renderer reaches through the
memory-watch doorbells): the pages behind a cached texture are write
protected in stage 2; the first write exits, the handler records a new
generation for the page, grants the write and lets the vCPU retry.

On macOS the guest runs in a VM, so the "page fault" is the same
HV_EXIT_REASON_EXCEPTION data abort as a doorbell, with the faulting
address inside the window. The host distinguishes them by address: the
doorbell page is one, the window another.
*/

#include "host.h"

#include <Hypervisor/Hypervisor.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>

#define WATCH_PAGE_SIZE 0x10000u /* the stage-2 granularity */
#define WATCH_PAGE_COUNT (GUEST_WINDOW_SIZE / WATCH_PAGE_SIZE)

static uint8_t page_protected[WATCH_PAGE_COUNT];
static uint32_t page_generation[WATCH_PAGE_COUNT];
static volatile uint32_t current_generation = 1;
static int watch_active;

static int in_window(uint64_t address)
{
	return address >= GUEST_WINDOW_BASE && address - GUEST_WINDOW_BASE < GUEST_WINDOW_SIZE;
}

static uint64_t watch_page(uint64_t address)
{
	return (address - GUEST_WINDOW_BASE) / WATCH_PAGE_SIZE;
}

static void mark_written(uint64_t page)
{
	page_generation[page] = __sync_add_and_fetch(&current_generation, 1);
	page_protected[page] = 0;
	/* grant the write again in stage 2 */
	hv_vm_protect(GUEST_WINDOW_BASE + page * WATCH_PAGE_SIZE, WATCH_PAGE_SIZE,
		HV_MEMORY_READ | HV_MEMORY_WRITE);
}

void host_memory_watch_initialize(void)
{
	watch_active = 1;
}

void host_memory_watch_protect(unsigned int address, unsigned int size)
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
			hv_vm_protect(GUEST_WINDOW_BASE + page * WATCH_PAGE_SIZE, WATCH_PAGE_SIZE,
				HV_MEMORY_READ);
		}
	}
}

unsigned int host_memory_watch_generation(unsigned int address, unsigned int size)
{
	uint64_t first, last, page, newest = 0;

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
	return (unsigned int)newest;
}

unsigned int host_memory_watch_serial(void)
{
	return current_generation;
}

void host_memory_watch_prepare_write(unsigned int address, unsigned int size)
{
	uint64_t start = address, first, last, page;

	if (!watch_active || !size)
		return;
	if (start + size <= GUEST_WINDOW_BASE || start >= (uint64_t)GUEST_WINDOW_BASE + GUEST_WINDOW_SIZE)
		return;
	if (start < GUEST_WINDOW_BASE)
		start = GUEST_WINDOW_BASE;
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

void host_memory_watch_forget(unsigned int address, unsigned int size)
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

/* the doorbell dispatch calls this when a data abort lands in the window:
1 when it was a watched write (the caller resumes the vCPU without
advancing anything: the store re-executes) */
int host_memory_watch_fault(uint64_t address)
{
	uint64_t page;

	if (!watch_active || !in_window(address))
		return 0;
	page = watch_page(address);
	if (!page_protected[page])
		return 0;
	mark_written(page);
	return 1;
}