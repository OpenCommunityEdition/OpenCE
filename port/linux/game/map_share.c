/*
MAP_SHARE.C

Custom Edition maps shared over network games (map_share.h,
port/linux/NETCODE.md "Map sharing").

A client joining a game on a map it does not have asks the host, over its
connection (_distributed_message_map_share_request: the level, and which
of its files it lacks: the map, bitmaps.map, sounds.map, loc.map). A host
that shares maps (network.map_share) hashes them (map_share_files.c) and
answers with each file's size and hash (_offer), and which of the game's
other machines may have them (_sources, again every few seconds: they are
the tracker's). The client then fetches the files from all of them at once,
as a torrent's peers do, over connections of their own on MAP_SHARE_PORT:

- each end says which files it has, whole or by piece, and tells the other
  of each piece it gets;
- the client asks for the rarest pieces first, a few at a time from each,
  and checks each against its hash (the pieces' hashes come from any of
  them: they are checked against the file's hash, the host's word); a
  machine that sends a wrong piece is not asked again;
- a machine sends what it has of the game's files (its map, the resource
  maps the map needs, a download's pieces) no faster than its limit
  (network.map_share_upload for the host, network.map_seed_upload for the
  others, network.map_seed turning that off), so the game's own traffic
  keeps the rest of the upload.

Over the internet the players reach their host alone: the host also says how
another player is reached (its key's hash), and a player asks that one for a
session as it asked the host (p2p.c: a player answers holders of the invite
it joined with while it may share). Each downloaded file is written beside
the others as <name>.part and renamed when whole (a .part found again is
checked piece by piece and taken up where it was), into custom_maps under
the map's own name, which is its hash's.

A machine still downloading does not hold the lobby: the machines that
have the map start, and it joins the game in progress once the download is
done. The host does not drop a machine that says it is downloading.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "bungie_net/network/transport.h"
#include "bungie_net/network/transport_endpoint_winsock.h"
#include "bungie_net/network/transport_address_constants.h"
#include "game/game.h"
#include "interface/interface.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "networking/network_connection.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "rasterizer/rasterizer.h"
#include "network_distributed.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "custom_edition_maps.h"
#include "map_share.h"
#include "../src/p2p.h"
#include "../src/map_downloads.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* (the C library's allocator, not the game's tracked one (cseries.h): the
hashing thread allocates what the game's thread frees) */
#undef malloc
#undef free
#undef realloc

/* port_config.c's, network_game_globals.c's, network_server_message_handler.c's,
network_server_manager.c's, cseries_windows.c's */
int config_boolean(const char *name);
long config_integer(const char *name);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
unsigned long network_game_server_machine_address(long machine_index);
word network_game_server_get_state(struct network_game_server *server, short *substate);
unsigned long system_milliseconds(void);

/* ---------- constants */

enum
{
	/* where each machine takes the others' connections */
	MAP_SHARE_PORT = P2P_MAP_SHARE_PORT,
	MAP_SHARE_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* the files of a map: the map and its three resource maps */
	MAXIMUM_FILES = NUMBER_OF_MAP_SHARE_FILE_KINDS,
	/* the machines the host names to a client, and those a client reaches
	at once (the host, and others) */
	MAXIMUM_SOURCES = 32,
	MAXIMUM_OUTGOING = 8,
	/* connections at once, those other machines made included, and of
	those, from one address */
	MAXIMUM_CONNECTIONS = 24,
	MAXIMUM_CONNECTIONS_PER_ADDRESS = 4,
	/* a connection's buffers: a whole frame in (the largest a block), and
	a few blocks out */
	INPUT_BYTES = 0x6000,
	OUTPUT_BYTES = 0xC000,
	MAXIMUM_FRAME_BYTES = INPUT_BYTES - 4,
	/* pieces asked of one connection at once, and of all of them; pieces
	another machine may ask of this one at once */
	REQUESTS_PER_CONNECTION = 2,
	MAXIMUM_PIECES_IN_FLIGHT = 16,
	SERVING_PER_CONNECTION = 8,
	/* the pieces' hashes sent at once */
	HASHES_PER_FRAME = 256,
	/* a level name as a message carries it */
	LEVEL_NAME_BYTES = 64,

	/* milliseconds */
	OFFER_TIMEOUT = 30000,
	STALL_TIMEOUT = 60000,
	PROGRESS_INTERVAL = 2000,
	SOURCES_INTERVAL = 10000,
	/* the host takes a machine for downloading this long after it said so */
	DOWNLOADING_TIMEOUT = 20000,
	CONNECTION_IDLE_TIMEOUT = 30000,
	CONNECT_TIMEOUT = 10000,
	KEEP_ALIVE_INTERVAL = 10000,
	REQUEST_TIMEOUT = 30000,
	SOURCE_RETRY_INTERVAL = 15000,
	LISTEN_RETRY_INTERVAL = 10000,
	RATE_WINDOW = 2000,
	LOG_INTERVAL = 10000,

	/* the pieces asked of a web seed at once (4 MiB), and the failed asks
	after which none is asked again this download */
	WEB_SEED_PIECES = 16,
	WEB_SEED_FAILURES = 3,
	/* milliseconds before asking again after a failed ask, or one that
	could not start (another fetch under way) */
	WEB_SEED_RETRY_INTERVAL = 5000,
	WEB_SEED_START_RETRY_INTERVAL = 1000,
	/* a web seed's URL (maps.web_seeds' folder and the file's name) */
	WEB_SEED_URL_BYTES = 1024,

	/* the largest file of each kind the cache loader takes
	(custom_edition_cache.c's combined offset space), so that a host cannot
	have its players fill their disks */
	MAXIMUM_MAP_BYTES = 0x30000000,
	MAXIMUM_RESOURCE_MAP_BYTES = 0x20000000,

	/* the kilobytes a second a machine sends, at least and at most */
	MINIMUM_UPLOAD_KILOBYTES = 16,
	MAXIMUM_UPLOAD_KILOBYTES = 65536,

	/* network_server_manager.c's host state in game, transport_endpoint_winsock.c's
	stream endpoint and hud_messaging.c's centred text (their enums are their
	own) */
	_map_share_server_state_ingame = 1,
	_map_share_endpoint_tcp = 0x12,
	_map_share_text_centered = 2,
};

/* a connection's frames: its length (4 bytes, little-endian, of what
follows), its type, and its body */
enum
{
	/* the files the connection is about: its magic, a version, the count,
	and each file's hash (the end that connected says; the other repeats
	them) */
	_frame_hello = 1,
	/* what this end has of a file: none, all, or a byte of pieces each
	eight (the first the lowest bit) */
	_frame_state,
	/* a piece this end got (the file, the piece) */
	_frame_have,
	/* a piece asked for (the file, the piece) */
	_frame_request,
	/* part of a piece (the file, the piece, its offset, the bytes) */
	_frame_block,
	/* a piece this end does not send (the file, the piece) */
	_frame_refuse,
	/* the pieces' hashes asked for (the file), and some of them (the file,
	the first, the count, the hashes) */
	_frame_hashes_request,
	_frame_hashes,
	_frame_keep_alive,
};

#define FRAME_MAGIC 0x3153484DUL /* 'MHS1' */
#define FRAME_VERSION 1

enum
{
	_have_none,
	_have_all,
	_have_pieces,
};

/* the host's answer to a request */
enum
{
	_offer_ok,
	/* it does not share maps (network.map_share) */
	_offer_off,
	/* not the game's map, not named for its hash, or not readable */
	_offer_unavailable,
};

enum
{
	_offer_in_progress_bit,
};

enum
{
	/* the machine is reached through internet play (its key hash), else
	at its address */
	_source_internet_bit,
	/* it is downloading the files too */
	_source_downloading_bit,
};

enum
{
	_progress_downloading,
	_progress_done,
	_progress_failed,
};

enum
{
	_download_none,
	/* asked the host; waiting for its offer */
	_download_asking,
	_download_transferring,
	_download_done,
	_download_failed,
};

enum
{
	/* its pieces' hashes not known yet */
	_file_awaiting_hashes,
	/* its part file being checked */
	_file_checking,
	_file_transferring,
	_file_complete,
};

enum
{
	_connection_connecting,
	_connection_open,
};

/* ---------- structures */

struct distributed_map_share_request
{
	char level_name[LEVEL_NAME_BYTES];
	unsigned long version;
	/* the files the client lacks: a bit each (_map_share_file_*) */
	byte wanted;
	byte pad[3];
};

struct distributed_map_share_offer
{
	char level_name[LEVEL_NAME_BYTES];
	byte result;
	byte file_count;
	byte flags;
	byte pad;
};

struct distributed_map_share_offer_file
{
	byte kind;
	byte pad[3];
	unsigned long size;
	byte hash[MAP_SHARE_HASH_BYTES];
};

struct distributed_map_share_sources
{
	byte count;
	byte flags;
	byte pad[2];
};

struct distributed_map_share_source
{
	byte machine_index;
	byte flags;
	byte pad[2];
	/* IPv4, host byte order */
	unsigned long address;
	byte identifier[6];
	byte pad2[2];
	byte key_hash[P2P_PLAYER_KEY_HASH_SIZE];
};

struct distributed_map_share_progress
{
	byte state;
	byte pad[3];
	unsigned long done;
	unsigned long total;
};

typedef char distributed_map_share_request_size_assert[sizeof(struct distributed_map_share_request) == 72 ? 1 : -1];
typedef char distributed_map_share_offer_size_assert[sizeof(struct distributed_map_share_offer) == 68 ? 1 : -1];
typedef char distributed_map_share_offer_file_size_assert[sizeof(struct distributed_map_share_offer_file) == 40 ? 1 : -1];
typedef char distributed_map_share_source_size_assert[sizeof(struct distributed_map_share_source) == 32 ? 1 : -1];
typedef char distributed_map_share_progress_size_assert[sizeof(struct distributed_map_share_progress) == 12 ? 1 : -1];

/* a file of the download */
struct download_file
{
	short kind;
	short state;
	unsigned long size;
	byte hash[MAP_SHARE_HASH_BYTES];
	long piece_count;
	/* known once a machine sent them, checked against the hash */
	byte *piece_hashes;
	/* the pieces' hashes as they come (from one connection at a time) */
	byte *hashes_coming;
	long hashes_received;
	short hashes_connection;
	unsigned long hashes_asked_at;
	/* the pieces had (a bit each), and asked for (a count each) */
	byte *have;
	byte *asked;
	long have_count;
	char part_path[MAP_SHARE_PATH_SIZE];
	char final_path[MAP_SHARE_PATH_SIZE];
	FILE *stream;
	/* the file on a web seed (the index's file of its name, version and
	size: port/linux/src/map_downloads.c), "" for none; and whether it sent
	a piece that is not this file's */
	char web_url[WEB_SEED_URL_BYTES];
	boolean web_off;
};

/* a machine the host named */
struct source
{
	byte machine_index;
	byte flags;
	unsigned long address;
	byte identifier[6];
	byte key_hash[P2P_PLAYER_KEY_HASH_SIZE];
	/* the host itself (the client's own connection's address) */
	boolean host;
	/* sent a wrong piece: not asked again */
	boolean banned;
	short connection;
	unsigned long tried_at;
};

struct download
{
	short state;
	/* this download's number, which its part files' checks are asked for
	with (map_share_pieces_hash: a check of the last's is not this one's) */
	long serial;
	char level_name[LEVEL_NAME_BYTES];
	unsigned long version;
	/* why the map cannot be played (custom_edition_cache_present's), for a
	download that fails */
	char message[256];
	byte wanted;
	boolean in_progress;
	short file_count;
	struct download_file files[MAXIMUM_FILES];
	short source_count;
	struct source sources[MAXIMUM_SOURCES + 1];
	unsigned long asked_at;
	unsigned long progress_at;
	unsigned long reported_at;
	/* the bytes of the pieces had, of all of them; and for the rate shown,
	those got in this window and the last's */
	unsigned long done_bytes;
	unsigned long total_bytes;
	unsigned long window_at;
	unsigned long window_bytes;
	unsigned long last_window_bytes;
	unsigned long last_window_time;
	unsigned long logged_at;
	/* the pieces asked of the web seed: whether any are, the file's, the
	first and how many (a run of them); the asks failed, and the bytes it
	brought */
	boolean web_active;
	short web_file;
	long web_first;
	long web_count;
	short web_failures;
	unsigned long web_bytes;
	unsigned long web_retry_at;
};

/* a piece asked of a connection */
struct request
{
	short file;
	long piece;
	unsigned long received;
	unsigned long asked_at;
	byte *buffer;
};

/* a piece another machine asked of this one, sent a block at a time */
struct serving
{
	short file;
	long piece;
	unsigned long offset;
};

