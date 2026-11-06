/*
WIN64_HOST.H

What the Windows x64 host's own files share (port/windows/host): the host
that runs the 64-bit x86 Linux build's guest image (x32 code,
tools/linux_guest_build.py) as a Windows program. The files it shares with
the Linux and Android hosts (port/android/host: host_common.c,
host_loader.c, host_sdl.c, host_watch_hash.c; port/linux/host/host_desktop.c)
see the few POSIX names they use through include/ here. docs/x64_route.md,
"Windows x64 host", has the design.
*/

#ifndef WIN64_HOST_H
#define WIN64_HOST_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "host.h"
#include "sys/mman.h"

/* ---------- start-up (win64_main.c, win64_memory.c, win64_thread.c) */

/* reserves the guest's fixed ranges (the Xbox window, the image's, Custom
Edition's tag cache) at the very start of the process; 0 on success. With
adopt, ranges a parent process reserved for this one (win64_main.c's
relaunch) are taken as they are. */
int host_memory_reserve_fixed(int adopt);
/* the fixed ranges, for a parent to reserve in its child */
int host_memory_fixed_count(void);
void host_memory_fixed_range(int index, uint64_t *base, uint64_t *size);
/* the vectored exception handler: the write tracking's faults, and a report
of any other */
void host_install_exception_handler(void);
void host_threads_initialize(void);
/* the calling thread's waitable timer, high-resolution where Windows has
one, for the guest's sleeps (win64_syscall.c); NULL on a thread that has no
guest stack */
HANDLE host_thread_timer(void);

/* ---------- guest memory (win64_memory.c) */

/* changes the protection of pages of host_low_map's (committing pages
that were reserved only); 0 on success */
int host_low_protect(void *address, size_t size, int protection);

/* ---------- system calls (win64_syscall.c) */

/* a Windows error as the guest's errno (halo_guest_syscall.h) */
int host_errno_of_windows(DWORD error);
/* a UTF-8 path from the guest as a wide one for Windows; NULL (and
*error, a guest errno) if it cannot be converted. The result is freed. */
wchar_t *host_path_in(const char *path, int *error);
/* a wide path from Windows as UTF-8 with forward slashes, the guest's
form; the length, or -1 if it does not fit */
int host_path_out(const wchar_t *path, char *buffer, size_t size);

/* ---------- OpenGL (win64_gl.c) */

/* resolves the OpenGL entry points of the import thunks once a context is
current (the generated host_thunk_table.c) */
void host_gl_resolve_entry_points(void);
int host_thunks_resolve_gl(void *(*resolve)(const char *name));

#endif
