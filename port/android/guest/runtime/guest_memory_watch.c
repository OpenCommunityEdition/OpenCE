/*
GUEST_MEMORY_WATCH.C

Guest memory write tracking (port/linux/src/memory_watch.c's interface).
Page protection faults are delivered to the host, which owns signal
handling in the Android process, so the tracking itself lives there
(port/android/host/host_memory.c).

The macOS host's VM (HALO_GUEST_VM) tracks writes in its own page tables
(port/macos/host/host_watch.c): the guest reads each page's generation from
a table it shares with the host, with no exit, and asks the host only to
protect pages that are not protected yet, or to forget them.
*/

#include "platform.h"
#include "guest_host.h"
#include "halo_android_abi.h"

#ifdef HALO_GUEST_VM
static struct halo_guest_watch_table watch;
/* 1 once the host shares the table, -1 if it cannot (then every range
reads as written: always correct, as a caller only redoes work) */
static int watch_state;
static unsigned long fallback_serial;

static int watch_ready(void)
{
	int state = __atomic_load_n(&watch_state, __ATOMIC_ACQUIRE);

	if (!state)
	{
		/* (the host shares one table however many threads ask) */
		state = host_memory_watch_share((unsigned int)&watch) == 0 ? 1 : -1;
		__atomic_store_n(&watch_state, state, __ATOMIC_RELEASE);
	}
	return state > 0;
}

/* the pages of the window [address, address + size) touches, first to
last; 0 if none */
static int watch_pages(unsigned long address, unsigned long size, unsigned long *first, unsigned long *last)
{
	unsigned long start = address, end = address + size;

	if (!size || end <= HALO_GUEST_WINDOW_BASE || start >= HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE)
		return 0;
	if (start < HALO_GUEST_WINDOW_BASE)
		start = HALO_GUEST_WINDOW_BASE;
	if (end > HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE || end < start)
		end = HALO_GUEST_WINDOW_BASE + HALO_GUEST_WINDOW_SIZE;
	*first = (start - HALO_GUEST_WINDOW_BASE) / HALO_GUEST_WATCH_PAGE_SIZE;
	*last = (end - 1 - HALO_GUEST_WINDOW_BASE) / HALO_GUEST_WATCH_PAGE_SIZE;
	return 1;
}

/* whether any page of [first, last] is (or is not) protected */
static int any_page(unsigned long first, unsigned long last, unsigned char protected_page)
{
	unsigned long page;

	for (page = first; page <= last; page++)
	{
		if ((__atomic_load_n(&watch.protected_page[page], __ATOMIC_RELAXED) != 0) == protected_page)
			return 1;
	}
	return 0;
}

void memory_watch_initialize(void)
{
	watch_ready();
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	unsigned long first, last;

	/* (the host is asked only when a page is not protected yet) */
	if (watch_ready() && watch_pages(address, size, &first, &last) && any_page(first, last, 0))
		host_memory_watch_protect(address, size);
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	unsigned long first, last, page, newest = 0;

	if (!watch_ready())
		return __atomic_add_fetch(&fallback_serial, 1, __ATOMIC_RELAXED);
	if (!watch_pages(address, size, &first, &last))
		return 0;
	for (page = first; page <= last; page++)
	{
		unsigned long generation = __atomic_load_n(&watch.generation[page], __ATOMIC_ACQUIRE);

		if (generation > newest)
			newest = generation;
	}
	return newest;
}

unsigned long memory_watch_serial(void)
{
	if (!watch_ready())
		return __atomic_add_fetch(&fallback_serial, 1, __ATOMIC_RELAXED);
	return __atomic_load_n(&watch.serial, __ATOMIC_RELAXED);
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	unsigned long first, last;

	if (watch_ready() && watch_pages((unsigned long)address, size, &first, &last) && any_page(first, last, 1))
		host_memory_watch_prepare_write((unsigned int)address, size);
}

void memory_watch_forget(void *address, unsigned long size)
{
	unsigned long first, last;

	if (watch_ready() && watch_pages((unsigned long)address, size, &first, &last))
		host_memory_watch_forget((unsigned int)address, size);
}

void memory_watch_begin_frame(void)
{
}
#else

void memory_watch_initialize(void)
{
	host_memory_watch_initialize();
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	host_memory_watch_protect(address, size);
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	return host_memory_watch_generation(address, size);
}

unsigned long memory_watch_serial(void)
{
	return host_memory_watch_serial();
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	host_memory_watch_prepare_write((unsigned int)address, size);
}

void memory_watch_forget(void *address, unsigned long size)
{
	host_memory_watch_forget((unsigned int)address, size);
}

void memory_watch_begin_frame(void)
{
	host_memory_watch_begin_frame();
}
#endif
