/*
HOST.H

Internals of the host that runs the ILP32 guest image: the Android port's
library (libmain.so), and the Linux executable of port/linux/host, which
shares every file here but host_main.c and host_touch.c. See
port/android/README.md for the overall design and
port/android/include/halo_android_abi.h for the guest contract.
*/

#ifndef __HALO_ANDROID_HOST_H
#define __HALO_ANDROID_HOST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "halo_android_abi.h"

/* ---------- logging and termination (host_common.c)

Each host's main sends the lines somewhere (host_log_set_sink): logcat (tag
"halo") on Android, the standard error stream on Linux. */

/* Android's log priorities (android/log.h) */
#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6
#define HOST_LOG_FATAL 7

/* writes one line (without its newline); called from signal handlers too, so
it must not take locks or use stdio */
typedef void (*host_log_sink)(int priority, const char *line);
void host_log_set_sink(host_log_sink sink);

void host_logf(int priority, const char *format, ...) __attribute__((format(printf, 2, 3)));
/* (also the guest's, host_imports.list) */
void host_log(int priority, const char *text);
/* guest services also used inside the host (host_main.c) */
void host_exit(int code) __attribute__((noreturn));
int host_errno(void);

/* logs, shows the message to the player and terminates */
void host_fatal(const char *format, ...) __attribute__((format(printf, 1, 2), noreturn));

/* ---------- starting the guest (host_common.c) */

/* the guest's environment, which each host's main fills */
#define HOST_ENVIRONMENT_MAXIMUM 128

struct host_environment
{
	char *entries[HOST_ENVIRONMENT_MAXIMUM];
	int count;
};

/* sets name=value, replacing an entry of that name; an entry that does not
fit is logged and dropped */
void host_environment_set(struct host_environment *environment, const char *name, const char *value);
/* the same for an entry "name=value" (one of this process's environ) */
void host_environment_add(struct host_environment *environment, const char *entry);
/* POSIX TZ (the value) for the current local offset: the guest's musl has
no zone database */
void host_time_zone(char *buffer, size_t size);
/* copies argv (the program's name) and the environment into guest memory,
for __guest_start */
uint32_t host_make_boot(const struct host_environment *environment, const char *program);

/* the debugging settings of config.toml at path that the host reads itself:
debug.sample_seconds (the sampler, host_debug.c; or HALO_SAMPLE in the
host's environment) and debug.memory_watch (false: page hashes,
host_memory_watch_use_hashes). hashes_reason, if not NULL, says why the host
must use page hashes whatever the setting. */
void host_apply_debug_settings(const char *path, const char *hashes_reason);

/* ---------- guest memory (host_memory.c)

All memory the guest can address lies below 4 GB. The host reserves the
Xbox window and the image's range at start-up, and hands out pages for
everything else (the guest's malloc arenas, thread stacks, anonymous
mappings) from pools of address space it reserves below 4 GB on demand. */

/* reserves the fixed ranges (the Xbox window and the image's) as early as
the process allows: from JNI_OnLoad, before the Java side of the game's
process allocates much (host_main.c); a failure is reported again by
host_memory_initialize */
void host_memory_reserve_early(void);
/* takes the fixed ranges for the image; returns 0 on success */
int host_memory_initialize(uint32_t image_base, uint32_t image_size);
/* 1 if the fixed ranges could not be reserved (something else holds them) */
int host_memory_fixed_unavailable(void);
/* lists the mappings below 4 GB in logcat and, if file is not NULL, in it */
void host_memory_report_low_mappings(FILE *file);
/* Makes the write tracking compare page contents instead of catching page
faults (host_watch_hash.h), for when the app cannot receive its own
SIGSEGV (ARM translation). Call before the guest starts. */
void host_memory_watch_use_hashes(void);
/* page-granular allocations below 4 GB; NULL on failure */
void *host_low_map(size_t size, int protection);
void host_low_unmap(void *address, size_t size);
/* 1 if [address, address + size) was handed out by host_low_map or is one of
the fixed ranges; takes no lock, so signal handlers may call it */
int host_low_owns(uintptr_t address, size_t size);
/* the guest's mmap/munmap/mprotect/madvise/mremap (host_syscall.c) */
long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long host_guest_munmap(uint64_t address, uint64_t size);
long host_guest_mprotect(uint64_t address, uint64_t size, int protection);

/* the SIGSEGV, SIGBUS and SIGILL handlers: the write tracking's faults, and
a report of any other (host_memory.c); the main thread calls it first */
void host_install_signal_handlers(void);
/* gives the calling thread a stack of its own for those handlers, so that a
stack overflow can be reported (host_thread.c does it for its threads); 0
on success */
int host_signal_stack_install(void);
void host_signal_stack_remove(void);

/* ---------- the guest image (host_loader.c) */

struct host_guest_image
{
	const struct halo_guest_header *header;
	uint32_t base, end;
};

extern struct host_guest_image host_image;

/* maps the image from the ELF file in memory; returns 0 on success */
int host_load_image(const void *elf, size_t size);

/* ---------- the processor's registers (host_arch.c) */

/* the program counter, stack pointer, frame pointer and link register of a
signal's ucontext_t; *lr is 0 where calls keep the return address on the
stack (x86-64) */
void host_arch_context(const void *ucontext, uint64_t *pc, uint64_t *sp, uint64_t *fp, uint64_t *lr);
/* the line'th line of the register dump of a ucontext_t into buffer; 0 past
the last line */
int host_arch_register_line(const void *ucontext, int line, char *buffer, size_t size);
/* follows the guest's frame records from fp (both guests keep the caller's
frame pointer then the return address, 8 bytes each), collecting up to
maximum return addresses; readable says whether 16 bytes at an address may
be read. Returns how many. */
int host_arch_walk_frames(uint64_t fp, int (*readable)(uintptr_t address, size_t size),
	uint64_t *returns, int maximum);

/* ---------- entering guest code (host_thread.c) */

/* calls the guest function at address with up to four 32-bit arguments on
this thread, which must have been made by host_native_thread_create (giving
the thread a guest struct pthread first if it has none); returns the
guest's w0 */
uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
/* starts a thread running function(argument) with its stack in guest
memory, so that it can call guest code; the stack is freed after it exits.
Returns 0 or an errno value */
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);
/* runs the guest's __guest_start on the calling thread (one made by
host_native_thread_create); does not return */
void host_run_guest_main(uint32_t boot) __attribute__((noreturn));

/* ---------- debugging (host_debug.c) */

void host_debug_thread_started(void);
void host_debug_thread_exited(void);
/* config.toml's debug.sample_seconds: seconds between samples of the guest
threads, as text */
void host_debug_start_sampler(const char *setting);

/* ---------- import table (generated by tools/android_imports.py into the
build's host/host_import_table.c) */

/* the host function for an import name, or NULL */
void *host_resolve_import(const char *name);

/* ---------- SDL / GL (host_sdl.c, host_gl.c) */

/* SDL objects are 64-bit pointers, which the guest cannot hold: it gets
small handles into a table instead (host_sdl.c; port/linux/host's
host_desktop.c uses it too) */
enum host_sdl_handle_type
{
	_handle_free,
	_handle_window,
	_handle_context,
	_handle_gamepad,
	_handle_audio,
	_handle_renderer,
};

/* the object's handle (the same one again for an object that has one), or
0 for NULL or a full table */
uint32_t host_sdl_handle_new(int type, void *object);
/* the object of a handle of that type, or NULL */
void *host_sdl_handle_get(uint32_t handle, int type);
void host_sdl_handle_free(uint32_t handle);

void *host_gl_resolve(const char *name);

#endif
