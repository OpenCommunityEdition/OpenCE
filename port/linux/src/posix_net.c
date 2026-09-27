/*
POSIX_NET.C

BSD socket helpers behind the Winsock layer in xnet.c (see posix.h). Built
with the host ABI.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "posix.h"

/* Winsock error codes (winsockx.h) */
#define WSAEINTR 10004
#define WSAEBADF 10009
#define WSAEACCES 10013
#define WSAEFAULT 10014
#define WSAEINVAL 10022
#define WSAEMFILE 10024
#define WSAEWOULDBLOCK 10035
#define WSAEINPROGRESS 10036
#define WSAEALREADY 10037
#define WSAENOTSOCK 10038
#define WSAEDESTADDRREQ 10039
#define WSAEMSGSIZE 10040
#define WSAEPROTOTYPE 10041
#define WSAENOPROTOOPT 10042
#define WSAEPROTONOSUPPORT 10043
#define WSAEOPNOTSUPP 10045
#define WSAEAFNOSUPPORT 10047
#define WSAEADDRINUSE 10048
#define WSAEADDRNOTAVAIL 10049
#define WSAENETDOWN 10050
#define WSAENETUNREACH 10051
#define WSAECONNABORTED 10053
#define WSAECONNRESET 10054
#define WSAENOBUFS 10055
#define WSAEISCONN 10056
#define WSAENOTCONN 10057
#define WSAETIMEDOUT 10060
#define WSAECONNREFUSED 10061
#define WSAEHOSTUNREACH 10065

/* Winsock SOL_SOCKET option values (winsockx.h) */
#define WINSOCK_SOL_SOCKET 0xffff
#define WINSOCK_SO_REUSEADDR 0x0004
#define WINSOCK_SO_KEEPALIVE 0x0008
#define WINSOCK_SO_BROADCAST 0x0020
#define WINSOCK_SO_LINGER 0x0080
#define WINSOCK_SO_SNDBUF 0x1001
#define WINSOCK_SO_RCVBUF 0x1002
#define WINSOCK_SO_ERROR 0x1007
#define WINSOCK_SO_TYPE 0x1008

static __thread int last_error;

static int fail(void)
{
	switch (errno)
	{
	case EINTR: last_error = WSAEINTR; break;
	case EBADF: last_error = WSAEBADF; break;
	case EACCES: case EPERM: last_error = WSAEACCES; break;
	case EFAULT: last_error = WSAEFAULT; break;
	case EMFILE: case ENFILE: last_error = WSAEMFILE; break;
	case EAGAIN: last_error = WSAEWOULDBLOCK; break;
	case EINPROGRESS: last_error = WSAEINPROGRESS; break;
	case EALREADY: last_error = WSAEALREADY; break;
	case ENOTSOCK: last_error = WSAENOTSOCK; break;
	case EDESTADDRREQ: last_error = WSAEDESTADDRREQ; break;
	case EMSGSIZE: last_error = WSAEMSGSIZE; break;
	case EPROTOTYPE: last_error = WSAEPROTOTYPE; break;
	case ENOPROTOOPT: last_error = WSAENOPROTOOPT; break;
	case EPROTONOSUPPORT: last_error = WSAEPROTONOSUPPORT; break;
	case EOPNOTSUPP: last_error = WSAEOPNOTSUPP; break;
	case EAFNOSUPPORT: last_error = WSAEAFNOSUPPORT; break;
	case EADDRINUSE: last_error = WSAEADDRINUSE; break;
	case EADDRNOTAVAIL: last_error = WSAEADDRNOTAVAIL; break;
	case ENETDOWN: last_error = WSAENETDOWN; break;
	case ENETUNREACH: last_error = WSAENETUNREACH; break;
	case ECONNABORTED: last_error = WSAECONNABORTED; break;
	case ECONNRESET: last_error = WSAECONNRESET; break;
	case ENOBUFS: case ENOMEM: last_error = WSAENOBUFS; break;
	case EISCONN: last_error = WSAEISCONN; break;
	case ENOTCONN: last_error = WSAENOTCONN; break;
	case ETIMEDOUT: last_error = WSAETIMEDOUT; break;
	case ECONNREFUSED: last_error = WSAECONNREFUSED; break;
	case EHOSTUNREACH: last_error = WSAEHOSTUNREACH; break;
	default: last_error = WSAEINVAL; break;
	}
	return -1;
}

static int succeed(int result)
{
	if (result < 0)
		return fail();
	last_error = 0;
	return result;
}

int posix_socket_last_error(void)
{
	return last_error;
}

