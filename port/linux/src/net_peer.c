/*
NET_PEER.C

Internet play's hooks into the socket layer. See net_peer.h for why these
are not in xnet.c any more.

Nothing here includes a socket header, and an address is an unsigned long
in network byte order, so this compiles the same on either side.
*/

#include "net_peer.h"

#include "net_fault.h"
#include "net_settings.h"
#include "p2p.h"

void net_peer_start(void)
{
	unsigned long local;

	/* in this order, and before the game's network threads start */
	net_settings_read();
	net_fault_enabled();
	p2p_initialize(net_settings_local_address(&local) ? local : net_settings_loopback_address());
}

void net_peer_socket_bound(int socket, int stream, int listening,
	unsigned long bound_address, unsigned short bound_port)
{
	if (!bound_port)
		return;
	/* (one bound to this machine alone, as the telnet console's, is not for
	peers to reach, and is not hosting; its port is still no stand-in's) */
	if (bound_address == net_settings_loopback_address())
		p2p_port_taken(stream, bound_port);
	else
		p2p_socket_port(socket, stream, listening, bound_port);
}

void net_peer_socket_closing(int socket, int datagram_port_or_zero)
{
	p2p_socket_closed(socket, (unsigned short)datagram_port_or_zero);
	net_fault_socket_closed(socket);
}

int net_peer_outgoing(int stream, int connecting, unsigned long *address, unsigned short *port)
{
	return p2p_outgoing(stream, connecting, address, port);
}

int net_peer_incoming(int stream, unsigned long *address, unsigned short *port)
{
	return p2p_incoming(stream, address, port);
}

int net_peer_is_peer(unsigned long address)
{
	/* 100.64/10, which internet play hands out for peers' stand-ins.
	xnet.c asks this of the host-order value after ntohl, which is right
	everywhere it is compiled; there is no ntohl here, because that would
	mean a socket header. The address is in network byte order, so its
	bytes are the dotted quad in order whatever this machine's own order
	is - the first 100 and the second beginning 01 - which is the same
	answer without one. */
	const unsigned char *bytes = (const unsigned char *)&address;

	return bytes[0] == 100 && (bytes[1] & 0xC0) == 0x40;
}

int net_peer_send(unsigned short source_port, unsigned long address, unsigned short port,
	const void *data, int size)
{
	return p2p_send_datagram(source_port, address, port, data, size);
}

int net_peer_broadcast(unsigned short source_port, unsigned short port, const void *data, int size)
{
	return p2p_broadcast_datagram(source_port, port, data, size);
}

int net_peer_broadcast_targets(unsigned short port, unsigned long *addresses, unsigned short *ports,
	int maximum_count)
{
	return p2p_broadcast_targets(port, addresses, ports, maximum_count);
}

const unsigned char *net_peer_identifier(void)
{
	return p2p_identifier();
}

int net_peer_address(const unsigned char *identifier, unsigned long *address)
{
	return p2p_peer_address(identifier, address);
}
