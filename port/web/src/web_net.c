/*
WEB_NET.C

The sockets of port/linux/src/posix.h for the web build: a network inside
the page.

Browsers have no UDP or plain TCP, but the game opens sockets even when it
plays alone: a split screen game is a network game whose host and clients
are the same machine, connected through Winsock
(transport_endpoint_winsock.c), and system link looks for games by
broadcast. Here every socket belongs to one machine with two addresses,
loopback and its address on a local network of its own (WEB_LOCAL_ADDRESS).
A datagram sent to either, or to a broadcast address, goes to the sockets
bound to its port; a stream socket connects to the socket listening on its
port, and the two exchange bytes through each other's queue. Anything sent
to another machine is lost, as on a network with nobody else on it.

As with Winsock, each call returns -1 on failure with the error code in
posix_socket_last_error(); blocking calls wait on one condition, which every
change signals.

The rest of posix.h's network half, which a browser has no use for, answers
"none" here: the command line, the desktop's link handler, Discord.
*/

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "posix.h"

/* Winsock error codes (winerror.h) */
#define WSAEBADF 10009
#define WSAEFAULT 10014
#define WSAEINVAL 10022
#define WSAEMFILE 10024
#define WSAEWOULDBLOCK 10035
#define WSAENOTSOCK 10038
#define WSAEMSGSIZE 10040
#define WSAENOPROTOOPT 10042
#define WSAEPROTONOSUPPORT 10043
#define WSAEOPNOTSUPP 10045
#define WSAEAFNOSUPPORT 10047
#define WSAEADDRINUSE 10048
#define WSAENETUNREACH 10051
#define WSAECONNRESET 10054
#define WSAENOBUFS 10055
#define WSAEISCONN 10056
#define WSAENOTCONN 10057
#define WSAESHUTDOWN 10058
#define WSAECONNREFUSED 10061

/* the Winsock and BSD values the game passes */
#define FAMILY_INET 2
#define TYPE_STREAM 1
#define TYPE_DATAGRAM 2
#define WINSOCK_SOL_SOCKET 0xffff
#define WINSOCK_SO_ERROR 0x1007
#define WINSOCK_SO_TYPE 0x1008
#define WINSOCK_SO_SNDBUF 0x1001
#define WINSOCK_SO_RCVBUF 0x1002
#define MESSAGE_PEEK 0x2

/* in network byte order: 127.0.0.1, and 10.0.0.1, the machine's address on
its own network */
#define LOOPBACK_ADDRESS 0x0100007fUL
#define WEB_LOCAL_ADDRESS 0x0100000aUL
#define WEB_LOCAL_BROADCAST 0xff00000aUL
#define LIMITED_BROADCAST 0xffffffffUL

/* descriptors apart from the file system's */
#define SOCKET_BASE 0x4000
#define MAXIMUM_SOCKETS 256
#define MAXIMUM_QUEUED_DATAGRAMS 256
#define MAXIMUM_DATAGRAM 65507
#define STREAM_CAPACITY (512 * 1024)
#define MAXIMUM_BACKLOG 16
#define FIRST_EPHEMERAL_PORT 49152

/* sockaddr_in, as Winsock and BSD share it */
struct address
{
	unsigned short family;
	unsigned short port; /* network byte order */
	unsigned int ip; /* network byte order */
	unsigned char zero[8];
};

struct datagram
{
	struct datagram *next;
	struct address from;
	int length;
	unsigned char data[];
};

enum
{
	_socket_free = 0,
	_socket_open,
	_socket_listening,
	_socket_connected,
};

struct web_socket
{
	int state;
	int type;
	int nonblocking;
	int bound;
	struct address local;
	struct address remote;
	/* datagrams: a queue */
	struct datagram *first, *last;
	int datagram_count;
	/* streams: the other end (an index, or -1 once it closed), the bytes
	received, and whether either side shut down */
	int peer;
	unsigned char *bytes;
	unsigned int read_position, byte_count;
	int receive_shut, send_shut, reset;
	/* listening: the connections waiting for accept, as indices */
	int pending[MAXIMUM_BACKLOG];
	int pending_count, backlog;
};

static struct web_socket sockets[MAXIMUM_SOCKETS];
static pthread_mutex_t net_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t net_changed = PTHREAD_COND_INITIALIZER;
static __thread int last_error;
static unsigned short next_ephemeral_port = FIRST_EPHEMERAL_PORT;

