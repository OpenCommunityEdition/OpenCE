/*
HOST_LINUX_MAIN.C

Entry point of the Linux arm64 port: the AArch64 host of an ILP32 guest, built
from the Android port.

It loads the guest image (the game, built as ILP32 code), gives it an
environment describing where the game data and saves live, and runs its
main() on a thread of its own with its stack in guest memory
(host_thread.c), on which everything here after startup runs; the main
thread waits for it.

Storage (compare the Android README): the game data (the directory
holding maps/) is given on the command line, or by HALO_DATA_ROOT, or is
$HOME/.local/share/halo; saves go to its save/ subdirectory, as does
config.toml (port/linux/src/port_config.c, which the game reads); this
file reads only debug.sample_seconds and debug.memory_watch from it.
*/

#include "../../android/host/host.h"
#include "tomlc17.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void host_install_signal_handlers(void);
void host_linux_set_paths(const char *data, const char *save);
int host_linux_offer_game_data(const char *destination);

static char data_root[512];
static char save_root[512];

/* ---------- paths */

/* create path and every missing parent, like mkdir -p */
static void make_directories(const char *path)
{
	char buffer[512];
	char *p;

	if (strlen(path) >= sizeof(buffer))
		return;
	strcpy(buffer, path);
	for (p = buffer + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;
			mkdir(buffer, 0775);
			*p = '/';
		}
	}
	mkdir(buffer, 0775);
}

/* the data root: the command line, else HALO_DATA_ROOT, else
$HOME/.local/share/halo; the save root is its save/ subdirectory */
static void resolve_roots(int argc, char **argv)
{
	const char *home;

	if (argc > 1 && argv[1][0])
		snprintf(data_root, sizeof(data_root), "%s", argv[1]);
	else if (getenv("HALO_DATA_ROOT") && getenv("HALO_DATA_ROOT")[0])
		snprintf(data_root, sizeof(data_root), "%s", getenv("HALO_DATA_ROOT"));
	else
	{
		home = getenv("HOME");
		if (!home || !home[0])
			host_fatal("the Halo game data was not found: give the folder as the first argument, or set HALO_DATA_ROOT, or HOME");
		snprintf(data_root, sizeof(data_root), "%s/.local/share/halo", home);
	}
	snprintf(save_root, sizeof(save_root), "%s/save", data_root);
	make_directories(data_root);
	mkdir(save_root, 0775);
}

/* halo_guest.elf: HALO_GUEST_ELF, else next to the executable */
static const char *guest_image_path(char *buffer, size_t size)
{
	const char *from_environment = getenv("HALO_GUEST_ELF");
	const char *base;

	if (from_environment && from_environment[0])
		return from_environment;
	base = SDL_GetBasePath();
	if (base)
		snprintf(buffer, size, "%shalo_guest.elf", base);
	else
		snprintf(buffer, size, "halo_guest.elf");
	return buffer;
}

/* ---------- main */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

/* the thread that runs the game: passes the display and its density, the
time zone and the data and save folders to the guest through its
environment, chooses the write tracking (page protection, or page hashes
when debug.memory_watch is false), and runs the guest's main; never
returns normally */
static void *game_main(void *unused)
{
	struct host_environment environment = { { 0 }, 0 };
	char zone[64];
	char path[600];
	char image_path[1024];
	size_t image_size = 0;
	void *image;
	uint32_t boot;

	(void)unused;
	if (!host_directory_has_maps(data_root))
	{
		host_fatal("The Halo game data was not found.\n\nCopy the PAL game data (build 01.01.14.2342), "
			"the folder that contains maps, into\n%s", data_root);
	}

	host_environment_set(&environment, "HOME", save_root);
	host_environment_set(&environment, "HALO_DATA_ROOT", data_root);
	host_environment_set(&environment, "HALO_SAVE_ROOT", save_root);
	/* the game renders 480 lines at the display's aspect ratio
	(landscape) unless display.screen_width says otherwise (d3d8_gl.c) */
	host_environment_set_display(&environment);
	host_time_zone(zone, sizeof(zone));
	host_environment_set(&environment, "TZ", zone);
	/* internet play's MQTT brokers (network.brokers_file): written next to
	config.toml at each start, as a desktop update replaces the file beside
	its game; read from next to the executable */
	{
		char brokers_path[1024];
		size_t brokers_size = 0;
		void *brokers;
		const char *base = SDL_GetBasePath();

		if (base)
			snprintf(brokers_path, sizeof(brokers_path), "%sbrokers.txt", base);
		else
			snprintf(brokers_path, sizeof(brokers_path), "brokers.txt");
		brokers = SDL_LoadFile(brokers_path, &brokers_size);
		snprintf(path, sizeof(path), "%s/brokers.txt", data_root);
		if (brokers && SDL_SaveFile(path, brokers, brokers_size))
			host_logf(HOST_LOG_INFO, "wrote %s", path);
		else if (brokers)
			host_logf(HOST_LOG_ERROR, "cannot write %s: %s", path, SDL_GetError());
		SDL_free(brokers);
	}
	snprintf(path, sizeof(path), "%s/config.toml", data_root);

	image = SDL_LoadFile(guest_image_path(image_path, sizeof(image_path)), &image_size);
	if (!image)
		host_fatal("cannot read the game image (%s): %s", image_path, SDL_GetError());
	if (host_load_image(image, image_size) != 0)
	{
		FILE *report;

		if (!host_memory_fixed_unavailable())
			host_fatal("cannot load the game image; see the terminal output for details");

		snprintf(path, sizeof(path), "%s/memory_map.txt", data_root);
		report = fopen(path, "w");
		host_memory_report_low_mappings(report);
		if (report)
			fclose(report);
		host_fatal("The game cannot start: the memory it needs at 0x80000000 is taken by something else "
			"on this system.\n\nmemory_map.txt from\n%s\nlists the mappings below 4 GB; please report it "
			"with that file (or the terminal output).", data_root);
	}
	SDL_free(image);

	host_start_sampler_from_config(path);
	if (!host_config_boolean_or(path, "debug.memory_watch", 1))
	{
		host_memory_watch_use_hashes();
		host_logf(HOST_LOG_INFO, "write tracking: page hashes (debug.memory_watch = false)");
	}
	else
	{
		host_logf(HOST_LOG_INFO, "write tracking: page protection");
	}
	host_linux_set_paths(data_root, save_root);
	boot = host_make_boot(&environment);
	host_logf(HOST_LOG_INFO, "data %s, saves %s", data_root, save_root);
	host_run_guest_main(boot);
}

int main(int argc, char **argv)
{
	host_logf(HOST_LOG_INFO, "Halo for Linux starting");
	host_install_signal_handlers();
	resolve_roots(argc, argv);
	/* the dialog needs the main thread; host_linux_offer_game_data returns 0
	when no prompt could be shown, and game_main then reports the missing data
	with host_fatal */
	if (!host_directory_has_maps(data_root))
		host_linux_offer_game_data(data_root);
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		pause();
}
