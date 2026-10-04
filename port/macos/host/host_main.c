/*
HOST_MAIN.C

Entry point of the macOS port.

It starts the virtual machine, loads the guest image (the game, built as
ILP32 code, from the build tree or next to the executable), gives it an
environment describing where the game data and saves live, and runs its
main() on a thread with its own guest stack; the SDL event loop is the
guest's (its d3d8_gl pumps the window's events through host_sdl.c), so
this thread only waits for the process to end.

Storage (see port/linux/README.md): the game data (the folder that holds
maps/) and the saves are where the Linux port keeps them — next to the
executable, or paths.data and paths.saves of config.toml, which the game
reads itself; this file only points the guest's environment at them.
*/

#include "host.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <mach/mach.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ---------- logging and termination */

static FILE *debug_file;

static void debug_open(void)
{
	if (debug_file)
		return;
	debug_file = fopen("debug.txt", "a");
}

void host_log(int priority, const char *text)
{
	static const char *priorities[] = { "info", "warn", "error", "fatal" };

	debug_open();
	fprintf(stderr, "[%s] %s\n", priorities[priority > 3 ? 3 : (unsigned)priority], text);
	if (debug_file)
	{
		fprintf(debug_file, "[%s] %s\n", priorities[priority > 3 ? 3 : (unsigned)priority], text);
		fflush(debug_file);
	}
}

void host_logf(int priority, const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_log(priority, message);
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_log(HOST_LOG_FATAL, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	host_logf(HOST_LOG_FATAL, "guest abort: %s", reason);
	abort();
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	exit(code);
}

int host_errno(void)
{
	return errno;
}

/* ---------- paths */

static char data_root[512];
static char save_root[512];

void host_android_path(int which, char *buffer, unsigned int size)
{
	snprintf(buffer, size, "%s", which ? save_root : data_root);
}

static int directory_has_maps(const char *root)
{
	char path[600];
	struct stat information;

	snprintf(path, sizeof(path), "%s/maps/ui.map", root);
	return stat(path, &information) == 0;
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
static uint32_t make_boot(const struct environment *environment)
{
	size_t size = 0x10000;
	char *memory = host_low_map(size);
	struct halo_guest_boot *boot;
	uint32_t *argv;
	uint32_t *environ_list;
	char *strings;
	int index;

	if (!memory)
		host_fatal("cannot allocate the guest's environment");
	boot = (struct halo_guest_boot *)memory;
	argv = (uint32_t *)(memory + sizeof(*boot));
	environ_list = argv + 2;
	strings = (char *)(environ_list + ENVIRONMENT_MAXIMUM + 1);
	strcpy(strings, "halo");
	argv[0] = host_guest_address(strings);
	argv[1] = 0;
	strings += strlen(strings) + 1;
	for (index = 0; index < environment->count; index++)
	{
		size_t length = strlen(environment->entries[index]) + 1;

		if (strings + length > memory + size)
			break;
		memcpy(strings, environment->entries[index], length);
		environ_list[index] = host_guest_address(strings);
		strings += length;
	}
	environ_list[index] = 0;
	boot->argc = 1;
	boot->argv = host_guest_address(argv);
	boot->environment = host_guest_address(environ_list);
	boot->page_size = 4096; /* what the guest's musl expects (its own
	                           structures use 4 KiB pages) */
	return host_guest_address(boot);
}

/* ---------- the game's thread */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

static void *game_main(void *unused)
{
	struct environment environment = { { 0 }, 0 };
	char zone[64];
	char path[600];
	const char *candidates[3];
	int index;
	size_t image_size = 0;
	void *image;
	uint32_t boot;

	(void)unused;
	/* the game data: $HALO_DATA_ROOT, the working folder, the folder of
	the executable, or assets/ in either (the Linux port's search) */
	candidates[0] = getenv("HALO_DATA_ROOT");
	candidates[1] = ".";
	candidates[2] = SDL_GetBasePath();
	for (index = 0; index < 3; index++)
	{
		if (candidates[index] && directory_has_maps(candidates[index]))
			break;
	}
	if (index == 3)
	{
		host_fatal("The Halo game data was not found.\n\nPut the folder that contains maps/ "
			"(from an Xbox disc image; the game can extract it) next to halo, or in assets/, "
			"or set HALO_DATA_ROOT to it.");
	}
	snprintf(data_root, sizeof(data_root), "%s", candidates[index]);
	snprintf(save_root, sizeof(save_root), "%s/save", data_root);
	mkdir(save_root, 0775);

	environment_set(&environment, "HOME", getenv("HOME") ? getenv("HOME") : save_root);
	environment_set(&environment, "HALO_DATA_ROOT", data_root);
	environment_set(&environment, "HALO_SAVE_ROOT", save_root);
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);
	snprintf(path, sizeof(path), "%s/config.toml", data_root);

	{
		/* the image: next to the executable, else the working folder */
		const char *bases[2] = { SDL_GetBasePath(), "." };
		const char *image_path = NULL;

		for (index = 0; index < 2 && !image_path; index++)
		{
			static char buffer[600];
			struct stat information;

			snprintf(buffer, sizeof(buffer), "%s/halo_guest.elf", bases[index]);
			if (stat(buffer, &information) == 0)
				image_path = buffer;
		}
		if (!image_path)
			host_fatal("halo_guest.elf is not next to halo (build it: ninja macos)");
		image = SDL_LoadFile(image_path, &image_size);
		if (!image)
			host_fatal("cannot read the game image: %s", SDL_GetError());
	}
	if (host_load_image(image, image_size) != 0)
		host_fatal("cannot load the game image; see debug.txt for details");
	free(image);

	boot = make_boot(&environment);
	host_logf(HOST_LOG_INFO, "data %s, saves %s", data_root, save_root);
	host_run_guest_main(boot);
	return NULL;
}

/* ---------- the crash handler (the guest's frames, through its own) */

static void crash_handler(int signal_number, siginfo_t *information, void *context)
{
	(void)information;
	(void)context;
	host_logf(HOST_LOG_ERROR, "the host crashed (signal %d); the guest's own log is debug.txt",
		signal_number);
	/* chain to the default (a report with the host's frames) */
	signal(signal_number, SIG_DFL);
	raise(signal_number);
}

void host_install_signal_handlers(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = crash_handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGBUS, &action, NULL);
}

/* ---------- the sampler (debug.sample_seconds; host_debug.c is the
Android one's interface, and the vCPU thread's guest frames are what it
reads) */

void host_debug_thread_started(void)
{
}

void host_debug_thread_exited(void)
{
}

void host_debug_start_sampler(const char *seconds)
{
	host_logf(HOST_LOG_INFO, "sampling is not built into this port yet (debug.sample_seconds %s)",
		seconds ? seconds : "");
}

/* ---------- main */

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	host_logf(HOST_LOG_INFO, "Halo for macOS starting");
	host_install_signal_handlers();
	if (host_vm_start() != 0)
		host_fatal("cannot create the virtual machine");
	if (host_vm_map_window() != 0)
		host_fatal("cannot map the guest's Xbox memory window");
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		pause();
}