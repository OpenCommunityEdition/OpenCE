/*
MAP_SHARE.H

Custom Edition maps shared over network games (map_share.c, map_share_files.c;
port/linux/NETCODE.md, "Map sharing").

Every map in the custom_maps folder is named for its contents: its file name
ends in _ and the first 8 hexadecimal digits of its hash (hugeass.map becomes
hugeass_1a2b3c4d.map, renamed as the map lists find it), so the level name a
host's game is on (custom_maps\hugeass_1a2b3c4d) says which version of the map
it is. A client joining without that file asks the host for it. If the host
shares maps (network.map_share), it says which files make up the map (the
map, and the resource maps it needs that the client lacks) and which other
players have them, and the client downloads them in pieces from all of them
at once, as a torrent's peers do, each piece checked against its hash. The
downloaded map keeps its name, so two versions of a map never meet.
*/

#ifndef __MAP_SHARE_H
#define __MAP_SHARE_H

/* ---------- constants */

enum
{
	/* a file's hash, of its pieces' (BLAKE2b, 32 bytes) */
	MAP_SHARE_HASH_BYTES = 32,
	/* the hexadecimal digits of the hash a map file's name ends in, after _ */
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
".."). */
boolean map_share_name_safe(
	char const *name);
/* the length of a map's file name without the hash it ends in (all of it,
when it ends in none) */
short map_share_name_base_length(
	char const *name);

/* The map lists found the map file <folder><name>.map, a Custom Edition
map, whose name is not its hash's: in the custom_maps folder it is hashed,
on a thread of its own, and the digits it is named by remembered while the
file stays the same. No file is renamed (map_share_name_digits). */
void map_share_name_map(
	char const *folder,
	char const *name);
/* The digits of the hash the map file <folder><name>.map is named by (a
level of it is custom_maps\<name>_<digits>), into digits
(MAP_SHARE_NAME_DIGITS + 1 characters): remembered, or hashed now (a
fraction of a second); FALSE if it cannot be read. */
boolean map_share_name_digits(
	char const *folder,
	char const *name,
	char *digits);

/* A host's game is on the level `level_name` (a buffer of `size`): a
Custom Edition map's named without its hash (custom_maps\hugeass, a saved
last map's or a test's) is named with it (custom_maps\hugeass_1a2b3c4d), its
file hashed where it is and never renamed, so that the players can download
it (network_server_manager.c). */
void map_share_level_name_hash(
	char *level_name,
	long size);

/* ---------- prototypes/MAP_SHARE.C */

/* A client joining a network game on the level `level_name` (with the
host's `version`, its header checksum) that this machine cannot play
(custom_edition_cache_present said why in `message`): TRUE if it asks the
host for it instead (a Custom Edition map named for its hash, or one whose
resource maps are missing, with network.map_download on). The download
runs in the lobby (the host waits for it to precache), or, for a game in
progress, before this machine's players join. */
boolean map_share_download_begin(
	char const *level_name,
	unsigned long version,
	char const *message);
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
said lately: it is not dropped for adding no players or loading slowly
(network_server_manager.c) */
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
/* the hash's first MAP_SHARE_NAME_DIGITS digits, as a name ends in */
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
