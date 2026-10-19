/*
HOST_WINDOW_TEST.C

A native test of port/macos/host/host_window.c (tools/test_macos_vm.py):
without windows of the app's, the window imports are SDL's (stand-ins
here); once the app installs its own, they are the app's, which keep the
size each window has now. Their OpenGL context is a stand-in until the
Metal renderer: it is made, made current and swapped, and draws nothing.
*/

#include "host.h"
#include "halo_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* ---------- SDL's, as host_sdl.c has them */

static int sdl_windows;

uint32_t host_sdl_create_window(const char *title, int width, int height, int64_t flags)
{
	(void)title;
	(void)width;
	(void)height;
	(void)flags;
	return (uint32_t)++sdl_windows;
}

void host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height)
{
	*width = window == 1 ? 640 : 0;
	*height = window == 1 ? 480 : 0;
}

static int sdl_contexts, sdl_swaps;

uint32_t host_sdl_gl_create_context(uint32_t window)
{
	(void)window;
	return (uint32_t)++sdl_contexts;
}

int host_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	return window != 0 && context != 0;
}

int host_sdl_gl_set_swap_interval(int interval)
{
	return interval == 0;
}

int host_sdl_gl_swap_window(uint32_t window)
{
	(void)window;
	sdl_swaps++;
	return 1;
}

/* ---------- the app's: a window of twice its points (a Retina screen) */

struct app_windows
{
	int count;
	int width[4];
	int height[4];
	char title[32];
	int64_t flags;
};

static uint32_t app_create(void *context, const char *title, int width, int height, int64_t flags)
{
	struct app_windows *windows = context;

	if (windows->count >= 4)
		return 0;
	snprintf(windows->title, sizeof(windows->title), "%s", title);
	windows->flags = flags;
	windows->width[windows->count] = width * 2;
	windows->height[windows->count] = height * 2;
	return (uint32_t)++windows->count;
}

static void app_size_in_pixels(void *context, uint32_t window, int *width, int *height)
{
	struct app_windows *windows = context;

	*width = 0;
	*height = 0;
	if (window >= 1 && window <= (uint32_t)windows->count)
	{
		*width = windows->width[window - 1];
		*height = windows->height[window - 1];
	}
}

int main(void)
{
	struct app_windows windows;
	struct halo_host_window_backend backend;
	int width, height;
	uint32_t window;

	/* SDL's, while the app has given none */
	window = host_window_create("Halo", 640, 480, 0);
	CHECK(window == 1 && sdl_windows == 1);
	host_window_size_in_pixels(window, &width, &height);
	CHECK(width == 640 && height == 480);
	/* and SDL's OpenGL context */
	CHECK(host_window_gl_create_context(window) == 1 && sdl_contexts == 1);
	CHECK(host_window_gl_make_current(window, 1) == 1);
	CHECK(host_window_gl_set_swap_interval(1) == 0);
	CHECK(host_window_gl_swap_window(window) == 1 && sdl_swaps == 1);

	memset(&windows, 0, sizeof(windows));
	memset(&backend, 0, sizeof(backend));
	backend.context = &windows;
	backend.create = app_create;
	backend.size_in_pixels = app_size_in_pixels;
	halo_host_set_window_backend(&backend);
	/* (copied: the app's table may go) */
	memset(&backend, 0, sizeof(backend));

	window = host_window_create("Halo", 640, 480, 0x20000001);
	CHECK(window == 1 && sdl_windows == 1);
	CHECK(strcmp(windows.title, "Halo") == 0 && windows.flags == 0x20000001);
	host_window_size_in_pixels(window, &width, &height);
	CHECK(width == 1280 && height == 960);
	/* a resize is what the next call reports */
	windows.width[0] = 2000;
	windows.height[0] = 1100;
	host_window_size_in_pixels(window, &width, &height);
	CHECK(width == 2000 && height == 1100);
	/* a window the app does not have is 0 by 0 */
	width = height = -1;
	host_window_size_in_pixels(7, &width, &height);
	CHECK(width == 0 && height == 0);

	/* the app's windows have a stand-in context, which SDL is not asked for */
	CHECK(host_window_gl_create_context(window) != 0 && sdl_contexts == 1);
	CHECK(host_window_gl_make_current(window, host_window_gl_create_context(window)) == 1);
	CHECK(host_window_gl_set_swap_interval(1) == 1);
	CHECK(host_window_gl_swap_window(window) == 1 && sdl_swaps == 1);
	/* (but a window the app does not have has none) */
	CHECK(host_window_gl_create_context(7) == 0);

	/* and the app may take its windows back */
	halo_host_set_window_backend(NULL);
	CHECK(host_window_create("Halo", 640, 480, 0) == 2 && sdl_windows == 2);
	printf("ok\n");
	return 0;
}
