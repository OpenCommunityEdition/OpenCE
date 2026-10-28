/*
MAP_SHARE_TRUST.C (test)

What map sharing (port/linux/game/map_share.c) takes of another machine's
word: the LAN addresses a host names, the pieces' hashes (asked of the host
first, none in a frame refused), a client's progress, which keeps it in the
host's game only while it grows, and its requests for the map, taken a few
seconds apart at most. test_map_share_trust.py takes the constants and
structures (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;

#include "config.inc"

#define csmemset(destination, value, size) memset(destination, value, size)
#define csmemcpy(destination, source, size) memcpy(destination, source, size)
#define csmemcmp(first, second, size) memcmp(first, second, size)
#define csstrcmp(first, second) strcmp(first, second)
#define csstrncmp(first, second, count) strncmp(first, second, count)
#define csstrncpy(destination, source, count) strncpy(destination, source, count)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define _error_silent 0
#define error(level, ...) ((void)0)
#define server_event_map_share(machine_index, map, state, done, total) ((void)0)
#define _distributed_message_map_share_offer 0x40

/* the fake world: a clock, the host's game and its machines, a catalog of one
file, and what the code under test did */
struct distributed_message_header
{
	word header;
	byte type;
	byte count;
	long game_time;
};

struct network_game
{
	struct
	{
		char name[128];
		long version;
	} map;
};

static unsigned long fake_now = 1000;
static struct network_game fake_game;
static unsigned long fake_machine_addresses[MAP_SHARE_MACHINES];
static struct map_share_file fake_file;
static int fake_offers;
static byte fake_offer_result;
static int fake_opened;

static struct map_share_globals map_share_globals;

static unsigned long system_milliseconds(void)
{
	return fake_now;
}

static struct network_game *map_share_network_game(void)
{
	return &fake_game;
}

static unsigned long network_game_server_machine_address(long machine_index)
{
	return machine_index >= 0 && machine_index < MAP_SHARE_MACHINES ? fake_machine_addresses[machine_index] : 0;
}

static boolean host_in_game(void)
{
	return FALSE;
}

static void map_share_fill_header(struct distributed_message_header *header, byte type, word size)
{
	(void)size;
	header->type = type;
}

/* (an offer's result, after its header and level name) */
static boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size)
{
	(void)machine_index;
	(void)size;
	fake_offer_result = ((byte const *)message)[sizeof(struct distributed_message_header) + LEVEL_NAME_BYTES];
	fake_offers++;
	return TRUE;
}

static void host_sources(long machine_index)
{
	(void)machine_index;
}

static struct map_share_file *map_share_file_get(short index)
{
	return index == 0 ? &fake_file : NULL;
}

static short map_share_file_hash(char const *path, short kind)
{
	(void)path;
	(void)kind;
	return 0;
}

static char const *tag_name_strip_path(char const *name)
{
	char const *slash = strrchr(name, '\\');

	return slash ? slash + 1 : name;
}

#define config_boolean(name) 1
#define custom_edition_level_name(level_name) (!strncmp(level_name, "custom_maps\\", 12))
#define custom_edition_map_file_path(level_name, path) (strcpy(path, "d:\\custom_maps\\hugeass.map"), 1)
#define custom_edition_resource_map_file_path(kind, path) 0
#define custom_edition_map_resource_maps_needed(level_name) 0UL
/* (map_share_files.c's: the hash's first 32 bits) */
#define map_share_hash_version(hash) \
	((unsigned long)(hash)[0] << 24 | (unsigned long)(hash)[1] << 16 | (unsigned long)(hash)[2] << 8 | (hash)[3])

/* the pieces' hashes: a file's hash the first piece's here, which is enough to
tell a whole list from none */
static void map_share_file_hash_of_pieces(unsigned long size, byte const *piece_hashes, long piece_count, byte *hash)
{
	(void)size;
	(void)piece_count;
	memcpy(hash, piece_hashes, MAP_SHARE_HASH_BYTES);
}

static boolean download_file_open(struct download_file *file, short index)
{
	(void)file;
	(void)index;
	fake_opened++;
	return TRUE;
}

static void download_fail(char const *reason)
{
	(void)reason;
}

static void connection_close(struct connection *connection, char const *reason)
{
	(void)reason;
	connection->used = FALSE;
}

#include "under_test.inc"

#define ADDRESS(a, b, c, d) \
	((unsigned long)(a) << 24 | (unsigned long)(b) << 16 | (unsigned long)(c) << 8 | (unsigned long)(d))

