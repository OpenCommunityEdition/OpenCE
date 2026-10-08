/*
HOST.H

Internals of the macOS port's host executable (Halo.app/Contents/MacOS/halo).
See port/macos/README.md for the overall design and
port/android/include/halo_android_abi.h for the guest contract, which the
macOS port shares with Android (built with HALO_MACOS).

The names and signatures here are those of the Android host
(port/android/host/host.h), so that code written against one builds against
the other; what only macOS has follows them.
*/

#ifndef __HALO_MACOS_HOST_H
#define __HALO_MACOS_HOST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "halo_android_abi.h"

/* ---------- logging (standard error and <data root>/host.log; host_main.c) */

void host_logf(int priority, const char *format, ...) __attribute__((format(printf, 2, 3)));
/* (Android's log priorities, which the guest's host_log passes too) */
#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6
#define HOST_LOG_FATAL 7
/* guest services also used inside the host (host_main.c) */
void host_exit(int code) __attribute__((noreturn));
/* errno after a host call, as the guest's Linux number (host_linux.h) */
int host_errno(void);

/* logs, shows the message to the player and terminates */
void host_fatal(const char *format, ...) __attribute__((format(printf, 1, 2), noreturn));

/* ---------- guest memory (host_memory.c)

All memory the guest can address lies below 4 GB. The host claims all of
that range at start-up (host_memory_claim), before any library can map
into it, and hands it out from then on: the Xbox window and the image's
range, then everything else the guest maps (its malloc arenas, thread
stacks, anonymous and file mappings). */

/* claims every free page below 4 GB; returns 0 on success */
int host_memory_claim(void);
/* 1 if every page of the range was free and is now claimed */
int host_memory_claimed(uint64_t address, uint64_t size);
/* reserves the fixed ranges (the window and the image) out of the claim;
returns 0 on success */
int host_memory_initialize(uint32_t image_base, uint32_t image_size);
/* page-granular allocations below 4 GB; NULL on failure */
void *host_low_map(size_t size, int protection);
void host_low_unmap(void *address, size_t size);
/* 1 if [address, address + size) was handed out by host_low_map or the
guest's mmap, or is one of the fixed ranges */
int host_low_owns(uintptr_t address, size_t size);
/* the guest's mmap/munmap/mprotect/madvise (host_syscall.c), with Linux
flag values and Linux errno results (-errno) */
long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long host_guest_munmap(uint64_t address, uint64_t size);
long host_guest_mprotect(uint64_t address, uint64_t size, int protection);
long host_guest_madvise(uint64_t address, uint64_t size, int advice);

/* the fatal signals' handlers (crashes are logged and end the process
with _exit, not as a crash the system reports) and the memory watch's
write faults */
void host_install_signal_handlers(void);
/* gives the calling thread an alternate signal stack, so that a stack
overflow is still reported; every thread that runs guest code gets one */
void host_signal_stack_install(void);

/* ---------- the guest image (host_loader.c) */

struct host_guest_image
{
	const struct halo_guest_header *header;
	uint32_t base, end;
};

extern struct host_guest_image host_image;

/* maps the image from the ELF file in memory; returns 0 on success */
int host_load_image(const void *elf, size_t size);

/* ---------- entering guest code (host_thread.c) */

/* calls the guest function at address with up to four 32-bit arguments on
this thread, which must run on a stack below 4 GB (one made by
host_native_thread_create, or the main thread after host_main.c switched
it), giving the thread a guest struct pthread first if it has none; returns
the guest's eax */
uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
/* starts a thread running function(argument) with its stack in guest
memory, so that it can call guest code; the stack is freed after it exits.
Returns 0 or an errno value */
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);
/* runs the guest's __guest_start on the calling thread; does not return */
void host_run_guest_main(uint32_t boot) __attribute__((noreturn));
/* the calling thread's guest thread id: a small number the host hands
out (musl keeps thread ids in 30 bits of its mutexes; Darwin's are 64-bit) */
int host_thread_id(void);

/* switches the calling thread's stack pointer to stack_top (16-byte
aligned below it) and calls function(argument) there; never returns
(host_stack.S) */
void host_switch_stack(void *stack_top, void (*function)(uint64_t), uint64_t argument) __attribute__((noreturn));

/* ---------- debugging (host_debug.c) */

void host_debug_thread_started(void);
void host_debug_thread_exited(void);
/* config.toml's debug.sample_seconds: seconds between samples of the guest
threads, as text */
void host_debug_start_sampler(const char *setting);

/* ---------- import table (generated host_import_table.c) */

/* the host function for an import name, or NULL */
void *host_resolve_import(const char *name);

/* ---------- SDL / GL (host_sdl.c, host_gl.c) */

void *host_gl_resolve(const char *name);

/* ---------- paths (host_main.c) */

/* the game data's directory (holding maps/) and the saves', which the
guest also gets as HALO_DATA_ROOT and HALO_SAVE_ROOT */
const char *host_data_root(void);
const char *host_save_root(void);

#endif