static unsigned short swap16(unsigned short value)
{
	return (unsigned short)((value >> 8) | (value << 8));
}

static int fail(int error)
{
	last_error = error;
	return -1;
}

static int succeed(int result)
{
	last_error = 0;
	return result;
}

/* (under net_lock) the open socket of a descriptor, or NULL */
static struct web_socket *socket_get(int descriptor)
{
	int index = descriptor - SOCKET_BASE;

	if (index < 0 || index >= MAXIMUM_SOCKETS || sockets[index].state == _socket_free)
		return NULL;
	return &sockets[index];
}

static int socket_index(const struct web_socket *socket)
{
	return (int)(socket - sockets);
}

static int is_local(unsigned int ip)
{
	return ip == 0 || ip == LOOPBACK_ADDRESS || ip == WEB_LOCAL_ADDRESS;
}

static int is_broadcast(unsigned int ip)
{
	return ip == LIMITED_BROADCAST || ip == WEB_LOCAL_BROADCAST;
}

static int port_in_use(int type, unsigned short port)
{
	int index;

	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		if (sockets[index].state != _socket_free && sockets[index].type == type && sockets[index].bound &&
			sockets[index].local.port == port)
		{
			return 1;
		}
	}
	return 0;
}

/* a free port (network byte order) */
static unsigned short ephemeral_port(int type)
{
	int tries;

	for (tries = 0; tries < 65536 - FIRST_EPHEMERAL_PORT; tries++)
	{
		unsigned short port = swap16(next_ephemeral_port);

		next_ephemeral_port = next_ephemeral_port == 65535 ? FIRST_EPHEMERAL_PORT : next_ephemeral_port + 1;
		if (!port_in_use(type, port))
			return port;
	}
	return 0;
}

static void bind_implicitly(struct web_socket *socket)
{
	if (socket->bound)
		return;
	memset(&socket->local, 0, sizeof(socket->local));
	socket->local.family = FAMILY_INET;
	socket->local.port = ephemeral_port(socket->type);
	socket->bound = 1;
}

static void datagrams_free(struct web_socket *socket)
{
	while (socket->first)
	{
		struct datagram *next = socket->first->next;

		free(socket->first);
		socket->first = next;
	}
	socket->last = NULL;
	socket->datagram_count = 0;
}

/* (under net_lock) frees a socket; its stream's other end sees it go */
static void socket_release(int index)
{
	struct web_socket *socket = &sockets[index];
	int pending;

	datagrams_free(socket);
	if (socket->state == _socket_connected && socket->peer >= 0)
		sockets[socket->peer].peer = -1;
	for (pending = 0; pending < socket->pending_count; pending++)
		socket_release(socket->pending[pending]);
	free(socket->bytes);
	memset(socket, 0, sizeof(*socket));
}

/* ---------- sockets */

int posix_socket_last_error(void)
{
	return last_error;
}

int posix_socket(int family, int type, int protocol)
{
	int index;

	(void)protocol;
	if (family != FAMILY_INET)
		return fail(WSAEAFNOSUPPORT);
	if (type != TYPE_STREAM && type != TYPE_DATAGRAM)
		return fail(WSAEPROTONOSUPPORT);
	pthread_mutex_lock(&net_lock);
	for (index = 0; index < MAXIMUM_SOCKETS && sockets[index].state != _socket_free; index++)
	{
	}
	if (index == MAXIMUM_SOCKETS)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAEMFILE);
	}
	memset(&sockets[index], 0, sizeof(sockets[index]));
	sockets[index].state = _socket_open;
	sockets[index].type = type;
	sockets[index].peer = -1;
	pthread_mutex_unlock(&net_lock);
	return succeed(SOCKET_BASE + index);
}

int posix_socket_close(int descriptor)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAENOTSOCK);
	}
	socket_release(socket_index(socket));
	pthread_cond_broadcast(&net_changed);
	pthread_mutex_unlock(&net_lock);
	return succeed(0);
}

