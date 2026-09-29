/*
HOST.H

Internals of the macOS port's host executable. See port/macos/README.md for
the design and port/android/include/halo_android_abi.h for the guest
contract, which the macOS port shares with the Android port.

The host is an x86-64 Mach-O executable that runs under Rosetta 2 on Apple
silicon (and natively on Intel Macs). It is linked with a small __PAGEZERO so
the low 4 GB of its address space is free for the guest: the game compiled
as x32 code (x86-64 instructions, 32-bit pointers) into a static ELF image.
*/

#ifndef __HALO_MACOS_HOST_H
#define __HALO_MACOS_HOST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "halo_android_abi.h"

/* ---------- logging (stderr and <data>/host.txt) */

void host_logf(int priority, const char *format, ...) __attribute__((format(printf, 2, 3)));
#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6
void host_log(int priority, const char *text);
void host_exit(int code) __attribute__((noreturn));
int host_errno(void);

/* logs, shows the message to the player and terminates */
void host_fatal(const char *format, ...) __attribute__((format(printf, 1, 2), noreturn));

/* the folder holding the executable (and maps/, config.toml, saves) */
extern char host_data_root[1024];
/* the path the guest sees as /proc/self/exe */
extern char host_executable_path[1024];

/* ---------- errno and flag translation (host_syscall.c) */

/* the Linux errno value for a macOS one */
int host_linux_errno(int value);

/* ---------- guest memory (host_memory.c)

All memory the guest can address lies below 4 GB. The host reserves one
region there at start-up, before anything else can map into it, and hands
out the Xbox window, the image's range and pages for everything else (the
guest's malloc arenas, thread stacks, anonymous mappings) from it. */

int host_memory_reserve(void);
int host_memory_initialize(uint32_t image_base, uint32_t image_size);
void *host_low_map(size_t size, int protection);
void host_low_unmap(void *address, size_t size);
int host_low_owns(uintptr_t address, size_t size);
long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long host_guest_munmap(uint64_t address, uint64_t size);
long host_guest_mprotect(uint64_t address, uint64_t size, int protection);
void host_install_signal_handlers(void);

/* ---------- the guest image (host_loader.c) */

struct host_guest_image
{
	const struct halo_guest_header *header;
	uint32_t base, end;
};

extern struct host_guest_image host_image;

int host_load_image(const void *elf, size_t size);

/* ---------- entering guest code (host_thread.c) */

uint32_t host_get_tp(void);
void host_set_tp(uint32_t thread);
uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);
/* runs the guest's __guest_start on the calling thread (the process's main
thread, which Cocoa needs for the window and events) on a stack in guest
memory; does not return */
void host_run_guest_main(uint32_t boot) __attribute__((noreturn));

void host_debug_thread_started(void);
void host_debug_thread_exited(void);

/* ---------- import table (generated host_import_table.c) */

void *host_resolve_import(const char *name);

/* ---------- SDL / GL (host_sdl.c, host_gl.c) */

void *host_gl_resolve(const char *name);

#endif
