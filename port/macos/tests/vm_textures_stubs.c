/*
VM_TEXTURES_STUBS.C

The game's platform layer as the texture test (vm_textures.c) needs it: a
memory watch whose generation and serial the test moves on itself, the
contiguous window's part it maps, settings left at their defaults, and the
log on the standard output; the device's render pass, which the test
ends itself. (The render state setters are vm_nv2a_stubs.c's.)
*/

#include "platform.h"

#include <stdarg.h>
#include <stdio.h>

unsigned long test_watch_generation = 1;
unsigned long test_watch_serial = 1;

void memory_watch_protect(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	(void)address;
	(void)size;
	return test_watch_generation;
}

unsigned long memory_watch_serial(void)
{
	return test_watch_serial;
}

BOOL platform_is_contiguous(const void *address)
{
	unsigned long at = (unsigned long)address;

	return at >= PLATFORM_CONTIGUOUS_BASE && at < PLATFORM_CONTIGUOUS_BASE + 0x1000000UL;
}

void xgpu_metal_pass_end(void)
{
}

int config_boolean(const char *name)
{
	(void)name;
	return 0;
}

const char *config_string(const char *name)
{
	(void)name;
	return "";
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
