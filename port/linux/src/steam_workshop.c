/*
STEAM_WORKSHOP.C

The subscribed Steam Workshop items' Halo CE maps, found in place (steam_workshop.h).
*/

#include "steam_workshop.h"
#include "mcc_manifest.h"
#include "server_json.h"
#include "port_config.h"
#include "posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STEAM_APP "976730"
#define CONTENT_FOLDER "steamapps/workshop/content/" STEAM_APP
#define MAXIMUM_STEAM_ROOTS 10

static struct
{
	int scanned;
	int enabled;
	int count;
	struct steam_workshop_map maps[STEAM_WORKSHOP_MAXIMUM_MAPS];
} catalog;

static int discovery_enabled(void)
{
#if defined(HALO_ANDROID) || defined(HALO_WEB)
	return 0;
#else
	return config_boolean("game.workshop_maps");
#endif
}

/* ---------- parsing */

static void copy_text(char *out, unsigned size, const char *text)
{
	unsigned length = (unsigned)strlen(text);

	if (length >= size)
	{
		length = size - 1;
		/* not inside a UTF-8 character */
		while (length && (((unsigned char)text[length]) & 0xC0) == 0x80)
			length--;
	}
	memcpy(out, text, length);
	out[length] = 0;
}

static int parse(struct server_json_document *document, const char *text, int length)
{
	if (!text || length <= 0 || length > STEAM_WORKSHOP_MAXIMUM_JSON ||
		!server_json_parse(document, text, length, 16))
		return 0;
	if (document->count < 1 || document->tokens[0].type != _server_json_object)
	{
		server_json_document_free(document);
		return 0;
	}
	return 1;
}

/* a plain item-relative path: not empty, not absolute or drive-lettered, no "..", no control characters */
static int path_is_plain(const char *path)
{
	const char *c;

	if (!path[0] || path[0] == '/' || path[0] == '\\' || path[1] == ':')
		return 0;
	for (c = path; *c; c++)
	{
		if ((unsigned char)*c < 0x20 || *c == 0x7F || (c[0] == '.' && c[1] == '.'))
			return 0;
	}
	return 1;
}

static void slashes(char *path)
{
	for (; *path; path++)
	{
		if (*path == '\\')
			*path = '/';
	}
}

static void neutral_title(struct server_json_document const *document, char *out)
{
	char text[256];

	out[0] = 0;
	if (server_json_get_string(document, server_json_member(document, server_json_member(document, 0, "Title"),
		"Neutral"), text, (int)sizeof(text)))
		copy_text(out, STEAM_WORKSHOP_TITLE_SIZE, text);
}

int steam_workshop_read_item(const char *text, int length, struct steam_workshop_item_text *item)
{
	struct server_json_document document;
	char engine[32];
	int maps, index, count;

	memset(item, 0, sizeof(*item));
	if (!parse(&document, text, length))
		return 0;
	neutral_title(&document, item->title);
	if (server_json_get_string(&document, server_json_member(&document, 0, "Engine"), engine, (int)sizeof(engine)))
		item->halo1 = !strcmp(engine, "Halo1");
	server_json_get_boolean(&document, server_json_member(&document,
		server_json_member(&document, 0, "GameModContents"), "HasCampaign"), &item->has_campaign);
	maps = server_json_member(&document, server_json_member(&document, 0, "GameModContents"), "MultiplayerMaps");
	if (maps >= 0)
	{
		if (document.tokens[maps].type != _server_json_array || document.tokens[maps].size > STEAM_WORKSHOP_MAXIMUM_ITEM_MAPS)
		{
			if (item->halo1 && document.tokens[maps].type == _server_json_array)
				fprintf(stderr, "MCC package '%s': MultiplayerMaps has %d entries; the supported limit is %d. Split the collection into smaller packages.\n",
					item->title, document.tokens[maps].size, STEAM_WORKSHOP_MAXIMUM_ITEM_MAPS);
			server_json_document_free(&document);
			return 0;
		}
		count = document.tokens[maps].size;
		for (index = 0; index < count; index++)
		{
			char *path = item->map_json[index];

			if (!server_json_get_string(&document, server_json_element(&document, maps, index), path,
				(int)sizeof(item->map_json[0])) || !path_is_plain(path))
			{
				server_json_document_free(&document);
				memset(item, 0, sizeof(*item));
				return 0;
			}
			slashes(path);
		}
		item->map_count = count;
	}
	server_json_document_free(&document);
	return 1;
}