struct connection
{
	boolean used;
	short state;
	boolean outgoing;
	/* the download's source this reached (outgoing), or NONE */
	short source;
	/* IPv4, host byte order */
	unsigned long address;
	struct transport_endpoint *endpoint;
	unsigned long created_at;
	unsigned long heard_at;
	unsigned long sent_at;
	/* the bytes of pieces it brought and took, for the log */
	unsigned long bytes_in;
	unsigned long bytes_out;
	boolean hello_heard;
	/* the files it is about, and for each: the download's file, the
	catalog's file this machine sends (map_share_files.c), what the other
	end has, whether this end said what it has, and the next of the pieces'
	hashes to send it (NONE: none asked) */
	short file_count;
	byte file_hashes[MAXIMUM_FILES][MAP_SHARE_HASH_BYTES];
	short download_file[MAXIMUM_FILES];
	short catalog_file[MAXIMUM_FILES];
	short remote_has[MAXIMUM_FILES];
	byte *remote_pieces[MAXIMUM_FILES];
	boolean state_heard[MAXIMUM_FILES];
	boolean state_told[MAXIMUM_FILES];
	long hashes_next[MAXIMUM_FILES];
	short request_count;
	struct request requests[REQUESTS_PER_CONNECTION];
	short serving_count;
	struct serving serving[SERVING_PER_CONNECTION];
	/* (allocated with the connection: INPUT_BYTES, OUTPUT_BYTES) */
	byte *input;
	long input_size;
	byte *output;
	long output_size;
};

/* what the host knows of each client machine's download */
struct host_machine
{
	boolean asked;
	struct distributed_map_share_request request;
	unsigned long asked_at;
	boolean offered;
	short catalog_files[MAXIMUM_FILES];
	short file_kinds[MAXIMUM_FILES];
	short file_count;
	unsigned long sources_at;
	/* downloading (and for DOWNLOADING_TIMEOUT after it said it was done:
	its players join then), and done */
	boolean downloading;
	boolean downloaded;
	unsigned long heard_at;
	unsigned long done;
	unsigned long total;
	unsigned long address;
};

struct map_share_globals
{
	struct download download;
	struct connection connections[MAXIMUM_CONNECTIONS];
	short next_connection;
	struct host_machine machines[MAP_SHARE_MACHINES];
	struct transport_endpoint *listener;
	unsigned long listen_tried_at;
	boolean listen_failed_logged;
	/* the upload's allowance (bytes) and when it was topped up */
	long upload_tokens;
	unsigned long upload_at;
	/* the catalog files of the game's map and its resource maps, which
	this machine sends (NONE: none), and the level they are of */
	char current_level[LEVEL_NAME_BYTES];
	short current_files[MAXIMUM_FILES];
};

/* ---------- globals */

static struct map_share_globals map_share_globals;

/* ---------- little-endian bytes */

static void put_long(
	byte *bytes,
	unsigned long value)
{
	bytes[0] = (byte)value;
	bytes[1] = (byte)(value >> 8);
	bytes[2] = (byte)(value >> 16);
	bytes[3] = (byte)(value >> 24);
}

static unsigned long get_long(
	byte const *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static unsigned long network_order(
	unsigned long value)
{
	byte bytes[4];
	unsigned long result;

	bytes[0] = (byte)(value >> 24);
	bytes[1] = (byte)(value >> 16);
	bytes[2] = (byte)(value >> 8);
	bytes[3] = (byte)value;
	csmemcpy(&result, bytes, sizeof(result));
	return result;
}

static unsigned long host_order(
	unsigned long value)
{
	byte const *bytes = (byte const *)&value;

	return (unsigned long)bytes[0] << 24 | (unsigned long)bytes[1] << 16 | (unsigned long)bytes[2] << 8 | bytes[3];
}

/* whether `time` milliseconds have passed from `since` to `now` (a time
taken after now has not) */
static boolean elapsed(
	unsigned long since,
	unsigned long now,
	unsigned long time)
{
	return (long)(now - since) >= (long)time;
}

/* whether an address (host byte order) is internet play's (100.64.0.0/10) */
static boolean address_internet(
	unsigned long address)
{
	return (address & 0xFFC00000UL) == 0x64400000UL;
}

/* ---------- this machine's network game */

static struct network_game *map_share_network_game(
	void)
{
	struct network_game_client *client = global_network_game_client_get();

	return client ? network_game_client_get_game(client) : NULL;
}

static boolean map_share_host(
	void)
{
	return global_network_game_server_get() != NULL;
}

/* the host's address as this client's connection has it (host byte
order; 0 if none) */
static unsigned long map_share_host_address(
	void)
{
	struct network_game_client *client = global_network_game_client_get();
	struct network_connection *connection = client ? network_game_client_get_connection(client) : NULL;
	struct transport_address reliable;
	struct transport_address unreliable;

	if (!connection || map_share_host())
		return 0;
	csmemset(&reliable, 0, sizeof(reliable));
	network_connection_get_address(connection, &reliable, &unreliable);
	return reliable.address_length == IPV4_ADDRESS_LENGTH ? reliable.address.long_words[0] : 0;
}

static long upload_limit(
	void)
{
	long kilobytes = map_share_host() ? config_integer("network.map_share_upload") :
		config_integer("network.map_seed_upload");

	return PIN(kilobytes, MINIMUM_UPLOAD_KILOBYTES, MAXIMUM_UPLOAD_KILOBYTES) * 1024;
}

/* whether this machine sends the game's files to the others: in a game on
a Custom Edition map, as its settings say */
static boolean map_share_sending(
	void)
{
	struct network_game *game = map_share_network_game();

	if (!game || !custom_edition_level_name(game->map.name) || network_game_client_get_local_machine_index() == NONE)
		return FALSE;
	return map_share_host() ? config_boolean("network.map_share") != 0 : config_boolean("network.map_seed") != 0;
}

/* a message's headers (voice_fill_header's) */
static void map_share_fill_header(
	struct distributed_message_header *header,
	byte type,
	word size)
{
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	header->type = type;
	header->count = 1;
	header->game_time = game_in_progress() ? game_time_get() : 0;
}

/* a level name copied into a message's field, ended within it */
static void level_name_copy(
	char *destination,
	char const *source)
{
	csmemset(destination, 0, LEVEL_NAME_BYTES);
	csstrncpy(destination, source, LEVEL_NAME_BYTES - 1);
}

static boolean level_name_valid(
	char const *name)
{
	return memchr(name, 0, LEVEL_NAME_BYTES) != NULL && name[0];
}

/* ---------- the game's files: those this machine sends */

/* the catalog files of the level's map and of the resource maps there are
(asked to be hashed only when another machine asks for them: hashing a
resource map reads hundreds of megabytes) */
static void current_files_update(
	boolean hash)
{
	struct map_share_globals *globals = &map_share_globals;
	struct network_game *game = map_share_network_game();
	char const *level_name = game ? game->map.name : "";
	short kind;

	if (csstrncmp(globals->current_level, level_name, LEVEL_NAME_BYTES - 1))
	{
		level_name_copy(globals->current_level, level_name);
		for (kind = 0; kind < MAXIMUM_FILES; kind++)
			globals->current_files[kind] = NONE;
	}
	if (!hash || !custom_edition_level_name(level_name))
		return;
	for (kind = 0; kind < MAXIMUM_FILES; kind++)
	{
		char path[MAP_SHARE_PATH_SIZE];
		boolean present = kind == _map_share_file_map ? custom_edition_map_file_path(level_name, path) :
			custom_edition_resource_map_file_path(kind, path);

		globals->current_files[kind] = present ? map_share_file_hash(path, kind) : NONE;
	}
}

/* whether the current files are all hashed (or failed): what this machine
has of a hash asked for is known */
static boolean current_files_hashed(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	short kind;

	for (kind = 0; kind < MAXIMUM_FILES; kind++)
	{
		struct map_share_file *file = map_share_file_get(globals->current_files[kind]);

		if (file && !file->ready && !file->failed)
			return FALSE;
	}
	return TRUE;
}

/* the catalog file with this hash that this machine sends, or NONE */
static short current_file_with_hash(
	byte const *hash)
{
	struct map_share_globals *globals = &map_share_globals;
	short kind;

	for (kind = 0; kind < MAXIMUM_FILES; kind++)
	{
		struct map_share_file *file = map_share_file_get(globals->current_files[kind]);

		if (file && file->ready && !csmemcmp(file->hash, hash, MAP_SHARE_HASH_BYTES))
			return globals->current_files[kind];
	}
	return NONE;
}

/* the download's file with this hash, or NONE */
static short download_file_with_hash(
	byte const *hash)
{
	struct download *download = &map_share_globals.download;
	short index;

	if (download->state != _download_transferring)
		return NONE;
	for (index = 0; index < download->file_count; index++)
	{
		if (!csmemcmp(download->files[index].hash, hash, MAP_SHARE_HASH_BYTES))
			return index;
	}
	return NONE;
}

/* ---------- connections: frames */

static void connection_close(
	struct connection *connection,
	char const *reason);

/* room for a frame of `size` bytes of body in the connection's output */
static boolean frame_room(
	struct connection *connection,
	long size)
{
	return connection->output_size + 5 + size <= OUTPUT_BYTES;
}

/* a frame's head written to the output, its body to follow (frame_room
checked) */
static byte *frame_begin(
	struct connection *connection,
	byte type,
	long size)
{
	byte *frame = connection->output + connection->output_size;

	put_long(frame, (unsigned long)(size + 1));
	frame[4] = type;
	connection->output_size += 5 + size;
	return frame + 5;
}

static void frame_piece(
	struct connection *connection,
	byte type,
	short file,
	long piece)
{
	byte *body;

	if (!frame_room(connection, 5))
		return;
	body = frame_begin(connection, type, 5);
	body[0] = (byte)file;
	put_long(body + 1, (unsigned long)piece);
}

static void send_hello(
	struct connection *connection)
{
	byte *body;
	short index;

	if (!frame_room(connection, 6 + connection->file_count * MAP_SHARE_HASH_BYTES))
		return;
	body = frame_begin(connection, _frame_hello, 6 + connection->file_count * MAP_SHARE_HASH_BYTES);
	put_long(body, FRAME_MAGIC);
	body[4] = FRAME_VERSION;
	body[5] = (byte)connection->file_count;
	for (index = 0; index < connection->file_count; index++)
		csmemcpy(body + 6 + index * MAP_SHARE_HASH_BYTES, connection->file_hashes[index], MAP_SHARE_HASH_BYTES);
}

/* what this end has of a connection's file, once it knows */
static void send_state(
	struct connection *connection,
	short index)
{
	struct download *download = &map_share_globals.download;
	short download_file = connection->download_file[index];
	byte *body;

	if (connection->catalog_file[index] != NONE)
	{
		if (!frame_room(connection, 2))
			return;
		body = frame_begin(connection, _frame_state, 2);
		body[0] = (byte)index;
		body[1] = _have_all;
	}
	else if (download_file != NONE && download->files[download_file].state == _file_transferring)
	{
		struct download_file *file = &download->files[download_file];
		long bytes = (file->piece_count + 7) / 8;

		if (!frame_room(connection, 2 + bytes))
			return;
		body = frame_begin(connection, _frame_state, 2 + bytes);
		body[0] = (byte)index;
		body[1] = _have_pieces;
		csmemcpy(body + 2, file->have, bytes);
	}
	else if (download_file != NONE && download->files[download_file].state == _file_complete)
	{
		if (!frame_room(connection, 2))
			return;
		body = frame_begin(connection, _frame_state, 2);
		body[0] = (byte)index;
		body[1] = _have_all;
	}
	else
	{
		/* (not known yet: said once it is) */
		if (download_file != NONE || !current_files_hashed())
			return;
		if (!frame_room(connection, 2))
			return;
		body = frame_begin(connection, _frame_state, 2);
		body[0] = (byte)index;
		body[1] = _have_none;
	}
	connection->state_told[index] = TRUE;
}

/* the connection's files matched to this machine's: the download's, and
those it sends */
static void connection_match_files(
	struct connection *connection)
{
	short index;

	for (index = 0; index < connection->file_count; index++)
	{
		short catalog_file = current_file_with_hash(connection->file_hashes[index]);

		connection->download_file[index] = download_file_with_hash(connection->file_hashes[index]);
		if (catalog_file != connection->catalog_file[index] && map_share_sending())
		{
			connection->catalog_file[index] = catalog_file;
			/* (had now: said again) */
			if (catalog_file != NONE)
				connection->state_told[index] = FALSE;
		}
	}
}

/* ---------- connections */

static struct connection *connection_new(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	short index;
	short file;

	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
	{
		struct connection *connection = &globals->connections[index];

		if (!connection->used)
		{
			csmemset(connection, 0, sizeof(*connection));
			connection->input = malloc(INPUT_BYTES);
			connection->output = malloc(OUTPUT_BYTES);
			if (!connection->input || !connection->output)
			{
				free(connection->input);
				free(connection->output);
				connection->input = NULL;
				connection->output = NULL;
				return NULL;
			}
			connection->used = TRUE;
			connection->source = NONE;
			connection->created_at = system_milliseconds();
			connection->heard_at = connection->created_at;
			connection->sent_at = connection->created_at;
			for (file = 0; file < MAXIMUM_FILES; file++)
			{
				connection->download_file[file] = NONE;
				connection->catalog_file[file] = NONE;
				connection->hashes_next[file] = NONE;
			}
			return connection;
		}
	}
	return NULL;
}

static void connection_free(
	struct connection *connection)
{
	struct download *download = &map_share_globals.download;
	short index;

	for (index = 0; index < connection->request_count; index++)
	{
		struct request *request = &connection->requests[index];

		if (request->file < download->file_count && download->files[request->file].asked &&
			download->files[request->file].asked[request->piece])
		{
			download->files[request->file].asked[request->piece]--;
		}
		free(request->buffer);
	}
	for (index = 0; index < MAXIMUM_FILES; index++)
	{
		free(connection->remote_pieces[index]);
		if (index < download->file_count && download->files[index].hashes_connection ==
			(short)(connection - map_share_globals.connections))
		{
			download->files[index].hashes_connection = NONE;
		}
	}
	if (connection->endpoint)
		delete_transport_endpoint(connection->endpoint);
	free(connection->input);
	free(connection->output);
	csmemset(connection, 0, sizeof(*connection));
}

static void connection_close(
	struct connection *connection,
	char const *reason)
{
	struct download *download = &map_share_globals.download;

	if (!connection->used)
		return;
	if (reason || connection->bytes_in || connection->bytes_out)
	{
		error(_error_silent, "map sharing: the connection with %d.%d.%d.%d closed: %s (%lu bytes in, %lu out)",
			(int)(connection->address >> 24), (int)(connection->address >> 16 & 255),
			(int)(connection->address >> 8 & 255), (int)(connection->address & 255), reason ? reason : "done",
			connection->bytes_in, connection->bytes_out);
	}
	if (connection->source != NONE && connection->source < download->source_count)
		download->sources[connection->source].connection = NONE;
	connection->source = NONE;
	connection_free(connection);
}

static void connection_set_files(
	struct connection *connection)
{
	struct download *download = &map_share_globals.download;
	short index;

	connection->file_count = download->file_count;
	for (index = 0; index < download->file_count; index++)
		csmemcpy(connection->file_hashes[index], download->files[index].hash, MAP_SHARE_HASH_BYTES);
}

/* connects to a source (its address known: host byte order), without
waiting: the connect is looked at each frame (connection_connected; the
transport's own waits up to ten seconds, or has a thread of its own) */
static void connection_connect(
	struct connection *connection,
	unsigned long address)
{
	struct sockaddr_in remote;
	unsigned long nonblocking = 1;
	SOCKET socket_handle;

	connection->address = address;
	connection->endpoint = create_transport_endpoint(_map_share_endpoint_tcp);
	if (!connection->endpoint)
	{
		connection_close(connection, "no endpoint");
		return;
	}
	socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_handle == INVALID_SOCKET)
	{
		connection_close(connection, "no socket");
		return;
	}
	connection->endpoint->socket = (long)socket_handle;
	csmemset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_port = (word)SWAP2(MAP_SHARE_PORT);
	remote.sin_addr.s_addr = network_order(address);
	if (ioctlsocket(socket_handle, FIONBIO, &nonblocking) != 0 ||
		(connect(socket_handle, (struct sockaddr *)&remote, sizeof(remote)) != 0 &&
		WSAGetLastError() != WSAEWOULDBLOCK))
	{
		connection_close(connection, "could not connect");
		return;
	}
	SET_FLAG(connection->endpoint->flags, _transport_endpoint_nonblocking_bit, TRUE);
	connection->state = _connection_connecting;
}

