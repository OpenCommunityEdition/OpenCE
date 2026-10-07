/*
HOST_MAIN.C

Entry point of the macOS port.

It loads the guest image (the game, built as ILP32 code), gives it an
environment and runs its main() on the process's first thread, which
Cocoa wants SDL's windows and events on, moved onto a stack in guest memory
(host_thread.c).

Where the game keeps its data (port/macos/README.md): run from the build
(build/macos/halo), beside the executable, as the Linux build does; run as
the app (Halo.app), in ~/Library/Application Support/Halo, which the guest
takes for its executable's folder (its /proc/self/exe and SDL_GetBasePath,
host_syscall.c and host_sdl.c), so that its desktop code finds maps/ and
config.toml there and extracts the game data there on the first start.
*/

#include "host.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void host_install_signal_handlers(void);

char host_executable_path[1024];
char host_base_path[1024];

/* ---------- logging and termination */

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	fputs("halo-macos: ", stderr);
	vfprintf(stderr, format, arguments);
	fputc('\n', stderr);
	va_end(arguments);
}

void host_log(int priority, uint32_t text)
{
	host_logf(priority, "%s", (const char *)G2H(text));
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_ERROR, "%s", message);
	/* (Cocoa's windows only on the main thread) */
	if (pthread_main_np())
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(uint32_t reason)
{
	host_logf(HOST_LOG_ERROR, "guest abort: %s", (const char *)G2H(reason));
	abort();
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	/* (the display's mode back, where the game changed it; Cocoa's only on
	the main thread) */
	if (pthread_main_np())
		SDL_Quit();
	_exit(code);
}

/* ---------- paths */

static void make_directories(const char *path)
{
	char partial[1024];
	size_t index;

	snprintf(partial, sizeof(partial), "%s", path);
	for (index = 1; partial[index]; index++)
	{
		if (partial[index] == '/')
		{
			partial[index] = 0;
			mkdir(partial, 0755);
			partial[index] = '/';
		}
	}
	mkdir(partial, 0755);
}

static int copy_file(const char *from, const char *to)
{
	FILE *input = fopen(from, "rb");
	FILE *output;
	char buffer[16384];
	size_t count;
	int ok = 1;

	if (!input)
		return 0;
	output = fopen(to, "wb");
	if (!output)
	{
		fclose(input);
		return 0;
	}
	while ((count = fread(buffer, 1, sizeof(buffer), input)) > 0)
		ok &= fwrite(buffer, 1, count, output) == count;
	fclose(input);
	return fclose(output) == 0 && ok;
}

/* the guest image's path; sets the guest's executable and base paths */
static void find_paths(char *image, size_t image_size)
{
	char executable[PATH_MAX], resolved[PATH_MAX];
	uint32_t size = sizeof(executable);
	char *bundle, *slash;

	if (_NSGetExecutablePath(executable, &size) != 0 || !realpath(executable, resolved))
		host_fatal("cannot find the game's executable");
	bundle = strstr(resolved, ".app/Contents/MacOS/");
	if (bundle)
	{
		/* the app: its image among its resources, its data the user's */
		const char *home = getenv("HOME");
		char data[1024], from[1200], to[1200];

		bundle[strlen(".app/Contents")] = 0;
		snprintf(image, image_size, "%s/Resources/halo_guest.elf", resolved);
		if (!home || !*home)
			host_fatal("HOME is not set");
		snprintf(data, sizeof(data), "%s/Library/Application Support/Halo", home);
		make_directories(data);
		snprintf(host_executable_path, sizeof(host_executable_path), "%s/halo", data);
		/* internet play's MQTT brokers (network.brokers_file): the app's
		list, written beside config.toml at each start, as a desktop update
		replaces the file beside its game */
		snprintf(from, sizeof(from), "%s/Resources/brokers.txt", resolved);
		snprintf(to, sizeof(to), "%s/brokers.txt", data);
		if (!copy_file(from, to))
			host_logf(HOST_LOG_WARN, "cannot write %s", to);
	}
	else
	{
		/* the build: everything beside the executable */
		snprintf(host_executable_path, sizeof(host_executable_path), "%s", resolved);
		slash = strrchr(resolved, '/');
		if (slash)
			*slash = 0;
		snprintf(image, image_size, "%s/halo_guest.elf", resolved);
	}
	snprintf(host_base_path, sizeof(host_base_path), "%s", host_executable_path);
	slash = strrchr(host_base_path, '/');
	if (slash)
		slash[1] = 0;
}

