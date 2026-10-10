/*
WEB_NET_TEST.C

port/web/src/web_net.c, the web build's sockets inside the page, compiled
for this computer and tried as the game uses them (tools/test_web_build.py
builds and runs it): datagrams to loopback, to the machine's own address and
by broadcast; a stream connected, accepted, written and read, and its end;
select; and the errors Winsock gives. Then the other browsers of a room:
the frames the page would carry to them (caught here in place of the page's
web_js_net_send), and theirs handed in as the page hands them, malformed ones
among them. Then internet play's: a broker's WebSocket (its URL's stand-in
address, the page's WebSocket opening, bytes each way, its end), a tunnel
packet the page's WebRTC hands in, and the command line's arguments.
*/

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "posix.h"

#define AF_INET_VALUE 2
#define SOCK_STREAM_VALUE 1
#define SOCK_DGRAM_VALUE 2
#define WSAEWOULDBLOCK 10035
#define WSAEMSGSIZE 10040
#define WSAEADDRINUSE 10048
#define WSAECONNREFUSED 10061

struct address
{
	unsigned short family;
	unsigned short port;
	unsigned int ip;
	unsigned char zero[8];
};

/* ---------- the page, as web_net.c sees it */

void web_net_receive(unsigned int from, int reliable, const unsigned char *frame, int size);
void web_net_peer_lost(unsigned int address);
void web_net_websocket_opened(int index, unsigned int connection);
void web_net_websocket_data(int index, unsigned int connection, const unsigned char *data, int size);
void web_net_websocket_closed(int index, unsigned int connection);
void web_net_inject(unsigned int from_ip, unsigned short from_port, unsigned short to_port, const void *data, int size);
void web_net_arguments(int count, char **values);

#define MAXIMUM_SENT 64

static struct
{
	unsigned int address;
	int reliable;
	unsigned char *frame;
	int size;
} sent[MAXIMUM_SENT];
static int sent_count;

void web_js_net_send(unsigned int address, int reliable, unsigned char *frame, int size)
{
	assert(sent_count < MAXIMUM_SENT);
	sent[sent_count].address = address;
	sent[sent_count].reliable = reliable;
	sent[sent_count].frame = frame;
	sent[sent_count].size = size;
	sent_count++;
}

/* the page's WebSocket, as web_net.c asked for it */
static struct
{
	int index;
	unsigned int connection;
	char url[256];
	int opened;
	int closed;
	unsigned char bytes[64];
	int size;
} websocket;

void web_js_websocket_open(int index, unsigned int connection, const char *url)
{
	websocket.index = index;
	websocket.connection = connection;
	snprintf(websocket.url, sizeof(websocket.url), "%s", url);
	websocket.opened++;
}

void web_js_websocket_send(int index, unsigned int connection, unsigned char *bytes, int size)
{
	assert(index == websocket.index && connection == websocket.connection && size <= (int)sizeof(websocket.bytes));
	memcpy(websocket.bytes, bytes, (size_t)size);
	websocket.size = size;
	free(bytes);
}

void web_js_websocket_close(int index, unsigned int connection)
{
	assert(index == websocket.index && connection == websocket.connection);
	websocket.closed++;
}

static void sent_clear(void)
{
	while (sent_count)
		free(sent[--sent_count].frame);
}

static struct address address_of(unsigned int a, unsigned int b, unsigned int c, unsigned int d, unsigned short port)
{
	struct address address;

	memset(&address, 0, sizeof(address));
	address.family = AF_INET_VALUE;
	address.port = (unsigned short)((port >> 8) | (port << 8));
	address.ip = a | (b << 8) | (c << 16) | (d << 24);
	return address;
}

