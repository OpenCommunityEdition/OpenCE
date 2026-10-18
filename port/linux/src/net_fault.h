/*
NET_FAULT.H

debug.network_latency, debug.network_loss, debug.network_corrupt,
debug.network_corrupt_stream and debug.network_corrupt_after: what this
machine receives is held back, lost and damaged, to test the netcode as
over the internet and to test that nothing another machine sends can crash
the game (AGENTS.md, "Networking": everything another machine sends is
untrusted).

This was port/linux/src/xnet.c's alone, which the Mac cannot compile, so
the part that is not a socket call lives here: the settings, the queue,
the random numbers and the damage. A platform keeps its own socket reads
and asks this what to do with them, which is the only part that differs.

Nothing here includes a socket header, so it compiles the same beside
glibc's Winsock namespace (xnet.c) and beside the Mac's privately renamed
Darwin one (port/mac/engine_sockets.c). An address is carried as opaque
bytes for the same reason.

The caller reads into a buffer of net_fault_packet_size() bytes, hands
each read to net_fault_keep, and then asks net_fault_take for whatever is
due. Times are the caller's own millisecond clock, whatever that is.
*/

#ifndef __HALO_NET_FAULT_H
#define __HALO_NET_FAULT_H

/* Whether any of the settings asks for a fault, read once on the first
call. Call it before the game's network threads start, as nothing here
locks the settings. */
int net_fault_enabled(void);

/* The bytes a caller must be able to read into: a held datagram is kept
whole, and one may be stretched to this. */
#define NET_FAULT_PACKET_SIZE 1500

/* The socket closes: what it held goes, so that a socket later given its
number does not inherit it. */
void net_fault_socket_closed(int socket);

/* Whether something held for the socket is due, which a select must treat
as the socket being readable. */
int net_fault_due(int socket, unsigned long now);

/* Hold what was just read from the socket. Returns 0 if it was lost
(debug.network_loss) or there was no room, 1 if it is held. A stream's end
(a read of 0) is the caller's to return at once and must not come here. */
int net_fault_keep(int socket, const void *data, int length,
	const void *address, int address_length, int datagram, unsigned long now);

/* The oldest of the socket's that has waited long enough: its bytes, or -1
when nothing is due, which the caller reports as would-block. *truncated is
set when a datagram did not fit, which the caller reports as WSAEMSGSIZE. */
int net_fault_take(int socket, void *buffer, int length,
	void *address, int *address_length, int datagram, unsigned long now,
	int *truncated);

#endif
