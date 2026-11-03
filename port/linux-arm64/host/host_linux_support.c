/*
HOST_LINUX_SUPPORT.C

The Linux host's logging and termination, and the imports the guest
expects that are named for Android. The Android build keeps its own
versions in host_main.c and host_render_policy.c; this file is the
Linux side of the same contract, without logcat and without JNI.

The Android port names these imports for the platform they came from;
the guest calls them by name, so the Linux host provides them with the
same names and signatures.
*/

#include "../../android/host/host.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#include <SDL3/SDL.h>

/* ---------- logging and termination */

#define MESSAGE_MAXIMUM 1024

void host_logf(int priority, const char *format, ...)
{
	char message[MESSAGE_MAXIMUM];
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	fprintf(stderr, "halo: %s\n", message);
}

void host_log(int priority, const char *text)
{
	(void)priority;
	fprintf(stderr, "halo: %s\n", text);
}

void host_fatal(const char *format, ...)
{
	char message[MESSAGE_MAXIMUM];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	fprintf(stderr, "halo: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	fprintf(stderr, "guest abort: %s\n", reason);
	abort();
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	_exit(code);
}

int host_errno(void)
{
	return errno;
}

/* ---------- paths */

static char data_root[512];
static char save_root[512];

/* fills the two roots host_android_path copies from; called by the Linux
main before the guest starts */
void host_linux_set_paths(const char *data, const char *save)
{
	snprintf(data_root, sizeof(data_root), "%s", data);
	snprintf(save_root, sizeof(save_root), "%s", save);
}

void host_android_path(int which, char *buffer, uint32_t size)
{
	snprintf(buffer, size, "%s", which ? save_root : data_root);
}

/* 0 = unknown; the Linux host has no SoC to classify, and unknown
hardware keeps both compatibility paths (see android_render_policy.h) */
int host_android_chip_family(void)
{
	return 0;
}

/* no system gestures on the Linux host; all 0 (the touch controls then
use the whole screen) */
void host_gesture_insets(int *insets)
{
	int index;

	for (index = 0; index < 4; index++)
		insets[index] = 0;
}