int steam_workshop_read_map(const char *text, int length, struct steam_workshop_map_text *map)
{
	struct server_json_document document;
	char scenario[256], thumbnail[128];
	const char *name, *c;
	unsigned name_length;

	memset(map, 0, sizeof(*map));
	if (!parse(&document, text, length))
		return 0;
	if (!server_json_get_string(&document, server_json_member(&document, 0, "ScenarioFile"), scenario,
		(int)sizeof(scenario)))
	{
		server_json_document_free(&document);
		return 0;
	}
	name = scenario;
	for (c = scenario; *c; c++)
	{
		if (*c == '/' || *c == '\\')
			name = c + 1;
	}
	for (c = name; *c; c++)
	{
		/* the name becomes a file name: no separator, drive colon or control character */
		if ((unsigned char)*c < 0x20 || *c == 0x7F || *c == ':')
			break;
	}
	if (!name[0] || *c || strlen(name) >= sizeof(map->name) || !strcmp(name, ".") || !strcmp(name, ".."))
	{
		server_json_document_free(&document);
		memset(map, 0, sizeof(*map));
		return 0;
	}
	strcpy(map->name, name);
	/* (the file's name, whether or not the manifest wrote its extension) */
	name_length = (unsigned)strlen(map->name);
	if (name_length > 4 && !strcmp(map->name + name_length - 4, ".map"))
		map->name[name_length - 4] = 0;
	neutral_title(&document, map->title);
	if (server_json_get_string(&document, server_json_member(&document, server_json_member(&document, 0, "Images"),
		"Thumbnail"), thumbnail, (int)sizeof(thumbnail)) && path_is_plain(thumbnail))
	{
		slashes(thumbnail);
		strcpy(map->thumbnail, thumbnail);
	}
	if (server_json_get_string(&document, server_json_member(&document, server_json_member(&document, 0, "Images"),
		"Large"), thumbnail, (int)sizeof(thumbnail)) && path_is_plain(thumbnail))
	{
		slashes(thumbnail);
		strcpy(map->large, thumbnail);
	}
	server_json_document_free(&document);
	return 1;
}

int steam_workshop_read_libraries(const char *text, int length, char (*paths)[STEAM_WORKSHOP_PATH_SIZE], int maximum)
{
	int count = 0, at = 0;

	/* "path"<white space>"value", the value's \\ and \" unescaped */
	while (count < maximum && at + 6 <= length)
	{
		int start;
		unsigned used = 0;

		if (memcmp(text + at, "\"path\"", 6))
		{
			at++;
			continue;
		}
		at += 6;
		while (at < length && (text[at] == ' ' || text[at] == '\t'))
			at++;
		if (at >= length || text[at] != '"')
			continue;
		start = ++at;
		while (at < length && text[at] != '"' && text[at] != '\n' && used + 1 < STEAM_WORKSHOP_PATH_SIZE)
		{
			if (text[at] == '\\' && at + 1 < length && (text[at + 1] == '\\' || text[at + 1] == '"'))
				at++;
			paths[count][used++] = text[at++];
		}
		paths[count][used] = 0;
		if (at < length && text[at] == '"' && at > start && used)
			count++;
	}
	return count;
}

/* ---------- files */

