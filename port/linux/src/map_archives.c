/*
MAP_ARCHIVES.C

A Custom Edition map fetched from where the community keeps them
(map_archives.h), for map_downloads.c: the index of the maps
(port/assets/network/map_index.txt) names, for most maps, the zips of the
community's archives that hold them (tools/map_index.py sources), and
maps.archives the archives to try, in order; then the web seeds
(maps.web_seeds), folders that hold every map of the index under its file
name:

- "halonet": HaloNet's map repository, a zip a map at
  https://maps.halonet.net/maps/<zip file name>;
- "halomaps": HaloMaps (halomaps.org), whose files are fetched as its page's
  Download button fetches them: the file's page
  (https://halomaps.org/hce/detail.cfm?fid=<id>) gives a token, which a
  POST of the page's form sends back for the zip;
- a web seed: the map itself, <folder><file name>.

All over HTTPS, with the self-updater's download (update.h: Mbed TLS on
Linux, WinHTTP on Windows), which checks the servers' certificates. A zip
is written beside the map's download (<path>.zip), and the map, the zip's
file of its name and size in any folder, taken out of it
(map_archives_unzip, map_archives_zip.c) into <path>.part; a web seed's map
is written there itself. It becomes the path given once whole and checked
against the index's info hash (map_archives_verify, map_archives_zip.c);
one that is not the index's is deleted, and the next source tried. The zip
is deleted after.

One fetch at a time, on a thread of its own, which the game's thread polls
and can stop (map_archives_stop: the download stops at its next block, and
the thread deletes what it wrote; the game's thread does not wait for it).
Android has no HTTPS download in its platform layer, and the web build
plays no Custom Edition maps: neither fetches from the archives (the web
build leaves this file out).
*/

#include "platform.h"
#include "posix.h"
#include "port_config.h"
#include "map_archives.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HALO_ANDROID
#include "update.h"
#endif

#define USER_AGENT "OpenCE (Halo; Custom Edition map downloads)"
#define HALONET_URL "https://maps.halonet.net/maps/"
#define HALOMAPS_PAGE_URL "https://halomaps.org/hce/detail.cfm?fid="
#define HALOMAPS_FORM_URL "https://halomaps.org/hce/detail.cfm"
/* (a page is tens of kilobytes) */
#define MAXIMUM_PAGE_SIZE (1024 * 1024)

static const struct
{
	const char *key;
	const char *name;
} archives[] =
{
	{ "halonet", "HaloNet" },
	{ "halomaps", "HaloMaps" },
	/* (maps.web_seeds': every map of the index, by its file name) */
	{ "web", "the web seed" },
};
#define WEB_SEED_ARCHIVE 2
#define ARCHIVE_COUNT ((int)(sizeof(archives) / sizeof(archives[0])))

/* ---------- names */

