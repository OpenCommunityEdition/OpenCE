/*
MAP_DOWNLOADS.H

Custom Edition maps downloaded from where the community keeps them
(map_downloads.c): a map a machine lacks when joining a game whose host does
not send it (map sharing, port/linux/game/map_share.c: a host of another
build, or one that does not share) comes from HaloNet's or HaloMaps's
archives or a web seed, as the index of the maps
(maps.index, port/assets/network/map_index.txt) names it, checked against
the index's hash.

Only integers and strings cross here: the game's units call these.
*/

#ifndef __HALO_LINUX_MAP_DOWNLOADS_H
#define __HALO_LINUX_MAP_DOWNLOADS_H

/* every frame, on the game's thread, a window or not (main.c's loop) */
void map_downloads_poll(void);

/* Map sharing's web seeds (port/linux/game/map_share.c): the URL, on the
which-th web seed (from 0), of the index's file that map sharing downloads
as file_name (<name>.map, or <name>_<hash>.map for a level named for its
hash; bitmaps.map and the other resource maps), of the header checksum
(the host's version; 0 for a resource map) and size size. 1 if there is one
(maps.downloads on, the index has the file at that size, and maps.web_seeds
that many https:// folders); else 0, as always on Android and the web. */
int map_downloads_web_seed(const char *file_name, unsigned long checksum, unsigned long long size, int which,
	char *url, int url_size);
/* starts fetching length bytes of the file at url from offset, on a thread
of its own, into a file of custom_maps/downloads: 1 if started */
int map_downloads_range_start(const char *url, unsigned long long offset, unsigned long length);
/* the range fetch's state: 0 idle, 1 under way, 2 done, 3 failed (why in
error) */
int map_downloads_range_poll(char *error, int error_size);
/* size bytes of a range fetched, from offset in it, into buffer: 1 if read */
int map_downloads_range_read(unsigned long offset, void *buffer, unsigned long size);
/* stops the range fetch, or lets go of a finished one (its file deleted) */
void map_downloads_range_stop(void);

/* the files a joining machine lacks for the host's map: level_name the
map's (custom_maps\<name>, or custom_maps\<name>_<hash>), version its
header checksum as the host sent it (0: any), files their names,
tab-separated ("<name>.map\tbitmaps.map"; custom_edition_cache_files_missing).
Starts downloading them, if the index has them all and maps.downloads is on:
returns 1, and the join is to be tried again once map_downloads_take_ready
says they are there. 0 when nothing was started */
int map_downloads_fetch(const char *level_name, unsigned long version, const char *files);
/* what the download is doing, for the menus' status line: 1 and the text
while one is under way (or just failed), else 0 */
int map_downloads_fetching(char *status, int size);
/* how a download under way goes, for the screen it shows over the menus
(port/linux/game/download_screen.c): 1 while one is, with its share done (0
to 1; 0 while its size is not known yet) and the map's name (its file's,
without .map), else 0 */
int map_downloads_progress(float *fraction, char *name, int size);
/* once, when a download's files are all in place: 1 and the level name
whose join is to be tried again */
int map_downloads_take_ready(char *level_name, int size);
/* stops a download */
void map_downloads_cancel(void);

#endif
