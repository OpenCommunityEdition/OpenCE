/*
NET_PEER.H

Internet play's hooks into the socket layer: the part that is not a socket
call.

port/linux/src/xnet.c is the Winsock layer for Linux, Windows, Android and
the web, and every one of internet play's entry points used to live in it.
The Mac has its own Winsock layer (port/mac/engine_sockets.c) and does not
compile xnet.c, so it had none of them: the whole DTLS tunnel and UPnP were
linked into the Mac build and then dead-stripped out of it again, because
nothing on that side ever called in.

This is the shape net_fault.c already has, for the same reason. What the
two layers share is the deciding - which port to tell internet play about,
whether an address is a peer's, what to do with a broadcast - and what they
do not share is the socket calls themselves. The deciding is here.

Nothing here includes a socket header, so it compiles the same beside
glibc's Winsock namespace (xnet.c) and beside the Mac's privately renamed
Darwin one (engine_sockets.c). An address is an unsigned long in network
byte order and a port an unsigned short in it, which is what p2p.h already
takes; a caller that holds a sockaddr unwraps it on its own side, where the
structure's name means something.

A caller passes the socket's type rather than this asking for it: a socket
option needs a socket header's constants, and each layer already knows what
it made.
*/

#ifndef __HALO_NET_PEER_H
#define __HALO_NET_PEER_H

/* WSAStartup's: the settings, the fault simulation and internet play, in
that order and before the game's network threads start. Safe to call
twice; internet play starts once. */
void net_peer_start(void);

/* The socket has a local port now - bound, given one by a connection or a
send, or listening. stream is nonzero for SOCK_STREAM. bound_address and
bound_port are what getsockname gave, in network byte order. */
void net_peer_socket_bound(int socket, int stream, int listening,
	unsigned long bound_address, unsigned short bound_port);

/* The socket is closing. datagram_port is its local port for a datagram
socket, 0 for a stream one, as p2p_socket_closed wants it. */
void net_peer_socket_closing(int socket, int datagram_port_or_zero);

/* Where a connection or datagram is going. Returns 1 when address and port
have been rewritten for a peer, 0 when they are not a peer's and stand as
they were, and -1 when the peer cannot be reached, which the caller reports
as WSAEHOSTUNREACH. connecting is the socket for a UDP connect, else -1. */
int net_peer_outgoing(int stream, int connecting, unsigned long *address, unsigned short *port);

/* Where one came from: nonzero when address and port have been rewritten to
the peer they are really from, zero when the caller's own translation
applies instead. */
int net_peer_incoming(int stream, unsigned long *address, unsigned short *port);

/* Whether an address is an internet play peer's stand-in (the 100.64/10
range internet play hands out), which a send must put on the tunnel rather
than on the wire. */
int net_peer_is_peer(unsigned long address);

/* A datagram to a peer: 1 sent on the tunnel, 0 not sent (send it
yourself), -1 the peer cannot be reached. */
int net_peer_send(unsigned short source_port, unsigned long address, unsigned short port,
	const void *data, int size);

/* A broadcast, onto the tunnel: how many peers took it. */
int net_peer_broadcast(unsigned short source_port, unsigned short port, const void *data, int size);

/* The peers a broadcast must also be sent to as ordinary datagrams, when
the socket has no port of its own yet: how many were filled in. */
int net_peer_broadcast_targets(unsigned short port, unsigned long *addresses, unsigned short *ports,
	int maximum_count);
/* what net_peer_broadcast_targets will fill at most */
#define NET_PEER_BROADCAST_PEERS 127

/* This machine's identifier, the six bytes an XNADDR carries. */
const unsigned char *net_peer_identifier(void);

/* The address a peer's identifier stands for: nonzero when it is a peer's
and *address is it. */
int net_peer_address(const unsigned char *identifier, unsigned long *address);

#endif