static int same_name(const char *a, const char *b, size_t length)
{
	size_t index;

	for (index = 0; index < length; index++)
	{
		char ca = a[index], cb = b[index];

		if (ca >= 'A' && ca <= 'Z')
			ca = (char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z')
			cb = (char)(cb - 'A' + 'a');
		if (ca != cb)
			return 0;
		if (!ca)
			return 1;
	}
	return 1;
}

/* whether maps.archives names the archive (a comma-separated list) */
static int archive_enabled(int archive)
{
	const char *list = config_string("maps.archives");
	size_t length = strlen(archives[archive].key);

	/* (the web seeds': whether there are any) */
	if (archive == WEB_SEED_ARCHIVE)
	{
		const char *seeds = config_string("maps.web_seeds");

		return seeds && strstr(seeds, "https://") != NULL;
	}

	while (*list)
	{
		const char *comma = strchr(list, ',');
		size_t item = comma ? (size_t)(comma - list) : strlen(list);

		while (item && *list == ' ')
			list++, item--;
		while (item && list[item - 1] == ' ')
			item--;
		if (item == length && same_name(list, archives[archive].key, length))
			return 1;
		if (!comma)
			break;
		list = comma + 1;
	}
	return 0;
}

/* the archive's id of the file in sources (tab-separated "<key>:<id>"), in
id (id_size characters); 1 if sources names the archive (a web seed has
every file: its id is the file name, set by the fetch) */
static int archive_source(const char *sources, int archive, char *id, int id_size)
{
	size_t key_length = strlen(archives[archive].key);
	const char *source = sources;

	if (archive == WEB_SEED_ARCHIVE)
	{
		if (id_size > 0)
			id[0] = 0;
		return 1;
	}

	while (source && *source)
	{
		const char *tab = strchr(source, '\t');
		size_t length = tab ? (size_t)(tab - source) : strlen(source);

		if (length > key_length + 1 && source[key_length] == ':' && same_name(source, archives[archive].key, key_length))
		{
			if (length - key_length - 1 >= (size_t)id_size)
				return 0;
			memcpy(id, source + key_length + 1, length - key_length - 1);
			id[length - key_length - 1] = 0;
			return 1;
		}
		source = tab ? tab + 1 : NULL;
	}
	return 0;
}

#ifndef HALO_ANDROID

/* ---------- the fetch's thread */

static struct
{
	pthread_mutex_t lock;
	int lock_made;
	/* set by map_archives_stop: the download ends at its next block */
	volatile int stop;
	enum map_archives_state state;
	unsigned long long received, total;
	int archive;
	char file_name[256];
	unsigned long long size;
	unsigned long piece_length;
	char info_hash[41];
	char sources[1024];
	char path[1024];
} fetch;

static void fetch_progress(void *context, unsigned long long received, unsigned long long total)
{
	(void)context;
	pthread_mutex_lock(&fetch.lock);
	fetch.received = received;
	fetch.total = total;
	pthread_mutex_unlock(&fetch.lock);
}

/* text percent-encoded for a URL's path or a form's value */
static void url_encode(const char *text, char *encoded, size_t size)
{
	static const char hex[] = "0123456789ABCDEF";
	size_t used = 0;

	for (; *text && used + 4 < size; text++)
	{
		unsigned char c = (unsigned char)*text;

		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr("-._~", c))
		{
			encoded[used++] = (char)c;
		}
		else
		{
			encoded[used++] = '%';
			encoded[used++] = hex[c >> 4];
			encoded[used++] = hex[c & 15];
		}
	}
	encoded[used] = 0;
}

/* HaloMaps's token for the file's download, out of its page (the form's
"hash" field: hexadecimal digits); 1 if found */
static int halomaps_token(const char *page_path, char *token, int token_size)
{
	static const char marker[] = "name=\"hash\" value=\"";
	struct posix_file_information information;
	char *page, *found;
	int file = open(page_path, O_RDONLY | O_CLOEXEC), ok = 0;
	unsigned long size;

	if (file < 0)
		return 0;
	if (posix_fstat(file, &information) != 0 || information.size_high || information.size_low > MAXIMUM_PAGE_SIZE)
	{
		close(file);
		return 0;
	}
	size = information.size_low;
	page = malloc(size + 1);
	if (page && map_archives_read_at(file, 0, page, size))
	{
		page[size] = 0;
		found = strstr(page, marker);
		if (found)
		{
			int length = 0;

			found += sizeof(marker) - 1;
			while (length < token_size - 1 && ((found[length] >= '0' && found[length] <= '9') ||
				(found[length] >= 'A' && found[length] <= 'F') || (found[length] >= 'a' && found[length] <= 'f')))
			{
				token[length] = found[length];
				length++;
			}
			token[length] = 0;
			ok = length > 0 && found[length] == '"';
		}
	}
	free(page);
	close(file);
	return ok;
}

