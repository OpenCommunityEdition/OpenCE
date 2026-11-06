/*
HOST_BOOT.C

The host's boot code, apart from the Android app's: finding the game data,
building the environment the guest starts with (argv, environment and the
page size, copied into guest memory), reading the few settings of config.toml
the host needs, and the POSIX time zone for the local offset.

host_main.c uses it; it is a file of its own so that a host that is not an
Android app can share it.
*/

#include "host.h"
#include "tomlc17.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int host_directory_has_maps(const char *root)
{
	char path[600];
	struct stat information;

	snprintf(path, sizeof(path), "%s/maps/ui.map", root);
	return stat(path, &information) == 0;
}

void host_environment_set(struct host_environment *environment, const char *name, const char *value)
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
	if (environment->count < HOST_ENVIRONMENT_MAXIMUM)
		environment->entries[environment->count++] = entry;
	else
		free(entry);
}

/* debug.sample_seconds from config.toml, as text for the sampler, or 0 */
int host_config_sample_seconds(const char *path, char *text, size_t size)
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
int host_config_boolean_or(const char *path, const char *name, int otherwise)
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
void host_time_zone(char *buffer, size_t size)
{
	time_t now = time(NULL);
	struct tm local;
	long offset;

	localtime_r(&now, &local);
	offset = -local.tm_gmtoff;
	snprintf(buffer, size, "<L>%s%ld:%02ld", offset < 0 ? "-" : "", labs(offset) / 3600, (labs(offset) / 60) % 60);
}

/* the display's size, refresh rate and density, for the guest's environment */
void host_environment_set_display(struct host_environment *environment)
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
		host_environment_set(environment, "HALO_DISPLAY_WIDTH", width);
		host_logf(HOST_LOG_INFO, "display %dx%d: rendering %sx480", mode->w, mode->h, width);
	}
	if (mode)
	{
		/* display.max_fps's "twice the refresh rate" (sdl_platform.c):
		the guest cannot ask SDL for the display mode */
		snprintf(refresh, sizeof(refresh), "%g", mode->refresh_rate > 0.0f ? mode->refresh_rate : 60.0f);
		host_environment_set(environment, "HALO_DISPLAY_REFRESH", refresh);
	}
	{
		/* dp for the touch controls (port/linux/src/touch_input.c), and what the
		game uses for its UI scale */
		float density = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
		char text[16];

		if (density > 0.0f)
		{
			snprintf(text, sizeof(text), "%g", density);
			host_environment_set(environment, "HALO_DISPLAY_DENSITY", text);
			host_logf(HOST_LOG_INFO, "display density %s", text);
		}
	}
}

/* starts the debug sampler if config.toml sets debug.sample_seconds */
void host_start_sampler_from_config(const char *config_path)
{
	char seconds[32];

	if (host_config_sample_seconds(config_path, seconds, sizeof(seconds)))
		host_debug_start_sampler(seconds);
}

/* copies argv and the environment into guest memory */
uint32_t host_make_boot(const struct host_environment *environment)
{
	size_t size = 0x10000;
	char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *argv = (uint32_t *)(memory + sizeof(*boot));
	uint32_t *environ_list = argv + 2;
	char *strings = (char *)(environ_list + HOST_ENVIRONMENT_MAXIMUM + 1);
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
