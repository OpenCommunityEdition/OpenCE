/*
POSIX_TRACE_MARKER.C

The kernel's trace_marker kept from the GPU driver.

Mesa built with gpuvis (SteamOS's) writes a marker into the kernel's trace
buffer for each traced driver function while tracing is on, and SteamOS
keeps tracing on for its GPU performance captures (gpu-trace.service). On the
Steam Frame that is some 480,000 writes a second: the driver's thread spends
its time in them, and the game ran at about 50 frames a second instead of
the headset's 72. The Steam Deck runs the same service (not measured there). A
trace_marker that cannot be opened turns the markers off (gpuvis then skips
them).

This executable defines open() and openat() (and their 64-bit names), which
the link exports (tools/linux_build.py, and tools/linux_arm64_build.py for
the arm64 build's host), so the driver's calls come here:
they refuse trace_marker and pass everything else to the C library's.
HALO_GPU_TRACE_MARKERS=1 lets the driver open it, to capture with gpuvis.
Built with the host ABI.
*/

/* open and open64 as two functions, not open as glibc's open64 */
#undef _FILE_OFFSET_BITS
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef int (*open_function)(const char *path, int flags, ...);
typedef int (*openat_function)(int directory, const char *path, int flags, ...);

static int refused(const char *path)
{
	static int allowed = -1;

	if (!path || !strstr(path, "trace_marker"))
		return 0;
	if (allowed < 0)
	{
		const char *setting = getenv("HALO_GPU_TRACE_MARKERS");

		allowed = setting && *setting && strcmp(setting, "0") != 0;
	}
	if (allowed)
		return 0;
	errno = EACCES;
	return 1;
}

/* the mode, which follows only with O_CREAT or O_TMPFILE (glibc's
__OPEN_NEEDS_MODE: O_TMPFILE holds O_DIRECTORY's bit, which a directory
opened without a mode has) */
#define MODE_ARGUMENT(flags, last, mode) \
	do \
	{ \
		va_list arguments; \
		va_start(arguments, last); \
		mode = ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) ? (mode_t)va_arg(arguments, int) : 0; \
		va_end(arguments); \
	} while (0)

#define OPEN(name) \
	int name(const char *path, int flags, ...) \
	{ \
		static open_function real; \
		mode_t mode; \
		MODE_ARGUMENT(flags, flags, mode); \
		if (refused(path)) \
			return -1; \
		if (!real) \
			real = (open_function)dlsym(RTLD_NEXT, #name); \
		return real(path, flags, mode); \
	}

#define OPENAT(name) \
	int name(int directory, const char *path, int flags, ...) \
	{ \
		static openat_function real; \
		mode_t mode; \
		MODE_ARGUMENT(flags, flags, mode); \
		if (refused(path)) \
			return -1; \
		if (!real) \
			real = (openat_function)dlsym(RTLD_NEXT, #name); \
		return real(directory, path, flags, mode); \
	}

OPEN(open)
OPEN(open64)
OPENAT(openat)
OPENAT(openat64)