/* the zip of the file's id from the archive, into zip_path; 1 on success */
static int fetch_zip(int archive, const char *id, const char *zip_path, char *error, int error_size)
{
	char url[2048];
	char encoded[1024];

	if (!strcmp(archives[archive].key, "halonet"))
	{
		url_encode(id, encoded, sizeof(encoded));
		snprintf(url, sizeof(url), HALONET_URL "%s", encoded);
		return update_request(url, NULL, USER_AGENT, &fetch.stop, zip_path, fetch_progress, NULL, error, error_size);
	}
	else
	{
		char page_path[1100];
		char token[64];
		char form[256];
		int got_token;
		const char *digit;

		/* (a file's id is a number) */
		for (digit = id; *digit; digit++)
		{
			if (*digit < '0' || *digit > '9')
			{
				snprintf(error, (size_t)error_size, "a bad file id %s", id);
				return 0;
			}
		}
		snprintf(page_path, sizeof(page_path), "%s.html", fetch.path);
		unlink(page_path);
		snprintf(url, sizeof(url), HALOMAPS_PAGE_URL "%s", id);
		if (!update_request(url, NULL, USER_AGENT, &fetch.stop, page_path, NULL, NULL, error, error_size))
			return 0;
		got_token = halomaps_token(page_path, token, sizeof(token));
		unlink(page_path);
		if (!got_token)
		{
			snprintf(error, (size_t)error_size, "its page has no download");
			return 0;
		}
		snprintf(form, sizeof(form), "fid=%s&action=dl&hash=%s&submit=download", id, token);
		return update_request(HALOMAPS_FORM_URL, form, USER_AGENT, &fetch.stop, zip_path, fetch_progress, NULL, error,
			error_size);
	}
}

/* the map from the first web seed (maps.web_seeds: folders, separated by
commas) that has it whole, into part_path; 1 on success */
static int fetch_web_seeds(const char *part_path)
{
	const char *list = config_string("maps.web_seeds");
	char encoded[1024];

	url_encode(fetch.file_name, encoded, sizeof(encoded));
	while (list && *list && !fetch.stop)
	{
		const char *comma = strchr(list, ',');
		size_t length = comma ? (size_t)(comma - list) : strlen(list);
		char folder[1024];
		char url[2100];
		char error[192];
		struct posix_file_information information;

		while (length && (*list == ' ' || *list == '\t'))
		{
			list++;
			length--;
		}
		while (length && (list[length - 1] == ' ' || list[length - 1] == '\t'))
			length--;
		if (length && length < sizeof(folder) && !strncmp(list, "https://", 8))
		{
			memcpy(folder, list, length);
			folder[length] = 0;
			snprintf(url, sizeof(url), "%s%s%s", folder, folder[length - 1] == '/' ? "" : "/", encoded);
			pthread_mutex_lock(&fetch.lock);
			fetch.received = fetch.total = 0;
			pthread_mutex_unlock(&fetch.lock);
			platform_log("map archives: fetching %s from %s", fetch.file_name, folder);
			error[0] = 0;
			unlink(part_path);
			if (update_request(url, NULL, USER_AGENT, &fetch.stop, part_path, fetch_progress, NULL, error,
					sizeof(error)) &&
				posix_stat(part_path, &information) == 0 &&
				(((unsigned long long)information.size_high << 32) | information.size_low) == fetch.size &&
				map_archives_verify(part_path, fetch.file_name, fetch.size, fetch.piece_length, fetch.info_hash, error,
					sizeof(error)))
			{
				platform_log("map archives: %s came from %s", fetch.file_name, folder);
				return 1;
			}
			platform_log("map archives: %s did not come from %s: %s", fetch.file_name, folder,
				fetch.stop ? "stopped" : error[0] ? error : "another size");
			unlink(part_path);
		}
		list = comma ? comma + 1 : NULL;
	}
	return 0;
}

