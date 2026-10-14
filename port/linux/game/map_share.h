/*
MAP_SHARE.H

Custom Edition maps shared over network games (map_share.c, map_share_files.c;
port/linux/NETCODE.md, "Map sharing").

A host's game is on a map by its own name (custom_maps\hugeass, as every
screen shows it), and its game record's map version is the first 32 bits of
the map's hash (map_share_hash_version), which says which version of the map
it is. A client takes its file of that name and version (custom_edition_cache.c),
and one without it asks the host for it. If the host shares maps
(network.map_share), it says which files make up the map (the map, and the
resource maps it needs that the client lacks) and which other players have
them, and the client downloads them in pieces from all of them at once, as a
torrent's peers do, each piece checked against its hash. The downloaded map
takes its own name (hugeass.map), or, beside another version of that name,
the name and its hash's first digits (hugeass_1a2b3c4d.map). No map is
renamed: one a build of before named for its hash has its name given back.
*/

#ifndef __MAP_SHARE_H
#define __MAP_SHARE_H

/* ---------- constants */

enum
{
	/* a file's hash, of its pieces' (BLAKE2b, 32 bytes) */
	MAP_SHARE_HASH_BYTES = 32,
	/* the hexadecimal digits of the hash the file name of a second version
	of a map ends in, after _ */
	MAP_SHARE_NAME_DIGITS = 8,
	/* a file is hashed and sent in pieces of this size (the last shorter),
	each piece in blocks of this */
	MAP_SHARE_PIECE_BYTES = 0x40000,
	MAP_SHARE_BLOCK_BYTES = 0x4000,
	/* the files of a map: the map, and resource maps (as enum
	resource_map_type numbers them, cache_file_formats.h) */
	_map_share_file_map = 0,
	_map_share_file_bitmaps,
	_map_share_file_sounds,
	_map_share_file_locale,
	NUMBER_OF_MAP_SHARE_FILE_KINDS,
	/* a path of the platform's (d:\custom_maps\...) */
	MAP_SHARE_PATH_SIZE = 256,
};

/* ---------- prototypes/MAP_SHARE_FILES.C */

/* Whether a map's file name (without its extension) ends in _ and
MAP_SHARE_NAME_DIGITS lowercase hexadecimal digits: named for its hash. */
boolean map_share_name_hashed(
	char const *name);
/* Whether a map's name (from another machine's level name) is only a file
name of the custom_maps folder: printable ASCII, no separator or character
a file system refuses, no dot or space at either end (so never "." or
".."), and not a name Windows takes for a device (CON, NUL, COM1 and the
like, before the first dot). */
boolean map_share_name_safe(
	char const *name);
/* the length of a map's file name without the hash it ends in (all of it,
when it ends in none) */
short map_share_name_base_length(
	char const *name);

/* The map lists found the map file <folder><name>.map, a Custom Edition
map, whose name ends in _ and a hash's first digits, as a build of before
named every map of the custom_maps folder: if no map there has its name
without them, it is hashed, on a thread of its own, and if the hash is its
name's, given its name back (<name>.map, with its picture and description,
<name>.bmp and <name>.txt); the map lists are scanned again once it has been
(custom_edition_maps_look_again). Not while the map is loaded: it is asked
for again. */
void map_share_map_found(
	char const *folder,
	char const *name);

/* A host's game is on the level `level_name` (a buffer of `size`), as the
menus name it: a Custom Edition map's is made its file's own name
(custom_maps\hugeass, for hugeass.map, or for hugeass_1a2b3c4d.map, another
version's, whose hash that is), which every screen shows, and its version
kept for map_share_level_name_version (network_server_manager.c). */
void map_share_level_name_choose(
	char *level_name,
	long size);
/* the version of the map that level name was last made for, if it was */
boolean map_share_level_name_version(
	char const *level_name,
	unsigned long *version);

/* A map file's hash, read here once (a fraction of a second) and kept for
the rest of the run, by its path, size and the time it was written. FALSE
if it cannot be read. */
boolean map_share_map_hash(
	char const *path,
	byte *hash);
/* ... a hash known otherwise (a download's) kept */
void map_share_map_hash_remember(
	char const *path,
	byte const *hash);
/* the version of a map of this hash, a game record's: its first 32 bits
(the digits a file name of another version ends in), never 0 */
unsigned long map_share_hash_version(
	byte const *hash);
