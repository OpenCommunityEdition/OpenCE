/*
MAP_ARCHIVES.H

A Custom Edition map fetched from where the community keeps it
(map_archives.c): out of the zip HaloNet (maps.halonet.net) or HaloMaps
(halomaps.org) keeps it in, or from a web seed (maps.web_seeds), for
map_downloads.c; each checked against the index's hash of it. One at a
time, on a thread of its own.
*/

#ifndef __HALO_LINUX_MAP_ARCHIVES_H
#define __HALO_LINUX_MAP_ARCHIVES_H

enum map_archives_state
{
	_map_archives_idle,
	/* fetching a zip, or taking the map out of it */
	_map_archives_running,
	/* the map is at the path given, and is the index's (its info hash the
	index's: map_archives_verify) */
	_map_archives_done,
	/* no archive gave it */
	_map_archives_failed,
};

/* Starts fetching the file named (<size> bytes) out of one of the zips that
sources names (the index's, tab-separated: "halonet:<zip file name>",
"halomaps:<file id>"), the archives tried in maps.archives's order, then
from the web seeds, into path (with path.zip, path.part and path.html beside
it meanwhile). What a source gives that is not the index's (info_hash, of
pieces of piece_length) is let go, and the next source tried. 1 if
started; 0 if no source has it, the build has none (Android, the web), or a
fetch is under way. */
int map_archives_start(const char *file_name, unsigned long long size, unsigned long piece_length,
	const char *info_hash, const char *sources, const char *path);

/* How the fetch goes: its state, the bytes of the zip come so far and of
the whole (0 when not known), and the archive's name ("HaloNet") */
enum map_archives_state map_archives_poll(unsigned long long *received, unsigned long long *total, char *archive,
	int archive_size);

/* Stops a fetch under way (its files deleted), or lets go of a finished
one, so that another can start */
void map_archives_stop(void);

/* Whether sources (as map_archives_start's) names an archive of
maps.archives this build can fetch from */
int map_archives_available(const char *sources);

/* Map sharing's web seeds (port/linux/game/map_share.c): the URL of the
file named on the which-th web seed of maps.web_seeds (from 0); 0 if there
is none, or this build fetches nothing (Android, the web) */
int map_archives_web_seed_url(const char *file_name, int which, char *url, int url_size);

/* Starts fetching length bytes of the file at url from offset (an HTTP
range) into the file at path, on a thread of its own, beside the archives'
fetch: 1 if started; 0 if one is under way or this build fetches nothing.
Map sharing checks each piece of it against the piece's hash. */
int map_archives_range_start(const char *url, unsigned long long offset, unsigned long length, const char *path);

/* How the range fetch goes (done: the bytes are in its file; failed: why,
in error) */
enum map_archives_state map_archives_range_poll(char *error, int error_size);

/* Stops a range fetch under way (its file deleted), or lets go of a
finished one */
void map_archives_range_stop(void);

/* Takes the file named (<size> bytes, the first of the name and size in any
folder) out of the zip at zip_path into path: stored or deflated, its CRC
checked (map_archives_zip.c). 1 on success; else 0 and why in error.
(tools/map_download_check.c's "unzip", for the tests.) */
int map_archives_unzip(const char *zip_path, const char *file_name, unsigned long long size, const char *path,
	char *error, int error_size);

/* size bytes of the open file at offset into buffer; 1 if all were read */
int map_archives_read_at(int file, unsigned long long offset, void *buffer, unsigned long size);

/* Whether the file at path is the index's file named (<size> bytes): its
BitTorrent info hash (40 hexadecimal digits, of the single-file torrent
{length, name, piece length, pieces} with pieces of piece_length) is
info_hash. 1 if it is; else 0 and why in error. (tools/map_download_check.c
"verify", for the tests.) */
int map_archives_verify(const char *path, const char *file_name, unsigned long long size, unsigned long piece_length,
	const char *info_hash, char *error, int error_size);

#endif
