/*
HOST_LINUX_DATA.C

First start without game data on the Linux arm64 port: offers to extract the
maps folder from an Xbox disc image the player picks in a file dialog (the
x86 port's flow, port/linux/src/sdl_platform.c, with the same xiso.c).

Also defines platform_log, which xiso.c calls.
*/

#include "../../android/host/host.h"
#include "xiso.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void platform_log(const char *format, ...)
{
	char buffer[512];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(buffer, sizeof(buffer), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_INFO, "%s", buffer);
}

/* ---------- first start without game data */

struct data_extraction
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

static void data_extraction_progress(void *context, const char *file, unsigned long long done,
	unsigned long long total)
{
	struct data_extraction *extraction = context;

	pthread_mutex_lock(&extraction->lock);
	snprintf(extraction->file, sizeof(extraction->file), "%s", file);
	extraction->done = done;
	extraction->total = total;
	pthread_mutex_unlock(&extraction->lock);
}

static void *data_extraction_thread(void *context)
{
	struct data_extraction *extraction = context;
	int succeeded = xiso_extract_maps(extraction->image, extraction->destination, data_extraction_progress,
		extraction, extraction->error, sizeof(extraction->error)) != 0;

	pthread_mutex_lock(&extraction->lock);
	extraction->succeeded = succeeded;
	extraction->finished = 1;
	pthread_mutex_unlock(&extraction->lock);
	return NULL;
}

/* copies the maps, showing how far it has got; closing the window quits */
static int data_extract(const char *image, const char *destination, char *error, int error_size)
{
	static struct data_extraction extraction;
	SDL_Window *window;
	SDL_Renderer *renderer = NULL;
	pthread_t thread;
	int finished = 0;

	memset(&extraction, 0, sizeof(extraction));
	pthread_mutex_init(&extraction.lock, NULL);
	snprintf(extraction.image, sizeof(extraction.image), "%s", image);
	snprintf(extraction.destination, sizeof(extraction.destination), "%s", destination);
	if (pthread_create(&thread, NULL, data_extraction_thread, &extraction) != 0)
	{
		snprintf(error, (size_t)error_size, "Could not start the extraction.");
		return 0;
	}
	/* (waited for through extraction.finished) */
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
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			{
				platform_log("extraction cancelled");
				exit(EXIT_SUCCESS);
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
		snprintf(error, (size_t)error_size, "%s", extraction.error);
	return extraction.succeeded;
}

struct data_image_choice
{
	SDL_AtomicInt done;
	int failed;
	char path[1024];
	char error[256];
};

static void SDLCALL data_image_chosen(void *userdata, const char * const *files, int filter)
{
	struct data_image_choice *choice = userdata;

	(void)filter;
	if (!files)
	{
		/* no dialog could be shown (SDL needs the desktop portal through
		libdbus, or zenity) */
		choice->failed = 1;
		snprintf(choice->error, sizeof(choice->error), "%s",
			*SDL_GetError() ? SDL_GetError() : "no desktop portal or zenity");
	}
	else if (files[0])
	{
		snprintf(choice->path, sizeof(choice->path), "%s", files[0]);
	}
	SDL_SetAtomicInt(&choice->done, 1);
}

/* a disc image in the folder the maps are extracted to (next to the
executable), for systems where no file dialog can be shown */
static int data_find_image(const char *directory, char *path, int size)
{
	static const char *const patterns[] = { "*.iso", "*.xiso" };
	int pattern;

	for (pattern = 0; pattern < (int)(sizeof(patterns) / sizeof(patterns[0])); pattern++)
	{
		int count = 0;
		char **files = SDL_GlobDirectory(directory, patterns[pattern], SDL_GLOB_CASEINSENSITIVE, &count);

		if (files && count > 0)
		{
			snprintf(path, (size_t)size, "%s/%s", directory, files[0]);
			SDL_free(files);
			return 1;
		}
		SDL_free(files);
	}
	return 0;
}

/* the disc image the player picks; 0 if they pick none, with *failed
set (and error written) when no dialog could be shown */
static int data_choose_image(char *path, int size, int *failed, char *error, int error_size)
{
	static const SDL_DialogFileFilter filters[] =
	{
		{ "Xbox disc images", "iso;xiso" },
		{ "All files", "*" },
	};
	static struct data_image_choice choice;

	memset(&choice, 0, sizeof(choice));
	SDL_ShowOpenFileDialog(data_image_chosen, &choice, NULL, filters, 2, NULL, false);
	/* the dialog answers through events (and on some systems another
	thread) */
	while (!SDL_GetAtomicInt(&choice.done))
	{
		SDL_PumpEvents();
		SDL_Delay(50);
	}
	*failed = choice.failed;
	if (choice.failed)
		snprintf(error, (size_t)error_size, "%s", choice.error);
	if (!choice.path[0])
		return 0;
	snprintf(path, (size_t)size, "%s", choice.path);
	return 1;
}

/* 1 once <destination>/maps was extracted; otherwise the process exits */
int host_linux_offer_game_data(const char *destination)
{
	static const SDL_MessageBoxButtonData buttons[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
	};
	char message[2600];
	char found[1024];
	int image_found;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		platform_log("no game data prompt: %s", SDL_GetError());
		return 0;
	}
	image_found = data_find_image(destination, found, sizeof(found));
	if (image_found)
	{
		snprintf(message, sizeof(message),
			"Halo's game data (its maps folder) was not found.\n\n"
			"Extract the maps folder from the Xbox disc image %s? "
			"It is copied to %s/maps (about 2 GB).\n\n"
			"(Or put the maps folder there yourself, or set paths.data in config.toml.)",
			found, destination);
	}
	else
	{
		snprintf(message, sizeof(message),
			"Halo's game data (its maps folder) was not found.\n\n"
			"Extract the maps folder from an Xbox disc image (.iso) of Halo: Combat Evolved? "
			"It is copied to %s/maps (about 2 GB).\n\n"
			"(Or put the maps folder there yourself, or set paths.data in config.toml.)",
			destination);
	}
	for (;;)
	{
		SDL_MessageBoxData question = { SDL_MESSAGEBOX_INFORMATION, NULL, "Halo", message, 2, buttons, NULL };
		char image[1024];
		char error[512];
		int answer = 0;
		int failed = 0;

		if (!SDL_ShowMessageBox(&question, &answer) || answer != 1)
		{
			platform_log("no game data: quitting");
			exit(EXIT_SUCCESS);
		}
		if (image_found)
		{
			snprintf(image, sizeof(image), "%s", found);
		}
		else if (!data_choose_image(image, sizeof(image), &failed, error, sizeof(error)))
		{
			char explanation[1400];

			/* no image picked: ask again */
			if (!failed)
				continue;
			/* no dialog could be shown */
			platform_log("no file dialog: %s", error);
			snprintf(explanation, sizeof(explanation),
				"No file dialog could be shown (%s).\n\n"
				"Put the disc image (.iso) in %s and start Halo again, "
				"or install zenity.",
				error, destination);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", explanation, NULL);
			exit(EXIT_SUCCESS);
		}
		platform_log("extracting the maps folder from %s to %s", image, destination);
		if (data_extract(image, destination, error, sizeof(error)))
		{
			platform_log("extracted the maps folder");
			return 1;
		}
		platform_log("extraction failed: %s", error);
		image_found = 0;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", error, NULL);
	}
}
