/*
MAP_SHARE_RESTART.C (test)

What a map sharing download (port/linux/game/map_share.c) keeps of another
and leaves behind: the pieces another machine said it has, a bit each of the
download's file as the offer counted them, forgotten when the download ends
(another download's file of the same hash may be offered of another size),
and never read or written beyond them; and the files of a download that
cannot be played, deleted again from custom_maps (none that was there
before). test_map_share_restart.py takes the constants and structures
(config.inc) and the code (under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;

/* the allocator: each block followed by SLACK bytes of 0xFF, so that a bit
read beyond a bitmap is had, and one cleared beyond it shows */
enum
{
	SLACK = 1024,
	HEAD = 8,
};

static void *fake_malloc(size_t size)
{
	byte *block = malloc(HEAD + size + SLACK);

	memcpy(block, &size, sizeof(size));
	memset(block + HEAD, 0xA5, size);
	memset(block + HEAD + size, 0xFF, SLACK);
	return block + HEAD;
}

static void *fake_calloc(size_t count, size_t size)
{
	byte *block = fake_malloc(count * size);

	memset(block, 0, count * size);
	return block;
}

static void fake_free(void *pointer)
{
	if (pointer)
		free((byte *)pointer - HEAD);
}

static size_t fake_size(void const *pointer)
{
	size_t size;

	memcpy(&size, (byte const *)pointer - HEAD, sizeof(size));
	return size;
}

/* whether the slack after a block is as it was made */
static boolean fake_slack_intact(void const *pointer)
{
	byte const *slack = (byte const *)pointer + fake_size(pointer);
	long index;

	for (index = 0; index < SLACK; index++)
	{
		if (slack[index] != 0xFF)
			return FALSE;
	}
	return TRUE;
}

#define malloc(size) fake_malloc(size)
#define calloc(count, size) fake_calloc(count, size)
#define free(pointer) fake_free(pointer)

#include "config.inc"

#define csmemset(destination, value, size) memset(destination, value, size)
#define csmemcpy(destination, source, size) memcpy(destination, source, size)
#define csmemmove(destination, source, size) memmove(destination, source, size)
#define csmemcmp(first, second, size) memcmp(first, second, size)
#define csstrcmp(first, second) strcmp(first, second)
#define csstrcasecmp(first, second) strcasecmp(first, second)
#define csstrncpy(destination, source, count) strncpy(destination, source, count)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define _error_silent 0
#define error(level, ...) ((void)0)
#define _progress_done 1

/* the fake world: a clock, a folder of a few files, whether the map can be
played, and what the code under test did */
enum
{
	FAKE_FILES = 8,
};

static unsigned long fake_now = 1000;
static char fake_paths[FAKE_FILES][MAP_SHARE_PATH_SIZE];
static boolean fake_playable;
static int fake_failed;
static int fake_looked_again;

static struct map_share_globals map_share_globals;

static unsigned long system_milliseconds(void)
{
	return fake_now;
}

static short fake_find(char const *path)
{
	short index;

	for (index = 0; index < FAKE_FILES; index++)
	{
		if (fake_paths[index][0] && !strcmp(fake_paths[index], path))
			return index;
	}
	return NONE;
}

static boolean fake_exists(char const *path)
{
	return fake_find(path) != NONE;
}

static void fake_create(char const *path)
{
	short index;

	for (index = 0; index < FAKE_FILES && fake_paths[index][0]; index++)
		;
	if (index < FAKE_FILES)
		strcpy(fake_paths[index], path);
}

/* (MoveFileA does not replace a file: xbox_files.c) */
static boolean MoveFileA(char const *from, char const *to)
{
	short index = fake_find(from);

	if (index == NONE || fake_exists(to))
		return FALSE;
	strcpy(fake_paths[index], to);
	return TRUE;
}

static boolean DeleteFileA(char const *path)
{
	short index = fake_find(path);

	if (index == NONE)
		return FALSE;
	fake_paths[index][0] = 0;
	return TRUE;
}

/* (map_share_files.c's: a finished download's hash kept) */
static void map_share_map_hash_remember(char const *path, byte const *hash)
{
	(void)path;
	(void)hash;
}

/* (map_share.c's: a downloaded map named in custom_maps' list of them) */
static void downloaded_maps_mark(char const *path)
{
	(void)path;
}

static void custom_edition_maps_look_again(void)
{
	fake_looked_again++;
}

static boolean custom_edition_cache_present(char const *level_name, unsigned long version, char *message, long size)
{
	(void)level_name;
	(void)version;
	snprintf(message, (size_t)size, "%s", fake_playable ? "" : "Your map can't be played.");
	return fake_playable;
}

static void download_fail(char const *reason)
{
	(void)reason;
	fake_failed++;
	map_share_globals.download.state = _download_failed;
}

static void download_report(byte state)
{
	(void)state;
}

static short map_share_file_hashed(char const *path, short kind, unsigned long size, byte const *piece_hashes,
	long piece_count, byte const *hash)
{
	(void)path;
	(void)kind;
	(void)size;
	(void)piece_hashes;
	(void)piece_count;
	(void)hash;
	return NONE;
}