static char *read_file(const char *path, int *length)
{
	FILE *file = fopen(path, "rb");
	char *text;
	size_t got;

	*length = 0;
	if (!file)
		return NULL;
	text = malloc(STEAM_WORKSHOP_MAXIMUM_JSON + 2);
	if (!text)
	{
		fclose(file);
		return NULL;
	}
	got = fread(text, 1, STEAM_WORKSHOP_MAXIMUM_JSON + 1, file);
	fclose(file);
	if (!got)
	{
		free(text);
		return NULL;
	}
	text[got] = 0;
	/* one byte past the limit is "too large": the parser refuses it */
	*length = (int)got;
	return text;
}

static int is_directory(const char *path)
{
	struct posix_file_information information;

	return posix_stat(path, &information) == 0 && (information.flags & _posix_file_is_directory);
}

static int is_file(const char *path)
{
	struct posix_file_information information;

	return posix_stat(path, &information) == 0 && !(information.flags & _posix_file_is_directory);
}

static int join(char *out, unsigned size, const char *folder, const char *name)
{
	int written = snprintf(out, size, "%s/%s", folder, name);

	return written > 0 && (unsigned)written < size;
}

/* an item-relative path ('/' separators) to its host path, each part found case-blind: 1 if it exists */
static int resolve(const char *folder, const char *relative, char *out, unsigned size)
{
	char path[STEAM_WORKSHOP_PATH_SIZE], part[128], found[256], next[STEAM_WORKSHOP_PATH_SIZE];
	const char *c = relative;

	if (strlen(folder) >= sizeof(path))
		return 0;
	strcpy(path, folder);
	while (*c)
	{
		unsigned length = 0;

		while (*c == '/')
			c++;
		if (!*c)
			break;
		while (c[length] && c[length] != '/')
			length++;
		if (length >= sizeof(part))
			return 0;
		memcpy(part, c, length);
		part[length] = 0;
		c += length;
		if (!posix_find_entry_case_insensitive(path, part, found, sizeof(found)) ||
			!join(next, sizeof(next), path, found))
			return 0;
		strcpy(path, next);
	}
	if (!strcmp(path, folder) || strlen(path) >= size)
		return 0;
	strcpy(out, path);
	return 1;
}

static int has_item(const char *item_id)
{
	int index;

	for (index = 0; index < catalog.count; index++)
	{
		if (!strcmp(catalog.maps[index].item_id, item_id))
			return 1;
	}
	return 0;
}

static int digits(const char *text)
{
	unsigned length = (unsigned)strlen(text), index;

	if (!length || length >= STEAM_WORKSHOP_ID_SIZE)
		return 0;
	for (index = 0; index < length; index++)
	{
		if (text[index] < '0' || text[index] > '9')
			return 0;
	}
	return 1;
}