/* whether the connect is over: TRUE when it is, *connected saying how */
static boolean connection_connected(
	struct connection *connection,
	boolean *connected)
{
	SOCKET socket_handle = (SOCKET)connection->endpoint->socket;
	fd_set writeable;
	fd_set failed;
	struct timeval timeout;
	long ready;

	writeable.fd_array[0] = socket_handle;
	writeable.fd_count = 1;
	failed.fd_array[0] = socket_handle;
	failed.fd_count = 1;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	/* (a failed connect is in the error set: transport_endpoint_winsock.c) */
	ready = select(1, NULL, &writeable, &failed, &timeout);
	if (ready < 0)
	{
		*connected = FALSE;
		return TRUE;
	}
	if (!ready)
		return FALSE;
	*connected = !(failed.fd_count && __WSAFDIsSet(socket_handle, &failed));
	if (*connected)
		SET_FLAG(connection->endpoint->flags, _transport_endpoint_connected_bit, TRUE);
	return TRUE;
}

static void connection_accept(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	struct transport_endpoint *endpoint;
	struct connection *connection;
	struct transport_address address;
	short count;
	short index;

	while (globals->listener && endpoint_readable(globals->listener, 0))
	{
		count = 0;
		endpoint = accept_endpoint(globals->listener);
		if (!endpoint)
			return;
		csmemset(&address, 0, sizeof(address));
		if (set_endpoint_blocking(endpoint, FALSE) != _transport_error_none ||
			get_endpoint_address(endpoint, &address) != _transport_error_none ||
			(!map_share_sending() && map_share_globals.download.state != _download_transferring))
		{
			delete_transport_endpoint(endpoint);
			continue;
		}
		/* (a few from one address: anyone can connect) */
		for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		{
			count += globals->connections[index].used &&
				globals->connections[index].address == address.address.long_words[0];
		}
		connection = count < MAXIMUM_CONNECTIONS_PER_ADDRESS ? connection_new() : NULL;
		if (!connection)
		{
			delete_transport_endpoint(endpoint);
			continue;
		}
		connection->endpoint = endpoint;
		connection->address = address.address.long_words[0];
		connection->state = _connection_open;
		connection->outgoing = FALSE;
	}
}

/* the listener for the others' connections: while this machine is in a
network game and sends maps or downloads one */
static void listener_update(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	boolean want = map_share_network_game() &&
		(map_share_sending() || globals->download.state == _download_transferring);
	unsigned long now = system_milliseconds();

	if (!want)
	{
		if (globals->listener)
		{
			delete_transport_endpoint(globals->listener);
			globals->listener = NULL;
		}
		return;
	}
	if (globals->listener || (globals->listen_tried_at && !elapsed(globals->listen_tried_at, now, LISTEN_RETRY_INTERVAL)))
		return;
	globals->listen_tried_at = now | 1;
	globals->listener = create_transport_endpoint(_map_share_endpoint_tcp);
	if (globals->listener)
	{
		struct transport_address address;

		csmemset(&address, 0, sizeof(address));
		address.address_length = IPV4_ADDRESS_LENGTH;
		address.port = MAP_SHARE_PORT;
		if (bind_endpoint(globals->listener, &address) != _transport_error_none ||
			listen_endpoint(globals->listener) != _transport_error_none ||
			set_endpoint_blocking(globals->listener, FALSE) != _transport_error_none)
		{
			if (!globals->listen_failed_logged)
			{
				error(_error_silent, "map sharing: cannot take connections on port %d (in use?)", MAP_SHARE_PORT);
				globals->listen_failed_logged = TRUE;
			}
			delete_transport_endpoint(globals->listener);
			globals->listener = NULL;
		}
	}
}

/* ---------- the download */

static void download_report(
	byte state);
static void download_web_seeds_find(
	char const *name);
static void request_remove(
	struct connection *connection,
	short index);

/* the download over: its files closed (a part file is kept, to be taken
up again), and the connections it made closed, unless they are kept for
what this machine sends now that it has the files (keep_connections) */
static void download_end(
	boolean keep_connections)
{
	struct map_share_globals *globals = &map_share_globals;
	struct download *download = &globals->download;
	short index;

	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
	{
		struct connection *connection = &globals->connections[index];
		short file;

		if (!connection->used)
			continue;
		connection->source = NONE;
		if (connection->outgoing && !keep_connections)
		{
			connection_close(connection, NULL);
			continue;
		}
		/* (another's connection, which may still fetch what this machine
		sends: what it asked of the download let go) */
		while (connection->request_count)
			request_remove(connection, (short)(connection->request_count - 1));
		for (file = 0; file < MAXIMUM_FILES; file++)
			connection->download_file[file] = NONE;
	}
	if (download->web_active)
		map_downloads_range_stop();
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];

		if (file->stream)
			fclose(file->stream);
		free(file->piece_hashes);
		free(file->hashes_coming);
		free(file->have);
		free(file->asked);
	}
	csmemset(download, 0, sizeof(*download));
}

/* the download failed: the player told why when the main menu comes, and
the game left */
static void download_fail(
	char const *reason)
{
	struct download *download = &map_share_globals.download;
	wchar_t text[512];
	char message[512];
	short index;

	error(_error_silent, "map sharing: the download of %s failed: %s", download->level_name, reason);
	snprintf(message, sizeof(message), "%s\r\n\r\n%s", download->message, reason);
	for (index = 0; message[index] && index < NUMBEROF(text) - 1; index++)
		text[index] = (wchar_t)(unsigned char)message[index];
	text[index] = 0;
	download_report(_progress_failed);
	download_end(FALSE);
	download->state = _download_failed;
	display_error_text_when_main_menu_loaded(text);
	network_game_abort();
}

static void download_report(
	byte state)
{
	struct download *download = &map_share_globals.download;
	struct
	{
		struct distributed_message_header header;
		struct distributed_map_share_progress progress;
	} message;

	csmemset(&message, 0, sizeof(message));
	map_share_fill_header(&message.header, _distributed_message_map_share_progress, (word)sizeof(message));
	message.progress.state = state;
	message.progress.done = download->done_bytes;
	message.progress.total = download->total_bytes;
	network_distributed_client_send_reliably(&message, (word)sizeof(message));
	download->reported_at = system_milliseconds();
}

static void download_ask(
	void)
{
	struct download *download = &map_share_globals.download;
	struct
	{
		struct distributed_message_header header;
		struct distributed_map_share_request request;
	} message;

	csmemset(&message, 0, sizeof(message));
	map_share_fill_header(&message.header, _distributed_message_map_share_request, (word)sizeof(message));
	level_name_copy(message.request.level_name, download->level_name);
	message.request.version = download->version;
	message.request.wanted = download->wanted;
	network_distributed_client_send_reliably(&message, (word)sizeof(message));
}

/* the files this machine lacks for the level: a bit each */
static byte files_wanted(
	char const *level_name)
{
	char path[MAP_SHARE_PATH_SIZE];
	byte wanted = 0;
	short kind;

	if (!custom_edition_map_file_path(level_name, path))
		wanted |= FLAG(_map_share_file_map);
	for (kind = _map_share_file_bitmaps; kind < MAXIMUM_FILES; kind++)
	{
		if (!custom_edition_resource_map_file_path(kind, path))
			wanted |= FLAG(kind);
	}
	return wanted;
}

boolean map_share_download_begin(
	char const *level_name,
	unsigned long version,
	char const *message)
{
	struct download *download = &map_share_globals.download;
	char const *name;
	byte wanted;

	if (!level_name || !custom_edition_level_name(level_name) || csstrlen(level_name) >= LEVEL_NAME_BYTES ||
		!config_boolean("network.map_download") || map_share_host() || !global_network_game_client_get() ||
		!halo_custom_edition_tag_cache())
	{
		return FALSE;
	}
	if (download->state == _download_asking || download->state == _download_transferring)
	{
		if (!csstrcmp(download->level_name, level_name) && download->version == version)
			return TRUE;
		download_end(FALSE);
	}
	name = tag_name_strip_path(level_name);
	wanted = files_wanted(level_name);
	/* (the map named for its hash, so that the download is the host's
	version under its own name, a file of custom_maps alone, whatever the
	host's level name holds; a map this machine has of another version is
	not replaced) */
	if (!wanted || !map_share_name_safe(name) ||
		(TEST_FLAG(wanted, _map_share_file_map) && !map_share_name_hashed(name)) ||
		(!TEST_FLAG(wanted, _map_share_file_map) && custom_edition_map_checksum(level_name) != version && version))
	{
		return FALSE;
	}
	{
		static long serial;

		csmemset(download, 0, sizeof(*download));
		download->serial = ++serial;
	}
	download->state = _download_asking;
	level_name_copy(download->level_name, level_name);
	download->version = version;
	csstrncpy(download->message, message ? message : "", sizeof(download->message) - 1);
	download->wanted = wanted;
	download->asked_at = system_milliseconds();
	download->progress_at = download->asked_at;
	error(_error_silent, "map sharing: asking the host for %s (files 0x%X)", level_name, (unsigned)wanted);
	download_ask();
	return TRUE;
}

