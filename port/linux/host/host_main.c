/*
HOST_MAIN.C

Entry point of the Linux builds that run the game as an ILP32 guest (the
64-bit ARM build, tools/linux_arm64_build.py).

The host is the Android port's (port/android/host, port/android/README.md),
with this file in place of its host_main.c and host_touch.c; what both
mains do is in host_common.c there. It loads the guest image (the game,
built as ILP32 code) from inside the executable (guest_image.S), and runs
its main() on a thread of its own with its stack in guest memory
(host_thread.c). The main thread waits; the game ends the process.

The guest is the Linux desktop build of the game, so it finds the game data,
the saves and config.toml as the x86 build does (port/linux/README.md): it
gets the part of this process's environment that it reads (the HALO_
settings, HOME, the XDG folders), and /proc/self/exe is this executable.
The log is the standard error stream.
*/

#include "host.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

/* guest_image.S */
extern const unsigned char halo_guest_image_start[];
extern const unsigned char halo_guest_image_end[];

extern char **environ;

/* ---------- the log: the standard error stream */

/* one writev, so that the lines of threads do not mix (host_common.c: no
stdio, as signal handlers log through it too) */
static void log_to_stderr(int priority, const char *line)
{
	const char *level = priority >= HOST_LOG_FATAL ? "fatal: " : priority >= HOST_LOG_ERROR ? "error: " :
		priority >= HOST_LOG_WARN ? "warning: " : "";
	struct iovec parts[4];

	parts[0].iov_base = (void *)"halo-host: ";
	parts[0].iov_len = 11;
	parts[1].iov_base = (void *)level;
	parts[1].iov_len = strlen(level);
	parts[2].iov_base = (void *)line;
	parts[2].iov_len = strlen(line);
	parts[3].iov_base = (void *)"\n";
	parts[3].iov_len = 1;
	if (writev(2, parts, 4) < 0)
		return;
}

/* ---------- the guest's environment */

/* what the game reads: its settings (HALO_...), where the saves go
(port/linux/src/xbox_files.c) and where Discord's socket is (posix_net.c);
the display and sound are the host's. TZ is game_main's. */
static int environment_passes(const char *entry)
{
	static const char *const prefixes[] =
	{
		"HALO_", "HOME=", "XDG_", "USER=", "LANG=", "LC_", "TMPDIR=", "TMP=", "TEMP=",
	};
	size_t index;

	for (index = 0; index < sizeof(prefixes) / sizeof(prefixes[0]); index++)
	{
		if (!strncmp(entry, prefixes[index], strlen(prefixes[index])))
			return 1;
	}
	return 0;
}

/* ---------- main */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

/* the thread that runs the game: the guest's environment, the image, the
debugging settings of config.toml (next to the executable, as the game
reads it), and the guest's main; never returns normally */
static void *game_main(void *unused)
{
	struct host_environment environment = { { 0 }, 0 };
	const char *base = SDL_GetBasePath();
	char zone[64];
	char path[1024];
	char **entry;

	(void)unused;
	for (entry = environ; *entry; entry++)
	{
		if (environment_passes(*entry))
			host_environment_add(&environment, *entry);
	}
	host_time_zone(zone, sizeof(zone));
	host_environment_set(&environment, "TZ", zone);

	if (host_load_image(halo_guest_image_start, (size_t)(halo_guest_image_end - halo_guest_image_start)) != 0)
		host_fatal("cannot load the game image");
	snprintf(path, sizeof(path), "%sconfig.toml", base ? base : "");
	host_apply_debug_settings(path, NULL);
	host_run_guest_main(host_make_boot(&environment, "halo"));
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	/* Mesa's GL thread, as the x86 build asks for it (sdl_platform.c): the
	guest's environment is its own copy, so the guest's request cannot reach
	the driver, which reads this process's. An explicit setting stays. */
	setenv("mesa_glthread", "true", 0);
	host_log_set_sink(log_to_stderr);
	host_install_signal_handlers();
	host_signal_stack_install();
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		pause();
}
