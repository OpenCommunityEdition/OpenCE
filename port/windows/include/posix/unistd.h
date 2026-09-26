/*
UNISTD.H

The POSIX names the platform layer uses: file descriptors come from the
Windows C runtime (<io.h>), sysconf from port/windows/src/win32_posix.c.
*/

#ifndef __HALO_WINDOWS_UNISTD_H
#define __HALO_WINDOWS_UNISTD_H

#include <io.h>
#include <process.h>
#include <stdlib.h>
#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

#define _SC_PAGESIZE 1
#define _SC_PHYS_PAGES 2
#define _SC_AVPHYS_PAGES 3
#define _SC_NPROCESSORS_ONLN 4

/* 64-bit, unlike the Windows C runtime's off_t */
typedef long long halo_windows_off_t;
#define off_t halo_windows_off_t

long sysconf(int name);
/* reads and writes at a file position, as ReadFile and WriteFile do with an
OVERLAPPED offset (which also moves the file pointer) */
ssize_t pread(int descriptor, void *buffer, size_t count, off_t offset);
ssize_t pwrite(int descriptor, const void *buffer, size_t count, off_t offset);
/* only for /proc/self/exe, which gives the executable's path with forward
slashes (port/linux/src/xbox_files.c finds the data folder from it) */
ssize_t readlink(const char *path, char *buffer, size_t size);
int pause(void);

#endif