int posix_socket_bind(int descriptor, const void *address, int address_length)
{
	struct web_socket *socket;
	struct address wanted;

	if (!address || address_length < (int)sizeof(wanted) - 8)
		return fail(WSAEFAULT);
	memset(&wanted, 0, sizeof(wanted));
	memcpy(&wanted, address, address_length < (int)sizeof(wanted) ? (size_t)address_length : sizeof(wanted));
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAENOTSOCK);
	}
	if (socket->bound)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAEINVAL);
	}
	if (!is_local(wanted.ip))
	{
		pthread_mutex_unlock(&net_lock);
		return fail(10049); /* WSAEADDRNOTAVAIL */
	}
	if (!wanted.port)
		wanted.port = ephemeral_port(socket->type);
	else if (port_in_use(socket->type, wanted.port))
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAEADDRINUSE);
	}
	wanted.family = FAMILY_INET;
	socket->local = wanted;
	socket->bound = 1;
	pthread_mutex_unlock(&net_lock);
	return succeed(0);
}

int posix_socket_listen(int descriptor, int backlog)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket || socket->type != TYPE_STREAM || socket->state == _socket_connected)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(socket ? WSAEINVAL : WSAENOTSOCK);
	}
	bind_implicitly(socket);
	socket->state = _socket_listening;
	socket->backlog = backlog < 1 ? 1 : backlog > MAXIMUM_BACKLOG ? MAXIMUM_BACKLOG : backlog;
	pthread_mutex_unlock(&net_lock);
	return succeed(0);
}

/* (under net_lock) the socket listening on a port, or -1 */
static int find_listener(unsigned short port)
{
	int index;

	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		if (sockets[index].state == _socket_listening && sockets[index].local.port == port)
			return index;
	}
	return -1;
}

/* (under net_lock) a new connected socket, or -1 */
static int new_stream(void)
{
	int index;

	for (index = 0; index < MAXIMUM_SOCKETS && sockets[index].state != _socket_free; index++)
	{
	}
	if (index == MAXIMUM_SOCKETS)
		return -1;
	memset(&sockets[index], 0, sizeof(sockets[index]));
	sockets[index].bytes = malloc(STREAM_CAPACITY);
	if (!sockets[index].bytes)
		return -1;
	sockets[index].state = _socket_connected;
	sockets[index].type = TYPE_STREAM;
	sockets[index].bound = 1;
	sockets[index].peer = -1;
	return index;
}

int posix_socket_connect(int descriptor, const void *address, int address_length)
{
	struct web_socket *socket;
	struct address to;
	int listener, accepted;

	if (!address || address_length < (int)sizeof(to) - 8)
		return fail(WSAEFAULT);
	memset(&to, 0, sizeof(to));
	memcpy(&to, address, address_length < (int)sizeof(to) ? (size_t)address_length : sizeof(to));
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAENOTSOCK);
	}
	bind_implicitly(socket);
	if (socket->type == TYPE_DATAGRAM)
	{
		/* a datagram socket's default destination */
		socket->remote = to;
		pthread_mutex_unlock(&net_lock);
		return succeed(0);
	}
	if (socket->state == _socket_connected)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAEISCONN);
	}
	listener = is_local(to.ip) ? find_listener(to.port) : -1;
	if (listener < 0)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(is_local(to.ip) ? WSAECONNREFUSED : WSAENETUNREACH);
	}
	if (sockets[listener].pending_count >= sockets[listener].backlog ||
		!(socket->bytes = malloc(STREAM_CAPACITY)) || (accepted = new_stream()) < 0)
	{
		free(socket->bytes);
		socket->bytes = NULL;
		pthread_mutex_unlock(&net_lock);
		return fail(WSAECONNREFUSED);
	}
	/* the two ends, each the other's peer */
	socket->state = _socket_connected;
	socket->remote = to;
	socket->remote.family = FAMILY_INET;
	if (!socket->remote.ip)
		socket->remote.ip = LOOPBACK_ADDRESS;
	socket->peer = accepted;
	if (!socket->local.ip)
		socket->local.ip = socket->remote.ip;
	sockets[accepted].local = socket->remote;
	sockets[accepted].remote = socket->local;
	sockets[accepted].peer = socket_index(socket);
	sockets[listener].pending[sockets[listener].pending_count++] = accepted;
	pthread_cond_broadcast(&net_changed);
	pthread_mutex_unlock(&net_lock);
	return succeed(0);
}

/* (under net_lock) waits for any change, up to the deadline (NULL: none);
0 once it has passed */
static int wait_changed(const struct timespec *deadline)
{
	if (!deadline)
	{
		pthread_cond_wait(&net_changed, &net_lock);
		return 1;
	}
	return pthread_cond_timedwait(&net_changed, &net_lock, deadline) == 0;
}