/* ---------- the guest's environment */

#define ENVIRONMENT_MAXIMUM 64

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
	size_t size = 0x10000;
	uint32_t address = host_low_map(size);
	char *memory = G2H(address);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *guest_argv = (uint32_t *)(memory + sizeof(*boot));
	uint32_t *guest_environment;
	char *strings;
	int index;

	if (!address)
		host_fatal("cannot allocate the guest's environment");
	if (argc > 16)
		argc = 16;
	guest_environment = guest_argv + argc + 1;
	strings = (char *)(guest_environment + ENVIRONMENT_MAXIMUM + 1);
	for (index = 0; index < argc; index++)
	{
		size_t length = strlen(argv[index]) + 1;

		if (strings + length > memory + size)
			break;
		memcpy(strings, argv[index], length);
		guest_argv[index] = H2G(strings);
		strings += length;
	}
	guest_argv[index] = 0;
	boot->argc = (uint32_t)index;
	for (index = 0; index < environment->count; index++)
	{
		size_t length = strlen(environment->entries[index]) + 1;

		if (strings + length > memory + size)
			break;
		memcpy(strings, environment->entries[index], length);
		guest_environment[index] = H2G(strings);
		strings += length;
	}
	guest_environment[index] = 0;
	boot->argv = H2G(guest_argv);
	boot->environment = H2G(guest_environment);
	boot->page_size = 0x1000;
	return address;
}

/* ---------- main */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

static void guest_main(void *boot)
{
	host_run_guest_main((uint32_t)(uintptr_t)boot);
}

int main(int argc, char *argv[])
{
	extern char **environ;
	struct environment environment = { { 0 }, 0 };
	char image_path[1200], zone[64];
	size_t image_size = 0;
	void *image;
	char **variable;
	uint32_t boot;

	/* a write to a connection the other end closed fails instead of ending
	the game */
	signal(SIGPIPE, SIG_IGN);
	find_paths(image_path, sizeof(image_path));
	host_install_signal_handlers();

	image = SDL_LoadFile(image_path, &image_size);
	if (!image)
		host_fatal("cannot read the game image %s: %s", image_path, SDL_GetError());
	if (host_load_image(image, image_size) != 0)
		host_fatal("cannot load the game image %s; see the log for details", image_path);
	SDL_free(image);

	/* the host's own settings for the game (HALO_*, config.toml's
	environment overrides), and what the guest's C library needs */
	for (variable = environ; *variable; variable++)
	{
		const char *equals = strchr(*variable, '=');

		if (equals && !strncmp(*variable, "HALO_", 5))
		{
			char name[256];

			snprintf(name, sizeof(name), "%.*s", (int)(equals - *variable), *variable);
			environment_set(&environment, name, equals + 1);
		}
	}
	if (getenv("HOME"))
		environment_set(&environment, "HOME", getenv("HOME"));
	if (getenv("TMPDIR"))
		environment_set(&environment, "TMPDIR", getenv("TMPDIR"));
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);

	boot = make_boot(argc, argv, &environment);
	host_logf(HOST_LOG_INFO, "starting %s (as %s)", image_path, host_executable_path);
	host_run_on_guest_stack(guest_main, (void *)(uintptr_t)boot, MAIN_STACK_SIZE);
	return 1;
}
