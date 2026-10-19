/*
XISO_INSTALL.C

See xiso_install.h. The extraction is port/linux/src/xiso.c, unchanged.
Everything on the disc image is untrusted: the file system is bounds-checked
by xiso.c, and each map is checked here by its header before it is kept.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xiso_install.h"
#include "xiso.h"

/* the maps a working install has (the menus, the first campaign level and
a multiplayer map); the other maps are checked when they are there */
static const char *required_maps[] = { "ui.map", "bloodgulch.map", "a10.map" };

#define HEADER_SIZE 0x800
#define MAXIMUM_CACHE_LENGTH 0x11600000UL

static unsigned long read_u32(const unsigned char *b)
{
	return (unsigned long)b[0] | (unsigned long)b[1] << 8 | (unsigned long)b[2] << 16 | (unsigned long)b[3] << 24;
}

static void set_error(char *error, int size, const char *format, const char *detail)
{
	if (error && size > 0)
		snprintf(error, (size_t)size, format, detail ? detail : "");
}

/* one cache file's header: 'head' (the bytes "daeh", stored little-endian),
version 5 as the Xbox caches (cache_file_header_verify), a plausible length,
a name that ends, and 'foot' at 0x7FC */
static int check_map(const char *folder, const char *name, char *error, int error_size)
{
	char path[1400];
	unsigned char header[HEADER_SIZE];
	FILE *file;
	size_t got;
	unsigned long length, version;

	snprintf(path, sizeof(path), "%s/%s", folder, name);
	file = fopen(path, "rb");
	if (!file)
	{
		set_error(error, error_size, "The map %s could not be opened.", name);
		return 0;
	}
	got = fread(header, 1, sizeof(header), file);
	fclose(file);
	if (got != sizeof(header))
	{
		set_error(error, error_size, "The map %s is too short to be a Halo map.", name);
		return 0;
	}
	if (memcmp(header, "daeh", 4) || memcmp(header + 0x7FC, "toof", 4))
	{
		set_error(error, error_size, "The file %s is not a Halo cache file.", name);
		return 0;
	}
	version = read_u32(header + 4);
	length = read_u32(header + 8);
	if (version != 5)
	{
		set_error(error, error_size, "The map %s is not from the Xbox game (the disc image is of another version).", name);
		return 0;
	}
	if (length < HEADER_SIZE || length > MAXIMUM_CACHE_LENGTH || !memchr(header + 0x20, 0, 0x20))
	{
		set_error(error, error_size, "The map %s has a damaged header.", name);
		return 0;
	}
	return 1;
}

static int has_map_suffix(const char *name)
{
	size_t n = strlen(name);

	return n > 4 && !strcmp(name + n - 4, ".map");
}

int xiso_verify_maps(const char *maps_folder, char *error, int error_size)
{
	DIR *directory;
	struct dirent *entry;
	unsigned i;
	char path[1400];
	struct stat info;

	for (i = 0; i < sizeof(required_maps) / sizeof(*required_maps); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", maps_folder, required_maps[i]);
		if (stat(path, &info) != 0 || !S_ISREG(info.st_mode))
		{
			set_error(error, error_size, "The disc image has no %s: it is not Halo: Combat Evolved for the Xbox.", required_maps[i]);
			return 0;
		}
	}
	directory = opendir(maps_folder);
	if (!directory)
	{
		set_error(error, error_size, "The maps folder %s could not be read.", maps_folder);
		return 0;
	}
	while ((entry = readdir(directory)) != NULL)
	{
		if (has_map_suffix(entry->d_name) && !check_map(maps_folder, entry->d_name, error, error_size))
		{
			closedir(directory);
			return 0;
		}
	}
	closedir(directory);
	return 1;
}

/* removes a folder and what is in it (never through a link); depth-limited */
static void remove_tree(const char *path, int depth)
{
	DIR *directory;
	struct dirent *entry;
	char child[1400];
	struct stat info;

	if (lstat(path, &info) != 0)
		return;
	if (!S_ISDIR(info.st_mode) || depth > 4)
	{
		unlink(path);
		return;
	}
	directory = opendir(path);
	if (directory)
	{
		while ((entry = readdir(directory)) != NULL)
		{
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
			remove_tree(child, depth + 1);
		}
		closedir(directory);
	}
	rmdir(path);
}

int xiso_install(const char *image_path, const char *data_root, xiso_install_progress progress, void *context,
	volatile int *cancel, int *cancelled, char *error, int error_size)
{
	char temporary[1100], maps_in_temporary[1200], final[1100];
	char message[512];
	struct stat info;
	int ok;

	if (cancelled)
		*cancelled = 0;
	if (strlen(data_root) > 1000)
	{
		set_error(error, error_size, "The folder name %s is too long.", data_root);
		return 0;
	}
	snprintf(temporary, sizeof(temporary), "%s/.xiso-tmp", data_root);
	snprintf(maps_in_temporary, sizeof(maps_in_temporary), "%s/maps", temporary);
	snprintf(final, sizeof(final), "%s/maps", data_root);

	if (stat(final, &info) == 0)
	{
		set_error(error, error_size, "The game data folder %s is already there.", final);
		return 0;
	}
	if (mkdir(data_root, 0755) != 0 && errno != EEXIST)
	{
		set_error(error, error_size, "Could not create the folder %s.", data_root);
		return 0;
	}
	/* (a leftover of an interrupted run) */
	remove_tree(temporary, 0);
	if (mkdir(temporary, 0755) != 0)
	{
		set_error(error, error_size, "Could not create a folder in %s.", data_root);
		return 0;
	}

	xiso_shim_set_cancel(cancel);
	message[0] = 0;
	ok = xiso_extract_maps(image_path, temporary, progress, context, message, (int)sizeof(message));
	xiso_shim_set_cancel(NULL);
	if (cancel && *cancel)
	{
		if (cancelled)
			*cancelled = 1;
		set_error(error, error_size, "Canceled.%s", NULL);
		remove_tree(temporary, 0);
		return 0;
	}
	if (ok && !xiso_verify_maps(maps_in_temporary, message, (int)sizeof(message)))
		ok = 0;
	if (ok && rename(maps_in_temporary, final) != 0)
	{
		snprintf(message, sizeof(message), "Could not create %s.", final);
		ok = 0;
	}
	if (ok)
	{
		/* the movies, which the extraction copies too when the disc has them
		(the Bink movies need both): the data works without them */
		char from[1200], to[1100];

		snprintf(from, sizeof(from), "%s/default.xbe", temporary);
		snprintf(to, sizeof(to), "%s/default.xbe", data_root);
		if (stat(from, &info) == 0)
			rename(from, to);
		snprintf(from, sizeof(from), "%s/bink", temporary);
		snprintf(to, sizeof(to), "%s/bink", data_root);
		if (stat(from, &info) == 0)
			rename(from, to);
	}
	remove_tree(temporary, 0);
	if (!ok)
		set_error(error, error_size, "%s", message);
	return ok;
}
