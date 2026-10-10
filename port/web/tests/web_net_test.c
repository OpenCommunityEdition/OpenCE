/*
WEB_NET_TEST.C

port/web/src/web_net.c, the web build's sockets inside the page, compiled
for this computer and tried as the game uses them (tools/test_web_build.py
builds and runs it): datagrams to loopback, to the machine's own address and
by broadcast; a stream connected, accepted, written and read, and its end;
select; and the errors Winsock gives.
*/

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
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

	/* another machine's: lost, as on an empty network */
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

int main(void)
{
	test_datagrams();
	test_streams();
	printf("web_net: ok\n");
	return 0;
}