/* the version of the map file at `path` */
boolean map_share_map_version(
	char const *path,
	unsigned long *version);

/* ---------- prototypes/MAP_SHARE.C */

/* A client joining a network game on the level `level_name` (with the
host's `version`, map_share_hash_version's) that this machine cannot play
(custom_edition_cache_present said why in `message`): TRUE if it asks the
host for it instead (a Custom Edition map of a version it lacks, or one
whose resource maps are missing, with network.map_download on). The download
runs in the lobby (the host waits for it to precache), or, for a game in
progress, before this machine's players join. */
boolean map_share_download_begin(
	char const *level_name,
	unsigned long version,
	char const *message);
/* the version of the map the network game this machine is in is on, if it
is on the level `level_name` (or a map of its name: custom_edition_cache.c
takes the file of that version) */
boolean map_share_game_map_version(
	char const *level_name,
	unsigned long *version);
/* whether the level's files are being downloaded (cache_files.c gives the
map no time to precache until they are) */
boolean map_share_downloading(
	char const *level_name);
/* whether this machine's players wait to join the host's game: until its
settings name the map (a game in progress would start them at once), and
for a game in progress until the map is downloaded (network_client_manager.c) */
boolean map_share_holds_players(
	void);
/* (the host) whether a client machine is downloading the game's map, as it
said lately, and what it says it has grew lately: it is not dropped for
adding no players or loading slowly (network_server_manager.c) */
boolean map_share_machine_downloading(
	long machine_index);
/* (the host) whether that download is not finished: the machines that have
the map do not wait for it (network_server_manager.c) */
boolean map_share_machine_needs_map(
	long machine_index);

/* the distributed messages of map sharing (network_distributed.c, and the
host's message handler), in the lobby too: from_stream says the message came
over the sender's connection, not as a datagram (anyone can send one) */
boolean map_share_handles_message(
	word const *message,
	word size);
void map_share_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream);

/* every frame (main.c): the hashing, the transfers and the messages */
void map_share_update(
	void);
/* the download's progress, drawn over the screen (interface.c) */
void map_share_render(
	void);

/* ---------- prototypes/MAP_SHARE_FILES.C (map_share.c's) */

/* A file hashed for sharing: its size, its pieces' hashes and its hash.
map_share_file_hash asks for the file at a path (a map, or a resource map
of a kind, _map_share_file_*) to be hashed on the hashing thread, if it is
not (or has changed since); its index in the catalog, or NONE. */
struct map_share_file
{
	boolean used;
	short kind;
	char path[MAP_SHARE_PATH_SIZE];
	/* hashed: ready (pieces and hash known) or failed */
	boolean ready;
	boolean failed;
	unsigned long size;
	unsigned long modified;
	long piece_count;
	byte *piece_hashes;
	byte hash[MAP_SHARE_HASH_BYTES];
};

short map_share_file_hash(
	char const *path,
	short kind);
/* ... and a file whose pieces' hashes are known (a download's, which were
checked): taken as it is, not hashed again */
short map_share_file_hashed(
	char const *path,
	short kind,
	unsigned long size,
	byte const *piece_hashes,
	long piece_count,
	byte const *hash);
struct map_share_file *map_share_file_get(
	short index);
/* the ready file with this hash, or NONE */
short map_share_file_find(
	byte const *hash);
/* the pieces of a file of `size` bytes */
long map_share_piece_count(
	unsigned long size);
/* the hash of a file of `size` bytes from its pieces' hashes */
void map_share_file_hash_of_pieces(
	unsigned long size,
	byte const *piece_hashes,
	long piece_count,
	byte *hash);
/* the hash's first MAP_SHARE_NAME_DIGITS digits, as the name of another
version ends in */
void map_share_hash_digits(
	byte const *hash,
	char *digits);

/* Asks for the pieces of the file at `path` (a download's, part written)
to be hashed, for the caller (`owner`) to compare with what they should be:
FALSE if the hashing thread is busy with another such request. Done, with
the pieces' hashes, when map_share_pieces_hashed returns TRUE (failed: no
hashes, the file unreadable). */
boolean map_share_pieces_hash(
	char const *path,
	long owner);
boolean map_share_pieces_hashed(
	long owner,
	byte **piece_hashes,
	long *piece_count,
	unsigned long *size);

/* the hashing thread's work: started and collected each frame */
void map_share_files_update(
	void);

#endif
