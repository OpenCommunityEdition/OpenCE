/*
MAP_DOWNLOADS.C

Custom Edition maps downloaded from where the community keeps them
(map_downloads.h).

A machine joining a game on a Custom Edition map it lacks, or has another
version of, whose host does not send it (map sharing does when the host
shares and names the map for its hash: port/linux/game/map_share.c), asks
here for it before it joins (ui_widget_port_join_map_fetch in
menu_functions.c; cache_files_map_present, when the host changes map after
the join; map_share.c, when the host turns the request down), with Custom
Edition's bitmaps.map, sounds.map and loc.map when they are missing. Each is
looked up in the index of the maps (maps.index: the file name and, for the
map, the header checksum the host sent, which is the game's map version)
and fetched by map_archives.c into custom_maps\downloads: out of HaloNet's
or HaloMaps's zip that holds it (maps.archives), else from a web seed
(maps.web_seeds), each checked against the index's hash before it is
taken. One file at a time, on map_archives.c's thread.

A file that arrives is moved into custom_maps: a map as
<name>_<hash>.map, the name map sharing gives a version of it
(map_share_name_digits), beside any other version there; a resource map
under its own name, only when there is none. No file already there is ever
replaced, renamed or changed. Once every file is there the menus try the
join again (map_downloads_take_ready). The menus show how it goes
(map_downloads_fetching, map_downloads_progress).
*/

#include "platform.h"
#include "posix.h"
#include "port_config.h"
#include "map_archives.h"
#include "map_downloads.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <fcntl.h>
#endif

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define INDEX_FORMAT "# halo map index v1"
/* the Custom Edition maps' folders (custom_edition_cache.h's
CUSTOM_EDITION_MAP_DIRECTORY and CUSTOM_EDITION_INSTALL_MAP_DIRECTORY):
downloads go into the first */
#define CUSTOM_MAPS_XBOX_PATH "d:\\custom_maps\\"
#define INSTALL_MAPS_XBOX_PATH "h:\\maps\\"
/* a Custom Edition map's level name (CUSTOM_EDITION_LEVEL_NAME_PREFIX) */
#define LEVEL_NAME_PREFIX "custom_maps\\"
#define DOWNLOADS_FOLDER "downloads"
#define MAP_FILE_EXTENSION ".map"
/* the digits a map's name ends in after _ (map_share.h's
MAP_SHARE_NAME_DIGITS) */
#define NAME_DIGITS 8
#define FETCH_MAXIMUM_FILES 4
#define FAILURE_SHOWN_MILLISECONDS 15000
#define NAME_SIZE 128

/* (port/linux/game/map_share_files.c's: the digits a map file is named by,
<folder><name>.map's hash) */
unsigned char map_share_name_digits(const char *folder, const char *name, char *digits);

/* ---------- the index */

struct index_entry
{
	/* into maps_index.names */
	unsigned long name;
	unsigned long checksum;
	unsigned long long size;
	unsigned long piece_length;
	char info_hash[41];
	/* into maps_index.names: the archives' zips that hold it, as the
	index's columns after the info hash (tab-separated "halonet:<zip>",
	"halomaps:<file id>"; map_archives.h), "" for none */
	unsigned long sources;
};

static struct
{
	int loaded;
	struct index_entry *entries;
	int count;
	char *names;
	unsigned long names_size;
	char path[1024];
} maps_index;