static void *fetch_thread(void *unused)
{
	char zip_path[1100];
	char part_path[1100];
	int archive;
	enum map_archives_state result = _map_archives_failed;

	(void)unused;
	snprintf(zip_path, sizeof(zip_path), "%s.zip", fetch.path);
	snprintf(part_path, sizeof(part_path), "%s.part", fetch.path);
	for (archive = 0; archive < ARCHIVE_COUNT && !fetch.stop; archive++)
	{
		char id[512];
		char error[192];

		if (!archive_enabled(archive) || !archive_source(fetch.sources, archive, id, sizeof(id)))
			continue;
		if (archive == WEB_SEED_ARCHIVE)
		{
			pthread_mutex_lock(&fetch.lock);
			fetch.archive = archive;
			pthread_mutex_unlock(&fetch.lock);
			if (fetch_web_seeds(part_path))
			{
				if (fetch.stop)
					break;
				unlink(fetch.path);
				if (rename(part_path, fetch.path) != 0)
				{
					platform_log("map archives: %s could not be put in place", fetch.file_name);
					break;
				}
				result = _map_archives_done;
			}
			break;
		}
		pthread_mutex_lock(&fetch.lock);
		fetch.archive = archive;
		fetch.received = fetch.total = 0;
		pthread_mutex_unlock(&fetch.lock);
		platform_log("map archives: fetching %s from %s (%s)", fetch.file_name, archives[archive].name, id);
		error[0] = 0;
		if (!fetch_zip(archive, id, zip_path, error, sizeof(error)))
		{
			platform_log("map archives: %s's zip did not come from %s: %s", fetch.file_name, archives[archive].name,
				fetch.stop ? "stopped" : error);
			unlink(zip_path);
			continue;
		}
		if (!map_archives_unzip(zip_path, fetch.file_name, fetch.size, part_path, error, sizeof(error)))
		{
			platform_log("map archives: %s could not be taken out of %s's zip: %s", fetch.file_name,
				archives[archive].name, error);
			unlink(zip_path);
			continue;
		}
		unlink(zip_path);
		/* (another version than the index's, say: the next source) */
		if (!map_archives_verify(part_path, fetch.file_name, fetch.size, fetch.piece_length, fetch.info_hash, error,
			sizeof(error)))
		{
			platform_log("map archives: %s from %s is not the index's: %s", fetch.file_name, archives[archive].name,
				error);
			unlink(part_path);
			continue;
		}
		/* (in place of what an earlier download of it left in the downloads
		folder, never of a map: the path is the downloads folder's) */
		if (fetch.stop)
			break;
		unlink(fetch.path);
		if (rename(part_path, fetch.path) != 0)
		{
			platform_log("map archives: %s could not be put in place", fetch.file_name);
			break;
		}
		platform_log("map archives: %s came from %s", fetch.file_name, archives[archive].name);
		result = _map_archives_done;
		break;
	}
	if (result != _map_archives_done)
		unlink(part_path);
	pthread_mutex_lock(&fetch.lock);
	fetch.state = fetch.stop ? _map_archives_failed : result;
	pthread_mutex_unlock(&fetch.lock);
	return NULL;
}

/* the fetch's state, read under its lock */
static enum map_archives_state map_archives_poll_state(void)
{
	enum map_archives_state state;

	pthread_mutex_lock(&fetch.lock);
	state = fetch.state;
	pthread_mutex_unlock(&fetch.lock);
	return state;
}

int map_archives_available(const char *sources)
{
	char id[512];
	int archive;

	for (archive = 0; archive < ARCHIVE_COUNT; archive++)
	{
		if (archive_enabled(archive) && archive_source(sources, archive, id, sizeof(id)))
			return 1;
	}
	return 0;
}

