/*
SYS/MMAN.H

The memory mapping names the files the Windows x64 host shares with the
Linux and Android hosts use (host_common.c, host_loader.c), for anonymous
memory in the guest's own ranges only (port/windows/host/win64_memory.c).
*/

#ifndef WIN64_HOST_SYS_MMAN_H
#define WIN64_HOST_SYS_MMAN_H

#include <stddef.h>
#include <stdint.h>

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

/* (anonymous memory only, in the image's range or the pools) */
void *host_win_mmap(void *address, size_t size, int protection, int flags, int fd, long long offset);
int host_win_mprotect(void *address, size_t size, int protection);

#define mmap host_win_mmap
#define mprotect host_win_mprotect

#endif