static void scan_item(const char *item_folder, const char *item_id)
{
	static struct steam_workshop_item_text item;
	static struct steam_workshop_map_text map;
	static struct mcc_manifest campaign;
	char path[STEAM_WORKSHOP_PATH_SIZE], maps_folder[STEAM_WORKSHOP_PATH_SIZE], found[256], file_name[96];
	char reason[128];
	char *text;
	int length, index, count;

	if (!resolve(item_folder, "ModInfo.json", path, sizeof(path)) || !(text = read_file(path, &length)))
		return;
	index = steam_workshop_read_item(text, length, &item);
	free(text);
	if (!index || !item.halo1 || !resolve(item_folder, "maps", maps_folder, sizeof(maps_folder)) ||
		!is_directory(maps_folder))
		return;
	campaign.count = 0;
	if (item.has_campaign && config_boolean("game.mcc_campaigns") &&
		mcc_manifest_load(item_folder, &campaign, reason) < 0)
		fprintf(stderr, "MCC package '%s' (%s): campaign metadata rejected: %s\n",
			item.title, item_id, reason);
	count = item.map_count + campaign.count;
	for (index = 0; index < count && catalog.count < STEAM_WORKSHOP_MAXIMUM_MAPS; index++)
	{
		struct steam_workshop_map *entry = &catalog.maps[catalog.count];

		if (index < item.map_count)
		{
			if (!resolve(item_folder, item.map_json[index], path, sizeof(path)) || !(text = read_file(path, &length)))
				continue;
			length = steam_workshop_read_map(text, length, &map);
			free(text);
		}
		else
		{
			map = campaign.missions[index - item.map_count];
			length = 1;
		}
		snprintf(file_name, sizeof(file_name), "%s.map", map.name);
		if (!length || !posix_find_entry_case_insensitive(maps_folder, file_name, found, sizeof(found)) ||
			!join(entry->path, sizeof(entry->path), maps_folder, found))
			continue;
		/* the name as the file is spelled on disk */
		found[strlen(found) - 4] = 0;
		if (!found[0] || strlen(found) >= sizeof(entry->name) || !is_file(entry->path))
			continue;
		copy_text(entry->item_id, sizeof(entry->item_id), item_id);
		copy_text(entry->item_title, sizeof(entry->item_title), item.title);
		copy_text(entry->name, sizeof(entry->name), found);
		copy_text(entry->title, sizeof(entry->title), map.title[0] ? map.title : found);
		entry->preview[0] = 0;
		if (map.thumbnail[0] && !resolve(item_folder, map.thumbnail, entry->preview, sizeof(entry->preview)))
			entry->preview[0] = 0;
		catalog.count++;
	}
}

static void scan_library(const char *library)
{
	char content[STEAM_WORKSHOP_PATH_SIZE], item_folder[STEAM_WORKSHOP_PATH_SIZE], name[256];
	void *directory;

	if (!join(content, sizeof(content), library, CONTENT_FOLDER) || !(directory = posix_directory_open(content)))
		return;
	while (catalog.count < STEAM_WORKSHOP_MAXIMUM_MAPS && posix_directory_next(directory, name, sizeof(name)))
	{
		/* an item already scanned is the same item seen through another path to its library */
		if (!digits(name) || has_item(name) || !join(item_folder, sizeof(item_folder), content, name) ||
			!is_directory(item_folder))
			continue;
		scan_item(item_folder, name);
	}
	posix_directory_close(directory);
}

#ifndef _WIN32
static int linux_root(char *path, const char *base, const char *suffix)
{
	int written;
	const char *c;

	/* Environment hints are literal absolute paths, never shell expressions.
	 * Reject long values rather than looking in a truncated, different folder. */
	if (!base || base[0] != '/')
		return 0;
	for (c = base; *c; c++)
	{
		if ((unsigned char)*c < 0x20 || *c == 0x7F)
			return 0;
	}
	written = snprintf(path, STEAM_WORKSHOP_PATH_SIZE, "%s%s%s", base, suffix[0] ? "/" : "", suffix);
	return written > 0 && written < STEAM_WORKSHOP_PATH_SIZE;
}

static int linux_roots(char (*paths)[STEAM_WORKSHOP_PATH_SIZE], int maximum)
{
	static const char *const places[] = { ".steam/steam", ".steam/root", ".local/share/Steam",
		".steam/debian-installation", ".var/app/com.valvesoftware.Steam/.local/share/Steam",
		"snap/steam/common/.local/share/Steam" };
	const char *home = getenv("HOME");
	int count = 0, index;

	/* Steam launch contexts can supply their client root. XDG relocates the
	 * user's data directory; old home locations remain available as fallbacks. */
	if (count < maximum && linux_root(paths[count], getenv("STEAM_COMPAT_CLIENT_INSTALL_PATH"), ""))
		count++;
	if (count < maximum && linux_root(paths[count], getenv("XDG_DATA_HOME"), "Steam"))
		count++;
	for (index = 0; index < (int)(sizeof(places) / sizeof(places[0])) && count < maximum; index++)
	{
		if (linux_root(paths[count], home, places[index]))
			count++;
	}
	return count;
}
#endif

