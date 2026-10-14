/*
INSTALL_LAYOUT.H

Where the Linux build's own files are, and where the player's go
(install_layout.c). Only strings cross here, and it calls nothing of the
game's, so that it can be compiled anywhere the C library is.
*/

#ifndef INSTALL_LAYOUT_H
#define INSTALL_LAYOUT_H

#include <stddef.h>

/* the name of the executable in a tree (bin/opence) and of its folders
(lib/opence, share/opence, share/doc/opence) */
#define INSTALL_NAME "opence"

/* the game's folder, without a separator at its end: the root of the tree
whose bin/ holds the executable, or else the executable's folder; "" if
the executable's path cannot be read */
const char *install_folder(void);

/* 1 when the executable is bin/opence of a tree (share/opence beside bin/),
0 when its folder holds everything (build/linux/halo, the flat releases) */
int install_is_tree(void);

/* 1 when a package manager installed the game, or the player cannot write
to its folder (/usr, /opt, a read-only disk): the game does not update
itself, and the player's files go to their XDG folders (those the game's
folder lacks, when it is read-only and not packaged; none without a home
folder); 0 for a portable game */
int install_is_system(void);

/* why install_is_system() is 1, for the log: "" when it is 0 */
const char *install_system_reason(void);

/* the folder of config.toml, with a separator at its end (made if it is
missing): the game's folder (an installed game's when it is read-only,
not packaged and has config.toml), or $XDG_CONFIG_HOME/halo-linux */
const char *install_config_folder(void);

/* where the game data (maps/) is copied and looked for when no other place
has it: the game's folder (an installed game's when it is read-only, not
packaged and has maps/), or $XDG_DATA_HOME/halo-linux; without a separator
at its end */
const char *install_data_folder(void);

/* the path of a read-only file the release brings (brokers.txt):
share/opence/<name> in a tree, else <name> beside the executable */
void install_shared_file(const char *name, char *path, size_t size);

#endif