boolean map_share_downloading(
	char const *level_name)
{
	struct download *download = &map_share_globals.download;

	return (download->state == _download_asking || download->state == _download_transferring) &&
		level_name && !csstrcmp(download->level_name, level_name);
}

boolean map_share_holds_players(
	void)
{
	struct download *download = &map_share_globals.download;
	struct network_game *game = map_share_network_game();

	/* (a client whose host has not named the game's map yet: whether this
	machine has it is not known, and a game in progress would start it at
	once. Asking: whether the game is in progress is not known yet) */
	if (!map_share_host() && game && !game->map.name[0])
		return TRUE;
	/* (failed: the game is being left) */
	return download->state == _download_asking || download->state == _download_failed ||
		(download->state == _download_transferring && download->in_progress);
}

/* the download's file made anew for writing: its part file the file's size */
static boolean download_file_create(
	struct download_file *file)
{
	FILE *stream = fopen(file->part_path, "wb");

	if (!stream)
		return FALSE;
	if (file->size && (fseek(stream, (long)file->size - 1, SEEK_SET) != 0 || fputc(0, stream) == EOF))
	{
		fclose(stream);
		return FALSE;
	}
	fclose(stream);
	file->stream = fopen(file->part_path, "r+b");
	if (!file->stream)
		return FALSE;
	file->state = _file_transferring;
	return TRUE;
}

/* the download's file opened for writing: a part file of its size there
already checked piece by piece first (on the hashing thread, asked again
while it is busy: download_update), else one made anew */
static boolean download_file_open(
	struct download_file *file,
	short index)
{
	FILE *stream = fopen(file->part_path, "rb");
	long size = -1;

	if (stream)
	{
		if (fseek(stream, 0, SEEK_END) == 0)
			size = ftell(stream);
		fclose(stream);
	}
	if (size == (long)file->size && file->size)
	{
		file->state = _file_checking;
		map_share_pieces_hash(file->part_path, map_share_globals.download.serial << 3 | index);
		return TRUE;
	}
	return download_file_create(file);
}

/* the host's offer: the files to fetch */
static void download_offered(
	struct distributed_map_share_offer const *offer,
	struct distributed_map_share_offer_file const *files)
{
	struct download *download = &map_share_globals.download;
	char const *name = tag_name_strip_path(download->level_name);
	byte offered = 0;
	short index;

	if (download->state != _download_asking || csstrcmp(offer->level_name, download->level_name))
		return;
	if (offer->result != _offer_ok || !offer->file_count || offer->file_count > MAXIMUM_FILES)
	{
		char files[256];

		/* (from the community's archives instead, when the index of the maps
		has it: port/linux/src/map_downloads.c; the player joins again once
		it is there) */
		if (custom_edition_cache_files_missing(download->level_name, download->version, files, sizeof(files)) > 0 &&
			map_downloads_fetch(download->level_name, download->version, files))
		{
			download_fail("The host does not send this map, so it is being downloaded from the map archives "
				"(Join Game shows how it goes). Join again when it is there.");
			return;
		}
		download_fail(offer->result == _offer_off ? "The host does not share its maps." :
			"The host cannot share this map.");
		return;
	}
	download->in_progress = TEST_FLAG(offer->flags, _offer_in_progress_bit);
	download->file_count = offer->file_count;
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];
		char digits[MAP_SHARE_NAME_DIGITS + 1];

		csmemset(file, 0, sizeof(*file));
		file->kind = files[index].kind;
		file->size = files[index].size;
		file->hashes_connection = NONE;
		csmemcpy(file->hash, files[index].hash, MAP_SHARE_HASH_BYTES);
		file->piece_count = map_share_piece_count(file->size);
		map_share_hash_digits(file->hash, digits);
		/* (a file this machine did not ask for, or offered twice, or a map
		whose hash is not its name's) */
		if (file->kind >= MAXIMUM_FILES || !TEST_FLAG(download->wanted, file->kind) ||
			TEST_FLAG(offered, file->kind) || !file->size ||
			file->size > (unsigned long)(file->kind == _map_share_file_map ? MAXIMUM_MAP_BYTES :
			MAXIMUM_RESOURCE_MAP_BYTES) ||
			(file->kind == _map_share_file_map && csstrcmp(name + map_share_name_base_length(name) + 1, digits)))
		{
			download->file_count = index;
			download_fail("The host's offer was not of this map.");
			return;
		}
		offered |= (byte)FLAG(file->kind);
		if (file->kind == _map_share_file_map)
		{
			snprintf(file->final_path, sizeof(file->final_path), "%s%s.map", CUSTOM_EDITION_MAP_DIRECTORY, name);
		}
		else
		{
			snprintf(file->final_path, sizeof(file->final_path), "%s%s.map", CUSTOM_EDITION_MAP_DIRECTORY,
				resource_map_type_describe((enum resource_map_type)file->kind));
		}
		snprintf(file->part_path, sizeof(file->part_path), "%s.part", file->final_path);
		file->have = calloc((unsigned long)(file->piece_count + 7) / 8, 1);
		file->asked = calloc((unsigned long)file->piece_count, 1);
		if (!file->have || !file->asked)
		{
			download->file_count = index + 1;
			download_fail("There is not enough memory to download the map.");
			return;
		}
		download->total_bytes += file->size;
	}
	/* (the hashes come first: no piece can be checked without them) */
	for (index = 0; index < download->file_count; index++)
		download->files[index].state = _file_awaiting_hashes;
	download_web_seeds_find(name);
	download->state = _download_transferring;
	download->progress_at = system_milliseconds();
	error(_error_silent, "map sharing: downloading %s, %d files, %lu bytes", download->level_name,
		(int)download->file_count, download->total_bytes);
}

/* the host's sources: the client's to reach */
static void download_sources(
	struct distributed_map_share_sources const *sources,
	struct distributed_map_share_source const *entries)
{
	struct download *download = &map_share_globals.download;
	short index;

	if (download->state != _download_asking && download->state != _download_transferring)
		return;
	download->in_progress = TEST_FLAG(sources->flags, _offer_in_progress_bit);
	/* the host, first (its address, the connection's) */
	if (!download->source_count)
	{
		csmemset(&download->sources[0], 0, sizeof(download->sources[0]));
		download->sources[0].host = TRUE;
		download->sources[0].address = map_share_host_address();
		download->sources[0].connection = NONE;
		download->source_count = 1;
	}
	for (index = 0; index < sources->count && index < MAXIMUM_SOURCES; index++)
	{
		struct distributed_map_share_source const *entry = &entries[index];
		struct source *source = NULL;
		short known;

		if (entry->machine_index == network_game_client_get_local_machine_index())
			continue;
		for (known = 1; known < download->source_count; known++)
		{
			if (download->sources[known].machine_index == entry->machine_index &&
				!csmemcmp(download->sources[known].key_hash, entry->key_hash, sizeof(entry->key_hash)) &&
				download->sources[known].address == entry->address)
			{
				source = &download->sources[known];
				break;
			}
		}
		if (!source)
		{
			if (download->source_count >= NUMBEROF(download->sources))
				continue;
			source = &download->sources[download->source_count++];
			csmemset(source, 0, sizeof(*source));
			source->machine_index = entry->machine_index;
			source->address = entry->address;
			csmemcpy(source->identifier, entry->identifier, sizeof(source->identifier));
			csmemcpy(source->key_hash, entry->key_hash, sizeof(source->key_hash));
			source->connection = NONE;
		}
		source->flags = entry->flags;
	}
}

/* (internet play: the source reached, and its address) */
static boolean source_address(
	struct source *source,
	unsigned long *address)
{
	unsigned long virtual_address;

	if (source->host)
	{
		*address = source->address;
		return *address != 0;
	}
	if (!TEST_FLAG(source->flags, _source_internet_bit))
	{
		*address = source->address;
		return TRUE;
	}
	if (!p2p_peer_reached(source->identifier, &virtual_address))
		return FALSE;
	*address = host_order(virtual_address);
	return TRUE;
}

/* the sources reached: the host, and up to MAXIMUM_OUTGOING others */
static void download_reach_sources(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	struct download *download = &globals->download;
	unsigned long host_address = map_share_host_address();
	boolean internet = address_internet(host_address);
	short local_machine = network_game_client_get_local_machine_index();
	unsigned long now = system_milliseconds();
	short outgoing = 0;
	short index;

	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		outgoing += globals->connections[index].used && globals->connections[index].outgoing;
	for (index = 0; index < download->source_count && outgoing < MAXIMUM_OUTGOING + 1; index++)
	{
		struct source *source = &download->sources[index];
		struct connection *connection;
		unsigned long address;

		if (source->banned || source->connection != NONE ||
			(source->tried_at && !elapsed(source->tried_at, now, SOURCE_RETRY_INTERVAL)))
		{
			continue;
		}
		if (!source->host)
		{
			/* (the internet's players over internet play alone, a LAN's at
			their addresses alone; of two downloading, the one of the lower
			machine connects) */
			if (internet != TEST_FLAG(source->flags, _source_internet_bit))
				continue;
			if (TEST_FLAG(source->flags, _source_downloading_bit) && source->machine_index < local_machine)
				continue;
		}
		if (!source_address(source, &address))
		{
			if (TEST_FLAG(source->flags, _source_internet_bit))
				p2p_reach_player(source->key_hash);
			continue;
		}
		source->tried_at = now | 1;
		connection = connection_new();
		if (!connection)
			return;
		connection->outgoing = TRUE;
		connection->source = index;
		source->connection = (short)(connection - globals->connections);
		connection_set_files(connection);
		connection_connect(connection, address);
		outgoing++;
	}
}

/* a piece had: written, told to every connection about its file */
static void download_piece_had(
	short file_index,
	long piece,
	byte const *data)
{
	struct map_share_globals *globals = &map_share_globals;
	struct download *download = &globals->download;
	struct download_file *file = &download->files[file_index];
	unsigned long offset = (unsigned long)piece * MAP_SHARE_PIECE_BYTES;
	unsigned long bytes = MIN(MAP_SHARE_PIECE_BYTES, file->size - offset);
	short index;

	if (file->have[piece >> 3] & (1 << (piece & 7)))
		return;
	if (!file->stream || fseek(file->stream, (long)offset, SEEK_SET) != 0 ||
		fwrite(data, 1, bytes, file->stream) != bytes)
	{
		download_fail("The map could not be written (is the disk full?).");
		return;
	}
	file->have[piece >> 3] |= (byte)(1 << (piece & 7));
	file->have_count++;
	download->done_bytes += bytes;
	download->window_bytes += bytes;
	download->progress_at = system_milliseconds();
	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
	{
		struct connection *connection = &globals->connections[index];
		short connection_file;

		if (!connection->used || connection->state != _connection_open || !connection->hello_heard)
			continue;
		for (connection_file = 0; connection_file < connection->file_count; connection_file++)
		{
			if (connection->download_file[connection_file] == file_index)
				frame_piece(connection, _frame_have, connection_file, piece);
		}
	}
}

/* the download's files, all had: renamed, the map checked */
static void download_finish(
	void)
{
	struct download *download = &map_share_globals.download;
	char message[256];
	short index;

	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];

		if (file->stream)
		{
			fclose(file->stream);
			file->stream = NULL;
		}
		if (!MoveFileA(file->part_path, file->final_path))
		{
			download_fail("The downloaded map could not be put in custom_maps (a file of its name is there).");
			return;
		}
	}
	custom_edition_maps_look_again();
	if (!custom_edition_cache_present(download->level_name, download->version, message, sizeof(message)))
	{
		csstrncpy(download->message, message, sizeof(download->message) - 1);
		download_fail("The map was downloaded, but cannot be played.");
		return;
	}
	error(_error_silent, "map sharing: downloaded %s (%lu bytes)", download->level_name, download->done_bytes);
	download_report(_progress_done);
	/* (sent on to the others from here, without hashing them again) */
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];

		map_share_file_hashed(file->final_path, file->kind, file->size, file->piece_hashes, file->piece_count,
			file->hash);
	}
	download_end(TRUE);
	download->state = _download_done;
	if (map_share_sending())
		current_files_update(TRUE);
}

