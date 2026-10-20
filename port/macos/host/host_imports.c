/*
HOST_IMPORTS.C

The guest's imports (port/android/guest/runtime/guest_host.h) that are not
SDL's (port/android/host/host_sdl.c), the POSIX helpers'
(port/linux/src/posix_*.c), or system calls (host_syscall.c).

For now the host is headless: only debug.null_renderer is supported, so
the OpenGL ES imports do nothing, and the Android app's touch controls and
gesture insets are absent.
*/

#include "host.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------- process */

void host_log(int priority, const char *text)
{
	(void)priority;
	fprintf(stderr, "halo: %s\n", text ? text : "");
}

void host_abort(const char *reason)
{
	host_fatal("the guest aborted: %s", reason ? reason : "(no reason)");
}

/* (host_dispatch.c keeps the errno each import left) */
int host_errno(void)
{
	return host_linux_errno(host_saved_errno);
}

/* ---------- threads: the guest's thread pointer is its vCPU's TPIDR_EL0 */

unsigned int host_get_tp(void)
{
	return host_vcpu_thread_pointer();
}

void host_set_tp(unsigned int thread)
{
	host_vcpu_set_thread_pointer(thread);
}

/* ---------- paths: the data folder (HALO_DATA_ROOT, else the current
folder) holds both the game's data and its saves */

void host_android_path(int which, char *buffer, unsigned int size)
{
	const char *root = getenv("HALO_DATA_ROOT");
	char folder[1024];

	(void)which;
	if (!size)
		return;
	if (!root || !*root)
		root = getcwd(folder, sizeof(folder)) ? folder : ".";
	snprintf(buffer, size, "%s", root);
}

void host_gesture_insets(int *insets)
{
	if (insets)
		memset(insets, 0, 4 * sizeof(*insets));
}

/* ---------- the Android app's touch controls: none */

void host_touch_read(int *state)
{
	if (state)
		memset(state, 0, 7 * sizeof(*state));
}

void host_touch_look_read(float *delta)
{
	if (delta)
		memset(delta, 0, 4 * sizeof(*delta));
}

void host_touch_rumble(unsigned int low, unsigned int high)
{
	(void)low;
	(void)high;
}

void host_touch_scene(int scene)
{
	(void)scene;
}

void host_touch_bindings(const int *controls)
{
	(void)controls;
}

/* ---------- OpenGL ES: none while headless */

void host_gl_get_string(unsigned int name, int index, char *buffer, unsigned int size)
{
	(void)name;
	(void)index;
	if (buffer && size)
		buffer[0] = '\0';
}

int host_gl_has_extension(const char *name)
{
	(void)name;
	return 0;
}

void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data)
{
	(void)buffer;
	(void)offset;
	if (data)
		memset(data, 0, size);
}

void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data)
{
	(void)target;
	(void)offset;
	(void)size;
	(void)data;
}

void host_gl_fence_frame(unsigned int slot)
{
	(void)slot;
}

void host_gl_wait_frame(unsigned int slot)
{
	(void)slot;
}
