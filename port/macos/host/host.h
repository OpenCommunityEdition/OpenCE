/*
HOST.H — shared definitions of the macOS host (port/macos/host).

The macOS port runs the Android guest image (the game, the platform layer
shared with Linux, and a small musl runtime, compiled as ILP32 AArch64
code; see port/android/README.md) inside a Hypervisor.framework virtual
machine. macOS refuses to map memory below 4 GB in a normal process (the
kernel kills binaries whose __PAGEZERO is smaller, and denies sub-4 GB
mappings), so the guest runs at EL1 in a VM whose stage-2 tables we own:
guest virtual addresses are identity-mapped to guest physical addresses
below 4 GB, backed by ordinary host memory wherever the guest may touch.

The guest calls the host through doorbell stubs (tools/macos_imports.py):
each import stores to its own address in the doorbell page, which stage 2
leaves unwritable, so the store exits the vCPU with a data abort whose IPA
names the import; the host services the call and advances the guest PC
past the store.

Every function declared here (the doorbells of host_imports.list, plus
this port's own) is either static to one file or implemented in port/
macos/host; the import dispatch (tools/macos_import_dispatch.py) reads
this header to type each wrapper.
*/

#ifndef __HALO_MACOS_HOST_H
#define __HALO_MACOS_HOST_H

#include "halo_android_abi.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

/* ---------- guest memory map (guest virtual == guest physical) */

#define GUEST_WINDOW_BASE HALO_GUEST_WINDOW_BASE
#define GUEST_WINDOW_SIZE HALO_GUEST_WINDOW_SIZE

#define GUEST_IMAGE_BASE HALO_GUEST_IMAGE_BASE
#define GUEST_IMAGE_MAXIMUM (16u * 1024 * 1024)

/* pools: guest stacks and the guest's malloc mappings */
#define GUEST_POOLS_BASE 0x90000000u
#define GUEST_POOLS_SIZE 0x30000000u /* 768 MB */

/* the doorbell page (tools/macos_imports.py; one word per import) */
#define GUEST_DOORBELL_BASE 0x10000000u
#define GUEST_DOORBELL_SIZE 0x10000u

/* everything the guest can address */
#define GUEST_LOW_LIMIT 0x100000000ull

/* the hypervisor maps at this machine's page size (16 KiB on arm64) */
#define GUEST_PAGE_SIZE 0x10000u

/* ---------- logging and termination (host_main.c) */

enum
{
	HOST_LOG_INFO,
	HOST_LOG_WARN,
	HOST_LOG_ERROR,
	HOST_LOG_FATAL,
};

void host_logf(int priority, const char *format, ...);
void host_log(int priority, const char *text);
void host_fatal(const char *format, ...) __attribute__((noreturn));
void host_abort(const char *reason) __attribute__((noreturn));
void host_exit(int code) __attribute__((noreturn));
int host_errno(void);
void host_debug_thread_started(void);
void host_debug_thread_exited(void);
void host_debug_start_sampler(const char *seconds);

/* ---------- the doorbells (in the order of port/android/host_imports.list
plus this port's; guest_host.h comments what each does) */

/* process */
long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f);
void host_log(int priority, const char *text);
void host_abort(const char *reason);
void host_exit(int code);
int host_errno(void);

/* threads: the guest's thread pointer, and starting guest threads */
unsigned int host_get_tp(void);
void host_set_tp(unsigned int thread);
int host_thread_create(unsigned int thread, unsigned int stack_size);

/* memory write tracking */
void host_memory_watch_initialize(void);
void host_memory_watch_protect(unsigned int address, unsigned int size);
unsigned int host_memory_watch_generation(unsigned int address, unsigned int size);
unsigned int host_memory_watch_serial(void);
void host_memory_watch_prepare_write(unsigned int address, unsigned int size);
void host_memory_watch_forget(unsigned int address, unsigned int size);
/* the run loop asks when a data abort lands in the window; 1 when the
store may re-execute (a watched page's first write) */
int host_memory_watch_fault(uint64_t address);