static void copy_address(const struct address *source, void *address, int *address_length)
{
	if (address && address_length && *address_length > 0)
	{
		memcpy(address, source, *address_length < (int)sizeof(*source) ? (size_t)*address_length : sizeof(*source));
		*address_length = (int)sizeof(*source);
	}
}

int posix_socket_accept(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	int accepted;

	pthread_mutex_lock(&net_lock);
	for (;;)
	{
		socket = socket_get(descriptor);
		if (!socket || socket->state != _socket_listening)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(socket ? WSAEINVAL : WSAENOTSOCK);
		}
		if (socket->pending_count)
			break;
		if (socket->nonblocking)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAEWOULDBLOCK);
		}
		wait_changed(NULL);
	}
	accepted = socket->pending[0];
	memmove(socket->pending, socket->pending + 1, sizeof(socket->pending[0]) * (size_t)(socket->pending_count - 1));
	socket->pending_count--;
	copy_address(&sockets[accepted].remote, address, address_length);
	pthread_mutex_unlock(&net_lock);
	return succeed(SOCKET_BASE + accepted);
}

/* (under net_lock) a datagram to the sockets bound to its port at its
address (all of them for a broadcast); returns 0, or a Winsock error */
static int deliver_datagram(const struct web_socket *from, const void *buffer, int length, const struct address *to)
{
	int index;

	if (length < 0 || length > MAXIMUM_DATAGRAM)
		return WSAEMSGSIZE;
	/* (another machine's: lost, as on a network nobody else is on) */
	if (!is_local(to->ip) && !is_broadcast(to->ip))
		return 0;
	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		struct web_socket *socket = &sockets[index];
		struct datagram *datagram;

		if (socket->state == _socket_free || socket->type != TYPE_DATAGRAM || !socket->bound ||
			socket->local.port != to->port)
		{
			continue;
		}
		/* bound to an address of its own: only what is sent to it (or
		broadcast) */
		if (socket->local.ip && !is_broadcast(to->ip) && to->ip && socket->local.ip != to->ip)
			continue;
		/* a full queue drops it, as a full receive buffer does */
		if (socket->datagram_count >= MAXIMUM_QUEUED_DATAGRAMS)
			continue;
		datagram = malloc(sizeof(*datagram) + (size_t)length);
		if (!datagram)
			return WSAENOBUFS;
		datagram->next = NULL;
		memset(&datagram->from, 0, sizeof(datagram->from));
		datagram->from.family = FAMILY_INET;
		datagram->from.port = from->local.port;
		datagram->from.ip = from->local.ip ? from->local.ip :
			to->ip == LOOPBACK_ADDRESS ? LOOPBACK_ADDRESS : WEB_LOCAL_ADDRESS;
		datagram->length = length;
		memcpy(datagram->data, buffer, (size_t)length);
		if (socket->last)
			socket->last->next = datagram;
		else
			socket->first = datagram;
		socket->last = datagram;
		socket->datagram_count++;
	}
	pthread_cond_broadcast(&net_changed);
	return 0;
}

/* (under net_lock) bytes onto the other end's queue; the count sent, or -1
with the error */
static int stream_send(struct web_socket *socket, const unsigned char *buffer, int length)
{
	struct web_socket *peer;
	unsigned int room, sent = 0;

	for (;;)
	{
		if (socket->send_shut)
			return fail(WSAESHUTDOWN);
		if (socket->peer < 0 || socket->reset)
			return fail(WSAECONNRESET);
		peer = &sockets[socket->peer];
		if (peer->receive_shut)
			return fail(WSAECONNRESET);
		room = STREAM_CAPACITY - peer->byte_count;
		if (room)
			break;
		if (socket->nonblocking)
			return fail(WSAEWOULDBLOCK);
		wait_changed(NULL);
	}
	while (sent < (unsigned int)length && peer->byte_count < STREAM_CAPACITY)
	{
		unsigned int position = (peer->read_position + peer->byte_count) % STREAM_CAPACITY;
		unsigned int chunk = STREAM_CAPACITY - position;

		if (chunk > STREAM_CAPACITY - peer->byte_count)
			chunk = STREAM_CAPACITY - peer->byte_count;
		if (chunk > (unsigned int)length - sent)
			chunk = (unsigned int)length - sent;
		memcpy(peer->bytes + position, buffer + sent, chunk);
		peer->byte_count += chunk;
		sent += chunk;
	}
	pthread_cond_broadcast(&net_changed);
	return succeed((int)sent);
}

