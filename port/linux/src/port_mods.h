/*
PORT_MODS.H

The mods in the data root's mods folder and their tag files (port_mods.c),
which the game plays over a map's own (port/linux/game/mods.c, which
declares these for itself: keep the two the same).
*/

#ifndef PORT_MODS_H
#define PORT_MODS_H

/* finds the mods again, and lists the files of those that are on: how
many are on. Every file index from before is gone */
long port_mods_scan(void);
/* the mods that are on, in the load order, a line each: its folder, and
its version if its mod.toml says one ("trollface 1.0"); how many fitted
into text (a line that does not fit is left out whole) */
int port_mods_list(char *text, unsigned long size);
/* how many times the mods have been found (port_mods_scan) */
unsigned long port_mods_scans(void);
/* how many mods the last port_mods_scan found that are on */
long port_mods_played(void);

/* the file a tag's path names (its name and extension: "sound\x.sound",
any case, either slash) of the mods played on the map `map_name` (its
file's name: "a10"), the one loaded last; -1 if none has it */
long port_mods_file_find(const char *tag_file, const char *map_name);
/* a file's size and modification time now; 0 if it has gone */
int port_mods_file_stamp(long file, unsigned long *size, unsigned long stamp[2]);
/* reads size bytes of a file at offset into buffer (the Xbox's memory too);
0 unless all of them were read */
int port_mods_file_read(long file, unsigned long offset, void *buffer, unsigned long size);
/* the place in the order the mods' files were listed in of a file's mod
(later plays over earlier), or -1 */
long port_mods_file_order(long file);
/* the folder name of a file's mod, and the file's path (for messages) */
const char *port_mods_file_mod(long file);
const char *port_mods_file_path(long file);
/* whether the mod of a file says its bitmaps' multipurpose maps and HUD
meters keep their channels where the Xbox does (mod.toml's channel_order) */
int port_mods_file_xbox_channels(long file);

/* the mods found, played or not, the folder of each (in the folders' order),
and a line about each for the console */
long port_mods_count(void);
const char *port_mods_folder(long index);
int port_mods_describe(long index, char *text, unsigned long size, const char *map_name);

/* ---------- the load order (the Mods menu's)

What these change is the mods' state as port_mods_save keeps it: the files
played follow it at the next port_mods_scan. */

/* what is wrong with a mod, worst first (port_mods_status) */
enum
{
	_port_mod_fine,
	/* it loads on the wrong side of a mod that is on, as it asks itself */
	_port_mod_out_of_order,
	/* a mod that is on says it cannot be played with it */
	_port_mod_incompatible,
	/* it cannot be played (made for a later build) */
	_port_mod_unplayable,
};

/* the mod at a place in the load order (0 loads first), or -1 */
long port_mods_at(long position);
/* a mod's name (its mod.toml's, else its folder's), and whether it is on */
const char *port_mods_name(long index);
int port_mods_on(long index);
/* what is wrong with a mod, and a line saying so into text (else empty) */
int port_mods_status(long index, char *text, unsigned long size);
/* what its mod.toml says of a mod, a line each (its version, authors,
folder and maps), then its description; 0 if there is no such mod */
int port_mods_details(long index, char *text, unsigned long size);
/* moves the mod at `position` by `places` (later: positive): its place now */
long port_mods_move(long position, long places);
/* turns a mod on or off */
void port_mods_turn(long index, int on);
/* keeps the load order and the mods turned off in the mods folder's
load_order.toml; 0 if it cannot be written */
int port_mods_save(void);
/* the load order and the mods turned off as load_order.toml keeps them */
void port_mods_revert(void);

/* The mods that are on and that other mods that are on say they cannot be
played with: how many, and a line for each into text ("A says it cannot be
played with B."), and their names into names. port_mods_disable_conflicting
turns them off (port_mods_save keeps it). */
int port_mods_conflicts(char *text, unsigned long size);
int port_mods_conflicting_names(char *text, unsigned long size);
void port_mods_disable_conflicting(void);

/* At startup (platform_sdl_initialize, before the window opens): finds the
mods, and when some that are on say they cannot be played with others that
are on, asks the player whether to play them all anyway, turn the others
off (and keep that), or quit. */
void port_mods_startup(void);

#endif