/* SDL (host_sdl.c): window, context, gamepad and audio stream objects
are small handles on this side */
int host_sdl_init(unsigned int flags);
int host_sdl_set_hint(const char *name, const char *value);
void host_sdl_get_error(char *buffer, unsigned int size);
long long host_sdl_ticks(void);
long long host_sdl_thread_id(void);
unsigned int host_sdl_create_window(const char *title, int width, int height, long long flags);
void host_sdl_window_size_in_pixels(unsigned int window, int *width, int *height);
int host_sdl_set_relative_mouse(unsigned int window, int enabled);
int host_sdl_gl_set_attribute(int attribute, int value);
unsigned int host_sdl_gl_create_context(unsigned int window);
int host_sdl_gl_make_current(unsigned int window, unsigned int context);
int host_sdl_gl_set_swap_interval(int interval);
int host_sdl_gl_swap_window(unsigned int window);
int host_sdl_poll_event(void *event);
int host_sdl_set_clipboard_text(const char *text);
void host_sdl_get_clipboard_text(char *buffer, unsigned int size);
void host_sdl_scancode_name(int scancode, char *buffer, unsigned int size);
int host_sdl_scancode_from_name(const char *name);
int host_sdl_show_toast(const char *message, int duration, int gravity, int x, int y);
int host_sdl_show_simple_message_box(unsigned int flags, const char *title, const char *message);
int host_sdl_get_gamepads(unsigned int *ids, int capacity);
unsigned int host_sdl_open_gamepad(unsigned int id);
unsigned int host_sdl_gamepad_from_id(unsigned int id);
int host_sdl_gamepad_axis(unsigned int gamepad, int axis);
int host_sdl_gamepad_button(unsigned int gamepad, int button);
int host_sdl_gamepad_type(unsigned int gamepad);
int host_sdl_rumble_gamepad(unsigned int gamepad, unsigned int low, unsigned int high,
	unsigned int milliseconds);
unsigned int host_sdl_open_audio_stream(unsigned int device, const void *spec,
	unsigned int callback, unsigned int userdata);
int host_sdl_put_audio_stream_data(unsigned int stream, const void *data, int length);
int host_sdl_resume_audio_stream_device(unsigned int stream);

/* paths of the game data and saves (host_main.c) */
void host_android_path(int which, char *buffer, unsigned int size);

/* GL helpers the guest calls (host_gl.c) */
void host_gl_get_string(unsigned int name, int index, char *buffer, unsigned int size);
int host_gl_has_extension(const char *name);
unsigned int host_gl_read_buffer_word(unsigned int buffer, unsigned int offset);
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data);
void host_gl_fence_frame(unsigned int slot);
void host_gl_wait_frame(unsigned int slot);

/* ---------- the VM and its mirror (host_vm.c) */

void *host_low_map(size_t size);
void host_low_unmap(void *address, size_t size);
int host_low_owns(uintptr_t address, size_t size);
/* installs a page in the mirror (the loader and the pools use it) */
void host_mirror_install(uint32_t guest, void *host);
/* the host address of a guest address, or NULL */
uint8_t *host_guest_pointer(unsigned int guest);

int host_vm_start(void);
int host_vm_map_window(void);

/* vCPUs */
int host_vcpu_attach(void);
void host_vcpu_detach(void);
void host_vcpu_set_stack(unsigned int stack_top);
void host_vcpu_run_forever(void) __attribute__((noreturn));

/* ---------- the guest image (host_loader.c) */

struct host_guest_image
{
	const struct halo_guest_header *header;
	unsigned int base;
	unsigned int end;
};

extern struct host_guest_image host_image;
uint8_t *host_image_host_memory(void);
int host_load_image(const void *file, size_t size);
void *host_resolve_import(const char *name);

/* ---------- calls into the guest (host_vm.c, host_thread.c) */

unsigned int host_call_guest(unsigned int function, unsigned int a, unsigned int b,
	unsigned int c, unsigned int d);
void host_run_guest_main(unsigned int boot);
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);

/* ---------- GL resolution (host_gl.c) */

void *host_gl_resolve(const char *name);

/* ---------- the import dispatch's table (generated; host_vm.c reads it) */

struct import_entry
{
	const char *name;
	void (*function)(void);
};

/* the doorbell dispatch's view of the guest registers (host_vm.c owns
them; the generated wrappers reach through these) */
struct guest_registers
{
	uint32_t x[31];
	uint32_t sp;
	uint64_t pc;
};

struct guest_registers *host_vcpu_dispatch_registers(void);

extern const struct import_entry host_import_table[];
extern const unsigned host_import_count;
/* the GL trampoline: calls the resolved entry with the current
registers (tools/macos_gl_dispatch.py makes it) */
void hostgl_trampoline(void);

/* ---------- the guest's thread pointer of this host thread (host_thread.c;
the doorbell wrappers host_get_tp/host_set_tp forward here) */

extern __thread unsigned int guest_tp;

#endif /* __HALO_MACOS_HOST_H */
