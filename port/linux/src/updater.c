/*
UPDATER.C

The desktop ports' self-updater (Linux and Windows; the Android app updates
itself in Java, port/android).

A build of the main branch made by GitHub Actions knows its build number
(HALO_BUILD_NUMBER, the workflow's run number, which names its release:
build-<number>); other builds have none and never look for updates. When
update.auto in config.toml is true (the default), the game asks GitHub for
the latest release when it starts, on a thread of its own: the game starts
meanwhile, and nothing happens if the release is not newer or cannot be
reached. If it is newer, the game asks whether to update:

- Yes: the release's build for this platform and configuration
  (halo-<platform>-<release|debug>.zip) is downloaded next to the executable
  (into update.partial/) and unpacked, its files put in place of the running
  game's (which become <name>.old, deleted at the next start), and the new
  game started; this one quits.
- No: nothing, until the next start.
- Do not ask again: after the player confirms it, update.auto = false is
  written to config.toml.

The system side (the HTTPS download, the files, starting the new game) is
update.h's: posix_update.c on Linux, win32_update.c on Windows.
*/

#include "platform.h"
#include "port_config.h"
#include "update.h"

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)

#include "zlib_prefixed.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* (given for this file by the build: tools/linux_build.py, windows_build.py) */
#ifndef HALO_BUILD_NUMBER
#define HALO_BUILD_NUMBER 0
#endif
#ifndef HALO_BUILD_FLAVOR
#define HALO_BUILD_FLAVOR "release"
#endif

#define UPDATE_REPOSITORY "OpenCommunityEdition/OpenCE"
#ifdef _WIN32
#define UPDATE_PLATFORM "windows"
#define PATH_SEPARATOR "\\"
#else
#define UPDATE_PLATFORM "linux"
#define PATH_SEPARATOR "/"
#endif
#define UPDATE_ASSET "halo-" UPDATE_PLATFORM "-" HALO_BUILD_FLAVOR ".zip"
#define UPDATE_DIRECTORY "update.partial"
#define MAXIMUM_UPDATE_FILES 32
/* the most a download may be: GitHub's answer about the latest release, a
release's zip (about 30 MB) */
#define MAXIMUM_CHECK_SIZE (1024ULL * 1024)
#define MAXIMUM_UPDATE_SIZE (256ULL * 1024 * 1024)

enum
{
	_updater_idle,
	_updater_checking,
	_updater_available,
	_updater_handled,
};

static SDL_AtomicInt updater_state;
static long updater_latest_build;
static char updater_directory[1024];
static char updater_executable[1024];

/* ---------- paths */

static void updater_path(char *path, size_t size, const char *name)
{
	snprintf(path, size, "%s" PATH_SEPARATOR "%s", updater_directory, name);
}

static void updater_partial_path(char *path, size_t size, const char *name)
{
	snprintf(path, size, "%s" PATH_SEPARATOR UPDATE_DIRECTORY PATH_SEPARATOR "%s", updater_directory, name);
}

/* ---------- the zip file (the release's), stored or deflated entries */

static unsigned long zip_word(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8;
}

static unsigned long zip_long(const unsigned char *bytes)
{
	return zip_word(bytes) | zip_word(bytes + 2) << 16;
}