int posix_socket(int family, int type, int protocol)
{
	return succeed(socket(family, type | SOCK_CLOEXEC, protocol));
}

int posix_socket_close(int socket)
{
	return succeed(close(socket));
}

int posix_socket_bind(int socket, const void *address, int address_length)
{
	return succeed(bind(socket, address, (socklen_t)address_length));
}

int posix_socket_connect(int socket, const void *address, int address_length)
{
	/* A non-blocking connect that is under way is EINPROGRESS here but
	WSAEWOULDBLOCK in Winsock, which is what the game waits on before it
	selects for the socket becoming writeable (connect_endpoint,
	transport_endpoint_winsock.c); as WSAEINPROGRESS it gave up at once,
	and every system link join failed, a split screen game's join of its
	own host included. */
	int result = connect(socket, address, (socklen_t)address_length);

	if (result < 0 && errno == EINPROGRESS)
	{
		last_error = WSAEWOULDBLOCK;
		return -1;
	}
	return succeed(result);
}

int posix_socket_listen(int socket, int backlog)
{
	return succeed(listen(socket, backlog));
}

int posix_socket_accept(int socket, void *address, int *address_length)
{
	socklen_t length = address_length ? (socklen_t)*address_length : 0;
	int result = accept4(socket, address, address_length ? &length : NULL, SOCK_CLOEXEC);

	if (address_length)
		*address_length = (int)length;
	return succeed(result);
}

int posix_socket_send(int socket, const void *buffer, int length, int flags)
{
	return succeed((int)send(socket, buffer, (size_t)length, flags | MSG_NOSIGNAL));
}

int posix_socket_sendto(int socket, const void *buffer, int length, int flags,
	const void *address, int address_length)
{
	return succeed((int)sendto(socket, buffer, (size_t)length, flags | MSG_NOSIGNAL,
		address, (socklen_t)address_length));
}

int posix_socket_recv(int socket, void *buffer, int length, int flags)
{
	return succeed((int)recv(socket, buffer, (size_t)length, flags));
}

int posix_socket_recvfrom(int socket, void *buffer, int length, int flags,
	void *address, int *address_length)
{
	socklen_t socket_length = address_length ? (socklen_t)*address_length : 0;
	int result = (int)recvfrom(socket, buffer, (size_t)length, flags, address,
		address_length ? &socket_length : NULL);

	if (address_length)
		*address_length = (int)socket_length;
	return succeed(result);
}

int posix_socket_shutdown(int socket, int how)
{
	return succeed(shutdown(socket, how));
}

int posix_socket_set_nonblocking(int socket, int nonblocking)
{
	int flags = fcntl(socket, F_GETFL);

	if (flags < 0)
		return fail();
	flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
	return succeed(fcntl(socket, F_SETFL, flags));
}

int posix_socket_bytes_available(int socket, posix_ulong *count)
{
	int available = 0;
	int result = ioctl(socket, FIONREAD, &available);

	if (result >= 0)
		*count = (posix_ulong)available;
	return succeed(result);
}

static int translate_option(int level, int name, int *host_level, int *host_name)
{
	if (level != WINSOCK_SOL_SOCKET)
	{
		/* IPPROTO_IP / IPPROTO_TCP option numbers are shared */
		*host_level = level;
		*host_name = name;
		return 0;
	}
	*host_level = SOL_SOCKET;
	switch (name)
	{
	case WINSOCK_SO_REUSEADDR: *host_name = SO_REUSEADDR; return 0;
	case WINSOCK_SO_KEEPALIVE: *host_name = SO_KEEPALIVE; return 0;
	case WINSOCK_SO_BROADCAST: *host_name = SO_BROADCAST; return 0;
	case WINSOCK_SO_LINGER: *host_name = SO_LINGER; return 0;
	case WINSOCK_SO_SNDBUF: *host_name = SO_SNDBUF; return 0;
	case WINSOCK_SO_RCVBUF: *host_name = SO_RCVBUF; return 0;
	case WINSOCK_SO_ERROR: *host_name = SO_ERROR; return 0;
	case WINSOCK_SO_TYPE: *host_name = SO_TYPE; return 0;
	default: return -1;
	}
}

int posix_socket_setsockopt(int socket, int level, int name, const void *value, int length)
{
	int host_level, host_name;

	if (translate_option(level, name, &host_level, &host_name) != 0)
	{
		/* Xbox-only options such as SO_ENCRYPT have nothing to do here */
		last_error = 0;
		return 0;
	}
	return succeed(setsockopt(socket, host_level, host_name, value, (socklen_t)length));
}

