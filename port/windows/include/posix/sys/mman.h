/*
SYS/MMAN.H

The memory mapping calls the platform layer makes for the Xbox memory
window (port/linux/src/xbox_memory.c), over VirtualAlloc
(port/windows/src/win32_posix.c): a PROT_NONE MAP_NORESERVE mapping reserves
(or, inside a reservation, releases) pages, any other anonymous mapping
commits fresh zeroed pages.
*/

#ifndef __HALO_WINDOWS_SYS_MMAN_H
#define __HALO_WINDOWS_SYS_MMAN_H

#include <stddef.h>

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000
#define MAP_FIXED_NOREPLACE 0x100000

#define MAP_FAILED ((void *)-1)

void *mmap(void *address, size_t length, int protection, int flags, int descriptor, long offset);
int munmap(void *address, size_t length);
int mprotect(void *address, size_t length, int protection);

#endif
