/*
POSIX_NET.C

BSD socket helpers behind the Winsock layer in xnet.c (see posix.h). Built
with the host ABI.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
/* The macOS port's host (port/macos). Its BSD sockaddr starts with a length
byte and a one-byte family where Winsock and Linux have a 16-bit family, so
addresses are converted both ways (address_in, address_out). */
#include <crt_externs.h>
#include <mach-o/dyld.h>
#define SOCK_CLOEXEC 0
#define LINUX_AF_INET6 10

static const struct sockaddr *address_in(const void *address, int length, struct sockaddr_storage *storage,
	socklen_t *host_length)
{
	unsigned short family;

	if (!address || length < 2 || length > (int)sizeof(*storage))
	{
		*host_length = (socklen_t)length;
		return address;
	}
	memcpy(storage, address, (size_t)length);
	memcpy(&family, address, sizeof(family));
	storage->ss_len = (unsigned char)length;
	storage->ss_family = (sa_family_t)(family == LINUX_AF_INET6 ? AF_INET6 : family);
	*host_length = (socklen_t)length;
	return (const struct sockaddr *)storage;
}

static void address_out(void *address, int *length, const struct sockaddr_storage *storage, socklen_t host_length)
{
	unsigned short family = storage->ss_family == AF_INET6 ? LINUX_AF_INET6 : storage->ss_family;
	int size;

	if (!address || !length)
		return;
	size = (int)host_length < *length ? (int)host_length : *length;
	memcpy(address, storage, (size_t)size);
	if (size >= 2)
		memcpy(address, &family, sizeof(family));
	*length = (int)host_length;
}

static int accept4(int socket, struct sockaddr *address, socklen_t *length, int flags)
{
	int result = accept(socket, address, length);

	(void)flags;
	if (result >= 0)
	{
		int one = 1;

		fcntl(result, F_SETFD, FD_CLOEXEC);
		setsockopt(result, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
	}
	return result;
}

static ssize_t getrandom(void *buffer, size_t size, unsigned int flags)
{
	(void)flags;
	arc4random_buf(buffer, size);
	return (ssize_t)size;
}
#endif
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
#ifdef __APPLE__
	int result = socket(family, type, protocol);

	if (result >= 0)
	{
		int one = 1;

		fcntl(result, F_SETFD, FD_CLOEXEC);
		setsockopt(result, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
	}
	return succeed(result);
#else
	return succeed(socket(family, type | SOCK_CLOEXEC, protocol));
#endif
}

int posix_socket_close(int socket)
{
	return succeed(close(socket));
}

int posix_socket_bind(int socket, const void *address, int address_length)
{
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t length;
	const struct sockaddr *host_address = address_in(address, address_length, &storage, &length);

	return succeed(bind(socket, host_address, length));
#else
	return succeed(bind(socket, address, (socklen_t)address_length));
#endif
}

int posix_socket_connect(int socket, const void *address, int address_length)
{
	/* A non-blocking connect that is under way is EINPROGRESS here but
	WSAEWOULDBLOCK in Winsock, which is what the game waits on before it
	selects for the socket becoming writeable (connect_endpoint,
	transport_endpoint_winsock.c); as WSAEINPROGRESS it gave up at once,
	and every system link join failed, a split screen game's join of its
	own host included. */
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t length;
	const struct sockaddr *host_address = address_in(address, address_length, &storage, &length);
	int result = connect(socket, host_address, length);
#else
	int result = connect(socket, address, (socklen_t)address_length);
#endif

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
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t length = sizeof(storage);
	int result = accept4(socket, (struct sockaddr *)&storage, &length, SOCK_CLOEXEC);

	if (result >= 0)
		address_out(address, address_length, &storage, length);
	return succeed(result);
#else
	socklen_t length = address_length ? (socklen_t)*address_length : 0;
	int result = accept4(socket, address, address_length ? &length : NULL, SOCK_CLOEXEC);

	if (address_length)
		*address_length = (int)length;
	return succeed(result);
#endif
}

int posix_socket_send(int socket, const void *buffer, int length, int flags)
{
	return succeed((int)send(socket, buffer, (size_t)length, flags | MSG_NOSIGNAL));
}

int posix_socket_sendto(int socket, const void *buffer, int length, int flags,
	const void *address, int address_length)
{
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t host_length;
	const struct sockaddr *host_address = address_in(address, address_length, &storage, &host_length);

	return succeed((int)sendto(socket, buffer, (size_t)length, flags | MSG_NOSIGNAL, host_address, host_length));
#else
	return succeed((int)sendto(socket, buffer, (size_t)length, flags | MSG_NOSIGNAL,
		address, (socklen_t)address_length));
#endif
}

int posix_socket_recv(int socket, void *buffer, int length, int flags)
{
	return succeed((int)recv(socket, buffer, (size_t)length, flags));
}

int posix_socket_recvfrom(int socket, void *buffer, int length, int flags,
	void *address, int *address_length)
{
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t socket_length = sizeof(storage);
	int result = (int)recvfrom(socket, buffer, (size_t)length, flags, (struct sockaddr *)&storage, &socket_length);

	if (result >= 0)
		address_out(address, address_length, &storage, socket_length);
	return succeed(result);
#else
	socklen_t socket_length = address_length ? (socklen_t)*address_length : 0;
	int result = (int)recvfrom(socket, buffer, (size_t)length, flags, address,
		address_length ? &socket_length : NULL);

	if (address_length)
		*address_length = (int)socket_length;
	return succeed(result);
#endif
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

int posix_socket_set_nodelay(int socket)
{
	int value = 1;

	return succeed(setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)));
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
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t length = sizeof(storage);
	int result = getsockname(socket, (struct sockaddr *)&storage, &length);

	if (result >= 0)
		address_out(address, address_length, &storage, length);
	return succeed(result);
#else
	socklen_t length = (socklen_t)*address_length;
	int result = getsockname(socket, address, &length);

	*address_length = (int)length;
	return succeed(result);
#endif
}

