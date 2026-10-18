/*
NET_SETTINGS.H

network.address and network.broadcast (port_config.c), read once and kept
in network byte order.

port/linux/src/xnet.c has had this since system link was written, but its
copy swaps whatever the host is, so it is right only for the little-endian
builds that compile it. The Mac is also PowerPC and has its own socket
adapter (port/mac/engine_sockets.c), which cannot compile xnet.c: Apple's
GCC 4.0 has no __thread, and the XDK's Winsock names would collide with
Darwin's. So the settings live here, in no platform's socket headers, and
both sides read the same ones the same way.

Addresses are IPv4 in network byte order, as a sockaddr_in carries them.
*/

#ifndef __HALO_NET_SETTINGS_H
#define __HALO_NET_SETTINGS_H

enum
{
	/* the addresses network.broadcast may name */
	NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS = 256
};

/* read the settings if they have not been read (the first call logs what it
refuses); every call below does this first, so nothing else need call it */
void net_settings_read(void);

/* this machine's address, if network.address names a usable one: the address
to bind to in place of INADDR_ANY, and the one a destination of 127.0.0.1
means. 0 if it does not. */
int net_settings_local_address(unsigned long *address);

/* the addresses network.broadcast names, to send to in place of a real
broadcast (255.255.255.255 among them sends a real one too); their count,
which is 0 when the setting is empty */
int net_settings_broadcast_targets(unsigned long *targets, int maximum_count);

/* 127.0.0.1, in network byte order */
unsigned long net_settings_loopback_address(void);

/* a dotted quad of decimal numbers up to 255 filling [text, end), spaces
around it allowed; the address in network byte order. 0 if it is not one. */
int net_settings_parse_ipv4(const char *text, const char *end, unsigned long *address);

#endif
