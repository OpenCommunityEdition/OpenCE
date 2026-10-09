/*
GUEST_HOST.H

The host services the guest imports (the guest's view; the host defines
them in port/android/host). Every name here must also be listed in
port/android/host_imports.list, which generates the import stubs.

Parameter types follow the rules in halo_android_abi.h: 32-bit values are
int or unsigned int, 64-bit values long long, and pointers are passed as
they are.
*/

#ifndef __GUEST_HOST_H
#define __GUEST_HOST_H

/* ---------- process */

/* performs a Linux system call on the guest's behalf, converting the
structures whose layout differs; returns the raw result (-errno on failure) */
long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f);

/* Android log priorities (android/log.h) */
void host_log(int priority, const char *text);
void host_abort(const char *reason) __attribute__((noreturn));
void host_exit(int code) __attribute__((noreturn));
/* the host's errno on this thread, after a call to a host function */
int host_errno(void);

/* ---------- threads

The guest's thread pointer (its struct pthread) is kept by the host for
each thread. */

unsigned int host_get_tp(void);
void host_set_tp(unsigned int thread);
/* starts a host thread with a stack in guest memory that calls the image's
__guest_thread_start(thread); returns 0 or an errno value */
int host_thread_create(unsigned int thread, unsigned int stack_size);

/* ---------- memory write tracking (port/linux/src/memory_watch.c) */

void host_memory_watch_initialize(void);
void host_memory_watch_protect(unsigned int address, unsigned int size);
unsigned int host_memory_watch_generation(unsigned int address, unsigned int size);
unsigned int host_memory_watch_serial(void);
void host_memory_watch_prepare_write(unsigned int address, unsigned int size);
void host_memory_watch_forget(unsigned int address, unsigned int size);

/* ---------- SDL (guest/runtime/guest_sdl.c)

Window, context, gamepad and audio stream objects are small integer
handles on this side. */

int host_sdl_init(unsigned int flags);
/* halo_touch_state from halo_touch.h; flat Android player-one snapshot. */
void host_touch_read(void *buffer);
void host_touch_menu(int active);
void host_touch_pointer_read(void *buffer);
int host_sdl_set_hint(const char *name, const char *value);
void host_sdl_get_error(char *buffer, unsigned int size);
long long host_sdl_ticks(void);
long long host_sdl_thread_id(void);
unsigned int host_sdl_create_window(const char *title, int width, int height, long long flags);
void host_sdl_window_size(unsigned int window, int *width, int *height);
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
int host_sdl_show_toast(const char *message, int duration, int gravity, int x, int y);
int host_sdl_show_simple_message_box(unsigned int flags, const char *title, const char *message);
int host_sdl_get_gamepads(unsigned int *ids, int capacity);
unsigned int host_sdl_open_gamepad(unsigned int id);
unsigned int host_sdl_gamepad_from_id(unsigned int id);
int host_sdl_gamepad_axis(unsigned int gamepad, int axis);
int host_sdl_gamepad_button(unsigned int gamepad, int button);
int host_sdl_gamepad_type(unsigned int gamepad);
/* the gamepad's USB vendor id (Meta's headsets' own controllers: 0x2833) */
int host_sdl_gamepad_vendor(unsigned int gamepad);
int host_sdl_rumble_gamepad(unsigned int gamepad, unsigned int low, unsigned int high, unsigned int milliseconds);
/* callback: void (*)(void *userdata, unsigned int stream, int additional, int total),
called on the audio thread */
unsigned int host_sdl_open_audio_stream(unsigned int device, const void *spec, unsigned int callback, unsigned int userdata);
int host_sdl_put_audio_stream_data(unsigned int stream, const void *data, int length);
int host_sdl_resume_audio_stream_device(unsigned int stream);

/* ---------- OpenGL ES */

/* copies glGetString(name) (or glGetStringi when index >= 0) */
void host_gl_get_string(unsigned int name, int index, char *buffer, unsigned int size);
/* nonzero if the context supports the named extension */
int host_gl_has_extension(const char *name);
/* a 32-bit word of a GL buffer object, waiting for the GPU */
unsigned int host_gl_read_buffer_word(unsigned int buffer, unsigned int offset);
/* size bytes of a buffer object from offset, waiting for the GPU once */
void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data);
/* unsynchronized write into the buffer bound to target */
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data);
/* storage for the buffer bound to target, mapped once for good; 1 on success */
int host_gl_buffer_persist(unsigned int target, unsigned int size);
/* a write into such a buffer: a copy, no GL call; 0 if it is not one */
int host_gl_buffer_write_persistent(unsigned int buffer, unsigned int offset, unsigned int size, const void *data);
/* fences the GPU work queued so far as that of ring slot `slot`; waits for
the GPU to finish the work last fenced for a slot */
void host_gl_fence_frame(unsigned int slot);
void host_gl_wait_frame(unsigned int slot);
/* 1 once the GPU has passed the work last fenced for the slot, without waiting */
int host_gl_frame_done(unsigned int slot);

/* ---------- Android */

/* the storage directories the port uses, copied into buffer */
void host_android_path(int which, char *buffer, unsigned int size);

/* ---------- OpenXR (HALO_VR builds: host_imports_vr.list, host/host_xr.c)

The structures are halo_android_abi.h's. */

struct halo_xr_info;
struct halo_xr_frame;
struct halo_xr_layers;
/* creates the session for the current GL context, with a quad swapchain
of the size given; 0 on success */
int host_xr_init(struct halo_xr_info *info, unsigned int quad_width, unsigned int quad_height);
/* waits for and begins the runtime's next frame; 0 when none was begun
(the session not running: it has polled and slept briefly) */
int host_xr_begin_frame(struct halo_xr_frame *frame);
/* the acquired image's index in info->images[which], or -1 */
int host_xr_acquire(unsigned int which);
void host_xr_release(unsigned int which);
/* ends the frame begun, showing these layers (NULL: none) */
void host_xr_end_frame(const struct halo_xr_layers *layers);
/* the head's heading and position next frame become the origin */
void host_xr_recenter(void);
/* asks for the display refresh rate nearest at or below hertz; returns it,
or 0 when the runtime cannot change it */
float host_xr_set_refresh_rate(float hertz);
void host_xr_haptic(unsigned int hand, float amplitude, float seconds);
/* remakes both eye swapchains at this size (within the runtime's maximum),
none of their images acquired, and describes them in info again; 0 on
success, else the old ones stay */
int host_xr_resize_eyes(struct halo_xr_info *info, unsigned int width, unsigned int height);

#endif
