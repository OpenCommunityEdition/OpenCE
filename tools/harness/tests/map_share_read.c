/*
MAP_SHARE_READ.C (test)

How much a map sharing connection (port/linux/game/map_share.c) reads in a
call: all its socket has (a download's blocks, a frame each), not one input
buffer, and no more than MAXIMUM_READS_PER_CALL buffers at once.
test_map_share_read.py takes the constants (config.inc) and the code
(under_test.inc).
*/

#include "harness.h"

#include "config.inc"

#define csmemmove(destination, source, size) memmove(destination, source, size)

/* the part of a connection connection_read uses */
struct connection
{
	boolean used;
	boolean hello_heard;
	void *endpoint;
	byte *input;
	long input_size;
	unsigned long heard_at;
};

/* the socket: a stream of frames, given as much as is asked of it at once,
then "would block" */
static byte *stream;
static long stream_size;
static long stream_at;
static long endpoint_reads;

static long read_endpoint(void *endpoint, byte *buffer, long size)
{
	long count = stream_size - stream_at < size ? stream_size - stream_at : size;

	(void)endpoint;
	endpoint_reads++;
	if (!count)
		return _transport_result_operation_would_block;
	memcpy(buffer, stream + stream_at, (size_t)count);
	stream_at += count;
	return count;
}

static unsigned long system_milliseconds(void) { return 1; }

static long blocks;
static void frame_hello(struct connection *connection, byte const *body, long size)
{
	(void)body;
	(void)size;
	connection->hello_heard = TRUE;
}
static void frame_block(struct connection *connection, byte const *body, long size)
{
	(void)connection;
	(void)body;
	(void)size;
	blocks++;
}
#define FRAME_UNUSED(name) \
	static void name(struct connection *connection, byte const *body, long size) \
	{ (void)connection; (void)body; (void)size; }
FRAME_UNUSED(frame_state)
FRAME_UNUSED(frame_have)
FRAME_UNUSED(frame_request)
FRAME_UNUSED(frame_refuse)
FRAME_UNUSED(frame_hashes_request)
FRAME_UNUSED(frame_hashes)

static void connection_close(struct connection *connection, char const *why)
{
	(void)why;
	connection->used = FALSE;
}

#include "under_test.inc"

/* a stream of a hello and this many blocks of 16 KB, as a host sends them */
enum { BLOCK_BYTES = 0x4000 };

static void stream_make(long count)
{
	long frame = 4 + 1 + 9 + BLOCK_BYTES;
	long index;
	byte *at;

	stream_size = 5 + count * frame;
	stream = calloc((size_t)stream_size, 1);
	at = stream;
	at[0] = 1;
	at[4] = _frame_hello;
	at += 5;
	for (index = 0; index < count; index++)
	{
		unsigned long length = (unsigned long)(frame - 4);

		at[0] = (byte)length;
		at[1] = (byte)(length >> 8);
		at[2] = (byte)(length >> 16);
		at[3] = (byte)(length >> 24);
		at[4] = _frame_block;
		at += frame;
	}
	stream_at = 0;
	endpoint_reads = 0;
	blocks = 0;
}

static struct connection *connection_make(void)
{
	struct connection *connection = calloc(1, sizeof(*connection));

	connection->used = TRUE;
	connection->input = calloc(INPUT_BYTES, 1);
	return connection;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* six blocks waiting, four input buffers' worth: one call takes them
	all (it took one buffer, about a block, a call: a block a frame) */
	CASE("reads-all-waiting")
	{
		struct connection *connection = connection_make();

		stream_make(6);
		connection_read(connection);
		CHECK(blocks == 6, "%ld of 6 blocks read in one call", blocks);
		CHECK(connection->used && connection->input_size == 0, "left %ld bytes", connection->input_size);
		return 0;
	}
	/* more waiting than MAXIMUM_READS_PER_CALL buffers: a call stops there,
	the next goes on */
	CASE("reads-bounded")
	{
		struct connection *connection = connection_make();
		long first;

		stream_make(MAXIMUM_READS_PER_CALL * 3);
		connection_read(connection);
		first = blocks;
		CHECK(endpoint_reads <= MAXIMUM_READS_PER_CALL + 1, "%ld reads in one call", endpoint_reads);
		CHECK(first > 0 && first < MAXIMUM_READS_PER_CALL * 3, "%ld blocks in one call", first);
		connection_read(connection);
		CHECK(blocks > first, "the next call read nothing (%ld)", blocks);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