/* a hashes frame (the file, the first, the count, then its hashes, each one's
first byte its number) fed to the connection */
static void hashes_frame(struct connection *connection, long first, long count)
{
	static byte body[7 + 0x10000];
	long index;

	body[0] = 0;
	body[1] = (byte)first;
	body[2] = (byte)(first >> 8);
	body[3] = 0;
	body[4] = 0;
	body[5] = (byte)count;
	body[6] = (byte)(count >> 8);
	memset(body + 7, 0, (size_t)count * MAP_SHARE_HASH_BYTES);
	for (index = 0; index < count; index++)
		body[7 + index * MAP_SHARE_HASH_BYTES] = (byte)(first + index);
	frame_hashes(connection, body, 7 + count * MAP_SHARE_HASH_BYTES);
}

/* the host's game on a map whose file is 3 MB, machine 3 in it */
static void host_setup(void)
{
	strcpy(fake_game.map.name, "custom_maps\\hugeass");
	fake_game.map.version = 0x0e2b3ee4;
	fake_machine_addresses[3] = ADDRESS(10, 0, 0, 3);
	memset(&fake_file, 0, sizeof(fake_file));
	fake_file.hash[0] = 0x0e;
	fake_file.hash[1] = 0x2b;
	fake_file.hash[2] = 0x3e;
	fake_file.hash[3] = 0xe4;
	fake_file.used = TRUE;
	fake_file.ready = TRUE;
	fake_file.size = 3 * 1024 * 1024;
	fake_file.piece_count = 12;
}

/* machine 3 asks for the map, and is offered it (as host_update does once
the file is hashed) */
static void host_ask(void)
{
	struct distributed_map_share_request request;

	memset(&request, 0, sizeof(request));
	strcpy(request.level_name, fake_game.map.name);
	request.wanted = FLAG(_map_share_file_map);
	host_request(3, &request);
	host_offer(3, _offer_ok);
}

/* machine 3's request for a level, as it comes over its connection */
static void host_asked_for(char const *level_name)
{
	struct distributed_map_share_request request;

	memset(&request, 0, sizeof(request));
	strcpy(request.level_name, level_name);
	request.wanted = FLAG(_map_share_file_map);
	host_asked(3, &request);
}

/* machine 3 says it has `done` bytes */
static void host_told(unsigned long done)
{
	struct distributed_map_share_progress progress;

	memset(&progress, 0, sizeof(progress));
	progress.state = _progress_downloading;
	progress.done = done;
	progress.total = fake_file.size;
	host_progress(3, &progress);
}