/* an entry's data, unpacked, to the file at path; why it could not be is
written to reason */
static int zip_extract_entry(SDL_IOStream *zip, unsigned long local_offset, int method,
	unsigned long packed_size, unsigned long size, unsigned long crc, const char *path,
	char *reason, size_t reason_size)
{
	unsigned char header[30];
	unsigned char input[16384], output[16384];
	SDL_IOStream *file;
	z_stream stream;
	unsigned long remaining = packed_size, written = 0, checksum = crc32(0L, Z_NULL, 0);
	int succeeded = 0, ended = 0;

	if (SDL_SeekIO(zip, (Sint64)local_offset, SDL_IO_SEEK_SET) < 0 ||
		SDL_ReadIO(zip, header, sizeof(header)) != sizeof(header) || zip_long(header) != 0x04034b50 ||
		SDL_SeekIO(zip, (Sint64)(zip_word(header + 26) + zip_word(header + 28)), SDL_IO_SEEK_CUR) < 0)
	{
		snprintf(reason, reason_size, "its header at %lu could not be read (%s)", local_offset, SDL_GetError());
		return 0;
	}
	file = SDL_IOFromFile(path, "wb");
	if (!file)
	{
		snprintf(reason, reason_size, "could not create %s (%s)", path, SDL_GetError());
		return 0;
	}
	memset(&stream, 0, sizeof(stream));
	if (method == 8 && inflateInit2(&stream, -MAX_WBITS) != Z_OK)
	{
		snprintf(reason, reason_size, "zlib would not start (%s)", stream.msg ? stream.msg : "no message");
		SDL_CloseIO(file);
		return 0;
	}
	for (;;)
	{
		size_t count = remaining < sizeof(input) ? remaining : sizeof(input);

		if (count && SDL_ReadIO(zip, input, count) != count)
		{
			snprintf(reason, reason_size, "the download ended %lu bytes early (%s)", remaining, SDL_GetError());
			break;
		}
		remaining -= (unsigned long)count;
		if (method == 0)
		{
			/* (no more than the entry says it holds) */
			if (count > size - written)
			{
				snprintf(reason, reason_size, "it unpacks to more than %lu bytes", size);
				break;
			}
			if (SDL_WriteIO(file, input, count) != count)
			{
				snprintf(reason, reason_size, "could not write %s after %lu bytes (%s)", path, written, SDL_GetError());
				break;
			}
			checksum = crc32(checksum, input, (uInt)count);
			written += (unsigned long)count;
		}
		else
		{
			int result = Z_OK;

			stream.next_in = input;
			stream.avail_in = (uInt)count;
			do
			{
				size_t produced;

				stream.next_out = output;
				stream.avail_out = sizeof(output);
				result = inflate(&stream, Z_NO_FLUSH);
				if (result != Z_OK && result != Z_STREAM_END)
					break;
				produced = sizeof(output) - stream.avail_out;
				/* (no more than the entry says, before it is on the disk) */
				if (written + (unsigned long)produced > size)
				{
					snprintf(reason, reason_size, "it unpacks to more than %lu bytes", size);
					result = Z_ERRNO;
					break;
				}
				if (SDL_WriteIO(file, output, produced) != produced)
				{
					snprintf(reason, reason_size, "could not write %s after %lu bytes (%s)", path, written, SDL_GetError());
					result = Z_ERRNO;
					break;
				}
				checksum = crc32(checksum, output, (uInt)produced);
				written += (unsigned long)produced;
			} while (stream.avail_out == 0 && result != Z_STREAM_END);
			if (result == Z_STREAM_END)
				ended = 1;
			else if (result != Z_OK && result != Z_BUF_ERROR)
			{
				if (result != Z_ERRNO)
				{
					snprintf(reason, reason_size, "zlib stopped with %d after %lu of %lu bytes (%s)", result, written, size,
						stream.msg ? stream.msg : "no message");
				}
				break;
			}
		}
		/* (all of the input unpacked is enough, as the size and the CRC are
		checked: the game's zlib 1.1, used here before, could want a byte past
		the end of a raw stream before it said the stream had ended) */
		if (ended || !remaining)
		{
			if (written != size)
				snprintf(reason, reason_size, "it unpacked to %lu bytes, not %lu", written, size);
			else if (checksum != crc)
				snprintf(reason, reason_size, "its CRC is %08lx, not %08lx", checksum, crc);
			else
				succeeded = 1;
			break;
		}
	}
	if (method == 8)
		inflateEnd(&stream);
	if (!SDL_CloseIO(file) && succeeded)
	{
		snprintf(reason, reason_size, "could not finish writing %s (%s)", path, SDL_GetError());
		succeeded = 0;
	}
	return succeeded;
}

