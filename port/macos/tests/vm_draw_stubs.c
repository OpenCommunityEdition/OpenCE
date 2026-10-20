/*
VM_DRAW_STUBS.C

The game's platform layer as the draw test (vm_draw.c) needs it: a window
of the capture's size (64x64), settings left at their defaults, no
pointer, a memory watch that never moves on, and the log
on the standard output. (No high-res HUD or menu art: vm_hires_stubs.c.)
*/

#include "xgpu.h"
#include "sdl_platform.h"
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_SIDE 64

void memory_watch_protect(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
	return 1;
}

unsigned long memory_watch_serial(void)
{
	return 1;
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	(void)address;
	(void)size;
}

/* (never freed: the test makes few) */
void *platform_contiguous_alloc(unsigned long size, unsigned long alignment, unsigned long physical_address,
	DWORD protect)
{
	unsigned char *block;

	(void)physical_address;
	(void)protect;
	if (alignment < 16)
		alignment = 16;
	if (!(block = calloc(1, size + alignment)))
		return NULL;
	return (void *)(((unsigned long)block + alignment - 1) & ~(alignment - 1));
}

void platform_contiguous_free(void *address)
{
	(void)address;
}

BOOL platform_is_contiguous(const void *address)
{
	(void)address;
	return FALSE;
}

int config_boolean(const char *name)
{
	(void)name;
	return 0;
}

long config_integer(const char *name)
{
	(void)name;
	return 0;
}

const char *config_string(const char *name)
{
	(void)name;
	return "";
}

unsigned long config_changes(void)
{
	return 0;
}

int halo_interpolation_enabled(void)
{
	return 0;
}

BOOL platform_video_initialize(unsigned long width, unsigned long height)
{
	(void)width;
	(void)height;
	return TRUE;
}

void platform_video_drawable_size(int *width, int *height)
{
	*width = WINDOW_SIDE;
	*height = WINDOW_SIDE;
}

void platform_video_window_size(int *width, int *height)
{
	*width = WINDOW_SIDE;
	*height = WINDOW_SIDE;
}

void platform_pump_events(void)
{
}

void platform_menus_set_active(BOOL active)
{
	(void)active;
}

BOOL platform_ui_pointer_read(struct platform_ui_pointer *pointer)
{
	(void)pointer;
	return FALSE;
}

void platform_ui_pointer_set_active(BOOL active)
{
	(void)active;
}

void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	printf("log: ");
	vprintf(format, arguments);
	printf("\n");
	va_end(arguments);
}