/* `time` milliseconds pass, machine 3 saying it has `done` bytes every two
seconds */
static void host_wait(unsigned long time, unsigned long done)
{
	unsigned long end = fake_now + time;

	while (fake_now < end)
	{
		fake_now += 2000;
		host_told(done);
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a LAN host names its players' private addresses alone, and never
	this machine's, nor 0.0.0.0, multicast or internet play's */
	CASE("source-addresses")
	{
		unsigned long lan_host = ADDRESS(192, 168, 1, 5);

		CHECK(source_address_allowed(ADDRESS(192, 168, 1, 9), lan_host), "a player beside the host was refused");
		CHECK(source_address_allowed(ADDRESS(10, 1, 2, 3), lan_host), "a private network's player was refused");
		CHECK(source_address_allowed(ADDRESS(172, 20, 0, 1), lan_host), "a 172.16/12 player was refused");
		CHECK(source_address_allowed(ADDRESS(169, 254, 7, 7), lan_host), "a link-local player was refused");
		CHECK(!source_address_allowed(ADDRESS(8, 8, 8, 8), lan_host), "a LAN host sent its players to the internet");
		CHECK(!source_address_allowed(ADDRESS(172, 32, 0, 1), lan_host), "172.32 was taken for a private network's");
		CHECK(!source_address_allowed(ADDRESS(127, 0, 0, 1), lan_host), "a LAN host sent its players to themselves");
		CHECK(!source_address_allowed(0, lan_host), "0.0.0.0 was taken");
		CHECK(!source_address_allowed(ADDRESS(224, 0, 0, 1), lan_host), "a multicast address was taken");
		CHECK(!source_address_allowed(ADDRESS(255, 255, 255, 255), lan_host), "the broadcast address was taken");
		CHECK(!source_address_allowed(ADDRESS(100, 64, 0, 9), lan_host), "internet play's address was taken");
		/* (copies of the game on one machine, and a VPN of public addresses) */
		CHECK(source_address_allowed(ADDRESS(127, 0, 0, 2), ADDRESS(127, 0, 0, 1)),
			"a copy beside a loopback host was refused");
		CHECK(source_address_allowed(ADDRESS(25, 4, 5, 6), ADDRESS(25, 1, 2, 3)), "a VPN's player was refused");
		return 0;
	}
	/* the pieces' hashes: the host's at once, the others it named a little
	later, connections other machines made last */
	CASE("hashes-sources")
	{
		struct download *download = &map_share_globals.download;
		struct connection *connections = map_share_globals.connections;

		download->offered_at = fake_now;
		download->source_count = 2;
		download->sources[0].host = TRUE;
		connections[0].outgoing = TRUE;
		connections[0].source = 0;
		connections[1].outgoing = TRUE;
		connections[1].source = 1;
		connections[2].outgoing = FALSE;
		connections[2].source = NONE;
		CHECK(hashes_source(&connections[0], fake_now), "the host was not asked at once");
		CHECK(!hashes_source(&connections[1], fake_now), "another player was asked before the host had a chance");
		CHECK(!hashes_source(&connections[2], fake_now), "a connection anyone could make was asked at once");
		CHECK(hashes_source(&connections[1], fake_now + HASHES_SOURCES_DELAY), "the players the host named were never asked");
		CHECK(!hashes_source(&connections[2], fake_now + HASHES_SOURCES_DELAY),
			"a connection anyone could make was asked as soon as the host's players");
		CHECK(hashes_source(&connections[2], fake_now + REQUEST_TIMEOUT), "the other connections were never asked");
		return 0;
	}
	/* a hashes frame that brings none keeps nothing alive; the rest are
	taken in order, and the file opened once they all came */
	CASE("hashes-frames")
	{
		struct download *download = &map_share_globals.download;
		struct download_file *file = &download->files[0];
		struct connection *connection = &map_share_globals.connections[0];
		unsigned long asked_at = fake_now;

		download->file_count = 1;
		file->piece_count = 300;
		file->hashes_coming = calloc(300, MAP_SHARE_HASH_BYTES);
		file->hashes_connection = 0;
		file->hashes_started_at = asked_at;
		file->hashes_asked_at = asked_at;
		connection->used = TRUE;
		connection->file_count = 1;
		connection->download_file[0] = 0;
		fake_now += REQUEST_TIMEOUT / 2;
		hashes_frame(connection, 0, 0);
		CHECK(file->hashes_asked_at == asked_at, "a frame of no hashes kept the request alive");
		CHECK(file->hashes_received == 0, "a frame of no hashes counted");
		hashes_frame(connection, 0, 256);
		CHECK(file->hashes_received == 256, "the first hashes were not taken: %ld", file->hashes_received);
		CHECK(file->hashes_asked_at == fake_now, "hashes that came did not keep the request alive");
		hashes_frame(connection, 0, 44);
		CHECK(file->hashes_received == 256, "hashes out of order were taken");
		hashes_frame(connection, 256, 45);
		CHECK(file->hashes_received == 256, "more hashes than the file's pieces were taken");
		hashes_frame(connection, 256, 44);
		CHECK(file->piece_hashes && fake_opened == 1, "the whole list was not taken");
		return 0;
	}
	/* a client whose progress stops is not taken for downloading once an
	honest one would have given up, however often it says it is */
	CASE("progress-advances")
	{
		host_setup();
		host_ask();
		CHECK(fake_offers == 1, "no offer was made");
		CHECK(map_share_machine_downloading(3) && map_share_machine_needs_map(3), "an offered machine is not downloading");
		host_wait(DOWNLOADING_STALL_TIMEOUT - 2000, 0);
		CHECK(map_share_machine_downloading(3), "a machine was given up on before an honest one would give up");
		host_wait(4000, 0);
		CHECK(!map_share_machine_downloading(3), "a machine that has had nothing for long is still downloading");
		CHECK(!map_share_machine_needs_map(3), "a machine that has had nothing for long still holds the others");
		host_told(MAP_SHARE_PIECE_BYTES);
		CHECK(map_share_machine_downloading(3), "a piece that came did not count");
		/* (a byte at a time is not a piece) */
		host_wait(DOWNLOADING_STALL_TIMEOUT + 2000, MAP_SHARE_PIECE_BYTES + 1000);
		CHECK(!map_share_machine_downloading(3), "a byte more each time kept a machine downloading");
		/* (the whole, said once, counts once; more than the whole is the whole) */
		host_told(0xFFFFFFFFUL);
		CHECK(map_share_machine_downloading(3), "the last pieces did not count");
		CHECK(map_share_globals.machines[3].done == fake_file.size, "more than was offered was taken: %lu",
			map_share_globals.machines[3].done);
		host_wait(DOWNLOADING_STALL_TIMEOUT + 2000, 0xFFFFFFFFUL);
		CHECK(!map_share_machine_downloading(3), "saying it has it all kept a machine downloading");
		return 0;
	}
	/* asking again does not start the count again; a machine that left is
	forgotten */
	CASE("progress-asked-again")
	{
		host_setup();
		host_ask();
		host_wait(DOWNLOADING_STALL_TIMEOUT + 2000, 0);
		CHECK(!map_share_machine_downloading(3), "a machine that has had nothing for long is still downloading");
		host_ask();
		CHECK(fake_offers == 2, "the request asked again was not answered");
		CHECK(!map_share_machine_downloading(3), "asking again made a machine downloading again");
		host_ask();
		host_ask();
		CHECK(!map_share_machine_downloading(3), "asking again and again made a machine downloading again");
		/* (another machine in its slot) */
		fake_machine_addresses[3] = ADDRESS(10, 0, 0, 4);
		host_ask();
		CHECK(map_share_machine_downloading(3), "another machine in the slot was taken for the last");
		return 0;
	}
	/* nor does asking for another level between, which is answered that it
	is not the game's; a game on another map starts the count again */
	CASE("progress-other-level-between")
	{
		struct distributed_map_share_request request;

		host_setup();
		host_ask();
		host_wait(DOWNLOADING_STALL_TIMEOUT + 2000, 0);
		CHECK(!map_share_machine_downloading(3), "a machine that has had nothing for long is still downloading");
		memset(&request, 0, sizeof(request));
		strcpy(request.level_name, "custom_maps\\bloodgulch");
		request.wanted = FLAG(_map_share_file_map);
		host_request(3, &request);
		CHECK(fake_offers == 2 && fake_offer_result == _offer_unavailable, "another level was offered");
		host_ask();
		CHECK(!map_share_machine_downloading(3), "asking for another level between made a machine downloading again");
		host_request(3, &request);
		host_ask();
		CHECK(!map_share_machine_downloading(3), "asking for another level between, again, made it downloading");
		/* (the host's game on another map) */
		strcpy(fake_game.map.name, "custom_maps\\hugeass2");
		host_ask();
		CHECK(map_share_machine_downloading(3), "a game on another map kept the last map's count");
		return 0;
	}
	/* a machine's requests are taken MAP_REQUEST_INTERVAL apart at most: one
	sooner waits, the last of them taken in its turn (a millisecond more: the
	time a request was taken is kept odd, never 0) */
	CASE("offer-of-the-version")
	{
		host_setup();
		host_asked_for(fake_game.map.name);
		host_update();
		CHECK(fake_offers == 1 && fake_offer_result == _offer_ok, "the game's version was not offered");
		/* (the host's file another version than the game is on) */
		host_setup();
		fake_file.hash[3] = 0xe5;
		fake_now += MAP_REQUEST_INTERVAL + 1;
		host_asked_for(fake_game.map.name);
		host_update();
		CHECK(fake_offers == 2 && fake_offer_result == _offer_unavailable,
			"a map of another version than the game's was offered");
		return 0;
	}
	CASE("requests-apart")
	{
		int asked;

		host_setup();
		host_asked_for(fake_game.map.name);
		host_update();
		CHECK(fake_offers == 1 && fake_offer_result == _offer_ok, "the first request was not answered");
		for (asked = 0; asked < 50; asked++)
		{
			fake_now += 10;
			host_asked_for(asked & 1 ? fake_game.map.name : "custom_maps\\bloodgulch");
			host_update();
		}
		CHECK(fake_offers == 1, "requests one after another were all answered: %d", fake_offers);
		fake_now += MAP_REQUEST_INTERVAL + 1;
		host_update();
		CHECK(fake_offers == 2 && fake_offer_result == _offer_ok, "the last request was not answered in its turn");
		CHECK(!strcmp(map_share_globals.machines[3].request.level_name, fake_game.map.name),
			"a request before the last was taken");
		host_update();
		CHECK(fake_offers == 2, "a request was answered twice");
		/* (asked again later, its download failed: answered at once) */
		fake_now += MAP_REQUEST_INTERVAL + 1;
		host_asked_for(fake_game.map.name);
		host_update();
		CHECK(fake_offers == 3, "a request a while after the last was not answered at once");
		return 0;
	}
	/* a request waiting its turn after one answered that the level is not the
	game's, which leaves nothing else to look at */
	CASE("requests-apart-after-another-level")
	{
		host_setup();
		host_asked_for("custom_maps\\bloodgulch");
		CHECK(fake_offers == 1 && fake_offer_result == _offer_unavailable, "another level was offered");
		host_asked_for(fake_game.map.name);
		host_update();
		CHECK(fake_offers == 1, "a request just after another was answered at once");
		fake_now += MAP_REQUEST_INTERVAL + 1;
		host_update();
		CHECK(fake_offers == 2 && fake_offer_result == _offer_ok, "the request that waited was never answered");
		/* (another machine in the slot: not held for the last's requests) */
		fake_machine_addresses[3] = ADDRESS(10, 0, 0, 4);
		host_asked_for(fake_game.map.name);
		CHECK(map_share_globals.machines[3].address == ADDRESS(10, 0, 0, 4),
			"another machine in the slot waited for the last's turn");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