int posix_socket_getsockopt(int socket, int level, int name, void *value, int *length)
{
	int host_level, host_name;
	socklen_t socket_length = (socklen_t)*length;
	int result;

	if (translate_option(level, name, &host_level, &host_name) != 0)
	{
		last_error = WSAENOPROTOOPT;
		return -1;
	}
	result = getsockopt(socket, host_level, host_name, value, &socket_length);
	*length = (int)socket_length;
	return succeed(result);
}

int posix_socket_getsockname(int socket, void *address, int *address_length)
{
	socklen_t length = (socklen_t)*address_length;
	int result = getsockname(socket, address, &length);

	*address_length = (int)length;
	return succeed(result);
}

int posix_socket_getpeername(int socket, void *address, int *address_length)
{
	socklen_t length = (socklen_t)*address_length;
	int result = getpeername(socket, address, &length);

	*address_length = (int)length;
	return succeed(result);
}

static int fill_set(fd_set *set, const int *descriptors, int count, int maximum)
{
	int index;

	FD_ZERO(set);
	for (index = 0; index < count; index++)
	{
		if (descriptors[index] >= 0 && descriptors[index] < FD_SETSIZE)
		{
			FD_SET(descriptors[index], set);
			if (descriptors[index] > maximum)
				maximum = descriptors[index];
		}
	}
	return maximum;
}

static void keep_ready(fd_set *set, int *descriptors, int *count)
{
	int index, kept = 0;

	for (index = 0; index < *count; index++)
	{
		if (descriptors[index] >= 0 && descriptors[index] < FD_SETSIZE && FD_ISSET(descriptors[index], set))
			descriptors[kept++] = descriptors[index];
	}
	*count = kept;
}

int posix_socket_select(int *read, int *read_count, int *write, int *write_count,
	int *error, int *error_count, posix_long timeout_seconds, posix_long timeout_microseconds, int infinite)
{
	fd_set read_set, write_set, error_set;
	struct timeval timeout;
	int maximum = -1;
	int result;

	maximum = fill_set(&read_set, read, read ? *read_count : 0, maximum);
	maximum = fill_set(&write_set, write, write ? *write_count : 0, maximum);
	maximum = fill_set(&error_set, error, error ? *error_count : 0, maximum);
	timeout.tv_sec = timeout_seconds;
	timeout.tv_usec = timeout_microseconds;
	result = select(maximum + 1, read ? &read_set : NULL, write ? &write_set : NULL,
		error ? &error_set : NULL, infinite ? NULL : &timeout);
	if (result < 0)
		return fail();
	if (write)
	{
		/* Winsock reports a socket writeable once its connect has succeeded;
		one whose connect failed is not (it is in the error set), where
		POSIX reports it writeable with the failure in SO_ERROR. The game
		takes writeable as connected (connect_endpoint). */
		int index;

		for (index = 0; index < *write_count; index++)
		{
			int descriptor = write[index];
			int pending = 0;
			socklen_t length = sizeof(pending);

			if (descriptor >= 0 && descriptor < FD_SETSIZE && FD_ISSET(descriptor, &write_set) &&
				getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &pending, &length) == 0 && pending)
			{
				FD_CLR(descriptor, &write_set);
				result--;
				errno = pending;
				fail();
			}
		}
	}
	if (read)
		keep_ready(&read_set, read, read_count);
	if (write)
		keep_ready(&write_set, write, write_count);
	if (error)
		keep_ready(&error_set, error, error_count);
	/* like Winsock, a select with nothing ready leaves the last error as it
	was: after a connect under way, still WSAEWOULDBLOCK, which the game
	reads as not connected yet */
	if (result > 0)
		last_error = 0;
	return result;
}

posix_ulong posix_local_ipv4_address(void)
{
	struct ifaddrs *addresses, *entry;
	posix_ulong result = 0;

	if (getifaddrs(&addresses) != 0)
		return 0;
	for (entry = addresses; entry; entry = entry->ifa_next)
	{
		if (entry->ifa_addr && entry->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *address = (struct sockaddr_in *)entry->ifa_addr;

			if (address->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
			{
				result = address->sin_addr.s_addr;
				break;
			}
		}
	}
	freeifaddrs(addresses);
	return result;
}

void posix_random_bytes(void *buffer, posix_ulong size)
{
	unsigned char *cursor = buffer;

	while (size)
	{
		ssize_t count = getrandom(cursor, size, 0);

		if (count <= 0)
		{
			if (count < 0 && errno == EINTR)
				continue;
			break;
		}
		cursor += count;
		size -= (posix_ulong)count;
	}
}
