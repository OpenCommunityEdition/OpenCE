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
/* 1 if [address, address + size) lies in a pool (host_low_map's) or one of
the fixed ranges. Not whether it can be read: those hold reserved and
PROT_NONE pages too (host_arch_read_memory tells that). */
int host_low_owns(uintptr_t address, size_t size);
/* the guest's mmap/munmap/mprotect/madvise (host_syscall.c): only on the
pools, the fixed ranges and the ranges the guest mapped itself; -errno */
long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long host_guest_munmap(uint64_t address, uint64_t size);
long host_guest_mprotect(uint64_t address, uint64_t size, int protection);
long host_guest_madvise(uint64_t address, uint64_t size, int advice);
/* memory_watch_forget (also the guest's): the pages were remapped or
reprotected */
void host_memory_watch_forget(uint32_t address, uint32_t size);

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

/* The image's class and machine for this host's guest: a 64-bit AArch64
ELF file (arm64_32 code) on AArch64, a 32-bit x86-64 one (x32) on x86-64. */
#if defined(__aarch64__)
#define HOST_IMAGE_CLASS 2 /* ELFCLASS64 */
#define HOST_IMAGE_MACHINE 183 /* EM_AARCH64 */
#elif defined(__x86_64__)
#define HOST_IMAGE_CLASS 1 /* ELFCLASS32 */
#define HOST_IMAGE_MACHINE 62 /* EM_X86_64 */
#else
#error "no guest image for this architecture"
#endif

#define HOST_IMAGE_SEGMENTS_MAXIMUM 16

/* a loadable segment, in either ELF class */
struct host_image_segment
{
	uint64_t offset, address, file_size, memory_size;
	uint32_t flags;
};

struct host_image_layout
{
	uint64_t low, high; /* the page-aligned range the segments take */
	struct host_image_segment segments[HOST_IMAGE_SEGMENTS_MAXIMUM];
	uint32_t segment_count;
};

/* checks an ELF file before anything is mapped: its class and machine, a
static executable (no PT_DYNAMIC or PT_INTERP), and its loadable segments
inside the file and inside the image's reserved range
(HALO_GUEST_IMAGE_BASE, HALO_GUEST_IMAGE_RESERVE); fills layout. 0, or -1
with the reason logged. */
int host_image_validate(const void *file, size_t size, int elf_class, int machine, struct host_image_layout *layout);
/* checks the header's import table, its names and count, and its entry
points against the image, whose bytes from base to end are at memory (the
image itself once loaded); 0 with the import count, or -1 with the reason
logged */
int host_image_validate_imports(const uint8_t *memory, uint32_t base, uint32_t end,
	const struct host_image_layout *layout, uint32_t *count);

/* ---------- the processor's registers (host_arch.c) */

/* the program counter, stack pointer, frame pointer and link register of a
signal's ucontext_t; *lr is 0 where calls keep the return address on the
stack (x86-64) */
void host_arch_context(const void *ucontext, uint64_t *pc, uint64_t *sp, uint64_t *fp, uint64_t *lr);
/* the line'th line of the register dump of a ucontext_t into buffer; 0 past
the last line */
int host_arch_register_line(const void *ucontext, int line, char *buffer, size_t size);
/* copies size bytes at address into buffer, never faulting: 0 if any of
them cannot be read (unmapped, PROT_NONE). Async-signal-safe (it may change
errno). */
int host_arch_read_memory(uintptr_t address, void *buffer, size_t size);
/* follows the frame records from fp (both guests, and the host, keep the
caller's frame pointer then the return address, 8 bytes each), collecting
up to maximum return addresses; read copies a record (host_arch_read_memory,
or a test's), and the walk stops where it fails. Returns how many. */
int host_arch_walk_frames(uint64_t fp, int (*read)(uintptr_t address, void *buffer, size_t size),
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