int map_archives_start(const char *file_name, unsigned long long size, unsigned long piece_length,
	const char *info_hash, const char *sources, const char *path)
{
	pthread_t thread;

	if (!fetch.lock_made)
	{
		pthread_mutex_init(&fetch.lock, NULL);
		fetch.lock_made = 1;
	}
	/* (a stopped fetch's thread may still be ending) */
	if (map_archives_poll_state() == _map_archives_running || !sources || !map_archives_available(sources) ||
		strlen(file_name) >= sizeof(fetch.file_name) || strlen(sources) >= sizeof(fetch.sources) ||
		strlen(path) + 8 >= sizeof(fetch.path) || !piece_length || strlen(info_hash) != 40)
	{
		return 0;
	}
	if (fetch.stop)
	{
		/* (a stopped fetch's thread has ended: what it left deleted) */
		char other_path[1100];

		snprintf(other_path, sizeof(other_path), "%s.zip", fetch.path);
		unlink(other_path);
		snprintf(other_path, sizeof(other_path), "%s.html", fetch.path);
		unlink(other_path);
	}
	snprintf(fetch.file_name, sizeof(fetch.file_name), "%s", file_name);
	snprintf(fetch.sources, sizeof(fetch.sources), "%s", sources);
	snprintf(fetch.path, sizeof(fetch.path), "%s", path);
	fetch.size = size;
	fetch.piece_length = piece_length;
	snprintf(fetch.info_hash, sizeof(fetch.info_hash), "%s", info_hash);
	fetch.stop = 0;
	fetch.received = fetch.total = 0;
	/* (the first source that will be tried, for the screen meanwhile) */
	for (fetch.archive = 0; fetch.archive < ARCHIVE_COUNT - 1; fetch.archive++)
	{
		char id[512];

		if (archive_enabled(fetch.archive) && archive_source(sources, fetch.archive, id, sizeof(id)))
			break;
	}
	fetch.state = _map_archives_running;
	/* (detached: it touches nothing of the fetch once its state, under the
	lock, says it ended, so that another can start then) */
	if (pthread_create(&thread, NULL, fetch_thread, NULL) != 0)
	{
		fetch.state = _map_archives_idle;
		return 0;
	}
	pthread_detach(thread);
	return 1;
}

enum map_archives_state map_archives_poll(unsigned long long *received, unsigned long long *total, char *archive,
	int archive_size)
{
	enum map_archives_state state;

	if (!fetch.lock_made)
		return _map_archives_idle;
	pthread_mutex_lock(&fetch.lock);
	state = fetch.state;
	*received = fetch.received;
	*total = fetch.total;
	snprintf(archive, (size_t)archive_size, "%s", archives[fetch.archive].name);
	pthread_mutex_unlock(&fetch.lock);
	return state;
}

void map_archives_stop(void)
{
	enum map_archives_state state;

	if (!fetch.lock_made)
		return;
	pthread_mutex_lock(&fetch.lock);
	state = fetch.state;
	pthread_mutex_unlock(&fetch.lock);
	/* (one under way ends by itself, at its download's next block, and is
	idle once map_archives_start finds it ended) */
	if (state == _map_archives_running)
	{
		fetch.stop = 1;
		return;
	}
	fetch.state = _map_archives_idle;
}

/* ---------- map sharing's web seeds: a range of a file */

int map_archives_web_seed_url(const char *file_name, int which, char *url, int url_size)
{
	const char *list = config_string("maps.web_seeds");
	char encoded[1024];

	url_encode(file_name, encoded, sizeof(encoded));
	while (list && *list)
	{
		const char *comma = strchr(list, ',');
		size_t length = comma ? (size_t)(comma - list) : strlen(list);

		while (length && (*list == ' ' || *list == '\t'))
		{
			list++;
			length--;
		}
		while (length && (list[length - 1] == ' ' || list[length - 1] == '\t'))
			length--;
		if (length > 8 && !strncmp(list, "https://", 8) && which-- == 0)
		{
			if (length + strlen(encoded) + 2 > (size_t)url_size)
				return 0;
			snprintf(url, (size_t)url_size, "%.*s%s%s", (int)length, list, list[length - 1] == '/' ? "" : "/",
				encoded);
			return 1;
		}
		list = comma ? comma + 1 : NULL;
	}
	return 0;
}

static struct
{
	pthread_mutex_t lock;
	int lock_made;
	volatile int stop;
	enum map_archives_state state;
	char url[2100];
	unsigned long long offset;
	unsigned long length;
	char path[1024];
	char error[192];
} range_fetch;

