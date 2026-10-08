/*
HOST_LAUNCHER.C

The macOS host's first start (host_services.h): where the game data and
the saves live, and the game data's maps folder, copied out of a Halo disc
image the player picks when the data root has none yet. This is the
desktop ports' offer (port/linux/src/sdl_platform.c,
platform_offer_game_data), made by the host before the guest starts, as
Android's launcher does: the guest has no offer (HALO_GUEST), since the file
dialog's callback and the message box's data cannot cross into it. It runs
on the process's main thread, which Cocoa's dialogs and windows need, while
the copy (port/linux/src/xiso.c, built into the host) runs on a thread of
its own.

The data root is ~/Library/Application Support/OpenCE: the settings
(config.toml), maps/, the game's log (debug.txt) and the rest the game
keeps beside its data, never inside the app, which is signed and may be
read-only; the saves are in its save/ folder. HALO_DATA_ROOT and
HALO_SAVE_ROOT, if set, put them elsewhere (a repository's assets, say).
*/

#include "host.h"
#include "host_services.h"
#include "posix.h"
#include "tomlc17.h"
#include "xiso.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char launcher_data_root[1024];

const char *host_launcher_data_root(void)
{
	return launcher_data_root;
}

/* xiso.c's log */
void platform_log(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_INFO, "%s", message);
}

/* ---------- paths */

static const char *home_directory(void)
{
	const char *home = getenv("HOME");
	struct passwd *entry;

	if (home && *home)
		return home;
	entry = getpwuid(getuid());
	return entry && entry->pw_dir && *entry->pw_dir ? entry->pw_dir : NULL;
}

/* creates every missing directory along path */
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

/* copies value into a caller's buffer; a path that does not fit is fatal
(a shortened one would be another folder) */
static void set_path(char *buffer, unsigned size, const char *value)
{
	size_t length = strlen(value);

	while (length > 1 && value[length - 1] == '/')
		length--;
	if (length >= size)
		host_fatal("the path %s is too long", value);
	memcpy(buffer, value, length);
	buffer[length] = 0;
}

/* whether root has the game's maps: maps/ui.map, in any case (a disk may
tell case apart) */
static int has_game_data(const char *root)
{
	char maps[256], ui[256], path[1400];

	if (!posix_find_entry_case_insensitive(root, "maps", maps, sizeof(maps)))
		return 0;
	snprintf(path, sizeof(path), "%s/%s", root, maps);
	return posix_find_entry_case_insensitive(path, "ui.map", ui, sizeof(ui));
}

/* a run nobody is watching (HALO_NO_DIALOGS; debug.hidden_window,
debug.exit_after, from the environment as the game reads them or from
config.toml): nothing is offered, as on the other desktops, and no dialog is
shown, which nobody would dismiss */
int host_unattended(const char *data_root)
{
	const char *hidden = getenv("HALO_HIDDEN_WINDOW");
	const char *exit_after = getenv("HALO_EXIT_AFTER");
	char path[1100];
	toml_result_t result;
	int found = 0;

	if (getenv("HALO_NO_DIALOGS") || (hidden && *hidden) || (exit_after && strtod(exit_after, NULL) > 0.0))
		return 1;
	if (!data_root || !*data_root)
		return 0;
	snprintf(path, sizeof(path), "%s/config.toml", data_root);
	if (access(path, R_OK) != 0)
		return 0;
	result = toml_parse_file_ex(path);
	if (!result.ok)
		return 0;
	{
		toml_datum_t hidden_window = toml_seek(result.toptab, "debug.hidden_window");
		toml_datum_t seconds = toml_seek(result.toptab, "debug.exit_after");

		found = (hidden_window.type == TOML_BOOLEAN && hidden_window.u.boolean) ||
			(seconds.type == TOML_FP64 && seconds.u.fp64 > 0.0) ||
			(seconds.type == TOML_INT64 && seconds.u.int64 > 0);
	}
	toml_free(result);
	return found;
}

/* ---------- copying the maps out of a disc image */

struct extraction
{
	pthread_mutex_t lock;
	char image[1024];
	char destination[1024];
	char file[256];
	unsigned long long done;
	unsigned long long total;
	int finished;
	int succeeded;
	char error[512];
};

static void extraction_progress(void *context, const char *file, unsigned long long done, unsigned long long total)
{
	struct extraction *extraction = context;

	pthread_mutex_lock(&extraction->lock);
	snprintf(extraction->file, sizeof(extraction->file), "%s", file);
	extraction->done = done;
	extraction->total = total;
	pthread_mutex_unlock(&extraction->lock);
}