/* whether an entry's name is one of a release's files, which are all at the
zip's top, named with letters, digits and . _ + - (a folder, or a name that
could leave update.partial/ or mean something else to the system, is not) */
static int zip_name_allowed(const char *name)
{
	const char *c;

	if (!*name || *name == '.')
		return 0;
	for (c = name; *c; c++)
	{
		if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '.' ||
			*c == '_' || *c == '+' || *c == '-'))
		{
			return 0;
		}
	}
	return 1;
}

/* the zip's files (a flat folder) into update.partial/, their names in names */
static int zip_extract(SDL_IOStream *zip, char names[][256], int *name_count, char *error, size_t error_size)
{
	unsigned char tail[65536 + 22];
	Sint64 size;
	size_t tail_size, index;
	unsigned long entries = 0, directory_offset = 0, entry;
	int found = 0, succeeded = 0;

	*name_count = 0;
	/* the end of the central directory, in the last 64 KB */
	size = SDL_GetIOSize(zip);
	tail_size = size < (Sint64)sizeof(tail) ? (size_t)size : sizeof(tail);
	if (size < 22 || SDL_SeekIO(zip, size - (Sint64)tail_size, SDL_IO_SEEK_SET) < 0 ||
		SDL_ReadIO(zip, tail, tail_size) != tail_size)
	{
		goto done;
	}
	for (index = tail_size - 22 + 1; index-- > 0;)
	{
		if (zip_long(tail + index) == 0x06054b50)
		{
			entries = zip_word(tail + index + 10);
			directory_offset = zip_long(tail + index + 16);
			found = 1;
			break;
		}
	}
	if (!found || SDL_SeekIO(zip, (Sint64)directory_offset, SDL_IO_SEEK_SET) < 0)
		goto done;
	for (entry = 0; entry < entries; entry++)
	{
		unsigned char header[46];
		char name[256];
		unsigned long name_length, extra_length, comment_length;
		Sint64 next;
		char path[1200];
		char reason[512] = "";

		if (SDL_ReadIO(zip, header, sizeof(header)) != sizeof(header) || zip_long(header) != 0x02014b50)
			goto done;
		name_length = zip_word(header + 28);
		extra_length = zip_word(header + 30);
		comment_length = zip_word(header + 32);
		if (name_length >= sizeof(name) || SDL_ReadIO(zip, name, name_length) != name_length)
			goto done;
		name[name_length] = 0;
		next = SDL_TellIO(zip) + (Sint64)(extra_length + comment_length);
		/* (a folder, or a name that would leave the game's folder, is left
		out: the release's files are all at its top) */
		if (memchr(name, 0, name_length))
			goto done;
		if (!zip_name_allowed(name))
		{
			if (name_length && name[name_length - 1] != '/')
				platform_log("update: left out %s (not one of a release's names)", name);
		}
		else if (*name_count < MAXIMUM_UPDATE_FILES)
		{
			int method = (int)zip_word(header + 10);

			if (method != 0 && method != 8)
			{
				snprintf(error, error_size, "the download packs %s in a way this build cannot read", name);
				goto done;
			}
			updater_partial_path(path, sizeof(path), name);
			if (!zip_extract_entry(zip, zip_long(header + 42), method, zip_long(header + 20), zip_long(header + 24),
				zip_long(header + 16), path, reason, sizeof(reason)))
			{
				snprintf(error, error_size, "could not unpack %s: %s", name, reason);
				goto done;
			}
			snprintf(names[*name_count], 256, "%s", name);
			(*name_count)++;
		}
		if (SDL_SeekIO(zip, next, SDL_IO_SEEK_SET) < 0)
			goto done;
	}
	succeeded = *name_count > 0;

done:
	if (!succeeded && !error[0])
		snprintf(error, error_size, "the download is not a zip file this build can read");
	return succeeded;
}

/* ---------- checking */

