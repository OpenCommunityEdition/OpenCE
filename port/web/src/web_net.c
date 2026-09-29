/*
WEB_NET.C

The sockets of port/linux/src/posix.h for the web build: a network inside
the page.

Browsers have no UDP or plain TCP, but the game needs sockets even when it
plays alone: a split screen game is a network game whose host and clients
are the same machine, connected through Winsock (transport_endpoint_winsock.c).
Here every socket belongs to one machine with two addresses, loopback and
WEB_LOCAL_ADDRESS. Datagrams sent to either, or to a broadcast address, go
to the socket bound to their port; stream sockets connect to the listening
socket of their port and exchange bytes through queues.

As with Winsock, each call returns -1 on failure with the error code in
posix_socket_last_error().
*/

#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "posix.h"

/* Winsock error codes (winerror.h) */
#define WSAEBADF 10009
#define WSAEFAULT 10014
#define WSAEINVAL 10022
#define WSAEMFILE 10024
#define WSAEWOULDBLOCK 10035
#define WSAENOTSOCK 10038
#define WSAEMSGSIZE 10040
#define WSAEPROTONOSUPPORT 10043
#define WSAEOPNOTSUPP 10045
#define WSAEAFNOSUPPORT 10047
#define WSAEADDRINUSE 10048
#define WSAECONNRESET 10054
#define WSAENOBUFS 10055
#define WSAEISCONN 10056
#define WSAENOTCONN 10057
#define WSAECONNREFUSED 10061

#define AF_INET_VALUE 2
#define SOCK_STREAM_VALUE 1
#define SOCK_DGRAM_VALUE 2

/* 10.0.2.15, in network byte order: the machine's "LAN" address */
#define WEB_LOCAL_ADDRESS 0x0F02000AUL
#define LOOPBACK_ADDRESS 0x0100007FUL

#define SOCKET_BASE 1000 /* descriptors apart from the file system's */
#define MAXIMUM_SOCKETS 256
#define MAXIMUM_DATAGRAMS 512
#define MAXIMUM_DATAGRAM 65536
#define STREAM_CAPACITY (1024 * 1024)
#define MAXIMUM_BACKLOG 16
#define FIRST_EPHEMERAL_PORT 49152

/* sockaddr_in as Winsock and BSD share it */
struct address
{
	unsigned short family;
	unsigned short port;     /* network byte order */
	unsigned int ip;         /* network byte order */
	unsigned char zero[8];
};

struct datagram
{
	struct datagram *next;
	struct address from;
	int length;
	unsigned char data[];
};

struct web_socket
{
	int used;
	int type;
	int nonblocking;
	int bound, listening, connected, peer_closed, shut_down;
	struct address local, remote;
	/* datagrams */
	struct datagram *first, *last;
	int datagram_count;
	/* a stream's received bytes (a ring) */
	unsigned char *stream;
	int stream_head, stream_count;
	int peer;                   /* the connected socket's index, or -1 */
	/* a listener's connections waiting to be accepted */
	int backlog[MAXIMUM_BACKLOG];
	int backlog_count;
};

static struct web_socket sockets[MAXIMUM_SOCKETS];
static pthread_mutex_t network_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t network_changed = PTHREAD_COND_INITIALIZER;
static unsigned short next_ephemeral_port = FIRST_EPHEMERAL_PORT;
static __thread int last_error;

static unsigned short swap16(unsigned short value)
{
	return (unsigned short)((value >> 8) | (value << 8));
}

/* HALO_NET_DEBUG=1 logs the network's traffic */
static int trace_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0)
		enabled = getenv("HALO_NET_DEBUG") != NULL;
	return enabled;
}

#define TRACE(...) do { if (trace_enabled()) fprintf(stderr, "web_net: " __VA_ARGS__); } while (0)

static int fail(int error)
{
	last_error = error;
	if (error != WSAEWOULDBLOCK)
		TRACE("error %d\n", error);
	return -1;
}

static int succeed(int result)
{
	last_error = 0;
	return result;
}

/* the socket of a descriptor, with the lock held; NULL (and the error) if
there is none */
static struct web_socket *socket_get(int descriptor)
{
	int index = descriptor - SOCKET_BASE;

	if (index < 0 || index >= MAXIMUM_SOCKETS || !sockets[index].used)
	{
		last_error = WSAENOTSOCK;
		return NULL;
	}
	return &sockets[index];
}

