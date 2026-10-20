/*
GUEST_MEMORY_WATCH.C

Guest memory write tracking (port/linux/src/memory_watch.c's interface).
Page protection faults are delivered to the host, which owns signal
handling in the Android process, so the tracking itself lives there
(port/android/host/host_memory.c).

The macOS host's VM (HALO_GUEST_VM) does not track writes: every range
reads as written, which is always correct (a caller only redoes work), so
the guest answers that itself, with no exit to the host.
*/

#include "platform.h"
#include "guest_host.h"

#ifdef HALO_GUEST_VM
static unsigned long watch_serial;

void memory_watch_initialize(void)
{
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
	return __atomic_add_fetch(&watch_serial, 1, __ATOMIC_RELAXED);
}

unsigned long memory_watch_serial(void)
{
	return __atomic_add_fetch(&watch_serial, 1, __ATOMIC_RELAXED);
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	(void)address;
	(void)size;
}

void memory_watch_forget(void *address, unsigned long size)
{
	(void)address;
	(void)size;
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