/* the build number of GitHub's latest release, 0 if there is none */
static long updater_latest_release(void)
{
	char path[1200];
	char error[512] = "";
	size_t size = 0;
	char *text;
	const char *tag;
	long build = 0;

	updater_path(path, sizeof(path), "update-check.json");
	if (!update_download("https://api.github.com/repos/" UPDATE_REPOSITORY "/releases/latest", path,
		MAXIMUM_CHECK_SIZE, NULL, NULL, error, sizeof(error)))
	{
		platform_log("update: could not check for a new version: %s", error);
		return 0;
	}
	text = SDL_LoadFile(path, &size);
	update_delete_file(path);
	if (!text)
		return 0;
	/* "tag_name": "build-<number>" */
	tag = strstr(text, "\"tag_name\"");
	if (tag)
	{
		tag = strchr(tag + 10, '"');
		if (tag && !strncmp(tag, "\"build-", 7))
			build = strtol(tag + 7, NULL, 10);
	}
	SDL_free(text);
	return build;
}

static int SDLCALL updater_check_thread(void *context)
{
	long latest = updater_latest_release();

	(void)context;
	if (latest > HALO_BUILD_NUMBER)
	{
		platform_log("update: build %ld is available (this is build %d)", latest, HALO_BUILD_NUMBER);
		updater_latest_build = latest;
		SDL_SetAtomicInt(&updater_state, _updater_available);
	}
	else
	{
		if (latest)
			platform_log("update: this is the latest build (%d)", HALO_BUILD_NUMBER);
		SDL_SetAtomicInt(&updater_state, _updater_handled);
	}
	return 0;
}

/* ---------- updating */

struct updater_download
{
	SDL_Mutex *lock;
	char url[512];
	char zip_path[1200];
	unsigned long long received, total;
	int finished, succeeded;
	char error[512];
};

static void updater_download_progress(void *context, unsigned long long received, unsigned long long total)
{
	struct updater_download *download = context;

	SDL_LockMutex(download->lock);
	download->received = received;
	download->total = total;
	SDL_UnlockMutex(download->lock);
}

static int SDLCALL updater_download_thread(void *context)
{
	struct updater_download *download = context;
	char error[512] = "";
	int succeeded = update_download(download->url, download->zip_path, MAXIMUM_UPDATE_SIZE, updater_download_progress,
		download, error, sizeof(error));

	SDL_LockMutex(download->lock);
	download->succeeded = succeeded;
	snprintf(download->error, sizeof(download->error), "%s", error);
	download->finished = 1;
	SDL_UnlockMutex(download->lock);
	return 0;
}

/* the release's zip's address */
static void updater_zip_url(char *url, size_t size)
{
	snprintf(url, size, "https://github.com/" UPDATE_REPOSITORY "/releases/download/build-%ld/" UPDATE_ASSET,
		updater_latest_build);
}