static void test_datagrams(void)
{
	struct address loopback = address_of(127, 0, 0, 1, 2302), any = address_of(0, 0, 0, 0, 2302);
	struct address broadcast = address_of(255, 255, 255, 255, 2302), from;
	int receiver = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	int sender = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	int other = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	int length = sizeof(from);
	char buffer[64];

	assert(receiver >= 0 && sender >= 0 && other >= 0);
	assert(posix_socket_bind(receiver, &any, sizeof(any)) == 0);
	/* (a port in use) */
	assert(posix_socket_bind(other, &any, sizeof(any)) == -1 && posix_socket_last_error() == WSAEADDRINUSE);
	posix_socket_set_nonblocking(receiver, 1);
	assert(posix_socket_recv(receiver, buffer, sizeof(buffer), 0) == -1 &&
		posix_socket_last_error() == WSAEWOULDBLOCK);

	assert(posix_socket_sendto(sender, "hello", 5, 0, &loopback, sizeof(loopback)) == 5);
	assert(posix_socket_recvfrom(receiver, buffer, sizeof(buffer), 0, &from, &length) == 5);
	assert(!memcmp(buffer, "hello", 5) && length == (int)sizeof(from) && from.ip == loopback.ip && from.port);

	/* a broadcast, and the machine's own address */
	assert(posix_socket_sendto(sender, "all", 3, 0, &broadcast, sizeof(broadcast)) == 3);
	assert(posix_socket_recv(receiver, buffer, sizeof(buffer), 0) == 3 && !memcmp(buffer, "all", 3));
	{
		struct address own = address_of(0, 0, 0, 0, 2302);

		own.ip = posix_local_ipv4_address();
		assert(posix_socket_sendto(sender, "own", 3, 0, &own, sizeof(own)) == 3);
		assert(posix_socket_recv(receiver, buffer, sizeof(buffer), 0) == 3);
	}

	/* another machine's: not here (the page carries it to the room's
	browsers, or loses it) */
	{
		struct address far = address_of(10, 9, 9, 9, 2302);
		posix_ulong available = 99;

		assert(posix_socket_sendto(sender, "far", 3, 0, &far, sizeof(far)) == 3);
		assert(posix_socket_bytes_available(receiver, &available) == 0 && available == 0);
	}

	/* a datagram larger than the buffer: its start, and WSAEMSGSIZE */
	assert(posix_socket_sendto(sender, "0123456789", 10, 0, &loopback, sizeof(loopback)) == 10);
	assert(posix_socket_recv(receiver, buffer, 4, 0) == -1 && posix_socket_last_error() == WSAEMSGSIZE);

	posix_socket_close(receiver);
	posix_socket_close(sender);
	posix_socket_close(other);
}

static void *connect_later(void *context)
{
	struct address *to = context;
	int client = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);

	assert(posix_socket_connect(client, to, sizeof(*to)) == 0);
	assert(posix_socket_send(client, "stream", 6, 0) == 6);
	posix_socket_shutdown(client, 1);
	return (void *)(long)client;
}

static void test_streams(void)
{
	struct address port = address_of(0, 0, 0, 0, 2303), loopback = address_of(127, 0, 0, 1, 2303);
	struct address nobody = address_of(127, 0, 0, 1, 2304);
	int listener = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	int lonely = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	int accepted, client, read_list[1], read_count = 1;
	pthread_t thread;
	void *result;
	char buffer[64];
	int got = 0, count;

	assert(posix_socket_bind(listener, &port, sizeof(port)) == 0);
	assert(posix_socket_listen(listener, 4) == 0);
	assert(posix_socket_connect(lonely, &nobody, sizeof(nobody)) == -1 && posix_socket_last_error() == WSAECONNREFUSED);

	/* a blocking accept, woken by a connection from another thread */
	pthread_create(&thread, NULL, connect_later, &loopback);
	accepted = posix_socket_accept(listener, NULL, NULL);
	assert(accepted >= 0);
	pthread_join(thread, &result);
	client = (int)(long)result;

	/* select sees the bytes; the end follows them */
	read_list[0] = accepted;
	assert(posix_socket_select(read_list, &read_count, NULL, NULL, NULL, NULL, 1, 0, 0) == 1 && read_count == 1);
	while ((count = posix_socket_recv(accepted, buffer + got, sizeof(buffer) - got, 0)) > 0)
		got += count;
	assert(count == 0 && got == 6 && !memcmp(buffer, "stream", 6));

	/* nothing to read on the listener: select times out */
	read_list[0] = listener;
	read_count = 1;
	assert(posix_socket_select(read_list, &read_count, NULL, NULL, NULL, NULL, 0, 1000, 0) == 0 && read_count == 0);

	/* the other end closed: a read ends, a write fails */
	posix_socket_close(accepted);
	assert(posix_socket_recv(client, buffer, sizeof(buffer), 0) == 0);
	assert(posix_socket_send(client, "x", 1, 0) == -1);
	posix_socket_close(client);
	posix_socket_close(listener);
	posix_socket_close(lonely);
}