static boolean map_share_sending(void)
{
	return FALSE;
}

static void current_files_update(boolean hash)
{
	(void)hash;
}

static void connection_close(struct connection *connection, char const *reason)
{
	(void)reason;
	connection->used = FALSE;
}

#include "under_test.inc"

/* the download's file 0 of `piece_count` pieces, being fetched */
static void download_setup(long piece_count)
{
	struct download *download = &map_share_globals.download;
	struct download_file *file = &download->files[0];

	download->state = _download_transferring;
	download->file_count = 1;
	memset(file, 0, sizeof(*file));
	file->state = _file_transferring;
	file->size = (unsigned long)piece_count * MAP_SHARE_PIECE_BYTES;
	file->piece_count = piece_count;
	file->hashes_connection = NONE;
	file->have = calloc((size_t)(piece_count + 7) / 8, 1);
	file->asked = calloc((size_t)piece_count, 1);
}

/* connection 0, another machine's, open, about the download's file */
static struct connection *connection_setup(void)
{
	struct connection *connection = &map_share_globals.connections[0];
	short index;

	memset(connection, 0, sizeof(*connection));
	connection->used = TRUE;
	connection->state = _connection_open;
	connection->outgoing = FALSE;
	connection->source = NONE;
	connection->hello_heard = TRUE;
	connection->file_count = 1;
	for (index = 0; index < MAXIMUM_FILES; index++)
	{
		connection->download_file[index] = NONE;
		connection->catalog_file[index] = NONE;
		connection->hashes_next[index] = NONE;
	}
	connection->download_file[0] = 0;
	return connection;
}

/* a frame of a file and a piece (have, refuse) */
static void piece_frame(void (*handle)(struct connection *, byte const *, long), struct connection *connection,
	long piece)
{
	byte body[5];

	body[0] = 0;
	body[1] = (byte)piece;
	body[2] = (byte)(piece >> 8);
	body[3] = (byte)(piece >> 16);
	body[4] = (byte)(piece >> 24);
	handle(connection, body, 5);
}

/* a state frame of the pieces the other end has: the first `count` of them,
in a bitmap of `bytes` */
static void state_frame(struct connection *connection, long count, long bytes)
{
	static byte body[2 + 0x1000];
	long piece;

	body[0] = 0;
	body[1] = _have_pieces;
	memset(body + 2, 0, (size_t)bytes);
	for (piece = 0; piece < count; piece++)
		body[2 + (piece >> 3)] |= (byte)(1 << (piece & 7));
	frame_state(connection, body, 2 + bytes);
}