static void *extraction_thread(void *context)
{
	struct extraction *extraction = context;
	int succeeded;

	/* (a crash here is reported and ends the process, as on the others) */
	host_signal_stack_install();
	succeeded = xiso_extract_maps(extraction->image, extraction->destination, extraction_progress, extraction,
		extraction->error, sizeof(extraction->error)) != 0;

	pthread_mutex_lock(&extraction->lock);
	extraction->succeeded = succeeded;
	extraction->finished = 1;
	pthread_mutex_unlock(&extraction->lock);
	return NULL;
}

enum
{
	_extract_failed,
	_extract_succeeded,
	_extract_cancelled,
};

/* copies the maps, showing how far it has got in a small window; closing
it (or quitting) cancels, and the caller then ends the process, the copy
with it: the partial copy is maps.partial, never maps */
static int extract(const char *image, const char *destination, char *error, int error_size)
{
	/* (the thread may outlive a cancel) */
	static struct extraction extraction;
	SDL_Window *window;
	SDL_Renderer *renderer = NULL;
	pthread_t thread;
	int finished = 0;

	memset(&extraction, 0, sizeof(extraction));
	pthread_mutex_init(&extraction.lock, NULL);
	snprintf(extraction.image, sizeof(extraction.image), "%s", image);
	snprintf(extraction.destination, sizeof(extraction.destination), "%s", destination);
	if (pthread_create(&thread, NULL, extraction_thread, &extraction) != 0)
	{
		snprintf(error, (size_t)error_size, "Could not start the extraction.");
		return _extract_failed;
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
		char file[256];
		unsigned long long done, total;

		while (SDL_PollEvent(&event))
		{
			if (host_sdl_take_event(&event))
				continue;
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			{
				platform_log("extraction cancelled");
				return _extract_cancelled;
			}
		}
		pthread_mutex_lock(&extraction.lock);
		finished = extraction.finished;
		snprintf(file, sizeof(file), "%s", extraction.file);
		done = extraction.done;
		total = extraction.total;
		pthread_mutex_unlock(&extraction.lock);
		if (renderer)
		{
			char line[320];
			SDL_FRect bar = { 20.0f, 100.0f, 600.0f, 24.0f };
			float fraction = total ? (float)((double)done / (double)total) : 0.0f;

			SDL_SetRenderDrawColor(renderer, 12, 16, 20, 255);
			SDL_RenderClear(renderer);
			SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
			SDL_SetRenderScale(renderer, 2.0f, 2.0f);
			SDL_RenderDebugText(renderer, 10.0f, 10.0f, "Extracting the maps folder...");
			SDL_SetRenderScale(renderer, 1.0f, 1.0f);
			snprintf(line, sizeof(line), "%s  (%llu of %llu MB)", file, done >> 20, total >> 20);
			SDL_RenderDebugText(renderer, 20.0f, 70.0f, line);
			SDL_SetRenderDrawColor(renderer, 60, 66, 72, 255);
			SDL_RenderFillRect(renderer, &bar);
			bar.w *= fraction;
			SDL_SetRenderDrawColor(renderer, 90, 160, 90, 255);
			SDL_RenderFillRect(renderer, &bar);
			SDL_RenderPresent(renderer);
		}
		SDL_Delay(16);
	}
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
	if (!extraction.succeeded)
	{
		snprintf(error, (size_t)error_size, "%s", extraction.error);
		return _extract_failed;
	}
	return _extract_succeeded;
}

/* ---------- asking the player */

struct image_choice
{
	SDL_AtomicInt done;
	char path[1024];
};

static void SDLCALL image_chosen(void *userdata, const char * const *files, int filter)
{
	struct image_choice *choice = userdata;

	(void)filter;
	if (files && files[0])
		snprintf(choice->path, sizeof(choice->path), "%s", files[0]);
	else if (!files)
		platform_log("the file dialog failed: %s", SDL_GetError());
	SDL_SetAtomicInt(&choice->done, 1);
}

/* the disc image the player picks; 0 if they pick none */
static int choose_image(char *path, size_t size)
{
	static const SDL_DialogFileFilter filters[] =
	{
		{ "Xbox disc images", "iso;xiso" },
	};
	static struct image_choice choice;

	memset(&choice, 0, sizeof(choice));
	SDL_ShowOpenFileDialog(image_chosen, &choice, NULL, filters, 1, NULL, false);
	/* (Cocoa's dialog answers before it returns; others through events.
	Pumped, not polled: the events stay queued for the game, a link the
	app was opened with among them) */
	while (!SDL_GetAtomicInt(&choice.done))
	{
		SDL_PumpEvents();
		SDL_Delay(20);
	}
	if (!choice.path[0])
		return 0;
	snprintf(path, size, "%s", choice.path);
	return 1;
}