/* (in network byte order) */
static unsigned int ip_of(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
	return a | (b << 8) | (c << 16) | (d << 24);
}

static void test_room_datagrams(void)
{
	unsigned int other = ip_of(10, 9, 9, 9);
	struct address to_other = address_of(10, 9, 9, 9, 2302), any = address_of(0, 0, 0, 0, 2302);
	struct address broadcast = address_of(255, 255, 255, 255, 2302), from;
	int socket = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	int length = sizeof(from);
	unsigned char frame[64];
	char buffer[64];

	/* this machine is the room's address the page gave it */
	assert(posix_local_ipv4_address() == ip_of(10, 1, 2, 3));
	assert(posix_socket_bind(socket, &any, sizeof(any)) == 0);
	posix_socket_set_nonblocking(socket, 1);

	/* to another browser: a frame on the channel that may lose it */
	sent_clear();
	assert(posix_socket_sendto(socket, "hi", 2, 0, &to_other, sizeof(to_other)) == 2);
	assert(sent_count == 1 && sent[0].address == other && !sent[0].reliable && sent[0].size == 9 + 2);
	assert(sent[0].frame[0] == 1 && !memcmp(sent[0].frame + 1, &other, 4) && !memcmp(sent[0].frame + 9, "hi", 2));
	/* a broadcast: to every browser, and here */
	sent_clear();
	assert(posix_socket_sendto(socket, "all", 3, 0, &broadcast, sizeof(broadcast)) == 3);
	assert(sent_count == 1 && sent[0].address == 0xffffffffu);
	assert(posix_socket_recv(socket, buffer, sizeof(buffer), 0) == 3);
	sent_clear();

	/* from another browser: as from its address and port */
	frame[0] = 1;
	memcpy(frame + 1, &(unsigned int){ ip_of(10, 1, 2, 3) }, 4);
	frame[5] = 0x12;
	frame[6] = 0x34;
	memcpy(frame + 7, &any.port, 2);
	memcpy(frame + 9, "back", 4);
	web_net_receive(other, 0, frame, 13);
	assert(posix_socket_recvfrom(socket, buffer, sizeof(buffer), 0, &from, &length) == 4);
	assert(!memcmp(buffer, "back", 4) && from.ip == other && from.port == 0x3412);

	/* not for this machine, too short, from no browser's address, or
	nothing: dropped */
	memcpy(frame + 1, &(unsigned int){ ip_of(10, 7, 7, 7) }, 4);
	web_net_receive(other, 0, frame, 13);
	web_net_receive(other, 0, frame, 5);
	web_net_receive(ip_of(127, 0, 0, 1), 0, frame, 13);
	web_net_receive(other, 0, NULL, 0);
	assert(posix_socket_recv(socket, buffer, sizeof(buffer), 0) == -1 &&
		posix_socket_last_error() == WSAEWOULDBLOCK);
	posix_socket_close(socket);
}