int posix_socket_send(int descriptor, const void *buffer, int length, int flags)
{
	struct web_socket *socket;
	int result;

	(void)flags;
	if (length < 0 || (length && !buffer))
		return fail(WSAEFAULT);
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
		result = fail(WSAENOTSOCK);
	else if (socket->type == TYPE_STREAM)
		result = socket->state == _socket_connected ? stream_send(socket, buffer, length) : fail(WSAENOTCONN);
	else if (!socket->remote.port)
		result = fail(WSAENOTCONN);
	else
	{
		int error = deliver_datagram(socket, buffer, length, &socket->remote);

		result = error ? fail(error) : succeed(length);
	}
	pthread_mutex_unlock(&net_lock);
	return result;
}

int posix_socket_sendto(int descriptor, const void *buffer, int length, int flags,
	const void *address, int address_length)
{
	struct web_socket *socket;
	struct address to;
	int result;

	if (!address || address_length < (int)sizeof(to) - 8)
		return posix_socket_send(descriptor, buffer, length, flags);
	if (length < 0 || (length && !buffer))
		return fail(WSAEFAULT);
	memset(&to, 0, sizeof(to));
	memcpy(&to, address, address_length < (int)sizeof(to) ? (size_t)address_length : sizeof(to));
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
		result = fail(WSAENOTSOCK);
	else if (socket->type == TYPE_STREAM)
		result = socket->state == _socket_connected ? stream_send(socket, buffer, length) : fail(WSAENOTCONN);
	else
	{
		int error;

		bind_implicitly(socket);
		error = deliver_datagram(socket, buffer, length, &to);
		result = error ? fail(error) : succeed(length);
	}
	pthread_mutex_unlock(&net_lock);
	return result;
}

/* (under net_lock) whether a read would not wait */
static int readable(const struct web_socket *socket)
{
	if (socket->type == TYPE_DATAGRAM)
		return socket->first != NULL;
	if (socket->state == _socket_listening)
		return socket->pending_count > 0;
	if (socket->state != _socket_connected)
		return 0;
	/* (data, or the end: the other end closed or shut down its sending) */
	return socket->byte_count > 0 || socket->peer < 0 || socket->receive_shut ||
		sockets[socket->peer].send_shut;
}

static int receive(int descriptor, void *buffer, int length, int flags, void *address, int *address_length)
{
	struct web_socket *socket;
	int peek = (flags & MESSAGE_PEEK) != 0;

	if (length < 0 || (length && !buffer))
		return fail(WSAEFAULT);
	pthread_mutex_lock(&net_lock);
	for (;;)
	{
		socket = socket_get(descriptor);
		if (!socket)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAENOTSOCK);
		}
		if (socket->type == TYPE_STREAM && socket->state != _socket_connected)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAENOTCONN);
		}
		if (readable(socket))
			break;
		if (socket->nonblocking)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAEWOULDBLOCK);
		}
		if (socket->type == TYPE_DATAGRAM)
			bind_implicitly(socket);
		wait_changed(NULL);
	}
	if (socket->type == TYPE_DATAGRAM)
	{
		struct datagram *datagram = socket->first;
		int copied = datagram->length < length ? datagram->length : length;
		int truncated = datagram->length > length;

		memcpy(buffer, datagram->data, (size_t)copied);
		copy_address(&datagram->from, address, address_length);
		if (!peek)
		{
			socket->first = datagram->next;
			if (!socket->first)
				socket->last = NULL;
			socket->datagram_count--;
			free(datagram);
		}
		pthread_mutex_unlock(&net_lock);
		/* (as Winsock: the datagram's start, and WSAEMSGSIZE) */
		return truncated ? fail(WSAEMSGSIZE) : succeed(copied);
	}
	else
	{
		unsigned int count = socket->byte_count < (unsigned int)length ? socket->byte_count : (unsigned int)length;
		unsigned int index;

		/* (nothing left and the other end gone, or either side shut down:
		the end of the stream, 0) */
		for (index = 0; index < count; index++)
			((unsigned char *)buffer)[index] = socket->bytes[(socket->read_position + index) % STREAM_CAPACITY];
		if (!peek)
		{
			socket->read_position = (socket->read_position + count) % STREAM_CAPACITY;
			socket->byte_count -= count;
			pthread_cond_broadcast(&net_changed);
		}
		copy_address(&socket->remote, address, address_length);
		pthread_mutex_unlock(&net_lock);
		return succeed((int)count);
	}
}