int posix_socket_getpeername(int socket, void *address, int *address_length)
{
#ifdef __APPLE__
	struct sockaddr_storage storage;
	socklen_t length = sizeof(storage);
	int result = getpeername(socket, (struct sockaddr *)&storage, &length);

	if (result >= 0)
		address_out(address, address_length, &storage, length);
	return succeed(result);
#else
	socklen_t length = (socklen_t)*address_length;
	int result = getpeername(socket, address, &length);

	*address_length = (int)length;
	return succeed(result);
#endif
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

posix_ulong posix_resolve_ipv4(const char *host)
{
	struct addrinfo hints, *results;
	posix_ulong address = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if (getaddrinfo(host, NULL, &hints, &results) != 0)
		return 0;
	if (results && results->ai_addr && results->ai_addr->sa_family == AF_INET)
		address = ((struct sockaddr_in *)results->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(results);
	return address;
}

/* ---------- the process and the desktop */

int posix_command_line_argument(int index, char *buffer, posix_ulong size)
{
#ifdef __ANDROID__
	(void)index;
	(void)buffer;
	(void)size;
	return 0;
#elif defined(__APPLE__)
	int count = *_NSGetArgc();
	char **arguments = *_NSGetArgv();

	if (index < 0 || index >= count || !size)
		return 0;
	snprintf(buffer, size, "%s", arguments[index]);
	return 1;
#else
	char command_line[4096];
	ssize_t length;
	ssize_t offset = 0;
	int descriptor = open("/proc/self/cmdline", O_RDONLY);

	if (descriptor < 0 || !size)
		return descriptor >= 0 ? (close(descriptor), 0) : 0;
	length = read(descriptor, command_line, sizeof(command_line) - 1);
	close(descriptor);
	if (length <= 0)
		return 0;
	command_line[length] = '\0';
	/* the arguments are NUL-separated */
	while (index-- > 0)
	{
		offset += (ssize_t)strlen(command_line + offset) + 1;
		if (offset >= length)
			return 0;
	}
	snprintf(buffer, size, "%s", command_line + offset);
	return 1;
#endif
}

posix_ulong posix_process_id(void)
{
	return (posix_ulong)getpid();
}

#ifndef __ANDROID__
/* runs a program with its arguments and waits for it; its exit status, or -1 */
static int run_program(char *const arguments[])
{
	extern char **environ;
	pid_t process;
	int status;

	if (posix_spawnp(&process, arguments[0], NULL, NULL, arguments, environ) != 0)
		return -1;
	if (waitpid(process, &status, 0) < 0)
		return -1;
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

int posix_register_url_scheme(const char *scheme, const char *description)
{
#if defined(__ANDROID__) || defined(__APPLE__)
	/* (macOS registers URL schemes through an application bundle's
	Info.plist, which the port's executable does not have) */
	(void)scheme;
	(void)description;
	return 0;
#else
	/* a desktop entry declaring the executable as the scheme's handler, and
	the scheme's default application set to it (as xdg-open reads it) */
	char executable[1024], directory[1024], path[1200], name[128], entry[2048], existing[2048];
	const char *data_home = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
	FILE *file;
	size_t existing_length = 0;

	if (length <= 0)
		return 0;
	executable[length] = '\0';
	if (data_home && *data_home)
		snprintf(directory, sizeof(directory), "%s/applications", data_home);
	else if (home && *home)
		snprintf(directory, sizeof(directory), "%s/.local/share/applications", home);
	else
		return 0;
	snprintf(name, sizeof(name), "halo-ce-universal-%s.desktop", scheme);
	snprintf(path, sizeof(path), "%s/%s", directory, name);
	snprintf(entry, sizeof(entry),
		"[Desktop Entry]\n"
		"Type=Application\n"
		"Name=%s\n"
		"Exec=\"%s\" %%u\n"
		"NoDisplay=true\n"
		"MimeType=x-scheme-handler/%s;\n",
		description, executable, scheme);
	/* unchanged since the last start: nothing to do */
	file = fopen(path, "r");
	if (file)
	{
		existing_length = fread(existing, 1, sizeof(existing) - 1, file);
		fclose(file);
		existing[existing_length] = '\0';
		if (!strcmp(existing, entry))
			return 1;
	}
	mkdir(directory, 0755);
	file = fopen(path, "w");
	if (!file)
		return 0;
	fputs(entry, file);
	fclose(file);
	{
		char mime_type[160];
		char *arguments[] = { "xdg-mime", "default", name, mime_type, NULL };

		snprintf(mime_type, sizeof(mime_type), "x-scheme-handler/%s", scheme);
		run_program(arguments);
	}
	return 1;
#endif
}

/* ---------- Discord's local socket */

int posix_discord_connect(void)
{
#ifdef __ANDROID__
	return -1;
#else
	/* where Discord (and its Flatpak and Snap packages) put discord-ipc-N */
	static const char *const variables[] = { "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP" };
	static const char *const subdirectories[] = { "", "app/com.discordapp.Discord/", "snap.discord/",
		".flatpak/dev.vencord.Vesktop/xdg-run/" };
	const char *directories[5];
	int directory_count = 0;
	int index;

	for (index = 0; index < (int)(sizeof(variables) / sizeof(*variables)); index++)
	{
		const char *value = getenv(variables[index]);

		if (value && *value)
			directories[directory_count++] = value;
	}
	directories[directory_count++] = "/tmp";
	for (index = 0; index < directory_count; index++)
	{
		int subdirectory;

		for (subdirectory = 0; subdirectory < (int)(sizeof(subdirectories) / sizeof(*subdirectories)); subdirectory++)
		{
			int number;

			for (number = 0; number < 10; number++)
			{
				struct sockaddr_un address;
				int socket_descriptor;

				memset(&address, 0, sizeof(address));
				address.sun_family = AF_UNIX;
				snprintf(address.sun_path, sizeof(address.sun_path), "%s/%sdiscord-ipc-%d",
					directories[index], subdirectories[subdirectory], number);
				if (access(address.sun_path, F_OK) != 0)
					continue;
				socket_descriptor = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
				if (socket_descriptor < 0)
					return -1;
				if (connect(socket_descriptor, (struct sockaddr *)&address, sizeof(address)) == 0)
				{
					fcntl(socket_descriptor, F_SETFL, fcntl(socket_descriptor, F_GETFL) | O_NONBLOCK);
					return socket_descriptor;
				}
				close(socket_descriptor);
			}
		}
	}
	return -1;
#endif
}

int posix_discord_write(int handle, const void *buffer, int length)
{
	const char *cursor = buffer;
	int remaining = length;

	while (remaining > 0)
	{
		ssize_t written = send(handle, cursor, (size_t)remaining, MSG_NOSIGNAL);

		if (written < 0)
		{
			struct pollfd poll_descriptor;

			if (errno == EINTR)
				continue;
			if (errno != EAGAIN)
				return -1;
			poll_descriptor.fd = handle;
			poll_descriptor.events = POLLOUT;
			if (poll(&poll_descriptor, 1, 1000) <= 0)
				return -1;
			continue;
		}
		cursor += written;
		remaining -= (int)written;
	}
	return length;
}

int posix_discord_read(int handle, void *buffer, int length)
{
	ssize_t count = recv(handle, buffer, (size_t)length, MSG_DONTWAIT);

	if (count > 0)
		return (int)count;
	if (count < 0 && (errno == EAGAIN || errno == EINTR))
		return 0;
	return -1;
}

void posix_discord_close(int handle)
{
	if (handle >= 0)
		close(handle);
}

/* ---------- Tailscale

`tailscale status --json` names this machine ("Self") and the tailnet's
other machines ("Peer", each with "Online"), each with its "TailscaleIPs".
A thread asks every 15 seconds, so the game's thread never waits for the
command; the first call waits up to three seconds for the first answer. */

#ifdef __ANDROID__
int posix_tailscale_addresses(posix_ulong *self, posix_ulong *peers, int capacity)
{
	(void)peers;
	(void)capacity;
	*self = 0;
	return -1;
}
#else
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define TAILSCALE_PEERS 128
#define TAILSCALE_OUTPUT_MAXIMUM (8 * 1024 * 1024)

static struct
{
	pthread_mutex_t lock;
	pthread_cond_t answered;
	int started;
	int answered_once;
	int running;
	unsigned long self;
	int peer_count;
	unsigned long peers[TAILSCALE_PEERS];
} tailscale = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER };

extern char **environ;

/* the tailscale command: on PATH's usual places, or inside the macOS app */
static const char *tailscale_command(void)
{
	static const char *const candidates[] =
	{
		"/usr/local/bin/tailscale", "/opt/homebrew/bin/tailscale", "/usr/bin/tailscale", "/usr/sbin/tailscale",
		"/Applications/Tailscale.app/Contents/MacOS/Tailscale",
	};
	size_t index;

	for (index = 0; index < sizeof(candidates) / sizeof(candidates[0]); index++)
	{
		if (access(candidates[index], X_OK) == 0)
			return candidates[index];
	}
	return NULL;
}

/* the command's output, NUL terminated (free() it), or NULL; gives up after
five seconds */
static char *tailscale_run(void)
{
	const char *command = tailscale_command();
	char *arguments[4];
	posix_spawn_file_actions_t actions;
	int pipe_ends[2];
	pid_t child;
	char *output = NULL;
	size_t length = 0, capacity = 0;
	time_t deadline;
	int status;

	if (!command || pipe(pipe_ends) != 0)
		return NULL;
	arguments[0] = (char *)command;
	arguments[1] = "status";
	arguments[2] = "--json";
	arguments[3] = NULL;
	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_adddup2(&actions, pipe_ends[1], 1);
	posix_spawn_file_actions_addclose(&actions, pipe_ends[0]);
	posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
	if (posix_spawn(&child, command, &actions, NULL, arguments, environ) != 0)
	{
		posix_spawn_file_actions_destroy(&actions);
		close(pipe_ends[0]);
		close(pipe_ends[1]);
		return NULL;
	}
	posix_spawn_file_actions_destroy(&actions);
	close(pipe_ends[1]);
	deadline = time(NULL) + 5;
	for (;;)
	{
		struct pollfd ready = { pipe_ends[0], POLLIN, 0 };
		ssize_t read_count;

		if (time(NULL) > deadline || poll(&ready, 1, 1000) < 0)
			break;
		if (!ready.revents)
			continue;
		if (length + 65536 + 1 > capacity)
		{
			char *grown;

			if (capacity >= TAILSCALE_OUTPUT_MAXIMUM)
				break;
			capacity = capacity ? capacity * 2 : 65536 * 2;
			grown = realloc(output, capacity);
			if (!grown)
				break;
			output = grown;
		}
		read_count = read(pipe_ends[0], output + length, 65536);
		if (read_count <= 0)
			break;
		length += (size_t)read_count;
	}
	close(pipe_ends[0]);
	if (waitpid(child, &status, WNOHANG) == 0)
	{
		kill(child, SIGKILL);
		waitpid(child, &status, 0);
	}
	if (output)
		output[length] = 0;
	return output;
}

/* a small JSON reader: just enough to walk the status to the fields above */

enum tailscale_role
{
	_role_other,
	_role_self,
	_role_peer_map,
	_role_peer,
};

struct tailscale_node
{
	unsigned long address;
	int online;
};

struct json
{
	const char *cursor;
	int error;
};

static void json_space(struct json *json)
{
	while (*json->cursor == ' ' || *json->cursor == '\t' || *json->cursor == '\n' || *json->cursor == '\r')
		json->cursor++;
}

/* a string (escapes are kept as they are; none of the names used have any) */
static void json_string(struct json *json, char *out, size_t size)
{
	size_t used = 0;

	json_space(json);
	if (*json->cursor != '"')
	{
		json->error = 1;
		return;
	}
	json->cursor++;
	while (*json->cursor && *json->cursor != '"')
	{
		if (*json->cursor == '\\' && json->cursor[1])
			json->cursor++;
		if (out && used + 1 < size)
			out[used++] = *json->cursor;
		json->cursor++;
	}
	if (*json->cursor != '"')
	{
		json->error = 1;
		return;
	}
	json->cursor++;
	if (out && size)
		out[used] = 0;
}

static void json_value(struct json *json, enum tailscale_role role, int depth, const char *key,
	struct tailscale_node *node);

static void json_object(struct json *json, enum tailscale_role role, int depth)
{
	struct tailscale_node node = { 0, 0 };

	json->cursor++;
	json_space(json);
	while (!json->error && *json->cursor && *json->cursor != '}')
	{
		char key[64];
		enum tailscale_role child = _role_other;

		json_string(json, key, sizeof(key));
		json_space(json);
		if (*json->cursor != ':')
		{
			json->error = 1;
			return;
		}
		json->cursor++;
		if (role == _role_other && depth == 0 && !strcmp(key, "Self"))
			child = _role_self;
		else if (role == _role_other && depth == 0 && !strcmp(key, "Peer"))
			child = _role_peer_map;
		else if (role == _role_peer_map)
			child = _role_peer;
		json_value(json, child, depth + 1, key,
			role == _role_self || role == _role_peer ? &node : NULL);
		json_space(json);
		if (*json->cursor == ',')
			json->cursor++;
		json_space(json);
	}
	if (*json->cursor != '}')
	{
		json->error = 1;
		return;
	}
	json->cursor++;
	if (role == _role_self)
	{
		tailscale.self = node.address;
	}
	else if (role == _role_peer && node.online && node.address && tailscale.peer_count < TAILSCALE_PEERS)
	{
		tailscale.peers[tailscale.peer_count++] = node.address;
	}
}

static void json_value(struct json *json, enum tailscale_role role, int depth, const char *key,
	struct tailscale_node *node)
{
	json_space(json);
	if (depth > 32)
	{
		json->error = 1;
		return;
	}
	switch (*json->cursor)
	{
	case '{':
		json_object(json, role, depth);
		break;
	case '[':
		json->cursor++;
		json_space(json);
		while (!json->error && *json->cursor && *json->cursor != ']')
		{
			if (node && !strcmp(key, "TailscaleIPs") && *json->cursor == '"')
			{
				char text[64];
				struct in_addr address;

				/* the first IPv4 address */
				json_string(json, text, sizeof(text));
				if (!node->address && inet_pton(AF_INET, text, &address) == 1)
					node->address = address.s_addr;
			}
			else
			{
				json_value(json, _role_other, depth + 1, "", NULL);
			}
			json_space(json);
			if (*json->cursor == ',')
				json->cursor++;
			json_space(json);
		}
		if (*json->cursor != ']')
		{
			json->error = 1;
			return;
		}
		json->cursor++;
		break;
	case '"':
		json_string(json, NULL, 0);
		break;
	default:
		/* a number, true, false or null */
		if (node && !strcmp(key, "Online"))
			node->online = !strncmp(json->cursor, "true", 4);
		while (*json->cursor && *json->cursor != ',' && *json->cursor != '}' && *json->cursor != ']')
			json->cursor++;
		break;
	}
}

static void tailscale_refresh(void)
{
	char *output = tailscale_run();
	struct json json;

	pthread_mutex_lock(&tailscale.lock);
	tailscale.self = 0;
	tailscale.peer_count = 0;
	tailscale.running = 0;
	if (output && strstr(output, "\"BackendState\"") && strstr(output, "\"Running\""))
	{
		json.cursor = output;
		json.error = 0;
		json_space(&json);
		if (*json.cursor == '{')
			json_object(&json, _role_other, 0);
		tailscale.running = !json.error && tailscale.self != 0;
		if (!tailscale.running)
		{
			tailscale.self = 0;
			tailscale.peer_count = 0;
		}
	}
	tailscale.answered_once = 1;
	pthread_cond_broadcast(&tailscale.answered);
	pthread_mutex_unlock(&tailscale.lock);
	free(output);
}

static void *tailscale_thread(void *unused)
{
	(void)unused;
	for (;;)
	{
		tailscale_refresh();
		sleep(15);
	}
	return NULL;
}

int posix_tailscale_addresses(posix_ulong *self, posix_ulong *peers, int capacity)
{
	int count, index;

	pthread_mutex_lock(&tailscale.lock);
	if (!tailscale.started)
	{
		pthread_t thread;

		tailscale.started = 1;
		if (tailscale_command() && pthread_create(&thread, NULL, tailscale_thread, NULL) == 0)
			pthread_detach(thread);
		else
			tailscale.answered_once = 1;
	}
	if (!tailscale.answered_once)
	{
		struct timespec until;

		clock_gettime(CLOCK_REALTIME, &until);
		until.tv_sec += 3;
		while (!tailscale.answered_once)
		{
			if (pthread_cond_timedwait(&tailscale.answered, &tailscale.lock, &until) != 0)
				break;
		}
	}
	*self = 0;
	count = -1;
	if (tailscale.running)
	{
		*self = (posix_ulong)tailscale.self;
		count = tailscale.peer_count < capacity ? tailscale.peer_count : capacity;
		for (index = 0; index < count; index++)
			peers[index] = (posix_ulong)tailscale.peers[index];
	}
	pthread_mutex_unlock(&tailscale.lock);
	return count;
}
#endif
