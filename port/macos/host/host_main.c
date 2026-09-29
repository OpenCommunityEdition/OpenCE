/*
HOST_MAIN.C

Entry point of the macOS port.

It loads the guest image (the game, built as x32 code: halo_guest.elf next
to the executable), gives it an environment and its command line, points SDL
at ANGLE (OpenGL ES over Metal, libEGL.dylib and libGLESv2.dylib next to the
executable) and runs the game's main() on the process's main thread, on a
stack in guest memory (host_thread.c): Cocoa wants the window and its events
there.

The folder holding the executable is the game's folder, as in the Linux
build: maps/ (extracted there on first start from the player's disc image),
config.toml, the saved games and the logs (debug.txt, the game's; host.txt,
this file's).
*/

#include "host.h"

#include <SDL3/SDL.h>
#include <crt_externs.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

extern void *host_gles_library;
int host_gl_load(void);

char host_data_root[1024];
char host_executable_path[1024];

/* ---------- logging and termination */

static FILE *log_file;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void host_log(int priority, const char *text)
{
	static const char *const names[] = { "", "", "", "", "info", "warning", "error" };
	const char *name = priority >= 0 && priority <= 6 ? names[priority] : "";

	pthread_mutex_lock(&log_lock);
	fprintf(stderr, "halo host %s: %s\n", name, text);
	if (log_file)
	{
		fprintf(log_file, "%s: %s\n", name, text);
		fflush(log_file);
	}
	pthread_mutex_unlock(&log_lock);
}

void host_logf(int priority, const char *format, ...)
{
	char message[2048];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_log(priority, message);
}

void host_fatal(const char *format, ...)
{
	char message[2048];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_log(HOST_LOG_ERROR, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	host_logf(HOST_LOG_ERROR, "guest abort: %s", reason);
	abort();
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	SDL_Quit();
	_exit(code);
}

/* the Android port's storage paths; the desktop platform layer finds its
folders itself, so these are the game's folder */
void host_android_path(int which, char *buffer, uint32_t size)
{
	(void)which;
	snprintf(buffer, size, "%s", host_data_root);
}

/* ---------- debugging hooks (the Android port's sampler is not needed:
lldb attaches to Rosetta processes) */

void host_debug_thread_started(void)
{
}

void host_debug_thread_exited(void)
{
}

/* ---------- the guest's environment */

#define ENVIRONMENT_MAXIMUM 128
#define ARGUMENT_MAXIMUM 16

struct environment
{
	char *entries[ENVIRONMENT_MAXIMUM];
	int count;
};

static void environment_set(struct environment *environment, const char *name, const char *value)
{
	size_t length = strlen(name);
	char *entry;
	int index;

	entry = malloc(length + strlen(value) + 2);
	sprintf(entry, "%s=%s", name, value);
	for (index = 0; index < environment->count; index++)
	{
		if (!strncmp(environment->entries[index], name, length) && environment->entries[index][length] == '=')
		{
			free(environment->entries[index]);
			environment->entries[index] = entry;
			return;
		}
	}
	if (environment->count < ENVIRONMENT_MAXIMUM)
		environment->entries[environment->count++] = entry;
	else
		free(entry);
}

/* POSIX TZ for the current local offset (the guest's musl has no zone
database) */
static void time_zone(char *buffer, size_t size)
{
	time_t now = time(NULL);
	struct tm local;
	long offset;

	localtime_r(&now, &local);
	offset = -local.tm_gmtoff;
	snprintf(buffer, size, "<L>%s%ld:%02ld", offset < 0 ? "-" : "", labs(offset) / 3600, (labs(offset) / 60) % 60);
}

/* copies argv and the environment into guest memory */
static uint32_t make_boot(int argc, char **argv, const struct environment *environment)
{
	size_t size = 0x20000;
	char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *guest_argv = (uint32_t *)(memory + sizeof(*boot));
	uint32_t *environ_list = guest_argv + ARGUMENT_MAXIMUM + 1;
	char *strings = (char *)(environ_list + ENVIRONMENT_MAXIMUM + 1);
	int index, count = 0;

	if (!memory)
		host_fatal("cannot allocate the guest's environment");
	for (index = 0; index < argc && count < ARGUMENT_MAXIMUM; index++)
	{
		const char *text = index == 0 ? host_executable_path : argv[index];
		size_t length = strlen(text) + 1;

		/* macOS adds -psn_... when started from the Finder */
		if (index > 0 && !strncmp(text, "-psn_", 5))
			continue;
		if (strings + length > memory + size)
			break;
		memcpy(strings, text, length);
		guest_argv[count++] = (uint32_t)(uintptr_t)strings;
		strings += length;
	}
	guest_argv[count] = 0;
	for (index = 0; index < environment->count; index++)
	{
		size_t length = strlen(environment->entries[index]) + 1;

		if (strings + length > memory + size)
			break;
		memcpy(strings, environment->entries[index], length);
		environ_list[index] = (uint32_t)(uintptr_t)strings;
		strings += length;
	}
	environ_list[index] = 0;
	boot->argc = (uint32_t)count;
	boot->argv = (uint32_t)(uintptr_t)guest_argv;
	boot->environment = (uint32_t)(uintptr_t)environ_list;
	boot->page_size = (uint32_t)getpagesize();
	return (uint32_t)(uintptr_t)boot;
}

/* ---------- start-up */

static void find_folders(void)
{
	char path[PATH_MAX];
	char resolved[PATH_MAX];
	uint32_t size = sizeof(path);
	char *slash;

	if (_NSGetExecutablePath(path, &size) != 0 || !realpath(path, resolved))
		host_fatal("cannot find the executable's folder");
	snprintf(host_data_root, sizeof(host_data_root), "%s", resolved);
	slash = strrchr(host_data_root, '/');
	if (slash)
		*slash = 0;
	/* the guest's platform layer looks for its folder through
	/proc/self/exe (host_syscall.c) */
	snprintf(host_executable_path, sizeof(host_executable_path), "%s/halo", host_data_root);
}

static void *read_file(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	void *data = NULL;
	long length;

	if (!file)
		return NULL;
	if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) > 0 && fseek(file, 0, SEEK_SET) == 0)
	{
		data = malloc((size_t)length);
		if (data && fread(data, 1, (size_t)length, file) == (size_t)length)
		{
			*size = (size_t)length;
		}
		else
		{
			free(data);
			data = NULL;
		}
	}
	fclose(file);
	return data;
}

