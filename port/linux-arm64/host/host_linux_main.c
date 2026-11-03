/*
HOST_LINUX_MAIN.C

Entry point of the Linux arm64 port.

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

static char data_root[512];
static char save_root[512];

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

/* debug.sample_seconds from config.toml, as text for the sampler, or 0 */
static int config_sample_seconds(const char *path, char *text, size_t size)
{
	toml_result_t result = toml_parse_file_ex(path);
	int found = 0;

	if (!result.ok)
		return 0;
	{
		toml_datum_t seconds = toml_seek(result.toptab, "debug.sample_seconds");
		double value = seconds.type == TOML_FP64 ? seconds.u.fp64 :
			seconds.type == TOML_INT64 ? (double)seconds.u.int64 : 0.0;

		if (value > 0.0)
		{
			snprintf(text, size, "%g", value);
			found = 1;
		}
	}
	toml_free(result);
	return found;
}

/* a boolean setting of config.toml (a dotted name), or otherwise when the
file, the setting or a boolean is missing */
static int config_boolean_or(const char *path, const char *name, int otherwise)
{
	toml_result_t result = toml_parse_file_ex(path);
	int value = otherwise;

	if (!result.ok)
		return otherwise;
	{
		toml_datum_t datum = toml_seek(result.toptab, name);

		if (datum.type == TOML_BOOLEAN)
			value = datum.u.boolean ? 1 : 0;
	}
	toml_free(result);
	return value;
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
	char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *argv = (uint32_t *)(memory + sizeof(*boot));
	uint32_t *environ_list = argv + 2;
	char *strings = (char *)(environ_list + ENVIRONMENT_MAXIMUM + 1);
	int index;

	if (!memory)
		host_fatal("cannot allocate the guest's environment");
	strcpy(strings, "halo");
	argv[0] = (uint32_t)(uintptr_t)strings;
	argv[1] = 0;
	strings += strlen(strings) + 1;
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
	boot->argc = 1;
	boot->argv = (uint32_t)(uintptr_t)argv;
	boot->environment = (uint32_t)(uintptr_t)environ_list;
	boot->page_size = (uint32_t)getpagesize();
	return (uint32_t)(uintptr_t)boot;
}

/* ---------- paths */

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
	struct environment environment = { { 0 }, 0 };
	char zone[64];
	char path[600];
	char image_path[1024];
	size_t image_size = 0;
	void *image;
	uint32_t boot;

	(void)unused;
	if (!directory_has_maps(data_root))
	{
		host_fatal("The Halo game data was not found.\n\nCopy the PAL game data (build 01.01.14.2342), "
			"the folder that contains maps, into\n%s", data_root);
	}

	environment_set(&environment, "HOME", save_root);
	environment_set(&environment, "HALO_DATA_ROOT", data_root);
	environment_set(&environment, "HALO_SAVE_ROOT", save_root);
	{
		/* the game renders 480 lines at the display's aspect ratio
		(landscape) unless display.screen_width says otherwise (d3d8_gl.c) */
		const SDL_DisplayMode *mode;
		char width[16];
		char refresh[16];

		SDL_InitSubSystem(SDL_INIT_VIDEO);
		mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		if (mode && mode->w > 0 && mode->h > 0)
		{
			int longer = mode->w > mode->h ? mode->w : mode->h;
			int shorter = mode->w > mode->h ? mode->h : mode->w;

			snprintf(width, sizeof(width), "%d", (480 * longer / shorter) & ~1);
			environment_set(&environment, "HALO_DISPLAY_WIDTH", width);
			host_logf(HOST_LOG_INFO, "display %dx%d: rendering %sx480", mode->w, mode->h, width);
		}
		if (mode)
		{
			/* display.max_fps's "twice the refresh rate" (sdl_platform.c):
			the guest cannot ask SDL for the display mode */
			snprintf(refresh, sizeof(refresh), "%g", mode->refresh_rate > 0.0f ? mode->refresh_rate : 60.0f);
			environment_set(&environment, "HALO_DISPLAY_REFRESH", refresh);
		}
		{
			/* dp for the touch controls (port/linux/src/touch_input.c), and
			what the game uses for its UI scale */
			float density = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
			char text[16];

			if (density > 0.0f)
			{
				snprintf(text, sizeof(text), "%g", density);
				environment_set(&environment, "HALO_DISPLAY_DENSITY", text);
				host_logf(HOST_LOG_INFO, "display density %s", text);
			}
		}
	}
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);
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

	{
		char seconds[32];

		if (config_sample_seconds(path, seconds, sizeof(seconds)))
			host_debug_start_sampler(seconds);
	}
	if (!config_boolean_or(path, "debug.memory_watch", 1))
	{
		host_memory_watch_use_hashes();
		host_logf(HOST_LOG_INFO, "write tracking: page hashes (debug.memory_watch = false)");
	}
	else
	{
		host_logf(HOST_LOG_INFO, "write tracking: page protection");
	}
	host_linux_set_paths(data_root, save_root);
	boot = make_boot(&environment);
	host_logf(HOST_LOG_INFO, "data %s, saves %s", data_root, save_root);
	host_run_guest_main(boot);
}

int main(int argc, char **argv)
{
	host_logf(HOST_LOG_INFO, "Halo for Linux starting");
	host_install_signal_handlers();
	resolve_roots(argc, argv);
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		pause();
}