int posix_socket_recv(int descriptor, void *buffer, int length, int flags)
{
	return receive(descriptor, buffer, length, flags, NULL, NULL);
}

int posix_socket_recvfrom(int descriptor, void *buffer, int length, int flags,
	void *address, int *address_length)
{
	return receive(descriptor, buffer, length, flags, address, address_length);
}

int posix_socket_shutdown(int descriptor, int how)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (!socket)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAENOTSOCK);
	}
	if (socket->type == TYPE_STREAM && socket->state != _socket_connected)
	{
		pthread_mutex_unlock(&net_lock);
		return fail(WSAENOTCONN);
	}
	/* 0 receiving, 1 sending, 2 both (SD_RECEIVE, SD_SEND, SD_BOTH) */
	if (how == 0 || how == 2)
		socket->receive_shut = 1;
	if (how == 1 || how == 2)
		socket->send_shut = 1;
	pthread_cond_broadcast(&net_changed);
	pthread_mutex_unlock(&net_lock);
	return succeed(0);
}

int posix_socket_set_nonblocking(int descriptor, int nonblocking)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (socket)
		socket->nonblocking = nonblocking != 0;
	pthread_mutex_unlock(&net_lock);
	return socket ? succeed(0) : fail(WSAENOTSOCK);
}

int posix_socket_set_nodelay(int descriptor)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	pthread_mutex_unlock(&net_lock);
	return socket ? succeed(0) : fail(WSAENOTSOCK);
}

int posix_socket_bytes_available(int descriptor, posix_ulong *count)
{
	struct web_socket *socket;

	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (socket->type == TYPE_DATAGRAM)
			*count = socket->first ? (posix_ulong)socket->first->length : 0;
		else
			*count = socket->byte_count;
	}
	pthread_mutex_unlock(&net_lock);
	return socket ? succeed(0) : fail(WSAENOTSOCK);
}

int posix_socket_setsockopt(int descriptor, int level, int name, const void *value, int length)
{
	struct web_socket *socket;

	(void)level;
	(void)name;
	(void)value;
	(void)length;
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	pthread_mutex_unlock(&net_lock);
	/* (broadcasts, reuse, buffer sizes: nothing to set in the page) */
	return socket ? succeed(0) : fail(WSAENOTSOCK);
}

int posix_socket_getsockopt(int descriptor, int level, int name, void *value, int *length)
{
	struct web_socket *socket;
	int answer;

	if (!value || !length || *length < (int)sizeof(int))
		return fail(WSAEFAULT);
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	answer = socket ? socket->type : 0;
	pthread_mutex_unlock(&net_lock);
	if (!socket)
		return fail(WSAENOTSOCK);
	if (level != WINSOCK_SOL_SOCKET)
		return fail(WSAENOPROTOOPT);
	switch (name)
	{
	case WINSOCK_SO_ERROR: answer = 0; break;
	case WINSOCK_SO_TYPE: break;
	case WINSOCK_SO_SNDBUF:
	case WINSOCK_SO_RCVBUF: answer = STREAM_CAPACITY; break;
	default: return fail(WSAENOPROTOOPT);
	}
	memcpy(value, &answer, sizeof(answer));
	*length = (int)sizeof(answer);
	return succeed(0);
}

int posix_socket_getsockname(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	struct address local;

	if (!address || !address_length)
		return fail(WSAEFAULT);
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (!socket->bound)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAEINVAL);
		}
		local = socket->local;
	}
	pthread_mutex_unlock(&net_lock);
	if (!socket)
		return fail(WSAENOTSOCK);
	copy_address(&local, address, address_length);
	return succeed(0);
}

int posix_socket_getpeername(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	struct address remote;

	if (!address || !address_length)
		return fail(WSAEFAULT);
	pthread_mutex_lock(&net_lock);
	socket = socket_get(descriptor);
	if (socket)
		remote = socket->remote;
	pthread_mutex_unlock(&net_lock);
	if (!socket)
		return fail(WSAENOTSOCK);
	if (!remote.port)
		return fail(WSAENOTCONN);
	copy_address(&remote, address, address_length);
	return succeed(0);
}

