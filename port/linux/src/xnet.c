/*
XNET.C

Xbox Winsock and XNet for the Linux build, over BSD sockets.

Game code reaches these under the halo_ws_ names (see
halo_linux_winsock_names.h), since glibc exports its own cdecl socket(),
bind(), ... that must not be confused with the __stdcall Winsock ones.

XNet's secure addressing collapses to plain IPv4: a host's XNADDR carries its
real address, key exchange keys are random but unused, and XNADDR to
IN_ADDR translation is the identity. That is enough for system link play on
a LAN.
*/

#include "platform.h"
#include "posix.h"

#include <string.h>

/* ---------- Winsock */

static int winsock_result(int result)
{
	if (result < 0)
	{
		WSASetLastError(posix_socket_last_error());
		return SOCKET_ERROR;
	}
	return result;
}

static __thread int winsock_last_error;

int WSAAPI WSAGetLastError(void)
{
	return winsock_last_error;
}

void WSAAPI WSASetLastError(int error)
{
	winsock_last_error = error;
}

int WSAAPI WSAStartup(WORD version_requested, LPWSADATA data)
{
	if (data)
	{
		memset(data, 0, sizeof(*data));
		data->wVersion = version_requested;
		data->wHighVersion = MAKEWORD(2, 2);
		data->iMaxSockets = 64;
		data->iMaxUdpDg = 1264;
	}
	return 0;
}

int WSAAPI WSACleanup(void)
{
	return 0;
}

SOCKET WSAAPI halo_ws_socket(int family, int type, int protocol)
{
	int result = posix_socket(family, type, protocol);

	if (result < 0)
	{
		WSASetLastError(posix_socket_last_error());
		return INVALID_SOCKET;
	}
	return (SOCKET)result;
}

int WSAAPI halo_ws_closesocket(SOCKET socket)
{
	return winsock_result(posix_socket_close((int)socket));
}

int WSAAPI halo_ws_bind(SOCKET socket, const struct sockaddr *address, int address_length)
{
	return winsock_result(posix_socket_bind((int)socket, address, address_length));
}

int WSAAPI halo_ws_connect(SOCKET socket, const struct sockaddr *address, int address_length)
{
	return winsock_result(posix_socket_connect((int)socket, address, address_length));
}

int WSAAPI halo_ws_listen(SOCKET socket, int backlog)
{
	return winsock_result(posix_socket_listen((int)socket, backlog));
}

SOCKET WSAAPI halo_ws_accept(SOCKET socket, struct sockaddr *address, int *address_length)
{
	int result = posix_socket_accept((int)socket, address, address_length);

	if (result < 0)
	{
		WSASetLastError(posix_socket_last_error());
		return INVALID_SOCKET;
	}
	return (SOCKET)result;
}

int WSAAPI halo_ws_send(SOCKET socket, const char *buffer, int length, int flags)
{
	return winsock_result(posix_socket_send((int)socket, buffer, length, flags));
}

int WSAAPI halo_ws_sendto(SOCKET socket, const char *buffer, int length, int flags,
	const struct sockaddr *address, int address_length)
{
	return winsock_result(posix_socket_sendto((int)socket, buffer, length, flags, address, address_length));
}

int WSAAPI halo_ws_recv(SOCKET socket, char *buffer, int length, int flags)
{
	return winsock_result(posix_socket_recv((int)socket, buffer, length, flags));
}

int WSAAPI halo_ws_recvfrom(SOCKET socket, char *buffer, int length, int flags,
	struct sockaddr *address, int *address_length)
{
	return winsock_result(posix_socket_recvfrom((int)socket, buffer, length, flags, address, address_length));
}

int WSAAPI halo_ws_shutdown(SOCKET socket, int how)
{
	return winsock_result(posix_socket_shutdown((int)socket, how));
}

int WSAAPI halo_ws_ioctlsocket(SOCKET socket, long command, u_long *argument)
{
	switch ((unsigned long)command)
	{
	case (unsigned long)FIONBIO:
		return winsock_result(posix_socket_set_nonblocking((int)socket, *argument != 0));
	case (unsigned long)FIONREAD:
		return winsock_result(posix_socket_bytes_available((int)socket, argument));
	default:
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}
}

int WSAAPI halo_ws_setsockopt(SOCKET socket, int level, int name, const char *value, int length)
{
	return winsock_result(posix_socket_setsockopt((int)socket, level, name, value, length));
}

int WSAAPI halo_ws_getsockopt(SOCKET socket, int level, int name, char *value, int *length)
{
	return winsock_result(posix_socket_getsockopt((int)socket, level, name, value, length));
}

int WSAAPI halo_ws_getsockname(SOCKET socket, struct sockaddr *address, int *address_length)
{
	return winsock_result(posix_socket_getsockname((int)socket, address, address_length));
}

int WSAAPI halo_ws_getpeername(SOCKET socket, struct sockaddr *address, int *address_length)
{
	return winsock_result(posix_socket_getpeername((int)socket, address, address_length));
}

/* ---------- select and fd_set */