/* the download's files: the map and bitmaps.map, whole in their part files */
static void finish_setup(void)
{
	struct download *download = &map_share_globals.download;
	short index;

	memset(download, 0, sizeof(*download));
	memset(fake_paths, 0, sizeof(fake_paths));
	download->state = _download_transferring;
	strcpy(download->level_name, "custom_maps\\hugeass_0e2b3ee4");
	download->file_count = 2;
	strcpy(download->files[0].final_path, "d:\\custom_maps\\hugeass_0e2b3ee4.map");
	strcpy(download->files[1].final_path, "d:\\custom_maps\\bitmaps.map");
	for (index = 0; index < download->file_count; index++)
	{
		struct download_file *file = &download->files[index];

		file->kind = index;
		file->state = _file_complete;
		file->piece_count = 1;
		file->size = 1;
		file->have_count = 1;
		file->hashes_connection = NONE;
		snprintf(file->part_path, sizeof(file->part_path), "%s.part", file->final_path);
		fake_create(file->part_path);
	}
	/* (a file of custom_maps the download did not bring) */
	fake_create("d:\\custom_maps\\sounds.map");
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	srand(1);
	/* a download of a file of one piece ends, the connection kept; the next
	is of a file of the same hash, 3072 pieces long: what the other end says
	it has of it is a bitmap of 3072 pieces, not of the one */
	CASE("restart-forgets")
	{
		struct connection *connection;

		download_setup(1);
		connection = connection_setup();
		piece_frame(frame_have, connection, 0);
		CHECK(connection->remote_pieces[0] && connection->remote_piece_count[0] == 1, "a piece had was not kept");
		download_end(FALSE);
		CHECK(connection->used, "another machine's connection was not kept");
		CHECK(!connection->remote_pieces[0] && !connection->remote_piece_count[0],
			"the pieces the other end had of the last download's file were kept");
		download_setup(3072);
		connection->download_file[0] = 0;
		piece_frame(frame_have, connection, 3071);
		CHECK(fake_size(connection->remote_pieces[0]) * 8 > 3071, "a bit was set beyond a bitmap of %lu bytes",
			(unsigned long)fake_size(connection->remote_pieces[0]));
		CHECK(remote_piece_had(connection, 0, 3071), "the piece had was not taken");
		return 0;
	}
	/* a bitmap of fewer pieces than the download's file has (said of a file
	of the same hash of another size) is made again of the file's */
	CASE("have-of-another-size")
	{
		struct connection *connection;

		download_setup(1);
		connection = connection_setup();
		piece_frame(frame_have, connection, 0);
		map_share_globals.download.files[0].piece_count = 3072;
		piece_frame(frame_have, connection, 3071);
		CHECK(fake_size(connection->remote_pieces[0]) * 8 > 3071, "a bit was set beyond a bitmap of %lu bytes",
			(unsigned long)fake_size(connection->remote_pieces[0]));
		CHECK(connection->remote_piece_count[0] == 3072, "the bitmap's pieces were not counted: %ld",
			connection->remote_piece_count[0]);
		CHECK(remote_piece_had(connection, 0, 3071), "the piece had was not taken");
		CHECK(!remote_piece_had(connection, 0, 3070), "a piece not had was taken");
		return 0;
	}
	/* the pieces chosen of a connection whose bitmap covers fewer pieces than
	the file has are among those it covers */
	CASE("choose-within")
	{
		struct download_file *file = &map_share_globals.download.files[0];
		struct connection *connection;
		int attempt;

		download_setup(8);
		connection = connection_setup();
		state_frame(connection, 8, 1);
		CHECK(connection->remote_piece_count[0] == 8, "the state's pieces were not counted");
		file->piece_count = 3072;
		free(file->have);
		free(file->asked);
		file->have = calloc(3072 / 8, 1);
		file->asked = calloc(3072, 1);
		for (attempt = 0; attempt < 200; attempt++)
		{
			long piece = piece_choose(connection, 0, FALSE);

			CHECK(piece >= 0 && piece < 8, "a piece beyond the other end's bitmap was chosen: %ld", piece);
		}
		CHECK(fake_slack_intact(connection->remote_pieces[0]), "the bitmap was written beyond");
		return 0;
	}
	/* a piece refused beyond the connection's bitmap clears nothing beyond it */
	CASE("refuse-within")
	{
		struct connection *connection;

		download_setup(8);
		connection = connection_setup();
		state_frame(connection, 8, 1);
		map_share_globals.download.files[0].piece_count = 3072;
		free(map_share_globals.download.files[0].asked);
		map_share_globals.download.files[0].asked = calloc(3072, 1);
		connection->requests[0].file = 0;
		connection->requests[0].piece = 100;
		connection->requests[0].buffer = malloc(MAP_SHARE_PIECE_BYTES);
		connection->request_count = 1;
		piece_frame(frame_refuse, connection, 100);
		CHECK(!connection->request_count, "the refused piece is still asked for");
		CHECK(fake_slack_intact(connection->remote_pieces[0]), "a bit was cleared beyond the bitmap");
		return 0;
	}
	/* a download that can be played is left in custom_maps */
	CASE("finish-playable")
	{
		finish_setup();
		fake_playable = TRUE;
		download_finish();
		CHECK(!fake_failed, "a download that can be played failed");
		CHECK(fake_exists("d:\\custom_maps\\hugeass_0e2b3ee4.map") && fake_exists("d:\\custom_maps\\bitmaps.map"),
			"a download that can be played was not left in custom_maps");
		CHECK(!fake_exists("d:\\custom_maps\\bitmaps.map.part"), "a part file was left");
		CHECK(fake_looked_again == 1, "the map lists looked again %d times", fake_looked_again);
		return 0;
	}
	/* one that cannot is deleted again, and nothing it did not bring */
	CASE("finish-unplayable")
	{
		finish_setup();
		fake_playable = FALSE;
		download_finish();
		CHECK(fake_failed == 1, "a download that cannot be played did not fail");
		CHECK(!fake_exists("d:\\custom_maps\\hugeass_0e2b3ee4.map"), "a map that cannot be played was left");
		CHECK(!fake_exists("d:\\custom_maps\\bitmaps.map"), "a resource map of a map that cannot be played was left");
		CHECK(fake_exists("d:\\custom_maps\\sounds.map"), "a file the download did not bring was deleted");
		CHECK(fake_looked_again == 2, "the map lists did not look again once the files were deleted (%d times)",
			fake_looked_again);
		return 0;
	}
	/* a file not put in custom_maps (one of its name there): those that were
	put there deleted again, and not the one that was there */
	CASE("finish-refused")
	{
		finish_setup();
		fake_create("d:\\custom_maps\\bitmaps.map");
		fake_playable = TRUE;
		download_finish();
		CHECK(fake_failed == 1, "a download whose file could not be put in custom_maps did not fail");
		CHECK(!fake_exists("d:\\custom_maps\\hugeass_0e2b3ee4.map"), "the files put in custom_maps were left");
		CHECK(fake_exists("d:\\custom_maps\\bitmaps.map"), "the file that was there was deleted");
		CHECK(fake_exists("d:\\custom_maps\\bitmaps.map.part"), "the part file not put there was deleted");
		CHECK(fake_looked_again == 1, "the map lists did not look again once the files were deleted (%d times)",
			fake_looked_again);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