static int is_local(unsigned int ip)
{
	unsigned char last = (unsigned char)(ip >> 24);

	return ip == 0 || ip == LOOPBACK_ADDRESS || ip == WEB_LOCAL_ADDRESS || ip == 0xFFFFFFFFu || last == 255 ||
		(ip & 0xFF) == 127;
}

static int port_in_use(int type, unsigned short port)
{
	int index;

	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		if (sockets[index].used && sockets[index].bound && sockets[index].type == type &&
			sockets[index].local.port == port && !(type == SOCK_STREAM_VALUE && sockets[index].connected &&
			!sockets[index].listening))
		{
			return 1;
		}
	}
	return 0;
}

static unsigned short ephemeral_port(int type)
{
	int tries;

	for (tries = 0; tries < 16384; tries++)
	{
		unsigned short port = swap16(next_ephemeral_port);

		next_ephemeral_port = next_ephemeral_port == 65535 ? FIRST_EPHEMERAL_PORT : next_ephemeral_port + 1;
		if (!port_in_use(type, port))
			return port;
	}
	return 0;
}

static int is_loopback(unsigned int ip)
{
	return (ip & 0xFF) == 127;
}

/* binds an unbound socket to any address and a free port */
static void bind_implicitly(struct web_socket *socket)
{
	if (!socket->bound)
	{
		memset(&socket->local, 0, sizeof(socket->local));
		socket->local.family = AF_INET_VALUE;
		socket->local.ip = 0;
		socket->local.port = ephemeral_port(socket->type);
		socket->bound = 1;
	}
}

/* the address a packet to destination comes from, as a host's routing
would choose it for a socket bound to any address */
static struct address source_address(const struct web_socket *socket, unsigned int destination)
{
	struct address source = socket->local;

