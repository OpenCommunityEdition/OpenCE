/* MCC cache address reservations. A file can request a free 64 MiB window;
 * MAP_FIXED_NOREPLACE never replaces an Xbox, CE or host allocation. */
#include "platform.h"
#include <sys/mman.h>
#include <stdint.h>

void *mcc_memory_reserve(uint32_t address, uint32_t size)
{
    void *wanted = (void *)(uintptr_t)address;
    void *allocation;
    /* Keep clear of the Xbox arena, Custom Edition window and low addresses.
     * MCC Tool's current base is 0x50000000; other non-overlapping linked
     * addresses are accepted only if the OS confirms the window is free. */
    if ((address & 0xFFFFu) || size != 0x04000000u ||
        address < 0x42000000u || address > 0x70000000u)
        return NULL;
    allocation = mmap(wanted, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
    if (allocation == wanted)
        return allocation;
    if (allocation != MAP_FAILED)
        munmap(allocation, size);
    return NULL;
}

void mcc_memory_release(void *allocation, uint32_t size)
{
    if (allocation)
        munmap(allocation, size);
}