static void load_angle(void)
{
	char egl[1200], gles[1200];

	snprintf(egl, sizeof(egl), "%s/libEGL.dylib", host_data_root);
	snprintf(gles, sizeof(gles), "%s/libGLESv2.dylib", host_data_root);
	host_gles_library = dlopen(gles, RTLD_NOW | RTLD_GLOBAL);
	if (!host_gles_library)
		host_fatal("cannot load OpenGL ES (ANGLE) from %s: %s", gles, dlerror());
	if (!host_gl_load())
		host_fatal("%s lacks OpenGL ES 3 entry points", gles);
	SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, gles);
	SDL_SetHint(SDL_HINT_EGL_LIBRARY, egl);
	/* ANGLE on Metal (its default on macOS) */
	setenv("ANGLE_DEFAULT_PLATFORM", "metal", 0);
}

int main(int argc, char *argv[])
{
	struct environment environment = { { 0 }, 0 };
	char path[1200];
	char zone[64];
	void *image;
	size_t image_size = 0;
	char **host_environment = *_NSGetEnviron();
	int index;

	if (host_memory_reserve() != 0)
	{
		fprintf(stderr, "halo: the low 4 GB of the address space is not free; is this the x86-64 build "
			"(run under Rosetta on Apple silicon)?\n");
		return 1;
	}
	find_folders();
	if (chdir(host_data_root) != 0)
		host_fatal("cannot enter %s", host_data_root);
	snprintf(path, sizeof(path), "%s/host.txt", host_data_root);
	log_file = fopen(path, "w");
	host_logf(HOST_LOG_INFO, "Halo for macOS starting in %s", host_data_root);
	host_install_signal_handlers();
	load_angle();

	/* the environment: the player's HALO_* settings (port_config.c),
	HOME, and a TZ musl understands */
	for (index = 0; host_environment[index]; index++)
	{
		const char *entry = host_environment[index];

		if (!strncmp(entry, "HALO_", 5) || !strncmp(entry, "HOME=", 5) || !strncmp(entry, "USER=", 5) ||
			!strncmp(entry, "LANG=", 5))
		{
			const char *equals = strchr(entry, '=');
			char name[256];

			if (!equals || equals - entry >= (long)sizeof(name))
				continue;
			memcpy(name, entry, (size_t)(equals - entry));
			name[equals - entry] = 0;
			environment_set(&environment, name, equals + 1);
		}
	}
	snprintf(path, sizeof(path), "%s/", host_data_root);
	environment_set(&environment, "HALO_BASE_PATH", path);
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);

	snprintf(path, sizeof(path), "%s/halo_guest.elf", host_data_root);
	image = read_file(path, &image_size);
	if (!image)
		host_fatal("cannot read the game image %s", path);
	if (host_load_image(image, image_size) != 0)
		host_fatal("cannot load the game image; see %s/host.txt for details", host_data_root);
	free(image);

	host_run_guest_main(make_boot(argc, argv, &environment));
}