static void *range_thread(void *unused)
{
	char error[192];
	int ok;

	(void)unused;
	error[0] = 0;
	ok = update_request_range(range_fetch.url, USER_AGENT, &range_fetch.stop, range_fetch.offset, range_fetch.length,
		range_fetch.path, error, sizeof(error));
	if (!ok || range_fetch.stop)
		unlink(range_fetch.path);
	pthread_mutex_lock(&range_fetch.lock);
	snprintf(range_fetch.error, sizeof(range_fetch.error), "%s", range_fetch.stop ? "stopped" : error);
	range_fetch.state = ok && !range_fetch.stop ? _map_archives_done : _map_archives_failed;
	pthread_mutex_unlock(&range_fetch.lock);
	return NULL;
}

static enum map_archives_state range_state(void)
{
	enum map_archives_state state;

	if (!range_fetch.lock_made)
		return _map_archives_idle;
	pthread_mutex_lock(&range_fetch.lock);
	state = range_fetch.state;
	pthread_mutex_unlock(&range_fetch.lock);
	return state;
}

int map_archives_range_start(const char *url, unsigned long long offset, unsigned long length, const char *path)
{
	pthread_t thread;

	if (!range_fetch.lock_made)
	{
		pthread_mutex_init(&range_fetch.lock, NULL);
		range_fetch.lock_made = 1;
	}
	if (range_state() == _map_archives_running || !length || strlen(url) >= sizeof(range_fetch.url) ||
		strlen(path) >= sizeof(range_fetch.path))
	{
		return 0;
	}
	snprintf(range_fetch.url, sizeof(range_fetch.url), "%s", url);
	snprintf(range_fetch.path, sizeof(range_fetch.path), "%s", path);
	range_fetch.offset = offset;
	range_fetch.length = length;
	range_fetch.stop = 0;
	range_fetch.error[0] = 0;
	range_fetch.state = _map_archives_running;
	if (pthread_create(&thread, NULL, range_thread, NULL) != 0)
	{
		range_fetch.state = _map_archives_idle;
		return 0;
	}
	pthread_detach(thread);
	return 1;
}

enum map_archives_state map_archives_range_poll(char *error, int error_size)
{
	enum map_archives_state state;

	if (!range_fetch.lock_made)
		return _map_archives_idle;
	pthread_mutex_lock(&range_fetch.lock);
	state = range_fetch.state;
	if (error && error_size > 0)
		snprintf(error, (size_t)error_size, "%s", range_fetch.error);
	pthread_mutex_unlock(&range_fetch.lock);
	return state;
}

void map_archives_range_stop(void)
{
	if (range_state() == _map_archives_running)
	{
		/* (it ends by itself at its download's next block, deleting what it
		wrote; map_archives_range_start waits for that) */
		range_fetch.stop = 1;
		return;
	}
	if (range_fetch.lock_made)
		range_fetch.state = _map_archives_idle;
}

#else

/* Android: no HTTPS download in its platform layer (update.h is the
desktop updater's), so no fetch from the archives */

int map_archives_available(const char *sources)
{
	(void)sources;
	(void)archive_source;
	(void)archive_enabled;
	return 0;
}

int map_archives_start(const char *file_name, unsigned long long size, unsigned long piece_length,
	const char *info_hash, const char *sources, const char *path)
{
	(void)piece_length;
	(void)info_hash;
	(void)file_name;
	(void)size;
	(void)sources;
	(void)path;
	return 0;
}

enum map_archives_state map_archives_poll(unsigned long long *received, unsigned long long *total, char *archive,
	int archive_size)
{
	*received = *total = 0;
	if (archive_size > 0)
		archive[0] = 0;
	return _map_archives_idle;
}

void map_archives_stop(void)
{
}

int map_archives_web_seed_url(const char *file_name, int which, char *url, int url_size)
{
	(void)file_name;
	(void)which;
	(void)url;
	(void)url_size;
	return 0;
}

int map_archives_range_start(const char *url, unsigned long long offset, unsigned long length, const char *path)
{
	(void)url;
	(void)offset;
	(void)length;
	(void)path;
	return 0;
}

enum map_archives_state map_archives_range_poll(char *error, int error_size)
{
	if (error && error_size > 0)
		error[0] = 0;
	return _map_archives_idle;
}

void map_archives_range_stop(void)
{
}

#endif
