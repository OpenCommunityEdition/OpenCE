/*
HOST_WINDOW.C

The guest's window: the app's (port/macos/app's OpenCEWindow, an AppKit
window with a Metal layer) once it has installed its windows with
halo_host_set_window_backend, else SDL's, as the command-line host has it.
The window imports are main-thread imports (tools/macos_dispatch.py), so
the backend's functions are called on the main thread, where AppKit wants
them; the table itself is set before the guest starts.

Until the Metal renderer draws (the command ring), the app's windows have a
stand-in OpenGL context: it is made, made current and swapped, so the
guest's video starts and its event loop runs, while the OpenGL ES imports
themselves are still refused and nothing is drawn.
*/

#include "host.h"
#include "halo_host.h"

#include <stddef.h>

static struct halo_host_window_backend window_backend;
static int window_backend_set;

void halo_host_set_window_backend(const struct halo_host_window_backend *backend)
{
	window_backend_set = backend != NULL;
	if (backend)
		window_backend = *backend;
}

const struct halo_host_window_backend *host_window_backend(void)
{
	return window_backend_set ? &window_backend : NULL;
}

uint32_t host_window_create(const char *title, int width, int height, int64_t flags)
{
	if (!window_backend_set)
		return host_sdl_create_window(title, width, height, flags);
	return window_backend.create(window_backend.context, title ? title : "", width, height, flags);
}

void host_window_size_in_pixels(uint32_t window, int *width, int *height)
{
	if (!window_backend_set)
	{
		host_sdl_window_size_in_pixels(window, width, height);
		return;
	}
	*width = 0;
	*height = 0;
	window_backend.size_in_pixels(window_backend.context, window, width, height);
}

/* ---------- the OpenGL context */

uint32_t host_window_gl_create_context(uint32_t window)
{
	int width = 0, height = 0;

	if (!window_backend_set)
		return host_sdl_gl_create_context(window);
	/* a window the app has; the stand-in context is named by its window */
	window_backend.size_in_pixels(window_backend.context, window, &width, &height);
	return width > 0 && height > 0 ? window : 0;
}

int host_window_gl_make_current(uint32_t window, uint32_t context)
{
	if (!window_backend_set)
		return host_sdl_gl_make_current(window, context);
	return context == 0 || context == window;
}

int host_window_gl_set_swap_interval(int interval)
{
	if (!window_backend_set)
		return host_sdl_gl_set_swap_interval(interval);
	return 1;
}

int host_window_gl_swap_window(uint32_t window)
{
	if (!window_backend_set)
		return host_sdl_gl_swap_window(window);
	return window != 0;
}
