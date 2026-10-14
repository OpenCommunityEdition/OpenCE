/*
INSTALL_LAYOUT.C

Where the Linux build's own files are, and where the player's go.

The Linux release (tools/linux_package.py) is a tree laid out as a Unix
prefix is, which a package manager can copy to /usr as it is:

	bin/opence                   the game
	lib/opence/libSDL3.so.0      its SDL (an rpath of $ORIGIN/../lib/opence)
	share/opence/brokers.txt     read-only data (install_shared_file)
	share/applications/opence.desktop, share/icons/hicolor/<size>/apps/opence.png
	share/doc/opence/, share/licenses/opence/

The executable is in a tree when its folder is named bin and share/opence
is beside that folder. Otherwise (build/linux/halo, and the flat releases
before the tree) its folder holds everything. The game's folder is the
tree's root, or the executable's folder.

A portable game (a release unpacked in the player's folders, or a build)
keeps the player's files in the game's folder, as the flat releases did:
config.toml, the maps/ copied from a disc image (and so debug.txt), and the
self-updater's work. An installed game keeps them in the player's XDG
folders: config.toml in $XDG_CONFIG_HOME/halo-linux (~/.config/halo-linux)
and the game data in $XDG_DATA_HOME/halo-linux (~/.local/share/halo-linux,
the save root's folder, xbox_files.c); and it does not update itself
(updater.c). A game is installed when its tree has share/opence/packaged,
which a package adds, or when the player cannot write to the game's folder
(/usr, /opt).

Only the C library is called (no logging: the log's place depends on
this), so the Linux builds that run the game as a guest can compile it
into their host too.
*/

#include "install_layout.h"

#if !defined(_WIN32) && !defined(HALO_ANDROID) && !defined(HALO_WEB)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* (the C library's: the game's <sys/stat.h> is MSVC's,
port/linux/include/sys/stat.h; mode_t is an unsigned int on Linux) */
int mkdir(const char *path, unsigned int mode);

#define INSTALL_PATH_SIZE 1024
/* the folder in the XDG folders: the save root's (xbox_files.c) */
#define INSTALL_USER_FOLDER "halo-linux"

static int install_found;
static char install_executable_folder[INSTALL_PATH_SIZE];
static char install_root[INSTALL_PATH_SIZE];
static int install_tree;
static const char *install_reason = "";
static char install_config[INSTALL_PATH_SIZE];
static char install_data[INSTALL_PATH_SIZE];

/* ---------- helpers */

static int install_exists(const char *path)
{
	return access(path, F_OK) == 0;
}

/* every missing folder along path */
static void install_make_folders(const char *path)
{
	char partial[INSTALL_PATH_SIZE];
	size_t index;

	snprintf(partial, sizeof(partial), "%s", path);
	for (index = 1; partial[index]; index++)
	{
		if (partial[index] == '/')
		{
			partial[index] = 0;
			if (mkdir(partial, 0755) != 0 && errno != EEXIST)
				return;
			partial[index] = '/';
		}
	}
	mkdir(partial, 0755);
}

/* $<variable>/halo-linux, or $HOME/<fallback>/halo-linux (the XDG base
directories, which must be absolute); 0 without either */
static int install_xdg_folder(const char *variable, const char *fallback, char *path, size_t size)
{
	const char *base = getenv(variable);
	const char *home = getenv("HOME");

	if (base && base[0] == '/')
		snprintf(path, size, "%s/" INSTALL_USER_FOLDER, base);
	else if (home && home[0] == '/')
		snprintf(path, size, "%s/%s/" INSTALL_USER_FOLDER, home, fallback);
	else
		return 0;
	return 1;
}

/* ---------- finding them */

static void install_find(void)
{
	char executable[INSTALL_PATH_SIZE];
	char marker[INSTALL_PATH_SIZE + 32];
	ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
	char *slash;

	if (length > 0)
	{
		executable[length] = 0;
		slash = strrchr(executable, '/');
		if (slash)
		{
			*slash = 0;
			snprintf(install_executable_folder, sizeof(install_executable_folder), "%s",
				slash == executable ? "/" : executable);
		}
	}
	snprintf(install_root, sizeof(install_root), "%s", install_executable_folder);
	slash = strrchr(install_executable_folder, '/');
	if (slash && !strcmp(slash + 1, "bin"))
	{
		char parent[INSTALL_PATH_SIZE];

		snprintf(parent, sizeof(parent), "%.*s", (int)(slash - install_executable_folder),
			install_executable_folder);
		if (!parent[0])
			snprintf(parent, sizeof(parent), "/");
		snprintf(marker, sizeof(marker), "%s/share/" INSTALL_NAME, strcmp(parent, "/") ? parent : "");
		if (install_exists(marker))
		{
			install_tree = 1;
			snprintf(install_root, sizeof(install_root), "%s", parent);
		}
	}

	if (install_tree)
	{
		snprintf(marker, sizeof(marker), "%s/share/" INSTALL_NAME "/packaged",
			strcmp(install_root, "/") ? install_root : "");
		if (install_exists(marker))
			install_reason = "share/" INSTALL_NAME "/packaged";
	}
	if (!install_reason[0] && install_root[0] && access(install_root, W_OK) != 0)
		install_reason = "its folder is read-only";

	if (install_reason[0] && install_xdg_folder("XDG_CONFIG_HOME", ".config", install_config, sizeof(install_config)) &&
		install_xdg_folder("XDG_DATA_HOME", ".local/share", install_data, sizeof(install_data)))
	{
		install_make_folders(install_config);
		install_make_folders(install_data);
		snprintf(install_config + strlen(install_config), sizeof(install_config) - strlen(install_config), "/");
		return;
	}
	if (install_reason[0])
	{
		/* (no home folder: the game's folder, whatever it allows) */
		install_reason = "";
	}
	snprintf(install_data, sizeof(install_data), "%s", install_root);
	snprintf(install_config, sizeof(install_config), "%s%s", install_root,
		install_root[0] && strcmp(install_root, "/") ? "/" : "");
}

/* (found on the first question, which config.toml's loading asks on the
main thread before the game starts its others: no lock, which the guest
builds' C library would not have) */
static void install_ready(void)
{
	if (!install_found)
	{
		install_find();
		install_found = 1;
	}
}

/* ---------- public code */

const char *install_folder(void)
{
	install_ready();
	return install_root;
}

int install_is_tree(void)
{
	install_ready();
	return install_tree;
}

int install_is_system(void)
{
	install_ready();
	return install_reason[0] != 0;
}

const char *install_system_reason(void)
{
	install_ready();
	return install_reason;
}

const char *install_config_folder(void)
{
	install_ready();
	return install_config;
}

const char *install_data_folder(void)
{
	install_ready();
	return install_data;
}

void install_shared_file(const char *name, char *path, size_t size)
{
	install_ready();
	if (install_tree)
		snprintf(path, size, "%s/share/" INSTALL_NAME "/%s", strcmp(install_root, "/") ? install_root : "", name);
	else
		snprintf(path, size, "%s/%s", install_executable_folder, name);
}

#endif