/* (under net_lock) whether a write would not wait */
static int writeable(const struct web_socket *socket)
{
	if (socket->type == TYPE_DATAGRAM)
		return 1;
	if (socket->state != _socket_connected)
		return 0;
	return socket->peer < 0 || sockets[socket->peer].byte_count < STREAM_CAPACITY;
}

/* (under net_lock) keeps the ready descriptors of a list; how many; -1 if
one is not a socket */
static int keep_ready(int *descriptors, int *count, int (*ready)(const struct web_socket *), int keep)
{
	int index, kept = 0;

	if (!descriptors || !count)
		return 0;
	for (index = 0; index < *count; index++)
	{
		struct web_socket *socket = socket_get(descriptors[index]);

		if (!socket)
			return -1;
		if (ready && ready(socket))
		{
			if (keep)
				descriptors[kept] = descriptors[index];
			kept++;
		}
	}
	if (keep)
		*count = kept;
	return kept;
}

int posix_socket_select(int *read, int *read_count, int *write, int *write_count,
	int *error, int *error_count, posix_long timeout_seconds, posix_long timeout_microseconds, int infinite)
{
	struct timespec deadline;
	int result;

	if (!infinite)
	{
		clock_gettime(CLOCK_REALTIME, &deadline);
		deadline.tv_sec += timeout_seconds + timeout_microseconds / 1000000;
		deadline.tv_nsec += (long)(timeout_microseconds % 1000000) * 1000L;
		if (deadline.tv_nsec >= 1000000000L)
		{
			deadline.tv_sec++;
			deadline.tv_nsec -= 1000000000L;
		}
	}
	pthread_mutex_lock(&net_lock);
	for (;;)
	{
		int read_ready = keep_ready(read, read_count, readable, 0);
		int write_ready = keep_ready(write, write_count, writeable, 0);
		int error_ready = keep_ready(error, error_count, NULL, 0);

		if (read_ready < 0 || write_ready < 0 || error_ready < 0)
		{
			pthread_mutex_unlock(&net_lock);
			return fail(WSAENOTSOCK);
		}
		result = read_ready + write_ready;
		if (result || (!infinite && !wait_changed(&deadline)))
			break;
		if (infinite)
			wait_changed(NULL);
	}
	keep_ready(read, read_count, readable, 1);
	keep_ready(write, write_count, writeable, 1);
	keep_ready(error, error_count, NULL, 1);
	pthread_mutex_unlock(&net_lock);
	/* (as Winsock: nothing ready leaves the last error as it was) */
	if (result > 0)
		last_error = 0;
	return result;
}

/* ---------- addresses */

posix_ulong posix_local_ipv4_address(void)
{
	return WEB_LOCAL_ADDRESS;
}

posix_ulong posix_resolve_ipv4(const char *host)
{
	unsigned int parts[4];
	char extra;

	/* dotted quads only: a page cannot look names up */
	if (host && sscanf(host, "%u.%u.%u.%u%c", &parts[0], &parts[1], &parts[2], &parts[3], &extra) == 4 &&
		parts[0] < 256 && parts[1] < 256 && parts[2] < 256 && parts[3] < 256)
	{
		return (posix_ulong)(parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24));
	}
	return 0;
}

void posix_random_bytes(void *buffer, posix_ulong size)
{
	unsigned char *cursor = buffer;

	/* (the browser's crypto.getRandomValues, 256 bytes at a time) */
	while (size)
	{
		size_t chunk = size > 256 ? 256 : size;

		if (getentropy(cursor, chunk) != 0)
			abort();
		cursor += chunk;
		size -= chunk;
	}
}

/* ---------- the process and the desktop: none in a page */

int posix_command_line_argument(int index, char *buffer, posix_ulong size)
{
	(void)index;
	if (buffer && size)
		buffer[0] = 0;
	return 0;
}

posix_ulong posix_process_id(void)
{
	return 1;
}

int posix_register_url_scheme(const char *scheme, const char *description)
{
	(void)scheme;
	(void)description;
	return 0;
}

int posix_user_secret(unsigned char *secret, int size)
{
	(void)secret;
	(void)size;
	return 0;
}

int posix_discord_connect(void)
{
	return -1;
}

int posix_discord_write(int handle, const void *buffer, int length)
{
	(void)handle;
	(void)buffer;
	(void)length;
	return -1;
}

int posix_discord_read(int handle, void *buffer, int length)
{
	(void)handle;
	(void)buffer;
	(void)length;
	return -1;
}

void posix_discord_close(int handle)
{
	(void)handle;
}
