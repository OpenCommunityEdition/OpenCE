/*
MCC_INSTALL.C

The game's data from a Steam installation of Halo: The Master Chief
Collection, on a first start without it (sdl_platform.c,
platform_offer_game_data): its Halo 1 folder, found through Steam's
libraries, and the import of its maps as the Xbox's, which other copies of
the game run (halo --import-mcc d:\maps --from <folder> <map>,
port/linux/game/mcc_import.c), one map each and several at once, while this
one shows how far they have got.
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

enum
{
	/* the imports run at once: each takes a core and about 0.5 GB (a
	campaign map's sounds take it 30 seconds), and they all read MCC's
	sound banks; with more than the ten campaign maps, the import takes no
	less than the longest of them */
	MCC_INSTALL_MAXIMUM_JOBS = 12,
	MCC_INSTALL_MAXIMUM_MAPS = 64,
};

/* one import, of one map */
struct mcc_install_worker
{
	SDL_Process *process;
	SDL_IOStream *output;
	/* the map's index in mcc_install.names */
	int map;
	/* (the reader's) the map's line of a map written, or of one that failed,
	and the end of the output */
	BOOL written;
	BOOL failed;
	BOOL finished;
};

static struct
{
	pthread_mutex_t lock;
	char names[MCC_INSTALL_MAXIMUM_MAPS][64];
	/* the size of each map's file in MCC: the larger first, and the
	progress counted by them */
	Sint64 sizes[MCC_INSTALL_MAXIMUM_MAPS];
	struct mcc_install_worker workers[MCC_INSTALL_MAXIMUM_JOBS];
	char last_error[512];
} mcc_install;

/* one line of a map's import: the map written ("mcc import: <map>: N
structure bsps, ..."), or failed ("mcc import: <map>: failed"; its line kept
for the error) */
static void mcc_install_line(struct mcc_install_worker *worker, const char *line)
{
	const char *name = mcc_install.names[worker->map];
	size_t length = strlen(name);

	if (strncmp(line, "mcc import: ", 12) || strncmp(line + 12, name, length) || line[12 + length] != ':')
		return;
	pthread_mutex_lock(&mcc_install.lock);
	if (strstr(line + 12 + length, " structure bsps, "))
		worker->written = TRUE;
	if (!strcmp(line + 12 + length, ": failed"))
	{
		worker->failed = TRUE;
		snprintf(mcc_install.last_error, sizeof(mcc_install.last_error), "%s", line);
	}
	pthread_mutex_unlock(&mcc_install.lock);
}

static void *mcc_install_reader(void *context)
{
	struct mcc_install_worker *worker = context;
	char line[1024];
	int used = 0;

	for (;;)
	{
		char byte;

		if (SDL_ReadIO(worker->output, &byte, 1) != 1)
		{
			/* (the stream does not wait for the copy: nothing yet is no end) */
			if (SDL_GetIOStatus(worker->output) == SDL_IO_STATUS_NOT_READY)
			{
				SDL_Delay(10);
				continue;
			}
			break;
		}
		if (byte == '\n' || used == (int)sizeof(line) - 1)
		{
			line[used] = 0;
			if (used && line[used - 1] == '\r')
				line[used - 1] = 0;
			/* (one line at a time in the log, of all the imports) */
			pthread_mutex_lock(&mcc_install.lock);
			platform_log("%s", line);
			pthread_mutex_unlock(&mcc_install.lock);
			mcc_install_line(worker, line);
			used = 0;
		}
		else
			line[used++] = byte;
	}
	/* (the last the reader does with the worker, which is then free) */
	pthread_mutex_lock(&mcc_install.lock);
	worker->finished = TRUE;
	pthread_mutex_unlock(&mcc_install.lock);
	return NULL;
}

