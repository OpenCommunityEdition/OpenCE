/*
HOST_WATCH.C

Write tracking for the guest's renderer (port/linux/src/memory_watch.c's
interface), in the VM's own page tables. The guest shares a table
(struct halo_guest_watch_table, halo_android_abi.h) with
host_memory_watch_share and reads each page's generation from it without
an exit. Protecting a page removes write access in the VM's stage-2 tables
(hv_vm_protect, at the host's page size); the first write to it is a data
abort that exits to the host (host_watch_fault, from host_vm.c), which gives
the page a new generation, restores write access and lets the write run
again. The host's own writes into guest memory are not seen, as the
kernel's are not on Linux: the file layer reads into the window through a
bounce buffer (xbox_files.c) and announces other writes
(memory_watch_prepare_write).
*/

#include "host.h"

#include <Hypervisor/Hypervisor.h>
#include <pthread.h>
#include <unistd.h>

static struct halo_guest_watch_table *table;
static pthread_mutex_t watch_lock = PTHREAD_MUTEX_INITIALIZER;

/* the window's pages that [address, address + size) touches, first to
last; 0 if none */
static int watch_pages(unsigned int address, unsigned int size, uint32_t *first, uint32_t *last)
{
	uint64_t start = address, end = (uint64_t)address + size;

	if (!table || !size || end <= HALO_GUEST_WINDOW_BASE || start >= HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
		return 0;
	if (start < HALO_GUEST_WINDOW_BASE)
		start = HALO_GUEST_WINDOW_BASE;
	if (end > HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
		end = HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE;
	*first = (uint32_t)((start - HALO_GUEST_WINDOW_BASE) / HALO_GUEST_WATCH_PAGE_SIZE);
	*last = (uint32_t)((end - 1 - HALO_GUEST_WINDOW_BASE) / HALO_GUEST_WATCH_PAGE_SIZE);
	return 1;
}

static void set_access(uint32_t first, uint32_t count, hv_memory_flags_t flags)
{
	hv_ipa_t address = HALO_GUEST_WINDOW_BASE + (hv_ipa_t)first * HALO_GUEST_WATCH_PAGE_SIZE;

	if (hv_vm_protect(address, (size_t)count * HALO_GUEST_WATCH_PAGE_SIZE, flags) != HV_SUCCESS)
		host_fatal("cannot change the VM's access to guest memory at %#llx", (unsigned long long)address);
}

/* (under watch_lock) a new generation for the page, now writable */
static void mark_written(uint32_t page)
{
	uint32_t generation = __atomic_add_fetch(&table->serial, 1, __ATOMIC_RELAXED);

	if (table->protected_page[page])
	{
		set_access(page, 1, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
		table->protected_page[page] = 0;
	}
	__atomic_store_n(&table->generation[page], generation, __ATOMIC_RELEASE);
}

int host_memory_watch_share(unsigned int address)
{
	struct halo_guest_watch_table *shared;
	uint32_t page;

	/* (hv_vm_protect's granularity is the host's page) */
	if (HALO_GUEST_WATCH_PAGE_SIZE % getpagesize())
		return -1;
	shared = (struct halo_guest_watch_table *)opence_guest_host_pointer(host_guest, address, sizeof(*shared));
	if (!shared || (address & 3) || !host_guest_pointer(HALO_GUEST_WINDOW_BASE) ||
		!opence_guest_host_pointer(host_guest, HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE))
		return -1;
	pthread_mutex_lock(&watch_lock);
	if (table && table != shared)
	{
		pthread_mutex_unlock(&watch_lock);
		return -1;
	}
	if (!table)
	{
		shared->serial = 1;
		for (page = 0; page < HALO_GUEST_WATCH_PAGE_COUNT; page++)
		{
			shared->generation[page] = 0;
			shared->protected_page[page] = 0;
		}
		table = shared;
	}
	pthread_mutex_unlock(&watch_lock);
	return 0;
}

void host_memory_watch_initialize(void)
{
}

void host_memory_watch_protect(unsigned int address, unsigned int size)
{
	uint32_t first, last, page, run;

	if (!watch_pages(address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	/* (each run of unprotected pages with one call) */
	for (page = first; page <= last; page = run)
	{
		for (run = page; run <= last && !table->protected_page[run]; run++)
			table->protected_page[run] = 1;
		if (run > page)
			set_access(page, run - page, HV_MEMORY_READ | HV_MEMORY_EXEC);
		else
			run = page + 1;
	}
	pthread_mutex_unlock(&watch_lock);
}

unsigned int host_memory_watch_generation(unsigned int address, unsigned int size)
{
	uint32_t first, last, page, newest = 0;

	if (!watch_pages(address, size, &first, &last))
		return 0;
	for (page = first; page <= last; page++)
	{
		uint32_t generation = __atomic_load_n(&table->generation[page], __ATOMIC_ACQUIRE);

		if (generation > newest)
			newest = generation;
	}
	return newest;
}

unsigned int host_memory_watch_serial(void)
{
	return table ? __atomic_load_n(&table->serial, __ATOMIC_RELAXED) : 0;
}

void host_memory_watch_prepare_write(unsigned int address, unsigned int size)
{
	uint32_t first, last, page;

	if (!watch_pages(address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	for (page = first; page <= last; page++)
	{
		if (table->protected_page[page])
			mark_written(page);
	}
	pthread_mutex_unlock(&watch_lock);
}

void host_memory_watch_forget(unsigned int address, unsigned int size)
{
	uint32_t first, last, page;

	if (!watch_pages(address, size, &first, &last))
		return;
	pthread_mutex_lock(&watch_lock);
	for (page = first; page <= last; page++)
		mark_written(page);
	pthread_mutex_unlock(&watch_lock);
}

void host_memory_watch_begin_frame(void)
{
}

int host_watch_fault(uint64_t address, int write)
{
	uint32_t first, last;

	if (!write || address > 0xffffffffu || !watch_pages((unsigned int)address, 1, &first, &last))
		return 0;
	pthread_mutex_lock(&watch_lock);
	/* (another vCPU may have taken the same page's fault first: then the
	write only runs again, its page writable for certain) */
	if (table->protected_page[first])
		mark_written(first);
	else
		set_access(first, 1, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
	pthread_mutex_unlock(&watch_lock);
	return 1;
}