	if (!source.ip)
		source.ip = is_loopback(destination) ? LOOPBACK_ADDRESS : WEB_LOCAL_ADDRESS;
	return source;
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

static void socket_release(int index)
{
	struct web_socket *socket = &sockets[index];
	int entry;

	datagrams_free(socket);
	free(socket->stream);
	if (socket->peer >= 0 && sockets[socket->peer].used && sockets[socket->peer].peer == index)
	{
		sockets[socket->peer].peer_closed = 1;
		sockets[socket->peer].peer = -1;
	}
	for (entry = 0; entry < socket->backlog_count; entry++)
		socket_release(socket->backlog[entry]);
	memset(socket, 0, sizeof(*socket));
}

/* ---------- posix.h */

int posix_socket_last_error(void)
{
	return last_error;
}

int posix_socket(int family, int type, int protocol)
{
	int index;

	(void)protocol;
	if (family != AF_INET_VALUE)
		return fail(WSAEAFNOSUPPORT);
	if (type != SOCK_STREAM_VALUE && type != SOCK_DGRAM_VALUE)
		return fail(WSAEPROTONOSUPPORT);
	pthread_mutex_lock(&network_lock);
	for (index = 0; index < MAXIMUM_SOCKETS && sockets[index].used; index++)
		;
	if (index == MAXIMUM_SOCKETS)
	{
		pthread_mutex_unlock(&network_lock);
		return fail(WSAEMFILE);
	}
	memset(&sockets[index], 0, sizeof(sockets[index]));
	sockets[index].used = 1;
	sockets[index].type = type;
	sockets[index].peer = -1;
	pthread_mutex_unlock(&network_lock);
	return succeed(SOCKET_BASE + index);
}

int posix_socket_close(int descriptor)
{
	struct web_socket *socket;

	TRACE("close %d\n", descriptor);
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
		socket_release(descriptor - SOCKET_BASE);
	pthread_cond_broadcast(&network_changed);
	pthread_mutex_unlock(&network_lock);
	return socket ? succeed(0) : -1;
}

int posix_socket_bind(int descriptor, const void *address, int address_length)
{
	struct web_socket *socket;
	struct address wanted;
	int result = -1;

	if (!address || address_length < (int)sizeof(struct address) - 8)
		return fail(WSAEFAULT);
	memcpy(&wanted, address, sizeof(wanted) - 8);
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (socket->bound)
			result = fail(WSAEINVAL);
		else if (wanted.port && port_in_use(socket->type, wanted.port))
			result = fail(WSAEADDRINUSE);
		else
		{
			memset(&socket->local, 0, sizeof(socket->local));
			socket->local.family = AF_INET_VALUE;
			socket->local.ip = wanted.ip;
			socket->local.port = wanted.port ? wanted.port : ephemeral_port(socket->type);
			socket->bound = 1;
			TRACE("bind %d (type %d) to %08x:%u\n", descriptor, socket->type, socket->local.ip, swap16(socket->local.port));
			result = succeed(0);
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

int posix_socket_listen(int descriptor, int backlog)
{
	struct web_socket *socket;
	int result = -1;

	(void)backlog;
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (socket->type != SOCK_STREAM_VALUE)
			result = fail(WSAEOPNOTSUPP);
		else
		{
			bind_implicitly(socket);
			socket->listening = 1;
			result = succeed(0);
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

static int find_listener(unsigned short port)
{
	int index;

	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		if (sockets[index].used && sockets[index].listening && sockets[index].local.port == port)
			return index;
	}
	return -1;
}

int posix_socket_connect(int descriptor, const void *address, int address_length)
{
	struct web_socket *socket;
	struct address target;
	int result = -1;

	if (!address || address_length < (int)sizeof(struct address) - 8)
		return fail(WSAEFAULT);
	memcpy(&target, address, sizeof(target) - 8);
	TRACE("connect %d to %08x:%u\n", descriptor, target.ip, swap16(target.port));
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		int client = descriptor - SOCKET_BASE;

		bind_implicitly(socket);
		if (socket->type == SOCK_STREAM_VALUE && !socket->local.ip)
			socket->local = source_address(socket, target.ip);
		memset(&socket->remote, 0, sizeof(socket->remote));
		socket->remote.family = AF_INET_VALUE;
		socket->remote.ip = target.ip;
		socket->remote.port = target.port;
		if (socket->type == SOCK_DGRAM_VALUE)
		{
			socket->connected = 1;
			result = succeed(0);
		}
		else if (socket->connected)
		{
			result = fail(WSAEISCONN);
		}
		else
		{
			int listener = is_local(target.ip) ? find_listener(target.port) : -1;
			int server;

			for (server = 0; server < MAXIMUM_SOCKETS && sockets[server].used; server++)
				;
			if (listener < 0)
				result = fail(WSAECONNREFUSED);
			else if (server == MAXIMUM_SOCKETS || sockets[listener].backlog_count == MAXIMUM_BACKLOG)
				result = fail(WSAENOBUFS);
			else
			{
				/* the listener's end of the connection, until accepted */
				struct web_socket *end = &sockets[server];

				memset(end, 0, sizeof(*end));
				end->used = 1;
				end->type = SOCK_STREAM_VALUE;
				end->bound = 1;
				end->connected = 1;
				end->local = sockets[listener].local;
				end->local.ip = is_loopback(target.ip) ? LOOPBACK_ADDRESS : WEB_LOCAL_ADDRESS;
				end->remote = socket->local;
				end->peer = client;
				end->stream = malloc(STREAM_CAPACITY);
				socket->stream = malloc(STREAM_CAPACITY);
				socket->peer = server;
				socket->connected = 1;
				sockets[listener].backlog[sockets[listener].backlog_count++] = server;
				pthread_cond_broadcast(&network_changed);
				/* Winsock's non-blocking connect: under way, then writeable
				(transport_endpoint_winsock.c waits for that) */
				if (socket->nonblocking)
					result = fail(WSAEWOULDBLOCK);
				else
					result = succeed(0);
			}
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

static int wait_changed(struct web_socket *socket)
{
	if (socket->nonblocking)
		return 0;
	pthread_cond_wait(&network_changed, &network_lock);
	return 1;
}

int posix_socket_accept(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	int result = -1;

	pthread_mutex_lock(&network_lock);
	for (;;)
	{
		socket = socket_get(descriptor);
		if (!socket)
			break;
		if (!socket->listening)
		{
			result = fail(WSAEINVAL);
			break;
		}
		if (socket->backlog_count)
		{
			int accepted = socket->backlog[0];

			memmove(socket->backlog, socket->backlog + 1, (size_t)--socket->backlog_count * sizeof(int));
			if (address && address_length && *address_length >= (int)sizeof(struct address))
			{
				memcpy(address, &sockets[accepted].remote, sizeof(struct address));
				*address_length = (int)sizeof(struct address);
			}
			TRACE("accept %d -> %d\n", descriptor, SOCKET_BASE + accepted);
			result = succeed(SOCKET_BASE + accepted);
			break;
		}
		if (!wait_changed(socket))
		{
			result = fail(WSAEWOULDBLOCK);
			break;
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

static int stream_send(struct web_socket *socket, const unsigned char *buffer, int length)
{
	struct web_socket *peer;
	int written = 0;

	TRACE("send %d bytes on %d (connected %d peer %d closed %d)\n", length, (int)(socket - sockets) + SOCKET_BASE,
		socket->connected, socket->peer, socket->peer_closed);
	if (!socket->connected)
		return fail(WSAENOTCONN);
	for (;;)
	{
		if (socket->peer < 0 || socket->peer_closed)
			return fail(WSAECONNRESET);
		peer = &sockets[socket->peer];
		while (written < length && peer->stream_count < STREAM_CAPACITY)
		{
			peer->stream[(peer->stream_head + peer->stream_count) % STREAM_CAPACITY] = buffer[written++];
			peer->stream_count++;
		}
		pthread_cond_broadcast(&network_changed);
		if (written == length || (written && socket->nonblocking))
			return succeed(written);
		if (!wait_changed(socket))
			return fail(WSAEWOULDBLOCK);
	}
}

static int deliver_datagram(struct web_socket *from, const void *buffer, int length, const struct address *to)
{
	int index, delivered = 0;

	if (length > MAXIMUM_DATAGRAM)
		return fail(WSAEMSGSIZE);
	if (!is_local(to->ip))
		return succeed(length); /* nowhere else to go: lost, as on a network */
	for (index = 0; index < MAXIMUM_SOCKETS; index++)
	{
		struct web_socket *target = &sockets[index];
		struct datagram *datagram;

		if (!target->used || target->type != SOCK_DGRAM_VALUE || !target->bound || target->local.port != to->port)
			continue;
		if (target->datagram_count >= MAXIMUM_DATAGRAMS)
			continue;
		datagram = malloc(sizeof(*datagram) + (size_t)length);
		if (!datagram)
			continue;
		datagram->next = NULL;
		datagram->from = source_address(from, to->ip);
		datagram->length = length;
		memcpy(datagram->data, buffer, (size_t)length);
		if (target->last)
			target->last->next = datagram;
		else
			target->first = datagram;
		target->last = datagram;
		target->datagram_count++;
		delivered = 1;
	}
	TRACE("datagram %d bytes from %d to %08x:%u: %s\n", length, (int)(from - sockets) + SOCKET_BASE, to->ip,
		swap16(to->port), delivered ? "delivered" : "lost");
	if (delivered)
		pthread_cond_broadcast(&network_changed);
	return succeed(length);
}

int posix_socket_send(int descriptor, const void *buffer, int length, int flags)
{
	struct web_socket *socket;
	int result = -1;

	(void)flags;
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (socket->type == SOCK_STREAM_VALUE)
			result = stream_send(socket, buffer, length);
		else if (!socket->connected)
			result = fail(WSAENOTCONN);
		else
			result = deliver_datagram(socket, buffer, length, &socket->remote);
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

int posix_socket_sendto(int descriptor, const void *buffer, int length, int flags,
	const void *address, int address_length)
{
	struct web_socket *socket;
	struct address to;
	int result = -1;

	if (!address || address_length < (int)sizeof(struct address) - 8)
		return posix_socket_send(descriptor, buffer, length, flags);
	memcpy(&to, address, sizeof(to) - 8);
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (socket->type == SOCK_STREAM_VALUE)
			result = stream_send(socket, buffer, length);
		else
		{
			bind_implicitly(socket);
			result = deliver_datagram(socket, buffer, length, &to);
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

static int receive(int descriptor, void *buffer, int length, int flags, void *address, int *address_length)
{
	struct web_socket *socket;
	int peek = (flags & 2) != 0; /* MSG_PEEK */
	int result = -1;

	pthread_mutex_lock(&network_lock);
	for (;;)
	{
		socket = socket_get(descriptor);
		if (!socket)
			break;
		if (socket->type == SOCK_DGRAM_VALUE)
		{
			if (socket->first)
			{
				struct datagram *datagram = socket->first;
				int count = datagram->length < length ? datagram->length : length;

				memcpy(buffer, datagram->data, (size_t)count);
				if (address && address_length && *address_length >= (int)sizeof(struct address))
				{
					memcpy(address, &datagram->from, sizeof(struct address));
					*address_length = (int)sizeof(struct address);
				}
				if (!peek)
				{
					socket->first = datagram->next;
					if (!socket->first)
						socket->last = NULL;
					socket->datagram_count--;
					free(datagram);
				}
				result = count < datagram->length && !peek ? fail(WSAEMSGSIZE) : succeed(count);
				break;
			}
		}
		else
		{
			if (!socket->connected)
			{
				result = fail(WSAENOTCONN);
				break;
			}
			if (socket->stream_count)
			{
				int count = socket->stream_count < length ? socket->stream_count : length;
				int index;

				for (index = 0; index < count; index++)
					((unsigned char *)buffer)[index] = socket->stream[(socket->stream_head + index) % STREAM_CAPACITY];
				if (!peek)
				{
					socket->stream_head = (socket->stream_head + count) % STREAM_CAPACITY;
					socket->stream_count -= count;
					pthread_cond_broadcast(&network_changed);
				}
				if (address && address_length && *address_length >= (int)sizeof(struct address))
				{
					memcpy(address, &socket->remote, sizeof(struct address));
					*address_length = (int)sizeof(struct address);
				}
				TRACE("recv %d bytes on %d\n", count, descriptor);
				result = succeed(count);
				break;
			}
			if (socket->peer_closed)
			{
				/* the other end closed: end of stream */
				result = succeed(0);
				break;
			}
		}
		if (!wait_changed(socket))
		{
			result = fail(WSAEWOULDBLOCK);
			break;
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
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

	TRACE("shutdown %d %d\n", descriptor, how);
	(void)how;
	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		socket->shut_down = 1;
		if (socket->peer >= 0)
			sockets[socket->peer].peer_closed = 1;
		pthread_cond_broadcast(&network_changed);
	}
	pthread_mutex_unlock(&network_lock);
	return socket ? succeed(0) : -1;
}

int posix_socket_set_nonblocking(int descriptor, int nonblocking)
{
	struct web_socket *socket;

	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
		socket->nonblocking = nonblocking != 0;
	pthread_mutex_unlock(&network_lock);
	return socket ? succeed(0) : -1;
}

int posix_socket_set_nodelay(int descriptor)
{
	(void)descriptor;
	return succeed(0);
}

int posix_socket_bytes_available(int descriptor, posix_ulong *count)
{
	struct web_socket *socket;

	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
		*count = (posix_ulong)(socket->type == SOCK_DGRAM_VALUE ?
			(socket->first ? socket->first->length : 0) : socket->stream_count);
	pthread_mutex_unlock(&network_lock);
	return socket ? succeed(0) : -1;
}

int posix_socket_setsockopt(int descriptor, int level, int name, const void *value, int length)
{
	(void)level;
	(void)name;
	(void)value;
	(void)length;
	pthread_mutex_lock(&network_lock);
	if (!socket_get(descriptor))
	{
		pthread_mutex_unlock(&network_lock);
		return -1;
	}
	pthread_mutex_unlock(&network_lock);
	return succeed(0);
}

int posix_socket_getsockopt(int descriptor, int level, int name, void *value, int *length)
{
	struct web_socket *socket;

	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket && value && length && *length >= 4)
	{
		int result = 0;

		if (level == 0xffff && name == 0x1008) /* SO_TYPE */
			result = socket->type;
		if (level == 0xffff && (name == 0x1001 || name == 0x1002)) /* buffer sizes */
			result = STREAM_CAPACITY;
		memcpy(value, &result, 4);
		*length = 4;
	}
	pthread_mutex_unlock(&network_lock);
	return socket ? succeed(0) : -1;
}

int posix_socket_getsockname(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	int result = -1;

	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (!address || !address_length || *address_length < (int)sizeof(struct address))
			result = fail(WSAEFAULT);
		else
		{
			struct address local = socket->local;

			if (!socket->bound)
			{
				memset(&local, 0, sizeof(local));
				local.family = AF_INET_VALUE;
			}
			memcpy(address, &local, sizeof(local));
			*address_length = (int)sizeof(local);
			result = succeed(0);
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

int posix_socket_getpeername(int descriptor, void *address, int *address_length)
{
	struct web_socket *socket;
	int result = -1;

	pthread_mutex_lock(&network_lock);
	socket = socket_get(descriptor);
	if (socket)
	{
		if (!socket->connected)
			result = fail(WSAENOTCONN);
		else if (!address || !address_length || *address_length < (int)sizeof(struct address))
			result = fail(WSAEFAULT);
		else
		{
			memcpy(address, &socket->remote, sizeof(struct address));
			*address_length = (int)sizeof(struct address);
			result = succeed(0);
		}
	}
	pthread_mutex_unlock(&network_lock);
	return result;
}

/* with the lock held */
static int readable(int descriptor)
{
	struct web_socket *socket = socket_get(descriptor);

	if (!socket)
		return 0;
	if (socket->listening)
		return socket->backlog_count > 0;
	if (socket->type == SOCK_DGRAM_VALUE)
		return socket->first != NULL;
	return socket->stream_count > 0 || socket->peer_closed;
}

static int writeable(int descriptor)
{
	struct web_socket *socket = socket_get(descriptor);

	if (!socket)
		return 0;
	if (socket->type == SOCK_DGRAM_VALUE)
		return 1;
	return socket->connected && !socket->peer_closed && socket->peer >= 0 &&
		sockets[socket->peer].stream_count < STREAM_CAPACITY;
}

static int keep(int *descriptors, int *count, int (*ready)(int))
{
	int index, kept = 0;

	if (!descriptors || !count)
		return 0;
	for (index = 0; index < *count; index++)
	{
		if (ready(descriptors[index]))
			descriptors[kept++] = descriptors[index];
	}
	*count = kept;
	return kept;
}

static int any_ready(const int *descriptors, int count, int (*ready)(int))
{
	int index;

	for (index = 0; descriptors && index < count; index++)
	{
		if (ready(descriptors[index]))
			return 1;
	}
	return 0;
}

int posix_socket_select(int *read, int *read_count, int *write, int *write_count,
	int *error, int *error_count, posix_long timeout_seconds, posix_long timeout_microseconds, int infinite)
{
	struct timespec deadline;
	int result;

	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += timeout_seconds + timeout_microseconds / 1000000;
	deadline.tv_nsec += (timeout_microseconds % 1000000) * 1000L;
	if (deadline.tv_nsec >= 1000000000L)
	{
		deadline.tv_sec++;
		deadline.tv_nsec -= 1000000000L;
	}
	pthread_mutex_lock(&network_lock);
	for (;;)
	{
		if (any_ready(read, read ? *read_count : 0, readable) ||
			any_ready(write, write ? *write_count : 0, writeable))
			break;
		if (!infinite && timeout_seconds <= 0 && timeout_microseconds <= 0)
			break;
		if (infinite)
			pthread_cond_wait(&network_changed, &network_lock);
		else if (pthread_cond_timedwait(&network_changed, &network_lock, &deadline) == ETIMEDOUT)
			break;
	}
	result = keep(read, read_count, readable) + keep(write, write_count, writeable);
	if (error && error_count)
		*error_count = 0;
	pthread_mutex_unlock(&network_lock);
	/* like Winsock, a select with nothing ready leaves the last error as it
	was (a connect under way stays WSAEWOULDBLOCK) */
	if (result > 0)
		last_error = 0;
	return result;
}

posix_ulong posix_local_ipv4_address(void)
{
	return (posix_ulong)WEB_LOCAL_ADDRESS;
}

posix_ulong posix_resolve_ipv4(const char *host)
{
	unsigned int parts[4];
	int index = 0;
	const char *cursor = host;

	if (!host)
		return 0;
	if (!strcmp(host, "localhost"))
		return (posix_ulong)LOOPBACK_ADDRESS;
	for (index = 0; index < 4; index++)
	{
		char *end;
		unsigned long value = strtoul(cursor, &end, 10);

		if (end == cursor || value > 255 || (index < 3 && *end != '.') || (index == 3 && *end))
			return 0;
		parts[index] = (unsigned int)value;
		cursor = end + 1;
	}
	return (posix_ulong)(parts[0] | parts[1] << 8 | parts[2] << 16 | parts[3] << 24);
}