/* the import of the map `map` started in `worker`, whose output a reader
follows: FALSE if it could not be, its error in mcc_install.last_error */
static BOOL mcc_install_start(struct mcc_install_worker *worker, int map, const char *executable,
	const char *halo1, SDL_Environment *environment)
{
	const char *arguments[7];
	SDL_PropertiesID properties;
	pthread_t thread;

	memset(worker, 0, sizeof(*worker));
	worker->map = map;
	arguments[0] = executable;
	arguments[1] = "--import-mcc";
	arguments[2] = "d:\\maps";
	arguments[3] = "--from";
	arguments[4] = halo1;
	arguments[5] = mcc_install.names[map];
	arguments[6] = NULL;
	properties = SDL_CreateProperties();
	SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)arguments);
	/* (the environment itself, not its variables) */
	if (environment)
		SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, environment);
	SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
	SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
	worker->process = SDL_CreateProcessWithProperties(properties);
	SDL_DestroyProperties(properties);
	if (!worker->process)
	{
		snprintf(mcc_install.last_error, sizeof(mcc_install.last_error), "The import could not be started: %s",
			SDL_GetError());
		return FALSE;
	}
	worker->output = SDL_GetProcessOutput(worker->process);
	if (pthread_create(&thread, NULL, mcc_install_reader, worker) != 0)
	{
		SDL_KillProcess(worker->process, true);
		SDL_WaitProcess(worker->process, true, NULL);
		SDL_DestroyProcess(worker->process);
		worker->process = NULL;
		snprintf(mcc_install.last_error, sizeof(mcc_install.last_error), "The import could not be followed.");
		return FALSE;
	}
	pthread_detach(thread);
	return TRUE;
}

/* the imports to run at once: one core left to the rest, and 1 GB of memory
each */
static int mcc_install_jobs(int total)
{
	int jobs = SDL_GetNumLogicalCPUCores() - 1;
	int memory = SDL_GetSystemRAM() / 1024;

	if (memory > 0 && jobs > memory)
		jobs = memory;
	if (jobs > MCC_INSTALL_MAXIMUM_JOBS)
		jobs = MCC_INSTALL_MAXIMUM_JOBS;
	if (jobs > total)
		jobs = total;
	return jobs < 1 ? 1 : jobs;
}

/* the maps in the order they are imported, the larger first (so that the
last to end is a small one), and each one's size in MCC */
static void mcc_install_order(const char *halo1, int total)
{
	int index;

	for (index = 0; index < total; index++)
	{
		char path[1200];
		SDL_PathInfo information;

		snprintf(path, sizeof(path), "%s/maps/%s.map", halo1, mcc_install.names[index]);
		mcc_install.sizes[index] = SDL_GetPathInfo(path, &information) ? (Sint64)information.size : 0;
		/* (inserted among those before it, kept in their order where equal) */
		{
			int other;
			char name[64];
			Sint64 size = mcc_install.sizes[index];

			memcpy(name, mcc_install.names[index], sizeof(name));
			for (other = index; other > 0 && mcc_install.sizes[other - 1] < size; other--)
			{
				memcpy(mcc_install.names[other], mcc_install.names[other - 1], sizeof(name));
				mcc_install.sizes[other] = mcc_install.sizes[other - 1];
			}
			memcpy(mcc_install.names[other], name, sizeof(name));
			mcc_install.sizes[other] = size;
		}
	}
}

/* the map file the import of `map` writes, removed: one that did not end
(a partial file is no map) */
static void mcc_install_remove_map(const char *destination, int map)
{
	char path[1200];

	snprintf(path, sizeof(path), "%s/maps/%s.map", destination, mcc_install.names[map]);
	SDL_RemovePath(path);
}