static int same_name(const char *a, const char *b)
{
	for (;; a++, b++)
	{
		char ca = *a, cb = *b;

		if (ca >= 'A' && ca <= 'Z')
			ca = (char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z')
			cb = (char)(cb - 'A' + 'a');
		if (ca != cb)
			return 0;
		if (!ca)
			return 1;
	}
}

static void index_path(char *path, int size)
{
	const char *name = config_string("maps.index");

	if (name[0] == '/' || name[0] == '\\' || (name[0] && name[1] == ':'))
	{
		snprintf(path, (size_t)size, "%s", name);
		return;
	}
	config_folder(path, (unsigned long)size);
	snprintf(path + strlen(path), (size_t)size - strlen(path), "%s", name);
}

/* reads the index once: each line a map's file name, header checksum,
size, piece length and info hash, and the archives' zips that hold it,
tab-separated (tools/map_index.py) */
static void index_load(void)
{
	char *file;
	size_t file_size = 0;
	char *line;
	char *next;
	int capacity = 0;
	int number = 0;

	if (maps_index.loaded)
		return;
	maps_index.loaded = 1;
	index_path(maps_index.path, sizeof(maps_index.path));
	file = config_file_read(maps_index.path, &file_size);
	if (!file)
	{
		platform_log("map downloads: the index %s cannot be read (maps.index)", maps_index.path);
		return;
	}
	if (strncmp(file, INDEX_FORMAT, strlen(INDEX_FORMAT)))
	{
		platform_log("map downloads: %s is not a map index (its first line is not \"%s\")", maps_index.path,
			INDEX_FORMAT);
		free(file);
		return;
	}
	maps_index.names = malloc(file_size + 1);
	if (!maps_index.names)
	{
		free(file);
		return;
	}
	for (line = file; line && *line; line = next)
	{
		char *fields[5];
		char *sources;
		int field;
		char *end;
		struct index_entry *entry;
		size_t name_length;

		number++;
		next = strchr(line, '\n');
		if (next)
			*next++ = 0;
		end = line + strlen(line);
		while (end > line && (end[-1] == '\r' || end[-1] == ' '))
			*--end = 0;
		if (!line[0] || line[0] == '#')
			continue;
		fields[0] = line;
		for (field = 1; field < 5; field++)
		{
			char *tab = strchr(fields[field - 1], '\t');

			if (!tab)
				break;
			*tab = 0;
			fields[field] = tab + 1;
		}
		sources = field == 5 ? strchr(fields[4], '\t') : NULL;
		if (sources)
			*sources++ = 0;
		if (field < 5 || strlen(fields[4]) != 40)
		{
			platform_log("map downloads: %s line %d is not an entry (left out)", maps_index.path, number);
			continue;
		}
		if (maps_index.count == capacity)
		{
			struct index_entry *grown;

			capacity = capacity ? capacity * 2 : 1024;
			grown = realloc(maps_index.entries, (size_t)capacity * sizeof(*grown));
			if (!grown)
				break;
			maps_index.entries = grown;
		}
		entry = &maps_index.entries[maps_index.count];
		name_length = strlen(fields[0]);
		entry->name = maps_index.names_size;
		memcpy(maps_index.names + maps_index.names_size, fields[0], name_length + 1);
		maps_index.names_size += (unsigned long)name_length + 1;
		entry->checksum = strtoul(fields[1], NULL, 16);
		entry->size = strtoull(fields[2], NULL, 10);
		entry->piece_length = strtoul(fields[3], NULL, 10);
		memcpy(entry->info_hash, fields[4], 41);
		/* (the names and sources together are shorter than the file) */
		entry->sources = maps_index.names_size;
		name_length = sources ? strlen(sources) : 0;
		memcpy(maps_index.names + maps_index.names_size, sources ? sources : "", name_length + 1);
		maps_index.names_size += (unsigned long)name_length + 1;
		name_length = strlen(maps_index.names + entry->name);
		if (!name_length || !entry->size || !entry->piece_length)
		{
			platform_log("map downloads: %s line %d has a bad size or piece length (left out)", maps_index.path,
				number);
			continue;
		}
		maps_index.count++;
	}
	free(file);
	platform_log("map downloads: %d maps indexed in %s", maps_index.count, maps_index.path);
}

/* the entry of the file named, of the checksum (0: any; an entry's 0 is
its own version's, as some maps' headers have, not any) */
static const struct index_entry *index_find(const char *file_name, unsigned long checksum)
{
	int which;

	index_load();
	for (which = 0; which < maps_index.count; which++)
	{
		const struct index_entry *entry = &maps_index.entries[which];

		if (same_name(maps_index.names + entry->name, file_name) && (!checksum || entry->checksum == checksum))
			return entry;
	}
	return NULL;
}

/* the length of a name without the _ and hash digits it ends in (a map
named for its hash, map sharing's <name>_<hash>); 0 if it ends in none */
static size_t hashed_base_length(const char *name, size_t length)
{
	size_t index;

	if (length <= NAME_DIGITS + 1 || name[length - NAME_DIGITS - 1] != '_')
		return 0;
	for (index = length - NAME_DIGITS; index < length; index++)
	{
		char c = name[index];

		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
			return 0;
	}
	return length - NAME_DIGITS - 1;
}

/* the entry of the file named (<name>.map) or, for a level named for the
map's hash (<name>_<hash>.map), of the name without the hash; of the
checksum (0: any) */
static const struct index_entry *index_find_file(const char *file_name, unsigned long checksum)
{
	const struct index_entry *entry = index_find(file_name, checksum);
	size_t length = strlen(file_name);
	size_t base;
	char name[NAME_SIZE];

	if (entry || length <= strlen(MAP_FILE_EXTENSION) || length >= sizeof(name))
		return entry;
	base = hashed_base_length(file_name, length - strlen(MAP_FILE_EXTENSION));
	if (!base)
		return NULL;
	snprintf(name, sizeof(name), "%.*s" MAP_FILE_EXTENSION, (int)base, file_name);
	return index_find(name, checksum);
}

/* ---------- map sharing's web seeds */

static int downloads_folder(char *path, int size);

int map_downloads_web_seed(const char *file_name, unsigned long checksum, unsigned long long size, int which,
	char *url, int url_size)
{
	const struct index_entry *entry;

	if (!config_boolean("maps.downloads") || !config_boolean("game.custom_edition") || !file_name)
		return 0;
	entry = index_find_file(file_name, checksum);
	if (!entry || entry->size != size)
		return 0;
	return map_archives_web_seed_url(maps_index.names + entry->name, which, url, url_size);
}

/* the range fetch's file (custom_maps/downloads/web_seed_range.part), set
when one starts */
static char range_path[1100];

int map_downloads_range_start(const char *url, unsigned long long offset, unsigned long length)
{
	char folder[1024];
	unsigned long long received, total;
	char archive[64];

	/* (not while a map comes from the archives: one HTTPS request goes at a
	time, posix_update.c, and that one may take minutes) */
	if (map_archives_poll(&received, &total, archive, sizeof(archive)) == _map_archives_running ||
		!downloads_folder(folder, sizeof(folder)))
	{
		return 0;
	}
	snprintf(range_path, sizeof(range_path), "%sweb_seed_range.part", folder);
	return map_archives_range_start(url, offset, length, range_path);
}

int map_downloads_range_read(unsigned long offset, void *buffer, unsigned long size)
{
	FILE *file;
	int ok;

	if (!range_path[0] || map_archives_range_poll(NULL, 0) != _map_archives_done)
		return 0;
	file = fopen(range_path, "rb");
	if (!file)
		return 0;
	ok = fseek(file, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
	fclose(file);
	return ok;
}

int map_downloads_range_poll(char *error, int error_size)
{
	switch (map_archives_range_poll(error, error_size))
	{
	case _map_archives_running:
		return 1;
	case _map_archives_done:
		return 2;
	case _map_archives_failed:
		return 3;
	default:
		return 0;
	}
}

void map_downloads_range_stop(void)
{
	/* (one under way deletes its file as it ends) */
	if (map_archives_range_poll(NULL, 0) != _map_archives_running && range_path[0])
		unlink(range_path);
	map_archives_range_stop();
}

/* ---------- paths */

static int file_present(const char *path)
{
	struct posix_file_information information;

	return posix_stat(path, &information) == 0 && !(information.flags & _posix_file_is_directory);
}

/* a file moved to a name no file has: never in place of one (POSIX's
rename would replace it, so the name is taken first; Windows's fails) */
static int move_to_new_name(const char *from, const char *to)
{
#ifndef _WIN32
	int taken = open(to, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);

	if (taken < 0)
		return 0;
	close(taken);
	if (rename(from, to) != 0)
	{
		unlink(to);
		return 0;
	}
	return 1;
#else
	return rename(from, to) == 0;
#endif
}

/* the folder made if missing; 1 if it is there */
static int folder_made(const char *path)
{
	struct posix_file_information information;

	if (posix_make_directory(path))
		return 1;
	return posix_stat(path, &information) == 0 && (information.flags & _posix_file_is_directory);
}

/* the custom_maps folder (made if missing), with its separator */
static int custom_maps_folder(char *path, int size)
{
	size_t length;

	platform_translate_path(CUSTOM_MAPS_XBOX_PATH, path, (unsigned long)size);
	length = strlen(path);
	while (length && (path[length - 1] == '/' || path[length - 1] == '\\'))
		path[--length] = 0;
	if (!folder_made(path))
	{
		platform_log("map downloads: the folder %s cannot be made", path);
		return 0;
	}
	snprintf(path + length, (size_t)size - length, PATH_SEPARATOR);
	return 1;
}

/* custom_maps\downloads (made if missing), with its separator */
static int downloads_folder(char *path, int size)
{
	size_t length;

	if (!custom_maps_folder(path, size))
		return 0;
	length = strlen(path);
	snprintf(path + length, (size_t)size - length, "%s", DOWNLOADS_FOLDER);
	if (!folder_made(path))
	{
		platform_log("map downloads: the folder %s cannot be made", path);
		return 0;
	}
	length = strlen(path);
	snprintf(path + length, (size_t)size - length, PATH_SEPARATOR);
	return 1;
}

/* the map's file name from its level name: custom_maps\<file> (or the
file's name alone) is <file>.map; "" for any other (the game's own maps) */
static void level_file_name(const char *level_name, char *file_name, int size)
{
	const char *file = level_name;
	size_t prefix = strlen(LEVEL_NAME_PREFIX);

	if (!strncmp(level_name, LEVEL_NAME_PREFIX, prefix) || !strncmp(level_name, "custom_maps/", prefix))
		file = level_name + prefix;
	else if (strpbrk(level_name, "\\/:"))
		file = "";
	/* (a name with no folder, as the folders' files have) */
	if (!file[0] || strpbrk(file, "\\/:*?") || file[0] == '.' ||
		strlen(file) + strlen(MAP_FILE_EXTENSION) >= NAME_SIZE)
	{
		file_name[0] = 0;
		return;
	}
	snprintf(file_name, (size_t)size, "%s" MAP_FILE_EXTENSION, file);
}

/* whether a resource map is in a Custom Edition maps folder (custom_maps,
then a Custom Edition install's) */
static int resource_map_present(const char *file_name)
{
	static const char *const folders[] = { CUSTOM_MAPS_XBOX_PATH, INSTALL_MAPS_XBOX_PATH };
	char xbox_path[256];
	char path[1024];
	int folder;

	for (folder = 0; folder < (int)(sizeof(folders) / sizeof(folders[0])); folder++)
	{
		/* (a Custom Edition install's folder only when paths.custom_edition names one) */
		if (folder == 1 && !config_string("paths.custom_edition")[0])
			continue;
		snprintf(xbox_path, sizeof(xbox_path), "%s%s", folders[folder], file_name);
		platform_translate_path(xbox_path, path, sizeof(path));
		if (file_present(path))
			return 1;
	}
	return 0;
}

static int resource_map_name(const char *file_name)
{
	return same_name(file_name, "bitmaps.map") || same_name(file_name, "sounds.map") || same_name(file_name, "loc.map");
}

/* ---------- downloading */

/* how a file of a fetch is coming */
enum
{
	/* waiting for the archives (one file at a time) */
	_fetch_waiting,
	/* from an archive or a web seed (map_archives.c) */
	_fetch_running,
};

static struct
{
	int active;
	char level_name[NAME_SIZE];
	unsigned long version;
	struct
	{
		char name[NAME_SIZE];
		int done;
		int phase;
		const struct index_entry *entry;
		/* where it is downloaded (custom_maps\downloads) */
		char path[1200];
	} files[FETCH_MAXIMUM_FILES];
	int count;
	unsigned long started;
	int ready;
	char failure[192];
	unsigned long failed_time;
} fetch;

static void fetch_stop(void)
{
	int which;

	for (which = 0; which < fetch.count; which++)
	{
		if (!fetch.files[which].done && fetch.files[which].phase == _fetch_running)
			map_archives_stop();
	}
	fetch.active = 0;
	fetch.count = 0;
}

static void fetch_fail(const char *name, const char *reason)
{
	snprintf(fetch.failure, sizeof(fetch.failure), "Could not download %s: %s", name, reason);
	fetch.failed_time = GetTickCount() ? GetTickCount() : 1;
	platform_log("map downloads: %s", fetch.failure);
	fetch_stop();
}

int map_downloads_fetch(const char *level_name, unsigned long version, const char *files)
{
	char map_file[NAME_SIZE];
	char folder[1024];
	const char *text = files;
	int count = 0;
	const struct index_entry *wanted[FETCH_MAXIMUM_FILES];
	int which;

	/* (none while Custom Edition maps cannot be played: game.custom_edition) */
	if (!config_boolean("maps.downloads") || !config_boolean("game.custom_edition") || !level_name || !files ||
		!files[0])
	{
		return 0;
	}
	if (fetch.active)
	{
		if (same_name(fetch.level_name, level_name) && fetch.version == version)
			return 1;
		platform_log("map downloads: the download of %s's files gives way to %s's", fetch.level_name, level_name);
		fetch_stop();
	}
	level_file_name(level_name, map_file, sizeof(map_file));
	while (*text)
	{
		/* (tab-separated: some maps' names have commas, none a tab) */
		const char *tab = strchr(text, '\t');
		int length = tab ? (int)(tab - text) : (int)strlen(text);
		char name[NAME_SIZE];
		unsigned long checksum;

		if (length > 0 && length < NAME_SIZE && count < FETCH_MAXIMUM_FILES)
		{
			memcpy(name, text, (size_t)length);
			name[length] = 0;
			/* (the map is the host's version; a resource map any) */
			checksum = same_name(name, map_file) ? version : 0;
			wanted[count] = index_find_file(name, checksum);
			if (!wanted[count])
			{
				platform_log("map downloads: %s (checksum %08lX) is not in the index: not downloaded", name, checksum);
				return 0;
			}
			/* (one no source this build fetches from has: not started) */
			if (!map_archives_available(maps_index.names + wanted[count]->sources))
			{
				platform_log("map downloads: no source of maps.archives or maps.web_seeds has %s", name);
				return 0;
			}
			count++;
		}
		text = tab ? tab + 1 : text + length;
	}
	if (!count || !downloads_folder(folder, sizeof(folder)))
		return 0;
	memset(&fetch, 0, sizeof(fetch));
	for (which = 0; which < count; which++)
	{
		const struct index_entry *entry = wanted[which];

		snprintf(fetch.files[which].path, sizeof(fetch.files[which].path), "%s%s", folder,
			maps_index.names + entry->name);
		snprintf(fetch.files[which].name, sizeof(fetch.files[which].name), "%s", maps_index.names + entry->name);
		fetch.files[which].entry = entry;
		fetch.files[which].phase = _fetch_waiting;
		fetch.count++;
	}
	snprintf(fetch.level_name, sizeof(fetch.level_name), "%s", level_name);
	fetch.version = version;
	fetch.active = 1;
	fetch.started = GetTickCount();
	platform_log("map downloads: downloading %d file(s) for %s: %s", count, level_name, files);
	return 1;
}

/* a file that arrived: into custom_maps, never in place of a file there.
A map as <name>_<hash>.map, map sharing's name of its version, beside any
other version; a resource map under its name, when none is there yet */
static int fetch_file_done(int which)
{
	char folder[1024];
	char destination[1200];
	const char *name = fetch.files[which].name;
	size_t length = strlen(name);

	if (!custom_maps_folder(folder, sizeof(folder)))
		return 0;
	if (resource_map_name(name))
	{
		snprintf(destination, sizeof(destination), "%s%s", folder, name);
	}
	else
	{
		char base[NAME_SIZE];
		char xbox_folder[64];
		char digits[NAME_DIGITS + 1];

		if (length <= strlen(MAP_FILE_EXTENSION) || length >= sizeof(base))
			return 0;
		snprintf(base, sizeof(base), "%.*s", (int)(length - strlen(MAP_FILE_EXTENSION)), name);
		snprintf(xbox_folder, sizeof(xbox_folder), "%s%s\\", CUSTOM_MAPS_XBOX_PATH, DOWNLOADS_FOLDER);
		if (!map_share_name_digits(xbox_folder, base, digits))
		{
			platform_log("map downloads: %s could not be hashed for its name", name);
			return 0;
		}
		snprintf(destination, sizeof(destination), "%s%s_%s" MAP_FILE_EXTENSION, folder, base, digits);
	}
	if (file_present(destination))
	{
		/* (the same version: its name is its hash's, or a resource map
		that came meanwhile) */
		platform_log("map downloads: %s is there already", destination);
		unlink(fetch.files[which].path);
	}
	else if (!move_to_new_name(fetch.files[which].path, destination))
	{
		platform_log("map downloads: %s could not be moved into %s", name, folder);
		return 0;
	}
	fetch.files[which].done = 1;
	platform_log("map downloads: %s is %s", name, destination);
	return 1;
}

static void fetch_poll(void)
{
	int which;
	int done = 0;

	if (!fetch.active)
		return;
	for (which = 0; which < fetch.count; which++)
	{
		const struct index_entry *entry = fetch.files[which].entry;
		unsigned long long received, total;
		char archive[64];
		enum map_archives_state state;

		if (fetch.files[which].done)
		{
			done++;
			continue;
		}
		if (fetch.files[which].phase == _fetch_waiting)
		{
			int other;

			/* (one at a time: the next file waits while another's fetch is
			under way or its result not yet taken) */
			for (other = 0; other < fetch.count && (fetch.files[other].done || fetch.files[other].phase != _fetch_running);
				other++)
			{
			}
			if (other < fetch.count ||
				map_archives_poll(&received, &total, archive, sizeof(archive)) == _map_archives_running)
			{
				continue;
			}
			map_archives_stop();
			if (!map_archives_start(fetch.files[which].name, entry->size, entry->piece_length, entry->info_hash,
				maps_index.names + entry->sources, fetch.files[which].path))
			{
				fetch_fail(fetch.files[which].name, "it could not be started");
				return;
			}
			fetch.files[which].phase = _fetch_running;
			continue;
		}
		state = map_archives_poll(&received, &total, archive, sizeof(archive));
		if (state == _map_archives_running)
			continue;
		map_archives_stop();
		if (state != _map_archives_done)
		{
			fetch_fail(fetch.files[which].name, "no archive or web seed gave it (debug.txt says why)");
			return;
		}
		if (!fetch_file_done(which))
		{
			fetch_fail(fetch.files[which].name, "it could not be put in place");
			return;
		}
		done++;
	}
	if (done == fetch.count)
	{
		platform_log("map downloads: every file of %s is there (%lu s)", fetch.level_name,
			(GetTickCount() - fetch.started) / 1000);
		fetch.active = 0;
		fetch.ready = 1;
	}
}

static void size_text(unsigned long long bytes, char *text, int size)
{
	if (bytes >= 100ULL << 20)
		snprintf(text, (size_t)size, "%llu MB", bytes >> 20);
	else if (bytes >= 1ULL << 20)
		snprintf(text, (size_t)size, "%.1f MB", (double)bytes / (double)(1 << 20));
	else
		snprintf(text, (size_t)size, "%llu KB", bytes >> 10);
}

int map_downloads_fetching(char *status, int size)
{
	int which;

	if (fetch.active)
	{
		for (which = 0; which < fetch.count; which++)
		{
			unsigned long long received, total;
			char archive[64];
			char which_text[24] = "";
			char have[32], all[32];

			if (fetch.files[which].done || fetch.files[which].phase != _fetch_running)
				continue;
			if (fetch.count > 1)
				snprintf(which_text, sizeof(which_text), " (%d of %d)", which + 1, fetch.count);
			map_archives_poll(&received, &total, archive, sizeof(archive));
			size_text(received, have, sizeof(have));
			size_text(total, all, sizeof(all));
			if (total)
			{
				snprintf(status, (size_t)size, "Downloading %s%s from %s: %s of %s", fetch.files[which].name,
					which_text, archive, have, all);
			}
			else
			{
				snprintf(status, (size_t)size, "Downloading %s%s from %s", fetch.files[which].name, which_text,
					archive);
			}
			return 1;
		}
		snprintf(status, (size_t)size, "Downloading...");
		return 1;
	}
	if (fetch.failed_time && GetTickCount() - fetch.failed_time < FAILURE_SHOWN_MILLISECONDS)
	{
		snprintf(status, (size_t)size, "%s", fetch.failure);
		return 1;
	}
	return 0;
}

int map_downloads_progress(float *fraction, char *name, int size)
{
	float done = 0.0f;
	int which;

	if (!fetch.active)
		return 0;
	/* (each file an equal share: the size of one not begun is not known) */
	for (which = 0; which < fetch.count; which++)
	{
		if (fetch.files[which].done)
		{
			done += 1.0f;
		}
		else if (fetch.files[which].phase == _fetch_running)
		{
			unsigned long long received, total;
			char archive[64];

			map_archives_poll(&received, &total, archive, sizeof(archive));
			if (total)
				done += (float)((double)received / (double)total);
		}
	}
	*fraction = fetch.count ? done / (float)fetch.count : 0.0f;
	/* (the map's name: its file's, the fetch's first, without .map) */
	snprintf(name, (size_t)size, "%.*s", (int)(strlen(fetch.files[0].name) > strlen(MAP_FILE_EXTENSION) ?
		strlen(fetch.files[0].name) - strlen(MAP_FILE_EXTENSION) : strlen(fetch.files[0].name)), fetch.files[0].name);
	return 1;
}

int map_downloads_take_ready(char *level_name, int size)
{
	if (!fetch.ready)
		return 0;
	fetch.ready = 0;
	snprintf(level_name, (size_t)size, "%s", fetch.level_name);
	return 1;
}

void map_downloads_cancel(void)
{
	if (fetch.active)
	{
		platform_log("map downloads: the download of %s's files is stopped", fetch.level_name);
		fetch_stop();
	}
	fetch.ready = 0;
}

void map_downloads_poll(void)
{
	fetch_poll();
}