/* the rarest piece of a file the connection has that this machine needs
and has not asked anyone for (any it has not asked this connection for,
once every piece has been asked for: the last few from whoever is
quickest), or NONE */
static long piece_choose(
	struct connection *connection,
	short connection_file,
	boolean again)
{
	struct map_share_globals *globals = &map_share_globals;
	struct download_file *file = &globals->download.files[connection->download_file[connection_file]];
	long best = NONE;
	short best_count = 0x7FFF;
	long start;
	long step;

	if (!file->piece_count)
		return NONE;
	/* (from a random place, so the machines downloading ask for different
	pieces of the same rarity) */
	start = (long)(((unsigned long)rand() << 15 ^ (unsigned long)rand()) % (unsigned long)file->piece_count);
	for (step = 0; step < file->piece_count; step++)
	{
		long piece = (start + step) % file->piece_count;
		short count = 0;
		short index;

		if ((file->have[piece >> 3] & (1 << (piece & 7))) || (!again && file->asked[piece]) ||
			(again && file->asked[piece] >= 2))
		{
			continue;
		}
		if (connection->remote_has[connection_file] != _have_all &&
			!(connection->remote_has[connection_file] == _have_pieces && connection->remote_pieces[connection_file] &&
			(connection->remote_pieces[connection_file][piece >> 3] & (1 << (piece & 7)))))
		{
			continue;
		}
		for (index = 0; index < connection->request_count; index++)
		{
			if (connection->requests[index].file == connection->download_file[connection_file] &&
				connection->requests[index].piece == piece)
			{
				break;
			}
		}
		if (index < connection->request_count)
			continue;
		/* (its rarity: how many of the connections have it, each by its own
		index of the file) */
		for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		{
			struct connection const *other = &globals->connections[index];
			short other_file;

			if (!other->used || other->state != _connection_open || !other->hello_heard)
				continue;
			for (other_file = 0; other_file < other->file_count; other_file++)
			{
				if (other->download_file[other_file] == connection->download_file[connection_file])
					break;
			}
			if (other_file < other->file_count &&
				(other->remote_has[other_file] == _have_all ||
				(other->remote_has[other_file] == _have_pieces && other->remote_pieces[other_file] &&
				(other->remote_pieces[other_file][piece >> 3] & (1 << (piece & 7))))))
			{
				count++;
			}
		}
		if (count < best_count)
		{
			best = piece;
			best_count = count;
		}
	}
	return best;
}

/* whether any piece needed has not been asked for */
static boolean download_unasked_pieces(
	void)
{
	struct download *download = &map_share_globals.download;
	short index;

	for (index = 0; index < download->file_count; index++)
	{
		struct download_file const *file = &download->files[index];
		long piece;

		if (file->state != _file_transferring)
		{
			if (file->state != _file_complete)
				return TRUE;
			continue;
		}
		for (piece = 0; piece < file->piece_count; piece++)
		{
			if (!(file->have[piece >> 3] & (1 << (piece & 7))) && !file->asked[piece])
				return TRUE;
		}
	}
	return FALSE;
}

/* the connection asked for pieces, while it may */
static void connection_ask(
	struct connection *connection)
{
	struct download *download = &map_share_globals.download;
	long in_flight = 0;
	short index;

	if (download->state != _download_transferring || !connection->hello_heard)
		return;
	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		in_flight += map_share_globals.connections[index].used ? map_share_globals.connections[index].request_count : 0;
	while (connection->request_count < REQUESTS_PER_CONNECTION && in_flight < MAXIMUM_PIECES_IN_FLIGHT &&
		frame_room(connection, 5))
	{
		struct request *request;
		short connection_file;
		long piece = NONE;
		short again;

		for (again = 0; again < 2 && piece == NONE; again++)
		{
			/* (the last pieces asked again of whoever is quicker, once
			every piece needed has been asked for) */
			if (again && download_unasked_pieces())
				break;
			for (connection_file = 0; connection_file < connection->file_count; connection_file++)
			{
				short file_index = connection->download_file[connection_file];

				if (file_index == NONE || download->files[file_index].state != _file_transferring)
					continue;
				piece = piece_choose(connection, connection_file, again != 0);
				if (piece != NONE)
					break;
			}
		}
		if (piece == NONE)
			return;
		request = &connection->requests[connection->request_count];
		request->buffer = malloc(MAP_SHARE_PIECE_BYTES);
		if (!request->buffer)
			return;
		request->file = connection->download_file[connection_file];
		request->piece = piece;
		request->received = 0;
		request->asked_at = system_milliseconds();
		download->files[request->file].asked[piece]++;
		connection->request_count++;
		in_flight++;
		frame_piece(connection, _frame_request, connection_file, piece);
	}
}

/* the request of the connection's at `index` done with (had, refused or
given up on) */
static void request_remove(
	struct connection *connection,
	short index)
{
	struct download *download = &map_share_globals.download;
	struct request *request = &connection->requests[index];

	if (request->file < download->file_count && download->files[request->file].asked &&
		download->files[request->file].asked[request->piece])
	{
		download->files[request->file].asked[request->piece]--;
	}
	free(request->buffer);
	connection->request_count--;
	csmemmove(request, request + 1, (connection->request_count - index) * sizeof(*request));
	/* (the next is sent now: its time from here) */
	if (!index && connection->request_count)
		connection->requests[0].asked_at = system_milliseconds();
}

/* ---------- a connection's frames, as they come */

static void frame_hello(
	struct connection *connection,
	byte const *body,
	long size)
{
	short count;
	short index;

	if (size < 6 || get_long(body) != FRAME_MAGIC || body[4] != FRAME_VERSION || connection->hello_heard)
	{
		connection_close(connection, "not a map sharing connection");
		return;
	}
	count = body[5];
	if (count < 1 || count > MAXIMUM_FILES || size != 6 + count * MAP_SHARE_HASH_BYTES)
	{
		connection_close(connection, "a bad hello");
		return;
	}
	/* (the end that connected says which files; this end repeats them) */
	if (!connection->outgoing)
	{
		connection->file_count = count;
		for (index = 0; index < count; index++)
			csmemcpy(connection->file_hashes[index], body + 6 + index * MAP_SHARE_HASH_BYTES, MAP_SHARE_HASH_BYTES);
		send_hello(connection);
		/* (asked for files this machine may have: those it sends hashed, to
		know) */
		if (map_share_sending())
			current_files_update(TRUE);
	}
	else if (count != connection->file_count ||
		csmemcmp(connection->file_hashes, body + 6, count * MAP_SHARE_HASH_BYTES))
	{
		connection_close(connection, "another file's hello");
		return;
	}
	connection->hello_heard = TRUE;
	connection_match_files(connection);
}

static void frame_state(
	struct connection *connection,
	byte const *body,
	long size)
{
	struct download *download = &map_share_globals.download;
	short index;

	if (size < 2 || body[0] >= connection->file_count)
		return;
	index = body[0];
	free(connection->remote_pieces[index]);
	connection->remote_pieces[index] = NULL;
	connection->remote_has[index] = _have_none;
	connection->state_heard[index] = TRUE;
	if (body[1] == _have_all)
	{
		connection->remote_has[index] = _have_all;
	}
	else if (body[1] == _have_pieces)
	{
		/* (which pieces only matters to a machine downloading the file too:
		to another, the end is downloading it) */
		connection->remote_has[index] = _have_pieces;
		if (connection->download_file[index] != NONE)
		{
			long bytes = (download->files[connection->download_file[index]].piece_count + 7) / 8;

			connection->remote_pieces[index] = size == 2 + bytes ? malloc(bytes) : NULL;
			if (connection->remote_pieces[index])
				csmemcpy(connection->remote_pieces[index], body + 2, bytes);
		}
	}
}

static void frame_have(
	struct connection *connection,
	byte const *body,
	long size)
{
	struct download *download = &map_share_globals.download;
	struct download_file *file;
	short index;
	long piece;

	if (size != 5 || body[0] >= connection->file_count || connection->download_file[body[0]] == NONE)
		return;
	index = body[0];
	file = &download->files[connection->download_file[index]];
	piece = (long)get_long(body + 1);
	if (piece < 0 || piece >= file->piece_count || connection->remote_has[index] == _have_all)
		return;
	if (!connection->remote_pieces[index])
	{
		connection->remote_pieces[index] = calloc((unsigned long)(file->piece_count + 7) / 8, 1);
		if (!connection->remote_pieces[index])
			return;
	}
	connection->remote_pieces[index][piece >> 3] |= (byte)(1 << (piece & 7));
	connection->remote_has[index] = _have_pieces;
}

/* whether this machine has the connection's file's piece, to send */
static boolean piece_sendable(
	struct connection *connection,
	short index,
	long piece)
{
	struct download *download = &map_share_globals.download;
	struct map_share_file *catalog_file = map_share_file_get(connection->catalog_file[index]);

	if (catalog_file && catalog_file->ready && map_share_sending())
		return piece >= 0 && piece < catalog_file->piece_count;
	if (connection->download_file[index] != NONE)
	{
		struct download_file *file = &download->files[connection->download_file[index]];

		return piece >= 0 && piece < file->piece_count && (file->have[piece >> 3] & (1 << (piece & 7)));
	}
	return FALSE;
}

static void frame_request(
	struct connection *connection,
	byte const *body,
	long size)
{
	long piece;

	if (size != 5 || body[0] >= connection->file_count)
		return;
	piece = (long)get_long(body + 1);
	if (!piece_sendable(connection, body[0], piece) || connection->serving_count >= SERVING_PER_CONNECTION)
	{
		frame_piece(connection, _frame_refuse, body[0], piece);
		return;
	}
	connection->serving[connection->serving_count].file = body[0];
	connection->serving[connection->serving_count].piece = piece;
	connection->serving[connection->serving_count].offset = 0;
	connection->serving_count++;
}

static void frame_block(
	struct connection *connection,
	byte const *body,
	long size)
{
	struct download *download = &map_share_globals.download;
	struct download_file *file;
	struct request *request = NULL;
	unsigned long offset;
	unsigned long bytes;
	unsigned long piece_bytes;
	byte hash[MAP_SHARE_HASH_BYTES];
	short index;
	long piece;

	if (size < 9 || body[0] >= connection->file_count || connection->download_file[body[0]] == NONE)
		return;
	piece = (long)get_long(body + 1);
	offset = get_long(body + 5);
	bytes = (unsigned long)(size - 9);
	for (index = 0; index < connection->request_count; index++)
	{
		if (connection->requests[index].file == connection->download_file[body[0]] &&
			connection->requests[index].piece == piece)
		{
			request = &connection->requests[index];
			break;
		}
	}
	if (!request)
		return;
	file = &download->files[request->file];
	piece_bytes = MIN(MAP_SHARE_PIECE_BYTES, file->size - (unsigned long)piece * MAP_SHARE_PIECE_BYTES);
	if (offset != request->received || bytes > piece_bytes - offset || !bytes)
	{
		connection_close(connection, "a block out of place");
		return;
	}
	csmemcpy(request->buffer + offset, body + 9, bytes);
	request->received += bytes;
	connection->bytes_in += bytes;
	request->asked_at = system_milliseconds();
	if (request->received < piece_bytes)
		return;
	p2p_blake2b_256(request->buffer, piece_bytes, hash);
	if (!file->piece_hashes || csmemcmp(hash, file->piece_hashes + piece * MAP_SHARE_HASH_BYTES, MAP_SHARE_HASH_BYTES))
	{
		if (connection->source != NONE)
			download->sources[connection->source].banned = TRUE;
		connection_close(connection, "a piece that is not the map's");
		return;
	}
	download_piece_had(request->file, piece, request->buffer);
	/* (the download may have ended) */
	if (connection->used && index < connection->request_count)
		request_remove(connection, index);
}

static void frame_refuse(
	struct connection *connection,
	byte const *body,
	long size)
{
	short index;
	long piece;

	if (size != 5 || body[0] >= connection->file_count)
		return;
	piece = (long)get_long(body + 1);
	for (index = 0; index < connection->request_count; index++)
	{
		if (connection->requests[index].file == connection->download_file[body[0]] &&
			connection->requests[index].piece == piece)
		{
			request_remove(connection, index);
			/* (what it has is not what it said: asked again only by what it
			says next) */
			if (connection->remote_has[body[0]] == _have_all)
				connection->remote_has[body[0]] = _have_none;
			else if (connection->remote_pieces[body[0]])
				connection->remote_pieces[body[0]][piece >> 3] &= (byte)~(1 << (piece & 7));
			return;
		}
	}
}

static void frame_hashes_request(
	struct connection *connection,
	byte const *body,
	long size)
{
	if (size != 1 || body[0] >= connection->file_count)
		return;
	connection->hashes_next[body[0]] = 0;
}

