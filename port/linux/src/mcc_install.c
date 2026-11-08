/*
MCC_INSTALL.C

The game's data from a Steam installation of Halo: The Master Chief
Collection, on a first start without it (sdl_platform.c,
platform_offer_game_data): its Halo 1 folder, found through Steam's
libraries, and the import of its maps as the Xbox's, which a second copy of
the game runs (halo --import-mcc d:\maps --from <folder>,
port/linux/game/mcc_import.c) while this one shows how far it has got.
*/

#include "platform.h"
#include "posix.h"
#include "update.h"

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* port/linux/game/mcc_import_patch.c */
int mcc_import_default_maps(char names[][64], int maximum_count);

#define MCC_HALO1_FOLDER "steamapps/common/Halo The Master Chief Collection/halo1"

/* ---------- finding it */

static BOOL mcc_install_halo1(const char *library, char *path, int size)
{
	char map[2048];
	SDL_PathInfo information;

	snprintf(path, (size_t)size, "%s/%s", library, MCC_HALO1_FOLDER);
	snprintf(map, sizeof(map), "%s/maps/a10.map", path);
	return SDL_GetPathInfo(map, &information) && information.type == SDL_PATHTYPE_FILE;
}

BOOL mcc_install_find(char *path, int size)
{
	char steam[1024], listing[1100];
	char *text;
	const char *cursor;
	size_t length = 0;

	if (!posix_steam_root(steam, sizeof(steam)))
		return FALSE;
	if (mcc_install_halo1(steam, path, size))
		return TRUE;
	/* the other libraries: steamapps/libraryfolders.vdf, whose "path" keys
	name them (with their backslashes doubled) */
	snprintf(listing, sizeof(listing), "%s/steamapps/libraryfolders.vdf", steam);
	text = SDL_LoadFile(listing, &length);
	for (cursor = text; cursor && (cursor = strstr(cursor, "\"path\"")) != NULL; )
	{
		char library[1024];
		int used = 0;

		cursor = strchr(cursor + 6, '"');
		if (!cursor)
			break;
		for (cursor++; *cursor && *cursor != '"' && used < (int)sizeof(library) - 1; cursor++)
		{
			if (*cursor == '\\' && cursor[1] == '\\')
				cursor++;
			library[used++] = *cursor;
		}
		library[used] = 0;
		if (used && mcc_install_halo1(library, path, size))
		{
			SDL_free(text);
			return TRUE;
		}
	}
	SDL_free(text);
	return FALSE;
}

/* ---------- the import */

static struct
{
	pthread_mutex_t lock;
	SDL_IOStream *output;
	int done;
	int failed;
	char current[64];
	char last_error[512];
	BOOL finished;
} mcc_install;

/* one line of the import's output: a map written ("<map>: N structure
bsps, ..."), or one that failed */
static void mcc_install_line(const char *line)
{
	const char *name = strncmp(line, "mcc import: ", 12) ? NULL : line + 12;
	const char *colon = name ? strchr(name, ':') : NULL;

	if (!colon || colon - name >= (long)sizeof(mcc_install.current))
		return;
	pthread_mutex_lock(&mcc_install.lock);
	memcpy(mcc_install.current, name, (size_t)(colon - name));
	mcc_install.current[colon - name] = 0;
	if (strstr(colon, " structure bsps, "))
		mcc_install.done++;
	else if (!strcmp(colon, ": failed"))
	{
		mcc_install.done++;
		mcc_install.failed++;
		snprintf(mcc_install.last_error, sizeof(mcc_install.last_error), "%s", line);
	}
	pthread_mutex_unlock(&mcc_install.lock);
}

static void *mcc_install_reader(void *context)
{
	char line[1024];
	int used = 0;

	(void)context;
	for (;;)
	{
		char byte;

		if (SDL_ReadIO(mcc_install.output, &byte, 1) != 1)
			break;
		if (byte == '\n' || used == (int)sizeof(line) - 1)
		{
			line[used] = 0;
			if (used && line[used - 1] == '\r')
				line[used - 1] = 0;
			platform_log("%s", line);
			mcc_install_line(line);
			used = 0;
		}
		else
			line[used++] = byte;
	}
	pthread_mutex_lock(&mcc_install.lock);
	mcc_install.finished = TRUE;
	pthread_mutex_unlock(&mcc_install.lock);
	return NULL;
}