static void test_room_streams(void)
{
	unsigned int other = ip_of(10, 9, 9, 9);
	struct address to_other = address_of(10, 9, 9, 9, 2400), port = address_of(0, 0, 0, 0, 2401);
	int client = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	int listener = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	unsigned char frame[64], connection[4];
	char buffer[64];
	int accepted;

	/* a stream this machine opens: its opening, its bytes, its end */
	sent_clear();
	assert(posix_socket_connect(client, &to_other, sizeof(to_other)) == 0);
	assert(sent_count == 1 && sent[0].address == other && sent[0].reliable && sent[0].size == 9);
	assert(sent[0].frame[0] == (2 | 0x80));
	memcpy(connection, sent[0].frame + 1, 4);
	sent_clear();
	assert(posix_socket_send(client, "ping", 4, 0) == 4);
	assert(sent_count == 1 && sent[0].frame[0] == (3 | 0x80) && !memcmp(sent[0].frame + 5, "ping", 4));
	sent_clear();
	/* the other end's bytes (it did not open it), then its end */
	frame[0] = 3;
	memcpy(frame + 1, connection, 4);
	memcpy(frame + 5, "pong", 4);
	web_net_receive(other, 1, frame, 9);
	/* (the same on the channel that may lose it is not taken) */
	web_net_receive(other, 0, frame, 9);
	assert(posix_socket_recv(client, buffer, sizeof(buffer), 0) == 4 && !memcmp(buffer, "pong", 4));
	frame[0] = 4;
	web_net_receive(other, 1, frame, 5);
	assert(posix_socket_recv(client, buffer, sizeof(buffer), 0) == 0);
	assert(posix_socket_send(client, "x", 1, 0) == -1);
	posix_socket_close(client);
	sent_clear();

	/* a stream another browser opens to a port listening here */
	assert(posix_socket_bind(listener, &port, sizeof(port)) == 0 && posix_socket_listen(listener, 2) == 0);
	posix_socket_set_nonblocking(listener, 1);
	frame[0] = 2 | 0x80;
	frame[1] = 0;
	frame[2] = 0;
	frame[3] = 0;
	frame[4] = 7;
	frame[5] = 0x50;
	frame[6] = 0x00;
	memcpy(frame + 7, &port.port, 2);
	web_net_receive(other, 1, frame, 9);
	accepted = posix_socket_accept(listener, NULL, NULL);
	assert(accepted >= 0);
	frame[0] = 3 | 0x80;
	memcpy(frame + 5, "join", 4);
	web_net_receive(other, 1, frame, 9);
	assert(posix_socket_recv(accepted, buffer, sizeof(buffer), 0) == 4 && !memcmp(buffer, "join", 4));
	assert(posix_socket_send(accepted, "ok", 2, 0) == 2);
	assert(sent_count == 1 && sent[0].frame[0] == 3 && sent[0].frame[4] == 7);
	sent_clear();
	/* the other browser leaves: the stream ends */
	web_net_peer_lost(other);
	assert(posix_socket_recv(accepted, buffer, sizeof(buffer), 0) == 0);
	posix_socket_close(accepted);

	/* an opening to a port nobody listens on: its end goes back at once */
	frame[0] = 2 | 0x80;
	frame[4] = 8;
	frame[7] = 0x99;
	frame[8] = 0x99;
	web_net_receive(other, 1, frame, 9);
	assert(sent_count == 1 && sent[0].frame[0] == 4 && sent[0].frame[4] == 8);
	sent_clear();
	/* a stream's frame of no stream, or of an unknown type: nothing */
	frame[0] = 3;
	frame[4] = 99;
	web_net_receive(other, 1, frame, 9);
	frame[0] = 9;
	web_net_receive(other, 1, frame, 9);
	assert(sent_count == 0);
	posix_socket_close(listener);
	sent_clear();
}

