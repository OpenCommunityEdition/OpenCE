/*
HALO_HOST.H

The macOS host's entry point (port/macos/host/host_main.c), which
libhalo_host.a exports to the command-line host and the app.
*/

#ifndef HALO_HOST_H
#define HALO_HOST_H

#include <stddef.h>
#include <stdint.h>

/* halo_guest.elf beside the executable, or in ../share/opence */
void halo_host_default_image_path(char *path, size_t size);

/* loads the guest image at image_path, creates the VM and runs the guest's
main with argc and argv (argv[0] is the guest's program name) on the
calling thread. It returns only if the VM cannot be created, with 1; a
guest image that cannot be read or loaded ends the process (71) */
int halo_host_run(const char *image_path, int argc, char **argv);

/* from now on, the imports that need the main thread (windows, events,
message boxes) are carried out there, by its run loop, for the guest's
threads: called before halo_host_run is, on another thread, by a host whose
main thread runs a run loop (the app's NSApplication, or dispatch_main) */
void halo_host_use_main_queue(void);
/* fails the main thread's queued requests, and any made later (as the app
terminates, when its run loop stops) */
void halo_host_shutdown(void);

/* the app's windows, in place of SDL's (host_window.c): its functions are
called on the main thread. create gives the window's handle, nonzero, or 0;
size_in_pixels gives a window's drawable size now (0 by 0 for a handle
create did not give) */
struct halo_host_window_backend
{
	void *context;
	uint32_t (*create)(void *context, const char *title, int width, int height, int64_t flags);
	void (*size_in_pixels)(void *context, uint32_t window, int *width, int *height);
	/* the mouse captured (hidden, held in the window, moving relatively)
	or let go; 1 if done. host_input.c lets it go while the window has
	not the focus */
	int (*set_relative_mouse)(void *context, uint32_t window, int enabled);
};

/* the windows the guest makes from now on are backend's (copied), or SDL's
again with NULL: called before the guest makes its window */
void halo_host_set_window_backend(const struct halo_host_window_backend *backend);

/* the app's windows' keyboard and mouse, as AppKit gives them (host_input.c),
for the guest's next polls; on the main thread. key_code is an NSEvent's
keyCode; flags its modifierFlags, for a modifier key's flagsChanged; x and
y are in the view's points from its top left; button numbers are AppKit's
(0 left, 1 right, 2 middle); precise is a trackpad's scrolling */
void halo_host_post_key(uint32_t window, uint16_t key_code, int down, int repeat);
void halo_host_post_modifiers(uint32_t window, uint16_t key_code, uint64_t flags);
void halo_host_post_mouse_motion(uint32_t window, float x, float y, float dx, float dy);
void halo_host_post_mouse_button(uint32_t window, int number, int down, float x, float y);
void halo_host_post_wheel(uint32_t window, float dx, float dy, int precise);
void halo_host_post_focus(uint32_t window, int has_focus);

/* the app's sound, in place of SDL's (host_audio.c), for the game's
playback stream: its functions are called on the guest's threads. open
makes a stream of `channels` float channels at `rate` Hz, named by
`stream`, whose frames halo_host_audio_render gives; resume plays it; 1 if
done. close stops it: no render for it runs once close returns */
struct halo_host_audio_backend
{
	void *context;
	int (*open)(void *context, uint32_t stream, int channels, int rate);
	int (*resume)(void *context, uint32_t stream);
	void (*close)(void *context, uint32_t stream);
};

/* the streams the guest opens from now on are backend's (copied), or SDL's
again with NULL: called before the guest opens its sound */
void halo_host_set_audio_backend(const struct halo_host_audio_backend *backend);
/* `frames` frames of stream `stream`, a channel each, into channels[0] to
channels[channel_count - 1] (silence for what the stream lacks, and for a
channel it has not); on Core Audio's real-time thread, which it never
blocks */
void halo_host_audio_render(uint32_t stream, float *const *channels, int channel_count, int frames);

#endif