static void frame_hashes(
	struct connection *connection,
	byte const *body,
	long size)
{
	struct download *download = &map_share_globals.download;
	struct download_file *file;
	unsigned long first;
	long count;
	byte hash[MAP_SHARE_HASH_BYTES];
	short connection_index = (short)(connection - map_share_globals.connections);

	if (size < 7 || body[0] >= connection->file_count || connection->download_file[body[0]] == NONE)
		return;
	file = &download->files[connection->download_file[body[0]]];
	first = get_long(body + 1);
	count = body[5] | body[6] << 8;
	if (file->piece_hashes || file->hashes_connection != connection_index || !file->hashes_coming ||
		size != 7 + count * MAP_SHARE_HASH_BYTES || first != (unsigned long)file->hashes_received ||
		count > file->piece_count - file->hashes_received)
	{
		return;
	}
	csmemcpy(file->hashes_coming + first * MAP_SHARE_HASH_BYTES, body + 7, count * MAP_SHARE_HASH_BYTES);
	file->hashes_received += count;
	file->hashes_asked_at = system_milliseconds();
	if (file->hashes_received < file->piece_count)
		return;
	/* (checked against the file's hash, the host's word) */
	map_share_file_hash_of_pieces(file->size, file->hashes_coming, file->piece_count, hash);
	file->hashes_connection = NONE;
	if (csmemcmp(hash, file->hash, MAP_SHARE_HASH_BYTES))
	{
		file->hashes_received = 0;
		if (connection->source != NONE)
			download->sources[connection->source].banned = TRUE;
		connection_close(connection, "pieces' hashes that are not the map's");
		return;
	}
	file->piece_hashes = file->hashes_coming;
	file->hashes_coming = NULL;
	if (!download_file_open(file, connection->download_file[body[0]]))
		download_fail("The map could not be written to custom_maps.");
}

/* the frames read in, handled */
static void connection_read(
	struct connection *connection)
{
	for (;;)
	{
		long result;
		long offset = 0;

		if (connection->input_size < INPUT_BYTES)
		{
			result = read_endpoint(connection->endpoint, connection->input + connection->input_size,
				INPUT_BYTES - connection->input_size);
			if (result == _transport_result_operation_would_block)
				result = 0;
			else if (result <= 0)
			{
				connection_close(connection, "lost");
				return;
			}
			connection->input_size += result;
			if (!result && connection->input_size < 5)
				return;
			if (result)
				connection->heard_at = system_milliseconds();
		}
		while (connection->used && connection->input_size - offset >= 5)
		{
			unsigned long length = get_long(connection->input + offset);
			byte type;
			byte const *body;
			long size;

			if (!length || length > MAXIMUM_FRAME_BYTES)
			{
				connection_close(connection, "a frame too large");
				return;
			}
			if ((unsigned long)(connection->input_size - offset - 4) < length)
				break;
			type = connection->input[offset + 4];
			body = connection->input + offset + 5;
			size = (long)length - 1;
			offset += 4 + (long)length;
			if (type != _frame_hello && !connection->hello_heard)
			{
				connection_close(connection, "no hello");
				return;
			}
			switch (type)
			{
			case _frame_hello: frame_hello(connection, body, size); break;
			case _frame_state: frame_state(connection, body, size); break;
			case _frame_have: frame_have(connection, body, size); break;
			case _frame_request: frame_request(connection, body, size); break;
			case _frame_block: frame_block(connection, body, size); break;
			case _frame_refuse: frame_refuse(connection, body, size); break;
			case _frame_hashes_request: frame_hashes_request(connection, body, size); break;
			case _frame_hashes: frame_hashes(connection, body, size); break;
			}
		}
		if (!connection->used)
			return;
		if (offset)
		{
			csmemmove(connection->input, connection->input + offset, connection->input_size - offset);
			connection->input_size -= offset;
		}
		/* (nothing more to read, or a frame not whole yet) */
		if (connection->input_size < INPUT_BYTES)
			return;
	}
}

/* the output written, as much as the connection takes */
static void connection_flush(
	struct connection *connection)
{
	while (connection->output_size > 0)
	{
		long result = write_endpoint(connection->endpoint, connection->output, connection->output_size);

		if (result == _transport_result_operation_would_block)
			return;
		if (result <= 0)
		{
			connection_close(connection, "lost");
			return;
		}
		csmemmove(connection->output, connection->output + result, connection->output_size - result);
		connection->output_size -= result;
		connection->sent_at = system_milliseconds();
	}
}

/* the pieces' hashes this machine was asked for, that fit */
static void connection_send_hashes(
	struct connection *connection)
{
	struct download *download = &map_share_globals.download;
	short index;

	for (index = 0; index < connection->file_count; index++)
	{
		byte const *hashes = NULL;
		long piece_count = 0;
		struct map_share_file *catalog_file = map_share_file_get(connection->catalog_file[index]);

		if (connection->hashes_next[index] == NONE)
			continue;
		if (catalog_file && catalog_file->ready)
		{
			hashes = catalog_file->piece_hashes;
			piece_count = catalog_file->piece_count;
		}
		else if (connection->download_file[index] != NONE &&
			download->files[connection->download_file[index]].piece_hashes)
		{
			hashes = download->files[connection->download_file[index]].piece_hashes;
			piece_count = download->files[connection->download_file[index]].piece_count;
		}
		if (!hashes)
		{
			/* (not known here: asked of another) */
			if (current_files_hashed())
				connection->hashes_next[index] = NONE;
			continue;
		}
		while (connection->hashes_next[index] < piece_count)
		{
			long first = connection->hashes_next[index];
			long count = MIN(HASHES_PER_FRAME, piece_count - first);
			byte *body;

			if (!frame_room(connection, 7 + count * MAP_SHARE_HASH_BYTES))
				return;
			body = frame_begin(connection, _frame_hashes, 7 + count * MAP_SHARE_HASH_BYTES);
			body[0] = (byte)index;
			put_long(body + 1, (unsigned long)first);
			body[5] = (byte)count;
			body[6] = (byte)(count >> 8);
			csmemcpy(body + 7, hashes + first * MAP_SHARE_HASH_BYTES, count * MAP_SHARE_HASH_BYTES);
			connection->hashes_next[index] += count;
		}
		connection->hashes_next[index] = NONE;
	}
}

/* the size of a connection's file this machine sends (0: none) */
static unsigned long serving_file_size(
	struct connection *connection,
	short index)
{
	struct download *download = &map_share_globals.download;
	struct map_share_file *catalog_file = map_share_file_get(connection->catalog_file[index]);

	if (catalog_file && catalog_file->ready && map_share_sending())
		return catalog_file->size;
	if (connection->download_file[index] != NONE)
		return download->files[connection->download_file[index]].size;
	return 0;
}

/* a block of a piece asked for, read from this machine's file */
static boolean block_read(
	struct connection *connection,
	struct serving const *serving,
	byte *buffer,
	unsigned long *bytes)
{
	struct download *download = &map_share_globals.download;
	struct map_share_file *catalog_file = map_share_file_get(connection->catalog_file[serving->file]);
	unsigned long size;
	unsigned long offset = (unsigned long)serving->piece * MAP_SHARE_PIECE_BYTES + serving->offset;
	FILE *stream;
	boolean read;

	if (catalog_file && catalog_file->ready && map_share_sending())
	{
		size = catalog_file->size;
		stream = fopen(catalog_file->path, "rb");
	}
	else if (connection->download_file[serving->file] != NONE)
	{
		size = download->files[connection->download_file[serving->file]].size;
		stream = download->files[connection->download_file[serving->file]].stream;
		if (!stream)
			return FALSE;
		*bytes = MIN(MAP_SHARE_BLOCK_BYTES, MIN(size - offset, MAP_SHARE_PIECE_BYTES - serving->offset));
		return fseek(stream, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, *bytes, stream) == *bytes;
	}
	else
	{
		return FALSE;
	}
	if (!stream)
		return FALSE;
	*bytes = MIN(MAP_SHARE_BLOCK_BYTES, MIN(size - offset, MAP_SHARE_PIECE_BYTES - serving->offset));
	read = fseek(stream, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, *bytes, stream) == *bytes;
	fclose(stream);
	return read;
}

/* the pieces asked of this machine, a block at a time, round all the
connections, as the upload's allowance lets */
static void serve_blocks(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	unsigned long now = system_milliseconds();
	long limit = upload_limit();
	boolean sent = TRUE;

	/* (the allowance: the limit a second, a quarter second of it at once) */
	globals->upload_tokens += (long)(MIN(now - globals->upload_at, 1000UL) * (unsigned long)limit / 1000);
	globals->upload_tokens = MIN(globals->upload_tokens, MAX(limit / 4, MAP_SHARE_BLOCK_BYTES * 2));
	globals->upload_at = now;
	while (sent && globals->upload_tokens >= MAP_SHARE_BLOCK_BYTES)
	{
		short step;

		sent = FALSE;
		for (step = 0; step < MAXIMUM_CONNECTIONS && globals->upload_tokens >= MAP_SHARE_BLOCK_BYTES; step++)
		{
			short index = (short)((globals->next_connection + step) % MAXIMUM_CONNECTIONS);
			struct connection *connection = &globals->connections[index];
			struct serving *serving;
			unsigned long bytes;
			byte *body;

			if (!connection->used || connection->state != _connection_open || !connection->serving_count ||
				!frame_room(connection, 9 + MAP_SHARE_BLOCK_BYTES))
			{
				continue;
			}
			serving = &connection->serving[0];
			body = connection->output + connection->output_size + 5;
			if (!block_read(connection, serving, body + 9, &bytes) || !bytes)
			{
				frame_piece(connection, _frame_refuse, serving->file, serving->piece);
				connection->serving_count--;
				csmemmove(serving, serving + 1, connection->serving_count * sizeof(*serving));
				continue;
			}
			body = frame_begin(connection, _frame_block, 9 + (long)bytes);
			body[0] = (byte)serving->file;
			put_long(body + 1, (unsigned long)serving->piece);
			put_long(body + 5, serving->offset);
			serving->offset += bytes;
			connection->bytes_out += bytes;
			globals->upload_tokens -= (long)bytes;
			sent = TRUE;
			if ((unsigned long)serving->piece * MAP_SHARE_PIECE_BYTES + serving->offset >=
				serving_file_size(connection, serving->file) || serving->offset >= MAP_SHARE_PIECE_BYTES)
			{
				connection->serving_count--;
				csmemmove(serving, serving + 1, connection->serving_count * sizeof(*serving));
			}
			connection_flush(connection);
		}
		globals->next_connection = (short)((globals->next_connection + 1) % MAXIMUM_CONNECTIONS);
	}
}

/* each connection: connected, read, its states said, its pieces and
hashes asked for, written */
static void connections_update(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	struct download *download = &globals->download;
	unsigned long now = system_milliseconds();
	short index;

	for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
	{
		struct connection *connection = &globals->connections[index];
		short file_index;

		if (!connection->used)
			continue;
		if (connection->state == _connection_connecting)
		{
			boolean connected;

			if (!connection_connected(connection, &connected))
			{
				if (elapsed(connection->created_at, now, CONNECT_TIMEOUT))
					connection_close(connection, "could not connect in time");
				continue;
			}
			if (!connected)
			{
				connection_close(connection, "could not connect");
				continue;
			}
			connection->state = _connection_open;
			connection->heard_at = now;
			send_hello(connection);
		}
		/* (matched first: the download may have changed since) */
		if (connection->hello_heard)
			connection_match_files(connection);
		connection_read(connection);
		if (!connection->used)
			continue;
		if (elapsed(connection->heard_at, now, CONNECTION_IDLE_TIMEOUT))
		{
			connection_close(connection, "silent");
			continue;
		}
		if (!connection->hello_heard)
		{
			connection_flush(connection);
			continue;
		}
		for (file_index = 0; file_index < connection->file_count; file_index++)
		{
			short download_file = connection->download_file[file_index];

			if (!connection->state_told[file_index])
				send_state(connection, file_index);
			/* (the pieces' hashes asked of one that has the file whole) */
			if (download_file != NONE && download->files[download_file].state == _file_awaiting_hashes &&
				download->files[download_file].hashes_connection == NONE &&
				connection->remote_has[file_index] == _have_all && frame_room(connection, 1))
			{
				struct download_file *file = &download->files[download_file];
				byte *body;

				free(file->hashes_coming);
				file->hashes_coming = malloc((unsigned long)MAX(file->piece_count, 1) * MAP_SHARE_HASH_BYTES);
				if (!file->hashes_coming)
					continue;
				file->hashes_received = 0;
				file->hashes_connection = index;
				file->hashes_asked_at = now;
				body = frame_begin(connection, _frame_hashes_request, 1);
				body[0] = (byte)file_index;
			}
		}
		/* (nothing left to give each other: the other end has every file
		whole, and this one needs none of them) */
		for (file_index = 0; file_index < connection->file_count; file_index++)
		{
			short download_file = connection->download_file[file_index];

			if (connection->remote_has[file_index] != _have_all ||
				(download_file != NONE && download->files[download_file].state != _file_complete))
			{
				break;
			}
		}
		if (file_index == connection->file_count && !connection->serving_count && !connection->output_size)
		{
			connection_close(connection, NULL);
			continue;
		}
		/* (nor anything to give each other: the other end has none of the
		files, and downloads none of them, or it would say what it has) */
		for (file_index = 0; file_index < connection->file_count; file_index++)
		{
			if (!connection->state_heard[file_index] || connection->remote_has[file_index] != _have_none)
				break;
		}
		if (file_index == connection->file_count && !connection->serving_count && !connection->output_size)
		{
			connection_close(connection, "it has none of the files");
			continue;
		}
		/* (a piece asked for and not coming: asked of another. The pieces
		come in turn, so the first's time is since its last block. Likewise
		the pieces' hashes) */
		if (connection->request_count && elapsed(connection->requests[0].asked_at, now, REQUEST_TIMEOUT))
		{
			connection_close(connection, "a piece did not come");
			continue;
		}
		for (file_index = 0; file_index < download->file_count; file_index++)
		{
			if (download->files[file_index].hashes_connection == index &&
				elapsed(download->files[file_index].hashes_asked_at, now, REQUEST_TIMEOUT))
			{
				break;
			}
		}
		if (file_index < download->file_count)
		{
			connection_close(connection, "the pieces' hashes did not come");
			continue;
		}
		connection_send_hashes(connection);
		connection_ask(connection);
		if (elapsed(connection->sent_at, now, KEEP_ALIVE_INTERVAL) && !connection->output_size && frame_room(connection, 0))
			frame_begin(connection, _frame_keep_alive, 0);
		connection_flush(connection);
	}
}