BOOL mcc_install_import(const char *halo1, const char *destination, char *error, int error_size)
{
	char executable[1024], maps[1100];
	SDL_Environment *environment;
	SDL_Window *window;
	SDL_Renderer *renderer = NULL;
	int total, jobs, next = 0, running = 0, done = 0, failed = 0, index;
	Sint64 total_bytes = 0, done_bytes = 0;

	memset(&mcc_install, 0, sizeof(mcc_install));
	total = mcc_import_default_maps(mcc_install.names, MCC_INSTALL_MAXIMUM_MAPS);
	if (!total || !update_executable_path(executable, sizeof(executable)))
	{
		snprintf(error, (size_t)error_size, "This build cannot import the Master Chief Collection's maps.");
		return FALSE;
	}
	pthread_mutex_init(&mcc_install.lock, NULL);
	mcc_install_order(halo1, total);
	for (index = 0; index < total; index++)
		total_bytes += mcc_install.sizes[index] > 0 ? mcc_install.sizes[index] : 1;
	jobs = mcc_install_jobs(total);
	platform_log("mcc install: %d maps, %d at once", total, jobs);
	snprintf(maps, sizeof(maps), "%s/maps", destination);
	SDL_CreateDirectory(maps);
	/* (the copies' data root is the folder the maps are written to) */
	environment = SDL_CreateEnvironment(true);
	if (environment)
		SDL_SetEnvironmentVariable(environment, "HALO_DATA_ROOT", destination, true);
	window = SDL_CreateWindow("Halo", 640, 150, 0);
	if (window)
	{
		renderer = SDL_CreateRenderer(window, NULL);
		if (renderer)
			SDL_SetRenderVSync(renderer, 1);
	}
	while (next < total || running)
	{
		SDL_Event event;
		char current[80], line[320];
		int worker_index, used = 0;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			{
				platform_log("import cancelled");
				for (worker_index = 0; worker_index < jobs; worker_index++)
				{
					struct mcc_install_worker *worker = &mcc_install.workers[worker_index];

					if (!worker->process)
						continue;
					SDL_KillProcess(worker->process, true);
					SDL_WaitProcess(worker->process, true, NULL);
					mcc_install_remove_map(destination, worker->map);
				}
				/* (the maps folder, if nothing is written to it: the next
				start offers the game data again) */
				SDL_RemovePath(maps);
				exit(EXIT_SUCCESS);
			}
		}
		/* the imports that ended, counted, and the next maps' started in
		their places */
		for (worker_index = 0; worker_index < jobs; worker_index++)
		{
			struct mcc_install_worker *worker = &mcc_install.workers[worker_index];
			BOOL finished, written;
			int exit_code = 1;

			if (worker->process)
			{
				pthread_mutex_lock(&mcc_install.lock);
				finished = worker->finished;
				written = worker->written && !worker->failed;
				pthread_mutex_unlock(&mcc_install.lock);
				if (!finished)
					continue;
				SDL_WaitProcess(worker->process, true, &exit_code);
				SDL_DestroyProcess(worker->process);
				worker->process = NULL;
				running--;
				/* (a map is written when its import says so and ends well: one
				that stops without a word, as a fatal error can, is not) */
				if (written && exit_code == 0)
				{
					done++;
					done_bytes += mcc_install.sizes[worker->map] > 0 ? mcc_install.sizes[worker->map] : 1;
				}
				else
				{
					failed++;
					mcc_install_remove_map(destination, worker->map);
					pthread_mutex_lock(&mcc_install.lock);
					if (!worker->failed)
					{
						snprintf(mcc_install.last_error, sizeof(mcc_install.last_error),
							"mcc import: %s: ended (exit code %d) without writing it", mcc_install.names[worker->map],
							exit_code);
					}
					pthread_mutex_unlock(&mcc_install.lock);
				}
			}
			if (next < total)
			{
				if (mcc_install_start(worker, next, executable, halo1, environment))
					running++;
				else
					failed++;
				next++;
			}
		}
		/* the maps being imported */
		current[0] = 0;
		for (worker_index = 0; worker_index < jobs; worker_index++)
		{
			const struct mcc_install_worker *worker = &mcc_install.workers[worker_index];

			if (worker->process && used < (int)sizeof(current) - 1)
			{
				used += snprintf(current + used, sizeof(current) - (size_t)used, "%s%s", used ? ", " : "",
					mcc_install.names[worker->map]);
				if (used >= (int)sizeof(current))
					used = (int)sizeof(current) - 1;
			}
		}
		if (renderer)
		{
			SDL_FRect bar = { 20.0f, 100.0f, 600.0f, 24.0f };

			SDL_SetRenderDrawColor(renderer, 12, 16, 20, 255);
			SDL_RenderClear(renderer);
			SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
			SDL_SetRenderScale(renderer, 2.0f, 2.0f);
			SDL_RenderDebugText(renderer, 10.0f, 10.0f, "Importing the maps from MCC...");
			SDL_SetRenderScale(renderer, 1.0f, 1.0f);
			snprintf(line, sizeof(line), "%d of %d maps", done, total);
			SDL_RenderDebugText(renderer, 20.0f, 60.0f, line);
			/* (as many of their names as the line holds) */
			snprintf(line, sizeof(line), "%.72s", current);
			SDL_RenderDebugText(renderer, 20.0f, 78.0f, line);
			SDL_SetRenderDrawColor(renderer, 60, 66, 72, 255);
			SDL_RenderFillRect(renderer, &bar);
			/* (by the maps' sizes: a campaign map takes ten times a
			multiplayer map's time) */
			bar.w *= (float)((double)done_bytes / (double)total_bytes);
			SDL_SetRenderDrawColor(renderer, 90, 160, 90, 255);
			SDL_RenderFillRect(renderer, &bar);
			SDL_RenderPresent(renderer);
		}
		SDL_Delay(16);
	}
	if (environment)
		SDL_DestroyEnvironment(environment);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
	if (failed || done < total)
	{
		/* (removed only if empty) */
		SDL_RemovePath(maps);
		snprintf(error, (size_t)error_size, "%d of %d maps could not be imported (the log says why). %s",
			total - done, total, mcc_install.last_error);
		return FALSE;
	}
	return TRUE;
}

#endif