BOOL mcc_install_import(const char *halo1, const char *destination, char *error, int error_size)
{
	static char names[64][64];
	char executable[1024], maps[1100];
	const char *arguments[7];
	SDL_Environment *environment;
	char **variables;
	SDL_PropertiesID properties;
	SDL_Process *process;
	SDL_Window *window;
	SDL_Renderer *renderer = NULL;
	pthread_t thread;
	int total = mcc_import_default_maps(names, 64), exit_code = 1;
	BOOL finished = FALSE;

	if (!total || !update_executable_path(executable, sizeof(executable)))
	{
		snprintf(error, (size_t)error_size, "This build cannot import the Master Chief Collection's maps.");
		return FALSE;
	}
	snprintf(maps, sizeof(maps), "%s/maps", destination);
	SDL_CreateDirectory(maps);
	arguments[0] = executable;
	arguments[1] = "--import-mcc";
	arguments[2] = "d:\\maps";
	arguments[3] = "--from";
	arguments[4] = halo1;
	arguments[5] = NULL;
	/* (the copy's data root is the folder the maps are written to) */
	environment = SDL_CreateEnvironment(true);
	if (environment)
		SDL_SetEnvironmentVariable(environment, "HALO_DATA_ROOT", destination, true);
	variables = environment ? SDL_GetEnvironmentVariables(environment) : NULL;
	properties = SDL_CreateProperties();
	SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)arguments);
	if (variables)
		SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, variables);
	SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
	SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
	process = SDL_CreateProcessWithProperties(properties);
	SDL_DestroyProperties(properties);
	SDL_free(variables);
	if (environment)
		SDL_DestroyEnvironment(environment);
	if (!process)
	{
		snprintf(error, (size_t)error_size, "The import could not be started: %s", SDL_GetError());
		return FALSE;
	}
	memset(&mcc_install, 0, sizeof(mcc_install));
	pthread_mutex_init(&mcc_install.lock, NULL);
	mcc_install.output = SDL_GetProcessOutput(process);
	if (pthread_create(&thread, NULL, mcc_install_reader, NULL) != 0)
	{
		SDL_KillProcess(process, true);
		SDL_DestroyProcess(process);
		snprintf(error, (size_t)error_size, "The import could not be followed.");
		return FALSE;
	}
	pthread_detach(thread);
	window = SDL_CreateWindow("Halo", 640, 150, 0);
	if (window)
	{
		renderer = SDL_CreateRenderer(window, NULL);
		if (renderer)
			SDL_SetRenderVSync(renderer, 1);
	}
	while (!finished)
	{
		SDL_Event event;
		char current[64], line[320];
		int done;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			{
				platform_log("import cancelled");
				SDL_KillProcess(process, true);
				exit(EXIT_SUCCESS);
			}
		}
		pthread_mutex_lock(&mcc_install.lock);
		finished = mcc_install.finished;
		done = mcc_install.done;
		snprintf(current, sizeof(current), "%s", mcc_install.current);
		pthread_mutex_unlock(&mcc_install.lock);
		if (renderer)
		{
			SDL_FRect bar = { 20.0f, 100.0f, 600.0f, 24.0f };

			SDL_SetRenderDrawColor(renderer, 12, 16, 20, 255);
			SDL_RenderClear(renderer);
			SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
			SDL_SetRenderScale(renderer, 2.0f, 2.0f);
			SDL_RenderDebugText(renderer, 10.0f, 10.0f, "Importing the maps from MCC...");
			SDL_SetRenderScale(renderer, 1.0f, 1.0f);
			snprintf(line, sizeof(line), "%s  (%d of %d maps)", current, done, total);
			SDL_RenderDebugText(renderer, 20.0f, 70.0f, line);
			SDL_SetRenderDrawColor(renderer, 60, 66, 72, 255);
			SDL_RenderFillRect(renderer, &bar);
			bar.w *= (float)done / (float)total;
			SDL_SetRenderDrawColor(renderer, 90, 160, 90, 255);
			SDL_RenderFillRect(renderer, &bar);
			SDL_RenderPresent(renderer);
		}
		SDL_Delay(16);
	}
	SDL_WaitProcess(process, true, &exit_code);
	SDL_DestroyProcess(process);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
	if (exit_code != 0 || mcc_install.failed)
	{
		snprintf(error, (size_t)error_size, "%d of %d maps could not be imported (the log says why). %s",
			mcc_install.failed ? mcc_install.failed : total - mcc_install.done, total, mcc_install.last_error);
		return FALSE;
	}
	return TRUE;
}

#endif