/* ---------- the client's download, each frame */

/* ---------- the web seeds

A web seed (maps.web_seeds: a folder of the maps by file name, on a web
server) is one more source of the pieces, beside the players: a file of
the download that the index of the maps has (its name and version, and
the offer's size: port/linux/src/map_downloads.c) is asked of it a run of
pieces at a time (an HTTP range, on a thread of its own), pieces no player
has been asked for, and each piece is checked against its hash as a
player's is. One that is not the file's is the end of the web seed for
that file. None on Android and the web (no HTTPS there), or with
maps.downloads off. */

/* each file's web seed, once the offer is known (name: the level's map
file, without .map) */
static void download_web_seeds_find(
	char const *name)
{
	struct download *download = &map_share_globals.download;
	short index;

	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];
		char file_name[MAP_SHARE_PATH_SIZE];

		file->web_url[0] = 0;
		if (file->kind == _map_share_file_map)
			csprintf(file_name, "%s.map", name);
		else
			csprintf(file_name, "%s.map", resource_map_type_describe((enum resource_map_type)file->kind));
		if (map_downloads_web_seed(file_name, file->kind == _map_share_file_map ? download->version : 0, file->size,
			0, file->web_url, sizeof(file->web_url)))
		{
			error(_error_silent, "map sharing: %s comes from the web seed too (%s)", file_name, file->web_url);
		}
	}
}

/* the web seed's pieces, as they come (each checked), and the next run of
them asked for */
static void download_web_seed_update(
	void)
{
	struct download *download = &map_share_globals.download;
	char reason[192];
	short index;

	if (download->web_active)
	{
		struct download_file *file = &download->files[download->web_file];
		int state = map_downloads_range_poll(reason, sizeof(reason));
		long piece;

		if (state == 1)
			return;
		download->web_active = FALSE;
		if (state == 2)
		{
			byte *buffer = malloc(MAP_SHARE_PIECE_BYTES);

			for (piece = download->web_first; buffer && piece < download->web_first + download->web_count; piece++)
			{
				unsigned long offset = (unsigned long)(piece - download->web_first) * MAP_SHARE_PIECE_BYTES;
				unsigned long bytes = MIN(MAP_SHARE_PIECE_BYTES, file->size - (unsigned long)piece * MAP_SHARE_PIECE_BYTES);
				byte hash[MAP_SHARE_HASH_BYTES];

				if (file->have[piece >> 3] & (1 << (piece & 7)))
					continue;
				if (!map_downloads_range_read(offset, buffer, bytes))
				{
					download->web_failures++;
					download->web_retry_at = system_milliseconds() + WEB_SEED_RETRY_INTERVAL;
					break;
				}
				p2p_blake2b_256(buffer, bytes, hash);
				if (csmemcmp(hash, file->piece_hashes + piece * MAP_SHARE_HASH_BYTES, MAP_SHARE_HASH_BYTES))
				{
					error(_error_silent, "map sharing: the web seed's piece %ld of %s is not the map's: not asked again",
						piece, file->final_path);
					file->web_off = TRUE;
					break;
				}
				download->web_bytes += bytes;
				download_piece_had(download->web_file, piece, buffer);
				/* (the download may have ended: a file that could not be
				written) */
				if (download->state != _download_transferring)
				{
					free(buffer);
					map_downloads_range_stop();
					return;
				}
			}
			free(buffer);
		}
		else
		{
			download->web_failures++;
			download->web_retry_at = system_milliseconds() + WEB_SEED_RETRY_INTERVAL;
			error(_error_silent, "map sharing: the web seed did not send pieces %ld to %ld of %s: %s",
				download->web_first, download->web_first + download->web_count - 1, file->final_path, reason);
		}
		for (piece = download->web_first; piece < download->web_first + download->web_count; piece++)
		{
			if (file->asked[piece])
				file->asked[piece]--;
		}
		map_downloads_range_stop();
	}
	if (download->web_failures >= WEB_SEED_FAILURES ||
		(download->web_retry_at && (long)(system_milliseconds() - download->web_retry_at) < 0))
	{
		return;
	}
	/* the next run: the first pieces no one has been asked for, of a file
	whose pieces' hashes are known */
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];
		long first;
		long count;

		if (file->state != _file_transferring || !file->piece_hashes || !file->web_url[0] || file->web_off)
			continue;
		for (first = 0; first < file->piece_count; first++)
		{
			if (!(file->have[first >> 3] & (1 << (first & 7))) && !file->asked[first])
				break;
		}
		if (first == file->piece_count)
			continue;
		for (count = 1; count < WEB_SEED_PIECES && first + count < file->piece_count; count++)
		{
			long piece = first + count;

			if ((file->have[piece >> 3] & (1 << (piece & 7))) || file->asked[piece])
				break;
		}
		/* (a fetch stopped may still be ending, or the archives' under way:
		asked again in a moment) */
		if (!map_downloads_range_start(file->web_url, (unsigned long long)first * MAP_SHARE_PIECE_BYTES,
			MIN((unsigned long)count * MAP_SHARE_PIECE_BYTES, file->size - (unsigned long)first * MAP_SHARE_PIECE_BYTES)))
		{
			download->web_retry_at = system_milliseconds() + WEB_SEED_START_RETRY_INTERVAL;
			return;
		}
		for (count = 0; count < WEB_SEED_PIECES && first + count < file->piece_count; count++)
		{
			long piece = first + count;

			if ((file->have[piece >> 3] & (1 << (piece & 7))) || (count && file->asked[piece]))
				break;
			file->asked[piece]++;
		}
		download->web_active = TRUE;
		download->web_file = index;
		download->web_first = first;
		download->web_count = count;
		return;
	}
}

static void download_update(
	void)
{
	struct download *download = &map_share_globals.download;
	struct network_game *game = map_share_network_game();
	unsigned long now = system_milliseconds();
	short index;

	if (download->state != _download_asking && download->state != _download_transferring)
		return;
	/* (the game left, or its map changed) */
	if (!game || csstrcmp(game->map.name, download->level_name))
	{
		error(_error_silent, "map sharing: the download of %s stopped: the game is on another map now",
			download->level_name);
		download_end(FALSE);
		return;
	}
	if (download->state == _download_asking)
	{
		if (elapsed(download->asked_at, now, OFFER_TIMEOUT))
			download_fail("The host did not answer the request for the map.");
		return;
	}
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];
		byte *piece_hashes;
		long piece_count;
		unsigned long size;

		/* a part file checked: the pieces it has taken */
		if (file->state == _file_checking &&
			map_share_pieces_hashed(download->serial << 3 | index, &piece_hashes, &piece_count, &size))
		{
			long piece;

			file->stream = piece_hashes && piece_count == file->piece_count && size == file->size ?
				fopen(file->part_path, "r+b") : NULL;
			for (piece = 0; file->stream && piece < file->piece_count; piece++)
			{
				if (!csmemcmp(piece_hashes + piece * MAP_SHARE_HASH_BYTES, file->piece_hashes + piece * MAP_SHARE_HASH_BYTES,
					MAP_SHARE_HASH_BYTES))
				{
					file->have[piece >> 3] |= (byte)(1 << (piece & 7));
					file->have_count++;
					download->done_bytes += MIN(MAP_SHARE_PIECE_BYTES, file->size - (unsigned long)piece * MAP_SHARE_PIECE_BYTES);
				}
			}
			free(piece_hashes);
			if (file->stream)
			{
				error(_error_silent, "map sharing: took up %s where it was (%ld of %ld pieces)", file->part_path,
					file->have_count, file->piece_count);
				file->state = _file_transferring;
			}
			/* (unreadable, or changed meanwhile: made anew) */
			else if (!download_file_create(file))
			{
				download_fail("The map could not be written to custom_maps.");
				return;
			}
			download->progress_at = now;
		}
		else if (file->state == _file_checking)
		{
			/* (the hashing thread busy with another's: asked again) */
			map_share_pieces_hash(file->part_path, download->serial << 3 | index);
		}
		if (file->state == _file_transferring && file->have_count == file->piece_count)
			file->state = _file_complete;
	}
	for (index = 0; index < download->file_count && download->files[index].state == _file_complete; index++)
		;
	if (index == download->file_count)
	{
		download_finish();
		return;
	}
	if (elapsed(download->progress_at, now, STALL_TIMEOUT))
	{
		download_fail("The download stopped: no player sent any of the map for a minute.");
		return;
	}
	download_reach_sources();
	download_web_seed_update();
	if (download->state != _download_transferring)
		return;
	if (elapsed(download->reported_at, now, PROGRESS_INTERVAL))
		download_report(_progress_downloading);
	if (elapsed(download->logged_at, now, LOG_INTERVAL))
	{
		short connections = 0;

		for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		{
			connections += map_share_globals.connections[index].used &&
				map_share_globals.connections[index].state == _connection_open;
		}
		download->logged_at = now;
		error(_error_silent, "map sharing: %lu of %lu bytes, %d connections, %lu bytes a second (%lu from the web seed)",
			download->done_bytes, download->total_bytes, (int)connections,
			download->last_window_time ? download->last_window_bytes * 1000 / download->last_window_time : 0,
			download->web_bytes);
	}
	if (elapsed(download->window_at, now, RATE_WINDOW))
	{
		download->last_window_bytes = download->window_bytes;
		download->last_window_time = now - download->window_at;
		download->window_bytes = 0;
		download->window_at = now;
	}
}

/* ---------- the host */

static boolean host_machine_valid(
	long machine_index)
{
	return machine_index >= 0 && machine_index < MAP_SHARE_MACHINES;
}

boolean map_share_machine_downloading(
	long machine_index)
{
	struct host_machine const *machine = host_machine_valid(machine_index) ?
		&map_share_globals.machines[machine_index] : NULL;

	return machine && machine->downloading && !elapsed(machine->heard_at, system_milliseconds(), DOWNLOADING_TIMEOUT);
}

boolean map_share_machine_needs_map(
	long machine_index)
{
	struct host_machine const *machine = host_machine_valid(machine_index) ?
		&map_share_globals.machines[machine_index] : NULL;

	return machine && machine->downloading && !machine->downloaded &&
		!elapsed(machine->heard_at, system_milliseconds(), DOWNLOADING_TIMEOUT);
}

static boolean host_in_game(
	void)
{
	struct network_game_server *server = global_network_game_server_get();

	return server && network_game_server_get_state(server, NULL) == _map_share_server_state_ingame;
}

/* the answer to a machine's request: its files' sizes and hashes */
static void host_offer(
	long machine_index,
	byte result)
{
	struct host_machine *machine = &map_share_globals.machines[machine_index];
	struct
	{
		struct distributed_message_header header;
		struct distributed_map_share_offer offer;
		struct distributed_map_share_offer_file files[MAXIMUM_FILES];
	} message;
	short count = result == _offer_ok ? machine->file_count : 0;
	word size = (word)(sizeof(message.header) + sizeof(message.offer) + count * sizeof(message.files[0]));
	short index;

	csmemset(&message, 0, sizeof(message));
	map_share_fill_header(&message.header, _distributed_message_map_share_offer, size);
	csmemcpy(message.offer.level_name, machine->request.level_name, LEVEL_NAME_BYTES);
	message.offer.result = result;
	message.offer.file_count = (byte)count;
	message.offer.flags = host_in_game() ? FLAG(_offer_in_progress_bit) : 0;
	for (index = 0; index < count; index++)
	{
		struct map_share_file *file = map_share_file_get(machine->catalog_files[index]);

		message.files[index].kind = (byte)machine->file_kinds[index];
		message.files[index].size = file->size;
		csmemcpy(message.files[index].hash, file->hash, MAP_SHARE_HASH_BYTES);
	}
	network_distributed_server_send_to_machine_reliably(machine_index, &message, size);
	machine->offered = TRUE;
	machine->asked = FALSE;
	error(_error_silent, "map sharing: offered machine #%ld %s (%d files)%s", machine_index,
		machine->request.level_name, (int)count, result == _offer_ok ? "" :
		result == _offer_off ? ": not shared (network.map_share)" : ": unavailable");
}