/* the Steam folders to look in, then the libraries those name: library paths, each once */
static int libraries(char (*paths)[STEAM_WORKSHOP_PATH_SIZE], int maximum)
{
	char roots[MAXIMUM_STEAM_ROOTS][STEAM_WORKSHOP_PATH_SIZE];
	int root_count = 0, count = 0, root, index;
	const char *setting = config_string("paths.steam");

	if (setting && setting[0])
		snprintf(roots[root_count++], STEAM_WORKSHOP_PATH_SIZE, "%s", setting);
#ifdef _WIN32
	{
		const char *program_files[] = { getenv("ProgramFiles(x86)"), getenv("ProgramFiles") };

		/* Keep the configured root ahead of automatic discovery. */
		root_count += steam_workshop_windows_roots(roots + root_count, MAXIMUM_STEAM_ROOTS - root_count);
		for (index = 0; index < 2; index++)
		{
			if (program_files[index] && program_files[index][0] && root_count < MAXIMUM_STEAM_ROOTS)
				snprintf(roots[root_count++], STEAM_WORKSHOP_PATH_SIZE, "%s/Steam", program_files[index]);
		}
		if (root_count < MAXIMUM_STEAM_ROOTS)
			snprintf(roots[root_count++], STEAM_WORKSHOP_PATH_SIZE, "C:/Program Files (x86)/Steam");
	}
#else
	root_count += linux_roots(roots + root_count, MAXIMUM_STEAM_ROOTS - root_count);
#endif
	for (root = 0; root < root_count && count < maximum; root++)
	{
		char vdf[STEAM_WORKSHOP_PATH_SIZE], (*named)[STEAM_WORKSHOP_PATH_SIZE];
		char *text;
		int length, named_count, other;

		/* the root itself is a library (a setting may name a library folder as well) */
		for (other = 0; other < count && strcmp(paths[other], roots[root]); other++)
			;
		if (other == count && is_directory(roots[root]))
			snprintf(paths[count++], STEAM_WORKSHOP_PATH_SIZE, "%s", roots[root]);
		if (!join(vdf, sizeof(vdf), roots[root], "steamapps/libraryfolders.vdf") ||
			!(text = read_file(vdf, &length)))
			continue;
		named = malloc(sizeof(*named) * STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
		if (named)
		{
			named_count = steam_workshop_read_libraries(text, length, named, STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
			for (index = 0; index < named_count && count < maximum; index++)
			{
				for (other = 0; other < count && strcmp(paths[other], named[index]); other++)
					;
				if (other == count && is_directory(named[index]))
					snprintf(paths[count++], STEAM_WORKSHOP_PATH_SIZE, "%s", named[index]);
			}
			free(named);
		}
		free(text);
	}
	return count;
}

int steam_workshop_scan(void)
{
	char (*paths)[STEAM_WORKSHOP_PATH_SIZE];
	int count, index;

	catalog.scanned = 1;
	catalog.count = 0;
	catalog.enabled = discovery_enabled();
	if (!catalog.enabled)
		return 0;
	paths = malloc(sizeof(*paths) * 2 * STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
	if (!paths)
		return 0;
	count = libraries(paths, 2 * STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
	for (index = 0; index < count && catalog.count < STEAM_WORKSHOP_MAXIMUM_MAPS; index++)
		scan_library(paths[index]);
	free(paths);
	return catalog.count;
}

int steam_workshop_count(void)
{
	if (!catalog.scanned || catalog.enabled != discovery_enabled())
		steam_workshop_scan();
	return catalog.count;
}

const struct steam_workshop_map *steam_workshop_map(int index)
{
	return index >= 0 && index < steam_workshop_count() ? &catalog.maps[index] : NULL;
}

static const struct steam_workshop_map *item_map(const char *item_id)
{
	int index, count = steam_workshop_count();

	for (index = 0; item_id && index < count; index++)
	{
		if (!strcmp(catalog.maps[index].item_id, item_id))
			return &catalog.maps[index];
	}
	return NULL;
}

int steam_workshop_package_folder(const char *item_id, char *folder, unsigned long size)
{
	const struct steam_workshop_map *map = item_map(item_id);
	char *last;

	if (!map || strlen(map->path) >= size)
		return 0;
	strcpy(folder, map->path);
	last = strrchr(folder, '/');
	if (!last)
		last = strrchr(folder, '\\');
	if (!last)
		return 0;
	*last = 0;
	return 1;
}

/* Only the two resource names in MCC package/shared namespaces are redirected.
 * Keep validation, complete-reference matching and fingerprints in mcc_resources.c. */
int steam_workshop_resource_path(const char *request, const char *data_root, char *path, unsigned long size)
{
	char relative[128], local[160], package[STEAM_WORKSHOP_PATH_SIZE];
	char found[STEAM_WORKSHOP_PATH_SIZE], resource[64], *separator;
	char (*paths)[STEAM_WORKSHOP_PATH_SIZE];
	const char *installed;
	unsigned index;
	int count, library, result = 0;

	if (!discovery_enabled() || strlen(request) >= sizeof(relative) ||
		strlen(data_root) >= STEAM_WORKSHOP_PATH_SIZE)
		return 0;
	for (index = 0; request[index]; index++)
	{
		char c = request[index];
		relative[index] = c == '\\' ? '/' : c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
	}
	relative[index] = 0;
	separator = strrchr(relative, '/');
	if (!separator || (strcmp(separator + 1, "bitmaps.map") && strcmp(separator + 1, "sounds.map")))
		return 0;
	strcpy(resource, separator + 1);
	*separator = 0;
	if (digits(relative))
	{
		/* A manually installed package, or any beside-level resource entry,
		 * stays authoritative even when incomplete, invalid or unreadable. */
		snprintf(local, sizeof(local), "mcc_maps/%s", relative);
		if (resolve(data_root, local, found, sizeof(found)) ||
			!steam_workshop_package_folder(relative, package, sizeof(package)) ||
			resolve(package, resource, found, sizeof(found)))
			return 0;
		slashes(package);
		separator = strrchr(package, '/');
		if (!separator) return 0;
		*separator = 0;
		snprintf(local, sizeof(local), "halo1/maps/%s", resource);
		return resolve(package, local, path, size);
	}
	if (!strcmp(relative, "_shared/singleplayer"))
		installed = "steamapps/common/Halo The Master Chief Collection/halo1/maps";
	else if (!strcmp(relative, "_shared/multiplayer"))
		installed = "steamapps/common/Halo The Master Chief Collection/halo1/maps/custom_edition";
	else
		return 0;
	snprintf(local, sizeof(local), "mcc_maps/%s/%s", relative, resource);
	if (resolve(data_root, local, found, sizeof(found)))
		return 0;
	/* The same ordered Steam libraries as map discovery. An explicit local
	 * shared file wins; no other Workshop item's resources are searched. */
	paths = malloc(sizeof(*paths) * 2 * STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
	if (!paths) return 0;
	count = libraries(paths, 2 * STEAM_WORKSHOP_MAXIMUM_LIBRARIES);
	for (library = 0; library < count; library++)
	{
		if (resolve(paths[library], installed, package, sizeof(package)) &&
			resolve(package, resource, path, size))
		{
			result = 1;
			break;
		}
	}
	free(paths);
	return result;
}

int steam_workshop_item_title(const char *item_id, char *title, unsigned long size)
{
	const struct steam_workshop_map *map = item_map(item_id);

	if (!map)
		return 0;
	copy_text(title, (unsigned)size, map->item_title);
	return 1;
}