static void test_internet_play(void)
{
	unsigned int first = posix_resolve_ipv4("wss://broker.example:8084/mqtt");
	unsigned int second = posix_resolve_ipv4("ws://127.0.0.1:18831");
	struct address to, from, bound;
	unsigned char buffer[16];
	char argument[64];
	char *arguments[] = { "halo", "--HALO_X=1", "halo://join/00" };
	int read[1], write[1], read_count, write_count, error_count = 0;
	int length = sizeof(from);
	int socket, datagram;

	/* a broker's URL has a stand-in address of its own, the same each time */
	assert((first & 0xffffff) == (198u | 19u << 8) && first >> 24 == 1);
	assert(second >> 24 == 2 && posix_resolve_ipv4("wss://broker.example:8084/mqtt") == first);
	assert(posix_resolve_ipv4("10.0.0.7") == (10u | 7u << 24) && posix_resolve_ipv4("example.com") == 0);

	/* connecting opens the page's WebSocket; writeable once it opened */
	socket = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	posix_socket_set_nonblocking(socket, 1);
	to = address_of(198, 19, 0, 1, 8084);
	assert(posix_socket_connect(socket, &to, sizeof(to)) == -1 && posix_socket_last_error() == WSAEWOULDBLOCK);
	assert(websocket.opened == 1 && !strcmp(websocket.url, "wss://broker.example:8084/mqtt"));
	write[0] = socket;
	write_count = 1;
	read_count = 0;
	assert(posix_socket_select(read, &read_count, write, &write_count, NULL, &error_count, 0, 0, 0) == 0);
	assert(posix_socket_send(socket, "x", 1, 0) == -1 && posix_socket_last_error() == WSAEWOULDBLOCK);
	web_net_websocket_opened(websocket.index, websocket.connection);
	write_count = 1;
	assert(posix_socket_select(read, &read_count, write, &write_count, NULL, &error_count, 0, 0, 0) == 1);
	assert(posix_socket_send(socket, "\x10\x02", 2, 0) == 2 && websocket.size == 2 && websocket.bytes[0] == 0x10);
	/* what arrives is read as a stream's bytes; another connection's, never */
	web_net_websocket_data(websocket.index, websocket.connection + 1, (const unsigned char *)"no", 2);
	web_net_websocket_data(websocket.index, websocket.connection, (const unsigned char *)"\x20\x02\x00\x00", 4);
	assert(posix_socket_recv(socket, buffer, sizeof(buffer), 0) == 4 && buffer[0] == 0x20);
	assert(posix_socket_recv(socket, buffer, sizeof(buffer), 0) == -1 && posix_socket_last_error() == WSAEWOULDBLOCK);
	/* its end: read as the stream's */
	web_net_websocket_closed(websocket.index, websocket.connection);
	assert(posix_socket_recv(socket, buffer, sizeof(buffer), 0) == 0);
	posix_socket_close(socket);
	assert(websocket.closed == 1);
	/* a URL never resolved is no broker's */
	socket = posix_socket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
	posix_socket_set_nonblocking(socket, 1);
	to = address_of(198, 19, 0, 9, 1);
	assert(posix_socket_connect(socket, &to, sizeof(to)) == -1 && posix_socket_last_error() != WSAEWOULDBLOCK);
	posix_socket_close(socket);

	/* a tunnel packet from a WebRTC connection: to the socket of its port,
	from the connection's stand-in address */
	datagram = posix_socket(AF_INET_VALUE, SOCK_DGRAM_VALUE, 0);
	bound = address_of(0, 0, 0, 0, 0);
	assert(posix_socket_bind(datagram, &bound, sizeof(bound)) == 0);
	assert(posix_socket_getsockname(datagram, &bound, &length) == 0);
	web_net_inject(198u | 18u << 8 | 3u << 24, 0x8813, bound.port, "tunnel", 6);
	length = sizeof(from);
	assert(posix_socket_recvfrom(datagram, buffer, sizeof(buffer), 0, &from, &length) == 6);
	assert(!memcmp(buffer, "tunnel", 6) && from.ip == (198u | 18u << 8 | 3u << 24) && from.port == 0x8813);
	web_net_inject(1, 1, bound.port, "x", 0);
	posix_socket_set_nonblocking(datagram, 1);
	assert(posix_socket_recvfrom(datagram, buffer, sizeof(buffer), 0, &from, &length) == -1);
	posix_socket_close(datagram);

	/* the page's arguments (an invite link among them) */
	web_net_arguments(3, arguments);
	assert(posix_command_line_argument(2, argument, sizeof(argument)) && !strcmp(argument, "halo://join/00"));
	assert(!posix_command_line_argument(3, argument, sizeof(argument)) && !argument[0]);
}

int main(void)
{
	/* (the address the page gives the machine in a room, read once) */
	setenv("HALO_WEB_ADDRESS", "10.1.2.3", 1);
	test_datagrams();
	test_streams();
	sent_clear();
	test_room_datagrams();
	test_room_streams();
	test_internet_play();
	printf("web_net: ok\n");
	return 0;
}