/* downloads the release's zip, showing how far it has got in a window of its
own (drawn in software, clear of the game's OpenGL); 1 when it is there */
static int updater_download_zip(const char *zip_path, char *error, size_t error_size)
{
	static struct updater_download download;
	SDL_Window *window;
	SDL_Renderer *renderer = NULL;
	SDL_Thread *thread;
	int finished = 0;

	memset(&download, 0, sizeof(download));
	download.lock = SDL_CreateMutex();
	updater_zip_url(download.url, sizeof(download.url));
	snprintf(download.zip_path, sizeof(download.zip_path), "%s", zip_path);
	thread = SDL_CreateThread(updater_download_thread, "update download", &download);
	if (!thread)
	{
		SDL_DestroyMutex(download.lock);
		snprintf(error, error_size, "could not start the download");
		return 0;
	}
	window = SDL_CreateWindow("Halo", 640, 150, 0);
	if (window)
		renderer = SDL_CreateRenderer(window, SDL_SOFTWARE_RENDERER);
	while (!finished)
	{
		SDL_Event event;
		unsigned long long received, total;

		/* (the game's own events wait: its window is not drawn meanwhile) */
		while (SDL_PollEvent(&event))
		{
		}
		SDL_LockMutex(download.lock);
		finished = download.finished;
		received = download.received;
		total = download.total;
		SDL_UnlockMutex(download.lock);
		if (renderer)
		{
			char line[160];
			SDL_FRect bar = { 20.0f, 100.0f, 600.0f, 24.0f };
			float fraction = total ? (float)((double)received / (double)total) : 0.0f;

			SDL_SetRenderDrawColor(renderer, 12, 16, 20, 255);
			SDL_RenderClear(renderer);
			SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
			SDL_SetRenderScale(renderer, 2.0f, 2.0f);
			SDL_RenderDebugText(renderer, 10.0f, 10.0f, "Downloading the new version...");
			SDL_SetRenderScale(renderer, 1.0f, 1.0f);
			snprintf(line, sizeof(line), "build %ld  (%llu of %llu MB)", updater_latest_build, received >> 20,
				total >> 20);
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
	SDL_WaitThread(thread, NULL);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
	SDL_DestroyMutex(download.lock);
	if (!download.succeeded)
		snprintf(error, error_size, "%s", download.error);
	return download.succeeded;
}

/* the downloaded zip, read once and unpacked from that copy; the zip is
deleted either way */
static int updater_unpack(const char *zip_path, char names[][256], int *name_count, char *error, size_t error_size)
{
	size_t zip_size = 0;
	unsigned char *zip = SDL_LoadFile(zip_path, &zip_size);
	SDL_IOStream *stream;
	int succeeded = 0;

	update_delete_file(zip_path);
	if (!zip || zip_size > MAXIMUM_UPDATE_SIZE)
	{
		snprintf(error, error_size, "could not read the download");
		goto done;
	}
	stream = SDL_IOFromConstMem(zip, zip_size);
	if (!stream)
	{
		snprintf(error, error_size, "could not read the download");
		goto done;
	}
	succeeded = zip_extract(stream, names, name_count, error, error_size);
	SDL_CloseIO(stream);

done:
	SDL_free(zip);
	return succeeded;
}

/* the first count of the new files, already in place, taken back out (to
the update folder) and the old ones (<name>.old) put back, the last first:
the install as it was before the update. A new file that had no old one is
deleted */
static void updater_put_back(char names[][256], int count)
{
	int index, failures = 0;

	for (index = count - 1; index >= 0; index--)
	{
		char path[1200], new_path[1200], old_path[1300];

		updater_path(path, sizeof(path), names[index]);
		updater_partial_path(new_path, sizeof(new_path), names[index]);
		snprintf(old_path, sizeof(old_path), "%s.old", path);
		if (SDL_GetPathInfo(old_path, NULL))
		{
			if (!update_replace_file(path, old_path, new_path))
			{
				platform_log("update: could not put back %s", names[index]);
				failures++;
			}
		}
		else
		{
			update_delete_file(path);
		}
	}
	if (failures)
		platform_log("update: %d of %d replaced files could not be put back", failures, count);
	else
		platform_log("update: the %d replaced files put back: the install is as it was", count);
}

/* the new files in place of the old ones, each old one kept as <name>.old;
if one cannot be, the ones before it put back as they were, and 0 */
static int updater_replace_files(char names[][256], int name_count, char *error, size_t error_size)
{
	int index;

	for (index = 0; index < name_count; index++)
	{
		char path[1200], new_path[1200], old_path[1300];

		updater_path(path, sizeof(path), names[index]);
		updater_partial_path(new_path, sizeof(new_path), names[index]);
		snprintf(old_path, sizeof(old_path), "%s.old", path);
		if (!update_replace_file(path, new_path, old_path))
		{
			/* (that one is as it was: update_replace_file) */
			snprintf(error, error_size, "could not replace %s", path);
			if (index > 0)
				updater_put_back(names, index);
			return 0;
		}
	}
	return 1;
}

/* the release's zip downloaded on this thread, with no window (a dedicated
server's); 1 when it is there */
static int updater_download_zip_quietly(const char *zip_path, char *error, size_t error_size)
{
	char url[512];

	updater_zip_url(url, sizeof(url));
	return update_download(url, zip_path, MAXIMUM_UPDATE_SIZE, NULL, NULL, error, (int)error_size);
}

/* downloads (showing how far it has got, if windowed), unpacks and puts in
place the new build: 1, else 0 and why */
static int updater_install(int windowed, char *error, size_t error_size)
{
	char names[MAXIMUM_UPDATE_FILES][256];
	char zip_path[1200], partial[1200];
	int name_count = 0;

	updater_path(partial, sizeof(partial), UPDATE_DIRECTORY);
	updater_partial_path(zip_path, sizeof(zip_path), UPDATE_ASSET);
	platform_log("update: downloading build %ld (" UPDATE_ASSET ")", updater_latest_build);
	if (!update_make_directory(partial))
	{
		snprintf(error, error_size, "could not make %s (is the game's folder read-only?)", partial);
	}
	else if ((windowed ? updater_download_zip(zip_path, error, error_size) :
		updater_download_zip_quietly(zip_path, error, error_size)) &&
		updater_unpack(zip_path, names, &name_count, error, error_size) &&
		updater_replace_files(names, name_count, error, error_size))
	{
		update_delete_file(partial);
		return 1;
	}
	return 0;
}

/* downloads, unpacks and puts in place the new build, and starts it; returns
only if something failed */
static void updater_update(void)
{
	char error[512] = "";

	if (updater_install(1, error, sizeof(error)))
	{
		platform_log("update: starting build %ld", updater_latest_build);
		if (update_launch(updater_executable))
			exit(EXIT_SUCCESS);
		snprintf(error, sizeof(error), "the new version is in place, but could not be started: start it again");
	}
	platform_log("update: failed: %s", error);
	{
		char message[800];

		snprintf(message, sizeof(message), "The update failed:\n\n%s", error);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	}
}

/* the files a previous update left behind, and the notice of extract-xiso,
which releases carried until the disc image readers were written again */
static void updater_clean_up(void)
{
	static const char *const names[] =
	{
		"halo.old", "halo.exe.old", "SDL3.dll.old", "libSDL3.so.0.old", "SDL3-LICENSE.txt.old",
		"extract-xiso-LICENSE.txt.old", "mbedtls-LICENSE.txt.old", "LICENSE.txt.old",
		"extract-xiso-LICENSE.txt",
	};
	char path[1200];
	size_t index;

	for (index = 0; index < sizeof(names) / sizeof(*names); index++)
	{
		updater_path(path, sizeof(path), names[index]);
		update_delete_file(path);
	}
	updater_partial_path(path, sizeof(path), UPDATE_ASSET);
	update_delete_file(path);
	updater_path(path, sizeof(path), UPDATE_DIRECTORY);
	update_delete_file(path);
}

/* ---------- the game's */

/* at start-up (sdl_platform.c): looks for a new version, in the background */
void updater_start(void)
{
	char *slash;

	if (!update_executable_path(updater_executable, sizeof(updater_executable)))
		return;
	snprintf(updater_directory, sizeof(updater_directory), "%s", updater_executable);
	slash = strrchr(updater_directory, PATH_SEPARATOR[0]);
	if (!slash)
		return;
	*slash = 0;
	updater_clean_up();
	/* (not for builds without a number, the player's no, or runs nobody is
	watching, but for a test with its answer) */
	if (HALO_BUILD_NUMBER <= 0)
	{
		platform_log("update: not looked for: only the builds CI numbers update themselves (this one has no "
			"number)");
		return;
	}
	if (!config_boolean("update.auto"))
	{
		platform_log("update: not looked for: update.auto is false");
		return;
	}
	if (!config_string("debug.update_answer")[0] && (config_boolean("debug.hidden_window") ||
		config_real("debug.exit_after") > 0.0 || config_string("debug.network_test")[0]))
	{
		return;
	}
	SDL_SetAtomicInt(&updater_state, _updater_checking);
	{
		SDL_Thread *thread = SDL_CreateThread(updater_check_thread, "update check", NULL);

		if (thread)
			SDL_DetachThread(thread);
		else
			SDL_SetAtomicInt(&updater_state, _updater_handled);
	}
}

/* ---------- a dedicated server's (server.auto_update: server_director.c) */

/* looks for a new version on a thread of its own, unless a look is under
way (update.auto and the tests' rules are not asked: the setting is the
operator's): 0 if it cannot be looked for (a build without a number) */
int updater_server_check(void)
{
	char *slash;

	if (HALO_BUILD_NUMBER <= 0)
		return 0;
	if (!updater_directory[0])
	{
		if (!update_executable_path(updater_executable, sizeof(updater_executable)))
			return 0;
		snprintf(updater_directory, sizeof(updater_directory), "%s", updater_executable);
		slash = strrchr(updater_directory, PATH_SEPARATOR[0]);
		if (!slash)
		{
			updater_directory[0] = 0;
			return 0;
		}
		*slash = 0;
		updater_clean_up();
	}
	if (SDL_GetAtomicInt(&updater_state) == _updater_checking)
		return 1;
	SDL_SetAtomicInt(&updater_state, _updater_checking);
	{
		SDL_Thread *thread = SDL_CreateThread(updater_check_thread, "update check", NULL);

		if (thread)
			SDL_DetachThread(thread);
		else
			SDL_SetAtomicInt(&updater_state, _updater_handled);
	}
	return 1;
}

/* the newer build the last look found, 0 if none (or the look goes on) */
long updater_server_available(void)
{
	return SDL_GetAtomicInt(&updater_state) == _updater_available ? updater_latest_build : 0;
}

/* the newer build put in place of this one's files, not started (the server
stops, to be started again by what runs it): 1, else 0 and why. The game's
thread waits for the whole download: updater_server_install_start does not */
int updater_server_install(char *error, int error_size)
{
	if (SDL_GetAtomicInt(&updater_state) != _updater_available)
	{
		snprintf(error, (size_t)error_size, "no newer build is known");
		return 0;
	}
	SDL_SetAtomicInt(&updater_state, _updater_handled);
	return updater_install(0, error, (size_t)error_size);
}

enum
{
	_updater_install_none,
	_updater_install_running,
	_updater_install_installed,
	_updater_install_failed,
};

/* the server's install on a thread of its own: its state, and why it
failed (written before the state says so) */
static SDL_AtomicInt updater_install_state;
static char updater_install_error[512];

static void updater_install_failed(const char *why)
{
	snprintf(updater_install_error, sizeof(updater_install_error), "%s", why);
	SDL_SetAtomicInt(&updater_install_state, _updater_install_failed);
}

static int SDLCALL updater_install_thread(void *context)
{
	char error[512] = "";

	(void)context;
	if (updater_install(0, error, sizeof(error)))
		SDL_SetAtomicInt(&updater_install_state, _updater_install_installed);
	else
		updater_install_failed(error[0] ? error : "the install failed");
	return 0;
}

/* the newer build put in place of this one's files on a thread of its own
(the game goes on meanwhile: updater_server_install_poll tells its end): 1
if it began or goes on, else 0 (updater_server_install_poll says why) */
int updater_server_install_start(void)
{
	SDL_Thread *thread;

	if (SDL_GetAtomicInt(&updater_install_state) == _updater_install_running)
		return 1;
	if (SDL_GetAtomicInt(&updater_state) != _updater_available)
	{
		updater_install_failed("no newer build is known");
		return 0;
	}
	SDL_SetAtomicInt(&updater_state, _updater_handled);
	SDL_SetAtomicInt(&updater_install_state, _updater_install_running);
	thread = SDL_CreateThread(updater_install_thread, "update install", NULL);
	if (!thread)
	{
		updater_install_failed("could not start the install");
		return 0;
	}
	SDL_DetachThread(thread);
	return 1;
}

/* the install's state: 0 while it goes on, 1 when the new build is in
place, -1 if it failed (or none was started), and why in error */
int updater_server_install_poll(char *error, int size)
{
	int state = SDL_GetAtomicInt(&updater_install_state);

	if (size > 0)
		error[0] = 0;
	switch (state)
	{
	case _updater_install_running:
		return 0;
	case _updater_install_installed:
		return 1;
	case _updater_install_failed:
		snprintf(error, (size_t)size, "%s", updater_install_error);
		return -1;
	}
	snprintf(error, (size_t)size, "no install was started");
	return -1;
}

/* whether the game runs under gamescope (the Steam Deck's Game Mode), where
a system dialog such as SDL's message box crashes the game. gamescope sets
GAMESCOPE_WAYLAND_DISPLAY for the programs it runs and Game Mode's session
XDG_CURRENT_DESKTOP; Steam sets SteamDeck and SteamGamepadUI there. Wine
passes them on to the Windows build under Proton. */
static int updater_gamescope(void)
{
	const char *wayland_display = getenv("GAMESCOPE_WAYLAND_DISPLAY");
	const char *desktop = getenv("XDG_CURRENT_DESKTOP");
	const char *steam_deck = getenv("SteamDeck");
	const char *gamepad_ui = getenv("SteamGamepadUI");

	return (wayland_display && wayland_display[0]) || (desktop && strstr(desktop, "gamescope")) ||
		(steam_deck && !strcmp(steam_deck, "1") && gamepad_ui && !strcmp(gamepad_ui, "1"));
}

/* every frame, on the game's thread (sdl_platform.c): asks the player once a
new version is found */
void updater_poll(SDL_Window *window)
{
	static const SDL_MessageBoxButtonData question_buttons[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
		{ 0, 2, "Do not ask again" },
	};
	static const SDL_MessageBoxButtonData confirm_buttons[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
	};
	char message[400];
	int answer = 0;
	int fullscreen;
	const char *test_answer = config_string("debug.update_answer");

	if (SDL_GetAtomicInt(&updater_state) != _updater_available)
		return;
	SDL_SetAtomicInt(&updater_state, _updater_handled);
	/* (an automated test's answer: debug.update_answer) */
	if (test_answer[0])
	{
		platform_log("update: answering %s (debug.update_answer)", test_answer);
		if (!strcmp(test_answer, "yes"))
			updater_update();
		else if (!strcmp(test_answer, "never"))
			config_write_boolean("update.auto", 0);
		return;
	}
	/* (no question under gamescope: the player updates from the desktop, or
	with debug.update_answer) */
	if (updater_gamescope())
	{
		platform_log("update: build %ld is out (this is build %d); not asking under gamescope (Steam Deck Game Mode)",
			updater_latest_build, HALO_BUILD_NUMBER);
		return;
	}
	/* (a dialog cannot show above a fullscreen game) */
	fullscreen = window && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
	if (fullscreen)
		SDL_SetWindowFullscreen(window, false);
	snprintf(message, sizeof(message),
		"A new version of Halo was detected (build %ld; this is build %d).\n\n"
		"Do you want to update? The game will close and start the new version.",
		updater_latest_build, HALO_BUILD_NUMBER);
	{
		SDL_MessageBoxData question = { SDL_MESSAGEBOX_INFORMATION, window, "Halo: new version", message,
			3, question_buttons, NULL };

		if (!SDL_ShowMessageBox(&question, &answer))
			answer = 0;
	}
	if (answer == 2)
	{
		SDL_MessageBoxData confirm = { SDL_MESSAGEBOX_WARNING, window, "Halo: new version",
			"Stop asking about new versions?\n\n"
			"To ask again, set auto = true in the [update] section of config.toml.",
			2, confirm_buttons, NULL };
		int confirmed = 0;

		if (SDL_ShowMessageBox(&confirm, &confirmed) && confirmed == 1)
		{
			if (config_write_boolean("update.auto", 0))
				platform_log("update: update.auto = false written to config.toml");
			else
				platform_log("update: could not write update.auto to config.toml");
		}
	}
	else if (answer == 1)
	{
		updater_update();
	}
	if (fullscreen)
		SDL_SetWindowFullscreen(window, true);
}

#else

void updater_start(void)
{
}

int updater_server_check(void)
{
	return 0;
}

long updater_server_available(void)
{
	return 0;
}

int updater_server_install(char *error, int error_size)
{
	if (error_size > 0)
		error[0] = 0;
	return 0;
}

int updater_server_install_start(void)
{
	return 0;
}

int updater_server_install_poll(char *error, int size)
{
	if (size > 0)
		error[0] = 0;
	return -1;
}

#ifdef HALO_WEB
#include <SDL3/SDL.h>

/* (the site updates itself: port/web/site/sw.js) */
void updater_poll(SDL_Window *window)
{
	(void)window;
}
#endif

#endif
