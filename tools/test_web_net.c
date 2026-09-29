/* Focused socket regressions, using the real browser socket implementation.
 * Run from the repository root:
 *   clang -std=c11 -pthread -fsanitize=address -g -Iport/linux/src \
 *     tools/test_web_net.c -o build/test_web_net && build/test_web_net
 */
#include <assert.h>
#include "../port/web/src/web_net.c"

static struct web_shared_state shared;
struct web_shared_state *web_shared_state(void) { return &shared; }

static void incoming(unsigned int kind, unsigned short local_port,
	const void *payload, unsigned int length)
{
	struct web_packet_header header = {
		.size = (sizeof(header) + length + 3) & ~3u,
		.kind = kind,
		.source_ip = 0x02404064u,
		.destination_ip = WEB_DEFAULT_ADDRESS,
		.source_port = swap16(5150),
		.destination_port = local_port,
		.length = length,
	};
	unsigned int write = (unsigned int)shared.net_in_write;

	ring_copy_in(shared.net_in, WEB_NET_IN_BYTES, write, &header, sizeof(header));
	if (length)
		ring_copy_in(shared.net_in, WEB_NET_IN_BYTES, write + sizeof(header), payload, length);
	shared.net_in_write = (int32_t)(write + header.size);
}

static void test_stream_backpressure(void)
{
	int descriptor = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	struct web_socket *socket = socket_get(descriptor);
	unsigned char payload[64], small[16];
	unsigned char *received = malloc(STREAM_CAPACITY);
	posix_ulong available;
	unsigned int blocked_read;

	assert(descriptor >= SOCKET_BASE && received);
	assert(posix_socket_set_nonblocking(descriptor, 1) == 0);
	socket->connected = socket->bound = socket->remote_link = 1;
	socket->local.port = swap16(49152);
	socket->remote.ip = 0x02404064u;
	socket->remote.port = swap16(5150);
	socket->stream = malloc(STREAM_CAPACITY);
	assert(socket->stream);
	memset(socket->stream, 'a', STREAM_CAPACITY);
	socket->stream_head = STREAM_CAPACITY - 16; /* Exercise socket-ring wrap. */
	socket->stream_count = STREAM_CAPACITY - 32;
	for (unsigned int i = 0; i < sizeof(payload); i++) payload[i] = (unsigned char)i;
	/* Exercise shared-ring wrap too, and keep CLOSE behind the blocked DATA. */
	shared.net_in_read = shared.net_in_write = WEB_NET_IN_BYTES - 16;
	blocked_read = (unsigned int)shared.net_in_read;
	incoming(WEB_PACKET_DATA, socket->local.port, payload, sizeof(payload));
	incoming(WEB_PACKET_CLOSE, socket->local.port, NULL, 0);
	assert(posix_socket_bytes_available(descriptor, &available) == 0);
	assert(available == STREAM_CAPACITY - 32);
	assert((unsigned int)shared.net_in_read == blocked_read && !socket->peer_closed);
	assert(posix_socket_recv(descriptor, small, sizeof(small), 0) == sizeof(small));
	assert(posix_socket_bytes_available(descriptor, &available) == 0);
	assert((unsigned int)shared.net_in_read == blocked_read); /* Still 16 bytes short. */
	assert(posix_socket_recv(descriptor, small, sizeof(small), 0) == sizeof(small));
	/* The next pump has room for all 64 bytes, then observes the ordered CLOSE. */
	assert(posix_socket_bytes_available(descriptor, &available) == 0);
	assert(available == STREAM_CAPACITY && socket->peer_closed);
	assert(shared.net_in_read == shared.net_in_write);
	assert(posix_socket_recv(descriptor, received, STREAM_CAPACITY, 0) == STREAM_CAPACITY);
	for (int i = 0; i < STREAM_CAPACITY - (int)sizeof(payload); i++) assert(received[i] == 'a');
	assert(memcmp(received + STREAM_CAPACITY - sizeof(payload), payload, sizeof(payload)) == 0);
	assert(posix_socket_recv(descriptor, small, sizeof(small), 0) == 0);
	assert(posix_socket_close(descriptor) == 0);
	free(received);
}

static void test_datagram_peek_and_truncation(void)
{
	int descriptor = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	struct address to = { .family = AF_INET_VALUE, .port = swap16(5151), .ip = WEB_DEFAULT_ADDRESS };
	unsigned char received[32];
	const char payload[] = "datagram bytes";
	posix_ulong available;

	assert(descriptor >= SOCKET_BASE);
	assert(posix_socket_bind(descriptor, &to, sizeof(to)) == 0);
	assert(posix_socket_set_nonblocking(descriptor, 1) == 0);
	incoming(WEB_PACKET_DATAGRAM, to.port, payload, sizeof(payload));
	assert(posix_socket_recv(descriptor, received, 4, 2) == 4); /* MSG_PEEK */
	assert(memcmp(received, payload, 4) == 0);
	assert(posix_socket_bytes_available(descriptor, &available) == 0 && available == sizeof(payload));
	assert(posix_socket_recv(descriptor, received, sizeof(received), 2) == sizeof(payload));
	assert(posix_socket_recv(descriptor, received, sizeof(received), 0) == sizeof(payload));
	assert(memcmp(received, payload, sizeof(payload)) == 0);
	assert(posix_socket_bytes_available(descriptor, &available) == 0 && available == 0);
	incoming(WEB_PACKET_DATAGRAM, to.port, payload, sizeof(payload));
	assert(posix_socket_recv(descriptor, received, 4, 0) == -1);
	assert(posix_socket_last_error() == WSAEMSGSIZE);
	assert(memcmp(received, payload, 4) == 0);
	assert(posix_socket_bytes_available(descriptor, &available) == 0 && available == 0);
	assert(posix_socket_close(descriptor) == 0);
}

int main(void)
{
	test_stream_backpressure();
	test_datagram_peek_and_truncation();
	puts("web_net stream backpressure and datagram regression tests passed");
	return 0;
}