/* the machines a downloading one may fetch from: the game's others */
static void host_sources(
	long machine_index)
{
	struct map_share_globals *globals = &map_share_globals;
	struct
	{
		struct distributed_message_header header;
		struct distributed_map_share_sources sources;
		struct distributed_map_share_source entries[MAXIMUM_SOURCES];
	} message;
	short local_machine = network_game_client_get_local_machine_index();
	short count = 0;
	long index;
	word size;

	csmemset(&message, 0, sizeof(message));
	for (index = 0; index < MAP_SHARE_MACHINES && count < MAXIMUM_SOURCES; index++)
	{
		struct distributed_map_share_source *entry = &message.entries[count];
		unsigned long address = network_game_server_machine_address(index);

		if (index == machine_index || index == local_machine || !address)
			continue;
		entry->machine_index = (byte)index;
		entry->address = address;
		if (address_internet(address))
		{
			if (!p2p_peer_key_hash(network_order(address), entry->identifier, entry->key_hash))
				continue;
			entry->flags |= FLAG(_source_internet_bit);
		}
		if (map_share_machine_downloading(index) && !globals->machines[index].downloaded)
			entry->flags |= FLAG(_source_downloading_bit);
		count++;
	}
	size = (word)(sizeof(message.header) + sizeof(message.sources) + count * sizeof(message.entries[0]));
	map_share_fill_header(&message.header, _distributed_message_map_share_sources, size);
	message.sources.count = (byte)count;
	message.sources.flags = host_in_game() ? FLAG(_offer_in_progress_bit) : 0;
	network_distributed_server_send_to_machine_reliably(machine_index, &message, size);
	globals->machines[machine_index].sources_at = system_milliseconds();
}

/* a machine's request: its files asked to be hashed, the answer given once
they are */
static void host_request(
	long machine_index,
	struct distributed_map_share_request const *request)
{
	struct host_machine *machine = &map_share_globals.machines[machine_index];
	struct network_game *game = map_share_network_game();
	char const *name;
	char path[MAP_SHARE_PATH_SIZE];
	unsigned long needed;
	short kind;

	csmemset(machine, 0, sizeof(*machine));
	machine->request = *request;
	machine->asked_at = system_milliseconds();
	machine->heard_at = machine->asked_at;
	machine->address = network_game_server_machine_address(machine_index);
	if (!config_boolean("network.map_share"))
	{
		host_offer(machine_index, _offer_off);
		return;
	}
	/* (the game's map alone, a Custom Edition map's, named for its hash if
	it is asked for) */
	name = tag_name_strip_path(request->level_name);
	if (!game || csstrcmp(game->map.name, request->level_name) || !custom_edition_level_name(request->level_name) ||
		!custom_edition_map_file_path(request->level_name, path) ||
		(TEST_FLAG(request->wanted, _map_share_file_map) && !map_share_name_hashed(name)))
	{
		host_offer(machine_index, _offer_unavailable);
		return;
	}
	needed = custom_edition_map_resource_maps_needed(request->level_name) | FLAG(_map_share_file_map);
	for (kind = 0; kind < MAXIMUM_FILES; kind++)
	{
		if (!TEST_FLAG(request->wanted, kind) || !TEST_FLAG(needed, kind))
			continue;
		if (kind != _map_share_file_map && !custom_edition_resource_map_file_path(kind, path))
			continue;
		machine->catalog_files[machine->file_count] = map_share_file_hash(path, kind);
		machine->file_kinds[machine->file_count] = kind;
		if (machine->catalog_files[machine->file_count] != NONE)
			machine->file_count++;
		/* (the map's path again, after a resource map's) */
		custom_edition_map_file_path(request->level_name, path);
	}
	if (!machine->file_count)
	{
		host_offer(machine_index, _offer_unavailable);
		return;
	}
	machine->asked = TRUE;
	machine->downloading = TRUE;
	error(_error_silent, "map sharing: machine #%ld asks for %s (files 0x%X)", machine_index, request->level_name,
		(unsigned)request->wanted);
}

/* the host's machines: answered once their files are hashed, told the
sources again now and then, forgotten once gone */
static void host_update(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	unsigned long now = system_milliseconds();
	long index;

	for (index = 0; index < MAP_SHARE_MACHINES; index++)
	{
		struct host_machine *machine = &globals->machines[index];
		unsigned long address = network_game_server_machine_address(index);
		short file_index;

		if (!machine->asked && !machine->downloading)
			continue;
		/* (left: another machine may take its slot) */
		if (!address || address != machine->address)
		{
			csmemset(machine, 0, sizeof(*machine));
			continue;
		}
		if (machine->asked)
		{
			boolean ready = TRUE;
			boolean failed = FALSE;

			for (file_index = 0; file_index < machine->file_count; file_index++)
			{
				struct map_share_file *file = map_share_file_get(machine->catalog_files[file_index]);

				if (!file)
					failed = TRUE;
				else if (file->failed)
					failed = TRUE;
				else if (!file->ready)
					ready = FALSE;
			}
			if (failed)
			{
				host_offer(index, _offer_unavailable);
				machine->downloading = FALSE;
				continue;
			}
			if (!ready)
				continue;
			/* (the map's hash its name's, as the client checks) */
			for (file_index = 0; file_index < machine->file_count; file_index++)
			{
				struct map_share_file *file = map_share_file_get(machine->catalog_files[file_index]);
				char const *name = tag_name_strip_path(machine->request.level_name);
				char digits[MAP_SHARE_NAME_DIGITS + 1];

				map_share_hash_digits(file->hash, digits);
				if (machine->file_kinds[file_index] == _map_share_file_map &&
					csstrcmp(name + map_share_name_base_length(name) + 1, digits))
				{
					error(_error_silent, "map sharing: %s is not named for its hash (%s): not shared", file->path,
						digits);
					failed = TRUE;
				}
			}
			host_offer(index, failed ? _offer_unavailable : _offer_ok);
			if (failed)
			{
				machine->downloading = FALSE;
				continue;
			}
			host_sources(index);
		}
		if (machine->downloading && !machine->downloaded && machine->offered &&
			elapsed(machine->sources_at, now, SOURCES_INTERVAL) &&
			!elapsed(machine->heard_at, now, DOWNLOADING_TIMEOUT))
		{
			host_sources(index);
		}
	}
}

/* a machine's progress */
static void host_progress(
	long machine_index,
	struct distributed_map_share_progress const *progress)
{
	struct host_machine *machine = &map_share_globals.machines[machine_index];

	if (!machine->offered)
		return;
	machine->heard_at = system_milliseconds();
	machine->done = progress->done;
	machine->total = progress->total;
	machine->downloading = progress->state != _progress_failed;
	machine->downloaded = progress->state == _progress_done;
	if (progress->state != _progress_downloading)
	{
		error(_error_silent, "map sharing: machine #%ld %s %s", machine_index,
			progress->state == _progress_done ? "downloaded" : "could not download", machine->request.level_name);
	}
}

/* ---------- messages */

boolean map_share_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_map_share_request || header.type == _distributed_message_map_share_offer ||
		header.type == _distributed_message_map_share_sources || header.type == _distributed_message_map_share_progress;
}

void map_share_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	byte const *entries = (byte const *)message + sizeof(header);
	long length = (long)size - (long)sizeof(header);

	/* (over the sender's connection alone: a datagram can be sent as from
	anyone) */
	if (!map_share_handles_message(message, size) || !from_stream)
		return;
	csmemcpy(&header, message, sizeof(header));
	switch (header.type)
	{
	case _distributed_message_map_share_request:
	{
		struct distributed_map_share_request request;

		if (machine_index == NONE || !map_share_host() || !host_machine_valid(machine_index) ||
			length != (long)sizeof(request))
		{
			return;
		}
		csmemcpy(&request, entries, sizeof(request));
		if (!level_name_valid(request.level_name))
			return;
		host_request(machine_index, &request);
		break;
	}
	case _distributed_message_map_share_progress:
	{
		struct distributed_map_share_progress progress;

		if (machine_index == NONE || !map_share_host() || !host_machine_valid(machine_index) ||
			length != (long)sizeof(progress))
		{
			return;
		}
		csmemcpy(&progress, entries, sizeof(progress));
		host_progress(machine_index, &progress);
		break;
	}
	case _distributed_message_map_share_offer:
	{
		struct distributed_map_share_offer offer;
		struct distributed_map_share_offer_file files[MAXIMUM_FILES];

		if (machine_index != NONE || map_share_host() || length < (long)sizeof(offer))
			return;
		csmemcpy(&offer, entries, sizeof(offer));
		if (!level_name_valid(offer.level_name) || offer.file_count > MAXIMUM_FILES ||
			length != (long)(sizeof(offer) + offer.file_count * sizeof(files[0])))
		{
			return;
		}
		csmemcpy(files, entries + sizeof(offer), offer.file_count * sizeof(files[0]));
		download_offered(&offer, files);
		break;
	}
	case _distributed_message_map_share_sources:
	{
		struct distributed_map_share_sources sources;
		struct distributed_map_share_source sources_entries[MAXIMUM_SOURCES];

		if (machine_index != NONE || map_share_host() || length < (long)sizeof(sources))
			return;
		csmemcpy(&sources, entries, sizeof(sources));
		if (sources.count > MAXIMUM_SOURCES ||
			length != (long)(sizeof(sources) + sources.count * sizeof(sources_entries[0])))
		{
			return;
		}
		csmemcpy(sources_entries, entries + sizeof(sources), sources.count * sizeof(sources_entries[0]));
		download_sources(&sources, sources_entries);
		break;
	}
	}
}

/* ---------- every frame */

void map_share_update(
	void)
{
	struct map_share_globals *globals = &map_share_globals;
	struct network_game *game = map_share_network_game();
	unsigned long host_address = map_share_host_address();
	short index;

	map_share_files_update();
	/* (internet play: a player that shares answers the others, who reach it
	by its key hash) */
	p2p_set_answering_players(!map_share_host() && address_internet(host_address) && map_share_sending());
	/* (none in a build that cannot run Custom Edition maps: the web's) */
	if (!game || !halo_custom_edition_tag_cache())
	{
		/* (one done or failed is forgotten too: the next game's players are
		not held for it) */
		if (globals->download.state != _download_none)
			download_end(FALSE);
		for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		{
			if (globals->connections[index].used)
				connection_close(&globals->connections[index], NULL);
		}
		connections_update();
		listener_update();
		csmemset(globals->machines, 0, sizeof(globals->machines));
		return;
	}
	current_files_update(FALSE);
	if (map_share_host())
		host_update();
	download_update();
	listener_update();
	connection_accept();
	connections_update();
	serve_blocks();
}

void map_share_render(
	void)
{
	struct download *download = &map_share_globals.download;
	long font_tag_index;
	real_argb_color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	rectangle2d bounds;
	char text[160];
	char const *name;
	short base_length;
	long percent;

	if (download->state != _download_asking && download->state != _download_transferring)
		return;
	font_tag_index = interface_get_tag_index(_interface_font_terminal);
	if (font_tag_index == NONE)
		return;
	name = tag_name_strip_path(download->level_name);
	base_length = map_share_name_base_length(name);
	if (download->state == _download_asking)
	{
		snprintf(text, sizeof(text), "Asking the host for the map %.*s...", (int)base_length, name);
	}
	else
	{
		unsigned long rate = download->last_window_time ?
			download->last_window_bytes * 1000 / download->last_window_time : 0;
		short sources = 0;
		short index;

		for (index = 0; index < MAXIMUM_CONNECTIONS; index++)
		{
			struct connection const *connection = &map_share_globals.connections[index];

			sources += connection->used && connection->state == _connection_open && connection->hello_heard;
		}
		percent = download->total_bytes ? (long)(download->done_bytes / (download->total_bytes / 100 + 1)) : 0;
		snprintf(text, sizeof(text), "Downloading the map %.*s: %ld%% of %lu MB from %d player%s, %lu KB/s",
			(int)base_length, name, MIN(percent, 100), (download->total_bytes + 0x80000) >> 20, (int)sources,
			sources == 1 ? "" : "s", rate >> 10);
	}
	/* (at the top of the screen, above the menus' safe area, where the
	lobby's title is) */
	bounds = render.camera.window_bounds;
	offset_rectangle2d(&bounds, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	bounds.y0 = 4;
	bounds.y1 = 22;
	draw_string_set_draw_mode(font_tag_index, NONE, _map_share_text_centered, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
}