static int descriptors_from_set(halo_ws_fd_set *set, int *descriptors)
{
	u_int index;

	for (index = 0; index < set->fd_count && index < FD_SETSIZE; index++)
		descriptors[index] = (int)set->fd_array[index];
	return (int)index;
}

static void set_from_descriptors(halo_ws_fd_set *set, const int *descriptors, int count)
{
	int index;

	for (index = 0; index < count; index++)
		set->fd_array[index] = (SOCKET)descriptors[index];
	set->fd_count = (u_int)count;
}

int WSAAPI halo_ws_select(int descriptor_count, halo_ws_fd_set *read_set, halo_ws_fd_set *write_set,
	halo_ws_fd_set *error_set, const struct halo_ws_timeval *timeout)
{
	int read[FD_SETSIZE], write[FD_SETSIZE], error[FD_SETSIZE];
	int read_count = read_set ? descriptors_from_set(read_set, read) : 0;
	int write_count = write_set ? descriptors_from_set(write_set, write) : 0;
	int error_count = error_set ? descriptors_from_set(error_set, error) : 0;
	int result;

	(void)descriptor_count;
	result = posix_socket_select(
		read_set ? read : NULL, &read_count,
		write_set ? write : NULL, &write_count,
		error_set ? error : NULL, &error_count,
		timeout ? timeout->tv_sec : 0, timeout ? timeout->tv_usec : 0, timeout == NULL);
	if (result < 0)
		return winsock_result(result);
	if (read_set)
		set_from_descriptors(read_set, read, read_count);
	if (write_set)
		set_from_descriptors(write_set, write, write_count);
	if (error_set)
		set_from_descriptors(error_set, error, error_count);
	return result;
}

int PASCAL __WSAFDIsSet(SOCKET socket, halo_ws_fd_set *set)
{
	u_int index;

	for (index = 0; index < set->fd_count; index++)
	{
		if (set->fd_array[index] == socket)
			return 1;
	}
	return 0;
}

/* ---------- byte order and addresses */

u_long WSAAPI halo_ws_htonl(u_long value)
{
	return __builtin_bswap32(value);
}

u_long WSAAPI halo_ws_ntohl(u_long value)
{
	return __builtin_bswap32(value);
}

u_short WSAAPI halo_ws_htons(u_short value)
{
	return (u_short)((value << 8) | (value >> 8));
}

u_short WSAAPI halo_ws_ntohs(u_short value)
{
	return (u_short)((value << 8) | (value >> 8));
}

unsigned long WSAAPI halo_ws_inet_addr(const char *text)
{
	unsigned long parts[4];
	int count = 0;

	while (count < 4)
	{
		unsigned long value = 0;
		int digits = 0;

		while (*text >= '0' && *text <= '9')
		{
			value = value * 10 + (unsigned long)(*text++ - '0');
			digits++;
		}
		if (!digits || value > 255)
			return INADDR_NONE;
		parts[count++] = value;
		if (*text != '.')
			break;
		text++;
	}
	if (count != 4 || *text)
		return INADDR_NONE;
	/* network byte order on a little-endian host */
	return parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
}

/* ---------- XNet */

INT WSAAPI XNetStartup(const XNetStartupParams *parameters)
{
	(void)parameters;
	return 0;
}

INT WSAAPI XNetCleanup(void)
{
	return 0;
}

INT WSAAPI XNetRandom(BYTE *buffer, UINT size)
{
	posix_random_bytes(buffer, size);
	return 0;
}

INT WSAAPI XNetCreateKey(XNKID *key_identifier, XNKEY *key)
{
	posix_random_bytes(key_identifier, sizeof(*key_identifier));
	posix_random_bytes(key, sizeof(*key));
	return 0;
}

INT WSAAPI XNetRegisterKey(const XNKID *key_identifier, const XNKEY *key)
{
	(void)key_identifier;
	(void)key;
	return 0;
}

INT WSAAPI XNetUnregisterKey(const XNKID *key_identifier)
{
	(void)key_identifier;
	return 0;
}

INT WSAAPI XNetXnAddrToInAddr(const XNADDR *address, const XNKID *key_identifier, IN_ADDR *result)
{
	(void)key_identifier;
	*result = address->ina;
	return 0;
}

DWORD WSAAPI XNetGetTitleXnAddr(XNADDR *address)
{
	unsigned long ip = posix_local_ipv4_address();

	memset(address, 0, sizeof(*address));
	address->bSizeOfStruct = sizeof(*address);
	address->ina.s_addr = ip;
	return ip ? (XNET_GET_XNADDR_ETHERNET | XNET_GET_XNADDR_DHCP) : XNET_GET_XNADDR_ETHERNET;
}

DWORD WSAAPI XNetGetEthernetLinkStatus(void)
{
	return posix_local_ipv4_address() ?
		(XNET_ETHERNET_LINK_ACTIVE | XNET_ETHERNET_LINK_100MBPS | XNET_ETHERNET_LINK_FULL_DUPLEX) : 0;
}
