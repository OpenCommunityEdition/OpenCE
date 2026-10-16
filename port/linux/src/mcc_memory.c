/* MCC cache address reservations. A file can request a free 64 MiB window;
 * MAP_FIXED_NOREPLACE never replaces an Xbox, CE or host allocation. */
#include "platform.h"
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#ifdef HALO_WINDOWS
/* Keep the linked MCC window out of the Windows heap/driver allocator's
 * way before starting the renderer. Reserve address space, not committed
 * pages; unloading decommits the pages while retaining our reservation. */
static void *mcc_window;
static int mcc_window_active;

static void *mcc_window_hold(uint32_t address)
{
    void *wanted = (void *)(uintptr_t)address;
    void *allocation = mmap(wanted, 0x04000000u, PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
    if (allocation == wanted) return allocation;
    if (allocation != MAP_FAILED) {
        munmap(allocation, 0x04000000u);
        errno = EINVAL;
    }
    return NULL;
}

__attribute__((constructor(103)))
static void mcc_window_initialize(void)
{
    mcc_window = mcc_window_hold(0x50000000u);
    if (!mcc_window)
        platform_log("mcc memory: early reservation at 50000000 failed (%s); will retry at map load", strerror(errno));
}

__attribute__((destructor))
static void mcc_window_dispose(void)
{
    if (mcc_window) munmap(mcc_window, 0x04000000u);
}
#endif

void *mcc_memory_reserve(uint32_t address, uint32_t size)
{
    void *wanted = (void *)(uintptr_t)address;
    void *allocation;
    /* Keep clear of the Xbox arena, Custom Edition window and low addresses.
     * MCC Tool's current base is 0x50000000; other non-overlapping linked
     * addresses are accepted only if the OS confirms the window is free. */
    if ((address & 0xFFFFu) || size != 0x04000000u ||
        address < 0x42000000u || address > 0x70000000u) {
        errno = EINVAL;
        return NULL;
    }
#ifdef HALO_WINDOWS
    if (mcc_window_active) {
        errno = EBUSY;
        return NULL;
    }
    /* Other accepted linked bases retain their previous support. Only our
     * own inactive reservation can be released or recommitted. */
    if (mcc_window && mcc_window != wanted) {
        if (munmap(mcc_window, size)) return NULL;
        mcc_window = NULL;
    }
    if (!mcc_window) mcc_window = mcc_window_hold(address);
    if (!mcc_window) return NULL;
    allocation = mmap(mcc_window, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0);
    if (allocation == mcc_window) {
        mcc_window_active = 1;
        return allocation;
    }
#else
    allocation = mmap(wanted, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
    if (allocation == wanted)
        return allocation;
#endif
    if (allocation != MAP_FAILED) {
        munmap(allocation, size);
        errno = EINVAL;
    }
    return NULL;
}

void mcc_memory_release(void *allocation, uint32_t size)
{
#ifdef HALO_WINDOWS
    if (allocation && allocation == mcc_window && size == 0x04000000u && mcc_window_active) {
        if (mmap(allocation, size, PROT_NONE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0) == allocation)
            mcc_window_active = 0;
        else
            platform_log("mcc memory: could not decommit tag window (%s)", strerror(errno));
    }
#else
    if (allocation)
        munmap(allocation, size);
#endif
}