/* asks whether to extract the maps; 1 if so, 0 to quit */
static int ask_to_extract(const char *data_root)
{
	static const SDL_MessageBoxButtonData buttons[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Choose Disc Image..." },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit" },
	};
	char message[1600];
	SDL_MessageBoxData question = { SDL_MESSAGEBOX_INFORMATION, NULL, "Halo", NULL, 2, buttons, NULL };
	int answer = 0;

	snprintf(message, sizeof(message),
		"Halo's game data (its maps folder) was not found.\n\n"
		"Extract the maps folder from an Xbox disc image (.iso) of Halo: Combat Evolved? "
		"It is copied to %s/maps (about 2 GB).\n\n"
		"(Or quit and put the maps folder there yourself.)",
		data_root);
	question.message = message;
	if (!SDL_ShowMessageBox(&question, &answer))
	{
		platform_log("cannot ask about the game data: %s", SDL_GetError());
		return 0;
	}
	return answer == 1;
}

/* makes way for the copy, whose last step renames maps.partial to maps: a
maps folder without ui.map (a partial copy by hand, or one that holds only
the .DS_Store Finder leaves) is removed if empty, else moved aside to
maps.incomplete, never deleted. 0, with error, if it cannot be moved */
static int clear_old_maps(const char *root, char *error, size_t error_size)
{
	char on_disk[256], path[1400], aside[1400];
	int index;

	if (!posix_find_entry_case_insensitive(root, "maps", on_disk, sizeof(on_disk)))
		return 1;
	snprintf(path, sizeof(path), "%s/%s/.DS_Store", root, on_disk);
	unlink(path);
	snprintf(path, sizeof(path), "%s/%s", root, on_disk);
	if (rmdir(path) == 0)
		return 1;
	for (index = 0; index < 100; index++)
	{
		if (index)
			snprintf(aside, sizeof(aside), "%s/maps.incomplete-%d", root, index);
		else
			snprintf(aside, sizeof(aside), "%s/maps.incomplete", root);
		if (access(aside, F_OK) != 0)
			break;
	}
	if (index < 100 && rename(path, aside) == 0)
	{
		platform_log("moved the incomplete %s aside to %s", path, aside);
		return 1;
	}
	snprintf(error, error_size, "%s has no ui.map and cannot be moved aside (%s). Move or delete it, then try again.",
		path, strerror(errno));
	return 0;
}


/* ---------- the start */

int host_launcher_prepare(char *data_root, unsigned data_root_size, char *save_root, unsigned save_root_size)
{
	const char *data = getenv("HALO_DATA_ROOT");
	const char *saves = getenv("HALO_SAVE_ROOT");
	char path[1100];

	if (data && *data)
	{
		set_path(data_root, data_root_size, data);
	}
	else
	{
		const char *home = home_directory();

		if (!home)
			host_fatal("cannot find this user's home folder");
		snprintf(path, sizeof(path), "%s/Library/Application Support/OpenCE", home);
		set_path(data_root, data_root_size, path);
	}
	if (saves && *saves)
	{
		set_path(save_root, save_root_size, saves);
	}
	else
	{
		snprintf(path, sizeof(path), "%s/save", data_root);
		set_path(save_root, save_root_size, path);
	}
	make_directories(data_root);
	make_directories(save_root);
	snprintf(launcher_data_root, sizeof(launcher_data_root), "%s", data_root);
	if (has_game_data(data_root))
		return 0;
	host_logf(HOST_LOG_INFO, "no maps/ in %s", data_root);
	if (host_unattended(data_root))
		return 0;
	SDL_SetHint(SDL_HINT_APP_NAME, "Halo");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		host_logf(HOST_LOG_ERROR, "SDL_Init failed: %s", SDL_GetError());
		return 0;
	}
	for (;;)
	{
		char image[1024];
		char error[512];

		if (!ask_to_extract(data_root))
		{
			platform_log("no game data: quitting");
			return 1;
		}
		/* no image picked: ask again */
		if (!choose_image(image, sizeof(image)))
			continue;
		if (!clear_old_maps(data_root, error, sizeof(error)))
		{
			platform_log("%s", error);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", error, NULL);
			continue;
		}
		platform_log("extracting the maps folder from %s to %s", image, data_root);
		switch (extract(image, data_root, error, sizeof(error)))
		{
		case _extract_cancelled:
			return 1;
		case _extract_succeeded:
			if (has_game_data(data_root))
			{
				platform_log("extracted the maps folder");
				return 0;
			}
			snprintf(error, sizeof(error), "The maps folder was copied, but %s has no maps/ui.map.", data_root);
			break;
		default:
			break;
		}
		platform_log("extraction failed: %s", error);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", error, NULL);
	}
}
